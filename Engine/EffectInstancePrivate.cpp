/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2020 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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
#include <bitset>
#include <QDebug>

#include "Engine/AppInstance.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/OSGLContext.h"
#include "Engine/OSGLFunctions.h"
#include "Engine/PluginMemory.h"
#include "Engine/Timer.h"
#include "Engine/Plugin.h"
#include "Engine/Project.h"
#include "Engine/TreeRender.h"
#include "Engine/RenderStats.h"
#include "Engine/RotoStrokeItem.h"
#include "Engine/ViewIdx.h"

#define kNatronPersistentWarningCheckForNan "NatronPersistentWarningCheckForNan"

NATRON_NAMESPACE_ENTER


EffectInstance::Implementation::Implementation(EffectInstance* publicInterface)
: _publicInterface(publicInterface)
, common(new EffectInstanceCommonData)
, renderData()
, defKnobs(new DefaultKnobs)
, renderKnobs()
, pluginMemoryChunks()
{

}

EffectInstance::Implementation::Implementation(EffectInstance* publicInterface, const Implementation& other)
: _publicInterface(publicInterface)
, common(other.common)
, renderData(new RenderCloneData)
, defKnobs(other.defKnobs)
, renderKnobs()
, pluginMemoryChunks()
{

}

EffectInstance::Implementation::~Implementation()
{
    // Ensure all plug-in created memory is freed
    if (!pluginMemoryChunks.empty()) {
        qDebug() << _publicInterface->getNode()->getScriptName_mt_safe().c_str() << " is leaking image memory it has allocated";
    }
    for (std::list<PluginMemoryPtr>::iterator it = pluginMemoryChunks.begin(); it != pluginMemoryChunks.end(); ++it) {
        (*it)->deallocateMemory();
    }
}


void
EffectInstance::Implementation::setFrameRangeResults(const GetFrameRangeResultsPtr& range)
{
    if (!renderData) {
        return ;
    }
    QMutexLocker k(&renderData->lock);
    renderData->frameRangeResults = range;
}

GetFrameRangeResultsPtr
EffectInstance::Implementation::getFrameRangeResults() const
{
    if (!renderData) {
        return GetFrameRangeResultsPtr();
    }
    QMutexLocker k(&renderData->lock);
    return renderData->frameRangeResults;
}

void
EffectInstance::Implementation::setTimeInvariantMetadataResults(const GetTimeInvariantMetadataResultsPtr& metadata)
{
    if (!renderData) {
        return;
    }
    QMutexLocker k(&renderData->lock);
    renderData->metadataResults = metadata;
}


GetTimeInvariantMetadataResultsPtr
EffectInstance::Implementation::getTimeInvariantMetadataResults() const
{
    if (!renderData) {
        return GetTimeInvariantMetadataResultsPtr();
    }
    QMutexLocker k(&renderData->lock);
    return renderData->metadataResults;
}


RenderScale
EffectInstance::getCombinedScale(unsigned int mipMapLevel, const RenderScale& proxyScale)
{
    RenderScale ret = proxyScale;
    double mipMapScale = Image::getScaleFromMipMapLevel(mipMapLevel);
    ret.x *= mipMapScale;
    ret.y *= mipMapScale;
    return ret;
}

ActionRetCodeEnum
EffectInstance::Implementation::resolveRenderBackend(const TreeRenderExecutionDataPtr& requestPassSharedData, const FrameViewRequestPtr& requestPassData, const RectI& roi, CacheAccessModeEnum *cachePolicy, RenderBackendTypeEnum* renderBackend)
{
    // Default to CPU
    *renderBackend = eRenderBackendTypeCPU;

    TreeRenderPtr render = _publicInterface->getCurrentRender();
    OSGLContextPtr glGpuContext = render->getGPUOpenGLContext();
    OSGLContextPtr glCpuContext = render->getCPUOpenGLContext();


    PluginOpenGLRenderSupport openGLSupport = _publicInterface->getOpenGLRenderSupport();
    const bool supportsOSMesa = _publicInterface->canCPUImplementationSupportOSMesa();

    if ((openGLSupport == ePluginOpenGLRenderSupportNeeded || openGLSupport == ePluginOpenGLRenderSupportYes) && glGpuContext) {

        *renderBackend = eRenderBackendTypeOpenGL;

        // If the plug-in knows how to render on CPU, check if we should actually render on CPU instead.
        if (openGLSupport == ePluginOpenGLRenderSupportNeeded) {
            // We do not want to cache OpenGL renders
            if (*cachePolicy != eCacheAccessModeNone) {
                *cachePolicy = eCacheAccessModeNone;
            }
        } else if (openGLSupport == ePluginOpenGLRenderSupportYes) {

            // We do not want to cache OpenGL renders, so fallback on CPU.
            if (*cachePolicy != eCacheAccessModeNone) {
                *renderBackend = eRenderBackendTypeCPU;
            }

            // If this image is requested multiple times , do not render it on OpenGL since we do not use the cache.
            if (*renderBackend == eRenderBackendTypeOpenGL) {
                if (requestPassData->getNumListeners(requestPassSharedData) > 1) {
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

    } // canDoOpenGLRendering

    if (*renderBackend != eRenderBackendTypeOpenGL) {

        // If implementation of the render is OpenGL but it can support OSMesa, fallback on OSMesa
        if (supportsOSMesa && glCpuContext) {
            if ( (roi.width() < (glCpuContext)->getMaxOpenGLWidth()) &&
                ( roi.height() < (glCpuContext)->getMaxOpenGLHeight()) ) {
                *renderBackend = eRenderBackendTypeOSMesa;
            }
        }
    }

    if (openGLSupport == ePluginOpenGLRenderSupportNeeded && *renderBackend != eRenderBackendTypeOpenGL && *renderBackend != eRenderBackendTypeOSMesa) {
        QString message = tr("OpenGL render is required for %1 but could not be launched. This may be because it was disabled in the Preferences or because your hadware does not meet the requirements.").arg(QString::fromUtf8(_publicInterface->getNode()->getLabel_mt_safe().c_str()));
        _publicInterface->getNode()->setPersistentMessage(eMessageTypeError, kNatronPersistentErrorGenericRenderMessage, message.toStdString());
        return eActionStatusFailed;
    }

    return eActionStatusOK;
} // resolveRenderBackend

CacheAccessModeEnum
EffectInstance::Implementation::shouldRenderUseCache(const TreeRenderExecutionDataPtr& requestPassSharedData, const FrameViewRequestPtr& requestPassData)
{
    bool retSet = false;
    CacheAccessModeEnum ret = eCacheAccessModeNone;

    // A writer never caches!
    if (!retSet && _publicInterface->isWriter()) {
        retSet = true;
        ret = eCacheAccessModeNone;
    }

    if (!retSet) {
        TreeRenderPtr render = _publicInterface->getCurrentRender();

        // Always cache the root node because a subsequent render may ask for it
        EffectInstancePtr treeRoot = render->getOriginalTreeRoot();

        // Always cache an image if the color picker requested it
        const bool imageRequestedForColorPicker = render->isExtraResultsRequestedForNode(_publicInterface->getNode());
        if (treeRoot == _publicInterface->getNode()->getEffectInstance() || imageRequestedForColorPicker)  {
            ret = eCacheAccessModeReadWrite;
            retSet = true;
        }
    }

    if (!retSet) {
        const bool isFrameVaryingOrAnimated = _publicInterface->isFrameVarying() || _publicInterface->getHasAnimation();
        const int requestsCount = requestPassData->getNumListeners(requestPassSharedData);

        bool useCache = _publicInterface->shouldCacheOutput(isFrameVaryingOrAnimated,  requestsCount);
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

ActionRetCodeEnum
EffectInstance::Implementation::launchIsolatedRender(const FrameViewRequestPtr& requestPassData)
{
    // We are not during a render, create one.
    TreeRender::CtorArgsPtr rargs(new TreeRender::CtorArgs());
    rargs->provider = _publicInterface->getThisTreeRenderQueueProviderShared();
    rargs->time = _publicInterface->getCurrentRenderTime();
    rargs->view = _publicInterface->getCurrentRenderView();
    assert(_publicInterface->isRenderClone());
    rargs->treeRootEffect = toEffectInstance(_publicInterface->getMainInstance());
    rargs->canonicalRoI = requestPassData->getCurrentRoI();
    rargs->proxyScale = requestPassData->getProxyScale();
    rargs->mipMapLevel = requestPassData->getMipMapLevel();
    rargs->plane = requestPassData->getPlaneDesc();
    rargs->draftMode = requestPassData->getParentRender()->isDraftRender();
    rargs->playback = requestPassData->getParentRender()->isPlayback();
    rargs->byPassCache = false;
    TreeRenderPtr renderObject = TreeRender::create(rargs);
    _publicInterface->launchRender(renderObject);
    ActionRetCodeEnum status = _publicInterface->waitForRenderFinished(renderObject);
    if (isFailureRetCode(status)) {
        return status;
    }
    FrameViewRequestPtr outputRequest = renderObject->getOutputRequest();
    assert(outputRequest);
    requestPassData->setFullscaleImagePlane(outputRequest->getFullscaleImagePlane());
    requestPassData->setRequestedScaleImagePlane(outputRequest->getRequestedScaleImagePlane());
    requestPassData->initStatus(FrameViewRequest::eFrameViewRequestStatusRendered);
    return eActionStatusOK;
} // launchIsolatedRender

ActionRetCodeEnum
EffectInstance::Implementation::tiledRenderingFunctor(const RectToRender & rectToRender,
                                                      const TiledRenderingFunctorArgs& args)
{


    TreeRenderPtr render = _publicInterface->getCurrentRender();

    // Record the time spend to render for this frame/view for this thread
    TimeLapsePtr timeRecorder;
    RenderStatsPtr stats = render->getStatsObject();
    if (stats && stats->isInDepthProfilingEnabled()) {
        timeRecorder.reset(new TimeLapse);
    }

    // If using OpenGL, bind the frame buffer
    if (args.backendType == eRenderBackendTypeOpenGL) {

        assert(args.glContext);

        GLuint fboID = args.glContext->getOrCreateFBOId();
        GL_GPU::BindFramebuffer(GL_FRAMEBUFFER, fboID);
        glCheckError(GL_GPU);
    }


    // If this tile is identity, copy input image instead
    ActionRetCodeEnum stat;
    /*qDebug() << QThread::currentThread() << (U64)render.get() <<  _publicInterface << _publicInterface->getScriptName_mt_safe().c_str() << "render" << (double)_publicInterface->getCurrentRenderTime() << args.cachedPlanes.begin()->first.getPlaneLabel().c_str() << rectToRender.rect.x1 << rectToRender.rect.y1 << rectToRender.rect.x2 << rectToRender.rect.y2 << "(identity ? " << (rectToRender.identityInputNumber !=-1)<< ")" ;*/

    if (_publicInterface->isAccumulationEnabled()) {
        render->setOrUnionActiveStrokeUpdateArea(rectToRender.rect);
    }

    if (rectToRender.identityInputNumber != -1) {
        stat = renderHandlerIdentity(rectToRender, args);
    } else {
        stat = renderHandlerPlugin(rectToRender, args);
        if (isFailureRetCode(stat)) {
            return stat;
        }
        _publicInterface->getNode()->clearPersistentMessage(kNatronPersistentErrorGenericRenderMessage);

        // Apply post-processing
        stat = renderHandlerPostProcess(rectToRender, args);
        if (isFailureRetCode(stat)) {
            return stat;
        }
    }


    if (timeRecorder) {
        stats->addRenderInfosForNode(_publicInterface->getNode(), timeRecorder->getTimeSinceCreation());
    }
    return render->isRenderAborted() ? eActionStatusAborted : eActionStatusOK;
} // tiledRenderingFunctor


ActionRetCodeEnum
EffectInstance::Implementation::renderHandlerIdentity(const RectToRender & rectToRender,
                                                      const TiledRenderingFunctorArgs& args)
{

    TreeRenderPtr render = _publicInterface->getCurrentRender();
    const bool checkNaNs = render->isNaNHandlingEnabled();

    for (std::map<ImagePlaneDesc, ImagePtr>::const_iterator it = args.cachedPlanes.begin(); it != args.cachedPlanes.end(); ++it) {
        IdentityPlaneKey p;
        p.identityInputNb = rectToRender.identityInputNumber;
        p.identityPlane = it->first;
        p.identityTime = rectToRender.identityTime;
        p.identityView = rectToRender.identityView;

        RectI roi;
        rectToRender.rect.intersect(it->second->getBounds(), &roi);


        IdentityPlanesMap::const_iterator foundFetchedPlane = args.identityPlanes.find(p);

        if (foundFetchedPlane == args.identityPlanes.end()) {
            ActionRetCodeEnum stat = it->second->fillZero(roi);
            if (isFailureRetCode(stat)) {
                return stat;
            }
        } else {
            Image::CopyPixelsArgs cpyArgs;
            cpyArgs.roi = roi;
            ActionRetCodeEnum stat = it->second->copyPixels(*foundFetchedPlane->second, cpyArgs);
            if (isFailureRetCode(stat)) {
                return stat;
            }
        }

        if (checkNaNs) {

            bool foundNan = false;
            ActionRetCodeEnum stat = it->second->checkForNaNs(roi, &foundNan);
            if (isFailureRetCode(stat)) {
                return stat;
            }
            if (!foundNan) {
                _publicInterface->getNode()->clearPersistentMessage(kNatronPersistentWarningCheckForNan);
            } else {
                QString warning;
                warning.append( tr("NaN values detected in (") );
                warning.append( QString::number(roi.x1) );
                warning.append( QChar::fromLatin1(',') );
                warning.append( QString::number(roi.y1) );
                warning.append( QString::fromUtf8(")-(") );
                warning.append( QString::number(roi.x2) );
                warning.append( QChar::fromLatin1(',') );
                warning.append( QString::number(roi.y2) );
                warning.append( QString::fromUtf8("). ") );
                warning.append( tr("They have been converted to 1") );
                _publicInterface->getNode()->setPersistentMessage( eMessageTypeWarning, kNatronPersistentWarningCheckForNan, warning.toStdString() );
            }
        } // checkNaNs

    }

    return render->isRenderAborted() ? eActionStatusAborted : eActionStatusOK;
} // renderHandlerIdentity

template <typename GL>
static void setupGLForRender(const ImagePtr& image,
                             const OSGLContextPtr& glContext,
                             const RectI& roi,
                             bool callGLFinish,
                             OSGLContextAttacherPtr *glContextAttacher)
{

    RectI imageBounds = image->getBounds();
    RectI viewportBounds;
    if (GL::isGPU()) {

        GLImageStoragePtr glTexture = image->getGLImageStorage();
        assert(glTexture);
        assert(glTexture->getOpenGLContext() == glContext);

        viewportBounds = imageBounds;
        int textureTarget = glTexture->getGLTextureTarget();
        GL::Enable(textureTarget);
        assert(image->getStorageMode() == eStorageModeGLTex);

        GL::ActiveTexture(GL_TEXTURE0);
        GL::BindTexture( textureTarget, glTexture->getGLTextureID() );
        assert(GL::IsTexture(glTexture->getGLTextureID()));
        assert(GL::GetError() == GL_NO_ERROR);
        glCheckError(GL);
        GL::FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, textureTarget, glTexture->getGLTextureID(), 0 /*LoD*/);
        glCheckError(GL);
        assert(GL::CheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
        glCheckFramebufferError(GL);

    } else {

        viewportBounds = roi;
        assert(image->getStorageMode() == eStorageModeRAM);
        assert(image->getBufferFormat() == eImageBufferLayoutRGBAPackedFullRect);

        Image::CPUData data;
        image->getCPUData(&data);

        float* buffers[4] = {NULL, NULL, NULL, NULL};
        int pixelStride;
        Image::getChannelPointers<float>((const float**)data.ptrs, roi.x1, roi.y1, data.bounds, data.nComps, buffers, &pixelStride);
        assert(buffers[0]);


        // With OSMesa we render directly to the context framebuffer
        *glContextAttacher = OSGLContextAttacher::create(glContext,
                                                         roi.width(),
                                                         roi.height(),
                                                         imageBounds.width(),
                                                         buffers[0]);
        (*glContextAttacher)->attach();
    }

    // setup the output viewport
    OSGLContext::setupGLViewport<GL>(viewportBounds, roi);

    // Enable scissor to make the plug-in not render outside of the viewport...
    GL::Enable(GL_SCISSOR_TEST);
    GL::Scissor( roi.x1 - viewportBounds.x1, roi.y1 - viewportBounds.y1, roi.width(), roi.height() );

    if (callGLFinish) {
        // Ensure that previous asynchronous operations are done (e.g: glTexImage2D) some plug-ins seem to require it (Hitfilm Ignite plugin-s)
        GL::Finish();
    }
} // setupGLForRender

template <typename GL>
static void finishGLRender()
{
    GL::Disable(GL_SCISSOR_TEST);
    GL::ActiveTexture(GL_TEXTURE0);
    GL::BindFramebuffer(GL_FRAMEBUFFER, 0);
    if (!GL::isGPU()) {
        GL::Flush(); // waits until commands are submitted but does not wait for the commands to finish executing
        GL::Finish(); // waits for all previously submitted commands to complete executing
    }
    glCheckError(GL);
}

ActionRetCodeEnum
EffectInstance::Implementation::renderHandlerPlugin(const RectToRender & rectToRender,
                                                    const TiledRenderingFunctorArgs& args)
{

    TreeRenderPtr render = _publicInterface->getCurrentRender();
    RenderActionArgs actionArgs;
    {


        assert(args.requestData->getComponentsResults());
        actionArgs.processChannels = args.requestData->getComponentsResults()->getProcessChannels();
        actionArgs.proxyScale = args.requestData->getProxyScale();
        actionArgs.mipMapLevel = args.requestData->getRenderMappedMipMapLevel();
        actionArgs.backendType = args.backendType;
        actionArgs.roi = rectToRender.rect;
        actionArgs.time = _publicInterface->getCurrentRenderTime();
        actionArgs.view = _publicInterface->getCurrentRenderView();
        actionArgs.glContext = args.glContext;
        actionArgs.glContextData = args.glContextData;
    }

    std::list<std::list<std::pair<ImagePlaneDesc, ImagePtr> > > planesLists;

    // If the plug-in not multi-planar, we must provide images matching the color plane N components
    // specified by the getTimeInvariantMetadata action.
    std::map<ImagePlaneDesc, ImagePtr> nonMultiplanarPlanes;

    bool multiPlanar = _publicInterface->isMultiPlanar();
    // If we can render all planes at once, do it, otherwise just render them all sequentially
    if (!multiPlanar) {
        assert(args.cachedPlanes.size() == 1);

        for (std::map<ImagePlaneDesc, ImagePtr>::const_iterator it = args.cachedPlanes.begin(); it != args.cachedPlanes.end(); ++it) {


            ImagePlaneDesc metadataPlane, pairedPlane;
            _publicInterface->getMetadataComponents(-1, &metadataPlane, &pairedPlane);

            std::pair<ImagePlaneDesc, ImagePtr> p;
            if (metadataPlane == it->first) {
                p = *it;
            } else {
                Image::InitStorageArgs initArgs;
                initArgs.bounds = actionArgs.roi;
                initArgs.renderClone = _publicInterface->shared_from_this();
                initArgs.plane = metadataPlane;
                initArgs.bitdepth = it->second->getBitDepth();
                initArgs.proxyScale = it->second->getProxyScale();
                initArgs.mipMapLevel = it->second->getMipMapLevel();
                initArgs.glContext = args.glContext;
                ImagePtr tmpImage = Image::create(initArgs);
                if (!tmpImage) {
                    return eActionStatusFailed;
                }
                p = std::make_pair(it->first, tmpImage);
            }

            nonMultiplanarPlanes.insert(p);
            std::list<std::pair<ImagePlaneDesc, ImagePtr> > tmp;
            tmp.push_back(p);
            planesLists.push_back(tmp);
        }
    } else {
        nonMultiplanarPlanes = args.cachedPlanes;
        std::list<std::pair<ImagePlaneDesc, ImagePtr> > tmp;
        for (std::map<ImagePlaneDesc, ImagePtr>::const_iterator it = args.cachedPlanes.begin(); it != args.cachedPlanes.end(); ++it) {
            tmp.push_back(*it);
        }
        planesLists.push_back(tmp);
    }

    for (std::list<std::list<std::pair<ImagePlaneDesc, ImagePtr> > >::iterator it = planesLists.begin(); it != planesLists.end(); ++it) {

        actionArgs.outputPlanes = *it;

        OSGLContextAttacherPtr contextAttacher;

        const ImagePtr& mainImagePlane = actionArgs.outputPlanes.front().second;

        // For OSMesa, if the main image plane to render is not a 4 component image, we have to render first in a temporary image, then copy back the results.
        ImagePtr osmesaRenderImage;
        if (args.backendType == eRenderBackendTypeOpenGL ||
            args.backendType == eRenderBackendTypeOSMesa) {

            // Effects that render multiple planes at once are NOT supported by the OpenGL render suite
            // We only bind to the framebuffer color attachment 0 the "main" output image plane
            assert(actionArgs.outputPlanes.size() == 1);
            if (args.glContext->isGPUContext()) {
                setupGLForRender<GL_GPU>(mainImagePlane, args.glContext, actionArgs.roi, _publicInterface->getNode()->isGLFinishRequiredBeforeRender(), &contextAttacher);
            } else {
                osmesaRenderImage = mainImagePlane;
                if (mainImagePlane->getComponentsCount() != 4) {
                    Image::InitStorageArgs initArgs;
                    initArgs.bounds = actionArgs.roi;
                    initArgs.renderClone = _publicInterface->shared_from_this();
                    initArgs.plane = ImagePlaneDesc::getRGBAComponents();
                    initArgs.bitdepth = mainImagePlane->getBitDepth();
                    initArgs.proxyScale = mainImagePlane->getProxyScale();
                    initArgs.mipMapLevel = mainImagePlane->getMipMapLevel();
                    initArgs.glContext = args.glContext;
                    osmesaRenderImage = Image::create(initArgs);
                    if (!osmesaRenderImage) {
                        return eActionStatusFailed;
                    }
                }
                setupGLForRender<GL_CPU>(osmesaRenderImage, args.glContext, actionArgs.roi, _publicInterface->getNode()->isGLFinishRequiredBeforeRender(), &contextAttacher);
            }
        }
        ActionRetCodeEnum stat = _publicInterface->render_public(actionArgs);

        // Ensure that the plug-in return status is eActionStatusAborted if the render is aborted and not failed.
        if (isFailureRetCode(stat) && stat != eActionStatusAborted) {
            stat = _publicInterface->isRenderAborted() ? eActionStatusAborted : stat;
        }

        if (args.backendType == eRenderBackendTypeOpenGL ||
            args.backendType == eRenderBackendTypeOSMesa) {
            if (args.glContext && args.glContext->isGPUContext()) {
                GLImageStoragePtr glEntry = mainImagePlane->getGLImageStorage();
                assert(glEntry);
                GL_GPU::BindTexture(glEntry->getGLTextureTarget(), 0);
                finishGLRender<GL_GPU>();
            } else {
                finishGLRender<GL_CPU>();
            }
        }

        if (osmesaRenderImage) {
            Image::CopyPixelsArgs cpyArgs;
            cpyArgs.roi = actionArgs.roi;
            cpyArgs.conversionChannel = 3;
            cpyArgs.alphaHandling = Image::eAlphaChannelHandlingFillFromChannel;
            stat = mainImagePlane->copyPixels(*osmesaRenderImage, cpyArgs);
        }

        if (isFailureRetCode(stat)) {
            return stat;
        }

    } // for (std::list<std::list<std::pair<ImagePlaneDesc,ImagePtr> > >::iterator it = planesLists.begin(); it != planesLists.end(); ++it)

    bool aborted = render->isRenderAborted();
    if (aborted) {
        return eActionStatusAborted;
    }

    // If we created a temporary plane because the plug-in is not multiplanar, copy back to the final plane
    std::map<ImagePlaneDesc, ImagePtr>::const_iterator itOther = nonMultiplanarPlanes.begin();
    for (std::map<ImagePlaneDesc, ImagePtr>::const_iterator it = args.cachedPlanes.begin(); it != args.cachedPlanes.end(); ++it, ++itOther) {
        if (it->second != itOther->second) {
            Image::CopyPixelsArgs copyArgs;
            copyArgs.roi = actionArgs.roi;
            copyArgs.alphaHandling = itOther->second->getComponentsCount() == 1 || itOther->second->getComponentsCount() == 4 ? Image::eAlphaChannelHandlingFillFromChannel : Image::eAlphaChannelHandlingCreateFill0;
            ActionRetCodeEnum stat = it->second->copyPixels(*itOther->second, copyArgs);
            if (isFailureRetCode(stat)) {
                return stat;
            }
        }
    }

    return eActionStatusOK;
} // renderHandlerPlugin

ActionRetCodeEnum
EffectInstance::Implementation::renderHandlerPostProcess(const RectToRender & rectToRender,
                                                         const TiledRenderingFunctorArgs& args)
{


    // Get the mask image if host masking is needed
    ImagePtr maskImage;

    unsigned int curMipMap = args.requestData->getRenderMappedMipMapLevel();
    const RenderScale& proxyScale = args.requestData->getProxyScale();

    assert(args.requestData->getComponentsResults());
    const std::map<int, std::list<ImagePlaneDesc> > &inputPlanesNeeded = args.requestData->getComponentsResults()->getNeededInputPlanes();
    std::bitset<4> processChannels = args.requestData->getComponentsResults()->getProcessChannels();

    bool hostMasking = _publicInterface->isHostMaskEnabled();
    if ( hostMasking ) {

        int maskInputNb = -1;
        int inputsCount = _publicInterface->getNInputs();
        for (int i = 0; i < inputsCount; ++i) {
            if (_publicInterface->getNode()->isInputMask(i) && _publicInterface->isMaskEnabled(i)) {
                maskInputNb = i;
            }
        }
        if (maskInputNb != -1) {

            // Fetch the mask image
            std::map<int, std::list<ImagePlaneDesc> >::const_iterator foundNeededLayers = inputPlanesNeeded.find(maskInputNb);
            if (foundNeededLayers != inputPlanesNeeded.end() && !foundNeededLayers->second.empty()) {

                GetImageInArgs inArgs(&curMipMap, &proxyScale, &rectToRender.rect, &args.backendType);
                inArgs.plane = &foundNeededLayers->second.front();
                inArgs.inputNb = maskInputNb;
                GetImageOutArgs outArgs;
                if (_publicInterface->getImagePlane(inArgs, &outArgs)) {
                    maskImage = outArgs.image;
                }
            }

        }
    } // hostMasking




    int mainInputNb = _publicInterface->getNode()->getPreferredInput();
    if ( (mainInputNb != -1) && _publicInterface->getNode()->isInputMask(mainInputNb) ) {
        mainInputNb = -1;
    }


    //bool unPremultIfNeeded = planes.outputPremult == eImagePremultiplicationPremultiplied;
    bool useMaskMix = hostMasking;
    double mix = 1.;
    if (_publicInterface->isHostMixEnabled()) {
        mix = _publicInterface->getHostMixingValue(_publicInterface->getCurrentRenderTime(), _publicInterface->getCurrentRenderView());
        useMaskMix = true;
    }

    const bool checkNaNs = _publicInterface->getCurrentRender()->isNaNHandlingEnabled();

    // Check for NaNs, copy to output image and mark for rendered
    for (std::map<ImagePlaneDesc, ImagePtr>::const_iterator it = args.cachedPlanes.begin(); it != args.cachedPlanes.end(); ++it) {

        if (checkNaNs) {

            bool foundNan = false;
            ActionRetCodeEnum stat = eActionStatusOK;
            if (it->second->getBitDepth() == eImageBitDepthFloat && it->second->getStorageMode() == eStorageModeRAM) {
                stat = it->second->checkForNaNs(rectToRender.rect, &foundNan);
                if (isFailureRetCode(stat)) {
                    return stat;
                }
            }

            if (!foundNan) {
                _publicInterface->getNode()->clearPersistentMessage(kNatronPersistentWarningCheckForNan);
            } else {
                QString warning;
                warning.append( tr("NaN values detected in (") );
                warning.append( QString::number(rectToRender.rect.x1) );
                warning.append( QChar::fromLatin1(',') );
                warning.append( QString::number(rectToRender.rect.y1) );
                warning.append( QString::fromUtf8(")-(") );
                warning.append( QString::number(rectToRender.rect.x2) );
                warning.append( QChar::fromLatin1(',') );
                warning.append( QString::number(rectToRender.rect.y2) );
                warning.append( QString::fromUtf8("). ") );
                warning.append( tr("They have been converted to 1") );
                _publicInterface->getNode()->setPersistentMessage( eMessageTypeWarning, kNatronPersistentWarningCheckForNan, warning.toStdString() );
            }
        } // checkNaNs

        ImagePtr mainInputImage;
        bool copyUnProcessed = it->second->canCallCopyUnProcessedChannels(processChannels);
        if ((copyUnProcessed || useMaskMix) && mainInputNb != -1) {
            // Get the main input image to copy channels from it if a RGBA checkbox is unchecked

            std::map<int, std::list<ImagePlaneDesc> >::const_iterator foundNeededLayers = inputPlanesNeeded.find(mainInputNb);

            GetImageInArgs inArgs(&curMipMap, &proxyScale, &rectToRender.rect, &args.backendType);
            if (foundNeededLayers != inputPlanesNeeded.end() && !foundNeededLayers->second.empty()) {


                std::list<ImagePlaneDesc>::const_iterator inputLayerIt = ImagePlaneDesc::findEquivalentLayer(it->first, foundNeededLayers->second.begin(), foundNeededLayers->second.end());
                if (inputLayerIt != foundNeededLayers->second.end()) {
                    inArgs.plane = &foundNeededLayers->second.front();
                    inArgs.inputNb = mainInputNb;
                    GetImageOutArgs outArgs;
                    if (_publicInterface->getImagePlane(inArgs, &outArgs)) {
                        mainInputImage = outArgs.image;
                    }
                }
            }
        }

        if (copyUnProcessed && mainInputImage) {
            ActionRetCodeEnum stat = it->second->copyUnProcessedChannels(rectToRender.rect, processChannels, mainInputImage);
            if (isFailureRetCode(stat)) {
                return stat;
            }
        }


        if (useMaskMix) {
            ActionRetCodeEnum stat = it->second->applyMaskMix(rectToRender.rect, maskImage, mainInputImage, maskImage.get() /*masked*/, false /*maskInvert*/, mix);
            if (isFailureRetCode(stat)) {
                return stat;
            }
        }


    } // for each plane to render
    return eActionStatusOK;
} // renderHandlerPostProcess


NATRON_NAMESPACE_EXIT
