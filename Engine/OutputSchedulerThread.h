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
#include <boost/enable_shared_from_this.hpp>
#endif

#include <QtCore/QThread>
#include <QtCore/QMutex>

#include "Global/GlobalDefines.h"

#include "Engine/GenericSchedulerThread.h"
#include "Engine/ProcessFrameThread.h"
#include "Engine/ViewIdx.h"
#include "Engine/ThreadPool.h"
#include "Engine/TimeValue.h"
#include "Engine/TreeRenderQueueProvider.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER


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


/**
 * @brief A result of one execution of a tree in createFrameRenderResults()
 * Each sub-result is listed in a RenderFrameResultsContainer
 **/
class RenderFrameSubResult
{

public:

    RenderFrameSubResult()
    : view()
    {}

    virtual ~RenderFrameSubResult() {}

    /**
     * @brief Waits for all TreeRender(s) composing the sub-results to be finished and return the status code
     **/
    virtual ActionRetCodeEnum waitForResultsReady(const TreeRenderQueueProviderPtr& provider) = 0;

    /**
     * @brief Abort all TreeRender(s) composing the sub-results
     **/
    virtual void abortRender() = 0;

    /**
     * @brief Launch all TreeRender(s) composing the sub-results
     **/
    virtual void launchRenders(const TreeRenderQueueProviderPtr& provider) = 0;

    // Which view is rendered by this sub-result ?
    ViewIdx view;

    // Render statistics for this sub result
    RenderStatsPtr stats;
};

typedef boost::shared_ptr<RenderFrameSubResult> RenderFrameSubResultPtr;


/**
 * @brief A class that encapsulates all the results produced by createFrameRenderResults()
 * It contains for a single frame the list of images that were produced
 **/
class RenderFrameResultsContainer
{
public:

    RenderFrameResultsContainer(const TreeRenderQueueProviderPtr& provider)
    : provider(provider)
    , time()
    , frames()
    {

    }

    virtual ~RenderFrameResultsContainer() {}

    ActionRetCodeEnum waitForRendersFinished() WARN_UNUSED_RETURN
    {
        TreeRenderQueueProviderPtr p = provider.lock();
        assert(p);
        for (std::list<RenderFrameSubResultPtr>::const_iterator it = frames.begin(); it != frames.end(); ++it) {
            ActionRetCodeEnum stat = (*it)->waitForResultsReady(p);
            if (isFailureRetCode(stat)) {
                return stat;
            }
        }
        return eActionStatusOK;
    }

    void launchRenders()
    {
        TreeRenderQueueProviderPtr p = provider.lock();
        assert(p);
        for (std::list<RenderFrameSubResultPtr>::const_iterator it = frames.begin(); it != frames.end(); ++it) {
            (*it)->launchRenders(p);
        }
    }

    void abortRenders()
    {
        for (std::list<RenderFrameSubResultPtr>::const_iterator it = frames.begin(); it != frames.end(); ++it) {
            (*it)->abortRender();
        }
    }

private:

    TreeRenderQueueProviderWPtr provider;

public:


    // The frame at which we launched the render
    TimeValue time;

    // The list of frames that should be processed together by the scheduler
    std::list<RenderFrameSubResultPtr> frames;

    
};

typedef boost::shared_ptr<RenderFrameResultsContainer> RenderFrameResultsContainerPtr;

/**
 * @brief Interface for the thread that starts RenderThreadTask and order the results. Its may task is to schedule jobs and regulate the output.
 * This interface is used to implement Viewer playback and also rendering on disk for writers.
 **/
struct OutputSchedulerThreadPrivate;
class OutputSchedulerThread
    : public GenericSchedulerThread
    , public ProcessFrameI
    , public TreeRenderQueueProvider
    , public boost::enable_shared_from_this<OutputSchedulerThread>
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    enum ProcessFrameModeEnum
    {
        eProcessFrameBySchedulerThread = 0, //< the processFrame function will be called by the ProcessFrameThread thread.
        eProcessFrameByMainThread //< the processFrame function will be called by the application's main-thread.
    };

protected:

    virtual TreeRenderQueueProviderConstPtr getThisTreeRenderQueueProviderShared() const OVERRIDE FINAL
    {
        return shared_from_this();
    }

    OutputSchedulerThread(const RenderEnginePtr& engine,
                          const NodePtr& effect);

public:

    

    virtual ~OutputSchedulerThread();

    /**
     * @brief When enabled, after a frame is rendered, the processFrame function is called. Note that the processFrame
     * function is called in the ordered that renders were launched with createFrameRenderResults. 
     * This is used for example by the Viewer to upload the rendered frames to the OpenGL texture.
     * @mode Controls whether the processFrame call should be operated on the main-thread or on a dedicated thread.
     * By default processFrame is disabled.
     **/
    void setProcessFrameEnabled(bool enabled, OutputSchedulerThread::ProcessFrameModeEnum mode);

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
     * @brief To be called upon a failed render, all concurrent renders will be aborted
     **/
    void notifyRenderFailure(ActionRetCodeEnum status);


    /**
     * @brief Returns the thread render arguments as set in the livingRunArgs
     * This can only be called on the scheduler thread (this)
     **/
    OutputSchedulerThreadStartArgsPtr getCurrentRunArgs() const;

    void getLastRunArgs(RenderDirectionEnum* direction, std::vector<ViewIdx>* viewsToRender) const;


    /**
     *@brief The slot called by the GUI to set the requested fps.
     **/
    void setDesiredFPS(double d);


    /**
     * @brief Returns the desired user FPS that the internal scheduler should stick to
     **/
    double getDesiredFPS() const;

    NodePtr getOutputNode() const;


    RenderEnginePtr getEngine() const;

Q_SIGNALS:

    void s_executeCallbackOnMainThread(QString);

protected:


    // Overriden from ProcessFrameI
    virtual void processFrame(const ProcessFrameArgsBase& args) OVERRIDE = 0;
    virtual void onFrameProcessed(const ProcessFrameArgsBase& args) OVERRIDE = 0;

    void setRenderFinished(bool finished);

    void runAfterFrameRenderedCallback(TimeValue frame);

    virtual ProcessFrameArgsBasePtr createProcessFrameArgs(const OutputSchedulerThreadStartArgsPtr& runArgs, const RenderFrameResultsContainerPtr& results) = 0;
    

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
     * @brief Must create TreeRender object(s) for the given frame (and potentially multiple views)
     **/
    virtual ActionRetCodeEnum createFrameRenderResults(TimeValue time, const std::vector<ViewIdx>& viewsToRender, bool enableRenderStats, RenderFrameResultsContainerPtr* future) = 0;

    /**
     * @brief Called upon failure of a thread to render an image
     **/
    virtual void onRenderFailed(ActionRetCodeEnum status) = 0;



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
    // Overriden from TreeRenderQueueProvider
    virtual void requestMoreRenders() OVERRIDE FINAL;

    // Overriden from GenericSchedulerThread
    virtual void onWaitForAbortCompleted() OVERRIDE FINAL;
    virtual void onWaitForThreadToQuit() OVERRIDE FINAL;
    virtual void onAbortRequested(bool keepOldestRender) OVERRIDE FINAL;
    virtual void onQuitRequested(bool allowRestarts) OVERRIDE FINAL;


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

    void beginSequenceRender();

    void endSequenceRender();


    void startFrameRenderFromLastStartedFrame();
    
    void startFrameRender(TimeValue startingFrame);

    friend struct OutputSchedulerThreadPrivate;
    boost::scoped_ptr<OutputSchedulerThreadPrivate> _imp;
};



NATRON_NAMESPACE_EXIT

#endif // Engine_OutputSchedulerThread_h
