/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
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

#include "ViewerDisplayScheduler.h"

#include "Engine/AppInstance.h"
#include "Engine/TimeLine.h"
#include "Engine/Image.h"
#include "Engine/ImageCacheEntry.h"
#include "Engine/Node.h"
#include "Engine/RenderEngine.h"
#include "Engine/RotoStrokeItem.h"
#include "Engine/Settings.h"
#include "Engine/TreeRender.h"
#include "Engine/ViewerNode.h"
#include "Engine/ViewerInstance.h"

NATRON_NAMESPACE_ENTER

NATRON_NAMESPACE_ANONYMOUS_ENTER

static unsigned
getViewerMipMapLevel(const ViewerNodePtr& viewer, bool draftModeEnabled, bool fullFrameProcessing)
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


static ImagePtr
convertImageForViewerDisplay(const RectI& bounds,
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
} // convertImageForViewerDisplay


NATRON_NAMESPACE_ANONYMOUS_EXIT


ActionRetCodeEnum
ViewerRenderFrameSubResult::waitForResultsReady(const TreeRenderQueueProviderPtr& provider)
{
    for (int i = 0; i < 2; ++i) {
        if (perInputsData[i].render) {
            ActionRetCodeEnum stat = provider->waitForRenderFinished(perInputsData[i].render);
            if (isFailureRetCode(stat)) {
                return stat;
            }
            onTreeRenderFinished(i);
        }
    }
    return eActionStatusOK;
}

void
ViewerRenderFrameSubResult::launchRenders(const TreeRenderQueueProviderPtr& provider)
{
    for (int i = 0; i < 2; ++i) {
        if (perInputsData[i].render) {
            provider->launchRender(perInputsData[i].render);
        }
    }
}

void
ViewerRenderFrameSubResult::abortRender()
{
    for (int i = 0; i < 2; ++i) {
        if (perInputsData[i].render) {
            perInputsData[i].render->setRenderAborted();
        }
    }
}

ViewerDisplayScheduler::ViewerDisplayScheduler(const RenderEnginePtr& engine,
                                               const NodePtr& viewer)
: OutputSchedulerThread(engine, viewer)
{
    setProcessFrameEnabled(true, eProcessFrameByMainThread);
}


// make_shared enabler (because make_shared needs access to the private constructor)
// see https://stackoverflow.com/a/20961251/2607517
struct ViewerDisplayScheduler::MakeSharedEnabler: public ViewerDisplayScheduler
{
    MakeSharedEnabler(const RenderEnginePtr& engine,
                      const NodePtr& viewer) : ViewerDisplayScheduler(engine, viewer) {
    }
};


OutputSchedulerThreadPtr
ViewerDisplayScheduler::create(const RenderEnginePtr& engine,
                               const NodePtr& viewer)
{
    return boost::make_shared<ViewerDisplayScheduler::MakeSharedEnabler>(engine, viewer);
}


ViewerDisplayScheduler::~ViewerDisplayScheduler()
{
}


void
ViewerDisplayScheduler::timelineGoTo(TimeValue time)
{
    ViewerNodePtr isViewer = getOutputNode()->isEffectViewerNode();
    assert(isViewer);
    isViewer->getTimeline()->seekFrame(time, true, isViewer, eTimelineChangeReasonPlaybackSeek);
}

TimeValue
ViewerDisplayScheduler::timelineGetTime() const
{
    ViewerNodePtr isViewer = getOutputNode()->isEffectViewerNode();
    return TimeValue(isViewer->getTimeline()->currentFrame());
}

void
ViewerDisplayScheduler::getFrameRangeToRender(TimeValue &first,
                                              TimeValue &last) const
{
    ViewerNodePtr isViewer = getOutputNode()->isEffectViewerNode();
    ViewerNodePtr leadViewer = isViewer->getApp()->getLastViewerUsingTimeline();
    ViewerNodePtr v = leadViewer ? leadViewer : isViewer;
    assert(v);
    int left, right;
    v->getTimelineBounds(&left, &right);
    first = TimeValue(left);
    last = TimeValue(right);
}



static ActionRetCodeEnum createFrameRenderResultsForView(const ViewerNodePtr& viewer,
                                                         const TreeRenderQueueProviderPtr& provider,
                                                         const ViewerRenderFrameResultsContainerPtr& results,
                                                         ViewIdx view,
                                                         const RenderStatsPtr& stats,
                                                         bool isPlayback,
                                                         const RectD* partialUpdateRoIParam,
                                                         unsigned int mipMapLevel,
                                                         ViewerCompositingOperatorEnum viewerBlend,
                                                         bool byPassCache,
                                                         bool draftModeEnabled,
                                                         bool fullFrameProcessing,
                                                         bool viewerBEqualsViewerA,
                                                         const RotoStrokeItemPtr& activeDrawingStroke)
{

    // Initialize for each view a sub-result.
    // Each view has 2 renders: the A and B viewerprocess
    ViewerRenderFrameSubResultPtr subResult(new ViewerRenderFrameSubResult);
    results->frames.push_back(subResult);
    subResult->view = view;
    subResult->stats = stats;

    if (partialUpdateRoIParam) {
        subResult->textureTransferType = OpenGLViewerI::TextureTransferArgs::eTextureTransferTypeOverlay;
    } else if (activeDrawingStroke && activeDrawingStroke->getRenderCloneCurrentStrokeStartPointIndex() > 0) {
        // Upon painting ticks, we just have to update the viewer for the area that was painted
        subResult->textureTransferType = OpenGLViewerI::TextureTransferArgs::eTextureTransferTypeModify;
    } else {
        subResult->textureTransferType = OpenGLViewerI::TextureTransferArgs::eTextureTransferTypeReplace;
    }

    for (int viewerInputIndex = 0; viewerInputIndex < 2; ++viewerInputIndex) {
        subResult->perInputsData[viewerInputIndex].retCode = eActionStatusFailed;
        if (viewerInputIndex == 1 && (viewerBEqualsViewerA || viewerBlend == eViewerCompositingOperatorNone)) {
            if (viewerBEqualsViewerA && viewerBlend != eViewerCompositingOperatorNone) {
                subResult->copyInputBFromA = true;
            }
            continue;
        }
        if (viewer->isViewerPaused(viewerInputIndex)) {
            subResult->perInputsData[viewerInputIndex].retCode = eActionStatusAborted;
            continue;
        }
        ViewerInstancePtr viewerProcess = viewer->getViewerProcessNode(viewerInputIndex);
        subResult->perInputsData[viewerInputIndex].viewerProcessNode = viewerProcess->getNode();



        TreeRender::CtorArgsPtr initArgs(new TreeRender::CtorArgs);
        initArgs->treeRootEffect = viewerProcess;
        initArgs->provider = provider;
        initArgs->time = results->time;
        initArgs->view = subResult->view;

        // Render by default on disk is always using a mipmap level of 0 but using the proxy scale of the project
        initArgs->mipMapLevel = mipMapLevel;


#pragma message WARN("Todo: set proxy scale here")
        initArgs->proxyScale = RenderScale(1.);

        // Render the RoD if fullframe processing
        if (partialUpdateRoIParam) {
            initArgs->canonicalRoI = *partialUpdateRoIParam;
        } else {
            RectD roi;
            if (!fullFrameProcessing) {
                roi = viewerProcess->getViewerRoI();
            }
            initArgs->canonicalRoI = roi;

        }
        
        initArgs->stats = stats;
        initArgs->activeRotoDrawableItem = activeDrawingStroke;
        initArgs->draftMode = draftModeEnabled;
        initArgs->playback = isPlayback;
        initArgs->byPassCache = byPassCache;
        initArgs->preventConcurrentTreeRenders = (activeDrawingStroke || partialUpdateRoIParam);
        if (!isPlayback && subResult->textureTransferType == OpenGLViewerI::TextureTransferArgs::eTextureTransferTypeReplace && !activeDrawingStroke) {
            subResult->perInputsData[viewerInputIndex].colorPickerNode = viewerInputIndex == 0 ? viewer->getCurrentAInput() : viewer->getCurrentBInput();
            if (subResult->perInputsData[viewerInputIndex].colorPickerNode) {
                // Also sample the "main" input of the color picker node, this is useful for keyers.
                int mainInput = subResult->perInputsData[viewerInputIndex].colorPickerNode->getPreferredInput();
                subResult->perInputsData[viewerInputIndex].colorPickerInputNode = subResult->perInputsData[viewerInputIndex].colorPickerNode->getInput(mainInput);
            }
        }
        if (subResult->perInputsData[viewerInputIndex].colorPickerNode) {
            initArgs->extraNodesToSample.push_back(subResult->perInputsData[viewerInputIndex].colorPickerNode);
        }
        if (subResult->perInputsData[viewerInputIndex].colorPickerInputImage) {
            initArgs->extraNodesToSample.push_back(subResult->perInputsData[viewerInputIndex].colorPickerInputNode);
        }

        subResult->perInputsData[viewerInputIndex].render = TreeRender::create(initArgs);
        if (!subResult->perInputsData[viewerInputIndex].render) {
            return eActionStatusFailed;
        }

    } // for each viewer input
    return eActionStatusOK;
} // createFrameRenderResultsForView

ActionRetCodeEnum
ViewerDisplayScheduler::createFrameRenderResultsGeneric(const ViewerNodePtr& viewer,
                                                        const TreeRenderQueueProviderPtr& provider,
                                                        TimeValue time,
                                                        bool isPlayback,
                                                        const RotoStrokeItemPtr& activeDrawingStroke,
                                                        const std::vector<ViewIdx>& viewsToRender,
                                                        bool enableRenderStats,
                                                        RenderFrameResultsContainerPtr* future)
{
    // A global statistics object for this frame render if requested
    RenderStatsPtr stats;
    if (enableRenderStats) {
        stats.reset( new RenderStats(enableRenderStats) );
    }


    // Get parameters from the viewport
    bool fullFrameProcessing = viewer->isFullFrameProcessingEnabled();
    bool draftModeEnabled = viewer->getApp()->isDraftRenderEnabled();
    unsigned int mipMapLevel = getViewerMipMapLevel(viewer, draftModeEnabled, fullFrameProcessing);
    bool byPassCache = viewer->isRenderWithoutCacheEnabledAndTurnOff();
    ViewerCompositingOperatorEnum viewerBlend = viewer->getCurrentOperator();
    bool viewerBEqualsViewerA = viewer->getCurrentAInput() == viewer->getCurrentBInput();

    // Create the global results object
    ViewerRenderFrameResultsContainerPtr results(new  ViewerRenderFrameResultsContainer(provider));
    *future = results;
    results->time = time;
    results->recenterViewer = viewer->getViewerCenterPoint(&results->viewerCenter);

    std::list<RectD> rois;
    if (!viewer->isDoingPartialUpdates()) {
        rois.push_back(RectD());
    } else {

        // If the viewer is doing partial updates (i.e: during tracking we only update the markers areas)
        // Then we launch multiple renders over the partial areas
        std::list<RectD> partialUpdates = viewer->getPartialUpdateRects();
        for (std::list<RectD>::const_iterator it = partialUpdates.begin(); it != partialUpdates.end(); ++it) {
            if (!it->isNull()) {
                rois.push_back(*it);
            }
        }
    }

    for (std::list<RectD>::const_iterator it = rois.begin(); it != rois.end(); ++it) {
        // Render all requested views
        for (std::size_t view_i = 0; view_i < viewsToRender.size(); ++view_i) {
            ActionRetCodeEnum stat = createFrameRenderResultsForView(viewer, provider, results, viewsToRender[view_i], stats, isPlayback, it->isNull() ? 0 : &(*it), mipMapLevel, viewerBlend, byPassCache, draftModeEnabled, fullFrameProcessing, viewerBEqualsViewerA, activeDrawingStroke);
            if (isFailureRetCode(stat)) {
                return stat;
            }
        } // for each view

    }
    return eActionStatusOK;
} // createFrameRenderResultsGeneric


ActionRetCodeEnum
ViewerDisplayScheduler::createFrameRenderResults(TimeValue time, const std::vector<ViewIdx>& viewsToRender, bool enableRenderStats, RenderFrameResultsContainerPtr* results)
{

    ViewerNodePtr viewer = toViewerNode(getOutputNode()->getEffectInstance());
    assert(viewer);
    return createFrameRenderResultsGeneric(viewer, shared_from_this(), time, true /*isPlayback*/, RotoStrokeItemPtr(), viewsToRender, enableRenderStats, results);
} // createFrameRenderResults

void
ViewerRenderFrameSubResult::onTreeRenderFinished(int inputIndex)
{
    PerViewerInputRenderData& inputData = perInputsData[inputIndex];

    if (inputIndex == 1 && copyInputBFromA) {
        inputData = perInputsData[0];
        return;
    }

    FrameViewRequestPtr outputRequest;
    if (inputData.render) {
        inputData.retCode = inputData.render->getStatus();
        outputRequest = inputData.render->getOutputRequest();
    }
    if (outputRequest) {
        inputData.viewerProcessImage = outputRequest->getRequestedScaleImagePlane();
    }

    // There might be no output image if the RoI that was passed to render is outside of the RoD of the effect
    if (isFailureRetCode(inputData.retCode) || !inputData.viewerProcessImage) {
        inputData.viewerProcessImage.reset();
        inputData.render.reset();
        return;
    }


    // Find the key of the image and store it so that in the gui
    // we can later on re-use this key to check the cache for the timeline's cache line
    ImageCacheEntryPtr cacheEntry = inputData.viewerProcessImage->getCacheEntry();
    if (cacheEntry) {
        inputData.viewerProcessImageKey = cacheEntry->getCacheKey();
    }

    // Convert the image to a format that can be uploaded to a OpenGL texture


    RectI imageConvertRoI;
    RectD ctorCanonicalRoI = inputData.render->getCtorRoI();
    if (inputData.render && !ctorCanonicalRoI.isNull()) {
        RenderScale scale = EffectInstance::getCombinedScale(inputData.viewerProcessImage->getMipMapLevel(), inputData.viewerProcessImage->getProxyScale());
        double par = inputData.viewerProcessNode->getEffectInstance()->getAspectRatio(-1);
        ctorCanonicalRoI.toPixelEnclosing(scale, par, &imageConvertRoI);
    } else {
        imageConvertRoI = inputData.viewerProcessImage->getBounds();
    }

    // If we are drawing with the RotoPaint node, only update the texture portion
    if (textureTransferType == OpenGLViewerI::TextureTransferArgs::eTextureTransferTypeModify) {
        RectI strokeArea;
        bool strokeAreaSet = inputData.render->getRotoPaintActiveStrokeUpdateArea(&strokeArea);
        if (strokeAreaSet) {
            imageConvertRoI = strokeArea;
        }
    }

    // The viewer-process node may not have rendered a 4 channel image, but this is required but the OpenGL viewer
    // which only draws RGBA images.

    // If we are in accumulation, force a copy of the image because another render thread might modify it in a future render whilst it may
    // still be read from the main-thread when updating the ViewerGL texture.
    // If texture transfer is eTextureTransferTypeOverlay, we want to upload the texture to exactly what was requested
    const bool forceOutputImageCopy = (inputData.viewerProcessImage == inputData.viewerProcessNode->getEffectInstance()->getAccumBuffer(inputData.viewerProcessImage->getLayer()) ||
                                       (textureTransferType == OpenGLViewerI::TextureTransferArgs::eTextureTransferTypeOverlay && inputData.viewerProcessImage->getBounds() != imageConvertRoI));
    inputData.viewerProcessImage = convertImageForViewerDisplay(imageConvertRoI, forceOutputImageCopy, true /*the texture must have 4 channels*/, inputData.viewerProcessImage);

    // Extra color-picker images as-well.
    if (inputData.colorPickerNode) {
        {
            FrameViewRequestPtr req = inputData.render->getExtraRequestedResultsForNode(inputData.colorPickerNode);
            if (req) {
                inputData.colorPickerImage = req->getRequestedScaleImagePlane();
                if (inputData.colorPickerImage) {
                    inputData.colorPickerImage = convertImageForViewerDisplay(inputData.colorPickerImage->getBounds(), false, false /*the picker can accept non 4-channel image*/, inputData.colorPickerImage);
                }
            }
        }
        if (inputData.colorPickerInputNode) {
            FrameViewRequestPtr req = inputData.render->getExtraRequestedResultsForNode(inputData.colorPickerInputNode);
            if (req) {
                inputData.colorPickerInputImage = req->getRequestedScaleImagePlane();
                if (inputData.colorPickerInputImage) {
                    inputData.colorPickerInputImage = convertImageForViewerDisplay(inputData.colorPickerInputImage->getBounds(), false, false /*the picker can accept non 4-channel image*/, inputData.colorPickerInputImage);
                }
            }
        }
    }

    inputData.render.reset();
} // onTreeRenderFinished

bool
ViewerDisplayScheduler::processFramesResults(const ViewerNodePtr& viewer,const RenderFrameResultsContainerPtr& results)
{
    if ( results->frames.empty() ) {
        viewer->redrawViewer();
        return false;
    }

    ViewerRenderFrameResultsContainerPtr viewerResults = boost::dynamic_pointer_cast<ViewerRenderFrameResultsContainer>(results);
    assert(viewerResults);

    bool didSomething = false;

    for (std::list<RenderFrameSubResultPtr>::const_iterator it = viewerResults->frames.begin(); it != viewerResults->frames.end(); ++it) {
        ViewerRenderFrameSubResult* viewerObject = dynamic_cast<ViewerRenderFrameSubResult*>(it->get());
        assert(viewerObject);

        ViewerNode::UpdateViewerArgs args;
        args.time = results->time;
        args.view = (*it)->view;
        args.type = viewerObject->textureTransferType;
        args.recenterViewer = viewerResults->recenterViewer;
        args.viewerCenter = viewerResults->viewerCenter;


        for (int i = 0; i < 2; ++i) {

            const PerViewerInputRenderData& inputData = viewerObject->perInputsData[i];

            ViewerNode::UpdateViewerArgs::TextureUpload upload;
            upload.image = inputData.viewerProcessImage;
            upload.colorPickerImage = inputData.colorPickerImage;
            upload.colorPickerInputImage = inputData.colorPickerInputImage;
            upload.viewerProcessImageKey = inputData.viewerProcessImageKey;

            if (inputData.retCode == eActionStatusAborted || (inputData.retCode == eActionStatusOK && !upload.image)) {
                // If aborted or no image was rendered but the result was OK (one of the reasons could be the caller requested a RoI outside of the bounds of the image), don't transfer any texture, just redraw the viewer.
                continue;
            }
            args.viewerUploads[i].push_back(upload);
        }
        if (!args.viewerUploads[0].empty() || !args.viewerUploads[1].empty()) {
            viewer->updateViewer(args);
            didSomething = true;
        }
    }
    viewer->redrawViewer();
    return didSomething;
} // processFramesResults

ProcessFrameArgsBasePtr
ViewerDisplayScheduler::createProcessFrameArgs(const OutputSchedulerThreadStartArgsPtr& /*runArgs*/, const RenderFrameResultsContainerPtr& results)
{
    ProcessFrameArgsBasePtr ret(new ProcessFrameArgsBase);
    ret->results = results;
    return ret;
}

void
ViewerDisplayScheduler::processFrame(const ProcessFrameArgsBase& args)
{
    ViewerNodePtr isViewer = getOutputNode()->isEffectViewerNode();
    assert(isViewer);
    processFramesResults(isViewer, args.results);

} // processFrame



void
ViewerDisplayScheduler::onFrameProcessed(const ProcessFrameArgsBase& /*args*/)
{
    // Do nothing
} // onFrameProcessed


void
ViewerDisplayScheduler::onRenderFailed(ActionRetCodeEnum status)
{
    // Upon failure clear the viewer to black. The node that failed should have posted a persistent message.
    if (status == eActionStatusAborted) {
        // When aborted, do not clear the viewer
        return;
    }

    ViewerNodePtr effect = getOutputNode()->isEffectViewerNode();
    effect->disconnectViewer();
}

void
ViewerDisplayScheduler::onRenderStopped(bool /*/aborted*/)
{
    // Refresh all previews in the tree
    NodePtr effect = getOutputNode();
    
    effect->getApp()->refreshAllPreviews();
    
    if ( effect->getApp()->isGuiFrozen() ) {
        getEngine()->s_refreshAllKnobs();
    }
}

TimeValue
ViewerDisplayScheduler::getLastRenderedTime() const
{
    ViewerNodePtr effect = getOutputNode()->isEffectViewerNode();
    return TimeValue(effect->getLastRenderedTime());
}

NATRON_NAMESPACE_EXIT
