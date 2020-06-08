/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 * Copyright (C) 2018-2020 The Natron developers
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
#include <sstream> // stringstream

#include <boost/scoped_ptr.hpp>
#include <boost/algorithm/clamp.hpp>

#include <QtCore/QMetaType>
#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>
#include <QtCore/QCoreApplication>
#include <QtCore/QString>
#include <QtCore/QThreadPool>
#include <QtCore/QDebug>
#include <QtCore/QTextStream>
#include <QtCore/QRunnable>

#ifdef DEBUG
#include "Global/FloatingPointExceptions.h"
#endif
#include "Engine/AbortableRenderInfo.h"
#include "Engine/AppManager.h"
#include "Engine/AppInstance.h"
#include "Engine/EffectInstance.h"
#include "Engine/Image.h"
#include "Engine/KnobFile.h"
#include "Engine/Node.h"
#include "Engine/OpenGLViewerI.h"
#include "Engine/GenericSchedulerThreadWatcher.h"
#include "Engine/Project.h"
#include "Engine/RenderStats.h"
#include "Engine/RotoContext.h"
#include "Engine/Settings.h"
#include "Engine/Timer.h"
#include "Engine/TimeLine.h"
#include "Engine/TLSHolder.h"
#include "Engine/UpdateViewerParams.h"
#include "Engine/ViewIdx.h"
#include "Engine/ViewerInstance.h"
#include "Engine/WriteNode.h"

#ifdef DEBUG
//#define TRACE_SCHEDULER
//#define TRACE_CURRENT_FRAME_SCHEDULER
#endif

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

#define NATRON_SCHEDULER_ABORT_AFTER_X_UNSUCCESSFUL_ITERATIONS 5000

NATRON_NAMESPACE_ENTER


///Sort the frames by time and then by view

struct BufferedFrameKey
{
    int time;
};

struct BufferedFrameCompare_less
{
    bool operator()(const BufferedFrameKey& lhs,
                    const BufferedFrameKey& rhs) const
    {
        return lhs.time < rhs.time;
    }
};

typedef std::multimap<BufferedFrameKey, BufferedFrame, BufferedFrameCompare_less> FrameBuffer;


NATRON_NAMESPACE_ANONYMOUS_ENTER

class MetaTypesRegistration
{
public:
    inline MetaTypesRegistration()
    {
        qRegisterMetaType<BufferedFrames>("BufferedFrames");
        qRegisterMetaType<BufferableObjectPtrList>("BufferableObjectPtrList");
    }
};

NATRON_NAMESPACE_ANONYMOUS_EXIT


static MetaTypesRegistration registration;
struct RenderThread
{
    RenderThreadTask* thread;
    bool active;
};

typedef std::list<RenderThread> RenderThreads;


struct ProducedFrame
{
    BufferableObjectPtrList frames;
    U64 age;
    RenderStatsPtr stats;
};

struct ProducedFrameCompareAgeLess
{
    bool operator() (const ProducedFrame& lhs,
                     const ProducedFrame& rhs) const
    {
        return lhs.age < rhs.age;
    }
};

typedef std::set<ProducedFrame, ProducedFrameCompareAgeLess> ProducedFrameSet;

class OutputSchedulerThreadExecMTArgs
    : public GenericThreadExecOnMainThreadArgs
{
public:

    BufferedFrames frames;

    OutputSchedulerThreadExecMTArgs()
        : GenericThreadExecOnMainThreadArgs()
    {}

    virtual ~OutputSchedulerThreadExecMTArgs() {}
};

typedef boost::shared_ptr<OutputSchedulerThreadExecMTArgs> OutputSchedulerThreadExecMTArgsPtr;

#ifndef NATRON_PLAYBACK_USES_THREAD_POOL
static bool
isBufferFull(int nbBufferedElement,
             int hardwardIdealThreadCount)
{
    return nbBufferedElement >= hardwardIdealThreadCount * 3;
}

#endif

struct OutputSchedulerThreadPrivate
{
    FrameBuffer buf; //the frames rendered by the worker threads that needs to be rendered in order by the output device
    QWaitCondition bufEmptyCondition;
    mutable QMutex bufMutex;

    //doesn't need any protection since it never changes and is set in the constructor
    OutputSchedulerThread::ProcessFrameModeEnum mode; //is the frame to be processed on the main-thread (i.e OpenGL rendering) or on the scheduler thread
    Timer timer; // Timer regulating the engine execution. It is controlled by the GUI and MT-safe.
    boost::scoped_ptr<TimeLapse> renderTimer; // Timer used to report stats when rendering

    ///When the render threads are not using the appendToBuffer API, the scheduler has no way to know the rendering is finished
    ///but to count the number of frames rendered via notifyFrameRended which is called by the render thread.
    mutable QMutex renderFinishedMutex;
    U64 nFramesRendered;
    bool renderFinished; //< set to true when nFramesRendered = runArgs->lastFrame - runArgs->firstFrame + 1

    // Pointer to the args used in threadLoopOnce(), only usable from the scheduler thread
    OutputSchedulerThreadStartArgsWPtr runArgs;
    mutable QMutex lastRunArgsMutex;
    std::vector<ViewIdx> lastPlaybackViewsToRender;
    RenderDirectionEnum lastPlaybackRenderDirection;

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
    int expectFrameToRender;
    OutputEffectInstanceWPtr outputEffect; //< The effect used as output device
    RenderEngine* engine;

#ifdef NATRON_SCHEDULER_SPAWN_THREADS_WITH_TIMER
    QTimer threadSpawnsTimer;
    QMutex lastRecordedFPSMutex;
    double lastRecordedFPS;

#endif

    QMutex bufferedOutputMutex;
    int lastBufferedOutputSize;


    OutputSchedulerThreadPrivate(RenderEngine* engine,
                                 const OutputEffectInstancePtr& effect,
                                 OutputSchedulerThread::ProcessFrameModeEnum mode)
        : buf()
        , bufEmptyCondition()
        , bufMutex()
        , mode(mode)
        , timer()
        , renderTimer()
        , renderFinishedMutex()
        , nFramesRendered(0)
        , renderFinished(false)
        , runArgs()
        , lastRunArgsMutex()
        , lastPlaybackViewsToRender()
        , lastPlaybackRenderDirection(eRenderDirectionForward)
        , renderThreadsMutex()
        , renderThreads()
        , allRenderThreadsInactiveCond()
#ifdef NATRON_PLAYBACK_USES_THREAD_POOL
        , threadPool( QThreadPool::globalInstance() )
#else
        , allRenderThreadsQuitCond()
        , framesToRender()
        , framesToRenderNotEmptyCond()
#endif
        , framesToRenderMutex()
        , lastFramePushedIndex(0)
        , expectFrameToRender(0)
        , outputEffect(effect)
        , engine(engine)
#ifdef NATRON_SCHEDULER_SPAWN_THREADS_WITH_TIMER
        , threadSpawnsTimer()
        , lastRecordedFPSMutex()
        , lastRecordedFPS(0.)
#endif
        , bufferedOutputMutex()
        , lastBufferedOutputSize(0)
    {
    }

    void appendBufferedFrame(double time,
                             ViewIdx view,
                             const RenderStatsPtr& stats,
                             const BufferableObjectPtr& image)
    {
        ///Private, shouldn't lock
        assert( !bufMutex.tryLock() );
#ifdef TRACE_SCHEDULER
        QString idStr;
        if (image) {
            idStr = QString::fromUtf8("ID: ") + QString::number( image->getUniqueID() );
        }
        qDebug() << "Parallel Render Thread: Rendered Frame:" << time << " View:" << (int)view << idStr;
#endif
        BufferedFrameKey key;
        BufferedFrame value;
        value.time = key.time = time;
        value.view = view;
        value.frame = image;
        value.stats = stats;
        buf.insert( std::make_pair(key, value) );
    }

    struct ViewUniqueIDPair
    {
        int view;
        int uniqueId;
    };

    struct ViewUniqueIDPairCompareLess
    {
        bool operator() (const ViewUniqueIDPair& lhs,
                         const ViewUniqueIDPair& rhs) const
        {
            if (lhs.view < rhs.view) {
                return true;
            } else if (lhs.view > rhs.view) {
                return false;
            } else {
                if (lhs.uniqueId < rhs.uniqueId) {
                    return true;
                } else if (lhs.uniqueId > rhs.uniqueId) {
                    return false;
                } else {
                    return false;
                }
            }
        }
    };

    typedef std::set<ViewUniqueIDPair, ViewUniqueIDPairCompareLess> ViewUniqueIDSet;

    void getFromBufferAndErase(double time,
                               BufferedFrames& frames)
    {
        ///Private, shouldn't lock
        assert( !bufMutex.tryLock() );

        /*
           Note that the frame buffer does not hold any particular ordering and just contains all the frames as they
           were received by render threads.
           In the buffer, for any particular given time there can be:
           - Multiple views
           - Multiple "unique ID" (corresponds to viewer input A or B)

           Also since we are rendering ahead, we can have a buffered frame at time 23,
           and also another frame at time 23, each of which could have multiple unique IDs and so on

           To retrieve what we need to render, we extract at least one view and unique ID for this particular time
         */

        ViewUniqueIDSet uniqueIdsRetrieved;
        BufferedFrameKey key;
        key.time = time;
        std::pair<FrameBuffer::iterator, FrameBuffer::iterator> range = buf.equal_range(key);
        std::list<std::pair<BufferedFrameKey, BufferedFrame> > toKeep;
        for (FrameBuffer::iterator it = range.first; it != range.second; ++it) {
            bool keepInBuf = true;
            if (it->second.frame) {
                ViewUniqueIDPair p;
                p.view = (int)it->second.view;
                p.uniqueId = it->second.frame->getUniqueID();
                std::pair<ViewUniqueIDSet::iterator, bool> alreadyRetrievedIndex = uniqueIdsRetrieved.insert(p);
                if (alreadyRetrievedIndex.second) {
                    frames.push_back(it->second);
                    keepInBuf = false;
                }
            }


            if (keepInBuf) {
                toKeep.push_back(*it);
            }
        }
        if ( range.first != buf.end() ) {
            buf.erase(range.first, range.second);
            buf.insert( toKeep.begin(), toKeep.end() );
        }
    }

    void appendRunnable(RenderThreadTask* runnable)
    {
        assert( !renderThreadsMutex.tryLock() );
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
        assert( !renderThreadsMutex.tryLock() );
        for (RenderThreads::iterator it = renderThreads.begin(); it != renderThreads.end(); ++it) {
            if (it->thread == runnable) {
                return it;
            }
        }

        return renderThreads.end();
    }

    int getNBufferedFrames() const
    {
        QMutexLocker l(&bufMutex);

        return buf.size();
    }

    static bool getNextFrameInSequence(PlaybackModeEnum pMode,
                                       RenderDirectionEnum direction,
                                       int frame,
                                       int firstFrame,
                                       int lastFrame,
                                       unsigned int frameStep,
                                       int* nextFrame,
                                       RenderDirectionEnum* newDirection);
    static void getNearestInSequence(RenderDirectionEnum direction,
                                     int frame,
                                     int firstFrame,
                                     int lastFrame,
                                     int* nextFrame);


    void waitForRenderThreadsToBeDone()
    {
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

    int getNActiveRenderThreads() const
    {
        ///Private shouldn't lock
        assert( !renderThreadsMutex.tryLock() );

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
        for (;; ) {
            bool hasRemoved = false;
            for (RenderThreads::iterator it = renderThreads.begin(); it != renderThreads.end(); ++it) {
                if ( it->thread->hasQuit() ) {
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
    void removeAllQuitRenderThreads()
    {
        ///Private shouldn't lock
        assert( !renderThreadsMutex.tryLock() );

        removeQuitRenderThreadsInternal();

        ///Wake-up the main-thread if it was waiting for all threads to quit
        allRenderThreadsQuitCond.wakeOne();
    }

#endif

    void waitForRenderThreadsToQuit()
    {
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
            assert( renderThreads.empty() );
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

OutputSchedulerThread::OutputSchedulerThread(RenderEngine* engine,
                                             const OutputEffectInstancePtr& effect,
                                             ProcessFrameModeEnum mode)
    : GenericSchedulerThread()
    , _imp( new OutputSchedulerThreadPrivate(engine, effect, mode) )
{
    QObject::connect( &_imp->timer, SIGNAL(fpsChanged(double,double)), _imp->engine, SIGNAL(fpsChanged(double,double)) );


#ifdef NATRON_SCHEDULER_SPAWN_THREADS_WITH_TIMER
    QObject::connect( &_imp->threadSpawnsTimer, SIGNAL(timeout()), this, SLOT(onThreadSpawnsTimerTriggered()) );
#endif

    setThreadName("Scheduler thread");
}

OutputSchedulerThread::~OutputSchedulerThread()
{
    ///Wake-up all threads and tell them that they must quit
    stopRenderThreads(0);


    ///Make sure they are all gone, there will be a deadlock here if that's not the case.
    _imp->waitForRenderThreadsToQuit();
}

bool
OutputSchedulerThreadPrivate::getNextFrameInSequence(PlaybackModeEnum pMode,
                                                     RenderDirectionEnum direction,
                                                     int frame,
                                                     int firstFrame,
                                                     int lastFrame,
                                                     unsigned int frameStep,
                                                     int* nextFrame,
                                                     RenderDirectionEnum* newDirection)
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
            if (direction == eRenderDirectionForward) {
                *nextFrame = firstFrame + frameStep;
            } else {
                *nextFrame  = lastFrame - frameStep;
            }
            break;
        case ePlaybackModeBounce:
            if (direction == eRenderDirectionForward) {
                *newDirection = eRenderDirectionBackward;
                *nextFrame  = lastFrame - frameStep;
            } else {
                *newDirection = eRenderDirectionForward;
                *nextFrame  = firstFrame + frameStep;
            }
            break;
        case ePlaybackModeOnce:
        default:
            if (direction == eRenderDirectionForward) {
                *nextFrame = firstFrame + frameStep;
                break;
            } else {
                return false;
            }
        }
    } else if (frame >= lastFrame) {
        switch (pMode) {
        case ePlaybackModeLoop:
            if (direction == eRenderDirectionForward) {
                *nextFrame = firstFrame;
            } else {
                *nextFrame = lastFrame - frameStep;
            }
            break;
        case ePlaybackModeBounce:
            if (direction == eRenderDirectionForward) {
                *newDirection = eRenderDirectionBackward;
                *nextFrame = lastFrame - frameStep;
            } else {
                *newDirection = eRenderDirectionForward;
                *nextFrame = firstFrame + frameStep;
            }
            break;
        case ePlaybackModeOnce:
        default:
            if (direction == eRenderDirectionForward) {
                return false;
            } else {
                *nextFrame = lastFrame - frameStep;
                break;
            }
        }
    } else {
        if (direction == eRenderDirectionForward) {
            *nextFrame = frame + frameStep;
        } else {
            *nextFrame = frame - frameStep;
        }
    }

    return true;
} // OutputSchedulerThreadPrivate::getNextFrameInSequence

void
OutputSchedulerThreadPrivate::getNearestInSequence(RenderDirectionEnum direction,
                                                   int frame,
                                                   int firstFrame,
                                                   int lastFrame,
                                                   int* nextFrame)
{
    if ( (frame >= firstFrame) && (frame <= lastFrame) ) {
        *nextFrame = frame;
    } else if (frame < firstFrame) {
        if (direction == eRenderDirectionForward) {
            *nextFrame = firstFrame;
        } else {
            *nextFrame = lastFrame;
        }
    } else { // frame > lastFrame
        if (direction == eRenderDirectionForward) {
            *nextFrame = lastFrame;
        } else {
            *nextFrame = firstFrame;
        }
    }
}

#ifndef NATRON_PLAYBACK_USES_THREAD_POOL
void
OutputSchedulerThread::pushFramesToRender(int startingFrame,
                                          int nThreads)
{
    QMutexLocker l(&_imp->framesToRenderMutex);

    _imp->lastFramePushedIndex = startingFrame;

    pushFramesToRenderInternal(startingFrame, nThreads);
}

void
OutputSchedulerThread::pushFramesToRenderInternal(int startingFrame,
                                                  int nThreads)
{
    // QMutexLocker l(&_imp->framesToRenderMutex); already locked (check below)
    assert( !_imp->framesToRenderMutex.tryLock() );

    ///Make sure at least 1 frame is pushed
    if (nThreads <= 0) {
        nThreads = 1;
    }

    RenderDirectionEnum direction;
    int firstFrame, lastFrame, frameStep;
    OutputSchedulerThreadStartArgsPtr runArgs = _imp->runArgs.lock();
    assert(runArgs);
    direction = runArgs->pushTimelineDirection;
    firstFrame = runArgs->firstFrame;
    lastFrame = runArgs->lastFrame;
    frameStep = runArgs->frameStep;

    PlaybackModeEnum pMode = _imp->engine->getPlaybackMode();
    RenderDirectionEnum newDirection = direction;
    if (firstFrame == lastFrame) {
        _imp->framesToRender.push_back(startingFrame);
#ifdef TRACE_SCHEDULER
        qDebug() << "Scheduler Thread: Pushing frame to render: " << startingFrame;
#endif
        _imp->lastFramePushedIndex = startingFrame;
    } else {
        ///Push 2x the count of threads to be sure no one will be waiting
        while ( (int)_imp->framesToRender.size() < nThreads * 2 ) {
            _imp->framesToRender.push_back(startingFrame);
#ifdef TRACE_SCHEDULER
            QString pushDirectionStr = newDirection == eRenderDirectionForward ? QLatin1String("Forward") : QLatin1String("Backward");
            qDebug() << "Scheduler Thread:  Pushing frame to render: " << startingFrame << ", new push direction is " << pushDirectionStr;
#endif
            _imp->lastFramePushedIndex = startingFrame;
            runArgs->pushTimelineDirection = newDirection;

            if ( !OutputSchedulerThreadPrivate::getNextFrameInSequence(pMode, newDirection, startingFrame,
                                                                       firstFrame, lastFrame, frameStep, &startingFrame, &newDirection) ) {
                break;
            }
        }
    }


    ///Wake up render threads to notify them there's work to do
    _imp->framesToRenderNotEmptyCond.wakeAll();
}

void
OutputSchedulerThread::pushAllFrameRange()
{
    QMutexLocker l(&_imp->framesToRenderMutex);
    RenderDirectionEnum direction;
    int firstFrame, lastFrame, frameStep;
    OutputSchedulerThreadStartArgsPtr runArgs = _imp->runArgs.lock();

    assert(runArgs);
    direction = runArgs->pushTimelineDirection;
    firstFrame = runArgs->firstFrame;
    lastFrame = runArgs->lastFrame;
    frameStep = runArgs->frameStep;


    if (direction == eRenderDirectionForward) {
        for (int i = firstFrame; i <= lastFrame; i += frameStep) {
#ifdef TRACE_SCHEDULER
            qDebug() << "Scheduler Thread: Pushing frame to render: " << i;
#endif
            _imp->framesToRender.push_back(i);
        }
    } else {
        for (int i = lastFrame; i >= firstFrame; i -= frameStep) {
#ifdef TRACE_SCHEDULER
            qDebug() << "Scheduler Thread: Pushing frame to render: " << i;
#endif
            _imp->framesToRender.push_back(i);
        }
    }
    ///Wake up render threads to notify them there's work to do
    _imp->framesToRenderNotEmptyCond.wakeAll();
}

void
OutputSchedulerThread::pushFramesToRender(int nThreads)
{
    QMutexLocker l(&_imp->framesToRenderMutex);
    RenderDirectionEnum direction;
    int firstFrame, lastFrame, frameStep;
    OutputSchedulerThreadStartArgsPtr runArgs = _imp->runArgs.lock();

    assert(runArgs);
    direction = runArgs->pushTimelineDirection;
    firstFrame = runArgs->firstFrame;
    lastFrame = runArgs->lastFrame;
    frameStep = runArgs->frameStep;

    PlaybackModeEnum pMode = _imp->engine->getPlaybackMode();
    int frame = _imp->lastFramePushedIndex;

    if ( (firstFrame == lastFrame) && (frame == firstFrame) ) {
        return;
    }
    RenderDirectionEnum newDirection = direction;
    ///If startingTime is already taken into account in the framesToRender, push new frames from the last one in the stack instead
    bool canContinue = OutputSchedulerThreadPrivate::getNextFrameInSequence(pMode, direction, frame,
                                                                            firstFrame, lastFrame, frameStep, &frame, &newDirection);

    if ( canContinue && (direction != newDirection) ) {
        runArgs->pushTimelineDirection = newDirection;
    }
    if (canContinue) {
        pushFramesToRenderInternal(frame, nThreads);
    } else {
        ///Still wake up threads that may still sleep
        _imp->framesToRenderNotEmptyCond.wakeAll();
    }
}

int
OutputSchedulerThread::pickFrameToRender(RenderThreadTask* thread,
                                         bool* enableRenderStats,
                                         std::vector<ViewIdx>* viewsToRender)
{
    ///Flag the thread as inactive
    {
        QMutexLocker l(&_imp->renderThreadsMutex);
        RenderThreads::iterator found = _imp->getRunnableIterator(thread);
        assert( found != _imp->renderThreads.end() );
        found->active = false;

        ///Wake up the scheduler if it is waiting for all threads do be inactive
        _imp->allRenderThreadsInactiveCond.wakeOne();
    }


    bool gotFrame = false;
    int frame = -1;
    {
        QMutexLocker l(&_imp->framesToRenderMutex);
        while ( _imp->framesToRender.empty() && !thread->mustQuit() ) {
            ///Notify that we're no longer doing work
            thread->notifyIsRunning(false);

            _imp->framesToRenderNotEmptyCond.wait(&_imp->framesToRenderMutex);
        }

        if ( !_imp->framesToRender.empty() ) {
            ///Notify that we're running for good, will do nothing if flagged already running
            thread->notifyIsRunning(true);

            frame = _imp->framesToRender.front();

            _imp->framesToRender.pop_front();

            gotFrame = true;
        }
    }

    // thread is quitting, make sure we notified the application it is no longer running
    if (!gotFrame) {
        thread->notifyIsRunning(false);

        *enableRenderStats = false;

        return -1;
    } else {
        ///Flag the thread as active
        {
            QMutexLocker l(&_imp->renderThreadsMutex);
            RenderThreads::iterator found = _imp->getRunnableIterator(thread);
            assert( found != _imp->renderThreads.end() );
            found->active = true;
        }

        OutputSchedulerThreadStartArgsPtr args = _imp->runArgs.lock();
        *enableRenderStats = args->enableRenderStats;
        *viewsToRender = args->viewsToRender;

        return frame;
    }
} // OutputSchedulerThread::pickFrameToRender

#else // NATRON_PLAYBACK_USES_THREAD_POOL

void
OutputSchedulerThread::startTasksFromLastStartedFrame()
{
    int frame;
    bool canContinue;

    {
        QMutexLocker l(&_imp->framesToRenderMutex);
        RenderDirectionEnum direction;
        int firstFrame, lastFrame, frameStep;
        {
            QMutexLocker l(&_imp->runArgsMutex);
            direction = _imp->livingRunArgs.timelineDirection;
            firstFrame = _imp->livingRunArgs.firstFrame;
            lastFrame = _imp->livingRunArgs.lastFrame;
            frameStep = _imp->livingRunArgs.frameStep;
        }
        PlaybackModeEnum pMode = _imp->engine->getPlaybackMode();

        frame = _imp->lastFramePushedIndex;
        if ( (firstFrame == lastFrame) && (frame == firstFrame) ) {
            return;
        }
        RenderDirectionEnum newDirection = direction;
        ///If startingTime is already taken into account in the framesToRender, push new frames from the last one in the stack instead
        canContinue = OutputSchedulerThreadPrivate::getNextFrameInSequence(pMode, direction, frame,
                                                                           firstFrame, lastFrame, frameStep, &frame, &newDirection);
        if (newDirection != direction) {
            QMutexLocker l(&_imp->runArgsMutex);
            _imp->livingRunArgs.timelineDirection = newDirection;
        }
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
    nFrames = boost::algorithm::clamp(maxThreads - activeThreads, 1, 1);
#endif


    RenderDirectionEnum direction;
    int firstFrame, lastFrame, frameStep;
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
        RenderDirectionEnum newDirection = direction;
        for (int i = 0; i < nFrames; ++i) {
            RenderThreadTask* task = createRunnable(frame, useStats, viewsToRender);
            _imp->appendRunnable(task);


            {
                QMutexLocker k(&_imp->framesToRenderMutex);
                _imp->lastFramePushedIndex = frame;
            }

            if ( !OutputSchedulerThreadPrivate::getNextFrameInSequence(pMode, direction, frame,
                                                                       firstFrame, lastFrame, frameStep, &frame, &newDirection) ) {
                break;
            }
        }
        if (newDirection != newDirection) {
            QMutexLocker l(&_imp->runArgsMutex);
            _imp->livingRunArgs.timelineDirection = newDirection;
        }
    }
} // OutputSchedulerThread::startTasks

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
    adjustNumberOfThreads(&newNThreads, &lastNThreads);

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

    if ( found != _imp->renderThreads.end() ) {
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

void
OutputSchedulerThread::startRender()
{
    if ( isFPSRegulationNeeded() ) {
        _imp->timer.playState = ePlayStateRunning;
    }

    // Start measuring
    _imp->renderTimer.reset(new TimeLapse);

    ///We will push frame to renders starting at startingFrame.
    ///They will be in the range determined by firstFrame-lastFrame
    int startingFrame;
    int firstFrame, lastFrame;
    int frameStep;
    bool forward;
    OutputSchedulerThreadStartArgsPtr args = _imp->runArgs.lock();
    ///Copy the last requested run args

    firstFrame = args->firstFrame;
    lastFrame = args->lastFrame;
    frameStep = args->frameStep;
    startingFrame = timelineGetTime();
    forward = args->pushTimelineDirection == eRenderDirectionForward;


    aboutToStartRender();

    ///Notify everyone that the render is started
    _imp->engine->s_renderStarted(forward);

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


    ///If the output effect is sequential (only WriteFFMPEG for now)
    EffectInstancePtr effect = _imp->outputEffect.lock();
    WriteNode* isWriteNode = dynamic_cast<WriteNode*>( effect.get() );
    if (isWriteNode) {
        NodePtr embeddedWriter = isWriteNode->getEmbeddedWriter();
        if (embeddedWriter) {
            effect = embeddedWriter->getEffectInstance();
        }
    }
    SequentialPreferenceEnum pref = effect->getSequentialPreference();
    if ( (pref == eSequentialPreferenceOnlySequential) || (pref == eSequentialPreferencePreferSequential) ) {
        RenderScale scaleOne(1.);
        if (effect->beginSequenceRender_public( firstFrame, lastFrame,
                                                frameStep,
                                                false,
                                                scaleOne, true,
                                                true,
                                                false,
                                                ViewIdx(0),
                                                false /*useOpenGL*/,
                                                EffectInstance::OpenGLContextEffectDataPtr() ) == eStatusFailed) {
            l.unlock();


            _imp->engine->abortRenderingNoRestart();

            return;
        }
    }

    {
        QMutexLocker k(&_imp->framesToRenderMutex);
        _imp->expectFrameToRender = startingFrame;
    }
    SchedulingPolicyEnum policy = getSchedulingPolicy();
    if (policy == eSchedulingPolicyFFA) {
#ifndef NATRON_PLAYBACK_USES_THREAD_POOL
        ///push all frame range and let the threads deal with it
        pushAllFrameRange();
#endif
    } else {
#ifndef NATRON_PLAYBACK_USES_THREAD_POOL
        ///Push as many frames as there are threads
        pushFramesToRender(startingFrame, nThreads);
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
} // OutputSchedulerThread::startRender

void
OutputSchedulerThread::stopRender()
{
    _imp->timer.playState = ePlayStatePause;

#ifdef NATRON_SCHEDULER_SPAWN_THREADS_WITH_TIMER
    QMutexLocker k(&_imp->lastRecordedFPSMutex);
    _imp->lastRecordedFPS = _imp->timer.getActualFrameRate();
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

    ///If the output effect is sequential (only WriteFFMPEG for now)
    EffectInstancePtr effect = _imp->outputEffect.lock();
    WriteNode* isWriteNode = dynamic_cast<WriteNode*>( effect.get() );
    if (isWriteNode) {
        NodePtr embeddedWriter = isWriteNode->getEmbeddedWriter();
        if (embeddedWriter) {
            effect = embeddedWriter->getEffectInstance();
        }
    }
    SequentialPreferenceEnum pref = effect->getSequentialPreference();
    if ( (pref == eSequentialPreferenceOnlySequential) || (pref == eSequentialPreferencePreferSequential) ) {
        int firstFrame, lastFrame;
        OutputSchedulerThreadStartArgsPtr args = _imp->runArgs.lock();
        firstFrame = args->firstFrame;
        lastFrame = args->lastFrame;

        RenderScale scaleOne(1.);
        ignore_result( effect->endSequenceRender_public( firstFrame, lastFrame,
                                                         1,
                                                         !appPTR->isBackground(),
                                                         scaleOne, true,
                                                         !appPTR->isBackground(),
                                                         false,
                                                         ViewIdx(0),
                                                         false /*use OpenGL render*/,
                                                         EffectInstance::OpenGLContextEffectDataPtr() ) );
    }


    bool wasAborted = isBeingAborted();


    ///Notify everyone that the render is finished
    _imp->engine->s_renderFinished(wasAborted ? 1 : 0);

    onRenderStopped(wasAborted);

    // When playing once disable auto-restart
    if (!wasAborted && _imp->engine->getPlaybackMode() == ePlaybackModeOnce) {
        _imp->engine->setPlaybackAutoRestartEnabled(false);
    }


    {
        QMutexLocker k(&_imp->bufMutex);
        _imp->buf.clear();
    }

    _imp->renderTimer.reset();
} // OutputSchedulerThread::stopRender

GenericSchedulerThread::ThreadStateEnum
OutputSchedulerThread::threadLoopOnce(const GenericThreadStartArgsPtr &inArgs)
{
    OutputSchedulerThreadStartArgsPtr args = boost::dynamic_pointer_cast<OutputSchedulerThreadStartArgs>(inArgs);

    assert(args);
    _imp->runArgs = args;

    ThreadStateEnum state = eThreadStateActive;
    int expectedTimeToRenderPreviousIteration = INT_MIN;

    // This is the number of time this thread was woken up by a render thread because a frame was available for processing,
    // but this is not the frame this thread expects to render. If it reaches a certain amount, we detected a stall and abort.
    int nbIterationsWithoutProcessing = 0;

    startRender();

    for (;; ) {
        ///When set to true, we don't sleep in the bufEmptyCondition but in the startCondition instead, indicating
        ///we finished a render
        bool renderFinished = false;

        {
            ///_imp->renderFinished might be set when in FFA scheduling policy
            QMutexLocker l(&_imp->renderFinishedMutex);
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


        while (!bufferEmpty) {
            state = resolveState();
            if ( (state == eThreadStateAborted) || (state == eThreadStateStopped) ) {
                ///Do not wait in the buf wait condition and go directly into the stopEngine()
                renderFinished = true;
                break;
            }


            {
                QMutexLocker k(&_imp->framesToRenderMutex);
                expectedTimeToRender = _imp->expectFrameToRender;
            }

#ifdef TRACE_SCHEDULER
            if ( (expectedTimeToRenderPreviousIteration == INT_MIN) || (expectedTimeToRenderPreviousIteration != expectedTimeToRender) ) {
                qDebug() << "Scheduler Thread: waiting for " << expectedTimeToRender << " to be rendered...";
            }
#endif
            if (expectedTimeToRenderPreviousIteration == expectedTimeToRender) {
                ++nbIterationsWithoutProcessing;
                if (nbIterationsWithoutProcessing >= NATRON_SCHEDULER_ABORT_AFTER_X_UNSUCCESSFUL_ITERATIONS) {
#ifdef TRACE_SCHEDULER
                    qDebug() << "Scheduler Thread: Detected stall after " << NATRON_SCHEDULER_ABORT_AFTER_X_UNSUCCESSFUL_ITERATIONS
                             << "unsuccessful iterations";
#endif
                    renderFinished = true;
                    break;
                }
            } else {
                nbIterationsWithoutProcessing = 0;
            }
            OutputSchedulerThreadExecMTArgsPtr framesToRender = boost::make_shared<OutputSchedulerThreadExecMTArgs>();
            {
                QMutexLocker l(&_imp->bufMutex);
                _imp->getFromBufferAndErase(expectedTimeToRender, framesToRender->frames);
            }

            ///The expected frame is not yet ready, go to sleep again
            if ( framesToRender->frames.empty() ) {
                expectedTimeToRenderPreviousIteration = expectedTimeToRender;
                break;
            }

#ifdef TRACE_SCHEDULER
            qDebug() << "Scheduler Thread: received frame to process" << expectedTimeToRender;
#endif

            int nextFrameToRender = -1;
            RenderDirectionEnum newDirection = eRenderDirectionForward;

            if (!renderFinished) {
                ///////////
                /////Refresh frame range if needed (for viewers)


                int firstFrame, lastFrame;
                getFrameRangeToRender(firstFrame, lastFrame);


                RenderDirectionEnum timelineDirection;
                int frameStep;

                ///Refresh the firstframe/lastFrame as they might have changed on the timeline
                args->firstFrame = firstFrame;
                args->lastFrame = lastFrame;


                timelineDirection = args->processTimelineDirection;
                frameStep = args->frameStep;


                ///////////
                ///Determine if we finished rendering or if we should just increment/decrement the timeline
                ///or just loop/bounce
                PlaybackModeEnum pMode = _imp->engine->getPlaybackMode();
                if ( (firstFrame == lastFrame) && (pMode == ePlaybackModeOnce) ) {
                    renderFinished = true;
                    newDirection = eRenderDirectionForward;
                } else {
                    renderFinished = !OutputSchedulerThreadPrivate::getNextFrameInSequence(pMode, timelineDirection,
                                                                                           expectedTimeToRender, firstFrame,
                                                                                           lastFrame, frameStep, &nextFrameToRender, &newDirection);
                }

                if (newDirection != timelineDirection) {
                    args->processTimelineDirection = newDirection;
                }

                if (!renderFinished) {
                    {
                        QMutexLocker k(&_imp->framesToRenderMutex);
                        _imp->expectFrameToRender = nextFrameToRender;
                    }

#ifndef NATRON_SCHEDULER_SPAWN_THREADS_WITH_TIMER

#ifndef NATRON_PLAYBACK_USES_THREAD_POOL
                    ///////////
                    /////If we were analysing the CPU activity, now set the appropriate number of threads to render.
                    int newNThreads, lastNThreads;
                    adjustNumberOfThreads(&newNThreads, &lastNThreads);

                    ///////////
                    /////Append render requests for the render threads

                    ///Simple heuristic to limit the size of the internal buffer.
                    ///If the buffer grows too much, we will keep shared ptr to images, hence keep them in RAM which
                    ///can lead to RAM issue for the end user.
                    ///We can end up in this situation for very simple graphs where the rendering of the output node (the writer or viewer)
                    ///is much slower than things upstream, hence the buffer grows quickly, and fills up the RAM.
                    bool bufferFull;
                    {
                        QMutexLocker k(&_imp->bufMutex);
                        int nbThreadsHardware = appPTR->getHardwareIdealThreadCount();
                        bufferFull = isBufferFull(_imp->buf.size(), nbThreadsHardware);
                    }
                    if (!bufferFull) {
                        pushFramesToRender(newNThreads);
                    }
#else
                    startTasksFromLastStartedFrame();
#endif

#endif
                }
            } // if (!renderFinished) {

            if (_imp->timer.playState == ePlayStateRunning) {
                _imp->timer.waitUntilNextFrameIsDue(); // timer synchronizing with the requested fps
            }


            state = resolveState();
            if ( (state == eThreadStateAborted) || (state == eThreadStateStopped) ) {
                // Do not wait in the buf wait condition and go directly into the stopEngine()
                renderFinished = true;
                break;
            }

            if (_imp->mode == eProcessFrameBySchedulerThread) {
                processFrame(framesToRender->frames);
            } else {
                requestExecutionOnMainThread(framesToRender);
            }

            expectedTimeToRenderPreviousIteration = expectedTimeToRender;

#ifdef TRACE_SCHEDULER
            QString pushDirectionStr = newDirection == eRenderDirectionForward ? QLatin1String("Forward") : QLatin1String("Backward");
            qDebug() << "Scheduler Thread: Frame " << expectedTimeToRender << " processed, setting expectedTimeToRender to " << nextFrameToRender
                     << ", new process direction is " << pushDirectionStr;
#endif
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

            assert( !framesToRender->frames.empty() );
            {
                const BufferedFrame& frame = framesToRender->frames.front();
                std::vector<ViewIdx> views(1);
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

        if (state == eThreadStateActive) {
            state = resolveState();
        }

        if ( (state == eThreadStateAborted) || (state == eThreadStateStopped) ) {
            renderFinished = true;
        }

        if (!renderFinished) {
            assert(state == eThreadStateActive);
            QMutexLocker bufLocker (&_imp->bufMutex);
            // Wait here for more frames to be rendered, we will be woken up once appendToBuffer(...) is called
            _imp->bufEmptyCondition.wait(&_imp->bufMutex);
        } else {
            if ( !_imp->engine->isPlaybackAutoRestartEnabled() ) {
                //Move the timeline to the last rendered frame to keep it in sync with what is displayed
                timelineGoTo( getLastRenderedTime() );
            }
            break;
        }
    } // for (;;)

    stopRender();

    return state;
} // OutputSchedulerThread::threadLoopOnce

void
OutputSchedulerThread::onAbortRequested(bool /*keepOldestRender*/)
{
    ///We make sure the render-threads don't wait for the main-thread to process a frame
    ///This function (abortRendering) was probably called from a user event that was posted earlier in the
    ///event-loop, we just flag that the next event that will process the frame should NOT process it by
    ///resetting the processRunning flag
    // Flag directly all threads that they are aborted, this enables each thread to have a shorter code-path
    // when checking for abortion and will generally abort faster
    {
        QMutexLocker l(&_imp->renderThreadsMutex);
        for (RenderThreads::iterator it = _imp->renderThreads.begin(); it != _imp->renderThreads.end(); ++it) {
            AbortableThread* isAbortableThread = dynamic_cast<AbortableThread*>(it->thread);
            if (isAbortableThread) {
                bool userInteraction;
                AbortableRenderInfoPtr abortInfo;
                EffectInstancePtr treeRoot;
                isAbortableThread->getAbortInfo(&userInteraction, &abortInfo, &treeRoot);
                if (abortInfo) {
                    abortInfo->setAborted();
                }
            }
        }
    }

    ///If the scheduler is asleep waiting for the buffer to be filling up, we post a fake request
    ///that will not be processed anyway because the first thing it does is checking for abort
    {
        QMutexLocker l2(&_imp->bufMutex);
        _imp->bufEmptyCondition.wakeOne();
    }
}

void
OutputSchedulerThread::executeOnMainThread(const GenericThreadExecOnMainThreadArgsPtr& inArgs)
{
    OutputSchedulerThreadExecMTArgs* args = dynamic_cast<OutputSchedulerThreadExecMTArgs*>( inArgs.get() );

    assert(args);
    processFrame(args->frames);
}

#ifndef NATRON_PLAYBACK_USES_THREAD_POOL
void
OutputSchedulerThread::adjustNumberOfThreads(int* newNThreads,
                                             int *lastNThreads)
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
    optimalNThreads = std::max(1, optimalNThreads);


    if ( ( (runningThreads < optimalNThreads) && (currentParallelRenders < optimalNThreads) ) || (currentParallelRenders == 0) ) {
        ////////
        ///Launch 1 thread
        QMutexLocker l(&_imp->renderThreadsMutex);

        _imp->appendRunnable( createRunnable() );
        *newNThreads = currentParallelRenders +  1;
    } else if ( (runningThreads > optimalNThreads) && (currentParallelRenders > optimalNThreads) ) {
        ////////
        ///Stop 1 thread
        stopRenderThreads(1);
        *newNThreads = currentParallelRenders - 1;
    } else {
        /////////
        ///Keep the current count
        *newNThreads = std::max(1, currentParallelRenders);
    }
}

#endif // ifndef NATRON_PLAYBACK_USES_THREAD_POOL

void
OutputSchedulerThread::notifyFrameRendered(int frame,
                                           ViewIdx viewIndex,
                                           const std::vector<ViewIdx>& viewsToRender,
                                           const RenderStatsPtr& stats,
                                           SchedulingPolicyEnum policy)
{
    assert(viewsToRender.size() > 0);

    bool isLastView = viewIndex == viewsToRender[viewsToRender.size() - 1] || viewIndex == -1;

    // Report render stats if desired
    OutputEffectInstancePtr effect = _imp->outputEffect.lock();
    if (stats) {
        double timeSpentForFrame;
        std::map<NodePtr, NodeRenderStats > statResults = stats->getStats(&timeSpentForFrame);
        if ( !statResults.empty() ) {
            effect->reportStats(frame, viewIndex, timeSpentForFrame, statResults);
        }
    }


    bool isBackground = appPTR->isBackground();
    OutputSchedulerThreadStartArgsPtr runArgs = _imp->runArgs.lock();
    assert(runArgs);

    // If FFA all parallel renders call render on the Writer in their own thread,
    // otherwise the OutputSchedulerThread thread calls the render of the Writer.
    U64 nbTotalFrames;
    U64 nbFramesRendered;
    //bool renderingIsFinished = false;
    if (policy == eSchedulingPolicyFFA) {
        QMutexLocker l(&_imp->renderFinishedMutex);
        if (isLastView) {
            ++_imp->nFramesRendered;
        }
        nbTotalFrames = std::ceil( (double)(runArgs->lastFrame - runArgs->firstFrame + 1) / runArgs->frameStep );
        nbFramesRendered = _imp->nFramesRendered;


        if (_imp->nFramesRendered == nbTotalFrames) {
            _imp->renderFinished = true;
            l.unlock();

            // Notify the scheduler rendering is finished by append a fake frame to the buffer
            {
                QMutexLocker bufLocker (&_imp->bufMutex);
                _imp->appendBufferedFrame( 0, viewIndex, RenderStatsPtr(), BufferableObjectPtr() );
                _imp->bufEmptyCondition.wakeOne();
            }
        } else {
            l.unlock();

#ifndef NATRON_SCHEDULER_SPAWN_THREADS_WITH_TIMER

#ifndef NATRON_PLAYBACK_USES_THREAD_POOL
            ///////////
            /////Now set the appropriate number of threads to render.
            int newNThreads, nbCurParallelRenders;
            adjustNumberOfThreads(&newNThreads, &nbCurParallelRenders);
#else
            startTasksFromLastStartedFrame();
#endif

#endif
        }
        //renderingIsFinished = _imp->renderFinished;
    } else {
        nbTotalFrames = std::floor( (double)(runArgs->lastFrame - runArgs->firstFrame + 1) / runArgs->frameStep );
        if (runArgs->processTimelineDirection == eRenderDirectionForward) {
            nbFramesRendered = (frame - runArgs->firstFrame) / runArgs->frameStep;
        } else {
            nbFramesRendered = (runArgs->lastFrame - frame) / runArgs->frameStep;
        }
    } // if (policy == eSchedulingPolicyFFA) {

    double fractionDone = 0.;
    assert(nbTotalFrames > 0);
    if (nbTotalFrames != 0) {
        QMutexLocker k(&_imp->renderFinishedMutex);
        fractionDone = (double)_imp->nFramesRendered / nbTotalFrames;
    }
    assert(_imp->renderTimer);
    double timeSpentSinceStartSec = _imp->renderTimer->getTimeSinceCreation();
    double estimatedFps = (double)nbFramesRendered / timeSpentSinceStartSec;
    // total estimated time is: timeSpentSinceStartSec / fractionDone
    // remaining time is thus:
    double timeRemaining = (nbTotalFrames <= 0 || _imp->nFramesRendered <= 0) ? -1. : timeSpentSinceStartSec / fractionDone - timeSpentSinceStartSec;

    // If running in background, notify to the pipe that we rendered a frame
    if (isBackground) {
        QString longMessage;
        QTextStream ts(&longMessage);
        QString frameStr = QString::number(frame);
        QString fpsStr = QString::number(estimatedFps, 'f', 1);
        QString percentageStr = QString::number(fractionDone * 100, 'f', 1);
        QString timeRemainingStr = timeRemaining < 0 ? tr("unknown") : Timer::printAsTime(timeRemaining, true);

        longMessage = (tr("%1 ==> Frame: %2, Progress: %3%, %4 Fps, Time Remaining: %5")
                       .arg(QString::fromUtf8(effect->getScriptName_mt_safe().c_str()))
                       .arg(frameStr)
                       .arg(percentageStr)
                       .arg(fpsStr)
                       .arg(timeRemainingStr));

        QString shortMessage = QString::fromUtf8(kFrameRenderedStringShort) + frameStr + QString::fromUtf8(kProgressChangedStringShort) + QString::number(fractionDone);
        {
            QMutexLocker l(&_imp->bufferedOutputMutex);
            std::string toPrint = longMessage.toStdString();
            if ( (_imp->lastBufferedOutputSize != 0) && ( _imp->lastBufferedOutputSize > longMessage.size() ) ) {
                int nSpacesToAppend = _imp->lastBufferedOutputSize - longMessage.size();
                toPrint.append(nSpacesToAppend, ' ');
            }
            //std::cout << '\r';
            std::cout << toPrint;
            std::cout << std::endl;

            /*std::cout.flush();
               if (renderingIsFinished) {
                std::cout << std::endl;
               }*/
            _imp->lastBufferedOutputSize = longMessage.size();
        }

        appPTR->writeToOutputPipe(longMessage, shortMessage, false);
    }

    // Notify we rendered a frame
    if (isLastView) {
        _imp->engine->s_frameRendered(frame, fractionDone);
    }

    // Call Python after frame ranedered callback
    if ( isLastView && effect->isWriter() ) {
        std::string cb = effect->getNode()->getAfterFrameRenderCallback();
        if ( !cb.empty() ) {
            std::vector<std::string> args;
            std::string error;
            try {
                NATRON_PYTHON_NAMESPACE::getFunctionArguments(cb, &error, &args);
            } catch (const std::exception& e) {
                effect->getApp()->appendToScriptEditor( std::string("Failed to get signature of onFrameRendered callback: ")
                                                        + e.what() );

                return;
            }

            if ( !error.empty() ) {
                effect->getApp()->appendToScriptEditor("Failed to get signature of onFrameRendered callback: " + error);

                return;
            }

            std::string signatureError;
            signatureError.append("The after frame render callback supports the following signature(s):\n");
            signatureError.append("- callback(frame, thisNode, app)");
            if ( (args.size() != 3) || (args[0] != "frame") || (args[1] != "thisNode") || (args[2] != "app") ) {
                effect->getApp()->appendToScriptEditor("Wrong signature of onFrameRendered callback: " + signatureError);

                return;
            }

            std::stringstream ss;
            std::string appStr = effect->getApp()->getAppIDString();
            std::string outputNodeName = appStr + "." + effect->getNode()->getFullyQualifiedName();
            ss << cb << "(" << frame << ", ";
            ss << outputNodeName << ", " << appStr << ")";
            std::string script = ss.str();
            try {
                runCallbackWithVariables( QString::fromUtf8( script.c_str() ) );
            } catch (const std::exception& e) {
                notifyRenderFailure( e.what() );

                return;
            }
        }
    }
} // OutputSchedulerThread::notifyFrameRendered

void
OutputSchedulerThread::appendToBuffer_internal(double time,
                                               ViewIdx view,
                                               const RenderStatsPtr& stats,
                                               const BufferableObjectPtr& frame,
                                               bool wakeThread)
{
    if ( QThread::currentThread() == qApp->thread() ) {
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
        _imp->appendBufferedFrame(time, view, stats, frame);
        if (wakeThread) {
            ///Wake up the scheduler thread that an image is available if it is asleep so it can process it.
            _imp->bufEmptyCondition.wakeOne();
        }
    }
}

void
OutputSchedulerThread::appendToBuffer(double time,
                                      ViewIdx view,
                                      const RenderStatsPtr& stats,
                                      const BufferableObjectPtr& image)
{
    appendToBuffer_internal(time, view, stats, image, true);
}

void
OutputSchedulerThread::appendToBuffer(double time,
                                      ViewIdx view,
                                      const RenderStatsPtr& stats,
                                      const BufferableObjectPtrList& frames)
{
    if ( frames.empty() ) {
        return;
    }
    BufferableObjectPtrList::const_iterator next = frames.begin();
    if ( next != frames.end() ) {
        ++next;
    }
    for (BufferableObjectPtrList::const_iterator it = frames.begin(); it != frames.end(); ++it) {
        if ( next != frames.end() ) {
            appendToBuffer_internal(time, view, stats, *it, false);
            ++next;
        } else {
            appendToBuffer_internal(time, view, stats, *it, true);
        }
    }
}

void
OutputSchedulerThread::setDesiredFPS(double d)
{
    _imp->timer.setDesiredFrameRate(d);
}

double
OutputSchedulerThread::getDesiredFPS() const
{
    return _imp->timer.getDesiredFrameRate();
}

void
OutputSchedulerThread::getLastRunArgs(RenderDirectionEnum* direction,
                                      std::vector<ViewIdx>* viewsToRender) const
{
    QMutexLocker k(&_imp->lastRunArgsMutex);

    *direction = _imp->lastPlaybackRenderDirection;
    *viewsToRender = _imp->lastPlaybackViewsToRender;
}

void
OutputSchedulerThread::renderFrameRange(bool isBlocking,
                                        bool enableRenderStats,
                                        int firstFrame,
                                        int lastFrame,
                                        int frameStep,
                                        const std::vector<ViewIdx>& viewsToRender,
                                        RenderDirectionEnum direction)
{
    {
        QMutexLocker k(&_imp->lastRunArgsMutex);
        _imp->lastPlaybackRenderDirection = direction;
        _imp->lastPlaybackViewsToRender = viewsToRender;
    }
    if (direction == eRenderDirectionForward) {
        timelineGoTo(firstFrame);
    } else {
        timelineGoTo(lastFrame);
    }

    OutputSchedulerThreadStartArgsPtr args = boost::make_shared<OutputSchedulerThreadStartArgs>(isBlocking, enableRenderStats, firstFrame, lastFrame, frameStep, viewsToRender, direction);

    {
        QMutexLocker k(&_imp->renderFinishedMutex);
        _imp->nFramesRendered = 0;
        _imp->renderFinished = false;
    }

    startTask(args);
}

void
OutputSchedulerThread::renderFromCurrentFrame(bool enableRenderStats,
                                              const std::vector<ViewIdx>& viewsToRender,
                                              RenderDirectionEnum timelineDirection)
{
    {
        QMutexLocker k(&_imp->lastRunArgsMutex);
        _imp->lastPlaybackRenderDirection = timelineDirection;
        _imp->lastPlaybackViewsToRender = viewsToRender;
    }
    int firstFrame, lastFrame;

    getFrameRangeToRender(firstFrame, lastFrame);

    ///Make sure current frame is in the frame range
    int currentTime = timelineGetTime();
    OutputSchedulerThreadPrivate::getNearestInSequence(timelineDirection, currentTime, firstFrame, lastFrame, &currentTime);
    OutputSchedulerThreadStartArgsPtr args = boost::make_shared<OutputSchedulerThreadStartArgs>(false, enableRenderStats, firstFrame, lastFrame, 1, viewsToRender, timelineDirection);
    startTask(args);
}

void
OutputSchedulerThread::notifyRenderFailure(const std::string& errorMessage)
{
    ///Abort all ongoing rendering
    OutputSchedulerThreadStartArgsPtr args = _imp->runArgs.lock();

    assert(args);


    ///Handle failure: for viewers we make it black and don't display the error message which is irrelevant
    handleRenderFailure(errorMessage);

    _imp->engine->abortRenderingNoRestart();

    if (args->isBlocking) {
        waitForAbortToComplete_enforce_blocking();
    }
}

OutputSchedulerThreadStartArgsPtr
OutputSchedulerThread::getCurrentRunArgs() const
{
    return _imp->runArgs.lock();
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
             it != _imp->renderThreads.end() && (i < nThreadsToStop || nThreadsToStop == 0); ++it) {
            if ( !it->thread->mustQuit() ) {
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
    if ( !callback.isEmpty() ) {
        OutputEffectInstancePtr effect = _imp->outputEffect.lock();
        QString script = callback;
        std::string appID = effect->getApp()->getAppIDString();
        std::string nodeName = effect->getNode()->getFullyQualifiedName();
        std::string nodeFullName = appID + "." + nodeName;
        script.append( QString::fromUtf8( nodeFullName.c_str() ) );
        script.append( QLatin1Char(',') );
        script.append( QString::fromUtf8( appID.c_str() ) );
        script.append( QString::fromUtf8(")\n") );

        std::string err, output;
        if ( !NATRON_PYTHON_NAMESPACE::interpretPythonScript(callback.toStdString(), &err, &output) ) {
            effect->getApp()->appendToScriptEditor("Failed to run callback: " + err);
            throw std::runtime_error(err);
        } else if ( !output.empty() ) {
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
    OutputEffectInstanceWPtr output;

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


    RenderThreadTaskPrivate(const OutputEffectInstancePtr& output,
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
RenderThreadTask::RenderThreadTask(const OutputEffectInstancePtr& output,
                                   OutputSchedulerThread* scheduler)
    : QThread()
    , AbortableThread(this)
    , _imp( new RenderThreadTaskPrivate(output, scheduler) )
{
    setThreadName("Parallel render thread");
}

#else
RenderThreadTask::RenderThreadTask(const OutputEffectInstancePtr& output,
                                   OutputSchedulerThread* scheduler,
                                   const int time,
                                   const bool useRenderStats,
                                   const std::vector<int>& viewsToRender)
    : QRunnable()
    , _imp( new RenderThreadTaskPrivate(output, scheduler, time, useRenderStats, viewsToRender) )
{
}

#endif
RenderThreadTask::~RenderThreadTask()
{
}

void
RenderThreadTask::run()
{
#ifdef DEBUG
    boost_adaptbx::floating_point::exception_trapping trap(boost_adaptbx::floating_point::exception_trapping::division_by_zero |
                                                           boost_adaptbx::floating_point::exception_trapping::invalid |
                                                           boost_adaptbx::floating_point::exception_trapping::overflow);
#endif
#ifndef NATRON_PLAYBACK_USES_THREAD_POOL
    notifyIsRunning(true);

    for (;; ) {
        bool enableRenderStats;
        std::vector<ViewIdx> viewsToRender;
        int time = _imp->scheduler->pickFrameToRender(this, &enableRenderStats, &viewsToRender);

        if ( mustQuit() ) {
            break;
        }

#ifdef TRACE_SCHEDULER
        qDebug() << "Parallel Render Thread: Picking frame to render: " << time;
#endif
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

    appPTR->fetchAndAddNRunningThreads(running ? 1 : -1);
}

#endif

////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
//////////////////////// DefaultScheduler ////////////


DefaultScheduler::DefaultScheduler(RenderEngine* engine,
                                   const OutputEffectInstancePtr& effect)
    : OutputSchedulerThread(engine, effect, eProcessFrameBySchedulerThread)
    , _effect(effect)
    , _currentTimeMutex()
    , _currentTime(0)
{
    engine->setPlaybackMode(ePlaybackModeOnce);
}

DefaultScheduler::~DefaultScheduler()
{
}

class DefaultRenderFrameRunnable
    : public RenderThreadTask
{
public:


#ifndef NATRON_PLAYBACK_USES_THREAD_POOL
    DefaultRenderFrameRunnable(const OutputEffectInstancePtr& writer,
                               OutputSchedulerThread* scheduler)
        : RenderThreadTask(writer, scheduler)
    {
    }

#else
    DefaultRenderFrameRunnable(const OutputEffectInstancePtr& writer,
                               OutputSchedulerThread* scheduler,
                               const int time,
                               const bool useRenderStats,
                               const std::vector<int>& viewsToRender)
        : RenderThreadTask(writer, scheduler, time, useRenderStats, viewsToRender)
    {
    }

#endif


    virtual ~DefaultRenderFrameRunnable()
    {
    }

private:


    virtual void renderFrame(int time,
                             const std::vector<ViewIdx>& viewsToRender,
                             bool enableRenderStats)
    {
        OutputEffectInstancePtr output = _imp->output.lock();

        if (!output) {
            _imp->scheduler->notifyRenderFailure("");

            return;
        }

        AbortableThread* isAbortableThread = dynamic_cast<AbortableThread*>( QThread::currentThread() );

        ///Even if enableRenderStats is false, we at least profile the time spent rendering the frame when rendering with a Write node.
        ///Though we don't enable render stats for sequential renders (e.g: WriteFFMPEG) since this is 1 file.
        RenderStatsPtr stats = boost::make_shared<RenderStats>(enableRenderStats);
        NodePtr outputNode = output->getNode();
        std::string cb = outputNode->getBeforeFrameRenderCallback();
        if ( !cb.empty() ) {
            std::vector<std::string> args;
            std::string error;
            try {
                NATRON_PYTHON_NAMESPACE::getFunctionArguments(cb, &error, &args);
            } catch (const std::exception& e) {
                output->getApp()->appendToScriptEditor( std::string("Failed to get signature of beforeFrameRendered callback: ")
                                                        + e.what() );

                return;
            }

            if ( !error.empty() ) {
                output->getApp()->appendToScriptEditor("Failed to get signature of beforeFrameRendered callback: " + error);

                return;
            }

            std::string signatureError;
            signatureError.append("The before frame render callback supports the following signature(s):\n");
            signatureError.append("- callback(frame, thisNode, app)");
            if ( (args.size() != 3) || (args[0] != "frame") || (args[1] != "thisNode") || (args[2] != "app") ) {
                output->getApp()->appendToScriptEditor("Wrong signature of beforeFrameRendered callback: " + signatureError);

                return;
            }

            std::stringstream ss;
            std::string appStr = outputNode->getApp()->getAppIDString();
            std::string outputNodeName = appStr + "." + outputNode->getFullyQualifiedName();
            ss << cb << "(" << time << ", " << outputNodeName << ", " << appStr << ")";
            std::string script = ss.str();
            try {
                _imp->scheduler->runCallbackWithVariables( QString::fromUtf8( script.c_str() ) );
            } catch (const std::exception &e) {
                _imp->scheduler->notifyRenderFailure( e.what() );

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
            EffectInstancePtr activeInputToRender;
            //if (renderDirectly) {
            activeInputToRender = output;
            WriteNode* isWriteNode = dynamic_cast<WriteNode*>( output.get() );
            if (isWriteNode) {
                NodePtr embeddedWriter = isWriteNode->getEmbeddedWriter();
                if (embeddedWriter) {
                    activeInputToRender = embeddedWriter->getEffectInstance();
                }
            }
            assert(activeInputToRender);
            NodePtr activeInputNode = activeInputToRender->getNode();
            U64 activeInputToRenderHash = isWriteNode ? isWriteNode->getHash() : activeInputToRender->getHash();
            const double par = activeInputToRender->getAspectRatio(-1);
            const bool isRenderDueToRenderInteraction = false;
            const bool isSequentialRender = true;

            for (std::size_t view = 0; view < viewsToRender.size(); ++view) {
                StatusEnum stat = activeInputToRender->getRegionOfDefinition_public(activeInputToRenderHash, time, scale, viewsToRender[view], &rod, &isProjectFormat);
                if (stat == eStatusFailed) {
                    _imp->scheduler->notifyRenderFailure("Error caught while rendering");

                    return;
                }
                std::list<ImagePlaneDesc> components;
                ImageBitDepthEnum imageDepth;

                //Use needed components to figure out what we need to render
                EffectInstance::ComponentsNeededMap neededComps;
                std::list<ImagePlaneDesc> passThroughPlanes;
                bool processAll;
                double ptTime;
                int ptView;
                std::bitset<4> processChannels;
                int ptInput;
                activeInputToRender->getComponentsNeededAndProduced_public(activeInputToRenderHash,time, viewsToRender[view], &neededComps, &passThroughPlanes, &processAll, &ptTime, &ptView, &processChannels, &ptInput);


                //Retrieve bitdepth only
                imageDepth = activeInputToRender->getBitDepth(-1);
                components.clear();

                EffectInstance::ComponentsNeededMap::iterator foundOutput = neededComps.find(-1);
                if ( foundOutput != neededComps.end() ) {
                    for (std::list<ImagePlaneDesc>::const_iterator it2 = foundOutput->second.begin(); it2 != foundOutput->second.end(); ++it2) {
                        components.push_back(*it2);
                    }
                }
                RectI renderWindow;
                rod.toPixelEnclosing(scale, par, &renderWindow);


                AbortableRenderInfoPtr abortInfo = AbortableRenderInfo::create(true, 0);
                if (isAbortableThread) {
                    isAbortableThread->setAbortInfo(isRenderDueToRenderInteraction, abortInfo, activeInputToRender);
                }

                ParallelRenderArgsSetter frameRenderArgs(time,
                                                         viewsToRender[view],
                                                         isRenderDueToRenderInteraction,  // is this render due to user interaction ?
                                                         isSequentialRender,
                                                         abortInfo, //abortInfo
                                                         activeInputNode, // viewer requester
                                                         0, //texture index
                                                         output->getApp()->getTimeLine().get(),
                                                         NodePtr(),
                                                         false,
                                                         false,
                                                         stats);

                {
                    FrameRequestMap request;
                    stat = EffectInstance::computeRequestPass(time, viewsToRender[view], mipMapLevel, rod, activeInputNode, request);
                    if (stat == eStatusFailed) {
                        _imp->scheduler->notifyRenderFailure("Error caught while rendering");

                        return;
                    }
                    frameRenderArgs.updateNodesRequest(request);
                }
                RenderingFlagSetter flagIsRendering( activeInputToRender->getNode() );
                std::map<ImagePlaneDesc, ImagePtr> planes;
                boost::scoped_ptr<EffectInstance::RenderRoIArgs> renderArgs( new EffectInstance::RenderRoIArgs(time, //< the time at which to render
                                                                                                               scale, //< the scale at which to render
                                                                                                               mipMapLevel, //< the mipmap level (redundant with the scale)
                                                                                                               viewsToRender[view], //< the view to render
                                                                                                               false,
                                                                                                               renderWindow, //< the region of interest (in pixel coordinates)
                                                                                                               rod, // < any precomputed rod ? in canonical coordinates
                                                                                                               components,
                                                                                                               imageDepth,
                                                                                                               false,
                                                                                                               activeInputToRender.get(),
                                                                                                               eStorageModeRAM,
                                                                                                               time) );
                EffectInstance::RenderRoIRetCode retCode;
                retCode = activeInputToRender->renderRoI(*renderArgs, &planes);
                if (retCode != EffectInstance::eRenderRoIRetCodeOk) {
                    if (retCode == EffectInstance::eRenderRoIRetCodeAborted) {
                        _imp->scheduler->notifyRenderFailure("Render aborted");
                    } else {
                        _imp->scheduler->notifyRenderFailure("Error caught while rendering");
                    }

                    return;
                }

                ///If we need sequential rendering, pass the image to the output scheduler that will ensure the sequential ordering
                /*if (!renderDirectly) {
                    for (std::map<ImagePlaneDesc,ImagePtr>::iterator it = planes.begin(); it != planes.end(); ++it) {
                        _imp->scheduler->appendToBuffer(time, viewsToRender[view], stats, boost::dynamic_pointer_cast<BufferableObject>(it->second));
                    }
                   } else {*/
                _imp->scheduler->notifyFrameRendered(time, viewsToRender[view], viewsToRender, stats, eSchedulingPolicyFFA);
                //}
            }
        } catch (const std::exception& e) {
            _imp->scheduler->notifyRenderFailure( std::string("Error while rendering: ") + e.what() );
        }
    } // renderFrame
};

#ifndef NATRON_PLAYBACK_USES_THREAD_POOL
RenderThreadTask*
DefaultScheduler::createRunnable()
{
    return new DefaultRenderFrameRunnable(_effect.lock(), this);
}

#else
RenderThreadTask*
DefaultScheduler::createRunnable(int frame,
                                 bool useRenderStarts,
                                 const std::vector<int>& viewsToRender)
{
    return new DefaultRenderFrameRunnable(_effect.lock(), this, frame, useRenderStarts, viewsToRender);
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
    assert( !frames.empty() );
    //Only consider the first frame, we shouldn't have multiple view here anyway.
    const BufferedFrame& frame = frames.front();

    ///Writers render to scale 1 always
    RenderScale scale(1.);
    OutputEffectInstancePtr effect = _effect.lock();
    U64 hash = effect->getHash();
    bool isProjectFormat;
    RectD rod;
    RectI roi;
    std::list<ImagePlaneDesc> components;

    {
        ImagePlaneDesc metadataPlane, metadataPairedPlane;
        effect->getMetadataComponents(-1, &metadataPlane, &metadataPairedPlane);
        if (metadataPlane.getNumComponents() > 0) {
            components.push_back(metadataPlane);
        }
    }

    ImageBitDepthEnum imageDepth = effect->getBitDepth(-1);
    const double par = effect->getAspectRatio(-1);
    const bool isRenderDueToRenderInteraction = false;
    const bool isSequentialRender = true;

    for (BufferedFrames::const_iterator it = frames.begin(); it != frames.end(); ++it) {
        AbortableRenderInfoPtr abortInfo = AbortableRenderInfo::create(true, 0);

        setAbortInfo(isRenderDueToRenderInteraction, abortInfo, effect);

        ParallelRenderArgsSetter frameRenderArgs(it->time,
                                                 it->view,
                                                 isRenderDueToRenderInteraction,  // is this render due to user interaction ?
                                                 isSequentialRender, // is this sequential ?
                                                 abortInfo, //abortInfo
                                                 effect->getNode(), //tree root
                                                 0, //texture index
                                                 effect->getApp()->getTimeLine().get(),
                                                 NodePtr(),
                                                 false,
                                                 false,
                                                 it->stats);

        ignore_result( effect->getRegionOfDefinition_public(hash, it->time, scale, it->view, &rod, &isProjectFormat) );
        rod.toPixelEnclosing(0, par, &roi);


        RenderingFlagSetter flagIsRendering( effect->getNode() );
        ImagePtr inputImage = boost::dynamic_pointer_cast<Image>(it->frame);
        assert(inputImage);

        EffectInstance::InputImagesMap inputImages;
        inputImages[0].push_back(inputImage);
        boost::scoped_ptr<EffectInstance::RenderRoIArgs> renderArgs( new EffectInstance::RenderRoIArgs(frame.time,
                                                                                                       scale, 0,
                                                                                                       it->view,
                                                                                                       true, // for writers, always by-pass cache for the write node only @see renderRoiInternal
                                                                                                       roi,
                                                                                                       rod,
                                                                                                       components,
                                                                                                       imageDepth,
                                                                                                       false,
                                                                                                       effect.get(),
                                                                                                       eStorageModeRAM,
                                                                                                       frame.time,
                                                                                                       inputImages) );
        try {
            std::map<ImagePlaneDesc, ImagePtr> planes;
            EffectInstance::RenderRoIRetCode retCode;
            retCode = effect->renderRoI(*renderArgs, &planes);
            if (retCode != EffectInstance::eRenderRoIRetCodeOk) {
                notifyRenderFailure("");
            }
        } catch (const std::exception& e) {
            notifyRenderFailure( e.what() );
        }
    }
} // DefaultScheduler::processFrame

void
DefaultScheduler::timelineStepOne(RenderDirectionEnum direction)
{
    QMutexLocker k(&_currentTimeMutex);

    if (direction == eRenderDirectionForward) {
        ++_currentTime;
    } else {
        --_currentTime;
    }
}

void
DefaultScheduler::timelineGoTo(int time)
{
    QMutexLocker k(&_currentTimeMutex);

    _currentTime =  time;
}

int
DefaultScheduler::timelineGetTime() const
{
    QMutexLocker k(&_currentTimeMutex);

    return _currentTime;
}

void
DefaultScheduler::getFrameRangeToRender(int& first,
                                        int& last) const
{
    OutputSchedulerThreadStartArgsPtr args = getCurrentRunArgs();

    first = args->firstFrame;
    last = args->lastFrame;
}

void
DefaultScheduler::handleRenderFailure(const std::string& errorMessage)
{
    if ( appPTR->isBackground() ) {
        std::cerr << errorMessage << std::endl;
    }
}

SchedulingPolicyEnum
DefaultScheduler::getSchedulingPolicy() const
{
    /*SequentialPreferenceEnum sequentiallity = _effect.lock()->getSequentialPreference();
       if (sequentiallity == eSequentialPreferenceNotSequential) {*/
    return eSchedulingPolicyFFA;
    /*} else {
        return eSchedulingPolicyOrdered;
       }*/
}

void
DefaultScheduler::aboutToStartRender()
{
    OutputSchedulerThreadStartArgsPtr args = getCurrentRunArgs();
    OutputEffectInstancePtr effect = _effect.lock();

    {
        QMutexLocker k(&_currentTimeMutex);
        if (args->pushTimelineDirection == eRenderDirectionForward) {
            _currentTime  = args->firstFrame;
        } else {
            _currentTime  = args->lastFrame;
        }
    }
    bool isBackGround = appPTR->isBackground();

    if (!isBackGround) {
        effect->setKnobsFrozen(true);
    } else {
        QString longText = QString::fromUtf8( effect->getScriptName_mt_safe().c_str() ) + tr(" ==> Rendering started");
        appPTR->writeToOutputPipe(longText, QString::fromUtf8(kRenderingStartedShort), true);
    }

    // Activate the internal writer node for a write node
    WriteNode* isWriter = dynamic_cast<WriteNode*>( effect.get() );
    if (isWriter) {
        isWriter->onSequenceRenderStarted();
    }

    std::string cb = effect->getNode()->getBeforeRenderCallback();
    if ( !cb.empty() ) {
        std::vector<std::string> args;
        std::string error;
        try {
            NATRON_PYTHON_NAMESPACE::getFunctionArguments(cb, &error, &args);
        } catch (const std::exception& e) {
            effect->getApp()->appendToScriptEditor( std::string("Failed to get signature of beforeRender callback: ")
                                                    + e.what() );

            return;
        }

        if ( !error.empty() ) {
            effect->getApp()->appendToScriptEditor("Failed to get signature of beforeRender callback: " + error);

            return;
        }

        std::string signatureError;
        signatureError.append("The beforeRender callback supports the following signature(s):\n");
        signatureError.append("- callback(thisNode, app)");
        if ( (args.size() != 2) || (args[0] != "thisNode") || (args[1] != "app") ) {
            effect->getApp()->appendToScriptEditor("Failed to run beforeRender callback: " + signatureError);

            return;
        }


        std::stringstream ss;
        std::string appStr = effect->getApp()->getAppIDString();
        std::string outputNodeName = appStr + "." + effect->getNode()->getFullyQualifiedName();
        ss << cb << "(" << outputNodeName << ", " << appStr << ")";
        std::string script = ss.str();
        try {
            runCallbackWithVariables( QString::fromUtf8( script.c_str() ) );
        } catch (const std::exception &e) {
            notifyRenderFailure( e.what() );
        }
    }
} // DefaultScheduler::aboutToStartRender

void
DefaultScheduler::onRenderStopped(bool aborted)
{
    OutputEffectInstancePtr effect = _effect.lock();
    bool isBackGround = appPTR->isBackground();

    if (!isBackGround) {
        effect->setKnobsFrozen(false);
    }

    {
        QString longText = QString::fromUtf8( effect->getScriptName_mt_safe().c_str() ) + tr(" ==> Rendering finished");
        appPTR->writeToOutputPipe(longText, QString::fromUtf8(kRenderingFinishedStringShort), true);
    }

    effect->notifyRenderFinished();

    std::string cb = effect->getNode()->getAfterRenderCallback();
    if ( !cb.empty() ) {
        std::vector<std::string> args;
        std::string error;
        try {
            NATRON_PYTHON_NAMESPACE::getFunctionArguments(cb, &error, &args);
        } catch (const std::exception& e) {
            effect->getApp()->appendToScriptEditor( std::string("Failed to get signature of afterRender callback: ")
                                                    + e.what() );

            return;
        }

        if ( !error.empty() ) {
            effect->getApp()->appendToScriptEditor("Failed to get signature of afterRender callback: " + error);

            return;
        }

        std::string signatureError;
        signatureError.append("The after render callback supports the following signature(s):\n");
        signatureError.append("- callback(aborted, thisNode, app)");
        if ( (args.size() != 3) || (args[0] != "aborted") || (args[1] != "thisNode") || (args[2] != "app") ) {
            effect->getApp()->appendToScriptEditor("Wrong signature of afterRender callback: " + signatureError);

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
            runCallbackWithVariables( QString::fromUtf8( script.c_str() ) );
        } catch (...) {
            //Ignore expcetions in callback since the render is finished anyway
        }
    }
} // DefaultScheduler::onRenderStopped

////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
//////////////////////// ViewerDisplayScheduler ////////////


ViewerDisplayScheduler::ViewerDisplayScheduler(RenderEngine* engine,
                                               const ViewerInstancePtr& viewer)
    : OutputSchedulerThread(engine, viewer, eProcessFrameByMainThread) //< OpenGL rendering is done on the main-thread
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
    ViewerInstancePtr viewer = _viewer.lock();

    if ( !frames.empty() ) {
        viewer->aboutToUpdateTextures();
    }
    if ( !frames.empty() ) {
        for (BufferedFrames::const_iterator it = frames.begin(); it != frames.end(); ++it) {
            UpdateViewerParamsPtr params = boost::dynamic_pointer_cast<UpdateViewerParams>(it->frame);
            assert(params);
            viewer->updateViewer(params);
        }
        viewer->redrawViewerNow();
    } else {
        viewer->redrawViewer();
    }
}

void
ViewerDisplayScheduler::timelineStepOne(RenderDirectionEnum direction)
{
    ViewerInstancePtr viewer = _viewer.lock();

    if (direction == eRenderDirectionForward) {
        viewer->getTimeline()->incrementCurrentFrame();
    } else {
        viewer->getTimeline()->decrementCurrentFrame();
    }
}

void
ViewerDisplayScheduler::timelineGoTo(int time)
{
    ViewerInstancePtr viewer = _viewer.lock();

    viewer->getTimeline()->seekFrame(time, false, 0, eTimelineChangeReasonPlaybackSeek);
}

int
ViewerDisplayScheduler::timelineGetTime() const
{
    return _viewer.lock()->getTimeline()->currentFrame();
}

void
ViewerDisplayScheduler::getFrameRangeToRender(int &first,
                                              int &last) const
{
    ViewerInstancePtr viewer = _viewer.lock();
    ViewerInstance* leadViewer = viewer->getApp()->getLastViewerUsingTimeline();
    ViewerInstance* v = leadViewer ? leadViewer : viewer.get();

    assert(v);
    v->getTimelineBounds(&first, &last);
}

class ViewerRenderFrameRunnable
    : public RenderThreadTask
{
    ViewerInstanceWPtr _viewer;

public:

#ifndef NATRON_PLAYBACK_USES_THREAD_POOL
    ViewerRenderFrameRunnable(const ViewerInstancePtr& viewer,
                              OutputSchedulerThread* scheduler)
        : RenderThreadTask(viewer, scheduler)
        , _viewer(viewer)
    {
    }

#else
    ViewerRenderFrameRunnable(const ViewerInstancePtr& viewer,
                              OutputSchedulerThread* scheduler,
                              const int frame,
                              const bool useRenderStarts,
                              const std::vector<int>& viewsToRender)
        : RenderThreadTask(viewer, scheduler, frame, useRenderStarts, viewsToRender)
        , _viewer(viewer)
    {
    }

#endif


    virtual ~ViewerRenderFrameRunnable()
    {
    }

private:

    virtual void renderFrame(int time,
                             const std::vector<ViewIdx>& viewsToRender,
                             bool enableRenderStats)
    {
        RenderStatsPtr stats;

        if (enableRenderStats) {
            stats = boost::make_shared<RenderStats>(enableRenderStats);
        }
        ///The viewer always uses the scheduler thread to regulate the output rate, @see ViewerInstance::renderViewer_internal
        ///it calls appendToBuffer by itself
        ViewerInstance::ViewerRenderRetCode stat = ViewerInstance::eViewerRenderRetCodeRedraw;

        //Viewer can only render 1 view for now
        assert(viewsToRender.size() == 1);
        ViewIdx view = viewsToRender.front();
        ViewerInstancePtr viewer = _viewer.lock();
        U64 viewerHash = viewer->getHash();
        ViewerArgsPtr args[2];
        ViewerInstance::ViewerRenderRetCode status[2] = {
            ViewerInstance::eViewerRenderRetCodeFail, ViewerInstance::eViewerRenderRetCodeFail
        };
        bool clearTexture[2] = { false, false };
        BufferableObjectPtrList toAppend;

        for (int i = 0; i < 2; ++i) {
            args[i] = boost::make_shared<ViewerArgs>();
            status[i] = viewer->getRenderViewerArgsAndCheckCache_public( time, true, view, i, viewerHash, true, NodePtr(), stats, args[i].get() );
            clearTexture[i] = status[i] == ViewerInstance::eViewerRenderRetCodeFail || status[i] == ViewerInstance::eViewerRenderRetCodeBlack;
            if (status[i] == ViewerInstance::eViewerRenderRetCodeFail) {
                //Just clear the viewer, nothing to do
                args[i]->params.reset();
                args[i].reset();
            } else if (status[i] == ViewerInstance::eViewerRenderRetCodeBlack) {
                if (args[i]->params) {
                    args[i]->params->tiles.clear();
                    toAppend.push_back(args[i]->params);
                    args[i]->params.reset();
                }
                args[i].reset();
            }
        }

        if ( (status[0] == ViewerInstance::eViewerRenderRetCodeFail) && (status[1] == ViewerInstance::eViewerRenderRetCodeFail) ) {
            viewer->disconnectViewer();
            return;
        }

        if (clearTexture[0]) {
            viewer->disconnectTexture(0, status[0] == ViewerInstance::eViewerRenderRetCodeFail);
        }
        if (clearTexture[0]) {
            viewer->disconnectTexture(1, status[1] == ViewerInstance::eViewerRenderRetCodeFail);
        }

        if ( ( (status[0] == ViewerInstance::eViewerRenderRetCodeRedraw) && !args[0]->params->isViewerPaused ) &&
                    ( ( status[1] == ViewerInstance::eViewerRenderRetCodeRedraw) && !args[1]->params->isViewerPaused ) ) {
            return;
        } else {
            for (int i = 0; i < 2; ++i) {
                if (args[i] && args[i]->params) {
                    if ( ( (args[i]->params->nbCachedTile > 0) && ( args[i]->params->nbCachedTile == (int)args[i]->params->tiles.size() ) ) || args[i]->params->isViewerPaused ) {
                        toAppend.push_back(args[i]->params);
                        args[i].reset();
                    }
                }
            }
        }


        if ( ( args[0] && (status[0] != ViewerInstance::eViewerRenderRetCodeFail) ) || ( args[1] && (status[1] != ViewerInstance::eViewerRenderRetCodeFail) ) ) {
            try {
                stat = viewer->renderViewer(view, false, true, viewerHash, true, NodePtr(), true,  args, ViewerCurrentFrameRequestSchedulerStartArgsPtr(), stats);
            } catch (...) {
                stat = ViewerInstance::eViewerRenderRetCodeFail;
            }
        }
        if (stat == ViewerInstance::eViewerRenderRetCodeFail) {
            ///Don't report any error message otherwise we will flood the viewer with irrelevant messages such as
            ///"Render failed", instead we let the plug-in that failed post an error message which will be more helpful.
            _imp->scheduler->notifyRenderFailure( std::string() );
        } else {
            for (int i = 0; i < 2; ++i) {
                if (args[i] && args[i]->params) {
                    toAppend.push_back(args[i]->params);
                    args[i].reset();
                }
            }
        }
        _imp->scheduler->appendToBuffer(time, view, stats, toAppend);
    } // renderFrame
};


#ifndef NATRON_PLAYBACK_USES_THREAD_POOL
RenderThreadTask*
ViewerDisplayScheduler::createRunnable()
{
    return new ViewerRenderFrameRunnable(_viewer.lock(), this);
}

#else
RenderThreadTask*
ViewerDisplayScheduler::createRunnable(int frame,
                                       bool useRenderStarts,
                                       const std::vector<int>& viewsToRender)
{
    return new ViewerRenderFrameRunnable(_viewer.lock(), this, frame, useRenderStarts, viewsToRender);
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
    ViewerInstancePtr viewer = _viewer.lock();

    viewer->getApp()->refreshAllPreviews();

    if ( !viewer->getApp() || viewer->getApp()->isGuiFrozen() ) {
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

    //If true then a current frame render can start playback, protected by abortedRequestedMutex
    bool canAutoRestartPlayback;
    QMutex canAutoRestartPlaybackMutex; // protects abortRequested
    OutputEffectInstanceWPtr output;
    mutable QMutex pbModeMutex;
    PlaybackModeEnum pbMode;
    ViewerCurrentFrameRequestScheduler* currentFrameScheduler;

    // Only used on the main-thread
    boost::scoped_ptr<RenderEngineWatcher> engineWatcher;
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

    RenderEnginePrivate(const OutputEffectInstancePtr& output)
        : schedulerCreationLock()
        , scheduler(0)
        , canAutoRestartPlayback(false)
        , canAutoRestartPlaybackMutex()
        , output(output)
        , pbModeMutex()
        , pbMode(ePlaybackModeLoop)
        , currentFrameScheduler(0)
        , refreshQueue()
    {
    }
};

RenderEngine::RenderEngine(const OutputEffectInstancePtr& output)
    : _imp( new RenderEnginePrivate(output) )
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
RenderEngine::createScheduler(const OutputEffectInstancePtr& effect)
{
    return new DefaultScheduler(this, effect);
}

OutputEffectInstancePtr
RenderEngine::getOutput() const
{
    return _imp->output.lock();
}

void
RenderEngine::renderFrameRange(bool isBlocking,
                               bool enableRenderStats,
                               int firstFrame,
                               int lastFrame,
                               int frameStep,
                               const std::vector<ViewIdx>& viewsToRender,
                               RenderDirectionEnum forward)
{
    setPlaybackAutoRestartEnabled(true);

    {
        QMutexLocker k(&_imp->schedulerCreationLock);
        if (!_imp->scheduler) {
            _imp->scheduler = createScheduler( _imp->output.lock() );
        }
    }

    _imp->scheduler->renderFrameRange(isBlocking, enableRenderStats, firstFrame, lastFrame, frameStep, viewsToRender, forward);
}

void
RenderEngine::renderFromCurrentFrame(bool enableRenderStats,
                                     const std::vector<ViewIdx>& viewsToRender,
                                     RenderDirectionEnum forward)
{
    setPlaybackAutoRestartEnabled(true);

    {
        QMutexLocker k(&_imp->schedulerCreationLock);
        if (!_imp->scheduler) {
            _imp->scheduler = createScheduler( _imp->output.lock() );
        }
    }

    _imp->scheduler->renderFromCurrentFrame(enableRenderStats, viewsToRender, forward);
}

void
RenderEngine::onCurrentFrameRenderRequestPosted()
{
    assert( QThread::currentThread() == qApp->thread() );

    //Okay we are at the end of the event loop, concatenate all similar events
    RenderEnginePrivate::RefreshRequest r;
    bool rSet = false;
    while ( !_imp->refreshQueue.empty() ) {
        const RenderEnginePrivate::RefreshRequest& queueBegin = _imp->refreshQueue.front();
        if (!rSet) {
            rSet = true;
        } else {
            if ( (queueBegin.enableAbort == r.enableAbort) && (queueBegin.enableStats == r.enableStats) ) {
                _imp->refreshQueue.pop_front();
                continue;
            }
        }
        r = queueBegin;
        renderCurrentFrameInternal(r.enableStats, r.enableAbort);
        _imp->refreshQueue.pop_front();
    }
}

void
RenderEngine::renderCurrentFrameInternal(bool enableRenderStats,
                                         bool canAbort)
{
    assert( QThread::currentThread() == qApp->thread() );

    ViewerInstance* isViewer = dynamic_cast<ViewerInstance*>( _imp->output.lock().get() );
    if (!isViewer) {
        qDebug() << "RenderEngine::renderCurrentFrame for a writer is unsupported";

        return;
    }


    ///If the scheduler is already doing playback, continue it
    if (_imp->scheduler) {
        bool working = _imp->scheduler->isWorking();
        if (working) {
            _imp->scheduler->abortThreadedTask();
        }
        if ( working || isPlaybackAutoRestartEnabled() ) {
            RenderDirectionEnum lastDirection;
            std::vector<ViewIdx> lastViews;
            _imp->scheduler->getLastRunArgs(&lastDirection, &lastViews);
            _imp->scheduler->renderFromCurrentFrame( enableRenderStats, lastViews,  lastDirection);

            return;
        }
    }


    {
        QMutexLocker k(&_imp->schedulerCreationLock);
        if (!_imp->scheduler) {
            _imp->scheduler = createScheduler( _imp->output.lock() );
        }
    }

    if (!_imp->currentFrameScheduler) {
        _imp->currentFrameScheduler = new ViewerCurrentFrameRequestScheduler(isViewer);
    }

    _imp->currentFrameScheduler->renderCurrentFrame(enableRenderStats, canAbort);
}

void
RenderEngine::renderCurrentFrame(bool enableRenderStats,
                                 bool canAbort)
{
    assert( QThread::currentThread() == qApp->thread() );
    RenderEnginePrivate::RefreshRequest r;
    r.enableStats = enableRenderStats;
    r.enableAbort = canAbort;
    _imp->refreshQueue.push_back(r);
    Q_EMIT currentFrameRenderRequestPosted();
}

void
RenderEngine::setPlaybackAutoRestartEnabled(bool enabled)
{
    QMutexLocker k(&_imp->canAutoRestartPlaybackMutex);

    _imp->canAutoRestartPlayback = enabled;
}

bool
RenderEngine::isPlaybackAutoRestartEnabled() const
{
    QMutexLocker k(&_imp->canAutoRestartPlaybackMutex);

    return _imp->canAutoRestartPlayback;
}

void
RenderEngine::quitEngine(bool allowRestarts)
{
    if (_imp->scheduler) {
        _imp->scheduler->quitThread(allowRestarts);
    }

    if (_imp->currentFrameScheduler) {
        _imp->currentFrameScheduler->quitThread(allowRestarts);
    }
}

void
RenderEngine::waitForEngineToQuit_not_main_thread()
{
    if (_imp->scheduler) {
        _imp->scheduler->waitForThreadToQuit_not_main_thread();
    }

    if (_imp->currentFrameScheduler) {
        _imp->currentFrameScheduler->waitForThreadToQuit_not_main_thread();
    }
}

void
RenderEngine::waitForEngineToQuit_main_thread(bool allowRestart)
{
    assert( QThread::currentThread() == qApp->thread() );
    assert(!_imp->engineWatcher);
    _imp->engineWatcher.reset( new RenderEngineWatcher(this) );
    QObject::connect( _imp->engineWatcher.get(), SIGNAL(taskFinished(int,GenericWatcherCallerArgsPtr)), this, SLOT(onWatcherEngineQuitEmitted()) );
    _imp->engineWatcher->scheduleBlockingTask(allowRestart ? RenderEngineWatcher::eBlockingTaskWaitForQuitAllowRestart : RenderEngineWatcher::eBlockingTaskWaitForQuitDisallowRestart);
}

void
RenderEngine::waitForEngineToQuit_enforce_blocking()
{
    if (_imp->scheduler) {
        _imp->scheduler->waitForThreadToQuit_enforce_blocking();
    }

    if (_imp->currentFrameScheduler) {
        _imp->currentFrameScheduler->waitForThreadToQuit_enforce_blocking();
    }
}

bool
RenderEngine::abortRenderingInternal(bool keepOldestRender)
{
    bool ret = false;

    if (_imp->currentFrameScheduler) {
        ret |= _imp->currentFrameScheduler->abortThreadedTask(keepOldestRender);
    }

    if ( _imp->scheduler && _imp->scheduler->isWorking() ) {
        //If any playback active, abort it
        ret |= _imp->scheduler->abortThreadedTask(keepOldestRender);
    }

    return ret;
}

bool
RenderEngine::abortRenderingNoRestart(bool keepOldestRender)
{
    if ( abortRenderingInternal(keepOldestRender) ) {
        setPlaybackAutoRestartEnabled(false);

        return true;
    }

    return false;
}

bool
RenderEngine::abortRenderingAutoRestart()
{
    if ( abortRenderingInternal(true) ) {
        return true;
    }

    return false;
}

void
RenderEngine::waitForAbortToComplete_not_main_thread()
{
    if (_imp->currentFrameScheduler) {
        _imp->currentFrameScheduler->waitForAbortToComplete_not_main_thread();
    }
    if (_imp->scheduler) {
        _imp->scheduler->waitForAbortToComplete_not_main_thread();
    }
}

void
RenderEngine::waitForAbortToComplete_enforce_blocking()
{
    if (_imp->scheduler) {
        _imp->scheduler->waitForAbortToComplete_enforce_blocking();
    }

    if (_imp->currentFrameScheduler) {
        _imp->currentFrameScheduler->waitForAbortToComplete_enforce_blocking();
    }
}

void
RenderEngine::onWatcherEngineAbortedEmitted()
{
    assert(_imp->engineWatcher);
    if (!_imp->engineWatcher) {
        return;
    }
    _imp->engineWatcher.reset();
    Q_EMIT engineAborted();
}

void
RenderEngine::onWatcherEngineQuitEmitted()
{
    assert(_imp->engineWatcher);
    if (!_imp->engineWatcher) {
        return;
    }
    _imp->engineWatcher.reset();
    Q_EMIT engineQuit();
}

void
RenderEngine::waitForAbortToComplete_main_thread()
{
    assert( QThread::currentThread() == qApp->thread() );
    assert(!_imp->engineWatcher);
    _imp->engineWatcher.reset( new RenderEngineWatcher(this) );
    QObject::connect( _imp->engineWatcher.get(), SIGNAL(taskFinished(int,GenericWatcherCallerArgsPtr)), this, SLOT(onWatcherEngineAbortedEmitted()) );
    _imp->engineWatcher->scheduleBlockingTask(RenderEngineWatcher::eBlockingTaskWaitForAbort);
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
    bool working = false;

    if (_imp->scheduler) {
        working |= _imp->scheduler->isWorking();
    }

    if (!working && _imp->currentFrameScheduler) {
        working |= _imp->currentFrameScheduler->isWorking();
    }

    return working;
}

bool
RenderEngine::isDoingSequentialRender() const
{
    return _imp->scheduler ? _imp->scheduler->isWorking() : false;
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
            _imp->scheduler = createScheduler( _imp->output.lock() );
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
RenderEngine::notifyFrameProduced(const BufferableObjectPtrList& frames,
                                  const RenderStatsPtr& stats,
                                  const ViewerCurrentFrameRequestSchedulerStartArgsPtr& request)
{
    _imp->currentFrameScheduler->notifyFrameProduced(frames, stats, request);
}

OutputSchedulerThread*
ViewerRenderEngine::createScheduler(const OutputEffectInstancePtr& effect)
{
    return new ViewerDisplayScheduler( this, boost::dynamic_pointer_cast<ViewerInstance>(effect) );
}

////////////////////////ViewerCurrentFrameRequestScheduler////////////////////////
class CurrentFrameFunctorArgs
    : public GenericThreadStartArgs
{
public:

    ViewIdx view;
    int time;
    RenderStatsPtr stats;
    ViewerInstance* viewer;
    U64 viewerHash;
    ViewerCurrentFrameRequestSchedulerStartArgsPtr request;
    ViewerCurrentFrameRequestSchedulerPrivate* scheduler;
    bool canAbort;
    NodePtr isRotoPaintRequest;
    RotoStrokeItemWPtr strokeItem;
    ViewerArgsPtr args[2];
    bool isRotoNeatRender;

    CurrentFrameFunctorArgs()
        : GenericThreadStartArgs()
        , view(0)
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

    CurrentFrameFunctorArgs(ViewIdx view,
                            int time,
                            const RenderStatsPtr& stats,
                            ViewerInstance* viewer,
                            U64 viewerHash,
                            ViewerCurrentFrameRequestSchedulerPrivate* scheduler,
                            bool canAbort,
                            const NodePtr& isRotoPaintRequest,
                            const RotoStrokeItemPtr& strokeItem,
                            bool isRotoNeatRender)
        : GenericThreadStartArgs()
        , view(view)
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

typedef boost::shared_ptr<CurrentFrameFunctorArgs> CurrentFrameFunctorArgsPtr;

class RenderCurrentFrameFunctorRunnable;
struct ViewerCurrentFrameRequestSchedulerPrivate
{
    ViewerInstance* viewer;
    QThreadPool* threadPool;
    QMutex producedFramesMutex;
    ProducedFrameSet producedFrames;
    QWaitCondition producedFramesNotEmpty;


    /**
     * Single thread used by the ViewerCurrentFrameRequestScheduler when the global thread pool has reached its maximum
     * activity to keep the renders responsive even if the thread pool is choking.
     **/
    ViewerCurrentFrameRequestRendererBackup backupThread;
    mutable QMutex currentFrameRenderTasksMutex;
    QWaitCondition currentFrameRenderTasksCond;
    std::list<RenderCurrentFrameFunctorRunnable*> currentFrameRenderTasks;

    // Used to attribute an age to each renderCurrentFrameRequest
    U64 ageCounter;

    ViewerCurrentFrameRequestSchedulerPrivate(ViewerInstance* viewer)
        : viewer(viewer)
        , threadPool( QThreadPool::globalInstance() )
        , producedFramesMutex()
        , producedFrames()
        , producedFramesNotEmpty()
        , backupThread()
        , currentFrameRenderTasksCond()
        , currentFrameRenderTasks()
        , ageCounter(0)
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

        while ( !currentFrameRenderTasks.empty() ) {
            currentFrameRenderTasksCond.wait(&currentFrameRenderTasksMutex);
        }
    }

    void notifyFrameProduced(const BufferableObjectPtrList& frames,
                             const RenderStatsPtr& stats,
                             U64 age)
    {
        QMutexLocker k(&producedFramesMutex);
        ProducedFrame p;

        p.frames = frames;
        p.age = age;
        p.stats = stats;
        producedFrames.insert(p);
        producedFramesNotEmpty.wakeOne();
    }

    void processProducedFrame(const RenderStatsPtr& stats, const BufferableObjectPtrList& frames);
};

class RenderCurrentFrameFunctorRunnable
    : public QRunnable
{
    CurrentFrameFunctorArgsPtr _args;

public:

    RenderCurrentFrameFunctorRunnable(const CurrentFrameFunctorArgsPtr& args)
        : _args(args)
    {
    }

    virtual ~RenderCurrentFrameFunctorRunnable()
    {
    }

    virtual void run() OVERRIDE FINAL
    {
#ifdef DEBUG
        boost_adaptbx::floating_point::exception_trapping trap(boost_adaptbx::floating_point::exception_trapping::division_by_zero |
                                                               boost_adaptbx::floating_point::exception_trapping::invalid |
                                                               boost_adaptbx::floating_point::exception_trapping::overflow);
#endif
        ///The viewer always uses the scheduler thread to regulate the output rate, @see ViewerInstance::renderViewer_internal
        ///it calls appendToBuffer by itself
        ViewerInstance::ViewerRenderRetCode stat = ViewerInstance::eViewerRenderRetCodeFail;
        BufferableObjectPtrList ret;

        try {
            if (!_args->isRotoPaintRequest || _args->isRotoNeatRender) {
                stat = _args->viewer->renderViewer(_args->view, QThread::currentThread() == qApp->thread(), false, _args->viewerHash, _args->canAbort,
                                                   NodePtr(), true, _args->args, _args->request, _args->stats);
            } else {
                stat = _args->viewer->getViewerArgsAndRenderViewer(_args->time, _args->canAbort, _args->view, _args->viewerHash, _args->isRotoPaintRequest, _args->strokeItem.lock(), _args->stats, &_args->args[0], &_args->args[1]);
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
                if (_args->args[i] && _args->args[i]->params) {
                    if (_args->args[i]->params->tiles.size() > 0) {
                        ret.push_back(_args->args[i]->params);
                    }
                }
            }
        }

        if (_args->request) {
#ifdef DEBUG
            for (BufferableObjectPtrList::iterator it = ret.begin(); it != ret.end(); ++it) {
                UpdateViewerParams* isParams = dynamic_cast<UpdateViewerParams*>( it->get() );
                assert(isParams);
                assert( !isParams->tiles.empty() );
                for (std::list<UpdateViewerParams::CachedTile>::iterator it2 = isParams->tiles.begin(); it2 != isParams->tiles.end(); ++it2) {
                    assert(it2->ramBuffer);
                }
            }
#endif
            _args->scheduler->notifyFrameProduced(ret, _args->stats, _args->request->age);
        } else {
            assert( QThread::currentThread() == qApp->thread() );
            _args->scheduler->processProducedFrame(_args->stats, ret);
        }

        ///This thread is done, clean-up its TLS
        appPTR->getAppTLS()->cleanupTLSForThread();


        _args->scheduler->removeRunnableTask(this);
    } // run
};


class ViewerCurrentFrameRequestSchedulerExecOnMT
    : public GenericThreadExecOnMainThreadArgs
{
public:

    RenderStatsPtr stats;
    BufferableObjectPtrList frames;

    ViewerCurrentFrameRequestSchedulerExecOnMT()
        : GenericThreadExecOnMainThreadArgs()
    {
    }

    virtual ~ViewerCurrentFrameRequestSchedulerExecOnMT()
    {
    }
};

typedef boost::shared_ptr<ViewerCurrentFrameRequestSchedulerExecOnMT> ViewerCurrentFrameRequestSchedulerExecOnMTPtr;

ViewerCurrentFrameRequestScheduler::ViewerCurrentFrameRequestScheduler(ViewerInstance* viewer)
    : GenericSchedulerThread()
    , _imp( new ViewerCurrentFrameRequestSchedulerPrivate(viewer) )
{
    setThreadName("ViewerCurrentFrameRequestScheduler");
}

ViewerCurrentFrameRequestScheduler::~ViewerCurrentFrameRequestScheduler()
{
    // Should've been stopped before anyway
    if (_imp->backupThread.quitThread(false)) {
        _imp->backupThread.waitForAbortToComplete_enforce_blocking();
    }
}

GenericSchedulerThread::TaskQueueBehaviorEnum
ViewerCurrentFrameRequestScheduler::tasksQueueBehaviour() const
{
    return eTaskQueueBehaviorProcessInOrder;
}

GenericSchedulerThread::ThreadStateEnum
ViewerCurrentFrameRequestScheduler::threadLoopOnce(const GenericThreadStartArgsPtr &inArgs)
{
    ThreadStateEnum state = eThreadStateActive;
    ViewerCurrentFrameRequestSchedulerStartArgsPtr args = boost::dynamic_pointer_cast<ViewerCurrentFrameRequestSchedulerStartArgs>(inArgs);

    assert(args);

#ifdef TRACE_CURRENT_FRAME_SCHEDULER
    qDebug() << getThreadName().c_str() << "Thread loop once, waiting for" << args->age << "to be produced";
#endif

    ///Wait for the work to be done
    ViewerCurrentFrameRequestSchedulerExecOnMTPtr mtArgs = boost::make_shared<ViewerCurrentFrameRequestSchedulerExecOnMT>();
    {
        QMutexLocker k(&_imp->producedFramesMutex);
        ProducedFrameSet::iterator found = _imp->producedFrames.end();
        for (ProducedFrameSet::iterator it = _imp->producedFrames.begin(); it != _imp->producedFrames.end(); ++it) {
            if (it->age == args->age) {
                found = it;
                break;
            }
        }

        while ( found == _imp->producedFrames.end() ) {
            state = resolveState();
            if ( (state == eThreadStateStopped) || (state == eThreadStateAborted) ) {
                //_imp->producedFrames.clear();
                break;
            }
            _imp->producedFramesNotEmpty.wait(&_imp->producedFramesMutex);

            for (ProducedFrameSet::iterator it = _imp->producedFrames.begin(); it != _imp->producedFrames.end(); ++it) {
                if (it->age == args->age) {
                    found = it;
                    break;
                }
            }
        }

        if ( found != _imp->producedFrames.end() ) {
#ifdef TRACE_CURRENT_FRAME_SCHEDULER
            qDebug() << getThreadName().c_str() << "Found" << args->age << "produced";
#endif

            mtArgs->frames = found->frames;
            mtArgs->stats = found->stats;

            // Erase from the produced frames all renders that are older that the age we want to render
            // since they are no longer going to be used.
            // Only do this if the render had frames it is not just a redraw
            //if (!found->frames.empty()) {
            //    ++found;
            //    _imp->producedFrames.erase(_imp->producedFrames.begin(), found);
            //} else {
            ++found;
            _imp->producedFrames.erase(_imp->producedFrames.begin(), found);
            //}
        } else {
#ifdef TRACE_CURRENT_FRAME_SCHEDULER
            qDebug() << getThreadName().c_str() << "Got aborted, skip waiting for" << args->age;
#endif
        }
    } // QMutexLocker k(&_imp->producedQueueMutex);


    if (state == eThreadStateActive) {
        state = resolveState();
    }
    // Do not uncomment the second part: if we don't update on the viewer aborted tasks then user will never even see valid images that were fully rendered
    if (state == eThreadStateStopped /*|| (state == eThreadStateAborted)*/ ) {
        return state;
    }

    requestExecutionOnMainThread(mtArgs);

#ifdef TRACE_CURRENT_FRAME_SCHEDULER
    qDebug() << getThreadName().c_str() << "Frame processed" << args->age;
#endif

    return state;
} // ViewerCurrentFrameRequestScheduler::threadLoopOnce

void
ViewerCurrentFrameRequestScheduler::executeOnMainThread(const GenericThreadExecOnMainThreadArgsPtr& inArgs)
{
    ViewerCurrentFrameRequestSchedulerExecOnMT* args = dynamic_cast<ViewerCurrentFrameRequestSchedulerExecOnMT*>( inArgs.get() );

    assert(args);
    if (args) {
        _imp->processProducedFrame(args->stats, args->frames);
    }
}

void
ViewerCurrentFrameRequestSchedulerPrivate::processProducedFrame(const RenderStatsPtr& stats,
                                                                const BufferableObjectPtrList& frames)
{
    assert( QThread::currentThread() == qApp->thread() );

    if ( !frames.empty() ) {
        viewer->aboutToUpdateTextures();
    }

    //bool hasDoneSomething = false;
    for (BufferableObjectPtrList::const_iterator it2 = frames.begin(); it2 != frames.end(); ++it2) {
        assert(*it2);
        UpdateViewerParamsPtr params = boost::dynamic_pointer_cast<UpdateViewerParams>(*it2);
        assert(params);
        if ( params && (params->tiles.size() >= 1) ) {
            if (stats) {
                double timeSpent;
                std::map<NodePtr, NodeRenderStats > ret = stats->getStats(&timeSpent);
                viewer->reportStats(0, ViewIdx(0), timeSpent, ret);
            }

            viewer->updateViewer(params);
        }
    }


    ///At least redraw the viewer, we might be here when the user removed a node upstream of the viewer.
    viewer->redrawViewer();
}

void
ViewerCurrentFrameRequestScheduler::onAbortRequested(bool keepOldestRender)
{
#ifdef TRACE_CURRENT_FRAME_SCHEDULER
    qDebug() << getThreadName().c_str() << "Received abort request";
#endif
    //This will make all processing nodes that call the abort() function return true
    //This function marks all active renders of the viewer as aborted (except the oldest one)
    //and each node actually check if the render has been aborted in EffectInstance::Implementation::aborted()
    _imp->viewer->markAllOnGoingRendersAsAborted(keepOldestRender);
    _imp->backupThread.abortThreadedTask();
}

void
ViewerCurrentFrameRequestScheduler::onQuitRequested(bool allowRestarts)
{
    _imp->backupThread.quitThread(allowRestarts);
}

void
ViewerCurrentFrameRequestScheduler::onWaitForThreadToQuit()
{
    _imp->waitForRunnableTasks();
    _imp->backupThread.waitForThreadToQuit_enforce_blocking();
}

void
ViewerCurrentFrameRequestScheduler::onWaitForAbortCompleted()
{
    _imp->waitForRunnableTasks();
    _imp->backupThread.waitForAbortToComplete_enforce_blocking();
}

void
ViewerCurrentFrameRequestScheduler::notifyFrameProduced(const BufferableObjectPtrList& frames,
                                                        const RenderStatsPtr& stats,
                                                        const ViewerCurrentFrameRequestSchedulerStartArgsPtr& request)
{
    _imp->notifyFrameProduced(frames, stats,  request->age);
}

void
ViewerCurrentFrameRequestScheduler::renderCurrentFrame(bool enableRenderStats,
                                                       bool canAbort)
{
    int frame = _imp->viewer->getTimeline()->currentFrame();
    int viewsCount = _imp->viewer->getRenderViewsCount();
    ViewIdx view = viewsCount > 0 ? _imp->viewer->getViewerCurrentView() : ViewIdx(0);
    U64 viewerHash = _imp->viewer->getHash();
    ViewerInstance::ViewerRenderRetCode status[2] = {
        ViewerInstance::eViewerRenderRetCodeFail, ViewerInstance::eViewerRenderRetCodeFail
    };

    // Do not render viewer that are not made visible by the user
    if ( !_imp->viewer->isViewerUIVisible() ) {
        return;
    }

    RenderStatsPtr stats;
    if (enableRenderStats) {
        stats.reset( new RenderStats(enableRenderStats) );
    }

    bool isTracking = _imp->viewer->isDoingPartialUpdates();
    NodePtr rotoPaintNode;
    RotoStrokeItemPtr curStroke;
    bool isDrawing = false;
    _imp->viewer->getApp()->getActiveRotoDrawingStroke(&rotoPaintNode, &curStroke, &isDrawing);

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

    ViewerArgsPtr args[2];
    if (!rotoPaintNode || isRotoNeatRender) {
        bool clearTexture[2] = {false, false};

        for (int i = 0; i < 2; ++i) {
            args[i] = boost::make_shared<ViewerArgs>();
            status[i] = _imp->viewer->getRenderViewerArgsAndCheckCache_public( frame, false, view, i, viewerHash, canAbort, rotoPaintNode, stats, args[i].get() );

            clearTexture[i] = status[i] == ViewerInstance::eViewerRenderRetCodeFail || status[i] == ViewerInstance::eViewerRenderRetCodeBlack;
            if (clearTexture[i] || args[i]->params->isViewerPaused) {
                //Just clear the viewer, nothing to do
                args[i]->params.reset();
            }

            if ( (status[i] == ViewerInstance::eViewerRenderRetCodeRedraw) && args[i]->params ) {
                //We must redraw (re-render) don't hold a pointer to the cached frame
                args[i]->params->tiles.clear();
            }
        }

        if (status[0] == ViewerInstance::eViewerRenderRetCodeFail && status[1] == ViewerInstance::eViewerRenderRetCodeFail) {
            _imp->viewer->disconnectViewer();
            return;
        }
        if (clearTexture[0]) {
            _imp->viewer->disconnectTexture(0, status[0] == ViewerInstance::eViewerRenderRetCodeFail);
        }
        if (clearTexture[0]) {
            _imp->viewer->disconnectTexture(1, status[1] == ViewerInstance::eViewerRenderRetCodeFail);
        }

        if ( (status[0] == ViewerInstance::eViewerRenderRetCodeRedraw) && (status[1] == ViewerInstance::eViewerRenderRetCodeRedraw) ) {
            _imp->viewer->redrawViewer();

            return;
        }

        bool hasTextureCached = false;
        for (int i = 0; i < 2; ++i) {
            if ( args[i]->params && (args[i]->params->nbCachedTile > 0) && ( args[i]->params->nbCachedTile == (int)args[i]->params->tiles.size() ) ) {
                hasTextureCached = true;
                break;
            }
        }
        if (hasTextureCached) {
            _imp->viewer->aboutToUpdateTextures();
        }

        for (int i = 0; i < 2; ++i) {
            if ( args[i]->params && (args[i]->params->nbCachedTile > 0) && ( args[i]->params->nbCachedTile == (int)args[i]->params->tiles.size() ) ) {
                /*
                   The texture was cached
                 */
                if ( stats && (i == 0) ) {
                    double timeSpent;
                    std::map<NodePtr, NodeRenderStats > statResults = stats->getStats(&timeSpent);
                    _imp->viewer->reportStats(frame, view, timeSpent, statResults);
                }
                _imp->viewer->updateViewer(args[i]->params);
                args[i].reset();
            }
        }
        if ( (!args[0] && !args[1]) ||
             ( !args[0] && ( status[0] == ViewerInstance::eViewerRenderRetCodeRender) && args[1] && ( status[1] == ViewerInstance::eViewerRenderRetCodeFail) ) ||
             ( !args[1] && ( status[1] == ViewerInstance::eViewerRenderRetCodeRender) && args[0] && ( status[0] == ViewerInstance::eViewerRenderRetCodeFail) ) ) {
            _imp->viewer->redrawViewer();

            return;
        }
    }
#ifdef BOOST_NO_CXX11_VARIADIC_TEMPLATES
    CurrentFrameFunctorArgsPtr functorArgs( new CurrentFrameFunctorArgs(view,
                                                                                        frame,
                                                                                        stats,
                                                                                        _imp->viewer,
                                                                                        viewerHash,
                                                                                        _imp.get(),
                                                                                        canAbort,
                                                                                        rotoPaintNode,
                                                                                        curStroke,
                                                                                        isRotoNeatRender) );
#else
    CurrentFrameFunctorArgsPtr functorArgs = boost::make_shared<CurrentFrameFunctorArgs>(view,
                                                                                                         frame,
                                                                                                         stats,
                                                                                                         _imp->viewer,
                                                                                                         viewerHash,
                                                                                                         _imp.get(),
                                                                                                         canAbort,
                                                                                                         rotoPaintNode,
                                                                                                         curStroke,
                                                                                                         isRotoNeatRender);
#endif
    functorArgs->args[0] = args[0];
    functorArgs->args[1] = args[1];

    if (appPTR->getCurrentSettings()->getNumberOfThreads() == -1) {
        RenderCurrentFrameFunctorRunnable task(functorArgs);
        task.run();
    } else {
        // Identify this render request with an age
        ViewerCurrentFrameRequestSchedulerStartArgsPtr request = boost::make_shared<ViewerCurrentFrameRequestSchedulerStartArgs>();
        request->age = _imp->ageCounter;

        // If we reached the max amount of age, reset to 0... should never happen anyway
        if ( _imp->ageCounter >= std::numeric_limits<U64>::max() ) {
            _imp->ageCounter = 0;
        } else {
            ++_imp->ageCounter;
        }

        startTask(request);
        functorArgs->request = request;

        /*
         * Let at least 1 free thread in the thread-pool to allow the renderer to use the thread pool if we use the thread-pool
         */
        int maxThreads = _imp->threadPool->maxThreadCount();

        //When painting, limit the number of threads to 1 to be sure strokes are painted in the right order
        if (rotoUse1Thread || isTracking) {
            maxThreads = 1;
        }
        if ( (maxThreads == 1) || (_imp->threadPool->activeThreadCount() >= maxThreads - 1) ) {
            _imp->backupThread.startTask(functorArgs);
        } else {
            RenderCurrentFrameFunctorRunnable* task = new RenderCurrentFrameFunctorRunnable(functorArgs);
            _imp->appendRunnableTask(task);
            _imp->threadPool->start(task);
        }
    }
} // ViewerCurrentFrameRequestScheduler::renderCurrentFrame

ViewerCurrentFrameRequestRendererBackup::ViewerCurrentFrameRequestRendererBackup()
    : GenericSchedulerThread()
{
    setThreadName("ViewerCurrentFrameRequestRendererBackup");
}

ViewerCurrentFrameRequestRendererBackup::~ViewerCurrentFrameRequestRendererBackup()
{
}

GenericSchedulerThread::ThreadStateEnum
ViewerCurrentFrameRequestRendererBackup::threadLoopOnce(const GenericThreadStartArgsPtr& inArgs)
{
    CurrentFrameFunctorArgsPtr args = boost::dynamic_pointer_cast<CurrentFrameFunctorArgs>(inArgs);

    assert(args);
    RenderCurrentFrameFunctorRunnable task(args);
    task.run();

    return eThreadStateActive;
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_OutputSchedulerThread.cpp"
