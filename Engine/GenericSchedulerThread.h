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

#ifndef GENERICSCHEDULERTHREAD_H
#define GENERICSCHEDULERTHREAD_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <QThread>

#include "Engine/EngineFwd.h"
#include "Engine/ThreadPool.h"

NATRON_NAMESPACE_ENTER


class GenericThreadStartArgs
{
    // Used to wake-up the thread when calling abortThreadedTask
    bool _isNull;

public:


    GenericThreadStartArgs(bool isNullTask = false)
        : _isNull(isNullTask)
    {
    }

    virtual ~GenericThreadStartArgs() {}

    bool isNull() const
    {
        return _isNull;
    }
};

class GenericThreadExecOnMainThreadArgs
{
public:

    GenericThreadExecOnMainThreadArgs()
    {
    }

    virtual ~GenericThreadExecOnMainThreadArgs()
    {
    }
};

/**
 * @brief Implements a generic thread class.
 * This class can start an action, abort it, quit the thread.
 * This class is used for rendering or tracking.
 *
 * Here is an example usage of this thread class:
 *
 * Create a thread:
 *
 *      GenericSchedulerThread thread;
 *
 * Request work: way:
 *      thread.startTask(task);
 *
 * Abort ongoing work from a thread DIFFERENT THAN THE MAIN-THREAD. Note that this is not blocking! The thread may not be aborted after the return of this call:
 *      thread.abortThreadedTask();
 *
 * To wait for the abortion to complete, call: (this is blocking)
 *      thread.waitForAbortToComplete_not_main_thread();
 *
 * To stop the thread from running from a thread DIFFERENT THAN THE MAIN-THREAD and abort any ongoing work:
 *      thread.quitThread();

 * Note that this is not blocking either, to actually ensure that the thread is not running anymore, call:
 *      thread.waitForThreadToQuit_not_main_thread();
 *
 * To abort (blocking) ongoing work FROM THE MAIN-THREAD connect to the signal taskAborted() to be notified when done and call:
 *      thread.waitForAbortToCompleteQueued_main_thread();
 *
 * To stop the thread from running FROM THE MAIN-THREAD connect to the signal threadFinished() to be notified when done and call:
 *      waitForThreadToQuitQueued_main_thread();
 *
 **/
struct GenericSchedulerThreadPrivate;
class GenericSchedulerThread
    : public QThread
      , public AbortableThread
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    enum ThreadStateEnum
    {
        // The thread is not running, this class may safely be destroyed
        eThreadStateStopped,

        // The thread is running, but waiting for work to be done, you must call quitThread before destroying the object
        eThreadStateIdle,

        // Same as eThreadStateIdle, except that it indicates the thread was aborted by user request
        eThreadStateAborted,

        // The thread is running and doing work, you must call quiThread before destroying the object. To stop any ongoing
        // task, call abortThreadedTask
        eThreadStateActive
    };

    enum TaskQueueBehaviorEnum
    {
        // When picking up a task to process...:

        // The thread will process in order the tasks enqueued by startTask
        eTaskQueueBehaviorProcessInOrder,

        // The thread will process the most recently enqueued tasks and discard all otherz
        eTaskQueueBehaviorSkipToMostRecent
    };

    GenericSchedulerThread();

    virtual ~GenericSchedulerThread();

    /**
     * @brief Requests the thread to finish its execution.
     * @param allowRestarts, if true, this object may be used again. If false this
     * object will no longer be usable.
     * Note: this function is not blocking and the thread may still be running when returning this function,
     * @returns false if the thread was not running, true otherwise.
     **/
    bool quitThread(bool allowRestarts);

    /**
     * @brief True if quitThread() was called and the thread did not finish yet
     **/
    bool mustQuitThread() const;

    /**
     * @brief Blocks the calling thread until
     * the thread associated with this object has finished execution (i.e. when it returns from run()). This function will return true if the thread has finished. It also returns true if the thread has not been started yet.
     * This function may NOT be called on the main-thread, because we may deadlock if executeOnMainThread is called OR block the UI. If called on the main-thread, false will be returned.
     * Rather, if you are on the main-thread, call waitForThreadToQuitQueued_main_thread() or use a GenericSchedulerThreadWatcher to perform the wait and let it notify you with a signal when the operation is done.
     **/
    bool waitForThreadToQuit_not_main_thread();

    /**
     * @brief Same as waitForThreadToQuit_not_main_thread() but can be safely called on the main-thread (and should be only called on the main-thread).
     * The thread may NOT yet be finished when returning from this function.
     * Connect to the signal threadFinished() to be notified when the thread is really done.
     **/
    void waitForThreadToQuitQueued_main_thread(bool allowRestart);

    /**
     * @brief Should be used as last resort, or if we necessarily wants the quit operation to be blocking.
     **/
    bool waitForThreadToQuit_enforce_blocking();

    /**
     * @brief Returns the thread state
     **/
    ThreadStateEnum getThreadState() const;

    /**
     * @brief Returns true if the scheduler is active and working
     **/
    bool isWorking() const
    {
        return getThreadState() == eThreadStateActive;
    }

    /**
     * @brief Requests the thread to execute the given task. Inherit GenericThreadStartArgs to customize the action.
     * The task is what will be passed to threadLoopOnce().
     * @returns true if the task could be enqueued, false otherwise.
     **/
    bool startTask(const GenericThreadStartArgsPtr& inArgs);

    /**
     * @brief Blocks the calling thread until
     * the thread state is now eThreadStateIdle (i.e. the thread is waiting). This function will return true if the thread is idling. It also returns true if the thread has not been started yet.
     * This function may NOT be called on the main-thread, because we may deadlock if executeOnMainThread is called OR block the UI. If called on the main-thread, false will be returned.
     * Rather, if you are on the main-thread, call waitForAbortToCompleteQueued_main_thread or use a GenericSchedulerThreadWatcher to perform the wait and connect to the signal taskAborted()
     * to be notified when the task is really done.
     **/
    bool waitForAbortToComplete_not_main_thread();

    /**
     * @brief Should be used as last resort, or if we necessarily wants an abort operation to be blocking.
     **/
    bool waitForAbortToComplete_enforce_blocking();

    /**
     * @brief Same as waitForAbortToComplete_not_main_thread() but can be safely called on the main-thread (and should be only called on the main-thread).
     * The task may NOT yet be aborted when returning from this function.
     * Connect to the signal taskAborted() to be notified when the task is really done.
     **/
    void waitForAbortToCompleteQueued_main_thread();


    /**
     * @brief Returns true if the thread is being aborted but it has not yet been processed
     **/
    bool isBeingAborted() const;

Q_SIGNALS:

    /**
     * @brief Triggered whenever this thread has a new state.
     * @param state The new state of the thread, @see ThreadStateEnum
     **/
    void stateChanged(int state);

    // Emitted by requestExecutionOnMainThread
    void executionOnMainThreadRequested(GenericThreadExecOnMainThreadArgsPtr args);

    void taskAborted();

    void threadFinished();

public Q_SLOTS:

    /**
     * @brief Requests to abort all computations.
     * This function is not blocking and once returned you may NOT assume that the thread is completely aborted.
     * @returns true if the thread was running and actively working and we did not post any abort request before, false otherwise
     **/
    bool abortThreadedTask(bool keepOldestRender = true);

    void onWatcherTaskAbortedEmitted();

    void onWatcherTaskFinishedEmitted();

private Q_SLOTS:

    void onExecutionOnMainThreadReceived(const GenericThreadExecOnMainThreadArgsPtr& args);

protected:

    /**
     * @brief How to pick the task to process from the consumer thread
     **/
    virtual TaskQueueBehaviorEnum tasksQueueBehaviour() const = 0;

    /**
     * @brief Called whenever abort is requested
     **/
    virtual void onAbortRequested(bool keepOldestRender) { Q_UNUSED(keepOldestRender); }

    /**
     * @brief Called when quitThread has been requested
     **/
    virtual void onQuitRequested(bool /*allowRestarts*/) {}

    /**
     * @brief Called when we successfully waited for an abort to be processed. This can be used to wait for extra threads
     **/
    virtual void onWaitForAbortCompleted() {}

    /**
     * @brief Called when we successfully waited for the thread to exit the run function. This can be used to wait for extra threads
     **/
    virtual void onWaitForThreadToQuit() {}

    /**
     * @brief Must be implemented to execute the work of the thread for 1 loop. This function will be called in a infinite loop by the thread
     * @return The state of the thread. By default should be eThreadStateActive. If threadLoopOnce might be long, you can periodically check
     * resolveState() to figure out if the user aborted the computation or not, in which case you need to return the value it returned.
     **/
    virtual ThreadStateEnum threadLoopOnce(const GenericThreadStartArgsPtr& inArgs) = 0;

    /**
     * @brief To be implemented if your implementation of threadLoopOnce() wants to use requestExecutionOnMainThread.
     **/
    virtual void executeOnMainThread(const GenericThreadExecOnMainThreadArgsPtr& /*inArgs*/) {}


    /**
     * @brief May be called in threadLoopOnce to figure out if the task was aborted or requested to finish
     **/
    ThreadStateEnum resolveState();

    /**
     * @brief Requests the function executeOnMainThread to be called on the main-thread with the given arguments.
     * This function is blocking and will return only when the client code in executeOnMainThread has returned.
     * You may only call this function from this thread, i.e: the thread that is running the run() function.
     * You may only call this function whilst the thread state is eThreadStateActive
     **/
    void requestExecutionOnMainThread(const GenericThreadExecOnMainThreadArgsPtr& inArgs);

private:

    virtual void run() OVERRIDE FINAL;

    friend struct GenericSchedulerThreadPrivate;
    boost::scoped_ptr<GenericSchedulerThreadPrivate> _imp;
};

NATRON_NAMESPACE_EXIT

#endif // GENERICSCHEDULERTHREAD_H
