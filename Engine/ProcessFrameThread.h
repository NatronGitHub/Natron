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


#ifndef NATRON_ENGINE_PROCESSFRAMETHREAD_H
#define NATRON_ENGINE_PROCESSFRAMETHREAD_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/weak_ptr.hpp>
#endif

#include "Engine/GenericSchedulerThread.h"


NATRON_NAMESPACE_ENTER

/**
 * @brief Base class for arguments passsed to ProcessFrameI
 **/
class ProcessFrameArgsBase
{
public:

    RenderFrameResultsContainerPtr results;

    ProcessFrameArgsBase()
    : results()
    {
    }

    virtual ~ProcessFrameArgsBase()
    {

    }
};

typedef boost::shared_ptr<ProcessFrameArgsBase> ProcessFrameArgsBasePtr;


/**
 * @brief Interface for threads that need to process a frame after being rendered.
 * For instance, the processing may concern uploading the viewer's OpenGL texture.
 **/
class ProcessFrameI
{
public:

    ProcessFrameI()
    {

    }

    virtual ~ProcessFrameI() {

    }

protected:

    /**
     * @brief Called whenever there are images available to process in the buffer.
     * Once processed, the frame will be removed from the buffer.
     * This function may be either executed on the main thread or on the ProcessFrameThread thread
     * depending on the executeOnMainThread flag passed to ProcessFrameThreadStartArgs
     **/
    virtual void processFrame(const ProcessFrameArgsBase& args) = 0;

    /**
     * @brief Callback called on the ProcessFrameThread once the processFrame() function returns
     **/
    virtual void onFrameProcessed(const ProcessFrameArgsBase& args) = 0;

private:

    // Called by ProcessFrameThread
    friend class ProcessFrameThread;
    void notifyProcessFrame(const ProcessFrameArgsBase& args) 
    {
        processFrame(args);
    }


    void notifyFrameProcessed(const ProcessFrameArgsBase& args)
    {
        onFrameProcessed(args);
    }
};

/**
 * @brief Arguments passed to the ProcessFrameThread thread
 **/
class ProcessFrameThreadStartArgs
: public GenericThreadStartArgs
{
public:

    ProcessFrameI* processor;
    ProcessFrameArgsBasePtr args;
    bool executeOnMainThread;

    ProcessFrameThreadStartArgs()
    : GenericThreadStartArgs()
    , processor(0)
    , args()
    , executeOnMainThread(false)
    {
    }

    virtual ~ProcessFrameThreadStartArgs()
    {
    }
};

typedef boost::shared_ptr<ProcessFrameThreadStartArgs> ProcessFrameThreadStartArgsPtr;


/**
 * @brief Thread that processes a frame that was rendered: its main job is to update the viewer texture. Since it needs to use the OpenGL
 * context of the main-thread, it may execute the function on the main thread or instead may lock execution of OpenGL code on the main-thread
 * while it updates the viewer in this thread.
 **/
class ProcessFrameThread : public GenericSchedulerThread
{

public:

    ProcessFrameThread();

    virtual ~ProcessFrameThread();

private:

    virtual void executeOnMainThread(const ExecOnMTArgsPtr& inArgs) OVERRIDE FINAL;

    void processTask(const ProcessFrameThreadStartArgsPtr& task);

    virtual TaskQueueBehaviorEnum tasksQueueBehaviour() const OVERRIDE FINAL {
        return eTaskQueueBehaviorSkipToMostRecent;
    }

    virtual ThreadStateEnum threadLoopOnce(const GenericThreadStartArgsPtr& inArgs) OVERRIDE FINAL;

};

NATRON_NAMESPACE_EXIT

#endif // NATRON_ENGINE_PROCESSFRAMETHREAD_H
