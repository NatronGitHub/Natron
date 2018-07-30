/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://natrongithub.github.io/>,
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

#ifndef GENERICSCHEDULERTHREADWATCHER_H
#define GENERICSCHEDULERTHREADWATCHER_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <list>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#endif

#include <QtCore/QThread>

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

/**
 * @brief The purpose of this class is to safely destroy a GenericSchedulerThread and to get notify whenever the GenericSchedulerThread abortion is complete
 * or whenever it is finished (i.e: waitForThreadToQuit() has returned true)
 *
 * Basically, if you need to ensure that a GenericSchedulerThread has finished, you would create a GenericSchedulerThreadWatcher with the thread in parameter
 * and connect a slot to the threadFinished() signal. Whenever executing this slot, it is then safe to assume that the thread is finished and you can also
 * destroy the watcher.
 *
 * You should never use the GenericSchedulerThreadWatcher to perform multiple tasks, because it is then uncertain for which task, which slot you are going to be receving,
 * even though the class is safely implemented to support multiple tasks.
 *
 * Note: the GenericSchedulerThread object should live as long as this object lives.
 **/

class GenericWatcherCallerArgs
{
public:
    GenericWatcherCallerArgs()
    {
    }

    virtual ~GenericWatcherCallerArgs()
    {
    }
};


struct GenericWatcherPrivate;
class GenericWatcher
    : public QThread
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:


    GenericWatcher();

    ~GenericWatcher();

    /**
     * @brief Start watching the given taskID. In derived implementation taskID should be handled in handleBlockingTask() and typically casted to an enum for clarity.
     * @params inArgs This can be derived from GenericWatcherCallerArgs to store any arguments relevant to the local function you were in when calling scheduleBlockingTask()
     * and that you want to retrieve when the taskFinished() signal is emitted.
     **/
    void scheduleBlockingTask( int taskID, const GenericWatcherCallerArgsPtr& inArgs = GenericWatcherCallerArgsPtr() );

    void stopWatching();

Q_SIGNALS:

    void taskFinished(int taskID, GenericWatcherCallerArgsPtr args);

protected:


    virtual void handleBlockingTask(int taskID) = 0;

private:

    virtual void run() OVERRIDE FINAL;
    boost::scoped_ptr<GenericWatcherPrivate> _imp;
};


class GenericSchedulerThreadWatcher
    : public GenericWatcher
{
public:

    // The kind of tasks that this watcher supports, each of them will call the corresponding wait function on the GenericSchedulerThread
    // and emit the signal when appropriate
    enum BlockingTaskEnum
    {
        eBlockingTaskWaitForAbort,
        eBlockingTaskWaitForQuitDisallowRestart,
        eBlockingTaskWaitForQuitAllowRestart,
    };


    GenericSchedulerThreadWatcher(GenericSchedulerThread* thread)
        : GenericWatcher()
        , _thread(thread)
    {
    }

    virtual ~GenericSchedulerThreadWatcher()
    {
    }

private:

    virtual void handleBlockingTask(int taskID) OVERRIDE FINAL;
    GenericSchedulerThread* _thread;
};


class RenderEngineWatcher
    : public GenericWatcher
{
public:

    // The kind of tasks that this watcher supports, each of them will call the corresponding wait function on the GenericSchedulerThread
    // and emit the signal when appropriate
    enum BlockingTaskEnum
    {
        eBlockingTaskWaitForAbort,
        eBlockingTaskWaitForQuitDisallowRestart,
        eBlockingTaskWaitForQuitAllowRestart,
    };


    RenderEngineWatcher(RenderEngine* engine)
        : GenericWatcher()
        , _engine(engine)
    {
    }

    virtual ~RenderEngineWatcher()
    {
    }

private:

    virtual void handleBlockingTask(int taskID) OVERRIDE FINAL;
    RenderEngine* _engine;
};


class NodeRenderWatcher
    : public GenericWatcher
{
public:

    // The kind of tasks that this watcher supports, each of them will call the corresponding wait function on the GenericSchedulerThread
    // and emit the signal when appropriate
    enum BlockingTaskEnum
    {
        eBlockingTaskQuitAnyProcessing,
        eBlockingTaskAbortAnyProcessing
    };


    NodeRenderWatcher(const NodePtr& node)
        : GenericWatcher()
    {
        _nodes.push_back(node);
    }

    NodeRenderWatcher(const NodesList& nodes)
        : GenericWatcher()
        , _nodes(nodes)
    {

    }

    virtual ~NodeRenderWatcher()
    {
    }

private:

    virtual void handleBlockingTask(int taskID) OVERRIDE FINAL;
    NodesList _nodes;
};


NATRON_NAMESPACE_EXIT

#endif // GENERICSCHEDULERTHREADWATCHER_H
