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

#ifndef NATRON_ENGINE_FRAMEVIEWREQUEST_H
#define NATRON_ENGINE_FRAMEVIEWREQUEST_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <set>
#include <map>
#include <list>
#include <cmath>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#endif
#include "Global/GlobalDefines.h"

#include "Engine/EffectInstanceActionResults.h"
#include "Engine/RectD.h"
#include "Engine/ViewIdx.h"
#include "Engine/TimeValue.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER;

// If 2 image times differ by lesser than this epsilon they are assumed the same.
#define NATRON_IMAGE_TIME_EQUALITY_EPS 1e-5
#define NATRON_IMAGE_TIME_EQUALITY_DECIMALS 5

inline TimeValue roundImageTimeToEpsilon(TimeValue time)
{
    int exp = std::pow(10, NATRON_IMAGE_TIME_EQUALITY_DECIMALS);
    return TimeValue(std::floor(time * exp + 0.5) / exp);
}


typedef std::map<int, RectD> RoIMap; // RoIs are in canonical coordinates


struct FrameViewPair
{
    TimeValue time;
    ViewIdx view;
};


struct FrameView_compare_less
{
    bool operator() (const FrameViewPair & lhs,
                     const FrameViewPair & rhs) const;
};

typedef std::map<FrameViewPair, U64, FrameView_compare_less> FrameViewHashMap;

inline bool findFrameViewHash(TimeValue time, ViewIdx view, const FrameViewHashMap& table, U64* hash)
{
    FrameViewPair fv = {roundImageTimeToEpsilon(time), view};
    FrameViewHashMap::const_iterator it = table.find(fv);
    if (it != table.end()) {
        *hash = it->second;
        return true;
    }
    *hash = 0;
    return false;

}

/**
 * @brief These are datas related to a single frame/view render of a node.
 * Each request to a node time/view will increase the visits count on this request.
 * This is thread-safe.
 **/
struct FrameViewRequestPrivate;
class FrameViewRequest
{
public:

    /**
     * @brief Status code returned by the notifyRenderStarted() function.
     **/
    enum FrameViewRequestStatusEnum
    {
        // The launchRender() function was never called for this object:
        // The caller is expected to render the frame
        eFrameViewRequestStatusNotRendered,

        // This object is pass-through: launchRender() should
        // return immediately
        eFrameViewRequestStatusPassThrough,

        // The frame is being rendered by another thread, the caller
        // is expected to call waitForPendingResults() until
        // results become available.
        eFrameViewRequestStatusPending,

        // The frame has been rendered: the caller can just return
        // immediately from launchRender()
        eFrameViewRequestStatusRendered,
    };

    FrameViewRequest(TimeValue time,
                     ViewIdx view,
                     const RenderScale& proxyScale,
                     unsigned int mipMapLevel,
                     const ImagePlaneDesc& plane,
                     U64 timeViewHash,
                     const EffectInstancePtr& renderClone);

    ~FrameViewRequest();

    /**
     * @brief Get the frame of the render
     **/
    TimeValue getTime() const;

    /**
     * @brief Get the view of the render
     **/
    ViewIdx getView() const;

    /**
     * @brief Get the mipmap level at which to render
     **/
    unsigned int getMipMapLevel() const;

    /**
     * @brief Get the status of the request
     **/
    FrameViewRequestStatusEnum getStatus() const;

    /**
     * @brief Called in requestRender to initialize the status of this object.
     **/
    void initStatus(FrameViewRequestStatusEnum status);

    /**
     * @brief Called on startup of launchRender() function to 
     * determine what next to do.
     * @see FrameViewRequestStatusEnum for what to do given the status.
     **/
    FrameViewRequestStatusEnum notifyRenderStarted();

    /**
     * @brief Called when launchRender() ends when it was rendering a frame.
     * This is only needed if the status returned by notifyRenderStarted() is 
     * eFrameViewRequestStatusNotRendered.
     **/
    void notifyRenderFinished(ActionRetCodeEnum stat);

#if 0
    /**
     * @brief This should be called right after notifyRenderStarted() if the value
     * returned was eFrameViewRequestStatusPending. After this function the launchRender
     * function can just return
     **/
    ActionRetCodeEnum waitForPendingResults();
#endif
    
    /**
     * @brief Get the render mapped mipmap level (i.e: 0 if the node
     * does not support render scale)
     **/
    unsigned int getRenderMappedMipMapLevel() const;
    void setRenderMappedMipMapLevel(unsigned int mipMapLevel) const;

    /**
     * @brief Returns the proxy scale - it is applied combined with the downscaling given
     * by the mipmap level.
     **/
    const RenderScale& getProxyScale() const;

    /**
     * @brief Get the plane to render
     **/
    const ImagePlaneDesc& getPlaneDesc() const;
    void setPlaneDesc(const ImagePlaneDesc& plane);


    /**
     * @brief Return the image plane to render.
     * This is the final image rendered.
     **/
    ImagePtr getImagePlane() const;
    void setImagePlane(const ImagePtr& image);


    /**
     * @brief Get the cache policy for this frame/view
     **/
    CacheAccessModeEnum getCachePolicy() const;
    void setCachePolicy(CacheAccessModeEnum policy);

    /**
     * @brief Return the render clone passed to the ctor
     **/
    EffectInstancePtr getRenderClone() const;


    /**
     * @brief Add a new dependency to this frame/view. This frame/view will not be able to render until all dependencies will be rendered.
     **/
    void addDependency(const RequestPassSharedDataPtr& requestData, const FrameViewRequestPtr& other);

    /**
     * @brief Remove a dependency previously added by addDependency but does not destroy it.
     * This should be called once the "other" frame/view * is done rendering.
     * @returns The number of dependencies left
     **/
    int markDependencyAsRendered(const RequestPassSharedDataPtr& requestData, const FrameViewRequestPtr& other);

    /**
     * @brief Destroy all dependencies that were already rendered
     **/
    void clearRenderedDependencies(const RequestPassSharedDataPtr& requestData);

    /**
     * @brief Get the number of dependencies left to render for this frame/view.
     * If this returns 0, then this frame/view can be rendered.
     **/
    int getNumDependencies(const RequestPassSharedDataPtr& requestData) const;

    /**
     * @brief Add the "other" frame/view as a listener of this frame/view. This means
     * that this frame/view is in the dependencies list of the "other" frame/view.
     **/
    void addListener(const RequestPassSharedDataPtr& requestData, const FrameViewRequestPtr& other);

    /**
     * @brief Returns all frame/view requests that have this frame/view as a dependency.
     **/
    std::list<FrameViewRequestPtr> getListeners(const RequestPassSharedDataPtr& requestData) const;

    /**
     * @brief Same as getListeners().size()
     **/
    std::size_t getNumListeners(const RequestPassSharedDataPtr& requestData) const;

    /**
     * @brief  When true, a subsequent render of this frame/view will not be allowed to read the cache
     * but will still be able to write to the cache. That render should then set this flag to false.
     * By default this flag is false.
     **/
    bool checkIfByPassCacheEnabledAndTurnoff() const;

    /**
     * @brief Retrieves the bounding box of all region of interest requested for this time/view.
     * This allows to ensure the render happens only once.
     **/
    RectD getCurrentRoI() const;

    /**
     * @brief Set the region of interest for this frame view.
     **/
    void setCurrentRoI(const RectD& roi);

    /**
     * @brief Returns the hash for this frame view
     **/
    U64 getHash() const;

    /**
     * @brief Returns the frames needed action results for this frame/view
     **/
    GetFramesNeededResultsPtr getFramesNeededResults() const;

    /**
     * @brief Set the frames needed action results for this frame/view
     **/
    void setFramesNeededResults(const GetFramesNeededResultsPtr& framesNeeded);

    /**
     * @brief Returns the components needed action results for this frame/view
     **/
    GetComponentsResultsPtr getComponentsResults() const;

    /**
     * @brief Set the components needed action results for this frame/view
     **/
    void setComponentsNeededResults(const GetComponentsResultsPtr& comps);

    /**
     * @brief Returns the distortion action results for this frame/view
     **/
    GetDistortionResultsPtr getDistortionResults() const;

    /**
     * @brief Set the distortion action results for this frame/view
     **/
    void setDistortionResults(const GetDistortionResultsPtr& results);

    /**
     * @brief The distorsion stack of upstream distorsion effects
     **/
    Distortion2DStackPtr getDistorsionStack() const;
    void setDistorsionStack(const Distortion2DStackPtr& stack);

private:

    boost::scoped_ptr<FrameViewRequestPrivate> _imp;
    
};



NATRON_NAMESPACE_EXIT;

#endif // NATRON_ENGINE_FRAMEVIEWREQUEST_H
