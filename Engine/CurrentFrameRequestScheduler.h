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

#ifndef NATRON_ENGINE_CURRENTFRAMEREQUESTSCHEDULER_H
#define NATRON_ENGINE_CURRENTFRAMEREQUESTSCHEDULER_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#endif

#include "Engine/EngineFwd.h"
#include "Engine/GenericSchedulerThread.h"
#include "Engine/ProcessFrameThread.h"
#include "Engine/TreeRenderQueueProvider.h"

NATRON_NAMESPACE_ENTER

class CurrentFrameFunctorArgs;

/**
 * @brief The OutputSchedulerThread class (and its derivatives) are meant to be used for playback/render on disk and regulates the output ordering.
 * This class achieves kinda the same goal: it provides the ability to give it a work queue and process the work queue in the same order.
 * Typically when zooming, you want to launch as many thread as possible for each zoom increment and update the viewer in the same order that the one
 * in which you launched the thread in the first place.
 * Instead of re-using the OutputSchedulerClass and adding extra handling for special cases we separated it in a different class, specialized for this kind
 * of "current frame re-rendering" which needs much less code to run than all the code in OutputSchedulerThread.
 *
 * Note that this is not a thread, rendering is spawned in render threads in the main thread pool using RenderCurrentFrameFunctorRunnable (in the cpp)
 **/
struct ViewerCurrentFrameRequestSchedulerPrivate;
class ViewerCurrentFrameRequestScheduler
: public GenericSchedulerThread
, public ProcessFrameI
, public TreeRenderQueueProvider
, public boost::enable_shared_from_this<ViewerCurrentFrameRequestScheduler>
{
protected:

    ViewerCurrentFrameRequestScheduler(const RenderEnginePtr& renderEngine, const NodePtr& viewer);

    virtual TreeRenderQueueProviderConstPtr getThisTreeRenderQueueProviderShared() const OVERRIDE FINAL
    {
        return shared_from_this();
    }

public:

    static ViewerCurrentFrameRequestSchedulerPtr create(const RenderEnginePtr& renderEngine, const NodePtr& viewer)
    {
        return ViewerCurrentFrameRequestSchedulerPtr(new ViewerCurrentFrameRequestScheduler(renderEngine, viewer));
    }

    virtual ~ViewerCurrentFrameRequestScheduler();

    void renderCurrentFrame(bool enableRenderStats);

    virtual void onWaitForAbortCompleted() OVERRIDE FINAL;
    virtual void onWaitForThreadToQuit() OVERRIDE FINAL;
    virtual void onAbortRequested(bool keepOldestRender) OVERRIDE FINAL;
    virtual void onQuitRequested(bool allowRestarts) OVERRIDE FINAL;

    bool hasThreadsAlive() const;

    virtual void processFrame(const ProcessFrameArgsBase& args) OVERRIDE FINAL;

    virtual void onFrameProcessed(const ProcessFrameArgsBase& args) OVERRIDE FINAL;

private:

    /**
     * @brief How to pick the task to process from the consumer thread
     **/
    virtual TaskQueueBehaviorEnum tasksQueueBehaviour() const OVERRIDE FINAL
    {
        return eTaskQueueBehaviorProcessInOrder;
    }

    virtual void onTreeRenderFinished(const TreeRenderPtr& render) OVERRIDE FINAL;

    virtual ThreadStateEnum threadLoopOnce(const GenericThreadStartArgsPtr& inArgs) OVERRIDE FINAL;

    boost::scoped_ptr<ViewerCurrentFrameRequestSchedulerPrivate> _imp;
};



NATRON_NAMESPACE_EXIT


#endif // NATRON_ENGINE_CURRENTFRAMEREQUESTSCHEDULER_H
