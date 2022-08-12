/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2022 The Natron developers
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
#include "Engine/ViewIdx.h"
#include "Engine/EngineFwd.h"
#include "Engine/ThreadPool.h"

//#define NATRON_PLAYBACK_USES_THREAD_POOL


NATRON_NAMESPACE_ENTER

typedef RenderStatsPtr RenderStatsPtr;

struct BufferedFrame
{
    ViewIdx view;
    double time;
    RenderStatsPtr stats;
    BufferableObjectPtr frame;

    BufferedFrame()
        : view(0)
        , time(0)
        , stats()
        , frame()
    {
    }
};

typedef std::list<BufferedFrame> BufferedFrames;

class ViewerCurrentFrameRequestSchedulerStartArgs
    : public GenericThreadStartArgs
{
public:


    U64 age;

    ViewerCurrentFrameRequestSchedulerStartArgs()
        : GenericThreadStartArgs()
        , age(0)
    {
    }

    virtual ~ViewerCurrentFrameRequestSchedulerStartArgs()
    {
    }
};

class OutputSchedulerThread;

struct RenderThreadTaskPrivate;

#ifndef NATRON_PLAYBACK_USES_THREAD_POOL
class RenderThreadTask
    :  public QThread
      , public AbortableThread
#else
class RenderThreadTask
    :  public QRunnable
#endif
{
public:

#ifndef NATRON_PLAYBACK_USES_THREAD_POOL
    RenderThreadTask(const OutputEffectInstancePtr& output,
                     OutputSchedulerThread* scheduler);
#else
    RenderThreadTask(const OutputEffectInstancePtr& output,
                     OutputSchedulerThread* scheduler,
                     const int time,
                     const bool useRenderStats,
                     const std::vector<ViewIdx>& viewsToRender);
#endif

    virtual ~RenderThreadTask();

    virtual void run() OVERRIDE FINAL;

#ifndef NATRON_PLAYBACK_USES_THREAD_POOL
    /**
     * @brief Call this to quit the thread whenever it will return to the pickFrameToRender function
     **/
    void scheduleForRemoval();

    bool mustQuit() const;

    bool hasQuit() const;

    void notifyIsRunning(bool running);
#endif

protected:

    /**
     * @brief Must render the frame
     **/
    virtual void renderFrame(int time, const std::vector<ViewIdx>& viewsToRender, bool enableRenderStats) = 0;
    std::unique_ptr<RenderThreadTaskPrivate> _imp;
};

enum RenderDirectionEnum
{
    eRenderDirectionForward = 0,
    eRenderDirectionBackward
};


class OutputSchedulerThreadStartArgs
    : public GenericThreadStartArgs
{
public:

    bool isBlocking;
    bool enableRenderStats;
    int firstFrame;
    int lastFrame;
    int frameStep;
    std::vector<ViewIdx> viewsToRender;
    RenderDirectionEnum processTimelineDirection, pushTimelineDirection;


    OutputSchedulerThreadStartArgs(bool isBlocking,
                                   bool enableRenderStats,
                                   int firstFrame,
                                   int lastFrame,
                                   int frameStep,
                                   const std::vector<ViewIdx>& viewsToRender,
                                   RenderDirectionEnum forward)
        : GenericThreadStartArgs()
        , isBlocking(isBlocking)
        , enableRenderStats(enableRenderStats)
        , firstFrame(firstFrame)
        , lastFrame(lastFrame)
        , frameStep(frameStep)
        , viewsToRender(viewsToRender)
        , processTimelineDirection(forward)
        , pushTimelineDirection(forward)
    {
    }

    virtual ~OutputSchedulerThreadStartArgs()
    {
    }
};

/**
 * @brief The scheduler that will control the render threads and order the output if needed
 **/
struct OutputSchedulerThreadPrivate;
class OutputSchedulerThread
    : public GenericSchedulerThread
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
                          const OutputEffectInstancePtr& effect,
                          ProcessFrameModeEnum mode);

    virtual ~OutputSchedulerThread();

    /**
     * @brief When a render thread has finished rendering a frame, it must
     * append it here for buffering to make sure the output device (Viewer, Writer, etc...) will proceed the frames
     * in respect to the time parameter.
     * This wakes up the scheduler thread waiting on the bufCondition. If you need to append several frames
     * use the other version of this function.
     **/
    void appendToBuffer(double time,
                        ViewIdx view,
                        const RenderStatsPtr& stats,
                        const BufferableObjectPtr& frame);
    void appendToBuffer(double time,
                        ViewIdx view,
                        const RenderStatsPtr& stats,
                        const BufferableObjectPtrList& frames);

private:

    void appendToBuffer_internal(double time,
                                 ViewIdx view,
                                 const RenderStatsPtr& stats,
                                 const BufferableObjectPtr& frame,
                                 bool wakeThread);

public:

    /**
     * @brief Call this to render from firstFrame to lastFrame included.
     **/
    void renderFrameRange(bool isBlocking,
                          bool enableRenderStats,
                          int firstFrame,
                          int lastFrame,
                          int frameStep,
                          const std::vector<ViewIdx>& viewsToRender,
                          RenderDirectionEnum forward);

    /**
     * @brief Same as renderFrameRange except that the frame range will be computed automatically and it will
     * start from the current frame.
     * This is not appropriate to call this function from a writer.
     **/
    void renderFromCurrentFrame(bool enableRenderStats,
                                const std::vector<ViewIdx>& viewsToRender,
                                RenderDirectionEnum forward);


    /**
     * @brief Called when a frame has been rendered completely
     * @param policy If eSchedulingPolicyFFA the render thread is not using the appendToBuffer API
     * but is directly rendering (e.g: a Writer rendering image sequences doesn't need to be ordered)
     * then the scheduler takes this as a hint to know how many frames have been rendered.
     **/
    void notifyFrameRendered(int frame,
                             ViewIdx viewIndex,
                             const std::vector<ViewIdx>& viewsToRender,
                             const RenderStatsPtr& stats,
                             SchedulingPolicyEnum policy);

    /**
     * @brief To be called by concurrent worker threads in case of failure, all renders will be aborted
     **/
    void notifyRenderFailure(const std::string& errorMessage);


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

#ifndef NATRON_PLAYBACK_USES_THREAD_POOL
    /**
     * @brief Called by render-threads to pick some work to do or to get asleep if there's nothing to do
     **/
    int pickFrameToRender(RenderThreadTask* thread, bool* enableRenderStats, std::vector<ViewIdx>* viewsToRender);
#endif

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

private Q_SLOTS:

    void onThreadSpawnsTimerTriggered();


Q_SIGNALS:

    void s_executeCallbackOnMainThread(QString);

protected:


    /**
     * @brief Called whenever there are images available to process in the buffer.
     * Once processed, the frame will be removed from the buffer.
     *
     * According to the ProcessFrameModeEnum given to the scheduler this function will be called either by the scheduler thread (this)
     * or by the application's main-thread (typically to do OpenGL rendering).
     **/
    virtual void processFrame(const BufferedFrames& frames) = 0;

    /**
     * @brief Must be implemented to increment/decrement the timeline by one frame.
     * @param forward If true, must increment otherwise must decrement
     **/
    virtual void timelineStepOne(RenderDirectionEnum direction) = 0;

    /**
     * @brief Set the timeline to the next frame to be rendered, this is used by startSchedulerAtFrame() when starting rendering
     * then timelineStepOne() is used instead.
     **/
    virtual void timelineGoTo(int time) = 0;

    /**
     * @brief Should we try to maintain a constant FPS ?
     **/
    virtual bool isFPSRegulationNeeded() const { return false; }

    /**
     * @brief Must return the frame range to render. For the viewer this is what is indicated on the global timeline,
     * for writers this is its internal timeline.
     **/
    virtual void getFrameRangeToRender(int& first, int& last)  const = 0;

    /**
     * @brief Return the frame expected to be rendered
     **/
    virtual int timelineGetTime() const = 0;

    /**
     * @brief Must create a runnable task that will render 1 frame in a separate thread.
     * The internal thread pool will take care of the thread
     * The task will pick frames to render until there are no more to be rendered.
     **/
#ifndef NATRON_PLAYBACK_USES_THREAD_POOL
    virtual RenderThreadTask* createRunnable() = 0;
#else
    virtual RenderThreadTask* createRunnable(int frame, bool useRenderStarts, const std::vector<ViewIdx>& viewsToRender) = 0;
#endif

    /**
     * @brief Called upon failure of a thread to render an image
     **/
    virtual void handleRenderFailure(const std::string& errorMessage) = 0;

    /**
     * @brief Must return the scheduling policy that the output device will have
     **/
    virtual SchedulingPolicyEnum getSchedulingPolicy() const = 0;

    /**
     * @brief Returns the last successful render time.
     * This makes sense only for Viewers to keep the timeline in sync with what is displayed.
     **/
    virtual int getLastRenderedTime() const { return timelineGetTime(); }

    /**
     * @brief Callback when startRender() is called
     **/
    virtual void aboutToStartRender() {}

    /**
     * @brief Callback when stopRender() is called
     **/
    virtual void onRenderStopped(bool /*aborted*/) {}

    RenderEngine* getEngine() const;

private:

    virtual void onAbortRequested(bool keepOldestRender) OVERRIDE FINAL;
    virtual void executeOnMainThread(const GenericThreadExecOnMainThreadArgsPtr& inArgs) OVERRIDE FINAL;

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


#ifndef NATRON_PLAYBACK_USES_THREAD_POOL
    /**
     * @brief Called by the scheduler threads to wake-up render threads and make them do some work
     * It calls pushFramesToRenderInternal. It starts pushing frames from lastFramePushedIndex
     **/
    void pushFramesToRender(int nThreads);

    /**
     *@brief Called in startRender() when we need to start pushing frames to render
     **/
    void pushFramesToRender(int startingFrame, int nThreads);


    void pushFramesToRenderInternal(int startingFrame, int nThreads);

    void pushAllFrameRange();

    /**
     * @brief Starts/stops more threads according to CPU activity and user preferences
     * @param optimalNThreads[out] Will be set to the new number of threads
     **/
    void adjustNumberOfThreads(int* newNThreads, int *lastNThreads);
#else
    void startTasksFromLastStartedFrame();
    void startTasks(int startingFrame);
#endif

    /**
     * @brief Make nThreadsToStop quit running. If 0 then all threads will be destroyed.
     **/
    void stopRenderThreads(int nThreadsToStop);


    std::unique_ptr<OutputSchedulerThreadPrivate> _imp;
};


class DefaultScheduler
    : public OutputSchedulerThread
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    DefaultScheduler(RenderEngine* engine,
                     const OutputEffectInstancePtr& effect);

    virtual ~DefaultScheduler();

private:

    virtual void processFrame(const BufferedFrames& frames) OVERRIDE FINAL;
    virtual void timelineStepOne(RenderDirectionEnum direction) OVERRIDE FINAL;
    virtual void timelineGoTo(int time) OVERRIDE FINAL;
    virtual void getFrameRangeToRender(int& first, int& last) const OVERRIDE FINAL;
    virtual int timelineGetTime() const OVERRIDE FINAL WARN_UNUSED_RETURN;

#ifndef NATRON_PLAYBACK_USES_THREAD_POOL
    virtual RenderThreadTask* createRunnable() OVERRIDE FINAL WARN_UNUSED_RETURN;
#else
    virtual RenderThreadTask* createRunnable(int frame, bool useRenderStarts, const std::vector<ViewIdx>& viewsToRender) OVERRIDE FINAL WARN_UNUSED_RETURN;
#endif

    virtual void handleRenderFailure(const std::string& errorMessage) OVERRIDE FINAL;
    virtual SchedulingPolicyEnum getSchedulingPolicy() const OVERRIDE FINAL;
    virtual void aboutToStartRender() OVERRIDE FINAL;
    virtual void onRenderStopped(bool aborted) OVERRIDE FINAL;
    OutputEffectInstanceWPtr _effect;
    mutable QMutex _currentTimeMutex;
    int _currentTime;
};


class ViewerInstance;
class ViewerDisplayScheduler
    : public OutputSchedulerThread
{
public:

    ViewerDisplayScheduler(RenderEngine* engine,
                           const ViewerInstancePtr& viewer);

    virtual ~ViewerDisplayScheduler();

private:

    virtual void processFrame(const BufferedFrames& frames) OVERRIDE FINAL;
    virtual void timelineStepOne(RenderDirectionEnum direction) OVERRIDE FINAL;
    virtual void timelineGoTo(int time) OVERRIDE FINAL;
    virtual int timelineGetTime() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool isFPSRegulationNeeded() const OVERRIDE FINAL WARN_UNUSED_RETURN { return true; }

    virtual void getFrameRangeToRender(int& first, int& last) const OVERRIDE FINAL;

#ifndef NATRON_PLAYBACK_USES_THREAD_POOL
    virtual RenderThreadTask* createRunnable() OVERRIDE FINAL WARN_UNUSED_RETURN;
#else
    virtual RenderThreadTask* createRunnable(int frame, bool useRenderStarts, const std::vector<ViewIdx>& viewsToRender) OVERRIDE FINAL WARN_UNUSED_RETURN;
#endif

    virtual void handleRenderFailure(const std::string& errorMessage) OVERRIDE FINAL;
    virtual SchedulingPolicyEnum getSchedulingPolicy() const OVERRIDE FINAL { return eSchedulingPolicyOrdered; }

    virtual int getLastRenderedTime() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void onRenderStopped(bool aborted) OVERRIDE FINAL;
    ViewerInstanceWPtr _viewer;
};

/**
 * @brief The OutputSchedulerThread class (and its derivatives) are meant to be used for playback/render on disk and regulates the output ordering.
 * This class achieves kinda the same goal: it provides the ability to give it a work queue and process the work queue in the same order.
 * Typically when zooming, you want to launch as many thread as possible for each zoom increment and update the viewer in the same order that the one
 * in which you launched the thread in the first place.
 * Instead of re-using the OutputSchedulerClass and adding extra handling for special cases we separated it in a different class, specialized for this kind
 * of "current frame re-rendering" which needs much less code to run than all the code in OutputSchedulerThread
 **/

struct ViewerCurrentFrameRequestSchedulerPrivate;
class ViewerCurrentFrameRequestScheduler
    : public GenericSchedulerThread
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    ViewerCurrentFrameRequestScheduler(ViewerInstance* viewer);

    virtual ~ViewerCurrentFrameRequestScheduler();

    void renderCurrentFrame(bool enableRenderStats, bool canAbort);

    void notifyFrameProduced(const BufferableObjectPtrList& frames, const RenderStatsPtr& stats, const ViewerCurrentFrameRequestSchedulerStartArgsPtr& request);

private:

    virtual void onWaitForAbortCompleted() OVERRIDE FINAL;
    virtual void onWaitForThreadToQuit() OVERRIDE FINAL;
    virtual void onAbortRequested(bool keepOldestRender) OVERRIDE FINAL;
    virtual void onQuitRequested(bool allowRestarts) OVERRIDE FINAL;
    virtual void executeOnMainThread(const GenericThreadExecOnMainThreadArgsPtr& inArgs) OVERRIDE FINAL;

    /**
     * @brief How to pick the task to process from the consumer thread
     **/
    virtual TaskQueueBehaviorEnum tasksQueueBehaviour() const OVERRIDE FINAL;

    /**
     * @brief Must be implemented to execute the work of the thread for 1 loop. This function will be called in a infinite loop by the thread
     **/
    virtual ThreadStateEnum threadLoopOnce(const GenericThreadStartArgsPtr& inArgs) OVERRIDE FINAL WARN_UNUSED_RETURN;
    std::unique_ptr<ViewerCurrentFrameRequestSchedulerPrivate> _imp;
};

class ViewerArgs;
class CurrentFrameFunctorArgs;

/**
 * @brief Single thread used by the ViewerCurrentFrameRequestScheduler when the global thread pool has reached its maximum
 * activity to keep the renders responsive even if the thread pool is choking.
 **/
struct ViewerCurrentFrameRequestRendererBackupPrivate;
class ViewerCurrentFrameRequestRendererBackup
    : public GenericSchedulerThread
{
public:

    ViewerCurrentFrameRequestRendererBackup();

    virtual ~ViewerCurrentFrameRequestRendererBackup();

private:

    /**
     * @brief How to pick the task to process from the consumer thread
     **/
    virtual TaskQueueBehaviorEnum tasksQueueBehaviour() const OVERRIDE FINAL
    {
        return eTaskQueueBehaviorProcessInOrder;
    }

    /**
     * @brief Must be implemented to execute the work of the thread for 1 loop. This function will be called in a infinite loop by the thread
     **/
    virtual ThreadStateEnum threadLoopOnce(const GenericThreadStartArgsPtr& inArgs) OVERRIDE FINAL WARN_UNUSED_RETURN;
};


/**
 * @brief This class manages multiple OutputThreadScheduler so that each render request gets processed as soon as possible.
 **/
struct RenderEnginePrivate;
class RenderEngine
    : public QObject
{
    Q_OBJECT


    friend class OutputSchedulerThread;
    friend class ViewerDisplayScheduler;

public:

    RenderEngine(const OutputEffectInstancePtr& output);

    virtual ~RenderEngine();

    OutputEffectInstancePtr getOutput() const;

    /**
     * @brief Call this to render from firstFrame to lastFrame included.
     **/
    void renderFrameRange(bool isBlocking,
                          bool enableRenderStats,
                          int firstFrame,
                          int lastFrame,
                          int frameStep,
                          const std::vector<ViewIdx>& viewsToRender,
                          RenderDirectionEnum forward);

    /**
     * @brief Same as renderFrameRange except that the frame range will be computed automatically and it will
     * start from the current frame.
     * This is not appropriate to call this function from a writer.
     **/
    void renderFromCurrentFrame(bool enableRenderStats,
                                const std::vector<ViewIdx>& viewsToRender,
                                RenderDirectionEnum forward);


    /**
     * @brief Basically it just renders with the current frame on the timeline.
     * @param abortPrevious If true then it will stop any ongoing render and render the current frame
     * in a separate thread
     **/
    void renderCurrentFrame(bool enableRenderStats, bool canAbort);

    /**
     * @brief Whether the playback can be automatically restarted by a single render request
     **/
    bool isPlaybackAutoRestartEnabled() const;

    /**
     * @brief Set auto-playback restart enabled
     **/
    void setPlaybackAutoRestartEnabled(bool enabled);

    /**
     * @brief Returns the playback mode of the internal scheduler
     **/
    PlaybackModeEnum getPlaybackMode() const;

    /**
     * @brief Returns the desired user FPS that the internal scheduler should stick to
     **/
    double getDesiredFPS() const;

    /**
     * @brief Quit all processing, making sure all threads are finished, this is not blocking
     **/
    void quitEngine(bool allowRestarts);

    /**
     * @brief Blocks the calling thread until all threads are finished, do not call from the main-thread!
     **/
    void waitForEngineToQuit_not_main_thread();

    /**
     * @brief Same as waitForEngineToQuit_not_main_thread except that this function is not blocking and should be only called
     * from the main-thread. You will get notified with the signal engineQuit when the engine really quits.
     **/
    void waitForEngineToQuit_main_thread(bool allowRestart);

    /**
     * @brief Use this as last resort, this will block the caller thread until all threads have finished
     **/
    void waitForEngineToQuit_enforce_blocking();


    /**
     * @brief Aborts all computations. This is not blocking.
     * @returns False if the no thread was currently running, true if the abort request was taken into account.
     * The AutoRestart version attempts to restart the playback upon a call to renderCurrentFrame
     **/
    bool abortRenderingAutoRestart();
    bool abortRenderingNoRestart(bool keepOldestRender = true);

private:

    bool abortRenderingInternal(bool keepOldestRender);

public:

    /**
     * @brief Blocks the calling thread until all renders are aborted, do not call from the main-thread!
     **/
    void waitForAbortToComplete_not_main_thread();

    /**
     * @brief Same as waitForAbortToComplete_not_main_thread except that this function is not blocking and should be only called
     * from the main-thread. You will get notified with the signal engineAborted when the engine really quits.
     **/
    void waitForAbortToComplete_main_thread();

    /**
     * @brief Use this as last resort, this will block the caller thread until abort is completed
     **/
    void waitForAbortToComplete_enforce_blocking();

    /**
     * @brief Returns true if threads owned by the engine are still alive
     **/
    bool hasThreadsAlive() const;

    /**
     * @brief Returns true if the scheduler is active and some render threads are doing work.
     **/
    bool hasThreadsWorking() const;

    /**
     * @brief Returns true if a sequential render is being aborted
     **/
    bool isSequentialRenderBeingAborted() const;

    /**
     * @brief Returns true if playback is active
     **/
    bool isDoingSequentialRender() const;

public Q_SLOTS:

    void abortRendering_non_blocking()
    {
        abortRenderingNoRestart();
    }

    /**
     * @brief Set the playback mode
     * @param mode Corresponds to the PlaybackModeEnum enum
     **/
    void setPlaybackMode(int mode);


    /**
     *@brief The slot called by the GUI to set the requested fps.
     **/
    void setDesiredFPS(double d);

    void onCurrentFrameRenderRequestPosted();

    void onWatcherEngineAbortedEmitted();

    void onWatcherEngineQuitEmitted();

Q_SIGNALS:

    /**
     * @brief Emitted when the fps has changed
     * This will not be emitted after calling renderCurrentFrame
     **/
    void fpsChanged(double actualFps, double desiredFps);

    /**
     * @brief Emitted after a frame is rendered.
     * This will not be emitted after calling renderCurrentFrame
     **/
    void frameRendered(int time, double progress);


    /**
     * @brief Emitted when the stopRender() function is called
     * @param retCode Will be set to 1 if the render was finished because it was aborted, 0 otherwise.
     * This will not be emitted after calling renderCurrentFrame
     **/
    void renderFinished(int retCode);


    void renderStarted(bool forward);

    // Emitted after calling waitForAbortToComplete_main_thread
    void engineAborted();

    // Emitted after calling waitForEngineToQuit_main_thread
    void engineQuit();

    /**
     * @brief Emitted when gui is frozen and rendering is finished to update all knobs
     **/
    void refreshAllKnobs();

    void currentFrameRenderRequestPosted();

protected:


    /**
     * @brief Must create the main-scheduler that will be used for scheduling playback/writing on disk.
     **/
    virtual OutputSchedulerThread* createScheduler(const OutputEffectInstancePtr& effect);

private:

    void renderCurrentFrameInternal(bool enableRenderStats, bool canAbort);


    /**
     * The following functions are called by the OutputThreadScheduler to Q_EMIT the corresponding signals
     **/
    void s_fpsChanged(double actual,
                      double desired) { Q_EMIT fpsChanged(actual, desired); }

    void s_frameRendered(int time,
                         double progress) { Q_EMIT frameRendered(time, progress); }

    void s_renderStarted(bool forward) { Q_EMIT renderStarted(forward); }

    void s_renderFinished(int retCode) { Q_EMIT renderFinished(retCode); }

    void s_refreshAllKnobs() { Q_EMIT refreshAllKnobs(); }

    friend class ViewerInstance;
    friend class OutputEffectInstance;
    void notifyFrameProduced(const BufferableObjectPtrList& frames, const RenderStatsPtr& stats, const ViewerCurrentFrameRequestSchedulerStartArgsPtr& request);


    std::unique_ptr<RenderEnginePrivate> _imp;
};

class ViewerRenderEngine
    : public RenderEngine
{
public:

    ViewerRenderEngine(const OutputEffectInstancePtr& output)
        : RenderEngine(output)
    {}

    virtual ~ViewerRenderEngine() {}

private:

    virtual OutputSchedulerThread* createScheduler(const OutputEffectInstancePtr& effect) OVERRIDE FINAL WARN_UNUSED_RETURN;
};

NATRON_NAMESPACE_EXIT

#endif // Engine_OutputSchedulerThread_h
