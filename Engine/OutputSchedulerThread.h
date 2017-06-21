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

#ifndef Engine_OutputSchedulerThread_h
#define Engine_OutputSchedulerThread_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <vector>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/weak_ptr.hpp>
#endif

#include <QtCore/QThread>
#include <QtCore/QMutex>

#include "Global/GlobalDefines.h"

#include "Engine/BufferableObject.h"
#include "Engine/GenericSchedulerThread.h"
#include "Engine/ProcessFrameThread.h"
#include "Engine/ViewIdx.h"
#include "Engine/ThreadPool.h"
#include "Engine/TimeValue.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER;


/**
 * @brief Arguments passed to the OutputSchedulerThread
 **/
class OutputSchedulerThreadStartArgs
    : public GenericThreadStartArgs
{
public:

    bool isBlocking;
    bool enableRenderStats;
    TimeValue firstFrame;
    TimeValue lastFrame;
    TimeValue startingFrame;
    TimeValue frameStep;
    std::vector<ViewIdx> viewsToRender;
    RenderDirectionEnum direction;


    OutputSchedulerThreadStartArgs(bool isBlocking,
                                   bool enableRenderStats,
                                   TimeValue firstFrame,
                                   TimeValue lastFrame,
                                   TimeValue startingFrame,
                                   TimeValue frameStep,
                                   const std::vector<ViewIdx>& viewsToRender,
                                   RenderDirectionEnum forward)
        : GenericThreadStartArgs()
        , isBlocking(isBlocking)
        , enableRenderStats(enableRenderStats)
        , firstFrame(firstFrame)
        , lastFrame(lastFrame)
        , startingFrame(startingFrame)
        , frameStep(frameStep)
        , viewsToRender(viewsToRender)
        , direction(forward)
    {
    }

    virtual ~OutputSchedulerThreadStartArgs()
    {
    }
};

typedef boost::shared_ptr<OutputSchedulerThreadStartArgs> OutputSchedulerThreadStartArgsPtr;

/**
 * @brief Interface for the thread that starts RenderThreadTask and order the results. Its may task is to schedule jobs and regulate the output.
 * This interface is used to implement Viewer playback and also rendering on disk for writers.
 **/
struct OutputSchedulerThreadPrivate;
class OutputSchedulerThread
    : public GenericSchedulerThread
    , public ProcessFrameI
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    friend class RenderThreadTask;
    enum ProcessFrameModeEnum
    {
        eProcessFrameBySchedulerThread = 0, //< the processFrame function will be called by the OutputSchedulerThread thread.
        eProcessFrameByMainThread //< the processFrame function will be called by the application's main-thread.
    };

    OutputSchedulerThread(RenderEngine* engine,
                          const NodePtr& effect,
                          ProcessFrameModeEnum mode);

    virtual ~OutputSchedulerThread();

    /**
     * @brief When a render thread has finished rendering a frame, it must
     * append it here for buffering to make sure the output device (Viewer, Writer, etc...) will proceed the frames
     * in respect to the time parameter.
     * This wakes up the scheduler thread waiting on the bufCondition. If you need to append several frames
     * use the other version of this function.
     **/
    void appendToBuffer(const BufferedFrameContainerPtr& frames);


public:

    /**
     * @brief Call this to render from firstFrame to lastFrame included using an interval of frameStep
     * @param viewToRender These are the views to render, if not set it will be determined given the tree root
     * view awareness.
     **/
    void renderFrameRange(bool isBlocking,
                          bool enableRenderStats,
                          TimeValue firstFrame,
                          TimeValue lastFrame,
                          TimeValue frameStep,
                          const std::vector<ViewIdx>& viewsToRender,
                          RenderDirectionEnum forward);

    /**
     * @brief Same as renderFrameRange except that the frame range will be computed automatically and it will
     * start from the current frame.
     * This is not appropriate to call this function from a writer.
     **/
    void renderFromCurrentFrame(bool isBlocking,
                                bool enableRenderStats,
                                TimeValue frameStep,
                                const std::vector<ViewIdx>& viewsToRender,
                                RenderDirectionEnum forward);

    void clearSequentialRendersQueue();

    /**
     * @brief Called when a frame has been rendered completetly
     * @param policy If eSchedulingPolicyFFA the render thread is not using the appendToBuffer API
     * but is directly rendering (e.g: a Writer rendering image sequences doesn't need to be ordered)
     * then the scheduler takes this as a hint to know how many frames have been rendered.
     **/
    void notifyFrameRendered(const BufferedFrameContainerPtr& stats,
                             SchedulingPolicyEnum policy);

    void runAfterFrameRenderedCallback(TimeValue frame);

    /**
     * @brief To be called by concurrent worker threads in case of failure, all renders will be aborted
     **/
    void notifyRenderFailure(ActionRetCodeEnum stat, const std::string& errorMessage);


    /**
     * @brief Returns the thread render arguments as set in the livingRunArgs
     * This can only be called on the scheduler thread (this)
     **/
    OutputSchedulerThreadStartArgsPtr getCurrentRunArgs() const;

    void getLastRunArgs(RenderDirectionEnum* direction, std::vector<ViewIdx>* viewsToRender) const;

    /**
     * @brief Returns the current number of render threads
     **/
    int getNRenderThreads() const;

    /**
     * @brief Returns the current number of render threads doing work
     **/
    int getNActiveRenderThreads() const;

    /**
     * @brief Called by the render-threads when mustQuit() is true on the thread
     **/
    void notifyThreadAboutToQuit(RenderThreadTask* thread);

    /**
     *@brief The slot called by the GUI to set the requested fps.
     **/
    void setDesiredFPS(double d);


    /**
     * @brief Returns the desired user FPS that the internal scheduler should stick to
     **/
    double getDesiredFPS() const;

    void runCallbackWithVariables(const QString& callback);

    NodePtr getOutputNode() const;

    /**
     * @brief Must return the scheduling policy that the output device will have
     **/
    virtual SchedulingPolicyEnum getSchedulingPolicy() const = 0;

    RenderEngine* getEngine() const;

    virtual void processFrame(const ProcessFrameArgsBase& args) OVERRIDE = 0;


Q_SIGNALS:

    void s_executeCallbackOnMainThread(QString);

protected:


    /**
     * @brief Set the timeline to the next frame to be rendered, this is used by startSchedulerAtFrame()
     **/
    virtual void timelineGoTo(TimeValue time) = 0;

    /**
     * @brief Should we try to maintain a constant FPS ?
     **/
    virtual bool isFPSRegulationNeeded() const { return false; }

    /**
     * @brief Must return the frame range to render. For the viewer this is what is indicated on the global timeline,
     * for writers this is its internal timeline.
     **/
    virtual void getFrameRangeToRender(TimeValue& first, TimeValue& last)  const = 0;

    /**
     * @brief Return the frame expected to be rendered
     **/
    virtual TimeValue timelineGetTime() const = 0;

    /**
     * @brief Must create a runnable task that will render 1 frame in a separate thread.
     * The internal thread pool will take care of the thread
     * The task will pick frames to render until there are no more to be rendered.
     **/
    virtual RenderThreadTask* createRunnable(TimeValue frame, bool useRenderStarts, const std::vector<ViewIdx>& viewsToRender) = 0;

    /**
     * @brief Called upon failure of a thread to render an image
     **/
    virtual void handleRenderFailure(ActionRetCodeEnum stat, const std::string& errorMessage) = 0;



    /**
     * @brief Returns the last successful render time.
     * This makes sense only for Viewers to keep the timeline in sync with what is displayed.
     **/
    virtual TimeValue getLastRenderedTime() const { return timelineGetTime(); }

    /**
     * @brief Callback when startRender() is called
     **/
    virtual void aboutToStartRender() {}

    /**
     * @brief Callback when stopRender() is called
     **/
    virtual void onRenderStopped(bool /*aborted*/) {}



private:

    virtual void onAbortRequested(bool keepOldestRender) OVERRIDE FINAL;

    /**
     * @brief How to pick the task to process from the consumer thread
     **/
    virtual TaskQueueBehaviorEnum tasksQueueBehaviour() const OVERRIDE FINAL
    {
        return eTaskQueueBehaviorSkipToMostRecent;
    }

    /**
     * @brief Must be implemented to execute the work of the thread for 1 loop. This function will be called in a infinite loop by the thread
     **/
    virtual ThreadStateEnum threadLoopOnce(const GenericThreadStartArgsPtr& inArgs) OVERRIDE FINAL WARN_UNUSED_RETURN;

    void startRender();

    void stopRender();


    void startTasksFromLastStartedFrame();
    
    void startTasks(TimeValue startingFrame);

    friend struct OutputSchedulerThreadPrivate;
    boost::scoped_ptr<OutputSchedulerThreadPrivate> _imp;
};



NATRON_NAMESPACE_EXIT;

#endif // Engine_OutputSchedulerThread_h
