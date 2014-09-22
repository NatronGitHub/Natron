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
#include <QMetaType>
#include <QMutex>
#include <QWaitCondition>
#include <QCoreApplication>
#include <QString>
#include <QThreadPool>
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
#include "Engine/ViewerInstancePrivate.h"


#define NATRON_FPS_REFRESH_RATE_SECONDS 1.5

///Let the FrameBuffer be at most NATRON_MAX_BUFFERRED_FRAMES, otherwise it might start being a huge memory sink
///and not be so efficient after all
#define NATRON_MAX_BUFFERRED_FRAMES 20

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
    
    ///The index of the next frame that a render thread should pick
    int nextFrameToPick;
    
    ///Hint to the scheduler as to how many threads we should launch
    bool playbackOrRender;
    
    ///The picking direction corresponds to the direction the render threads should use to pick new frames to render ahead
    ///whereas the timelineDirection represents the direction the timeline should move to
    OutputSchedulerThread::RenderDirection pickingDirection,timelineDirection;
    
    

};

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
    
    QThreadPool threadPool; //< worker threads that will render frames concurrently
    
    ///active runnables, only accessed from the scheduler thread
    std::list<RenderThreadTaskI*> runnables;
    
    Natron::OutputEffectInstance* outputEffect; //< The effect used as output device
    
    ///Only use on the Scheduler (this) thread.
    ///When true we adjust the number of threads to start at the end of the rendering of a frame
    bool analysingCPUActivity;
    
    OutputSchedulerThreadPrivate(Natron::OutputEffectInstance* effect,OutputSchedulerThread::Mode mode)
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
    , threadPool()
    , runnables()
    , outputEffect(effect)
    , analysingCPUActivity(false)
    {
       
    }
    
    void appendBufferedFrame(double time,int view,const boost::shared_ptr<BufferableObject>& image)
    {
        BufferedFrame k;
        k.time = time;
        k.view = view;
        k.frame = image;
        buf.insert(k);
    }
    
    void appendRunnable(RenderThreadTaskI* runnable)
    {
        runnables.push_back(runnable);
    }
    
    void removeRunnable(RenderThreadTaskI* runnable)
    {
        std::list<RenderThreadTaskI*>::iterator found = std::find(runnables.begin(), runnables.end(), runnable);
        if (found != runnables.end()) {
            runnables.erase(found);
        }
    }
    
    int getNBufferedFrames() const {
        QMutexLocker l(&bufMutex);
        return buf.size();
    }
    
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
    
}

void
OutputSchedulerThread::startRender()
{
    
    if ( isFPSRegulationNeeded() ) {
        _imp->timer->playState = RUNNING;
    }
    
    {
        ///Copy the last requested run args
        
        QMutexLocker l(&_imp->runArgsMutex);
        _imp->livingRunArgs = _imp->requestedRunArgs;
        
    }
    
    
    aboutToStartRender();
    
    ///Flag that we're now doing work
    {
        QMutexLocker l(&_imp->workingMutex);
        _imp->working = true;
    }
    
    if (!_imp->livingRunArgs.playbackOrRender) {
        
        ///If we're just re-rendering one frame, launch only one thread
        _imp->threadPool.start( createRunnable(1, false) );
        
    } else {
        
        /// Start all the render threads given user prefs.
        int nThreads = appPTR->getCurrentSettings()->getNumberOfParallelRenders();
        
        if (nThreads == 0) {
            ///User wants us to determine CPU activity automatically.
            ///We first start one runnable and monitor activty across all Natron's threads
            ///and when the first frame is finished we appropriately start other runnables.
            _imp->analysingCPUActivity = true;
            
            _imp->threadPool.start( createRunnable(1, false) );
        } else {
            
            ///The Thread-pool must not exceed the number of thread requested by the user.
            _imp->threadPool.setMaxThreadCount(nThreads);
            
            ///Trust the user: create nThreads runnable that will concurrently render all frames.
            for (int i = 0; i < nThreads; ++i) {
                _imp->threadPool.start( createRunnable(nThreads, true) );
            }
            
        }
    }
}

void
OutputSchedulerThread::stopRender()
{
    _imp->timer->playState = PAUSE;
 
    ///Wait for all render threads to be done
    _imp->threadPool.waitForDone();

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
            _imp->buf.clear();
        }
        
        ///Notify everyone that the render is finished
        emit renderFinished(wasAborted ? 1 : 0);
        
        onRenderStopped();
        
        ///Deactivate analysis of CPU activity if it is still activated
        _imp->analysingCPUActivity = false;
        
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
                    ///We must render in order, hence wait for the frame at the expected time to render
                    
                    for (FrameBuffer::iterator it = _imp->buf.begin(); it != _imp->buf.end(); ++it) {
                        if (it->time == expectedTimeToRender) {
                            frameToRender = *it;
                            _imp->buf.erase(it);
                            break;
                        }
                    }
                }
                
                ///The expected frame is not yet ready, go to sleep again
                if (!frameToRender.frame) {
                    break;
                }
                
                
                if (_imp->timer->playState == RUNNING) {
                    _imp->timer->waitUntilNextFrameIsDue(); // timer synchronizing with the requested fps
                }
                
                
                
                if (_imp->mode == TREAT_ON_SCHEDULER_THREAD) {
                    treatFrame(frameToRender.time, frameToRender.view, frameToRender.frame);
                } else {
                    ///Treat on main-thread
                    
                    QMutexLocker treatLocker (&_imp->treatMutex);
                    _imp->treatRunning = true;
                    
                    emit s_doTreatOnMainThread(frameToRender.time, frameToRender.view, frameToRender.frame);
                                        
                    while (_imp->treatRunning) {
                        _imp->treatCondition.wait(&_imp->treatMutex);
                    }
                }
                
                ////////////
                /////At this point the frame has been treated by the output device
                
                
                notifyFrameRendered(expectedTimeToRender,false);
                
                ///////////
                /////If we were analysing the CPU activity, now set the appropriate number of threads to render.
                if (_imp->analysingCPUActivity) {
                    
                    
                    
                    _imp->analysingCPUActivity = false;
                }
                
                
                ///////////
                /////Move the internal timeline indicating the next expected frame to render
                
                
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
                if (!_imp->livingRunArgs.playbackOrRender) {
                    
                    renderFinished = true;
                    
                } else {
                    if (expectedTimeToRender >= firstFrame && expectedTimeToRender < lastFrame && timelineDirection == RENDER_FORWARD) {
                        ///General case, just move the timeline by one
                        timelineStepOne(timelineDirection);
                    } else if (expectedTimeToRender > firstFrame && expectedTimeToRender <= lastFrame && timelineDirection == RENDER_BACKWARD) {
                        
                        timelineStepOne(timelineDirection);
                        
                    } else if (expectedTimeToRender <= firstFrame && timelineDirection == RENDER_BACKWARD) {
                        Natron::PlaybackMode pMode = getPlaybackMode();
                        switch (pMode) {
                            case Natron::PLAYBACK_BOUNCE: {
                                {
                                    QMutexLocker l(&_imp->runArgsMutex);
                                    _imp->livingRunArgs.timelineDirection = RENDER_FORWARD;
                                }
                                timelineGoTo(firstFrame + 1);
                                
                            }   break;
                            case Natron::PLAYBACK_LOOP:
                                timelineGoTo(lastFrame);
                                break;
                            case Natron::PLAYBACK_ONCE:
                            default:
                                renderFinished = true;
                                break;
                        }
                    } else if (expectedTimeToRender >= lastFrame && timelineDirection == RENDER_FORWARD) {
                        Natron::PlaybackMode pMode = getPlaybackMode();
                        switch (pMode) {
                            case Natron::PLAYBACK_BOUNCE: {
                                {
                                    QMutexLocker l(&_imp->runArgsMutex);
                                    _imp->livingRunArgs.timelineDirection = RENDER_BACKWARD;
                                }
                                timelineGoTo(lastFrame - 1);
                                
                            }   break;
                            case Natron::PLAYBACK_LOOP:
                                timelineGoTo(firstFrame);
                                break;
                            case Natron::PLAYBACK_ONCE:
                            default:
                                renderFinished = true;
                                break;
                        }
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
OutputSchedulerThread::notifyFrameRendered(int frame,bool countFrameRendered)
{
    emit frameRendered(frame);
    
    if (countFrameRendered) {
        QMutexLocker l(&_imp->runArgsMutex);
        ++_imp->nFramesRendered;
        if ( _imp->nFramesRendered == (U64)(_imp->livingRunArgs.lastFrame - _imp->livingRunArgs.firstFrame + 1) ) {
            
            l.unlock();
            
            _imp->renderFinished = true;
            
            ///Notify the scheduler rendering is finished by append a fake frame to the buffer
            {
                QMutexLocker bufLocker (&_imp->bufMutex);
                _imp->appendBufferedFrame(0, 0, boost::shared_ptr<BufferableObject>());
                _imp->bufCondition.wakeOne();
            }
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
        
        if (blocking) {
            l.unlock();
            _imp->threadPool.waitForDone();
            l.relock();
        }
        
        if (isMainThread) {
            
            _imp->treatRunning = false;
            _imp->treatCondition.wakeOne();
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
    
    
    if (QThread::currentThread() == qApp->thread()){
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
    timelineGoTo(firstFrame);
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
        _imp->requestedRunArgs.pickingDirection = direction;
        _imp->requestedRunArgs.timelineDirection = direction;
        
        if (direction == RENDER_FORWARD) {
            _imp->requestedRunArgs.nextFrameToPick = _imp->requestedRunArgs.firstFrame;
        } else {
            _imp->requestedRunArgs.nextFrameToPick = _imp->requestedRunArgs.lastFrame;
        }
        
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
        
        if (currentTime >= firstFrame && currentTime < lastFrame && timelineDirection == RENDER_FORWARD) {
            
            ///General case, just move the timeline by one
            timelineStepOne(timelineDirection);
            
        } else if (currentTime > firstFrame && currentTime <= lastFrame && timelineDirection == RENDER_BACKWARD) {
            
            timelineStepOne(timelineDirection);
            
        } else if (currentTime <= firstFrame) {
            Natron::PlaybackMode pMode = getPlaybackMode();
            switch (pMode) {
                case Natron::PLAYBACK_BOUNCE: {
                    timelineDirection = RENDER_FORWARD;
                    timelineGoTo(firstFrame + 1);
                    
                }   break;
                case Natron::PLAYBACK_LOOP:
                case Natron::PLAYBACK_ONCE:
                default:
                    if (timelineDirection == RENDER_BACKWARD) {
                        timelineGoTo(lastFrame);
                    } else {
                        timelineGoTo(firstFrame);
                    }
                    break;
            }
        } else if (currentTime >= lastFrame) {
            Natron::PlaybackMode pMode = getPlaybackMode();
            switch (pMode) {
                case Natron::PLAYBACK_BOUNCE: {
                    timelineDirection = RENDER_BACKWARD;
                    timelineGoTo(lastFrame - 1);
                    
                }   break;
                case Natron::PLAYBACK_LOOP:
                case Natron::PLAYBACK_ONCE:
                default:
                    if (timelineDirection == RENDER_BACKWARD) {
                        timelineGoTo(firstFrame);
                    } else {
                        timelineGoTo(lastFrame);
                    }
                    break;
            }
        }
        
        _imp->requestedRunArgs.firstFrame = firstFrame;
        _imp->requestedRunArgs.lastFrame = lastFrame;
        _imp->requestedRunArgs.playbackOrRender = true;
        _imp->requestedRunArgs.timelineDirection = timelineDirection;
        _imp->requestedRunArgs.pickingDirection = timelineDirection;
        _imp->requestedRunArgs.nextFrameToPick = timelineGetTime();
    }
    renderInternal();
}

void
OutputSchedulerThread::renderCurrentFrame()
{
    
    ///If doing playback, let it run, it will be refreshed automatically
    bool doingPlayback = isDoingPlayback();
    if (doingPlayback) {
        QMutexLocker l(&_imp->runArgsMutex);
        _imp->livingRunArgs.nextFrameToPick = timelineGetTime();
        return;
    }
    
    ///Abort any ongoing render
    abortRendering(false);

    
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
        _imp->requestedRunArgs.pickingDirection = RENDER_FORWARD;
        _imp->requestedRunArgs.nextFrameToPick = currentFrame;
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

bool
OutputSchedulerThread::pickFrameToRender(int* frame)
{
    ///Check for abortion
    {
        QMutexLocker locker(&_imp->abortedRequestedMutex);
        if (_imp->abortRequested > 0) {
            return false;
        }
    }
    
    
    QMutexLocker l(&_imp->runArgsMutex);
 
    if (_imp->livingRunArgs.nextFrameToPick < _imp->livingRunArgs.firstFrame && _imp->livingRunArgs.pickingDirection == RENDER_BACKWARD) {
        
        ///For just a re-render of the current frame, stop because we already rendered it
        if (_imp->livingRunArgs.firstFrame == _imp->livingRunArgs.lastFrame && !_imp->livingRunArgs.playbackOrRender) {
            return false;
        }
        
        PlaybackMode pMode = getPlaybackMode();
        switch (pMode) {
            case Natron::PLAYBACK_LOOP:
                *frame = _imp->livingRunArgs.lastFrame;
                _imp->livingRunArgs.nextFrameToPick = *frame - 1;
                return true;
            case Natron::PLAYBACK_BOUNCE:
                *frame = _imp->livingRunArgs.firstFrame + 1;
                _imp->livingRunArgs.nextFrameToPick = *frame + 1;
                _imp->livingRunArgs.pickingDirection = RENDER_FORWARD;
                return true;
            case Natron::PLAYBACK_ONCE:
            default:
                return false;
        }
        
    } else if (_imp->livingRunArgs.nextFrameToPick > _imp->livingRunArgs.lastFrame && _imp->livingRunArgs.pickingDirection == RENDER_FORWARD) {
        
        ///For just a re-render of the current frame, stop because we already rendered it
        if (_imp->livingRunArgs.firstFrame == _imp->livingRunArgs.lastFrame && !_imp->livingRunArgs.playbackOrRender) {
            return false;
        }
        
        PlaybackMode pMode = getPlaybackMode();
        switch (pMode) {
            case Natron::PLAYBACK_LOOP:
                *frame = _imp->livingRunArgs.firstFrame;
                _imp->livingRunArgs.nextFrameToPick = *frame + 1;
                return true;
            case Natron::PLAYBACK_BOUNCE:
                *frame = _imp->livingRunArgs.lastFrame - 1;
                _imp->livingRunArgs.nextFrameToPick = *frame - 1;
                _imp->livingRunArgs.pickingDirection = RENDER_BACKWARD;
                return true;
            case Natron::PLAYBACK_ONCE:
            default:
                return false;
        }
        
    } else {
        
        ///General case
        
        *frame = _imp->livingRunArgs.nextFrameToPick;
        if (_imp->livingRunArgs.pickingDirection == RENDER_FORWARD) {
            ++_imp->livingRunArgs.nextFrameToPick;
        } else {
            --_imp->livingRunArgs.nextFrameToPick;
        }
    }
    
    return true;
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
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
//////////////////////// RenderThreadTask ////////////

RenderThreadTaskI::RenderThreadTaskI(int nThreads,bool playbackOrRender,OutputSchedulerThread* scheduler)
: _time(0)
, _nThreads(nThreads)
, _playbackOrRender(playbackOrRender)
, _scheduler(scheduler)
{
    _scheduler->_imp->appendRunnable(this);
}

RenderThreadTaskI::~RenderThreadTaskI() {
    _scheduler->_imp->removeRunnable(this);
}


class RenderThreadTask : public RenderThreadTaskI, public QRunnable
{
    
    
public:
    
    RenderThreadTask(int nThreads,bool playbackOrRender,Natron::OutputEffectInstance* output,OutputSchedulerThread* scheduler)
    : RenderThreadTaskI(nThreads,playbackOrRender,scheduler)
    , QRunnable()
    , _output(output)
    {
    }
    
    virtual ~RenderThreadTask() {}
    
    virtual void
    run() OVERRIDE FINAL
    {
        while ( _scheduler->pickFrameToRender(&_time) ) {
            renderFrame();
        }
    }
    
    virtual void
    putAsleep() OVERRIDE FINAL
    {

    }
    
    virtual void
    wakeUp() OVERRIDE FINAL
    {
        
    }
    
protected:
    
    /**
     * @brief Must render the frame
     **/
    virtual void renderFrame() = 0;
    
    Natron::OutputEffectInstance* _output;
    QWaitCondition _bufferFullCond;
};

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
    
    DefaultRenderFrameRunnable(int nThreads,Natron::OutputEffectInstance* writer,OutputSchedulerThread* scheduler)
    : RenderThreadTask(nThreads,true,writer,scheduler)
    {
        
    }
    
    virtual ~DefaultRenderFrameRunnable()
    {
        
    }
    
private:
    
    
    virtual void
    renderFrame() {
        
        try {
            ////Writers always render at scale 1.
            int mipMapLevel = 0;
            RenderScale scale;
            scale.x = scale.y = 1.;
            
            RectD rod;
            bool isProjectFormat;
            int viewsCount = _output->getApp()->getProject()->getProjectViewsCount();
            
            
            int mainView = 0;
            
            Natron::SequentialPreference sequentiallity = _output->getSequentialPreference();
            
            ///The effect is sequential (e.g: WriteFFMPEG), and thus cannot render multiple views, we have to choose one
            ///We pick the user defined main view in the project settings
            
            bool canOnlyHandleOneView = sequentiallity == Natron::EFFECT_ONLY_SEQUENTIAL || sequentiallity == Natron::EFFECT_PREFER_SEQUENTIAL;
            if (canOnlyHandleOneView) {
                mainView = _output->getApp()->getMainView();
            }
            
            ///The hash at which the writer will render
            U64 writerHash = _output->getHash();
            
            /// If the writer dosn't need to render the frames in any sequential order (such as image sequences for instance), then
            /// we just render the frames directly in this thread, no need to use the scheduler thread for maximum efficiency.
            /// On the other hand if the current number of concurrent frame renders is 1, we make use of the scheduler thread
            /// to speed up rendering by writing ahead
            bool renderDirectly = sequentiallity == Natron::EFFECT_NOT_SEQUENTIAL  && _nThreads > 1;
            
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
                    activeInputToRender = _output;
                } else {
                    activeInputToRender = _output->getInput(0);
                    if (activeInputToRender) {
                        activeInputToRender = activeInputToRender->getNearestNonDisabled();
                    }
                    
                }
                U64 activeInputToRenderHash = activeInputToRender->getHash();
                
                Status stat = activeInputToRender->getRegionOfDefinition_public(activeInputToRenderHash,_time, scale, i, &rod, &isProjectFormat);
                if (stat != StatFailed) {
                    ImageComponents components;
                    ImageBitDepth imageDepth;
                    activeInputToRender->getPreferredDepthAndComponents(-1, &components, &imageDepth);
                    RectI renderWindow;
                    rod.toPixelEnclosing(scale, &renderWindow);
                    
                    boost::shared_ptr<Natron::Image> img =
                    activeInputToRender->renderRoI( EffectInstance::RenderRoIArgs(_time, //< the time at which to render
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
                        _scheduler->appendToBuffer(_time, i, boost::dynamic_pointer_cast<BufferableObject>(img));
                    } else {
                        _scheduler->notifyFrameRendered(_time,true);
                    }
                    
                } else {
                    break;
                }
            }
            
        } catch (const std::exception& e) {
            _scheduler->notifyRenderFailure(std::string("Error while rendering: ") + e.what());
        }
    }
};

QRunnable*
DefaultScheduler::createRunnable(int nThreads,bool /*playbackOrRender*/)
{
    return new DefaultRenderFrameRunnable(nThreads,_effect,this);
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
    if (direction == OutputSchedulerThread::RENDER_FORWARD) {
        _viewer->getTimeline()->incrementCurrentFrame(_viewer);
    } else {
        _viewer->getTimeline()->decrementCurrentFrame(_viewer);
    }
}

void
ViewerDisplayScheduler::timelineGoTo(int time)
{
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
    
    ViewerRenderFrameRunnable(int nThreads,bool playbackOrRender,ViewerInstance* viewer,OutputSchedulerThread* scheduler)
    : RenderThreadTask(nThreads,playbackOrRender,viewer,scheduler)
    , _viewer(viewer)
    {
        
    }
    
    virtual ~ViewerRenderFrameRunnable()
    {
        
    }
    
private:
    
    virtual void
    renderFrame() {
        
        ///The viewer always uses the scheduler thread to regulate the output rate, @see ViewerInstance::renderViewer_internal
        ///it calls appendToBuffer by itself
        Status stat;
        
        try {
            stat = _viewer->renderViewer(_time,false,_playbackOrRender);
        } catch (...) {
            stat = StatFailed;
        }
        
        if (stat == StatFailed) {
            ///Don't report any error message otherwise we will flood the viewer with irrelevant messages such as
            ///"Render failed", instead we let the plug-in that failed post an error message which will be more helpful.
            _scheduler->notifyRenderFailure(std::string());
        }

    }
};

QRunnable*
ViewerDisplayScheduler::createRunnable(int nThreads,bool playbackOrRender)
{
    return new ViewerRenderFrameRunnable(nThreads,playbackOrRender,_viewer,this);
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
