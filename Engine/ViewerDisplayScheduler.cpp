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

#include "ViewerDisplayScheduler.h"

#include "Engine/AppInstance.h"
#include "Engine/TimeLine.h"
#include "Engine/Node.h"
#include "Engine/RenderEngine.h"
#include "Engine/ViewerNode.h"
#include "Engine/ViewerRenderFrameRunnable.h"

NATRON_NAMESPACE_ENTER;



ViewerDisplayScheduler::ViewerDisplayScheduler(RenderEngine* engine,
                                               const NodePtr& viewer)
: OutputSchedulerThread(engine, viewer, eProcessFrameByMainThread)
{

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

void
ViewerDisplayScheduler::processFrame(const ProcessFrameArgsBase& args)
{
    ViewerRenderBufferedFrameContainer* framesContainer = dynamic_cast<ViewerRenderBufferedFrameContainer*>(args.frames.get());
    assert(framesContainer);

    ViewerNodePtr isViewer = getOutputNode()->isEffectViewerNode();
    assert(isViewer);

    if ( framesContainer->frames.empty() ) {
        isViewer->redrawViewer();
        return;
    }

    for (std::list<BufferedFramePtr>::const_iterator it = framesContainer->frames.begin(); it != framesContainer->frames.end(); ++it) {
        ViewerRenderBufferedFrame* viewerObject = dynamic_cast<ViewerRenderBufferedFrame*>(it->get());
        assert(viewerObject);

        ViewerNode::UpdateViewerArgs args;
        args.time = framesContainer->time;
        args.view = (*it)->view;
        args.type = viewerObject->type;
        args.recenterViewer = false;


        for (int i = 0; i < 2; ++i) {
            ViewerNode::UpdateViewerArgs::TextureUpload upload;
            upload.image = viewerObject->viewerProcessImages[i];
            upload.colorPickerImage = viewerObject->colorPickerImages[i];
            upload.colorPickerInputImage = viewerObject->colorPickerInputImages[i];
            upload.viewerProcessImageKey = viewerObject->viewerProcessImageKey[i];
            if (viewerObject->retCode[i] == eActionStatusAborted ||
                (viewerObject->retCode[i] == eActionStatusOK && !upload.image)) {
                continue;
            }
            args.viewerUploads[i].push_back(upload);
        }
        if (!args.viewerUploads[0].empty() || !args.viewerUploads[1].empty()) {
            isViewer->updateViewer(args);
        }
    }
    isViewer->redrawViewer();

}

RenderThreadTask*
ViewerDisplayScheduler::createRunnable(TimeValue frame,
                                       bool useRenderStarts,
                                       const std::vector<ViewIdx>& viewsToRender)
{
    return new ViewerRenderFrameRunnable(getOutputNode(), this, frame, useRenderStarts, viewsToRender);
}



void
ViewerDisplayScheduler::handleRenderFailure(ActionRetCodeEnum /*stat*/, const std::string& /*errorMessage*/)
{
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

NATRON_NAMESPACE_EXIT;
