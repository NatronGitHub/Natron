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
#include "Engine/RenderStats.h"
#include "Engine/RotoStrokeItem.h"
#include "Engine/ViewIdx.h"


NATRON_NAMESPACE_ENTER;


EffectInstance::Implementation::Implementation(EffectInstance* publicInterface)
: _publicInterface(publicInterface)
, common(new EffectInstanceCommonData)
, renderData()
, defKnobs(new DefaultKnobs)
{
}

EffectInstance::Implementation::Implementation(EffectInstance* publicInterface, const Implementation& other)
: _publicInterface(publicInterface)
, common(other.common)
, renderData(new RenderCloneData)
, defKnobs(other.defKnobs)
{
    {
        QMutexLocker k(&common->pluginsPropMutex);
        renderData->props = common->props;
    }
}


bool
RenderCloneData::getTimeViewInvariantHash(U64* hash) const
{
    QMutexLocker k(&lock);
    if (!timeViewInvariantHashValid) {
        return false;
    }
    *hash = timeViewInvariantHash;
    return true;
}

void
RenderCloneData::setTimeInvariantMetadataHash(U64 hash)
{
    QMutexLocker k(&lock);
    metadataTimeInvariantHashValid = true;
    metadataTimeInvariantHash = hash;
}

bool
RenderCloneData::getTimeInvariantMetadataHash(U64* hash) const
{
    QMutexLocker k(&lock);
    if (!metadataTimeInvariantHashValid) {
        return false;
    }
    *hash = metadataTimeInvariantHash;
    return true;

}

void
RenderCloneData::setTimeViewInvariantHash(U64 hash)
{
    QMutexLocker k(&lock);
    timeViewInvariantHash = hash;
    timeViewInvariantHashValid = true;
}


bool
RenderCloneData::getFrameViewHash(TimeValue time, ViewIdx view, U64* hash) const
{
    FrameViewPair p = {time,view};
    QMutexLocker k(&lock);
    FrameViewHashMap::const_iterator found = timeViewVariantHash.find(p);
    if (found == timeViewVariantHash.end()) {
        return false;
    }
    *hash = found->second;
    return true;
}

void
RenderCloneData::setFrameViewHash(TimeValue time, ViewIdx view, U64 hash)
{
    QMutexLocker k(&lock);
    FrameViewPair p = {time,view};
    timeViewVariantHash[p] = hash;
}


FrameViewRequestPtr
EffectInstance::Implementation::getFrameViewRequest(TimeValue time,
                                        ViewIdx view) const
{
    // Needs to be locked: frame requests may be added spontaneously by the plug-in
    QMutexLocker k(&renderData->lock);

    FrameViewPair p = {time,view};
    NodeFrameViewRequestData::const_iterator found = renderData->frames.find(p);
    if (found == renderData->frames.end()) {
        return FrameViewRequestPtr();
    }
    return found->second;
}

bool
EffectInstance::Implementation::getOrCreateFrameViewRequest(TimeValue time, ViewIdx view, unsigned int mipMapLevel, const ImagePlaneDesc& plane, FrameViewRequestPtr* request)
{

    // Needs to be locked: frame requests may be added spontaneously by the plug-in
    QMutexLocker k(&renderData->lock);

    FrameViewPair p = {time, view};
    NodeFrameViewRequestData::iterator found = renderData->frames.find(p);
    if (found != renderData->frames.end()) {
        *request = found->second;
        return false;
    }

    U64 hash;
    if (!renderData->getFrameViewHash(time, view, &hash)) {
        HashableObject::ComputeHashArgs args;
        args.time = time;
        args.view = view;
        args.hashType = HashableObject::eComputeHashTypeTimeViewVariant;
        hash = _publicInterface->computeHash(args);
        renderData->setFrameViewHash(time, view, hash);
    }

    FrameViewRequestPtr& ret = renderData->frames[p];
    ret.reset(new FrameViewRequest(time, view, mipMapLevel, plane, hash, _publicInterface->shared_from_this()));
    *request = ret;
    return true;
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
EffectInstance::Implementation::getCombinedScale(unsigned int mipMapLevel, const RenderScale& proxyScale)
{
    RenderScale ret = proxyScale;
    double mipMapScale = Image::getScaleFromMipMapLevel(mipMapLevel);
    ret.x *= mipMapScale;
    ret.y *= mipMapScale;
    return ret;
}

ActionRetCodeEnum
EffectInstance::Implementation::resolveRenderBackend(const FrameViewRequestPtr& requestPassData, const RectI& roi, RenderBackendTypeEnum* renderBackend)
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
            _publicInterface->setPersistentMessage(eMessageTypeError, message.toStdString());
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
                if (requestPassData->getListeners().size() > 1) {
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
EffectInstance::Implementation::shouldRenderUseCache(const FrameViewRequestPtr& requestPassData)
{
    bool retSet = false;
    CacheAccessModeEnum ret = eCacheAccessModeNone;

    // A writer never caches!
    if (!retSet && _publicInterface->isWriter()) {
        retSet = true;
        ret = eCacheAccessModeNone;
    }

    if (!retSet) {
        NodePtr treeRoot = _publicInterface->getCurrentRender()->getTreeRoot();
        if (treeRoot->getEffectInstance().get() == _publicInterface)  {
            // Always cache the root node because a subsequent render may ask for it
            ret = eCacheAccessModeReadWrite;
            retSet = true;
        }
    }

    if (!retSet) {
        const bool isFrameVaryingOrAnimated = _publicInterface->isFrameVarying();
        const int requestsCount = requestPassData->getListeners().size();

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
EffectInstance::Implementation::tiledRenderingFunctorInSeparateThread(const RectToRender & rectToRender,
                                                                      const TiledRenderingFunctorArgs& args,
                                                                      QThread* spawnerThread)
{
    ///Make the thread-storage live as long as the render action is called if we're in a newly launched thread in eRenderSafetyFullySafeFrame mode
    QThread* curThread = QThread::currentThread();

    boost::scoped_ptr<FrameViewRequestSetter_RAII> tlsSetFrameViewRequest;
    if (spawnerThread != curThread) {

        // Set the frame view request on the TLS
        EffectInstanceTLSDataPtr tls = _publicInterface->getTLSObject();
        assert(tls);
        tlsSetFrameViewRequest.reset(new FrameViewRequestSetter_RAII(tls, args.requestData));
    }


    ActionRetCodeEnum ret = tiledRenderingFunctor(rectToRender, args);

    //Exit of the host frame threading thread
    if (spawnerThread != curThread) {
        appPTR->getAppTLS()->cleanupTLSForThread();
    }

    return ret;
} // tiledRenderingFunctorInSeparateThread


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
    if (rectToRender.backendType == eRenderBackendTypeOpenGL) {
        
        assert(args.glContext);

        GLuint fboID = args.glContext->getOrCreateFBOId();
        GL_GPU::BindFramebuffer(GL_FRAMEBUFFER, fboID);
        glCheckError(GL_GPU);
    }

    RenderScale combinedScale = EffectInstance::Implementation::getCombinedScale(args.requestData->getRenderMappedMipMapLevel(), render->getProxyScale());
    
    // If this tile is identity, copy input image instead
    ActionRetCodeEnum stat;
    if (rectToRender.identityInputNumber != -1) {
        stat = renderHandlerIdentity(rectToRender, args);
    } else {
        stat = renderHandlerPlugin(tls, rectToRender, combinedScale, args);
        if (isFailureRetCode(stat)) {
            return stat;
        }
        // Apply post-processing
        renderHandlerPostProcess(rectToRender, args);
    }


    // The render went OK: copy the temporary image with the plug-in preferred format to the cache image
    for (std::map<ImagePlaneDesc, ImagePtr>::const_iterator it = args.producedImagePlanes.begin(); it != args.producedImagePlanes.end(); ++it) {

        std::map<ImagePlaneDesc, ImagePtr>::const_iterator foundTmpImage = rectToRender.tmpRenderPlanes.find(it->first);
        if (foundTmpImage == rectToRender.tmpRenderPlanes.end()) {
            assert(false);
            continue;
        }

        Image::CopyPixelsArgs cpyArgs;
        cpyArgs.roi = rectToRender.rect;
        it->second->copyPixels(*foundTmpImage->second, cpyArgs);
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

    for (std::map<ImagePlaneDesc, ImagePtr>::const_iterator it = rectToRender.tmpRenderPlanes.begin(); it != rectToRender.tmpRenderPlanes.end(); ++it) {
        boost::scoped_ptr<EffectInstance::GetImageInArgs> inArgs( new EffectInstance::GetImageInArgs() );
        inArgs->renderBackend = &rectToRender.backendType;
        inArgs->requestData = args.requestData;
        inArgs->inputTime = rectToRender.identityTime;
        inArgs->inputView = rectToRender.identityView;
        inArgs->inputMipMapLevel = args.requestData->getRenderMappedMipMapLevel();
        inArgs->inputProxyScale = render->getProxyScale();
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
        cpyArgs.roi = rectToRender.rect;
        it->second->copyPixels(*inputResults.image, cpyArgs);
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
        assert(image->getStorageMode() == eStorageModeDisk || image->getStorageMode() == eStorageModeRAM);
        assert(image->getBufferFormat() == eImageBufferLayoutRGBAPackedFullRect);

        Image::Tile tile;
        bool ok = image->getTileAt(0, 0, &tile);
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
EffectInstance::Implementation::renderHandlerPlugin(const RectToRender & rectToRender,
                                                    const RenderScale& combinedScale,
                                                    const TiledRenderingFunctorArgs& args)
{

    TreeRenderPtr render = _publicInterface->getCurrentRender();
    RenderActionArgs actionArgs;
    {
        assert(args.requestData->getComponentsResults());
        actionArgs.processChannels = args.requestData->getComponentsResults()->getProcessChannels();
        actionArgs.renderScale = combinedScale;
        actionArgs.backendType = rectToRender.backendType;
        actionArgs.roi = rectToRender.rect;
        actionArgs.time = args.requestData->getTime();
        actionArgs.view = args.requestData->getView();
        actionArgs.glContext = args.glContext;
        actionArgs.requestData = args.requestData;
        actionArgs.glContextData = args.glContextData;
    }

    std::list< std::list<std::pair<ImagePlaneDesc, ImagePtr> > > planesLists;

    bool multiPlanar = _publicInterface->isMultiPlanar();
    // If we can render all planes at once, do it, otherwise just render them all sequentially
    if (!multiPlanar) {
        for (std::map<ImagePlaneDesc, ImagePtr>::const_iterator it = rectToRender.tmpRenderPlanes.begin(); it != rectToRender.tmpRenderPlanes.end(); ++it) {
            std::list<std::pair<ImagePlaneDesc, ImagePtr> > tmp;
            tmp.push_back(*it);
            planesLists.push_back(tmp);
        }
    } else {
        std::list<std::pair<ImagePlaneDesc, ImagePtr> > tmp;
        for (std::map<ImagePlaneDesc, ImagePtr>::const_iterator it = rectToRender.tmpRenderPlanes.begin(); it != rectToRender.tmpRenderPlanes.end(); ++it) {
            tmp.push_back(*it);
        }
        planesLists.push_back(tmp);
    }

    for (std::list<std::list<std::pair<ImagePlaneDesc, ImagePtr> > >::iterator it = planesLists.begin(); it != planesLists.end(); ++it) {

        actionArgs.outputPlanes = *it;

        const ImagePtr& mainImagePlane = actionArgs.outputPlanes.front().second;
        if (rectToRender.backendType == eRenderBackendTypeOpenGL ||
            rectToRender.backendType == eRenderBackendTypeOSMesa) {

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

        if (rectToRender.backendType == eRenderBackendTypeOpenGL ||
            rectToRender.backendType == eRenderBackendTypeOSMesa) {
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


void
EffectInstance::Implementation::renderHandlerPostProcess(const RectToRender & rectToRender,
                                                         const TiledRenderingFunctorArgs& args)
{


    // Get the mask image if host masking is needed
    ImagePtr maskImage;

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

                GetImageInArgs inArgs(args.requestData, &rectToRender.backendType);
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
        mix = _publicInterface->getHostMixingValue(args.requestData->getTime(), args.requestData->getView());
        useMaskMix = true;
    }

    const bool checkNaNs = _publicInterface->getCurrentRender()->isNaNHandlingEnabled();

    // Check for NaNs, copy to output image and mark for rendered
    for (std::map<ImagePlaneDesc, ImagePtr>::const_iterator it = rectToRender.tmpRenderPlanes.begin(); it != rectToRender.tmpRenderPlanes.end(); ++it) {

        if (checkNaNs) {

            if (it->second->checkForNaNs(rectToRender.rect)) {
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
        } // checkNaNs

        ImagePtr mainInputImage;

        if (mainInputNb != -1) {
            // Get the main input image to copy channels from it if a RGBA checkbox is unchecked

            std::map<int, std::list<ImagePlaneDesc> >::const_iterator foundNeededLayers = inputPlanesNeeded.find(mainInputNb);

            GetImageInArgs inArgs(args.requestData, &rectToRender.backendType);
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
                it->second->copyUnProcessedChannels(rectToRender.rect, processChannels, mainInputImage);
            }
        }
        

        if (useMaskMix) {
            it->second->applyMaskMix(rectToRender.rect, maskImage, mainInputImage, maskImage.get() /*masked*/, false /*maskInvert*/, mix);
        }

        
    } // for each plane to render
    
} // renderHandlerPostProcess


NATRON_NAMESPACE_EXIT;


