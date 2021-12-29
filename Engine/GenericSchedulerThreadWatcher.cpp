/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2021 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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


#include "GenericSchedulerThreadWatcher.h"

#include <QtCore/QMetaType>
#include <QtCore/QWaitCondition>

#include "Engine/GenericSchedulerThread.h"
#include "Engine/OutputSchedulerThread.h"
#include "Engine/Node.h"
#ifdef DEBUG
#include "Global/FloatingPointExceptions.h"
#endif


NATRON_NAMESPACE_ENTER


NATRON_NAMESPACE_ANONYMOUS_ENTER

class GenericWatcherCallerArgsMetaTypesRegistration
{
public:
    inline GenericWatcherCallerArgsMetaTypesRegistration()
    {
        qRegisterMetaType<GenericWatcherCallerArgsPtr>("GenericWatcherCallerArgsPtr");
    }
};

NATRON_NAMESPACE_ANONYMOUS_EXIT
static GenericWatcherCallerArgsMetaTypesRegistration registration;
struct GenericWatcherPrivate
{
    struct Task
    {
        int id;
        GenericWatcherCallerArgsPtr args;
    };

    mutable QMutex tasksMutex;
    std::list<Task> tasks;
    bool mustQuit;
    QWaitCondition mustQuitCond;
    mutable QMutex mustQuitMutex;
    int startRequests;
    QWaitCondition startRequestsCond;
    mutable QMutex startRequestsMutex;

    GenericWatcherPrivate()
        : tasksMutex()
        , tasks()
        , mustQuit(false)
        , mustQuitCond()
        , mustQuitMutex()
        , startRequests(0)
    {
    }
};

GenericWatcher::GenericWatcher()
    : QThread()
    , _imp( new GenericWatcherPrivate() )
{
}

GenericWatcher::~GenericWatcher()
{
    stopWatching();
}

void
GenericWatcher::stopWatching()
{
    if ( !isRunning() ) {
        return;
    }
    {
        QMutexLocker k(&_imp->tasksMutex);
        _imp->tasks.clear();
    }

    {
        QMutexLocker quitLocker(&_imp->mustQuitMutex);
        _imp->mustQuit = true;


        // Post a stub request to wake up the thread
        {
            QMutexLocker k(&_imp->startRequestsMutex);
            ++_imp->startRequests;
            _imp->startRequestsCond.wakeOne();
        }
        while (_imp->mustQuit) {
            _imp->mustQuitCond.wait(quitLocker.mutex());
        }
    }

    wait();
}

void
GenericWatcher::run()
{
#ifdef DEBUG
    boost_adaptbx::floating_point::exception_trapping trap(boost_adaptbx::floating_point::exception_trapping::division_by_zero |
                                                           boost_adaptbx::floating_point::exception_trapping::invalid |
                                                           boost_adaptbx::floating_point::exception_trapping::overflow);
#endif
    for (;; ) {
        {
            QMutexLocker quitLocker(&_imp->mustQuitMutex);
            if (_imp->mustQuit) {
                _imp->mustQuit = false;
                _imp->mustQuitCond.wakeAll();

                return;
            }
        }
        int taskID = -1;
        GenericWatcherCallerArgsPtr inArgs;
        {
            QMutexLocker k(&_imp->tasksMutex);
            if ( !_imp->tasks.empty() ) {
                const GenericWatcherPrivate::Task& t = _imp->tasks.front();
                taskID = t.id;
                inArgs = t.args;
                _imp->tasks.pop_front();
            }
        }
        if (taskID != -1) {
            handleBlockingTask(taskID);
            Q_EMIT taskFinished(taskID, inArgs);
        }

        {
            QMutexLocker l(&_imp->startRequestsMutex);
            while (_imp->startRequests <= 0) {
                _imp->startRequestsCond.wait(l.mutex());
            }
            ///We got the request, reset it back to 0
            _imp->startRequests = 0;
        }
    } // for(;;)
}

void
GenericWatcher::scheduleBlockingTask(int taskID,
                                     const GenericWatcherCallerArgsPtr& args)
{
    {
        QMutexLocker quitLocker(&_imp->mustQuitMutex);
        if (_imp->mustQuit) {
            return;
        }
    }

    {
        QMutexLocker(&_imp->tasksMutex);
        GenericWatcherPrivate::Task t;
        t.id = taskID;
        t.args = args;
        _imp->tasks.push_back(t);
    }

    if ( !isRunning() ) {
        start();
    } else {
        ///Wake up the thread with a start request
        QMutexLocker locker(&_imp->startRequestsMutex);
        if (_imp->startRequests <= 0) {
            ++_imp->startRequests;
        }
        _imp->startRequestsCond.wakeOne();
    }
}

void
GenericSchedulerThreadWatcher::handleBlockingTask(int taskID)
{
    BlockingTaskEnum task = (BlockingTaskEnum)taskID;

    switch (task) {
    case eBlockingTaskWaitForAbort:
        _thread->abortThreadedTask();
        _thread->waitForAbortToComplete_not_main_thread();
        break;

    case eBlockingTaskWaitForQuitAllowRestart:
    case eBlockingTaskWaitForQuitDisallowRestart:
        _thread->quitThread(task == eBlockingTaskWaitForQuitAllowRestart);
        _thread->waitForThreadToQuit_not_main_thread();
        break;
    }
}

void
RenderEngineWatcher::handleBlockingTask(int taskID)
{
    BlockingTaskEnum task = (BlockingTaskEnum)taskID;

    switch (task) {
    case eBlockingTaskWaitForAbort:
        _engine->abortRenderingNoRestart();
        _engine->waitForAbortToComplete_not_main_thread();
        break;

    case eBlockingTaskWaitForQuitAllowRestart:
    case eBlockingTaskWaitForQuitDisallowRestart:
        _engine->quitEngine(task == eBlockingTaskWaitForQuitAllowRestart);
        _engine->waitForEngineToQuit_not_main_thread();
        break;
    }
}

void
NodeRenderWatcher::handleBlockingTask(int taskID)
{
    BlockingTaskEnum task = (BlockingTaskEnum)taskID;

    for (NodesList::const_iterator it = _nodes.begin(); it != _nodes.end(); ++it) {
        if (!*it) {
            continue;
        }
        switch (task) {
        case eBlockingTaskAbortAnyProcessing:
            (*it)->abortAnyProcessing_blocking();
            break;

        case eBlockingTaskQuitAnyProcessing:
            (*it)->quitAnyProcessing_blocking(false);
            break;
        }
    }
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_GenericSchedulerThreadWatcher.cpp"
