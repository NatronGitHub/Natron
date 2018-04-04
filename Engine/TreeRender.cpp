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

#ifdef DEBUG
#include "Global/FloatingPointExceptions.h"
#endif
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
#include "Engine/TreeRenderQueueManager.h"
#include "Engine/TLSHolder.h"

//#define TRACE_RENDER_DEPENDENCIES

NATRON_NAMESPACE_ENTER

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

    TreeRender::CtorArgsPtr ctorArgs;

    // Protects state
    mutable QMutex stateMutex;
    
    // The state of the object to avoid calling render on a failed tree
    ActionRetCodeEnum state;

    // All cloned knob holders for this render
    mutable QMutex renderClonesMutex;
    std::list<KnobHolderPtr> renderClones;

    // The request output results
    FrameViewRequestPtr outputRequest;

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
    , outputRequest()
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

    /**
     * @brief Creates a TreeRenderExecutionData object to be used by the TreeRenderQueueManager
     **/
    TreeRenderExecutionDataPtr createExecutionDataInternal(bool isMainExecution,
                                                           const EffectInstancePtr& treeRoot,
                                                           TimeValue time,
                                                           ViewIdx view,
                                                           const RenderScale& proxyScale,
                                                           unsigned int mipMapLevel,
                                                           const ImagePlaneDesc* planeParam,
                                                           const RectD* canonicalRoIParam,
                                                           EffectInstance::AcceptedRequestConcatenationFlags concatenationFlags,
                                                           bool createTreeRenderIfUnrenderedImage);


};

TreeRender::CtorArgs::CtorArgs()
: time(0)
, view(0)
, treeRootEffect()
, extraNodesToSample()
, activeRotoDrawableItem()
, stats()
, canonicalRoI()
, plane()
, proxyScale(1.)
, mipMapLevel(0)
, draftMode(false)
, playback(false)
, byPassCache(false)
, preventConcurrentTreeRenders(false)
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
TreeRender::setOrUnionActiveStrokeUpdateArea(const RectI& area)
{
    QMutexLocker k(&_imp->extraRequestedResultsMutex);
    if (!_imp->activeStrokeUpdateAreaSet) {
        _imp->activeStrokeUpdateArea = area;
        _imp->activeStrokeUpdateAreaSet = true;
    } else {
        _imp->activeStrokeUpdateArea.merge(area);
    }
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

TreeRenderQueueProviderConstPtr
TreeRender::getProvider() const
{
    return _imp->ctorArgs->provider;
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

RectD
TreeRender::getCtorRoI() const
{
    return _imp->ctorArgs->canonicalRoI;
}

bool
TreeRender::isConcurrentRendersAllowed() const
{
    return !_imp->ctorArgs->preventConcurrentTreeRenders;
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
    // If we don't have requirements, don't fetch a context
    const bool hasGPUOpenGLRequirements = appPTR->hasOpenGLForRequirements(eOpenGLRequirementsTypeRendering);

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
                    if (hasGPUOpenGLRequirements) {
                        glContext = appPTR->getGPUContextPool()->getOrCreateOpenGLContext(true/*retrieveLastContext*/);
                    }
                    cpuContext = appPTR->getGPUContextPool()->getOrCreateCPUOpenGLContext(true/*retrieveLastContext*/);
                    isStroke->setDrawingGLContext(glContext, cpuContext);
                } catch (const std::exception& /*e*/) {

                }
            }
        }
    } else {
        try {
            if (hasGPUOpenGLRequirements) {
                glContext = appPTR->getGPUContextPool()->getOrCreateOpenGLContext(false/*retrieveLastContext*/);
            }
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

void
TreeRender::setResults(const FrameViewRequestPtr& request, const TreeRenderExecutionDataPtr& execData)
{
    // Only update the status of the tree for the main execution
    if (execData->isTreeMainExecution()) {
        ActionRetCodeEnum status = execData->getStatus();
        if (isFailureRetCode(status)) {
            QMutexLocker k(&_imp->stateMutex);
            _imp->state = status;
        }
    }
    if (request) {
        QMutexLocker k(&_imp->extraRequestedResultsMutex);

        EffectInstancePtr effect = request->getEffect();
        if (!effect) {
            assert(false);
            return;
        }
        if (effect && effect->getNode() == _imp->ctorArgs->treeRootEffect->getNode()) {
            _imp->outputRequest = request;
        } else {
            std::map<NodePtr, FrameViewRequestPtr>::iterator foundRequested = _imp->extraRequestedResults.find(effect->getNode());
            if (foundRequested != _imp->extraRequestedResults.end() && !foundRequested->second) {
                foundRequested->second = request;
            }
        }
    }


} // addExtraResultsForNode

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

void
TreeRender::cleanupRenderClones()
{
    TreeRenderPtr thisShared = shared_from_this();
    QMutexLocker k(&_imp->renderClonesMutex);
    for (std::list<KnobHolderPtr>::const_iterator it = _imp->renderClones.begin(); it != _imp->renderClones.end(); ++it) {
        (*it)->getMainInstance()->removeRenderClone(thisShared);
    }
    _imp->renderClones.clear();
}


struct TreeRenderExecutionDataPrivate
{

    TreeRenderExecutionData* _publicInterface;

    // a TreeRender may have multiple executions: One is the main execution that returns the image
    // of the requested arguments passed to the ctor, but sub-executions may be created for example
    // in getImagePlane or to retrieve extraneous images for the color-picker
    bool isMainExecutionOfTree;

    // Protects dependencyFreeRenders and allRenderTasks
    mutable QMutex dependencyFreeRendersMutex;

    // A set of renders that we can launch right now
    boost::scoped_ptr<DependencyFreeRenderSet> dependencyFreeRenders;

    // The status global to the tasks, protected by dependencyFreeRendersMutex
    ActionRetCodeEnum status;

    // Pointer to the tree render that produced this TreeRenderExecutionData
    TreeRenderWPtr treeRender;

    // The canonical region of interest on the tree root
    RectD canonicalRoI;

    // The plane requested on the tree root
    ImagePlaneDesc plane;

    // The request created in output
    FrameViewRequestPtr outputRequest;

    // See bug https://bugreports.qt.io/browse/QTBUG-20251
    // The Qt thread-pool mem-leaks the runnable if using release/reserveThread
    // Instead we explicitly manage them and ensure they do not hold any external strong refs.
    std::set<FrameViewRenderRunnablePtr> launchedRunnables;

    // If true, anything that request new images to be rendered from within the execution should create a new TreeRender
    // instead. See discussion in @FrameViewRequest: this is to overcome thread-safety for host frame-threading effects.
    bool createTreeRenderIfUnrenderedImage;

    TreeRenderExecutionDataPrivate(TreeRenderExecutionData* publicInterface, bool createTreeRenderIfUnrenderedImage)
    : _publicInterface(publicInterface)
    , isMainExecutionOfTree(false)
    , dependencyFreeRendersMutex()
    , dependencyFreeRenders()
    , status(eActionStatusOK)
    , treeRender()
    , canonicalRoI()
    , plane()
    , outputRequest()
    , launchedRunnables()
    , createTreeRenderIfUnrenderedImage(createTreeRenderIfUnrenderedImage)
    {
        
    }

    void onTaskFinished(const FrameViewRequestPtr& request, ActionRetCodeEnum stat);

    void removeDependencyLinkFromRequest(const FrameViewRequestPtr& request);


};

TreeRenderExecutionData::TreeRenderExecutionData(bool createTreeRenderIfUnrenderedImage)
: _imp(new TreeRenderExecutionDataPrivate(this, createTreeRenderIfUnrenderedImage))
{

}

TreeRenderExecutionData::~TreeRenderExecutionData()
{
}

bool
TreeRenderExecutionData::isTreeMainExecution() const
{
    return _imp->isMainExecutionOfTree;
}

TreeRenderPtr
TreeRenderExecutionData::getTreeRender() const
{
    return _imp->treeRender.lock();
}

ActionRetCodeEnum
TreeRenderExecutionData::getStatus() const
{
    QMutexLocker k(&_imp->dependencyFreeRendersMutex);
    return _imp->status;
}

FrameViewRequestPtr
TreeRenderExecutionData::getOutputRequest() const
{
    return _imp->outputRequest;
}

bool
TreeRenderExecutionData::isNewTreeRenderUponUnrenderedImageEnabled() const
{
    return _imp->createTreeRenderIfUnrenderedImage;
}

void
TreeRenderExecutionData::addTaskToRender(const FrameViewRequestPtr& render)
{
    QMutexLocker k(&_imp->dependencyFreeRendersMutex);

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

void
TreeRenderExecutionDataPrivate::removeDependencyLinkFromRequest(const FrameViewRequestPtr& request)
{
    assert(!dependencyFreeRendersMutex.tryLock());

    TreeRenderExecutionDataPtr thisShared = _publicInterface->shared_from_this();

    std::list<FrameViewRequestPtr> listeners = request->getListeners(thisShared);
    for (std::list<FrameViewRequestPtr>::const_iterator it = listeners.begin(); it != listeners.end(); ++it) {
        int numDepsLeft = (*it)->markDependencyAsRendered(thisShared, request);

        // If the task has all its dependencies available, add it to the render queue.
        if (numDepsLeft == 0) {
            if (dependencyFreeRenders->find(*it) == dependencyFreeRenders->end()) {
#ifdef TRACE_RENDER_DEPENDENCIES
                qDebug() << thisShared.get() << "Adding" << (*it)->getEffect()->getScriptName_mt_safe().c_str() << (*it)->getPlaneDesc().getPlaneLabel().c_str()  << "(" << it->get() << ") to the dependency-free list";
#endif
                dependencyFreeRenders->insert(*it);
            }
        }
    }

}

void
TreeRenderExecutionDataPrivate::onTaskFinished(const FrameViewRequestPtr& request, ActionRetCodeEnum requestStatus)
{


    TreeRenderExecutionDataPtr sharedData = _publicInterface->shared_from_this();

    // Remove all stashed input frame view requests that we kept around.
    request->clearRenderedDependencies(sharedData);

    TreeRenderPtr render = treeRender.lock();


    {
        QMutexLocker k(&dependencyFreeRendersMutex);

        if (isFailureRetCode(requestStatus)) {
            status = requestStatus;
        }

        // For each frame/view that depend on this frame, remove it from the dependencies list.
        removeDependencyLinkFromRequest(request);

    }
    
    // If the results for this node were requested by the caller, insert them
    assert(render);
    if (render) {
        render->setResults(request, sharedData);
    }

    bool wasLastTaskRemaining = (request == sharedData->getOutputRequest());
    appPTR->getTasksQueueManager()->notifyTaskInRenderFinished(sharedData, wasLastTaskRemaining, isRunningInThreadPoolThread());


} // onTaskFinished

struct FrameViewRenderRunnable::Implementation
{


    TreeRenderExecutionDataWPtr sharedData;
    FrameViewRequestPtr request;
    //TreeRenderPrivate* _imp;

    Implementation(const TreeRenderExecutionDataPtr& sharedData, const FrameViewRequestPtr& request)
    : sharedData(sharedData)
    , request(request)
    {

    }
};


FrameViewRenderRunnable::FrameViewRenderRunnable(const TreeRenderExecutionDataPtr& sharedData, const FrameViewRequestPtr& request)
: QRunnable()
, _imp(new FrameViewRenderRunnable::Implementation(sharedData, request))
{
    assert(request);
}


// make_shared enabler (because make_shared needs access to the private constructor)
// see https://stackoverflow.com/a/20961251/2607517
struct FrameViewRenderRunnable::MakeSharedEnabler: public FrameViewRenderRunnable
{
    MakeSharedEnabler(const TreeRenderExecutionDataPtr& sharedData,
                      const FrameViewRequestPtr& request) : FrameViewRenderRunnable(sharedData, request) {
    }
};


FrameViewRenderRunnablePtr
FrameViewRenderRunnable::create(const TreeRenderExecutionDataPtr& sharedData, const FrameViewRequestPtr& request)
{
    return boost::make_shared<FrameViewRenderRunnable::MakeSharedEnabler>(sharedData, request);
}


FrameViewRenderRunnable::~FrameViewRenderRunnable()
{
}

void
FrameViewRenderRunnable::run()
{
#ifdef DEBUG
    boost_adaptbx::floating_point::exception_trapping trap(boost_adaptbx::floating_point::exception_trapping::division_by_zero |
                                                           boost_adaptbx::floating_point::exception_trapping::invalid |
                                                           boost_adaptbx::floating_point::exception_trapping::overflow);
#endif
    TreeRenderExecutionDataPtr sharedData = _imp->sharedData.lock();

    // Check the status of the execution tasks because another concurrent render might have failed
    ActionRetCodeEnum stat = sharedData->getStatus();

    if (!isFailureRetCode(stat)) {
#ifdef TRACE_RENDER_DEPENDENCIES
        qDebug() << sharedData.get() << "Launching render of" << renderClone->getScriptName_mt_safe().c_str() << request->getPlaneDesc().getPlaneLabel().c_str();
#endif
        EffectInstancePtr renderClone = _imp->request->getEffect();
        stat = renderClone->launchNodeRender(sharedData, _imp->request);
    }

    sharedData->_imp->onTaskFinished(_imp->request, stat);


} // run

TreeRenderExecutionDataPtr
TreeRenderPrivate::createExecutionDataInternal(bool isMainExecution,
                                               const EffectInstancePtr& treeRoot,
                                               TimeValue time,
                                               ViewIdx view,
                                               const RenderScale& proxyScale,
                                               unsigned int mipMapLevel,
                                               const ImagePlaneDesc* planeParam,
                                               const RectD* canonicalRoIParam,
                                               EffectInstance::AcceptedRequestConcatenationFlags concatenationFlags,
                                               bool createTreeRenderIfUnrenderedImage)
{
    TreeRenderExecutionDataPtr requestData(new TreeRenderExecutionData(createTreeRenderIfUnrenderedImage));

    TreeRenderPtr thisShared = _publicInterface->shared_from_this();
    requestData->_imp->treeRender = thisShared;
    requestData->_imp->isMainExecutionOfTree = isMainExecution;
    
    if (isFailureRetCode(state)) {
        requestData->_imp->status = state;
        return requestData;
    }

    EffectInstancePtr rootRenderClone;
    {
        FrameViewRenderKey key = {time, view, thisShared};
        rootRenderClone = toEffectInstance(treeRoot->createRenderClone(key));
        assert(rootRenderClone->isRenderClone());
    }



    // Resolve plane to render if not provided
    if (planeParam) {
        requestData->_imp->plane = *planeParam;
    } else {
        requestData->_imp->status = TreeRenderPrivate::getTreeRootPlane(rootRenderClone, time, view, &requestData->_imp->plane);
        if (isFailureRetCode(requestData->_imp->status)) {
            return requestData;
        }
    }

    // Resolve RoI to render if not provided
    if (canonicalRoIParam) {
        requestData->_imp->canonicalRoI = *canonicalRoIParam;
    } else {
        // Get the combined scale
        RenderScale combinedScale = EffectInstance::getCombinedScale(mipMapLevel, proxyScale);
        requestData->_imp->status = TreeRenderPrivate::getTreeRootRoD(rootRenderClone, time, view, combinedScale, &requestData->_imp->canonicalRoI);
        if (isFailureRetCode(requestData->_imp->status)) {
            return requestData;
        }
    }

    requestData->_imp->dependencyFreeRenders.reset(new DependencyFreeRenderSet);


    // Execute the request pass on the tree. This is a recursive pass that builds the topological sort of FrameViewRequest to render with their dependencies.
    requestData->_imp->status = treeRoot->requestRender(time, view, proxyScale, mipMapLevel, requestData->_imp->plane, requestData->_imp->canonicalRoI, -1, concatenationFlags, FrameViewRequestPtr(), requestData, &requestData->_imp->outputRequest, 0 /*createdRenderClone*/);

    if (isFailureRetCode(requestData->_imp->status)) {
        return requestData;
    }

    // At this point, the request pass should have created the first batch of dependency-free renders.
    // The list cannot be empty, otherwise it should have failed before.
    assert(!requestData->_imp->dependencyFreeRenders->empty());
    if (requestData->_imp->dependencyFreeRenders->empty()) {
        requestData->_imp->status = eActionStatusFailed;
    }
    return requestData;

} // createExecutionDataInternal

TreeRenderExecutionDataPtr
TreeRender::createSubExecutionData(const EffectInstancePtr& treeRoot,
                                   TimeValue time,
                                   ViewIdx view,
                                   const RenderScale& proxyScale,
                                   unsigned int mipMapLevel,
                                   const ImagePlaneDesc* planeParam,
                                   const RectD* canonicalRoIParam,
                                   int concatenationFlags,
                                   bool createTreeRenderIfUnrenderedImage)
{


    return _imp->createExecutionDataInternal(/*removeRenderClonesWhenFinished*/ false, treeRoot, time, view, proxyScale, mipMapLevel, planeParam, canonicalRoIParam, (EffectInstance::AcceptedRequestConcatenationFlags)concatenationFlags, createTreeRenderIfUnrenderedImage);
}

TreeRenderExecutionDataPtr
TreeRender::createMainExecutionData()
{
    EffectInstance::AcceptedRequestConcatenationFlags concatenationFlags = EffectInstance::eAcceptedRequestConcatenationNone;

    return _imp->createExecutionDataInternal(true /*removeRenderClonesWhenFinished*/, _imp->ctorArgs->treeRootEffect, _imp->ctorArgs->time, _imp->ctorArgs->view, _imp->ctorArgs->proxyScale, _imp->ctorArgs->mipMapLevel, _imp->ctorArgs->plane.getNumComponents() == 0 ? 0 : &_imp->ctorArgs->plane, _imp->ctorArgs->canonicalRoI.isNull() ? 0 : &_imp->ctorArgs->canonicalRoI, concatenationFlags, false);
}

std::list<TreeRenderExecutionDataPtr>
TreeRender::getExtraRequestedResultsExecutionData()
{
    EffectInstance::AcceptedRequestConcatenationFlags concatenationFlags = EffectInstance::eAcceptedRequestConcatenationNone;

    std::list<TreeRenderExecutionDataPtr> ret;
    // Now if the image to render was cached, we may not have retrieved the requested color picker images, in which case we have to render them
    for (std::map<NodePtr, FrameViewRequestPtr>::iterator it = _imp->extraRequestedResults.begin(); it != _imp->extraRequestedResults.end(); ++it) {
        if (!it->second) {
            TreeRenderExecutionDataPtr execData =  createSubExecutionData(it->first->getEffectInstance(), _imp->ctorArgs->time, _imp->ctorArgs->view, _imp->ctorArgs->proxyScale, _imp->ctorArgs->mipMapLevel, _imp->ctorArgs->plane.getNumComponents() == 0 ? 0 : &_imp->ctorArgs->plane, _imp->ctorArgs->canonicalRoI.isNull() ? 0 : &_imp->ctorArgs->canonicalRoI, concatenationFlags, false);
            ret.push_back(execData);
        }
    }
    return ret;
}


FrameViewRequestPtr
TreeRender::getOutputRequest() const
{
    return _imp->outputRequest;
}

ActionRetCodeEnum
TreeRender::getStatus() const
{
    QMutexLocker k(&_imp->stateMutex);
    return _imp->state;
}


int
TreeRenderExecutionData::executeAvailableTasks(int nTasksToLaunch)
{

    assert(nTasksToLaunch != 0);

    QThreadPool* threadPool = QThreadPool::globalInstance();

    int nTasksRemaining = nTasksToLaunch;

    TreeRenderExecutionDataPtr thisShared = shared_from_this();

    QMutexLocker k(&_imp->dependencyFreeRendersMutex);

    if (!_imp->dependencyFreeRenders) {
        return 0;
    }

    int nTasksStarted = 0;

    // Launch all dependency-free tasks in parallel
    while ((nTasksRemaining == -1 || nTasksRemaining > 0) && _imp->dependencyFreeRenders->size() > 0) {

        FrameViewRequestPtr request = *_imp->dependencyFreeRenders->begin();
        _imp->dependencyFreeRenders->erase(_imp->dependencyFreeRenders->begin());
#ifdef TRACE_RENDER_DEPENDENCIES
        qDebug() << this <<  "Queuing " << request->getEffect()->getScriptName_mt_safe().c_str() << " in task pool";
#endif
        FrameViewRenderRunnablePtr runnable = FrameViewRenderRunnable::create(thisShared, request);
        FrameViewRequest::FrameViewRequestStatusEnum requestStatus = request->getStatus();
        if ((requestStatus != FrameViewRequest::eFrameViewRequestStatusPassThrough) && !isFailureRetCode(_imp->status)) {
            // Only launch the runnable in a separate thread if its actually going to do any rendering.
            runnable->setAutoDelete(false);
            _imp->launchedRunnables.insert(runnable);
            threadPool->start(runnable.get());

            --nTasksRemaining;
            ++nTasksStarted;
        } else {
            k.unlock();
            runnable->run();
            k.relock();
        }

    }

    return nTasksStarted;

} // executeAvailableTasks


NATRON_NAMESPACE_EXIT

