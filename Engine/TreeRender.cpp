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

#include "TreeRender.h"

#include <set>
#include <QThread>
#include <QMutex>
#include <QTimer>
#include <QDebug>

#include "Engine/Image.h"
#include "Engine/EffectInstance.h"
#include "Engine/GPUContextPool.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/RotoStrokeItem.h"
#include "Engine/Settings.h"
#include "Engine/Timer.h"
#include "Engine/ThreadPool.h"
#include "Engine/TreeRenderNodeArgs.h"

// After this amount of time, if any thread identified in this render is still remaining
// that means they are stuck probably doing a long processing that cannot be aborted or in a separate thread
// that we did not spawn. Anyway, report to the user that we cannot control this thread anymore and that it may
// waste resources.
#define NATRON_ABORT_TIMEOUT_MS 5000


NATRON_NAMESPACE_ENTER;

typedef std::set<AbortableThread*> ThreadSet;

struct FindDependenciesNode
{
    // Did we already recurse on this node (i.e: did we call upstream on input nodes)
    bool recursed;

    // How many times did we visit this node
    int visitCounter;

    // A map of the hash for each frame/view pair
    FrameViewHashMap frameViewHash;


    FindDependenciesNode()
    : recursed(false)
    , visitCounter(0)
    , frameViewHash()
    {}
};


typedef std::map<NodePtr,FindDependenciesNode> FindDependenciesMap;


struct TreeRenderPrivate
{

    TreeRender* _publicInterface;

    // A map of the per node tree render args set on each node
    std::map<NodePtr, TreeRenderNodeArgsPtr> perNodeArgs;

    // The nodes that had thread local storage set on them
    NodesList nodes;

    // The main root of the tree from which we want the image
    NodePtr treeRoot;

    // the time to render
    double time;

    // the view to render
    ViewIdx view;

    // the OpenGL contexts
    OSGLContextWPtr openGLContext, cpuOpenGLContext;

    // Are we aborted ?
    QAtomicInt aborted;

    // Protects threadsForThisRender
    mutable QMutex threadsMutex;

    // A set of threads used in this render
    ThreadSet threadsForThisRender;

    // protects timerStarted, abortTimeoutTimer and ownerThread
    mutable QMutex timerMutex;

    // Used to track when a thread is stuck in an action after abort
    QTimer* abortTimeoutTimer;
    QThread* ownerThread;
    bool timerStarted;

    bool canAbort;
    bool isPlayback;
    bool isDraft;
    bool byPassCache;
    bool handleNaNs;
    bool useConcatenations;


    TreeRenderPrivate(TreeRender* publicInterface)
    : _publicInterface(publicInterface)
    , perNodeArgs()
    , nodes()
    , treeRoot()
    , time(0)
    , view()
    , openGLContext()
    , cpuOpenGLContext()
    , aborted()
    , threadsMutex()
    , threadsForThisRender()
    , timerMutex()
    , abortTimeoutTimer(0)
    , ownerThread(QThread::currentThread())
    , timerStarted(false)
    , canAbort(true)
    , isPlayback(false)
    , isDraft(false)
    , byPassCache(false)
    , handleNaNs(true)
    , useConcatenations(true)
    {
        aborted.fetchAndStoreAcquire(0);

        abortTimeoutTimer->setSingleShot(true);
        QObject::connect( abortTimeoutTimer, SIGNAL(timeout()), publicInterface, SLOT(onAbortTimerTimeout()) );
        QObject::connect( publicInterface, SIGNAL(startTimerInOriginalThread()), publicInterface, SLOT(onStartTimerInOriginalThreadTriggered()) );
    }

    /**
     * @brief Must be called right away after the constructor to initialize the data
     * specific to this render.
     **/
    void init(const TreeRender::CtorArgsPtr& inArgs, const TreeRenderPtr& publicInterface);

    /**
     * @brief Should be called before launching any call to renderRoI to optimize the render
     **/
    StatusEnum optimizeRoI(unsigned int mipMapLevel, const RectD& canonicalRoI);

    void fetchOpenGLContext(const TreeRender::CtorArgsPtr& inArgs);

    void getDependenciesRecursive_internal(const TreeRenderPtr& render,
                                           const NodePtr& node,
                                           double time,
                                           ViewIdx view,
                                           FindDependenciesMap& finalNodes,
                                           U64* nodeHash);


    void setNodeTLSInternal(const TreeRender::CtorArgsPtr& inArgs,
                            const NodePtr& node,
                            bool initTLS,
                            bool addFrameViewHash,
                            double time,
                            ViewIdx view,
                            U64 hash,
                            const OSGLContextPtr& gpuContext,
                            const OSGLContextPtr& cpuContext);
};



TreeRender::TreeRender()
: _imp(new TreeRenderPrivate(this))
{
}

TreeRender::~TreeRender()
{
    // post an event to delete the timer in the thread that created it
    if (_imp->abortTimeoutTimer) {
        _imp->abortTimeoutTimer->deleteLater();
    }

    // Invalidate the TLS on all nodes that had it set
    for (NodesList::iterator it = _imp->nodes.begin(); it != _imp->nodes.end(); ++it) {
        if ( !(*it) || !(*it)->getEffectInstance() ) {
            continue;
        }
        (*it)->getEffectInstance()->invalidateParallelRenderArgsTLS();
    }

}

TreeRenderNodeArgsPtr
TreeRender::getNodeRenderArgs(const NodePtr& node) const
{
    if (!node) {
        return TreeRenderNodeArgsPtr();
    }
    std::map<NodePtr, TreeRenderNodeArgsPtr>::const_iterator it = _imp->perNodeArgs.find(node);
    if (it == _imp->perNodeArgs.end()) {
        return TreeRenderNodeArgsPtr();
    }
    return it->second;
}

TreeRenderNodeArgsPtr
TreeRender::getOrCreateRenderArgs(const NodePtr& node)
{
    if (!node) {
        return TreeRenderNodeArgsPtr();
    }
    std::map<NodePtr, TreeRenderNodeArgsPtr>::const_iterator it = _imp->perNodeArgs.find(node);
    if (it != _imp->perNodeArgs.end()) {
        return it->second;
    }
    TreeRenderNodeArgsPtr ret(new TreeRenderNodeArgs(shared_from_this(), node));
    return ret;
}

bool
TreeRender::canAbort() const
{
    return _imp->canAbort;
}

bool
TreeRender::isAborted() const
{
    return (int)_imp->aborted > 0;
}

void
TreeRender::setAborted()
{
    int abortedValue = _imp->aborted.fetchAndAddAcquire(1);

    if (abortedValue > 0) {
        return;
    }
    bool callInSeparateThread = false;
    {
        QMutexLocker k(&_imp->timerMutex);
        _imp->timerStarted = true;
        callInSeparateThread = QThread::currentThread() != _imp->ownerThread;
    }

    // Star the timer in its owner thread, i.e the thread that created it
    if (callInSeparateThread) {
        Q_EMIT startTimerInOriginalThread();
    } else {
        onStartTimerInOriginalThreadTriggered();
    }
}

void
TreeRender::registerThreadForRender(AbortableThread* thread)
{
    QMutexLocker k(&_imp->threadsMutex);

    _imp->threadsForThisRender.insert(thread);
}

bool
TreeRender::unregisterThreadForRender(AbortableThread* thread)
{
    bool ret = false;
    bool threadsEmpty = false;
    {
        QMutexLocker k(&_imp->threadsMutex);
        ThreadSet::iterator found = _imp->threadsForThisRender.find(thread);

        if ( found != _imp->threadsForThisRender.end() ) {
            _imp->threadsForThisRender.erase(found);
            ret = true;
        }
        // Stop the timer if no more threads are running for this render
        threadsEmpty = _imp->threadsForThisRender.empty();
    }

    if (threadsEmpty) {
        {
            QMutexLocker k(&_imp->timerMutex);
            if (_imp->abortTimeoutTimer) {
                _imp->timerStarted = false;
            }
        }
    }

    return ret;
}

void
TreeRender::onStartTimerInOriginalThreadTriggered()
{
    assert(QThread::currentThread() == _imp->ownerThread);
    _imp->abortTimeoutTimer->start(NATRON_ABORT_TIMEOUT_MS);
}

void
TreeRender::onAbortTimerTimeout()
{
    {
        QMutexLocker k(&_imp->timerMutex);
        assert(QThread::currentThread() == _imp->ownerThread);
        _imp->abortTimeoutTimer->deleteLater();
        _imp->abortTimeoutTimer = 0;
        if (!_imp->timerStarted) {
            // The timer was stopped
            return;
        }
    }

    // Runs in the thread that called setAborted()
    ThreadSet threads;
    {
        QMutexLocker k(&_imp->threadsMutex);
        if ( _imp->threadsForThisRender.empty() ) {
            return;
        }
        threads = _imp->threadsForThisRender;
    }
    QString timeoutStr = Timer::printAsTime(NATRON_ABORT_TIMEOUT_MS / 1000, false);
    std::stringstream ss;

    ss << tr("One or multiple render seems to not be responding anymore after numerous attempt made by %1 to abort them for the last %2.").arg ( QString::fromUtf8( NATRON_APPLICATION_NAME) ).arg(timeoutStr).toStdString() << std::endl;
    ss << tr("This is likely due to a render taking too long in a plug-in.").toStdString() << std::endl << std::endl;

    std::stringstream ssThreads;
    ssThreads << tr("List of stalled render(s):").toStdString() << std::endl << std::endl;

    bool hasAtLeastOneThreadInNodeAction = false;
    for (ThreadSet::const_iterator it = threads.begin(); it != threads.end(); ++it) {
        std::string actionName;
        NodePtr node;
        (*it)->getCurrentActionInfos(&actionName, &node);
        if (node) {
            hasAtLeastOneThreadInNodeAction = true;
            // Don't show a dialog on timeout for writers since reading/writing from/to a file may be long and most libraries don't provide write callbacks anyway
            if ( node->getEffectInstance()->isReader() || node->getEffectInstance()->isWriter() ) {
                return;
            }
            std::string nodeName, pluginId;
            nodeName = node->getFullyQualifiedName();
            pluginId = node->getPluginID();

            ssThreads << " - " << (*it)->getThreadName()  << tr(" stalled in:").toStdString() << std::endl << std::endl;

            if ( !nodeName.empty() ) {
                ssThreads << "    Node: " << nodeName << std::endl;
            }
            if ( !pluginId.empty() ) {
                ssThreads << "    Plugin: " << pluginId << std::endl;
            }
            if ( !actionName.empty() ) {
                ssThreads << "    Action: " << actionName << std::endl;
            }
            ssThreads << std::endl;
        }
    }
    ss << std::endl;

    if (!hasAtLeastOneThreadInNodeAction) {
        return;
    } else {
        ss << ssThreads.str();
    }

    // Hold a sharedptr to this because it might get destroyed before the dialog returns
    TreeRenderPtr thisShared = shared_from_this();

    if ( appPTR->isBackground() ) {
        qDebug() << ss.str().c_str();
    } else {
        ss << tr("Would you like to kill these renders?").toStdString() << std::endl << std::endl;
        ss << tr("WARNING: Killing them may not work or may leave %1 in a bad state. The application may crash or freeze as a consequence of this. It is advised to restart %1 instead.").arg( QString::fromUtf8( NATRON_APPLICATION_NAME) ).toStdString();

        std::string message = ss.str();
        StandardButtonEnum reply = Dialogs::questionDialog(tr("A Render is not responding anymore").toStdString(), ss.str(), false, StandardButtons(eStandardButtonYes | eStandardButtonNo), eStandardButtonNo);
        if (reply == eStandardButtonYes) {
            // Kill threads
            QMutexLocker k(&_imp->threadsMutex);
            for (ThreadSet::const_iterator it = _imp->threadsForThisRender.begin(); it != _imp->threadsForThisRender.end(); ++it) {
                (*it)->killThread();
            }
        }
    }
} // onAbortTimerTimeout

void
TreeRenderPrivate::fetchOpenGLContext(const TreeRender::CtorArgsPtr& inArgs)
{

    // Ensure this thread gets an OpenGL context for the render of the frame
    OSGLContextPtr glContext, cpuContext;
    if (inArgs->activeRotoDrawableItem) {

        // When painting, always use the same context since we paint over the same texture
        assert(inArgs->activeRotoDrawableItem);
        RotoStrokeItem* isStroke = dynamic_cast<RotoStrokeItem*>(inArgs->activeRotoDrawableItem.get());
        assert(isStroke);
        if (isStroke) {
            isStroke->getDrawingGLContext(&glContext, &cpuContext);
            if (!glContext && !cpuContext) {
                try {
                    glContext = appPTR->getGPUContextPool()->getOrCreateOpenGLContext(true/*retrieveLastContext*/);
                    cpuContext = appPTR->getGPUContextPool()->getOrCreateCPUOpenGLContext(true/*retrieveLastContext*/);
                    isStroke->setDrawingGLContext(glContext, cpuContext);
                } catch (const std::exception& /*e*/) {

                }
            }
        }
    } else {
        try {
            glContext = appPTR->getGPUContextPool()->getOrCreateOpenGLContext(false/*retrieveLastContext*/);
            cpuContext = appPTR->getGPUContextPool()->getOrCreateCPUOpenGLContext(false/*retrieveLastContext*/);
        } catch (const std::exception& /*e*/) {

        }
    }

    openGLContext = glContext;
    cpuOpenGLContext = cpuContext;
}


void
TreeRenderPrivate::setNodeTLSInternal(const TreeRender::CtorArgsPtr& inArgs,
                                      const NodePtr& node,
                                      bool initTLS,
                                      bool addFrameViewHash,
                                      double time,
                                      ViewIdx view,
                                      U64 hash,
                                      const OSGLContextPtr& gpuContext,
                                      const OSGLContextPtr& cpuContext)
{
    EffectInstancePtr effect = node->getEffectInstance();
    assert(effect);

    if (!initTLS) {
        // Find a TLS object that was created in a call to this function before
        TreeRenderNodeArgsPtr curTLS = effect->getParallelRenderArgsTLS();
        if (!curTLS) {
            // Not found, create it
            initTLS = true;
        } else {
            assert(curTLS->request);
            // Found: add the hash for this frame/view pair and also increase the visitsCount
            if (addFrameViewHash) {
                curTLS->request->setFrameViewHash(time, view, hash);
            }
            curTLS->visitsCount = visitsCounter;

        }
    }

    // Create the TLS object for this frame render
    if (initTLS) {
        RenderSafetyEnum safety = node->getCurrentRenderThreadSafety();
        PluginOpenGLRenderSupport glSupport = node->getCurrentOpenGLRenderSupport();

        EffectInstance::SetParallelRenderTLSArgsPtr tlsArgs(new EffectInstance::SetParallelRenderTLSArgs);
        tlsArgs->time = inArgs->time;
        tlsArgs->view = inArgs->view;
        tlsArgs->treeRoot = inArgs->treeRoot;
        tlsArgs->glContext = gpuContext;
        tlsArgs->cpuGlContext = cpuContext;
        tlsArgs->currentThreadSafety = safety;
        tlsArgs->currentOpenGLSupport = glSupport;
        tlsArgs->doNanHandling = doNansHandling;
        tlsArgs->draftMode = inArgs->draftMode;
        tlsArgs->stats = inArgs->stats;
        tlsArgs->visitsCount = visitsCounter;
        tlsArgs->parent = _publicInterface->shared_from_this();

        ParallelRenderArgsPtr curTLS = effect->initParallelRenderArgsTLS(tlsArgs);
        assert(curTLS);
        if (addFrameViewHash) {
            curTLS->request->setFrameViewHash(time, view, hash);
        }
    }
}


/**
 * @brief Builds the internal render tree (including this node) and all its dependencies through expressions as well (which
 * also may be recursive).
 * The hash is also computed for each frame/view pair of each node.
 * This function throw exceptions upon error.
 **/
void
TreeRenderPrivate::getDependenciesRecursive_internal(const TreeRenderPtr& render,
                                                     const NodePtr& node,
                                                     double inArgsTime,
                                                     ViewIdx view,
                                                     FindDependenciesMap& finalNodes,
                                                     U64* nodeHash)
{



    // There may be cases where nodes gets added to the finalNodes in getAllExpressionDependenciesRecursive(), but we still
    // want to recurse upstream for them too
    bool foundInMap = false;
    bool alreadyRecursedUpstream = false;

    // Sanity check
    if ( !node || !node->isNodeCreated() ) {
        return;
    }

    EffectInstancePtr effect = node->getEffectInstance();
    if (!effect) {
        return;
    }

    // When building the render tree, the actual graph is flattened and groups no longer exist!
    assert(!dynamic_cast<NodeGroup*>(effect.get()));

    double time = inArgsTime;
    {
        int roundedTime = std::floor(time + 0.5);
        if (roundedTime != time && !effect->canRenderContinuously()) {
            time = roundedTime;
        }
    }

    FindDependenciesNode *nodeData = 0;

    // Check if we visited the node already
    {
        FindDependenciesMap::iterator found = finalNodes.find(node);
        if (found != finalNodes.end()) {
            nodeData = &found->second;
            foundInMap = true;
            if (found->second.recursed) {
                // We already called getAllUpstreamNodesRecursiveWithDependencies on its inputs
                // Increment the number of time we visited this node
                ++found->second.visitCounter;
                alreadyRecursedUpstream = true;
            } else {
                // Now we set the recurse flag
                nodeData->recursed = true;
                nodeData->visitCounter = 1;
            }

        }
    }

    if (!nodeData) {
        // Add this node to the set
        nodeData = &finalNodes[node];
        nodeData->recursed = true;
        nodeData->visitCounter = 1;
    }


    // If the frame args did not exist yet for this node, create them
    TreeRenderNodeArgsPtr frameArgs = render->getNodeRenderArgs(node);
    

    // Compute frames-needed, which will also give us the hash for this node.
    // This action will most likely make getValue calls on knobs, so we need to create the TLS object
    // on the effect with at least the render values cache even if we don't have yet other informations.
    U64 hashValue;
    FramesNeededMap framesNeeded = effect->getFramesNeeded_public(time, view, nodeData->visitCounter == 1 /*createTLS*/, render, &hashValue);


    // Now update the TLS with the hash value and initialize data if needed
    setNodeTLSInternal(inArgs, doNansHandling, node, nodeData->visitCounter == 1 /*initTLS*/, nodeData->visitCounter, true /*addFrameViewHash*/, time, view, hashValue, glContext, cpuContext);

    // If the node is within a group, also apply TLS on the group so that knobs have a consistant state throughout the render, even though
    // the group node will not actually render anything.
    {
        NodeGroupPtr isContainerAGroup = toNodeGroup(effect->getNode()->getGroup());
        if (isContainerAGroup) {
            isContainerAGroup->createFrameRenderTLS(inArgs->abortInfo);
            setNodeTLSInternal(inArgs, doNansHandling, isContainerAGroup->getNode(), true /*createTLS*/, 0 /*visitCounter*/, false /*addFrameViewHash*/, time, view, hashValue, glContext, cpuContext);
        }
    }


    // If we already got this node from expressions dependencies, don't do it again
    if (!foundInMap || !alreadyRecursedUpstream) {

        std::set<NodePtr> expressionsDeps;
        effect->getAllExpressionDependenciesRecursive(expressionsDeps);

        // Also add all expression dependencies but mark them as we did not recursed on them yet
        for (std::set<NodePtr>::iterator it = expressionsDeps.begin(); it != expressionsDeps.end(); ++it) {
            FindDependenciesNode n;
            n.recursed = false;
            n.visitCounter = 0;
            std::pair<FindDependenciesMap::iterator,bool> insertOK = finalNodes.insert(std::make_pair(node, n));

            // Set the TLS on the expression dependency
            if (insertOK.second) {
                (*it)->getEffectInstance()->createFrameRenderTLS(inArgs->abortInfo);
                setNodeTLSInternal(inArgs, doNansHandling, *it, true /*createTLS*/, 0 /*visitCounter*/, false /*addFrameViewHash*/, time, view, hashValue, glContext, cpuContext);
            }
        }
    }


    // Recurse on frames needed
    for (FramesNeededMap::const_iterator it = framesNeeded.begin(); it != framesNeeded.end(); ++it) {

        EffectInstancePtr inputEffect = EffectInstance::resolveInputEffectForFrameNeeded(it->first, effect.get());
        if (!inputEffect) {
            continue;
        }

        // For all views requested in input
        for (FrameRangesMap::const_iterator viewIt = it->second.begin(); viewIt != it->second.end(); ++viewIt) {

            // For all ranges in this view
            for (U32 range = 0; range < viewIt->second.size(); ++range) {

                // For all frames in the range
                for (double f = viewIt->second[range].min; f <= viewIt->second[range].max; f += 1.) {
                    U64 inputHash;
                    getDependenciesRecursive_internal(inArgs, doNansHandling, glContext, cpuContext, inputEffect->getNode(), treeRoot, f, viewIt->first, finalNodes, &inputHash);
                }
            }

        }
    }

    // For the viewer, since it has no hash of its own, do NOT set the frameViewHash.
    ViewerInstance* isViewerInstance = dynamic_cast<ViewerInstance*>(effect.get());
    if (!isViewerInstance) {
        FrameViewPair fv = {time, view};
        nodeData->frameViewHash[fv] = hashValue;
    }
    
    
    if (nodeHash) {
        *nodeHash = hashValue;
    }
    
} // getDependenciesRecursive_internal


void
TreeRenderPrivate::init(const TreeRender::CtorArgsPtr& inArgs, const TreeRenderPtr& publicInterface)
{
    assert(inArgs->treeRoot);

    time = inArgs->time;
    view = inArgs->view;
    treeRoot = inArgs->treeRoot;
    isPlayback = inArgs->playback;
    isDraft = inArgs->draftMode;
    byPassCache = inArgs->byPassCache;


    // If abortable thread, set abort info on the thread, to make the render abortable faster
    AbortableThread* isAbortable = dynamic_cast<AbortableThread*>( ownerThread );
    if (isAbortable) {
        isAbortable->setCurrentRender(publicInterface);
    }

    fetchOpenGLContext(inArgs);

    OSGLContextPtr glContext = _imp->openGLContext.lock(), cpuContext = _imp->cpuOpenGLContext.lock();

    bool doNanHandling = appPTR->getCurrentSettings()->isNaNHandlingEnabled();


    // Get dependencies node tree and apply TLS
    FindDependenciesMap dependenciesMap;
    getDependenciesRecursive_internal(inArgs, doNanHandling, glContext, cpuContext, inArgs->treeRoot, inArgs->treeRoot, inArgs->time, inArgs->view, dependenciesMap, 0);

    for (FindDependenciesMap::iterator it = dependenciesMap.begin(); it != dependenciesMap.end(); ++it) {
        nodes.push_back(it->first);
    }
    
    
}

ImagePtr
TreeRender::renderImage(const CtorArgsPtr& inArgs, ImagePtr* renderedImage, RenderRoIRetCode* renderStatus)
{
    TreeRenderPtr render(new TreeRender());
    try {
        render->_imp->init(inArgs, render);
    } catch (...) {
        *renderStatus = eRenderRoIRetCodeFailed;
    }

    EffectInstancePtr effectToRender = inArgs->treeRoot->getEffectInstance();


    // Use the provided RoI, otherwise render the RoD
    RectD canonicalRoi;
    RectI pixelRoI;
    if (inArgs->canonicalRoI) {
        canonicalRoi = *inArgs->canonicalRoI;
    } else {
        RenderScale scale = Image::getScaleFromMipMapLevel(inArgs->mipMapLevel);
        StatusEnum stat = effectToRender->getRegionOfDefinition_public(0, inArgs->time, scale, inArgs->view, &canonicalRoi);
        if (stat == eStatusFailed) {
            *renderStatus = eRenderRoIRetCodeFailed;
            return ImagePtr();
        }
    }

    // Cycle through the tree to make sure all nodes render once with the appropriate RoI
    {

        TreeRenderNodeArgsPtr nodeFrameArgs = render->getNodeRenderArgs(inArgs->treeRoot);

        // Should have been set in init() when calling getFramesNeeded
        assert(nodeFrameArgs);
        
        StatusEnum stat = EffectInstance::getInputsRoIsFunctor(time,
                                                               view,
                                                               inArgs->mipMapLevel,
                                                               render,
                                                               nodeFrameArgs,
                                                               inArgs->treeRoot,
                                                               inArgs->treeRoot,
                                                               inArgs->treeRoot,
                                                               canonicalRoi);

        if (stat == eStatusFailed) {
            *renderStatus = eRenderRoIRetCodeFailed;
            return ImagePtr();
        }
    }

    *renderStatus = effectToRender->renderRoI(RenderRoIArgs(time,
                                                            scale,
                                                            renderMappedMipMapLevel,
                                                            view,
                                                            false /*byPassCache*/,
                                                            pixelRoI,
                                                            requestedComps,
                                                            depth,
                                                            true,
                                                            shared_from_this(),
                                                            inputNb,
                                                            returnStorage,
                                                            thisEffectRenderTime,
                                                            inputImagesThreadLocal), &inputRenderResults);



    return ImagePtr();


} // renderImage

NATRON_NAMESPACE_EXIT;
