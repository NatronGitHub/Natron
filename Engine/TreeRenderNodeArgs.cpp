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

#include "TreeRenderNodeArgs.h"

#include <cassert>
#include <stdexcept>

#include <QDebug>
#include <QtConcurrentMap> // QtCore on Qt4, QtConcurrent on Qt5


#if !defined(SBK_RUN) && !defined(Q_MOC_RUN)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
// /usr/local/include/boost/bind/arg.hpp:37:9: warning: unused typedef 'boost_static_assert_typedef_37' [-Wunused-local-typedef]
#include <boost/bind.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
#endif

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/Settings.h"
#include "Engine/Hash64.h"
#include "Engine/EffectInstance.h"
#include "Engine/Image.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/NodeMetadata.h"
#include "Engine/GPUContextPool.h"
#include "Engine/OSGLContext.h"
#include "Engine/RenderValuesCache.h"
#include "Engine/RotoPaint.h"
#include "Engine/RotoStrokeItem.h"
#include "Engine/TreeRender.h"
#include "Engine/ThreadPool.h"
#include "Engine/ViewIdx.h"
#include "Engine/ViewerInstance.h"


NATRON_NAMESPACE_ENTER;

bool
FrameView_compare_less::operator() (const FrameViewPair & lhs,
                                    const FrameViewPair & rhs) const
{
    if (std::abs(lhs.time - rhs.time) < NATRON_IMAGE_TIME_EQUALITY_EPS) {
        if (lhs.view == -1 || rhs.view == -1 || lhs.view == rhs.view) {
            return false;
        }
        if (lhs.view < rhs.view) {
            return true;
        } else {
            // lhs.view > rhs.view
            return false;
        }
    } else if (lhs.time < rhs.time) {
        return true;
    } else {
        assert(lhs.time > rhs.time);
        return false;
    }
}

struct PreRenderedDataKey
{
    TimeValue time;
    ViewIdx view;
    int inputNb;
};

struct PreRenderedDataKey_Compare
{
    bool operator() (const PreRenderedDataKey& lhs, const PreRenderedDataKey& rhs) const
    {
        if (lhs.inputNb < rhs.inputNb) {
            return true;
        } else if (lhs.inputNb > rhs.inputNb) {
            return false;
        } else {
            if (lhs.time < rhs.time) {
                return true;
            } else if (lhs.time > rhs.time) {
                return false;
            } else {
                return lhs.view < rhs.view;
            }
        }
    }
};

struct PreRenderedDataStuff
{
    std::map<ImagePlaneDesc, ImagePtr> planes;
    Distortion2DStackPtr distortion;
};

typedef std::map<PreRenderedDataKey, PreRenderedDataStuff, PreRenderedDataKey_Compare> PreRenderedDataMap;

struct FrameViewRequestPrivate
{
    // Protects all data members;
    mutable QMutex lock;

    // Weak reference to the render local arguments for the corresponding effect
    TreeRenderNodeArgsWPtr renderArgs;

    // The time at which to render
    TimeValue time;

    // The view at which to render
    ViewIdx view;

    // The mipmap level at which to render
    unsigned int mipMapLevel, renderMappedMipMapLevel;

    // The plane to render
    ImagePlaneDesc plane;

    // The caching policy for this frame/view
    CacheAccessModeEnum cachingPolicy;

    // Protects status
    mutable QMutex statusMutex;

    // Used when the status is pending
    QWaitCondition statusPendingCond;

    // The status of the frame/view
    FrameViewRequest::FrameViewRequestStatusEnum status;

    // The retCode of the launchRender function
    ActionRetCodeEnum retCode;

    // The output image
    ImagePtr image;

    // Final roi. Each request led from different branches has its roi unioned into the finalRoI
    RectD finalRoi;

    // Dependencies of this frame/view.
    // This frame/view will not be able to render until all dependencies will be rendered.
    // (i.e: the set is empty)
    std::set<FrameViewRequestWPtr> dependencies;

    // The listeners of this frame/view:
    // This frame/view is in the dependencies list each of the listeners.
    std::set<FrameViewRequestWPtr> listeners;

    // The required frame/views in input, set on first request
    GetFramesNeededResultsPtr frameViewsNeeded;

    // The hash for this frame view
    U64 frameViewHash;

    // The pre-rendered input images
    PreRenderedDataMap inputImages;

    // The needed components at this frame/view
    GetComponentsResultsPtr neededComps;

    // The distortion at this frame/view
    DistortionFunction2DPtr distortion;

    // The stack of upstram effect distortions
    Distortion2DStackPtr distortionStack;

    // True if cache write is allowed but not cache read
    bool byPassCache;


    FrameViewRequestPrivate(TimeValue time,
                            ViewIdx view,
                            unsigned int mipMapLevel,
                            const ImagePlaneDesc& plane,
                            U64 timeViewHash,
                            const TreeRenderNodeArgsPtr& renderArgs)
    : lock()
    , renderArgs(renderArgs)
    , time(time)
    , view(view)
    , mipMapLevel(mipMapLevel)
    , renderMappedMipMapLevel(mipMapLevel)
    , plane(plane)
    , cachingPolicy(eCacheAccessModeReadWrite)
    , statusMutex()
    , statusPendingCond()
    , status(FrameViewRequest::eFrameViewRequestStatusNotRendered)
    , retCode(eActionStatusOK)
    , image()
    , finalRoi()
    , dependencies()
    , listeners()
    , frameViewsNeeded()
    , frameViewHash(timeViewHash)
    , inputImages()
    , neededComps()
    , distortion()
    , distortionStack()
    , byPassCache()
    {
        
    }
};

FrameViewRequest::FrameViewRequest(TimeValue time,
                                   ViewIdx view,
                                   unsigned int mipMapLevel,
                                   const ImagePlaneDesc& plane,
                                   U64 timeViewHash,
                                   const TreeRenderNodeArgsPtr& renderArgs)
: _imp(new FrameViewRequestPrivate(time, view, mipMapLevel, plane, timeViewHash, renderArgs))
{

    assert(renderArgs);
    if (renderArgs->getParentRender()->isByPassCacheEnabled()) {
        _imp->byPassCache = true;
    }
}

FrameViewRequest::~FrameViewRequest()
{

}

TreeRenderNodeArgsPtr
FrameViewRequest::getRenderArgs() const
{
    return _imp->renderArgs.lock();
}

TimeValue
FrameViewRequest::getTime() const
{
    return _imp->time;
}

ViewIdx
FrameViewRequest::getView() const
{
    return _imp->view;
}

unsigned int
FrameViewRequest::getMipMapLevel() const
{
    return _imp->mipMapLevel;
}

unsigned int
FrameViewRequest::getRenderMappedMipMapLevel() const
{
    return _imp->renderMappedMipMapLevel;
}

void
FrameViewRequest::setRenderMappedMipMapLevel(unsigned int mipMapLevel) const
{
    _imp->renderMappedMipMapLevel = mipMapLevel;
}

const ImagePlaneDesc&
FrameViewRequest::getPlaneDesc() const
{
    return _imp->plane;
}

void
FrameViewRequest::setPlaneDesc(const ImagePlaneDesc& plane)
{
    _imp->plane = plane;
}

void
FrameViewRequest::setImagePlane(const ImagePtr& image)
{
    _imp->image = image;
}

ImagePtr
FrameViewRequest::getImagePlane() const
{
    return _imp->image;
}

CacheAccessModeEnum
FrameViewRequest::getCachePolicy() const
{
    return _imp->cachingPolicy;
}

void
FrameViewRequest::setCachePolicy(CacheAccessModeEnum policy)
{
    _imp->cachingPolicy = policy;
}

void
FrameViewRequest::initStatus(FrameViewRequestStatusEnum status)
{
    QMutexLocker k(&_imp->statusMutex);
    _imp->status = status;
}

FrameViewRequest::FrameViewRequestStatusEnum
FrameViewRequest::notifyRenderStarted()
{
    QMutexLocker k(&_imp->statusMutex);
    if (_imp->status == FrameViewRequest::eFrameViewRequestStatusNotRendered) {
        _imp->status = FrameViewRequest::eFrameViewRequestStatusPending;
        return FrameViewRequest::eFrameViewRequestStatusNotRendered;
    }
    return _imp->status;
}


void
FrameViewRequest::notifyRenderFinished(ActionRetCodeEnum stat)
{
    QMutexLocker k(&_imp->statusMutex);
    _imp->retCode = stat;

    // Wake-up all other threads stuck in waitForPendingResults()
    _imp->statusPendingCond.wakeAll();
}


ActionRetCodeEnum
FrameViewRequest::waitForPendingResults()
{
    // If this thread is a threadpool thread, it may wait for a while that results gets available.
    // Release the thread to the thread pool so that it may use this thread for other runnables
    // and reserve it back when done waiting.
    bool hasReleasedThread = false;
    if (isRunningInThreadPoolThread()) {
        QThreadPool::globalInstance()->releaseThread();
        hasReleasedThread = true;
    }
    ActionRetCodeEnum ret;
    {
        QMutexLocker k(&_imp->statusMutex);
        while (_imp->status == FrameViewRequest::eFrameViewRequestStatusPending) {
            _imp->statusPendingCond.wait(&_imp->statusMutex);
        }
        ret = _imp->retCode;
    }
    if (hasReleasedThread) {
        QThreadPool::globalInstance()->reserveThread();
    }

    return ret;
} // waitForPendingResults

void
FrameViewRequest::appendPreRenderedInputs(int inputNb,
                                          TimeValue time,
                                          ViewIdx view,
                                          const std::map<ImagePlaneDesc, ImagePtr>& planes,
                                          const Distortion2DStackPtr& distortionStack)
{
    QMutexLocker k(&_imp->lock);

    PreRenderedDataKey key;
    key.time = time;
    key.view = view;
    key.inputNb = inputNb;

    PreRenderedDataStuff& data = _imp->inputImages[key];
    data.distortion = distortionStack;

    for (std::map<ImagePlaneDesc, ImagePtr>::const_iterator it = planes.begin(); it != planes.end(); ++it) {
        bool isColorPlane = it->first.isColorPlane();

        std::map<ImagePlaneDesc, ImagePtr>::iterator foundImage = data.planes.end();
        for (std::map<ImagePlaneDesc, ImagePtr>::iterator it2 = data.planes.begin(); it2 != data.planes.end(); ++it2) {
            if (it2->first.isColorPlane() && isColorPlane) {
                foundImage = it2;
                break;
            } else if (it->first == it2->first) {
                foundImage = it2;
                break;
            }
        }
        // If we already have a pre-rendered corresponding image, replace it only if the new image
        // is at least equal in bounds or greater
        if (foundImage != data.planes.end()) {
            if (it->second->getBounds().contains(foundImage->second->getBounds())) {
                foundImage->second = it->second;
            }
        } else {
            data.planes.insert(*it);
        }
    }
} // appendPreRenderedInputs



void
FrameViewRequest::getPreRenderedInputs(int inputNb,
                                       TimeValue time,
                                       ViewIdx view,
                                       const RectI& roi,
                                       const std::list<ImagePlaneDesc>& layers,
                                       std::map<ImagePlaneDesc, ImagePtr>* planes,
                                       std::list<ImagePlaneDesc>* planesLeftToRendered,
                                       Distortion2DStackPtr* distortionStack) const
{
    QMutexLocker k(&_imp->lock);
    PreRenderedDataKey key;
    key.time = time;
    key.view = view;
    key.inputNb = inputNb;

    PreRenderedDataMap::const_iterator foundData = _imp->inputImages.find(key);
    if (foundData == _imp->inputImages.end()) {
        *planesLeftToRendered = layers;
        return;
    }

    for (std::list<ImagePlaneDesc>::const_iterator it = layers.begin(); it != layers.end(); ++it) {

        bool isColorPlane = it->isColorPlane();

        std::map<ImagePlaneDesc, ImagePtr>::const_iterator foundImage = foundData->second.planes.end();
        for (std::map<ImagePlaneDesc, ImagePtr>::const_iterator it2 = foundData->second.planes.begin(); it2 != foundData->second.planes.end(); ++it2) {
            if (it2->first.isColorPlane() && isColorPlane) {
                foundImage = it2;
                break;
            } else if (*it == it2->first) {
                foundImage = it2;
                break;
            }
        }
        if (foundImage != foundData->second.planes.end()) {
            if (foundImage->second->getBounds().contains(roi)) {
                planes->insert(*foundImage);
                continue;
            }
        }
        planesLeftToRendered->push_back(*it);
    }

    if (foundData->second.distortion) {
        *distortionStack = foundData->second.distortion;
    }

} // getPreRenderedInputs


void
FrameViewRequest::clearPreRenderedInputs()
{

    QMutexLocker k(&_imp->lock);
    _imp->inputImages.clear();
} // clearPreRenderedInputs

RectD
FrameViewRequest::getCurrentRoI() const
{
    QMutexLocker k(&_imp->lock);
    return _imp->finalRoi;
}

void
FrameViewRequest::setCurrentRoI(const RectD& roi)
{
    QMutexLocker k(&_imp->lock);
    _imp->finalRoi = roi;
}

void
FrameViewRequest::addDependency(const FrameViewRequestPtr& effectRequesting)
{
    QMutexLocker k(&_imp->lock);
    _imp->dependencies.insert(effectRequesting);
}

void
FrameViewRequest::removeDependency(const FrameViewRequestPtr& effectRequesting)
{
    QMutexLocker k(&_imp->lock);
    _imp->dependencies.insert(effectRequesting);
}

int
FrameViewRequest::getNumDependencies() const
{
    QMutexLocker k(&_imp->lock);
    return _imp->dependencies.size();
}

void
FrameViewRequest::addListener(const FrameViewRequestPtr& other)
{
    QMutexLocker k(&_imp->lock);
    _imp->listeners.insert(other);
}

std::list<FrameViewRequestPtr>
FrameViewRequest::getListeners() const
{
    std::list<FrameViewRequestPtr> ret;
    QMutexLocker k(&_imp->lock);
    for (std::set<FrameViewRequestWPtr>::const_iterator it = _imp->listeners.begin(); it != _imp->listeners.end(); ++it) {
        ret.push_back(it->lock());
    }
    return ret;
}

bool
FrameViewRequest::checkIfByPassCacheEnabledAndTurnoff() const
{
    QMutexLocker k(&_imp->lock);
    if (_imp->byPassCache) {
        _imp->byPassCache = false;
        return true;
    }
    return false;
}



U64
FrameViewRequest::getHash() const
{
    QMutexLocker k(&_imp->lock);
    return _imp->frameViewHash;
}


GetFramesNeededResultsPtr
FrameViewRequest::getFramesNeededResults() const
{
    QMutexLocker k(&_imp->lock);
    return _imp->frameViewsNeeded;

}

void
FrameViewRequest::setFramesNeededResults(const GetFramesNeededResultsPtr& framesNeeded)
{
    QMutexLocker k(&_imp->lock);
    _imp->frameViewsNeeded = framesNeeded;
}

GetComponentsResultsPtr
FrameViewRequest::getComponentsResults() const
{
    QMutexLocker k(&_imp->lock);
    return _imp->neededComps;
}

void
FrameViewRequest::setComponentsNeededResults(const GetComponentsResultsPtr& comps)
{
    QMutexLocker k(&_imp->lock);
    _imp->neededComps = comps;
}

DistortionFunction2DPtr
FrameViewRequest::getDistortionResults() const
{
    QMutexLocker k(&_imp->lock);
    return _imp->distortion;
}

void
FrameViewRequest::setDistortionResults(const DistortionFunction2DPtr& results)
{
    QMutexLocker k(&_imp->lock);
    _imp->distortion = results;
}

Distortion2DStackPtr
FrameViewRequest::getDistorsionStack() const
{
    QMutexLocker k(&_imp->lock);
    return _imp->distortionStack;
}

void
FrameViewRequest::setDistorsionStack(const Distortion2DStackPtr& stack)
{
    QMutexLocker k(&_imp->lock);
    _imp->distortionStack = stack;
}

EffectInstancePtr
EffectInstance::resolveInputEffectForFrameNeeded(const int inputNb,
                                                 int* channelForMask)
{
    // Check if the input is a mask
    if (channelForMask) {
        *channelForMask = -1;
    }
    if (isInputMask(inputNb)) {

        // If the mask is disabled, don't even bother
        if (!isMaskEnabled(inputNb) ) {
            return EffectInstancePtr();
        }

        ImagePlaneDesc maskComps;

        std::list<ImagePlaneDesc> upstreamAvailableLayers;

        TreeRenderNodeArgsPtr currentRender = getCurrentRender_TLS();
        ActionRetCodeEnum stat = getAvailableLayers(getCurrentTime_TLS(), getCurrentView_TLS(), inputNb, currentRender, &upstreamAvailableLayers);
        if (isFailureRetCode(stat)) {
            return EffectInstancePtr();
        }
        int channelForAlphaInput = getNode()->getMaskChannel(inputNb, upstreamAvailableLayers, &maskComps);


        if (channelForMask) {
            *channelForMask = channelForAlphaInput;
        }

        // No mask or no layer selected for the mask
        if ((channelForAlphaInput == -1) || (maskComps.getNumComponents() == 0)) {
            return EffectInstancePtr();
        }

    }

    return getInput(inputNb);

} // resolveInputEffectForFrameNeeded


struct TreeRenderNodeArgsPrivate
{
    mutable QMutex lock;

    // A parent to the render holding this struct
    TreeRenderWPtr parentRender;

    // The node to which these args correspond to
    NodePtr node;

    // Pointer to render args of all inputs
    std::vector<TreeRenderNodeArgsWPtr> inputRenderArgs;

    // Contains data for all frame/view pair that are going to be computed
    // for this frame/view pair with the overall RoI to avoid rendering several times with this node.
    NodeFrameViewRequestData frames;

    // The results of the get frame range action for this render
    GetFrameRangeResultsPtr frameRangeResults;

    // The time invariant metadas for the render
    GetTimeInvariantMetaDatasResultsPtr metadatasResults;

    // Local copies of render data such as knob values, node inputs etc...
    // that should remain the same throughout the render of the frame.
    RenderValuesCachePtr valuesCache;

    // Current thread safety: it might change in the case of the rotopaint: while drawing, the safety is instance safe,
    // whereas afterwards we revert back to the plug-in thread safety
    RenderSafetyEnum currentThreadSafety;

    //Current OpenGL support: it might change during instanceChanged action
    PluginOpenGLRenderSupport currentOpenglSupport;

    // Current sequential support
    SequentialPreferenceEnum currentSequentialPref;

    // This is the hash used to cache time and view invariant stuff
    U64 timeViewInvariantHash;

    // Hash used for metadata (depdending only for parameters that
    // are metadata dependent)
    U64 metadataTimeInvariantHash;

    // Time/view variant hash
    FrameViewHashMap timeViewVariantHash;

    // The number of times this node has been visited (i.e: the number of times we called renderRoI on it)
    int nVisits;

    // True if hash is valid
    bool timeViewInvariantHashValid;

    bool metadataTimeInvariantHashValid;

    // The support for tiles is local to a render and may change depending on GPU usage or other parameters
    bool tilesSupported;

    // The support for render scale is local to a render
    bool renderScaleSupported;

    // The distort flag may change (e.g Reformat may provide a transform or not depending on a parameter)
    bool canDistort;

    bool canTransformDeprecated;

    TreeRenderNodeArgsPrivate(const TreeRenderPtr& render, const NodePtr& node)
    : lock()
    , parentRender(render)
    , node(node)
    , inputRenderArgs(node->getMaxInputCount())
    , frames()
    , frameRangeResults()
    , valuesCache(new RenderValuesCache)
    , currentThreadSafety(node->getCurrentRenderThreadSafety())
    , currentOpenglSupport(node->getCurrentOpenGLRenderSupport())
    , currentSequentialPref(node->getCurrentSequentialRenderSupport())
    , timeViewInvariantHash(0)
    , metadataTimeInvariantHash(0)
    , timeViewVariantHash()
    , nVisits(0)
    , timeViewInvariantHashValid(false)
    , metadataTimeInvariantHashValid(false)
    , tilesSupported(node->getCurrentSupportTiles())
    , renderScaleSupported(node->getCurrentSupportRenderScale())
    , canDistort(node->getCurrentCanDistort())
    , canTransformDeprecated(node->getCurrentCanTransform())
    {

        // Create a copy of the roto item if needed
        RotoDrawableItemPtr attachedRotoItem = node->getOriginalAttachedItem();
        if (attachedRotoItem && attachedRotoItem->isRenderCloneNeeded()) {
            attachedRotoItem->getOrCreateCachedDrawable(render);
        }

    }

    

};


TreeRenderNodeArgs::TreeRenderNodeArgs(const TreeRenderPtr& render, const NodePtr& node)
: _imp(new TreeRenderNodeArgsPrivate(render, node))
{
    
}

TreeRenderNodeArgsPtr
TreeRenderNodeArgs::create(const TreeRenderPtr& render, const NodePtr& node)
{
    TreeRenderNodeArgsPtr ret(new TreeRenderNodeArgs(render, node));
    return ret;
}

TreeRenderNodeArgs::~TreeRenderNodeArgs()
{

}

RenderValuesCachePtr
TreeRenderNodeArgs::getRenderValuesCache() const
{
    return _imp->valuesCache;
}

TreeRenderPtr
TreeRenderNodeArgs::getParentRender() const
{
    // MT-safe: never changes throughout the lifetime of the object
    return _imp->parentRender.lock();
}

TimeValue
TreeRenderNodeArgs::getTime() const
{
    return getParentRender()->getTime();
}


ViewIdx
TreeRenderNodeArgs::getView() const
{
    return getParentRender()->getView();
}

bool
TreeRenderNodeArgs::isRenderAborted() const
{
    // MT-safe: is aborted is MT-safe on the TreeRender class.
    return _imp->parentRender.lock()->isRenderAborted();
}

NodePtr
TreeRenderNodeArgs::getNode() const
{
    // MT-safe: never changes throughout the lifetime of the object
    return _imp->node;
}

void
TreeRenderNodeArgs::setInputRenderArgs(int inputNb, const TreeRenderNodeArgsPtr& inputRenderArgs)
{
    // MT-safe: never changes throughout the lifetime of the object
    if (inputNb < 0 || inputNb >= (int)_imp->inputRenderArgs.size()) {
        throw std::invalid_argument("TreeRenderNodeArgs::setInputRenderArgs: input index out of range");
    }
    _imp->inputRenderArgs[inputNb] = inputRenderArgs;
}

TreeRenderNodeArgsPtr
TreeRenderNodeArgs::getInputRenderArgs(int inputNb) const
{
    // MT-safe: never changes throughout the lifetime of the object
    if (inputNb < 0 || inputNb >= (int)_imp->inputRenderArgs.size()) {
        return TreeRenderNodeArgsPtr();
    }
    return _imp->inputRenderArgs[inputNb].lock();
}

NodePtr
TreeRenderNodeArgs::getInputNode(int inputNb) const
{
    TreeRenderNodeArgsPtr args = getInputRenderArgs(inputNb);
    if (!args) {
        return NodePtr();
    }
    return args->getNode();
}


FrameViewRequestPtr
TreeRenderNodeArgs::getFrameViewRequest(TimeValue time,
                                      ViewIdx view) const
{
    // Needs to be locked: frame requests may be added spontaneously by the plug-in
    QMutexLocker k(&_imp->lock);

    FrameViewPair p = {time,view};
    NodeFrameViewRequestData::const_iterator found = _imp->frames.find(p);
    if (found == _imp->frames.end()) {
        return FrameViewRequestPtr();
    }
    return found->second;
}

bool
TreeRenderNodeArgs::getOrCreateFrameViewRequest(TimeValue time, ViewIdx view, unsigned int mipMapLevel, const ImagePlaneDesc& plane, FrameViewRequestPtr* request)
{
    
    // Needs to be locked: frame requests may be added spontaneously by the plug-in
    QMutexLocker k(&_imp->lock);

    FrameViewPair p = {time, view};
    NodeFrameViewRequestData::iterator found = _imp->frames.find(p);
    if (found != _imp->frames.end()) {
        *request = found->second;
        return false;
    }

    U64 hash;
    if (!getFrameViewHash(time, view, &hash)) {
        HashableObject::ComputeHashArgs args;
        args.time = time;
        args.view = view;
        args.render = shared_from_this();
        args.hashType = HashableObject::eComputeHashTypeTimeViewVariant;
        hash = getNode()->getEffectInstance()->computeHash(args);
        setFrameViewHash(time, view, hash);
    }

    FrameViewRequestPtr& ret = _imp->frames[p];
    ret.reset(new FrameViewRequest(time, view, mipMapLevel, plane, hash, shared_from_this()));
    *request = ret;
    return true;
}


RenderSafetyEnum
TreeRenderNodeArgs::getCurrentRenderSafety() const
{
    // MT-safe: never changes throughout the lifetime of the object
    return _imp->currentThreadSafety;
}


PluginOpenGLRenderSupport
TreeRenderNodeArgs::getCurrentRenderOpenGLSupport() const
{
    // MT-safe: never changes throughout the lifetime of the object
    return _imp->currentOpenglSupport;
}

bool
TreeRenderNodeArgs::getCurrentTilesSupport() const
{
    // MT-safe: never changes throughout the lifetime of the object
    return _imp->tilesSupported;
}

bool
TreeRenderNodeArgs::getCurrentRenderScaleSupport() const
{
    // MT-safe: never changes throughout the lifetime of the object
    return _imp->renderScaleSupported;
}

bool
TreeRenderNodeArgs::getCurrentDistortSupport() const
{
    // MT-safe: never changes throughout the lifetime of the object
    return _imp->canDistort;
}

bool
TreeRenderNodeArgs::getCurrentTransformationSupport_deprecated() const
{
    return _imp->canTransformDeprecated;
}

SequentialPreferenceEnum
TreeRenderNodeArgs::getCurrentRenderSequentialPreference() const
{
    // MT-safe: never changes throughout the lifetime of the object
    return _imp->currentSequentialPref;
}

void
TreeRenderNodeArgs::setFrameRangeResults(const GetFrameRangeResultsPtr& range)
{
    QMutexLocker k(&_imp->lock);
    _imp->frameRangeResults = range;
}

GetFrameRangeResultsPtr
TreeRenderNodeArgs::getFrameRangeResults() const
{
    QMutexLocker k(&_imp->lock);
    return _imp->frameRangeResults;
}

void
TreeRenderNodeArgs::setTimeInvariantMetadataResults(const GetTimeInvariantMetaDatasResultsPtr& metadatas)
{
    QMutexLocker k(&_imp->lock);
    _imp->metadatasResults = metadatas;
}


GetTimeInvariantMetaDatasResultsPtr
TreeRenderNodeArgs::getTimeInvariantMetadataResults() const
{
    QMutexLocker k(&_imp->lock);
    return _imp->metadatasResults;
}

bool
TreeRenderNodeArgs::getTimeViewInvariantHash(U64* hash) const
{
    QMutexLocker k(&_imp->lock);
    if (!_imp->timeViewInvariantHashValid) {
        return false;
    }
    *hash = _imp->timeViewInvariantHash;
    return true;
}

void
TreeRenderNodeArgs::setTimeInvariantMetadataHash(U64 hash)
{
    QMutexLocker k(&_imp->lock);
    _imp->metadataTimeInvariantHashValid = true;
    _imp->metadataTimeInvariantHash = hash;
}

bool
TreeRenderNodeArgs::getTimeInvariantMetadataHash(U64* hash) const
{
    QMutexLocker k(&_imp->lock);
    if (!_imp->metadataTimeInvariantHashValid) {
        return false;
    }
    *hash = _imp->metadataTimeInvariantHash;
    return true;

}

void
TreeRenderNodeArgs::setTimeViewInvariantHash(U64 hash)
{
    QMutexLocker k(&_imp->lock);
    _imp->timeViewInvariantHash = hash;
    _imp->timeViewInvariantHashValid = true;
}

bool
TreeRenderNodeArgs::getFrameViewHash(TimeValue time, ViewIdx view, U64* hash) const
{
    FrameViewPair p = {time,view};
    QMutexLocker k(&_imp->lock);
    FrameViewHashMap::const_iterator found = _imp->timeViewVariantHash.find(p);
    if (found == _imp->timeViewVariantHash.end()) {
        return false;
    }
    *hash = found->second;
    return true;
}

void
TreeRenderNodeArgs::setFrameViewHash(TimeValue time, ViewIdx view, U64 hash)
{
    QMutexLocker k(&_imp->lock);
    FrameViewPair p = {time,view};
    _imp->timeViewVariantHash[p] = hash;
}

ActionRetCodeEnum
TreeRenderNodeArgs::requestRender(TimeValue time,
                                  ViewIdx view,
                                  unsigned int mipMapLevel,
                                  const ImagePlaneDesc& plane,
                                  const RectD& canonicalRenderWindow,
                                  int inputNbInRequester,
                                  const FrameViewRequestPtr& requester,
                                  FrameViewRequestPtr* createdRequest)
{

    NodePtr node = getNode();
    EffectInstancePtr effect = node->getEffectInstance();
    
    TreeRenderNodeArgsPtr thisShared = shared_from_this();

    FrameViewPair frameView;
    // Requested time is rounded to an epsilon so we can be sure to find it again in getImage, accounting for precision
    frameView.time = roundImageTimeToEpsilon(time);
    frameView.view = view;

    // Create the frame/view request object
    FrameViewRequestPtr thisFrameView;
    {
        bool created = getOrCreateFrameViewRequest(frameView.time, frameView.view, mipMapLevel, plane, &thisFrameView);
        (void)created;
    }
    *createdRequest = thisFrameView;

    // If this render has no dependencies, add it to the things to render
    if (thisFrameView->getNumDependencies() == 0) {
        getParentRender()->addDependencyFreeRender(thisFrameView);
    }
    getParentRender()->addTaskToRender(thisFrameView);

    // Add this frame/view as depdency of the requester
    if (requester) {
        requester->addDependency(thisFrameView);
    }

    effect->requestRender(thisFrameView, canonicalRenderWindow, inputNbInRequester, requester);

    return eActionStatusOK;
} // requestRender



NATRON_NAMESPACE_EXIT;
