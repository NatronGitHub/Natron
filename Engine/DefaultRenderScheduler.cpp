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

#include "Engine/AppManager.h"
#include "Engine/AppInstance.h"
#include "Engine/EffectInstance.h"
#include "Engine/Node.h"
#include "Engine/RenderEngine.h"
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

DefaultScheduler::DefaultScheduler(const RenderEnginePtr& engine,
                                   const NodePtr& effect)
: OutputSchedulerThread(engine, effect)
, _currentTimeMutex()
, _currentTime(0)
{
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
        stats.reset( new RenderStats(enableRenderStats) );
    }

    TreeRenderQueueProviderPtr thisShared = shared_from_this();

    future->reset(new RenderFrameResultsContainer(thisShared));
    (*future)->time = time;

    for (std::size_t view = 0; view < viewsToRender.size(); ++view) {

        boost::shared_ptr<DefaultRenderFrameSubResult> subResults(new DefaultRenderFrameSubResult);
        subResults->view = viewsToRender[view];
        subResults->stats = stats;

        {
            TreeRender::CtorArgsPtr args(new TreeRender::CtorArgs);
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


void
DefaultScheduler::processFrame(const ProcessFrameArgsBase& /*args*/)
{
    // We don't have to do anything: the write node's render action writes to the file for us.

} // processFrame


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
    OutputSchedulerThreadStartArgsPtr args = getCurrentRunArgs();
    NodePtr outputNode = getOutputNode();

    {
        QMutexLocker k(&_currentTimeMutex);
        _currentTime  = args->startingFrame;
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
    NodePtr outputNode = getOutputNode();

    {
        QString longText = QString::fromUtf8( outputNode->getScriptName_mt_safe().c_str() ) + tr(" ==> Rendering finished");
        appPTR->writeToOutputPipe(longText, QString::fromUtf8(kRenderingFinishedStringShort), true);
    }

} // DefaultScheduler::onRenderStopped

NATRON_NAMESPACE_EXIT
NATRON_NAMESPACE_USING
#include "moc_DefaultRenderScheduler.cpp"
