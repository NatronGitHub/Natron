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

#ifndef PARALLELRENDERARGS_H
#define PARALLELRENDERARGS_H

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

// This controls how many frames a plug-in can pre-fetch (per view and per input)
// This is to avoid cases where the user would for example use the FrameBlend node with a huge amount of frames so that they
// do not all stick altogether in memory
#define NATRON_MAX_FRAMES_NEEDED_PRE_FETCHING 4


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

    FrameViewRequest(TimeValue time, ViewIdx view, unsigned int mipMapLevel, const ImagePlaneDesc& plane, const TreeRenderNodeArgsPtr& render);

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
     * @brief Get the render mapped mipmap level (i.e: 0 if the node
     * does not support render scale)
     **/
    unsigned int getRenderMappedMipMapLevel() const;
    void setRenderMappedMipMapLevel(unsigned int mipMapLevel) const;

    /**
     * @brief Get the plane to render
     **/
    const ImagePlaneDesc& getPlaneDesc() const;
    void setPlaneDesc(const ImagePlaneDesc& plane);


    /**
     * @brief Return the image plane to render
     **/
    ImagePtr getImagePlane() const;
    void setImagePlane(const ImagePtr& image);

    /**
     * @brief Get the cache policy for this frame/view
     **/
    CacheAccessModeEnum getCachePolicy() const;
    void setCachePolicy(CacheAccessModeEnum policy);

    /**
     * @brief Return the render args passed to the ctor
     **/
    TreeRenderNodeArgsPtr getRenderArgs() const;

    /**
     * @brief When an input image of this request is rendered, we hold a pointer to it in this object
     * until the image of this object's frame/view has been rendered.
     **/
    void appendPreRenderedInputs(int inputNb,
                                 TimeValue time,
                                 ViewIdx view,
                                 const std::map<ImagePlaneDesc, ImagePtr>& planes,
                                 const Distortion2DStackPtr& distortionStack);


    /**
     * @brief Get results that were previously appended in appendPreRenderedInputs
     **/
    void getPreRenderedInputs(int inputNb,
                              TimeValue time,
                              ViewIdx view,
                              const RectI& roi,
                              const std::list<ImagePlaneDesc>& layers,
                              std::map<ImagePlaneDesc, ImagePtr>* planes,
                              std::list<ImagePlaneDesc>* planesLeftToRendered,
                              Distortion2DStackPtr* distortionStack) const;

    /**
     * @brief Clear any data held by a call to appendPreRenderedInputs.
     * If images are backed in the cache they will not be destroyed yet, otherwise they will be deleted.
     **/
    void clearPreRenderedInputs();

    /**
     * @brief Add a new dependency to this frame/view. This frame/view will not be able to render until all dependencies will be rendered.
     **/
    void addDependency(const FrameViewRequestPtr& other);

    /**
     * @brief Remove a dependency previously added by addDependency. This should be called once the "other" frame/view 
     * is done rendering.
     **/
    void removeDependency(const FrameViewRequestPtr& other);

    /**
     * @brief Get the number of dependencies left to render for this frame/view.
     * If this returns 0, then this frame/view can be rendered.
     **/
    int getNumDependencies() const;

    /**
     * @brief Add the "other" frame/view as a listener of this frame/view. This means
     * that this frame/view is in the dependencies list of the "other" frame/view.
     **/
    void addListener(const FrameViewRequestPtr& other);

    /**
     * @brief Returns all frame/view requests that have this frame/view as a dependency.
     **/
    std::list<FrameViewRequestPtr> getListeners() const;

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
    bool getHash(U64* hash) const;

    /**
     * @brief Set the hash for this frame view
     **/
    void setHash(U64 hash);

    /**
     * @brief Returns the identity action results for this frame/view
     **/
    IsIdentityResultsPtr getIdentityResults() const;

    /**
     * @brief Set the identity action results for this frame/view
     **/
    void setIdentityResults(const IsIdentityResultsPtr& results);

    /**
     * @brief Returns the get region of definition action results for this frame/view
     **/
    GetRegionOfDefinitionResultsPtr getRegionOfDefinitionResults() const;

    /**
     * @brief Set the get region of definition action results for this frame/view
     **/
    void setRegionOfDefinitionResults(const GetRegionOfDefinitionResultsPtr& results);

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
    DistortionFunction2DPtr getDistortionResults() const;

    /**
     * @brief Set the distortion action results for this frame/view
     **/
    void setDistortionResults(const DistortionFunction2DPtr& results);

    /**
     * @brief The distorsion stack of upstream distorsion effects
     **/
    Distortion2DStackPtr getDistorsionStack() const;
    void setDistorsionStack(const Distortion2DStackPtr& stack);

private:

    boost::scoped_ptr<FrameViewRequestPrivate> _imp;
    
};

typedef std::map<FrameViewPair, boost::shared_ptr<FrameViewRequest>, FrameView_compare_less> NodeFrameViewRequestData;


/**
 * @brief Render-local arguments given to render a frame by the tree.
 * This is different than the FrameViewRequest because it is not local to a
 * renderRoI call but to the rendering of a whole frame.
 * This class is not thread-safe, because every field are set in the thread
 * that created the TreeRender object and then they are no longer changed.
 **/
struct TreeRenderNodeArgsPrivate;
class TreeRenderNodeArgs : public boost::enable_shared_from_this<TreeRenderNodeArgs>
{
    TreeRenderNodeArgs(const TreeRenderPtr& render, const NodePtr& node);

public:

    static TreeRenderNodeArgsPtr create(const TreeRenderPtr& render, const NodePtr& node);


    virtual ~TreeRenderNodeArgs();

    RenderValuesCachePtr getRenderValuesCache() const;

    /**
     * @brief Returns a pointer to the render object owning this object.
     **/
    TreeRenderPtr getParentRender() const;

    /**
     * @brief Get the frame of the render
     **/
    TimeValue getTime() const;

    /**
     * @brief Get the view of the render
     **/
    ViewIdx getView() const;


    /**
     * @brief Convenience function for getParentRender()->isRenderAborted()
     **/
    bool isRenderAborted() const;

    /**
     * @brief Get the node associated to these render args
     **/
    NodePtr getNode() const;

    /**
     * @brief Set the input node render args at the given input number
     **/
    void setInputRenderArgs(int inputNb, const TreeRenderNodeArgsPtr& inputRenderArgs);

    /**
     * @brief Get the input node render args at the given input number
     **/
    TreeRenderNodeArgsPtr getInputRenderArgs(int inputNb) const;

    /**
     * @brief Get the input node corresponding to the given inputnb.
     * This is guaranteed to remain constant throughout the lifetime of the render
     * unlike the getInput() method on the Node object.
     **/
    NodePtr getInputNode(int inputNb) const;

    /**
     * @brief Returns the node thread-safety for this render
     **/
    RenderSafetyEnum getCurrentRenderSafety() const;

    /**
     * @brief Returns the node thread-safety for this render
     **/
    PluginOpenGLRenderSupport getCurrentRenderOpenGLSupport() const;

    /**
     * @brief Returns the node thread-safety for this render
     **/
    SequentialPreferenceEnum getCurrentRenderSequentialPreference() const;

    /**
     * @brief Returns the node tile support for this render
     **/
    bool getCurrentTilesSupport() const;

    /**
     * @brief Returns the node render scale support for this render
     **/
    bool getCurrentRenderScaleSupport() const;

    /**
     * @brief Returns the node distortion support for this render
     **/
    bool getCurrentDistortSupport() const;

    bool getCurrentTransformationSupport_deprecated() const;

    /**
     * @brief Set the results of the getFrameRange action for this render
     **/
    void setFrameRangeResults(const GetFrameRangeResultsPtr& range);

    /**
     * @brief Get the results of the getFrameRange action for this render
     **/
    GetFrameRangeResultsPtr getFrameRangeResults() const;

    /**
     * @brief Set the results of the getMetadata action for this render
     **/
    void setTimeInvariantMetadataResults(const GetTimeInvariantMetaDatasResultsPtr& metadatas);

    /**
     * @brief Get the results of the getFrameRange action for this render
     **/
    GetTimeInvariantMetaDatasResultsPtr getTimeInvariantMetadataResults() const;

    /**
     * @brief Get the time and view invariant hash
     **/
    bool getTimeViewInvariantHash(U64* hash) const;
    void setTimeViewInvariantHash(U64 hash);


    /**
     * @brief Get the time and view invariant hash used for metadatas
     **/
    void setTimeInvariantMetadataHash(U64 hash);
    bool getTimeInvariantMetadataHash(U64* hash) const;

    /**
     * @brief Convenience function, same as getFrameViewRequest(time,view)->finalRoi
     **/
    bool getFrameViewCanonicalRoI(TimeValue time, ViewIdx view, RectD* roi) const;

    /**
     * @brief Convenience function, same as getFrameViewRequest(time,view)->frameViewHash
     **/
    bool getFrameViewHash(TimeValue time, ViewIdx view, U64* hash) const;

    /**
     * @brief Returns a previously requested frame/view request from requestRender. This contains most actions
     * results for the frame/view as well as the RoI required to render on the effect for this particular frame/view pair.
     * The time passed in parameter should always be rounded for effects that are not continuous.
     **/
    FrameViewRequestPtr getFrameViewRequest(TimeValue time, ViewIdx view) const;

    /**
     * @brief Same as getFrameViewRequest excepts that if it does not exist it will create it.
     * @returns True if it was created, false otherwise
     **/
    bool getOrCreateFrameViewRequest(TimeValue time, ViewIdx view, unsigned int mipMapLevel, const ImagePlaneDesc& plane, FrameViewRequestPtr* request);

    /**
     * @brief Requests a render at the given time/view for the given render window (in canonical coordinates) for this node.
     * This function will recurse upstream on all dependencies and build a list of things to render in order.
     **/
    ActionRetCodeEnum requestRender(TimeValue time,
                                    ViewIdx view,
                                    unsigned int mipMapLevel,
                                    const ImagePlaneDesc& plane,
                                    const RectD & canonicalRenderWindow,
                                    const FrameViewRequestPtr& requester,
                                    FrameViewRequestPtr* createdRequest);
    
    
    
private:

    boost::scoped_ptr<TreeRenderNodeArgsPrivate> _imp;
};

NATRON_NAMESPACE_EXIT;

#endif // PARALLELRENDERARGS_H
