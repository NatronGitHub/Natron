/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
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

#ifndef OUTPUTSCHEDULERTHREAD_H
#define OUTPUTSCHEDULERTHREAD_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#endif
#include <QThread>

#include "Global/GlobalDefines.h"
#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER;

typedef boost::shared_ptr<RenderStats> RenderStatsPtr;

/**
 * @brief Stub class used by internal implementation of OutputSchedulerThread to pass objects through signal/slots
 **/
class BufferableObject
{
    
    int uniqueID; //< used to differentiate frames which may belong to the same time/view (e.g when wipe is enabled)
public:
    
    int getUniqueID() const
    {
        return uniqueID;
    }
    
    BufferableObject() : uniqueID(0) {}
    
    virtual ~BufferableObject() {}
    
    void setUniqueID(int aid)
    {
        uniqueID = aid;
    }
    
    virtual std::size_t sizeInRAM() const = 0;
};

typedef std::list<boost::shared_ptr<BufferableObject> > BufferableObjectList;


struct BufferedFrame
{
    int view;
    double time;
    RenderStatsPtr stats;
    boost::shared_ptr<BufferableObject> frame;
    
    BufferedFrame()
    : view(0)
    , time(0)
    , stats()
    , frame()
    {
        
    }
};

typedef std::list<BufferedFrame> BufferedFrames;

class OutputSchedulerThread;

struct RenderThreadTaskPrivate;
class RenderThreadTask :  public QThread
{
    
    
public:
    
    RenderThreadTask(const boost::shared_ptr<OutputEffectInstance>& output,OutputSchedulerThread* scheduler);
    
    virtual ~RenderThreadTask();
    
    virtual void run() OVERRIDE FINAL;
    
    /**
     * @brief Call this to quit the thread whenever it will return to the pickFrameToRender function
     **/
    void scheduleForRemoval();
    
    bool mustQuit() const;

    bool hasQuit() const;
    
    void notifyIsRunning(bool running);
    
protected:
    
    /**
     * @brief Must render the frame
     **/
    virtual void renderFrame(int time, const std::vector<int>& viewsToRender, bool enableRenderStats) = 0;
        
    boost::scoped_ptr<RenderThreadTaskPrivate> _imp;
};



/**
 * @brief The scheduler that will control the render threads and order the output if needed
 **/
struct OutputSchedulerThreadPrivate;
class OutputSchedulerThread : public QThread
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:
    
    friend class RenderThreadTask;
    
    enum RenderDirectionEnum {
        eRenderDirectionForward = 0,
        eRenderDirectionBackward
    };
    
    enum ProcessFrameModeEnum
    {
        eProcessFrameBySchedulerThread = 0, //< the processFrame function will be called by the OutputSchedulerThread thread.
        eProcessFrameByMainThread //< the processFrame function will be called by the application's main-thread.
    };
    
    OutputSchedulerThread(RenderEngine* engine,const boost::shared_ptr<OutputEffectInstance>& effect,ProcessFrameModeEnum mode);
    
    virtual ~OutputSchedulerThread();
    
    /**
     * @brief When a render thread has finished rendering a frame, it must
     * append it here for buffering to make sure the output device (Viewer, Writer, etc...) will proceed the frames
     * in respect to the time parameter.
     * This wakes up the scheduler thread waiting on the bufCondition. If you need to append several frames 
     * use the other version of this function.
     **/
    void appendToBuffer(double time,
                        int view,
                        const RenderStatsPtr& stats,
                        const boost::shared_ptr<BufferableObject>& frame);
    void appendToBuffer(double time,
                        int view,
                        const RenderStatsPtr& stats,
                        const BufferableObjectList& frames);
    
private:
    
    void appendToBuffer_internal(double time,
                                 int view,
                                 const RenderStatsPtr& stats,
                                 const boost::shared_ptr<BufferableObject>& frame,
                                 bool wakeThread);
    
public:
    
    
    /**
     * @brief Once returned from that function, the object's thread will be finished and the object unusable.
     **/
    void quitThread();
    
    /**
     * @brief True if quitThread() was called
     **/
    bool mustQuitThread() const;
    
    /**
     * @brief Call this to render from firstFrame to lastFrame included.
     **/
    void renderFrameRange(bool isBlocking,
                          bool enableRenderStats,
                          int firstFrame,
                          int lastFrame,
                          int frameStep,
                          const std::vector<int>& viewsToRender,
                          RenderDirectionEnum forward);

    /**
     * @brief Same as renderFrameRange except that the frame range will be computed automatically and it will
     * start from the current frame.
     * This is not appropriate to call this function from a writer.
     **/
    void renderFromCurrentFrame(bool enableRenderStats,
                                const std::vector<int>& viewsToRender,
                                RenderDirectionEnum forward);
    
    /**
     * @brief Whether the playback can be automatically restarted by a single render request
     **/
    bool isPlaybackAutoRestartEnabled() const;

    
    /**
     * @brief Called when a frame has been rendered completetly
     * @param policy If eSchedulingPolicyFFA the render thread is not using the appendToBuffer API
     * but is directly rendering (e.g: a Writer rendering image sequences doesn't need to be ordered)
     * then the scheduler takes this as a hint to know how many frames have been rendered.
     **/
    void notifyFrameRendered(int frame,
                             int viewIndex,
                             const std::vector<int>& viewsToRender,
                             const RenderStatsPtr& stats,
                             SchedulingPolicyEnum policy);

    /**
     * @brief To be called by concurrent worker threads in case of failure, all renders will be aborted
     **/
    void notifyRenderFailure(const std::string& errorMessage);
    
    
    /**
     * @brief Returns true if the scheduler is active and some render threads are doing work.
     **/
    bool isWorking() const;

    
    void doAbortRenderingOnMainThread (bool blocking)
    {
        Q_EMIT s_abortRenderingOnMainThread(false,blocking);
    }
    
    
    /**
     * @brief Returns the render direction as set in the livingRunArgs, @see startRender()
     * This can only be called on the scheduler thread (this)
     **/
    RenderDirectionEnum getDirectionRequestedToRender() const;
    
    /**
     * @brief Returns the views as set in the livingRunArgs, @see startRender()
     * This can only be called on the scheduler thread (this)
     **/
    std::vector<int> getViewsRequestedToRender() const;
    
    /**
     * @brief Returns the current number of render threads
     **/
    int getNRenderThreads() const;
    
    /**
     * @brief Returns the current number of render threads doing work
     **/
    int getNActiveRenderThreads() const;
    
    /**
     * @brief Called by render-threads to pick some work to do or to get asleep if theres nothing to do
     **/
    int pickFrameToRender(RenderThreadTask* thread, bool* enableRenderStats, std::vector<int>* viewsToRender);
    

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
    
    /**
     * @brief Returns true if a render is being aborted
     **/
    bool isBeingAborted() const;

public Q_SLOTS:
    
    void doProcessFrameMainThread(const BufferedFrames& frames,bool mustSeekTimeline,int time);
    
    /**
     @brief Aborts all computations. This turns on the flag abortRequested and will inform the engine that it needs to stop.
     * This function is blocking and once returned you can assume the rendering is completly aborted.
     *
     * Note: In the case we want to abort rendering because the user changed the layout of the graph, we should
     * call abortRendering() before changing the connections so that we're sure that all the tree is properly aborted.
     * This is due to the fact that the inputs are thread-local storage and if calling abortRendering in the main-thread
     * we will then setAborted(true) only the inputs that are seen by the main-thread, but they could be different for
     * another render thread.
     *
     * WARNING: This function can NOT be called from a render thread launched from the thread-pool as this function
     * explicitly waits for all threads in the thread-pool to be done.
     * If you want to abortRendering() from one of those threads, call doAbortRenderingOnMainThreadInstead
     **/
    void abortRendering(bool autoRestart, bool blocking);
    
    
Q_SIGNALS:
    
    void s_doProcessOnMainThread(const BufferedFrames& frames,bool mustSeekTimeline,int time);
    
    void s_abortRenderingOnMainThread(bool userRequested,bool blocking);
    
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
    virtual void getFrameRangeToRender(int& first,int& last)  const = 0;
    
    /**
     * @brief Returns the frame range requested as set in the livingRunArgs, @see startRender()
     * This can only be called on the scheduler thread (this)
     **/
    void getFrameRangeRequestedToRender(int &first,int& last) const;
    


    /**
     * @brief Return the frame expected to be rendered
     **/
    virtual int timelineGetTime() const = 0;
    
    /**
     * @brief Must create a runnable task that will render 1 frame in a separate thread.
     * The internal thread pool will take care of the thread
     * The task will pick frames to render until there are no more to be rendered.
     * @param playbackOrRender Used as a hint to know that we're rendering for playback or render on disk
     * and not just for one frame
     **/
    virtual RenderThreadTask* createRunnable() = 0;
    
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
    
    virtual void run() OVERRIDE FINAL;
    
    /**
     * @brief Called by the scheduler threads to wake-up render threads and make them do some work
     * It calls pushFramesToRenderInternal. It starts pushing frames from lastFramePushedIndex
     **/
    void pushFramesToRender(int nThreads);
   
    /**
     *@brief Called in startRender() when we need to start pushing frames to render
     **/
    void pushFramesToRender(int startingFrame,int nThreads);
    
    
    void pushFramesToRenderInternal(int startingFrame,int nThreads);
    
    void pushAllFrameRange();
    
    /**
     * @brief Starts/stops more threads according to CPU activity and user preferences 
     * @param optimalNThreads[out] Will be set to the new number of threads
     **/
    void adjustNumberOfThreads(int* newNThreads, int *lastNThreads);
    
    /**
     * @brief Make nThreadsToStop quit running. If 0 then all threads will be destroyed.
     **/
    void stopRenderThreads(int nThreadsToStop);
    
    void startRender();

    void stopRender();
    
    void renderInternal();
    
    boost::scoped_ptr<OutputSchedulerThreadPrivate> _imp;
    
};


class DefaultScheduler : public OutputSchedulerThread
{
public:
    
    DefaultScheduler(RenderEngine* engine,const boost::shared_ptr<OutputEffectInstance>& effect);
    
    virtual ~DefaultScheduler();
    

private:
    
    virtual void processFrame(const BufferedFrames& frames) OVERRIDE FINAL;
    
    virtual void timelineStepOne(RenderDirectionEnum direction) OVERRIDE FINAL;
    
    virtual void timelineGoTo(int time) OVERRIDE FINAL;
    
    virtual void getFrameRangeToRender(int& first,int& last) const OVERRIDE FINAL;
    
    virtual int timelineGetTime() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    
    virtual RenderThreadTask* createRunnable() OVERRIDE FINAL WARN_UNUSED_RETURN;
    
    virtual void handleRenderFailure(const std::string& errorMessage) OVERRIDE FINAL;
    
    virtual SchedulingPolicyEnum getSchedulingPolicy() const OVERRIDE FINAL;
    
    virtual void aboutToStartRender() OVERRIDE FINAL;
    
    virtual void onRenderStopped(bool aborted) OVERRIDE FINAL;
    

    
    boost::weak_ptr<OutputEffectInstance> _effect;
};


class ViewerInstance;
class ViewerDisplayScheduler : public OutputSchedulerThread
{
    
public:
    
    ViewerDisplayScheduler(RenderEngine* engine, const boost::shared_ptr<ViewerInstance>& viewer);
    
    virtual ~ViewerDisplayScheduler();
    
    
private:

    virtual void processFrame(const BufferedFrames& frames) OVERRIDE FINAL;
    
    virtual void timelineStepOne(RenderDirectionEnum direction) OVERRIDE FINAL;
    
    virtual void timelineGoTo(int time) OVERRIDE FINAL;
    
    virtual int timelineGetTime() const OVERRIDE FINAL WARN_UNUSED_RETURN;
        
    virtual bool isFPSRegulationNeeded() const OVERRIDE FINAL WARN_UNUSED_RETURN { return true; }
    
    virtual void getFrameRangeToRender(int& first,int& last) const OVERRIDE FINAL;
    
    virtual RenderThreadTask* createRunnable() OVERRIDE FINAL WARN_UNUSED_RETURN;
    
    virtual void handleRenderFailure(const std::string& errorMessage) OVERRIDE FINAL;
    
    virtual SchedulingPolicyEnum getSchedulingPolicy() const OVERRIDE FINAL { return eSchedulingPolicyOrdered; }
    
    virtual int getLastRenderedTime() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    
    virtual void onRenderStopped(bool aborted) OVERRIDE FINAL;
    
    boost::weak_ptr<ViewerInstance> _viewer;
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
class ViewerCurrentFrameRequestScheduler : public QThread
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:
    
    ViewerCurrentFrameRequestScheduler(ViewerInstance* viewer);
    
    virtual ~ViewerCurrentFrameRequestScheduler();
    
    void renderCurrentFrame(bool enableRenderStats,bool canAbort);
    
    void quitThread();
    
    void abortRendering(bool blocking);
    
    bool hasThreadsWorking() const;
    
    void notifyFrameProduced(const BufferableObjectList& frames,const RenderStatsPtr& stats, const boost::shared_ptr<RequestedFrame>& request);
    
public Q_SLOTS:
    
    void doProcessProducedFrameOnMainThread(const RenderStatsPtr& stats,const BufferableObjectList& frames);
    
Q_SIGNALS:
    
    void s_processProducedFrameOnMainThread(const RenderStatsPtr& stats,const BufferableObjectList& frames);
    
private:
    
    virtual void run() OVERRIDE FINAL;
    
    boost::scoped_ptr<ViewerCurrentFrameRequestSchedulerPrivate> _imp;
    
};

struct ViewerArgs;
struct CurrentFrameFunctorArgs;

/**
 * @brief Single thread used by the ViewerCurrentFrameRequestScheduler when the global thread pool has reached its maximum
 * activity to keep the renders responsive even if the thread pool is choking. 
 **/
struct ViewerCurrentFrameRequestRendererBackupPrivate;
class ViewerCurrentFrameRequestRendererBackup : public QThread
{
public:
    
    ViewerCurrentFrameRequestRendererBackup();
    
    virtual ~ViewerCurrentFrameRequestRendererBackup();
    
    void renderCurrentFrame(const boost::shared_ptr<CurrentFrameFunctorArgs>& args);
    
    void quitThread();
    
private:
    
    virtual void run() OVERRIDE FINAL;
    
    boost::scoped_ptr<ViewerCurrentFrameRequestRendererBackupPrivate> _imp;
};


/**
 * @brief This class manages multiple OutputThreadScheduler so that each render request gets processed as soon as possible.
 **/
struct RenderEnginePrivate;
class RenderEngine : public QObject
{
    
    Q_OBJECT
    
    
    friend class OutputSchedulerThread;
    friend class ViewerDisplayScheduler;
    
public:
    
    RenderEngine(const boost::shared_ptr<OutputEffectInstance>& output);
    
    virtual ~RenderEngine();
   
    
    /**
     * @brief Call this to render from firstFrame to lastFrame included.
     **/
    void renderFrameRange(bool isBlocking,
                          bool enableRenderStats,
                          int firstFrame,
                          int lastFrame,
                          int frameStep,
                          const std::vector<int>& viewsToRender,
                          OutputSchedulerThread::RenderDirectionEnum forward);
    
    /**
     * @brief Same as renderFrameRange except that the frame range will be computed automatically and it will
     * start from the current frame.
     * This is not appropriate to call this function from a writer.
     **/
    void renderFromCurrentFrame(bool enableRenderStats,
                                const std::vector<int>& viewsToRender,
                                OutputSchedulerThread::RenderDirectionEnum forward);
    
    
    /**
     * @brief Basically it just renders with the current frame on the timeline.
     * @param abortPrevious If true then it will stop any ongoing render and render the current frame
     * in a separate thread
     **/
    void renderCurrentFrame(bool enableRenderStats,bool canAbort);

    /**
     * @brief Returns the playback mode of the internal scheduler
     **/
    PlaybackModeEnum getPlaybackMode() const;
    
    /**
     * @brief Returns the desired user FPS that the internal scheduler should stick to
     **/
    double getDesiredFPS() const;
    
    /**
     * @brief Quit all processing, making sure all threads are finished.
     **/
    void quitEngine();
    
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

    
    /**
     * @brief Set the playback mode
     * @param mode Corresponds to the PlaybackModeEnum enum
     **/
    void setPlaybackMode(int mode);
    
    
    /**
     *@brief The slot called by the GUI to set the requested fps.
     **/
    void setDesiredFPS(double d);
    
    
    /**
     * @brief Aborts the internal scheduler and returns true if it was working.
     **/
    bool abortRendering(bool enableAutoRestartPlayback, bool blocking);
    void abortRendering_Blocking() { abortRendering(true,true); }

    
    void onCurrentFrameRenderRequestPosted();
    
Q_SIGNALS:
    
    /**
     * @brief Emitted when the fps has changed
     * This will not be emitted after calling renderCurrentFrame
     **/
    void fpsChanged(double actualFps,double desiredFps);
    
    /**
     * @brief Emitted after a frame is rendered.
     * This will not be emitted after calling renderCurrentFrame
     **/
    void frameRendered(int time);
    
    /**
     * @brief Same as frameRendered(int) but with more infos
     **/
    void frameRenderedWithTimer(int frame, double timeElapsed, double timeRemaining);
    
    /**
     * @brief Emitted when the stopRender() function is called
     * @param retCode Will be set to 1 if the render was finished because it was aborted, 0 otherwise.
     * This will not be emitted after calling renderCurrentFrame
     **/
    void renderFinished(int retCode);
    
    
    void renderStarted(bool forward);

    /**
    * @brief Emitted when gui is frozen and rendering is finished to update all knobs
     **/
    void refreshAllKnobs();
    
    void currentFrameRenderRequestPosted();

protected:
    
    
    /**
     * @brief Must create the main-scheduler that will be used for scheduling playback/writing on disk.
     **/
    virtual OutputSchedulerThread* createScheduler(const boost::shared_ptr<OutputEffectInstance>& effect) ;
    
private:
    
    void renderCurrentFrameInternal(bool enableRenderStats,bool canAbort);

    
    /**
     * The following functions are called by the OutputThreadScheduler to Q_EMIT the corresponding signals
     **/
    void s_fpsChanged(double actual,double desired) { Q_EMIT fpsChanged(actual, desired); }
    void s_frameRendered(int time) { Q_EMIT frameRendered(time); }
    void s_frameRenderedWithTimer(int time, double timeElapsed, double timeRemaining) {
        Q_EMIT frameRenderedWithTimer(time, timeElapsed, timeRemaining);
    }
    void s_renderStarted(bool forward) { Q_EMIT renderStarted(forward); }
    void s_renderFinished(int retCode) { Q_EMIT renderFinished(retCode); }
    void s_refreshAllKnobs() { Q_EMIT refreshAllKnobs(); }
    
    friend class ViewerInstance;
    void notifyFrameProduced(const BufferableObjectList& frames, const RenderStatsPtr& stats ,const boost::shared_ptr<RequestedFrame>& request);

    
    boost::scoped_ptr<RenderEnginePrivate> _imp;
};

class ViewerRenderEngine : public RenderEngine
{
    
public:
    
    ViewerRenderEngine(const boost::shared_ptr<OutputEffectInstance>& output)
    : RenderEngine(output)
    {}
    
    virtual ~ViewerRenderEngine() {}
    
private:
    
    virtual OutputSchedulerThread* createScheduler(const boost::shared_ptr<OutputEffectInstance>& effect) OVERRIDE FINAL WARN_UNUSED_RETURN;
};

NATRON_NAMESPACE_EXIT;

#endif // OUTPUTSCHEDULERTHREAD_H
