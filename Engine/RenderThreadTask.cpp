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

#include "RenderThreadTask.h"

#include "Engine/OutputSchedulerThread.h"


#include "Engine/AppManager.h"
#include "Engine/AppInstance.h"
#include "Engine/Node.h"
#include "Engine/TreeRender.h"
#include "Engine/WriteNode.h"

NATRON_NAMESPACE_ENTER

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





DefaultRenderFrameRunnable::DefaultRenderFrameRunnable(const NodePtr& writer,
                                                       OutputSchedulerThread* scheduler,
                                                       const TimeValue time,
                                                       const bool useRenderStats,
                                                       const std::vector<ViewIdx>& viewsToRender)
: RenderThreadTask(writer, scheduler, time, useRenderStats, viewsToRender)
, renderObjectsMutex()
, renderObjects()
{
}

void DefaultRenderFrameRunnable::abortRender()
{
    QMutexLocker k(&renderObjectsMutex);
    for (std::list<TreeRenderPtr>::const_iterator it = renderObjects.begin(); it != renderObjects.end(); ++it) {
        (*it)->setRenderAborted();
    }
}


void DefaultRenderFrameRunnable::runBeforeFrameRenderCallback(TimeValue frame, const NodePtr& outputNode)
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


ActionRetCodeEnum DefaultRenderFrameRunnable::renderFrameInternal(NodePtr outputNode,
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



void
DefaultRenderFrameRunnable::renderFrame(TimeValue time,
                                        const std::vector<ViewIdx>& viewsToRender,
                                        bool enableRenderStats) 
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


NATRON_NAMESPACE_EXIT

