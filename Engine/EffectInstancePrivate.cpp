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
#include <QDebug>

#include "Engine/AppInstance.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/OSGLContext.h"
#include "Engine/OSGLFunctions.h"
#include "Engine/PluginMemory.h"
#include "Engine/Timer.h"
#include "Engine/TreeRender.h"
#include "Engine/RenderStats.h"
#include "Engine/RotoStrokeItem.h"
#include "Engine/ViewIdx.h"

#define kNatronPersistentWarningCheckForNan "NatronPersistentWarningCheckForNan"

NATRON_NAMESPACE_ENTER;


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
    {
        QMutexLocker k(&common->pluginsPropMutex);
        renderData->props = common->props;
    }
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
EffectInstance::Implementation::setTimeInvariantMetadataResults(const GetTimeInvariantMetaDatasResultsPtr& metadatas)
{
    if (!renderData) {
        return;
    }
    QMutexLocker k(&renderData->lock);
    renderData->metadatasResults = metadatas;
}


GetTimeInvariantMetaDatasResultsPtr
EffectInstance::Implementation::getTimeInvariantMetadataResults() const
{
    if (!renderData) {
        return GetTimeInvariantMetaDatasResultsPtr();
    }
    QMutexLocker k(&renderData->lock);
    return renderData->metadatasResults;
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
EffectInstance::Implementation::resolveRenderBackend(const RequestPassSharedDataPtr& requestPassSharedData, const FrameViewRequestPtr& requestPassData, const RectI& roi, RenderBackendTypeEnum* renderBackend)
{
    // Default to CPU
    *renderBackend = eRenderBackendTypeCPU;

    TreeRenderPtr render = _publicInterface->getCurrentRender();
    OSGLContextPtr glGpuContext = render->getGPUOpenGLContext();
    OSGLContextPtr glCpuContext = render->getCPUOpenGLContext();

    bool canDoOpenGLRendering;

    PluginOpenGLRenderSupport openGLSupport = _publicInterface->getCurrentOpenGLSupport();
    canDoOpenGLRendering = (openGLSupport == ePluginOpenGLRenderSupportNeeded || openGLSupport == ePluginOpenGLRenderSupportYes);


    
    if (canDoOpenGLRendering) {

        // Enable GPU render if the plug-in cannot render another way or if all conditions are met
        if (openGLSupport == ePluginOpenGLRenderSupportNeeded && !_publicInterface->getNode()->getPlugin()->isOpenGLEnabled()) {

            QString message = tr("OpenGL render is required for  %1 but was disabled in the Preferences for this plug-in, please enable it and restart %2").arg(QString::fromUtf8(_publicInterface->getNode()->getLabel().c_str())).arg(QString::fromUtf8(NATRON_APPLICATION_NAME));
            _publicInterface->getNode()->setPersistentMessage(eMessageTypeError, kNatronPersistentErrorGenericRenderMessage, message.toStdString());
            return eActionStatusFailed;
        }


        *renderBackend = eRenderBackendTypeOpenGL;

        // If the plug-in knows how to render on CPU, check if we should actually render on CPU instead.
        if (openGLSupport == ePluginOpenGLRenderSupportYes) {

            // User want to force caching of this node but we cannot cache OpenGL renders, so fallback on CPU.
            if ( _publicInterface->isForceCachingEnabled() ) {
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
        bool supportsOSMesa = _publicInterface->canCPUImplementationSupportOSMesa();
        if (supportsOSMesa) {
            if ( (roi.width() < (glCpuContext)->getMaxOpenGLWidth()) &&
                ( roi.height() < (glCpuContext)->getMaxOpenGLHeight()) ) {
                *renderBackend = eRenderBackendTypeOSMesa;
            }
        }
    }

    return eActionStatusOK;
} // resolveRenderBackend

CacheAccessModeEnum
EffectInstance::Implementation::shouldRenderUseCache(const RequestPassSharedDataPtr& requestPassSharedData, const FrameViewRequestPtr& requestPassData)
{
    bool retSet = false;
    CacheAccessModeEnum ret = eCacheAccessModeNone;

    // A writer never caches!
    if (!retSet && _publicInterface->isWriter()) {
        retSet = true;
        ret = eCacheAccessModeNone;
    }

    if (!retSet) {
        EffectInstancePtr treeRoot = _publicInterface->getCurrentRender()->getOriginalTreeRoot();
        if (treeRoot == _publicInterface->getNode()->getEffectInstance())  {
            // Always cache the root node because a subsequent render may ask for it
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

    for (std::map<ImagePlaneDesc, ImagePtr>::const_iterator it = args.cachedPlanes.begin(); it != args.cachedPlanes.end(); ++it) {
        boost::scoped_ptr<EffectInstance::GetImageInArgs> inArgs( new EffectInstance::GetImageInArgs() );
        inArgs->renderBackend = &args.backendType;
        inArgs->currentRenderWindow = &rectToRender.rect;
        inArgs->inputTime = &rectToRender.identityTime;
        inArgs->inputView = &rectToRender.identityView;
        unsigned int curMipMap = args.requestData->getRenderMappedMipMapLevel();
        inArgs->currentActionMipMapLevel = &curMipMap;
        const RenderScale& curProxyScale = args.requestData->getProxyScale();
        inArgs->currentActionProxyScale = &curProxyScale;
        inArgs->inputNb = rectToRender.identityInputNumber;
        inArgs->plane = &it->first;

        GetImageOutArgs inputResults;
        {
            bool gotPlanes = _publicInterface->getImagePlane(*inArgs, &inputResults);
            if (!gotPlanes) {
                return eActionStatusFailed;
            }
        }

        Image::CopyPixelsArgs cpyArgs;
        rectToRender.rect.intersect(it->second->getBounds(), &cpyArgs.roi);
        ActionRetCodeEnum stat = it->second->copyPixels(*inputResults.image, cpyArgs);
        if (isFailureRetCode(stat)) {
            return stat;
        }
    }

    return render->isRenderAborted() ? eActionStatusAborted : eActionStatusOK;
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
        assert(image->getStorageMode() == eStorageModeRAM);
        assert(image->getBufferFormat() == eImageBufferLayoutRGBAPackedFullRect);

        Image::CPUData data;
        image->getCPUData(&data);

        void* buffer = (void*)Image::pixelAtStatic(roi.x1, roi.y1, data.bounds, data.nComps, getSizeOfForBitDepth(data.bitDepth), (unsigned char*)data.ptrs[0]);
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

    std::list< std::list<std::pair<ImagePlaneDesc, ImagePtr> > > planesLists;

    bool multiPlanar = _publicInterface->isMultiPlanar();
    // If we can render all planes at once, do it, otherwise just render them all sequentially
    if (!multiPlanar) {
        for (std::map<ImagePlaneDesc, ImagePtr>::const_iterator it = args.cachedPlanes.begin(); it != args.cachedPlanes.end(); ++it) {
            std::list<std::pair<ImagePlaneDesc, ImagePtr> > tmp;
            tmp.push_back(*it);
            planesLists.push_back(tmp);
        }
    } else {
        std::list<std::pair<ImagePlaneDesc, ImagePtr> > tmp;
        for (std::map<ImagePlaneDesc, ImagePtr>::const_iterator it = args.cachedPlanes.begin(); it != args.cachedPlanes.end(); ++it) {
            tmp.push_back(*it);
        }
        planesLists.push_back(tmp);
    }

    for (std::list<std::list<std::pair<ImagePlaneDesc, ImagePtr> > >::iterator it = planesLists.begin(); it != planesLists.end(); ++it) {

        actionArgs.outputPlanes = *it;

        const ImagePtr& mainImagePlane = actionArgs.outputPlanes.front().second;
        if (args.backendType == eRenderBackendTypeOpenGL ||
            args.backendType == eRenderBackendTypeOSMesa) {

            // Effects that render multiple planes at once are NOT supported by the OpenGL render suite
            // We only bind to the framebuffer color attachment 0 the "main" output image plane
            assert(actionArgs.outputPlanes.size() == 1);
            if (args.glContext->isGPUContext()) {
                setupGLForRender<GL_GPU>(mainImagePlane, args.glContext, actionArgs.roi, _publicInterface->getNode()->isGLFinishRequiredBeforeRender());
            } else {
                setupGLForRender<GL_CPU>(mainImagePlane, args.glContext, actionArgs.roi, _publicInterface->getNode()->isGLFinishRequiredBeforeRender());
            }
        }
        ActionRetCodeEnum stat = _publicInterface->render_public(actionArgs);

        if (args.backendType == eRenderBackendTypeOpenGL ||
            args.backendType == eRenderBackendTypeOSMesa) {
            if (args.glContext) {
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

    } // for (std::list<std::list<std::pair<ImagePlaneDesc,ImagePtr> > >::iterator it = planesLists.begin(); it != planesLists.end(); ++it)

    return render->isRenderAborted() ? eActionStatusAborted : eActionStatusOK;
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
    if ( (mainInputNb != -1) && _publicInterface->isInputMask(mainInputNb) ) {
        mainInputNb = -1;
    }


    //bool unPremultIfNeeded = planes.outputPremult == eImagePremultiplicationPremultiplied;
    bool useMaskMix = hostMasking;
    double mix = 1.;
    if (_publicInterface->isHostMixingEnabled()) {
        mix = _publicInterface->getHostMixingValue(_publicInterface->getCurrentRenderTime(), _publicInterface->getCurrentRenderView());
        useMaskMix = true;
    }

    const bool checkNaNs = _publicInterface->getCurrentRender()->isNaNHandlingEnabled();

    // Check for NaNs, copy to output image and mark for rendered
    for (std::map<ImagePlaneDesc, ImagePtr>::const_iterator it = args.cachedPlanes.begin(); it != args.cachedPlanes.end(); ++it) {

        if (checkNaNs) {

            if (!it->second->checkForNaNs(rectToRender.rect)) {
                _publicInterface->getNode()->clearPersistentMessage(kNatronPersistentWarningCheckForNan);
            } else {
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
                _publicInterface->getNode()->setPersistentMessage( eMessageTypeWarning, kNatronPersistentWarningCheckForNan, warning.toStdString() );
            }
        } // checkNaNs

        ImagePtr mainInputImage;

        bool copyUnProcessed = it->second->canCallCopyUnProcessedChannels(processChannels);
        if (copyUnProcessed && mainInputNb != -1) {
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
            if (mainInputImage) {
                ActionRetCodeEnum stat = it->second->copyUnProcessedChannels(rectToRender.rect, processChannels, mainInputImage);
                if (isFailureRetCode(stat)) {
                    return stat;
                }
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


NATRON_NAMESPACE_EXIT;


