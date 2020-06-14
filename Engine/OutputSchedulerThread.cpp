/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2020 The Natron developers
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

#ifdef DEBUG
#include "Global/FloatingPointExceptions.h"
#endif
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


#define kPythonCallbackPersistentMessageError "PythonCallbackPersistentMessageError"

NATRON_NAMESPACE_ENTER


struct RenderFrameResultsContainer_Compare
{
    bool operator()(const RenderFrameResultsContainerPtr& lhs, const RenderFrameResultsContainerPtr& rhs)
    {
        return lhs->time < rhs->time;
    }
};

typedef std::set<RenderFrameResultsContainerPtr, RenderFrameResultsContainer_Compare> FrameBuffer;


NATRON_NAMESPACE_ANONYMOUS_ENTER

class MetaTypesRegistration
{
public:
    inline MetaTypesRegistration()
    {
        qRegisterMetaType<RenderFrameResultsContainerPtr>("RenderFrameResultsContainerPtr");
    }
};

NATRON_NAMESPACE_ANONYMOUS_EXIT


static MetaTypesRegistration registration;


struct OutputSchedulerThreadPrivate
{
    OutputSchedulerThread* _publicInterface;

    // Ptr to the engine owning this scheduler
    RenderEngineWPtr engine;

    // A container for frames that we launched and are waiting results for
    FrameBuffer launchedFrames;
    mutable QMutex launchedFramesMutex;

    bool processFrameEnabled;
    OutputSchedulerThread::ProcessFrameModeEnum processFrameMode;

    Timer timer; // Timer regulating the engine execution. It is controlled by the GUI and MT-safe.

    // Protects renderFinished
    mutable QMutex renderFinishedMutex;

    // Set to true if the render should stop
    bool renderFinished;

    // Pointer to the args used in threadLoopOnce(), only usable from the scheduler thread
    boost::weak_ptr<OutputSchedulerThreadStartArgs> runArgs;


    mutable QMutex lastRunArgsMutex;
    std::vector<ViewIdx> lastPlaybackViewsToRender;
    RenderDirectionEnum lastPlaybackRenderDirection;


    // Protects lastFrameRequested & expectedFrameToRender & schedulerRenderDirection
    QMutex lastFrameRequestedMutex;

    // The last frame requested to render
    TimeValue lastFrameRequested;

    // The frame expected by the scheduler thread to be rendered
    TimeValue expectedFrameToRender;

    // The direction of the scheduler
    RenderDirectionEnum schedulerRenderDirection;

    NodeWPtr outputEffect; //< The effect used as output device


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

    OutputSchedulerThreadPrivate(const RenderEnginePtr& engine,
                                 OutputSchedulerThread* publicInterface,
                                 const NodePtr& effect)
        : _publicInterface(publicInterface)
        , engine(engine)
        , launchedFrames()
        , launchedFramesMutex()
        , processFrameEnabled(false)
        , processFrameMode(OutputSchedulerThread::eProcessFrameByMainThread)
        , timer()
        , renderFinishedMutex()
        , renderFinished(false)
        , runArgs()
        , lastRunArgsMutex()
        , lastPlaybackViewsToRender()
        , lastPlaybackRenderDirection(eRenderDirectionForward)
        , lastFrameRequestedMutex()
        , lastFrameRequested(0)
        , expectedFrameToRender(0)
        , schedulerRenderDirection(eRenderDirectionForward)
        , outputEffect(effect)
        , sequentialRenderQueueMutex()
        , sequentialRenderQueue()
        , processFrameThread()
    {
    }

    void validateRenderSequenceArgs(RenderSequenceArgs& args) const;

    void launchNextSequentialRender();

    /**
     * @brief Utility function to determine the next frame to render given the current context.
     **/
    static bool getNextFrameInSequence(PlaybackModeEnum pMode,
                                       RenderDirectionEnum direction,
                                       TimeValue frame,
                                       TimeValue firstFrame,
                                       TimeValue lastFrame,
                                       TimeValue frameStep,
                                       TimeValue* nextFrame,
                                       RenderDirectionEnum* newDirection);

    /**
     * @brief Utility function to determine the nearest frame within the range [firstFrame-lastFrame] ofr the given frame.
     **/
    static void getNearestInSequence(RenderDirectionEnum direction,
                                     TimeValue frame,
                                     TimeValue firstFrame,
                                     TimeValue lastFrame,
                                     TimeValue* nextFrame);

    void runBeforeFrameRenderCallback(TimeValue frame);

    void runAfterFrameRenderedCallback(TimeValue frame);

    void runBeforeRenderCallback();

    void runAfterRenderCallback(bool aborted);

    void runCallbackWithVariables(const QString& callback);


};

OutputSchedulerThread::OutputSchedulerThread(const RenderEnginePtr& engine,
                                             const NodePtr& effect)
    : GenericSchedulerThread()
    , _imp( new OutputSchedulerThreadPrivate(engine, this, effect) )
{
    QObject::connect( &_imp->timer, SIGNAL(fpsChanged(double,double)), engine.get(), SIGNAL(fpsChanged(double,double)) );

    setThreadName("Scheduler thread");
}

OutputSchedulerThread::~OutputSchedulerThread()
{

}

void
OutputSchedulerThread::setProcessFrameEnabled(bool enabled, OutputSchedulerThread::ProcessFrameModeEnum mode)
{
    _imp->processFrameEnabled = enabled;
    _imp->processFrameMode = mode;
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
OutputSchedulerThread::startFrameRenderFromLastStartedFrame()
{

    TimeValue frame;
    bool canContinue;

    {
        OutputSchedulerThreadStartArgsPtr args = getCurrentRunArgs();
        if (!args) {
            return;
        }
        PlaybackModeEnum pMode = _imp->engine.lock()->getPlaybackMode();

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
            if (canContinue) {
                _imp->lastFrameRequested = frame;
            }
        }
    }

    if (canContinue) {
        startFrameRender(frame);
    }
} // startFrameRenderFromLastStartedFrame

void
OutputSchedulerThread::requestMoreRenders()
{
    startFrameRenderFromLastStartedFrame();
} // requestMoreRenders

void
OutputSchedulerThread::startFrameRender(TimeValue startingFrame)
{

    OutputSchedulerThreadStartArgsPtr args = getCurrentRunArgs();

    _imp->runBeforeFrameRenderCallback(startingFrame);

    RenderFrameResultsContainerPtr future;
    ActionRetCodeEnum stat = createFrameRenderResults(startingFrame, args->viewsToRender, args->enableRenderStats, &future);
    if (isFailureRetCode(stat)) {
        notifyRenderFailure(stat);
        return;
    }


    {
        QMutexLocker k(&_imp->launchedFramesMutex);
        _imp->launchedFrames.insert(future);
    }


    // Launch the render
    future->launchRenders();


} // startFrameRender

void
OutputSchedulerThread::setRenderFinished(bool finished)
{
    QMutexLocker l(&_imp->renderFinishedMutex);
    _imp->renderFinished = finished;

}

void OutputSchedulerThreadPrivate::runBeforeFrameRenderCallback(TimeValue frame)
{
    NodePtr outputNode = outputEffect.lock();
    std::string cb = outputNode->getEffectInstance()->getBeforeFrameRenderCallback();
    if ( cb.empty() ) {
        return;
    }
    std::vector<std::string> args;
    std::string error;
    try {
        NATRON_PYTHON_NAMESPACE::getFunctionArguments(cb, &error, &args);
    } catch (const std::exception& e) {
        outputNode->getApp()->appendToScriptEditor( std::string("Failed to get signature of beforeFrameRendered callback: ")
                                                   + e.what() );

        return;
    }

    if ( !error.empty() ) {
        outputNode->getApp()->appendToScriptEditor("Failed to get signature of beforeFrameRendered callback: " + error);

        return;
    }

    std::string signatureError;
    signatureError.append("The beforeFrameRendered callback supports the following signature:\n");
    signatureError.append("- callback(frame, thisNode, app)");
    if ( (args.size() != 3) || (args[0] != "frame") || (args[1] != "thisNode") || (args[2] != "app") ) {
        outputNode->getApp()->appendToScriptEditor("Wrong signature of beforeFrameRendered callback: " + signatureError);

        return;
    }

    std::stringstream ss;
    std::string appStr = outputNode->getApp()->getAppIDString();
    std::string outputNodeName = appStr + "." + outputNode->getFullyQualifiedName();
    ss << cb << "(" << frame << ", " << outputNodeName << ", " << appStr << ")";
    std::string script = ss.str();
    try {
        runCallbackWithVariables( QString::fromUtf8( script.c_str() ) );
        outputNode->clearPersistentMessage(kPythonCallbackPersistentMessageError);
    } catch (const std::exception &e) {
        outputNode->setPersistentMessage(eMessageTypeWarning, kPythonCallbackPersistentMessageError, e.what());
    }
} // runBeforeFrameRenderCallback

void
OutputSchedulerThread::runAfterFrameRenderedCallback(TimeValue frame)
{
    _imp->runAfterFrameRenderedCallback(frame);
}

void
OutputSchedulerThreadPrivate::runAfterFrameRenderedCallback(TimeValue frame)
{
    NodePtr effect = outputEffect.lock();
    std::string cb = effect->getEffectInstance()->getAfterFrameRenderCallback();
    if ( cb.empty() ) {
        return;
    }
    std::vector<std::string> args;
    std::string error;
    try {
        NATRON_PYTHON_NAMESPACE::getFunctionArguments(cb, &error, &args);
    } catch (const std::exception& e) {
        effect->getApp()->appendToScriptEditor( std::string("Failed to get signature of afterFrameRendered callback: ")
                                               + e.what() );

        return;
    }

    if ( !error.empty() ) {
        effect->getApp()->appendToScriptEditor("Failed to get signature of afterFrameRendered callback: " + error);

        return;
    }

    std::string signatureError;
    signatureError.append("The afterFrameRendered callback supports the following signature(s):\n");
    signatureError.append("- callback(frame, thisNode, app)");
    if ( (args.size() != 3) || (args[0] != "frame") || (args[1] != "thisNode") || (args[2] != "app") ) {
        effect->getApp()->appendToScriptEditor("Wrong signature of afterFrameRendered callback: " + signatureError);

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
        effect->clearPersistentMessage(kPythonCallbackPersistentMessageError);
    } catch (const std::exception &e) {
        effect->setPersistentMessage(eMessageTypeWarning, kPythonCallbackPersistentMessageError, e.what());
    }


} // runAfterFrameRenderedCallback


void
OutputSchedulerThreadPrivate::runBeforeRenderCallback()
{
    NodePtr outputNode = outputEffect.lock();
    std::string cb = outputNode->getEffectInstance()->getBeforeRenderCallback();
    if (cb.empty()) {
        return;
    }
    std::vector<std::string> args;
    std::string error;
    try {
        NATRON_PYTHON_NAMESPACE::getFunctionArguments(cb, &error, &args);
    } catch (const std::exception& e) {
        outputNode->getApp()->appendToScriptEditor( std::string("Failed to get signature of beforeRender callback: ")
                                                   + e.what() );

        return;
    }

    if ( !error.empty() ) {
        outputNode->getApp()->appendToScriptEditor("Failed to get signature of beforeRender callback: " + error);

        return;
    }

    std::string signatureError;
    signatureError.append("The beforeRender callback supports the following signature:\n");
    signatureError.append("- callback(thisNode, app)");
    if ( (args.size() != 2) || (args[0] != "thisNode") || (args[1] != "app") ) {
        outputNode->getApp()->appendToScriptEditor("Wrong signature of beforeRender callback: " + signatureError);

        return;
    }


    std::stringstream ss;
    std::string appStr = outputNode->getApp()->getAppIDString();
    std::string outputNodeName = appStr + "." + outputNode->getFullyQualifiedName();
    ss << cb << "(" << outputNodeName << ", " << appStr << ")";
    std::string script = ss.str();
    try {
        runCallbackWithVariables( QString::fromUtf8( script.c_str() ) );
        outputNode->clearPersistentMessage(kPythonCallbackPersistentMessageError);
    } catch (const std::exception &e) {
        outputNode->setPersistentMessage(eMessageTypeWarning, kPythonCallbackPersistentMessageError, e.what());
    }

} // runBeforeRenderCallback

void
OutputSchedulerThreadPrivate::runAfterRenderCallback(bool aborted)
{
    NodePtr outputNode = outputEffect.lock();
    std::string cb = outputNode->getEffectInstance()->getAfterRenderCallback();
    if ( cb.empty() ) {
        return;
    }
    std::vector<std::string> args;
    std::string error;
    try {
        NATRON_PYTHON_NAMESPACE::getFunctionArguments(cb, &error, &args);
    } catch (const std::exception& e) {
        outputNode->getApp()->appendToScriptEditor( std::string("Failed to get signature of afterRender callback: ")
                                                   + e.what() );

        return;
    }

    if ( !error.empty() ) {
        outputNode->getApp()->appendToScriptEditor("Failed to get signature of afterRender callback: " + error);

        return;
    }

    std::string signatureError;
    signatureError.append("The afterRender callback supports the following signature:\n");
    signatureError.append("- callback(aborted, thisNode, app)");
    if ( (args.size() != 3) || (args[0] != "aborted") || (args[1] != "thisNode") || (args[2] != "app") ) {
        outputNode->getApp()->appendToScriptEditor("Wrong signature of afterRender callback: " + signatureError);

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
        outputNode->clearPersistentMessage(kPythonCallbackPersistentMessageError);
    } catch (const std::exception &e) {
        outputNode->setPersistentMessage(eMessageTypeWarning, kPythonCallbackPersistentMessageError, e.what());
    }


} // runAfterRenderCallback

void
OutputSchedulerThread::beginSequenceRender()
{
    if ( isFPSRegulationNeeded() ) {
        _imp->timer.playState = ePlayStateRunning;
    }

    setRenderFinished(false);


    // We will push frame to renders starting at startingFrame.
    // They will be in the range determined by firstFrame-lastFrame
    TimeValue startingFrame;
    TimeValue firstFrame, lastFrame;
    TimeValue frameStep;
    RenderDirectionEnum direction;

    {
        OutputSchedulerThreadStartArgsPtr args = getCurrentRunArgs();

        firstFrame = args->firstFrame;
        lastFrame = args->lastFrame;
        frameStep = args->frameStep;
        startingFrame = args->startingFrame;
        direction = args->direction;
    }

    _imp->runBeforeRenderCallback();
    aboutToStartRender();

    // Notify everyone that the render is started
    _imp->engine.lock()->s_renderStarted(direction == eRenderDirectionForward);



    ///If the output effect is sequential (only WriteFFMPEG for now)
    NodePtr node = _imp->outputEffect.lock();
    WriteNodePtr isWrite = toWriteNode( node->getEffectInstance() );
    if (isWrite) {
        NodePtr embeddedWriter = isWrite->getEmbeddedWriter();
        if (embeddedWriter) {
            node = embeddedWriter;
        }
    }
    SequentialPreferenceEnum pref = node->getEffectInstance()->getSequentialRenderSupport();
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

            _imp->engine.lock()->abortRenderingNoRestart();

            return;
        }
    }

    {
        QMutexLocker k(&_imp->lastFrameRequestedMutex);
        _imp->expectedFrameToRender = startingFrame;
        _imp->schedulerRenderDirection = direction;
        _imp->lastFrameRequested = startingFrame;
    }

    startFrameRender(startingFrame);


} // beginSequenceRender

void
OutputSchedulerThread::endSequenceRender()
{
    // Wait that all processFrames calls are finished
    _imp->processFrameThread.quitThread(true /*allowRestart*/, false /*abortTask*/);
    _imp->processFrameThread.waitForThreadToQuit_enforce_blocking();

    TimeValue firstFrame, lastFrame, frameStep;

    {
        QMutexLocker k(&_imp->lastRunArgsMutex);
        OutputSchedulerThreadStartArgsPtr args = getCurrentRunArgs();
        firstFrame = args->firstFrame;
        lastFrame = args->lastFrame;
        frameStep = args->frameStep;
        _imp->runArgs.reset();
    }

    _imp->timer.playState = ePlayStatePause;

    // Remove all active renders for the scheduler
    waitForAllTreeRenders();

    assert(!hasTreeRendersLaunched() && !hasTreeRendersFinished());

    RenderEnginePtr engine = _imp->engine.lock();

    // If the node is a writer, ensure that we point to the actual underlying encoder node and not the write node Group container.
    NodePtr node = _imp->outputEffect.lock();
    WriteNodePtr isWrite = toWriteNode( node->getEffectInstance() );
    if (isWrite) {
        NodePtr embeddedWriter = isWrite->getEmbeddedWriter();
        if (embeddedWriter) {
            node = embeddedWriter;
        }
    }


    // Call endSequenceRender action for a sequential writer (WriteFFMPEG)
    SequentialPreferenceEnum pref = node->getEffectInstance()->getSequentialRenderSupport();
    if ( (pref == eSequentialPreferenceOnlySequential) || (pref == eSequentialPreferencePreferSequential) ) {


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


    // Did we stop the render because we got aborted ?
    bool wasAborted = isBeingAborted();

    _imp->runAfterRenderCallback(wasAborted);
    onRenderStopped(wasAborted);

    // Notify everyone that the render is finished by triggering a signal on the RenderEngine
    engine->s_renderFinished(wasAborted ? 1 : 0);

    // When playback mode is play once disable auto-restart
    if (!wasAborted && engine->getPlaybackMode() == ePlaybackModeOnce) {
        engine->setPlaybackAutoRestartEnabled(false);
    }

    {
        QMutexLocker k(&_imp->launchedFramesMutex);
        _imp->launchedFrames.clear();
    }


    // Remove the render request and launch any pending playback request
    {
        QMutexLocker k(&_imp->sequentialRenderQueueMutex);
        if (!_imp->sequentialRenderQueue.empty()) {
            _imp->sequentialRenderQueue.pop_front();
        }
    }
    _imp->launchNextSequentialRender();

} // endSequenceRender

GenericSchedulerThread::ThreadStateEnum
OutputSchedulerThread::threadLoopOnce(const GenericThreadStartArgsPtr &inArgs)
{
    OutputSchedulerThreadStartArgsPtr args = boost::dynamic_pointer_cast<OutputSchedulerThreadStartArgs>(inArgs);
    assert(args);
    _imp->runArgs = args;

    ThreadStateEnum state = eThreadStateActive;

    TreeRenderQueueProviderPtr thisShared = shared_from_this();

    // Call beginSequenceRender callback and launch a frame render at the current frame
    beginSequenceRender();

    for (;;) {

        // True when the sequential render is finished
        bool renderFinished = false;

        {
            // _imp->renderFinished might be set in onFrameProcessed()
            QMutexLocker l(&_imp->renderFinishedMutex);
            if (_imp->renderFinished) {
                renderFinished = true;
            }
        }


        // Check if we were not aborted/failed
        if (state == eThreadStateActive) {
            state = resolveState();
        }
        if ( (state == eThreadStateAborted) || (state == eThreadStateStopped) ) {
            // Do not wait in the buf wait condition and go directly into the stopEngine()
            renderFinished = true;
        }


        // This is the frame that the scheduler expects to process now
        TimeValue expectedTimeToRender;
        {
            QMutexLocker k(&_imp->lastFrameRequestedMutex);
            expectedTimeToRender = _imp->expectedFrameToRender;
        }

        // Find results for the given frame
        RenderFrameResultsContainerPtr results;
        bool mustProcessFrame = false;
        if (!renderFinished) {
            {
                QMutexLocker l(&_imp->launchedFramesMutex);

                RenderFrameResultsContainerPtr tmp(new RenderFrameResultsContainer(thisShared));
                tmp->time = expectedTimeToRender;
                FrameBuffer::iterator found = _imp->launchedFrames.find(tmp);
                if (found != _imp->launchedFrames.end()) {
                    results = *found;
                } else {
                    qDebug() << "Launched render not found";
                    break;
                }
            }
            // Wait for the render to finish
            ActionRetCodeEnum status = results->waitForRendersFinished();
            if (isFailureRetCode(status)) {
                notifyRenderFailure(status);
                renderFinished = true;
            } else {
                mustProcessFrame = true;
            }
        }



        // Remove the results from the launched frames
        {
            QMutexLocker l(&_imp->launchedFramesMutex);

            RenderFrameResultsContainerPtr tmp(new RenderFrameResultsContainer(thisShared));
            tmp->time = expectedTimeToRender;
            FrameBuffer::iterator found = _imp->launchedFrames.find(tmp);
            assert(found != _imp->launchedFrames.end());
            if (found != _imp->launchedFrames.end()) {
                _imp->launchedFrames.erase(found);
            }
        }


        // Check if we were not aborted/failed
        if (!renderFinished) {
            state = resolveState();
            if ( (state == eThreadStateAborted) || (state == eThreadStateStopped) ) {
                // Do not wait in the buf wait condition and go directly into the stopEngine()
                renderFinished = true;
            }
        }


        // Start a new render before calling processFrame()
        TimeValue nextFrameToRender(-1.);
        if (!renderFinished) {

            RenderDirectionEnum newDirection = eRenderDirectionForward;


            // Refresh frame range arguments if needed (for viewers): the user may have changed it
            TimeValue firstFrame, lastFrame;
            getFrameRangeToRender(firstFrame, lastFrame);


            RenderDirectionEnum timelineDirection;
            TimeValue frameStep;

            // Refresh the firstframe/lastFrame as they might have changed on the timeline
            args->firstFrame = firstFrame;
            args->lastFrame = lastFrame;


            timelineDirection = _imp->schedulerRenderDirection;
            frameStep = args->frameStep;


            // Determine if we finished rendering or if we should just increment/decrement the timeline
            // or just loop/bounce
            PlaybackModeEnum pMode = _imp->engine.lock()->getPlaybackMode();
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

        } // !renderFinished

        if (!renderFinished) {

            {
                QMutexLocker k(&_imp->lastFrameRequestedMutex);
                _imp->expectedFrameToRender = nextFrameToRender;
            }


            startFrameRenderFromLastStartedFrame();
        }

        // Process the frame (for viewer playback this will upload the image to the OpenGL texture).
        // This is done by a separate thread so that this thread can launch other jobs.
        if (mustProcessFrame) {

            ProcessFrameThreadStartArgsPtr processArgs(new ProcessFrameThreadStartArgs);
            processArgs->processor = this;
            processArgs->args = createProcessFrameArgs(args, results);
            processArgs->executeOnMainThread = (_imp->processFrameMode == eProcessFrameByMainThread);
            if (_imp->processFrameEnabled) {
                _imp->processFrameThread.startTask(processArgs);
            } else {
                // Call onFrameProcessed directly
                onFrameProcessed(*processArgs->args);
            }
        }

        if (!renderFinished) {
            if (_imp->timer.playState == ePlayStateRunning) {
                _imp->timer.waitUntilNextFrameIsDue(); // timer synchronizing with the requested fps
            }

            state = resolveState();
            if ( (state == eThreadStateAborted) || (state == eThreadStateStopped) ) {
                // Do not wait in the buf wait condition and go directly into the stopEngine()
                renderFinished = true;
            }

            // The timeline might have changed if another thread moved the playhead
            TimeValue timelineCurrentTime = timelineGetTime();
            if (timelineCurrentTime != expectedTimeToRender) {
                timelineGoTo(timelineCurrentTime);
            } else {
                timelineGoTo(nextFrameToRender);
            }

        } else {
            if ( !_imp->engine.lock()->isPlaybackAutoRestartEnabled() ) {
                //Move the timeline to the last rendered frame to keep it in sync with what is displayed
                timelineGoTo( getLastRenderedTime() );
            }
            break;
        }

    } // for (;;)

    // Call the endSequenceRender callback and wait for any launched render
    endSequenceRender();

    return state;
} // OutputSchedulerThread::threadLoopOnce

void
OutputSchedulerThread::onAbortRequested(bool /*keepOldestRender*/)
{
    {
        QMutexLocker l(&_imp->launchedFramesMutex);
        for (FrameBuffer::iterator it = _imp->launchedFrames.begin(); it != _imp->launchedFrames.end(); ++it) {
            (*it)->abortRenders();
        }
    }
    _imp->processFrameThread.abortThreadedTask();
}

void
OutputSchedulerThread::onWaitForAbortCompleted()
{
    waitForAllTreeRenders();
    _imp->processFrameThread.waitForAbortToComplete_enforce_blocking();
}

void
OutputSchedulerThread::onWaitForThreadToQuit()
{
    waitForAllTreeRenders();
    _imp->processFrameThread.waitForThreadToQuit_enforce_blocking();
}

void
OutputSchedulerThread::onQuitRequested(bool allowRestarts)
{
    _imp->processFrameThread.quitThread(allowRestarts);

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
OutputSchedulerThreadPrivate::validateRenderSequenceArgs(RenderSequenceArgs& args) const
{
    NodePtr treeRoot = outputEffect.lock();


    if (args.viewsToRender.empty()) {
        int viewsCount = treeRoot->getApp()->getProject()->getProjectViewsCount();
        args.viewsToRender.resize(viewsCount);
        for (int i = 0; i < viewsCount; ++i) {
            args.viewsToRender[i] = ViewIdx(i);
        }
    }

    EffectInstancePtr effect = treeRoot->getEffectInstance();
    WriteNodePtr isWriter = toWriteNode(effect);
    // The effect is sequential (e.g: WriteFFMPEG), and thus cannot render multiple views, we have to choose one
    SequentialPreferenceEnum sequentiallity = isWriter ? isWriter->getEmbeddedWriter()->getEffectInstance()->getSequentialRenderSupport() : effect->getSequentialRenderSupport();

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

    bool viewAware = isWriter ? isWriter->getEmbeddedWriter()->getEffectInstance()->isViewAware() : effect->isViewAware();

    if (viewAware) {

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
                    QString message = Node::tr("%1 does not support multi-view, only the view %2 will be rendered.")
                    .arg( QString::fromUtf8( treeRoot->getLabel_mt_safe().c_str() ) )
                    .arg( QString::fromUtf8( mainViewName.c_str() ) );
                    if (!args.blocking) {
                        message.append( QChar::fromLatin1('\n') );
                        message.append( Node::tr("You can use the %v or %V indicator in the filename to render to separate files.\n"
                                                 "Would you like to continue?") );
                        StandardButtonEnum rep = Dialogs::questionDialog(Node::tr("Multi-view support").toStdString(), message.toStdString(), false, StandardButtons(eStandardButtonOk | eStandardButtonCancel), eStandardButtonOk);
                        if (rep != eStandardButtonOk) {
                            return;
                        }
                    } else {
                        Dialogs::warningDialog( Node::tr("Multi-view support").toStdString(), message.toStdString() );
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
            QString message = Node::tr("%1 does not support multi-view, only the view %2 will be rendered.")
            .arg( QString::fromUtf8( treeRoot->getLabel_mt_safe().c_str() ) )
            .arg( QString::fromUtf8( mainViewName.c_str() ) );
            if (!args.blocking) {
                message.append( QChar::fromLatin1('\n') );
                message.append( Node::tr("You can use the %v or %V indicator in the filename to render to separate files.\n"
                                         "Would you like to continue?") );
                StandardButtonEnum rep = Dialogs::questionDialog(Node::tr("Multi-view support").toStdString(), message.toStdString(), false, StandardButtons(eStandardButtonOk | eStandardButtonCancel), eStandardButtonOk);
                if (rep != eStandardButtonOk) {
                    // Notify progress that we were aborted
                    engine.lock()->s_renderFinished(1);

                    return;
                }
            } else {
                Dialogs::warningDialog( Node::tr("Multi-view support").toStdString(), message.toStdString() );
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
                QString message = Node::tr("You are trying to render the frame range [%1 - %2] but you did not specify any hash ('#') character(s) or printf-like format ('%d') for the padding. This will result in the same image being overwritten multiple times.").arg(args.firstFrame).arg(args.lastFrame);
                if (!args.blocking) {
                    message += QLatin1String("\n");
                    message += Node::tr("Would you like to continue?");
                    StandardButtonEnum rep = Dialogs::questionDialog(Node::tr("Image Sequence").toStdString(), message.toStdString(), false, StandardButtons(eStandardButtonOk | eStandardButtonCancel), eStandardButtonOk);
                    if (rep != eStandardButtonOk && rep != eStandardButtonYes) {
                        // Notify progress that we were aborted
                        engine.lock()->s_renderFinished(1);

                        return;
                    }
                } else {
                    Dialogs::warningDialog( Node::tr("Image Sequence").toStdString(), message.toStdString() );
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

    OutputSchedulerThreadStartArgsPtr threadArgs = boost::make_shared<OutputSchedulerThreadStartArgs>(args.blocking, args.useStats, args.firstFrame, args.lastFrame, args.startingFrame, args.frameStep, args.viewsToRender, args.direction);
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
OutputSchedulerThread::notifyRenderFailure(ActionRetCodeEnum status)
{
    assert(isFailureRetCode(status));

    OutputSchedulerThreadStartArgsPtr args = getCurrentRunArgs();
    assert(args);


    // Virtual portion callback
    onRenderFailed(status);

    // Abort renders
    if (status == eActionStatusAborted) {
        _imp->engine.lock()->abortRenderingAutoRestart();
    } else {
        _imp->engine.lock()->abortRenderingNoRestart();
    }

    if (args->isBlocking && QThread::currentThread() != this) {
        waitForAbortToComplete_enforce_blocking();
    }
}

OutputSchedulerThreadStartArgsPtr
OutputSchedulerThread::getCurrentRunArgs() const
{
    return _imp->runArgs.lock();
}


RenderEnginePtr
OutputSchedulerThread::getEngine() const
{
    return _imp->engine.lock();
}

void
OutputSchedulerThreadPrivate::runCallbackWithVariables(const QString& callback)
{
    if ( !callback.isEmpty() ) {
        NodePtr effect = outputEffect.lock();
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


NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_OutputSchedulerThread.cpp"
