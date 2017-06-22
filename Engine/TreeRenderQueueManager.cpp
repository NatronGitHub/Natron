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

#include "Engine/FrameViewRequest.h"
#include "Engine/TreeRender.h"

NATRON_NAMESPACE_ENTER;

struct PerProviderRenders
{

    std::list<TreeRenderPtr> launchedRenders;
    std::set<TreeRenderPtr> finishedRenders;
};

typedef boost::shared_ptr<PerProviderRenders> PerProviderRendersPtr;

typedef std::map<TreeRenderQueueProviderConstWPtr, PerProviderRendersPtr> PerProviderRendersMap;

struct TreeRenderQueueManager::Implementation
{
    QMutex executionQueueMutex;
    QWaitCondition executionQueueNotEmptyCond;
    std::list<TreeRenderExecutionDataPtr> executionQueue;

    QMutex finishedExecutionsMutex;
    QWaitCondition finishedExecutionsCond;
    std::set<TreeRenderExecutionDataPtr> finishedExecutions;

    QMutex executionTasksFinishedCountMutex;
    QWaitCondition executionTasksFinishedCountCond;
    int executionTasksFinishedCount;

    QMutex finishedRendersMutex;
    QWaitCondition finishedRendersCond;
    PerProviderRendersMap perProviderRenders;

    QMutex mustQuitMutex;
    QWaitCondition mustQuitCond;
    bool mustQuit;

    Implementation()
    : executionQueueMutex()
    , executionQueueNotEmptyCond()
    , executionQueue()
    , finishedExecutionsMutex()
    , finishedExecutionsCond()
    , finishedExecutions()
    , executionTasksFinishedCountMutex()
    , executionTasksFinishedCountCond()
    , executionTasksFinishedCount(0)
    , finishedRendersMutex()
    , finishedRendersCond()
    , perProviderRenders()
    , mustQuitMutex()
    , mustQuitCond()
    , mustQuit(false)
    {

    }

};



TreeRenderQueueManager::TreeRenderQueueManager()
: QThread()
, _imp(new Implementation())
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
    TreeRenderExecutionDataPtr execData = render->createMainExecutionData();
    if (isFailureRetCode(execData->getStatus())) {
        render->setResults(FrameViewRequestPtr(), execData->getStatus());
    } else {
        {
            QMutexLocker k(&_imp->finishedRendersMutex);
            PerProviderRendersPtr& renders = _imp->perProviderRenders[render->getProvider()];
            if (!renders) {
                renders.reset(new PerProviderRenders);
            }
            renders->launchedRenders.push_back(render);
        }
        appendTreeRenderExecution(execData);
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
                                        const TreeRenderPtr& render)
{
    assert(render && render->getProvider());

#ifdef DEBUG
    // Check that the render was already registered with launchRender()
    {
        QMutexLocker k(&_imp->finishedRendersMutex);
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
    return execData;
} // launchSubRender

bool
TreeRenderQueueManager::hasTreeRendersFinished(const TreeRenderQueueProviderConstPtr& provider) const
{
    QMutexLocker k(&_imp->executionQueueMutex);
    PerProviderRendersMap::const_iterator foundProvider = _imp->perProviderRenders.find(provider);
    if (foundProvider == _imp->perProviderRenders.end()) {
        return false;
    }
    return !foundProvider->second->finishedRenders.empty();
}

TreeRenderPtr
TreeRenderQueueManager::waitForAnyTreeRenderFinished(const TreeRenderQueueProviderConstPtr& provider)
{
    QMutexLocker k(&_imp->finishedRendersMutex);

    PerProviderRendersMap::iterator foundProvider = _imp->perProviderRenders.find(provider);
    if (foundProvider == _imp->perProviderRenders.end()) {
        // No render has been requested for this provider
        return TreeRenderPtr();
    }

    PerProviderRenders& renders = *foundProvider->second;

    // Wait for a render to finish
    while (!renders.launchedRenders.empty() && renders.finishedRenders.empty()) {
        _imp->finishedRendersCond.wait(&_imp->finishedRendersMutex);

        // Lookup the iterator again, it could have been invalidated since we released the mutex.
        foundProvider = _imp->perProviderRenders.find(provider);
        if (foundProvider == _imp->perProviderRenders.end()) {
            // No render has been requested for this provider
            return TreeRenderPtr();
        }
        renders = *foundProvider->second;
    }

    TreeRenderPtr ret;
    if (renders.finishedRenders.empty()) {
        ret = *renders.finishedRenders.begin();
        renders.finishedRenders.erase(renders.finishedRenders.begin());

        std::list<TreeRenderPtr>::iterator foundLaunched = std::find(renders.launchedRenders.begin(), renders.launchedRenders.end(), ret);
        assert(foundLaunched != renders.launchedRenders.end());
        if (foundLaunched != renders.launchedRenders.end()) {
            renders.launchedRenders.erase(foundLaunched);
        }
    }

    // No more active renders for this provider
    if (renders.finishedRenders.empty() && renders.launchedRenders.empty()) {
        _imp->perProviderRenders.erase(foundProvider);
    }
    return ret;
} // waitForAnyTreeRenderFinished

void
TreeRenderQueueManager::waitForRenderFinished(const TreeRenderPtr& render)
{
    QMutexLocker k(&_imp->finishedRendersMutex);

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
        _imp->finishedRendersCond.wait(&_imp->finishedRendersMutex);

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
    renders.launchedRenders.erase(foundLaunched);
    renders.finishedRenders.erase(foundFinishedRender);

    // No more active renders for this provider
    if (renders.finishedRenders.empty() && renders.launchedRenders.empty()) {
        _imp->perProviderRenders.erase(foundProvider);
    }
} // waitForRenderFinished

void
TreeRenderQueueManager::waitForTreeRenderExecutionFinished(const TreeRenderExecutionDataPtr& execData)
{
    QMutexLocker k(&_imp->executionQueueMutex);

    std::set<TreeRenderExecutionDataPtr>::iterator foundFinishedRender = _imp->finishedExecutions.find(execData);
    while (foundFinishedRender == _imp->finishedExecutions.end()) {
        _imp->finishedExecutionsCond.wait(&_imp->executionQueueMutex);
        foundFinishedRender = _imp->finishedExecutions.find(execData);
    }
    _imp->finishedExecutions.erase(foundFinishedRender);
}

void
TreeRenderQueueManager::appendTreeRenderExecution(const TreeRenderExecutionDataPtr& render)
{

    QMutexLocker k(&_imp->executionQueueMutex);
    _imp->executionQueue.push_back(render);

    if (!isRunning()) {
        start();
    } else {
        _imp->executionQueueNotEmptyCond.wakeOne();
    }
}

void
TreeRenderQueueManager::notifyTaskInRenderFinished(const TreeRenderExecutionDataPtr& render)
{

    // Inform the TreeRenderQueueManager thread that one task is finished so it can start scheduling again
    {
        QMutexLocker k(&_imp->executionTasksFinishedCountMutex);
        ++_imp->executionTasksFinishedCount;
        _imp->executionTasksFinishedCountCond.wakeOne();
    }

    if (render->hasTasksToExecute()) {
        // The render execution has still tasks to render, nothing to do
        return;
    }

    // The execution is finished: if this is the main execution of a TreeRender, we may mark the render
    // finished.

    // The render may have requested extraneous images to render for the color-picker. Launch their execution now.
    // This should be fast since the results should be cached from the main execution of the tree.
    if (render->isTreeMainExecution()) {
        std::list<TreeRenderExecutionDataPtr> extraExecutions = render->getTreeRender()->getExtraRequestedResultsExecutionData();
        for (std::list<TreeRenderExecutionDataPtr>::const_iterator it = extraExecutions.begin(); it != extraExecutions.end(); ++it) {
            if (!isFailureRetCode((*it)->getStatus())) {
                appendTreeRenderExecution(*it);
                waitForTreeRenderExecutionFinished(*it);
            }
        }
    }

    {
        QMutexLocker k(&_imp->executionQueueMutex);
        // The execution must exist in the queue
        std::list<TreeRenderExecutionDataPtr>::iterator found = std::find(_imp->executionQueue.begin(), _imp->executionQueue.end(), render);
        if (found != _imp->executionQueue.end()) {
            _imp->executionQueue.erase(found);
        }
    }
    {
        QMutexLocker k(&_imp->finishedExecutionsMutex);
        _imp->finishedExecutions.insert(render);
        _imp->finishedExecutionsCond.wakeAll();
    }

    if (render->isTreeMainExecution()) {
        TreeRenderPtr treeRender = render->getTreeRender();


        QMutexLocker k(&_imp->finishedRendersMutex);
        PerProviderRendersPtr& providerData = _imp->perProviderRenders[treeRender->getProvider()];
        providerData->finishedRenders.insert(treeRender);
        _imp->finishedRendersCond.wakeAll();
    }


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
}

void
TreeRenderQueueManager::run()
{

    QThreadPool* tp = QThreadPool::globalInstance();


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

        // Cycle through the list of execution task and attempt to schedule jobs
        {
            QMutexLocker k(&_imp->executionTasksFinishedCountMutex);
            _imp->executionTasksFinishedCount = 0;
        }

        int nTasksLaunched = 0;

        TreeRenderExecutionDataPtr firstRender;

        // Execute available tasks on the first requested render
        {

            QMutexLocker k(&_imp->executionQueueMutex);

            if (!_imp->executionQueue.empty()) {
                firstRender = _imp->executionQueue.front();
            }

            // Start as many concurrent renders as we can on the first task: this is the oldest task that was requested by the user
            if (firstRender) {
                nTasksLaunched += firstRender->executeAvailableTasks(-1);

                // If we've got remaining threads,  cycle through other execution trees to launch other tasks

                std::list<TreeRenderExecutionDataPtr>::iterator it = _imp->executionQueue.begin();
                ++it;
                for (; it != _imp->executionQueue.end(); ++it) {
                    if (!(*it)) {
                        continue;
                    }
                    int availableThreads = tp->maxThreadCount() - tp->activeThreadCount();
                    if (availableThreads <= 0) {
                        break;
                    }

                    nTasksLaunched += (*it)->executeAvailableTasks(-1);
                }
            }
        }

        // If we still have threads idle and the last request is playback, fetch more renders
        if (firstRender && firstRender->getTreeRender()->isPlayback()) {
            int availableThreads = tp->maxThreadCount() - tp->activeThreadCount();
            if (availableThreads > 0) {
                TreeRenderQueueProviderPtr provider = boost::const_pointer_cast<TreeRenderQueueProvider>(firstRender->getTreeRender()->getProvider());
                TreeRenderPtr newRender = provider->fetchTreeRenderToLaunch();
                if (newRender) {
                    launchRender(newRender);
                }
            }
        }

        // If we launched some tasks, wait for at least 1 to finish before repeating again
        {
            QMutexLocker k(&_imp->executionTasksFinishedCountMutex);
            while (nTasksLaunched > 0 && _imp->executionTasksFinishedCount == 0) {
                _imp->executionTasksFinishedCountCond.wait(&_imp->executionTasksFinishedCountMutex);
            }
        }

        // Wait if no task is available
        QMutexLocker k(&_imp->executionQueueMutex);
        while (_imp->executionQueue.empty()) {
            _imp->executionQueueNotEmptyCond.wait(&_imp->executionQueueMutex);
        }
    } // for(;;)
}


NATRON_NAMESPACE_EXIT;
