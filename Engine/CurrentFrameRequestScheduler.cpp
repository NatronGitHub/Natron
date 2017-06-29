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
#include "Engine/TimeLine.h"
#include "Engine/TreeRender.h"
#include "Engine/ViewerRenderFrameRunnable.h"
#include "Engine/ViewerNode.h"

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

class CurrentFrameFunctorArgs
: public GenericThreadStartArgs
{
public:

    std::vector<ViewIdx> viewsToRender;
    TimeValue time;
    bool useStats;
    NodePtr viewer;
    ViewerCurrentFrameRequestSchedulerPrivate* scheduler;
    RotoStrokeItemPtr strokeItem;
    U64 age;

    CurrentFrameFunctorArgs()
    : GenericThreadStartArgs()
    , viewsToRender()
    , time(0)
    , useStats(false)
    , viewer()
    , scheduler(0)
    , strokeItem()
    , age(0)
    {
    }

    CurrentFrameFunctorArgs(ViewIdx view,
                            TimeValue time,
                            bool useStats,
                            const NodePtr& viewer,
                            ViewerCurrentFrameRequestSchedulerPrivate* scheduler,
                            const RotoStrokeItemPtr& strokeItem)
    : GenericThreadStartArgs()
    , viewsToRender()
    , time(time)
    , useStats(useStats)
    , viewer(viewer)
    , scheduler(scheduler)
    , strokeItem(strokeItem)
    , age(0)
    {
        viewsToRender.push_back(view);
    }

    ~CurrentFrameFunctorArgs()
    {

    }
};

struct TreeRenderAndAge
{
    // One render for each viewer process node
    std::list<TreeRenderPtr> renders;
    U64 age;
};

struct TreeRender_CompareAge
{
    bool operator() (const TreeRenderAndAge& lhs, const TreeRenderAndAge& rhs) const
    {
        return lhs.age < rhs.age;
    }
};

typedef std::set<TreeRenderAndAge, TreeRender_CompareAge> TreeRenderSetOrderedByAge;

struct ViewerCurrentFrameRequestSchedulerPrivate
{
    ViewerCurrentFrameRequestScheduler* _publicInterface;
    NodePtr viewer;
    QThreadPool* threadPool;

    /**
     * Single thread used by the ViewerCurrentFrameRequestScheduler when the global thread pool has reached its maximum
     * activity to keep the renders responsive even if the thread pool is choking.
     **/
    ViewerCurrentFrameRequestRendererBackup backupThread;
    mutable QMutex currentFrameRenderTasksMutex;
    QWaitCondition currentFrameRenderTasksCond;
    std::list<boost::shared_ptr<RenderCurrentFrameFunctorRunnable> > currentFrameRenderTasks;

    mutable QMutex renderAgeMutex; // protects renderAge displayAge currentRenders

    // This is the age to attribute to the next incomming render
    U64 renderAge;

    // This is the age of the last render attributed. If 0 then no render has been displayed yet.
    U64 displayAge;

    // A set of active renders and their age.
    TreeRenderSetOrderedByAge currentRenders;

    ProcessFrameThread processFrameThread;

    ViewerCurrentFrameRequestSchedulerPrivate(ViewerCurrentFrameRequestScheduler* publicInterface, const NodePtr& viewer)
    : _publicInterface(publicInterface)
    , viewer(viewer)
    , threadPool( QThreadPool::globalInstance() )
    , backupThread()
    , currentFrameRenderTasksCond()
    , currentFrameRenderTasks()
    , renderAge(1)
    , displayAge(0)
    , currentRenders()
    , processFrameThread()
    {
    }

    void appendRunnableTask(const boost::shared_ptr<RenderCurrentFrameFunctorRunnable>& task)
    {
        {
            QMutexLocker k(&currentFrameRenderTasksMutex);
            currentFrameRenderTasks.push_back(task);
        }
    }

    void removeRunnableTask(RenderCurrentFrameFunctorRunnable* task)
    {
        {
            QMutexLocker k(&currentFrameRenderTasksMutex);
            for (std::list<boost::shared_ptr<RenderCurrentFrameFunctorRunnable> >::iterator it = currentFrameRenderTasks.begin();
                 it != currentFrameRenderTasks.end(); ++it) {
                if (it->get() == task) {
                    currentFrameRenderTasks.erase(it);

                    currentFrameRenderTasksCond.wakeAll();
                    break;
                }
            }
        }
    }

    void waitForRunnableTasks()
    {
        QMutexLocker k(&currentFrameRenderTasksMutex);

        while ( !currentFrameRenderTasks.empty() ) {
            currentFrameRenderTasksCond.wait(&currentFrameRenderTasksMutex);
        }
    }

    void processProducedFrame(U64 age, const BufferedFrameContainerPtr& frames);
};


class RenderCurrentFrameFunctorRunnable
: public QRunnable
{
    boost::shared_ptr<CurrentFrameFunctorArgs> _args;

public:

    RenderCurrentFrameFunctorRunnable(const boost::shared_ptr<CurrentFrameFunctorArgs>& args)
    : _args(args)
    {
    }

    virtual ~RenderCurrentFrameFunctorRunnable()
    {
    }

    void createAndLaunchRenderInThread(const ViewerNodePtr &viewer,
                                       const RenderViewerProcessFunctorArgsPtr& processArgs,
                                       int viewerProcess_i,
                                       TimeValue time,
                                       const RenderStatsPtr& stats,
                                       const RotoStrokeItemPtr& activeStroke,
                                       const RectD* roiParam,
                                       ViewerRenderBufferedFrame* bufferedFrame)
    {

        ViewerRenderFrameRunnable::createRenderViewerProcessArgs(viewer, viewerProcess_i, time, bufferedFrame->view, false /*isPlayback*/, stats, bufferedFrame->type, activeStroke, roiParam, processArgs.get());

        // Register the current renders and their age on the scheduler so that they can be aborted
        {
            QMutexLocker k(&_args->scheduler->renderAgeMutex);
            TreeRenderAndAge curRender;
            curRender.age = _args->age;

            {
                TreeRenderSetOrderedByAge::iterator foundAge = _args->scheduler->currentRenders.find(curRender);
                if (foundAge != _args->scheduler->currentRenders.end()) {
                    curRender = *foundAge;
                    _args->scheduler->currentRenders.erase(foundAge);
                }
            }
            assert(processArgs->renderObject);
            curRender.renders.push_back(processArgs->renderObject);
            std::pair<TreeRenderSetOrderedByAge::iterator, bool> ok = _args->scheduler->currentRenders.insert(curRender);
            assert(ok.second);
            (void)ok;

        }

        if (viewer->isViewerPaused(viewerProcess_i)) {
            processArgs->retCode = eActionStatusAborted;
        } else {
            ViewerRenderFrameRunnable::launchRenderFunctor(processArgs, roiParam);
        }

        bufferedFrame->retCode[viewerProcess_i] = processArgs->retCode;
        bufferedFrame->viewerProcessImageKey[viewerProcess_i] = processArgs->viewerProcessImageCacheKey;
        bufferedFrame->viewerProcessImages[viewerProcess_i] = processArgs->outputImage;
        bufferedFrame->colorPickerImages[viewerProcess_i] = processArgs->colorPickerImage;
        bufferedFrame->colorPickerInputImages[viewerProcess_i] = processArgs->colorPickerInputImage;

    }


    void computeViewsForRoI(const ViewerNodePtr &viewer, const RectD* partialUpdateArea, const ViewerRenderBufferedFrameContainerPtr& framesContainer)
    {

        // Render each view sequentially. For now the viewer always asks to render 1 view since the interface can only allow 1 view at once per view
        for (std::size_t i = 0; i < _args->viewsToRender.size(); ++i) {

            ViewIdx view = _args->viewsToRender[i];

            // Create stats object if we want statistics
            RenderStatsPtr stats;
            if (_args->useStats) {
                stats.reset( new RenderStats(true) );
            }


            // Create the object wrapping the rendered image for this view
            ViewerRenderBufferedFramePtr bufferObject(new ViewerRenderBufferedFrame);
            bufferObject->view = view;
            bufferObject->stats = stats;
            if (partialUpdateArea) {
                bufferObject->type = OpenGLViewerI::TextureTransferArgs::eTextureTransferTypeOverlay;
            } else if (_args->strokeItem && _args->strokeItem->getRenderCloneCurrentStrokeStartPointIndex() > 0) {

                // Upon painting ticks, we just have to update the viewer for the area that was painted
                bufferObject->type = OpenGLViewerI::TextureTransferArgs::eTextureTransferTypeModify;
            }
            else {
                bufferObject->type = OpenGLViewerI::TextureTransferArgs::eTextureTransferTypeReplace;
            }


            // Create a tree render object for both viewer process nodes
            std::vector<RenderViewerProcessFunctorArgsPtr> processArgs(2);
            for (int i = 0; i < 2; ++i) {
                processArgs[i].reset(new RenderViewerProcessFunctorArgs);
            }
            ViewerCompositingOperatorEnum viewerBlend = viewer->getCurrentOperator();
            bool viewerBEqualsViewerA = viewer->getCurrentAInput() == viewer->getCurrentBInput();

            // Launch the 2nd viewer process in a separate thread
            QFuture<void> processBFuture ;
            if (!viewerBEqualsViewerA && viewerBlend != eViewerCompositingOperatorNone) {
                processBFuture = QtConcurrent::run(boost::bind(&RenderCurrentFrameFunctorRunnable::createAndLaunchRenderInThread,
                                                               this,
                                                               viewer,
                                                               processArgs[1],
                                                               1,
                                                               _args->time,
                                                               stats,
                                                               _args->strokeItem,
                                                               partialUpdateArea,
                                                               bufferObject.get()));
            }

            // Launch the 1st viewer process in this thread
            createAndLaunchRenderInThread(viewer, processArgs[0], 0, _args->time, stats, _args->strokeItem, partialUpdateArea, bufferObject.get());

            // Wait for the 2nd viewer process
            if (viewerBlend == eViewerCompositingOperatorNone) {
                bufferObject->retCode[1] = eActionStatusFailed;
            } else {
                if (!viewerBEqualsViewerA) {
                    processBFuture.waitForFinished();
                } else  {
                    bufferObject->retCode[1] = processArgs[0]->retCode;
                    bufferObject->viewerProcessImageKey[1] = processArgs[0]->viewerProcessImageCacheKey;
                    bufferObject->viewerProcessImages[1] = processArgs[0]->outputImage;
                }
            }
            framesContainer->frames.push_back(bufferObject);


        } // for all views

    }

    virtual void run() OVERRIDE FINAL
    {

        ViewerNodePtr viewer = _args->viewer->isEffectViewerNode();

        // The object that contains frames that we want to upload to the viewer UI all at once
        ViewerRenderBufferedFrameContainerPtr framesContainer(new ViewerRenderBufferedFrameContainer);
        framesContainer->time = _args->time;
        framesContainer->recenterViewer = viewer->getViewerCenterPoint(&framesContainer->viewerCenter);


        if (viewer->isDoingPartialUpdates()) {
            // If the viewer is doing partial updates (i.e: during tracking we only update the markers areas)
            // Then we launch multiple renders over the partial areas
            std::list<RectD> partialUpdates = viewer->getPartialUpdateRects();
            for (std::list<RectD>::const_iterator it = partialUpdates.begin(); it != partialUpdates.end(); ++it) {
                computeViewsForRoI(viewer, &(*it), framesContainer);
            }
        } else {
            computeViewsForRoI(viewer, 0, framesContainer);
        }

        // Call updateViewer() on the main thread
        {

            boost::shared_ptr<ViewerCurrentFrameRenderProcessFrameArgs> processArgs(new ViewerCurrentFrameRenderProcessFrameArgs);
            processArgs->age = _args->age;
            processArgs->frames = framesContainer;

            ProcessFrameThreadStartArgsPtr processStartArgs(new ProcessFrameThreadStartArgs);
            processStartArgs->args = processArgs;
            processStartArgs->executeOnMainThread = true;
            processStartArgs->processor = _args->scheduler->_publicInterface;
            _args->scheduler->processFrameThread.startTask(processStartArgs);
        }

        {
            // Remove the current render from the abortable renders list
            QMutexLocker k(&_args->scheduler->renderAgeMutex);
            for (TreeRenderSetOrderedByAge::iterator it = _args->scheduler->currentRenders.begin(); it != _args->scheduler->currentRenders.end(); ++it) {
                if ( it->age == _args->age ) {
                    _args->scheduler->currentRenders.erase(it);
                    break;
                }
            }
        }


        _args->scheduler->removeRunnableTask(this);
    } // run
};


ViewerCurrentFrameRequestScheduler::ViewerCurrentFrameRequestScheduler(const NodePtr& viewer)
: _imp( new ViewerCurrentFrameRequestSchedulerPrivate(this, viewer) )
{
}

ViewerCurrentFrameRequestScheduler::~ViewerCurrentFrameRequestScheduler()
{
    // Should've been stopped before anyway
    if (_imp->backupThread.quitThread(false)) {
        _imp->backupThread.waitForAbortToComplete_enforce_blocking();
    }
}

void
ViewerCurrentFrameRequestScheduler::processFrame(const ProcessFrameArgsBase& inArgs)
{
    assert(QThread::currentThread() == qApp->thread());
    const ViewerCurrentFrameRenderProcessFrameArgs* args = dynamic_cast<const ViewerCurrentFrameRenderProcessFrameArgs*>(&inArgs);
    assert(args);
    _imp->processProducedFrame(args->age, args->frames);

}

void
ViewerCurrentFrameRequestSchedulerPrivate::processProducedFrame(U64 age, const BufferedFrameContainerPtr& frames)
{
    if (!frames) {
        return;
    }
    bool updateAge = false;
    // Do not process the produced frame if the age is now older than what is displayed
    {
        QMutexLocker k(&renderAgeMutex);
        if (age <= displayAge) {
            return;
        }

    }

    ViewerRenderBufferedFrameContainer* isViewerFrameContainer = dynamic_cast<ViewerRenderBufferedFrameContainer*>(frames.get());
    assert(isViewerFrameContainer);

    ViewerNodePtr viewerNode = viewer->isEffectViewerNode();
    if (!viewerNode)  {
        return;
    }

    for (std::list<BufferedFramePtr>::const_iterator it2 = isViewerFrameContainer->frames.begin(); it2 != isViewerFrameContainer->frames.end(); ++it2) {
        ViewerRenderBufferedFrame* viewerObject = dynamic_cast<ViewerRenderBufferedFrame*>(it2->get());
        assert(viewerObject);

        ViewerNode::UpdateViewerArgs args;
        args.time = isViewerFrameContainer->time;
        args.view = viewerObject->view;
        args.type = viewerObject->type;
        args.recenterViewer = isViewerFrameContainer->recenterViewer;
        args.viewerCenter = isViewerFrameContainer->viewerCenter;

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
            updateAge = true;
            viewerNode->updateViewer(args);
        }

        if (viewerObject->stats) {
            double wallTime = 0;
            std::map<NodePtr, NodeRenderStats > statsMap = viewerObject->stats->getStats(&wallTime);
            viewerNode->reportStats(isViewerFrameContainer->time,  wallTime, statsMap);
        }

    }
    if (updateAge) {
        QMutexLocker k(&renderAgeMutex);
        // Update the display age
        displayAge = age;
    }
    // At least redraw the viewer, we might be here when the user removed a node upstream of the viewer.
    viewerNode->redrawViewer();
} // processProducedFrame

void
ViewerCurrentFrameRequestScheduler::onAbortRequested(bool keepOldestRender)
{
#ifdef TRACE_CURRENT_FRAME_SCHEDULER
    qDebug() << getThreadName().c_str() << "Received abort request";
#endif
    _imp->backupThread.abortThreadedTask();

    ViewerNodePtr viewerNode = _imp->viewer->isEffectViewerNode();

    // Do not abort the oldest render while scrubbing timeline or sliders so that the user gets some feedback
    const bool keepOldest = viewerNode->getApp()->isDraftRenderEnabled() || viewerNode->isDoingPartialUpdates() || keepOldestRender;

    QMutexLocker k(&_imp->renderAgeMutex);

    if ( _imp->currentRenders.empty() ) {
        return;
    }

    //Do not abort the oldest render, let it finish

    bool keptOneRenderActive = !keepOldest;

    // Only keep the oldest render active if it is worth displaying
    for (TreeRenderSetOrderedByAge::iterator it = _imp->currentRenders.begin(); it != _imp->currentRenders.end(); ++it) {
        bool hasRenderNotAborted = false;
        for (std::list<TreeRenderPtr>::const_iterator it2 = it->renders.begin(); it2 != it->renders.end(); ++it2) {
            if (!(*it2)->isRenderAborted()) {
                hasRenderNotAborted = true;
                break;
            }
        }
        if (!hasRenderNotAborted) {
            continue;
        }
        if (it->age >= _imp->displayAge && !keptOneRenderActive) {
            keptOneRenderActive = true;
            continue;
        }
        for (std::list<TreeRenderPtr>::const_iterator it2 = it->renders.begin(); it2 != it->renders.end(); ++it2) {
            (*it2)->setRenderAborted();
        }
    }
} // onAbortRequested

void
ViewerCurrentFrameRequestScheduler::onQuitRequested(bool allowRestarts)
{
    _imp->processFrameThread.quitThread(allowRestarts);
    _imp->backupThread.quitThread(allowRestarts);
}

void
ViewerCurrentFrameRequestScheduler::onWaitForThreadToQuit()
{
    _imp->waitForRunnableTasks();
    _imp->processFrameThread.waitForThreadToQuit_enforce_blocking();
    _imp->backupThread.waitForThreadToQuit_enforce_blocking();
}

void
ViewerCurrentFrameRequestScheduler::onWaitForAbortCompleted()
{
    _imp->waitForRunnableTasks();
    _imp->processFrameThread.waitForAbortToComplete_enforce_blocking();
    _imp->backupThread.waitForAbortToComplete_enforce_blocking();
}

void
ViewerCurrentFrameRequestScheduler::renderCurrentFrameInternal(const boost::shared_ptr<CurrentFrameFunctorArgs>& args, bool useSingleThread)
{


    // Start the work in a thread of the thread pool if we can.
    // Let at least 1 free thread in the thread-pool to allow the renderer to use the thread pool if we use the thread-pool
    int maxThreads;
    if (useSingleThread) {
        maxThreads = 1;
    } else {
        maxThreads = _imp->threadPool->maxThreadCount();
    }

    if ( (maxThreads == 1) || (_imp->threadPool->activeThreadCount() >= maxThreads - 1) ) {
        _imp->backupThread.startTask(args);
    } else {

        // See bug https://bugreports.qt.io/browse/QTBUG-20251
        // The Qt thread-pool mem-leaks the runnable if using release/reserveThread
        // Instead we explicitly manage them and ensure they do not hold any external strong refs.
        boost::shared_ptr<RenderCurrentFrameFunctorRunnable> task(new RenderCurrentFrameFunctorRunnable(args));
        task->setAutoDelete(false);
        _imp->appendRunnableTask(task);
        _imp->threadPool->start(task.get());
    }

} // renderCurrentFrameInternal

void
ViewerCurrentFrameRequestScheduler::renderCurrentFrame(bool enableRenderStats)
{
    // Sanity check, also do not render viewer that are not made visible by the user
    NodePtr treeRoot = _imp->viewer;
    ViewerNodePtr viewerNode = treeRoot->isEffectViewerNode();
    if (!treeRoot || !viewerNode || !viewerNode->isViewerUIVisible() ) {
        return;
    }

    // We are about to trigger a new render, cancel all other renders except the oldest so user gets some feedback.
    onAbortRequested(true /*keepOldestRender*/);

    // Get the frame/view to render
    TimeValue frame;
    ViewIdx view;
    {
        frame = TimeValue(viewerNode->getTimeline()->currentFrame());
        int viewsCount = viewerNode->getRenderViewsCount();
        view = viewsCount > 0 ? viewerNode->getCurrentRenderView() : ViewIdx(0);
    }



    // Are we tracking ?
    bool isTracking = viewerNode->isDoingPartialUpdates();

    // Are we painting ?
    // While painting, use a single render thread and always the same thread.
    RotoStrokeItemPtr curStroke = _imp->viewer->getApp()->getActiveRotoDrawingStroke();


    // Ok we have to render at least one of A or B input
    boost::shared_ptr<CurrentFrameFunctorArgs> functorArgs( new CurrentFrameFunctorArgs(view,
                                                                                        frame,
                                                                                        enableRenderStats,
                                                                                        _imp->viewer,
                                                                                        _imp.get(),
                                                                                        curStroke) );



    // Identify this render request with an age

    {
        QMutexLocker k(&_imp->renderAgeMutex);
        functorArgs->age = _imp->renderAge;
        // If we reached the max amount of age, reset to 0... should never happen anyway
        if ( _imp->renderAge >= std::numeric_limits<U64>::max() ) {
            _imp->renderAge = 0;
        } else {
            ++_imp->renderAge;
        }
    }

    // When painting, limit the number of threads to 1 to be sure strokes are painted in the right order
    renderCurrentFrameInternal(functorArgs, curStroke || isTracking);



} // ViewerCurrentFrameRequestScheduler::renderCurrentFrame

bool
ViewerCurrentFrameRequestScheduler::hasThreadsAlive() const
{
    QMutexLocker k(&_imp->renderAgeMutex);
    return _imp->currentRenders.size() > 0;
}

ViewerCurrentFrameRequestRendererBackup::ViewerCurrentFrameRequestRendererBackup()
: GenericSchedulerThread()
{
    setThreadName("ViewerCurrentFrameRequestRendererBackup");
}

ViewerCurrentFrameRequestRendererBackup::~ViewerCurrentFrameRequestRendererBackup()
{
}

GenericSchedulerThread::TaskQueueBehaviorEnum
ViewerCurrentFrameRequestRendererBackup::tasksQueueBehaviour() const
{
    return eTaskQueueBehaviorSkipToMostRecent;
}


GenericSchedulerThread::ThreadStateEnum
ViewerCurrentFrameRequestRendererBackup::threadLoopOnce(const GenericThreadStartArgsPtr& inArgs)
{
    boost::shared_ptr<CurrentFrameFunctorArgs> args = boost::dynamic_pointer_cast<CurrentFrameFunctorArgs>(inArgs);
    
    assert(args);
    RenderCurrentFrameFunctorRunnable task(args);
    task.run();
    
    return eThreadStateActive;
}


NATRON_NAMESPACE_EXIT
