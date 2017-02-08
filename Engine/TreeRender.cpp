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
#include <QtCore/QThread>
#include <QMutex>
#include <QTimer>
#include <QDebug>
#include <QWaitCondition>
#include <QRunnable>

#include "Engine/Image.h"
#include "Engine/EffectInstance.h"
#include "Engine/FrameViewRequest.h"
#include "Engine/GPUContextPool.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/RotoStrokeItem.h"
#include "Engine/Settings.h"
#include "Engine/Timer.h"
#include "Engine/ThreadPool.h"
#include "Engine/TLSHolder.h"

// After this amount of time, if any thread identified in this render is still remaining
// that means they are stuck probably doing a long processing that cannot be aborted or in a separate thread
// that we did not spawn. Anyway, report to the user that we cannot control this thread anymore and that it may
// waste resources.
#define NATRON_ABORT_TIMEOUT_MS 5000


NATRON_NAMESPACE_ENTER;

typedef std::set<AbortableThread*> ThreadSet;

enum TreeRenderStateEnum
{
    eTreeRenderStateOK,
    eTreeRenderStateInitFailed,
};


typedef std::set<FrameViewRequestPtr> DependencyFreeRenderSet;

struct TreeRenderPrivate
{

    TreeRender* _publicInterface;

    // Protects state
    mutable QMutex stateMutex;
    
    // The state of the object to avoid calling render on a failed tree
    ActionRetCodeEnum state;

    // A map of the per node tree render args set on each node
    std::map<EffectInstancePtr, EffectInstancePtr> renderClonesMap;

    // The nodes that had thread local storage set on them
    NodesList nodes;

    // The main root of the tree from which we want the image
    NodePtr treeRoot;
    
    // Render args of the root node
    EffectInstancePtr rootRenderClone;

    // the time to render
    TimeValue time;

    // the view to render
    ViewIdx view;

    // The RoI to render
    RectD canonicalRoI;
    
    // The plane to render
    ImagePlaneDesc plane;
    
    // The scale to apply to all parameters to obtain the final render
    RenderScale proxyScale;

    // The render mipmap level: this is an additional scale applied on top of the proxy scale.
    // A level of 0 is a factor of 1, a level 1 = 0.5, 2 = 0.25, etc...
    unsigned int mipMapLevel;

    // proxyScale * mipmap scale
    RenderScale proxyMipMapScale;

    // Rneder statistics
    RenderStatsPtr statsObject;

    // the OpenGL contexts
    OSGLContextWPtr openGLContext, cpuOpenGLContext;

    // Are we aborted ?
    QAtomicInt aborted;

    // Protects threadsForThisRender
    //mutable QMutex threadsMutex;

    // A set of threads used in this render
    //ThreadSet threadsForThisRender;

    // Protects dependencyFreeRenders and allRenderTasks
    mutable QMutex dependencyFreeRendersMutex;

    QWaitCondition dependencyFreeRendersEmptyCond;

    // A set of renders that we can launch right now
    DependencyFreeRenderSet dependencyFreeRenders;

    // All renders left to do
    std::set<FrameViewRequestPtr> allRenderTasksToProcess;

    // protects timerStarted, abortTimeoutTimer and ownerThread
    //mutable QMutex timerMutex;

    // Used to track when a thread is stuck in an action after abort
    //QTimer* abortTimeoutTimer;
    //QThread* ownerThread;
    //bool timerStarted;

    bool isPlayback;
    bool isDraft;
    bool byPassCache;
    bool handleNaNs;
    bool useConcatenations;


    TreeRenderPrivate(TreeRender* publicInterface)
    : _publicInterface(publicInterface)
    , stateMutex()
    , state(eActionStatusOK)
    , renderClonesMap()
    , nodes()
    , treeRoot()
    , rootRenderClone()
    , time(0)
    , view()
    , canonicalRoI()
    , plane()
    , proxyScale()
    , mipMapLevel(0)
    , proxyMipMapScale()
    , statsObject()
    , openGLContext()
    , cpuOpenGLContext()
    , aborted()
#if 0
    , threadsMutex()
    , threadsForThisRender()
#endif
    , dependencyFreeRendersMutex()
    , dependencyFreeRendersEmptyCond()
    , dependencyFreeRenders()
    , allRenderTasksToProcess()
#if 0
    , timerMutex()
    , abortTimeoutTimer(new QTimer)
    , ownerThread(QThread::currentThread())
    , timerStarted(false)
#endif
    , isPlayback(false)
    , isDraft(false)
    , byPassCache(false)
    , handleNaNs(true)
    , useConcatenations(true)
    {
        aborted.fetchAndStoreAcquire(0);
#if 0
        abortTimeoutTimer->setSingleShot(true);
        QObject::connect( abortTimeoutTimer, SIGNAL(timeout()), publicInterface, SLOT(onAbortTimerTimeout()) );
        QObject::connect( publicInterface, SIGNAL(startTimerInOriginalThread()), publicInterface, SLOT(onStartTimerInOriginalThreadTriggered()) );
#endif
    }

    /**
     * @brief Must be called right away after the constructor to initialize the data
     * specific to this render.
     * This returns a pointer to the render data for the tree root node.
     **/
    void init(const TreeRender::CtorArgsPtr& inArgs, const TreeRenderPtr& publicInterface);


    void fetchOpenGLContext(const TreeRender::CtorArgsPtr& inArgs);

    /**
     * @brief Builds the internal render tree (including this node) and all its dependencies through expressions as well (which
     * also may be recursive).
     * This function throw exceptions upon error.
     **/
    EffectInstancePtr buildRenderTreeRecursive(const NodePtr& node, std::set<NodePtr>* visitedNodes);


    EffectInstancePtr
    findRenderClone(const EffectInstancePtr& mainInstance) const
    {
        assert(!mainInstance->isRenderClone());
        std::map<EffectInstancePtr, EffectInstancePtr>::const_iterator it = renderClonesMap.find(mainInstance);
        if (it == renderClonesMap.end()) {
            return EffectInstancePtr();
        }
        return it->second;
    }


    void clearRenderClones();
};



TreeRender::TreeRender()
: _imp(new TreeRenderPrivate(this))
{
}

TreeRender::~TreeRender()
{
#if 0
    // post an event to delete the timer in the thread that created it
    if (_imp->abortTimeoutTimer) {
        _imp->abortTimeoutTimer->deleteLater();
    }
#endif
}


NodePtr
TreeRender::getTreeRoot() const
{
    return _imp->treeRoot;
}


OSGLContextPtr
TreeRender::getGPUOpenGLContext() const
{
    return _imp->openGLContext.lock();
}

OSGLContextPtr
TreeRender::getCPUOpenGLContext() const
{
    return _imp->cpuOpenGLContext.lock();
}


bool
TreeRender::isRenderAborted() const
{
    return (int)_imp->aborted > 0;
}

void
TreeRender::setRenderAborted()
{
#if 1
    _imp->aborted.fetchAndAddAcquire(1);
#else
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
#endif
}

bool
TreeRender::isPlayback() const
{
    return _imp->isPlayback;
}


bool
TreeRender::isDraftRender() const
{
    return _imp->isDraft;
}

bool
TreeRender::isByPassCacheEnabled() const
{
    return _imp->byPassCache;
}

bool
TreeRender::isNaNHandlingEnabled() const
{
    return _imp->handleNaNs;
}


bool
TreeRender::isConcatenationEnabled() const
{
    return _imp->useConcatenations;
}

TimeValue
TreeRender::getTime() const
{
    return _imp->time;
}

ViewIdx
TreeRender::getView() const
{
    return _imp->view;
}

const RenderScale&
TreeRender::getProxyScale() const
{
    return _imp->proxyScale;
}

unsigned int
TreeRender::getMipMapLevel() const
{
    return _imp->mipMapLevel;
}

const RenderScale&
TreeRender::getProxyMipMapScale() const
{
    return _imp->proxyMipMapScale;
}

RenderStatsPtr
TreeRender::getStatsObject() const
{
    return _imp->statsObject;
}

RectD
TreeRender::getCanonicalRoI() const
{
    return _imp->canonicalRoI;
}

#if 0
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
#endif

void
TreeRender::onStartTimerInOriginalThreadTriggered()
{
#if 0
    assert(QThread::currentThread() == _imp->ownerThread);
    _imp->abortTimeoutTimer->start(NATRON_ABORT_TIMEOUT_MS);
#endif
}

void
TreeRender::onAbortTimerTimeout()
{
#if 0
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
#endif
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



EffectInstancePtr
TreeRenderPrivate::buildRenderTreeRecursive(const NodePtr& node, std::set<NodePtr>* visitedNodes)
{

    assert(node);
    // Sanity check
    if ( !node || !node->isNodeCreated() ) {
        return EffectInstancePtr();
    }

    EffectInstancePtr mainInstance = node->getEffectInstance();
    if (!mainInstance) {
        return EffectInstancePtr();
    }

    if (visitedNodes->find(node) != visitedNodes->end()) {
        // Already visited this node
        EffectInstancePtr renderClone = findRenderClone(mainInstance);
        assert(renderClone);
        return renderClone;
    }

    // When building the render tree, the actual graph is flattened and groups no longer exist!
    assert(!dynamic_cast<NodeGroup*>(mainInstance.get()));

    visitedNodes->insert(node);

    // Recurse on all inputs to ensure they are part of the tree and make the connections to this
    // node render args
    std::vector<EffectInstancePtr> renderCloneInputs;
    {
        int nInputs = node->getMaxInputCount();
        renderCloneInputs.resize(nInputs);
        for (int i = 0; i < nInputs; ++i) {
            NodePtr inputNode = node->getInput(i);
            if (!inputNode) {
                continue;
            }

            renderCloneInputs[i] = buildRenderTreeRecursive(inputNode, visitedNodes);
        }
    }

    // Ensure this node has a render object. If this is the first time we visit this node it will create it.
    // The render object will copy and cache all knob values and inputs and anything that may change during
    // the render.
    // Since we did not make any action calls yet, we ensure that knob values remain the same throughout the render
    // as long as this object lives.
    EffectInstancePtr renderClone;
    {
        std::map<EffectInstancePtr, EffectInstancePtr>::const_iterator foundArgs = renderClonesMap.find(mainInstance);
        if (foundArgs != renderClonesMap.end()) {
            renderClone = foundArgs->second;
        } else {
            renderClone = toEffectInstance(mainInstance->createRenderClone(_publicInterface->shared_from_this()));
            assert(renderClone);
            renderClonesMap[mainInstance] = renderClone;

            for (std::size_t i = 0; i < renderCloneInputs.size(); ++i) {
                if (mainInstance->isInputMask(i) && !mainInstance->isMaskEnabled(i)) {
                    continue;
                }
                renderClone->setRenderCloneInput(renderCloneInputs[i], i);
            }
        }
    }


    // Visit all nodes that expressions of this node knobs may rely upon so we ensure they get a proper render object
    // and a render time and view when we run the expression.
    std::set<NodePtr> expressionsDeps;
    mainInstance->getAllExpressionDependenciesRecursive(expressionsDeps);

    for (std::set<NodePtr>::const_iterator it = expressionsDeps.begin(); it != expressionsDeps.end(); ++it) {
        buildRenderTreeRecursive(*it, visitedNodes);
    }

    return renderClone;
} // buildRenderTreeRecursive

void
TreeRenderPrivate::clearRenderClones()
{
    TreeRenderPtr thisShared = _publicInterface->shared_from_this();
    for (std::map<EffectInstancePtr, EffectInstancePtr>::const_iterator it = renderClonesMap.begin(); it != renderClonesMap.end(); ++it) {
        it->first->removeRenderClone(thisShared);
    }
    renderClonesMap.clear();
}

void
TreeRenderPrivate::init(const TreeRender::CtorArgsPtr& inArgs, const TreeRenderPtr& /*publicInterface*/)
{
    assert(inArgs->treeRoot);

    time = inArgs->time;
    view = inArgs->view;
    if (inArgs->canonicalRoI) {
        canonicalRoI = *inArgs->canonicalRoI;
    }
    if (inArgs->plane) {
        plane = *inArgs->plane;
    }
    proxyScale = inArgs->proxyScale;
    mipMapLevel = inArgs->mipMapLevel;
    double mipMapScale = Image::getScaleFromMipMapLevel(mipMapLevel);
    proxyMipMapScale.x = proxyScale.x * mipMapScale;
    proxyMipMapScale.y = proxyScale.y * mipMapScale;
    statsObject = inArgs->stats;
    treeRoot = inArgs->treeRoot;
    isPlayback = inArgs->playback;
    isDraft = inArgs->draftMode;
    byPassCache = inArgs->byPassCache;
    handleNaNs = appPTR->getCurrentSettings()->isNaNHandlingEnabled();


#if 0
    // If abortable thread, set abort info on the thread, to make the render abortable faster
    AbortableThread* isAbortable = dynamic_cast<AbortableThread*>( ownerThread );
    if (isAbortable) {
        isAbortable->setCurrentRender(publicInterface);
    }
#endif

    // Fetch the OpenGL context used for the render. It will not be attached to any render thread yet.
    fetchOpenGLContext(inArgs);


    // Build the render tree
    std::set<NodePtr> visitedNodes;
    rootRenderClone = buildRenderTreeRecursive(inArgs->treeRoot, &visitedNodes);
    
    EffectInstancePtr effectToRender = treeRoot->getEffectInstance();

    // Use the provided RoI, otherwise render the RoD
    if (canonicalRoI.isNull()) {
        GetRegionOfDefinitionResultsPtr results;
        ActionRetCodeEnum stat = effectToRender->getRegionOfDefinition_public(time, proxyMipMapScale, view, &results);
        if (isFailureRetCode(stat)) {
            state = stat;
            return;
        }
        assert(results);
        canonicalRoI = results->getRoD();
    }
    
    // Render by default the first plane available in output (usually the color plane)
    if (!inArgs->plane) {
        
        GetComponentsResultsPtr results;
        ActionRetCodeEnum stat = effectToRender->getLayersProducedAndNeeded_public(time, view, &results);
        if (isFailureRetCode(stat)) {
            state = stat;
            return;
        }
        assert(results);
        

        const std::list<ImagePlaneDesc>& producedLayers = results->getProducedPlanes();
        if (!producedLayers.empty()) {
            plane = producedLayers.front();
        }
    }

} // init

TreeRenderPtr
TreeRender::create(const CtorArgsPtr& inArgs)
{
    TreeRenderPtr render(new TreeRender());
    
    try {
        // Setup the render tree and make local copy of knob values for the render.
        // This will also set the per node render object in the TLS for OpenFX effects.
        render->_imp->init(inArgs, render);
    } catch (...) {
        render->_imp->state = eActionStatusFailed;


    }

     if (isFailureRetCode(render->_imp->state)) {
         render->_imp->clearRenderClones();
     }
    
    return render;
}

void
TreeRender::addDependencyFreeRender(const FrameViewRequestPtr& render)
{
    QMutexLocker k(&_imp->dependencyFreeRendersMutex);
    _imp->dependencyFreeRenders.insert(render);
}

void
TreeRender::addTaskToRender(const FrameViewRequestPtr& render)
{
     QMutexLocker k(&_imp->dependencyFreeRendersMutex);
    _imp->allRenderTasksToProcess.insert(render);
}

class TLSCleanupRAII
{
    TreeRenderPrivate* _imp;
public:

    TLSCleanupRAII(TreeRenderPrivate* imp)
    : _imp(imp)
    {

    }

    ~TLSCleanupRAII()
    {
        _imp->clearRenderClones();
    }
};


class FrameViewRenderRunnable : public QRunnable
{

    TreeRenderPrivate* _imp;
    FrameViewRequestPtr _request;

public:

    FrameViewRenderRunnable(TreeRenderPrivate* imp, const FrameViewRequestPtr& request)
    : QRunnable()
    , _imp(imp)
    , _request(request)
    {
        assert(request);
    }

    virtual ~FrameViewRenderRunnable()
    {

    }

private:

    virtual void run() OVERRIDE FINAL
    {

        EffectInstancePtr renderClone = _request->getRenderClone();
        ActionRetCodeEnum stat = renderClone->launchRender(_request);
        if (isFailureRetCode(stat)) {
            QMutexLocker k(&_imp->stateMutex);
            _imp->state = stat;
        }



        QMutexLocker k(&_imp->dependencyFreeRendersMutex);

        // Remove this render from all tasks left
        {
            std::set<FrameViewRequestPtr>::const_iterator foundTask = _imp->allRenderTasksToProcess.find(_request);
            assert(foundTask != _imp->allRenderTasksToProcess.end());
            if (foundTask != _imp->allRenderTasksToProcess.end()) {
                _imp->allRenderTasksToProcess.erase(foundTask);
            }
        }

        // For each frame/view that depend on this frame, remove it from the dependencies list.
        std::list<FrameViewRequestPtr> listeners = _request->getListeners();
        for (std::list<FrameViewRequestPtr>::const_iterator it = listeners.begin(); it != listeners.end(); ++it) {
            (*it)->markDependencyAsRendered(_request);

            // If the task has all its dependencies available, add it to the render queue.
            if ((*it)->getNumDependencies() == 0) {
                _imp->dependencyFreeRenders.insert(*it);
            }
        }

        // Notify the main render thread that some more stuff can be rendered (or that we are done)
        _imp->dependencyFreeRendersEmptyCond.wakeOne();
    }
};

ActionRetCodeEnum
TreeRender::launchRender(FrameViewRequestPtr* outputRequest)
{

    TLSCleanupRAII tlsCleaner(_imp.get());

    if (isFailureRetCode(_imp->state)) {
        return _imp->state;
    }

    EffectInstancePtr effectToRender = _imp->treeRoot->getEffectInstance();


    // Cycle through the tree to find and requested frames and RoIs
    {
        ActionRetCodeEnum stat = _imp->rootRenderClone->requestRender(_imp->time, _imp->view, _imp->mipMapLevel, _imp->plane, _imp->canonicalRoI, -1, FrameViewRequestPtr(), outputRequest);

        if (isFailureRetCode(stat)) {
            return stat;
        }
    }

    // At this point, the request pass should have created the first batch of dependency-free renders.
    // The list cannot be empty, otherwise it should have failed before.
    assert(!_imp->dependencyFreeRenders.empty());
    if (_imp->dependencyFreeRenders.empty()) {
        return eActionStatusFailed;
    }

    int numTasksRemaining;
    {
        QMutexLocker k(&_imp->dependencyFreeRendersMutex);
        numTasksRemaining = _imp->allRenderTasksToProcess.size();
    }
    QThreadPool* threadPool = QThreadPool::globalInstance();

    // While we still have tasks to render loop
    while (numTasksRemaining > 0) {


        // Launch all dependency-free tasks in parallel
        QMutexLocker k(&_imp->dependencyFreeRendersMutex);
        while (_imp->dependencyFreeRenders.size() > 0) {

            FrameViewRequestPtr request = *_imp->dependencyFreeRenders.begin();
            _imp->dependencyFreeRenders.erase(_imp->dependencyFreeRenders.begin());

            threadPool->start(new FrameViewRenderRunnable(_imp.get(), request));
        }

        // Wait until a task is finished: we should be able to launch more tasks afterwards.
        _imp->dependencyFreeRendersEmptyCond.wait(&_imp->dependencyFreeRendersMutex);

        // We have been woken-up by a finished task, check if the render is still OK
        {
            QMutexLocker k(&_imp->stateMutex);
            if (isFailureRetCode(_imp->state)) {
                return _imp->state;
            }
        }

        numTasksRemaining = _imp->allRenderTasksToProcess.size();
    }

    
    return eActionStatusOK;

} // launchRender

NATRON_NAMESPACE_EXIT;
NATRON_NAMESPACE_USING;
#include "moc_TreeRender.cpp"
