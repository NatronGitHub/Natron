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

#include "Engine/AppManager.h"
#include "Engine/FrameViewRequest.h"
#include "Engine/TreeRender.h"
#include "Engine/ThreadPool.h"

NATRON_NAMESPACE_ENTER;



struct PerProviderRenders
{

    // Renders that are queued for launch but not yet started
    // This is removed from this list as soon as the render is started and inserted in queuedRenders
    std::set<TreeRenderPtr> nonStartedRenders;

    // Renders that are queued in render but not yet finished.
    // It is removed from this list after calling waitForRenderFinished() or waitForAnyTreeRenderFinished().
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

    // Renders to launch: renders are queued here in launchRender()
    // Once hitting the main loop of the manager thread, the render is started and removed from this list
    mutable QMutex nonStartedRendersQueueMutex;
    std::list<TreeRenderPtr> nonStartedRendersQueue;

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
    , nActiveTasks(0)
    , perProviderRendersMutex()
    , perProviderRendersCond()
    , perProviderRenders()
    , nonStartedRendersQueueMutex()
    , nonStartedRendersQueue()
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

    int launchMoreTasks(int nTasksHint);

    void launchTreeRender(const TreeRenderPtr& render);

    void notifyTaskInRenderFinishedInternal(const TreeRenderExecutionDataPtr& render, bool isExecutionFinished, bool launchExtraRenders, bool isRunningInThreadPoolThread);

    void onTaskRenderFinished(const TreeRenderExecutionDataPtr& render);

    void appendTreeRenderExecution(const TreeRenderExecutionDataPtr& render);

    inline std::list<TreeRenderPtr>::iterator findQueueRender(const TreeRenderPtr& render)
    {
        // Private - should not lock
        assert(!nonStartedRendersQueueMutex.tryLock());

        for (std::list<TreeRenderPtr>::iterator it = nonStartedRendersQueue.begin(); it != nonStartedRendersQueue.end(); ++it) {
            if (*it == render) {
                return it;
            }
        }
        return nonStartedRendersQueue.end();
    }

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

    {
        QMutexLocker k(&_imp->perProviderRendersMutex);

        PerProviderRendersPtr& renders = _imp->perProviderRenders[render->getProvider()];
        if (!renders) {
            renders.reset(new PerProviderRenders);
        }
        renders->nonStartedRenders.insert(render);
    }
    {
        QMutexLocker k(&_imp->nonStartedRendersQueueMutex);
        _imp->nonStartedRendersQueue.push_back(render);
    }
    if (!isRunning()) {
        start();
    } else {
        QMutexLocker k(&_imp->executionQueueMutex);
        _imp->executionQueueNotEmptyCond.wakeOne();
    }

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
                                        int concatenationFlags)
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


    TreeRenderExecutionDataPtr execData = render->createSubExecutionData(treeRoot, time, view, proxyScale, mipMapLevel, planeParam, canonicalRoIParam, concatenationFlags);
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

        {
            QMutexLocker k(&_imp->executionQueueMutex);
            --_imp->nActiveTasks;
            //_imp->executionQueueNotEmptyCond.wakeOne();
        }
    }
}


void
TreeRenderQueueManager::reserveTask()
{
    if (isRunningInThreadPoolThread()) {
        QThreadPool::globalInstance()->reserveThread();

        {
            QMutexLocker k(&_imp->executionQueueMutex);
            ++_imp->nActiveTasks;
        }
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
    return !foundProvider->second->nonStartedRenders.empty() || !foundProvider->second->queuedRenders.empty() || !foundProvider->second->finishedRenders.empty();
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
    if (renders.finishedRenders.empty() && renders.queuedRenders.empty() && renders.nonStartedRenders.empty()) {
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

    // Ensure the render is not in the non-started renders list.
    assert(renders.nonStartedRenders.find(ret) == renders.nonStartedRenders.end());

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

    std::set<TreeRenderPtr>::iterator foundNotStarted = renders.nonStartedRenders.find(render);
    std::set<TreeRenderPtr>::iterator foundLaunched = renders.queuedRenders.find(render);
    if (foundNotStarted == renders.nonStartedRenders.end() && foundLaunched == renders.queuedRenders.end()) {
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
        foundNotStarted = renders.nonStartedRenders.find(render);
        foundLaunched = renders.queuedRenders.find(render);
        if (foundNotStarted == renders.nonStartedRenders.end() && foundLaunched == renders.queuedRenders.end()) {
            // The render was never launched...
            return eActionStatusFailed;
        }


        foundFinishedRender = renders.finishedRenders.find(render);
    }

    std::set<TreeRenderPtr>::iterator foundQueued = renders.queuedRenders.find(render);
    assert(foundQueued != renders.queuedRenders.end());
    // Ensure the render is not in the non-started renders list.
    assert(renders.nonStartedRenders.find(render) == renders.nonStartedRenders.end());

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
            executionQueueNotEmptyCond.wakeOne();
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
    // Inform the TreeRenderQueueManager thread that one task is finished so it can start scheduling again
    if (isRunningInThreadPoolThread) {
        QMutexLocker k(&executionQueueMutex);
        // Note that this MAY decrease below 0 if this function is called in the TreeRenderQueueManager:
        // in other words if the task was not launched in a thread-pool thread
        // but instead in this thread, the TreeRenderQueueManager did not get a chance yet to increase _imp->nActiveTasks
        --nActiveTasks;
        //executionQueueNotEmptyCond.wakeOne();
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
            _imp->executionQueueNotEmptyCond.wakeOne();

        }

        while (_imp->mustQuit) {
            _imp->mustQuitCond.wait(&_imp->mustQuitMutex);
        }
    }
    wait();
} // quitThread

int
TreeRenderQueueManager::Implementation::launchMoreTasks(int nTasksHint)
{

    int nTasksLaunched = 0;

    // A TreeRender may not allow rendering of concurrent TreeRenders (e.g: when drawing, to ensure renders are processed in order)
    bool allowConcurrentRenders = true;

    // Copy the execution queue so we do not hold the mutex
    std::list<TreeRenderExecutionDataWPtr> queue;
    {
        QMutexLocker k(&executionQueueMutex);
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
        return nTasksLaunched;
    }


    TreeRenderQueueProviderPtr provider;
    if (firstRenderTree) {
        provider = boost::const_pointer_cast<TreeRenderQueueProvider>(firstRenderTree->getProvider());
        assert(provider);
    }
    
    const int maxConcurrentFrameRenders = 4; // QThreadPool::globalInstance()->maxThreadCount();

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
    std::list<TreeRenderExecutionDataWPtr>::iterator it = queue.begin();
    ++it; // skip the first execution
    for (; it != queue.end(); ++it) {
        TreeRenderExecutionDataPtr renderExecution = it->lock();
        if (!renderExecution) {
            // Might happen when quitThread() is called
            continue;
        }
        if (!allowConcurrentRenders && renderExecution->getTreeRender() != firstRenderTree) {
            // This tree render may not allow concurrent executions.
            continue;
        }


        // Launch tasks
        int nLaunched = renderExecution->executeAvailableTasks(nTasksToLaunch);
        nTasksLaunched += nLaunched;
        nTasksToLaunch -= nLaunched;

        if (nTasksLaunched >= nTasksHint || nTasksToLaunch <= 0) {
            return nTasksLaunched;
        }
    }

    // If we still have threads idle and the renders queue contain unstarted renders, launch their execution
    if (allowConcurrentRenders) {
        std::list<TreeRenderPtr> rendersToLaunch;
        {
            QMutexLocker k(&nonStartedRendersQueueMutex);
            while (nTasksToLaunch > 0) {
                std::list<TreeRenderPtr>::iterator it = nonStartedRendersQueue.begin();
                if (it == nonStartedRendersQueue.end()) {
                    break;
                }

                if (!(*it)->isConcurrentRendersAllowed()) {
                    // Don't launch the render if it doesn't allow concurrent executions
                    break;
                }

                --nTasksToLaunch;


                rendersToLaunch.push_back(*it);

                nonStartedRendersQueue.erase(it);
            }
        }
        for (std::list<TreeRenderPtr>::const_iterator it = rendersToLaunch.begin(); it != rendersToLaunch.end(); ++it) {
            // Remove from the non-started set on the provider
            {
                QMutexLocker k2(&perProviderRendersMutex);
                PerProviderRendersPtr& providerRenders = perProviderRenders[(*it)->getProvider()];
                std::set<TreeRenderPtr>::iterator foundNonStarted = providerRenders->nonStartedRenders.find((*it));
                assert(foundNonStarted != providerRenders->nonStartedRenders.end());
                providerRenders->nonStartedRenders.erase(foundNonStarted);
                providerRenders->queuedRenders.insert(*it);
            }
            launchTreeRender(*it);
        }
    }
    
    // If we still have threads idle and the last request is playback, fetch more renders for that provider
    if (allowConcurrentRenders && firstRenderTree->isPlayback() && !provider->isWaitingForAllTreeRenders() && nTasksToLaunch > 0) {

        // If more than maxThreadsCount renders are finished or launched, do not launch more for this provider
        bool providerMaxQueueReached = true;
        {
            QMutexLocker k(&perProviderRendersMutex);
            PerProviderRendersMap::iterator foundProvider = perProviderRenders.find(provider);
            // The provider may no longer be in the queue, because the executionQueue might be already empty since we made a copy of it.
            if (foundProvider != perProviderRenders.end()) {
                providerMaxQueueReached = ((int)foundProvider->second->finishedRenders.size() >= maxConcurrentFrameRenders || (int)foundProvider->second->queuedRenders.size() >= maxConcurrentFrameRenders);
            }
        }


        // Do not spawn more renders if the execution queue reaches the max threads count.
        // After that we know that these executions will never be launched before
        // at least one of the previous renders finishes.
        if ((int)queue.size() < maxConcurrentFrameRenders && !providerMaxQueueReached) {
            provider->notifyNeedMoreRenders();
        }
    }
    return nTasksLaunched;
} // launchMoreTasks

void
LaunchRenderRunnable::run()
{
    {
        QMutexLocker k(&imp->executionQueueMutex);
        ++imp->nActiveTasks;
        //imp->executionQueueNotEmptyCond.wakeOne();
    }
    TreeRenderExecutionDataPtr execData = render->createMainExecutionData();
    {
        QMutexLocker k(&imp->executionQueueMutex);
        --imp->nActiveTasks;
    }
    if (isFailureRetCode(execData->getStatus())) {
        render->setResults(FrameViewRequestPtr(), execData);
        imp->onTaskRenderFinished(execData);
    } else {
        imp->appendTreeRenderExecution(execData);
    }
}

void
TreeRenderQueueManager::Implementation::launchTreeRender(const TreeRenderPtr& render)
{
    QThreadPool::globalInstance()->start(new LaunchRenderRunnable(render, this));
} // launchTreeRender


void
TreeRenderQueueManager::run()
{

    QThreadPool* tp = QThreadPool::globalInstance();


    for (;;) {

        const int maxTasksToLaunch = tp->maxThreadCount();

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

        // If there's nothing in the queue, try to start any tree render execution
        if (currentNExecutions == 0) {
            TreeRenderPtr renderToStart;
            {
                QMutexLocker k(&_imp->nonStartedRendersQueueMutex);
                    if (!_imp->nonStartedRendersQueue.empty()) {
                    renderToStart = _imp->nonStartedRendersQueue.front();
                    _imp->nonStartedRendersQueue.pop_front();
                }
            }
            if (renderToStart) {
                // Remove from the non-started set on the provider
                {
                    QMutexLocker k2(&_imp->perProviderRendersMutex);
                    PerProviderRendersPtr& providerRenders = _imp->perProviderRenders[renderToStart->getProvider()];
                    std::set<TreeRenderPtr>::iterator foundNonStarted = providerRenders->nonStartedRenders.find(renderToStart);
                    assert(foundNonStarted != providerRenders->nonStartedRenders.end());
                    providerRenders->nonStartedRenders.erase(foundNonStarted);
                    providerRenders->queuedRenders.insert(renderToStart);
                }
                _imp->launchTreeRender(renderToStart);
                {
                    QMutexLocker k(&_imp->executionQueueMutex);
                    currentNActiveTasks = _imp->nActiveTasks;
                    currentNExecutions = (int)_imp->executionQueue.size();
                }
            }

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
        std::size_t nonStartedRendersQueueSize;
        {
            QMutexLocker k2(&_imp->nonStartedRendersQueueMutex);
            nonStartedRendersQueueSize = _imp->nonStartedRendersQueue.size();
        }
        while ((_imp->executionQueue.empty() && nonStartedRendersQueueSize == 0)/* ||
               (((int)_imp->executionQueue.size() == currentNExecutions) && nTasksLaunched > 0 &&  _imp->nActiveTasks != 0 && _imp->nActiveTasks == currentNActiveTasks)*/) {
            _imp->executionQueueNotEmptyCond.wait(&_imp->executionQueueMutex);

            QMutexLocker k2(&_imp->nonStartedRendersQueueMutex);
            nonStartedRendersQueueSize = _imp->nonStartedRendersQueue.size();
        }
    } // for(;;)
} // run


NATRON_NAMESPACE_EXIT;
