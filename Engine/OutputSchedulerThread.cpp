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
#include <QThreadPool>
#include <QDebug>
#include <QtConcurrentRun>
#include <QFuture>
#include <QFutureWatcher>
#include <QRunnable>

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


#define NATRON_FPS_REFRESH_RATE_SECONDS 1.5


using namespace Natron;



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
            qRegisterMetaType<BufferedFrame>("BufferedFrame");
            qRegisterMetaType<BufferableObjectList>("BufferableObjectList");
        }
    };
}
static MetaTypesRegistration registration;


struct RunArgs
{
    ///The frame range that the scheduler should render
    int firstFrame,lastFrame;
    
    /// the timelineDirection represents the direction the timeline should move to
    OutputSchedulerThread::RenderDirection timelineDirection;

};

struct RenderThread {
    RenderThreadTask* thread;
    bool active;
};
typedef std::list<RenderThread> RenderThreads;

// Struct used in a queue when rendering the current frame with a viewer, the id is meaningless just to have a member
// in the structure. We then compare the pointer of this struct
struct RequestedFrame
{
    int id;
};

struct ProducedFrame
{
    BufferableObjectList frames;
    RequestedFrame* request;
};

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
    

    ///Worker threads
    mutable QMutex renderThreadsMutex;
    RenderThreads renderThreads;
    QWaitCondition allRenderThreadsInactiveCond; // wait condition to make sure all render threads are asleep
    QWaitCondition allRenderThreadsQuitCond; //to make sure all render threads have quit
    
    ///Work queue filled by the scheduler thread when in playback/render on disk
    QMutex framesToRenderMutex; // protects framesToRender & currentFrameRequests
    std::list<int> framesToRender;
    
    ///index of the last frame pushed (framesToRender.back())
    ///we store this because when we call pushFramesToRender we need to know what was the last frame that was queued
    ///Protected by framesToRenderMutex
    int lastFramePushedIndex;
    
    ///Render threads wait in this condition and the scheduler wake them when it needs to render some frames
    QWaitCondition framesToRenderNotEmptyCond;
    

    
    Natron::OutputEffectInstance* outputEffect; //< The effect used as output device
    RenderEngine* engine;
    
    OutputSchedulerThreadPrivate(RenderEngine* engine,Natron::OutputEffectInstance* effect,OutputSchedulerThread::Mode mode)
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
    , renderThreadsMutex()
    , renderThreads()
    , allRenderThreadsInactiveCond()
    , allRenderThreadsQuitCond()
    , framesToRenderMutex()
    , framesToRender()
    , lastFramePushedIndex(0)
    , framesToRenderNotEmptyCond()
    , outputEffect(effect)
    , engine(engine)
    {
       
    }
    
    void appendBufferedFrame(double time,int view,const boost::shared_ptr<BufferableObject>& image)
    {
        ///Private, shouldn't lock
        assert(!bufMutex.tryLock());
        
        BufferedFrame k;
        k.time = time;
        k.view = view;
        k.frame.push_back(image);
        if (image) {
            bufferRAMOccupation += image->sizeInRAM();
        }
        std::pair<FrameBuffer::iterator,bool> ret = buf.insert(k);
        if (!ret.second) {
            k = *ret.first;
            k.frame.push_back(image);
            buf.erase(ret.first);
            ret = buf.insert(k);
            assert(ret.second);
        }
    }
    
    void getFromBufferAndErase(double time,BufferedFrame& frame)
    {
        
        ///Private, shouldn't lock
        assert(!bufMutex.tryLock());
        
        for (FrameBuffer::iterator it = buf.begin(); it != buf.end(); ++it) {
            
            if (it->time == time) {
                frame = *it;
                if (!frame.frame.empty()) {
                    std::size_t size = 0 ;
                    for (std::list<boost::shared_ptr<BufferableObject> >::iterator it2 = frame.frame.begin(); it2 != frame.frame.end(); ++it2) {
                        if (*it2) {
                            size += (*it2)->sizeInRAM();
                        }
                    }
                    
                    
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


OutputSchedulerThread::OutputSchedulerThread(RenderEngine* engine,Natron::OutputEffectInstance* effect,Mode mode)
: QThread()
, _imp(new OutputSchedulerThreadPrivate(engine,effect,mode))
{
    QObject::connect(this, SIGNAL(s_doTreatOnMainThread(BufferedFrame)), this,
                     SLOT(doTreatFrameMainThread(BufferedFrame)));
    
    QObject::connect(_imp->timer.get(), SIGNAL(fpsChanged(double,double)), _imp->engine, SIGNAL(fpsChanged(double,double)));
    
    QObject::connect(this, SIGNAL(s_abortRenderingOnMainThread(bool)), this, SLOT(abortRendering(bool)));
    
    setObjectName("Scheduler thread");
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
    if (firstFrame == lastFrame) {
        *nextFrame = firstFrame;
        return true;
    }
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
    
    RenderDirection direction;
    int firstFrame,lastFrame;
    {
        QMutexLocker l(&_imp->runArgsMutex);
        direction = _imp->livingRunArgs.timelineDirection;
        firstFrame = _imp->livingRunArgs.firstFrame;
        lastFrame = _imp->livingRunArgs.lastFrame;
    }
    
    PlaybackMode pMode = _imp->engine->getPlaybackMode();
    
    
    if (firstFrame == lastFrame) {
        _imp->framesToRender.push_back(startingFrame);
        _imp->lastFramePushedIndex = startingFrame;
    } else {
        ///Push 2x the count of threads to be sure no one will be waiting
        while ((int)_imp->framesToRender.size() < nThreads * 2) {
            _imp->framesToRender.push_back(startingFrame);
            
            _imp->lastFramePushedIndex = startingFrame;
            
            if (!OutputSchedulerThreadPrivate::getNextFrameInSequence(pMode, direction, startingFrame,
                                                                      firstFrame, lastFrame, &startingFrame, &direction)) {
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
    QMutexLocker l(&_imp->framesToRenderMutex);

    RenderDirection direction;
    int firstFrame,lastFrame;
    {
        QMutexLocker l(&_imp->runArgsMutex);
        direction = _imp->livingRunArgs.timelineDirection;
        firstFrame = _imp->livingRunArgs.firstFrame;
        lastFrame = _imp->livingRunArgs.lastFrame;
    }
    
    PlaybackMode pMode = _imp->engine->getPlaybackMode();
    
    int frame = _imp->lastFramePushedIndex;
    if (firstFrame == lastFrame && frame == firstFrame) {
        return;
    }

    ///If startingTime is already taken into account in the framesToRender, push new frames from the last one in the stack instead
    bool canContinue = OutputSchedulerThreadPrivate::getNextFrameInSequence(pMode, direction, frame,
                                                                        firstFrame, lastFrame, &frame, &direction);
    
    if (canContinue) {
        pushFramesToRenderInternal(frame, nThreads);
    }
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
    
    ///We will push frame to renders starting at startingFrame.
    ///They will be in the range determined by firstFrame-lastFrame
    int startingFrame;
    int firstFrame,lastFrame;
    {
        ///Copy the last requested run args
        
        QMutexLocker l(&_imp->runArgsMutex);
        _imp->livingRunArgs = _imp->requestedRunArgs;
        firstFrame = _imp->livingRunArgs.firstFrame;
        lastFrame = _imp->livingRunArgs.lastFrame;
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
        _imp->appendRunnable(createRunnable());
        nThreads = 1;
    }
    
    QMutexLocker l(&_imp->renderThreadsMutex);
    
    
    Natron::SchedulingPolicy policy = getSchedulingPolicy();
    
    if (policy == Natron::SCHEDULING_FFA) {
        
        
        ///push all frame range and let the threads deal with it
        pushAllFrameRange();
    } else {
        
        ///If the output effect is sequential (only WriteFFMPEG for now)
        Natron::SequentialPreference pref = _imp->outputEffect->getSequentialPreference();
        if (pref == EFFECT_ONLY_SEQUENTIAL || pref == EFFECT_PREFER_SEQUENTIAL) {
            
            RenderScale scaleOne;
            scaleOne.x = scaleOne.y = 1.;
            if (_imp->outputEffect->beginSequenceRender_public(firstFrame, lastFrame,
                                                               1,
                                                               false,
                                                               scaleOne, true,
                                                               true,
                                                               _imp->outputEffect->getApp()->getMainView()) == StatFailed) {
                l.unlock();
                abortRendering(false);
                return;
            }
        }
        
        ///Push as many frames as there are threads
        pushFramesToRender(startingFrame,nThreads);
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
    
    
    ///If the output effect is sequential (only WriteFFMPEG for now)
    Natron::SequentialPreference pref = _imp->outputEffect->getSequentialPreference();
    if (pref == EFFECT_ONLY_SEQUENTIAL || pref == EFFECT_PREFER_SEQUENTIAL) {
        
        int firstFrame,lastFrame;
        {
            QMutexLocker l(&_imp->runArgsMutex);
            firstFrame = _imp->livingRunArgs.firstFrame;
            lastFrame = _imp->livingRunArgs.lastFrame;
        }

        
        RenderScale scaleOne;
        scaleOne.x = scaleOne.y = 1.;
        (void)_imp->outputEffect->endSequenceRender_public(firstFrame, lastFrame,
                                                           1,
                                                           !appPTR->isBackground(),
                                                           scaleOne, true,
                                                           !appPTR->isBackground(),
                                                           _imp->outputEffect->getApp()->getMainView());
           
        
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
            _imp->outputEffect->getApp()->getProject()->setAllNodesAborted(false);
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
        _imp->engine->s_renderFinished(wasAborted ? 1 : 0);
        
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
                if (frameToRender.frame.empty()) {
                    break;
                }
    
                int nextFrameToRender = -1;
               
                if (!renderFinished) {
                
                    ///////////
                    /////Refresh frame range if needed (for viewers)
                    

                    int firstFrame,lastFrame;
                    getFrameRangeToRender(firstFrame, lastFrame);
                    
                    
                    RenderDirection timelineDirection;
                    {
                        QMutexLocker l(&_imp->runArgsMutex);
                        
                        if ( isTimelineRangeSettable() && !isTimelineRangeSetByUser() ) {
                            
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
                    Natron::PlaybackMode pMode = _imp->engine->getPlaybackMode();
                    RenderDirection newDirection;
                    if (firstFrame == lastFrame && pMode == PLAYBACK_ONCE) {
                        renderFinished = true;
                        newDirection = RENDER_FORWARD;
                    } else {
                        renderFinished = !OutputSchedulerThreadPrivate::getNextFrameInSequence(pMode, timelineDirection,
                                                                                          expectedTimeToRender, firstFrame,
                                                                                          lastFrame, &nextFrameToRender, &newDirection);
                    }
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
                    treatFrame(frameToRender);
                    
                } else {
                    ///Treat on main-thread
                                    
                    QMutexLocker treatLocker (&_imp->treatMutex);
                    
                    ///Check for abortion while under treatMutex to be sure the main thread is not deadlock in abortRendering
                    {
                        QMutexLocker locker(&_imp->abortedRequestedMutex);
                        if (_imp->abortRequested > 0) {
                            
                            ///Do not wait in the buf wait condition and go directly into the stopRender()
                            renderFinished = true;
                            
                            break;
                        }
                    }
                    
                    _imp->treatRunning = true;
                    
                    emit s_doTreatOnMainThread(frameToRender);
                                        
                    while (_imp->treatRunning) {
                        _imp->treatCondition.wait(&_imp->treatMutex);
                    }
                }
                
                
                ////////////
                /////At this point the frame has been treated by the output device
                
                
                notifyFrameRendered(expectedTimeToRender,SCHEDULING_ORDERED);
                
    
                if (!renderFinished) {
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
    
    int runningThreads = appPTR->getNRunningThreads() + QThreadPool::globalInstance()->activeThreadCount();
    
    
    int currentParallelRenders = getNRenderThreads();
    
    if (userSettingParallelThreads == 0) {
        ///User wants it to be automatically computed, do a simple heuristic: launch as many parallel renders
        ///as there are cores
        optimalNThreads = appPTR->getHardwareIdealThreadCount();
    } else {
        optimalNThreads = userSettingParallelThreads;
    }
    optimalNThreads = std::max(1,optimalNThreads);
    
    
    if (runningThreads < optimalNThreads && currentParallelRenders < optimalNThreads) {
     
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

void
OutputSchedulerThread::notifyFrameRendered(int frame,Natron::SchedulingPolicy policy)
{
    _imp->engine->s_frameRendered(frame);
    
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
        BufferedFrame b;
        b.time = time;
        b.view = view;
        b.frame.push_back(image);
        treatFrame(b);
    } else {
        
        ///Called by the scheduler thread when an image is rendered
        
        QMutexLocker l(&_imp->bufMutex);
        _imp->appendBufferedFrame(time, view, image);
        
        ///Wake up the scheduler thread that an image is available if it is asleep so it can treat it.
        _imp->bufCondition.wakeOne();
    }
}


void
OutputSchedulerThread::doTreatFrameMainThread(const BufferedFrame& frame)
{
    assert(QThread::currentThread() == qApp->thread());
    {
        QMutexLocker treatLocker (&_imp->treatMutex);
        ///The flag might have been reseted back by abortRendering()
        if (!_imp->treatRunning) {
            return;
        }
    }
    
    
    treatFrame(frame);
    
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
        _imp->abortBeingProcessed = false;
        
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
                _imp->outputEffect->getApp()->getProject()->setAllNodesAborted(true);
                
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
    
    ///Wake-up all threads and tell them that they must quit
    stopRenderThreads(0);
    
    QMutexLocker l(&_imp->renderThreadsMutex);
    
    ///Make sure they are all gone, there will be a deadlock here if that's not the case.
    _imp->waitForRenderThreadsToQuit();
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
        _imp->requestedRunArgs.timelineDirection = timelineDirection;
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
    doAbortRenderingOnMainThread(false);
    
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
    OutputSchedulerThread* scheduler;
    
    Natron::OutputEffectInstance* output;
    
    QMutex mustQuitMutex;
    bool mustQuit;
    bool hasQuit;
    
    QMutex runningMutex;
    bool running;
    
    RenderThreadTaskPrivate(Natron::OutputEffectInstance* output,OutputSchedulerThread* scheduler)
    : scheduler(scheduler)
    , output(output)
    , mustQuitMutex()
    , mustQuit(false)
    , hasQuit(false)
    , runningMutex()
    , running(false)
    {
        
    }
};


RenderThreadTask::RenderThreadTask(Natron::OutputEffectInstance* output,OutputSchedulerThread* scheduler)
: QThread()
, _imp(new RenderThreadTaskPrivate(output,scheduler))
{
    setObjectName("Parallel render thread");
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


DefaultScheduler::DefaultScheduler(RenderEngine* engine,Natron::OutputEffectInstance* effect)
: OutputSchedulerThread(engine,effect,TREAT_ON_SCHEDULER_THREAD)
, _effect(effect)
{
    engine->setPlaybackMode(PLAYBACK_ONCE);
}

DefaultScheduler::~DefaultScheduler()
{
    
}

class DefaultRenderFrameRunnable : public RenderThreadTask
{
    
public:
    
    DefaultRenderFrameRunnable(Natron::OutputEffectInstance* writer,OutputSchedulerThread* scheduler)
    : RenderThreadTask(writer,scheduler)
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
       
            
            /// If the writer dosn't need to render the frames in any sequential order (such as image sequences for instance), then
            /// we just render the frames directly in this thread, no need to use the scheduler thread for maximum efficiency.
        
            bool renderDirectly = sequentiallity == Natron::EFFECT_NOT_SEQUENTIAL;
            
            
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
                } else {
                    _imp->scheduler->notifyRenderFailure("No input to render");
                    return;
                }
                
            }
            
            assert(activeInputToRender);
            U64 activeInputToRenderHash = activeInputToRender->getHash();
            
            const double par = activeInputToRender->getPreferredAspectRatio();
            
            for (int i = 0; i < viewsCount; ++i) {
                if ( canOnlyHandleOneView && (i != mainView) ) {
                    ///@see the warning in EffectInstance::evaluate
                    continue;
                }
                
                Status stat = activeInputToRender->getRegionOfDefinition_public(activeInputToRenderHash,time, scale, i, &rod, &isProjectFormat);
                if (stat != StatFailed) {
                    ImageComponents components;
                    ImageBitDepth imageDepth;
                    activeInputToRender->getPreferredDepthAndComponents(-1, &components, &imageDepth);
                    RectI renderWindow;
                    rod.toPixelEnclosing(scale, par, &renderWindow);
                    
                    Node::ParallelRenderArgsSetter frameRenderARgs(activeInputToRender->getNode().get(),
                                                                   time,
                                                                   i,
                                                                   false,  // is this render due to user interaction ?
                                                                   canOnlyHandleOneView, // is this sequential ?
                                                                   true,
                                                                   activeInputToRenderHash);
                    
                    boost::shared_ptr<Natron::Image> img =
                    activeInputToRender->renderRoI( EffectInstance::RenderRoIArgs(time, //< the time at which to render
                                                                                  scale, //< the scale at which to render
                                                                                  mipMapLevel, //< the mipmap level (redundant with the scale)
                                                                                  i, //< the view to render
                                                                                  false,
                                                                                  renderWindow, //< the region of interest (in pixel coordinates)
                                                                                  rod, // < any precomputed rod ? in canonical coordinates
                                                                                  components,
                                                                                  imageDepth));
                    
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
DefaultScheduler::createRunnable()
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
DefaultScheduler::treatFrame(const BufferedFrame& frame)
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
    
    const double par = _effect->getPreferredAspectRatio();
    
    (void)_effect->getRegionOfDefinition_public(hash,frame.time, scale, frame.view, &rod, &isProjectFormat);
    rod.toPixelEnclosing(0, par, &roi);

    Natron::SequentialPreference sequentiallity = _effect->getSequentialPreference();
    bool canOnlyHandleOneView = sequentiallity == Natron::EFFECT_ONLY_SEQUENTIAL || sequentiallity == Natron::EFFECT_PREFER_SEQUENTIAL;
    
    Node::ParallelRenderArgsSetter frameRenderARgs(_effect->getNode().get(),
                                                   frame.time,
                                                   frame.view,
                                                   false,  // is this render due to user interaction ?
                                                   canOnlyHandleOneView, // is this sequential ?
                                                   true,
                                                   hash);
    
    
    Natron::EffectInstance::RenderRoIArgs args(frame.time,
                                               scale,0,
                                               frame.view,
                                               true, // for writers, always by-pass cache for the write node only @see renderRoiInternal
                                               roi,
                                               rod,
                                               components,
                                               imageDepth,
                                               3);
    try {
        (void)_effect->renderRoI(args);
    } catch (const std::exception& e) {
        notifyRenderFailure(e.what());
    }
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


ViewerDisplayScheduler::ViewerDisplayScheduler(RenderEngine* engine,ViewerInstance* viewer)
: OutputSchedulerThread(engine,viewer,TREAT_ON_MAIN_THREAD) //< OpenGL rendering is done on the main-thread
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
ViewerDisplayScheduler::treatFrame(const BufferedFrame& frame)
{
    for (std::list<boost::shared_ptr<BufferableObject> >::const_iterator it = frame.frame.begin(); it!=frame.frame.end(); ++it) {
        _viewer->updateViewer(*it);
    }
    if (!frame.frame.empty()) {
        _viewer->redrawViewer();
    }
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
    _viewer->getTimeline()->seekFrame(time, _viewer, Natron::PLAYBACK_SEEK);
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
    
    ViewerRenderFrameRunnable(ViewerInstance* viewer,OutputSchedulerThread* scheduler)
    : RenderThreadTask(viewer,scheduler)
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
        
        int viewsCount = _viewer->getRenderViewsCount();
        int view = viewsCount > 0 ? _viewer->getCurrentView() : 0;

        std::list<boost::shared_ptr<BufferableObject> > frames;
        try {
            stat = _viewer->renderViewer(time,view,false,true,_viewer->getHash(),true,frames);
        } catch (...) {
            stat = StatFailed;
        }
        
        if (stat == StatFailed) {
            ///Don't report any error message otherwise we will flood the viewer with irrelevant messages such as
            ///"Render failed", instead we let the plug-in that failed post an error message which will be more helpful.
            _imp->scheduler->notifyRenderFailure(std::string());
        } else {
            for (std::list<boost::shared_ptr<BufferableObject> >::iterator it = frames.begin(); it != frames.end(); ++it) {
                assert(*it);
                _imp->scheduler->appendToBuffer(time, view, *it);
            }
        }

    }
};

RenderThreadTask*
ViewerDisplayScheduler::createRunnable()
{
    return new ViewerRenderFrameRunnable(_viewer,this);
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




////////////////////////// RenderEngine

struct RenderEnginePrivate
{
    QMutex schedulerCreationLock;
    OutputSchedulerThread* scheduler;
    
    Natron::OutputEffectInstance* output;
    
    mutable QMutex pbModeMutex;
    Natron::PlaybackMode pbMode;
    
    ViewerCurrentFrameRequestScheduler* currentFrameScheduler;
    
    RenderEnginePrivate(Natron::OutputEffectInstance* output)
    : schedulerCreationLock()
    , scheduler(0)
    , output(output)
    , pbModeMutex()
    , pbMode(PLAYBACK_LOOP)
    , currentFrameScheduler(0)
    {
        
    }
};

RenderEngine::RenderEngine(Natron::OutputEffectInstance* output)
: _imp(new RenderEnginePrivate(output))
{
    
}

RenderEngine::~RenderEngine()
{
    delete _imp->currentFrameScheduler;
    delete _imp->scheduler;
}

OutputSchedulerThread*
RenderEngine::createScheduler(Natron::OutputEffectInstance* effect)
{
    return new DefaultScheduler(this,effect);
}

void
RenderEngine::renderFrameRange(int firstFrame,int lastFrame,OutputSchedulerThread::RenderDirection forward)
{
    {
        QMutexLocker k(&_imp->schedulerCreationLock);
        if (!_imp->scheduler) {
            _imp->scheduler = createScheduler(_imp->output);
        }
    }
    
    _imp->scheduler->renderFrameRange(firstFrame, lastFrame, forward);
}

void
RenderEngine::renderFromCurrentFrame(OutputSchedulerThread::RenderDirection forward)
{
    
    {
        QMutexLocker k(&_imp->schedulerCreationLock);
        if (!_imp->scheduler) {
            _imp->scheduler = createScheduler(_imp->output);
        }
    }
    
    _imp->scheduler->renderFromCurrentFrame(forward);
}

void
RenderEngine::renderCurrentFrame(bool canAbort)
{
    assert(QThread::currentThread() == qApp->thread());
    
    ViewerInstance* isViewer = dynamic_cast<ViewerInstance*>(_imp->output);
    if ( !isViewer ) {
        qDebug() << "RenderEngine::renderCurrentFrame for a writer is unsupported";
        return;
    }
    
    
    ///If the scheduler is already doing playback, continue it
    if ( _imp->scheduler && _imp->scheduler->isWorking() ) {
        _imp->scheduler->abortRendering(false);
        _imp->scheduler->renderFromCurrentFrame( _imp->scheduler->getDirectionRequestedToRender() );
        return;
    }
    
    
    {
        QMutexLocker k(&_imp->schedulerCreationLock);
        if (!_imp->scheduler) {
            _imp->scheduler = createScheduler(_imp->output);
        }
    }
    
    {
        if ( !_imp->scheduler->isTimelineRangeSetByUser() ) {
            int firstFrame,lastFrame;
            _imp->scheduler->getPluginFrameRange(firstFrame,lastFrame);
            isViewer->getTimeline()->setFrameRange(firstFrame, lastFrame);
        }
    }
    
    if (!_imp->currentFrameScheduler) {
        _imp->currentFrameScheduler = new ViewerCurrentFrameRequestScheduler(isViewer);
    }
    
    _imp->currentFrameScheduler->renderCurrentFrame(canAbort);
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

void
RenderEngine::abortRendering(bool blocking)
{
    if (_imp->scheduler) {
        _imp->scheduler->abortRendering(blocking);
    }
}

void
RenderEngine::setPlaybackMode(int mode)
{
    QMutexLocker l(&_imp->pbModeMutex);
    _imp->pbMode = (Natron::PlaybackMode)mode;
}

Natron::PlaybackMode
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
            _imp->scheduler = createScheduler(_imp->output);
        }
    }
    _imp->scheduler->setDesiredFPS(d);
}

double
RenderEngine::getDesiredFPS() const
{
    
    return _imp->scheduler ? _imp->scheduler->getDesiredFPS() : 24;
}


OutputSchedulerThread*
ViewerRenderEngine::createScheduler(Natron::OutputEffectInstance* effect) 
{
    return new ViewerDisplayScheduler(this,dynamic_cast<ViewerInstance*>(effect));
}

////////////////////////ViewerCurrentFrameRequestScheduler////////////////////////



struct ViewerCurrentFrameRequestSchedulerPrivate
{
    
    ViewerInstance* viewer;
    
    QMutex requestsQueueMutex;
    std::list<RequestedFrame*> requestsQueue;
    QWaitCondition requestsQueueNotEmpty;
    
    QMutex producedQueueMutex;
    std::list<ProducedFrame> producedQueue;
    QWaitCondition producedQueueNotEmpty;
    
    
    bool treatRunning;
    QWaitCondition treatCondition;
    QMutex treatMutex;
    
    bool mustQuit;
    mutable QMutex mustQuitMutex;
    QWaitCondition mustQuitCond;

    
    ViewerCurrentFrameRequestSchedulerPrivate(ViewerInstance* viewer)
    : viewer(viewer)
    , requestsQueueMutex()
    , requestsQueue()
    , requestsQueueNotEmpty()
    , producedQueueMutex()
    , producedQueue()
    , producedQueueNotEmpty()
    , treatRunning(false)
    , treatCondition()
    , treatMutex()
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
            mustQuitCond.wakeAll();
            return true;
        }
        return false;
    }
    
    void notifyFrameProduced(const BufferableObjectList& frames,RequestedFrame* request)
    {
        QMutexLocker k(&producedQueueMutex);
        ProducedFrame p;
        p.frames = frames;
        p.request = request;
        producedQueue.push_back(p);
        producedQueueNotEmpty.wakeOne();
    }
    
    void treatProducedFrame(const BufferableObjectList& frames);

};



static void renderCurrentFrameFunctor(ViewerInstance* viewer,bool canAbort,RequestedFrame* request, ViewerCurrentFrameRequestSchedulerPrivate* scheduler)
{
    
    ///The viewer always uses the scheduler thread to regulate the output rate, @see ViewerInstance::renderViewer_internal
    ///it calls appendToBuffer by itself
    Status stat;
    
    U64 viewerHash = viewer->getHash();
    int frame = viewer->getTimeline()->currentFrame();
    
    int viewsCount = viewer->getRenderViewsCount();
    int view = viewsCount > 0 ? viewer->getCurrentView() : 0;
    
    BufferableObjectList ret;
    try {
        stat = viewer->renderViewer(frame,view,QThread::currentThread() == qApp->thread(),false,viewerHash,canAbort,ret);
    } catch (...) {
        stat = StatFailed;
    }
    
    if (stat == StatFailed) {
        ///Don't report any error message otherwise we will flood the viewer with irrelevant messages such as
        ///"Render failed", instead we let the plug-in that failed post an error message which will be more helpful.
        viewer->disconnectViewer();
        ret.clear();
    }
    
    if (request) {
        scheduler->notifyFrameProduced(ret, request);
    } else {
        
        assert(QThread::currentThread() == qApp->thread());
        scheduler->treatProducedFrame(ret);
    }
    
}

ViewerCurrentFrameRequestScheduler::ViewerCurrentFrameRequestScheduler(ViewerInstance* viewer)
: QThread()
, _imp(new ViewerCurrentFrameRequestSchedulerPrivate(viewer))
{
    setObjectName("ViewerCurrentFrameRequestScheduler");
    QObject::connect(this, SIGNAL(s_treatProducedFrameOnMainThread(BufferableObjectList)), this, SLOT(doTreatProducedFrameOnMainThread(BufferableObjectList)));
}

ViewerCurrentFrameRequestScheduler::~ViewerCurrentFrameRequestScheduler()
{
    
}


void
ViewerCurrentFrameRequestScheduler::run()
{
    for (;;) {
        
        if (_imp->checkForExit()) {
            return;
        }
        
        
        RequestedFrame* firstRequest = 0;
        {
            QMutexLocker k(&_imp->requestsQueueMutex);
            if (!_imp->requestsQueue.empty()) {
                firstRequest = _imp->requestsQueue.front();
                _imp->requestsQueue.pop_front();
            }
        }
        
        if (firstRequest) {
            
            ///Wait for the work to be done
            QMutexLocker k(&_imp->producedQueueMutex);
            
            std::list<ProducedFrame>::iterator found = _imp->producedQueue.end();
            for (std::list<ProducedFrame>::iterator it = _imp->producedQueue.begin(); it!= _imp->producedQueue.end(); ++it) {
                if (it->request == firstRequest) {
                    found = it;
                    break;
                }
            }
            
            while (found == _imp->producedQueue.end()) {
                _imp->producedQueueNotEmpty.wait(&_imp->producedQueueMutex);
                
                for (std::list<ProducedFrame>::iterator it = _imp->producedQueue.begin(); it!= _imp->producedQueue.end(); ++it) {
                    if (it->request == firstRequest) {
                        found = it;
                        break;
                    }
                }
            }
            
            assert(found != _imp->producedQueue.end());
            
            delete found->request;
            found->request = 0;
            
            if (_imp->checkForExit()) {
                return;
            }
            {
                QMutexLocker treatLocker(&_imp->treatMutex);
                _imp->treatRunning = true;
                emit s_treatProducedFrameOnMainThread(found->frames);
                
                while (_imp->treatRunning) {
                    _imp->treatCondition.wait(&_imp->treatMutex);
                }
            }
            
            _imp->producedQueue.erase(found);
        }
        
        
        {
            QMutexLocker k(&_imp->requestsQueueMutex);
            while (_imp->requestsQueue.empty()) {
                _imp->requestsQueueNotEmpty.wait(&_imp->requestsQueueMutex);
            }
        }
        
        ///If we reach here, we've been woken up because there's work to do
        
    } // for(;;)
}

void
ViewerCurrentFrameRequestScheduler::doTreatProducedFrameOnMainThread(const BufferableObjectList& frames)
{
    _imp->treatProducedFrame(frames);
}

void
ViewerCurrentFrameRequestSchedulerPrivate::treatProducedFrame(const BufferableObjectList& frames)
{
    assert(QThread::currentThread() == qApp->thread());
    
    if (!frames.empty()) {
        for (BufferableObjectList::const_iterator it2 = frames.begin(); it2 != frames.end(); ++it2) {
            assert(*it2);
            viewer->updateViewer(*it2);
        }
        if (!frames.empty()) {
            viewer->redrawViewer();
        }
    } else {
        ///At least redraw the viewer, we might be here when the user removed a node upstream of the viewer.
        viewer->callRedrawOnMainThread();
    }
    
    {
        QMutexLocker k(&treatMutex);
        treatRunning = false;
        treatCondition.wakeOne();
    }
}

void
ViewerCurrentFrameRequestScheduler::quitThread()
{
    if (!isRunning()) {
        return;
    }
    
    {
        QMutexLocker l2(&_imp->treatMutex);
        _imp->treatRunning = false;
        _imp->treatCondition.wakeOne();
    }
    
    {
        QMutexLocker k(&_imp->mustQuitMutex);
        _imp->mustQuit = true;
        
        ///Push a fake request
        {
            QMutexLocker k(&_imp->requestsQueueMutex);
            _imp->requestsQueue.push_back(NULL);
            _imp->requestsQueueNotEmpty.wakeOne();
        }
        
        while (_imp->mustQuit) {
            _imp->mustQuitCond.wait(&_imp->mustQuitMutex);
        }
    }
}

bool
ViewerCurrentFrameRequestScheduler::hasThreadsWorking() const
{
    QMutexLocker k(&_imp->requestsQueueMutex);
    return _imp->requestsQueue.size() > 0;
}

void
ViewerCurrentFrameRequestScheduler::renderCurrentFrame(bool canAbort)
{
    if (appPTR->getCurrentSettings()->getNumberOfThreads() == -1) {
        renderCurrentFrameFunctor(_imp->viewer, canAbort, NULL,_imp.get());
    } else {
        RequestedFrame *request = new RequestedFrame;
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
        QtConcurrent::run(renderCurrentFrameFunctor,_imp->viewer, canAbort,request,_imp.get());
    }
}
