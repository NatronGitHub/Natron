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

GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
// /usr/local/include/boost/bind/arg.hpp:37:9: warning: unused typedef 'boost_static_assert_typedef_37' [-Wunused-local-typedef]
#include <boost/bind.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/algorithm/clamp.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON

#include <QtCore/QMetaType>
#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>
#include <QtCore/QCoreApplication>
#include <QtCore/QString>
#include <QtCore/QThreadPool>
#include <QtCore/QDebug>
#include <QtCore/QTextStream>
#include <QtCore/QRunnable>
#include <QtCore/QDir>

#include <QtConcurrentRun>


#include <SequenceParsing.h>

#include "Global/QtCompat.h"

#include "Engine/AppManager.h"
#include "Engine/AppInstance.h"
#include "Engine/EffectInstance.h"
#include "Engine/FrameViewRequest.h"
#include "Engine/ImageCacheKey.h"
#include "Engine/Image.h"
#include "Engine/ImageCacheEntry.h"
#include "Engine/KnobFile.h"
#include "Engine/Node.h"
#include "Engine/KnobItemsTable.h"
#include "Engine/OpenGLViewerI.h"
#include "Global/FStreamsSupport.h"
#include "Engine/GenericSchedulerThreadWatcher.h"
#include "Engine/Project.h"
#include "Engine/RenderStats.h"
#include "Engine/Settings.h"
#include "Engine/Timer.h"
#include "Engine/TimeLine.h"
#include "Engine/TreeRender.h"
#include "Engine/TLSHolder.h"
#include "Engine/RotoPaint.h"
#include "Engine/RotoStrokeItem.h"
#include "Engine/ViewIdx.h"
#include "Engine/ViewerNode.h"
#include "Engine/ViewerInstance.h"
#include "Engine/WriteNode.h"

#ifdef DEBUG
//#define TRACE_SCHEDULER
//#define TRACE_CURRENT_FRAME_SCHEDULER
#endif

#define NATRON_FPS_REFRESH_RATE_SECONDS 1.5

#define NATRON_SCHEDULER_ABORT_AFTER_X_UNSUCCESSFUL_ITERATIONS 5000

NATRON_NAMESPACE_ENTER;


struct BufferedFrameContainer_Compare
{
    bool operator()(const BufferedFrameContainerPtr& lhs, const BufferedFrameContainerPtr& rhs)
    {
        return lhs->time < rhs->time;
    }
};

typedef std::set<BufferedFrameContainerPtr, BufferedFrameContainer_Compare> FrameBuffer;


NATRON_NAMESPACE_ANONYMOUS_ENTER

class MetaTypesRegistration
{
public:
    inline MetaTypesRegistration()
    {
        qRegisterMetaType<BufferedFrameContainerPtr>("BufferedFrameContainerPtr");
    }
};

NATRON_NAMESPACE_ANONYMOUS_EXIT


static MetaTypesRegistration registration;
struct RenderThread
{
    boost::shared_ptr<RenderThreadTask> runnable;
};

typedef std::list<RenderThread> RenderThreads;


struct ProducedFrame
{
    BufferedFrameContainerPtr frames;
    U64 age;
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

    BufferedFrameContainerPtr frames;

    OutputSchedulerThreadExecMTArgs()
        : GenericThreadExecOnMainThreadArgs()
    {}

    virtual ~OutputSchedulerThreadExecMTArgs() {}
};


struct OutputSchedulerThreadPrivate
{
    OutputSchedulerThread* _publicInterface;

    FrameBuffer buf; //the frames rendered by the worker threads that needs to be rendered in order by the output device
    QWaitCondition bufEmptyCondition;
    mutable QMutex bufMutex;

    //doesn't need any protection since it never changes and is set in the constructor
    OutputSchedulerThread::ProcessFrameModeEnum mode; //is the frame to be processed on the main-thread (i.e OpenGL rendering) or on the scheduler thread
    boost::scoped_ptr<Timer> timer; // Timer regulating the engine execution. It is controlled by the GUI and MT-safe.
    boost::scoped_ptr<TimeLapse> renderTimer; // Timer used to report stats when rendering

    ///When the render threads are not using the appendToBuffer API, the scheduler has no way to know the rendering is finished
    ///but to count the number of frames rendered via notifyFrameRended which is called by the render thread.
    mutable QMutex renderFinishedMutex;
    U64 nFramesRendered;
    bool renderFinished; //< set to true when nFramesRendered = runArgs->lastFrame - runArgs->firstFrame + 1

    // Pointer to the args used in threadLoopOnce(), only usable from the scheduler thread
    boost::weak_ptr<OutputSchedulerThreadStartArgs> runArgs;
    

    mutable QMutex lastRunArgsMutex;
    std::vector<ViewIdx> lastPlaybackViewsToRender;
    RenderDirectionEnum lastPlaybackRenderDirection;

    ///Worker threads
    mutable QMutex renderThreadsMutex;
    RenderThreads renderThreads, quitRenderThreads;
    QWaitCondition allRenderThreadsInactiveCond; // wait condition to make sure all render threads are asleep


    // Protects lastFrameRequested & expectedFrameToRender & schedulerRenderDirection
    QMutex lastFrameRequestedMutex;

    // The last frame requested to render
    TimeValue lastFrameRequested;

    // The frame expected by the scheduler thread to be rendered
    TimeValue expectedFrameToRender;

    // The direction of the scheduler
    RenderDirectionEnum schedulerRenderDirection;

    NodeWPtr outputEffect; //< The effect used as output device

    RenderEngine* engine;

    QMutex bufferedOutputMutex;
    int lastBufferedOutputSize;

    struct RenderSequenceArgs
    {
        std::vector<ViewIdx> viewsToRender;
        TimeValue firstFrame;
        TimeValue lastFrame;
        TimeValue frameStep;
        bool useStats;
        bool blocking;
        RenderDirectionEnum direction;
    };

    mutable QMutex sequentialRenderQueueMutex;
    std::list<RenderSequenceArgs> sequentialRenderQueue;


    OutputSchedulerThreadPrivate(RenderEngine* engine,
                                 OutputSchedulerThread* publicInterface,
                                 const NodePtr& effect,
                                 OutputSchedulerThread::ProcessFrameModeEnum mode)
        : _publicInterface(publicInterface)
        , buf()
        , bufEmptyCondition()
        , bufMutex()
        , mode(mode)
        , timer(new Timer)
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
        , lastFrameRequestedMutex()
        , lastFrameRequested(0)
        , expectedFrameToRender(0)
        , schedulerRenderDirection(eRenderDirectionForward)
        , outputEffect(effect)
        , engine(engine)
        , bufferedOutputMutex()
        , lastBufferedOutputSize(0)
        , sequentialRenderQueueMutex()
        , sequentialRenderQueue()
    {
    }

    void validateRenderSequenceArgs(RenderSequenceArgs& args) const;

    void launchNextSequentialRender();

    void appendBufferedFrame(const BufferedFrameContainerPtr& object)
    {
        ///Private, shouldn't lock
        assert( !bufMutex.tryLock() );
#ifdef TRACE_SCHEDULER
        qDebug() << "Parallel Render Thread: Rendered Frame:" << object->time << " View:" << (int)object->view << idStr;
#endif
        buf.insert(object);
    }


    BufferedFrameContainerPtr getFromBufferAndErase(TimeValue time)
    {
        // Private, shouldn't lock
        assert( !bufMutex.tryLock() );

        BufferedFrameContainerPtr ret;
        {
            BufferedFrameContainerPtr stub(new BufferedFrameContainer);
            stub->time = time;
            FrameBuffer::iterator found = buf.find(stub);
            if ( found != buf.end() ) {
                ret = *found;
                buf.erase(found);

            }
        }
        return ret;
    } // getFromBufferAndErase

    void startRunnable(const boost::shared_ptr<RenderThreadTask>& runnable)
    {
        assert( !renderThreadsMutex.tryLock() );
        RenderThread r;
        r.runnable = runnable;

        // See bug https://bugreports.qt.io/browse/QTBUG-20251
        // The Qt thread-pool mem-leaks the runnable if using release/reserveThread
        // Instead we explicitly manage them and ensure they do not hold any external strong refs.
        runnable->setAutoDelete(false);
        renderThreads.push_back(r);
        QThreadPool::globalInstance()->start(runnable.get());
    }

    RenderThreads::iterator getRunnableIterator(RenderThreadTask* runnable)
    {
        // Private shouldn't lock
        assert( !renderThreadsMutex.tryLock() );
        for (RenderThreads::iterator it = renderThreads.begin(); it != renderThreads.end(); ++it) {
            if (it->runnable.get() == runnable) {
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
                                       TimeValue frame,
                                       TimeValue firstFrame,
                                       TimeValue lastFrame,
                                       TimeValue frameStep,
                                       TimeValue* nextFrame,
                                       RenderDirectionEnum* newDirection);
    static void getNearestInSequence(RenderDirectionEnum direction,
                                     TimeValue frame,
                                     TimeValue firstFrame,
                                     TimeValue lastFrame,
                                     TimeValue* nextFrame);


    void waitForRenderThreadsToQuitInternal()
    {
        assert( !renderThreadsMutex.tryLock() );
        while (renderThreads.size() > 0) {
            allRenderThreadsInactiveCond.wait(&renderThreadsMutex, 200);
        }
        quitRenderThreads.clear();
    }

    int getNActiveRenderThreads() const
    {
        // Private shouldn't lock
        assert( !renderThreadsMutex.tryLock() );
        return (int)renderThreads.size();
    }


    void waitForRenderThreadsToQuit()
    {
        QMutexLocker l(&renderThreadsMutex);
        waitForRenderThreadsToQuitInternal();
    }
};

OutputSchedulerThread::OutputSchedulerThread(RenderEngine* engine,
                                             const NodePtr& effect,
                                             ProcessFrameModeEnum mode)
    : GenericSchedulerThread()
    , _imp( new OutputSchedulerThreadPrivate(engine, this, effect, mode) )
{
    QObject::connect( _imp->timer.get(), SIGNAL(fpsChanged(double,double)), _imp->engine, SIGNAL(fpsChanged(double,double)) );


    setThreadName("Scheduler thread");
}

OutputSchedulerThread::~OutputSchedulerThread()
{

    // Make sure all tasks are finished, there will be a deadlock here if that's not the case.
    _imp->waitForRenderThreadsToQuit();
}

NodePtr
OutputSchedulerThread::getOutputNode() const
{
    return _imp->outputEffect.lock();
}

bool
OutputSchedulerThreadPrivate::getNextFrameInSequence(PlaybackModeEnum pMode,
                                                     RenderDirectionEnum direction,
                                                     TimeValue frame,
                                                     TimeValue firstFrame,
                                                     TimeValue lastFrame,
                                                     TimeValue frameStep,
                                                     TimeValue* nextFrame,
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
                *nextFrame = TimeValue(firstFrame + frameStep);
            } else {
                *nextFrame  = TimeValue(lastFrame - frameStep);
            }
            break;
        case ePlaybackModeBounce:
            if (direction == eRenderDirectionForward) {
                *newDirection = eRenderDirectionBackward;
                *nextFrame  = TimeValue(lastFrame - frameStep);
            } else {
                *newDirection = eRenderDirectionForward;
                *nextFrame  = TimeValue(firstFrame + frameStep);
            }
            break;
        case ePlaybackModeOnce:
        default:
            if (direction == eRenderDirectionForward) {
                *nextFrame = TimeValue(firstFrame + frameStep);
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
                *nextFrame = TimeValue(lastFrame - frameStep);
            }
            break;
        case ePlaybackModeBounce:
            if (direction == eRenderDirectionForward) {
                *newDirection = eRenderDirectionBackward;
                *nextFrame = TimeValue(lastFrame - frameStep);
            } else {
                *newDirection = eRenderDirectionForward;
                *nextFrame = TimeValue(firstFrame + frameStep);
            }
            break;
        case ePlaybackModeOnce:
        default:
            if (direction == eRenderDirectionForward) {
                return false;
            } else {
                *nextFrame = TimeValue(lastFrame - frameStep);
                break;
            }
        }
    } else {
        if (direction == eRenderDirectionForward) {
            *nextFrame = TimeValue(frame + frameStep);
        } else {
            *nextFrame = TimeValue(frame - frameStep);
        }
    }

    return true;
} // OutputSchedulerThreadPrivate::getNextFrameInSequence

void
OutputSchedulerThreadPrivate::getNearestInSequence(RenderDirectionEnum direction,
                                                   TimeValue frame,
                                                   TimeValue firstFrame,
                                                   TimeValue lastFrame,
                                                   TimeValue* nextFrame)
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


void
OutputSchedulerThread::startTasksFromLastStartedFrame()
{

    TimeValue frame;
    bool canContinue;

    {
        OutputSchedulerThreadStartArgsPtr args = _imp->runArgs.lock();

        PlaybackModeEnum pMode = _imp->engine->getPlaybackMode();

        {
            QMutexLocker l(&_imp->lastFrameRequestedMutex);
            frame = _imp->lastFrameRequested;

            if ( (args->firstFrame == args->lastFrame) && (frame == args->firstFrame) ) {
                return;
            }

            RenderDirectionEnum newDirection = args->direction;
            ///If startingTime is already taken into account in the framesToRender, push new frames from the last one in the stack instead
            canContinue = OutputSchedulerThreadPrivate::getNextFrameInSequence(pMode, args->direction, frame,
                                                                               args->firstFrame, args->lastFrame, args->frameStep, &frame, &newDirection);
            if (newDirection != args->direction) {
                args->direction = newDirection;
            }
            _imp->lastFrameRequested = frame;
        }
    }

    if (canContinue) {
        startTasks(frame);
    }
} // startTasksFromLastStartedFrame

void
OutputSchedulerThread::startTasks(TimeValue startingFrame)
{

    OutputSchedulerThreadStartArgsPtr args = _imp->runArgs.lock();

    PlaybackModeEnum pMode = _imp->engine->getPlaybackMode();
    if (args->firstFrame == args->lastFrame) {
        boost::shared_ptr<RenderThreadTask> task(createRunnable(startingFrame, args->enableRenderStats, args->viewsToRender));
        {
            QMutexLocker k(&_imp->renderThreadsMutex);
            _imp->startRunnable(task);
        }
        QMutexLocker k(&_imp->lastFrameRequestedMutex);
        _imp->lastFrameRequested = startingFrame;
    } else {

        // For now just run one frame concurrently, it is better to try to render one frame the fastest
        const int nConcurrentFrames = 1;

        TimeValue frame = startingFrame;
        RenderDirectionEnum newDirection = args->direction;

        for (int i = 0; i < nConcurrentFrames; ++i) {

            boost::shared_ptr<RenderThreadTask> task(createRunnable(frame, args->enableRenderStats, args->viewsToRender));
            {
                QMutexLocker k(&_imp->renderThreadsMutex);
                _imp->startRunnable(task);
            }
            
            {
                QMutexLocker k(&_imp->lastFrameRequestedMutex);
                _imp->lastFrameRequested = frame;
            }

            if ( !OutputSchedulerThreadPrivate::getNextFrameInSequence(pMode, args->direction, frame,
                                                                       args->firstFrame, args->lastFrame, args->frameStep, &frame, &newDirection) ) {
                break;
            }
        }
        if (newDirection != args->direction) {
            args->direction = newDirection;
        }
    }
} // OutputSchedulerThread::startTasks


void
OutputSchedulerThread::notifyThreadAboutToQuit(RenderThreadTask* thread)
{
    QMutexLocker l(&_imp->renderThreadsMutex);
    RenderThreads::iterator found = _imp->getRunnableIterator(thread);

    if ( found != _imp->renderThreads.end() ) {
        _imp->quitRenderThreads.push_back(*found);
        _imp->renderThreads.erase(found);
        _imp->allRenderThreadsInactiveCond.wakeOne();
    }
}

void
OutputSchedulerThread::startRender()
{
    if ( isFPSRegulationNeeded() ) {
        _imp->timer->playState = ePlayStateRunning;
    }

    // Start measuring
    _imp->renderTimer.reset(new TimeLapse);

    // We will push frame to renders starting at startingFrame.
    // They will be in the range determined by firstFrame-lastFrame
    TimeValue startingFrame;
    TimeValue firstFrame, lastFrame;
    TimeValue frameStep;
    RenderDirectionEnum direction;

    {
        OutputSchedulerThreadStartArgsPtr args = _imp->runArgs.lock();

        firstFrame = args->firstFrame;
        lastFrame = args->lastFrame;
        frameStep = args->frameStep;
        startingFrame = timelineGetTime();
        direction = args->direction;
    }
    aboutToStartRender();
    
    // Notify everyone that the render is started
    _imp->engine->s_renderStarted(direction == eRenderDirectionForward);



    ///If the output effect is sequential (only WriteFFMPEG for now)
    NodePtr node = _imp->outputEffect.lock();
    WriteNodePtr isWrite = toWriteNode( node->getEffectInstance() );
    if (isWrite) {
        NodePtr embeddedWriter = isWrite->getEmbeddedWriter();
        if (embeddedWriter) {
            node = embeddedWriter;
        }
    }
    SequentialPreferenceEnum pref = node->getEffectInstance()->getSequentialPreference();
    if ( (pref == eSequentialPreferenceOnlySequential) || (pref == eSequentialPreferencePreferSequential) ) {
        RenderScale scaleOne(1.);
        ActionRetCodeEnum stat = node->getEffectInstance()->beginSequenceRender_public(firstFrame,
                                                                                       lastFrame,
                                                                                       frameStep,
                                                                                       false /*interactive*/,
                                                                                       scaleOne,
                                                                                       true /*isSequentialRender*/,
                                                                                       false /*isRenderResponseToUserInteraction*/,
                                                                                       false /*draftMode*/,
                                                                                       ViewIdx(0),
                                                                                       eRenderBackendTypeCPU /*useOpenGL*/,
                                                                                       EffectOpenGLContextDataPtr());
        if (isFailureRetCode(stat)) {
            
            _imp->engine->abortRenderingNoRestart();
            
            return;
        }
    }

    {
        QMutexLocker k(&_imp->lastFrameRequestedMutex);
        _imp->expectedFrameToRender = startingFrame;
        _imp->schedulerRenderDirection = direction;
    }

    startTasks(startingFrame);


} // OutputSchedulerThread::startRender

void
OutputSchedulerThread::stopRender()
{
    _imp->timer->playState = ePlayStatePause;

    // Remove all current threads so the new render doesn't have many threads concurrently trying to do the same thing at the same time
    _imp->waitForRenderThreadsToQuit();

    ///If the output effect is sequential (only WriteFFMPEG for now)
    NodePtr node = _imp->outputEffect.lock();
    WriteNodePtr isWrite = toWriteNode( node->getEffectInstance() );
    if (isWrite) {
        NodePtr embeddedWriter = isWrite->getEmbeddedWriter();
        if (embeddedWriter) {
            node = embeddedWriter;
        }
    }
    SequentialPreferenceEnum pref = node->getEffectInstance()->getSequentialPreference();
    if ( (pref == eSequentialPreferenceOnlySequential) || (pref == eSequentialPreferencePreferSequential) ) {
        TimeValue firstFrame, lastFrame, frameStep;

        {
            QMutexLocker k(&_imp->lastRunArgsMutex);
            OutputSchedulerThreadStartArgsPtr args = _imp->runArgs.lock();
            firstFrame = args->firstFrame;
            lastFrame = args->lastFrame;
            frameStep = args->frameStep;
        }

        RenderScale scaleOne(1.);
        ignore_result( node->getEffectInstance()->endSequenceRender_public(firstFrame,
                                                                           lastFrame,
                                                                           frameStep,
                                                                           !appPTR->isBackground(),
                                                                           scaleOne, true,
                                                                           !appPTR->isBackground(),
                                                                           false,
                                                                           ViewIdx(0),
                                                                           eRenderBackendTypeCPU /*use OpenGL render*/,
                                                                           EffectOpenGLContextDataPtr()) );
    }
    
    
    bool wasAborted = isBeingAborted();


    ///Notify everyone that the render is finished
    _imp->engine->s_renderFinished(wasAborted ? 1 : 0);

    // When playing once disable auto-restart
    if (!wasAborted && _imp->engine->getPlaybackMode() == ePlaybackModeOnce) {
        _imp->engine->setPlaybackAutoRestartEnabled(false);
    }


    {
        QMutexLocker k(&_imp->bufMutex);
        _imp->buf.clear();
    }

    _imp->renderTimer.reset();

    // Remove the render request and launch any pending renders
    {
        QMutexLocker k(&_imp->sequentialRenderQueueMutex);
        assert(!_imp->sequentialRenderQueue.empty());
        if (!_imp->sequentialRenderQueue.empty()) {
            _imp->sequentialRenderQueue.pop_front();
        }
    }
    _imp->launchNextSequentialRender();

} // OutputSchedulerThread::stopRender

GenericSchedulerThread::ThreadStateEnum
OutputSchedulerThread::threadLoopOnce(const GenericThreadStartArgsPtr &inArgs)
{
    OutputSchedulerThreadStartArgsPtr args = boost::dynamic_pointer_cast<OutputSchedulerThreadStartArgs>(inArgs);

    assert(args);
    _imp->runArgs = args;

    ThreadStateEnum state = eThreadStateActive;
    TimeValue expectedTimeToRenderPreviousIteration = TimeValue(INT_MIN);

    // This is the number of time this thread was woken up by a render thread because a frame was available for processing,
    // but this is not the frame this thread expects to render. If it reaches a certain amount, we detected a stall and abort.
    int nbIterationsWithoutProcessing = 0;

    startRender();

    for (;; ) {

        // When set to true, we don't sleep in the bufEmptyCondition but in the startCondition instead, indicating
        //we finished a render
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

        // This is the frame that the scheduler expects to process now
        TimeValue expectedTimeToRender;

        while (!bufferEmpty) {

            state = resolveState();
            if ( (state == eThreadStateAborted) || (state == eThreadStateStopped) ) {
                // Do not wait in the buf wait condition and go directly into the stopEngine()
                renderFinished = true;
                break;
            }


            {
                QMutexLocker k(&_imp->lastFrameRequestedMutex);
                expectedTimeToRender = _imp->expectedFrameToRender;
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
            boost::shared_ptr<OutputSchedulerThreadExecMTArgs> framesToRender( new OutputSchedulerThreadExecMTArgs() );
            {
                QMutexLocker l(&_imp->bufMutex);
                framesToRender->frames = _imp->getFromBufferAndErase(expectedTimeToRender);
            }

            if ( !framesToRender->frames ) {
                // The expected frame is not yet ready, go to sleep again
                expectedTimeToRenderPreviousIteration = expectedTimeToRender;
                break;
            }

#ifdef TRACE_SCHEDULER
            qDebug() << "Scheduler Thread: received frame to process" << expectedTimeToRender;
#endif

            TimeValue nextFrameToRender(-1.);
            RenderDirectionEnum newDirection = eRenderDirectionForward;

            if (!renderFinished) {
                ///////////
                /////Refresh frame range if needed (for viewers)


                TimeValue firstFrame, lastFrame;
                getFrameRangeToRender(firstFrame, lastFrame);


                RenderDirectionEnum timelineDirection;
                TimeValue frameStep;

                // Refresh the firstframe/lastFrame as they might have changed on the timeline
                args->firstFrame = firstFrame;
                args->lastFrame = lastFrame;


                timelineDirection = _imp->schedulerRenderDirection;
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
                    _imp->schedulerRenderDirection = newDirection;
                }

                if (!renderFinished) {
                    {
                        QMutexLocker k(&_imp->lastFrameRequestedMutex);
                        _imp->expectedFrameToRender = nextFrameToRender;
                    }


                    startTasksFromLastStartedFrame();

                }
            } // if (!renderFinished) {

            if (_imp->timer->playState == ePlayStateRunning) {
                _imp->timer->waitUntilNextFrameIsDue(); // timer synchronizing with the requested fps
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
                TimeValue timelineCurrentTime = timelineGetTime();
                if (timelineCurrentTime != expectedTimeToRender) {
                    timelineGoTo(timelineCurrentTime);
                } else {
                    timelineGoTo(nextFrameToRender);
                }
            }

            ////////////
            /////At this point the frame has been processed by the output device

            assert( framesToRender->frames );
            if (getSchedulingPolicy() == eSchedulingPolicyOrdered) {
                notifyFrameRendered(framesToRender->frames, eSchedulingPolicyOrdered);
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
            // Wait here for more frames to be rendered, we will be woken up once appendToBuffer is called
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
    {
        QMutexLocker l(&_imp->renderThreadsMutex);
        for (RenderThreads::iterator it = _imp->renderThreads.begin(); it != _imp->renderThreads.end(); ++it) {
            it->runnable->abortRender();
        }
    }

    // If the scheduler is asleep waiting for the buffer to be filling up, we post a fake request
    // that will not be processed anyway because the first thing it does is checking for abort
    {
        QMutexLocker l2(&_imp->bufMutex);
        _imp->bufEmptyCondition.wakeOne();
    }
}

void
OutputSchedulerThread::executeOnMainThread(const ExecOnMTArgsPtr& inArgs)
{
    OutputSchedulerThreadExecMTArgs* args = dynamic_cast<OutputSchedulerThreadExecMTArgs*>( inArgs.get() );

    assert(args);
    processFrame(args->frames);
}

void
OutputSchedulerThread::notifyFrameRendered(const BufferedFrameContainerPtr& frameContainer,
                                           SchedulingPolicyEnum policy)
{

    // This function only supports

    // Report render stats if desired
    NodePtr effect = _imp->outputEffect.lock();
    for (std::list<BufferedFramePtr>::const_iterator it = frameContainer->frames.begin(); it != frameContainer->frames.end(); ++it) {
        if ((*it)->stats && (*it)->stats->isInDepthProfilingEnabled()) {
            _imp->engine->reportStats(frameContainer->time , (*it)->stats);
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
        {
            QMutexLocker l(&_imp->renderFinishedMutex);
            ++_imp->nFramesRendered;

            nbTotalFrames = std::ceil( (double)(runArgs->lastFrame - runArgs->firstFrame + 1) / runArgs->frameStep );
            nbFramesRendered = _imp->nFramesRendered;


            if (_imp->nFramesRendered != nbTotalFrames) {
                startTasksFromLastStartedFrame();
            } else {
                _imp->renderFinished = true;
                l.unlock();

                // Notify the scheduler rendering is finished by append a fake frame to the buffer
                {
                    QMutexLocker bufLocker (&_imp->bufMutex);
                    _imp->bufEmptyCondition.wakeOne();
                }
            }
        }
    } else {
        nbTotalFrames = std::floor( (double)(runArgs->lastFrame - runArgs->firstFrame + 1) / runArgs->frameStep );
        if (runArgs->direction == eRenderDirectionForward) {
            nbFramesRendered = (frameContainer->time - runArgs->firstFrame) / runArgs->frameStep;
        } else {
            nbFramesRendered = (runArgs->lastFrame - frameContainer->time) / runArgs->frameStep;
        }
    } // if (policy == eSchedulingPolicyFFA) {

    double percentage = 0.;
    assert(nbTotalFrames > 0);
    if (nbTotalFrames != 0) {
        QMutexLocker k(&_imp->renderFinishedMutex);
        percentage = (double)_imp->nFramesRendered / nbTotalFrames;
    }
    assert(_imp->renderTimer);
    double timeSpentSinceStartSec = _imp->renderTimer->getTimeSinceCreation();
    double estimatedFps = (double)nbFramesRendered / timeSpentSinceStartSec;
    double timeRemaining = timeSpentSinceStartSec * (1. - percentage);

    // If running in background, notify to the pipe that we rendered a frame
    if (isBackground) {
        QString longMessage;
        QTextStream ts(&longMessage);
        QString frameStr = QString::number(frameContainer->time);
        QString fpsStr = QString::number(estimatedFps, 'f', 1);
        QString percentageStr = QString::number(percentage * 100, 'f', 1);
        QString timeRemainingStr = Timer::printAsTime(timeRemaining, true);

        ts << effect->getScriptName_mt_safe().c_str() << tr(" ==> Frame: ");
        ts << frameStr << tr(", Progress: ") << percentageStr << "%, " << fpsStr << tr(" Fps, Time Remaining: ") << timeRemainingStr;

        QString shortMessage = QString::fromUtf8(kFrameRenderedStringShort) + frameStr + QString::fromUtf8(kProgressChangedStringShort) + QString::number(percentage);
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
    _imp->engine->s_frameRendered(frameContainer->time, percentage);


    // Call Python after frame ranedered callback if policy is ordered, otherwise this has been called on the render thread
    if (policy == eSchedulingPolicyOrdered) {
        runAfterFrameRenderedCallback(frameContainer->time);
    }

} // OutputSchedulerThread::notifyFrameRendered

void
OutputSchedulerThread::appendToBuffer(const BufferedFrameContainerPtr& frame)
{
    // Called by the scheduler thread when an image is rendered
    
    QMutexLocker l(&_imp->bufMutex);
    _imp->appendBufferedFrame(frame);
    
    ///Wake up the scheduler thread that an image is available if it is asleep so it can process it.
    _imp->bufEmptyCondition.wakeOne();


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
OutputSchedulerThread::getLastRunArgs(RenderDirectionEnum* direction,
                                      std::vector<ViewIdx>* viewsToRender) const
{
    QMutexLocker k(&_imp->lastRunArgsMutex);

    *direction = _imp->lastPlaybackRenderDirection;
    *viewsToRender = _imp->lastPlaybackViewsToRender;
}

void
OutputSchedulerThreadPrivate::validateRenderSequenceArgs(RenderSequenceArgs& args) const
{
    NodePtr treeRoot = engine->getOutput();


    if (args.viewsToRender.empty()) {
        int viewsCount = treeRoot->getApp()->getProject()->getProjectViewsCount();
        args.viewsToRender.resize(viewsCount);
        for (int i = 0; i < viewsCount; ++i) {
            args.viewsToRender[i] = ViewIdx(i);
        }
    }

    // The effect is sequential (e.g: WriteFFMPEG), and thus cannot render multiple views, we have to choose one
    SequentialPreferenceEnum sequentiallity = treeRoot->getEffectInstance()->getSequentialPreference();
    bool canOnlyHandleOneView = sequentiallity == eSequentialPreferenceOnlySequential || sequentiallity == eSequentialPreferencePreferSequential;

    const ViewIdx mainView(0);

    if (canOnlyHandleOneView) {
        if (args.viewsToRender.size() != 1) {
            args.viewsToRender.clear();
            args.viewsToRender.push_back(mainView);
        }
    }

    KnobIPtr outputFileNameKnob = treeRoot->getKnobByName(kOfxImageEffectFileParamName);
    KnobFilePtr outputFileName = toKnobFile(outputFileNameKnob);
    std::string pattern = outputFileName ? outputFileName->getRawFileName() : std::string();

    if ( treeRoot->getEffectInstance()->isViewAware() ) {

        //If the Writer is view aware, check if it wants to render all views at once or not
        std::size_t foundViewPattern = pattern.find_first_of("%v");
        if (foundViewPattern == std::string::npos) {
            foundViewPattern = pattern.find_first_of("%V");
        }
        if (foundViewPattern == std::string::npos) {
            // No view pattern
            // all views will be overwritten to the same file
            // If this is WriteOIIO, check the parameter "viewsSelector" to determine if the user wants to encode all
            // views to a single file or not
            KnobIPtr viewsKnob = treeRoot->getKnobByName(kWriteOIIOParamViewsSelector);
            bool hasViewChoice = false;
            if ( viewsKnob && !viewsKnob->getIsSecret() ) {
                KnobChoicePtr viewsChoice = toKnobChoice(viewsKnob);
                if (viewsChoice) {
                    hasViewChoice = true;
                    int viewChoice_i = viewsChoice->getValue();
                    if (viewChoice_i == 0) { // the "All" choice
                        args.viewsToRender.clear();
                        // note: if the plugin renders all views to a single file, then rendering view 0 will do the job.
                        args.viewsToRender.push_back( ViewIdx(0) );
                    } else {
                        //The user has specified a view
                        args.viewsToRender.clear();
                        assert(viewChoice_i >= 1);
                        args.viewsToRender.push_back( ViewIdx(viewChoice_i - 1) );
                    }
                }
            }
            if (!hasViewChoice) {
                if (args.viewsToRender.size() > 1) {
                    std::string mainViewName;
                    const std::vector<std::string>& viewNames = treeRoot->getApp()->getProject()->getProjectViewNames();
                    if ( mainView < (int)viewNames.size() ) {
                        mainViewName = viewNames[mainView];
                    }
                    QString message = treeRoot->tr("%1 does not support multi-view, only the view %2 will be rendered.")
                    .arg( QString::fromUtf8( treeRoot->getLabel_mt_safe().c_str() ) )
                    .arg( QString::fromUtf8( mainViewName.c_str() ) );
                    if (!args.blocking) {
                        message.append( QChar::fromLatin1('\n') );
                        message.append( QString::fromUtf8("You can use the %v or %V indicator in the filename to render to separate files.\n"
                                                          "Would you like to continue?") );
                        StandardButtonEnum rep = Dialogs::questionDialog(treeRoot->tr("Multi-view support").toStdString(), message.toStdString(), false, StandardButtons(eStandardButtonOk | eStandardButtonCancel), eStandardButtonOk);
                        if (rep != eStandardButtonOk) {
                            return;
                        }
                    } else {
                        Dialogs::warningDialog( treeRoot->tr("Multi-view support").toStdString(), message.toStdString() );
                    }
                }
                // Render the main-view only...
                args.viewsToRender.clear();
                args.viewsToRender.push_back(mainView);
            }
        } else {
            // The user wants to write each view into a separate file
            // This will disregard the content of kWriteOIIOParamViewsSelector and the Writer
            // should write one view per-file.
        }


    } else { // !isViewAware
        if (args.viewsToRender.size() > 1) {
            std::string mainViewName;
            const std::vector<std::string>& viewNames = treeRoot->getApp()->getProject()->getProjectViewNames();
            if ( mainView < (int)viewNames.size() ) {
                mainViewName = viewNames[mainView];
            }
            QString message = treeRoot->tr("%1 does not support multi-view, only the view %2 will be rendered.")
            .arg( QString::fromUtf8( treeRoot->getLabel_mt_safe().c_str() ) )
            .arg( QString::fromUtf8( mainViewName.c_str() ) );
            if (!args.blocking) {
                message.append( QChar::fromLatin1('\n') );
                message.append( QString::fromUtf8("You can use the %v or %V indicator in the filename to render to separate files.\n"
                                                  "Would you like to continue?") );
                StandardButtonEnum rep = Dialogs::questionDialog(treeRoot->tr("Multi-view support").toStdString(), message.toStdString(), false, StandardButtons(eStandardButtonOk | eStandardButtonCancel), eStandardButtonOk);
                if (rep != eStandardButtonOk) {
                    // Notify progress that we were aborted
                    engine->s_renderFinished(1);

                    return;
                }
            } else {
                Dialogs::warningDialog( treeRoot->tr("Multi-view support").toStdString(), message.toStdString() );
            }
        }
    }

    // For a writer, make sure that the output directory path exists
    if (outputFileNameKnob) {
        std::string patternCpy = pattern;
        std::string path = SequenceParsing::removePath(patternCpy);
        std::map<std::string, std::string> env;
        treeRoot->getApp()->getProject()->getEnvironmentVariables(env);
        Project::expandVariable(env, path);
        if ( !path.empty() ) {
            QDir().mkpath( QString::fromUtf8( path.c_str() ) );
        }
    }


    if (args.firstFrame != args.lastFrame && !treeRoot->getEffectInstance()->isVideoWriter() && treeRoot->getEffectInstance()->isWriter()) {
        // We render a sequence, check that the user wants to render multiple images
        // Look first for # character
        std::size_t foundHash = pattern.find_first_of("#");
        if (foundHash == std::string::npos) {
            // Look for printf style numbering
            QRegExp exp(QString::fromUtf8("%[0-9]*d"));
            QString qp(QString::fromUtf8(pattern.c_str()));
            if (!qp.contains(exp)) {
                QString message = treeRoot->tr("You are trying to render the frame range [%1 - %2] but you did not specify any hash ('#') character(s) or printf-like format ('%d') for the padding. This will result in the same image being overwritten multiple times.").arg(args.firstFrame).arg(args.lastFrame);
                if (!args.blocking) {
                    message += treeRoot->tr("Would you like to continue?");
                    StandardButtonEnum rep = Dialogs::questionDialog(treeRoot->tr("Image Sequence").toStdString(), message.toStdString(), false, StandardButtons(eStandardButtonOk | eStandardButtonCancel), eStandardButtonOk);
                    if (rep != eStandardButtonOk) {
                        // Notify progress that we were aborted
                        engine->s_renderFinished(1);

                        return;
                    }
                } else {
                    Dialogs::warningDialog( treeRoot->tr("Image Sequence").toStdString(), message.toStdString() );
                }
                
            }
        }
    }
} // validateRenderSequenceArgs

void
OutputSchedulerThreadPrivate::launchNextSequentialRender()
{
    RenderSequenceArgs args;
    {
        QMutexLocker k(&sequentialRenderQueueMutex);
        if (sequentialRenderQueue.empty()) {
            return;
        }
        args = sequentialRenderQueue.front();
    }


    {
        QMutexLocker k(&lastRunArgsMutex);
        lastPlaybackRenderDirection = args.direction;
        lastPlaybackViewsToRender = args.viewsToRender;
    }
    if (args.direction == eRenderDirectionForward) {
        _publicInterface->timelineGoTo(args.firstFrame);
    } else {
        _publicInterface->timelineGoTo(args.lastFrame);
    }

    OutputSchedulerThreadStartArgsPtr threadArgs( new OutputSchedulerThreadStartArgs(args.blocking, args.useStats, args.firstFrame, args.lastFrame, args.frameStep, args.viewsToRender, args.direction) );

    {
        QMutexLocker k(&renderFinishedMutex);
        nFramesRendered = 0;
        renderFinished = false;
    }

    _publicInterface->startTask(threadArgs);

} // launchNextSequentialRender

void
OutputSchedulerThread::renderFrameRange(bool isBlocking,
                                        bool enableRenderStats,
                                        TimeValue firstFrame,
                                        TimeValue lastFrame,
                                        TimeValue frameStep,
                                        const std::vector<ViewIdx>& viewsToRender,
                                        RenderDirectionEnum direction)
{

    OutputSchedulerThreadPrivate::RenderSequenceArgs args;
    {
        args.blocking = isBlocking;
        args.useStats = enableRenderStats;
        args.firstFrame = firstFrame;
        args.lastFrame = lastFrame;
        args.frameStep = frameStep;
        args.viewsToRender = viewsToRender;
        args.direction = direction;
    }
    
    _imp->validateRenderSequenceArgs(args);

    {
        QMutexLocker k(&_imp->sequentialRenderQueueMutex);
        _imp->sequentialRenderQueue.push_back(args);

        // If we are already doing a sequential render, wait for it to complete first
        if (_imp->sequentialRenderQueue.size() > 1) {
            return;
        }
    }
    _imp->launchNextSequentialRender();

} // renderFrameRange

void
OutputSchedulerThread::renderFromCurrentFrame(bool isBlocking,
                                              bool enableRenderStats,
                                              TimeValue frameStep,
                                              const std::vector<ViewIdx>& viewsToRender,
                                              RenderDirectionEnum timelineDirection)
{

    TimeValue firstFrame, lastFrame;
    getFrameRangeToRender(firstFrame, lastFrame);
    // Make sure current frame is in the frame range
    TimeValue currentTime = timelineGetTime();
    OutputSchedulerThreadPrivate::getNearestInSequence(timelineDirection, currentTime, firstFrame, lastFrame, &currentTime);

    switch (timelineDirection) {
        case eRenderDirectionForward:
            firstFrame = currentTime;
            lastFrame = lastFrame;
            break;
        case eRenderDirectionBackward:
            lastFrame = currentTime;
            break;
    }
    renderFrameRange(isBlocking, enableRenderStats, firstFrame, lastFrame, frameStep, viewsToRender, timelineDirection);

}

void
OutputSchedulerThread::notifyRenderFailure(ActionRetCodeEnum stat, const std::string& errorMessage)
{
    ///Abort all ongoing rendering
    OutputSchedulerThreadStartArgsPtr args = _imp->runArgs.lock();

    assert(args);

    std::string message = errorMessage;
    if (message.empty()) {
        if (stat == eActionStatusFailed) {
            message = tr("Render Failed").toStdString();
        } else if (stat == eActionStatusAborted) {
            message = tr("Render Aborted").toStdString();
        }
    }

    ///Handle failure: for viewers we make it black and don't display the error message which is irrelevant
    handleRenderFailure(stat, message);
    
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



RenderEngine*
OutputSchedulerThread::getEngine() const
{
    return _imp->engine;
}

void
OutputSchedulerThread::runCallbackWithVariables(const QString& callback)
{
    if ( !callback.isEmpty() ) {
        NodePtr effect = _imp->outputEffect.lock();
        QString script = callback;
        std::string appID = effect->getApp()->getAppIDString();
        std::string nodeName = effect->getFullyQualifiedName();
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

void
OutputSchedulerThread::runAfterFrameRenderedCallback(TimeValue frame)
{
    NodePtr effect = getOutputNode();
    std::string cb = effect->getEffectInstance()->getAfterFrameRenderCallback();
    if ( cb.empty() ) {
        return;
    }
    std::vector<std::string> args;
    std::string error;
    try {
        NATRON_PYTHON_NAMESPACE::getFunctionArguments(cb, &error, &args);
    } catch (const std::exception& e) {
        effect->getApp()->appendToScriptEditor( std::string("Failed to run onFrameRendered callback: ")
                                               + e.what() );

        return;
    }

    if ( !error.empty() ) {
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

    if ( (args[0] != "frame") || (args[1] != "thisNode") || (args[2] != "app") ) {
        effect->getApp()->appendToScriptEditor("Failed to run after frame render callback: " + signatureError);

        return;
    }

    std::stringstream ss;
    std::string appStr = effect->getApp()->getAppIDString();
    std::string outputNodeName = appStr + "." + effect->getFullyQualifiedName();
    ss << cb << "(" << frame << ", ";
    ss << outputNodeName << ", " << appStr << ")";
    std::string script = ss.str();
    try {
        runCallbackWithVariables( QString::fromUtf8( script.c_str() ) );
    } catch (const std::exception& e) {
        notifyRenderFailure( eActionStatusFailed, e.what() );

        return;
    }

} // runAfterFrameRenderedCallback

////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
//////////////////////// RenderThreadTask ////////////

struct RenderThreadTaskPrivate
{
    OutputSchedulerThread* scheduler;
    NodeWPtr output;


    TimeValue time;
    bool useRenderStats;
    std::vector<ViewIdx> viewsToRender;


    RenderThreadTaskPrivate(const NodePtr& output,
                            OutputSchedulerThread* scheduler,
                            const TimeValue time,
                            const bool useRenderStats,
                            const std::vector<ViewIdx>& viewsToRender
                            )
        : scheduler(scheduler)
        , output(output)
        , time(time)
        , useRenderStats(useRenderStats)
        , viewsToRender(viewsToRender)
    {
    }
};



RenderThreadTask::RenderThreadTask(const NodePtr& output,
                                   OutputSchedulerThread* scheduler,
                                   const TimeValue time,
                                   const bool useRenderStats,
                                   const std::vector<ViewIdx>& viewsToRender)
    : QRunnable()
    , _imp( new RenderThreadTaskPrivate(output, scheduler, time, useRenderStats, viewsToRender) )
{
}

RenderThreadTask::~RenderThreadTask()
{
}

OutputSchedulerThread*
RenderThreadTask::getScheduler() const
{
    return _imp->scheduler;
}

void
RenderThreadTask::run()
{
    renderFrame(_imp->time, _imp->viewsToRender, _imp->useRenderStats);
    _imp->scheduler->notifyThreadAboutToQuit(this);
}


////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
//////////////////////// DefaultScheduler ////////////


DefaultScheduler::DefaultScheduler(RenderEngine* engine,
                                   const NodePtr& effect)
    : OutputSchedulerThread(engine, effect, eProcessFrameBySchedulerThread)
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

protected:

    mutable QMutex renderObjectsMutex;
    std::list<TreeRenderPtr> renderObjects;

public:



    DefaultRenderFrameRunnable(const NodePtr& writer,
                               OutputSchedulerThread* scheduler,
                               const TimeValue time,
                               const bool useRenderStats,
                               const std::vector<ViewIdx>& viewsToRender)
        : RenderThreadTask(writer, scheduler, time, useRenderStats, viewsToRender)
        , renderObjectsMutex()
        , renderObjects()
    {
    }



    virtual ~DefaultRenderFrameRunnable()
    {
    }

    virtual void abortRender() OVERRIDE
    {
        QMutexLocker k(&renderObjectsMutex);
        for (std::list<TreeRenderPtr>::const_iterator it = renderObjects.begin(); it != renderObjects.end(); ++it) {
            (*it)->setRenderAborted();
        }
    }

private:

    void runBeforeFrameRenderCallback(TimeValue frame, const NodePtr& outputNode)
    {
        std::string cb = outputNode->getEffectInstance()->getBeforeFrameRenderCallback();
        if ( cb.empty() ) {
            return;
        }
        std::vector<std::string> args;
        std::string error;
        try {
            NATRON_PYTHON_NAMESPACE::getFunctionArguments(cb, &error, &args);
        } catch (const std::exception& e) {
            outputNode->getApp()->appendToScriptEditor( std::string("Failed to run beforeFrameRendered callback: ")
                                                   + e.what() );

            return;
        }

        if ( !error.empty() ) {
            outputNode->getApp()->appendToScriptEditor("Failed to run before frame render callback: " + error);

            return;
        }

        std::string signatureError;
        signatureError.append("The before frame render callback supports the following signature(s):\n");
        signatureError.append("- callback(frame, thisNode, app)");
        if (args.size() != 3) {
            outputNode->getApp()->appendToScriptEditor("Failed to run before frame render callback: " + signatureError);

            return;
        }

        if ( (args[0] != "frame") || (args[1] != "thisNode") || (args[2] != "app") ) {
            outputNode->getApp()->appendToScriptEditor("Failed to run before frame render callback: " + signatureError);

            return;
        }

        std::stringstream ss;
        std::string appStr = outputNode->getApp()->getAppIDString();
        std::string outputNodeName = appStr + "." + outputNode->getFullyQualifiedName();
        ss << cb << "(" << frame << ", " << outputNodeName << ", " << appStr << ")";
        std::string script = ss.str();
        try {
            _imp->scheduler->runCallbackWithVariables( QString::fromUtf8( script.c_str() ) );
        } catch (const std::exception &e) {
            _imp->scheduler->notifyRenderFailure( eActionStatusFailed, e.what() );

            return;
        }


    }

protected:

    ActionRetCodeEnum renderFrameInternal(NodePtr outputNode,
                                          TimeValue time,
                                          ViewIdx view,
                                          const RenderStatsPtr& stats,
                                          ImagePtr* imagePlane)
    {
        if (!outputNode) {
            return eActionStatusFailed;
        }

        // If the output is a Write node, actually write is the internal write node encoder
        {
            WriteNodePtr isWrite = toWriteNode(outputNode->getEffectInstance());
            if (isWrite) {
                NodePtr embeddedWriter = isWrite->getEmbeddedWriter();
                if (embeddedWriter) {
                    outputNode = embeddedWriter;
                }
            }
        }
        assert(outputNode);


        TreeRender::CtorArgsPtr args(new TreeRender::CtorArgs);
        args->treeRootEffect = outputNode->getEffectInstance();
        args->time = time;
        args->view = view;

        // Render default layer produced
        args->plane = 0;

        // Render by default on disk is always using a mipmap level of 0 but using the proxy scale of the project
        args->mipMapLevel = 0;

#pragma message WARN("Todo: set proxy scale here")
        args->proxyScale = RenderScale(1.);

        // Render the RoD
        args->canonicalRoI = 0;
        args->stats = stats;
        args->draftMode = false;
        args->playback = true;
        args->byPassCache = false;

        ActionRetCodeEnum retCode = eActionStatusFailed;
        TreeRenderPtr render = TreeRender::create(args);
        if (render) {
            {
                QMutexLocker k(&renderObjectsMutex);
                renderObjects.push_back(render);
            }
            FrameViewRequestPtr outputRequest;
            retCode = render->launchRender(&outputRequest);
            if (!isFailureRetCode(retCode)) {
                *imagePlane = outputRequest->getRequestedScaleImagePlane();
            }
        }

        if (isFailureRetCode(retCode)) {
            return retCode;
        } else {
            return eActionStatusOK;
        }
    }

private:


    virtual void renderFrame(TimeValue time,
                             const std::vector<ViewIdx>& viewsToRender,
                             bool enableRenderStats) OVERRIDE
    {
        NodePtr outputNode = _imp->output.lock();
        assert(outputNode);

        // Notify we start rendering a frame to Python
        runBeforeFrameRenderCallback(time, outputNode);

        // Even if enableRenderStats is false, we at least profile the time spent rendering the frame when rendering with a Write node.
        // Though we don't enable render stats for sequential renders (e.g: WriteFFMPEG) since this is 1 file.
        RenderStatsPtr stats;
        if (outputNode->getEffectInstance()->isWriter()) {
            stats.reset( new RenderStats(enableRenderStats) );
        }
        
        BufferedFrameContainerPtr frameContainer(new BufferedFrameContainer);
        frameContainer->time = time;

        for (std::size_t view = 0; view < viewsToRender.size(); ++view) {
            
            BufferedFramePtr frame(new BufferedFrame);
            frame->view = viewsToRender[view];
            frame->stats = stats;
            
            ImagePtr imagePlane;
            ActionRetCodeEnum stat = renderFrameInternal(outputNode, time, viewsToRender[view], stats, &imagePlane);
            if (isFailureRetCode(stat)) {
                _imp->scheduler->notifyRenderFailure(stat, std::string());
            }
            
            frameContainer->frames.push_back(frame);
        }
        
        _imp->scheduler->notifyFrameRendered(frameContainer, eSchedulingPolicyFFA);


        // If policy is FFA run the callback on this thread, otherwise wait that it gets processed on the scheduler thread.
        if (getScheduler()->getSchedulingPolicy() == eSchedulingPolicyFFA) {
            getScheduler()->runAfterFrameRenderedCallback(time);
        }
    } // renderFrame
};


RenderThreadTask*
DefaultScheduler::createRunnable(TimeValue frame,
                                 bool useRenderStarts,
                                 const std::vector<ViewIdx>& viewsToRender)
{
    return new DefaultRenderFrameRunnable(getOutputNode(), this, frame, useRenderStarts, viewsToRender);
}


void
DefaultScheduler::processFrame(const BufferedFrameContainerPtr& /*frames*/)
{
    // We don't have anymore writer that need to process things in order. WriteFFMPEG is doing it for us
} // DefaultScheduler::processFrame


void
DefaultScheduler::timelineGoTo(TimeValue time)
{
    QMutexLocker k(&_currentTimeMutex);

    _currentTime =  time;
}

TimeValue
DefaultScheduler::timelineGetTime() const
{
    QMutexLocker k(&_currentTimeMutex);

    return _currentTime;
}

void
DefaultScheduler::getFrameRangeToRender(TimeValue& first,
                                        TimeValue& last) const
{
    OutputSchedulerThreadStartArgsPtr args = getCurrentRunArgs();

    first = args->firstFrame;
    last = args->lastFrame;
}

void
DefaultScheduler::handleRenderFailure(ActionRetCodeEnum /*stat*/, const std::string& errorMessage)
{
    if ( appPTR->isBackground() ) {
        if (!errorMessage.empty()) {
            std::cerr << errorMessage << std::endl;
        }
    }
}

SchedulingPolicyEnum
DefaultScheduler::getSchedulingPolicy() const
{
    return eSchedulingPolicyFFA;
}

void
DefaultScheduler::aboutToStartRender()
{
    OutputSchedulerThreadStartArgsPtr args = getCurrentRunArgs();
    NodePtr outputNode = getOutputNode();

    {
        QMutexLocker k(&_currentTimeMutex);
        if (args->direction == eRenderDirectionForward) {
            _currentTime  = args->firstFrame;
        } else {
            _currentTime  = args->lastFrame;
        }
    }
    bool isBackGround = appPTR->isBackground();

    if (isBackGround) {
        QString longText = QString::fromUtf8( outputNode->getScriptName_mt_safe().c_str() ) + tr(" ==> Rendering started");
        appPTR->writeToOutputPipe(longText, QString::fromUtf8(kRenderingStartedShort), true);
    }

    // Activate the internal writer node for a write node
    WriteNodePtr isWrite = toWriteNode( outputNode->getEffectInstance() );
    if (isWrite) {
        isWrite->onSequenceRenderStarted();
    }

    std::string cb = outputNode->getEffectInstance()->getBeforeRenderCallback();
    if ( !cb.empty() ) {
        std::vector<std::string> args;
        std::string error;
        try {
            NATRON_PYTHON_NAMESPACE::getFunctionArguments(cb, &error, &args);
        } catch (const std::exception& e) {
            outputNode->getApp()->appendToScriptEditor( std::string("Failed to run beforeRender callback: ")
                                                    + e.what() );

            return;
        }

        if ( !error.empty() ) {
            outputNode->getApp()->appendToScriptEditor("Failed to run beforeRender callback: " + error);

            return;
        }

        std::string signatureError;
        signatureError.append("The beforeRender callback supports the following signature(s):\n");
        signatureError.append("- callback(thisNode, app)");
        if (args.size() != 2) {
            outputNode->getApp()->appendToScriptEditor("Failed to run beforeRender callback: " + signatureError);

            return;
        }

        if ( (args[0] != "thisNode") || (args[1] != "app") ) {
            outputNode->getApp()->appendToScriptEditor("Failed to run beforeRender callback: " + signatureError);

            return;
        }


        std::stringstream ss;
        std::string appStr = outputNode->getApp()->getAppIDString();
        std::string outputNodeName = appStr + "." + outputNode->getFullyQualifiedName();
        ss << cb << "(" << outputNodeName << ", " << appStr << ")";
        std::string script = ss.str();
        try {
            runCallbackWithVariables( QString::fromUtf8( script.c_str() ) );
        } catch (const std::exception &e) {
            notifyRenderFailure( eActionStatusFailed, e.what() );
        }
    }
} // DefaultScheduler::aboutToStartRender

void
DefaultScheduler::onRenderStopped(bool aborted)
{
    NodePtr outputNode = getOutputNode();

    {
        QString longText = QString::fromUtf8( outputNode->getScriptName_mt_safe().c_str() ) + tr(" ==> Rendering finished");
        appPTR->writeToOutputPipe(longText, QString::fromUtf8(kRenderingFinishedStringShort), true);
    }


    std::string cb = outputNode->getEffectInstance()->getAfterRenderCallback();
    if ( !cb.empty() ) {
        std::vector<std::string> args;
        std::string error;
        try {
            NATRON_PYTHON_NAMESPACE::getFunctionArguments(cb, &error, &args);
        } catch (const std::exception& e) {
            outputNode->getApp()->appendToScriptEditor( std::string("Failed to run afterRender callback: ")
                                                    + e.what() );

            return;
        }

        if ( !error.empty() ) {
            outputNode->getApp()->appendToScriptEditor("Failed to run afterRender callback: " + error);

            return;
        }

        std::string signatureError;
        signatureError.append("The after render callback supports the following signature(s):\n");
        signatureError.append("- callback(aborted, thisNode, app)");
        if (args.size() != 3) {
            outputNode->getApp()->appendToScriptEditor("Failed to run afterRender callback: " + signatureError);

            return;
        }

        if ( (args[0] != "aborted") || (args[1] != "thisNode") || (args[2] != "app") ) {
            outputNode->getApp()->appendToScriptEditor("Failed to run afterRender callback: " + signatureError);

            return;
        }


        std::stringstream ss;
        std::string appStr = outputNode->getApp()->getAppIDString();
        std::string outputNodeName = appStr + "." + outputNode->getFullyQualifiedName();
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
                                               const NodePtr& viewer)
    : OutputSchedulerThread(engine, viewer, eProcessFrameByMainThread)
{

}

ViewerDisplayScheduler::~ViewerDisplayScheduler()
{
}




void
ViewerDisplayScheduler::timelineGoTo(TimeValue time)
{
    ViewerNodePtr isViewer = getOutputNode()->isEffectViewerNode();
    assert(isViewer);
    isViewer->getTimeline()->seekFrame(time, true, isViewer, eTimelineChangeReasonPlaybackSeek);
}

TimeValue
ViewerDisplayScheduler::timelineGetTime() const
{
    ViewerNodePtr isViewer = getOutputNode()->isEffectViewerNode();
    return TimeValue(isViewer->getTimeline()->currentFrame());
}

void
ViewerDisplayScheduler::getFrameRangeToRender(TimeValue &first,
                                              TimeValue &last) const
{
    ViewerNodePtr isViewer = getOutputNode()->isEffectViewerNode();
    ViewerNodePtr leadViewer = isViewer->getApp()->getLastViewerUsingTimeline();
    ViewerNodePtr v = leadViewer ? leadViewer : isViewer;
    assert(v);
    int left, right;
    v->getTimelineBounds(&left, &right);
    first = TimeValue(left);
    last = TimeValue(right);
}

class ViewerRenderBufferedFrame : public BufferedFrame
{
public:

    ViewerRenderBufferedFrame()
    : BufferedFrame()
    , isPartialRect(false)
    , viewerProcessImages()
    , canonicalRoi()
    , viewerProcessImageKey()
    {

    }

    virtual ~ViewerRenderBufferedFrame() {}

public:

    bool isPartialRect;
    ImagePtr viewerProcessImages[2];
    ImagePtr colorPickerImages[2];
    ImagePtr colorPickerInputImages[2];
    ActionRetCodeEnum retCode[2];
    RectD canonicalRoi[2];
    ImageCacheKeyPtr viewerProcessImageKey[2];
};

class ViewerRenderBufferedFrameContainer : public BufferedFrameContainer
{
public:
    
    ViewerRenderBufferedFrameContainer()
    : BufferedFrameContainer()
    , recenterViewer(0)
    , viewerCenter()
    {
        
    }
    
    virtual ~ViewerRenderBufferedFrameContainer() {}
    
    bool recenterViewer;
    Point viewerCenter;
};

typedef boost::shared_ptr<ViewerRenderBufferedFrame> ViewerRenderBufferedFramePtr;
typedef boost::shared_ptr<ViewerRenderBufferedFrameContainer> ViewerRenderBufferedFrameContainerPtr;

struct RenderViewerProcessFunctorArgs
{
    NodePtr viewerProcessNode;
    NodePtr colorPickerNode, colorPickerInputNode;
    RotoStrokeItemPtr activeStrokeItem;
    TreeRenderPtr renderObject;
    TimeValue time;
    ViewIdx view;
    RenderStatsPtr stats;
    ActionRetCodeEnum retCode;
    RectD roi;

    // Store the key of the first tile in the outputImage
    // so that we can later on check in the gui if
    // the image is still cached to update the cache line
    // on the timeline.
    ImageCacheKeyPtr viewerProcessImageCacheKey;
    ImagePtr outputImage;
    ImagePtr colorPickerImage, colorPickerInputImage;

    unsigned int viewerMipMapLevel;
    bool isDraftModeEnabled;
    bool isPlayback;
    bool byPassCache;

    RenderViewerProcessFunctorArgs()
    : retCode(eActionStatusOK)
    {

    }
};

typedef boost::shared_ptr<RenderViewerProcessFunctorArgs> RenderViewerProcessFunctorArgsPtr;


class ViewerRenderFrameRunnable
    : public DefaultRenderFrameRunnable
{
    ViewerNodePtr _viewer;

public:

    ViewerRenderFrameRunnable(const NodePtr& viewer,
                              OutputSchedulerThread* scheduler,
                              const TimeValue frame,
                              const bool useRenderStarts,
                              const std::vector<ViewIdx>& viewsToRender)
        : DefaultRenderFrameRunnable(viewer, scheduler, frame, useRenderStarts, viewsToRender)
        , _viewer(viewer->isEffectViewerNode())
    {
    }



    virtual ~ViewerRenderFrameRunnable()
    {
    }

public:


    static void createRenderViewerObject(RenderViewerProcessFunctorArgs* inArgs)
    {
        TreeRender::CtorArgsPtr args(new TreeRender::CtorArgs);
        args->treeRootEffect = inArgs->viewerProcessNode->getEffectInstance();
        args->time = inArgs->time;
        args->view = inArgs->view;

        // Render default plane produced
        args->plane = 0;

        // Render by default on disk is always using a mipmap level of 0 but using the proxy scale of the project
        args->mipMapLevel = inArgs->viewerMipMapLevel;

#pragma message WARN("Todo: set proxy scale here")
        args->proxyScale = RenderScale(1.);

        // Render the RoD
        args->canonicalRoI = inArgs->roi.isNull() ? 0 : &inArgs->roi;
        args->stats = inArgs->stats;
        args->draftMode = inArgs->isDraftModeEnabled;
        args->playback = inArgs->isPlayback;
        args->byPassCache = inArgs->byPassCache;
        args->activeRotoDrawableItem = inArgs->activeStrokeItem;
        if (inArgs->colorPickerNode) {
            args->extraNodesToSample.push_back(inArgs->colorPickerNode);
        }
        if (inArgs->colorPickerInputImage) {
            args->extraNodesToSample.push_back(inArgs->colorPickerInputNode);
        }

        inArgs->retCode = eActionStatusFailed;
        inArgs->renderObject = TreeRender::create(args);

    }

    static ImagePtr convertImageForViewerDisplay(const ImagePtr& image)
    {
        if (!image) {
            return image;
        }
        Image::InitStorageArgs initArgs;
        initArgs.bounds = image->getBounds();

        // Viewer textures are always RGBA

        initArgs.plane = ImagePlaneDesc::getRGBAComponents();
        initArgs.mipMapLevel = image->getMipMapLevel();
        initArgs.proxyScale = image->getProxyScale();
        initArgs.bitdepth = image->getBitDepth();
        initArgs.storage = eStorageModeRAM;
        initArgs.bufferFormat = eImageBufferLayoutRGBAPackedFullRect;
        ImagePtr mappedImage = Image::create(initArgs);
        if (!mappedImage) {
            return mappedImage;
        }
        Image::CopyPixelsArgs copyArgs;
        copyArgs.roi = initArgs.bounds;
        copyArgs.monoConversion = Image::eMonoToPackedConversionCopyToAll;
        mappedImage->copyPixels(*image, copyArgs);
        return mappedImage;
    }

    static void launchRenderFunctor(const RenderViewerProcessFunctorArgsPtr& inArgs)
    {
        assert(inArgs->renderObject);
        FrameViewRequestPtr outputRequest;
        inArgs->retCode = inArgs->renderObject->launchRender(&outputRequest);

        if (outputRequest) {
            inArgs->outputImage = outputRequest->getRequestedScaleImagePlane();
        }

        // There might be no output image if the RoI that was passed to render is outside of the RoD of the effect
        if (isFailureRetCode(inArgs->retCode) || !inArgs->outputImage) {
            inArgs->outputImage.reset();
            inArgs->renderObject.reset();
            return;
        }
        // Convert the image to a format that can be uploaded to a OpenGL texture
        {
            
            
            // Find the key of the image and store it so that in the gui
            // we can later on re-use this key to check the cache for the timeline's cache line
            ImageCacheEntryPtr cacheEntry = inArgs->outputImage->getCacheEntry();
            if (cacheEntry) {
                inArgs->viewerProcessImageCacheKey = cacheEntry->getCacheKey();
            }


            // The viewer-process node may not have rendered a 4 channel image, but this is required but the OpenGL viewer
            // which only draws RGBA images.
    
            inArgs->outputImage = convertImageForViewerDisplay(inArgs->outputImage);

            // Extra color-picker images as-well.
            if (inArgs->colorPickerNode) {
                {
                    FrameViewRequestPtr req = inArgs->renderObject->getExtraRequestedResultsForNode(inArgs->colorPickerNode);
                    if (req) {
                        inArgs->colorPickerImage = req->getRequestedScaleImagePlane();
                        inArgs->colorPickerImage = convertImageForViewerDisplay(inArgs->colorPickerImage);
                    }
                }
                if (inArgs->colorPickerInputNode) {
                    FrameViewRequestPtr req = inArgs->renderObject->getExtraRequestedResultsForNode(inArgs->colorPickerInputNode);
                    if (req) {
                        inArgs->colorPickerInputImage = req->getRequestedScaleImagePlane();
                        inArgs->colorPickerInputImage = convertImageForViewerDisplay(inArgs->colorPickerInputImage);
                    }
                }
            }
            
        }
        inArgs->renderObject.reset();
    }

    static unsigned getViewerMipMapLevel(const ViewerNodePtr& viewer, bool draftModeEnabled, bool fullFrameProcessing)
    {


        if (fullFrameProcessing) {
            return 0;
        }

        unsigned int mipMapLevel = 0;

        const double zoomFactor = viewer->getUIZoomFactor();

        int downcale_i = viewer->getDownscaleMipMapLevelKnobIndex();


        assert(downcale_i >= 0);
        if (downcale_i > 0) {
            mipMapLevel = downcale_i;
        } else {
            mipMapLevel = viewer->getMipMapLevelFromZoomFactor();
        }

        // If draft mode is enabled, compute the mipmap level according to the auto-proxy setting in the preferences
        if ( draftModeEnabled && appPTR->getCurrentSettings()->isAutoProxyEnabled() ) {
            unsigned int autoProxyLevel = appPTR->getCurrentSettings()->getAutoProxyMipMapLevel();
            if (zoomFactor > 1) {
                //Decrease draft mode at each inverse mipmaplevel level taken
                unsigned int invLevel = Image::getLevelFromScale(1. / zoomFactor);
                if (invLevel < autoProxyLevel) {
                    autoProxyLevel -= invLevel;
                } else {
                    autoProxyLevel = 0;
                }
            }
            mipMapLevel = (unsigned int)std::max( (int)mipMapLevel, (int)autoProxyLevel );

        }

        return mipMapLevel;
    } // getViewerMipMapLevel

    static void createRenderViewerProcessArgs(const ViewerNodePtr& viewer,
                                              int viewerProcess_i,
                                              TimeValue time,
                                              ViewIdx view,
                                              bool isPlayback,
                                              const RenderStatsPtr& stats,
                                              const RotoStrokeItemPtr& activeStroke,
                                              const RectD* roiParam,
                                              ViewerRenderBufferedFrame* bufferedFrame,
                                              RenderViewerProcessFunctorArgs* outArgs)
    {

        bool fullFrameProcessing = viewer->isFullFrameProcessingEnabled();
        bool draftModeEnabled = viewer->getApp()->isDraftRenderEnabled();
        unsigned int mipMapLevel = getViewerMipMapLevel(viewer, draftModeEnabled, fullFrameProcessing);
        bool byPassCache = viewer->isRenderWithoutCacheEnabledAndTurnOff();

        RectD roi;
        if (roiParam) {
            roi = *roiParam;
        } else if (!fullFrameProcessing) {
            roi = viewer->getUiContext()->getImageRectangleDisplayed();
        }
        outArgs->activeStrokeItem = activeStroke;
        outArgs->isPlayback = isPlayback;
        outArgs->isDraftModeEnabled = draftModeEnabled;
        outArgs->viewerMipMapLevel = mipMapLevel;
        outArgs->byPassCache = byPassCache;
        outArgs->roi = roi;
        outArgs->stats = stats;
        outArgs->time = time;
        outArgs->view = view;
        if (!isPlayback) {
            outArgs->colorPickerNode = viewerProcess_i == 0 ? viewer->getCurrentAInput() : viewer->getCurrentBInput();
            if (outArgs->colorPickerNode) {
                // Also sample the "main" input of the color picker node, this is useful for keyers.
                int mainInput = outArgs->colorPickerNode->getPreferredInput();
                outArgs->colorPickerInputNode = outArgs->colorPickerNode->getInput(mainInput);
            }
        }
        outArgs->viewerProcessNode = viewer->getViewerProcessNode(viewerProcess_i)->getNode();
        createRenderViewerObject(outArgs);
        bufferedFrame->canonicalRoi[viewerProcess_i] = roi;

    }

private:

    void createAndLaunchRenderInThread(const RenderViewerProcessFunctorArgsPtr& processArgs, int viewerProcess_i, TimeValue time, const RenderStatsPtr& stats, ViewerRenderBufferedFrame* bufferedFrame)
    {

        createRenderViewerProcessArgs(_viewer, viewerProcess_i, time, bufferedFrame->view, true /*isPlayback*/, stats,  RotoStrokeItemPtr(), 0 /*roiParam*/,  bufferedFrame, processArgs.get());

        // Register the render so that it can be aborted in abortRenders()
        {
            QMutexLocker k(&renderObjectsMutex);

            if (processArgs->renderObject) {
                renderObjects.push_back(processArgs->renderObject);
            }

        }

        launchRenderFunctor(processArgs);

        bufferedFrame->retCode[viewerProcess_i] = processArgs->retCode;
        bufferedFrame->viewerProcessImageKey[viewerProcess_i] = processArgs->viewerProcessImageCacheKey;
        bufferedFrame->viewerProcessImages[viewerProcess_i] = processArgs->outputImage;
        bufferedFrame->colorPickerImages[viewerProcess_i] = processArgs->colorPickerImage;
        bufferedFrame->colorPickerInputImages[viewerProcess_i] = processArgs->colorPickerInputImage;

    }

    virtual void renderFrame(TimeValue time,
                             const std::vector<ViewIdx>& viewsToRender,
                             bool enableRenderStats)
    {

        RenderStatsPtr stats;
        if (enableRenderStats) {
            stats.reset( new RenderStats(enableRenderStats) );
        }


        ViewerRenderBufferedFrameContainerPtr frameContainer(new  ViewerRenderBufferedFrameContainer());
        frameContainer->time = time;
        frameContainer->recenterViewer = false;
        
        for (std::size_t i = 0; i < viewsToRender.size(); ++i) {


            ViewerRenderBufferedFramePtr bufferObject(new ViewerRenderBufferedFrame);
            bufferObject->view = viewsToRender[i];
            bufferObject->stats = stats;
            bufferObject->isPartialRect = false;

            std::vector<RenderViewerProcessFunctorArgsPtr> processArgs(2);
            for (int i = 0; i < 2; ++i) {
                processArgs[i].reset(new RenderViewerProcessFunctorArgs);
                bufferObject->retCode[i] = eActionStatusFailed;
            }

            ViewerCompositingOperatorEnum viewerBlend = _viewer->getCurrentOperator();
            bool viewerBEqualsViewerA = _viewer->getCurrentAInput() == _viewer->getCurrentBInput();

            // Launch the 2nd viewer process in a separate thread
            QFuture<void> processBFuture;
            if (!viewerBEqualsViewerA && viewerBlend != eViewerCompositingOperatorNone) {
                processBFuture = QtConcurrent::run(this,
                                                   &ViewerRenderFrameRunnable::createAndLaunchRenderInThread,
                                                   processArgs[1],
                                                   1,
                                                   time,
                                                   stats,
                                                   bufferObject.get());
            }

            // Launch the 1st viewer process in this thread
            createAndLaunchRenderInThread(processArgs[0], 0, time, stats, bufferObject.get());

            // Wait for the 2nd viewer process
            if (viewerBlend != eViewerCompositingOperatorNone) {
                if (!viewerBEqualsViewerA) {
                    processBFuture.waitForFinished();
                } else {
                    bufferObject->retCode[1] = processArgs[0]->retCode;
                    bufferObject->viewerProcessImageKey[1] = processArgs[0]->viewerProcessImageCacheKey;
                    bufferObject->viewerProcessImages[1] = processArgs[0]->outputImage;
                    processArgs[0] = processArgs[1];
                }
            }

            // Check for failures
            for (int i = 0; i < 2; ++i) {

                // All other status code should not show an error
                if (processArgs[i]->retCode == eActionStatusFailed || processArgs[i]->retCode == eActionStatusOutOfMemory) {
                    _imp->scheduler->notifyRenderFailure(processArgs[i]->retCode, std::string());
                    break;
                }
            }

            frameContainer->frames.push_back(bufferObject);
            
            // Notify the scheduler thread which will in turn call processFrame

        } // for all views

        _imp->scheduler->appendToBuffer(frameContainer);

        if (stats) {
            getScheduler()->getEngine()->reportStats(time,  stats);
        }

    } // renderFrame
};


void
ViewerDisplayScheduler::processFrame(const BufferedFrameContainerPtr& frames)
{
    ViewerRenderBufferedFrameContainer* framesContainer = dynamic_cast<ViewerRenderBufferedFrameContainer*>(frames.get());
    assert(framesContainer);

    ViewerNodePtr isViewer = getOutputNode()->isEffectViewerNode();
    assert(isViewer);

    if ( framesContainer->frames.empty() ) {
        isViewer->redrawViewer();
        return;
    }

    for (std::list<BufferedFramePtr>::const_iterator it = framesContainer->frames.begin(); it != framesContainer->frames.end(); ++it) {
        ViewerRenderBufferedFrame* viewerObject = dynamic_cast<ViewerRenderBufferedFrame*>(it->get());
        assert(viewerObject);
        
        ViewerNode::UpdateViewerArgs args;
        args.time = framesContainer->time;
        args.view = (*it)->view;
        args.isPartialRect = false;
        args.recenterViewer = false;


        for (int i = 0; i < 2; ++i) {
            ViewerNode::UpdateViewerArgs::TextureUpload upload;
            upload.canonicalRoI = viewerObject->canonicalRoi[i];
            upload.image = viewerObject->viewerProcessImages[i];
            upload.colorPickerImage = viewerObject->colorPickerImages[i];
            upload.colorPickerInputImage = viewerObject->colorPickerInputImages[i];
            upload.viewerProcessImageKey = viewerObject->viewerProcessImageKey[i];
            if (viewerObject->retCode[i] == eActionStatusAborted ||
                (viewerObject->retCode[i] == eActionStatusOK && !upload.image)) {
                continue;
            }
            args.viewerUploads[i].push_back(upload);
        }
        if (!args.viewerUploads[0].empty() || !args.viewerUploads[1].empty()) {
            isViewer->updateViewer(args);
        }
    }
    isViewer->redrawViewerNow();
   
}

RenderThreadTask*
ViewerDisplayScheduler::createRunnable(TimeValue frame,
                                       bool useRenderStarts,
                                       const std::vector<ViewIdx>& viewsToRender)
{
    return new ViewerRenderFrameRunnable(getOutputNode(), this, frame, useRenderStarts, viewsToRender);
}



void
ViewerDisplayScheduler::handleRenderFailure(ActionRetCodeEnum /*stat*/, const std::string& /*errorMessage*/)
{
    ViewerNodePtr effect = getOutputNode()->isEffectViewerNode();
    effect->disconnectViewer();
}

void
ViewerDisplayScheduler::onRenderStopped(bool /*/aborted*/)
{
    // Refresh all previews in the tree
    NodePtr effect = getOutputNode();

    effect->getApp()->refreshAllPreviews();

    if ( effect->getApp()->isGuiFrozen() ) {
        getEngine()->s_refreshAllKnobs();
    }
}

TimeValue
ViewerDisplayScheduler::getLastRenderedTime() const
{
    ViewerNodePtr effect = getOutputNode()->isEffectViewerNode();
    return TimeValue(effect->getLastRenderedTime());
}

////////////////////////// RenderEngine

struct RenderEnginePrivate
{
    QMutex schedulerCreationLock;
    OutputSchedulerThread* scheduler;

    //If true then a current frame render can start playback, protected by abortedRequestedMutex
    bool canAutoRestartPlayback;
    QMutex canAutoRestartPlaybackMutex; // protects abortRequested
    NodeWPtr output;
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

    RenderEnginePrivate(const NodePtr& output)
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

RenderEngine::RenderEngine(const NodePtr& output)
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
RenderEngine::createScheduler(const NodePtr& effect)
{
    return new DefaultScheduler(this, effect);
}

NodePtr
RenderEngine::getOutput() const
{
    return _imp->output.lock();
}

void
RenderEngine::renderFrameRange(bool isBlocking,
                               bool enableRenderStats,
                               TimeValue firstFrame,
                               TimeValue lastFrame,
                               TimeValue frameStep,
                               const std::vector<ViewIdx>& viewsToRender,
                               RenderDirectionEnum forward)
{
    // We are going to start playback, abort any current viewer refresh
    if (_imp->currentFrameScheduler) {
        _imp->currentFrameScheduler->onAbortRequested(true);
    }
    
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
    // We are going to start playback, abort any current viewer refresh
    _imp->currentFrameScheduler->onAbortRequested(true);
    
    setPlaybackAutoRestartEnabled(true);

    {
        QMutexLocker k(&_imp->schedulerCreationLock);
        if (!_imp->scheduler) {
            _imp->scheduler = createScheduler( _imp->output.lock() );
        }
    }

    _imp->scheduler->renderFromCurrentFrame(false, enableRenderStats, TimeValue(1.) /*frameStep*/, viewsToRender, forward);
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
        renderCurrentFrameNowInternal(r.enableStats);
        _imp->refreshQueue.pop_front();
    }
}

void
RenderEngine::renderCurrentFrameWithRenderStats()
{
    renderCurrentFrameInternal(true);
}


void
RenderEngine::renderCurrentFrame()
{
    renderCurrentFrameInternal(_imp->output.lock()->getApp()->isRenderStatsActionChecked());
}

void
RenderEngine::renderCurrentFrameInternal(bool enableStats)
{
    assert( QThread::currentThread() == qApp->thread() );
    RenderEnginePrivate::RefreshRequest r;
    r.enableStats = enableStats;
    r.enableAbort = true;
    _imp->refreshQueue.push_back(r);
    Q_EMIT currentFrameRenderRequestPosted();
}

void
RenderEngine::renderCurrentFrameNow()
{
    renderCurrentFrameNowInternal(_imp->output.lock()->getApp()->isRenderStatsActionChecked());
}

void
RenderEngine::renderCurrentFrameNowInternal(bool enableRenderStats)
{
    assert( QThread::currentThread() == qApp->thread() );


    // If the scheduler is already doing playback, continue it
    if (_imp->scheduler) {
        bool working = _imp->scheduler->isWorking();
        if (working) {
            _imp->scheduler->abortThreadedTask();
        }
        if ( working || isPlaybackAutoRestartEnabled() ) {
            RenderDirectionEnum lastDirection;
            std::vector<ViewIdx> lastViews;
            _imp->scheduler->getLastRunArgs(&lastDirection, &lastViews);
            _imp->scheduler->renderFromCurrentFrame( false /*blocking*/, enableRenderStats, TimeValue(1.) /*frameStep*/, lastViews, lastDirection);

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
        NodePtr output = getOutput();
        _imp->currentFrameScheduler = new ViewerCurrentFrameRequestScheduler(output);
    }

    _imp->currentFrameScheduler->renderCurrentFrame(enableRenderStats);
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
        _imp->currentFrameScheduler->onAbortRequested(false);
        _imp->currentFrameScheduler->onQuitRequested(allowRestarts);
        _imp->currentFrameScheduler->onWaitForThreadToQuit();
    }
}

void
RenderEngine::waitForEngineToQuit_not_main_thread()
{
    if (_imp->scheduler) {
        _imp->scheduler->waitForThreadToQuit_not_main_thread();
    }

    if (_imp->currentFrameScheduler) {
        _imp->currentFrameScheduler->onWaitForThreadToQuit();
    }
}

void
RenderEngine::waitForEngineToQuit_main_thread(bool allowRestart)
{
    assert( QThread::currentThread() == qApp->thread() );
    assert(!_imp->engineWatcher);
    _imp->engineWatcher.reset( new RenderEngineWatcher(this) );
    QObject::connect( _imp->engineWatcher.get(), SIGNAL(taskFinished(int,WatcherCallerArgsPtr)), this, SLOT(onWatcherEngineQuitEmitted()) );
    _imp->engineWatcher->scheduleBlockingTask(allowRestart ? RenderEngineWatcher::eBlockingTaskWaitForQuitAllowRestart : RenderEngineWatcher::eBlockingTaskWaitForQuitDisallowRestart);
}

void
RenderEngine::waitForEngineToQuit_enforce_blocking()
{
    if (_imp->scheduler) {
        _imp->scheduler->waitForThreadToQuit_enforce_blocking();
    }

    if (_imp->currentFrameScheduler) {
        _imp->currentFrameScheduler->onWaitForThreadToQuit();
    }
}

bool
RenderEngine::abortRenderingInternal(bool keepOldestRender)
{
    bool ret = false;

    if (_imp->currentFrameScheduler) {
        _imp->currentFrameScheduler->onAbortRequested(keepOldestRender);
    }

    if ( _imp->scheduler && _imp->scheduler->isWorking() ) {
        //If any playback active, abort it
        ret |= _imp->scheduler->abortThreadedTask(keepOldestRender);
    }

    return ret;
}

bool
RenderEngine::abortRenderingNoRestart()
{
    if ( abortRenderingInternal(false) ) {
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
        _imp->currentFrameScheduler->onWaitForAbortCompleted();
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
        _imp->currentFrameScheduler->onWaitForAbortCompleted();
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
    QObject::connect( _imp->engineWatcher.get(), SIGNAL(taskFinished(int,WatcherCallerArgsPtr)), this, SLOT(onWatcherEngineAbortedEmitted()) );
    _imp->engineWatcher->scheduleBlockingTask(RenderEngineWatcher::eBlockingTaskWaitForAbort);
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
        currentFrameSchedulerRunning = _imp->currentFrameScheduler->hasThreadsAlive();
    }

    return schedulerRunning || currentFrameSchedulerRunning;
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
RenderEngine::reportStats(TimeValue time,
                          const RenderStatsPtr& stats)
{

    if (!stats) {
        return;
    }
    std::string filename;
    NodePtr output = getOutput();
    KnobIPtr fileKnob = output->getKnobByName(kOfxImageEffectFileParamName);

    if (fileKnob) {
        KnobFilePtr strKnob = toKnobFile(fileKnob);
        if  (strKnob) {
            QString qfileName = QString::fromUtf8( SequenceParsing::generateFileNameFromPattern(strKnob->getValue( DimIdx(0), ViewIdx(0) ), output->getApp()->getProject()->getProjectViewNames(), time, ViewIdx(0)).c_str() );
            QtCompat::removeFileExtension(qfileName);
            qfileName.append( QString::fromUtf8("-stats.txt") );
            filename = qfileName.toStdString();
        }
    }

    //If there's no filename knob, do not write anything
    if ( filename.empty() ) {
        std::cout << tr("Cannot write render statistics file: "
                        "%1 does not seem to have a parameter named \"filename\" "
                        "to determine the location where to write the stats file.")
        .arg( QString::fromUtf8( output->getScriptName_mt_safe().c_str() ) ).toStdString();

        return;
    }


    FStreamsSupport::ofstream ofile;
    FStreamsSupport::open(&ofile, filename);
    if (!ofile) {
        std::cout << tr("Failure to write render statistics file.").toStdString() << std::endl;

        return;
    }

    double wallTime = 0;
    std::map<NodePtr, NodeRenderStats > statsMap = stats->getStats(&wallTime);

    ofile << "Time spent to render frame (wall clock time): " << Timer::printAsTime(wallTime, false).toStdString() << std::endl;
    for (std::map<NodePtr, NodeRenderStats >::const_iterator it = statsMap.begin(); it != statsMap.end(); ++it) {
        ofile << "------------------------------- " << it->first->getScriptName_mt_safe() << "------------------------------- " << std::endl;
        ofile << "Time spent rendering: " << Timer::printAsTime(it->second.getTotalTimeSpentRendering(), false).toStdString() << std::endl;
    }
} // reportStats

OutputSchedulerThread*
ViewerRenderEngine::createScheduler(const NodePtr& effect)
{
    return new ViewerDisplayScheduler( this, effect );
}

void
ViewerRenderEngine::reportStats(TimeValue time,
                         const RenderStatsPtr& stats)
{
    ViewerNodePtr viewer = getOutput()->isEffectViewerNode();
    double wallTime;
    std::map<NodePtr, NodeRenderStats > statsMap = stats->getStats(&wallTime);
    viewer->reportStats(time, wallTime, statsMap);
}



////////////////////////ViewerCurrentFrameRequestScheduler////////////////////////
class CurrentFrameFunctorArgs
    : public GenericThreadStartArgs
{
public:

    std::vector<ViewIdx> viewsToRender;
    TimeValue time;
    bool useStats;
    NodePtr viewer;
    ViewerCurrentFrameRequestSchedulerPrivate* scheduler;
    RotoStrokeItemPtr strokeItem;
    U64 age;

    CurrentFrameFunctorArgs()
        : GenericThreadStartArgs()
        , viewsToRender()
        , time(0)
        , useStats(false)
        , viewer()
        , scheduler(0)
        , strokeItem()
        , age(0)
    {
    }

    CurrentFrameFunctorArgs(ViewIdx view,
                            TimeValue time,
                            bool useStats,
                            const NodePtr& viewer,
                            ViewerCurrentFrameRequestSchedulerPrivate* scheduler,
                            const RotoStrokeItemPtr& strokeItem)
        : GenericThreadStartArgs()
        , viewsToRender()
        , time(time)
        , useStats(useStats)
        , viewer(viewer)
        , scheduler(scheduler)
        , strokeItem(strokeItem)
        , age(0)
    {
        viewsToRender.push_back(view);
    }

    ~CurrentFrameFunctorArgs()
    {

    }
};

struct TreeRenderAndAge
{
    // One render for each viewer process node
    std::list<TreeRenderPtr> renders;
    U64 age;
};

struct TreeRender_CompareAge
{
    bool operator() (const TreeRenderAndAge& lhs, const TreeRenderAndAge& rhs) const
    {
        return lhs.age < rhs.age;
    }
};

typedef std::set<TreeRenderAndAge, TreeRender_CompareAge> TreeRenderSetOrderedByAge;

class RenderCurrentFrameFunctorRunnable;
struct ViewerCurrentFrameRequestSchedulerPrivate
{
    ViewerCurrentFrameRequestScheduler* _publicInterface;
    NodePtr viewer;
    QThreadPool* threadPool;

    /**
     * Single thread used by the ViewerCurrentFrameRequestScheduler when the global thread pool has reached its maximum
     * activity to keep the renders responsive even if the thread pool is choking.
     **/
    ViewerCurrentFrameRequestRendererBackup backupThread;
    mutable QMutex currentFrameRenderTasksMutex;
    QWaitCondition currentFrameRenderTasksCond;
    std::list<boost::shared_ptr<RenderCurrentFrameFunctorRunnable> > currentFrameRenderTasks;

    mutable QMutex renderAgeMutex; // protects renderAge displayAge currentRenders

    // This is the age to attribute to the next incomming render
    U64 renderAge;

    // This is the age of the last render attributed. If 0 then no render has been displayed yet.
    U64 displayAge;

    // A set of active renders and their age.
    TreeRenderSetOrderedByAge currentRenders;

    ViewerCurrentFrameRequestSchedulerPrivate(ViewerCurrentFrameRequestScheduler* publicInterface, const NodePtr& viewer)
        : _publicInterface(publicInterface)
        , viewer(viewer)
        , threadPool( QThreadPool::globalInstance() )
        , backupThread()
        , currentFrameRenderTasksCond()
        , currentFrameRenderTasks()
        , renderAge(1)
        , displayAge(0)
        , currentRenders()
    {
    }

    void appendRunnableTask(const boost::shared_ptr<RenderCurrentFrameFunctorRunnable>& task)
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
            for (std::list<boost::shared_ptr<RenderCurrentFrameFunctorRunnable> >::iterator it = currentFrameRenderTasks.begin();
                 it != currentFrameRenderTasks.end(); ++it) {
                if (it->get() == task) {
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

    void processProducedFrame(U64 age, const BufferedFrameContainerPtr& frames);
};

class RenderCurrentFrameFunctorRunnable
    : public QRunnable
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

    void createAndLaunchRenderInThread(const ViewerNodePtr &viewer,
                                       const RenderViewerProcessFunctorArgsPtr& processArgs,
                                       int viewerProcess_i,
                                       TimeValue time,
                                       const RenderStatsPtr& stats,
                                       const RotoStrokeItemPtr& activeStroke,
                                       const RectD* roiParam,
                                       ViewerRenderBufferedFrame* bufferedFrame)
    {

        ViewerRenderFrameRunnable::createRenderViewerProcessArgs(viewer, viewerProcess_i, time, bufferedFrame->view, false /*isPlayback*/, stats, activeStroke, roiParam,  bufferedFrame, processArgs.get());

        // Register the current renders and their age on the scheduler so that they can be aborted
        {
            QMutexLocker k(&_args->scheduler->renderAgeMutex);
            TreeRenderAndAge curRender;
            curRender.age = _args->age;

            {
                TreeRenderSetOrderedByAge::iterator foundAge = _args->scheduler->currentRenders.find(curRender);
                if (foundAge != _args->scheduler->currentRenders.end()) {
                    curRender = *foundAge;
                    _args->scheduler->currentRenders.erase(foundAge);
                }
            }
            assert(processArgs->renderObject);
            curRender.renders.push_back(processArgs->renderObject);
            std::pair<TreeRenderSetOrderedByAge::iterator, bool> ok = _args->scheduler->currentRenders.insert(curRender);
            assert(ok.second);
            (void)ok;

        }

        ViewerRenderFrameRunnable::launchRenderFunctor(processArgs);

        bufferedFrame->retCode[viewerProcess_i] = processArgs->retCode;
        bufferedFrame->viewerProcessImageKey[viewerProcess_i] = processArgs->viewerProcessImageCacheKey;
        bufferedFrame->viewerProcessImages[viewerProcess_i] = processArgs->outputImage;
        bufferedFrame->colorPickerImages[viewerProcess_i] = processArgs->colorPickerImage;
        bufferedFrame->colorPickerInputImages[viewerProcess_i] = processArgs->colorPickerInputImage;
        
    }


    void computeViewsForRoI(const ViewerNodePtr &viewer, const RectD* roiParam, const ViewerRenderBufferedFrameContainerPtr& framesContainer)
    {

        // Render each view sequentially. For now the viewer always asks to render 1 view since the interface can only allow 1 view at once per view
        for (std::size_t i = 0; i < _args->viewsToRender.size(); ++i) {

            ViewIdx view = _args->viewsToRender[i];

            // Create stats object if we want statistics
            RenderStatsPtr stats;
            if (_args->useStats) {
                stats.reset( new RenderStats(true) );
            }


            // Create the object wrapping the rendered image for this view
            ViewerRenderBufferedFramePtr bufferObject(new ViewerRenderBufferedFrame);
            bufferObject->view = view;
            bufferObject->stats = stats;
            bufferObject->isPartialRect = roiParam != 0;

            // Create a tree render object for both viewer process nodes
            std::vector<RenderViewerProcessFunctorArgsPtr> processArgs(2);
            for (int i = 0; i < 2; ++i) {
                processArgs[i].reset(new RenderViewerProcessFunctorArgs);
            }
            ViewerCompositingOperatorEnum viewerBlend = viewer->getCurrentOperator();
            bool viewerBEqualsViewerA = viewer->getCurrentAInput() == viewer->getCurrentBInput();

            // Launch the 2nd viewer process in a separate thread
            QFuture<void> processBFuture ;
            if (!viewerBEqualsViewerA && viewerBlend != eViewerCompositingOperatorNone) {
                processBFuture = QtConcurrent::run(boost::bind(&RenderCurrentFrameFunctorRunnable::createAndLaunchRenderInThread,
                                                               this,
                                                               viewer,
                                                               processArgs[1],
                                                               1,
                                                               _args->time,
                                                               stats,
                                                               _args->strokeItem,
                                                               roiParam,
                                                               bufferObject.get()));
            }

            // Launch the 1st viewer process in this thread
            createAndLaunchRenderInThread(viewer, processArgs[0], 0, _args->time, stats, _args->strokeItem, roiParam, bufferObject.get());

            // Wait for the 2nd viewer process
            if (viewerBlend != eViewerCompositingOperatorNone) {
                if (!viewerBEqualsViewerA) {
                    processBFuture.waitForFinished();
                } else  {
                    bufferObject->retCode[1] = processArgs[0]->retCode;
                    bufferObject->viewerProcessImageKey[1] = processArgs[0]->viewerProcessImageCacheKey;
                    bufferObject->viewerProcessImages[1] = processArgs[0]->outputImage;
                }
            }
            framesContainer->frames.push_back(bufferObject);

            
        } // for all views
        
    }

    virtual void run() OVERRIDE FINAL
    {

        ViewerNodePtr viewer = _args->viewer->isEffectViewerNode();

        // The object that contains frames that we want to upload to the viewer UI all at once
        ViewerRenderBufferedFrameContainerPtr framesContainer(new ViewerRenderBufferedFrameContainer);
        framesContainer->time = _args->time;
        framesContainer->recenterViewer = viewer->getViewerCenterPoint(&framesContainer->viewerCenter);


        if (viewer->isDoingPartialUpdates()) {
            // If the viewer is doing partial updates (i.e: during tracking we only update the markers areas)
            // Then we launch multiple renders over the partial areas
            std::list<RectD> partialUpdates = viewer->getPartialUpdateRects();
            for (std::list<RectD>::const_iterator it = partialUpdates.begin(); it != partialUpdates.end(); ++it) {
                computeViewsForRoI(viewer, &(*it), framesContainer);
            }
        } else {
            computeViewsForRoI(viewer, 0, framesContainer);
        }

        // Call updateViewer() on the main thread
        _args->scheduler->_publicInterface->s_doProcessFrameOnMainThread(_args->age, framesContainer);

        {
            // Remove the current render from the abortable renders list
            QMutexLocker k(&_args->scheduler->renderAgeMutex);
            for (TreeRenderSetOrderedByAge::iterator it = _args->scheduler->currentRenders.begin(); it != _args->scheduler->currentRenders.end(); ++it) {
                if ( it->age == _args->age ) {
                    _args->scheduler->currentRenders.erase(it);
                    break;
                }
            }
        }


        _args->scheduler->removeRunnableTask(this);
    } // run
};


ViewerCurrentFrameRequestScheduler::ViewerCurrentFrameRequestScheduler(const NodePtr& viewer)
: _imp( new ViewerCurrentFrameRequestSchedulerPrivate(this, viewer) )
{
    QObject::connect(this, SIGNAL(doProcessFrameOnMainThread(U64,BufferedFrameContainerPtr)), this, SLOT(onDoProcessFrameOnMainThreadReceived(U64,BufferedFrameContainerPtr)));
}

ViewerCurrentFrameRequestScheduler::~ViewerCurrentFrameRequestScheduler()
{
    // Should've been stopped before anyway
    if (_imp->backupThread.quitThread(false)) {
        _imp->backupThread.waitForAbortToComplete_enforce_blocking();
    }
}

void
ViewerCurrentFrameRequestScheduler::onDoProcessFrameOnMainThreadReceived(U64 age, const BufferedFrameContainerPtr& frames)
{
    assert(QThread::currentThread() == qApp->thread());
    _imp->processProducedFrame(age, frames);
}

void
ViewerCurrentFrameRequestSchedulerPrivate::processProducedFrame(U64 age, const BufferedFrameContainerPtr& frames)
{
    if (!frames) {
        return;
    }
    bool updateAge = false;
    // Do not process the produced frame if the age is now older than what is displayed
    {
        QMutexLocker k(&renderAgeMutex);
        if (age <= displayAge) {
            return;
        }

    }

    ViewerRenderBufferedFrameContainer* isViewerFrameContainer = dynamic_cast<ViewerRenderBufferedFrameContainer*>(frames.get());
    assert(isViewerFrameContainer);
    
    ViewerNodePtr viewerNode = viewer->isEffectViewerNode();


    for (std::list<BufferedFramePtr>::const_iterator it2 = isViewerFrameContainer->frames.begin(); it2 != isViewerFrameContainer->frames.end(); ++it2) {
        ViewerRenderBufferedFrame* viewerObject = dynamic_cast<ViewerRenderBufferedFrame*>(it2->get());
        assert(viewerObject);

        ViewerNode::UpdateViewerArgs args;
        args.time = isViewerFrameContainer->time;
        args.view = viewerObject->view;
        args.isPartialRect = viewerObject->isPartialRect;
        args.recenterViewer = isViewerFrameContainer->recenterViewer;
        args.viewerCenter = isViewerFrameContainer->viewerCenter;

        for (int i = 0; i < 2; ++i) {
            ViewerNode::UpdateViewerArgs::TextureUpload upload;
            upload.canonicalRoI = viewerObject->canonicalRoi[i];
            upload.image = viewerObject->viewerProcessImages[i];
            upload.colorPickerImage = viewerObject->colorPickerImages[i];
            upload.colorPickerInputImage = viewerObject->colorPickerInputImages[i];
            upload.viewerProcessImageKey = viewerObject->viewerProcessImageKey[i];
            if (viewerObject->retCode[i] == eActionStatusAborted ||
                (viewerObject->retCode[i] == eActionStatusOK && !upload.image)) {
                continue;
            }
            if (upload.canonicalRoI.isNull()) {
                continue;
            }
            args.viewerUploads[i].push_back(upload);
        }
        if (!args.viewerUploads[0].empty() || !args.viewerUploads[1].empty()) {
            updateAge = true;
            viewerNode->updateViewer(args);
        }

        if (viewerObject->stats) {
            double wallTime = 0;
            std::map<NodePtr, NodeRenderStats > statsMap = viewerObject->stats->getStats(&wallTime);
            viewerNode->reportStats(isViewerFrameContainer->time,  wallTime, statsMap);
        }

    }
    if (updateAge) {
        QMutexLocker k(&renderAgeMutex);
        // Update the display age
        displayAge = age;
    }
    // At least redraw the viewer, we might be here when the user removed a node upstream of the viewer.
    viewerNode->redrawViewer();
} // processProducedFrame

void
ViewerCurrentFrameRequestScheduler::onAbortRequested(bool keepOldestRender)
{
#ifdef TRACE_CURRENT_FRAME_SCHEDULER
    qDebug() << getThreadName().c_str() << "Received abort request";
#endif
    _imp->backupThread.abortThreadedTask();

    ViewerNodePtr viewerNode = _imp->viewer->isEffectViewerNode();

    // Do not abort the oldest render while scrubbing timeline or sliders so that the user gets some feedback
    const bool keepOldest = viewerNode->getApp()->isDraftRenderEnabled() || viewerNode->isDoingPartialUpdates() || keepOldestRender;

    QMutexLocker k(&_imp->renderAgeMutex);

    if ( _imp->currentRenders.empty() ) {
        return;
    }

    //Do not abort the oldest render, let it finish

    bool keptOneRenderActive = !keepOldest;

    // Only keep the oldest render active if it is worth displaying
    for (TreeRenderSetOrderedByAge::iterator it = _imp->currentRenders.begin(); it != _imp->currentRenders.end(); ++it) {
        bool hasRenderNotAborted = false;
        for (std::list<TreeRenderPtr>::const_iterator it2 = it->renders.begin(); it2 != it->renders.end(); ++it2) {
            if (!(*it2)->isRenderAborted()) {
                hasRenderNotAborted = true;
                break;
            }
        }
        if (!hasRenderNotAborted) {
            continue;
        }
        if (it->age >= _imp->displayAge && !keptOneRenderActive) {
            keptOneRenderActive = true;
            continue;
        }
        for (std::list<TreeRenderPtr>::const_iterator it2 = it->renders.begin(); it2 != it->renders.end(); ++it2) {
            (*it2)->setRenderAborted();
        }
    }
} // onAbortRequested

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
ViewerCurrentFrameRequestScheduler::renderCurrentFrameInternal(const boost::shared_ptr<CurrentFrameFunctorArgs>& args, bool useSingleThread)
{


    // Start the work in a thread of the thread pool if we can.
    // Let at least 1 free thread in the thread-pool to allow the renderer to use the thread pool if we use the thread-pool
    int maxThreads;
    if (useSingleThread) {
        maxThreads = 1;
    } else {
        maxThreads = _imp->threadPool->maxThreadCount();
    }

    if ( (maxThreads == 1) || (_imp->threadPool->activeThreadCount() >= maxThreads - 1) ) {
        _imp->backupThread.startTask(args);
    } else {

        // See bug https://bugreports.qt.io/browse/QTBUG-20251
        // The Qt thread-pool mem-leaks the runnable if using release/reserveThread
        // Instead we explicitly manage them and ensure they do not hold any external strong refs.
        boost::shared_ptr<RenderCurrentFrameFunctorRunnable> task(new RenderCurrentFrameFunctorRunnable(args));
        task->setAutoDelete(false);
        _imp->appendRunnableTask(task);
        _imp->threadPool->start(task.get());
    }

} // renderCurrentFrameInternal

void
ViewerCurrentFrameRequestScheduler::renderCurrentFrame(bool enableRenderStats)
{
    // Sanity check, also do not render viewer that are not made visible by the user
    NodePtr treeRoot = _imp->viewer;
    ViewerNodePtr viewerNode = treeRoot->isEffectViewerNode();
    if (!treeRoot || !viewerNode || !viewerNode->isViewerUIVisible() ) {
        return;
    }

    // We are about to trigger a new render, cancel all other renders except the oldest so user gets some feedback.
    onAbortRequested(true /*keepOldestRender*/);

    // Get the frame/view to render
    TimeValue frame;
    ViewIdx view;
    {
        frame = TimeValue(viewerNode->getTimeline()->currentFrame());
        int viewsCount = viewerNode->getRenderViewsCount();
        view = viewsCount > 0 ? viewerNode->getCurrentRenderView() : ViewIdx(0);
    }



    // Are we tracking ?
    bool isTracking = viewerNode->isDoingPartialUpdates();

    // Are we painting ?
    // While painting, use a single render thread and always the same thread.
    RotoStrokeItemPtr curStroke = _imp->viewer->getApp()->getActiveRotoDrawingStroke();


    // Ok we have to render at least one of A or B input
    boost::shared_ptr<CurrentFrameFunctorArgs> functorArgs( new CurrentFrameFunctorArgs(view,
                                                                                        frame,
                                                                                        enableRenderStats,
                                                                                        _imp->viewer,
                                                                                        _imp.get(),
                                                                                        curStroke) );



    // Identify this render request with an age

    {
        QMutexLocker k(&_imp->renderAgeMutex);
        functorArgs->age = _imp->renderAge;
        // If we reached the max amount of age, reset to 0... should never happen anyway
        if ( _imp->renderAge >= std::numeric_limits<U64>::max() ) {
            _imp->renderAge = 0;
        } else {
            ++_imp->renderAge;
        }
    }

    // When painting, limit the number of threads to 1 to be sure strokes are painted in the right order
    renderCurrentFrameInternal(functorArgs, curStroke || isTracking);



} // ViewerCurrentFrameRequestScheduler::renderCurrentFrame

bool
ViewerCurrentFrameRequestScheduler::hasThreadsAlive() const
{
    QMutexLocker k(&_imp->currentFrameRenderTasksMutex);
    return _imp->currentRenders.size() > 0;
}

ViewerCurrentFrameRequestRendererBackup::ViewerCurrentFrameRequestRendererBackup()
: GenericSchedulerThread()
{
    setThreadName("ViewerCurrentFrameRequestRendererBackup");
}

ViewerCurrentFrameRequestRendererBackup::~ViewerCurrentFrameRequestRendererBackup()
{
}

GenericSchedulerThread::TaskQueueBehaviorEnum
ViewerCurrentFrameRequestRendererBackup::tasksQueueBehaviour() const 
{
    return eTaskQueueBehaviorSkipToMostRecent;
}


GenericSchedulerThread::ThreadStateEnum
ViewerCurrentFrameRequestRendererBackup::threadLoopOnce(const GenericThreadStartArgsPtr& inArgs)
{
    boost::shared_ptr<CurrentFrameFunctorArgs> args = boost::dynamic_pointer_cast<CurrentFrameFunctorArgs>(inArgs);

    assert(args);
    RenderCurrentFrameFunctorRunnable task(args);
    task.run();

    return eThreadStateActive;
}

NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_OutputSchedulerThread.cpp"
