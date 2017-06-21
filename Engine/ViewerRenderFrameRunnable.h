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


#ifndef NATRON_ENGINE_VIEWERRENDERFRAMERUNNABLE_H
#define NATRON_ENGINE_VIEWERRENDERFRAMERUNNABLE_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Engine/RenderThreadTask.h"

#include "Engine/BufferableObject.h"
#include "Engine/OpenGLViewerI.h"


NATRON_NAMESPACE_ENTER;



struct RenderViewerProcessFunctorArgs
{
    OpenGLViewerI::TextureTransferArgs::TypeEnum type;
    NodePtr viewerProcessNode;
    NodePtr colorPickerNode, colorPickerInputNode;
    RotoStrokeItemPtr activeStrokeItem;
    TreeRenderPtr renderObject;
    TimeValue time;
    ViewIdx view;
    RenderStatsPtr stats;
    ActionRetCodeEnum retCode;
    RectD roi;


    // Store the key of the first tile in the outputImage
    // so that we can later on check in the gui if
    // the image is still cached to update the cache line
    // on the timeline.
    ImageCacheKeyPtr viewerProcessImageCacheKey;
    ImagePtr outputImage;
    ImagePtr colorPickerImage, colorPickerInputImage;

    unsigned int viewerMipMapLevel;
    bool isDraftModeEnabled;
    bool isPlayback;
    bool byPassCache;

    RenderViewerProcessFunctorArgs()
    : type(OpenGLViewerI::TextureTransferArgs::eTextureTransferTypeReplace)
    , retCode(eActionStatusOK)
    {

    }
};

typedef boost::shared_ptr<RenderViewerProcessFunctorArgs> RenderViewerProcessFunctorArgsPtr;


class ViewerRenderBufferedFrame : public BufferedFrame
{
public:

    ViewerRenderBufferedFrame()
    : BufferedFrame()
    , type(OpenGLViewerI::TextureTransferArgs::eTextureTransferTypeReplace)
    , viewerProcessImages()
    , viewerProcessImageKey()
    {

    }

    virtual ~ViewerRenderBufferedFrame() {}

public:

    OpenGLViewerI::TextureTransferArgs::TypeEnum type;
    ImagePtr viewerProcessImages[2];
    ImagePtr colorPickerImages[2];
    ImagePtr colorPickerInputImages[2];
    ActionRetCodeEnum retCode[2];
    ImageCacheKeyPtr viewerProcessImageKey[2];
};

class ViewerRenderBufferedFrameContainer : public BufferedFrameContainer
{
public:

    ViewerRenderBufferedFrameContainer()
    : BufferedFrameContainer()
    , recenterViewer(0)
    , viewerCenter()
    {

    }

    virtual ~ViewerRenderBufferedFrameContainer() {}

    bool recenterViewer;
    Point viewerCenter;
};

typedef boost::shared_ptr<ViewerRenderBufferedFrame> ViewerRenderBufferedFramePtr;
typedef boost::shared_ptr<ViewerRenderBufferedFrameContainer> ViewerRenderBufferedFrameContainerPtr;


/**
 * @brief Implementation of RenderThreadTask for the viewer node. 
 **/
class ViewerRenderFrameRunnable
: public DefaultRenderFrameRunnable
{
    ViewerNodePtr _viewer;

public:

    ViewerRenderFrameRunnable(const NodePtr& viewer,
                              OutputSchedulerThread* scheduler,
                              const TimeValue frame,
                              const bool useRenderStarts,
                              const std::vector<ViewIdx>& viewsToRender);

    virtual ~ViewerRenderFrameRunnable()
    {
    }

    static void createRenderViewerObject(RenderViewerProcessFunctorArgs* inArgs);

    static ImagePtr convertImageForViewerDisplay(const RectI& bounds,
                                                 bool forceCopy,
                                                 bool force4Components,
                                                 const ImagePtr& image);

    static void launchRenderFunctor(const RenderViewerProcessFunctorArgsPtr& inArgs, const RectD* roiParam);

    static unsigned getViewerMipMapLevel(const ViewerNodePtr& viewer, bool draftModeEnabled, bool fullFrameProcessing);

    static void createRenderViewerProcessArgs(const ViewerNodePtr& viewer,
                                              int viewerProcess_i,
                                              TimeValue time,
                                              ViewIdx view,
                                              bool isPlayback,
                                              const RenderStatsPtr& stats,
                                              OpenGLViewerI::TextureTransferArgs::TypeEnum type,
                                              const RotoStrokeItemPtr& activeStroke,
                                              const RectD* roiParam,
                                              RenderViewerProcessFunctorArgs* outArgs);

private:

    virtual void renderFrame(TimeValue time,
                             const std::vector<ViewIdx>& viewsToRender,
                             bool enableRenderStats) OVERRIDE FINAL;

    void createAndLaunchRenderInThread(const RenderViewerProcessFunctorArgsPtr& processArgs, int viewerProcess_i, TimeValue time, const RenderStatsPtr& stats, ViewerRenderBufferedFrame* bufferedFrame);
};

NATRON_NAMESPACE_EXIT;

#endif // NATRON_ENGINE_VIEWERRENDERFRAMERUNNABLE_H
