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
#include "Engine/ProcessFrameThread.h"
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
#include "Engine/RenderThreadTask.h"
#include "Engine/RenderEngine.h"
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

NATRON_NAMESPACE_ENTER


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
        TimeValue startingFrame;
        bool useStats;
        bool blocking;
        RenderDirectionEnum direction;
    };

    mutable QMutex sequentialRenderQueueMutex;
    std::list<RenderSequenceArgs> sequentialRenderQueue;

    ProcessFrameThread processFrameThread;

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
        , processFrameThread()
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
                *nextFrame = TimeValue(frame + frameStep);
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
                *nextFrame = TimeValue(frame - frameStep);
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
        startingFrame = args->startingFrame;
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

            {
                ProcessFrameThreadStartArgsPtr processArgs(new ProcessFrameThreadStartArgs);
                processArgs->processor = this;
                processArgs->args.reset(new ProcessFrameArgsBase);
                processArgs->args->frames = framesToRender->frames;
                processArgs->executeOnMainThread = (_imp->mode == eProcessFrameByMainThread);
                _imp->processFrameThread.startTask(processArgs);
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
    _publicInterface->timelineGoTo(args.startingFrame);

    OutputSchedulerThreadStartArgsPtr threadArgs( new OutputSchedulerThreadStartArgs(args.blocking, args.useStats, args.firstFrame, args.lastFrame, args.startingFrame, args.frameStep, args.viewsToRender, args.direction) );

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
        args.startingFrame = direction == eRenderDirectionForward ? args.firstFrame : args.lastFrame;
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

    OutputSchedulerThreadPrivate::RenderSequenceArgs args;
    {
        args.blocking = isBlocking;
        args.useStats = enableRenderStats;
        args.firstFrame = firstFrame;
        args.lastFrame = lastFrame;
        args.startingFrame = currentTime;
        args.frameStep = frameStep;
        args.viewsToRender = viewsToRender;
        args.direction = timelineDirection;
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
}

void
OutputSchedulerThread::clearSequentialRendersQueue()
{
    QMutexLocker k(&_imp->sequentialRenderQueueMutex);
    _imp->sequentialRenderQueue.clear();
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





NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_OutputSchedulerThread.cpp"
