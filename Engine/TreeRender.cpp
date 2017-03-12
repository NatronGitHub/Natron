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

struct FrameViewRequestSharedDataPair
{
    FrameViewRequestPtr request;
    RequestPassSharedDataPtr launchData;
};

// Render first the tasks with more dependencies: it has more chance to make more dependency-free new renders
// to enable better concurrency
struct FrameViewRequestComparePriority
{
    RequestPassSharedData* _launchData;

    FrameViewRequestComparePriority(RequestPassSharedData* launchData)
    : _launchData(launchData)
    {

    }

    bool operator() (const FrameViewRequestPtr& lhs, const FrameViewRequestPtr& rhs) const
    {
        RequestPassSharedDataPtr sharedData = _launchData->shared_from_this();

        int lNum = lhs->getNumListeners(sharedData);
        int rNum = rhs->getNumListeners(sharedData);
        if (lNum < rNum) {
            return true;
        } else if (lNum > rNum) {
            return false;
        } else {
            // Same number of listeners...there's no specific ordering
            return lhs.get() < rhs.get();
        }
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
    
    // Render args of the root node
    EffectInstancePtr rootRenderClone;

    // Map of nodes that belong to the tree upstream of tree root for which we desire
    // a pointer of the resulting image. This is useful for the Viewer to enable color-picking:
    // the output image is the image out of the ViewerProcess node, but what the user really
    // wants is the color-picker of the image in input of the Viewer (group) node.
    // These images can then be retrieved using the getExtraRequestedResultsForNode() function.
    std::map<NodePtr, FrameViewRequestPtr> extraRequestedResults;
    mutable QMutex extraRequestedResultsMutex;

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
    , rootRenderClone()
    , extraRequestedResults()
    , extraRequestedResultsMutex()
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

    ActionRetCodeEnum launchRenderInternal(const EffectInstancePtr& treeRoot,
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


EffectInstancePtr
TreeRender::getTreeRootRenderClone() const
{
    return _imp->rootRenderClone;
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
    if ((int)_imp->aborted > 0) {
        return true;
    }

    // If spawned from another render, check if it was aborted
    TreeRenderPtr originalRender = _imp->ctorArgs->originalRender.lock();
    if (!originalRender) {
        return false;
    }
    return originalRender->isRenderAborted();
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

RenderStatsPtr
TreeRender::getStatsObject() const
{
    return _imp->ctorArgs->stats;
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
    handleNaNs = appPTR->getCurrentSettings()->isNaNHandlingEnabled();
    useConcatenations = appPTR->getCurrentSettings()->isTransformConcatenationEnabled();
    
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
    
    try {
        // Setup the render tree and make local copy of knob values for the render.
        // This will also set the per node render object in the TLS for OpenFX effects.
        render->_imp->init(inArgs);
    } catch (...) {
        render->_imp->state = eActionStatusFailed;


    }

     if (isFailureRetCode(render->_imp->state)) {
         if (render->_imp->rootRenderClone) {
             render->_imp->rootRenderClone->removeRenderCloneRecursive(render);
         }
     }
    
    return render;
}


struct RequestPassSharedDataPrivate
{
    // Protects dependencyFreeRenders and allRenderTasks
    mutable QMutex dependencyFreeRendersMutex;

    QWaitCondition dependencyFreeRendersEmptyCond;

    // A set of renders that we can launch right now
    DependencyFreeRenderSet dependencyFreeRenders;

    // All renders left to do
    std::set<FrameViewRequestPtr> allRenderTasksToProcess;

    // The status global to the tasks, protected by dependencyFreeRendersMutex
    ActionRetCodeEnum stat;

    TreeRenderWPtr treeRender;

    RequestPassSharedDataPrivate(RequestPassSharedData* publicInterface)
    : dependencyFreeRendersMutex()
    , dependencyFreeRendersEmptyCond()
    , dependencyFreeRenders(FrameViewRequestComparePriority(publicInterface))
    , allRenderTasksToProcess()
    , stat(eActionStatusOK)
    , treeRender()
    {
        
    }
};

RequestPassSharedData::RequestPassSharedData()
: _imp(new RequestPassSharedDataPrivate(this))
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
    _imp->allRenderTasksToProcess.insert(render);
    if (render->getNumDependencies(shared_from_this()) == 0) {
        _imp->dependencyFreeRenders.insert(render);
    }
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
        if (_imp->rootRenderClone) {
            _imp->rootRenderClone->removeRenderCloneRecursive(_imp->_publicInterface->shared_from_this());
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

private:

    virtual void run() OVERRIDE FINAL
    {
        FrameViewRequestPtr request = _request.lock();
        RequestPassSharedDataPtr sharedData = _sharedData.lock();

        EffectInstancePtr renderClone = request->getRenderClone();
        ActionRetCodeEnum stat = renderClone->launchRender(sharedData, request);


        // Remove all stashes input frame view requests that we kept around.
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
                sharedData->_imp->dependencyFreeRenders.insert(*it);
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
TreeRenderPrivate::launchRenderInternal(const EffectInstancePtr& treeRoot,
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

    rootRenderClone = toEffectInstance(treeRoot->createRenderClone(_publicInterface->shared_from_this()));

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

    RequestPassSharedDataPtr requestData(new RequestPassSharedData());
    requestData->_imp->treeRender = _publicInterface->shared_from_this();

    // Cycle through the tree to find and requested frames and RoIs
    {
        ActionRetCodeEnum stat = treeRoot->requestRender(time, view, proxyScale, mipMapLevel, plane, canonicalRoI, -1, FrameViewRequestPtr(), requestData, outputRequest, 0);

        if (isFailureRetCode(stat)) {
            return stat;
        }
    }

    // At this point, the request pass should have created the first batch of dependency-free renders.
    // The list cannot be empty, otherwise it should have failed before.
    assert(!requestData->_imp->dependencyFreeRenders.empty());
    if (requestData->_imp->dependencyFreeRenders.empty()) {
        return eActionStatusFailed;
    }

    int numTasksRemaining;
    {
        QMutexLocker k(&requestData->_imp->dependencyFreeRendersMutex);
        numTasksRemaining = requestData->_imp->allRenderTasksToProcess.size();
    }
    QThreadPool* threadPool = QThreadPool::globalInstance();

    bool isThreadPoolThread = isRunningInThreadPoolThread();

    // See bug https://bugreports.qt.io/browse/QTBUG-20251
    // The Qt thread-pool mem-leaks the runnable if using release/reserveThread
    // Instead we explicitly manage them and ensure they do not hold any external strong refs.
    std::vector<boost::shared_ptr<FrameViewRenderRunnable> > runnables;

    // While we still have tasks to render loop
    while (numTasksRemaining > 0) {


        // Launch all dependency-free tasks in parallel
        QMutexLocker k(&requestData->_imp->dependencyFreeRendersMutex);
        while (requestData->_imp->dependencyFreeRenders.size() > 0) {

            FrameViewRequestPtr request = *requestData->_imp->dependencyFreeRenders.begin();
            requestData->_imp->dependencyFreeRenders.erase(requestData->_imp->dependencyFreeRenders.begin());
            boost::shared_ptr<FrameViewRenderRunnable> runnable(new FrameViewRenderRunnable(this, requestData, request));
            runnable->setAutoDelete(false);
            runnables.push_back(runnable);
            threadPool->start(runnable.get());
        }

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

        // We have been woken-up by a finished task, check if the render is still OK
        if (isFailureRetCode(requestData->_imp->stat)) {
            return requestData->_imp->stat;
        }

        
        numTasksRemaining = requestData->_imp->allRenderTasksToProcess.size();
    }
    runnables.clear();
    
    return eActionStatusOK;
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
    ActionRetCodeEnum stat =  _imp->launchRenderInternal(root, time, view, proxyScale, mipMapLevel, plane, canonicalRoI, outputRequest);
    return stat;
}

ActionRetCodeEnum
TreeRender::launchRender(FrameViewRequestPtr* outputRequest)
{
    
    TLSCleanupRAII tlsCleaner(_imp.get());

    // init() may have failed, return early then.
    if (isFailureRetCode(_imp->state)) {
        return _imp->state;
    }
    _imp->state = _imp->launchRenderInternal(_imp->ctorArgs->treeRootEffect, _imp->ctorArgs->time, _imp->ctorArgs->view, _imp->ctorArgs->proxyScale, _imp->ctorArgs->mipMapLevel, _imp->ctorArgs->plane, _imp->ctorArgs->canonicalRoI, outputRequest);
    return _imp->state;
} // launchRender

NATRON_NAMESPACE_EXIT;

