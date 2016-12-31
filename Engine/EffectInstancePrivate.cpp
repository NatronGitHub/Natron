/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
 *
 * Natron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Natron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "EffectInstancePrivate.h"

#include <cassert>
#include <stdexcept>

#include "Engine/AppInstance.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/TreeRender.h"
#include "Engine/TreeRenderNodeArgs.h"
#include "Engine/ViewIdx.h"


NATRON_NAMESPACE_ENTER;


EffectInstance::Implementation::Implementation(EffectInstance* publicInterface)
: _publicInterface(publicInterface)
, attachedContextsMutex(QMutex::Recursive)
, attachedContexts()
, mainInstance()
, isDoingInstanceSafeRender(false)
, renderClonesMutex()
, renderClonesPool()
, tlsData(new TLSHolder<EffectInstanceTLSData>())
{
}

EffectInstance::Implementation::Implementation(const Implementation& other)
: _publicInterface()
, attachedContextsMutex(QMutex::Recursive)
, attachedContexts()
, mainInstance(other._publicInterface->shared_from_this())
, isDoingInstanceSafeRender(false)
, renderClonesMutex()
, renderClonesPool()
, tlsData(other.tlsData)
{

}


RenderRoIRetCode
EffectInstance::Implementation::resolveRenderBackend(const RenderRoIArgs & args,
                                                     const FrameViewRequestPtr& requestPassData,
                                                     const RectI& roi,
                                                     RenderBackendTypeEnum* renderBackend,
                                                     OSGLContextPtr *glRenderContext)
{
    // Default to CPU
    *renderBackend = eRenderBackendTypeCPU;

    TreeRenderPtr render = args.renderArgs->getParentRender();
    OSGLContextPtr glGpuContext = render->getGPUOpenGLContext();
    OSGLContextPtr glCpuContext = render->getCPUOpenGLContext();

    bool canDoOpenGLRendering = (args.renderArgs->getCurrentRenderOpenGLSupport() == ePluginOpenGLRenderSupportNeeded ||
                                 args.renderArgs->getCurrentRenderOpenGLSupport() == ePluginOpenGLRenderSupportYes)
    && args.allowGPURendering && glGpuContext;

    if (canDoOpenGLRendering) {

        // Enable GPU render if the plug-in cannot render another way or if all conditions are met
        if (args.renderArgs->getCurrentRenderOpenGLSupport() == ePluginOpenGLRenderSupportNeeded && !_publicInterface->getNode()->getPlugin()->isOpenGLEnabled()) {

            QString message = tr("OpenGL render is required for  %1 but was disabled in the Preferences for this plug-in, please enable it and restart %2").arg(QString::fromUtf8(_publicInterface->getNode()->getLabel().c_str())).arg(QString::fromUtf8(NATRON_APPLICATION_NAME));
            _publicInterface->setPersistentMessage(eMessageTypeError, message.toStdString());
            return eRenderRoIRetCodeFailed;
        }


        *renderBackend = eRenderBackendTypeOpenGL;

        // If the plug-in knows how to render on CPU, check if we should actually render on CPU instead.
        if (args.renderArgs->getCurrentRenderOpenGLSupport() == ePluginOpenGLRenderSupportYes) {

            // User want to force caching of this node but we cannot cache OpenGL renders, so fallback on CPU.
            if ( _publicInterface->getNode()->isForceCachingEnabled() ) {
                *renderBackend = eRenderBackendTypeCPU;
            }

            // If this image is requested multiple times or it is a unknown frame, do not render it on OpenGL since we do not use the cache.
            if (*renderBackend == eRenderBackendTypeOpenGL) {
                if (requestPassData->getFramesNeededVisitsCount() > 1 || args.type == eRenderRoITypeUnknownFrame) {
                    *renderBackend = eRenderBackendTypeCPU;
                }
            }

            // Ensure that the texture will be at least smaller than the maximum OpenGL texture size
            if (*renderBackend == eRenderBackendTypeOpenGL) {
                if ( (roi.width() >= glGpuContext->getMaxOpenGLWidth()) ||
                    ( roi.height() >= glGpuContext->getMaxOpenGLHeight()) ) {
                    // Fallback on CPU rendering since the image is larger than the maximum allowed OpenGL texture size
                    *renderBackend = eRenderBackendTypeCPU;
                }
            }
        }

    }


    if (*renderBackend == eRenderBackendTypeOpenGL) {
        *glRenderContext = glGpuContext;
    } else {

        // If implementation of the render is OpenGL but it can support OSMesa, fallback on OSMesa
        bool supportsOSMesa = _publicInterface->canCPUImplementationSupportOSMesa() && glCpuContext;
        if (supportsOSMesa) {
            if ( (roi.width() < (*glRenderContext)->getMaxOpenGLWidth()) &&
                ( roi.height() < (*glRenderContext)->getMaxOpenGLHeight()) ) {
                *glRenderContext = glCpuContext;
                *renderBackend = eRenderBackendTypeOSMesa;
            }
        }
    }

    return eRenderRoIRetCodeOk;
} // resolveRenderBackend

CacheAccessModeEnum
EffectInstance::Implementation::shouldRenderUseCache(const RenderRoIArgs & args,
                                                     const FrameViewRequestPtr& requestPassData,
                                                     RenderBackendTypeEnum backend)
{
    bool retSet = false;
    CacheAccessModeEnum ret = eCacheAccessModeNone;

    // Do not use the cache for OpenGL on GPU: there is no swap
    if (backend == eRenderBackendTypeOpenGL) {
        retSet = true;
        ret = eCacheAccessModeNone;
    }

    // A writer never caches!
    if (!retSet && _publicInterface->isWriter()) {
        retSet = true;
        ret = eCacheAccessModeNone;
    }


    if (!retSet) {
        NodePtr treeRoot = args.renderArgs->getParentRender()->getTreeRoot();
        if (treeRoot->getEffectInstance().get() == _publicInterface)  {
            // Always cache the root node because a subsequent render may ask for it
            ret = eCacheAccessModeReadWrite;
            retSet = true;
        }
    }

    if (!retSet) {
        const bool isFrameVaryingOrAnimated = _publicInterface->isFrameVarying(args.renderArgs);
        const int requestsCount = requestPassData->getFramesNeededVisitsCount();

        bool useCache = _publicInterface->shouldCacheOutput(isFrameVaryingOrAnimated, args.time, args.view, requestsCount);
        if (useCache) {
            ret = eCacheAccessModeReadWrite;
        } else {
            ret = eCacheAccessModeNone;
        }
        retSet = true;
    }

    assert(retSet);
    if (ret == eCacheAccessModeReadWrite) {
        // If true, we don't read the cache but just write to it. Subsequent renders of this frame/view for the same
        // render will still be able to read/write. This is atomic, the first thread encountering it will turn it off.
        bool cacheWriteOnly = requestPassData->checkIfByPassCacheEnabledAndTurnoff();
        if (cacheWriteOnly) {
            ret = eCacheAccessModeWriteOnly;
        }
    }
    return ret;
} // shouldRenderUseCache

NATRON_NAMESPACE_EXIT;


