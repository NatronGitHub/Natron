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
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/NodeMetadata.h"
#include "Engine/GPUContextPool.h"
#include "Engine/OSGLContext.h"
#include "Engine/RotoPaint.h"
#include "Engine/RotoStrokeItem.h"
#include "Engine/TreeRender.h"
#include "Engine/ThreadPool.h"
#include "Engine/ViewIdx.h"
#include "Engine/ViewerInstance.h"

#ifdef DEBUG
//#define TRACE_REQUEST_LIFETIME
#endif

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
};

typedef std::map<RequestPassSharedDataWPtr, PerLaunchRequestData> LaunchRequestDataMap;

struct FrameViewRequestPrivate
{
    // Protects all data members;
    mutable QMutex lock;

    // Weak reference to the render local arguments for the corresponding effect
    EffectInstanceWPtr renderClone;

    // The time at which to render
    TimeValue time;

    // The view at which to render
    ViewIdx view;

    // The proxy scale
    RenderScale proxyScale;

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

    // For each launch request a list of dependencies and listeners.
    LaunchRequestDataMap requestData;

    // The required frame/views in input, set on first request
    GetFramesNeededResultsPtr frameViewsNeeded;

    // The hash for this frame view
    U64 frameViewHash;

    // The needed components at this frame/view
    GetComponentsResultsPtr neededComps;

    // The distortion at this frame/view
    GetDistortionResultsPtr distortion;

    // The stack of upstram effect distortions
    Distortion2DStackPtr distortionStack;

#ifdef TRACE_REQUEST_LIFETIME
    std::string nodeName;
#endif

    // True if cache write is allowed but not cache read
    bool byPassCache;

    FrameViewRequestPrivate(TimeValue time,
                            ViewIdx view,
                            const RenderScale& proxyScale,
                            unsigned int mipMapLevel,
                            const ImagePlaneDesc& plane,
                            U64 timeViewHash,
                            const EffectInstancePtr& renderClone)
    : lock()
    , renderClone(renderClone)
    , time(time)
    , view(view)
    , proxyScale(proxyScale)
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
    , requestData()
    , frameViewsNeeded()
    , frameViewHash(timeViewHash)
    , neededComps()
    , distortion()
    , distortionStack()
#ifdef TRACE_REQUEST_LIFETIME
    , nodeName(renderClone->getNode()->getScriptName_mt_safe())
#endif
    , byPassCache()
    {
        
    }
};

FrameViewRequest::FrameViewRequest(TimeValue time,
                                   ViewIdx view,
                                   const RenderScale& proxyScale,
                                   unsigned int mipMapLevel,
                                   const ImagePlaneDesc& plane,
                                   U64 timeViewHash,
                                   const EffectInstancePtr& renderClone)
: _imp(new FrameViewRequestPrivate(time, view, proxyScale, mipMapLevel, plane, timeViewHash, renderClone))
{
#ifdef TRACE_REQUEST_LIFETIME
    qDebug() << "Create request" << _imp->nodeName.c_str();
#endif
    if (renderClone->getCurrentRender()->isByPassCacheEnabled()) {
        _imp->byPassCache = true;
    }
}

FrameViewRequest::~FrameViewRequest()
{
#ifdef TRACE_REQUEST_LIFETIME
    qDebug() << "Delete request" << _imp->nodeName.c_str();
#endif
}

EffectInstancePtr
FrameViewRequest::getRenderClone() const
{
    return _imp->renderClone.lock();
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

FrameViewRequest::FrameViewRequestStatusEnum
FrameViewRequest::getStatus() const
{
    QMutexLocker k(&_imp->statusMutex);
    return _imp->status;
}

void
FrameViewRequest::initStatus(FrameViewRequestStatusEnum status)
{
    {
        QMutexLocker k(&_imp->statusMutex);
        _imp->status = status;
    }
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
    _imp->status = FrameViewRequest::eFrameViewRequestStatusRendered;

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

RectD
FrameViewRequest::getCurrentRoI() const
{

    {
        QMutexLocker k(&_imp->lock);
        return _imp->finalRoi;
    }
}

void
FrameViewRequest::setCurrentRoI(const RectD& roi)
{
    QMutexLocker k(&_imp->lock);
    _imp->finalRoi = roi;
}

void
FrameViewRequest::addDependency(const RequestPassSharedDataPtr& request, const FrameViewRequestPtr& deps)
{

    {
        QMutexLocker k(&_imp->lock);
        PerLaunchRequestData& data = _imp->requestData[request];
        data.dependencies.insert(deps);
    }
}

int
FrameViewRequest::markDependencyAsRendered(const RequestPassSharedDataPtr& request, const FrameViewRequestPtr& deps)
{
    FrameViewRequestStatusEnum status;
    {
        QMutexLocker k1(&_imp->statusMutex);
        status = _imp->status;
    }

    QMutexLocker k(&_imp->lock);
    PerLaunchRequestData& data = _imp->requestData[request];

    // If this FrameViewRequest is pass-through, copy results from the pass-through dependency
    if (status == eFrameViewRequestStatusPassThrough) {
        assert(deps && data.dependencies.size() == 1 && *data.dependencies.begin() == deps);
        _imp->image = deps->getImagePlane();
        _imp->finalRoi = deps->getCurrentRoI();
    }

    std::set<FrameViewRequestPtr>::iterator foundDep = data.dependencies.find(deps);
    assert(foundDep != data.dependencies.end());
    if (foundDep != data.dependencies.end()) {
        data.dependencies.erase(foundDep);
        data.renderedDependencies.insert(deps);
    }
    return data.dependencies.size();
}

void
FrameViewRequest::clearRenderedDependencies(const RequestPassSharedDataPtr& request)
{
    QMutexLocker k(&_imp->lock);
    PerLaunchRequestData& data = _imp->requestData[request];
    data.renderedDependencies.clear();
}

int
FrameViewRequest::getNumDependencies(const RequestPassSharedDataPtr& request) const
{
    QMutexLocker k(&_imp->lock);
    PerLaunchRequestData& data = _imp->requestData[request];
    return data.dependencies.size();
}

void
FrameViewRequest::addListener(const RequestPassSharedDataPtr& request, const FrameViewRequestPtr& other)
{
    QMutexLocker k(&_imp->lock);
    PerLaunchRequestData& data = _imp->requestData[request];
    data.listeners.insert(other);
}

std::list<FrameViewRequestPtr>
FrameViewRequest::getListeners(const RequestPassSharedDataPtr& request) const
{
    std::list<FrameViewRequestPtr> ret;
    QMutexLocker k(&_imp->lock);
    PerLaunchRequestData& data = _imp->requestData[request];
    for (std::set<FrameViewRequestWPtr>::const_iterator it = data.listeners.begin(); it != data.listeners.end(); ++it) {
        ret.push_back(it->lock());
    }
    return ret;
}

std::size_t
FrameViewRequest::getNumListeners(const RequestPassSharedDataPtr& request) const
{
    QMutexLocker k(&_imp->lock);
    PerLaunchRequestData& data = _imp->requestData[request];
    return data.listeners.size();
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

GetDistortionResultsPtr
FrameViewRequest::getDistortionResults() const
{
    QMutexLocker k(&_imp->lock);
    return _imp->distortion;
}

void
FrameViewRequest::setDistortionResults(const GetDistortionResultsPtr& results)
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



NATRON_NAMESPACE_EXIT;
