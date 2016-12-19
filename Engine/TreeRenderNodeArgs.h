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

#include "Engine/RectD.h"
#include "Engine/ViewIdx.h"
#include "Engine/EngineFwd.h"


// This controls how many frames a plug-in can pre-fetch (per view and per input)
// This is to avoid cases where the user would for example use the FrameBlend node with a huge amount of frames so that they
// do not all stick altogether in memory
#define NATRON_MAX_FRAMES_NEEDED_PRE_FETCHING 4


// If 2 image times differ by lesser than this epsilon they are assumed the same.
#define NATRON_IMAGE_TIME_EQUALITY_EPS 1e-5
#define NATRON_IMAGE_TIME_EQUALITY_DECIMALS 5

inline double roundImageTimeToEpsilon(double time)
{
    int exp = std::pow(10, NATRON_IMAGE_TIME_EQUALITY_DECIMALS);
    return std::floor(time * exp + 0.5) / exp;
}

NATRON_NAMESPACE_ENTER;

typedef std::map<EffectInstancePtr, RectD> RoIMap; // RoIs are in canonical coordinates
typedef std::map<ViewIdx, std::vector<RangeD> > FrameRangesMap;
typedef std::map<int, FrameRangesMap> FramesNeededMap;

struct InputMatrix
{
    EffectInstancePtr newInputEffect;
    boost::shared_ptr<Transform::Matrix3x3> cat;
    int newInputNbToFetchFrom;
};

typedef std::map<int, InputMatrix> InputMatrixMap;
typedef boost::shared_ptr<InputMatrixMap> InputMatrixMapPtr;

typedef std::map<int, EffectInstancePtr> ReRoutesMap;
typedef boost::shared_ptr<ReRoutesMap> ReRoutesMapPtr;


struct FrameViewPair
{
    double time;
    ViewIdx view;
};


struct FrameView_compare_less
{
    bool operator() (const FrameViewPair & lhs,
                     const FrameViewPair & rhs) const;
};

typedef std::map<FrameViewPair, U64, FrameView_compare_less> FrameViewHashMap;

inline bool findFrameViewHash(double time, ViewIdx view, const FrameViewHashMap& table, U64* hash)
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
 * @brief These are datas related to a single frame/view render of a node
 **/
struct FrameViewRequestPrivate;
class FrameViewRequest
{
public:

    FrameViewRequest();

    ~FrameViewRequest();

    void incrementVisitsCount();

    RectD getCurrentRoI() const;

    void setCurrentRoI(const RectD& roi);

    bool getHash(U64* hash) const;

    void setHash(U64 hash);

    void setFrameRangeResults(const RangeD& range);

    bool getFrameRangeResults(RangeD* range) const;

    bool getIdentityResults(int *identityInputNb, double *identityTime, ViewIdx* identityView) const;

    void setIdentityResults(int identityInputNb, double identityTime, ViewIdx identityView);

    bool getRegionOfDefinitionResults(RectD* rod) const;

    void setRegionOfDefinitionResults(const RectD& rod);

    bool getFramesNeededResults(FramesNeededMap* framesNeeded) const;

    void setFramesNeededResults(const FramesNeededMap& framesNeeded);

    ComponentsNeededResultsPtr getComponentsNeededResults() const;

    void setComponentsNeededResults(const ComponentsNeededResultsPtr& comps);

    DistorsionFunction2DPtr getDistorsionResults() const;

    void setDistorsionResults(const DistorsionFunction2DPtr& results);

private:

    boost::scoped_ptr<FrameViewRequestPrivate> _imp;
    
};

typedef std::map<FrameViewPair, FrameViewRequest, FrameView_compare_less> NodeFrameViewRequestData;


/**
 * @brief Thread-local arguments given to render a frame by the tree.
 * This is different than the RenderArgs because it is not local to a
 * renderRoI call but to the rendering of a whole frame.
 **/
struct TreeRenderNodeArgsPrivate;
class TreeRenderNodeArgs
{
public:

    TreeRenderNodeArgs(const TreeRenderPtr& render, const NodePtr& node);

    ~TreeRenderNodeArgs();

    /**
     * @brief Returns a pointer to the render object owning this object.
     **/
    TreeRenderPtr getParentRender() const;

    /**
     * @brief Get the node associated to these render args
     **/
    NodePtr getNode() const;

    /**
     * @brief Set/get the mapped render-scale for the node, i.e:
     * the scale at which the node wants to render.
     **/
    void setMappedRenderScale(const RenderScale& scale);
    const RenderScale& getMappedRenderScale() const;

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
     * @brief Set hash for the given time/view
     **/
    void setFrameViewHash(double time, ViewIdx view, U64 hash);

    /**
     * @brief Convenience function, same as getFrameViewRequest(time,view)->finalRoi
     **/
    bool getFrameViewCanonicalRoI(double time, ViewIdx view, RectD* roi) const;

    /**
     * @brief Convenience function, same as getFrameViewRequest(time,view)->frameViewHash
     **/
    bool getFrameViewHash(double time, ViewIdx view, U64* hash) const;

    /**
     * @brief Returns a previously requested frame/view request from optimizeRoI. This contains most actions
     * results for the frame/view as well as the RoI required to render on the effect for this particular frame/view pair.
     * The time passed in parameter should always be rounded for effects that are not continuous.
     **/
    const FrameViewRequest* getFrameViewRequest(double time, ViewIdx view) const;

    /**
     * @brief Same as getFrameViewRequest excepts that if it does not exist it will create it
     **/
    FrameViewRequest* getOrCreateFrameViewRequest(double time, ViewIdx view);

private:

    boost::scoped_ptr<TreeRenderNodeArgsPrivate> _imp;
};






NATRON_NAMESPACE_EXIT;

#endif // PARALLELRENDERARGS_H
