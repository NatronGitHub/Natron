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


#include "RenderEngine.h"

#include <SequenceParsing.h>

#include <QMutex>
#include <QCoreApplication>
#include <QThread>

#include "Global/FStreamsSupport.h"
#include "Global/QtCompat.h"


#include "Engine/AppInstance.h"
#include "Engine/CurrentFrameRequestScheduler.h"
#include "Engine/DefaultRenderScheduler.h"
#include "Engine/OutputSchedulerThread.h"
#include "Engine/Node.h"
#include "Engine/KnobTypes.h"
#include "Engine/KnobFile.h"
#include "Engine/Project.h"
#include "Engine/Timer.h"
#include "Engine/RenderStats.h"
#include "Engine/GenericSchedulerThreadWatcher.h"
#include "Engine/ViewerDisplayScheduler.h"
#include "Engine/ViewerNode.h"


NATRON_NAMESPACE_ENTER


struct RenderEnginePrivate
{
    QMutex schedulerCreationLock;
    OutputSchedulerThreadPtr scheduler;

    //If true then a current frame render can start playback, protected by abortedRequestedMutex
    bool canAutoRestartPlayback;
    QMutex canAutoRestartPlaybackMutex; // protects abortRequested
    NodeWPtr output;
    mutable QMutex pbModeMutex;
    PlaybackModeEnum pbMode;
    ViewerCurrentFrameRequestSchedulerPtr currentFrameScheduler;

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
    , scheduler()
    , canAutoRestartPlayback(false)
    , canAutoRestartPlaybackMutex()
    , output(output)
    , pbModeMutex()
    , pbMode(ePlaybackModeLoop)
    , currentFrameScheduler()
    , refreshQueue()
    {
    }
};


RenderEngine::RenderEngine(const NodePtr& output)
: _imp( new RenderEnginePrivate(output) )
{
    QObject::connect(this, SIGNAL(currentFrameRenderRequestPosted()), this, SLOT(onCurrentFrameRenderRequestPosted()), Qt::QueuedConnection);
}


// make_shared enabler (because make_shared needs access to the private constructor)
// see https://stackoverflow.com/a/20961251/2607517
struct RenderEngine::MakeSharedEnabler: public RenderEngine
{
    MakeSharedEnabler(const NodePtr& output) : RenderEngine(output) {
    }
};


RenderEnginePtr
RenderEngine::create(const NodePtr& output)
{
    return boost::make_shared<RenderEngine::MakeSharedEnabler>(output);
}


RenderEngine::~RenderEngine()
{
    // All renders should be finished
    assert(!_imp->scheduler || !_imp->scheduler->hasTreeRendersLaunched());
    assert(!_imp->currentFrameScheduler || !_imp->currentFrameScheduler->hasTreeRendersLaunched());
}


OutputSchedulerThreadPtr
RenderEngine::createScheduler(const NodePtr& effect)
{
    return DefaultScheduler::create(shared_from_this(), effect);
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
        _imp->currentFrameScheduler = ViewerCurrentFrameRequestScheduler::create(shared_from_this(), output);
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
RenderEngine::abortRenderingNoRestart()
{
    if ( abortRenderingInternal(false) ) {
        if (_imp->scheduler) {
            _imp->scheduler->clearSequentialRendersQueue();
        }
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
RenderEngine::hasActiveRender() const
{
    if (_imp->scheduler) {
        if (_imp->scheduler->hasTreeRendersLaunched()) {
            return true;
        }
    }
    if (_imp->currentFrameScheduler) {
        if (_imp->currentFrameScheduler->hasTreeRendersLaunched()) {
            return true;
        }
    }
    return false;
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


// make_shared enabler (because make_shared needs access to the private constructor)
// see https://stackoverflow.com/a/20961251/2607517
struct ViewerRenderEngine::MakeSharedEnabler: public ViewerRenderEngine
{
    MakeSharedEnabler(const NodePtr& output) : ViewerRenderEngine(output)
    {
    }
};

RenderEnginePtr
ViewerRenderEngine::create(const NodePtr& output)
{
    return boost::make_shared<ViewerRenderEngine::MakeSharedEnabler>(output);
}


OutputSchedulerThreadPtr
ViewerRenderEngine::createScheduler(const NodePtr& effect)
{
    return ViewerDisplayScheduler::create( shared_from_this(), effect );
}

void
ViewerRenderEngine::reportStats(TimeValue time,
                                const RenderStatsPtr& stats)
{
    ViewerNodePtr viewer = getOutput()->isEffectViewerNode();
    double wallTime = 0.;
    std::map<NodePtr, NodeRenderStats > statsMap = stats->getStats(&wallTime);
    viewer->reportStats(time, wallTime, statsMap);
}


NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_RenderEngine.cpp"
