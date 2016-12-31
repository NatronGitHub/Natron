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

struct FrameViewRequestPrivate
{
    // Protects all data members;
    mutable QMutex lock;

    // Final roi. Each request led from different branches has its roi unioned into the finalRoI
    RectD finalRoi;

    // Effects downstream that requested this frame/view for this node
    std::set<EffectInstanceWPtr> requesters;

    // The required frame/views in input, set on first request
    GetFramesNeededResultsPtr frameViewsNeeded;

    U64 frameViewHash;

    // The RoD of the effect at this frame/view
    GetRegionOfDefinitionResultsPtr rod;

    // Identity data at this frame/view
    IsIdentityResultsPtr identityData;

    // The needed components at this frame/view
    GetComponentsResultsPtr neededComps;

    // The distorsion at this frame/view
    GetDistorsionResultsPtr distorsion;

    // True if cache write is allowed but not cache read
    bool byPassCache;

    // True if the frameViewHash at least is valid
    bool hashValid;


    FrameViewRequestPrivate()
    : lock()
    , finalRoi()
    , requesters()
    , frameViewsNeeded()
    , frameViewHash(0)
    , rod()
    , identityData()
    , neededComps()
    , distorsion()
    , byPassCache()
    , hashValid(false)
    {
        
    }
};

FrameViewRequest::FrameViewRequest(const TreeRenderNodeArgsPtr& renderArgs)
: _imp(new FrameViewRequestPrivate())
{

    assert(renderArgs);
    if (renderArgs->getParentRender()->isByPassCacheEnabled()) {
        _imp->byPassCache = true;
    }
}

FrameViewRequest::~FrameViewRequest()
{

}

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
FrameViewRequest::incrementFramesNeededVisitsCount(const EffectInstancePtr& effectRequesting)
{
    QMutexLocker k(&_imp->lock);
    _imp->requesters.insert(effectRequesting);
}

int
FrameViewRequest::getFramesNeededVisitsCount() const
{
    QMutexLocker k(&_imp->lock);
    return _imp->requesters.size();
}

bool
FrameViewRequest::wasFrameViewRequestedByEffect(const EffectInstancePtr& effectRequesting) const
{
    QMutexLocker k(&_imp->lock);
    std::set<EffectInstanceWPtr>::const_iterator found = _imp->requesters.find(effectRequesting);
    if (found == _imp->requesters.end()) {
        return false;
    }
    return true;
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



bool
FrameViewRequest::getHash(U64* hash) const
{
    QMutexLocker k(&_imp->lock);
    if (!_imp->hashValid) {
        return false;
    }
    *hash = _imp->frameViewHash;
    return true;
}

void
FrameViewRequest::setHash(U64 hash)
{
    QMutexLocker k(&_imp->lock);
    _imp->frameViewHash = hash;
    _imp->hashValid = true;
}


IsIdentityResultsPtr
FrameViewRequest::getIdentityResults() const
{
    QMutexLocker k(&_imp->lock);
    return _imp->identityData;

}

void
FrameViewRequest::setIdentityResults(const IsIdentityResultsPtr& results)
{
    QMutexLocker k(&_imp->lock);
    _imp->identityData = results;
}

GetRegionOfDefinitionResultsPtr
FrameViewRequest::getRegionOfDefinitionResults() const
{
    QMutexLocker k(&_imp->lock);
    return _imp->rod;
}

void
FrameViewRequest::setRegionOfDefinitionResults(const GetRegionOfDefinitionResultsPtr& rod)
{
    QMutexLocker k(&_imp->lock);
    _imp->rod = rod;
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

GetDistorsionResultsPtr
FrameViewRequest::getDistorsionResults() const
{
    QMutexLocker k(&_imp->lock);
    return _imp->distorsion;
}

void
FrameViewRequest::setDistorsionResults(const GetDistorsionResultsPtr& results)
{
    QMutexLocker k(&_imp->lock);
    _imp->distorsion = results;
}

EffectInstancePtr
EffectInstance::resolveInputEffectForFrameNeeded(const int inputNb,
                                                 int* channelForMask)
{
    // Check if the input is a mask
    if (channelForMask) {
        *channelForMask = -1;
    }
    bool inputIsMask = isInputMask(inputNb);

    // If the mask is disabled, don't even bother
    if ( inputIsMask && !isMaskEnabled(inputNb) ) {
        return EffectInstancePtr();
    }

    ImageComponents maskComps;
    NodePtr maskInput;
    int channelForAlphaInput = getMaskChannel(inputNb, &maskComps, &maskInput);

    if (channelForMask) {
        *channelForMask = channelForAlphaInput;
    }

    // No mask or no layer selected for the mask
    if ( inputIsMask && ( (channelForAlphaInput == -1) || (maskComps.getNumComponents() == 0) ) ) {
        return EffectInstancePtr();
    }


    EffectInstancePtr inputEffect;
    inputEffect = getInput(inputNb);
    

    // Redirect the mask input
    if (maskInput) {
        inputEffect = maskInput->getEffectInstance();
    }

    return inputEffect;
}


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

    // The render-scale at which this effect can render
    RenderScale mappedScale;

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

    U64 metadataTimeInvariantHash;

    // The number of times this node has been visited (i.e: the number of times we called renderRoI on it)
    int nVisits;

    // True if hash is valid
    bool timeViewInvariantHashValid;

    bool metadataTimeInvariantHashValid;

    // The support for tiles is local to a render and may change depending on GPU usage or other parameters
    bool tilesSupported;

    // The distort flag may change (e.g Reformat may provide a transform or not depending on a parameter)
    bool canDistort;

    TreeRenderNodeArgsPrivate(const TreeRenderPtr& render, const NodePtr& node)
    : lock()
    , parentRender(render)
    , node(node)
    , inputRenderArgs(node->getMaxInputCount())
    , frames()
    , mappedScale(1.)
    , frameRangeResults()
    , valuesCache(new RenderValuesCache)
    , currentThreadSafety(node->getCurrentRenderThreadSafety())
    , currentOpenglSupport(node->getCurrentOpenGLRenderSupport())
    , currentSequentialPref(node->getCurrentSequentialRenderSupport())
    , timeViewInvariantHash(0)
    , metadataTimeInvariantHash(0)
    , nVisits(0)
    , timeViewInvariantHashValid(false)
    , metadataTimeInvariantHashValid(false)
    , tilesSupported(node->getCurrentSupportTiles())
    , canDistort(node->getCurrentCanDistort())
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
TreeRenderNodeArgs::isAborted() const
{
    // MT-safe: is aborted is MT-safe on the TreeRender class.
    return _imp->parentRender.lock()->isAborted();
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
TreeRenderNodeArgs::getOrCreateFrameViewRequest(TimeValue time, ViewIdx view, FrameViewRequestPtr* request)
{
    
    // Needs to be locked: frame requests may be added spontaneously by the plug-in
    QMutexLocker k(&_imp->lock);

    FrameViewPair p = {time, view};
    NodeFrameViewRequestData::iterator found = _imp->frames.find(p);
    if (found != _imp->frames.end()) {
        *request = found->second;
        return false;
    }

    FrameViewRequestPtr& ret = _imp->frames[p];
    ret.reset(new FrameViewRequest(shared_from_this()));
    *request = ret;
    return true;
}

void
TreeRenderNodeArgs::setMappedRenderScale(const RenderScale& scale)
{
    // MT-safe: never changes throughout the lifetime of the object
    _imp->mappedScale = scale;
}

const RenderScale&
TreeRenderNodeArgs::getMappedRenderScale() const
{
    // MT-safe: never changes throughout the lifetime of the object
    return _imp->mappedScale;
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
TreeRenderNodeArgs::getCurrentDistortSupport() const
{
    // MT-safe: never changes throughout the lifetime of the object
    return _imp->canDistort;
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
TreeRenderNodeArgs::getFrameViewCanonicalRoI(TimeValue time,
                                           ViewIdx view,
                                           RectD* roi) const
{
    FrameViewRequestConstPtr fv = getFrameViewRequest(time, view);

    if (!fv) {
        return false;
    }
    *roi = fv->getCurrentRoI();

    return true;
}

bool
TreeRenderNodeArgs::getFrameViewHash(TimeValue time, ViewIdx view, U64* hash) const
{
    FrameViewPair p = {time,view};
    NodeFrameViewRequestData::const_iterator found = _imp->frames.find(p);
    if (found == _imp->frames.end()) {
        return 0;
    }

    return found->second->getHash(hash);
}

StatusEnum
TreeRenderNodeArgs::roiVisitFunctor(TimeValue time,
                                    ViewIdx view,
                                    const RenderScale& scale,
                                    const RectD& canonicalRenderWindow,
                                    const EffectInstancePtr& caller)
{

    NodePtr node = getNode();
    EffectInstancePtr effect = node->getEffectInstance();

    TreeRenderNodeArgsPtr thisShared = shared_from_this();

    if (node->supportsRenderScaleMaybe() == eSupportsMaybe) {
        /*
         If this flag was not set already that means it probably failed all calls to getRegionOfDefinition.
         We safely fail here
         */
        return eStatusFailed;
    }


    assert(node->supportsRenderScaleMaybe() == eSupportsNo || node->supportsRenderScaleMaybe() == eSupportsYes);

    bool supportsRs = node->supportsRenderScale();

    RenderScale mappedScale = supportsRs ? scale : RenderScale(1.);
    setMappedRenderScale(mappedScale);


    FrameViewPair frameView;
    // Requested time is rounded to an epsilon so we can be sure to find it again in getImage, accounting for precision
    frameView.time = roundImageTimeToEpsilon(time);
    frameView.view = view;

    FrameViewRequestPtr fvRequest;
    {
        bool created = getOrCreateFrameViewRequest(frameView.time, frameView.view, &fvRequest);
        (void)created;

        // This frame was requested from a getFramesNeeded call, increment the number of visits for this frame/view
        if (caller != effect) {
            fvRequest->incrementFramesNeededVisitsCount(caller);
        }
    }



    double par = effect->getAspectRatio(thisShared, -1);
    EffectInstance::ViewInvarianceLevel viewInvariance = effect->isViewInvariant();

    // Check if identity on the render window
    int identityInputNb;
    TimeValue identityTime;
    ViewIdx identityView;
    RectI identityRegionPixel;
    canonicalRenderWindow.toPixelEnclosing(mappedScale, par, &identityRegionPixel);

    {
        IsIdentityResultsPtr results;
        StatusEnum stat = effect->isIdentity_public(true, time, mappedScale, identityRegionPixel, view, thisShared, &results);
        if (stat == eStatusFailed) {
            return eStatusFailed;
        }
        results->getIdentityData(&identityInputNb, &identityTime, &identityView);
    }

    // Upon first request of this frame/view, set the finalRoI, otherwise union it to the existing RoI.

    {
        RectD finalRoi = fvRequest->getCurrentRoI();
        bool finalRoIEmpty = finalRoi.isNull();
        if (!finalRoIEmpty && finalRoi.contains(canonicalRenderWindow)) {
            // Do not recurse if the roi did not add anything new to render
            return eStatusOK;
        }
        if (finalRoIEmpty) {
            fvRequest->setCurrentRoI(canonicalRenderWindow);
        } else {
            finalRoi.merge(canonicalRenderWindow);
            fvRequest->setCurrentRoI(finalRoi);
        }
    }

    if (identityInputNb == -2) {
        assert(identityTime != time || viewInvariance == EffectInstance::eViewInvarianceAllViewsInvariant);
        // be safe in release mode otherwise we hit an infinite recursion
        if ( (identityTime != time) || (viewInvariance == EffectInstance::eViewInvarianceAllViewsInvariant) ) {

            ViewIdx inputView = (view != 0 && viewInvariance == EffectInstance::eViewInvarianceAllViewsInvariant) ? ViewIdx(0) : view;
            StatusEnum stat = roiVisitFunctor(identityTime,
                                              inputView,
                                              scale,
                                              canonicalRenderWindow,
                                              effect);

            return stat;
        }

        //Should fail on the assert above
        return eStatusFailed;
    } else if (identityInputNb != -1) {
        EffectInstancePtr inputEffectIdentity = effect->getInput(identityInputNb);
        if (inputEffectIdentity) {


            NodePtr inputIdentityNode = inputEffectIdentity->getNode();

            TreeRenderNodeArgsPtr inputFrameArgs = getInputRenderArgs(identityInputNb);
            assert(inputFrameArgs);

            StatusEnum stat = inputFrameArgs->roiVisitFunctor(identityTime,
                                                              identityView,
                                                              scale,
                                                              canonicalRenderWindow,
                                                              effect);
            
            return stat;
        }

        // Aalways accept if identity has no input, it will produce a black image in the worst case scenario.
        return eStatusOK;
    }



    // Compute the regions of interest in input for this RoI.
    // The RoI returned is only valid for this canonicalRenderWindow, we don't cache it. Rather we cache on the input the bounding box
    // of all the calls of getRegionsOfInterest that were made down-stream so that the node gets rendered only once.
    RoIMap inputsRoi;
    {
        StatusEnum stat = effect->getRegionsOfInterest_public(time, mappedScale, canonicalRenderWindow, view, thisShared, &inputsRoi);
        if (stat == eStatusFailed) {
            return stat;
        }
    }


    // Get frames needed to recurse upstream.
    FramesNeededMap framesNeeded;
    {
        GetFramesNeededResultsPtr results;
        StatusEnum stat = effect->getFramesNeeded_public(time, view, thisShared, &results);
        if (stat == eStatusFailed) {
            return eStatusFailed;
        }
        results->getFramesNeeded(&framesNeeded);
    }

    // Recurse upstream
    for (FramesNeededMap::const_iterator it = framesNeeded.begin(); it != framesNeeded.end(); ++it) {
        int inputNb = it->first;
        assert(inputNb != -1);

        EffectInstancePtr inputEffect = effect->resolveInputEffectForFrameNeeded(inputNb, 0);
        if (!inputEffect) {
            continue;
        }
        NodePtr inputNode = inputEffect->getNode();
        assert(inputNode);


        TreeRenderNodeArgsPtr inputRenderArgs = getInputRenderArgs(inputNb);

        RoIMap::const_iterator foundInputRoI = inputsRoi.find(inputNb);
        if ( foundInputRoI == inputsRoi.end() ) {
            continue;
        }
        if ( foundInputRoI->second.isInfinite() ) {
            effect->setPersistentMessage( eMessageTypeError, effect->tr("%1 asked for an infinite region of interest upstream.").arg( QString::fromUtf8( node->getScriptName_mt_safe().c_str() ) ).toStdString() );

            return eStatusFailed;
        }
        if ( foundInputRoI->second.isNull() ) {
            continue;
        }
        RectD roi = foundInputRoI->second;

        // For all views requested in input
        for (FrameRangesMap::const_iterator viewIt = it->second.begin(); viewIt != it->second.end(); ++viewIt) {

            // For all ranges in this view
            for (U32 range = 0; range < viewIt->second.size(); ++range) {

                // For all frames in the range
                for (double f = viewIt->second[range].min; f <= viewIt->second[range].max; f += 1.) {

                    StatusEnum stat = inputRenderArgs->roiVisitFunctor(TimeValue(f),
                                                                       viewIt->first,
                                                                       scale,
                                                                       roi,
                                                                       effect);

                    if (stat == eStatusFailed) {
                        return eStatusFailed;
                    }
                }

            }
        }
    }


    return eStatusOK;
} // roiVisitFunctor

struct PreRenderFrame
{
    EffectInstancePtr caller;
    boost::shared_ptr<EffectInstance::GetImageInArgs> renderArgs;
};

struct PreRenderResult
{
    EffectInstance::GetImageOutArgs results;
    int inputNb;
    StatusEnum stat;
};

static
PreRenderResult
preRenderFrameFunctor(const PreRenderFrame& args)
{
    // Notify the node that we're going to render something with the input
    EffectInstance::NotifyInputNRenderingStarted_RAII inputNIsRendering_RAII(args.caller->getNode().get(), args.renderArgs->inputNb);

    PreRenderResult results;
    results.inputNb = args.renderArgs->inputNb;
    bool ok = args.caller->getImagePlanes(*args.renderArgs, &results.results);
    results.stat = ok ? eStatusOK : eStatusFailed;
    return results;
}

RenderRoIRetCode
TreeRenderNodeArgs::preRenderInputImages(TimeValue time,
                                         ViewIdx view,
                                         const RenderScale& scale,
                                         const std::map<int, std::list<ImageComponents> >& neededInputLayers,
                                         InputImagesMap* inputImages)
{
    // For all frames/views needed, recurse on inputs with the appropriate RoI

    NodePtr node = getNode();
    EffectInstancePtr effect = node->getEffectInstance();

    TreeRenderNodeArgsPtr thisShared = shared_from_this();


    // Get frames needed to recurse upstream. This will also compute the hash if needed
    FramesNeededMap framesNeeded;
    {
        GetFramesNeededResultsPtr results;
        StatusEnum stat = effect->getFramesNeeded_public(time, view, thisShared, &results);
        if (stat == eStatusFailed) {
            return eRenderRoIRetCodeFailed;
        }
        results->getFramesNeeded(&framesNeeded);
    }

    std::vector<PreRenderFrame> preRenderFrames;

    for (FramesNeededMap::const_iterator it = framesNeeded.begin(); it != framesNeeded.end(); ++it) {

        int inputNb = it->first;
        EffectInstancePtr inputEffect = effect->resolveInputEffectForFrameNeeded(inputNb, 0);
        if (!inputEffect) {
            continue;
        }
        NodePtr inputNode = inputEffect->getNode();
        assert(inputNode);

        assert(inputNb != -1);


        TreeRenderNodeArgsPtr inputRenderArgs = getInputRenderArgs(inputNb);

        ///There cannot be frames needed without components needed.
        std::map<int, std::list<ImageComponents> >::const_iterator foundCompsNeeded = neededInputLayers.find(inputNb);
        if ( foundCompsNeeded == neededInputLayers.end() ) {
            continue;
        }

        if (foundCompsNeeded->second.empty()) {
            continue;
        }

        {


            // For all views requested in input
            for (FrameRangesMap::const_iterator viewIt = it->second.begin(); viewIt != it->second.end(); ++viewIt) {

                // For all ranges in this view
                for (U32 range = 0; range < viewIt->second.size(); ++range) {

                    int nbFramesPreFetched = 0;

                    // If the range bounds are no integers and the range covers more than 1 frame (min != max),
                    // we have no clue of the interval we should use between the min and max.
                    if (viewIt->second[range].min != viewIt->second[range].max && viewIt->second[range].min != (int)viewIt->second[range].min) {
                        qDebug() << "WARNING:" <<  effect->getScriptName_mt_safe().c_str() << "is requesting a non integer frame range [" << viewIt->second[range].min << ","
                        << viewIt->second[range].max <<"], this is border-line and not specified if this is supported by OpenFX. Natron will render "
                        "this range assuming an interval of 1 between frame times.";
                    }


                    // For all frames in the range
                    for (double f = viewIt->second[range].min; f <= viewIt->second[range].max; f += 1.) {

                        // Sanity check
                        if (nbFramesPreFetched >= NATRON_MAX_FRAMES_NEEDED_PRE_FETCHING) {
                            break;
                        }

                        {
                            boost::shared_ptr<EffectInstance::GetImageInArgs> renderArgs;
                            renderArgs.reset( new EffectInstance::GetImageInArgs);


                            renderArgs->time = TimeValue(f);
                            renderArgs->view = viewIt->first;
                            renderArgs->scale = scale;
                            renderArgs->layers = &foundCompsNeeded->second;
                            renderArgs->inputNb = inputNb;
                            renderArgs->renderArgs = renderArgs;

                            PreRenderFrame preRender;
                            preRender.caller = effect;
                            preRender.renderArgs = renderArgs;
                            preRenderFrames.push_back(preRender);
                        }

                        ++nbFramesPreFetched;

                    } // for all frames

                } // for all ranges
            } // for all views
        } // EffectInstance::NotifyInputNRenderingStarted_RAII
    } // for all inputs


    if (preRenderFrames.empty()) {
        // Nothing to pre-render in input
        return eRenderRoIRetCodeOk;
    }

    // Launch all pre-renders in concurrent threads using the global thread pool.
    // If the current thread is a thread-pool thread, make it also do an iteration instead
    // of waiting for other threads
    bool isThreadPoolThread = isRunningInThreadPoolThread();
    PreRenderFrame currentThreadPreRender;

    if (isThreadPoolThread) {
        currentThreadPreRender = preRenderFrames.back();
        preRenderFrames.pop_back();
    }

    std::vector<PreRenderResult> allResults;
    QFuture<PreRenderResult> future = QtConcurrent::mapped(preRenderFrames, boost::bind(&preRenderFrameFunctor, _1));

    if (isThreadPoolThread) {
        PreRenderResult thisThreadResults = preRenderFrameFunctor(currentThreadPreRender);
        allResults.push_back(thisThreadResults);
    }

    // Wait for other threads to be finished
    future.waitForFinished();

    for (QFuture<PreRenderResult>::const_iterator it = future.begin(); it != future.end(); ++it) {
        allResults.push_back(*it);
    }

    // Check if we are aborted
    if (isAborted()) {
        return eRenderRoIRetCodeAborted;
    }

    for (std::vector<PreRenderResult>::const_iterator it = allResults.begin(); it != allResults.end(); ++it) {

        // If a pre-render failed, fail all the render
        if (it->stat != eStatusOK) {
            if (isAborted()) {
                return eRenderRoIRetCodeAborted;
            }
            return eRenderRoIRetCodeFailed;
        }

        ImageList* inputImagesList = 0;
        // We may not have pre-rendered any input images for this input, make sure we at least have an empty list.
        InputImagesMap::iterator foundInputImages = inputImages->find(it->inputNb);
        if ( foundInputImages == inputImages->end() ) {
            std::pair<InputImagesMap::iterator, bool> ret = inputImages->insert( std::make_pair( it->inputNb, ImageList() ) );
            inputImagesList = &ret.first->second;
            assert(ret.second);
        }

        for (std::map<ImageComponents, ImagePtr>::const_iterator it2 = it->results.imagePlanes.begin(); it2 != it->results.imagePlanes.end(); ++it2) {
            if (inputImagesList && it2->second) {
                inputImagesList->push_back(it2->second);
            }
        }

    }

    return eRenderRoIRetCodeOk;
} // preRenderInputImages


NATRON_NAMESPACE_EXIT;
