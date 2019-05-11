/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
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

#include "FrameViewRequest.h"

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
#include "Engine/ImageCacheEntry.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/NodeMetadata.h"
#include "Engine/GPUContextPool.h"
#include "Engine/OSGLContext.h"
#include "Engine/RotoPaint.h"
#include "Engine/RotoStrokeItem.h"
#include "Engine/TreeRender.h"
#include "Engine/TreeRenderQueueManager.h"
#include "Engine/ThreadPool.h"
#include "Engine/ViewIdx.h"
#include "Engine/ViewerInstance.h"

#ifdef DEBUG
//#define TRACE_REQUEST_LIFETIME
#endif

NATRON_NAMESPACE_ENTER

bool
FrameView_compare_less::operator() (const FrameViewPair & lhs,
                                    const FrameViewPair & rhs) const
{
    if (lhs.time == rhs.time) {
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

bool
FrameViewRenderKey_compare_less::operator() (const FrameViewRenderKey & lhs,
                 const FrameViewRenderKey & rhs) const
{
    TreeRenderPtr lRender = lhs.render.lock();
    TreeRenderPtr rRender = rhs.render.lock();
    if (lRender.get() < rRender.get()) {
        return true;
    } else if (lRender.get() > rRender.get()) {
        return false;
    }

    if (lhs.time < rhs.time) {
        return true;
    } else if (lhs.time > rhs.time) {
        return false;
    }

    if (lhs.view < rhs.view) {
        return true;
    } else if (lhs.view > rhs.view) {
        return false;
    }
    return false;
}


struct PerLaunchRequestData
{

    // Dependencies of this frame/view.
    // This frame/view will not be able to render until all dependencies will be rendered.
    // (i.e: the set is empty)
    std::set<FrameViewRequestPtr> dependencies;

    // List of dependnencies that we already rendered (they are no longer in the dependencies set)
    // but that we still keep around so that the associated image plane is not destroyed.
    std::set<FrameViewRequestPtr> renderedDependencies;

    // The listeners of this frame/view:
    // This frame/view is in the dependencies list each of the listeners.
    std::set<FrameViewRequestWPtr> listeners;


    PerLaunchRequestData()
    {

    }

};

typedef std::map<TreeRenderExecutionDataWPtr, PerLaunchRequestData> LaunchRequestDataMap;

struct FrameViewRequestPrivate
{
    // Protects most data members
    mutable QMutex lock;

    // Protects the FrameViewRequest against multiple threads trying to render it
    mutable QMutex renderLock;

    // True if requestRenderInternal() was run at least once on this request
    bool isDrescribed;

    // Weak reference to the render local arguments for the corresponding effect
    EffectInstanceWPtr renderClone;

    // The tree render associated to this request
    TreeRenderWPtr parentRender;

    // The plane to render
    ImagePlaneDesc plane;

    // The proxy scale
    RenderScale proxyScale;

    // The mipmap level at which to render
    unsigned int mipMapLevel, renderMappedMipMapLevel;

    // The caching policy for this frame/view
    CacheAccessModeEnum cachingPolicy;

    // The device used to render the request
    RenderBackendTypeEnum renderDevice;

    // Was renderDevice set already
    bool renderDeviceSet;

    // Fallback device to use if the device that rendered first did not succeed.
    // E.g: First attempt to render using OpenGL or Cuda and if it fails fallback on CPU
    RenderBackendTypeEnum fallbackRenderDevice;

    // True if the render should use fallbackRenderDevice
    bool fallbackRenderDeviceEnabled;

    // The retCode of the launchRender function
    ActionRetCodeEnum retCode;

    // The full scale image is used for effects that do not support renderscale.
    // The requestedScaleImage is the final image
    ImagePtr fullScaleImage, requestedScaleImage;

    // Final roi. Each request led from different branches has its roi unioned into the finalRoI
    RectD finalRoi;

    // For each launch request a list of dependencies and listeners.
    LaunchRequestDataMap requestData;

    // The status of the frame/view
    FrameViewRequest::FrameViewRequestStatusEnum status;

    // The required frame/views in input, set on first request
    GetFramesNeededResultsPtr frameViewsNeeded;

    // The needed components at this frame/view
    GetComponentsResultsPtr neededComps;

    // The distortion at this frame/view
    GetDistortionResultsPtr distortion;

    // The stack of upstram effect distortions
    Distortion2DStackPtr distortionStack;

#ifdef TRACE_REQUEST_LIFETIME
    std::string nodeName;
#endif

    // RoD at each mipmap level
    std::vector<RectD> canonicalRoDs;
    std::vector<RectI> pixelRoDs;

    // True if cache write is allowed but not cache read
    bool byPassCache;

    FrameViewRequestPrivate(const ImagePlaneDesc& plane,
                            unsigned int mipMapLevel,
                            const RenderScale& proxyScale,
                            const EffectInstancePtr& effect,
                            const TreeRenderPtr& render)
    : lock()
    , renderLock()
    , isDrescribed(false)
    , renderClone(effect)
    , parentRender(render)
    , plane(plane)
    , proxyScale(proxyScale)
    , mipMapLevel(mipMapLevel)
    , renderMappedMipMapLevel(mipMapLevel)
    , cachingPolicy(eCacheAccessModeReadWrite)
    , renderDevice(eRenderBackendTypeCPU)
    , renderDeviceSet(false)
    , fallbackRenderDevice(eRenderBackendTypeCPU)
    , fallbackRenderDeviceEnabled(false)
    , retCode(eActionStatusOK)
    , fullScaleImage()
    , requestedScaleImage()
    , finalRoi()
    , requestData()
    , status(FrameViewRequest::eFrameViewRequestStatusNotRendered)
    , frameViewsNeeded()
    , neededComps()
    , distortion()
    , distortionStack()
    , canonicalRoDs()
    , pixelRoDs()
    , byPassCache(false)
    {
#ifdef TRACE_REQUEST_LIFETIME
        nodeName = effect->getNode()->getScriptName_mt_safe();
        qDebug() << "Create request" << nodeName.c_str();
#endif
        if (effect->getCurrentRender()->isByPassCacheEnabled()) {
            byPassCache = true;
        }

        if (effect->getOpenGLRenderSupport() == ePluginOpenGLRenderSupportNeeded) {
            // The plug-in can only use GPU, so make the device fallback be GPU
            fallbackRenderDevice = eRenderBackendTypeOpenGL;
        }
    }
};

FrameViewRequest::FrameViewRequest(const ImagePlaneDesc& plane,
                                   unsigned int mipMapLevel,
                                   const RenderScale& proxyScale,
                                   const EffectInstancePtr& effect,
                                   const TreeRenderPtr& render)
: _imp(new FrameViewRequestPrivate(plane, mipMapLevel, proxyScale, effect, render))
{

}

FrameViewRequest::~FrameViewRequest()
{
#ifdef TRACE_REQUEST_LIFETIME
    qDebug() << "Delete request" << _imp->nodeName.c_str();
#endif
}

TreeRenderPtr
FrameViewRequest::getParentRender() const
{
    return _imp->parentRender.lock();
}

EffectInstancePtr
FrameViewRequest::getEffect() const
{
    return _imp->renderClone.lock();
}


unsigned int
FrameViewRequest::getMipMapLevel() const
{
    return _imp->mipMapLevel;
}

unsigned int
FrameViewRequest::getRenderMappedMipMapLevel() const
{
    assert(!_imp->renderLock.tryLock());
    return _imp->renderMappedMipMapLevel;
}

void
FrameViewRequest::setRenderMappedMipMapLevel(unsigned int mipMapLevel) const
{
    QMutexLocker k(&_imp->lock);
    _imp->renderMappedMipMapLevel = mipMapLevel;
}

const RenderScale&
FrameViewRequest::getProxyScale() const
{
    return _imp->proxyScale;
}

const ImagePlaneDesc&
FrameViewRequest::getPlaneDesc() const
{
    return _imp->plane;
}

void
FrameViewRequest::setPlaneDesc(const ImagePlaneDesc& plane)
{
    assert(!_imp->renderLock.tryLock());
    _imp->plane = plane;
}

void
FrameViewRequest::setRequestedScaleImagePlane(const ImagePtr& image)
{
    QMutexLocker k(&_imp->lock);
    _imp->requestedScaleImage = image;
}


ImagePtr
FrameViewRequest::getRequestedScaleImagePlane() const
{
    QMutexLocker k(&_imp->lock);
    return _imp->requestedScaleImage;
}


ImagePtr
FrameViewRequest::getFullscaleImagePlane() const
{
    QMutexLocker k(&_imp->lock);
    return _imp->fullScaleImage;
}

void
FrameViewRequest::setFullscaleImagePlane(const ImagePtr& image)
{
    QMutexLocker k(&_imp->lock);
    _imp->fullScaleImage = image;
}


CacheAccessModeEnum
FrameViewRequest::getCachePolicy() const
{
    return _imp->cachingPolicy;
}

void
FrameViewRequest::setCachePolicy(CacheAccessModeEnum policy)
{
    assert(!_imp->renderLock.tryLock());
    _imp->cachingPolicy = policy;
}

FrameViewRequest::FrameViewRequestStatusEnum
FrameViewRequest::getStatus() const
{
    QMutexLocker k(&_imp->lock);
    return _imp->status;
}

void
FrameViewRequest::setDescribedOnce(bool described)
{
    assert(!_imp->renderLock.tryLock());
    _imp->isDrescribed = described;
}

bool
FrameViewRequest::getDescribedOnce() const
{
    QMutexLocker k(&_imp->lock);
    return _imp->isDrescribed;
}

FrameViewRequestLocker::FrameViewRequestLocker(const FrameViewRequestPtr& request, bool doLock)
: request(request)
, locked(false)
{
    if (doLock) {
        lockRequest();
    }
}

FrameViewRequestLocker::~FrameViewRequestLocker()
{
    unlockRequest();
}

bool
FrameViewRequestLocker::tryLockRequest()
{
    if (!locked) {
        locked = request->_imp->renderLock.tryLock();
        return locked;
    } else {
        return false;
    }
}

void
FrameViewRequestLocker::lockRequest()
{
    if (!locked) {
        if (!request->_imp->renderLock.tryLock()) {
            // We may take a while before getting the lock, so release this thread to the thread pool in the meantime
            ReleaseTPThread_RAII _thread_releaser;
            request->_imp->renderLock.lock();
            locked = true;
        } else {
            locked = true;
        }

    }
}

void
FrameViewRequestLocker::unlockRequest()
{
    if (locked) {
        
        request->_imp->renderLock.unlock();
        locked = false;
    }

}

void
FrameViewRequest::initStatus(FrameViewRequest::FrameViewRequestStatusEnum status)
{
    assert(!_imp->renderLock.tryLock());
    _imp->status = status;
}

FrameViewRequest::FrameViewRequestStatusEnum
FrameViewRequest::notifyRenderStarted()
{
    assert(!_imp->renderLock.tryLock());
    // Only one single thread should be computing a FrameViewRequest
    assert(_imp->status != FrameViewRequest::eFrameViewRequestStatusPending);
    if (_imp->status == FrameViewRequest::eFrameViewRequestStatusNotRendered) {
        _imp->status = FrameViewRequest::eFrameViewRequestStatusPending;
        return FrameViewRequest::eFrameViewRequestStatusNotRendered;
    }
    return _imp->status;
}


void
FrameViewRequest::notifyRenderFinished(ActionRetCodeEnum stat)
{
    assert(!_imp->renderLock.tryLock());
    assert(_imp->status == FrameViewRequest::eFrameViewRequestStatusPending);
    _imp->retCode = stat;
    _imp->status = FrameViewRequest::eFrameViewRequestStatusRendered;
}


RectD
FrameViewRequest::getCurrentRoI() const
{
    return _imp->finalRoi;
}

void
FrameViewRequest::setCurrentRoI(const RectD& roi)
{
    assert(!_imp->renderLock.tryLock());
    _imp->finalRoi = roi;
}

void
FrameViewRequest::addDependency(const TreeRenderExecutionDataPtr& request, const FrameViewRequestPtr& deps)
{

    {
        QMutexLocker k(&_imp->lock);
        PerLaunchRequestData& data = _imp->requestData[request];
        data.dependencies.insert(deps);
    }
}

int
FrameViewRequest::markDependencyAsRendered(const TreeRenderExecutionDataPtr& request, const FrameViewRequestPtr& deps)
{

    FrameViewRequestStatusEnum status = getStatus();
    QMutexLocker k(&_imp->lock);
    PerLaunchRequestData& data = _imp->requestData[request];

    // If this FrameViewRequest is pass-through, copy results from the pass-through dependency
    if (status == eFrameViewRequestStatusPassThrough) {
        assert(deps && data.dependencies.size() == 1 && *data.dependencies.begin() == deps);
        _imp->requestedScaleImage = deps->getRequestedScaleImagePlane();
        _imp->fullScaleImage = _imp->requestedScaleImage;
        _imp->finalRoi = deps->getCurrentRoI();
    }

    std::set<FrameViewRequestPtr>::iterator foundDep = data.dependencies.find(deps);
    if (foundDep != data.dependencies.end()) {
        // The dependency might not exist if we did not call addDependency.
        // This may happen if we were aborted
        data.dependencies.erase(foundDep);
        data.renderedDependencies.insert(deps);
    }
    return data.dependencies.size();
}

void
FrameViewRequest::clearRenderedDependencies(const TreeRenderExecutionDataPtr& request)
{
    QMutexLocker k(&_imp->lock);
    PerLaunchRequestData& data = _imp->requestData[request];
    data.renderedDependencies.clear();
}

int
FrameViewRequest::getNumDependencies(const TreeRenderExecutionDataPtr& request) const
{
    QMutexLocker k(&_imp->lock);
    PerLaunchRequestData& data = _imp->requestData[request];
    return data.dependencies.size();
}

void
FrameViewRequest::addListener(const TreeRenderExecutionDataPtr& request, const FrameViewRequestPtr& other)
{
    assert(other);
    QMutexLocker k(&_imp->lock);
    PerLaunchRequestData& data = _imp->requestData[request];
    data.listeners.insert(other);
}

std::list<FrameViewRequestPtr>
FrameViewRequest::getListeners(const TreeRenderExecutionDataPtr& request) const
{
    std::list<FrameViewRequestPtr> ret;
    QMutexLocker k(&_imp->lock);
    PerLaunchRequestData& data = _imp->requestData[request];
    for (std::set<FrameViewRequestWPtr>::const_iterator it = data.listeners.begin(); it != data.listeners.end(); ++it) {
        FrameViewRequestPtr l = it->lock();
        if (l) {
            ret.push_back(l);
        }
    }
    return ret;
}

std::size_t
FrameViewRequest::getNumListeners(const TreeRenderExecutionDataPtr& request) const
{
    assert(!_imp->renderLock.tryLock());
    PerLaunchRequestData& data = _imp->requestData[request];
    return data.listeners.size();
}

bool
FrameViewRequest::checkIfByPassCacheEnabledAndTurnoff() const
{
    assert(!_imp->renderLock.tryLock());
    if (_imp->byPassCache) {
        _imp->byPassCache = false;
        return true;
    }
    return false;
}

void
FrameViewRequest::setByPassCacheEnabled(bool enabled)
{
    assert(!_imp->renderLock.tryLock());
    _imp->byPassCache = enabled;
}

void
FrameViewRequest::setFallbackRenderDevice(RenderBackendTypeEnum device)
{
    _imp->fallbackRenderDevice = device;
}

void
FrameViewRequest::setRenderDevice(RenderBackendTypeEnum device)
{
    assert(!_imp->renderLock.tryLock());
    _imp->renderDevice = device;
    _imp->renderDeviceSet = true;
}

bool
FrameViewRequest::isRenderDeviceSet() const
{
    assert(!_imp->renderLock.tryLock());
    return _imp->renderDeviceSet;
}

RenderBackendTypeEnum
FrameViewRequest::getRenderDevice() const
{
    assert(!_imp->renderLock.tryLock());
    return _imp->renderDevice;
}

RenderBackendTypeEnum
FrameViewRequest::getFallbackRenderDevice() const
{
    assert(!_imp->renderLock.tryLock());
    return _imp->fallbackRenderDevice;
}


void
FrameViewRequest::setFallbackRenderDeviceEnabled(bool enabled)
{
    assert(!_imp->renderLock.tryLock());
    _imp->fallbackRenderDeviceEnabled = enabled;
}

bool
FrameViewRequest::isFallbackRenderDeviceEnabled() const
{
    assert(!_imp->renderLock.tryLock());
    return _imp->fallbackRenderDeviceEnabled;
}

GetFramesNeededResultsPtr
FrameViewRequest::getFramesNeededResults() const
{
    assert(!_imp->renderLock.tryLock());
    return _imp->frameViewsNeeded;

}

void
FrameViewRequest::setFramesNeededResults(const GetFramesNeededResultsPtr& framesNeeded)
{
    assert(!_imp->renderLock.tryLock());
    _imp->frameViewsNeeded = framesNeeded;
}

GetComponentsResultsPtr
FrameViewRequest::getComponentsResults() const
{
    assert(!_imp->renderLock.tryLock());
    return _imp->neededComps;
}

void
FrameViewRequest::setComponentsNeededResults(const GetComponentsResultsPtr& comps)
{
    assert(!_imp->renderLock.tryLock());
    _imp->neededComps = comps;
}

GetDistortionResultsPtr
FrameViewRequest::getDistortionResults() const
{
    assert(!_imp->renderLock.tryLock());
    return _imp->distortion;
}

void
FrameViewRequest::setDistortionResults(const GetDistortionResultsPtr& results)
{
    assert(!_imp->renderLock.tryLock());
    _imp->distortion = results;
}

Distortion2DStackPtr
FrameViewRequest::getDistorsionStack() const
{
    return _imp->distortionStack;
}

void
FrameViewRequest::setDistorsionStack(const Distortion2DStackPtr& stack)
{
    assert(!_imp->renderLock.tryLock());
    _imp->distortionStack = stack;
}

bool
FrameViewRequest::getRoDAtEachMipMapLevel(std::vector<RectD>* canonicalRoDs, std::vector<RectI>* pixelRoDs) const
{
    assert(!_imp->renderLock.tryLock());
    if (_imp->canonicalRoDs.empty()) {
        return false;
    }
    *canonicalRoDs = _imp->canonicalRoDs;
    *pixelRoDs = _imp->pixelRoDs;
    return true;
}

void
FrameViewRequest::setRoDAtEachMipMapLevel(const std::vector<RectD>& canonicalRoDs, const std::vector<RectI>& pixelRoDs)
{
    assert(!_imp->renderLock.tryLock());
    _imp->canonicalRoDs = canonicalRoDs;
    _imp->pixelRoDs = pixelRoDs;
}

NATRON_NAMESPACE_EXIT
