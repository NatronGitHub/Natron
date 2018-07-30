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



#include "CurrentFrameRequestScheduler.h"

#include <set>


GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
// /usr/local/include/boost/bind/arg.hpp:37:9: warning: unused typedef 'boost_static_assert_typedef_37' [-Wunused-local-typedef]
#include <boost/bind.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/algorithm/clamp.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON

#include <QWaitCondition>
#include <QtConcurrentRun>
#include <QMutex>

#include "Engine/AppInstance.h"
#include "Engine/Node.h"
#include "Engine/RotoStrokeItem.h"
#include "Engine/RenderEngine.h"
#include "Engine/TimeLine.h"
#include "Engine/TreeRender.h"
#include "Engine/TreeRenderQueueManager.h"
#include "Engine/ViewerNode.h"
#include "Engine/ViewerDisplayScheduler.h"

NATRON_NAMESPACE_ENTER

class RenderCurrentFrameFunctorRunnable;


class ViewerCurrentFrameRenderProcessFrameArgs : public ProcessFrameArgsBase
{
public:

    U64 age;

    ViewerCurrentFrameRenderProcessFrameArgs()
    : ProcessFrameArgsBase()
    , age(0)
    {

    }

    virtual ~ViewerCurrentFrameRenderProcessFrameArgs()
    {
        
    }
};

struct RenderAndAge
{
    RenderFrameResultsContainerPtr results;

    // For each input whether the render is finished
    bool finishedRenders[2];
};

typedef std::map<U64, RenderAndAge> RendersMap;

struct ViewerCurrentFrameRequestSchedulerPrivate
{
    ViewerCurrentFrameRequestScheduler* _publicInterface;
    RenderEngineWPtr renderEngine;
    NodeWPtr viewer;


    mutable QMutex currentRendersMutex;
    // A set of active renders and their age.
    RendersMap currentRenders;

    mutable QMutex renderAgeMutex; // protects renderAge displayAge currentRenders

    // This is the age to attribute to the next incomming render
    U64 renderAge;

    // This is the age of the last render attributed. If 0 then no render has been displayed yet.
    U64 displayAge;

    ProcessFrameThread processFrameThread;

    ViewerCurrentFrameRequestSchedulerPrivate(ViewerCurrentFrameRequestScheduler* publicInterface, const RenderEnginePtr& renderEngine, const NodePtr& viewer)
    : _publicInterface(publicInterface)
    , renderEngine(renderEngine)
    , viewer(viewer)
    , currentRendersMutex()
    , currentRenders()
    , renderAge(1)
    , displayAge(0)
    , processFrameThread()
    {
    }

    U64 getRenderAgeAndIncrement();

};


ViewerCurrentFrameRequestScheduler::ViewerCurrentFrameRequestScheduler(const RenderEnginePtr& renderEngine, const NodePtr& viewer)
: _imp( new ViewerCurrentFrameRequestSchedulerPrivate(this, renderEngine, viewer) )
{
}

ViewerCurrentFrameRequestScheduler::~ViewerCurrentFrameRequestScheduler()
{
   
}

void
ViewerCurrentFrameRequestScheduler::processFrame(const ProcessFrameArgsBase& inArgs)
{
    assert(QThread::currentThread() == qApp->thread());
    const ViewerCurrentFrameRenderProcessFrameArgs* args = dynamic_cast<const ViewerCurrentFrameRenderProcessFrameArgs*>(&inArgs);
    assert(args);

    // Do not process the produced frame if the age is now older than what is displayed
    {
        QMutexLocker k(&_imp->renderAgeMutex);
        if (args->age <= _imp->displayAge) {
            return;
        }
    }

    ViewerNodePtr viewerNode = _imp->viewer.lock()->isEffectViewerNode();

    bool didSomething = ViewerDisplayScheduler::processFramesResults(viewerNode, args->results);
    if (didSomething) {
        // Update the display age
        QMutexLocker k(&_imp->renderAgeMutex);
        _imp->displayAge = args->age;
    }
} // processFrame

void
ViewerCurrentFrameRequestScheduler::onFrameProcessed(const ProcessFrameArgsBase& args)
{
    const RenderFrameResultsContainerPtr& results = args.results;

    // Report render stats if desired
    RenderEnginePtr engine = _imp->renderEngine.lock();
    
    for (std::list<RenderFrameSubResultPtr>::const_iterator it = results->frames.begin(); it != results->frames.end(); ++it) {
        if ((*it)->stats && (*it)->stats->isInDepthProfilingEnabled()) {
            engine->reportStats(results->time , (*it)->stats);
        }
        
    }
}

void
ViewerCurrentFrameRequestScheduler::onAbortRequested(bool keepOldestRender)
{
#ifdef TRACE_CURRENT_FRAME_SCHEDULER
    qDebug() << getThreadName().c_str() << "Received abort request";
#endif

    ViewerNodePtr viewerNode = _imp->viewer.lock()->isEffectViewerNode();

    // Do not abort the oldest render while scrubbing timeline or sliders so that the user gets some feedback
    const bool keepOldest = viewerNode->getApp()->isDraftRenderEnabled() || viewerNode->isDoingPartialUpdates() || keepOldestRender;


    //Do not abort the oldest render, let it finish
    bool keptOneRenderActive = !keepOldest;

    // Only keep the oldest render active if it is worth displaying
    U64 currentDisplayAge;
    {
        QMutexLocker k(&_imp->renderAgeMutex);
        currentDisplayAge = _imp->displayAge;
    }
    RendersMap currentRenders;
    {
        QMutexLocker k(&_imp->currentRendersMutex);
        currentRenders = _imp->currentRenders;
    }
    if ( currentRenders.empty() ) {
        return;
    }

    for (RendersMap::iterator it = currentRenders.begin(); it != currentRenders.end(); ++it) {
        if (it->first >= currentDisplayAge && !keptOneRenderActive) {
            keptOneRenderActive = true;
            continue;
        }
        it->second.results->abortRenders();
    }
    _imp->processFrameThread.abortThreadedTask();
} // onAbortRequested

void
ViewerCurrentFrameRequestScheduler::onQuitRequested(bool allowRestarts)
{
    _imp->processFrameThread.quitThread(allowRestarts);
}

void
ViewerCurrentFrameRequestScheduler::onWaitForThreadToQuit()
{
    waitForAllTreeRenders();
    _imp->processFrameThread.waitForThreadToQuit_enforce_blocking();
}

void
ViewerCurrentFrameRequestScheduler::onWaitForAbortCompleted()
{
    waitForAllTreeRenders();
    _imp->processFrameThread.waitForAbortToComplete_enforce_blocking();
}

U64
ViewerCurrentFrameRequestSchedulerPrivate::getRenderAgeAndIncrement()
{
    QMutexLocker k(&renderAgeMutex);
    U64 ret = renderAge;
    // If we reached the max amount of age, reset to 0... should never happen anyway
    if ( renderAge >= std::numeric_limits<U64>::max() ) {
        renderAge = 0;
    } else {
        ++renderAge;
    }
    return ret;
}

class CurrentFrameRequestStartArgs : public GenericThreadStartArgs
{
public:

    CurrentFrameRequestStartArgs()
    : GenericThreadStartArgs()
    {

    }

    virtual ~CurrentFrameRequestStartArgs()
    {

    }


    U64 renderAge;
    std::vector<ViewIdx> viewsToRender;
    RotoStrokeItemPtr curStroke;
    TimeValue frame;
    bool enableRenderStats;

};

void
ViewerCurrentFrameRequestScheduler::renderCurrentFrame(bool enableRenderStats)
{
    // Sanity check, also do not render viewer that are not made visible by the user
    NodePtr treeRoot = _imp->viewer.lock();
    if (!treeRoot) {
        return;
    }
    ViewerNodePtr viewerNode = treeRoot->isEffectViewerNode();
    if (!viewerNode || !viewerNode->isViewerUIVisible() ) {
        return;
    }


    boost::shared_ptr<CurrentFrameRequestStartArgs> args = boost::make_shared<CurrentFrameRequestStartArgs>();

    args->frame = TimeValue(viewerNode->getTimeline()->currentFrame());;
    args->enableRenderStats = enableRenderStats;

    // Identify this render request with an age
    args->renderAge = _imp->getRenderAgeAndIncrement();

    ViewIdx view;
    {
        int viewsCount = viewerNode->getRenderViewsCount();
        view = viewsCount > 0 ? viewerNode->getCurrentRenderView() : ViewIdx(0);
    }


    //  For now the viewer always asks to render 1 view since the interface can only allow 1 view at once per view
    args->viewsToRender.push_back(view);

    // While painting, use a single render thread and always the same thread.
    args->curStroke = treeRoot->getApp()->getActiveRotoDrawingStroke();


    // We are about to trigger a new render, cancel all other renders except the oldest so user gets some feedback.
    if (!args->curStroke) {
        onAbortRequested(true /*keepOldestRender*/);
    }

    // Launch the render in the ViewerCurrentFrameRequestScheduler thread
    startTask(args);
} // ViewerCurrentFrameRequestScheduler::renderCurrentFrame

bool
ViewerCurrentFrameRequestScheduler::hasThreadsAlive() const
{
    QMutexLocker k(&_imp->renderAgeMutex);
    return _imp->currentRenders.size() > 0;
}

void
ViewerCurrentFrameRequestScheduler::onTreeRenderFinished(const TreeRenderPtr& render)
{
    // Find in the currentRenders the results associated to this render. Note that there may be more than 1 TreeRender
    // associated to the results.
    RenderFrameResultsContainerPtr results;
    U64 renderAge = 0;

    bool allRendersFinished = false;
    {

        QMutexLocker k(&_imp->currentRendersMutex);
        for (RendersMap::iterator it = _imp->currentRenders.begin();  it!=_imp->currentRenders.end(); ++it) {
            for (std::list<RenderFrameSubResultPtr>::iterator it2 = it->second.results->frames.begin(); it2 != it->second.results->frames.end(); ++it2) {
                ViewerRenderFrameSubResult* viewerResult = dynamic_cast<ViewerRenderFrameSubResult*>(it2->get());
                assert(viewerResult);
                for (int i = 0; i < 2; ++i) {

                    if (viewerResult->perInputsData[i].render == render) {
                        results = it->second.results;
                        renderAge = it->first;
                        it->second.finishedRenders[i] = true;

                        allRendersFinished = true;
                        for (int j = 0; j < 2; ++j) {
                            if (viewerResult->perInputsData[j].render && !it->second.finishedRenders[j]) {
                                allRendersFinished = false;
                                break;
                            }
                        }
                        break;
                    }

                }
                if (results) {
                    break;
                }
            }
            if (results) {
                break;
            }
        }
    }

    if (!results) {
        return;
    }

    if (!allRendersFinished) {
        // There's multiple tree renders per render request (one for A and one for B of the Viewer) so wait for all of them to be done
        return;
    }


    {
        // Remove the current render from the abortable renders list
        QMutexLocker k(&_imp->currentRendersMutex);
        RendersMap::iterator found = _imp->currentRenders.find(renderAge);
        assert(found != _imp->currentRenders.end());
        if (found != _imp->currentRenders.end()) {
            _imp->currentRenders.erase(found);
        }

    }

    ActionRetCodeEnum stat = results->waitForRendersFinished();


    // Call updateViewer() on the main thread
    if (isFailureRetCode(stat)) {
        if (stat != eActionStatusAborted) {
            // Clear viewer to black if not aborted
            ViewerNodePtr viewerNode =  _imp->viewer.lock()->isEffectViewerNode();
            viewerNode->disconnectViewer();
        }

    } else {

        boost::shared_ptr<ViewerCurrentFrameRenderProcessFrameArgs> processArgs = boost::make_shared<ViewerCurrentFrameRenderProcessFrameArgs>();
        processArgs->age = renderAge;
        processArgs->results = results;

        ProcessFrameThreadStartArgsPtr processStartArgs = boost::make_shared<ProcessFrameThreadStartArgs>();
        processStartArgs->args = processArgs;
        processStartArgs->executeOnMainThread = true;
        processStartArgs->processor = this;
        _imp->processFrameThread.startTask(processStartArgs);
    }
} // onTreeRenderFinished

GenericSchedulerThread::ThreadStateEnum
ViewerCurrentFrameRequestScheduler::threadLoopOnce(const GenericThreadStartArgsPtr& inArgs)
{
    CurrentFrameRequestStartArgs* args = dynamic_cast<CurrentFrameRequestStartArgs*>(inArgs.get());

    ViewerNodePtr viewerNode =  _imp->viewer.lock()->isEffectViewerNode();

    RenderFrameResultsContainerPtr results;
    ActionRetCodeEnum stat = ViewerDisplayScheduler::createFrameRenderResultsGeneric(viewerNode, shared_from_this(), args->frame, false /*isPlayback*/, args->curStroke, args->viewsToRender, args->enableRenderStats, &results);

    if (isFailureRetCode(stat)) {
        return eThreadStateActive;
    }

    // Register the render
    {
        QMutexLocker k(&_imp->currentRendersMutex);
        RenderAndAge r;
        r.finishedRenders[0] = r.finishedRenders[1] = false;
        r.results = results;
        _imp->currentRenders.insert(std::make_pair(args->renderAge,r));
    }

    // Launch the render
    results->launchRenders();

    GenericSchedulerThread::ThreadStateEnum state = resolveState();
    return state;
} // threadLoopOnce

NATRON_NAMESPACE_EXIT
