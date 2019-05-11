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


#include "ProcessFrameThread.h"

#include <QtCore/QMetaType>
#include <QtCore/QCoreApplication>
#include <QtCore/QThread>

NATRON_NAMESPACE_ENTER


NATRON_NAMESPACE_ANONYMOUS_ENTER

class MetaTypesRegistration
{
public:
    inline MetaTypesRegistration()
    {
        qRegisterMetaType<ProcessFrameArgsBasePtr>("ProcessFrameArgsBasePtr");
    }
};

NATRON_NAMESPACE_ANONYMOUS_EXIT


ProcessFrameThread::ProcessFrameThread()
: GenericSchedulerThread()
{
}

ProcessFrameThread::~ProcessFrameThread()
{

}

class ProcessFrameThreadExecOnMainThreadArgs : public GenericThreadExecOnMainThreadArgs
{
public:

    ProcessFrameThreadStartArgsPtr args;

    ProcessFrameThreadExecOnMainThreadArgs()
    : GenericThreadExecOnMainThreadArgs()
    {}

    virtual ~ProcessFrameThreadExecOnMainThreadArgs()
    {

    }
};



void
ProcessFrameThread::executeOnMainThread(const ExecOnMTArgsPtr& inArgs)
{
    assert(QThread::currentThread() == qApp->thread());
    ProcessFrameThreadExecOnMainThreadArgs* args = dynamic_cast<ProcessFrameThreadExecOnMainThreadArgs*>(inArgs.get());
    assert(args);
    processTask(args->args);
}

void
ProcessFrameThread::processTask(const ProcessFrameThreadStartArgsPtr& task)
{
    if (!task || !task->processor || !task->args) {
        return;
    }
    task->processor->notifyProcessFrame(*task->args);
}

GenericSchedulerThread::ThreadStateEnum
ProcessFrameThread::threadLoopOnce(const GenericThreadStartArgsPtr& inArgs)
{
    ProcessFrameThreadStartArgsPtr args = boost::dynamic_pointer_cast<ProcessFrameThreadStartArgs>(inArgs);
    if (!args) {
        return eThreadStateActive;
    }

    if (args->executeOnMainThread) {
        boost::shared_ptr<ProcessFrameThreadExecOnMainThreadArgs> execMTArgs(new ProcessFrameThreadExecOnMainThreadArgs);
        execMTArgs->args = args;
        requestExecutionOnMainThread(execMTArgs);
    } else {
        processTask(args);
    }
    
    args->processor->notifyFrameProcessed(*args->args);

    return eThreadStateActive;
}

NATRON_NAMESPACE_EXIT
