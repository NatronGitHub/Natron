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

#include "RenderQueue.h"

#include <QCoreApplication>
#include <QWaitCondition>
#include <QMutex>

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/CLArgs.h"
#include "Engine/EffectInstance.h"
#include "Engine/KnobFile.h"
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/GroupOutput.h"
#include "Engine/OutputSchedulerThread.h"
#include "Engine/ProcessHandler.h"
#include "Engine/Project.h"
#include "Engine/Settings.h"
#include "Engine/RenderEngine.h"


NATRON_NAMESPACE_ENTER

struct RenderQueueItem
{
    RenderQueue::RenderWork work;
    QString sequenceName;
    QString savePath;
    ProcessHandlerPtr process;
};

struct RenderQueuePrivate
{
    RenderQueue* _publicInterface;
    AppInstanceWPtr app;


    mutable QMutex renderQueueMutex;

    // Used to block the calling threads while there are active renders
    QWaitCondition activeRendersNotEmptyCond;
    std::list<RenderQueueItem> renderQueue, activeRenders;

    RenderQueuePrivate(RenderQueue* publicInterface, const AppInstancePtr& app)
    : _publicInterface(publicInterface)
    , app(app)
    , renderQueueMutex()
    , activeRendersNotEmptyCond()
    , renderQueue()
    , activeRenders()
    {

    }

    /**
     * @brief Creates render work request from the given command line arguments.
     * Note that this function may create write nodes if asked for from the command line arguments.
     * This function throw exceptions upon failure with a detailed error message.
     *
     * FrameRange is a list of pair<frameStep, pair<firstFrame, lastFrame> >
     **/
    void createRenderRequestsFromCommandLineArgsInternal(const std::list<std::pair<int, std::pair<int, int> > >& frameRanges,
                                                         bool useStats,
                                                         const std::list<CLArgs::WriterArg>& writers,
                                                         std::list<RenderQueue::RenderWork>& requests);

    /**
     * @brief The main function that handles the queueing
     **/
    void dispatchQueue(bool blocking, const std::list<RenderQueue::RenderWork>& writers);

    /**
     * @brief Remove the given writer from the active renders queue and startup a new render
     * if the queue is not empty
     **/
    void startNextQueuedRender(const NodePtr& finishedWriter);

    /**
     * @brief Validates the frame range and step from the args
     **/
    bool validateRenderOptions(RenderQueue::RenderWork& w);

    /**
     * @brief Get a label indicating the status of the render
     **/
    static void getWriterRenderLabel(const NodePtr& treeRoot, std::string* label);

    /**
     * @brief The internal function that launchs the render
     **/
    void renderInternal(const RenderQueueItem& writerWork);


    AppInstancePtr getApp() const
    {
        return app.lock();
    }

};

RenderQueue::RenderQueue(const AppInstancePtr& app)
: _imp(new RenderQueuePrivate(this, app))
{
}

RenderQueue::~RenderQueue()
{
    
}

void
RenderQueuePrivate::getWriterRenderLabel(const NodePtr& treeRoot,  std::string* label)
{

    KnobIPtr fileKnob = treeRoot->getKnobByName(kOfxImageEffectFileParamName);
    if (fileKnob) {
        KnobStringBasePtr isString = toKnobStringBase(fileKnob);
        assert(isString);
        if (isString) {
            *label = isString->getValue();;
        }
    }
}


bool
RenderQueuePrivate::validateRenderOptions(RenderQueue::RenderWork& w)
{

    if (w.renderLabel.empty()) {
        RenderQueuePrivate::getWriterRenderLabel(w.treeRoot, &w.renderLabel);
    }

    if ( (w.frameStep == INT_MAX) || (w.frameStep == INT_MIN) || w.frameStep == 0 ) {
        ///Get the frame step from the frame step parameter of the Writer
        w.frameStep = TimeValue(w.treeRoot->getEffectInstance()->getFrameStepKnobValue());
    }

    if (w.frameStep == 0) {
        Dialogs::errorDialog(w.treeRoot->getLabel_mt_safe(),
                             RenderQueue::tr("Frame step cannot be 0, it must either be negative or positive.").toStdString(), false );

        return false;
    }

    // Validate the frame range to render
    if ( (w.firstFrame == INT_MIN) || (w.lastFrame == INT_MAX) ) {
        {
            GetFrameRangeResultsPtr results;
            ActionRetCodeEnum stat = w.treeRoot->getEffectInstance()->getFrameRange_public(&results);
            if (!isFailureRetCode(stat)) {
                RangeD range;
                results->getFrameRangeResults(&range);
                w.firstFrame = TimeValue(range.min);
                w.lastFrame = TimeValue(range.max);
            }
        }
        if ( (w.firstFrame == INT_MIN) || (w.lastFrame == INT_MAX) ) {
            getApp()->getProject()->getFrameRange(&w.firstFrame, &w.lastFrame);
        }

        if (w.firstFrame > w.lastFrame && w.frameStep > 0) {
            Dialogs::errorDialog(w.treeRoot->getLabel_mt_safe(),
                                 RenderQueue::tr("First frame index in the sequence is greater than the last frame index but frame step is positive").toStdString(), false );

            return false;
        }

        if (w.firstFrame < w.lastFrame && w.frameStep < 0) {
            Dialogs::errorDialog(w.treeRoot->getLabel_mt_safe(),
                                 RenderQueue::tr("First frame index in the sequence is lesser than the last frame index but frame step is negative").toStdString(), false );

            return false;
        }
    }
    return true;
} // validateRenderOptions


void
RenderQueuePrivate::dispatchQueue(bool doBlockingRender, const std::list<RenderQueue::RenderWork>& writers)
{
    if ( writers.empty() ) {
        return;
    }

    AppInstancePtr app = getApp();

    // If queueing is enabled and we have to render multiple writers, render them in order
    const bool isQueuingEnabled = appPTR->getCurrentSettings()->isRenderQueuingEnabled();

    // If enabled, we launch the render in a separate process launching NatronRenderer
    const bool renderInSeparateProcess = appPTR->getCurrentSettings()->isRenderInSeparatedProcessEnabled();

    // When launching in a separate process, make a temporary save file that we pass to NatronRenderer
    QString savePath;
    if (renderInSeparateProcess) {
        app->getProject()->saveProject_imp(QString(), QString(), true /*isAutoSave*/, false /*updateprojectProperties*/, &savePath);
    }

    // For all items to render, create the background process if needed or connect signals to enable the queue
    // Also notify the GUI that an item was added to the queue
    std::list<RenderQueueItem> itemsToQueue;
    for (std::list<RenderQueue::RenderWork>::const_iterator it = writers.begin(); it != writers.end(); ++it) {
        RenderQueueItem item;
        item.work = *it;

        if (it->treeRoot->getEffectInstance()->getDisabledKnobValue()) {
            continue;
        }

        // Check that the render options are OK
        if ( !validateRenderOptions(item.work) ) {
            continue;
        }

        item.savePath = savePath;

        if (renderInSeparateProcess) {
            item.process = boost::make_shared<ProcessHandler>(savePath, item.work.treeRoot);
            QObject::connect( item.process.get(), SIGNAL(processFinished(int)), _publicInterface, SLOT(onBackgroundRenderProcessFinished()) );
        } else {
            QObject::connect(item.work.treeRoot->getRenderEngine().get(), SIGNAL(renderFinished(int)), _publicInterface, SLOT(onQueuedRenderFinished(int)), Qt::UniqueConnection);
        }

        bool canPause = !item.work.treeRoot->getEffectInstance()->isVideoWriter();

        if (!it->isRestart) {
            app->notifyRenderStarted(item.sequenceName, item.work.firstFrame, item.work.lastFrame, item.work.frameStep, canPause, item.work.treeRoot, item.process);
        } else {
            app->notifyRenderRestarted(item.work.treeRoot, item.process);
        }
        itemsToQueue.push_back(item);
    }

    // No valid render, bail out
    if ( itemsToQueue.empty() ) {
        return;
    }

    if (!isQueuingEnabled) {
        // Just launch everything
        for (std::list<RenderQueueItem>::const_iterator it = itemsToQueue.begin(); it != itemsToQueue.end(); ++it) {
            renderInternal(*it);
        }
    } else {
        QMutexLocker k(&renderQueueMutex);
        if ( !activeRenders.empty() ) {
            renderQueue.insert( renderQueue.end(), itemsToQueue.begin(), itemsToQueue.end() );

            return;
        } else {
            std::list<RenderQueueItem>::const_iterator it = itemsToQueue.begin();
            const RenderQueueItem& firstWork = *it;
            ++it;
            for (; it != itemsToQueue.end(); ++it) {
                renderQueue.push_back(*it);
            }
            k.unlock();
            renderInternal(firstWork);
        }
    }
    if (doBlockingRender) {
        QMutexLocker k(&renderQueueMutex);
        while (!activeRenders.empty()) {
            // check every 50ms if the queue is not empty
            k.unlock();
            // process events so that the onQueuedRenderFinished slot can be called
            // to clear the activeRenders queue if needed
            QCoreApplication::processEvents();
            k.relock();
            activeRendersNotEmptyCond.wait(&renderQueueMutex, 50);
        }
    }
    
    
} // dispatchQueue

void
RenderQueuePrivate::createRenderRequestsFromCommandLineArgsInternal(const std::list<std::pair<int, std::pair<int, int> > >& frameRanges,
                                                                    bool useStats,
                                                                    const std::list<CLArgs::WriterArg>& writers,
                                                                    std::list<RenderQueue::RenderWork>& requests)
{
    AppInstancePtr app = getApp();

    for (std::list<CLArgs::WriterArg>::const_iterator it = writers.begin(); it != writers.end(); ++it) {
        NodePtr writerNode;
        if (!it->mustCreate) {

            std::string writerName = it->name.toStdString();
            writerNode = app->getNodeByFullySpecifiedName(writerName);

            if (!writerNode) {
                QString s = RenderQueue::tr("%1 does not belong to the project file. Please enter a valid Write node script-name.").arg(it->name);
                throw std::invalid_argument( s.toStdString() );
            } else {
                if ( !writerNode->isOutputNode() ) {
                    QString s = RenderQueue::tr("%1 is not an output node! It cannot render anything.").arg(it->name);
                    throw std::invalid_argument( s.toStdString() );
                }
            }

            if (writerNode->getEffectInstance()->getDisabledKnobValue()) {
                continue;
            }
            if ( !it->filename.isEmpty() ) {
                KnobIPtr fileKnob = writerNode->getKnobByName(kOfxImageEffectFileParamName);
                if (fileKnob) {
                    KnobFilePtr outFile = toKnobFile(fileKnob);
                    if (outFile) {
                        outFile->setValue(it->filename.toStdString());
                    }
                }
            }
        } else {
            CreateNodeArgsPtr args(CreateNodeArgs::create(PLUGINID_NATRON_WRITE, app->getProject()));
            args->setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
            args->setProperty<bool>(kCreateNodeArgsPropSettingsOpened, false);
            args->setProperty<bool>(kCreateNodeArgsPropAutoConnect, false);
            writerNode = app->createWriter( it->filename.toStdString(), args );
            if (!writerNode) {
                throw std::runtime_error( RenderQueue::tr("Failed to create writer for %1.").arg(it->filename).toStdString() );
            }

            //Connect the writer to the corresponding Output node input
            NodePtr output = app->getProject()->getNodeByFullySpecifiedName( it->name.toStdString() );
            if (!output) {
                throw std::invalid_argument( RenderQueue::tr("%1 is not the name of a valid Output node of the script").arg(it->name).toStdString() );
            }
            GroupOutputPtr isGrpOutput = toGroupOutput( output->getEffectInstance() );
            if (!isGrpOutput) {
                throw std::invalid_argument( RenderQueue::tr("%1 is not the name of a valid Output node of the script").arg(it->name).toStdString() );
            }
            NodePtr outputInput = output->getRealInput(0);
            if (outputInput) {
                writerNode->connectInput(outputInput, 0);
            }
        }


        if ( !frameRanges.empty()) {
            for (std::list<std::pair<int, std::pair<int, int> > >::const_iterator it2 = frameRanges.begin(); it2 != frameRanges.end(); ++it2) {
                RenderQueue::RenderWork request;
                request.treeRoot = writerNode;
                request.firstFrame = TimeValue(it2->second.first);
                request.lastFrame = TimeValue(it2->second.second);
                request.frameStep = TimeValue(it2->first);
                request.useRenderStats = useStats;
                requests.push_back(request);
            }
        } else {
            RenderQueue::RenderWork request;
            request.treeRoot = writerNode;

            // These will be determined from knobs
            request.firstFrame = TimeValue(INT_MIN);
            request.lastFrame = TimeValue(INT_MAX);
            request.frameStep = TimeValue(INT_MIN);
            request.useRenderStats = useStats;
            requests.push_back(request);
        }
    } // for each writer cl args
} // createRenderRequestsFromCommandLineArgsInternal


void
RenderQueue::createRenderRequestsFromCommandLineArgs(const CLArgs& cl, std::list<RenderQueue::RenderWork>& requests)
{
    std::list<CLArgs::WriterArg> writerArgs = cl.getWriterArgs();
    if ( writerArgs.empty() ) {

        // Start rendering for all writers found in the project
        std::list<EffectInstancePtr> writeNodes;
        _imp->getApp()->getProject()->getWriters(&writeNodes);

        for (std::list<EffectInstancePtr>::const_iterator it2 = writeNodes.begin(); it2 != writeNodes.end(); ++it2) {
            CLArgs::WriterArg wArgs;
            wArgs.name = QString::fromUtf8((*it2)->getNode()->getScriptName().c_str());
            wArgs.mustCreate = false;
            writerArgs.push_back(wArgs);
        }
    }
    _imp->createRenderRequestsFromCommandLineArgsInternal(cl.getFrameRanges(), cl.areRenderStatsEnabled(), writerArgs, requests);
}


void
RenderQueue::createRenderRequestsFromCommandLineArgs(bool enableRenderStats,
                                                     const std::list<std::string>& writers,
                                                     const std::list<std::pair<int, std::pair<int, int> > >& frameRanges,
                                                     std::list<RenderWork>& requests)
{
    AppInstancePtr app = _imp->getApp();
    
    std::list<CLArgs::WriterArg> writerArgs;
    if ( writers.empty() ) {

        // Start rendering for all writers found in the project
        std::list<EffectInstancePtr> writeNodes;
        app->getProject()->getWriters(&writeNodes);

        for (std::list<EffectInstancePtr>::const_iterator it2 = writeNodes.begin(); it2 != writeNodes.end(); ++it2) {
            CLArgs::WriterArg wArgs;
            wArgs.name = QString::fromUtf8((*it2)->getNode()->getScriptName().c_str());
            wArgs.mustCreate = false;
            writerArgs.push_back(wArgs);
        }
    } else {
        for (std::list<std::string>::const_iterator it = writers.begin(); it != writers.end(); ++it) {
            CLArgs::WriterArg wArgs;
            wArgs.name = QString::fromUtf8(it->c_str());
            wArgs.mustCreate = false;
            writerArgs.push_back(wArgs);
        }
    }

    _imp->createRenderRequestsFromCommandLineArgsInternal(frameRanges, enableRenderStats, writerArgs, requests);


} // createRenderRequestsFromCommandLineArgs


void
RenderQueue::renderBlocking(const std::list<RenderWork>& writers)
{
    _imp->dispatchQueue(true /*blocking*/, writers);
}

void
RenderQueue::renderNonBlocking(const std::list<RenderWork>& writers)
{
    // Can only render non blocking if the application is not background
    bool blocking = appPTR->isBackground();

    _imp->dispatchQueue(blocking, writers);
}

void
RenderQueuePrivate::renderInternal(const RenderQueueItem& w)
{
    {
        QMutexLocker k(&renderQueueMutex);
        activeRenders.push_back(w);
    }
    if (w.process) {
        w.process->startProcess();
    } else {
        
        // Note that we don't need to make the sequential render blocking since we already block in dispatchQueue()
        // The views passed are empty, meaning we want to render all views, see OutputSchedulerThreadPrivate::validateRenderSequenceArgs
        w.work.treeRoot->getRenderEngine()->renderFrameRange(false /*blocking*/, w.work.useRenderStats, w.work.firstFrame, w.work.lastFrame, w.work.frameStep, std::vector<ViewIdx>() /*views*/, eRenderDirectionForward);
    }
}


void
RenderQueue::onQueuedRenderFinished(int /*retCode*/)
{
    RenderEngine* engine = qobject_cast<RenderEngine*>( sender() );

    if (!engine) {
        return;
    }
    NodePtr effect = engine->getOutput();
    if (!effect) {
        return;
    }
    _imp->startNextQueuedRender(effect);
}

void
RenderQueue::onBackgroundRenderProcessFinished()
{
    ProcessHandler* proc = qobject_cast<ProcessHandler*>( sender() );
    NodePtr effect;

    if (proc) {
        effect = proc->getWriter();
    }
    if (effect) {
        _imp->startNextQueuedRender(effect);
    }
}

void
RenderQueue::removeRenderFromQueue(const NodePtr& writer)
{
    QMutexLocker k(&_imp->renderQueueMutex);

    for (std::list<RenderQueueItem>::iterator it = _imp->renderQueue.begin(); it != _imp->renderQueue.end(); ++it) {
        if (it->work.treeRoot == writer) {
            _imp->renderQueue.erase(it);
            break;
        }
    }
}

void
RenderQueuePrivate::startNextQueuedRender(const NodePtr& finishedWriter)
{
    RenderQueueItem nextWork;

    // Do not make the process die under the mutex otherwise we may deadlock
    ProcessHandlerPtr processDying;
    {
        QMutexLocker k(&renderQueueMutex);
        for (std::list<RenderQueueItem>::iterator it = activeRenders.begin(); it != activeRenders.end(); ++it) {
            if (it->work.treeRoot == finishedWriter) {
                processDying = it->process;
                activeRenders.erase(it);
                activeRendersNotEmptyCond.wakeAll();
                break;
            }
        }
        if ( !renderQueue.empty() ) {
            nextWork = renderQueue.front();
            renderQueue.pop_front();
        } else {
            return;
        }
    }
    processDying.reset();

    renderInternal(nextWork);
}



NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_RenderQueue.cpp"
