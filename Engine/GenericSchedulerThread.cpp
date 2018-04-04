/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
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

#include "GenericSchedulerThread.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>
#include <QtCore/QMetaType>
#include <QtCore/QDebug>

#ifdef DEBUG
#include "Global/FloatingPointExceptions.h"
#endif
#include "Engine/GenericSchedulerThreadWatcher.h"

#ifdef DEBUG
//#define TRACE_GENERIC_SCHEDULER_THREAD
#endif

NATRON_NAMESPACE_ENTER


NATRON_NAMESPACE_ANONYMOUS_ENTER

class GenericSchedulerThreadMetaTypesRegistration
{
public:
    inline GenericSchedulerThreadMetaTypesRegistration()
    {
        qRegisterMetaType<GenericThreadExecOnMainThreadArgsPtr>("GenericThreadExecOnMainThreadArgsPtr");
    }
};

NATRON_NAMESPACE_ANONYMOUS_EXIT
static GenericSchedulerThreadMetaTypesRegistration registration;
struct GenericSchedulerThreadPrivate
{
    GenericSchedulerThread* _p;

    // true when the thread must exit, protected by mustQuitMutex
    bool mustQuit;
    // true if we are allowed to start the thread, protected by mustQuitMutex
    bool startingThreadAllowed;
    // true if we are in quitThread
    // true if the last call to quitThread allowed starting threads, protected by mustQuitMutex
    bool lastQuitThreadAllowedRestart;
    QWaitCondition mustQuitCond;
    mutable QMutex mustQuitMutex;


    // positive when the user wants to stop the thread, protected by abortRequestedMutex
    int abortRequested;
    mutable QMutex abortRequestedMutex;
    mutable QWaitCondition abortRequestedCond;

    // The state of the thread protected by threadStateMutex
    GenericSchedulerThread::ThreadStateEnum threadState;
    mutable QMutex threadStateMutex;

    // The tasks queue, protected by enqueuedTasksMutex
    std::list<GenericThreadStartArgsPtr> enqueuedTasks, queuedTaskWhileProcessingAbort;
    QWaitCondition tasksEmptyCond;
    mutable QMutex enqueuedTasksMutex;

    // true when the main-thread is calling executeOnMainThread
    bool executingOnMainThread;
    QWaitCondition executingOnMainThreadCond;
    QMutex executingOnMainThreadMutex;

    // Only used on the main-thread
    boost::scoped_ptr<GenericSchedulerThreadWatcher> blockingOperationWatcher;

    GenericSchedulerThreadPrivate(GenericSchedulerThread* p)
    : _p(p)
    , mustQuit(false)
    , startingThreadAllowed(true)
    , lastQuitThreadAllowedRestart(true)
    , mustQuitCond()
    , mustQuitMutex()
    , abortRequested(0)
    , abortRequestedMutex()
    , abortRequestedCond()
    , threadState(GenericSchedulerThread::eThreadStateStopped)
    , threadStateMutex()
    , enqueuedTasks()
    , queuedTaskWhileProcessingAbort()
    , tasksEmptyCond()
    , enqueuedTasksMutex()
    , executingOnMainThread(false)
    , executingOnMainThreadCond()
    , executingOnMainThreadMutex()
    , blockingOperationWatcher()
    {
    }

    void setThreadState(GenericSchedulerThread::ThreadStateEnum state)
    {
        QMutexLocker k(&threadStateMutex);

        threadState = state;
    }

    // Returns the state of the thread
    GenericSchedulerThread::ThreadStateEnum resolveState();

    bool waitForAbortToComplete_internal(bool allowBlockingForMainThread);

    bool waitForThreadsToQuit_internal(bool allowBlockingForMainThread);
};

GenericSchedulerThread::GenericSchedulerThread()
    : QThread()
    , AbortableThread(this)
    , _imp( new GenericSchedulerThreadPrivate(this) )
{
    QObject::connect( this, SIGNAL(executionOnMainThreadRequested(GenericThreadExecOnMainThreadArgsPtr)), this, SLOT(onExecutionOnMainThreadReceived(GenericThreadExecOnMainThreadArgsPtr)) );
}

GenericSchedulerThread::~GenericSchedulerThread()
{
    // Ensure the thread is no longer running
    quitThread(false);

    // If blocking here from the main-thread you probably forgot to call waitForThreadToQuitQueued_main_thread() appropriately
    wait();
}

bool
GenericSchedulerThread::quitThread(bool allowRestarts, bool abortTask)
{
    if ( !isRunning() ) {
        return false;
    }

    // Disallow temporarily any thread to request a new render so that we do not end up starting the thread again
    // just after the abort
    {
        QMutexLocker k(&_imp->mustQuitMutex);
        // We already called quitThread
        if (_imp->mustQuit || !_imp->lastQuitThreadAllowedRestart) {
            return true;
        }
        _imp->mustQuit = true;
        _imp->startingThreadAllowed = false;
        _imp->lastQuitThreadAllowedRestart = allowRestarts;
    }

    if (abortTask && getThreadState() == eThreadStateActive) {
        abortThreadedTask();
    }


    // Clear any task enqueued and push a fake request
    {
        QMutexLocker k(&_imp->enqueuedTasksMutex);
        _imp->tasksEmptyCond.wakeOne();
    }

#ifdef TRACE_GENERIC_SCHEDULER_THREAD
    qDebug() << QThread::currentThread() << ": Termination request on " << getThreadName().c_str();
#endif

    onQuitRequested(allowRestarts);
    return true;
}

bool
GenericSchedulerThread::waitForThreadToQuit_not_main_thread()
{
    return _imp->waitForThreadsToQuit_internal(false);
}

bool
GenericSchedulerThreadPrivate::waitForThreadsToQuit_internal(bool allowBlockingForMainThread)
{
    if ( !_p->isRunning() ) {
        return false;
    }


    // This function may NOT be called on the main-thread, because we may deadlock if executeOnMainThread is called OR block the UI.
    if ( !allowBlockingForMainThread && ( QThread::currentThread() == qApp->thread() ) ) {
        return false;
    }

    {
        QMutexLocker k(&mustQuitMutex);
        while (mustQuit) {
            mustQuitCond.wait(&mustQuitMutex);
        }
    }

    // call pthread_join() to ensure the thread has stopped, this should be instantaneous anyway since we ensured we returned from the run() function already
    _p->wait();

    _p->onWaitForThreadToQuit();

    return true;
}

void
GenericSchedulerThread::waitForThreadToQuitQueued_main_thread(bool allowRestart)
{
    assert( QThread::currentThread() == qApp->thread() );
    // scoped_ptr
    _imp->blockingOperationWatcher.reset( new GenericSchedulerThreadWatcher(this) );
    QObject::connect( _imp->blockingOperationWatcher.get(), SIGNAL(taskFinished(int,WatcherCallerArgsPtr)), this, SLOT(onWatcherTaskFinishedEmitted()) );
    GenericSchedulerThreadWatcher::BlockingTaskEnum task = allowRestart ? GenericSchedulerThreadWatcher::eBlockingTaskWaitForQuitAllowRestart : GenericSchedulerThreadWatcher::eBlockingTaskWaitForQuitDisallowRestart;
    _imp->blockingOperationWatcher->scheduleBlockingTask(task);
}

bool
GenericSchedulerThread::waitForThreadToQuit_enforce_blocking()
{
    return _imp->waitForThreadsToQuit_internal(true);
}

void
GenericSchedulerThread::onWatcherTaskAbortedEmitted()
{
    if (!_imp->blockingOperationWatcher) {
        return;
    }
    _imp->blockingOperationWatcher.reset();
    Q_EMIT taskAborted();
}

void
GenericSchedulerThread::onWatcherTaskFinishedEmitted()
{
    if (!_imp->blockingOperationWatcher) {
        return;
    }
    _imp->blockingOperationWatcher.reset();
    Q_EMIT threadFinished();
}

GenericSchedulerThread::ThreadStateEnum
GenericSchedulerThreadPrivate::resolveState()
{
    GenericSchedulerThread::ThreadStateEnum ret = GenericSchedulerThread::eThreadStateActive;

    // flag that we have quit the thread
    {
        QMutexLocker k(&mustQuitMutex);
        if (mustQuit) {
            mustQuit = false;
            startingThreadAllowed = lastQuitThreadAllowedRestart;
            ret = GenericSchedulerThread::eThreadStateStopped;
            // Wake up threads waiting in waitForThreadToQuit
            mustQuitCond.wakeAll();
        }
    }

    {
        QMutexLocker k(&abortRequestedMutex);
        if (abortRequested > 0) {
            if (ret == GenericSchedulerThread::eThreadStateActive) {
                ret = GenericSchedulerThread::eThreadStateAborted;
            }
        }
    }


    return ret;
}

bool
GenericSchedulerThread::mustQuitThread() const
{
    QMutexLocker k(&_imp->mustQuitMutex);

    return _imp->mustQuit;
}

GenericSchedulerThread::ThreadStateEnum
GenericSchedulerThread::getThreadState() const
{
    QMutexLocker k(&_imp->threadStateMutex);

    return _imp->threadState;
}

bool
GenericSchedulerThread::abortThreadedTask(bool keepOldestRender)
{
    if ( !isRunning() ) {
        return false;
    }

    ThreadStateEnum state = getThreadState();
    bool shouldRequestAbort = state != eThreadStateIdle;

    if (shouldRequestAbort) {
        QMutexLocker l(&_imp->abortRequestedMutex);

#ifdef TRACE_GENERIC_SCHEDULER_THREAD
        qDebug() << QThread::currentThread() << ": Aborting task on" <<  getThreadName().c_str();
#endif
        ++_imp->abortRequested;

    }

    onAbortRequested(keepOldestRender);

    return true;
}

bool
GenericSchedulerThread::isBeingAborted() const
{
    QMutexLocker l(&_imp->abortRequestedMutex);

    return _imp->abortRequested > 0;
}

bool
GenericSchedulerThread::waitForAbortToComplete_not_main_thread()
{
    return _imp->waitForAbortToComplete_internal(false);
}

bool
GenericSchedulerThread::waitForAbortToComplete_enforce_blocking()
{
    return _imp->waitForAbortToComplete_internal(true);
}

bool
GenericSchedulerThreadPrivate::waitForAbortToComplete_internal(bool allowBlockingForMainThread)
{
    if ( !_p->isRunning() ) {
        return true;
    }

    GenericSchedulerThread::ThreadStateEnum state;
    {
        QMutexLocker k(&threadStateMutex);
        state = threadState;
    }
    if ( (state == GenericSchedulerThread::eThreadStateAborted) || GenericSchedulerThread::eThreadStateIdle ) {
        return false;
    }

    // This function may NOT be called on the main-thread, because we may deadlock if executeOnMainThread is called OR block the UI.
    if ( !allowBlockingForMainThread && ( QThread::currentThread() == qApp->thread() ) ) {
        return false;
    }


    {
        // Flag that we are going to be waiting on the main-thread so that the render thread does not attempt to execute something on the main-thread
        QMutexLocker k(&abortRequestedMutex);

        while (abortRequested > 0) {
            abortRequestedCond.wait(&abortRequestedMutex);
        }
    }
    _p->onWaitForAbortCompleted();

    return true;
}

void
GenericSchedulerThread::waitForAbortToCompleteQueued_main_thread()
{
    assert( QThread::currentThread() == qApp->thread() );
    // scoped_ptr
    _imp->blockingOperationWatcher.reset( new GenericSchedulerThreadWatcher(this) );
    QObject::connect( _imp->blockingOperationWatcher.get(), SIGNAL(taskFinished(int,WatcherCallerArgsPtr)), this, SLOT(onWatcherTaskAbortedEmitted()) );
    _imp->blockingOperationWatcher->scheduleBlockingTask(GenericSchedulerThreadWatcher::eBlockingTaskWaitForAbort);
}

bool
GenericSchedulerThread::startTask(const GenericThreadStartArgsPtr& inArgs)
{
    {
        QMutexLocker quitLocker(&_imp->mustQuitMutex);
        if (!_imp->startingThreadAllowed || _imp->mustQuit) {
            return false;
        }
    }

    bool running = isRunning();
    {
        QMutexLocker k1(&_imp->enqueuedTasksMutex);
        QMutexLocker k2(&_imp->abortRequestedMutex);
#ifdef TRACE_GENERIC_SCHEDULER_THREAD
        qDebug() << QThread::currentThread() << ": Requesting task on" <<  getThreadName().c_str() << ", is thread aborted?" << (bool)(_imp->abortRequested > 0);
#endif
        if (_imp->abortRequested > 0) {
            _imp->queuedTaskWhileProcessingAbort.push_back(inArgs);
        } else {
            _imp->enqueuedTasks.push_back(inArgs);
        }

        if (running) {
            _imp->tasksEmptyCond.wakeOne();
        }
    }
    if (!running) {
        start();
    }
    return true;
}

GenericSchedulerThread::ThreadStateEnum
GenericSchedulerThread::resolveState()
{
    return _imp->resolveState();
}

void
GenericSchedulerThread::run()
{
#ifdef DEBUG
    boost_adaptbx::floating_point::exception_trapping trap(boost_adaptbx::floating_point::exception_trapping::division_by_zero |
                                                           boost_adaptbx::floating_point::exception_trapping::invalid |
                                                           boost_adaptbx::floating_point::exception_trapping::overflow);
#endif
    for (;; ) {
        // Get the args to do the work
        TaskQueueBehaviorEnum behavior = tasksQueueBehaviour();
        ThreadStateEnum state = eThreadStateActive;
        {
            GenericThreadStartArgsPtr args;
            {
                QMutexLocker k(&_imp->enqueuedTasksMutex);
                switch (behavior) {
                    case eTaskQueueBehaviorProcessInOrder: {
                        if ( !_imp->enqueuedTasks.empty() ) {
                            args = _imp->enqueuedTasks.front();
                            _imp->enqueuedTasks.pop_front();
                        }
                        break;
                    }
                    case eTaskQueueBehaviorSkipToMostRecent: {
                        if ( !_imp->enqueuedTasks.empty() ) {
                            args = _imp->enqueuedTasks.back();
                            _imp->enqueuedTasks.pop_back();
                            std::list<GenericThreadStartArgsPtr> unskippableTasks;
                            for (std::list<GenericThreadStartArgsPtr>::iterator it = _imp->enqueuedTasks.begin(); it != _imp->enqueuedTasks.end(); ++it) {
                                if (!(*it)->isSkippable()) {
                                    unskippableTasks.push_back(*it);
                                }
                            }
                            _imp->enqueuedTasks = unskippableTasks;
                        }
                        break;
                    }
                }
            }
            {
                _imp->setThreadState(eThreadStateActive);
                Q_EMIT stateChanged( (int)state );
            }
            
            
            if ( args && !args->isNull() ) {
                // Do the work!
                state = threadLoopOnce(args);
            }
        }  // GenericThreadStartArgsPtr args;
        
        // The implementation might call resolveState from threadLoopOnce. If not, it will return eThreadStateActive by default so make
        // sure resolveState was called at least once
        if (state == eThreadStateActive) {
            // Returns eThreadStateActive if the thread was not aborted, otherwise returns eThreadStateAborted if aborted or eThreadStateStopped
            // if we must stop
            state = _imp->resolveState();
        }

        // Handle abort and termination requests
        // If the thread has not been aborted, put it asleep
        if (state == eThreadStateActive) {
            state = eThreadStateIdle;
        }

        _imp->setThreadState(state);
        Q_EMIT stateChanged( (int)state );

        if (state == eThreadStateStopped) {
            return;
        }
        
        // Reset the abort requested flag:
        // If a thread A called abortThreadedTask() multiple times and the scheduler thread B was running in the meantime, it could very well
        // stop and wait in the tasksEmptyCond
        QMutexLocker tasksLocker(&_imp->enqueuedTasksMutex);
        {
            QMutexLocker k(&_imp->abortRequestedMutex);
            if (state == eThreadStateAborted) {
                // If the processing was aborted, clear task that were requested prior to the abort flag was passed, and insert the task that were posted
                // after the abort was requested up until now
#ifdef TRACE_GENERIC_SCHEDULER_THREAD
                qDebug() << getThreadName().c_str() << ": Thread going idle after being aborted. " << _imp->queuedTaskWhileProcessingAbort.size() << "tasks were pushed while abort being processed.";
#endif
                if (behavior == eTaskQueueBehaviorProcessInOrder) {
                    _imp->enqueuedTasks.clear();
                }

                _imp->enqueuedTasks.insert( _imp->enqueuedTasks.end(), _imp->queuedTaskWhileProcessingAbort.begin(), _imp->queuedTaskWhileProcessingAbort.end() );
                _imp->queuedTaskWhileProcessingAbort.clear();
            } else {
#ifdef TRACE_GENERIC_SCHEDULER_THREAD
                qDebug() << getThreadName().c_str() << ": Thread going idle normally.";
#endif
            }

            if (_imp->abortRequested > 0) {
                _imp->abortRequested = 0;
                _imp->abortRequestedCond.wakeAll();
            }
        }

        while (_imp->enqueuedTasks.empty() && _imp->queuedTaskWhileProcessingAbort.empty()) {

            state = _imp->resolveState();

            if (state == eThreadStateStopped) {
                return;
            }

            _imp->tasksEmptyCond.wait(&_imp->enqueuedTasksMutex);

        }
#ifdef TRACE_GENERIC_SCHEDULER_THREAD
        qDebug() << getThreadName().c_str() << ": Received start request";
#endif

    } // for (;;)
} // run()

void
GenericSchedulerThread::requestExecutionOnMainThread(const GenericThreadExecOnMainThreadArgsPtr& inArgs)
{
    // We must be within the run() function
    assert(QThread::currentThread() == this);
    assert(getThreadState() == eThreadStateActive);

    QMutexLocker processLocker (&_imp->executingOnMainThreadMutex);
    assert(!_imp->executingOnMainThread);
    _imp->executingOnMainThread = true;

    Q_EMIT executionOnMainThreadRequested(inArgs);

    while (_imp->executingOnMainThread) {
        _imp->executingOnMainThreadCond.wait(&_imp->executingOnMainThreadMutex);
    }
}

void
GenericSchedulerThread::onExecutionOnMainThreadReceived(const GenericThreadExecOnMainThreadArgsPtr& args)
{
    assert( QThread::currentThread() == qApp->thread() );
    executeOnMainThread(args);

    QMutexLocker processLocker (&_imp->executingOnMainThreadMutex);
    assert(_imp->executingOnMainThread);
    _imp->executingOnMainThread = false;

    // There can only be one-thread waiting and this is this the GenericSchedulerThread thread
    _imp->executingOnMainThreadCond.wakeOne();
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_GenericSchedulerThread.cpp"
