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

#include "TreeRenderQueueManager.h"

#include <QMutex>
#include <QWaitCondition>
#include <QThreadPool>

#include <QtConcurrentRun>

#include "Engine/FrameViewRequest.h"
#include "Engine/TreeRender.h"
#include "Engine/ThreadPool.h"

NATRON_NAMESPACE_ENTER;

struct PerProviderRenders
{

    // Any render launched in the launchRender() function is queue here
    // It is removed from this list after calling waitForRenderFinished() or waitForAnyTreeRenderFinished().
    std::list<TreeRenderPtr> launchedRenders;

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

struct TreeRenderQueueManager::Implementation
{
    TreeRenderQueueManager* _publicInterface;

    // Protects executionQueue
    QMutex executionQueueMutex;

    // If no work is requested, the TreeRenderQueueManager thread sleeps until renders are requested
    QWaitCondition executionQueueNotEmptyCond;

    // The main execution queue in order
    std::list<TreeRenderExecutionDataPtr> executionQueue;

    // Protected by executionQueueMutex
    // Count the number of active runnables that launched a Node render.
    // This is decremented in notifyTaskInRenderFinished() and incremented in the run() function
    int nActiveTasks;

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
    , executionQueueNotEmptyCond()
    , executionQueue()
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
                                   std::set<TreeRenderPtr>::iterator finishedRendersIt,
                                   std::list<TreeRenderPtr>::iterator launchedIt,
                                   PerProviderRendersMap::iterator providersMapIt);

    int launchMoreTasks(int nTasksHint);

    void notifyTaskInRenderFinishedInternal(const TreeRenderExecutionDataPtr& render, bool launchExtraRenders, bool isRunningInThreadPoolThread);
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

ActionRetCodeEnum
TreeRenderQueueManager::launchRender(const TreeRenderPtr& render)
{
    assert(render && render->getProvider());
    TreeRenderExecutionDataPtr execData = render->createMainExecutionData();
    if (isFailureRetCode(execData->getStatus())) {
        render->setResults(FrameViewRequestPtr(), execData->getStatus());
    } else {
        {
            QMutexLocker k(&_imp->perProviderRendersMutex);
            PerProviderRendersPtr& renders = _imp->perProviderRenders[render->getProvider()];
            if (!renders) {
                renders.reset(new PerProviderRenders);
            }
            renders->launchedRenders.push_back(render);
        }
        appendTreeRenderExecution(execData);
    }
    return render->getStatus();
} // launchRender

TreeRenderExecutionDataPtr
TreeRenderQueueManager::launchSubRender(const EffectInstancePtr& treeRoot,
                                        TimeValue time,
                                        ViewIdx view,
                                        const RenderScale& proxyScale,
                                        unsigned int mipMapLevel,
                                        const ImagePlaneDesc* planeParam,
                                        const RectD* canonicalRoIParam,
                                        const TreeRenderPtr& render)
{
    assert(render && render->getProvider());

#ifdef DEBUG
    // Check that the render was already registered with launchRender()
    {
        QMutexLocker k(&_imp->perProviderRendersMutex);
        PerProviderRendersPtr& renders = _imp->perProviderRenders[render->getProvider()];
        assert(renders);
        if (renders) {
            assert(std::find(renders->launchedRenders.begin(), renders->launchedRenders.end(), render) != renders->launchedRenders.end());
        }
    }
#endif


    TreeRenderExecutionDataPtr execData = render->createSubExecutionData(treeRoot, time, view, proxyScale, mipMapLevel, planeParam, canonicalRoIParam);
    if (isFailureRetCode(execData->getStatus())) {
        render->setResults(FrameViewRequestPtr(), execData->getStatus());
    } else {
        appendTreeRenderExecution(execData);
    }

    // The sub task is launched from a thread pool thread, so temporarily uncount it as an active task.
    // If we do not do so, if all maxThreadsCount are busy in this function, all threads would deadlock in the wait
    // call below.
    {
        QMutexLocker k(&_imp->executionQueueMutex);
        --_imp->nActiveTasks;
        _imp->executionQueueNotEmptyCond.wakeOne();
    }

    _imp->waitForTreeRenderExecutionFinished(execData);

    {
        QMutexLocker k(&_imp->executionQueueMutex);
        ++_imp->nActiveTasks;
    }


    return execData;
} // launchSubRender


// A helper RAII style class to release the thread from the thread-pool and reserve it back upon destruction
// if the current thread is a thread-pool thread.
class ReleaseTPThread_RAII
{
    bool releasedThread;

public:

    ReleaseTPThread_RAII()
    : releasedThread(false)
    {
        // We may potentially wait for a long time so release the thread if we are in a thread pool thread.
        if (isRunningInThreadPoolThread()) {
            QThreadPool::globalInstance()->releaseThread();
            releasedThread = true;
        }

    }

    ~ReleaseTPThread_RAII()
    {
        if (releasedThread) {
            QThreadPool::globalInstance()->reserveThread();
        }
    }
};

bool
TreeRenderQueueManager::hasTreeRendersLaunched(const TreeRenderQueueProviderConstPtr& provider) const
{
    QMutexLocker k(&_imp->perProviderRendersMutex);
    PerProviderRendersMap::const_iterator foundProvider = _imp->perProviderRenders.find(provider);
    if (foundProvider == _imp->perProviderRenders.end()) {
        return false;
    }
    return !foundProvider->second->launchedRenders.empty() || !foundProvider->second->finishedRenders.empty();
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
TreeRenderQueueManager::Implementation::waitForTreeRenderInternal(const TreeRenderPtr& render,
                                                                  std::set<TreeRenderPtr>::iterator finishedRendersIt,
                                                                  std::list<TreeRenderPtr>::iterator launchedIt,
                                                                  PerProviderRendersMap::iterator providersMapIt)
{
    PerProviderRenders& renders = *providersMapIt->second;

    renders.launchedRenders.erase(launchedIt);
    renders.finishedRenders.erase(finishedRendersIt);

    // Remove any render clone from this render
    render->cleanupRenderClones();

    // No more active renders for this provider
    if (renders.finishedRenders.empty() && renders.launchedRenders.empty()) {
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
    if (renders.launchedRenders.empty() && renders.finishedRenders.empty()) {
        return TreeRenderPtr();
    }

    // Wait for a render to finish
    while (!renders.launchedRenders.empty() && renders.finishedRenders.empty()) {
        _imp->perProviderRendersCond.wait(&_imp->perProviderRendersMutex);

        // Lookup the iterator again, it could have been invalidated since we released the mutex.
        foundProvider = _imp->perProviderRenders.find(provider);
        if (foundProvider == _imp->perProviderRenders.end()) {
            // No render has been requested for this provider
            return TreeRenderPtr();
        }
        renders = *foundProvider->second;

        // Nothing was launched
        if (renders.launchedRenders.empty() && renders.finishedRenders.empty()) {
            return TreeRenderPtr();
        }

    }

    assert(!renders.finishedRenders.empty());


    // Remove the first finished render
    std::set<TreeRenderPtr>::iterator foundFinishedRender = renders.finishedRenders.begin();
    TreeRenderPtr ret = *foundFinishedRender;

    // Also remove from the launched list
    std::list<TreeRenderPtr>::iterator foundLaunched = std::find(renders.launchedRenders.begin(), renders.launchedRenders.end(), ret);
    assert(foundLaunched != renders.launchedRenders.end());

    _imp->waitForTreeRenderInternal(ret, foundFinishedRender, foundLaunched, foundProvider);
    return ret;
} // waitForAnyTreeRenderFinished

void
TreeRenderQueueManager::waitForRenderFinished(const TreeRenderPtr& render)
{
    ReleaseTPThread_RAII tpThreadRelease;

    QMutexLocker k(&_imp->perProviderRendersMutex);

    PerProviderRendersMap::iterator foundProvider = _imp->perProviderRenders.find(render->getProvider());
    if (foundProvider == _imp->perProviderRenders.end()) {
        // No render has been requested for this provider
        return;
    }

    PerProviderRenders& renders = *foundProvider->second;

    std::list<TreeRenderPtr>::iterator foundLaunched = std::find(renders.launchedRenders.begin(), renders.launchedRenders.end(), render);
    if (foundLaunched == renders.launchedRenders.end()) {
        // The render was never launched...
        return;
    }

    std::set<TreeRenderPtr>::iterator foundFinishedRender = renders.finishedRenders.find(render);
    while (foundFinishedRender == renders.finishedRenders.end()) {
        _imp->perProviderRendersCond.wait(&_imp->perProviderRendersMutex);

        // Lookup the iterator again, it could have been invalidated since we released the mutex.
        foundProvider = _imp->perProviderRenders.find(render->getProvider());
        if (foundProvider == _imp->perProviderRenders.end()) {
            // No render has been requested for this provider
            return;
        }
        renders = *foundProvider->second;
        foundLaunched = std::find(renders.launchedRenders.begin(), renders.launchedRenders.end(), render);
        if (foundLaunched == renders.launchedRenders.end()) {
            // The render was never launched...
            return;
        }


        foundFinishedRender = renders.finishedRenders.find(render);
    }
    _imp->waitForTreeRenderInternal(render, foundFinishedRender, foundLaunched, foundProvider);

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
TreeRenderQueueManager::appendTreeRenderExecution(const TreeRenderExecutionDataPtr& render)
{
    // Push the execution to the queue and notify the thread
    {
        QMutexLocker k(&_imp->executionQueueMutex);
        _imp->executionQueue.push_back(render);

        if (!isRunning()) {
            start();
        } else {
            _imp->executionQueueNotEmptyCond.wakeOne();
        }
    }

} // appendTreeRenderExecution

void
TreeRenderQueueManager::Implementation::launchAndWaitExtraExecutionTasks(const TreeRenderExecutionDataPtr& mainFinishedExecution, const std::list<TreeRenderExecutionDataPtr>& extraExecutions)
{
    for (std::list<TreeRenderExecutionDataPtr>::const_iterator it = extraExecutions.begin(); it != extraExecutions.end(); ++it) {
        if (!isFailureRetCode((*it)->getStatus())) {
            _publicInterface->appendTreeRenderExecution(*it);
            waitForTreeRenderExecutionFinished(*it);
        }
    }

    notifyTaskInRenderFinishedInternal(mainFinishedExecution, false /*notifyTaskInRenderFinishedInternal*/, false);

} // launchAndWaitExtraExecutionTasks

void
TreeRenderQueueManager::Implementation::notifyTaskInRenderFinishedInternal(const TreeRenderExecutionDataPtr& render,
                                                                           bool launchExtraRenders,
                                                                           bool isRunningInThreadPoolThread)
{
    // Inform the TreeRenderQueueManager thread that one task is finished so it can start scheduling again
    if (isRunningInThreadPoolThread) {
        QMutexLocker k(&executionQueueMutex);
        // Note that this MAY decrease below 0 if this function is called in the TreeRenderQueueManager:
        // in other words if the task was not launched in a thread-pool thread
        // but instead in this thread, the TreeRenderQueueManager did not get a chance yet to increase _imp->nActiveTasks
        --nActiveTasks;
        executionQueueNotEmptyCond.wakeOne();
    }

    if (render->hasTasksToExecute()) {
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
            if (!isRunningInThreadPoolThread) {
                QtConcurrent::run(this, &TreeRenderQueueManager::Implementation::launchAndWaitExtraExecutionTasks, render, extraExecutions);
            } else {
                launchAndWaitExtraExecutionTasks(render, extraExecutions);
            }
            return;
        }
    }

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
        assert(found != executionQueue.end());
        if (found != executionQueue.end()) {
            executionQueue.erase(found);
        }
    }


    if (render->isTreeMainExecution()) {
        TreeRenderPtr treeRender = render->getTreeRender();
        TreeRenderQueueProviderPtr provider = boost::const_pointer_cast<TreeRenderQueueProvider>(treeRender->getProvider());
        provider->notifyTreeRenderFinished(treeRender);
    }
} // notifyTaskInRenderFinishedInternal

void
TreeRenderQueueManager::notifyTaskInRenderFinished(const TreeRenderExecutionDataPtr& render, bool isRunningInThreadPoolThread)
{
    _imp->notifyTaskInRenderFinishedInternal(render, true /*notifyTaskInRenderFinishedInternal*/, isRunningInThreadPoolThread);
    
} // notifyTaskInRenderFinished

void
TreeRenderQueueManager::quitThread()
{
    {
        QMutexLocker k(&_imp->mustQuitMutex);
        _imp->mustQuit = true;

        // Push a fake element if needed
        {
            QMutexLocker k2(&_imp->executionQueueMutex);
            if (!_imp->executionQueue.empty()) {
                _imp->executionQueue.push_back(TreeRenderExecutionDataPtr());
                _imp->executionQueueNotEmptyCond.wakeOne();
            }
        }

        while (!_imp->mustQuit) {
            _imp->mustQuitCond.wait(&_imp->mustQuitMutex);
        }
    }
    wait();
} // quitThread

int
TreeRenderQueueManager::Implementation::launchMoreTasks(int nTasksHint)
{
    QThreadPool* tp = QThreadPool::globalInstance();

    int nTasksLaunched = 0;

    // A TreeRender may not allow rendering of concurrent TreeRenders (e.g: when drawing, to ensure renders are processed in order)
    bool allowConcurrentRenders = true;

    // Copy the execution queue so we do not hold the mutex
    std::list<TreeRenderExecutionDataPtr> queue;
    {
        QMutexLocker k(&executionQueueMutex);
        queue = executionQueue;
    }

    // Execute available tasks on the first requested render
    TreeRenderExecutionDataPtr firstRenderExecution;
    if (!queue.empty()) {
        firstRenderExecution = queue.front();
    }
    TreeRenderPtr firstRenderTree;
    if (firstRenderExecution) {
        firstRenderTree = firstRenderExecution->getTreeRender();
    }

    if (!firstRenderTree) {
        // Nothing to do
        return nTasksLaunched;
    }


    TreeRenderQueueProviderPtr provider;
    if (firstRenderTree) {
        provider = boost::const_pointer_cast<TreeRenderQueueProvider>(firstRenderTree->getProvider());
        assert(provider);
    }

    const int maxThreadsCount = tp->maxThreadCount();

    int nTasksToLaunch = nTasksHint;

    // Start as many concurrent renders as we can on the first task: this is the oldest task that was requested by the user
    nTasksLaunched = firstRenderExecution->executeAvailableTasks(nTasksToLaunch);

    // If we launched at least as many tasks as required, stop
    if (nTasksLaunched >= nTasksHint) {
        return nTasksLaunched;
    }

    // Decrement the number of tasks remaining to launch by what was launched before
    nTasksToLaunch -= nTasksLaunched;

    allowConcurrentRenders = firstRenderTree->isConcurrentRendersAllowed();

    // If we've got remaining threads, cycle through other execution trees in the queue to launch other tasks
    // until we reach the max threads count
    std::list<TreeRenderExecutionDataPtr>::iterator it = queue.begin();
    ++it; // skip the first execution
    for (; it != queue.end(); ++it) {
        if (!(*it)) {
            // Might happen when quitThread() is called
            continue;
        }
        if (!allowConcurrentRenders && (*it)->getTreeRender() != firstRenderTree) {
            // This tree render may not allow concurrent executions.
            continue;
        }

        // If we reached the maximum threads count, stop
        int availableThreads = maxThreadsCount - tp->activeThreadCount();
        if (availableThreads <= 0) {
            return nTasksLaunched;
        }

        // Launch tasks
        int nLaunched = (*it)->executeAvailableTasks(nTasksToLaunch);
        nTasksLaunched += nLaunched;
        nTasksToLaunch -= nLaunched;

        if (nTasksLaunched >= nTasksHint || nTasksToLaunch <= 0) {
            return nTasksLaunched;
        }
    }

    // If we still have threads idle and the last request is playback, fetch more renders for that provider
    if (allowConcurrentRenders && firstRenderTree->isPlayback() && nTasksToLaunch > 0) {

        // If more than maxThreadsCount renders are finished or launched, do not launch more for this provider
        bool providerMaxQueueReached = true;
        {
            QMutexLocker k(&perProviderRendersMutex);
            PerProviderRendersMap::iterator foundProvider = perProviderRenders.find(provider);
            // The provider may no longer be in the queue, because the executionQueue might be already empty since we made a copy of it.
            if (foundProvider != perProviderRenders.end()) {
                providerMaxQueueReached = ((int)foundProvider->second->finishedRenders.size() >= maxThreadsCount || (int)foundProvider->second->launchedRenders.size() >= maxThreadsCount);
            }
        }


        // Do not spawn more renders if the execution queue reaches the max threads count.
        // After that we know that these executions will never be launched before
        // at least one of the previous renders finishes.
        if ((int)queue.size() < maxThreadsCount && !providerMaxQueueReached) {
            provider->notifyNeedMoreRenders();
        }
    }
    return nTasksLaunched;
} // launchMoreTasks

void
TreeRenderQueueManager::run()
{

    QThreadPool* tp = QThreadPool::globalInstance();


    for (;;) {

        const int maxTasksToLaunch = 2;//tp->maxThreadCount();

        {
            // Check for exit
            QMutexLocker k(&_imp->mustQuitMutex);
            if (_imp->mustQuit) {
                _imp->mustQuit = false;
                _imp->mustQuitCond.wakeOne();
                return;
            }
        }

        // Check the current number of active tasks
        int currentNActiveTasks;
        int currentNExecutions;
        {
            QMutexLocker k(&_imp->executionQueueMutex);
            currentNActiveTasks = _imp->nActiveTasks;
            currentNExecutions = (int)_imp->executionQueue.size();
        }

        int nTasksLaunched = 0;

        // Launch tasks if possible
        if (currentNActiveTasks < maxTasksToLaunch) {

            // Launch at most maxThreadsCount parallel runnables (where each runnable is the render of a node).
            // This will make sure that at least all cores are occupied with a render of a different node.
            // Node renders are prioritized by how far the TreeRenderExecutionDataPtr object is in the queue.
            // This hint is used in the MultiThread suite
            int nTasksToLaunch = maxTasksToLaunch - std::max(0,currentNActiveTasks);

            assert(nTasksToLaunch > 0);
            nTasksLaunched = _imp->launchMoreTasks(nTasksToLaunch);

            if (nTasksLaunched > 0) {
                QMutexLocker k(&_imp->executionQueueMutex);
                _imp->nActiveTasks += nTasksLaunched;
                currentNActiveTasks = _imp->nActiveTasks;
            }
        }
        
        // Wait if:
        // - No execution is requested or
        // - No new execution was requested and no task has finished since the last launch
        QMutexLocker k(&_imp->executionQueueMutex);
        while (_imp->executionQueue.empty() ||
               (((int)_imp->executionQueue.size() == currentNExecutions) && nTasksLaunched > 0 &&  _imp->nActiveTasks != 0 && _imp->nActiveTasks == currentNActiveTasks)) {
            _imp->executionQueueNotEmptyCond.wait(&_imp->executionQueueMutex);
        }
    } // for(;;)
} // run


NATRON_NAMESPACE_EXIT;
