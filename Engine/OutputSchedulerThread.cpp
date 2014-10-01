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

#include "OutputSchedulerThread.h"

#include <iostream>
#include <set>
#include <list>
#include <QMetaType>
#include <QMutex>
#include <QWaitCondition>
#include <QCoreApplication>
#include <QString>


#include "Engine/AppManager.h"
#include "Engine/AppInstance.h"
#include "Engine/EffectInstance.h"
#include "Engine/Image.h"
#include "Engine/Node.h"
#include "Engine/OpenGLViewerI.h"
#include "Engine/Project.h"
#include "Engine/Settings.h"
#include "Engine/Timer.h"
#include "Engine/TimeLine.h"
#include "Engine/ViewerInstance.h"
#include "Engine/ViewerInstancePrivate.h"


#define NATRON_FPS_REFRESH_RATE_SECONDS 1.5


using namespace Natron;

struct BufferedFrame
{
    int view;
    double time;
    boost::shared_ptr<BufferableObject> frame;
};

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
                return false;
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
            qRegisterMetaType<boost::shared_ptr<BufferableObject> >("boost::shared_ptr<BufferableObject>");
        }
    };
}
static MetaTypesRegistration registration;


struct RunArgs
{
    ///The frame range that the scheduler should render
    int firstFrame,lastFrame;
    
    
    ///Hint to the scheduler as to how we should control the threads
    bool playbackOrRender;
    
    /// the timelineDirection represents the direction the timeline should move to
    OutputSchedulerThread::RenderDirection timelineDirection;

};

struct RenderThread {
    RenderThreadTask* thread;
    bool active;
};
typedef std::list<RenderThread> RenderThreads;



struct OutputSchedulerThreadPrivate
{
    
    FrameBuffer buf; //the frames rendered by the worker threads that needs to be rendered in order by the output device
    std::size_t bufferRAMOccupation; //the amount of RAM that the buffer keeps active
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
    
    int abortRequested; // true when the user wants to stop the engine, e.g: the user disconnected the viewer
    QWaitCondition abortedRequestedCondition;
    QMutex abortedRequestedMutex; // protects abortRequested

    
    QMutex abortBeingProcessedMutex; //protects abortBeingProcessed
    
    ///Basically when calling stopRender() we are resetting the abortRequested flag and putting the scheduler thread(this)
    ///asleep. We don't want that another thread attemps to post an abort request at the same time.
    bool abortBeingProcessed;
    
    bool treatRunning; //true when the scheduler is actively "treating" a frame (i.e: updating the viewer or writing in a file on disk)
    QWaitCondition treatCondition;
    QMutex treatMutex;

    //doesn't need any protection since it never changes and is set in the constructor
    OutputSchedulerThread::Mode mode; //is the frame to be treated on the main-thread (i.e OpenGL rendering) or on the scheduler thread

    
    boost::scoped_ptr<Timer> timer; // Timer regulating the engine execution. It is controlled by the GUI and MT-safe.
    
    
    ///The idea here is that the render() function will set the requestedRunArgs, and once the scheduler has finished
    ///the previous render it will copy them to the livingRunArgs to fullfil the new render request
    RunArgs requestedRunArgs,livingRunArgs;
    
    ///When the render threads are not using the appendToBuffer API, the scheduler has no way to know the rendering is finished
    ///but to count the number of frames rendered via notifyFrameRended which is called by the render thread.
    U64 nFramesRendered;
    bool renderFinished; //< set to true when nFramesRendered = livingRunArgs.lastFrame - livingRunArgs.firstFrame + 1
    
    QMutex runArgsMutex; // protects requestedRunArgs & livingRunArgs & nFramesRendered
    
    mutable QMutex pbModeMutex;
    Natron::PlaybackMode pbMode;
    
    ///Worker threads
    mutable QMutex renderThreadsMutex;
    RenderThreads renderThreads;
    QWaitCondition allRenderThreadsInactiveCond; // wait condition to make sure all render threads are asleep
    QWaitCondition allRenderThreadsQuitCond; //to make sure all render threads have quit
    
    ///Work queue filled by the scheduler thread
    QMutex framesToRenderMutex;
    std::list<int> framesToRender;
    
    ///index of the last frame pushed (framesToRender.back())
    ///we store this because when we call pushFramesToRender we need to know what was the last frame that was queued
    ///Protected by framesToRenderMutex
    int lastFramePushedIndex;
    
    ///Render threads wait in this condition and the scheduler wake them when it needs to render some frames
    QWaitCondition framesToRenderNotEmptyCond;
    

    
    Natron::OutputEffectInstance* outputEffect; //< The effect used as output device

    
    OutputSchedulerThreadPrivate(Natron::OutputEffectInstance* effect,OutputSchedulerThread::Mode mode)
    : buf()
    , bufferRAMOccupation(0)
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
    , abortedRequestedCondition()
    , abortedRequestedMutex()
    , abortBeingProcessedMutex()
    , abortBeingProcessed(false)
    , treatRunning(false)
    , treatCondition()
    , treatMutex()
    , mode(mode)
    , timer(new Timer)
    , requestedRunArgs()
    , livingRunArgs()
    , nFramesRendered(0)
    , renderFinished(false)
    , runArgsMutex()
    , pbModeMutex()
    , pbMode(PLAYBACK_LOOP)
    , renderThreadsMutex()
    , renderThreads()
    , allRenderThreadsInactiveCond()
    , allRenderThreadsQuitCond()
    , framesToRenderMutex()
    , framesToRender()
    , lastFramePushedIndex(0)
    , framesToRenderNotEmptyCond()
    , outputEffect(effect)
    {
       
    }
    
    void appendBufferedFrame(double time,int view,const boost::shared_ptr<BufferableObject>& image)
    {
        ///Private, shouldn't lock
        assert(!bufMutex.tryLock());
        
        BufferedFrame k;
        k.time = time;
        k.view = view;
        k.frame = image;
        if (image) {
            bufferRAMOccupation += image->sizeInRAM();
        }
        buf.insert(k);
    }
    
    void getFromBufferAndErase(double time,BufferedFrame& frame)
    {
        
        ///Private, shouldn't lock
        assert(!bufMutex.tryLock());
        
        for (FrameBuffer::iterator it = buf.begin(); it != buf.end(); ++it) {
            
            if (it->time == time) {
                frame = *it;
                if (frame.frame) {
                    std::size_t size = frame.frame->sizeInRAM();
                    
                    ///Avoid overflow: we might not fallback exactly to 0 because images have a dynamic size
                    ///but we don't need to update bufferRAMOccupation dynamically since it holds a global information
                    ///which is not a few bytes precise
                    if (size > bufferRAMOccupation) {
                        bufferRAMOccupation = 0;
                    } else {
                        bufferRAMOccupation -= size;
                    }
                }
                buf.erase(it);
                break;
            }
            
        }
    }
    
    void clearBuffer()
    {
        ///Private, shouldn't lock
        assert(!bufMutex.tryLock());
        
        buf.clear();
        bufferRAMOccupation = 0;
    }
    
    void appendRunnable(RenderThreadTask* runnable)
    {
        RenderThread r;
        r.thread = runnable;
        r.active = true;
        renderThreads.push_back(r);
        runnable->start();
        
    }
    
    RenderThreads::iterator getRunnableIterator(RenderThreadTask* runnable)
    {
        ///Private shouldn't lock
        assert(!renderThreadsMutex.tryLock());
        for (RenderThreads::iterator it = renderThreads.begin() ; it!=renderThreads.end();++it) {
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
    
    static bool getNextFrameInSequence(PlaybackMode pMode,OutputSchedulerThread::RenderDirection direction,int frame,
                                int firstFrame,int lastFrame,
                                int* nextFrame,OutputSchedulerThread::RenderDirection* newDirection);
    
    
    static void getNearestInSequence(OutputSchedulerThread::RenderDirection direction,int frame,
                                     int firstFrame,int lastFrame,
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
        while (renderThreads.size() > 0 && getNActiveRenderThreads() > 0) {
            allRenderThreadsInactiveCond.wait(&renderThreadsMutex);
        }
    }
    
    int getNActiveRenderThreads() const {
        ///Private shouldn't lock
        assert( !renderThreadsMutex.tryLock() );
        int ret = 0;
        for (RenderThreads::const_iterator it = renderThreads.begin() ; it!=renderThreads.end();++it) {
            if (it->active) {
                ++ret;
            }
        }
        return ret;
    }
    
    void removeQuitRenderThreadsInternal()
    {
        for (;;) {
            bool hasRemoved = false;
            for (RenderThreads::iterator it = renderThreads.begin() ; it!=renderThreads.end();++it) {
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
    
    void removeAllQuitRenderThreads() {
        ///Private shouldn't lock
        assert(!renderThreadsMutex.tryLock());
        
        removeQuitRenderThreadsInternal();
        
        ///Wake-up the main-thread if it was waiting for all threads to quit
        allRenderThreadsQuitCond.wakeOne();
    }
    
    void waitForRenderThreadsToQuit() {
        assert(!renderThreadsMutex.tryLock());
        while (renderThreads.size() > 0) {
            
            removeQuitRenderThreadsInternal();
        
            if (renderThreads.size() > 0) {
                allRenderThreadsQuitCond.wait(&renderThreadsMutex);
            }
        }
    }
    
};


OutputSchedulerThread::OutputSchedulerThread(Natron::OutputEffectInstance* effect,Mode mode)
: QThread()
, _imp(new OutputSchedulerThreadPrivate(effect,mode))
{
    QObject::connect(this, SIGNAL(s_doTreatOnMainThread(double,int,boost::shared_ptr<BufferableObject>)), this,
                     SLOT(doTreatFrameMainThread(double,int,boost::shared_ptr<BufferableObject>)));
    
    QObject::connect(_imp->timer.get(), SIGNAL(fpsChanged(double,double)), this, SIGNAL(fpsChanged(double,double)));
    
    QObject::connect(this, SIGNAL(s_abortRenderingOnMainThread(bool)), this, SLOT(abortRendering(bool)));
}

OutputSchedulerThread::~OutputSchedulerThread()
{
    ///Wake-up all threads and tell them that they must quit
    stopRenderThreads(0);
    
    QMutexLocker l(&_imp->renderThreadsMutex);

    ///Make sure they are all gone, there will be a deadlock here if that's not the case.
    _imp->waitForRenderThreadsToQuit();
}


bool
OutputSchedulerThreadPrivate::getNextFrameInSequence(PlaybackMode pMode,OutputSchedulerThread::RenderDirection direction,int frame,
                                                     int firstFrame,int lastFrame,
                                                     int* nextFrame,OutputSchedulerThread::RenderDirection* newDirection)
{
    *newDirection = direction;
    if (frame <= firstFrame) {
        switch (pMode) {
                case Natron::PLAYBACK_LOOP:
                if (direction == OutputSchedulerThread::RENDER_FORWARD) {
                    *nextFrame = firstFrame + 1;
                } else {
                    *nextFrame  = lastFrame - 1;
                }
                break;
                case Natron::PLAYBACK_BOUNCE:
                if (direction == OutputSchedulerThread::RENDER_FORWARD) {
                    *newDirection = OutputSchedulerThread::RENDER_BACKWARD;
                    *nextFrame  = lastFrame - 1;
                } else {
                    *newDirection = OutputSchedulerThread::RENDER_FORWARD;
                    *nextFrame  = firstFrame + 1;
                }
                break;
                case Natron::PLAYBACK_ONCE:
                default:
                if (direction == OutputSchedulerThread::RENDER_FORWARD) {
                    *nextFrame = firstFrame + 1;
                    break;
                } else {
                    return false;
                }
                
                
        }
    } else if (frame >= lastFrame) {
        switch (pMode) {
                case Natron::PLAYBACK_LOOP:
                if (direction == OutputSchedulerThread::RENDER_FORWARD) {
                    *nextFrame = firstFrame;
                } else {
                    *nextFrame = lastFrame - 1;
                }
                break;
                case Natron::PLAYBACK_BOUNCE:
                if (direction == OutputSchedulerThread::RENDER_FORWARD) {
                    *newDirection = OutputSchedulerThread::RENDER_BACKWARD;
                    *nextFrame = lastFrame - 1;
                } else {
                    *newDirection = OutputSchedulerThread::RENDER_FORWARD;
                    *nextFrame = firstFrame + 1;
                }
                break;
                case Natron::PLAYBACK_ONCE:
            default:
                if (direction == OutputSchedulerThread::RENDER_FORWARD) {
                    return false;
                } else {
                    *nextFrame = lastFrame - 1;
                    break;
                }

                
        }
    } else {
        if (direction == OutputSchedulerThread::RENDER_FORWARD) {
            *nextFrame = frame + 1;
            
        } else {
            *nextFrame = frame - 1;
        }
    }
    return true;

}

void
OutputSchedulerThreadPrivate::getNearestInSequence(OutputSchedulerThread::RenderDirection direction,int frame,
                          int firstFrame,int lastFrame,
                          int* nextFrame)
{
    if (frame >= firstFrame && frame <= lastFrame) {
        *nextFrame = frame;
    } else if (frame < firstFrame) {
        if (direction == OutputSchedulerThread::RENDER_FORWARD) {
            *nextFrame = firstFrame;
        } else {
            *nextFrame = lastFrame;
        }
    } else { // frame > lastFrame
        if (direction == OutputSchedulerThread::RENDER_FORWARD) {
            *nextFrame = lastFrame;
        } else {
            *nextFrame = firstFrame;
        }
    }
    
}

void
OutputSchedulerThread::pushFramesToRender(int startingFrame,int nThreads)
{
    {
        QMutexLocker l(&_imp->framesToRenderMutex);
        _imp->lastFramePushedIndex = startingFrame;
    }
    pushFramesToRenderInternal(startingFrame, nThreads);
}

void
OutputSchedulerThread::pushFramesToRenderInternal(int startingFrame,int nThreads)
{
    ///Make sure at least 1 frame is pushed
    if (nThreads <= 0) {
        nThreads = 1;
    }
    
    RenderDirection direction;
    int firstFrame,lastFrame;
    {
        QMutexLocker l(&_imp->runArgsMutex);
        direction = _imp->livingRunArgs.timelineDirection;
        firstFrame = _imp->livingRunArgs.firstFrame;
        lastFrame = _imp->livingRunArgs.lastFrame;
    }
    
    PlaybackMode pMode = getPlaybackMode();
    
    QMutexLocker l(&_imp->framesToRenderMutex);

    ///Push 2x the count of threads to be sure no one will be waiting
    while ((int)_imp->framesToRender.size() < nThreads * 2) {
        _imp->framesToRender.push_back(startingFrame);
        
        _imp->lastFramePushedIndex = startingFrame;
        
        if (!OutputSchedulerThreadPrivate::getNextFrameInSequence(pMode, direction, startingFrame,
                                                                  firstFrame, lastFrame, &startingFrame, &direction)) {
            break;
        }
    }
  
    ///Wake up render threads to notify them theres work to do
    _imp->framesToRenderNotEmptyCond.wakeAll();

}

void
OutputSchedulerThread::pushAllFrameRange()
{
    QMutexLocker l(&_imp->framesToRenderMutex);
    RenderDirection direction;
    int firstFrame,lastFrame;
    {
        QMutexLocker l(&_imp->runArgsMutex);
        direction = _imp->livingRunArgs.timelineDirection;
        firstFrame = _imp->livingRunArgs.firstFrame;
        lastFrame = _imp->livingRunArgs.lastFrame;
    }
    
    if (direction == RENDER_FORWARD) {
        for (int i = firstFrame; i <= lastFrame; ++i) {
            _imp->framesToRender.push_back(i);
        }
    } else {
        for (int i = lastFrame; i >= firstFrame; --i) {
            _imp->framesToRender.push_back(i);
        }
    }
    ///Wake up render threads to notify them theres work to do
    _imp->framesToRenderNotEmptyCond.wakeAll();
}

void
OutputSchedulerThread::pushFramesToRender(int nThreads)
{
    
    RenderDirection direction;
    int firstFrame,lastFrame;
    {
        QMutexLocker l(&_imp->runArgsMutex);
        direction = _imp->livingRunArgs.timelineDirection;
        firstFrame = _imp->livingRunArgs.firstFrame;
        lastFrame = _imp->livingRunArgs.lastFrame;
    }
    
    PlaybackMode pMode = getPlaybackMode();
    int frame;
    {
        QMutexLocker l(&_imp->framesToRenderMutex);
        frame = _imp->lastFramePushedIndex;

    }
    bool canContinue = true;
    
    ///If startingTime is already taken into account in the framesToRender, push new frames from the last one in the stack instead
    canContinue = OutputSchedulerThreadPrivate::getNextFrameInSequence(pMode, direction, frame,
                                                                        firstFrame, lastFrame, &frame, &direction);
    
    if (canContinue) {
        pushFramesToRenderInternal(frame, nThreads);
    }
    ///Wake up render threads to notify them theres work to do
    _imp->framesToRenderNotEmptyCond.wakeAll();
}

int
OutputSchedulerThread::pickFrameToRender(RenderThreadTask* thread)
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
    
    QMutexLocker l(&_imp->framesToRenderMutex);
    while ( _imp->framesToRender.empty() && !thread->mustQuit() ) {
        
        ///Notify that we're no longer doing work
        thread->notifyIsRunning(false);
        
        
        _imp->framesToRenderNotEmptyCond.wait(&_imp->framesToRenderMutex);
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
        
        return ret;
    } else {
        // thread is quitting, make sure we notified the application it is no longer running
        thread->notifyIsRunning(false);
        
    }
    return -1;
}

void
OutputSchedulerThread::notifyThreadAboutToQuit(RenderThreadTask* thread)
{
    QMutexLocker l(&_imp->renderThreadsMutex);
    RenderThreads::iterator found = _imp->getRunnableIterator(thread);
    if (found != _imp->renderThreads.end()) {
        found->active = false;
        _imp->allRenderThreadsInactiveCond.wakeOne();
        _imp->allRenderThreadsQuitCond.wakeOne();
    }
}

void
OutputSchedulerThread::startRender()
{
    
    if ( isFPSRegulationNeeded() ) {
        _imp->timer->playState = RUNNING;
    }
    
    bool playbackOrRender;
    RenderDirection direction;
    int startingFrame;
    {
        ///Copy the last requested run args
        
        QMutexLocker l(&_imp->runArgsMutex);
        _imp->livingRunArgs = _imp->requestedRunArgs;
        direction = _imp->livingRunArgs.timelineDirection;
        playbackOrRender = _imp->livingRunArgs.playbackOrRender;
        startingFrame = timelineGetTime();
    }
    
    
    aboutToStartRender();
    
    ///Flag that we're now doing work
    {
        QMutexLocker l(&_imp->workingMutex);
        _imp->working = true;
    }
    
    int nThreads;
    {
        QMutexLocker l(&_imp->renderThreadsMutex);
        _imp->removeAllQuitRenderThreads();
        nThreads = (int)_imp->renderThreads.size();
    }
    
    ///Start with one thread if it doesn't exist
    if (nThreads == 0) {
        _imp->appendRunnable(createRunnable(playbackOrRender));
        nThreads = 1;
    }
    
    QMutexLocker l(&_imp->renderThreadsMutex);
    
    if (playbackOrRender) {
        
        Natron::SchedulingPolicy policy = getSchedulingPolicy();
        
        if (policy == Natron::SCHEDULING_FFA) {
            
            
            ///push all frame range and let the threads deal with it
            pushAllFrameRange();
        } else {
            
            
            ///Push as many frames as there are threads
            pushFramesToRender(startingFrame,nThreads);
        }
        
    } else {
        
        
        ///Wake up only 1 thread since we want to render only 1 frame...
        QMutexLocker l(&_imp->framesToRenderMutex);
        _imp->lastFramePushedIndex = startingFrame;
        _imp->framesToRender.push_back(startingFrame);
        _imp->framesToRenderNotEmptyCond.wakeOne();
    }

}

void
OutputSchedulerThread::stopRender()
{
    _imp->timer->playState = PAUSE;
    
    ///Wait for all render threads to be done
    {
        QMutexLocker l(&_imp->renderThreadsMutex);
        
        _imp->removeAllQuitRenderThreads();
        _imp->waitForRenderThreadsToBeDone();
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
            _imp->outputEffect->getNode()->setAborted(false);
            _imp->abortedRequestedCondition.wakeAll();
        }
        
        ///Flag that we're no longer doing work
        {
            QMutexLocker l(&_imp->workingMutex);
            _imp->working = false;
        }
        
        ///Clear any frames that were processed ahead
        {
            QMutexLocker l2(&_imp->bufMutex);
            _imp->clearBuffer();
        }
        
        ///Notify everyone that the render is finished
        emit renderFinished(wasAborted ? 1 : 0);
        
        onRenderStopped();

        
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
                
                int expectedTimeToRender = timelineGetTime();
                
                BufferedFrame frameToRender;
                {
                    QMutexLocker l(&_imp->bufMutex);
                    _imp->getFromBufferAndErase(expectedTimeToRender, frameToRender);
                }
                
                ///The expected frame is not yet ready, go to sleep again
                if (!frameToRender.frame) {
                    break;
                }
                
                bool playbackOrRender;
                {
                    QMutexLocker l(&_imp->runArgsMutex);
                    playbackOrRender = _imp->livingRunArgs.playbackOrRender;
                }
                
                
                int nextFrameToRender = -1;
               
                if (!playbackOrRender) {
                    
                    renderFinished = true;
                    
                } else if (!renderFinished) {
                
                    ///////////
                    /////Refresh frame range if needed (for viewers)
                    

                    int firstFrame,lastFrame;
                    getFrameRangeToRender(firstFrame, lastFrame);
                    
                    
                    RenderDirection timelineDirection;
                    {
                        QMutexLocker l(&_imp->runArgsMutex);
                        
                        if ( _imp->livingRunArgs.playbackOrRender && isTimelineRangeSettable() && !isTimelineRangeSetByUser() ) {
                            
                            ///Refresh the firstframe/lastFrame as they might have changed on the timeline
                            _imp->livingRunArgs.firstFrame = firstFrame;
                            _imp->livingRunArgs.lastFrame = lastFrame;
                            
                            timelineSetBounds(firstFrame, lastFrame);
                        }
                        
                        
                        timelineDirection = _imp->livingRunArgs.timelineDirection;
                    }
                    
                    ///////////
                    ///Determine if we finished rendering or if we should just increment/decrement the timeline
                    ///or just loop/bounce
                    Natron::PlaybackMode pMode = getPlaybackMode();
                    RenderDirection newDirection;
                    renderFinished = !OutputSchedulerThreadPrivate::getNextFrameInSequence(pMode, timelineDirection,
                                                                                          expectedTimeToRender, firstFrame,
                                                                                          lastFrame, &nextFrameToRender, &newDirection);
                    if (newDirection != timelineDirection) {
                        QMutexLocker l(&_imp->runArgsMutex);
                        _imp->livingRunArgs.timelineDirection = newDirection;
                        _imp->requestedRunArgs.timelineDirection = newDirection;
                    }
                                        
                    if (!renderFinished) {
                        ///////////
                        /////If we were analysing the CPU activity, now set the appropriate number of threads to render.
                        int newNThreads;
                        adjustNumberOfThreads(&newNThreads);
                        
                        ///////////
                        /////Append render requests for the render threads
                        pushFramesToRender(newNThreads);
                    }
                }
                
                if (_imp->timer->playState == RUNNING) {
                    _imp->timer->waitUntilNextFrameIsDue(); // timer synchronizing with the requested fps
                }
                
                
                
                if (_imp->mode == TREAT_ON_SCHEDULER_THREAD) {
                    treatFrame(frameToRender.time, frameToRender.view, frameToRender.frame);
                } else {
                    ///Treat on main-thread
                                    
                    QMutexLocker treatLocker (&_imp->treatMutex);
                    
                    ///Check for abortion while under treatMutex to be sure the main thread is not deadlock in abortRendering
                    {
                        QMutexLocker locker(&_imp->abortedRequestedMutex);
                        if (_imp->abortRequested > 0) {
                            
                            ///Do not wait in the buf wait condition and go directly into the stopEngine()
                            renderFinished = true;
                            
                            break;
                        }
                    }
                    
                    _imp->treatRunning = true;
                    
                    emit s_doTreatOnMainThread(frameToRender.time, frameToRender.view, frameToRender.frame);
                                        
                    while (_imp->treatRunning) {
                        _imp->treatCondition.wait(&_imp->treatMutex);
                    }
                }
                
                ////////////
                /////At this point the frame has been treated by the output device
                
                
                notifyFrameRendered(expectedTimeToRender,SCHEDULING_ORDERED);
                
    
                if (!renderFinished && _imp->livingRunArgs.playbackOrRender) {
                    ///Timeline might have changed if another thread moved the playhead
                    int timelineCurrentTime = timelineGetTime();
                    if (timelineCurrentTime != expectedTimeToRender) {
                        timelineGoTo(timelineCurrentTime);
                    } else {
                        timelineGoTo(nextFrameToRender);
                    }
                    
                }
                
                ///////////
                /// End of the loop, refresh bufferEmpty
                {
                    QMutexLocker l(&_imp->bufMutex);
                    bufferEmpty = _imp->buf.empty();
                }
                
            } // while(!bufferEmpty)
            
            if (!renderFinished) {
                bool isAbortRequested;
                {
                    QMutexLocker abortRequestedLock (&_imp->abortedRequestedMutex);
                    isAbortRequested = _imp->abortRequested > 0;
                }
                if (!isAbortRequested) {
                    QMutexLocker bufLocker (&_imp->bufMutex);
                    ///Wait here for more frames to be rendered, we will be woken up once appendToBuffer(...) is called
                    _imp->bufCondition.wait(&_imp->bufMutex);
                } else {
                    break;
                }
            } else {
                break;
            }
        }
        
         stopRender();
        
    } // for(;;)
    
}

void
OutputSchedulerThread::adjustNumberOfThreads(int* newNThreads)
{
    ///////////
    /////If we were analysing the CPU activity, now set the appropriate number of threads to render.
    int optimalNThreads;
    
    int userSettingParallelThreads = appPTR->getCurrentSettings()->getNumberOfParallelRenders();
    
    int runningThreads = appPTR->getNRunningThreads();
    int currentParallelRenders = getNRenderThreads();
    
    if (userSettingParallelThreads == 0) {
        ///User wants it to be automatically computed, do a simple heuristic: launch as many parallel renders
        ///as there are cores
        optimalNThreads = appPTR->getHardwareIdealThreadCount();
    } else {
        optimalNThreads = userSettingParallelThreads;
    }
    
    
    
    if (runningThreads < optimalNThreads && currentParallelRenders < optimalNThreads) {
        
        bool playbackOrRender;
        {
            QMutexLocker l(&_imp->runArgsMutex);
            playbackOrRender = _imp->livingRunArgs.playbackOrRender;
        }
        
        ////////
        ///Launch 1 thread
        QMutexLocker l(&_imp->renderThreadsMutex);
        
        _imp->appendRunnable(createRunnable(playbackOrRender));
        *newNThreads = currentParallelRenders +  1;
        
    } else if (runningThreads > optimalNThreads && currentParallelRenders > 1) {
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

void
OutputSchedulerThread::notifyFrameRendered(int frame,Natron::SchedulingPolicy policy)
{
    emit frameRendered(frame);
    
    if (policy == SCHEDULING_FFA) {
        
        QMutexLocker l(&_imp->runArgsMutex);
        ++_imp->nFramesRendered;
        if ( _imp->nFramesRendered == (U64)(_imp->livingRunArgs.lastFrame - _imp->livingRunArgs.firstFrame + 1) ) {
            
            
            _imp->renderFinished = true;
            
            l.unlock();

            
            ///Notify the scheduler rendering is finished by append a fake frame to the buffer
            {
                QMutexLocker bufLocker (&_imp->bufMutex);
                _imp->appendBufferedFrame(0, 0, boost::shared_ptr<BufferableObject>());
                _imp->bufCondition.wakeOne();
            }
        } else {
            l.unlock();
            
            ///////////
            /////If we were analysing the CPU activity, now set the appropriate number of threads to render.
            int newNThreads;
            adjustNumberOfThreads(&newNThreads);

        }
    }
    if ( appPTR->isBackground() ) {
        QString frameStr = QString::number(frame);
        appPTR->writeToOutputPipe(kFrameRenderedStringLong + frameStr,kFrameRenderedStringShort + frameStr);
    }
    

}

void
OutputSchedulerThread::appendToBuffer(double time,int view,const boost::shared_ptr<BufferableObject>& image)
{
    if (QThread::currentThread() == qApp->thread()) {
        ///Single-threaded , call directly the function
        treatFrame(time, view, image);
    } else {
        
        ///Called by the scheduler thread when an image is rendered
        
        QMutexLocker l(&_imp->bufMutex);
        _imp->appendBufferedFrame(time, view, image);
        
        ///Wake up the scheduler thread that an image is available if it is asleep so it can treat it.
        _imp->bufCondition.wakeOne();
    }
}


void
OutputSchedulerThread::doTreatFrameMainThread(double time,int view,const boost::shared_ptr<BufferableObject>& frame)
{
    assert(QThread::currentThread() == qApp->thread());
    {
        QMutexLocker treatLocker (&_imp->treatMutex);
        ///The flag might have been reseted back by abortRendering()
        if (!_imp->treatRunning) {
            return;
        }
    }
    treatFrame(time, view, frame);
    QMutexLocker treatLocker (&_imp->treatMutex);
    _imp->treatRunning = false;
    _imp->treatCondition.wakeOne();
}

void
OutputSchedulerThread::abortRendering(bool blocking)
{
    
    if ( !isRunning() || !isWorking() ) {
        return;
    }


    bool isMainThread = QThread::currentThread() == qApp->thread();
    
    

    {
        ///Before posting an abort request, we must make sure the scheduler thread is not currently processing an abort request
        ///in stopRender(), we ensure the former by taking the abortBeingProcessedMutex lock
        QMutexLocker l(&_imp->abortedRequestedMutex);

        ///We make sure the render-thread doesn't wait for the main-thread to treat a frame
        ///This function (abortRendering) was probably called from a user event that was posted earlier in the
        ///event-loop, we just flag that the next event that will treat the frame should NOT treat it by
        ///reseting the treatRunning flag
        {
            QMutexLocker l2(&_imp->treatMutex);
            
            {
                QMutexLocker abortBeingProcessedLocker(&_imp->abortBeingProcessedMutex);
                
                ///We are already aborting but we don't want a blocking abort, it is useless to ask for a second abort
                if (!blocking && _imp->abortRequested > 0) {
                    return;
                }
                
                ///Flag the whole tree recursively that we aborted
                _imp->outputEffect->getNode()->setAborted(true);
                
                ++_imp->abortRequested;
            }
            
            ///Clear the work queue
            {
                QMutexLocker framesLocker (&_imp->framesToRenderMutex);
                _imp->framesToRender.clear();
            }
            
            
            if (isMainThread) {
                
                _imp->treatRunning = false;
                _imp->treatCondition.wakeOne();
            }
        }
        ///If the scheduler is asleep waiting for the buffer to be filling up, we post a fake request
        ///that will not be treated anyway because the first thing it does is checking for abort
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
    
    abortRendering(true);
    
    
    if (QThread::currentThread() == qApp->thread()) {
        ///If the scheduler thread was sleeping in the treat condition, waiting for the main-thread to finish
        ///treating the frame then waiting in the mustQuitCond would create a deadlock.
        ///Instead we discard the treating of the frame by taking the lock and setting treatRunning to false
        QMutexLocker treatLocker (&_imp->treatMutex);
        _imp->treatRunning = false;
        _imp->treatCondition.wakeOne();
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

void
OutputSchedulerThread::renderFrameRange(int firstFrame,int lastFrame,RenderDirection direction)
{
    if (direction == RENDER_FORWARD) {
        timelineGoTo(firstFrame);
    } else {
        timelineGoTo(lastFrame);
    }
    
    {
        
        QMutexLocker l(&_imp->runArgsMutex);
        _imp->requestedRunArgs.firstFrame = firstFrame;
        _imp->requestedRunArgs.lastFrame = lastFrame;
        _imp->requestedRunArgs.playbackOrRender = firstFrame != lastFrame;
        
        _imp->nFramesRendered = 0;
        _imp->renderFinished = false;
        
        ///Start with picking direction being the same as the timeline direction.
        ///Once the render threads are a few frames ahead the picking direction might be different than the
        ///timeline direction
        _imp->requestedRunArgs.timelineDirection = direction;

        
    }
    
    renderInternal();
    
}

void
OutputSchedulerThread::renderFromCurrentFrame(RenderDirection timelineDirection)
{
    

    {
        QMutexLocker l(&_imp->runArgsMutex);

        int firstFrame,lastFrame;
        if (isTimelineRangeSettable() && !isTimelineRangeSetByUser()) {
            getPluginFrameRange(firstFrame,lastFrame);
            timelineSetBounds(firstFrame, lastFrame);
        } else {
            getFrameRangeToRender(firstFrame, lastFrame);
        }
        
        
        ///Make sure current frame is in the frame range
        int currentTime = timelineGetTime();
        OutputSchedulerThreadPrivate::getNearestInSequence(timelineDirection, currentTime, firstFrame, lastFrame, &currentTime);
        
        _imp->requestedRunArgs.firstFrame = firstFrame;
        _imp->requestedRunArgs.lastFrame = lastFrame;
        _imp->requestedRunArgs.playbackOrRender = true;
        _imp->requestedRunArgs.timelineDirection = timelineDirection;
    }
    renderInternal();
}

void
OutputSchedulerThread::renderCurrentFrame(bool abortPrevious)
{
    
    
    
    ///Abort any ongoing render
    if (abortPrevious) {
        abortRendering(false);
    }
    
    ///If doing playback, let it run, just change the time
    bool doingPlayback = isDoingPlayback();
    if (doingPlayback) {
        renderFromCurrentFrame(getDirectionRequestedToRender());
        return;
    }

    
    int currentFrame = timelineGetTime();
    {
        QMutexLocker l(&_imp->runArgsMutex);
    
        if (isTimelineRangeSettable() && !isTimelineRangeSetByUser()) {
            int firstFrame,lastFrame;
            getPluginFrameRange(firstFrame,lastFrame);
            timelineSetBounds(firstFrame, lastFrame);
        }
        _imp->requestedRunArgs.firstFrame = currentFrame;
        _imp->requestedRunArgs.lastFrame = currentFrame;
        _imp->requestedRunArgs.playbackOrRender = false;
        _imp->requestedRunArgs.timelineDirection = RENDER_FORWARD;
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
            ++_imp->startRequests;
            _imp->startRequestsCond.wakeOne();
        }
    }
}

void
OutputSchedulerThread::notifyRenderFailure(const std::string& errorMessage)
{
    ///Abort all ongoing rendering
    doAbortRenderingOnMainThread(false);
    
    ///Handle failure: for viewers we make it black and don't display the error message which is irrelevant
    handleRenderFailure(errorMessage);
    
}

void
OutputSchedulerThread::setPlaybackMode(int mode)
{
    QMutexLocker l(&_imp->pbModeMutex);
    _imp->pbMode = (Natron::PlaybackMode)mode;
}

Natron::PlaybackMode
OutputSchedulerThread::getPlaybackMode() const
{
    QMutexLocker l(&_imp->pbModeMutex);
    return _imp->pbMode;
}

bool
OutputSchedulerThread::isWorking() const
{
    QMutexLocker l(&_imp->workingMutex);
    return _imp->working;
}

bool
OutputSchedulerThread::isDoingPlayback() const
{
    bool playback;
    {
        QMutexLocker l(&_imp->runArgsMutex);
        playback = _imp->livingRunArgs.playbackOrRender;
    }
    return playback && isWorking();
}

void
OutputSchedulerThread::getFrameRangeRequestedToRender(int &first,int& last) const
{
    first = _imp->livingRunArgs.firstFrame;
    last = _imp->livingRunArgs.lastFrame;
}

void
OutputSchedulerThread::getPluginFrameRange(int& first,int &last) const
{
    _imp->outputEffect->getFrameRange_public(_imp->outputEffect->getHash(), &first, &last);
    if (first == INT_MIN || last == INT_MAX) {
        getFrameRangeToRender(first, last);
    }
}

OutputSchedulerThread::RenderDirection
OutputSchedulerThread::getDirectionRequestedToRender() const
{
    QMutexLocker l(&_imp->runArgsMutex);
    return _imp->livingRunArgs.timelineDirection;
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
    


}
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
//////////////////////// RenderThreadTask ////////////

struct RenderThreadTaskPrivate
{
    bool playbackOrRender;
    OutputSchedulerThread* scheduler;
    
    Natron::OutputEffectInstance* output;
    
    QMutex mustQuitMutex;
    bool mustQuit;
    bool hasQuit;
    
    QMutex runningMutex;
    bool running;
    
    RenderThreadTaskPrivate(bool playbackOrRender,Natron::OutputEffectInstance* output,OutputSchedulerThread* scheduler)
    : playbackOrRender(playbackOrRender)
    , scheduler(scheduler)
    , output(output)
    , mustQuitMutex()
    , mustQuit(false)
    , hasQuit(false)
    , runningMutex()
    , running(false)
    {
        
    }
};


RenderThreadTask::RenderThreadTask(bool playbackOrRender,Natron::OutputEffectInstance* output,OutputSchedulerThread* scheduler)
: QThread()
, _imp(new RenderThreadTaskPrivate(playbackOrRender,output,scheduler))
{
    
}

RenderThreadTask::~RenderThreadTask()
{
    
}

void
RenderThreadTask::run()
{
    
    notifyIsRunning(true);
    
    for (;;) {
        
        int time = _imp->scheduler->pickFrameToRender(this);
        
        if ( mustQuit() ) {
            break;
        }
        
        renderFrame(time);
        
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

}

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

////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
//////////////////////// DefaultScheduler ////////////


DefaultScheduler::DefaultScheduler(Natron::OutputEffectInstance* effect)
: OutputSchedulerThread(effect,TREAT_ON_SCHEDULER_THREAD)
, _effect(effect)
{
    setPlaybackMode(PLAYBACK_ONCE);
}

DefaultScheduler::~DefaultScheduler()
{
    
}

class DefaultRenderFrameRunnable : public RenderThreadTask
{
    
public:
    
    DefaultRenderFrameRunnable(Natron::OutputEffectInstance* writer,OutputSchedulerThread* scheduler)
    : RenderThreadTask(true,writer,scheduler)
    {
        
    }
    
    virtual ~DefaultRenderFrameRunnable()
    {
        
    }
    
private:
    
    
    virtual void
    renderFrame(int time) {
        
        try {
            ////Writers always render at scale 1.
            int mipMapLevel = 0;
            RenderScale scale;
            scale.x = scale.y = 1.;
            
            RectD rod;
            bool isProjectFormat;
            int viewsCount = _imp->output->getApp()->getProject()->getProjectViewsCount();
            
            
            int mainView = 0;
            
            Natron::SequentialPreference sequentiallity = _imp->output->getSequentialPreference();
            
            ///The effect is sequential (e.g: WriteFFMPEG), and thus cannot render multiple views, we have to choose one
            ///We pick the user defined main view in the project settings
            
            bool canOnlyHandleOneView = sequentiallity == Natron::EFFECT_ONLY_SEQUENTIAL || sequentiallity == Natron::EFFECT_PREFER_SEQUENTIAL;
            if (canOnlyHandleOneView) {
                mainView = _imp->output->getApp()->getMainView();
            }
            
            ///The hash at which the writer will render
            U64 writerHash = _imp->output->getHash();
            
            /// If the writer dosn't need to render the frames in any sequential order (such as image sequences for instance), then
            /// we just render the frames directly in this thread, no need to use the scheduler thread for maximum efficiency.
        
            bool renderDirectly = sequentiallity == Natron::EFFECT_NOT_SEQUENTIAL;
            
            for (int i = 0; i < viewsCount; ++i) {
                if ( canOnlyHandleOneView && (i != mainView) ) {
                    ///@see the warning in EffectInstance::evaluate
                    continue;
                }
                
                // Do not catch exceptions: if an exception occurs here it is probably fatal, since
                // it comes from Natron itself. All exceptions from plugins are already caught
                // by the HostSupport library.
                EffectInstance* activeInputToRender;
                if (renderDirectly) {
                    activeInputToRender = _imp->output;
                } else {
                    activeInputToRender = _imp->output->getInput(0);
                    if (activeInputToRender) {
                        activeInputToRender = activeInputToRender->getNearestNonDisabled();
                    }
                    
                }
                U64 activeInputToRenderHash = activeInputToRender->getHash();
                
                Status stat = activeInputToRender->getRegionOfDefinition_public(activeInputToRenderHash,time, scale, i, &rod, &isProjectFormat);
                if (stat != StatFailed) {
                    ImageComponents components;
                    ImageBitDepth imageDepth;
                    activeInputToRender->getPreferredDepthAndComponents(-1, &components, &imageDepth);
                    RectI renderWindow;
                    rod.toPixelEnclosing(scale, &renderWindow);
                    
                    boost::shared_ptr<Natron::Image> img =
                    activeInputToRender->renderRoI( EffectInstance::RenderRoIArgs(time, //< the time at which to render
                                                                                  scale, //< the scale at which to render
                                                                                  mipMapLevel, //< the mipmap level (redundant with the scale)
                                                                                  i, //< the view to render
                                                                                  renderWindow, //< the region of interest (in pixel coordinates)
                                                                                  canOnlyHandleOneView, // is this sequential
                                                                                  false, // is this render due to user interaction ?
                                                                                  false, //< bypass cache ?
                                                                                  rod, // < any precomputed rod ? in canonical coordinates
                                                                                  components,
                                                                                  imageDepth),
                                                   &writerHash);
                    
                    ///If we need sequential rendering, pass the image to the output scheduler that will ensure the sequential ordering
                    if (!renderDirectly) {
                        _imp->scheduler->appendToBuffer(time, i, boost::dynamic_pointer_cast<BufferableObject>(img));
                    } else {
                        _imp->scheduler->notifyFrameRendered(time,SCHEDULING_FFA);
                    }
                    
                } else {
                    break;
                }
            }
            
        } catch (const std::exception& e) {
            _imp->scheduler->notifyRenderFailure(std::string("Error while rendering: ") + e.what());
        }
    }
};

RenderThreadTask*
DefaultScheduler::createRunnable(bool /*playbackOrRender*/)
{
    return new DefaultRenderFrameRunnable(_effect,this);
}



/**
 * @brief Called whenever there are images available to treat in the buffer.
 * Once treated, the frame will be removed from the buffer.
 *
 * According to the Mode given to the scheduler this function will be called either by the scheduler thread (this)
 * or by the application's main-thread (typically to do OpenGL rendering).
 **/
void
DefaultScheduler::treatFrame(double time,int view,const boost::shared_ptr<BufferableObject>& /*frame*/)
{
    
    ///Writers render to scale 1 always
    RenderScale scale;
    scale.x = scale.y = 1.;
    
    U64 hash = _effect->getHash();
    
    bool isProjectFormat;
    RectD rod;
    RectI roi;
    
    Natron::ImageComponents components;
    Natron::ImageBitDepth imageDepth;
    _effect->getPreferredDepthAndComponents(-1, &components, &imageDepth);
    
    (void)_effect->getRegionOfDefinition_public(hash,time, scale, view, &rod, &isProjectFormat);
    rod.toPixelEnclosing(0, &roi);

    
    Natron::EffectInstance::RenderRoIArgs args(time,
                                               scale,0,
                                               view,
                                               roi,
                                               true,
                                               false,
                                               false,
                                               rod,
                                               components,
                                               imageDepth,
                                               3);
    (void)_effect->renderRoI(args);
}

void
DefaultScheduler::timelineStepOne(OutputSchedulerThread::RenderDirection direction)
{
    if (direction == OutputSchedulerThread::RENDER_FORWARD) {
        _effect->incrementCurrentFrame();
    } else {
        _effect->decrementCurrentFrame();
    }
}

void
DefaultScheduler::timelineGoTo(int time)
{
    _effect->setCurrentFrame(time);
}

int
DefaultScheduler::timelineGetTime() const
{
    return _effect->getCurrentFrame();
}

void
DefaultScheduler::getFrameRangeToRender(int& first,int& last) const
{
    first = _effect->getFirstFrame();
    last = _effect->getLastFrame();
}

void
DefaultScheduler::timelineSetBounds(int left,int right)
{
    _effect->setFirstFrame(left);
    _effect->setLastFrame(right);
}

void
DefaultScheduler::handleRenderFailure(const std::string& errorMessage)
{
    std::cout << errorMessage << std::endl;
}

Natron::SchedulingPolicy
DefaultScheduler::getSchedulingPolicy() const
{
    Natron::SequentialPreference sequentiallity = _effect->getSequentialPreference();
    if (sequentiallity == Natron::EFFECT_NOT_SEQUENTIAL) {
        return Natron::SCHEDULING_FFA;
    } else {
        return Natron::SCHEDULING_ORDERED;
    }
}

void
DefaultScheduler::aboutToStartRender()
{
    int first,last;
    getFrameRangeRequestedToRender(first, last);
    
    _effect->setFirstFrame(first);
    _effect->setLastFrame(last);
    
    if (getDirectionRequestedToRender() == RENDER_FORWARD) {
        _effect->setCurrentFrame(first);
    } else {
        _effect->setCurrentFrame(last);
    }
    
    
    if ( !appPTR->isBackground() ) {
        _effect->setKnobsFrozen(true);
    } else {
        appPTR->writeToOutputPipe(kRenderingStartedLong, kRenderingStartedShort);
    }
}

void
DefaultScheduler::onRenderStopped()
{
    if ( !appPTR->isBackground() ) {
        _effect->setKnobsFrozen(false);
    } else {
        _effect->notifyRenderFinished();
    }
}

////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
//////////////////////// ViewerDisplayScheduler ////////////


ViewerDisplayScheduler::ViewerDisplayScheduler(ViewerInstance* viewer)
: OutputSchedulerThread(viewer,TREAT_ON_MAIN_THREAD) //< OpenGL rendering is done on the main-thread
, _viewer(viewer)
{
    
}

ViewerDisplayScheduler::~ViewerDisplayScheduler()
{
    
}


/**
 * @brief Called whenever there are images available to treat in the buffer.
 * Once treated, the frame will be removed from the buffer.
 *
 * According to the Mode given to the scheduler this function will be called either by the scheduler thread (this)
 * or by the application's main-thread (typically to do OpenGL rendering).
 **/
void
ViewerDisplayScheduler::treatFrame(double /*time*/,int /*view*/,const boost::shared_ptr<BufferableObject>& frame)
{
    boost::shared_ptr<UpdateViewerParams> params = boost::dynamic_pointer_cast<UpdateViewerParams>(frame);
    assert(params);
    _viewer->_imp->updateViewer(params);
}

void
ViewerDisplayScheduler::timelineStepOne(OutputSchedulerThread::RenderDirection direction)
{
    assert(_viewer);
    if (direction == OutputSchedulerThread::RENDER_FORWARD) {
        _viewer->getTimeline()->incrementCurrentFrame(_viewer);
    } else {
        _viewer->getTimeline()->decrementCurrentFrame(_viewer);
    }
}

void
ViewerDisplayScheduler::timelineGoTo(int time)
{
    assert(_viewer);
    _viewer->getTimeline()->seekFrame(time, _viewer);
}

int
ViewerDisplayScheduler::timelineGetTime() const
{
    return _viewer->getTimeline()->currentFrame();
}

void
ViewerDisplayScheduler::getFrameRangeToRender(int &first, int &last) const
{
    boost::shared_ptr<TimeLine> timeline = _viewer->getTimeline();
    first = timeline->leftBound();
    last = timeline->rightBound();
}


class ViewerRenderFrameRunnable : public RenderThreadTask
{
  
    ViewerInstance* _viewer;
    
public:
    
    ViewerRenderFrameRunnable(bool playbackOrRender,ViewerInstance* viewer,OutputSchedulerThread* scheduler)
    : RenderThreadTask(playbackOrRender,viewer,scheduler)
    , _viewer(viewer)
    {
        
    }
    
    virtual ~ViewerRenderFrameRunnable()
    {
        
    }
    
private:
    
    virtual void
    renderFrame(int time) {
        
        ///The viewer always uses the scheduler thread to regulate the output rate, @see ViewerInstance::renderViewer_internal
        ///it calls appendToBuffer by itself
        Status stat;
        
        try {
            stat = _viewer->renderViewer(time,false,_imp->playbackOrRender);
        } catch (...) {
            stat = StatFailed;
        }
        
        if (stat == StatFailed) {
            ///Don't report any error message otherwise we will flood the viewer with irrelevant messages such as
            ///"Render failed", instead we let the plug-in that failed post an error message which will be more helpful.
            _imp->scheduler->notifyRenderFailure(std::string());
        }

    }
};

RenderThreadTask*
ViewerDisplayScheduler::createRunnable(bool playbackOrRender)
{
    return new ViewerRenderFrameRunnable(playbackOrRender,_viewer,this);
}

void
ViewerDisplayScheduler::handleRenderFailure(const std::string& /*errorMessage*/)
{
    _viewer->disconnectViewer();
}

void
ViewerDisplayScheduler::onRenderStopped()
{
    ///Refresh all previews in the tree
    _viewer->getNode()->refreshPreviewsRecursivelyUpstream(_viewer->getTimeline()->currentFrame());
}

bool
ViewerDisplayScheduler::isTimelineRangeSetByUser() const
{
    return !_viewer->isFrameRangeLocked();
}

void
ViewerDisplayScheduler::timelineSetBounds(int left, int right)
{
    _viewer->getTimeline()->setFrameRange(left, right);
}
