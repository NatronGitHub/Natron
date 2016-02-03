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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "OutputSchedulerThread.h"

#include <iostream>
#include <set>
#include <list>
#include <algorithm> // min, max
#include <cassert>
#include <stdexcept>

#include <QtCore/QMetaType>
#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>
#include <QtCore/QCoreApplication>
#include <QtCore/QString>
#include <QtCore/QThreadPool>
#include <QtCore/QDebug>
#include <QtCore/QTextStream>
#include <QtCore/QThreadPool>
#include <QtCore/QRunnable>

#include "Global/MemoryInfo.h"

#include "Engine/AppManager.h"
#include "Engine/AppInstance.h"
#include "Engine/EffectInstance.h"
#include "Engine/Image.h"
#include "Engine/KnobFile.h"
#include "Engine/Node.h"
#include "Engine/OpenGLViewerI.h"
#include "Engine/Project.h"
#include "Engine/RenderStats.h"
#include "Engine/RotoContext.h"
#include "Engine/Settings.h"
#include "Engine/Timer.h"
#include "Engine/TimeLine.h"
#include "Engine/TLSHolder.h"
#include "Engine/ViewerInstance.h"
#include "Engine/ViewerInstancePrivate.h"

#define NATRON_FPS_REFRESH_RATE_SECONDS 1.5

/*
 When defined, parallel frame renders are spawned from a timer so that the frames
 appear to be rendered all at the same speed. 
 When undefined each time a frame is computed a new thread will be spawned
 until we reach the maximum allowed parallel frame renders.
 */
//#define NATRON_SCHEDULER_SPAWN_THREADS_WITH_TIMER

#ifdef NATRON_SCHEDULER_SPAWN_THREADS_WITH_TIMER
#define NATRON_SCHEDULER_THREADS_SPAWN_DEFAULT_TIMEOUT_MS 500
#endif

NATRON_NAMESPACE_ENTER;


///Sort the frames by time and then by view
struct BufferedFrameCompare_less
{
    bool operator()(const BufferedFrame& lhs,const BufferedFrame& rhs) const
    {
        if (lhs.time < rhs.time) {
            return true;
        } else if (lhs.time > rhs.time) {
            return false;
        } else {
            if (lhs.view < rhs.view) {
                return true;
            } else if (lhs.view > rhs.view) {
                return false;
            } else {
                if (lhs.frame && rhs.frame) {
                    if (lhs.frame->getUniqueID() < rhs.frame->getUniqueID()) {
                        return true;
                    } else if (lhs.frame->getUniqueID() > rhs.frame->getUniqueID()) {
                        return false;
                    } else {
                        return false;
                    }
                } else {
                    return false;
                }
            }
        }
    }
};

typedef std::set< BufferedFrame , BufferedFrameCompare_less > FrameBuffer;


namespace {
    class MetaTypesRegistration
    {
    public:
        inline MetaTypesRegistration()
        {
            qRegisterMetaType<BufferedFrames>("BufferedFrames");
            qRegisterMetaType<BufferableObjectList>("BufferableObjectList");
        }
    };
}
static MetaTypesRegistration registration;


struct RunArgs
{
    ///The frame range that the scheduler should render
    int firstFrame,lastFrame,frameStep;
    
    /// the timelineDirection represents the direction the timeline should move to
    OutputSchedulerThread::RenderDirectionEnum timelineDirection;
    
    bool enableRenderStats;
    
    bool isBlocking;
    
    std::vector<int> viewsToRender;

};

struct RenderThread {
    RenderThreadTask* thread;
    bool active;
};
typedef std::list<RenderThread> RenderThreads;

// Struct used in a queue when rendering the current frame with a viewer, the id is meaningless just to have a member
// in the structure. We then compare the pointer of this struct
class RequestedFrame
{
public:
    int id;
};

struct ProducedFrame
{
    BufferableObjectList frames;
    boost::shared_ptr<RequestedFrame> request;
    RenderStatsPtr stats;
    bool processRequest;
    bool isAborted;
};

#ifndef NATRON_PLAYBACK_USES_THREAD_POOL
static bool isBufferFull(int nbBufferedElement, int hardwardIdealThreadCount)
{
    return nbBufferedElement >= hardwardIdealThreadCount * 3;
}
#endif

struct OutputSchedulerThreadPrivate
{
    
    FrameBuffer buf; //the frames rendered by the worker threads that needs to be rendered in order by the output device
    QWaitCondition bufCondition;
    mutable QMutex bufMutex;
    
    bool working; // true when the scheduler is currently having render threads doing work
    mutable QMutex workingMutex;
    
    bool hasQuit; //true when the thread has exited and shouldn't be restarted.
    bool mustQuit; //true when the thread must exit
    QWaitCondition mustQuitCond;
    QMutex mustQuitMutex;
    
    int startRequests;
    QWaitCondition startRequestsCond;
    QMutex startRequestsMutex;
    
    int abortRequested; // true when the user wants to stop the engine, e.g: the user disconnected the viewer, protected by abortedRequestedMutex
    bool isAbortRequestBlocking; // protected by abortedRequestedMutex
    
    //If true then a current frame render can start playback, protected by abortedRequestedMutex
    bool canAutoRestartPlayback;
    
    QWaitCondition abortedRequestedCondition;
    QMutex abortedRequestedMutex; // protects abortRequested
    
    bool abortFlag; // Same as abortRequested > 0 but held by a mutex on a smaller scope.
    mutable QMutex abortFlagMutex; // protects abortFlag

    
    QMutex abortBeingProcessedMutex; //protects abortBeingProcessed
    
    ///Basically when calling stopRender() we are resetting the abortRequested flag and putting the scheduler thread(this)
    ///asleep. We don't want that another thread attemps to post an abort request at the same time.
    bool abortBeingProcessed;
    
    bool processRunning; //true when the scheduler is actively "processing" a frame (i.e: updating the viewer or writing in a file on disk)
    QWaitCondition processCondition;
    QMutex processMutex;

    //doesn't need any protection since it never changes and is set in the constructor
    OutputSchedulerThread::ProcessFrameModeEnum mode; //is the frame to be processed on the main-thread (i.e OpenGL rendering) or on the scheduler thread

    
    boost::scoped_ptr<Timer> timer; // Timer regulating the engine execution. It is controlled by the GUI and MT-safe.
    
    
    ///The idea here is that the render() function will set the requestedRunArgs, and once the scheduler has finished
    ///the previous render it will copy them to the livingRunArgs to fullfil the new render request
    RunArgs requestedRunArgs,livingRunArgs;
    
    ///When the render threads are not using the appendToBuffer API, the scheduler has no way to know the rendering is finished
    ///but to count the number of frames rendered via notifyFrameRended which is called by the render thread.
    U64 nFramesRendered;
    bool renderFinished; //< set to true when nFramesRendered = livingRunArgs.lastFrame - livingRunArgs.firstFrame + 1
    
    QMutex runArgsMutex; // protects requestedRunArgs & livingRunArgs & nFramesRendered
    

    ///Worker threads
    mutable QMutex renderThreadsMutex;
    RenderThreads renderThreads;
    QWaitCondition allRenderThreadsInactiveCond; // wait condition to make sure all render threads are asleep
    
#ifdef NATRON_PLAYBACK_USES_THREAD_POOL
    QThreadPool* threadPool;
    
#else
    
    QWaitCondition allRenderThreadsQuitCond; //to make sure all render threads have quit

 
    std::list<int> framesToRender;
  
    ///Render threads wait in this condition and the scheduler wake them when it needs to render some frames
    QWaitCondition framesToRenderNotEmptyCond;

#endif
    
    ///Work queue filled by the scheduler thread when in playback/render on disk
    QMutex framesToRenderMutex; // protects framesToRender & currentFrameRequests
    
    ///index of the last frame pushed (framesToRender.back())
    ///we store this because when we call pushFramesToRender we need to know what was the last frame that was queued
    ///Protected by framesToRenderMutex
    int lastFramePushedIndex;
 
    boost::weak_ptr<OutputEffectInstance> outputEffect; //< The effect used as output device
    RenderEngine* engine;
    
#ifdef NATRON_SCHEDULER_SPAWN_THREADS_WITH_TIMER
    QTimer threadSpawnsTimer;
    
    QMutex lastRecordedFPSMutex;
    double lastRecordedFPS;
    
#endif

    
    OutputSchedulerThreadPrivate(RenderEngine* engine,const boost::shared_ptr<OutputEffectInstance>& effect,OutputSchedulerThread::ProcessFrameModeEnum mode)
    : buf()
    , bufCondition()
    , bufMutex()
    , working(false)
    , workingMutex()
    , hasQuit(false)
    , mustQuit(false)
    , mustQuitCond()
    , mustQuitMutex()
    , startRequests(0)
    , startRequestsCond()
    , startRequestsMutex()
    , abortRequested(0)
    , isAbortRequestBlocking(false)
    , canAutoRestartPlayback(false)
    , abortedRequestedCondition()
    , abortedRequestedMutex()
    , abortFlag(false)
    , abortFlagMutex()
    , abortBeingProcessedMutex()
    , abortBeingProcessed(false)
    , processRunning(false)
    , processCondition()
    , processMutex()
    , mode(mode)
    , timer(new Timer)
    , requestedRunArgs()
    , livingRunArgs()
    , nFramesRendered(0)
    , renderFinished(false)
    , runArgsMutex()
    , renderThreadsMutex()
    , renderThreads()
    , allRenderThreadsInactiveCond()
#ifdef NATRON_PLAYBACK_USES_THREAD_POOL
    , threadPool(QThreadPool::globalInstance())
#else
    , allRenderThreadsQuitCond()
    , framesToRender()
    , framesToRenderNotEmptyCond()
#endif
    , framesToRenderMutex()
    , lastFramePushedIndex(0)
    , outputEffect(effect)
    , engine(engine)
#ifdef NATRON_SCHEDULER_SPAWN_THREADS_WITH_TIMER
    , threadSpawnsTimer()
    , lastRecordedFPSMutex()
    , lastRecordedFPS(0.)
#endif
    {

    }
    
    bool appendBufferedFrame(double time,
                             int view,
                             const RenderStatsPtr& stats,
                             const boost::shared_ptr<BufferableObject>& image) WARN_UNUSED_RETURN
    {
        ///Private, shouldn't lock
        assert(!bufMutex.tryLock());
        
        BufferedFrame k;
        k.time = time;
        k.view = view;
        k.frame = image;
        k.stats = stats;
        std::pair<FrameBuffer::iterator,bool> ret = buf.insert(k);
        return ret.second;
    }
    
    void getFromBufferAndErase(double time,BufferedFrames& frames)
    {
        
        ///Private, shouldn't lock
        assert(!bufMutex.tryLock());
        
        FrameBuffer newBuf;
        for (FrameBuffer::iterator it = buf.begin(); it != buf.end(); ++it) {
            
            if (it->time == time) {
                if (it->frame) {
                    frames.push_back(*it);
                }
            } else {
                newBuf.insert(*it);
            }
        }
        buf = newBuf;
    }
  
    
    void appendRunnable(RenderThreadTask* runnable)
    {
        assert(!renderThreadsMutex.tryLock());
        RenderThread r;
        r.thread = runnable;
        r.active = true;
        renderThreads.push_back(r);
#ifndef NATRON_PLAYBACK_USES_THREAD_POOL
        runnable->start();
#else
        threadPool->start(runnable);
#endif
        
    }
    
    RenderThreads::iterator getRunnableIterator(RenderThreadTask* runnable)
    {
        ///Private shouldn't lock
        assert(!renderThreadsMutex.tryLock());
        for (RenderThreads::iterator it = renderThreads.begin(); it != renderThreads.end(); ++it) {
            if (it->thread == runnable) {
                return it;
            }
        }
        return renderThreads.end();
    }
    

    int getNBufferedFrames() const {
        QMutexLocker l(&bufMutex);
        return buf.size();
    }
    
    static bool getNextFrameInSequence(PlaybackModeEnum pMode,
                                       OutputSchedulerThread::RenderDirectionEnum direction,
                                       int frame,
                                       int firstFrame,
                                       int lastFrame,
                                       unsigned int frameStep,
                                       int* nextFrame,
                                       OutputSchedulerThread::RenderDirectionEnum* newDirection);
    
    static void getNearestInSequence(OutputSchedulerThread::RenderDirectionEnum direction,
                                     int frame,
                                     int firstFrame,
                                     int lastFrame,
                                     int* nextFrame);
    
    /**
     * @brief Checks if mustQuit has been set to true, if so then it will return true and the scheduler thread should stop
     **/
    bool checkForExit()
    {
        QMutexLocker l(&mustQuitMutex);
        if (mustQuit) {
            mustQuit = false;
            mustQuitCond.wakeOne();
            return true;
        }
        
        return false;
    }
    
    void waitForRenderThreadsToBeDone() {
        
        assert( !renderThreadsMutex.tryLock() );
        while (renderThreads.size() > 0
#ifndef NATRON_PLAYBACK_USES_THREAD_POOL
               /*
                When not using the thread pool we use the same threads for computing several frames.
                When using the thread-pool tasks are actually just removed from renderThreads when they are finisehd
                */
               && getNActiveRenderThreads() > 0
#endif
               ) {
            allRenderThreadsInactiveCond.wait(&renderThreadsMutex);
        }
    }
    
    int getNActiveRenderThreads() const {
        ///Private shouldn't lock
        assert(!renderThreadsMutex.tryLock());
        
#ifndef NATRON_PLAYBACK_USES_THREAD_POOL
        int ret = 0;
        for (RenderThreads::const_iterator it = renderThreads.begin(); it != renderThreads.end(); ++it) {
            if (it->active) {
                ++ret;
            }
        }
        return ret;
#else
        /*
         When not using the thread pool we use the same threads for computing several frames.
         When using the thread-pool tasks are actually just removed from renderThreads when they are finisehd
         */
        return (int)renderThreads.size();
#endif
    }
    
#ifndef NATRON_PLAYBACK_USES_THREAD_POOL
    void removeQuitRenderThreadsInternal()
    {
        for (;;) {
            bool hasRemoved = false;
            for (RenderThreads::iterator it = renderThreads.begin(); it != renderThreads.end(); ++it) {
                if (it->thread->hasQuit()) {
                    it->thread->deleteLater();
                    renderThreads.erase(it);
                    hasRemoved = true;
                    break;
                }
            }
            
            if (!hasRemoved) {
                break;
            }
        }
    }
#endif

#ifndef NATRON_PLAYBACK_USES_THREAD_POOL
    void removeAllQuitRenderThreads() {
        ///Private shouldn't lock
        assert(!renderThreadsMutex.tryLock());
        
        removeQuitRenderThreadsInternal();
        
        ///Wake-up the main-thread if it was waiting for all threads to quit
        allRenderThreadsQuitCond.wakeOne();
    }
#endif
    
    void waitForRenderThreadsToQuit() {
    
#ifndef NATRON_PLAYBACK_USES_THREAD_POOL
        RenderThreads threads;
        {
            QMutexLocker l(&renderThreadsMutex);
            threads = renderThreads;
        }
        
        for (RenderThreads::iterator it = threads.begin(); it != threads.end(); ++it) {
            it->thread->wait();
        }
        {
            QMutexLocker l(&renderThreadsMutex);
            
            removeQuitRenderThreadsInternal();
            assert(renderThreads.empty());
        }
#else
        /*
         We don't need the threads to actually quit, just need the runnables to be done
         */
        QMutexLocker l(&renderThreadsMutex);
        waitForRenderThreadsToBeDone();
#endif
        
    }
    
};


OutputSchedulerThread::OutputSchedulerThread(RenderEngine* engine,const boost::shared_ptr<OutputEffectInstance>& effect,ProcessFrameModeEnum mode)
: QThread()
, _imp(new OutputSchedulerThreadPrivate(engine,effect,mode))
{
    QObject::connect(this, SIGNAL(s_doProcessOnMainThread(BufferedFrames)), this,
                     SLOT(doProcessFrameMainThread(BufferedFrames)));
    
    QObject::connect(_imp->timer.get(), SIGNAL(fpsChanged(double,double)), _imp->engine, SIGNAL(fpsChanged(double,double)));
    
    QObject::connect(this, SIGNAL(s_abortRenderingOnMainThread(bool,bool)), this, SLOT(abortRendering(bool,bool)));
    
    
#ifdef NATRON_SCHEDULER_SPAWN_THREADS_WITH_TIMER
    QObject::connect(&_imp->threadSpawnsTimer, SIGNAL(timeout()), this, SLOT(onThreadSpawnsTimerTriggered()));
#endif
    
    setObjectName("Scheduler thread");
}

OutputSchedulerThread::~OutputSchedulerThread()
{
    ///Wake-up all threads and tell them that they must quit
    stopRenderThreads(0);
    

    ///Make sure they are all gone, there will be a deadlock here if that's not the case.
    _imp->waitForRenderThreadsToQuit();
}


bool
OutputSchedulerThreadPrivate::getNextFrameInSequence(PlaybackModeEnum pMode,OutputSchedulerThread::RenderDirectionEnum direction,int frame,
                                                     int firstFrame,int lastFrame, unsigned int frameStep,
                                                     int* nextFrame,OutputSchedulerThread::RenderDirectionEnum* newDirection)
{
    assert(frameStep >= 1);
    *newDirection = direction;
    if (firstFrame == lastFrame) {
        *nextFrame = firstFrame;
        return true;
    }
    if (frame <= firstFrame) {
        switch (pMode) {
                case ePlaybackModeLoop:
                if (direction == OutputSchedulerThread::eRenderDirectionForward) {
                    *nextFrame = firstFrame + frameStep;
                } else {
                    *nextFrame  = lastFrame - frameStep;
                }
                break;
                case ePlaybackModeBounce:
                if (direction == OutputSchedulerThread::eRenderDirectionForward) {
                    *newDirection = OutputSchedulerThread::eRenderDirectionBackward;
                    *nextFrame  = lastFrame - frameStep;
                } else {
                    *newDirection = OutputSchedulerThread::eRenderDirectionForward;
                    *nextFrame  = firstFrame + frameStep;
                }
                break;
                case ePlaybackModeOnce:
                default:
                if (direction == OutputSchedulerThread::eRenderDirectionForward) {
                    *nextFrame = firstFrame + frameStep;
                    break;
                } else {
                    return false;
                }
                
                
        }
    } else if (frame >= lastFrame) {
        switch (pMode) {
                case ePlaybackModeLoop:
                if (direction == OutputSchedulerThread::eRenderDirectionForward) {
                    *nextFrame = firstFrame;
                } else {
                    *nextFrame = lastFrame - frameStep;
                }
                break;
                case ePlaybackModeBounce:
                if (direction == OutputSchedulerThread::eRenderDirectionForward) {
                    *newDirection = OutputSchedulerThread::eRenderDirectionBackward;
                    *nextFrame = lastFrame - frameStep;
                } else {
                    *newDirection = OutputSchedulerThread::eRenderDirectionForward;
                    *nextFrame = firstFrame + frameStep;
                }
                break;
                case ePlaybackModeOnce:
            default:
                if (direction == OutputSchedulerThread::eRenderDirectionForward) {
                    return false;
                } else {
                    *nextFrame = lastFrame - frameStep;
                    break;
                }

                
        }
    } else {
        if (direction == OutputSchedulerThread::eRenderDirectionForward) {
            *nextFrame = frame + frameStep;
            
        } else {
            *nextFrame = frame - frameStep;
        }
    }
    return true;

}

void
OutputSchedulerThreadPrivate::getNearestInSequence(OutputSchedulerThread::RenderDirectionEnum direction,int frame,
                          int firstFrame,int lastFrame,
                          int* nextFrame)
{
    if (frame >= firstFrame && frame <= lastFrame) {
        *nextFrame = frame;
    } else if (frame < firstFrame) {
        if (direction == OutputSchedulerThread::eRenderDirectionForward) {
            *nextFrame = firstFrame;
        } else {
            *nextFrame = lastFrame;
        }
    } else { // frame > lastFrame
        if (direction == OutputSchedulerThread::eRenderDirectionForward) {
            *nextFrame = lastFrame;
        } else {
            *nextFrame = firstFrame;
        }
    }
    
}

#ifndef NATRON_PLAYBACK_USES_THREAD_POOL
void
OutputSchedulerThread::pushFramesToRender(int startingFrame,int nThreads)
{

    QMutexLocker l(&_imp->framesToRenderMutex);
    _imp->lastFramePushedIndex = startingFrame;
    
    pushFramesToRenderInternal(startingFrame, nThreads);
}

void
OutputSchedulerThread::pushFramesToRenderInternal(int startingFrame,int nThreads)
{
    
    assert(!_imp->framesToRenderMutex.tryLock());
    
    ///Make sure at least 1 frame is pushed
    if (nThreads <= 0) {
        nThreads = 1;
    }
    
    RenderDirectionEnum direction;
    int firstFrame,lastFrame,frameStep;
    {
        QMutexLocker l(&_imp->runArgsMutex);
        direction = _imp->livingRunArgs.timelineDirection;
        firstFrame = _imp->livingRunArgs.firstFrame;
        lastFrame = _imp->livingRunArgs.lastFrame;
        frameStep = _imp->livingRunArgs.frameStep;
    }

    PlaybackModeEnum pMode = _imp->engine->getPlaybackMode();
    if (firstFrame == lastFrame) {
        _imp->framesToRender.push_back(startingFrame);
        _imp->lastFramePushedIndex = startingFrame;
    } else {
        ///Push 2x the count of threads to be sure no one will be waiting
        while ((int)_imp->framesToRender.size() < nThreads * 2) {
            _imp->framesToRender.push_back(startingFrame);
            _imp->lastFramePushedIndex = startingFrame;
            
            if (!OutputSchedulerThreadPrivate::getNextFrameInSequence(pMode, direction, startingFrame,
                                                                      firstFrame, lastFrame, frameStep, &startingFrame, &direction)) {
                break;
            }
        }
    }
  
    ///Wake up render threads to notify them theres work to do
    _imp->framesToRenderNotEmptyCond.wakeAll();

}

void
OutputSchedulerThread::pushAllFrameRange()
{
    QMutexLocker l(&_imp->framesToRenderMutex);
    RenderDirectionEnum direction;
    int firstFrame,lastFrame,frameStep;
    {
        QMutexLocker l(&_imp->runArgsMutex);
        direction = _imp->livingRunArgs.timelineDirection;
        firstFrame = _imp->livingRunArgs.firstFrame;
        lastFrame = _imp->livingRunArgs.lastFrame;
        frameStep = _imp->livingRunArgs.frameStep;
    }
    
    if (direction == eRenderDirectionForward) {
        for (int i = firstFrame; i <= lastFrame; i+=frameStep) {
            _imp->framesToRender.push_back(i);
        }
    } else {
        for (int i = lastFrame; i >= firstFrame; i-=frameStep) {
            _imp->framesToRender.push_back(i);
        }
    }
    ///Wake up render threads to notify them theres work to do
    _imp->framesToRenderNotEmptyCond.wakeAll();
}

void
OutputSchedulerThread::pushFramesToRender(int nThreads)
{
    QMutexLocker l(&_imp->framesToRenderMutex);

    RenderDirectionEnum direction;
    int firstFrame,lastFrame,frameStep;
    {
        QMutexLocker l(&_imp->runArgsMutex);
        direction = _imp->livingRunArgs.timelineDirection;
        firstFrame = _imp->livingRunArgs.firstFrame;
        lastFrame = _imp->livingRunArgs.lastFrame;
        frameStep = _imp->livingRunArgs.frameStep;
    }
    
    PlaybackModeEnum pMode = _imp->engine->getPlaybackMode();
    
    int frame = _imp->lastFramePushedIndex;
    if (firstFrame == lastFrame && frame == firstFrame) {
        return;
    }

    ///If startingTime is already taken into account in the framesToRender, push new frames from the last one in the stack instead
    bool canContinue = OutputSchedulerThreadPrivate::getNextFrameInSequence(pMode, direction, frame,
                                                                        firstFrame, lastFrame, frameStep, &frame, &direction);
    
    if (canContinue) {
        pushFramesToRenderInternal(frame, nThreads);
    } else {
        ///Still wake up threads that may still sleep
        _imp->framesToRenderNotEmptyCond.wakeAll();
    }
}


int
OutputSchedulerThread::pickFrameToRender(RenderThreadTask* thread,bool* enableRenderStats, std::vector<int>* viewsToRender)
{
    ///Flag the thread as inactive
    {
        QMutexLocker l(&_imp->renderThreadsMutex);
        RenderThreads::iterator found = _imp->getRunnableIterator(thread);
        assert(found != _imp->renderThreads.end());
        found->active = false;
        
        ///Wake up the scheduler if it is waiting for all threads do be inactive
        _imp->allRenderThreadsInactiveCond.wakeOne();
    }
    
    ///Simple heuristic to limit the size of the internal buffer.
    ///If the buffer grows too much, we will keep shared ptr to images, hence keep them in RAM which
    ///can lead to RAM issue for the end user.
    ///We can end up in this situation for very simple graphs where the rendering of the output node (the writer or viewer)
    ///is much slower than things upstream, hence the buffer grows quickly, and fills up the RAM.
    int nbThreadsHardware = appPTR->getHardwareIdealThreadCount();
    bool bufferFull;
    {
        QMutexLocker k(&_imp->bufMutex);
        bufferFull = isBufferFull((int)_imp->buf.size(),nbThreadsHardware);
    }
    
    QMutexLocker l(&_imp->framesToRenderMutex);
    while ((bufferFull || _imp->framesToRender.empty()) && !thread->mustQuit() ) {
        
        ///Notify that we're no longer doing work
        thread->notifyIsRunning(false);
        
        
        _imp->framesToRenderNotEmptyCond.wait(&_imp->framesToRenderMutex);
        {
            QMutexLocker k(&_imp->bufMutex);
            bufferFull = isBufferFull((int)_imp->buf.size(),nbThreadsHardware);
        }
        
    }
    
   
    if (!_imp->framesToRender.empty()) {
        
        ///Notify that we're running for good, will do nothing if flagged already running
        thread->notifyIsRunning(true);
        
        int ret = _imp->framesToRender.front();
        _imp->framesToRender.pop_front();
        ///Flag the thread as active
        {
            QMutexLocker l(&_imp->renderThreadsMutex);
            RenderThreads::iterator found = _imp->getRunnableIterator(thread);
            assert(found != _imp->renderThreads.end());
            found->active = true;
        }
        
        {
            QMutexLocker l(&_imp->runArgsMutex);
            *enableRenderStats = _imp->livingRunArgs.enableRenderStats;
            *viewsToRender = _imp->livingRunArgs.viewsToRender;
        }
        
        return ret;
    } else {
        // thread is quitting, make sure we notified the application it is no longer running
        thread->notifyIsRunning(false);
        
    }
    *enableRenderStats = false;
    return -1;
}

#else // NATRON_PLAYBACK_USES_THREAD_POOL

void
OutputSchedulerThread::startTasksFromLastStartedFrame()
{
    int frame;
    bool canContinue;
    
    {
        QMutexLocker l(&_imp->framesToRenderMutex);
        
        RenderDirectionEnum direction;
        int firstFrame,lastFrame,frameStep;
        {
            QMutexLocker l(&_imp->runArgsMutex);
            direction = _imp->livingRunArgs.timelineDirection;
            firstFrame = _imp->livingRunArgs.firstFrame;
            lastFrame = _imp->livingRunArgs.lastFrame;
            frameStep = _imp->livingRunArgs.frameStep;
        }
        
        PlaybackModeEnum pMode = _imp->engine->getPlaybackMode();
        
        frame = _imp->lastFramePushedIndex;
        if (firstFrame == lastFrame && frame == firstFrame) {
            return;
        }
        
        ///If startingTime is already taken into account in the framesToRender, push new frames from the last one in the stack instead
        canContinue = OutputSchedulerThreadPrivate::getNextFrameInSequence(pMode, direction, frame,
                                                                           firstFrame, lastFrame, frameStep, &frame, &direction);
    }
    if (canContinue) {
        QMutexLocker l(&_imp->renderThreadsMutex);
        startTasks(frame);
    }
    
}

void
OutputSchedulerThread::startTasks(int startingFrame)
{
    
    int maxThreads = _imp->threadPool->maxThreadCount();
    int activeThreads = _imp->getNActiveRenderThreads();
    
    //This thread is from the thread pool so do not count it as it is probably done anyway
    if (QThread::currentThread() != this) {
        activeThreads -= 1;
    }
    
    
    
    int nFrames;
#ifdef NATRON_SCHEDULER_SPAWN_THREADS_WITH_TIMER
    //We check every now and then if we need to start new threads
    {
        int nbAvailableThreads = maxThreads  - activeThreads;
        if (nbAvailableThreads <= 0) {
            return;
        }
        nFrames = 1;
    }
#else
    //Start one more thread until we use all the thread pool.
    //We leave some CPU available so that the multi-thread suite can take advantage of it
    nFrames = std::max(std::min(maxThreads - activeThreads, 2), 1);
#endif

    
    RenderDirectionEnum direction;
    int firstFrame,lastFrame,frameStep;
    bool useStats;
    std::vector<int> viewsToRender;
    {
        QMutexLocker l(&_imp->runArgsMutex);
        direction = _imp->livingRunArgs.timelineDirection;
        firstFrame = _imp->livingRunArgs.firstFrame;
        lastFrame = _imp->livingRunArgs.lastFrame;
        frameStep = _imp->livingRunArgs.frameStep;
        useStats = _imp->livingRunArgs.enableRenderStats;
        viewsToRender = _imp->livingRunArgs.viewsToRender;
    }
    
    PlaybackModeEnum pMode = _imp->engine->getPlaybackMode();
    if (firstFrame == lastFrame) {
        RenderThreadTask* task = createRunnable(startingFrame, useStats, viewsToRender);
        _imp->appendRunnable(task);
        
        QMutexLocker k(&_imp->framesToRenderMutex);
        _imp->lastFramePushedIndex = startingFrame;
    } else {
        int frame = startingFrame;
        for (int i = 0; i < nFrames; ++i) {
            RenderThreadTask* task = createRunnable(frame, useStats, viewsToRender);
            _imp->appendRunnable(task);
            
            
            {
                QMutexLocker k(&_imp->framesToRenderMutex);
                _imp->lastFramePushedIndex = frame;
            }
            
            if (!OutputSchedulerThreadPrivate::getNextFrameInSequence(pMode, direction, frame,
                                                                      firstFrame, lastFrame, frameStep, &frame, &direction)) {
                break;
            }
        }
    }
}

#endif //NATRON_PLAYBACK_USES_THREAD_POOL


void
OutputSchedulerThread::onThreadSpawnsTimerTriggered()
{
#ifdef NATRON_SCHEDULER_SPAWN_THREADS_WITH_TIMER

#ifdef NATRON_PLAYBACK_USES_THREAD_POOL
    startTasksFromLastStartedFrame();
#else
    ///////////
    /////If we were analysing the CPU activity, now set the appropriate number of threads to render.
    int newNThreads, lastNThreads;
    adjustNumberOfThreads(&newNThreads,&lastNThreads);
    
    ///////////
    /////Append render requests for the render threads
    pushFramesToRender(newNThreads);
#endif
    
#endif
}

void
OutputSchedulerThread::notifyThreadAboutToQuit(RenderThreadTask* thread)
{
    QMutexLocker l(&_imp->renderThreadsMutex);
    RenderThreads::iterator found = _imp->getRunnableIterator(thread);
    if (found != _imp->renderThreads.end()) {
        found->active = false;
#ifdef NATRON_PLAYBACK_USES_THREAD_POOL
        _imp->renderThreads.erase(found);
#endif
        _imp->allRenderThreadsInactiveCond.wakeOne();
        
#ifndef NATRON_PLAYBACK_USES_THREAD_POOL
        _imp->allRenderThreadsQuitCond.wakeOne();
#endif
    }
}

bool
OutputSchedulerThread::isBeingAborted() const
{
    QMutexLocker k(&_imp->abortFlagMutex);
    return _imp->abortFlag;
}


void
OutputSchedulerThread::startRender()
{
    

    
    if ( isFPSRegulationNeeded() ) {
        _imp->timer->playState = ePlayStateRunning;
    }
    
    ///We will push frame to renders starting at startingFrame.
    ///They will be in the range determined by firstFrame-lastFrame
    int startingFrame;
    int firstFrame,lastFrame;
    bool forward;
    {
        ///Copy the last requested run args
        
        QMutexLocker l(&_imp->runArgsMutex);
        _imp->livingRunArgs = _imp->requestedRunArgs;
        firstFrame = _imp->livingRunArgs.firstFrame;
        lastFrame = _imp->livingRunArgs.lastFrame;
        startingFrame = timelineGetTime();
        forward = _imp->livingRunArgs.timelineDirection == OutputSchedulerThread::eRenderDirectionForward;
    }
    
    aboutToStartRender();
    
    ///Notify everyone that the render is started
    _imp->engine->s_renderStarted(forward);
    
    ///Flag that we're now doing work
    {
        QMutexLocker l(&_imp->workingMutex);
        _imp->working = true;
    }

#ifndef NATRON_PLAYBACK_USES_THREAD_POOL
    int nThreads;
    {
        QMutexLocker l(&_imp->renderThreadsMutex);
        _imp->removeAllQuitRenderThreads();
        nThreads = (int)_imp->renderThreads.size();
    }

    ///Start with one thread if it doesn't exist
    if (nThreads == 0) {
        int lastNThreads;
        adjustNumberOfThreads(&nThreads, &lastNThreads);
    }
#endif
    
    QMutexLocker l(&_imp->renderThreadsMutex);
    
    
    SchedulingPolicyEnum policy = getSchedulingPolicy();
    if (policy == eSchedulingPolicyFFA) {
        
#ifndef NATRON_PLAYBACK_USES_THREAD_POOL
        ///push all frame range and let the threads deal with it
        pushAllFrameRange();
#endif
    } else {
        
        ///If the output effect is sequential (only WriteFFMPEG for now)
        boost::shared_ptr<OutputEffectInstance> effect = _imp->outputEffect.lock();
        SequentialPreferenceEnum pref = effect->getSequentialPreference();
        if (pref == eSequentialPreferenceOnlySequential || pref == eSequentialPreferencePreferSequential) {
            
            RenderScale scaleOne(1.);
            if (effect->beginSequenceRender_public(firstFrame, lastFrame,
                                                               1,
                                                               false,
                                                               scaleOne, true,
                                                               true,
                                                               false,
                                                               /*mainView*/0) == eStatusFailed) {
                l.unlock();
                abortRendering(false,false);
                return;
            }
        }
#ifndef NATRON_PLAYBACK_USES_THREAD_POOL
        ///Push as many frames as there are threads
        pushFramesToRender(startingFrame,nThreads);
#endif
    }
    

#ifdef NATRON_PLAYBACK_USES_THREAD_POOL
    startTasks(startingFrame);
#endif
    
#ifdef NATRON_SCHEDULER_SPAWN_THREADS_WITH_TIMER
    QMutexLocker k(&_imp->lastRecordedFPSMutex);
    double timeoutMS = _imp->lastRecordedFPS == 0. ? NATRON_SCHEDULER_THREADS_SPAWN_DEFAULT_TIMEOUT_MS : (1. / _imp->lastRecordedFPS) * 1000;
    _imp->threadSpawnsTimer.start(timeoutMS);
#endif
    
}

void
OutputSchedulerThread::stopRender()
{
    _imp->timer->playState = ePlayStatePause;
    
#ifdef NATRON_SCHEDULER_SPAWN_THREADS_WITH_TIMER
    QMutexLocker k(&_imp->lastRecordedFPSMutex);
    _imp->lastRecordedFPS = _imp->timer->getActualFrameRate();
    _imp->threadSpawnsTimer.stop();
#endif
    
    ///Wait for all render threads to be done
    
#ifndef NATRON_PLAYBACK_USES_THREAD_POOL
    ///Clear the work queue
    {
        QMutexLocker framesLocker (&_imp->framesToRenderMutex);
        _imp->framesToRender.clear();
    }
#endif
    
    ///Remove all current threads so the new render doesn't have many threads concurrently trying to do the same thing at the same time
#ifndef NATRON_PLAYBACK_USES_THREAD_POOL
    stopRenderThreads(0);
#endif
    _imp->waitForRenderThreadsToQuit();
    
    
    /*{
        QMutexLocker l(&_imp->renderThreadsMutex);
        
        _imp->removeAllQuitRenderThreads();
        _imp->waitForRenderThreadsToBeDone();
    }*/
    
    
    ///If the output effect is sequential (only WriteFFMPEG for now)
    boost::shared_ptr<OutputEffectInstance> effect = _imp->outputEffect.lock();
    SequentialPreferenceEnum pref = effect->getSequentialPreference();
    if (pref == eSequentialPreferenceOnlySequential || pref == eSequentialPreferencePreferSequential) {
        
        int firstFrame,lastFrame;
        {
            QMutexLocker l(&_imp->runArgsMutex);
            firstFrame = _imp->livingRunArgs.firstFrame;
            lastFrame = _imp->livingRunArgs.lastFrame;
        }

        
        RenderScale scaleOne(1.);
        ignore_result(effect->endSequenceRender_public(firstFrame, lastFrame,
                                                           1,
                                                           !appPTR->isBackground(),
                                                           scaleOne, true,
                                                           !appPTR->isBackground(),
                                                                false,
                                                           0 /*mainView*/));
           
        
    }
    
    {
        QMutexLocker abortBeingProcessedLocker(&_imp->abortBeingProcessedMutex);
        _imp->abortBeingProcessed = true;
        
        
        bool wasAborted;
        {
            QMutexLocker l(&_imp->abortedRequestedMutex);
            wasAborted = _imp->abortRequested > 0;
            
            ///reset back the abort flag
            _imp->abortRequested = 0;
            
  
            ///Notify everyone that the render is finished
            _imp->engine->s_renderFinished(wasAborted ? 1 : 0);
            
            onRenderStopped(wasAborted);
            
            ///Flag that we're no longer doing work
            {
                QMutexLocker l(&_imp->workingMutex);
                _imp->working = false;
            }
            
            
            
            {
                QMutexLocker k(&_imp->abortFlagMutex);
                _imp->abortFlag = false;
            }
            //_imp->outputEffect->getApp()->getProject()->setAllNodesAborted(false);
            _imp->abortedRequestedCondition.wakeAll();
        }
        
        
        {
            QMutexLocker k(&_imp->bufMutex);
            _imp->buf.clear();
        }

        
    }
    
    {
        QMutexLocker l(&_imp->startRequestsMutex);
        while (_imp->startRequests <= 0) {
            _imp->startRequestsCond.wait(&_imp->startRequestsMutex);
        }
        ///We got the request, reset it back to 0
        _imp->startRequests = 0;
    }
}

void
OutputSchedulerThread::run()
{
    for (;;) { ///infinite loop
        
        if ( _imp->checkForExit() ) {
            return;
        }
        
        startRender();
        for (;;) {
            ///When set to true, we don't sleep in the bufEmptyCondition but in the startCondition instead, indicating
            ///we finished a render
            bool renderFinished = false;
            
            {
                ///_imp->renderFinished might be set when in FFA scheduling policy
                QMutexLocker l(&_imp->runArgsMutex);
                if (_imp->renderFinished) {
                    renderFinished = true;
                }
            }
            
            bool bufferEmpty;
            {
                QMutexLocker l(&_imp->bufMutex);
                bufferEmpty = _imp->buf.empty();
            }
            
            int expectedTimeToRender;
            bool isAbortRequested;
            bool blocking;
            
            while (!bufferEmpty) {
                
                if ( _imp->checkForExit() ) {
                    return;
                }
                
                ///Check for abortion
                {
                    QMutexLocker locker(&_imp->abortedRequestedMutex);
                    if (_imp->abortRequested > 0) {
                        
                        ///Do not wait in the buf wait condition and go directly into the stopEngine()
                        renderFinished = true;
                        break;
                    }
                }
                
                expectedTimeToRender = timelineGetTime();
                
                BufferedFrames framesToRender;
                {
                    QMutexLocker l(&_imp->bufMutex);
                    _imp->getFromBufferAndErase(expectedTimeToRender, framesToRender);
                }
                
                ///The expected frame is not yet ready, go to sleep again
                if (framesToRender.empty()) {
                    break;
                }
    
                int nextFrameToRender = -1;
               
                if (!renderFinished) {
                
                    ///////////
                    /////Refresh frame range if needed (for viewers)
                    

                    int firstFrame,lastFrame;
                    getFrameRangeToRender(firstFrame, lastFrame);
                    
                    
                    RenderDirectionEnum timelineDirection;
                    int frameStep;
                    {
                        QMutexLocker l(&_imp->runArgsMutex);
                        
                        ///Refresh the firstframe/lastFrame as they might have changed on the timeline
                        _imp->livingRunArgs.firstFrame = firstFrame;
                        _imp->livingRunArgs.lastFrame = lastFrame;
                        
                        
                        
                        timelineDirection = _imp->livingRunArgs.timelineDirection;
                        frameStep = _imp->livingRunArgs.frameStep;
                    }
                    
                    ///////////
                    ///Determine if we finished rendering or if we should just increment/decrement the timeline
                    ///or just loop/bounce
                    PlaybackModeEnum pMode = _imp->engine->getPlaybackMode();
                    RenderDirectionEnum newDirection;
                    if (firstFrame == lastFrame && pMode == ePlaybackModeOnce) {
                        renderFinished = true;
                        newDirection = eRenderDirectionForward;
                    } else {
                        renderFinished = !OutputSchedulerThreadPrivate::getNextFrameInSequence(pMode, timelineDirection,
                                                                                          expectedTimeToRender, firstFrame,
                                                                                          lastFrame, frameStep, &nextFrameToRender, &newDirection);
                    }
                    if (newDirection != timelineDirection) {
                        QMutexLocker l(&_imp->runArgsMutex);
                        _imp->livingRunArgs.timelineDirection = newDirection;
                        _imp->requestedRunArgs.timelineDirection = newDirection;
                    }
                                        
                    if (!renderFinished) {
#ifndef NATRON_SCHEDULER_SPAWN_THREADS_WITH_TIMER
                        
#ifndef NATRON_PLAYBACK_USES_THREAD_POOL
                        ///////////
                        /////If we were analysing the CPU activity, now set the appropriate number of threads to render.
                        int newNThreads, lastNThreads;
                        adjustNumberOfThreads(&newNThreads,&lastNThreads);
                        
                        ///////////
                        /////Append render requests for the render threads
                        pushFramesToRender(newNThreads);
#else
                        startTasksFromLastStartedFrame();
#endif
                        
#endif
                    }
                } // if (!renderFinished) {
                
                if (_imp->timer->playState == ePlayStateRunning) {
                    _imp->timer->waitUntilNextFrameIsDue(); // timer synchronizing with the requested fps
                }
                
                
                {
                    QMutexLocker abortRequestedLock (&_imp->abortedRequestedMutex);
                    isAbortRequested = _imp->abortRequested > 0;
                }
                if (isAbortRequested) {
                    break;
                }
                
                if (_imp->mode == eProcessFrameBySchedulerThread) {
                    processFrame(framesToRender);
                } else {
                    ///Process on main-thread
                                    
                    QMutexLocker processLocker (&_imp->processMutex);
                    
                    ///Check for abortion while under processMutex to be sure the main thread is not deadlock in abortRendering
                    {
                        QMutexLocker locker(&_imp->abortedRequestedMutex);
                        if (_imp->abortRequested > 0) {
                            
                            ///Do not wait in the buf wait condition and go directly into the stopRender()
                            renderFinished = true;
                            
                            break;
                        }
                    }
                    assert(!_imp->processRunning);
                    _imp->processRunning = true;
            

                    Q_EMIT s_doProcessOnMainThread(framesToRender);
                    
                    while (_imp->processRunning) {
                        _imp->processCondition.wait(&_imp->processMutex);
                    }
        
                    
                } // if (_imp->mode == eProcessFrameBySchedulerThread) {
                
                if (!renderFinished) {
                    ///Timeline might have changed if another thread moved the playhead
                    int timelineCurrentTime = timelineGetTime();
                    if (timelineCurrentTime != expectedTimeToRender) {
                        timelineGoTo(timelineCurrentTime);
                    } else {
                        timelineGoTo(nextFrameToRender);
                    }
                    
                }
                
                ////////////
                /////At this point the frame has been processed by the output device
                
                assert(!framesToRender.empty());
                {
                    const BufferedFrame& frame = framesToRender.front();
                    std::vector<int> views(1);
                    views[0] = frame.view;
                    notifyFrameRendered(expectedTimeToRender, frame.view, views, frame.stats, eSchedulingPolicyOrdered);
                }
                
                ///////////
                /// End of the loop, refresh bufferEmpty
                {
                    QMutexLocker l(&_imp->bufMutex);
                    bufferEmpty = _imp->buf.empty();
                }
                
            } // while(!bufferEmpty)
            
            ///Refresh the abort requested flag because the abort might have got called in between
            {
                QMutexLocker abortRequestedLock (&_imp->abortedRequestedMutex);
                isAbortRequested = _imp->abortRequested > 0;
                blocking = _imp->isAbortRequestBlocking;
            }

           
            if (!renderFinished && !isAbortRequested) {
                
                QMutexLocker bufLocker (&_imp->bufMutex);
                ///Wait here for more frames to be rendered, we will be woken up once appendToBuffer(...) is called
                if (_imp->buf.empty()) {
                    _imp->bufCondition.wait(&_imp->bufMutex);
                }
                /*else {
                    
                    if (isBufferFull((int)_imp->buf.size(),appPTR->getHardwareIdealThreadCount())) {
                        qDebug() << "PLAYBACK STALL detected: Internal buffer is full but frame" << expectedTimeToRender
                        << "is still expected to be rendered. Stopping render.";
                        assert(false);
                        break;
                    }
                }*/
            } else {
                if (blocking) {
                    //Move the timeline to the last rendered frame to keep it in sync with what is displayed
                    timelineGoTo(getLastRenderedTime());
                }
                break;
            }
        } // for (;;)
        
         stopRender();
        
    } // for(;;)
    
}

#ifndef NATRON_PLAYBACK_USES_THREAD_POOL
void
OutputSchedulerThread::adjustNumberOfThreads(int* newNThreads, int *lastNThreads)
{
    ///////////
    /////If we were analysing the CPU activity, now set the appropriate number of threads to render.
    int optimalNThreads;
    
    ///How many parallel renders the user wants
    int userSettingParallelThreads = appPTR->getCurrentSettings()->getNumberOfParallelRenders();
    
    ///How many threads are running in the application
    int runningThreads = appPTR->getNRunningThreads() + QThreadPool::globalInstance()->activeThreadCount();
    
    ///How many current threads are used by THIS renderer
    int currentParallelRenders = getNRenderThreads();
    *lastNThreads = currentParallelRenders;
    
    if (userSettingParallelThreads == 0) {
        ///User wants it to be automatically computed, do a simple heuristic: launch as many parallel renders
        ///as there are cores
        optimalNThreads = appPTR->getHardwareIdealThreadCount();
    } else {
        optimalNThreads = userSettingParallelThreads;
    }
    optimalNThreads = std::max(1,optimalNThreads);


    if ((runningThreads < optimalNThreads && currentParallelRenders < optimalNThreads) || currentParallelRenders == 0) {
     
        ////////
        ///Launch 1 thread
        QMutexLocker l(&_imp->renderThreadsMutex);
        
        _imp->appendRunnable(createRunnable());
        *newNThreads = currentParallelRenders +  1;
        
    } else if (runningThreads > optimalNThreads && currentParallelRenders > optimalNThreads) {
        ////////
        ///Stop 1 thread
        stopRenderThreads(1);
        *newNThreads = currentParallelRenders - 1;
        
    } else {
        /////////
        ///Keep the current count
        *newNThreads = std::max(1,currentParallelRenders);
    }
}
#endif

void
OutputSchedulerThread::notifyFrameRendered(int frame,
                                           int viewIndex,
                                           const std::vector<int>& viewsToRender,
                                           const RenderStatsPtr& stats,
                                           SchedulingPolicyEnum policy)
{
    assert(viewsToRender.size() > 0);
    
    double percentage = 0.;
    double timeSpent;
    
    boost::shared_ptr<OutputEffectInstance> effect = _imp->outputEffect.lock();
    
    if (stats) {
        std::map<NodePtr,NodeRenderStats > statResults = stats->getStats(&timeSpent);
        if (!statResults.empty()) {
            effect->reportStats(frame, viewIndex, timeSpent, statResults);
        }
    }
    //U64 nbFramesLeftToRender;
    bool isBackground = appPTR->isBackground();
    int nbCurParallelRenders = 1;

    if (policy == eSchedulingPolicyFFA) {
        
        QMutexLocker l(&_imp->runArgsMutex);
        if (viewIndex == viewsToRender[viewsToRender.size() - 1] || viewIndex == -1) {
            ++_imp->nFramesRendered;
        }
        U64 totalFrames = std::ceil((double)(_imp->livingRunArgs.lastFrame - _imp->livingRunArgs.firstFrame + 1) / _imp->livingRunArgs.frameStep);
        assert(totalFrames > 0);
        if (totalFrames != 0) {
            percentage = (double)_imp->nFramesRendered / totalFrames;
        }
        //nbFramesLeftToRender = totalFrames - _imp->nFramesRendered;
        if ( _imp->nFramesRendered == totalFrames) {

            _imp->renderFinished = true;
            l.unlock();

            ///Notify the scheduler rendering is finished by append a fake frame to the buffer
            {
                QMutexLocker bufLocker (&_imp->bufMutex);
                ignore_result(_imp->appendBufferedFrame(0, 0, RenderStatsPtr(), boost::shared_ptr<BufferableObject>()));
                _imp->bufCondition.wakeOne();
            }
        } else {
            l.unlock();
            
#ifndef NATRON_SCHEDULER_SPAWN_THREADS_WITH_TIMER
            
#ifndef NATRON_PLAYBACK_USES_THREAD_POOL
            ///////////
            /////If we were analysing the CPU activity, now set the appropriate number of threads to render.
            int newNThreads;
            adjustNumberOfThreads(&newNThreads, &nbCurParallelRenders);
#else
            startTasksFromLastStartedFrame();
#endif
            
#endif
        }
    } else {
        {
            QMutexLocker l(&_imp->runArgsMutex);
            U64 totalFrames = std::floor((double)(_imp->livingRunArgs.lastFrame - _imp->livingRunArgs.firstFrame + 1) / _imp->livingRunArgs.frameStep);
            assert(totalFrames > 0);
            if (_imp->livingRunArgs.timelineDirection == eRenderDirectionForward) {
                if (totalFrames != 0) {
                    percentage = (double)(frame - _imp->livingRunArgs.firstFrame) / _imp->livingRunArgs.frameStep / totalFrames;
                }
               // nbFramesLeftToRender = (double)(_imp->livingRunArgs.lastFrame - frame) / _imp->livingRunArgs.frameStep;
            } else {
                if (totalFrames != 0) {
                    percentage = (double)(_imp->livingRunArgs.lastFrame - frame)  / totalFrames;
                }
               // nbFramesLeftToRender = (double)(frame - _imp->livingRunArgs.firstFrame) / _imp->livingRunArgs.frameStep;
            }
        }
        nbCurParallelRenders = getNRenderThreads();
    } // if (policy == eSchedulingPolicyFFA) {
    
    double avgTimeSpent = 0.;
    double timeRemaining = 0.;
    double totalTimeSpent = 0.;
    if (stats) {
        effect->updateRenderTimeInfos(timeSpent, &avgTimeSpent, &totalTimeSpent);
        if (percentage != 0) {
            timeRemaining = nbCurParallelRenders ? (totalTimeSpent * (1 - percentage) / percentage) / (double)nbCurParallelRenders : 0.;
        } else {
            //Unknown yet
            timeRemaining = -1;
        }
    }
    
    if (isBackground) {
        QString longMessage;
        QTextStream ts(&longMessage);
        QString frameStr = QString::number(frame);
        ts << kFrameRenderedStringLong << frameStr << " (" << QString::number(percentage * 100,'f',1) << "%)";
        if (stats) {
            QString timeSpentStr = Timer::printAsTime(timeSpent, false);
            QString timeRemainingStr = Timer::printAsTime(timeRemaining, true);
            ts << "\nTime elapsed for frame: " << timeSpentStr;
            ts << "\nTime remaining: " << timeRemainingStr;
            frameStr.append(';');
            frameStr.append(QString::number(timeSpent));
            frameStr.append(';');
            frameStr.append(QString::number(timeRemaining));
        }
        appPTR->writeToOutputPipe(longMessage,kFrameRenderedStringShort + frameStr);
    }
    
    if (viewIndex == viewsToRender[viewsToRender.size() - 1] || viewIndex == -1) {
        if (!stats) {
            _imp->engine->s_frameRendered(frame);
        } else {
            _imp->engine->s_frameRenderedWithTimer(frame, timeSpent, timeRemaining);
        }
    }
    
    if (effect->isWriter()) {
        std::string cb = effect->getNode()->getAfterFrameRenderCallback();
        if (!cb.empty()) {  
            std::vector<std::string> args;
            std::string error;
            try {
                Python::getFunctionArguments(cb, &error, &args);
            } catch (const std::exception& e) {
                effect->getApp()->appendToScriptEditor(std::string("Failed to run onFrameRendered callback: ")
                                                                 + e.what());
                return;
            }
            if (!error.empty()) {
                effect->getApp()->appendToScriptEditor("Failed to run after frame render callback: " + error);
                return;
            }
            
            std::string signatureError;
            signatureError.append("The after frame render callback supports the following signature(s):\n");
            signatureError.append("- callback(frame, thisNode, app)");
            if (args.size() != 3) {
                effect->getApp()->appendToScriptEditor("Failed to run after frame render callback: " + signatureError);
                return;
            }
            
            if (args[0] != "frame" || args[1] != "thisNode" || args[2] != "app" ) {
                effect->getApp()->appendToScriptEditor("Failed to run after frame render callback: " + signatureError);
                return;
            }
            
            std::stringstream ss;
            std::string appStr = effect->getApp()->getAppIDString();
            std::string outputNodeName = appStr + "." + effect->getNode()->getFullyQualifiedName();
            ss << cb << "(" << frame << ", ";
            ss << outputNodeName << ", " << appStr << ")";
            std::string script = ss.str();
            try {
                runCallbackWithVariables(script.c_str());
            } catch (const std::exception& e) {
                notifyRenderFailure(e.what());
                return;
            }
            
        }
    }
}

void
OutputSchedulerThread::appendToBuffer_internal(double time,
                                               int view,
                                               const RenderStatsPtr& stats,
                                               const boost::shared_ptr<BufferableObject>& frame,
                                               bool wakeThread)
{
    if (QThread::currentThread() == qApp->thread()) {
        ///Single-threaded , call directly the function
        if (frame) {
            BufferedFrame b;
            b.time = time;
            b.view = view;
            b.frame = frame;
            BufferedFrames frames;
            frames.push_back(b);
            processFrame(frames);
        }
    } else {
        
        ///Called by the scheduler thread when an image is rendered
        
        QMutexLocker l(&_imp->bufMutex);
        ignore_result(_imp->appendBufferedFrame(time, view, stats, frame));
        if (wakeThread) {
            ///Wake up the scheduler thread that an image is available if it is asleep so it can process it.
            _imp->bufCondition.wakeOne();
        }
        
    }
}

void
OutputSchedulerThread::appendToBuffer(double time,
                                      int view,
                                      const RenderStatsPtr& stats,
                                      const boost::shared_ptr<BufferableObject>& image)
{
    appendToBuffer_internal(time, view, stats, image, true);
}

void
OutputSchedulerThread::appendToBuffer(double time,
                                      int view,
                                      const RenderStatsPtr& stats,
                                      const BufferableObjectList& frames)
{
    if (frames.empty()) {
        return;
    }
    BufferableObjectList::const_iterator next = frames.begin();
    if (next != frames.end()) {
        ++next;
    }
    for (BufferableObjectList::const_iterator it = frames.begin(); it != frames.end(); ++it) {
        if (next != frames.end()) {
            appendToBuffer_internal(time, view, stats, *it, false);
            ++next;
        } else {
            appendToBuffer_internal(time, view, stats, *it, true);
        }
    }
}


void
OutputSchedulerThread::doProcessFrameMainThread(const BufferedFrames& frames)
{
    assert(QThread::currentThread() == qApp->thread());
    {
        QMutexLocker processLocker (&_imp->processMutex);
        ///The flag might have been reseted back by abortRendering()
        if (!_imp->processRunning) {
            return;
        }
    }
    
    
    processFrame(frames);
 
    {
        QMutexLocker processLocker (&_imp->processMutex);
        assert(_imp->processRunning);
        _imp->processRunning = false;
        _imp->processCondition.wakeOne();
    }
}

void
OutputSchedulerThread::abortRendering(bool autoRestart,bool blocking)
{
    
    if ( !isRunning() || !isWorking() ) {
        QMutexLocker l(&_imp->abortedRequestedMutex);
        
        ///Never allow playback auto-restart when it is not activated explicitly by the user
        _imp->canAutoRestartPlayback = false;
        return;
    }

    bool isMainThread = QThread::currentThread() == qApp->thread();
    
    boost::shared_ptr<OutputEffectInstance> effect = _imp->outputEffect.lock();

    {
        ///Before posting an abort request, we must make sure the scheduler thread is not currently processing an abort request
        ///in stopRender(), we ensure the former by taking the abortBeingProcessedMutex lock
        QMutexLocker l(&_imp->abortedRequestedMutex);
        _imp->abortBeingProcessed = false;
        _imp->canAutoRestartPlayback = autoRestart;
        _imp->isAbortRequestBlocking = blocking;
        
        ///We make sure the render-thread doesn't wait for the main-thread to process a frame
        ///This function (abortRendering) was probably called from a user event that was posted earlier in the
        ///event-loop, we just flag that the next event that will process the frame should NOT process it by
        ///reseting the processRunning flag
        {
            QMutexLocker l2(&_imp->processMutex);
            
            {
                QMutexLocker abortBeingProcessedLocker(&_imp->abortBeingProcessedMutex);
                
                ///We are already aborting but we don't want a blocking abort, it is useless to ask for a second abort
                if (!blocking && _imp->abortRequested > 0) {
                    return;
                }

                {
                    QMutexLocker k(&_imp->abortFlagMutex);
                    _imp->abortFlag = true;
                }
                effect->getApp()->getProject()->notifyRenderBeingAborted();
                
                ++_imp->abortRequested;
            }
            
            ///Clear the work queue
           /* {
                QMutexLocker framesLocker (&_imp->framesToRenderMutex);
                _imp->framesToRender.clear();
            }
            
            {
                QMutexLocker k(&_imp->bufMutex);
                _imp->buf.clear();
            }*/
            
            if (isMainThread) {
                
                _imp->processRunning = false;
                _imp->processCondition.wakeOne();
            }

            
        } // QMutexLocker l2(&_imp->processMutex);
        ///If the scheduler is asleep waiting for the buffer to be filling up, we post a fake request
        ///that will not be processed anyway because the first thing it does is checking for abort
        {
            QMutexLocker l2(&_imp->bufMutex);
            _imp->bufCondition.wakeOne();
        }
        
        while (blocking && _imp->abortRequested > 0 && QThread::currentThread() != this && isWorking()) {
            _imp->abortedRequestedCondition.wait(&_imp->abortedRequestedMutex);
        }
    }
}

void
OutputSchedulerThread::quitThread()
{
    if (!isRunning()) {
        return;
    }
    
    abortRendering(false,true);
    
    
    if (QThread::currentThread() == qApp->thread()) {
        ///If the scheduler thread was sleeping in the process condition, waiting for the main-thread to finish
        ///processing the frame then waiting in the mustQuitCond would create a deadlock.
        ///Instead we discard the processing of the frame by taking the lock and setting processRunning to false
        QMutexLocker processLocker (&_imp->processMutex);
        _imp->processRunning = false;
        _imp->processCondition.wakeOne();
    }
    
    {
        QMutexLocker l(&_imp->mustQuitMutex);
        _imp->mustQuit = true;
        
        ///Wake-up the thread with a fake request
        {
            QMutexLocker l3(&_imp->startRequestsMutex);
            ++_imp->startRequests;
            _imp->startRequestsCond.wakeOne();
        }
        
        ///Wait until it has really quit
        while (_imp->mustQuit) {
            _imp->mustQuitCond.wait(&_imp->mustQuitMutex);
        }
    }
    
    ///Wake-up all threads and tell them that they must quit
    stopRenderThreads(0);

    ///Make sure they are all gone, there will be a deadlock here if that's not the case.
    _imp->waitForRenderThreadsToQuit();
        
    
    
    wait();
}

bool
OutputSchedulerThread::mustQuitThread() const
{
    QMutexLocker l(&_imp->mustQuitMutex);
    return _imp->mustQuit;
}

void
OutputSchedulerThread::setDesiredFPS(double d)
{
    _imp->timer->setDesiredFrameRate(d);
}


double
OutputSchedulerThread::getDesiredFPS() const
{
    return _imp->timer->getDesiredFrameRate();
}

void
OutputSchedulerThread::renderFrameRange(bool isBlocking,
                                        bool enableRenderStats,
                                        int firstFrame,
                                        int lastFrame,
                                        int frameStep,
                                        const std::vector<int>& viewsToRender,
                                        RenderDirectionEnum direction)
{
    if (direction == eRenderDirectionForward) {
        timelineGoTo(firstFrame);
    } else {
        timelineGoTo(lastFrame);
    }
    
    {
        
        QMutexLocker l(&_imp->runArgsMutex);
        _imp->requestedRunArgs.firstFrame = firstFrame;
        _imp->requestedRunArgs.lastFrame = lastFrame;
        _imp->requestedRunArgs.isBlocking = isBlocking;
        _imp->requestedRunArgs.frameStep = frameStep;
        _imp->requestedRunArgs.viewsToRender = viewsToRender;
        
        _imp->nFramesRendered = 0;
        _imp->renderFinished = false;
        
        ///Start with picking direction being the same as the timeline direction.
        ///Once the render threads are a few frames ahead the picking direction might be different than the
        ///timeline direction
        _imp->requestedRunArgs.timelineDirection = direction;

        _imp->requestedRunArgs.enableRenderStats = enableRenderStats;
    }
    
    renderInternal();
    
}

bool
OutputSchedulerThread::isPlaybackAutoRestartEnabled() const
{
    QMutexLocker k(&_imp->abortedRequestedMutex);
    return _imp->canAutoRestartPlayback;
}

void
OutputSchedulerThread::renderFromCurrentFrame(bool enableRenderStats,
                                              const std::vector<int>& viewsToRender,
                                              RenderDirectionEnum timelineDirection)
{
    

    {
        QMutexLocker l(&_imp->runArgsMutex);

        int firstFrame,lastFrame;
        getFrameRangeToRender(firstFrame, lastFrame);
  
        ///Make sure current frame is in the frame range
        int currentTime = timelineGetTime();
        OutputSchedulerThreadPrivate::getNearestInSequence(timelineDirection, currentTime, firstFrame, lastFrame, &currentTime);
        
        _imp->requestedRunArgs.firstFrame = firstFrame;
        _imp->requestedRunArgs.lastFrame = lastFrame;
        _imp->requestedRunArgs.viewsToRender = viewsToRender;
        _imp->requestedRunArgs.frameStep = 1;
        _imp->requestedRunArgs.timelineDirection = timelineDirection;
        _imp->requestedRunArgs.enableRenderStats = enableRenderStats;
        _imp->requestedRunArgs.isBlocking = false;
    }
    renderInternal();
}


void
OutputSchedulerThread::renderInternal()
{
    
    QMutexLocker quitLocker(&_imp->mustQuitMutex);
    if (_imp->hasQuit) {
        return;
    }
    
    if (!_imp->mustQuit) {
        if ( !isRunning() ) {
            ///The scheduler must remain responsive hence has the highest priority
            start(HighestPriority);
        } else {
            ///Wake up the thread with a start request
            QMutexLocker locker(&_imp->startRequestsMutex);
            if (_imp->startRequests <= 0) {
                ++_imp->startRequests;
            }
            _imp->startRequestsCond.wakeOne();
        }
    }
}

void
OutputSchedulerThread::notifyRenderFailure(const std::string& errorMessage)
{
    ///Abort all ongoing rendering
    bool isBlocking;
    {
        QMutexLocker l(&_imp->runArgsMutex);
        isBlocking = _imp->livingRunArgs.isBlocking;
    }
    if (!isBlocking) {
        doAbortRenderingOnMainThread(false);
    } else {
        abortRendering(false, false);
    }
    
    ///Handle failure: for viewers we make it black and don't display the error message which is irrelevant
    handleRenderFailure(errorMessage);
    
}



bool
OutputSchedulerThread::isWorking() const
{
    QMutexLocker l(&_imp->workingMutex);
    return _imp->working;
}


void
OutputSchedulerThread::getFrameRangeRequestedToRender(int &first,int& last) const
{
    first = _imp->livingRunArgs.firstFrame;
    last = _imp->livingRunArgs.lastFrame;
}

OutputSchedulerThread::RenderDirectionEnum
OutputSchedulerThread::getDirectionRequestedToRender() const
{
    QMutexLocker l(&_imp->runArgsMutex);
    return _imp->livingRunArgs.timelineDirection;
}

std::vector<int>
OutputSchedulerThread::getViewsRequestedToRender() const
{
    QMutexLocker l(&_imp->runArgsMutex);
    return _imp->livingRunArgs.viewsToRender;
}

int
OutputSchedulerThread::getNRenderThreads() const
{
    QMutexLocker l(&_imp->renderThreadsMutex);
    return (int)_imp->renderThreads.size();
}

int
OutputSchedulerThread::getNActiveRenderThreads() const
{
    QMutexLocker l(&_imp->renderThreadsMutex);
    return _imp->getNActiveRenderThreads();
}

void
OutputSchedulerThread::stopRenderThreads(int nThreadsToStop)
{
    
#ifndef NATRON_PLAYBACK_USES_THREAD_POOL
    {
        
         ///First flag the number of threads to stop
        QMutexLocker l(&_imp->renderThreadsMutex);
        int i = 0;
        for (RenderThreads::iterator it = _imp->renderThreads.begin();
             it!=_imp->renderThreads.end() && (i < nThreadsToStop || nThreadsToStop == 0); ++it) {
            if (!it->thread->mustQuit()) {
                it->thread->scheduleForRemoval();
                ++i;
            }
            
        }
        
        ///Clean-up remaining zombie threads that are no longer useful
        _imp->removeAllQuitRenderThreads();
    }
    
    ///Wake-up all threads to make sure that they are notified that they must quit
    {
        QMutexLocker framesLocker(&_imp->framesToRenderMutex);
        _imp->framesToRenderNotEmptyCond.wakeAll();
    }
    
#else
    Q_UNUSED(nThreadsToStop);
    QMutexLocker l(&_imp->renderThreadsMutex);
    _imp->waitForRenderThreadsToBeDone();
#endif
    
    



}

RenderEngine*
OutputSchedulerThread::getEngine() const
{
    return _imp->engine;
}


void
OutputSchedulerThread::runCallbackWithVariables(const QString& callback)
{
    if (!callback.isEmpty()) {
        boost::shared_ptr<OutputEffectInstance> effect = _imp->outputEffect.lock();
        QString script = callback;
        std::string appID = effect->getApp()->getAppIDString();
        std::string nodeName = effect->getNode()->getFullyQualifiedName();
        std::string nodeFullName = appID + "." + nodeName;
        script.append(nodeFullName.c_str());
        script.append(",");
        script.append(appID.c_str());
        script.append(")\n");
        
        std::string err,output;
        if (!Python::interpretPythonScript(callback.toStdString(), &err, &output)) {
            effect->getApp()->appendToScriptEditor("Failed to run callback: " + err);
            throw std::runtime_error(err);
        } else if (!output.empty()) {
            effect->getApp()->appendToScriptEditor(output);
        }
    }
}


////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
//////////////////////// RenderThreadTask ////////////

struct RenderThreadTaskPrivate
{
    OutputSchedulerThread* scheduler;
    
    boost::weak_ptr<OutputEffectInstance> output;
    
#ifndef NATRON_PLAYBACK_USES_THREAD_POOL
    QMutex mustQuitMutex;
    bool mustQuit;
    bool hasQuit;
    
    QMutex runningMutex;
    bool running;
#else
    int time;
    bool useRenderStats;
    std::vector<int> viewsToRender;
#endif
    
    RenderThreadTaskPrivate(const boost::shared_ptr<OutputEffectInstance>& output,
                            OutputSchedulerThread* scheduler
                            #ifdef NATRON_PLAYBACK_USES_THREAD_POOL
                            ,
                            const int time,
                            const bool useRenderStats,
                            const std::vector<int>& viewsToRender
                            #endif
                            )
    : scheduler(scheduler)
    , output(output)
#ifndef NATRON_PLAYBACK_USES_THREAD_POOL
    , mustQuitMutex()
    , mustQuit(false)
    , hasQuit(false)
    , runningMutex()
    , running(false)
#else
    , time(time)
    , useRenderStats(useRenderStats)
    , viewsToRender(viewsToRender)
#endif
    {
        
    }
};


#ifndef NATRON_PLAYBACK_USES_THREAD_POOL
RenderThreadTask::RenderThreadTask(const boost::shared_ptr<OutputEffectInstance>& output,OutputSchedulerThread* scheduler)
: QThread()
, _imp(new RenderThreadTaskPrivate(output,scheduler))
{
    setObjectName("Parallel render thread");
}
#else
RenderThreadTask::RenderThreadTask(const boost::shared_ptr<OutputEffectInstance>& output,
                 OutputSchedulerThread* scheduler,
                 const int time,
                 const bool useRenderStats,
                 const std::vector<int>& viewsToRender)
: QRunnable()
, _imp(new RenderThreadTaskPrivate(output, scheduler, time, useRenderStats, viewsToRender))
{
    
}
#endif
RenderThreadTask::~RenderThreadTask()
{
    
}

void
RenderThreadTask::run()
{
    
#ifndef NATRON_PLAYBACK_USES_THREAD_POOL
    notifyIsRunning(true);
    
    for (;;) {
        
        bool enableRenderStats;
        std::vector<int> viewsToRender;
        int time = _imp->scheduler->pickFrameToRender(this,&enableRenderStats, &viewsToRender);
        
        if ( mustQuit() ) {
            break;
        }
        
        renderFrame(time, viewsToRender, enableRenderStats);
        
        appPTR->getAppTLS()->cleanupTLSForThread();
        
        if ( mustQuit() ) {
            break;
        }
    }
    
    {
        QMutexLocker l(&_imp->mustQuitMutex);
        _imp->hasQuit = true;
    }
    notifyIsRunning(false);
    _imp->scheduler->notifyThreadAboutToQuit(this);
#else // NATRON_PLAYBACK_USES_THREAD_POOL
    renderFrame(_imp->time, _imp->viewsToRender, _imp->useRenderStats);
    _imp->scheduler->notifyThreadAboutToQuit(this);
#endif

}

#ifndef NATRON_PLAYBACK_USES_THREAD_POOL
bool
RenderThreadTask::hasQuit() const
{
    
    QMutexLocker l(&_imp->mustQuitMutex);
    return _imp->hasQuit;
    
}

void
RenderThreadTask::scheduleForRemoval()
{
    QMutexLocker l(&_imp->mustQuitMutex);
    assert(!_imp->mustQuit);
    _imp->mustQuit = true;
}

bool
RenderThreadTask::mustQuit() const
{
    QMutexLocker l(&_imp->mustQuitMutex);
    return _imp->mustQuit;
}

void
RenderThreadTask::notifyIsRunning(bool running)
{
    {
        QMutexLocker l(&_imp->runningMutex);
        if (_imp->running == running) {
            return;
        }
        _imp->running = running;
    }
    
    appPTR->fetchAndAddNRunningThreads(running ? 1 : - 1);
}
#endif

////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
//////////////////////// DefaultScheduler ////////////


DefaultScheduler::DefaultScheduler(RenderEngine* engine,const boost::shared_ptr<OutputEffectInstance>& effect)
: OutputSchedulerThread(engine,effect,eProcessFrameBySchedulerThread)
, _effect(effect)
{
    engine->setPlaybackMode(ePlaybackModeOnce);
}

DefaultScheduler::~DefaultScheduler()
{
    
}

class DefaultRenderFrameRunnable : public RenderThreadTask
{
    
public:
    
    
#ifndef NATRON_PLAYBACK_USES_THREAD_POOL
    DefaultRenderFrameRunnable(const boost::shared_ptr<OutputEffectInstance>& writer,OutputSchedulerThread* scheduler)
    : RenderThreadTask(writer,scheduler)
    {
        
    }
#else
    DefaultRenderFrameRunnable(const boost::shared_ptr<OutputEffectInstance>& writer,
                               OutputSchedulerThread* scheduler,
                               const int time,
                               const bool useRenderStats,
                               const std::vector<int>& viewsToRender)
    : RenderThreadTask(writer,scheduler, time, useRenderStats, viewsToRender)
    {
        
    }
#endif
   
    
    virtual ~DefaultRenderFrameRunnable()
    {
        
    }
    
private:
    
    
    virtual void
    renderFrame(int time, const std::vector<int>& viewsToRender,  bool enableRenderStats) {
        
        boost::shared_ptr<OutputEffectInstance> output = _imp->output.lock();
        if (!output) {
            _imp->scheduler->notifyRenderFailure("");
            return;
        }
        
        SequentialPreferenceEnum sequentiallity = output->getSequentialPreference();
        
        /// If the writer dosn't need to render the frames in any sequential order (such as image sequences for instance), then
        /// we just render the frames directly in this thread, no need to use the scheduler thread for maximum efficiency.
        
        bool renderDirectly = sequentiallity == eSequentialPreferenceNotSequential;

        ///Even if enableRenderStats is false, we at least profile the time spent rendering the frame when rendering with a Write node.
        ///Though we don't enable render stats for sequential renders (e.g: WriteFFMPEG) since this is 1 file.
        RenderStatsPtr stats(new RenderStats(renderDirectly && enableRenderStats));
        
        NodePtr outputNode = output->getNode();
        
        std::string cb = outputNode->getBeforeFrameRenderCallback();
        if (!cb.empty()) {
            std::vector<std::string> args;
            std::string error;
            try {
                Python::getFunctionArguments(cb, &error, &args);
            } catch (const std::exception& e) {
                output->getApp()->appendToScriptEditor(std::string("Failed to run beforeFrameRendered callback: ")
                                                                 + e.what());
                return;
            }
            if (!error.empty()) {
                output->getApp()->appendToScriptEditor("Failed to run before frame render callback: " + error);
                return;
            }
            
            std::string signatureError;
            signatureError.append("The before frame render callback supports the following signature(s):\n");
            signatureError.append("- callback(frame, thisNode, app)");
            if (args.size() != 3) {
                output->getApp()->appendToScriptEditor("Failed to run before frame render callback: " + signatureError);
                return;
            }
            
            if (args[0] != "frame" || args[1] != "thisNode" || args[2] != "app" ) {
                output->getApp()->appendToScriptEditor("Failed to run before frame render callback: " + signatureError);
                return;
            }

            std::stringstream ss;
            std::string appStr = outputNode->getApp()->getAppIDString();
            std::string outputNodeName = appStr + "." + outputNode->getFullyQualifiedName();
            ss << cb << "(" << time << ", " << outputNodeName << ", " << appStr << ")";
            std::string script = ss.str();
            try {
                _imp->scheduler->runCallbackWithVariables(script.c_str());
            } catch (const std::exception &e) {
                _imp->scheduler->notifyRenderFailure(e.what());
                return;
            }

        }
        
        try {
            ////Writers always render at scale 1.
            int mipMapLevel = 0;
            RenderScale scale(1.);
            
            RectD rod;
            bool isProjectFormat;
            
            // Do not catch exceptions: if an exception occurs here it is probably fatal, since
            // it comes from Natron itself. All exceptions from plugins are already caught
            // by the HostSupport library.
            EffectInstPtr activeInputToRender;
            if (renderDirectly) {
                activeInputToRender = output;
            } else {
                activeInputToRender = output->getInput(0);
                if (activeInputToRender) {
                    activeInputToRender = activeInputToRender->getNearestNonDisabled();
                } else {
                    _imp->scheduler->notifyRenderFailure("No input to render");
                    return;
                }
                
            }
            
            assert(activeInputToRender);
            U64 activeInputToRenderHash = activeInputToRender->getHash();
            
            const double par = activeInputToRender->getPreferredAspectRatio();
            
            for (std::size_t view = 0; view < viewsToRender.size(); ++view) {
        
                StatusEnum stat = activeInputToRender->getRegionOfDefinition_public(activeInputToRenderHash,time, scale, viewsToRender[view], &rod, &isProjectFormat);
                if (stat == eStatusFailed) {
                    _imp->scheduler->notifyRenderFailure("Error caught while rendering");
                    return;
                }
                std::list<ImageComponents> components;
                ImageBitDepthEnum imageDepth;
                
                //Use needed components to figure out what we need to render
               EffectInstance::ComponentsNeededMap neededComps;
                bool processAll;
                SequenceTime ptTime;
                int ptView;
                std::bitset<4> processChannels;
                NodePtr ptInput;
                activeInputToRender->getComponentsNeededAndProduced_public(true, true, time, viewsToRender[view], &neededComps, &processAll, &ptTime, &ptView, &processChannels, &ptInput);

                
                //Retrieve bitdepth only
                activeInputToRender->getPreferredDepthAndComponents(-1, &components, &imageDepth);
                components.clear();
                
               EffectInstance::ComponentsNeededMap::iterator foundOutput = neededComps.find(-1);
                if (foundOutput != neededComps.end()) {
                    for (std::size_t j = 0; j < foundOutput->second.size(); ++j) {
                        components.push_back(foundOutput->second[j]);
                    }
                }
                RectI renderWindow;
                rod.toPixelEnclosing(scale, par, &renderWindow);
                
                FrameRequestMap request;
                stat = EffectInstance::computeRequestPass(time, viewsToRender[view], mipMapLevel, rod, outputNode, request);
                if (stat == eStatusFailed) {
                    _imp->scheduler->notifyRenderFailure("Error caught while rendering");
                    return;
                }
                
                ParallelRenderArgsSetter frameRenderArgs(time,
                                                         viewsToRender[view],
                                                         false,  // is this render due to user interaction ?
                                                         sequentiallity == eSequentialPreferenceOnlySequential || sequentiallity == eSequentialPreferencePreferSequential, // is this sequential ?
                                                         true, // canAbort ?
                                                         0, //renderAge
                                                         outputNode, // viewer requester
                                                         &request,
                                                         0, //texture index
                                                         output->getApp()->getTimeLine().get(),
                                                         NodePtr(),
                                                         false,
                                                         false,
                                                         false,
                                                         stats);
                
                RenderingFlagSetter flagIsRendering(activeInputToRender->getNode().get());
                
                ImageList planes;
                EffectInstance::RenderRoIRetCode retCode =
                activeInputToRender->renderRoI( EffectInstance::RenderRoIArgs(time, //< the time at which to render
                                                                              scale, //< the scale at which to render
                                                                              mipMapLevel, //< the mipmap level (redundant with the scale)
                                                                              viewsToRender[view], //< the view to render
                                                                              false,
                                                                              renderWindow, //< the region of interest (in pixel coordinates)
                                                                              rod, // < any precomputed rod ? in canonical coordinates
                                                                              components,
                                                                              imageDepth,
                                                                              false,
                                                                              output.get()),&planes);
                if (retCode != EffectInstance::eRenderRoIRetCodeOk) {
                    if (retCode == EffectInstance::eRenderRoIRetCodeAborted) {
                        _imp->scheduler->notifyRenderFailure("Render aborted");
                    } else {
                        _imp->scheduler->notifyRenderFailure("Error caught while rendering");
                    }
                    return;
                }
                
                ///If we need sequential rendering, pass the image to the output scheduler that will ensure the sequential ordering
                if (!renderDirectly) {
                    for (ImageList::iterator it = planes.begin(); it != planes.end(); ++it) {
                        _imp->scheduler->appendToBuffer(time, viewsToRender[view], stats, boost::dynamic_pointer_cast<BufferableObject>(*it));
                    }
                } else {
                    _imp->scheduler->notifyFrameRendered(time, viewsToRender[view], viewsToRender, stats, eSchedulingPolicyFFA);
                }
                
            }
            
        } catch (const std::exception& e) {
            _imp->scheduler->notifyRenderFailure(std::string("Error while rendering: ") + e.what());
        }
    }
};

#ifndef NATRON_PLAYBACK_USES_THREAD_POOL
RenderThreadTask*
DefaultScheduler::createRunnable()
{
    return new DefaultRenderFrameRunnable(_effect.lock(),this);
}
#else
RenderThreadTask*
DefaultScheduler::createRunnable(int frame, bool useRenderStarts, const std::vector<int>& viewsToRender)
{
    return new DefaultRenderFrameRunnable(_effect.lock(),this, frame, useRenderStarts, viewsToRender);
}
#endif



/**
 * @brief Called whenever there are images available to process in the buffer.
 * Once processed, the frame will be removed from the buffer.
 *
 * According to the ProcessFrameModeEnum given to the scheduler this function will be called either by the scheduler thread (this)
 * or by the application's main-thread (typically to do OpenGL rendering).
 **/
void
DefaultScheduler::processFrame(const BufferedFrames& frames)
{
    assert(!frames.empty());
    //Only consider the first frame, we shouldn't have multiple view here anyway.
    const BufferedFrame& frame = frames.front();
    
    ///Writers render to scale 1 always
    RenderScale scale(1.);
    
    boost::shared_ptr<OutputEffectInstance> effect = _effect.lock();
    
    U64 hash = effect->getHash();
    
    bool isProjectFormat;
    RectD rod;
    RectI roi;
    
    std::list<ImageComponents> components;
    ImageBitDepthEnum imageDepth;
    effect->getPreferredDepthAndComponents(-1, &components, &imageDepth);
    
    const double par = effect->getPreferredAspectRatio();
    
    SequentialPreferenceEnum sequentiallity = effect->getSequentialPreference();
    bool canOnlyHandleOneView = sequentiallity == eSequentialPreferenceOnlySequential || sequentiallity == eSequentialPreferencePreferSequential;
    
    for (BufferedFrames::const_iterator it = frames.begin(); it != frames.end(); ++it) {
        ignore_result(effect->getRegionOfDefinition_public(hash,it->time, scale, it->view, &rod, &isProjectFormat));
        rod.toPixelEnclosing(0, par, &roi);
        
        ParallelRenderArgsSetter frameRenderArgs(it->time,
                                                 it->view,
                                                 false,  // is this render due to user interaction ?
                                                 canOnlyHandleOneView, // is this sequential ?
                                                 true, //canAbort
                                                 0, //renderAge
                                                 effect->getNode(), //tree root
                                                 0,
                                                 0, //texture index
                                                 effect->getApp()->getTimeLine().get(),
                                                 NodePtr(),
                                                 false,
                                                 false,
                                                 false,
                                                 it->stats);
        
        RenderingFlagSetter flagIsRendering(effect->getNode().get());
        
        ImagePtr inputImage = boost::dynamic_pointer_cast<Image>(it->frame);
        assert(inputImage);
        
       EffectInstance::InputImagesMap inputImages;
        inputImages[0].push_back(inputImage);
        EffectInstance::RenderRoIArgs args(frame.time,
                                                   scale,0,
                                                   it->view,
                                                   true, // for writers, always by-pass cache for the write node only @see renderRoiInternal
                                                   roi,
                                                   rod,
                                                   components,
                                                   imageDepth,
                                                   false,
                                                   effect.get(),
                                                   inputImages);
        try {
            ImageList planes;
            EffectInstance::RenderRoIRetCode retCode = effect->renderRoI(args,&planes);
            if (retCode != EffectInstance::eRenderRoIRetCodeOk) {
                notifyRenderFailure("");
            }
        } catch (const std::exception& e) {
            notifyRenderFailure(e.what());
        }

    }
    
}

void
DefaultScheduler::timelineStepOne(OutputSchedulerThread::RenderDirectionEnum direction)
{
    if (direction == OutputSchedulerThread::eRenderDirectionForward) {
        _effect.lock()->incrementCurrentFrame();
    } else {
        _effect.lock()->decrementCurrentFrame();
    }
}

void
DefaultScheduler::timelineGoTo(int time)
{
    _effect.lock()->setCurrentFrame(time);
}

int
DefaultScheduler::timelineGetTime() const
{
    return _effect.lock()->getCurrentFrame();
}

void
DefaultScheduler::getFrameRangeToRender(int& first,int& last) const
{
    
    boost::shared_ptr<OutputEffectInstance> effect = _effect.lock();
    first = effect->getFirstFrame();
    last = effect->getLastFrame();
}


void
DefaultScheduler::handleRenderFailure(const std::string& errorMessage)
{
    std::cout << errorMessage << std::endl;
}

SchedulingPolicyEnum
DefaultScheduler::getSchedulingPolicy() const
{
    SequentialPreferenceEnum sequentiallity = _effect.lock()->getSequentialPreference();
    if (sequentiallity == eSequentialPreferenceNotSequential) {
        return eSchedulingPolicyFFA;
    } else {
        return eSchedulingPolicyOrdered;
    }
}


void
DefaultScheduler::aboutToStartRender()
{
    int first,last;
    getFrameRangeRequestedToRender(first, last);
    
    boost::shared_ptr<OutputEffectInstance> effect = _effect.lock();
    
    effect->setFirstFrame(first);
    effect->setLastFrame(last);
    
    if (getDirectionRequestedToRender() == eRenderDirectionForward) {
        effect->setCurrentFrame(first);
    } else {
        effect->setCurrentFrame(last);
    }
    
    bool isBackGround = appPTR->isBackground();
    
    if (!isBackGround) {
        effect->setKnobsFrozen(true);
    } else {
        appPTR->writeToOutputPipe(kRenderingStartedLong, kRenderingStartedShort);
    }
    
    std::string cb = effect->getNode()->getBeforeRenderCallback();
    if (!cb.empty()) {
        std::vector<std::string> args;
        std::string error;
        try {
            Python::getFunctionArguments(cb, &error, &args);
        } catch (const std::exception& e) {
            effect->getApp()->appendToScriptEditor(std::string("Failed to run beforeRender callback: ")
                                                             + e.what());
            return;
        }
        if (!error.empty()) {
            effect->getApp()->appendToScriptEditor("Failed to run beforeRender callback: " + error);
            return;
        }
        
        std::string signatureError;
        signatureError.append("The beforeRender callback supports the following signature(s):\n");
        signatureError.append("- callback(thisNode, app)");
        if (args.size() != 2) {
            effect->getApp()->appendToScriptEditor("Failed to run beforeRender callback: " + signatureError);
            return;
        }
        
        if (args[0] != "thisNode" || args[1] != "app" ) {
            effect->getApp()->appendToScriptEditor("Failed to run beforeRender callback: " + signatureError);
            return;
        }
        
        
        std::stringstream ss;
        std::string appStr = effect->getApp()->getAppIDString();
        std::string outputNodeName = appStr + "." + effect->getNode()->getFullyQualifiedName();
        ss << cb << "(" << outputNodeName << ", " << appStr << ")";
        std::string script = ss.str();
        try {
            runCallbackWithVariables(script.c_str());
        } catch (const std::exception &e) {
            notifyRenderFailure(e.what());
        }
    }
}

void
DefaultScheduler::onRenderStopped(bool aborted)
{
    
    boost::shared_ptr<OutputEffectInstance> effect = _effect.lock();
    
    bool isBackGround = appPTR->isBackground();
    if (!isBackGround) {
        effect->setKnobsFrozen(false);
    }
     effect->notifyRenderFinished();
    
    std::string cb = effect->getNode()->getAfterRenderCallback();
    if (!cb.empty()) {
        
        std::vector<std::string> args;
        std::string error;
        try {
            Python::getFunctionArguments(cb, &error, &args);
        } catch (const std::exception& e) {
            effect->getApp()->appendToScriptEditor(std::string("Failed to run afterRender callback: ")
                                                             + e.what());
            return;
        }
        if (!error.empty()) {
            effect->getApp()->appendToScriptEditor("Failed to run afterRender callback: " + error);
            return;
        }
        
        std::string signatureError;
        signatureError.append("The after render callback supports the following signature(s):\n");
        signatureError.append("- callback(aborted, thisNode, app)");
        if (args.size() != 3) {
            effect->getApp()->appendToScriptEditor("Failed to run afterRender callback: " + signatureError);
            return;
        }
        
        if (args[0] != "aborted" || args[1] != "thisNode" || args[2] != "app" ) {
            effect->getApp()->appendToScriptEditor("Failed to run afterRender callback: " + signatureError);
            return;
        }

        
        std::stringstream ss;
        std::string appStr = effect->getApp()->getAppIDString();
        std::string outputNodeName = appStr + "." + effect->getNode()->getFullyQualifiedName();
        ss << cb << "(";
        if (aborted) {
            ss << "True, ";
        } else {
            ss << "False, ";
        }
        ss << outputNodeName << ", " << appStr << ")";
        std::string script = ss.str();
        try {
            runCallbackWithVariables(script.c_str());
        } catch (...) {
            //Ignore expcetions in callback since the render is finished anyway
        }
    }

}

////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
//////////////////////// ViewerDisplayScheduler ////////////


ViewerDisplayScheduler::ViewerDisplayScheduler(RenderEngine* engine, const boost::shared_ptr<ViewerInstance>& viewer)
: OutputSchedulerThread(engine,viewer,eProcessFrameByMainThread) //< OpenGL rendering is done on the main-thread
, _viewer(viewer)
{
    
}

ViewerDisplayScheduler::~ViewerDisplayScheduler()
{
    
}


/**
 * @brief Called whenever there are images available to process in the buffer.
 * Once processed, the frame will be removed from the buffer.
 *
 * According to the ProcessFrameModeEnum given to the scheduler this function will be called either by the scheduler thread (this)
 * or by the application's main-thread (typically to do OpenGL rendering).
 **/
void
ViewerDisplayScheduler::processFrame(const BufferedFrames& frames)
{

    boost::shared_ptr<ViewerInstance> viewer = _viewer.lock();
    if (!frames.empty()) {
        for (BufferedFrames::const_iterator it = frames.begin(); it != frames.end(); ++it) {
            boost::shared_ptr<UpdateViewerParams> params = boost::dynamic_pointer_cast<UpdateViewerParams>(it->frame);
            assert(params);
            viewer->updateViewer(params);
        }
        viewer->redrawViewerNow();
    } else {
        viewer->redrawViewer();
    }
    
}

void
ViewerDisplayScheduler::timelineStepOne(OutputSchedulerThread::RenderDirectionEnum direction)
{
    boost::shared_ptr<ViewerInstance> viewer = _viewer.lock();
    if (direction == OutputSchedulerThread::eRenderDirectionForward) {
        viewer->getTimeline()->incrementCurrentFrame();
    } else {
        viewer->getTimeline()->decrementCurrentFrame();
    }
}

void
ViewerDisplayScheduler::timelineGoTo(int time)
{
    boost::shared_ptr<ViewerInstance> viewer = _viewer.lock();
    viewer->getTimeline()->seekFrame(time, false, 0, eTimelineChangeReasonPlaybackSeek);
}

int
ViewerDisplayScheduler::timelineGetTime() const
{
    return _viewer.lock()->getTimeline()->currentFrame();
}

void
ViewerDisplayScheduler::getFrameRangeToRender(int &first, int &last) const
{
    boost::shared_ptr<ViewerInstance> viewer = _viewer.lock();
    ViewerInstance* leadViewer = viewer->getApp()->getLastViewerUsingTimeline();
    ViewerInstance* v = leadViewer ? leadViewer : viewer.get();
    assert(v);
    v->getTimelineBounds(&first, &last);
}


class ViewerRenderFrameRunnable : public RenderThreadTask
{
  
    boost::weak_ptr<ViewerInstance> _viewer;
    
public:
    
#ifndef NATRON_PLAYBACK_USES_THREAD_POOL
    ViewerRenderFrameRunnable(const boost::shared_ptr<ViewerInstance>& viewer,OutputSchedulerThread* scheduler)
    : RenderThreadTask(viewer,scheduler)
    , _viewer(viewer)
    {
        
    }
#else
    ViewerRenderFrameRunnable(const boost::shared_ptr<ViewerInstance>& viewer,
                              OutputSchedulerThread* scheduler,
                              const int frame,
                              const bool useRenderStarts,
                              const std::vector<int>& viewsToRender)
    : RenderThreadTask(viewer,scheduler, frame, useRenderStarts, viewsToRender)
    , _viewer(viewer)
    {
        
    }
#endif
    
    
    virtual ~ViewerRenderFrameRunnable()
    {
        
    }
    
private:
    
    virtual void
    renderFrame(int time, const std::vector<int>& viewsToRender, bool enableRenderStats) {
        
        
        RenderStatsPtr stats;
        if (enableRenderStats) {
            stats.reset(new RenderStats(enableRenderStats));
        }
        ///The viewer always uses the scheduler thread to regulate the output rate, @see ViewerInstance::renderViewer_internal
        ///it calls appendToBuffer by itself
        ViewerInstance::ViewerRenderRetCode stat = ViewerInstance::eViewerRenderRetCodeRedraw;
        
        //Viewer can only render 1 view for now
        assert(viewsToRender.size() == 1);
        int view = viewsToRender.front();
        
        boost::shared_ptr<ViewerInstance> viewer = _viewer.lock();
        
        U64 viewerHash = viewer->getHash();
        boost::shared_ptr<ViewerArgs> args[2];
        
        ViewerInstance::ViewerRenderRetCode status[2] = {
            ViewerInstance::eViewerRenderRetCodeFail, ViewerInstance::eViewerRenderRetCodeFail
        };
        
        bool clearTexture[2] = { false, false };
        
        for (int i = 0; i < 2; ++i) {
            args[i].reset(new ViewerArgs);
            status[i] = viewer->getRenderViewerArgsAndCheckCache_public(time, true, true, view, i, viewerHash, NodePtr(), true, stats, args[i].get());
            clearTexture[i] = status[i] == ViewerInstance::eViewerRenderRetCodeFail || status[i] == ViewerInstance::eViewerRenderRetCodeBlack;
            if (clearTexture[i]) {
                //Just clear the viewer, nothing to do
                args[i]->params.reset();
            }
        }
       
        if (clearTexture[0] && clearTexture[1]) {
            _imp->scheduler->notifyRenderFailure(std::string());
            return;
        } else if (clearTexture[0] && !clearTexture[1]) {
            viewer->disconnectTexture(0);
        } else if (!clearTexture[0] && clearTexture[1]) {
            viewer->disconnectTexture(1);
        }

        if (status[0] == ViewerInstance::eViewerRenderRetCodeFail && status[1] == ViewerInstance::eViewerRenderRetCodeFail) {
            
            return;
        } else if (status[0] == ViewerInstance::eViewerRenderRetCodeRedraw || status[1] == ViewerInstance::eViewerRenderRetCodeRedraw) {
            return;
        } else {
            BufferableObjectList toAppend;
            for (int i = 0; i < 2; ++i) {
                if (args[i] && args[i]->params && args[i]->params->ramBuffer) {
                    toAppend.push_back(args[i]->params);
                    args[i].reset();
                }
            }
            _imp->scheduler->appendToBuffer(time, view, stats, toAppend);
        }
        
        
        if ((args[0] && status[0] != ViewerInstance::eViewerRenderRetCodeFail) || (args[1] && status[1] != ViewerInstance::eViewerRenderRetCodeFail)) {
            try {
                stat = viewer->renderViewer(view,false,true,viewerHash,true, NodePtr(), true,  args, boost::shared_ptr<RequestedFrame>(), stats);
            } catch (...) {
                stat = ViewerInstance::eViewerRenderRetCodeFail;
            }
        } else {
            return;
        }
        
        if (stat == ViewerInstance::eViewerRenderRetCodeFail) {
            ///Don't report any error message otherwise we will flood the viewer with irrelevant messages such as
            ///"Render failed", instead we let the plug-in that failed post an error message which will be more helpful.
            _imp->scheduler->notifyRenderFailure(std::string());
        } else {
            BufferableObjectList toAppend;
            for (int i = 0; i < 2; ++i) {
                if (args[i] && args[i]->params && args[i]->params->ramBuffer) {
                    toAppend.push_back(args[i]->params);
                }
            }
     
            _imp->scheduler->appendToBuffer(time, view, stats, toAppend);
            
        }

    }
};


#ifndef NATRON_PLAYBACK_USES_THREAD_POOL
RenderThreadTask*
ViewerDisplayScheduler::createRunnable()
{
    return new ViewerRenderFrameRunnable(_viewer.lock(),this);
}
#else
RenderThreadTask*
ViewerDisplayScheduler::createRunnable(int frame, bool useRenderStarts, const std::vector<int>& viewsToRender)
{
    return new ViewerRenderFrameRunnable(_viewer.lock(),this, frame, useRenderStarts, viewsToRender);
}
#endif


void
ViewerDisplayScheduler::handleRenderFailure(const std::string& /*errorMessage*/)
{
    _viewer.lock()->disconnectViewer();
}

void
ViewerDisplayScheduler::onRenderStopped(bool /*/aborted*/)
{
    ///Refresh all previews in the tree
    boost::shared_ptr<ViewerInstance> viewer = _viewer.lock();
    viewer->getNode()->refreshPreviewsRecursivelyUpstream(viewer->getTimeline()->currentFrame());
    
    if (!viewer->getApp() || viewer->getApp()->isGuiFrozen()) {
        getEngine()->s_refreshAllKnobs();
    }
}

int
ViewerDisplayScheduler::getLastRenderedTime() const
{
    return _viewer.lock()->getLastRenderedTime();
}


////////////////////////// RenderEngine

struct RenderEnginePrivate
{
    QMutex schedulerCreationLock;
    OutputSchedulerThread* scheduler;
    
    boost::weak_ptr<OutputEffectInstance> output;
    
    mutable QMutex pbModeMutex;
    PlaybackModeEnum pbMode;
    
    ViewerCurrentFrameRequestScheduler* currentFrameScheduler;
    
    struct RefreshRequest
    {
        bool enableStats;
        bool enableAbort;
    };
    
    /*
     This queue tracks all calls made to renderCurrentFrame() and attempts to concatenate the calls
     once the event loop fires the signal currentFrameRenderRequestPosted()
     This is only accessed on the main thread
     */
    std::list<RefreshRequest> refreshQueue;
    
    RenderEnginePrivate(const boost::shared_ptr<OutputEffectInstance>& output)
    : schedulerCreationLock()
    , scheduler(0)
    , output(output)
    , pbModeMutex()
    , pbMode(ePlaybackModeLoop)
    , currentFrameScheduler(0)
    , refreshQueue()
    {
        
    }
};

RenderEngine::RenderEngine(const boost::shared_ptr<OutputEffectInstance>& output)
: _imp(new RenderEnginePrivate(output))
{
    QObject::connect(this, SIGNAL(currentFrameRenderRequestPosted()), this, SLOT(onCurrentFrameRenderRequestPosted()), Qt::QueuedConnection);
}

RenderEngine::~RenderEngine()
{
    delete _imp->currentFrameScheduler;
    _imp->currentFrameScheduler = 0;
    delete _imp->scheduler;
    _imp->scheduler = 0;
}

OutputSchedulerThread*
RenderEngine::createScheduler(const boost::shared_ptr<OutputEffectInstance>& effect)
{
    return new DefaultScheduler(this,effect);
}

void
RenderEngine::renderFrameRange(bool isBlocking,
                               bool enableRenderStats,
                               int firstFrame,
                               int lastFrame,
                               int frameStep,
                               const std::vector<int>& viewsToRender,
                               OutputSchedulerThread::RenderDirectionEnum forward)
{
    {
        QMutexLocker k(&_imp->schedulerCreationLock);
        if (!_imp->scheduler) {
            _imp->scheduler = createScheduler(_imp->output.lock());
        }
    }
    
    _imp->scheduler->renderFrameRange(isBlocking, enableRenderStats,firstFrame, lastFrame, frameStep, viewsToRender, forward);
}

void
RenderEngine::renderFromCurrentFrame(bool enableRenderStats,const std::vector<int>& viewsToRender, OutputSchedulerThread::RenderDirectionEnum forward)
{
    
    {
        QMutexLocker k(&_imp->schedulerCreationLock);
        if (!_imp->scheduler) {
            _imp->scheduler = createScheduler(_imp->output.lock());
        }
    }
    
    _imp->scheduler->renderFromCurrentFrame(enableRenderStats, viewsToRender, forward);
}


void
RenderEngine::onCurrentFrameRenderRequestPosted()
{
    assert(QThread::currentThread() == qApp->thread());
    
    //Okay we are at the end of the event loop, concatenate all similar events
    RenderEnginePrivate::RefreshRequest r;
    bool rSet = false;
    while (!_imp->refreshQueue.empty()) {
        const RenderEnginePrivate::RefreshRequest& queueBegin = _imp->refreshQueue.front();
        if (!rSet) {
            rSet = true;
        } else {
            if (queueBegin.enableAbort == r.enableAbort && queueBegin.enableStats == r.enableStats) {
                _imp->refreshQueue.erase(_imp->refreshQueue.begin());
                continue;
            }
        }
        r = queueBegin;
        renderCurrentFrameInternal(r.enableStats, r.enableAbort);
        _imp->refreshQueue.erase(_imp->refreshQueue.begin());
    }
}

void
RenderEngine::renderCurrentFrameInternal(bool enableRenderStats,bool canAbort)
{
    assert(QThread::currentThread() == qApp->thread());
    
    ViewerInstance* isViewer = dynamic_cast<ViewerInstance*>(_imp->output.lock().get());
    if ( !isViewer ) {
        qDebug() << "RenderEngine::renderCurrentFrame for a writer is unsupported";
        return;
    }
    
    
    ///If the scheduler is already doing playback, continue it
    if ( _imp->scheduler ) {
        bool working = _imp->scheduler->isWorking();
        if (working) {
            _imp->scheduler->abortRendering(true,false);
        }
        if (working || _imp->scheduler->isPlaybackAutoRestartEnabled()) {
            _imp->scheduler->renderFromCurrentFrame(enableRenderStats, _imp->scheduler->getViewsRequestedToRender(),  _imp->scheduler->getDirectionRequestedToRender() );
            return;
        }
    }
    
    
    {
        QMutexLocker k(&_imp->schedulerCreationLock);
        if (!_imp->scheduler) {
            _imp->scheduler = createScheduler(_imp->output.lock());
        }
    }
    
    if (!_imp->currentFrameScheduler) {
        _imp->currentFrameScheduler = new ViewerCurrentFrameRequestScheduler(isViewer);
    }
    
    _imp->currentFrameScheduler->renderCurrentFrame(enableRenderStats,canAbort);

}

void
RenderEngine::renderCurrentFrame(bool enableRenderStats,bool canAbort)
{
    assert(QThread::currentThread() == qApp->thread());
    RenderEnginePrivate::RefreshRequest r;
    r.enableStats = enableRenderStats;
    r.enableAbort = canAbort;
    _imp->refreshQueue.push_back(r);
    Q_EMIT currentFrameRenderRequestPosted();
}



void
RenderEngine::quitEngine()
{
    if (_imp->scheduler) {
        _imp->scheduler->quitThread();
    }
    
    if (_imp->currentFrameScheduler) {
        _imp->currentFrameScheduler->quitThread();
    }
}

bool
RenderEngine::isSequentialRenderBeingAborted() const
{
    if (!_imp->scheduler) {
        return false;
    }
    return _imp->scheduler->isBeingAborted();
}

bool
RenderEngine::hasThreadsAlive() const
{

    bool schedulerRunning = false;
    if (_imp->scheduler) {
        schedulerRunning = _imp->scheduler->isRunning();
    }
    bool currentFrameSchedulerRunning = false;
    if (_imp->currentFrameScheduler) {
        currentFrameSchedulerRunning = _imp->currentFrameScheduler->isRunning();
    }
    
    return schedulerRunning || currentFrameSchedulerRunning;
}

bool
RenderEngine::hasThreadsWorking() const
{
 
    
    bool schedulerWorking = false;
    if (_imp->scheduler) {
        schedulerWorking = _imp->scheduler->isWorking();
    }
    bool currentFrameSchedulerWorking = false;
    if (_imp->currentFrameScheduler) {
        currentFrameSchedulerWorking = _imp->currentFrameScheduler->hasThreadsWorking();
    }
    
    return schedulerWorking || currentFrameSchedulerWorking;

}

bool
RenderEngine::isDoingSequentialRender() const
{
    return _imp->scheduler ? _imp->scheduler->isWorking() : false;
}

bool
RenderEngine::abortRendering(bool enableAutoRestartPlayback, bool blocking)
{
 
    if (_imp->currentFrameScheduler) {
        _imp->currentFrameScheduler->abortRendering(blocking);
    }

    if (_imp->scheduler && _imp->scheduler->isWorking()) {
        //If any playback active, abort it
        _imp->scheduler->abortRendering(enableAutoRestartPlayback, blocking);
        return true;
    }
    return false;
}

void
RenderEngine::setPlaybackMode(int mode)
{
    QMutexLocker l(&_imp->pbModeMutex);
    _imp->pbMode = (PlaybackModeEnum)mode;
}

PlaybackModeEnum
RenderEngine::getPlaybackMode() const
{
    QMutexLocker l(&_imp->pbModeMutex);
    return _imp->pbMode;
}

void
RenderEngine::setDesiredFPS(double d)
{
    {
        QMutexLocker k(&_imp->schedulerCreationLock);
        if (!_imp->scheduler) {
            _imp->scheduler = createScheduler(_imp->output.lock());
        }
    }
    _imp->scheduler->setDesiredFPS(d);
}

double
RenderEngine::getDesiredFPS() const
{
    
    return _imp->scheduler ? _imp->scheduler->getDesiredFPS() : 24;
}


void
RenderEngine::notifyFrameProduced(const BufferableObjectList& frames, const RenderStatsPtr& stats, const boost::shared_ptr<RequestedFrame>& request)
{
    _imp->currentFrameScheduler->notifyFrameProduced(frames, stats, request);
}

OutputSchedulerThread*
ViewerRenderEngine::createScheduler(const boost::shared_ptr<OutputEffectInstance>& effect) 
{
    return new ViewerDisplayScheduler(this,boost::dynamic_pointer_cast<ViewerInstance>(effect));
}

////////////////////////ViewerCurrentFrameRequestScheduler////////////////////////
struct CurrentFrameFunctorArgs
{
    int view;
    int time;
    RenderStatsPtr stats;
    ViewerInstance* viewer;
    U64 viewerHash;
    boost::shared_ptr<RequestedFrame> request;
    ViewerCurrentFrameRequestSchedulerPrivate* scheduler;
    bool canAbort;
    NodePtr isRotoPaintRequest;
    boost::shared_ptr<RotoStrokeItem> strokeItem;
    boost::shared_ptr<ViewerArgs> args[2];
    bool isRotoNeatRender;
    
    CurrentFrameFunctorArgs()
    : view(0)
    , time(0)
    , stats()
    , viewer(0)
    , viewerHash(0)
    , request()
    , scheduler(0)
    , canAbort(true)
    , isRotoPaintRequest()
    , strokeItem()
    , args()
    , isRotoNeatRender(false)
    {
    }
    
    CurrentFrameFunctorArgs(int view,
                            int time,
                            const RenderStatsPtr& stats,
                            ViewerInstance* viewer,
                            U64 viewerHash,
                            ViewerCurrentFrameRequestSchedulerPrivate* scheduler,
                            bool canAbort,
                            const NodePtr& isRotoPaintRequest,
                            const boost::shared_ptr<RotoStrokeItem>& strokeItem,
                            bool isRotoNeatRender)
    : view(view)
    , time(time)
    , stats(stats)
    , viewer(viewer)
    , viewerHash(viewerHash)
    , request()
    , scheduler(scheduler)
    , canAbort(canAbort)
    , isRotoPaintRequest(isRotoPaintRequest)
    , strokeItem(strokeItem)
    , args()
    , isRotoNeatRender(isRotoNeatRender)
    {
        if (isRotoPaintRequest && isRotoNeatRender) {
            isRotoPaintRequest->getRotoContext()->setIsDoingNeatRender(true);
        }
    }
    
    ~CurrentFrameFunctorArgs()
    {
        if (isRotoPaintRequest && isRotoNeatRender) {
            isRotoPaintRequest->getRotoContext()->setIsDoingNeatRender(false);
        }
    }
};

class RenderCurrentFrameFunctorRunnable;
struct ViewerCurrentFrameRequestSchedulerPrivate
{
    
    ViewerInstance* viewer;
    
    QThreadPool* threadPool;
    
    QMutex requestsQueueMutex;
    std::list<boost::shared_ptr<RequestedFrame> > requestsQueue;
    QWaitCondition requestsQueueNotEmpty;
    
    QMutex producedQueueMutex;
    std::list<ProducedFrame> producedQueue;
    QWaitCondition producedQueueNotEmpty;
    
    
    bool processRunning;
    QWaitCondition processCondition;
    QMutex processMutex;
    
    bool mustQuit;
    mutable QMutex mustQuitMutex;
    QWaitCondition mustQuitCond;
    
    int abortRequested;
    QMutex abortRequestedMutex;

    /**
     * Single thread used by the ViewerCurrentFrameRequestScheduler when the global thread pool has reached its maximum
     * activity to keep the renders responsive even if the thread pool is choking.
     **/
    ViewerCurrentFrameRequestRendererBackup backupThread;
    
    
    mutable QMutex currentFrameRenderTasksMutex;
    QWaitCondition currentFrameRenderTasksCond;
    std::list<RenderCurrentFrameFunctorRunnable*> currentFrameRenderTasks;
    
    ViewerCurrentFrameRequestSchedulerPrivate(ViewerInstance* viewer)
    : viewer(viewer)
    , threadPool(QThreadPool::globalInstance())
    , requestsQueueMutex()
    , requestsQueue()
    , requestsQueueNotEmpty()
    , producedQueueMutex()
    , producedQueue()
    , producedQueueNotEmpty()
    , processRunning(false)
    , processCondition()
    , processMutex()
    , mustQuit(false)
    , mustQuitMutex()
    , mustQuitCond()
    , abortRequested(0)
    , abortRequestedMutex()
    , backupThread()
    , currentFrameRenderTasksCond()
    , currentFrameRenderTasks()
    {
        
    }
    
    void appendRunnableTask(RenderCurrentFrameFunctorRunnable* task)
    {
        {
            QMutexLocker k(&currentFrameRenderTasksMutex);
            currentFrameRenderTasks.push_back(task);
        }
    }
    
    void removeRunnableTask(RenderCurrentFrameFunctorRunnable* task)
    {
        {
            QMutexLocker k(&currentFrameRenderTasksMutex);
            for (std::list<RenderCurrentFrameFunctorRunnable*>::iterator it = currentFrameRenderTasks.begin();
                 it != currentFrameRenderTasks.end(); ++it) {
                if (*it == task) {
                    currentFrameRenderTasks.erase(it);
                    
                    currentFrameRenderTasksCond.wakeAll();
                    break;
                }
            }
        }
    }
    
    void waitForRunnableTasks()
    {
        QMutexLocker k(&currentFrameRenderTasksMutex);
        while (!currentFrameRenderTasks.empty()) {
            currentFrameRenderTasksCond.wait(&currentFrameRenderTasksMutex);
        }
    }
    
    bool checkForExit()
    {
        QMutexLocker k(&mustQuitMutex);
        if (mustQuit) {
            mustQuit = false;
            mustQuitCond.wakeAll();
            return true;
        }
        return false;
    }
    
    bool checkForAbortion()
    {
        QMutexLocker k(&abortRequestedMutex);
        if (abortRequested > 0) {
            abortRequested = 0;
            return true;
        }
        return false;
    }
    
    void notifyFrameProduced(const BufferableObjectList& frames,const RenderStatsPtr& stats, const boost::shared_ptr<RequestedFrame>& request, bool processRequest)
    {
        QMutexLocker k(&producedQueueMutex);
        ProducedFrame p;
        p.frames = frames;
        p.request = request;
        p.isAborted = false;
        p.processRequest = processRequest;
        p.stats = stats;
        producedQueue.push_back(p);
        producedQueueNotEmpty.wakeOne();
    }
    
    void processProducedFrame(const RenderStatsPtr& stats, const BufferableObjectList& frames);

};

class RenderCurrentFrameFunctorRunnable : public QRunnable
{
    boost::shared_ptr<CurrentFrameFunctorArgs> _args;
    
public:
    
    RenderCurrentFrameFunctorRunnable(const boost::shared_ptr<CurrentFrameFunctorArgs>& args)
    : _args(args)
    {
        
    }
    
    virtual ~RenderCurrentFrameFunctorRunnable()
    {
        
    }
    
    virtual void run() OVERRIDE FINAL
    {
        ///The viewer always uses the scheduler thread to regulate the output rate, @see ViewerInstance::renderViewer_internal
        ///it calls appendToBuffer by itself
        ViewerInstance::ViewerRenderRetCode stat;
        
        BufferableObjectList ret;
        try {
            if (!_args->isRotoPaintRequest || _args->isRotoNeatRender) {
                stat = _args->viewer->renderViewer(_args->view,QThread::currentThread() == qApp->thread(),false,_args->viewerHash,_args->canAbort,
                                                  NodePtr(), true,_args->args, _args->request, _args->stats);
            } else {
                stat = _args->viewer->getViewerArgsAndRenderViewer(_args->time, _args->canAbort, _args->view, _args->viewerHash, _args->isRotoPaintRequest, _args->strokeItem, _args->stats,&_args->args[0],&_args->args[1]);
            }
        } catch (...) {
            stat = ViewerInstance::eViewerRenderRetCodeFail;
        }
        
        if (stat == ViewerInstance::eViewerRenderRetCodeFail) {
            ///Don't report any error message otherwise we will flood the viewer with irrelevant messages such as
            ///"Render failed", instead we let the plug-in that failed post an error message which will be more helpful.
            _args->viewer->disconnectViewer();
            ret.clear();
        } else {
            for (int i = 0; i < 2; ++i) {
                if (_args->args[i] && _args->args[i]->params && _args->args[i]->params->ramBuffer) {
                    ret.push_back(_args->args[i]->params);
                }
            }
        }
        
        if (_args->request) {
            _args->scheduler->notifyFrameProduced(ret, _args->stats, _args->request, true);
        } else {
            
            assert(QThread::currentThread() == qApp->thread());
            _args->scheduler->processProducedFrame(_args->stats, ret);
        }
        
        ///This thread is done, clean-up its TLS
        appPTR->getAppTLS()->cleanupTLSForThread();
        
        
        _args->scheduler->removeRunnableTask(this);
        

    }
};


ViewerCurrentFrameRequestScheduler::ViewerCurrentFrameRequestScheduler(ViewerInstance* viewer)
: QThread()
, _imp(new ViewerCurrentFrameRequestSchedulerPrivate(viewer))
{
    setObjectName("ViewerCurrentFrameRequestScheduler");
    QObject::connect(this, SIGNAL(s_processProducedFrameOnMainThread(RenderStatsPtr,BufferableObjectList)), this, SLOT(doProcessProducedFrameOnMainThread(RenderStatsPtr,BufferableObjectList)));
}

ViewerCurrentFrameRequestScheduler::~ViewerCurrentFrameRequestScheduler()
{
    
}


void
ViewerCurrentFrameRequestScheduler::run()
{
    boost::shared_ptr<RequestedFrame> firstRequest;
    for (;;) {
        
        if (_imp->checkForExit()) {
            return;
        }
        
        
        {
            QMutexLocker k(&_imp->requestsQueueMutex);
            if (!firstRequest && !_imp->requestsQueue.empty()) {
                firstRequest = _imp->requestsQueue.front();
                _imp->requestsQueue.pop_front();
            }
        }
        
        if (firstRequest) {
            
            ///Wait for the work to be done
            BufferableObjectList frames;
            RenderStatsPtr stats;
            bool frameAborted = false;
            {
                QMutexLocker k(&_imp->producedQueueMutex);
                
                std::list<ProducedFrame>::iterator found = _imp->producedQueue.end();
                for (std::list<ProducedFrame>::iterator it = _imp->producedQueue.begin(); it != _imp->producedQueue.end(); ++it) {
                    if (it->request == firstRequest) {
                        found = it;
                        break;
                    }
                }
                
                while (found == _imp->producedQueue.end()) {
					if (_imp->checkForExit()) {
						return;
					}
                    _imp->producedQueueNotEmpty.wait(&_imp->producedQueueMutex);
                    
                    for (std::list<ProducedFrame>::iterator it = _imp->producedQueue.begin(); it != _imp->producedQueue.end(); ++it) {
                        if (it->request == firstRequest) {
                            found = it;
                            break;
                        }
                    }
                }
                
                assert(found != _imp->producedQueue.end());
                frameAborted = found->isAborted;

                found->request.reset();
                if (found->processRequest) {
                    firstRequest.reset();
                }
                frames = found->frames;
                stats = found->stats;
                _imp->producedQueue.erase(found);
            } // QMutexLocker k(&_imp->producedQueueMutex);
           
            if (_imp->checkForExit()) {
                return;
            }
            
            if (!frameAborted) {
                _imp->viewer->setCurrentlyUpdatingOpenGLViewer(true);
                QMutexLocker processLocker(&_imp->processMutex);
                _imp->processRunning = true;
                Q_EMIT s_processProducedFrameOnMainThread(stats, frames);
                
                while (_imp->processRunning && !_imp->checkForAbortion()) {
                    _imp->processCondition.wait(&_imp->processMutex);
                }
                _imp->viewer->setCurrentlyUpdatingOpenGLViewer(false);
            }
            
        } // if (firstRequest) {
        
        
        {
            QMutexLocker k(&_imp->requestsQueueMutex);
            while (!firstRequest && _imp->requestsQueue.empty()) {
                _imp->requestsQueueNotEmpty.wait(&_imp->requestsQueueMutex);
            }
        }
        
        ///If we reach here, we've been woken up because there's work to do
        
    } // for(;;)
}

void
ViewerCurrentFrameRequestScheduler::doProcessProducedFrameOnMainThread(const RenderStatsPtr& stats, const BufferableObjectList& frames)
{
    _imp->processProducedFrame(stats, frames);
}

void
ViewerCurrentFrameRequestSchedulerPrivate::processProducedFrame(const RenderStatsPtr& stats, const BufferableObjectList& frames)
{
    assert(QThread::currentThread() == qApp->thread());
    
    //bool hasDoneSomething = false;
    for (BufferableObjectList::const_iterator it2 = frames.begin(); it2 != frames.end(); ++it2) {
        assert(*it2);
        boost::shared_ptr<UpdateViewerParams> params = boost::dynamic_pointer_cast<UpdateViewerParams>(*it2);
        assert(params);
        if (params && params->ramBuffer) {
            if (stats) {
                double timeSpent;
                std::map<NodePtr,NodeRenderStats > ret = stats->getStats(&timeSpent);
                viewer->reportStats(0, 0, timeSpent, ret);
            }

            viewer->updateViewer(params);
        }
    }
    
    
    ///At least redraw the viewer, we might be here when the user removed a node upstream of the viewer.
    viewer->redrawViewer();
    
    
    {
        QMutexLocker k(&processMutex);
        processRunning = false;
        processCondition.wakeOne();
    }
}

void
ViewerCurrentFrameRequestScheduler::abortRendering(bool blocking)
{
    //This will make all processing nodes that call the abort() function return true
    //This function marks all active renders of the viewer as aborted (except the oldest one)
    //and each node actually check if the render has been aborted in EffectInstance::Implementation::aborted()
    _imp->viewer->markAllOnGoingRendersAsAborted();
    
    if (!isRunning()) {
        return;
    }
    
    {
        QMutexLocker l2(&_imp->processMutex);
        _imp->processRunning = false;
        _imp->processCondition.wakeOne();
    }
    {
        //Clear any irrelevant render requests
        QMutexLocker k(&_imp->requestsQueueMutex);
        _imp->requestsQueue.clear();
    }
    {
        //mark all frames waiting to be displayed as aborted
        QMutexLocker k(&_imp->producedQueueMutex);
        for (std::list<ProducedFrame>::iterator it = _imp->producedQueue.begin(); it!=_imp->producedQueue.end(); ++it) {
            it->isAborted = true;
        }
    }
    {
        QMutexLocker k(&_imp->abortRequestedMutex);
        ++_imp->abortRequested;
    }
    
    if (blocking) {
        _imp->waitForRunnableTasks();
    }

}

void
ViewerCurrentFrameRequestScheduler::quitThread()
{
    if (!isRunning()) {
        return;
    }
    
    abortRendering(true);
    _imp->backupThread.quitThread();
    {
        QMutexLocker l2(&_imp->processMutex);
        _imp->processRunning = false;
        _imp->processCondition.wakeOne();
    }
    
    {
        QMutexLocker k(&_imp->mustQuitMutex);
        assert(!_imp->mustQuit);
        _imp->mustQuit = true;
        
        ///Push a fake request
        {
            QMutexLocker k(&_imp->requestsQueueMutex);
            _imp->requestsQueue.push_back(boost::shared_ptr<RequestedFrame>());
            _imp->notifyFrameProduced(BufferableObjectList(), RenderStatsPtr(), boost::shared_ptr<RequestedFrame>(), true);
            _imp->requestsQueueNotEmpty.wakeOne();
        }
        while (_imp->mustQuit) {
            _imp->mustQuitCond.wait(&_imp->mustQuitMutex);
        }
    }
    
    
    wait();
    
    ///Clear all queues
    {
        QMutexLocker k(&_imp->requestsQueueMutex);
        _imp->requestsQueue.clear();
    }
    {
        QMutexLocker k(&_imp->producedQueueMutex);
        _imp->producedQueue.clear();
    }
}

bool
ViewerCurrentFrameRequestScheduler::hasThreadsWorking() const
{
    QMutexLocker k(&_imp->requestsQueueMutex);
    return _imp->requestsQueue.size() > 0;
}

void
ViewerCurrentFrameRequestScheduler::notifyFrameProduced(const BufferableObjectList& frames, const RenderStatsPtr& stats, const boost::shared_ptr<RequestedFrame>& request)
{
    _imp->notifyFrameProduced(frames, stats,  request, false);
}

void
ViewerCurrentFrameRequestScheduler::renderCurrentFrame(bool enableRenderStats,bool canAbort)
{
    int frame = _imp->viewer->getTimeline()->currentFrame();
    int viewsCount = _imp->viewer->getRenderViewsCount();
    int view = viewsCount > 0 ? _imp->viewer->getViewerCurrentView() : 0;
    U64 viewerHash = _imp->viewer->getHash();
    
    ViewerInstance::ViewerRenderRetCode status[2] = {
        ViewerInstance::eViewerRenderRetCodeFail, ViewerInstance::eViewerRenderRetCodeFail
    };
    if (!_imp->viewer->getUiContext() || _imp->viewer->getApp()->isCreatingNode()) {
        return;
    }
    
    RenderStatsPtr stats;
    if (enableRenderStats) {
        stats.reset(new RenderStats(enableRenderStats));
    }
    
    NodePtr rotoPaintNode;
    boost::shared_ptr<RotoStrokeItem> curStroke;
    bool isDrawing;
    _imp->viewer->getApp()->getActiveRotoDrawingStroke(&rotoPaintNode, &curStroke,&isDrawing);
    
    bool rotoUse1Thread = false;
    bool isRotoNeatRender = false;
    if (!isDrawing) {
        isRotoNeatRender = rotoPaintNode ? rotoPaintNode->getRotoContext()->mustDoNeatRender() : false;
        if (rotoPaintNode && isRotoNeatRender) {
            rotoUse1Thread = true;
        } else {
            rotoPaintNode.reset();
            curStroke.reset();
        }
    } else {
        assert(rotoPaintNode);
        rotoUse1Thread = true;
    }
    
    boost::shared_ptr<ViewerArgs> args[2];
    if (!rotoPaintNode || isRotoNeatRender) {
        
        bool clearTexture[2] = {false, false};
        
        for (int i = 0; i < 2; ++i) {
            args[i].reset(new ViewerArgs);
            status[i] = _imp->viewer->getRenderViewerArgsAndCheckCache_public(frame, false, canAbort, view, i, viewerHash,rotoPaintNode, true, stats, args[i].get());
            
            clearTexture[i] = status[i] == ViewerInstance::eViewerRenderRetCodeFail || status[i] == ViewerInstance::eViewerRenderRetCodeBlack;
            if (clearTexture[i]) {
                //Just clear the viewer, nothing to do
                args[i]->params.reset();
            }
            
            if (status[i] == ViewerInstance::eViewerRenderRetCodeRedraw) {
                //We must redraw (re-render) don't hold a pointer to the cached frame
                args[i]->params->cachedFrame.reset();
            }
        }
        
        if (clearTexture[0] && clearTexture[1]) {
            _imp->viewer->disconnectViewer();
            return;
        } else if (clearTexture[0] && !clearTexture[1]) {
            _imp->viewer->disconnectTexture(0);
        } else if (!clearTexture[0] && clearTexture[1]) {
            _imp->viewer->disconnectTexture(1);
        }
        
        if (status[0] == ViewerInstance::eViewerRenderRetCodeRedraw && status[1] == ViewerInstance::eViewerRenderRetCodeRedraw) {
            _imp->viewer->redrawViewer();
            return;
        }
        
        for (int i = 0; i < 2 ; ++i) {
            if (args[i]->params && args[i]->params->ramBuffer) {
                /*
                 The texture was cached
                 */
                if (stats && i == 0) {
                    double timeSpent;
                    std::map<NodePtr,NodeRenderStats > statResults = stats->getStats(&timeSpent);
                    _imp->viewer->reportStats(frame, view, timeSpent, statResults);
                }
                _imp->viewer->updateViewer(args[i]->params);
                args[i].reset();
            }
        }
        if ((!args[0] && !args[1]) ||
            (!args[0] && status[0] == ViewerInstance::eViewerRenderRetCodeRender && args[1] && status[1] == ViewerInstance::eViewerRenderRetCodeFail) ||
            (!args[1] && status[1] == ViewerInstance::eViewerRenderRetCodeRender && args[0] && status[0] == ViewerInstance::eViewerRenderRetCodeFail)) {
            _imp->viewer->redrawViewer();
            return;
        }
    }
    boost::shared_ptr<CurrentFrameFunctorArgs> functorArgs(new CurrentFrameFunctorArgs(view,
                                                                                       frame,
                                                                                       stats,
                                                                                       _imp->viewer,
                                                                                       viewerHash,
                                                                                       _imp.get(),
                                                                                       canAbort,
                                                                                       rotoPaintNode,
                                                                                       curStroke,
                                                                                       isRotoNeatRender));
    functorArgs->args[0] = args[0];
    functorArgs->args[1] = args[1];
    
    if (appPTR->getCurrentSettings()->getNumberOfThreads() == -1) {
        RenderCurrentFrameFunctorRunnable task(functorArgs);
        task.run();
    } else {
        boost::shared_ptr<RequestedFrame> request(new RequestedFrame);
        request->id = 0;
        {
            QMutexLocker k(&_imp->requestsQueueMutex);
            _imp->requestsQueue.push_back(request);
            
            if (isRunning()) {
                _imp->requestsQueueNotEmpty.wakeOne();
            } else {
                start();
            }
        }
        functorArgs->request = request;
        
        /*
         * Let at least 1 free thread in the thread-pool to allow the renderer to use the thread pool if we use the thread-pool
         */
        int maxThreads = _imp->threadPool->maxThreadCount();
        
        //When painting, limit the number of threads to 1 to be sure strokes are painted in the right order
        if (rotoUse1Thread) {
            maxThreads = 1;
        }
        if (maxThreads == 1 || (_imp->threadPool->activeThreadCount() >= maxThreads - 1)) {
            _imp->backupThread.renderCurrentFrame(functorArgs);
        } else {
            RenderCurrentFrameFunctorRunnable* task = new RenderCurrentFrameFunctorRunnable(functorArgs);
            _imp->appendRunnableTask(task);
            _imp->threadPool->start(task);
        }
    }
    
}

struct ViewerCurrentFrameRequestRendererBackupPrivate
{
    QMutex requestsQueueMutex;
    std::list<boost::shared_ptr<CurrentFrameFunctorArgs> > requestsQueue;
    QWaitCondition requestsQueueNotEmpty;
    
    bool mustQuit;
    mutable QMutex mustQuitMutex;
    QWaitCondition mustQuitCond;
    
    ViewerCurrentFrameRequestRendererBackupPrivate()
    : requestsQueueMutex()
    , requestsQueue()
    , requestsQueueNotEmpty()
    , mustQuit(false)
    , mustQuitMutex()
    , mustQuitCond()
    {
        
    }
    
    bool checkForExit()
    {
        QMutexLocker k(&mustQuitMutex);
        if (mustQuit) {
            mustQuit = false;
            mustQuitCond.wakeOne();
            return true;
        }
        return false;
    }

};

ViewerCurrentFrameRequestRendererBackup::ViewerCurrentFrameRequestRendererBackup()
: QThread()
, _imp(new ViewerCurrentFrameRequestRendererBackupPrivate())
{
    setObjectName("ViewerCurrentFrameRequestRendererBackup");
}

ViewerCurrentFrameRequestRendererBackup::~ViewerCurrentFrameRequestRendererBackup()
{
    
}

void
ViewerCurrentFrameRequestRendererBackup::renderCurrentFrame(const boost::shared_ptr<CurrentFrameFunctorArgs>& args)
{
    {
        QMutexLocker k(&_imp->requestsQueueMutex);
        _imp->requestsQueue.push_back(args);
        
        if (isRunning()) {
                _imp->requestsQueueNotEmpty.wakeOne();
            } else {
                start();
            }
        }
}

void
ViewerCurrentFrameRequestRendererBackup::run()
{
    for (;;) {
        
        
        bool hasRequest = false;
        {
            boost::shared_ptr<CurrentFrameFunctorArgs> firstRequest;
            {
                QMutexLocker k(&_imp->requestsQueueMutex);
                if (!_imp->requestsQueue.empty()) {
                    hasRequest = true;
                    firstRequest = _imp->requestsQueue.front();
                    if (!firstRequest || !firstRequest->viewer) {
                        hasRequest = false;
                    }
                    _imp->requestsQueue.pop_front();
                }
            }
            
            if (_imp->checkForExit()) {
                return;
            }
            
            
            if (hasRequest) {
                RenderCurrentFrameFunctorRunnable task(firstRequest);
                task.run();
            }
        } // firstRequest
        
        {
            QMutexLocker k(&_imp->requestsQueueMutex);
            while (_imp->requestsQueue.empty()) {
                _imp->requestsQueueNotEmpty.wait(&_imp->requestsQueueMutex);
            }
        }
    }
}

void
ViewerCurrentFrameRequestRendererBackup::quitThread()
{
    if (!isRunning()) {
        return;
    }
    
    {
        QMutexLocker k(&_imp->mustQuitMutex);
        assert(!_imp->mustQuit);
        _imp->mustQuit = true;
        
        ///Push a fake request
        {
            QMutexLocker k(&_imp->requestsQueueMutex);
            _imp->requestsQueue.push_back(boost::shared_ptr<CurrentFrameFunctorArgs>());
            _imp->requestsQueueNotEmpty.wakeOne();
        }
        
        while (_imp->mustQuit) {
            _imp->mustQuitCond.wait(&_imp->mustQuitMutex);
        }
    }
    wait();
    //clear all queues
    {
        QMutexLocker k(&_imp->requestsQueueMutex);
        _imp->requestsQueue.clear();
    }
}

NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_OutputSchedulerThread.cpp"
