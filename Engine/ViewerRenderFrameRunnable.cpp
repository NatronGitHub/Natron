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

#include "ViewerRenderFrameRunnable.h"

#include <QFuture>
#include <QtConcurrentRun>


#include "Engine/AppInstance.h"
#include "Engine/Image.h"
#include "Engine/ImageCacheEntry.h"
#include "Engine/FrameViewRequest.h"
#include "Engine/Node.h"
#include "Engine/OutputSchedulerThread.h"
#include "Engine/RenderThreadTask.h"
#include "Engine/RenderEngine.h"
#include "Engine/RotoStrokeItem.h"
#include "Engine/Settings.h"
#include "Engine/ViewerInstance.h"
#include "Engine/ViewerNode.h"
#include "Engine/TreeRender.h"

NATRON_NAMESPACE_ENTER;

ViewerRenderFrameRunnable::ViewerRenderFrameRunnable(const NodePtr& viewer,
                                                     OutputSchedulerThread* scheduler,
                                                     const TimeValue frame,
                                                     const bool useRenderStarts,
                                                     const std::vector<ViewIdx>& viewsToRender)
: DefaultRenderFrameRunnable(viewer, scheduler, frame, useRenderStarts, viewsToRender)
, _viewer(viewer->isEffectViewerNode())
{
}




void ViewerRenderFrameRunnable::createRenderViewerObject(RenderViewerProcessFunctorArgs* inArgs)
{
    TreeRender::CtorArgsPtr args(new TreeRender::CtorArgs);
    args->treeRootEffect = inArgs->viewerProcessNode->getEffectInstance();
    args->time = inArgs->time;
    args->view = inArgs->view;

    // Render default plane produced
    args->plane = 0;

    // Render by default on disk is always using a mipmap level of 0 but using the proxy scale of the project
    args->mipMapLevel = inArgs->viewerMipMapLevel;

#pragma message WARN("Todo: set proxy scale here")
    args->proxyScale = RenderScale(1.);

    // Render the RoD
    args->canonicalRoI = inArgs->roi.isNull() ? 0 : &inArgs->roi;
    args->stats = inArgs->stats;
    args->draftMode = inArgs->isDraftModeEnabled;
    args->playback = inArgs->isPlayback;
    args->byPassCache = inArgs->byPassCache;
    args->activeRotoDrawableItem = inArgs->activeStrokeItem;
    if (inArgs->colorPickerNode) {
        args->extraNodesToSample.push_back(inArgs->colorPickerNode);
    }
    if (inArgs->colorPickerInputImage) {
        args->extraNodesToSample.push_back(inArgs->colorPickerInputNode);
    }

    inArgs->retCode = eActionStatusFailed;
    inArgs->renderObject = TreeRender::create(args);

}

ImagePtr ViewerRenderFrameRunnable::convertImageForViewerDisplay(const RectI& bounds,
                                                                 bool forceCopy,
                                                                 bool force4Components,
                                                                 const ImagePtr& image)
{
    if (!image) {
        return image;
    }
    if (!forceCopy && (image->getComponentsCount() == 4 || !force4Components) &&
        image->getStorageMode() == eStorageModeRAM &&
        image->getBufferFormat() == eImageBufferLayoutRGBAPackedFullRect) {
        return image;
    }
    Image::InitStorageArgs initArgs;
    initArgs.bounds = bounds;

    // Viewer textures are always RGBA: preserve layer if possible, otherwise convert to RGBA
    if (image->getLayer().getNumComponents() == 4 || !force4Components) {
        initArgs.plane = image->getLayer();
    } else {
        initArgs.plane = ImagePlaneDesc::getRGBAComponents();
    }
    initArgs.mipMapLevel = image->getMipMapLevel();
    initArgs.proxyScale = image->getProxyScale();
    initArgs.bitdepth = image->getBitDepth();
    initArgs.storage = eStorageModeRAM;
    initArgs.bufferFormat = eImageBufferLayoutRGBAPackedFullRect;
    ImagePtr mappedImage = Image::create(initArgs);
    if (!mappedImage) {
        return mappedImage;
    }
    Image::CopyPixelsArgs copyArgs;
    copyArgs.roi = initArgs.bounds;
    copyArgs.monoConversion = Image::eMonoToPackedConversionCopyToAll;
    copyArgs.forceCopyEvenIfBuffersHaveSameLayout = forceCopy;
    mappedImage->copyPixels(*image, copyArgs);
    return mappedImage;
}

void ViewerRenderFrameRunnable::launchRenderFunctor(const RenderViewerProcessFunctorArgsPtr& inArgs, const RectD* roiParam)
{
    assert(inArgs->renderObject);
    FrameViewRequestPtr outputRequest;
    inArgs->retCode = inArgs->renderObject->launchRender(&outputRequest);

    if (outputRequest) {
        inArgs->outputImage = outputRequest->getRequestedScaleImagePlane();
    }

    // There might be no output image if the RoI that was passed to render is outside of the RoD of the effect
    if (isFailureRetCode(inArgs->retCode) || !inArgs->outputImage) {
        inArgs->outputImage.reset();
        inArgs->renderObject.reset();
        return;
    }
    // Convert the image to a format that can be uploaded to a OpenGL texture
    {


        // Find the key of the image and store it so that in the gui
        // we can later on re-use this key to check the cache for the timeline's cache line
        ImageCacheEntryPtr cacheEntry = inArgs->outputImage->getCacheEntry();
        if (cacheEntry) {
            inArgs->viewerProcessImageCacheKey = cacheEntry->getCacheKey();
        }


        RectI imageConvertRoI;
        if (roiParam) {
            RenderScale scale = EffectInstance::getCombinedScale(inArgs->outputImage->getMipMapLevel(), inArgs->outputImage->getProxyScale());
            double par = inArgs->viewerProcessNode->getEffectInstance()->getAspectRatio(-1);
            roiParam->toPixelEnclosing(scale, par, &imageConvertRoI);
        } else {
            imageConvertRoI = inArgs->outputImage->getBounds();
        }
        // If we are drawing with the RotoPaint node, only update the texture portion
        if (inArgs->type == OpenGLViewerI::TextureTransferArgs::eTextureTransferTypeModify) {
            RectI strokeArea;
            bool strokeAreaSet = inArgs->renderObject->getRotoPaintActiveStrokeUpdateArea(&strokeArea);
            if (strokeAreaSet) {
                imageConvertRoI = strokeArea;
            }
        }

        // The viewer-process node may not have rendered a 4 channel image, but this is required but the OpenGL viewer
        // which only draws RGBA images.

        // If we are in accumulation, force a copy of the image because another render thread might modify it in a future render whilst it may
        // still be read from the main-thread when updating the ViewerGL texture.
        const bool forceOutputImageCopy = inArgs->outputImage == inArgs->viewerProcessNode->getEffectInstance()->getAccumBuffer(inArgs->outputImage->getLayer());
        inArgs->outputImage = convertImageForViewerDisplay(imageConvertRoI, forceOutputImageCopy, true /*the texture must have 4 channels*/, inArgs->outputImage);

        // Extra color-picker images as-well.
        if (inArgs->colorPickerNode) {
            {
                FrameViewRequestPtr req = inArgs->renderObject->getExtraRequestedResultsForNode(inArgs->colorPickerNode);
                if (req) {
                    inArgs->colorPickerImage = req->getRequestedScaleImagePlane();
                    inArgs->colorPickerImage = convertImageForViewerDisplay(inArgs->colorPickerImage->getBounds(), false, false /*the picker can accept non 4-channel image*/, inArgs->colorPickerImage);
                }
            }
            if (inArgs->colorPickerInputNode) {
                FrameViewRequestPtr req = inArgs->renderObject->getExtraRequestedResultsForNode(inArgs->colorPickerInputNode);
                if (req) {
                    inArgs->colorPickerInputImage = req->getRequestedScaleImagePlane();
                    inArgs->colorPickerInputImage = convertImageForViewerDisplay(inArgs->colorPickerInputImage->getBounds(), false, false /*the picker can accept non 4-channel image*/, inArgs->colorPickerInputImage);
                }
            }
        }

    }
    inArgs->renderObject.reset();
}

unsigned ViewerRenderFrameRunnable::getViewerMipMapLevel(const ViewerNodePtr& viewer, bool draftModeEnabled, bool fullFrameProcessing)
{


    if (fullFrameProcessing) {
        return 0;
    }

    unsigned int mipMapLevel = 0;

    const double zoomFactor = viewer->getUIZoomFactor();

    int downcale_i = viewer->getDownscaleMipMapLevelKnobIndex();


    assert(downcale_i >= 0);
    if (downcale_i > 0) {
        mipMapLevel = downcale_i;
    } else {
        mipMapLevel = viewer->getMipMapLevelFromZoomFactor();
    }

    // If draft mode is enabled, compute the mipmap level according to the auto-proxy setting in the preferences
    if ( draftModeEnabled && appPTR->getCurrentSettings()->isAutoProxyEnabled() ) {
        unsigned int autoProxyLevel = appPTR->getCurrentSettings()->getAutoProxyMipMapLevel();
        if (zoomFactor > 1) {
            //Decrease draft mode at each inverse mipmaplevel level taken
            unsigned int invLevel = Image::getLevelFromScale(1. / zoomFactor);
            if (invLevel < autoProxyLevel) {
                autoProxyLevel -= invLevel;
            } else {
                autoProxyLevel = 0;
            }
        }
        mipMapLevel = (unsigned int)std::max( (int)mipMapLevel, (int)autoProxyLevel );

    }

    return mipMapLevel;
} // getViewerMipMapLevel

void ViewerRenderFrameRunnable::createRenderViewerProcessArgs(const ViewerNodePtr& viewer,
                                                              int viewerProcess_i,
                                                              TimeValue time,
                                                              ViewIdx view,
                                                              bool isPlayback,
                                                              const RenderStatsPtr& stats,
                                                              OpenGLViewerI::TextureTransferArgs::TypeEnum type,
                                                              const RotoStrokeItemPtr& activeStroke,
                                                              const RectD* roiParam,
                                                              RenderViewerProcessFunctorArgs* outArgs)
{

    bool fullFrameProcessing = viewer->isFullFrameProcessingEnabled();
    bool draftModeEnabled = viewer->getApp()->isDraftRenderEnabled();
    unsigned int mipMapLevel = getViewerMipMapLevel(viewer, draftModeEnabled, fullFrameProcessing);
    bool byPassCache = viewer->isRenderWithoutCacheEnabledAndTurnOff();

    ViewerInstancePtr viewerProcess = viewer->getViewerProcessNode(viewerProcess_i);
    outArgs->viewerProcessNode = viewerProcess->getNode();

    RectD roi;
    if (roiParam) {
        roi = *roiParam;
    } else if (!fullFrameProcessing) {
        roi = viewerProcess->getViewerRoI();
    }
    outArgs->type = type;
    outArgs->activeStrokeItem = activeStroke;
    outArgs->isPlayback = isPlayback;
    outArgs->isDraftModeEnabled = draftModeEnabled;
    outArgs->viewerMipMapLevel = mipMapLevel;
    outArgs->byPassCache = byPassCache;
    outArgs->roi = roi;
    outArgs->stats = stats;
    outArgs->time = time;
    outArgs->view = view;
    if (!isPlayback && type == OpenGLViewerI::TextureTransferArgs::eTextureTransferTypeReplace && !activeStroke) {
        outArgs->colorPickerNode = viewerProcess_i == 0 ? viewer->getCurrentAInput() : viewer->getCurrentBInput();
        if (outArgs->colorPickerNode) {
            // Also sample the "main" input of the color picker node, this is useful for keyers.
            int mainInput = outArgs->colorPickerNode->getPreferredInput();
            outArgs->colorPickerInputNode = outArgs->colorPickerNode->getInput(mainInput);
        }
    }
    createRenderViewerObject(outArgs);

}


void ViewerRenderFrameRunnable::createAndLaunchRenderInThread(const RenderViewerProcessFunctorArgsPtr& processArgs, int viewerProcess_i, TimeValue time, const RenderStatsPtr& stats, ViewerRenderBufferedFrame* bufferedFrame)
{

    createRenderViewerProcessArgs(_viewer, viewerProcess_i, time, bufferedFrame->view, true /*isPlayback*/, stats, bufferedFrame->type, RotoStrokeItemPtr(), 0 /*roiParam*/, processArgs.get());

    // Register the render so that it can be aborted in abortRenders()
    {
        QMutexLocker k(&renderObjectsMutex);

        if (processArgs->renderObject) {
            renderObjects.push_back(processArgs->renderObject);
        }

    }


    if (_viewer->isViewerPaused(viewerProcess_i)) {
        processArgs->retCode = eActionStatusAborted;
    } else {
        launchRenderFunctor(processArgs, 0);
    }

    bufferedFrame->retCode[viewerProcess_i] = processArgs->retCode;
    bufferedFrame->viewerProcessImageKey[viewerProcess_i] = processArgs->viewerProcessImageCacheKey;
    bufferedFrame->viewerProcessImages[viewerProcess_i] = processArgs->outputImage;
    bufferedFrame->colorPickerImages[viewerProcess_i] = processArgs->colorPickerImage;
    bufferedFrame->colorPickerInputImages[viewerProcess_i] = processArgs->colorPickerInputImage;

}

void ViewerRenderFrameRunnable::renderFrame(TimeValue time,
                                            const std::vector<ViewIdx>& viewsToRender,
                                            bool enableRenderStats)
{

    RenderStatsPtr stats;
    if (enableRenderStats) {
        stats.reset( new RenderStats(enableRenderStats) );
    }


    ViewerRenderBufferedFrameContainerPtr frameContainer(new  ViewerRenderBufferedFrameContainer());
    frameContainer->time = time;
    frameContainer->recenterViewer = false;

    for (std::size_t i = 0; i < viewsToRender.size(); ++i) {


        ViewerRenderBufferedFramePtr bufferObject(new ViewerRenderBufferedFrame);
        bufferObject->view = viewsToRender[i];
        bufferObject->stats = stats;
        bufferObject->type = OpenGLViewerI::TextureTransferArgs::eTextureTransferTypeReplace;

        std::vector<RenderViewerProcessFunctorArgsPtr> processArgs(2);
        for (int i = 0; i < 2; ++i) {
            processArgs[i].reset(new RenderViewerProcessFunctorArgs);
            bufferObject->retCode[i] = eActionStatusFailed;
        }

        ViewerCompositingOperatorEnum viewerBlend = _viewer->getCurrentOperator();
        bool viewerBEqualsViewerA = _viewer->getCurrentAInput() == _viewer->getCurrentBInput();

        // Launch the 2nd viewer process in a separate thread
        QFuture<void> processBFuture;
        if (!viewerBEqualsViewerA && viewerBlend != eViewerCompositingOperatorNone) {
            processBFuture = QtConcurrent::run(this,
                                               &ViewerRenderFrameRunnable::createAndLaunchRenderInThread,
                                               processArgs[1],
                                               1,
                                               time,
                                               stats,
                                               bufferObject.get());
        }

        // Launch the 1st viewer process in this thread
        createAndLaunchRenderInThread(processArgs[0], 0, time, stats, bufferObject.get());

        // Wait for the 2nd viewer process
        if (viewerBlend != eViewerCompositingOperatorNone) {
            if (!viewerBEqualsViewerA) {
                processBFuture.waitForFinished();
            } else {
                bufferObject->retCode[1] = processArgs[0]->retCode;
                bufferObject->viewerProcessImageKey[1] = processArgs[0]->viewerProcessImageCacheKey;
                bufferObject->viewerProcessImages[1] = processArgs[0]->outputImage;
                processArgs[0] = processArgs[1];
            }
        }

        // Check for failures
        for (int i = 0; i < 2; ++i) {

            // All other status code should not show an error
            if (processArgs[i]->retCode == eActionStatusFailed || processArgs[i]->retCode == eActionStatusOutOfMemory) {
                getScheduler()->notifyRenderFailure(processArgs[i]->retCode, std::string());
                break;
            }
        }

        frameContainer->frames.push_back(bufferObject);
        
        // Notify the scheduler thread which will in turn call processFrame
        
    } // for all views
    
    getScheduler()->appendToBuffer(frameContainer);
    
    if (stats) {
        getScheduler()->getEngine()->reportStats(time,  stats);
    }
    
} // renderFrame

NATRON_NAMESPACE_EXIT;
