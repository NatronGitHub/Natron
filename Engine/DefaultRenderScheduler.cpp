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

#include "DefaultRenderScheduler.h"

#include <iostream>

#include <QTextStream>

#include "Engine/AppManager.h"
#include "Engine/AppInstance.h"
#include "Engine/EffectInstance.h"
#include "Engine/Node.h"
#include "Engine/RenderEngine.h"
#include "Engine/Timer.h"
#include "Engine/TreeRender.h"
#include "Engine/WriteNode.h"


NATRON_NAMESPACE_ENTER

class DefaultRenderFrameSubResult : public RenderFrameSubResult
{
public:

    DefaultRenderFrameSubResult()
    : RenderFrameSubResult()
    {}

    virtual ~DefaultRenderFrameSubResult()
    {

    }

    virtual ActionRetCodeEnum waitForResultsReady(const TreeRenderQueueProviderPtr& provider) OVERRIDE FINAL
    {
        ActionRetCodeEnum stat = provider->waitForRenderFinished(render);

        // We no longer need the render object since the processFrame() function does nothing.
        render.reset();

        return stat;
    }

    virtual void launchRenders(const TreeRenderQueueProviderPtr& provider) OVERRIDE FINAL
    {
        provider->launchRender(render);
    }

    virtual void abortRender() OVERRIDE FINAL
    {
        if (render) {
            render->setRenderAborted();
        }
    }


    TreeRenderPtr render;
};

struct DefaultScheduler::Implementation
{


    boost::scoped_ptr<TimeLapse> renderTimer; // Timer used to report stats when rendering

    ///When the render threads are not using the appendToBuffer API, the scheduler has no way to know the rendering is finished
    ///but to count the number of frames rendered via notifyFrameRended which is called by the render thread.
    mutable QMutex nFramesRenderedMutex;
    U64 nFramesRendered;

    mutable QMutex currentTimeMutex;
    TimeValue currentTime;


    mutable QMutex bufferedOutputMutex;
    int lastBufferedOutputSize;

    Implementation()
    : renderTimer()
    , nFramesRenderedMutex()
    , nFramesRendered(0)
    , currentTimeMutex()
    , currentTime(0)
    , bufferedOutputMutex()
    , lastBufferedOutputSize(0)
    {

    }
};

DefaultScheduler::DefaultScheduler(const RenderEnginePtr& engine,
                                   const NodePtr& effect)
: OutputSchedulerThread(engine, effect)
, _imp(new Implementation)
{
    setProcessFrameEnabled(true, eProcessFrameBySchedulerThread);
    engine->setPlaybackMode(ePlaybackModeOnce);
}

DefaultScheduler::~DefaultScheduler()
{
}

ActionRetCodeEnum
DefaultScheduler::createFrameRenderResults(TimeValue time, const std::vector<ViewIdx>& viewsToRender, bool enableRenderStats, RenderFrameResultsContainerPtr* future)
{
    // Even if enableRenderStats is false, we at least profile the time spent rendering the frame when rendering with a Write node.
    // Though we don't enable render stats for sequential renders (e.g: WriteFFMPEG) since this is 1 file.
    RenderStatsPtr stats;

    NodePtr outputNode = getOutputNode();
    WriteNodePtr isWrite;
    // If the output is a Write node, actually write is the internal write node encoder
    {
        isWrite = toWriteNode(outputNode->getEffectInstance());
        if (isWrite) {
            NodePtr embeddedWriter = isWrite->getEmbeddedWriter();
            if (embeddedWriter) {
                outputNode = embeddedWriter;
            }
        }
    }
    assert(outputNode);

    if (isWrite) {
        stats = boost::make_shared<RenderStats>(enableRenderStats);
    }

    TreeRenderQueueProviderPtr thisShared = shared_from_this();

    *future = boost::make_shared<RenderFrameResultsContainer>(thisShared);
    (*future)->time = time;

    for (std::size_t view = 0; view < viewsToRender.size(); ++view) {

        boost::shared_ptr<DefaultRenderFrameSubResult> subResults = boost::make_shared<DefaultRenderFrameSubResult>();
        subResults->view = viewsToRender[view];
        subResults->stats = stats;

        {
            TreeRender::CtorArgsPtr args = boost::make_shared<TreeRender::CtorArgs>();
            args->provider = thisShared;
            args->treeRootEffect = outputNode->getEffectInstance();
            args->time = time;
            args->view = viewsToRender[view];

            // Render by default on disk is always using a mipmap level of 0 but using the proxy scale of the project
            args->mipMapLevel = 0;

#pragma message WARN("Todo: set proxy scale here")
            args->proxyScale = RenderScale(1.);

            args->stats = stats;
            args->draftMode = false;
            args->playback = true;
            args->byPassCache = false;

            subResults->render = TreeRender::create(args);
            if (!subResults->render) {
                return eActionStatusFailed;
            }

        }
        (*future)->frames.push_back(subResults);
    }
    return eActionStatusOK;
} // createFrameRenderResults

class DefaultSchedulerProcessFrameArgs : public ProcessFrameArgsBase
{
public:

    OutputSchedulerThreadStartArgsPtr runArgs;

    DefaultSchedulerProcessFrameArgs()
    : ProcessFrameArgsBase()
    , runArgs()
    {

    }

    virtual ~DefaultSchedulerProcessFrameArgs()
    {
        
    }
};

ProcessFrameArgsBasePtr
DefaultScheduler::createProcessFrameArgs(const OutputSchedulerThreadStartArgsPtr& runArgs, const RenderFrameResultsContainerPtr& results)
{
    boost::shared_ptr<DefaultSchedulerProcessFrameArgs> processArgs = boost::make_shared<DefaultSchedulerProcessFrameArgs>();
    processArgs->results = results;
    processArgs->runArgs = runArgs;
    return processArgs;
} // createProcessFrameArgs

void
DefaultScheduler::processFrame(const ProcessFrameArgsBase& /*args*/)
{
    // We don't have to do anything: the write node's render action writes to the file for us.

} // processFrame


void
DefaultScheduler::onFrameProcessed(const ProcessFrameArgsBase& inArgs)
{

    const DefaultSchedulerProcessFrameArgs* args = dynamic_cast<const DefaultSchedulerProcessFrameArgs*>(&inArgs);

    const RenderFrameResultsContainerPtr& results = args->results;

    RenderEnginePtr engine = getEngine();

    // Report render stats if desired
    NodePtr effect = getOutputNode();
    for (std::list<RenderFrameSubResultPtr>::const_iterator it = results->frames.begin(); it != results->frames.end(); ++it) {
        if ((*it)->stats && (*it)->stats->isInDepthProfilingEnabled()) {
            engine->reportStats(results->time , (*it)->stats);
        }

    }

    U64 nbTotalFrames;
    U64 nbFramesRendered;

    bool renderFinished;
    {
        QMutexLocker l(&_imp->nFramesRenderedMutex);
        ++_imp->nFramesRendered;
        nbTotalFrames = std::ceil( (double)(args->runArgs->lastFrame - args->runArgs->firstFrame + 1) / args->runArgs->frameStep );
        nbFramesRendered = _imp->nFramesRendered;
        renderFinished = _imp->nFramesRendered == nbTotalFrames;
    }

    double fractionDone = (double)nbFramesRendered / nbTotalFrames;

    // If running in background, notify to the pipe that we rendered a frame
    if (appPTR->isBackground()) {
        assert(_imp->renderTimer);
        double timeSpentSinceStartSec = _imp->renderTimer->getTimeSinceCreation();
        double estimatedFps = (double)nbFramesRendered / timeSpentSinceStartSec;
        // total estimated time is: timeSpentSinceStartSec / fractionDone
        // remaning time is thus:
        double timeRemaining = (nbTotalFrames <= 0 || _imp->nFramesRendered <= 0) ? -1. : timeSpentSinceStartSec / fractionDone - timeSpentSinceStartSec;

        QString longMessage;
        QString frameStr = QString::number(results->time);
        QString fpsStr = QString::number(estimatedFps, 'f', 1);
        QString percentageStr = QString::number(fractionDone * 100, 'f', 1);
        QString timeRemainingStr = Timer::printAsTime(timeRemaining, true);

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

    // Fire the frameRendered signal on the RenderEngine
    engine->s_frameRendered(results->time, fractionDone);

    // Call Python after frame rendered callback
    runAfterFrameRenderedCallback(results->time);

    if (renderFinished) {
        setRenderFinished(renderFinished);
    }


    
} // onFrameProcessed



void
DefaultScheduler::timelineGoTo(TimeValue time)
{
    QMutexLocker k(&_imp->currentTimeMutex);
    _imp->currentTime =  time;
}

TimeValue
DefaultScheduler::timelineGetTime() const
{
    QMutexLocker k(&_imp->currentTimeMutex);
    return _imp->currentTime;
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
DefaultScheduler::onRenderFailed(ActionRetCodeEnum status)
{
    if ( appPTR->isBackground() ) {
        std::string message;

        if (status == eActionStatusFailed) {
            message = tr("Render Failed").toStdString();
        } else if (status == eActionStatusAborted) {
            message = tr("Render Aborted").toStdString();
        }
        if (!message.empty()) {
            std::cerr << message << std::endl;
        }
    }
}

void
DefaultScheduler::aboutToStartRender()
{

    {
        QMutexLocker k(&_imp->nFramesRenderedMutex);
        _imp->nFramesRendered = 0;
    }


    // Start measuring
    _imp->renderTimer.reset(new TimeLapse);

    OutputSchedulerThreadStartArgsPtr args = getCurrentRunArgs();
    NodePtr outputNode = getOutputNode();

    {
        QMutexLocker k(&_imp->currentTimeMutex);
        _imp->currentTime  = args->startingFrame;
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


} // DefaultScheduler::aboutToStartRender

void
DefaultScheduler::onRenderStopped(bool /*aborted*/)
{
    _imp->renderTimer.reset();


    NodePtr outputNode = getOutputNode();

    {
        QString longText = QString::fromUtf8( outputNode->getScriptName_mt_safe().c_str() ) + tr(" ==> Rendering finished");
        appPTR->writeToOutputPipe(longText, QString::fromUtf8(kRenderingFinishedStringShort), true);
    }

} // DefaultScheduler::onRenderStopped

NATRON_NAMESPACE_EXIT
NATRON_NAMESPACE_USING
#include "moc_DefaultRenderScheduler.cpp"
