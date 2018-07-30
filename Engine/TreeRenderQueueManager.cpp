/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#include "TreeRenderQueueManager.h"

#include <QMutex>
#include <QWaitCondition>
#include <QThreadPool>

#include <QtConcurrentRun>

#ifdef DEBUG
#include "Global/FloatingPointExceptions.h"
#endif
#include "Engine/AppManager.h"
#include "Engine/FrameViewRequest.h"
#include "Engine/TreeRender.h"
#include "Engine/ThreadPool.h"

NATRON_NAMESPACE_ENTER;



struct PerProviderRenders
{

    // Renders that are queued in render but not yet finished.
    // It is removed from this list after calling waitForRenderFinished() or waitForAnyTreeRenderFinished().
    // We do this instead of removing it when the render is actually finished, so that we can prevent stalling a thread
    // calling waitForRenderFinished() to block forever because it is waiting for a render that was never queued or already finished.
    std::set<TreeRenderPtr> queuedRenders;

    // It is removed from this list after calling waitForRenderFinished() or waitForAnyTreeRenderFinished().
    std::set<TreeRenderPtr> finishedRenders;

    // List of render (and sub-render) executions that are finished.
    // Executions are inserted in this list at the same time they are removed from the global executionQueue
    // of the TreeRenderQueueManager.
    // Executions are removed from this list once
    // waitForTreeRenderExecutionFinished is called.
    std::set<TreeRenderExecutionDataPtr> finishedExecutions;
};

typedef boost::shared_ptr<PerProviderRenders> PerProviderRendersPtr;

typedef std::map<TreeRenderQueueProviderConstWPtr, PerProviderRendersPtr> PerProviderRendersMap;

class LaunchRenderRunnable : public QRunnable
{
    TreeRenderPtr render;
    TreeRenderQueueManager::Implementation* imp;

public:

    LaunchRenderRunnable(const TreeRenderPtr& render, TreeRenderQueueManager::Implementation* imp)
    : render(render)
    , imp(imp)
    {

    }

    virtual ~LaunchRenderRunnable() {

    }

private:

    virtual void run() OVERRIDE FINAL;
};

struct TreeRenderQueueManager::Implementation
{
    TreeRenderQueueManager* _publicInterface;

    // Protects executionQueue
    mutable QMutex executionQueueMutex;

    // If no work is requested, the TreeRenderQueueManager thread sleeps until renders are requested
    // Protected by executionQueueMutex
    QWaitCondition activityChangedCond;

    // The main execution queue in order
    std::list<TreeRenderExecutionDataPtr> executionQueue;

    // Protected by executionQueueMutex
    // Every times a thread starts or end an operation such as queueing a render or finishing a render, we
    // notify the TreeRenderQueueManager thread by incrementing this number. The manager thread then reset it back to 0.
    std::size_t activityCheckCount;

    // Protects perProviderRenders
    QMutex perProviderRendersMutex;

    // Allows to notify threads that call waitForAnyTreeRenderFinished() or waitForRenderFinished() that a render execution is finished
    QWaitCondition perProviderRendersCond;

    // For each queue provider, a list of the launched renders, finished renders and finished executions
    PerProviderRendersMap perProviderRenders;

    // Protects mustQuit
    QMutex mustQuitMutex;

    // The thread calling quitThread() waits in this condition until the TreeRenderQueueManager() caught the mustQuit flag and set it back
    // to false.
    QWaitCondition mustQuitCond;

    // True when somebody called quitThread()
    bool mustQuit;

    Implementation(TreeRenderQueueManager* publicInterface)
    : _publicInterface(publicInterface)
    , executionQueueMutex()
    , activityChangedCond()
    , executionQueue()
    , activityCheckCount(0)
    , perProviderRendersMutex()
    , perProviderRendersCond()
    , perProviderRenders()
    , mustQuitMutex()
    , mustQuitCond()
    , mustQuit(false)
    {

    }

    /**
     * @brief Called in a separate thread once the main execution of a render finishes. 
     * This will launch the extra requested executions (including image(s) for color-picker)
     * and wait for it
     **/
    void launchAndWaitExtraExecutionTasks(const TreeRenderExecutionDataPtr& mainFinishedExecution, const std::list<TreeRenderExecutionDataPtr>& executions);

    /**
     * @brief Used to wait for the execution of a render to be finished. Do not call directly
     **/
    void waitForTreeRenderExecutionFinished(const TreeRenderExecutionDataPtr& execData) ;

    void waitForTreeRenderInternal(const TreeRenderPtr& render,
                                   std::set<TreeRenderPtr>::iterator queuedRenderIt,
                                   std::set<TreeRenderPtr>::iterator finishedRendersIt,
                                   PerProviderRendersMap::iterator providersMapIt);

    void launchMoreTasks();

    void notifyTaskInRenderFinishedInternal(const TreeRenderExecutionDataPtr& render, bool isExecutionFinished, bool launchExtraRenders, bool isRunningInThreadPoolThread);

    void onTaskRenderFinished(const TreeRenderExecutionDataPtr& render);

    void appendTreeRenderExecution(const TreeRenderExecutionDataPtr& render);

    bool canSleep() const;

    void notifyManagerThreadForModifications();

    void notifyManagerThreadForModifications_nolock();
};



TreeRenderQueueManager::TreeRenderQueueManager()
: QThread()
, _imp(new Implementation(this))
{
    setObjectName(QString::fromUtf8("TreeRenderQueueManager"));
}

TreeRenderQueueManager::~TreeRenderQueueManager()
{

}

void
TreeRenderQueueManager::launchRender(const TreeRenderPtr& render)
{
    assert(render && render->getProvider());

    // Insert the render to the queued list
    {
        QMutexLocker k2(&_imp->perProviderRendersMutex);
        PerProviderRendersPtr& providerRenders = _imp->perProviderRenders[render->getProvider()];
        if (!providerRenders) {
            providerRenders.reset(new PerProviderRenders);
        }
        std::pair<std::set<TreeRenderPtr>::iterator, bool> ok = providerRenders->queuedRenders.insert(render);
        if (!ok.second) {
            // Hum... render was already queued.
            qDebug() << "Attempting to call TreeRenderQueueManager::launchRender() on an already launched render, this is a bug from the caller.";
            return;
        }
    }


    QThreadPool::globalInstance()->start(new LaunchRenderRunnable(render, _imp.get()));

} // launchRender

TreeRenderExecutionDataPtr
TreeRenderQueueManager::launchSubRender(const EffectInstancePtr& treeRoot,
                                        TimeValue time,
                                        ViewIdx view,
                                        const RenderScale& proxyScale,
                                        unsigned int mipMapLevel,
                                        const ImagePlaneDesc* planeParam,
                                        const RectD* canonicalRoIParam,
                                        const TreeRenderPtr& render,
                                        int concatenationFlags,
                                        bool createTreeRenderIfUnrenderedImage)
{
    assert(render && render->getProvider());

#ifdef DEBUG
    // Check that the render was already registered with launchRender()
    {
        QMutexLocker k(&_imp->perProviderRendersMutex);
        PerProviderRendersPtr& renders = _imp->perProviderRenders[render->getProvider()];
        assert(renders);
        assert(std::find(renders->queuedRenders.begin(), renders->queuedRenders.end(), render) != renders->queuedRenders.end());
    }
#endif


    TreeRenderExecutionDataPtr execData = render->createSubExecutionData(treeRoot, time, view, proxyScale, mipMapLevel, planeParam, canonicalRoIParam, concatenationFlags, createTreeRenderIfUnrenderedImage);
    if (isFailureRetCode(execData->getStatus())) {
        render->setResults(FrameViewRequestPtr(), execData);
    } else {
        _imp->appendTreeRenderExecution(execData);
    }

    // The sub task is launched from a thread pool thread, so temporarily uncount it as an active task.
    // If we do not do so, if all maxThreadsCount are busy in this function, all threads would deadlock in the wait
    // call below.
    releaseTask();

    _imp->waitForTreeRenderExecutionFinished(execData);

    reserveTask();


    return execData;
} // launchSubRender

void
TreeRenderQueueManager::releaseTask()
{
    if (isRunningInThreadPoolThread()) {
        QThreadPool::globalInstance()->releaseThread();

        // We are making a thread available, notify the manager which may be able to load more renders.
        _imp->notifyManagerThreadForModifications();
    }
}


void
TreeRenderQueueManager::reserveTask()
{
    if (isRunningInThreadPoolThread()) {
        QThreadPool::globalInstance()->reserveThread();
    }
}


ReleaseTPThread_RAII::ReleaseTPThread_RAII()
: manager(appPTR->getTasksQueueManager())
{
    manager->releaseTask();

}

ReleaseTPThread_RAII::~ReleaseTPThread_RAII()
{
    manager->reserveTask();
}


bool
TreeRenderQueueManager::hasTreeRendersLaunched(const TreeRenderQueueProviderConstPtr& provider) const
{
    QMutexLocker k(&_imp->perProviderRendersMutex);
    PerProviderRendersMap::const_iterator foundProvider = _imp->perProviderRenders.find(provider);
    if (foundProvider == _imp->perProviderRenders.end()) {
        return false;
    }
    return !foundProvider->second->queuedRenders.empty() || !foundProvider->second->finishedRenders.empty();
}

bool
TreeRenderQueueManager::hasTreeRendersFinished(const TreeRenderQueueProviderConstPtr& provider) const
{
    QMutexLocker k(&_imp->perProviderRendersMutex);
    PerProviderRendersMap::const_iterator foundProvider = _imp->perProviderRenders.find(provider);
    if (foundProvider == _imp->perProviderRenders.end()) {
        return false;
    }
    return !foundProvider->second->finishedRenders.empty();
}

void
TreeRenderQueueManager::getRenderIndex(const TreeRenderPtr& render, int* index, int* numRenders) const
{
    *numRenders = 0;
    *index = -1;
    QMutexLocker k(&_imp->executionQueueMutex);
    for (std::list<TreeRenderExecutionDataPtr>::const_iterator it = _imp->executionQueue.begin(); it != _imp->executionQueue.end(); ++it) {
        if (!(*it)->isTreeMainExecution()) {
            continue;
        }
        *numRenders += 1;
        if (render == (*it)->getTreeRender()) {
            *index = *numRenders - 1;
        }
    }
}

void
TreeRenderQueueManager::Implementation::waitForTreeRenderInternal(const TreeRenderPtr& render,
                                                                  std::set<TreeRenderPtr>::iterator queuedRenderIt,
                                                                  std::set<TreeRenderPtr>::iterator finishedRendersIt,
                                                                  PerProviderRendersMap::iterator providersMapIt)
{
    // Private : does not lock
    assert(!perProviderRendersMutex.tryLock());

    PerProviderRenders& renders = *providersMapIt->second;

    renders.queuedRenders.erase(queuedRenderIt);
    renders.finishedRenders.erase(finishedRendersIt);

    // Remove any render clone from this render
    render->cleanupRenderClones();

    // No more active renders for this provider
    if (renders.finishedRenders.empty() && renders.queuedRenders.empty()) {
        perProviderRenders.erase(providersMapIt);
    }

} // waitForTreeRenderInternal

TreeRenderPtr
TreeRenderQueueManager::waitForAnyTreeRenderFinished(const TreeRenderQueueProviderConstPtr& provider)
{

    ReleaseTPThread_RAII tpThreadRelease;
    
    QMutexLocker k(&_imp->perProviderRendersMutex);

    PerProviderRendersMap::iterator foundProvider = _imp->perProviderRenders.find(provider);
    if (foundProvider == _imp->perProviderRenders.end()) {
        // No render has been requested for this provider
        return TreeRenderPtr();
    }

    PerProviderRenders& renders = *foundProvider->second;

    // Nothing was launched
    if (renders.queuedRenders.empty() && renders.finishedRenders.empty()) {
        return TreeRenderPtr();
    }

    // Wait for a render to finish
    while (!renders.queuedRenders.empty() && renders.finishedRenders.empty()) {
        _imp->perProviderRendersCond.wait(&_imp->perProviderRendersMutex);

        // Lookup the iterator again, it could have been invalidated since we released the mutex.
        foundProvider = _imp->perProviderRenders.find(provider);
        if (foundProvider == _imp->perProviderRenders.end()) {
            // No render has been requested for this provider
            return TreeRenderPtr();
        }
        renders = *foundProvider->second;

        // Nothing was launched
        if (renders.queuedRenders.empty() && renders.finishedRenders.empty()) {
            return TreeRenderPtr();
        }

    }

    assert(!renders.finishedRenders.empty());


    // Remove the first finished render
    std::set<TreeRenderPtr>::iterator foundFinishedRender = renders.finishedRenders.begin();
    TreeRenderPtr ret = *foundFinishedRender;

    std::set<TreeRenderPtr>::iterator foundQueued = renders.queuedRenders.find(ret);
    assert(foundQueued != renders.queuedRenders.end());

    _imp->waitForTreeRenderInternal(ret, foundQueued, foundFinishedRender, foundProvider);
    return ret;
} // waitForAnyTreeRenderFinished

ActionRetCodeEnum
TreeRenderQueueManager::waitForRenderFinished(const TreeRenderPtr& render)
{
    ReleaseTPThread_RAII tpThreadRelease;

    QMutexLocker k(&_imp->perProviderRendersMutex);

    PerProviderRendersMap::iterator foundProvider = _imp->perProviderRenders.find(render->getProvider());
    if (foundProvider == _imp->perProviderRenders.end()) {
        // No render has been requested for this provider
        return eActionStatusFailed;
    }

    PerProviderRenders& renders = *foundProvider->second;

    std::set<TreeRenderPtr>::iterator foundLaunched = renders.queuedRenders.find(render);
    if (foundLaunched == renders.queuedRenders.end()) {
        // The render was never launched...
        return eActionStatusFailed;
    }

    std::set<TreeRenderPtr>::iterator foundFinishedRender = renders.finishedRenders.find(render);
    while (foundFinishedRender == renders.finishedRenders.end()) {
        _imp->perProviderRendersCond.wait(&_imp->perProviderRendersMutex);

        // Lookup the iterator again, it could have been invalidated since we released the mutex.
        foundProvider = _imp->perProviderRenders.find(render->getProvider());
        if (foundProvider == _imp->perProviderRenders.end()) {
            // No render has been requested for this provider
            return eActionStatusFailed;
        }
        renders = *foundProvider->second;
        foundLaunched = renders.queuedRenders.find(render);
        if (foundLaunched == renders.queuedRenders.end()) {
            // The render was never launched...
            return eActionStatusFailed;
        }


        foundFinishedRender = renders.finishedRenders.find(render);
    }

    std::set<TreeRenderPtr>::iterator foundQueued = renders.queuedRenders.find(render);
    assert(foundQueued != renders.queuedRenders.end());
    _imp->waitForTreeRenderInternal(render, foundQueued, foundFinishedRender, foundProvider);

    ActionRetCodeEnum ret = render->getStatus();
    return ret;

} // waitForRenderFinished

void
TreeRenderQueueManager::Implementation::waitForTreeRenderExecutionFinished(const TreeRenderExecutionDataPtr& execData)
{
    ReleaseTPThread_RAII tpThreadRelease;

    // Did we find the render in the currently launched renders
    bool isLaunchedExecution;
    {
        QMutexLocker k(&executionQueueMutex);
        std::list<TreeRenderExecutionDataPtr>::iterator foundLaunched = std::find(executionQueue.begin(), executionQueue.end(), execData);
        isLaunchedExecution = foundLaunched != executionQueue.end();
    }


    QMutexLocker k(&perProviderRendersMutex);
    PerProviderRendersMap::iterator foundProvider = perProviderRenders.find(execData->getTreeRender()->getProvider());
    if (foundProvider == perProviderRenders.end()) {
        // No render has been requested for this provider
        return;
    }

    std::set<TreeRenderExecutionDataPtr>::iterator foundFinishedRender = foundProvider->second->finishedExecutions.find(execData);

    if (!isLaunchedExecution && foundFinishedRender == foundProvider->second->finishedExecutions.end()) {
        // Not launched and not finished: this is an unknown execution
        return;
    }

    while (foundFinishedRender == foundProvider->second->finishedExecutions.end()) {
        perProviderRendersCond.wait(&perProviderRendersMutex);

        // Get the iterator to the provider data since it might have been invalidated
        foundProvider = perProviderRenders.find(execData->getTreeRender()->getProvider());
        if (foundProvider == perProviderRenders.end()) {
            // No render has been requested for this provider
            return;
        }

        foundFinishedRender = foundProvider->second->finishedExecutions.find(execData);
    }

    foundProvider->second->finishedExecutions.erase(foundFinishedRender);

} // waitForTreeRenderExecutionFinished

void
TreeRenderQueueManager::Implementation::appendTreeRenderExecution(const TreeRenderExecutionDataPtr& render)
{
    // Push the execution to the queue and notify the thread
    {
        QMutexLocker k(&executionQueueMutex);
        executionQueue.push_back(render);

        if (!_publicInterface->isRunning()) {
            _publicInterface->start();
        } else {
            activityChangedCond.wakeOne();
        }
    }

} // appendTreeRenderExecution

void
TreeRenderQueueManager::Implementation::launchAndWaitExtraExecutionTasks(const TreeRenderExecutionDataPtr& mainFinishedExecution, const std::list<TreeRenderExecutionDataPtr>& extraExecutions)
{
    if (!isFailureRetCode(mainFinishedExecution->getStatus()) ) {
        for (std::list<TreeRenderExecutionDataPtr>::const_iterator it = extraExecutions.begin(); it != extraExecutions.end(); ++it) {
            if (!isFailureRetCode((*it)->getStatus())) {
                appendTreeRenderExecution(*it);
                waitForTreeRenderExecutionFinished(*it);
            }
        }
    }

    onTaskRenderFinished(mainFinishedExecution);
} // launchAndWaitExtraExecutionTasks

void
TreeRenderQueueManager::Implementation::onTaskRenderFinished(const TreeRenderExecutionDataPtr& render)
{
    {
        QMutexLocker k(&perProviderRendersMutex);
        PerProviderRendersMap::iterator foundProvider = perProviderRenders.find(render->getTreeRender()->getProvider());
        assert(foundProvider != perProviderRenders.end());
        if (foundProvider == perProviderRenders.end()) {
            // No render has been requested for this provider
            return;
        }

        // Insert the exeuction in the finishedExecutions list if this is not the main execution, otherwise no need to do so
        // since we insert the render in the finishedRenders list
        if (render->isTreeMainExecution()) {
            foundProvider->second->finishedRenders.insert(render->getTreeRender());
        } else {
            foundProvider->second->finishedExecutions.insert(render);
        }
        perProviderRendersCond.wakeAll();
    }
    {
        QMutexLocker k(&executionQueueMutex);
        // The execution must exist in the queue. Remove it now because we don't want the run() function to use it again.
        std::list<TreeRenderExecutionDataPtr>::iterator found = std::find(executionQueue.begin(), executionQueue.end(), render);
        if (found != executionQueue.end()) {
            // The execution may no longer be in the exeuction queue if it has a failed status because in that case we did not exit early
            // in the if condition at the start of the function.
            executionQueue.erase(found);
        }
        notifyManagerThreadForModifications_nolock();
    }


    if (render->isTreeMainExecution()) {
        TreeRenderPtr treeRender = render->getTreeRender();
        TreeRenderQueueProviderPtr provider = boost::const_pointer_cast<TreeRenderQueueProvider>(treeRender->getProvider());
        provider->notifyTreeRenderFinished(treeRender);
    }

} // onTaskRenderFinished

void
TreeRenderQueueManager::Implementation::notifyTaskInRenderFinishedInternal(const TreeRenderExecutionDataPtr& render,
                                                                           bool isExecutionFinished,
                                                                           bool launchExtraRenders,
                                                                           bool isRunningInThreadPoolThread)
{
    if (isRunningInThreadPoolThread) {
        // Inform the TreeRenderQueueManager thread that one task is finished so it can start scheduling again
        notifyManagerThreadForModifications();
    }

    if (!isExecutionFinished) {
        // The render execution has still tasks to render, nothing to do
        return;
    }

    // The execution is finished: if this is the main execution of a TreeRender, we may mark the render
    // finished.

    // The render may have requested extraneous images to render for the color-picker. Launch their execution now.
    // This should be fast since the results should be cached from the main execution of the tree.
    if (launchExtraRenders && render->isTreeMainExecution()) {
        std::list<TreeRenderExecutionDataPtr> extraExecutions = render->getTreeRender()->getExtraRequestedResultsExecutionData();
        if (!extraExecutions.empty()) {
            // Launch the executions in a separate thread if we are in the TreeRenderQueueManager thread because it will require the
            // TreeRenderQueueManager thread to schedule the tasks
            if (!isRunningInThreadPoolThread && !isFailureRetCode(render->getStatus())) {
                QtConcurrent::run(this, &TreeRenderQueueManager::Implementation::launchAndWaitExtraExecutionTasks, render, extraExecutions);
            } else {
                launchAndWaitExtraExecutionTasks(render, extraExecutions);
            }
            return;
        }
    }

    onTaskRenderFinished(render);

} // notifyTaskInRenderFinishedInternal

void
TreeRenderQueueManager::notifyTaskInRenderFinished(const TreeRenderExecutionDataPtr& render, bool isExecutionFinished, bool isRunningInThreadPoolThread)
{
    _imp->notifyTaskInRenderFinishedInternal(render, isExecutionFinished, true /*notifyTaskInRenderFinishedInternal*/, isRunningInThreadPoolThread);
    
} // notifyTaskInRenderFinished

void
TreeRenderQueueManager::quitThread()
{
    if (!isRunning()) {
        return;
    }
    {
        QMutexLocker k(&_imp->mustQuitMutex);
        _imp->mustQuit = true;

        // Push a fake element if needed
        {
            QMutexLocker k2(&_imp->executionQueueMutex);
            _imp->executionQueue.push_back(TreeRenderExecutionDataPtr());
            _imp->notifyManagerThreadForModifications_nolock();

        }

        while (_imp->mustQuit) {
            _imp->mustQuitCond.wait(&_imp->mustQuitMutex);
        }
    }
    wait();
} // quitThread

void
TreeRenderQueueManager::Implementation::launchMoreTasks()
{

    // Copy the execution queue so we do not hold the mutex
    std::list<TreeRenderExecutionDataWPtr> queue;
    {
        QMutexLocker k(&executionQueueMutex);

        // We are checking the queue now on the manager thread, refresh the activity check count to 0.
        activityCheckCount = 0;

        for (std::list<TreeRenderExecutionDataPtr>::iterator it = executionQueue.begin(); it != executionQueue.end(); ++it) {
            queue.push_back(*it);
        }
    }



    // Execute available tasks on the first requested render
    TreeRenderExecutionDataPtr firstRenderExecution;
    if (!queue.empty()) {
        firstRenderExecution = queue.front().lock();
    }
    TreeRenderPtr firstRenderTree;
    if (firstRenderExecution) {
        firstRenderTree = firstRenderExecution->getTreeRender();
    }

    if (!firstRenderTree) {
        // Nothing to do
        return;
    }


    TreeRenderQueueProviderPtr provider;
    if (firstRenderTree) {
        provider = boost::const_pointer_cast<TreeRenderQueueProvider>(firstRenderTree->getProvider());
        assert(provider);
    }


    // In one run of launchMoreTasks(), start at most maxTasksToLaunch.
    const int maxParallelTasks = QThreadPool::globalInstance()->maxThreadCount();
    const int maxTasksToLaunch = std::max(1, maxParallelTasks  - QThreadPool::globalInstance()->activeThreadCount());

    // Start as many concurrent renders as we can on the first task: this is the oldest task that was requested by the user
    int nTasksLaunched = firstRenderExecution->executeAvailableTasks(-1);

    // A TreeRender may not allow rendering of concurrent TreeRenders (e.g: when drawing, to ensure renders are processed in order)
    const bool allowConcurrentRenders = firstRenderTree->isConcurrentRendersAllowed();


    // If we've got remaining threads, cycle through other execution trees in the queue to launch other tasks
    // until we reach the max threads count
    std::list<TreeRenderExecutionDataWPtr>::iterator it = queue.begin();
    ++it; // skip the first execution
    for (; it != queue.end(); ++it) {

        if (nTasksLaunched >= maxTasksToLaunch) {
            break;
        }
        TreeRenderExecutionDataPtr renderExecution = it->lock();
        if (!renderExecution) {
            // Might happen when quitThread() is called
            continue;
        }
        if (!allowConcurrentRenders && renderExecution->getTreeRender() != firstRenderTree) {
            // This tree render may not allow concurrent executions.
            continue;
        }


        // Launch tasks in non priority render. Launch at most 1 parallel task in these render to let a chance to the first render in the queue
        // to use more threads.
        int nLaunched = renderExecution->executeAvailableTasks(1);
        nTasksLaunched += nLaunched;

    }


    // If we still have threads idle and the last request is playback, fetch more renders for that provider
    if (allowConcurrentRenders && firstRenderTree->isPlayback() && !provider->isWaitingForAllTreeRenders() && nTasksLaunched < maxTasksToLaunch) {

        // If more than maxThreadsCount renders are finished or launched, do not launch more for this provider
        bool providerMaxQueueReached = true;
        {
            QMutexLocker k(&perProviderRendersMutex);
            PerProviderRendersMap::iterator foundProvider = perProviderRenders.find(provider);
            // The provider may no longer be in the queue, because the executionQueue might be already empty since we made a copy of it.
            if (foundProvider != perProviderRenders.end()) {
                providerMaxQueueReached = ((int)foundProvider->second->finishedRenders.size() >= maxParallelTasks || (int)foundProvider->second->queuedRenders.size() >= maxParallelTasks);
            }
        }


        // Do not spawn more renders if the execution queue reaches the max threads count.
        // After that we know that these executions will never be launched before
        // at least one of the previous renders finishes.
        if (!providerMaxQueueReached) {
            provider->notifyNeedMoreRenders();
        }
    }
} // launchMoreTasks

void
LaunchRenderRunnable::run()
{
#ifdef DEBUG
    boost_adaptbx::floating_point::exception_trapping trap(boost_adaptbx::floating_point::exception_trapping::division_by_zero |
                                                           boost_adaptbx::floating_point::exception_trapping::invalid |
                                                           boost_adaptbx::floating_point::exception_trapping::overflow);
#endif
    TreeRenderExecutionDataPtr execData = render->createMainExecutionData();

    if (isFailureRetCode(execData->getStatus())) {
        render->setResults(FrameViewRequestPtr(), execData);
        imp->onTaskRenderFinished(execData);
    } else {
        imp->appendTreeRenderExecution(execData);

        // Notify the manager thread there may be tasks to render in the queue
        imp->notifyManagerThreadForModifications();
    }
}

void
TreeRenderQueueManager::run()
{
#ifdef DEBUG
    boost_adaptbx::floating_point::exception_trapping trap(boost_adaptbx::floating_point::exception_trapping::division_by_zero |
                                                           boost_adaptbx::floating_point::exception_trapping::invalid |
                                                           boost_adaptbx::floating_point::exception_trapping::overflow);
#endif
    for (;;) {

        {
            // Check for exit
            QMutexLocker k(&_imp->mustQuitMutex);
            if (_imp->mustQuit) {
                _imp->mustQuit = false;
                _imp->mustQuitCond.wakeOne();
                return;
            }
        }




        // Launch tasks if possible
        _imp->launchMoreTasks();

        
        // Try to sleep
        QMutexLocker k(&_imp->executionQueueMutex);
        while (_imp->canSleep()) {
            _imp->activityChangedCond.wait(&_imp->executionQueueMutex);
        }
    } // for(;;)
} // run

bool
TreeRenderQueueManager::Implementation::canSleep() const
{
    assert(!executionQueueMutex.tryLock());

    if (activityCheckCount > 0) {
        return false;
    }

    // Nothing changed
    return true;

} // canSleep

void TreeRenderQueueManager::Implementation::notifyManagerThreadForModifications()
{
    // Notify the manager thread something changed in the queue
    QMutexLocker k(&executionQueueMutex);
    notifyManagerThreadForModifications_nolock();

}

void TreeRenderQueueManager::Implementation::notifyManagerThreadForModifications_nolock()
{

    assert(!executionQueueMutex.tryLock());

    if (!_publicInterface->isRunning()) {
        _publicInterface->start();
    } else {
        ++activityCheckCount;
        activityChangedCond.wakeOne();
    }
}

NATRON_NAMESPACE_EXIT;
