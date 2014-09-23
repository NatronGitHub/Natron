//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#ifndef OUTPUTSCHEDULERTHREAD_H
#define OUTPUTSCHEDULERTHREAD_H

#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>

#include <QThread>

#include "Global/GlobalDefines.h"


///Natron
class ViewerInstance;
namespace Natron {
    class Node;
    class EffectInstance;
    class OutputEffectInstance;
}



/**
 * @brief Stub class used by internal implementation of OutputSchedulerThread to pass objects through signal/slots
 **/
class BufferableObject
{
public:
    
    
    BufferableObject() {}
    
    virtual ~BufferableObject() {}
    
    virtual std::size_t sizeInRAM() const = 0;
};

class OutputSchedulerThread;

struct RenderThreadTaskPrivate;
class RenderThreadTask :  public QThread
{
    
    
public:
    
    RenderThreadTask(bool playbackOrRender,Natron::OutputEffectInstance* output,OutputSchedulerThread* scheduler);
    
    virtual ~RenderThreadTask();
    
    virtual void run() OVERRIDE FINAL;
    
    /**
     * @brief Call this to quit the thread whenever it will return to the pickFrameToRender function
     **/
    void scheduleForRemoval();
    
    bool mustQuit() const;

    bool hasQuit() const;
    
protected:
    
    /**
     * @brief Must render the frame
     **/
    virtual void renderFrame(int time) = 0;
        
    boost::scoped_ptr<RenderThreadTaskPrivate> _imp;
};



/**
 * @brief The scheduler that will control the render threads and order the output if needed
 **/
struct OutputSchedulerThreadPrivate;
class OutputSchedulerThread : public QThread
{
    Q_OBJECT
    
public:
    
    friend class RenderThreadTask;
    
    enum RenderDirection {
        RENDER_FORWARD = 0,
        RENDER_BACKWARD
    };
    
    enum Mode
    {
        TREAT_ON_SCHEDULER_THREAD = 0, //< the treatFrame_blocking function will be called by the OutputSchedulerThread thread.
        TREAT_ON_MAIN_THREAD //< the treatFrame_blocking function will be called by the application's main-thread.
    };
    
    OutputSchedulerThread(Natron::OutputEffectInstance* effect,Mode mode);
    
    virtual ~OutputSchedulerThread();
    
    /**
     * @brief When a render thread has finished rendering a frame, it must
     * append it here for buffering to make sure the output device (Viewer, Writer, etc...) will proceed the frames
     * in respect to the time parameter.
     **/
    void appendToBuffer(double time,int view,const boost::shared_ptr<BufferableObject>& frame);
    
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
    void renderFrameRange(int firstFrame,int lastFrame,RenderDirection forward);

    /**
     * @brief Same as renderFrameRange except that the frame range will be computed automatically and it will
     * start from the current frame.
     * This is not appropriate to call this function from a writer.
     **/
    void renderFromCurrentFrame(RenderDirection forward);
    
    /**
     * @brief Basically it just calls render(...) with the current frame on the timeline.
     **/
    void renderCurrentFrame(bool abortPrevious = true);

    
    /**
     * @brief Called when a frame has been rendered completetly
     * @param countFrameRendered: When the render thread is not using the appendToBuffer API
     * but is directly rendering (e.g: a Writer rendering image sequences doesn't need to be ordered)
     * then countFrameRended needs to be set to true so the scheduler knows how many frames have been rendered.
     **/
    void notifyFrameRendered(int frame,bool countFrameRendered);

    /**
     * @brief To be called by concurrent worker threads in case of failure, all renders will be aborted
     **/
    void notifyRenderFailure(const std::string& errorMessage);
    
    
    /**
     * @brief Returns true if the scheduler is active and some render threads are doing work.
     **/
    bool isWorking() const;
    
    /**
     * @brief Returns true if the scheduler is currently rendering and doing playback.
     **/
    bool isDoingPlayback() const;
    
    /**
     * @brief Return the playback mode
     **/
    Natron::PlaybackMode getPlaybackMode() const;
    
    void doAbortRenderingOnMainThread (bool blocking)
    {
        emit s_abortRenderingOnMainThread(blocking);
    }
    
    
    /**
     * @brief Returns the render direction as set in the livingRunArgs, @see startRender()
     * This can only be called on the scheduler thread (this)
     **/
    RenderDirection getDirectionRequestedToRender() const;
    
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
    int pickFrameToRender(RenderThreadTask* thread);

    /**
     * @brief Called by the render-threads when mustQuit() is true on the thread
     **/
    void notifyThreadAboutToQuit(RenderThreadTask* thread);
public slots:
    
    void doTreatFrameMainThread(double time,int view,const boost::shared_ptr<BufferableObject>& frame);
    
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
    void abortRendering(bool blocking);
    
    void abortRendering_Blocking() { abortRendering(true); }
    
    /**
     * @brief Set the playback mode
     * @param mode Corresponds to the Natron::PlaybackMode enum
     **/
    void setPlaybackMode(int mode);
    
    
    /**
     *@brief The slot called by the GUI to set the requested fps.
     **/
    void setDesiredFPS(double d);
    
signals:
    
    void s_doTreatOnMainThread(double time,int view,const boost::shared_ptr<BufferableObject>& frame);
    
    void s_abortRenderingOnMainThread(bool blocking);
    
    void fpsChanged(double actualFps,double desiredFps);
    
    void frameRendered(int time);
    
    /**
     * @brief Emitted when the stopRender() function is called
     * @param retCode Will be set to 1 if the render was finished because it was aborted, 0 otherwise.
     **/
    void renderFinished(int retCode);
    
protected:
    
    
    /**
     * @brief Called whenever there are images available to treat in the buffer.
     * Once treated, the frame will be removed from the buffer.
     *
     * According to the Mode given to the scheduler this function will be called either by the scheduler thread (this)
     * or by the application's main-thread (typically to do OpenGL rendering).
     **/
    virtual void treatFrame(double time,int view,const boost::shared_ptr<BufferableObject>& frame) = 0;
    
    /**
     * @brief Must be implemented to increment/decrement the timeline by one frame.
     * @param forward If true, must increment otherwise must decrement
     **/
    virtual void timelineStepOne(RenderDirection direction) = 0;
    
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
     * @brief Returns the frame range of the output node, as given by the getFrameRange action
     **/
    void getPluginFrameRange(int& first,int &last) const;

    /**
     * @brief Return the frame expected to be rendered
     **/
    virtual int timelineGetTime() const = 0;
    
    /**
     * @brief Must return whether the user has unlocked the timeline range.
     * If true then the scheduler should not attempt to calculate it automatically
     **/
    virtual bool isTimelineRangeSetByUser() const { return false; }
    
    /**
     * @brief Typically if the user has changed the timeline bounds on the GUI, we want to update the frame range on which the scheduler
     * is rendering. For writers, it never changes.
     **/
    virtual bool isTimelineRangeSettable() const { return false; }
    
    /**
     * @brief Must set the timeline range
     **/
    virtual void timelineSetBounds(int left,int right) = 0;
    
    /**
     * @brief Must create a runnable task that will render 1 frame in a separate thread.
     * The internal thread pool will take care of the thread
     * The task will pick frames to render until there are no more to be rendered.
     * @param playbackOrRender Used as a hint to know that we're rendering for playback or render on disk
     * and not just for one frame
     **/
    virtual RenderThreadTask* createRunnable(bool playbackOrRender) = 0;
    
    /**
     * @brief Called upon failure of a thread to render an image
     **/
    virtual void handleRenderFailure(const std::string& errorMessage) = 0;
    
    /**
     * @brief Callback when startRender() is called
     **/
    virtual void aboutToStartRender() {}
    
    /**
     * @brief Callback when stopRender() is called
     **/
    virtual void onRenderStopped() {}
    
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
    
    /**
     * @brief Make nThreadsToStop quit running. If 0 then all threads will be destroyed.
     **/
    void stopRenderThreads(int nThreadsToStop);
    
    void startRender();

    void stopRender();
    
    void renderInternal();
    
    boost::scoped_ptr<OutputSchedulerThreadPrivate> _imp;
    
};

namespace Natron {
class OutputEffectInstance;
}
class DefaultScheduler : public OutputSchedulerThread
{
public:
    
    DefaultScheduler(Natron::OutputEffectInstance* effect);
    
    virtual ~DefaultScheduler();
    
private:
    
    virtual void treatFrame(double time,int view,const boost::shared_ptr<BufferableObject>& frame) OVERRIDE FINAL;
    
    virtual void timelineStepOne(RenderDirection direction) OVERRIDE FINAL;
    
    virtual void timelineGoTo(int time) OVERRIDE FINAL;
    
    virtual void getFrameRangeToRender(int& first,int& last) const OVERRIDE FINAL;
    
    virtual int timelineGetTime() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    
    virtual void timelineSetBounds(int left,int right) OVERRIDE FINAL;
    
    virtual RenderThreadTask* createRunnable(bool playbackOrRender) OVERRIDE FINAL WARN_UNUSED_RETURN;
    
    virtual void handleRenderFailure(const std::string& errorMessage) OVERRIDE FINAL;
    
    virtual void aboutToStartRender() OVERRIDE FINAL;
    
    virtual void onRenderStopped() OVERRIDE FINAL;
    
    Natron::OutputEffectInstance* _effect;
};


class ViewerInstance;
class ViewerDisplayScheduler : public OutputSchedulerThread
{
public:
    
    ViewerDisplayScheduler(ViewerInstance* viewer);
    
    virtual ~ViewerDisplayScheduler();
    
private:

    virtual void treatFrame(double time,int view,const boost::shared_ptr<BufferableObject>& frame) OVERRIDE FINAL;
    
    virtual void timelineStepOne(RenderDirection direction) OVERRIDE FINAL;
    
    virtual void timelineGoTo(int time) OVERRIDE FINAL;
    
    virtual int timelineGetTime() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    
    virtual bool isTimelineRangeSetByUser() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    
    virtual void timelineSetBounds(int left,int right) OVERRIDE FINAL;
    
    virtual bool isFPSRegulationNeeded() const OVERRIDE FINAL WARN_UNUSED_RETURN { return true; }
    
    virtual bool isTimelineRangeSettable() const OVERRIDE FINAL WARN_UNUSED_RETURN { return true; }
    
    virtual void getFrameRangeToRender(int& first,int& last) const OVERRIDE FINAL;
    
    virtual RenderThreadTask* createRunnable(bool playbackOrRender) OVERRIDE FINAL WARN_UNUSED_RETURN;
    
    virtual void handleRenderFailure(const std::string& errorMessage) OVERRIDE FINAL;
    
    virtual void onRenderStopped() OVERRIDE FINAL;
    
    ViewerInstance* _viewer;
};

#endif // OUTPUTSCHEDULERTHREAD_H
