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

#ifndef NATRON_ENGINE_VIEWERDISPLAYSCHEDULER_H
#define NATRON_ENGINE_VIEWERDISPLAYSCHEDULER_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/make_shared.hpp>
#endif

#include "Engine/OutputSchedulerThread.h"
#include "Engine/OpenGLViewerI.h"


NATRON_NAMESPACE_ENTER



struct PerViewerInputRenderData
{
    TreeRenderPtr render;
    ImagePtr viewerProcessImage, colorPickerImage, colorPickerInputImage;
    ActionRetCodeEnum retCode;
    ImageCacheKeyPtr viewerProcessImageKey;
    NodePtr viewerProcessNode;
    NodePtr colorPickerNode;
    NodePtr colorPickerInputNode;

    PerViewerInputRenderData()
    : render()
    , viewerProcessImage()
    , colorPickerImage()
    , colorPickerInputImage()
    , retCode(eActionStatusFailed)
    , viewerProcessImageKey()
    , viewerProcessNode()
    , colorPickerNode()
    , colorPickerInputNode()
    {

    }
};


class ViewerRenderFrameSubResult : public RenderFrameSubResult
{
public:

    ViewerRenderFrameSubResult()
    : RenderFrameSubResult()
    , textureTransferType(OpenGLViewerI::TextureTransferArgs::eTextureTransferTypeReplace)
    , copyInputBFromA(false)
    , perInputsData()
    {

    }

    virtual ~ViewerRenderFrameSubResult() {}

    virtual ActionRetCodeEnum waitForResultsReady(const TreeRenderQueueProviderPtr& provider) OVERRIDE FINAL;

    virtual void launchRenders(const TreeRenderQueueProviderPtr& provider) OVERRIDE FINAL;

    virtual void abortRender() OVERRIDE FINAL;

    void onTreeRenderFinished(int index);

public:

    OpenGLViewerI::TextureTransferArgs::TypeEnum textureTransferType;
    bool copyInputBFromA;
    PerViewerInputRenderData perInputsData[2];
};

class ViewerRenderFrameResultsContainer : public RenderFrameResultsContainer
{
public:

    ViewerRenderFrameResultsContainer(const TreeRenderQueueProviderPtr& provider)
    : RenderFrameResultsContainer(provider)
    , recenterViewer(0)
    , viewerCenter()
    {

    }

    virtual ~ViewerRenderFrameResultsContainer() {}

    bool recenterViewer;
    Point viewerCenter;
};

typedef boost::shared_ptr<ViewerRenderFrameSubResult> ViewerRenderFrameSubResultPtr;
typedef boost::shared_ptr<ViewerRenderFrameResultsContainer> ViewerRenderFrameResultsContainerPtr;


/**
 * @brief An OutputSchedulerThread implementation that also update the viewer with the rendered image
 **/
class ViewerDisplayScheduler
: public OutputSchedulerThread
{
    friend boost::shared_ptr<ViewerDisplayScheduler> boost::make_shared<ViewerDisplayScheduler>(const RenderEnginePtr& engine,
                                                                               const NodePtr& viewer);

protected:
    // used by boost::make_shared<>
    ViewerDisplayScheduler(const RenderEnginePtr& engine,
                           const NodePtr& viewer);

public:
    static OutputSchedulerThreadPtr create(const RenderEnginePtr& engine,
                                           const NodePtr& viewer)
    {
        return boost::make_shared<ViewerDisplayScheduler>(engine, viewer);
    }


    virtual ~ViewerDisplayScheduler();

    /**
     * @brief Uploads the given results to the viewer.
     * @returns True if something was done, false otherwise.
     **/
    static bool processFramesResults(const ViewerNodePtr& viewer,const RenderFrameResultsContainerPtr& results);

    /**
     * @brief Generic function for the viewer to launch a render. Used by CurrentFrameRequestScheduler and
     * ViewerDisplayScheduler
     **/
    static ActionRetCodeEnum createFrameRenderResultsGeneric(const ViewerNodePtr& viewer,
                                                       const TreeRenderQueueProviderPtr& provider,
                                                       TimeValue time,
                                                       bool isPlayback,
                                                       const RotoStrokeItemPtr& activeDrawingStroke,
                                                       const std::vector<ViewIdx>& viewsToRender,
                                                       bool enableRenderStats,
                                                       RenderFrameResultsContainerPtr* results) ;

private:

    virtual ProcessFrameArgsBasePtr createProcessFrameArgs(const OutputSchedulerThreadStartArgsPtr& runArgs, const RenderFrameResultsContainerPtr& results) OVERRIDE FINAL;

    // Overriden from ProcessFrameI
    virtual void processFrame(const ProcessFrameArgsBase& args) OVERRIDE FINAL;
    virtual void onFrameProcessed(const ProcessFrameArgsBase& args) OVERRIDE FINAL;


    virtual void timelineGoTo(TimeValue time) OVERRIDE FINAL;
    virtual TimeValue timelineGetTime() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool isFPSRegulationNeeded() const OVERRIDE FINAL WARN_UNUSED_RETURN { return true; }

    virtual void getFrameRangeToRender(TimeValue& first, TimeValue& last) const OVERRIDE FINAL;

    virtual ActionRetCodeEnum createFrameRenderResults(TimeValue time, const std::vector<ViewIdx>& viewsToRender, bool enableRenderStats, RenderFrameResultsContainerPtr* results) OVERRIDE;

    virtual void onRenderFailed(ActionRetCodeEnum status) OVERRIDE FINAL;

    virtual TimeValue getLastRenderedTime() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void onRenderStopped(bool aborted) OVERRIDE FINAL;
};



NATRON_NAMESPACE_EXIT

#endif // VIEWERDISPLAYSCHEDULER_H
