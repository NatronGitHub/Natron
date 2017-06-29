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
#include "Engine/GroupInput.h"
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

//#define TRACE_RENDER_DEPENDENCIES

// Define to disable multi-threading of branch evaluation in the compositing tree
//#define TREE_RENDER_DISABLE_MT


NATRON_NAMESPACE_ENTER

typedef std::set<AbortableThread*> ThreadSet;

enum TreeRenderStateEnum
{
    eTreeRenderStateOK,
    eTreeRenderStateInitFailed,
};

struct FrameViewRequestSharedDataPair
{
    FrameViewRequestPtr request;
    RequestPassSharedDataPtr launchData;
};

// Render first the tasks with more dependencies: it has more chance to make more dependency-free new renders
// to enable better concurrency
struct FrameViewRequestComparePriority
{
    RequestPassSharedDataPtr _launchData;

    FrameViewRequestComparePriority(const RequestPassSharedDataPtr& launchData)
    : _launchData(launchData)
    {

    }

    bool operator() (const FrameViewRequestPtr& lhs, const FrameViewRequestPtr& rhs) const
    {
#if 0
        // This code doesn't work 
        if (lhs.get() == rhs.get()) {
            return false;
        }
        int lNum = lhs->getNumListeners(_launchData);
        int rNum = rhs->getNumListeners(_launchData);
        if (lNum < rNum) {
            return true;
        } else if (lNum > rNum) {
            return false;
        } else {
            // Same number of listeners...there's no specific ordering
            return lhs.get() < rhs.get();
        }
#endif

        return lhs.get() < rhs.get();
    }
};

typedef std::set<FrameViewRequestPtr, FrameViewRequestComparePriority> DependencyFreeRenderSet;

struct TreeRenderPrivate
{

    TreeRender* _publicInterface;

    TreeRender::CtorArgsPtr ctorArgs;

    // Protects state
    mutable QMutex stateMutex;
    
    // The state of the object to avoid calling render on a failed tree
    ActionRetCodeEnum state;

    // All cloned knob holders for this render
    mutable QMutex renderClonesMutex;
    std::list<KnobHolderPtr> renderClones;

    // Map of nodes that belong to the tree upstream of tree root for which we desire
    // a pointer of the resulting image. This is useful for the Viewer to enable color-picking:
    // the output image is the image out of the ViewerProcess node, but what the user really
    // wants is the color-picker of the image in input of the Viewer (group) node.
    // These images can then be retrieved using the getExtraRequestedResultsForNode() function.
    std::map<NodePtr, FrameViewRequestPtr> extraRequestedResults;
    mutable QMutex extraRequestedResultsMutex;

    // While drawing a preview with the RotoPaint node, this is the bounding box of the area
    // to update on the viewer
    // Protected by extraRequestedResultsMutex
    RectI activeStrokeUpdateArea;
    bool activeStrokeUpdateAreaSet;

    // the OpenGL contexts
    OSGLContextWPtr openGLContext, cpuOpenGLContext;

    // Are we aborted ?
    QAtomicInt aborted;


    bool handleNaNs;
    bool useConcatenations;


    TreeRenderPrivate(TreeRender* publicInterface)
    : _publicInterface(publicInterface)
    , ctorArgs()
    , stateMutex()
    , state(eActionStatusOK)
    , renderClonesMutex()
    , renderClones()
    , extraRequestedResults()
    , extraRequestedResultsMutex()
    , activeStrokeUpdateArea()
    , activeStrokeUpdateAreaSet(false)
    , openGLContext()
    , cpuOpenGLContext()
    , aborted()
    , handleNaNs(true)
    , useConcatenations(true)
    {
        aborted.fetchAndStoreAcquire(0);

    }

    /**
     * @brief Must be called right away after the constructor to initialize the data
     * specific to this render.
     **/
    void init(const TreeRender::CtorArgsPtr& inArgs);


    void fetchOpenGLContext(const TreeRender::CtorArgsPtr& inArgs);

    static ActionRetCodeEnum getTreeRootRoD(const EffectInstancePtr& effect, TimeValue time, ViewIdx view, const RenderScale& scale, RectD* rod);

    static ActionRetCodeEnum getTreeRootPlane(const EffectInstancePtr& effect, TimeValue time, ViewIdx view, ImagePlaneDesc* plane);

    ActionRetCodeEnum launchRenderInternal(bool removeRenderClonesWhenFinished,
                                           const EffectInstancePtr& treeRoot,
                                           TimeValue time,
                                           ViewIdx view,
                                           const RenderScale& proxyScale,
                                           unsigned int mipMapLevel,
                                           const ImagePlaneDesc* plane,
                                           const RectD* canonicalRoI,
                                           FrameViewRequestPtr* outputRequest);
};

TreeRender::CtorArgs::CtorArgs()
: time(0)
, view(0)
, treeRootEffect()
, extraNodesToSample()
, activeRotoDrawableItem()
, stats()
, canonicalRoI(0)
, plane(0)
, proxyScale(1.)
, mipMapLevel(0)
, draftMode(false)
, playback(false)
, byPassCache(false)
{

}

TreeRender::TreeRender()
: _imp(new TreeRenderPrivate(this))
{
}

TreeRender::~TreeRender()
{

}


FrameViewRequestPtr
TreeRender::getExtraRequestedResultsForNode(const NodePtr& node) const
{
    QMutexLocker k(&_imp->extraRequestedResultsMutex);
    std::map<NodePtr, FrameViewRequestPtr>::const_iterator found = _imp->extraRequestedResults.find(node);
    if (found == _imp->extraRequestedResults.end()) {
        return FrameViewRequestPtr();
    }
    return found->second;
}

bool
TreeRender::isExtraResultsRequestedForNode(const NodePtr& node) const
{
    QMutexLocker k(&_imp->extraRequestedResultsMutex);
    std::map<NodePtr, FrameViewRequestPtr>::const_iterator found = _imp->extraRequestedResults.find(node);
    if (found == _imp->extraRequestedResults.end()) {
        return false;
    }
    return true;
}

bool
TreeRender::getRotoPaintActiveStrokeUpdateArea(RectI* updateArea) const
{
    QMutexLocker k(&_imp->extraRequestedResultsMutex);
    if (!_imp->activeStrokeUpdateAreaSet) {
        return false;
    }
    *updateArea = _imp->activeStrokeUpdateArea;
    return true;
}

void
TreeRender::setActiveStrokeUpdateArea(const RectI& area)
{
    QMutexLocker k(&_imp->extraRequestedResultsMutex);
    _imp->activeStrokeUpdateArea = area;
    _imp->activeStrokeUpdateAreaSet = true;
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

RotoDrawableItemPtr
TreeRender::getCurrentlyDrawingItem() const
{
    return _imp->ctorArgs->activeRotoDrawableItem;
}

bool
TreeRender::isRenderAborted() const
{
    if ((int)_imp->aborted > 0) {
        return true;
    }
    return false;
}

void
TreeRender::setRenderAborted()
{
    _imp->aborted.fetchAndAddAcquire(1);
}

bool
TreeRender::isPlayback() const
{
    return _imp->ctorArgs->playback;
}


bool
TreeRender::isDraftRender() const
{
    return _imp->ctorArgs->draftMode;
}

bool
TreeRender::isByPassCacheEnabled() const
{
    return _imp->ctorArgs->byPassCache;
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
    return _imp->ctorArgs->time;
}

ViewIdx
TreeRender::getView() const
{
    return _imp->ctorArgs->view;
}

const RenderScale&
TreeRender::getProxyScale() const
{
    return _imp->ctorArgs->proxyScale;
}

EffectInstancePtr
TreeRender::getOriginalTreeRoot() const
{
    return _imp->ctorArgs->treeRootEffect;
}

RenderStatsPtr
TreeRender::getStatsObject() const
{
    return _imp->ctorArgs->stats;
}

void
TreeRender::registerRenderClone(const KnobHolderPtr& holder)
{
    QMutexLocker k(&_imp->renderClonesMutex);
    _imp->renderClones.push_back(holder);
}

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



ActionRetCodeEnum
TreeRenderPrivate::getTreeRootRoD(const EffectInstancePtr& effect, TimeValue time, ViewIdx view, const RenderScale& scale, RectD* rod)
{

    GetRegionOfDefinitionResultsPtr results;
    ActionRetCodeEnum stat = effect->getRegionOfDefinition_public(time, scale, view, &results);
    if (isFailureRetCode(stat)) {
        return stat;
    }
    assert(results);
    *rod = results->getRoD();
    return stat;
}

ActionRetCodeEnum
TreeRenderPrivate::getTreeRootPlane(const EffectInstancePtr& effect, TimeValue time, ViewIdx view, ImagePlaneDesc* plane)
{
    GetComponentsResultsPtr results;
    ActionRetCodeEnum stat = effect->getLayersProducedAndNeeded_public(time, view, &results);
    if (isFailureRetCode(stat)) {
        return stat;
    }
    assert(results);
    const std::list<ImagePlaneDesc>& producedLayers = results->getProducedPlanes();
    if (!producedLayers.empty()) {
        *plane = producedLayers.front();
    }
    return stat;
}

void
TreeRenderPrivate::init(const TreeRender::CtorArgsPtr& inArgs)
{
    assert(inArgs->treeRootEffect);

    ctorArgs = inArgs;
    SettingsPtr settings = appPTR->getCurrentSettings();
    handleNaNs = settings && settings->isNaNHandlingEnabled();
    useConcatenations = settings && settings->isTransformConcatenationEnabled();
    
    // Initialize all requested extra nodes to a null result
    for (std::list<NodePtr>::const_iterator it = inArgs->extraNodesToSample.begin(); it != inArgs->extraNodesToSample.end(); ++it) {
        extraRequestedResults.insert(std::make_pair(*it, FrameViewRequestPtr()));
    }

    // Fetch the OpenGL context used for the render. It will not be attached to any render thread yet.
    fetchOpenGLContext(inArgs);


} // init

TreeRenderPtr
TreeRender::create(const CtorArgsPtr& inArgs)
{
    TreeRenderPtr render(new TreeRender());


    if (dynamic_cast<GroupInput*>(inArgs->treeRootEffect.get())) {
        // Hack for the GroupInput node: if the treeRoot passed is a GroupInput node we must forward to the corresponding input of the enclosing
        // group node, otherwise the render will fail because the GroupInput node itself does not have any input.
        NodeGroupPtr enclosingNodeGroup = toNodeGroup(inArgs->treeRootEffect->getNode()->getGroup());
        if (!enclosingNodeGroup) {
            render->_imp->state = eActionStatusFailed;
            return render;
        }

        NodePtr realInput = enclosingNodeGroup->getRealInputForInput(inArgs->treeRootEffect->getNode());
        if (!realInput) {
            render->_imp->state = eActionStatusFailed;
            return render;
        }
        inArgs->treeRootEffect = realInput->getEffectInstance();
    }


    assert(!inArgs->treeRootEffect->isRenderClone());
    
    try {
        // Setup the render tree and make local copy of knob values for the render.
        // This will also set the per node render object in the TLS for OpenFX effects.
        render->_imp->init(inArgs);
    } catch (...) {
        render->_imp->state = eActionStatusFailed;


    }


    return render;
}


struct RequestPassSharedDataPrivate
{
    // Protects dependencyFreeRenders and allRenderTasks
    mutable QMutex dependencyFreeRendersMutex;

    QWaitCondition dependencyFreeRendersEmptyCond;

    // A set of renders that we can launch right now
    boost::scoped_ptr<DependencyFreeRenderSet> dependencyFreeRenders;

    // All renders left to do
    std::set<FrameViewRequestPtr> allRenderTasksToProcess;

    // The status global to the tasks, protected by dependencyFreeRendersMutex
    ActionRetCodeEnum stat;

    TreeRenderWPtr treeRender;

    RequestPassSharedDataPrivate()
    : dependencyFreeRendersMutex()
    , dependencyFreeRendersEmptyCond()
    , dependencyFreeRenders()
    , allRenderTasksToProcess()
    , stat(eActionStatusOK)
    , treeRender()
    {
        
    }
};

RequestPassSharedData::RequestPassSharedData()
: _imp(new RequestPassSharedDataPrivate())
{

}

RequestPassSharedData::~RequestPassSharedData()
{

}

TreeRenderPtr
RequestPassSharedData::getTreeRender() const
{
    return _imp->treeRender.lock();
}

void
RequestPassSharedData::addTaskToRender(const FrameViewRequestPtr& render)
{
     QMutexLocker k(&_imp->dependencyFreeRendersMutex);
    {
        std::pair<DependencyFreeRenderSet::iterator, bool> ret = _imp->allRenderTasksToProcess.insert(render);
        (void)ret;
#ifdef TRACE_RENDER_DEPENDENCIES
        if (ret.second) {
            qDebug() << this << "Adding" << render->getEffect()->getScriptName_mt_safe().c_str() << render->getPlaneDesc().getPlaneLabel().c_str() << "(" << render.get() << ") to the global list";
        }
#endif
    }
    if (render->getNumDependencies(shared_from_this()) == 0) {
        std::pair<DependencyFreeRenderSet::iterator, bool> ret = _imp->dependencyFreeRenders->insert(render);
        (void)ret;
#ifdef TRACE_RENDER_DEPENDENCIES
        if (ret.second) {
            qDebug() << this << "Adding" << render->getEffect()->getScriptName_mt_safe().c_str() << render->getPlaneDesc().getPlaneLabel().c_str() << "(" << render.get() << ") to the dependency-free list";
        }
#endif

    }
}

class CleanupRenderClones_RAII
{
    TreeRenderPtr render;
public:

    CleanupRenderClones_RAII(const TreeRenderPtr& render)
    : render(render)
    {

    }

    ~CleanupRenderClones_RAII()
    {
        if (render) {
            QMutexLocker k(&render->_imp->renderClonesMutex);
            for (std::list<KnobHolderPtr>::const_iterator it = render->_imp->renderClones.begin(); it != render->_imp->renderClones.end(); ++it) {
                (*it)->getMainInstance()->removeRenderClone(render);
            }
            render->_imp->renderClones.clear();
        }
    }
};


class FrameViewRenderRunnable : public QRunnable
{

    RequestPassSharedDataWPtr _sharedData;
    FrameViewRequestWPtr _request;
    TreeRenderPrivate* _imp;
public:

    FrameViewRenderRunnable(TreeRenderPrivate* imp, const RequestPassSharedDataPtr& sharedData, const FrameViewRequestPtr& request)
    : QRunnable()
    , _sharedData(sharedData)
    , _request(request)
    , _imp(imp)
    {
        assert(request);
    }

    virtual ~FrameViewRenderRunnable()
    {
    }

    virtual void run() OVERRIDE FINAL
    {

        RequestPassSharedDataPtr sharedData = _sharedData.lock();

        ActionRetCodeEnum stat;
        {
            QMutexLocker k(&sharedData->_imp->dependencyFreeRendersMutex);
            stat = sharedData->_imp->stat;
        }

        FrameViewRequestPtr request = _request.lock();
        EffectInstancePtr renderClone = request->getEffect();

        if (!isFailureRetCode(stat)) {
#ifdef TRACE_RENDER_DEPENDENCIES
            qDebug() << sharedData.get() << "Launching render of" << renderClone->getScriptName_mt_safe().c_str() << request->getPlaneDesc().getPlaneLabel().c_str();
#endif
            stat = renderClone->launchRender(sharedData, request);
        }

        // Remove all stashed input frame view requests that we kept around.
        request->clearRenderedDependencies(sharedData);

        QMutexLocker k(&sharedData->_imp->dependencyFreeRendersMutex);

        if (isFailureRetCode(stat)) {
            sharedData->_imp->stat = stat;
        }

        // Remove this render from all tasks left
        {
            std::set<FrameViewRequestPtr>::const_iterator foundTask = sharedData->_imp->allRenderTasksToProcess.find(request);
            assert(foundTask != sharedData->_imp->allRenderTasksToProcess.end());
            if (foundTask != sharedData->_imp->allRenderTasksToProcess.end()) {
                sharedData->_imp->allRenderTasksToProcess.erase(foundTask);
            }
        }

        // For each frame/view that depend on this frame, remove it from the dependencies list.
        std::list<FrameViewRequestPtr> listeners = request->getListeners(sharedData);
        for (std::list<FrameViewRequestPtr>::const_iterator it = listeners.begin(); it != listeners.end(); ++it) {
            int numDepsLeft = (*it)->markDependencyAsRendered(sharedData, request);

            // If the task has all its dependencies available, add it to the render queue.
            if (numDepsLeft == 0) {
                if (sharedData->_imp->dependencyFreeRenders->find(*it) == sharedData->_imp->dependencyFreeRenders->end()) {
#ifdef TRACE_RENDER_DEPENDENCIES
                    qDebug() << sharedData.get() << "Adding" << (*it)->getEffect()->getScriptName_mt_safe().c_str() << (*it)->getPlaneDesc().getPlaneLabel().c_str()  << "(" << it->get() << ") to the dependency-free list";
#endif
                    assert(sharedData->_imp->allRenderTasksToProcess.find(*it) != sharedData->_imp->allRenderTasksToProcess.end());
                    sharedData->_imp->dependencyFreeRenders->insert(*it);
                }
            }
        }


        // If the results for this node were requested by the caller, insert them
        {
            QMutexLocker k(&_imp->extraRequestedResultsMutex);
            std::map<NodePtr, FrameViewRequestPtr>::iterator foundRequested = _imp->extraRequestedResults.find(renderClone->getNode());
            if (foundRequested != _imp->extraRequestedResults.end() && !foundRequested->second) {
                foundRequested->second = request;
            }
        }

        // Notify the main render thread that some more stuff can be rendered (or that we are done)
        sharedData->_imp->dependencyFreeRendersEmptyCond.wakeOne();
    }
};


ActionRetCodeEnum
TreeRenderPrivate::launchRenderInternal(bool removeRenderClonesWhenFinished,
                                        const EffectInstancePtr& treeRoot,
                                        TimeValue time,
                                        ViewIdx view,
                                        const RenderScale& proxyScale,
                                        unsigned int mipMapLevel,
                                        const ImagePlaneDesc* planeParam,
                                        const RectD* canonicalRoIParam,
                                        FrameViewRequestPtr* outputRequest)
{

    // Get the combined scale
    RenderScale scale = ctorArgs->proxyScale;
    {
        double mipMapScale = Image::getScaleFromMipMapLevel(mipMapLevel);
        scale.x *= mipMapScale;
        scale.y *= mipMapScale;
    }

    EffectInstancePtr rootRenderClone;
    {
        FrameViewRenderKey key = {time, view, _publicInterface->shared_from_this()};
        rootRenderClone = toEffectInstance(treeRoot->createRenderClone(key));
    }

    // If we are within a EffectInstance::getImagePlane() call, the treeRoot may already be a render clone spawned by this tree, in which case
    // we don't want to remove clones yet. Clean the clones when the render tree is done executing the last call to launchRenderInternal.
    boost::scoped_ptr<CleanupRenderClones_RAII> clonesCleaner;
    if (removeRenderClonesWhenFinished) {
        clonesCleaner.reset(new CleanupRenderClones_RAII(_publicInterface->shared_from_this()));
    }

    assert(rootRenderClone->isRenderClone());

    // Resolve plane to render if not provided
    ImagePlaneDesc plane;
    if (planeParam) {
        plane = *planeParam;
    } else {
        ActionRetCodeEnum stat = TreeRenderPrivate::getTreeRootPlane(rootRenderClone, time, view, &plane);
        if (isFailureRetCode(stat)) {
            return stat;
        }
    }

    // Resolve RoI to render if not provided
    RectD canonicalRoI;
    if (canonicalRoIParam) {
        canonicalRoI = *canonicalRoIParam;
    } else {
        ActionRetCodeEnum stat = TreeRenderPrivate::getTreeRootRoD(rootRenderClone, time, view, scale, &canonicalRoI);
        if (isFailureRetCode(stat)) {
            return stat;
        }
    }
    ActionRetCodeEnum stat = eActionStatusOK;
    {
        RequestPassSharedDataPtr requestData(new RequestPassSharedData());
        requestData->_imp->dependencyFreeRenders.reset(new DependencyFreeRenderSet(FrameViewRequestComparePriority(requestData)));

        requestData->_imp->treeRender = _publicInterface->shared_from_this();


#ifdef TRACE_RENDER_DEPENDENCIES
        qDebug() << "Starting launchRenderInternal" << requestData.get();
#endif

        // Cycle through the tree to find and requested frames and RoIs
        {
            stat = treeRoot->requestRender(time, view, proxyScale, mipMapLevel, plane, canonicalRoI, -1, FrameViewRequestPtr(), requestData, outputRequest, 0);

            if (isFailureRetCode(stat)) {
                return stat;
            }
        }

        // At this point, the request pass should have created the first batch of dependency-free renders.
        // The list cannot be empty, otherwise it should have failed before.
        assert(!requestData->_imp->dependencyFreeRenders->empty());
        if (requestData->_imp->dependencyFreeRenders->empty()) {
            return eActionStatusFailed;
        }

        int numTasksRemaining;
        {
            QMutexLocker k(&requestData->_imp->dependencyFreeRendersMutex);
            numTasksRemaining = requestData->_imp->allRenderTasksToProcess.size();
        }

#ifndef TREE_RENDER_DISABLE_MT
        QThreadPool* threadPool = QThreadPool::globalInstance();
        bool isThreadPoolThread = isRunningInThreadPoolThread();
#endif

        // See bug https://bugreports.qt.io/browse/QTBUG-20251
        // The Qt thread-pool mem-leaks the runnable if using release/reserveThread
        // Instead we explicitly manage them and ensure they do not hold any external strong refs.
        std::vector<boost::shared_ptr<FrameViewRenderRunnable> > runnables;

        // While we still have tasks to render loop
        while (numTasksRemaining > 0) {


            // Launch all dependency-free tasks in parallel
            QMutexLocker k(&requestData->_imp->dependencyFreeRendersMutex);
            while (requestData->_imp->dependencyFreeRenders->size() > 0) {

                FrameViewRequestPtr request = *requestData->_imp->dependencyFreeRenders->begin();
                requestData->_imp->dependencyFreeRenders->erase(requestData->_imp->dependencyFreeRenders->begin());
#ifdef TRACE_RENDER_DEPENDENCIES
                qDebug() << "Queuing " << request->getEffect()->getScriptName_mt_safe().c_str() << " in task pool";
#endif
                boost::shared_ptr<FrameViewRenderRunnable> runnable(new FrameViewRenderRunnable(this, requestData, request));
#ifdef TREE_RENDER_DISABLE_MT
                k.unlock();
                runnable->run();
                k.relock();
#else
                runnable->setAutoDelete(false);
                runnables.push_back(runnable);
                threadPool->start(runnable.get());
#endif
            }

#ifndef TREE_RENDER_DISABLE_MT
            // If this thread is a threadpool thread, it may wait for a while that results gets available.
            // Release the thread to the thread pool so that it may use this thread for other runnables
            // and reserve it back when done waiting.
            if (isThreadPoolThread) {
                QThreadPool::globalInstance()->releaseThread();
            }

            // Wait until a task is finished: we should be able to launch more tasks afterwards.
            requestData->_imp->dependencyFreeRendersEmptyCond.wait(&requestData->_imp->dependencyFreeRendersMutex);
            
            if (isThreadPoolThread) {
                QThreadPool::globalInstance()->reserveThread();
            }
#endif   
            numTasksRemaining = requestData->_imp->allRenderTasksToProcess.size();
        }
        runnables.clear();

        stat = requestData->_imp->stat;
    } // requestData


    if (removeRenderClonesWhenFinished) {
        // Now if the image to render was cached, we may not have retrieved the requested color picker images, in which case we have to render them
        for (std::map<NodePtr, FrameViewRequestPtr>::iterator it = extraRequestedResults.begin(); it != extraRequestedResults.end(); ++it) {
            if (!it->second) {
                FrameViewRequestPtr colorPickerRequest;
                ActionRetCodeEnum tmpStat =  launchRenderInternal(false /*removeRenderClonesWhenFinished*/, it->first->getEffectInstance(), ctorArgs->time, ctorArgs->view, ctorArgs->proxyScale, ctorArgs->mipMapLevel, &plane, ctorArgs->canonicalRoI, &colorPickerRequest);
                if (isFailureRetCode(tmpStat)) {
                    return tmpStat;
                }
                it->second = colorPickerRequest;
            }
        }
    }

    return stat;
} // launchRenderInternal

ActionRetCodeEnum
TreeRender::launchRenderWithArgs(const EffectInstancePtr& root,
                                 TimeValue time,
                                 ViewIdx view,
                                 const RenderScale& proxyScale,
                                 unsigned int mipMapLevel,
                                 const ImagePlaneDesc* plane,
                                 const RectD* canonicalRoI,
                                 FrameViewRequestPtr* outputRequest)
{
    ActionRetCodeEnum stat =  _imp->launchRenderInternal(false /*removeRenderClonesWhenFinished*/, root, time, view, proxyScale, mipMapLevel, plane, canonicalRoI, outputRequest);
    return stat;
}

ActionRetCodeEnum
TreeRender::launchRender(FrameViewRequestPtr* outputRequest)
{
    
#ifdef TRACE_RENDER_DEPENDENCIES
    qDebug() << "Starting Tree Render";
#endif
    // init() may have failed, return early then.
    if (isFailureRetCode(_imp->state)) {
        return _imp->state;
    }
    _imp->state = _imp->launchRenderInternal(true /*removeRenderClonesWhenFinished*/, _imp->ctorArgs->treeRootEffect, _imp->ctorArgs->time, _imp->ctorArgs->view, _imp->ctorArgs->proxyScale, _imp->ctorArgs->mipMapLevel, _imp->ctorArgs->plane, _imp->ctorArgs->canonicalRoI, outputRequest);

    return _imp->state;
} // launchRender

NATRON_NAMESPACE_EXIT

