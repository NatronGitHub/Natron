/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2017 INRIA and Alexandre Gauthier-Foichat
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

#include "Engine/AppInstance.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/OSGLContext.h"
#include "Engine/OSGLFunctions.h"
#include "Engine/Timer.h"
#include "Engine/TreeRender.h"
#include "Engine/TreeRenderNodeArgs.h"
#include "Engine/RenderStats.h"
#include "Engine/RotoStrokeItem.h"
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


ActionRetCodeEnum
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
            return eActionStatusFailed;
        }


        *renderBackend = eRenderBackendTypeOpenGL;

        // If the plug-in knows how to render on CPU, check if we should actually render on CPU instead.
        if (args.renderArgs->getCurrentRenderOpenGLSupport() == ePluginOpenGLRenderSupportYes) {

            // User want to force caching of this node but we cannot cache OpenGL renders, so fallback on CPU.
            if ( _publicInterface->getNode()->isForceCachingEnabled() ) {
                *renderBackend = eRenderBackendTypeCPU;
            }

            // If this image is requested multiple times , do not render it on OpenGL since we do not use the cache.
            if (*renderBackend == eRenderBackendTypeOpenGL) {
                if (requestPassData->getFramesNeededVisitsCount() > 1) {
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

    return eActionStatusOK;
} // resolveRenderBackend

CacheAccessModeEnum
EffectInstance::Implementation::shouldRenderUseCache(const RenderRoIArgs & args,
                                                     const FrameViewRequestPtr& requestPassData)
{
    bool retSet = false;
    CacheAccessModeEnum ret = eCacheAccessModeNone;

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

        bool useCache = _publicInterface->shouldCacheOutput(isFrameVaryingOrAnimated, args.renderArgs, requestsCount);
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
EffectInstance::Implementation::tiledRenderingFunctor(EffectInstance::Implementation::TiledRenderingFunctorArgs & args,
                                                      const RectToRender & specificData,
                                                      QThread* callingThread)
{
    ///Make the thread-storage live as long as the render action is called if we're in a newly launched thread in eRenderSafetyFullySafeFrame mode
    QThread* curThread = QThread::currentThread();

    if (callingThread != curThread) {
        ///We are in the case of host frame threading, see kOfxImageEffectPluginPropHostFrameThreading
        ///We know that in the renderAction, TLS will be needed, so we do a deep copy of the TLS from the caller thread
        ///to this thread
        appPTR->getAppTLS()->copyTLS(callingThread, curThread);
    }


    ActionRetCodeEnum ret = tiledRenderingFunctor(specificData,
                                                  args.args,
                                                  args.renderMappedScale,
                                                  args.processChannels,
                                                  args.mainInputImage,
                                                  args.planesToRender,
                                                  args.glContext);

    //Exit of the host frame threading thread
    if (callingThread != curThread) {
        appPTR->getAppTLS()->cleanupTLSForThread();
    }

    return ret;
} // tiledRenderingFunctor



ActionRetCodeEnum
EffectInstance::Implementation::tiledRenderingFunctor(const RectToRender & rectToRender,
                                                      const RenderRoIArgs* args,
                                                      RenderScale renderMappedScale,
                                                      std::bitset<4> processChannels,
                                                      const ImagePtr& mainInputImage,
                                                      ImagePlanesToRenderPtr planesToRender,
                                                      OSGLContextAttacherPtr glContext)
{


    EffectInstanceTLSDataPtr tls = tlsData->getOrCreateTLSData();

    // We may have copied the TLS from a thread that spawned us. The other thread might have already started the render actino:
    // ensure that we start this thread with a clean state.
    tls->ensureLastActionInStackIsNotRender();


    // Record the time spend to render for this frame/view for this thread
    TimeLapsePtr timeRecorder;
    RenderStatsPtr stats = args->renderArgs->getParentRender()->getStatsObject();
    if (stats && stats->isInDepthProfilingEnabled()) {
        timeRecorder.reset(new TimeLapse);
    }

    // If using OpenGL, bind the frame buffer
    if (planesToRender->backendType == eRenderBackendTypeOpenGL) {
        assert(glContext);

        glContext->attach();

        GLuint fboID = glContext->getContext()->getOrCreateFBOId();
        GL_GPU::BindFramebuffer(GL_FRAMEBUFFER, fboID);
        glCheckError(GL_GPU);
    }


    // If this tile is identity, copy input image instead
    ActionRetCodeEnum stat;
    if (rectToRender.identityInputNumber != -1) {
        stat = renderHandlerIdentity(rectToRender, args, renderMappedScale, planesToRender);
    } else {
        stat = renderHandlerInternal(tls, rectToRender, args, renderMappedScale, glContext, planesToRender, processChannels);
        if (isFailureRetCode(stat)) {
            return stat;
        }
        // Apply post-processing
        renderHandlerPostProcess(rectToRender, args, renderMappedScale, planesToRender, mainInputImage, processChannels);
    }


    // The render went OK: copy the temporary image with the plug-in preferred format to the cache image
    for (std::map<ImageComponents, PlaneToRender>::iterator it = planesToRender->planes.begin(); it != planesToRender->planes.end(); ++it) {
        if (it->second.cacheImage != it->second.tmpImage) {
            Image::CopyPixelsArgs cpyArgs;
            cpyArgs.roi = rectToRender.rect;
            it->second.cacheImage->copyPixels(*it->second.tmpImage, cpyArgs);
        }
    }


    if (timeRecorder) {
        stats->addRenderInfosForNode(_publicInterface->getNode(), timeRecorder->getTimeSinceCreation());
    }
    return args->renderArgs->isRenderAborted() ? eActionStatusAborted : eActionStatusOK;
} // EffectInstance::tiledRenderingFunctor


ActionRetCodeEnum
EffectInstance::Implementation::renderHandlerIdentity(const RectToRender & rectToRender,
                                                      const RenderRoIArgs* args,
                                                      const RenderScale &renderMappedScale,
                                                      const ImagePlanesToRenderPtr& planesToRender)
{



    boost::scoped_ptr<EffectInstance::GetImageInArgs> renderArgs( new EffectInstance::GetImageInArgs() );
    renderArgs->currentTime = args->time;
    renderArgs->currentView = args->view;
    renderArgs->currentScale = renderMappedScale;
    renderArgs->renderBackend = &planesToRender->backendType;
    renderArgs->renderArgs = args->renderArgs;
    renderArgs->inputTime = rectToRender.identityTime;
    renderArgs->inputView = rectToRender.identityView;
    renderArgs->inputNb = rectToRender.identityInputNumber;

    std::list<ImageComponents> components;
    for (std::map<ImageComponents, PlaneToRender>::iterator it = planesToRender->planes.begin(); it != planesToRender->planes.end(); ++it) {
        components.push_back(it->first);
    }
    renderArgs->layers = &components;

    GetImageOutArgs inputResults;
    {
        bool gotPlanes = _publicInterface->getImagePlanes(*renderArgs, &inputResults);
        if (!gotPlanes) {
            return eActionStatusFailed;
        }
    }


    for (std::map<ImageComponents, PlaneToRender>::iterator it = planesToRender->planes.begin(); it != planesToRender->planes.end(); ++it) {
        std::map<ImageComponents, ImagePtr>::const_iterator foundIdentityPlane = inputResults.imagePlanes.find(it->first);
        if (foundIdentityPlane == inputResults.imagePlanes.end()) {
            continue;
        }
        Image::CopyPixelsArgs cpyArgs;
        cpyArgs.roi = rectToRender.rect;
        it->second.tmpImage->copyPixels(*foundIdentityPlane->second, cpyArgs);
    }

    return args->renderArgs->isRenderAborted() ? eActionStatusAborted : eActionStatusOK;
} // renderHandlerIdentity

template <typename GL>
static void setupGLForRender(const ImagePtr& image,
                             const OSGLContextPtr& glContext,
                             const RectI& roi,
                             bool callGLFinish)
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
        assert(image->getStorageMode() == eStorageModeDisk || image->getStorageMode() == eStorageModeRAM);
        assert(image->getBufferFormat() == eImageBufferLayoutRGBAPackedFullRect);

        Image::Tile tile;
        bool ok = image->getTileAt(0, &tile);
        assert(ok);
        (void)ok;
        Image::CPUTileData tileData;
        image->getCPUTileData(tile, &tileData);

        void* buffer = (void*)Image::pixelAtStatic(roi.x1, roi.y1, tileData.tileBounds, tileData.nComps, getSizeOfForBitDepth(tileData.bitDepth), (unsigned char*)tileData.ptrs[0]);
        assert(buffer);

        // With OSMesa we render directly to the context framebuffer
        OSGLContextAttacherPtr glContextAttacher = OSGLContextAttacher::create(glContext,
                                                                               roi.width(),
                                                                               roi.height(),
                                                                               imageBounds.width(),
                                                                               buffer);
        glContextAttacher->attach();
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
EffectInstance::Implementation::renderHandlerInternal(const EffectInstanceTLSDataPtr& tls,
                                                      const RectToRender & rectToRender,
                                                      const RenderRoIArgs* args,
                                                      const RenderScale& renderMappedScale,
                                                      const OSGLContextAttacherPtr glContext,
                                                      const ImagePlanesToRenderPtr& planesToRender,
                                                      std::bitset<4> processChannels)
{


    RenderActionArgs actionArgs;
    {
        actionArgs.processChannels = processChannels;
        actionArgs.renderScale = renderMappedScale;
        actionArgs.backendType = planesToRender->backendType;
        actionArgs.roi = rectToRender.rect;
        actionArgs.time = args->time;
        actionArgs.view = args->view;
        actionArgs.glContextAttacher = glContext;
    }

    std::list< std::list<std::pair<ImageComponents, ImagePtr> > > planesLists;

    bool multiPlanar = _publicInterface->isMultiPlanar();
    // If we can render all planes at once, do it, otherwise just render them all sequentially
    if (!multiPlanar) {
        for (std::map<ImageComponents, PlaneToRender >::iterator it = planesToRender->planes.begin(); it != planesToRender->planes.end(); ++it) {
            std::list<std::pair<ImageComponents, ImagePtr> > tmp;
            tmp.push_back(std::make_pair(it->first, it->second.tmpImage));
            planesLists.push_back(tmp);
        }
    } else {
        std::list<std::pair<ImageComponents, ImagePtr> > tmp;
        for (std::map<ImageComponents, PlaneToRender >::iterator it = planesToRender->planes.begin(); it != planesToRender->planes.end(); ++it) {
            tmp.push_back(std::make_pair(it->first, it->second.tmpImage));
        }
        planesLists.push_back(tmp);
    }

    OSGLContextPtr openGLContext = glContext->getContext();

    for (std::list<std::list<std::pair<ImageComponents, ImagePtr> > >::iterator it = planesLists.begin(); it != planesLists.end(); ++it) {

        actionArgs.outputPlanes = *it;

        const ImagePtr& mainImagePlane = actionArgs.outputPlanes.front().second;
        if (planesToRender->backendType == eRenderBackendTypeOpenGL ||
            planesToRender->backendType == eRenderBackendTypeOSMesa) {

            actionArgs.glContextData = planesToRender->glContextData;

            // Effects that render multiple planes at once are NOT supported by the OpenGL render suite
            // We only bind to the framebuffer color attachment 0 the "main" output image plane
            assert(actionArgs.outputPlanes.size() == 1);
            if (openGLContext->isGPUContext()) {
                setupGLForRender<GL_GPU>(mainImagePlane, openGLContext, actionArgs.roi, _publicInterface->getNode()->isGLFinishRequiredBeforeRender());
            } else {
                setupGLForRender<GL_CPU>(mainImagePlane, openGLContext, actionArgs.roi, _publicInterface->getNode()->isGLFinishRequiredBeforeRender());
            }
        }
        ActionRetCodeEnum stat;
        {

            ///This RAII struct controls the lifetime of the render action arguments
            RenderActionArgsSetter_RAII actionArgsTls(tls,
                                                      actionArgs.time,
                                                      actionArgs.view,
                                                      actionArgs.renderScale,
                                                      actionArgs.roi,
                                                      planesToRender->planes);

            stat = _publicInterface->render_public(actionArgs);
        }

        if (planesToRender->backendType == eRenderBackendTypeOpenGL ||
            planesToRender->backendType == eRenderBackendTypeOSMesa) {
            if (openGLContext) {
                GLImageStoragePtr glEntry = mainImagePlane->getGLImageStorage();
                assert(glEntry);
                GL_GPU::BindTexture(glEntry->getGLTextureTarget(), 0);
                finishGLRender<GL_GPU>();
            } else {
                finishGLRender<GL_CPU>();
            }
        }

        if (isFailureRetCode(stat)) {
            return stat;
        }

    } // for (std::list<std::list<std::pair<ImageComponents,ImagePtr> > >::iterator it = planesLists.begin(); it != planesLists.end(); ++it)

    return args->renderArgs->isRenderAborted() ? eActionStatusAborted : eActionStatusOK;
} // EffectInstance::Implementation::renderHandlerInternal


void
EffectInstance::Implementation::renderHandlerPostProcess(const RectToRender & rectToRender,
                                                         const RenderRoIArgs* args,
                                                         const RenderScale& renderMappedScale,
                                                         const ImagePlanesToRenderPtr& planesToRender,
                                                         const ImagePtr& mainInputImage,
                                                         std::bitset<4> processChannels)
{


    ImagePtr maskImage;

    bool hostMasking = _publicInterface->isHostMaskingEnabled();
    if ( hostMasking ) {

        int maskInputNb = -1;
        int inputsCount = _publicInterface->getMaxInputCount();
        for (int i = 0; i < inputsCount; ++i) {
            if (_publicInterface->isMaskEnabled(i)) {
                maskInputNb = i;
            }
        }

        if (maskInputNb != -1) {

            // Fetch the mask image

            ImageComponents maskLayer;
            _publicInterface->getNode()->getMaskChannel(maskInputNb, &maskLayer);

            GetImageInArgs inArgs;
            std::list<ImageComponents> layersToFetch;
            layersToFetch.push_back(maskLayer);
            inArgs.layers = &layersToFetch;
            inArgs.inputNb = maskInputNb;
            inArgs.currentTime = args->time;
            inArgs.currentView = args->view;
            inArgs.inputTime = inArgs.currentTime;
            inArgs.inputView = inArgs.currentView;
            inArgs.currentScale = renderMappedScale;
            inArgs.renderBackend = &planesToRender->backendType;
            inArgs.renderArgs = args->renderArgs;
            GetImageOutArgs outArgs;
            bool gotImage = _publicInterface->getImagePlanes(inArgs, &outArgs);
            if (gotImage) {
                assert(!outArgs.imagePlanes.empty());
                maskImage = outArgs.imagePlanes.begin()->second;
            }

        }
    } // hostMasking



    // A node that is part of a stroke render implementation needs to accumulate so set the last rendered image pointer
    RotoStrokeItemPtr attachedItem = toRotoStrokeItem(_publicInterface->getNode()->getAttachedRotoItem());

    //bool unPremultIfNeeded = planes.outputPremult == eImagePremultiplicationPremultiplied;
    bool useMaskMix = hostMasking;
    double mix = 1.;
    if (_publicInterface->isHostMixingEnabled()) {
        mix = _publicInterface->getNode()->getHostMixingValue(args->time, args->view);
        useMaskMix = true;
    }

    // Check for NaNs, copy to output image and mark for rendered
    for (std::map<ImageComponents, PlaneToRender>::const_iterator it = planesToRender->planes.begin(); it != planesToRender->planes.end(); ++it) {

        //bool unPremultRequired = unPremultIfNeeded && it->second.tmpImage->getComponentsCount() == 4 && it->second.renderMappedImage->getComponentsCount() == 3;

        if (args->renderArgs->getParentRender()->isNaNHandlingEnabled()) {

            if (it->second.tmpImage->checkForNaNs(rectToRender.rect)) {
                QString warning = QString::fromUtf8( _publicInterface->getNode()->getScriptName_mt_safe().c_str() );
                warning.append( QString::fromUtf8(": ") );
                warning.append( tr("rendered rectangle (") );
                warning.append( QString::number(rectToRender.rect.x1) );
                warning.append( QChar::fromLatin1(',') );
                warning.append( QString::number(rectToRender.rect.y1) );
                warning.append( QString::fromUtf8(")-(") );
                warning.append( QString::number(rectToRender.rect.x2) );
                warning.append( QChar::fromLatin1(',') );
                warning.append( QString::number(rectToRender.rect.y2) );
                warning.append( QString::fromUtf8(") ") );
                warning.append( tr("contains NaN values. They have been converted to 1.") );
                _publicInterface->setPersistentMessage( eMessageTypeWarning, warning.toStdString() );
            }
        }

        if (mainInputImage) {
            it->second.tmpImage->copyUnProcessedChannels(rectToRender.rect, processChannels, mainInputImage);
        }


        if (useMaskMix) {
            it->second.tmpImage->applyMaskMix(rectToRender.rect, maskImage, mainInputImage, maskImage.get() /*masked*/, false /*maskInvert*/, mix);
        }
        
        // Set the accumulation buffer for this node if needed
        if (attachedItem) {
            _publicInterface->getNode()->setLastRenderedImage(it->second.tmpImage);
        }
        
    } // for (std::map<ImageComponents,PlaneToRender>::const_iterator it = outputPlanes.begin(); it != outputPlanes.end(); ++it) {
    
} // EffectInstance::Implementation::renderHandlerPostProcess


NATRON_NAMESPACE_EXIT;


