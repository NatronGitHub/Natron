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

#include <boost/scoped_ptr.hpp>

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

    // Number of requests to this frame/view for this node
    int nRequests;

    // The required frame/views in input, set on first request
    FramesNeededMap frameViewsNeeded;

    U64 frameViewHash;

    // Set when the first request is made
    RectD rod;

    // Identity data, set on first request
    bool isIdentity;
    int identityInputNb;
    ViewIdx identityView;
    double inputIdentityTime;

    // The needed components
    ComponentsNeededResultsPtr neededComps;

    // The distorsion
    DistorsionFunction2DPtr distorsion;

    // The result of getTimeDomain
    RangeD frameRange;

    // True if the frameViewHash at least is valid
    bool hashValid;

    // True if the frameViewsNeeded is set
    bool framesViewsNeededValid;

    // True if the RoD is valid
    bool rodValid;

    // True if the identity datas are set
    bool identityValid;

    // True if the frame range datas are valid
    bool frameRangeValid;

    
    FrameViewRequestPrivate()
    : finalRoi()
    , nRequests(0)
    , frameViewsNeeded()
    , frameViewHash(0)
    , rod()
    , isIdentity(false)
    , identityInputNb(-1)
    , identityView()
    , inputIdentityTime(0)
    , neededComps()
    , distorsion()
    , frameRange()
    , hashValid(false)
    , framesViewsNeededValid(false)
    , rodValid(false)
    , identityValid(false)
    , frameRangeValid(false)
    {
        
    }
};

FrameViewRequest::FrameViewRequest()
: _imp(new FrameViewRequestPrivate())
{

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
FrameViewRequest::incrementVisitsCount()
{
    QMutexLocker k(&_imp->lock);
    ++_imp->nRequests;
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

void
FrameViewRequest::setFrameRangeResults(const RangeD& range)
{
    QMutexLocker k(&_imp->lock);
    _imp->frameRangeValid = true;
    _imp->frameRange = range;
}

bool
FrameViewRequest::getFrameRangeResults(RangeD* range) const
{
    QMutexLocker k(&_imp->lock);
    if (!_imp->frameRangeValid) {
        return false;
    }
    *range = _imp->frameRange;
    return true;
}

bool
FrameViewRequest::getIdentityResults(int *identityInputNb, double *identityTime, ViewIdx* identityView) const
{
    QMutexLocker k(&_imp->lock);
    if (!_imp->identityValid) {
        return false;
    }
    *identityInputNb = _imp->identityInputNb;
    *identityTime = _imp->inputIdentityTime;
    *identityView = _imp->identityView;
    return true;
}

void
FrameViewRequest::setIdentityResults(int identityInputNb, double identityTime, ViewIdx identityView)
{
    QMutexLocker k(&_imp->lock);
    _imp->identityValid = true;
    _imp->identityInputNb = identityInputNb;
    _imp->inputIdentityTime = identityTime;
    _imp->identityView = identityView;
}

bool
FrameViewRequest::getRegionOfDefinitionResults(RectD* rod) const
{
    QMutexLocker k(&_imp->lock);
    if (!_imp->rodValid) {
        return false;
    }
    *rod = _imp->rod;
    return true;
}

void
FrameViewRequest::setRegionOfDefinitionResults(const RectD& rod)
{
    QMutexLocker k(&_imp->lock);
    _imp->rodValid = true;
    _imp->rod = rod;
}

bool
FrameViewRequest::getFramesNeededResults(FramesNeededMap* framesNeeded) const
{
    QMutexLocker k(&_imp->lock);
    if (!_imp->framesViewsNeededValid) {
        return false;
    }
    *framesNeeded = _imp->frameViewsNeeded;
    return true;

}

void
FrameViewRequest::setFramesNeededResults(const FramesNeededMap& framesNeeded)
{
    QMutexLocker k(&_imp->lock);
    _imp->frameViewsNeeded = framesNeeded;
    _imp->framesViewsNeededValid = true;
}

ComponentsNeededResultsPtr
FrameViewRequest::getComponentsNeededResults() const
{
    QMutexLocker k(&_imp->lock);
    return _imp->neededComps;
}

void
FrameViewRequest::setComponentsNeededResults(const ComponentsNeededResultsPtr& comps)
{
    QMutexLocker k(&_imp->lock);
    _imp->neededComps = comps;
}

DistorsionFunction2DPtr
FrameViewRequest::getDistorsionResults() const
{
    QMutexLocker k(&_imp->lock);
    return _imp->distorsion;
}

void
FrameViewRequest::setDistorsionResults(const DistorsionFunction2DPtr& results)
{
    QMutexLocker k(&_imp->lock);
    _imp->distorsion = results;
}

EffectInstancePtr
EffectInstance::resolveInputEffectForFrameNeeded(const int inputNb, const EffectInstance* thisEffect)
{
    // Check if the input is a mask
    bool inputIsMask = thisEffect->isInputMask(inputNb);

    // If the mask is disabled, don't even bother
    if ( inputIsMask && !thisEffect->isMaskEnabled(inputNb) ) {
        return EffectInstancePtr();
    }

    ImageComponents maskComps;
    NodePtr maskInput;
    int channelForAlphaInput = thisEffect->getMaskChannel(inputNb, &maskComps, &maskInput);

    // No mask or no layer selected for the mask
    if ( inputIsMask && ( (channelForAlphaInput == -1) || (maskComps.getNumComponents() == 0) ) ) {
        return EffectInstancePtr();
    }

    // Redirect for transforms if needed
    EffectInstancePtr inputEffect;
    inputEffect = thisEffect->getInput(inputNb);
    

    // Redirect the mask input
    if (maskInput) {
        inputEffect = maskInput->getEffectInstance();
    }

    return inputEffect;
}

// Same as FramesNeededMap but we also get a pointer to EffectInstancePtr as key
typedef std::map<EffectInstancePtr, std::pair</*inputNb*/ int, FrameRangesMap> > PreRenderFrames;

RenderRoIRetCode
EffectInstance::treeRecurseFunctor(bool isRenderFunctor,
                                   const TreeRenderPtr& render,
                                   const NodePtr& node,
                                   const FramesNeededMap& framesNeeded,
                                   const RoIMap* inputRois, // roi functor specific
                                   unsigned int originalMipMapLevel, // roi functor specific
                                   double time,
                                   ViewIdx /*view*/,
                                   const NodePtr& treeRoot,
                                   InputImagesMap* inputImages, // render functor specific
                                   const ComponentsNeededMap* neededComps, // render functor specific
                                   bool useScaleOneInputs)// render functor specific
{
    // For all frames/views needed, call recursively on inputs with the appropriate RoI

    EffectInstancePtr effect = node->getEffectInstance();

    PreRenderFrames framesToRender;


    //Add frames needed to the frames to render
    for (FramesNeededMap::const_iterator it = framesNeeded.begin(); it != framesNeeded.end(); ++it) {
        int inputNb = it->first;
        EffectInstancePtr inputEffect = resolveInputEffectForFrameNeeded(inputNb, effect.get());
        if (inputEffect) {

            framesToRender[inputEffect] = std::make_pair(inputNb, it->second);
        }
    }


    for (PreRenderFrames::const_iterator it = framesToRender.begin(); it != framesToRender.end(); ++it) {
        const EffectInstancePtr& inputEffect = it->first;
        NodePtr inputNode = inputEffect->getNode();
        assert(inputNode);

        int inputNb = it->second.first;
        assert(inputNb != -1);

        ImageList* inputImagesList = 0;
        if (isRenderFunctor) {
            InputImagesMap::iterator foundInputImages = inputImages->find(inputNb);
            if ( foundInputImages == inputImages->end() ) {
                std::pair<InputImagesMap::iterator, bool> ret = inputImages->insert( std::make_pair( inputNb, ImageList() ) );
                inputImagesList = &ret.first->second;
                assert(ret.second);
            }
        }

        // When we are not in the render functor, the RoI on the input is given by getRegionsOfInterest action.
        // When we are rendering, we ask in input the merge of all getRegionsOfInterest calls that were made on each
        // branch leading to that node.
        RectD roi;
        TreeRenderNodeArgsPtr frameArgs;
        if (isRenderFunctor) {
            assert(render);
            frameArgs = render->getNodeRenderArgs(inputNode);
        }

        if (!isRenderFunctor) {
            assert(inputRois);

            RoIMap::const_iterator foundInputRoI = inputRois->find(inputEffect);
            if ( foundInputRoI == inputRois->end() ) {
                continue;
            }
            if ( foundInputRoI->second.isInfinite() ) {
                effect->setPersistentMessage( eMessageTypeError, tr("%1 asked for an infinite region of interest upstream.").arg( QString::fromUtf8( node->getScriptName_mt_safe().c_str() ) ).toStdString() );

                return eRenderRoIRetCodeFailed;
            }

            if ( foundInputRoI->second.isNull() ) {
                continue;
            }
            roi = foundInputRoI->second;
        }

        ///There cannot be frames needed without components needed.
        const std::vector<ImageComponents>* compsNeeded = 0;

        if (neededComps) {
            ComponentsNeededMap::const_iterator foundCompsNeeded = neededComps->find(inputNb);
            if ( foundCompsNeeded == neededComps->end() ) {
                continue;
            } else {
                compsNeeded = &foundCompsNeeded->second;
            }
        }


        const double inputPar = inputEffect->getAspectRatio(-1);


        {
            ///Notify the node that we're going to render something with the input
            boost::shared_ptr<EffectInstance::NotifyInputNRenderingStarted_RAII> inputNIsRendering_RAII;
            if (isRenderFunctor) {
                assert(it->second.first != -1); //< see getInputNumber
                inputNIsRendering_RAII.reset( new EffectInstance::NotifyInputNRenderingStarted_RAII(node.get(), inputNb) );
            }

            // For all views requested in input
            for (FrameRangesMap::const_iterator viewIt = it->second.second.begin(); viewIt != it->second.second.end(); ++viewIt) {

                // For all ranges in this view
                for (U32 range = 0; range < viewIt->second.size(); ++range) {

                    int nbFramesPreFetched = 0;

                    if (isRenderFunctor) {
                        // If the range bounds are no integers and the range covers more than 1 frame (min != max),
                        // we have no clue of the interval we should use between the min and max.
                        if (viewIt->second[range].min != viewIt->second[range].max && viewIt->second[range].min != (int)viewIt->second[range].min) {
                            qDebug() << "WARNING:" <<  effect->getScriptName_mt_safe().c_str() << "is requesting a non integer frame range [" << viewIt->second[range].min << ","
                            << viewIt->second[range].max <<"], this is border-line and not specified if this is supported by OpenFX. Natron will render "
                            "this range assuming an interval of 1 between frame times.";
                        }
                    }
                    
                    // For all frames in the range
                    for (double f = viewIt->second[range].min; f <= viewIt->second[range].max; f += 1.) {
                        if (!isRenderFunctor) {

                            StatusEnum stat = EffectInstance::getInputsRoIsFunctor(f,
                                                                                   viewIt->first,
                                                                                   originalMipMapLevel,
                                                                                   render,
                                                                                   frameArgs,
                                                                                   inputNode,
                                                                                   node,
                                                                                   treeRoot,
                                                                                   roi);

                            if (stat == eStatusFailed) {
                                return eRenderRoIRetCodeFailed;
                            }

                            ///Do not count frames pre-fetched in RoI functor mode, it is harmless and may
                            ///limit calculations that will be done later on anyway.
                        } else {
                            
                            // Sanity check
                            if (nbFramesPreFetched >= NATRON_MAX_FRAMES_NEEDED_PRE_FETCHING) {
                                break;
                            }

                            RenderScale scaleOne(1.);
                            RenderScale scale( Image::getScaleFromMipMapLevel(originalMipMapLevel) );


                            std::list<ImageComponents> componentsToRender;

                            assert(compsNeeded);
                            if (!compsNeeded) {
                                continue;
                            }
                            for (U32 k = 0; k < compsNeeded->size(); ++k) {
                                if (compsNeeded->at(k).getNumComponents() > 0) {
                                    componentsToRender.push_back( compsNeeded->at(k) );
                                }
                            }
                            if ( componentsToRender.empty() ) {
                                continue;
                            }

                            assert(frameArgs);
                            if (frameArgs) {
                                // Use the final roi (merged from all branches leading to that node) for that frame/view pair
                                frameArgs->getFrameViewCanonicalRoI(f, viewIt->first, &roi);
                            }

                            if (roi.isNull()) {
                                continue;
                            }

                            RectI inputRoIPixelCoords;
                            const unsigned int upstreamMipMapLevel = useScaleOneInputs ? 0 : originalMipMapLevel;
                            const RenderScale & upstreamScale = useScaleOneInputs ? scaleOne : scale;
                            roi.toPixelEnclosing(upstreamMipMapLevel, inputPar, &inputRoIPixelCoords);

                            RenderRoIResults inputResults;
                            {
                                boost::scoped_ptr<EffectInstance::RenderRoIArgs> renderArgs;
                                renderArgs.reset( new EffectInstance::RenderRoIArgs( f, //< time
                                                                                    upstreamScale, //< scale
                                                                                    upstreamMipMapLevel, //< mipmapLevel (redundant with the scale)
                                                                                    viewIt->first, //< view
                                                                                    inputRoIPixelCoords, //< roi in pixel coordinates
                                                                                    componentsToRender, //< requested comps
                                                                                    false,
                                                                                    effect,
                                                                                    inputNb,
                                                                                    time /*callerRenderTime*/) );

                                RenderRoIRetCode ret;
                                ret = inputEffect->renderRoI(*renderArgs, &inputResults); //< requested bitdepth
                                if (ret != eRenderRoIRetCodeOk) {
                                    return ret;
                                }
                            }
                            for (std::map<ImageComponents, ImagePtr>::iterator it3 = inputResults.outputPlanes.begin(); it3 != inputResults.outputPlanes.end(); ++it3) {
                                if (inputImagesList && it3->second) {
                                    inputImagesList->push_back(it3->second);
                                }
                            }

                            if ( effect->aborted() ) {
                                return eRenderRoIRetCodeAborted;
                            }

                            if ( !inputResults.outputPlanes.empty() ) {
                                ++nbFramesPreFetched;
                            }
                        } // if (!isRenderFunctor) {
                    } // for all frames
                    
                } // for all ranges
            } // for all views
        } // EffectInstance::NotifyInputNRenderingStarted_RAII
    } // for all inputs
    return eRenderRoIRetCodeOk;
} // EffectInstance::treeRecurseFunctor

StatusEnum
EffectInstance::getInputsRoIsFunctor(double inArgsTime,
                                     ViewIdx view,
                                     unsigned originalMipMapLevel,
                                     const TreeRenderPtr& render,
                                     const TreeRenderNodeArgsPtr& frameArgs,
                                     const NodePtr& node,
                                     const NodePtr& /*callerNode*/,
                                     const NodePtr& treeRoot,
                                     const RectD& canonicalRenderWindow)
{

    if (!node || !frameArgs) {
        return eStatusFailed;
    }
    EffectInstancePtr effect = node->getEffectInstance();



    // Round time to nearest integer if the effect is not continuous

    double time = inArgsTime;
    {
        int roundedTime = std::floor(time + 0.5);
        if (roundedTime != time && !effect->canRenderContinuously()) {
            time = roundedTime;
        }
    }

    if (effect->supportsRenderScaleMaybe() == EffectInstance::eSupportsMaybe) {
        /*
           If this flag was not set already that means it probably failed all calls to getRegionOfDefinition.
           We safely fail here
         */
        return eStatusFailed;
    }


    assert(effect->supportsRenderScaleMaybe() == EffectInstance::eSupportsNo || effect->supportsRenderScaleMaybe() == EffectInstance::eSupportsYes);
    bool supportsRs = effect->supportsRenderScale();

    const unsigned int mappedLevel = supportsRs ? originalMipMapLevel : 0;

    RenderScale mappedScale = RenderScale(Image::getScaleFromMipMapLevel(mappedLevel));
    frameArgs->setMappedRenderScale(mappedScale);


    FrameViewPair frameView;
    // Requested time is rounded to an epsilon so we can be sure to find it again in getImage, accounting for precision
    frameView.time = roundImageTimeToEpsilon(time);
    frameView.view = view;

    FrameViewRequest* fvRequest = frameArgs->getOrCreateFrameViewRequest(frameView.time, frameView.view);

    // We should already have the hash by now because getFramesNeeded was already called in the constructor of TreeRender
    U64 frameViewHash;
    bool gotHash = fvRequest->getHash(&frameViewHash);
    assert(gotHash);
    (void)gotHash;

    // Increment the number of visits for this frame/view
    fvRequest->incrementVisitsCount();

    double par = effect->getAspectRatio(-1);
    ViewInvarianceLevel viewInvariance = effect->isViewInvariant();

    // Check if identity on the render window
    int identityInputNb;
    double identityTime;
    ViewIdx identityView;
    RectI identityRegionPixel;
    canonicalRenderWindow.toPixelEnclosing(mappedLevel, par, &identityRegionPixel);
    bool isIdentity = effect->isIdentity_public(true, time, mappedScale, identityRegionPixel, view, frameArgs, &identityTime, &identityView, &identityInputNb);
    (void)isIdentity;

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
        assert(identityTime != time || viewInvariance == eViewInvarianceAllViewsInvariant);
        // be safe in release mode otherwise we hit an infinite recursion
        if ( (identityTime != time) || (viewInvariance == eViewInvarianceAllViewsInvariant) ) {
            
            ViewIdx inputView = (view != 0 && viewInvariance == eViewInvarianceAllViewsInvariant) ? ViewIdx(0) : view;
            StatusEnum stat = getInputsRoIsFunctor(identityTime,
                                                   inputView,
                                                   originalMipMapLevel,
                                                   render,
                                                   frameArgs,
                                                   node,
                                                   node,
                                                   treeRoot,
                                                   canonicalRenderWindow);

            return stat;
        }

        //Should fail on the assert above
        return eStatusFailed;
    } else if (identityInputNb != -1) {
        EffectInstancePtr inputEffectIdentity = effect->getInput(identityInputNb);
        if (inputEffectIdentity) {

            
            NodePtr inputIdentityNode = inputEffectIdentity->getNode();

            TreeRenderNodeArgsPtr inputFrameArgs = render->getNodeRenderArgs(inputIdentityNode);

            StatusEnum stat = getInputsRoIsFunctor(identityTime,
                                                   identityView,
                                                   originalMipMapLevel,
                                                   render,
                                                   inputFrameArgs,
                                                   inputIdentityNode,
                                                   node,
                                                   treeRoot,
                                                   canonicalRenderWindow);

            return stat;
        }

        // Aalways accept if identity has no input, it will produce a black image in the worst case scenario.
        return eStatusOK;
    }


    
    // Compute the regions of interest in input for this RoI.
    // The RoI returned is only valid for this canonicalRenderWindow, we don't cache it. Rather we cache on the input the bounding box
    // of all the calls of getRegionsOfInterest that were made down-stream so that the node gets rendered only once.
    RoIMap inputsRoi;
    effect->getRegionsOfInterest_public(time, mappedScale, canonicalRenderWindow, view, frameArgs, &inputsRoi);


    // Get frames needed to recurse upstream
    FramesNeededMap framesNeeded = effect->getFramesNeeded_public(time, view, frameArgs, 0);

    RenderRoIRetCode ret = treeRecurseFunctor(false,
                                              render,
                                              node,
                                              framesNeeded,
                                              &inputsRoi,
                                              originalMipMapLevel,
                                              time,
                                              view,
                                              treeRoot,
                                              0,
                                              0,
                                              false);
    if (ret == eRenderRoIRetCodeFailed) {
        return eStatusFailed;
    }

    return eStatusOK;
} // EffectInstance::getInputsRoIsFunctor




struct TreeRenderNodeArgsPrivate
{
    // A parent to the render holding this struct
    TreeRenderWPtr parentRender;

    // The node to which these args correspond to
    NodePtr node;

    // Contains data for all frame/view pair that are going to be computed
    // for this frame/view pair with the overall RoI to avoid rendering several times with this node.
    NodeFrameViewRequestData frames;

    // The render-scale at which this effect can render
    RenderScale mappedScale;

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

    // The number of times this node has been visited (i.e: the number of times we called renderRoI on it)
    int nVisits;

    // The support for tiles is local to a render and may change depending on GPU usage or other parameters
    bool tilesSupported;

    // The distort flag may change (e.g Reformat may provide a transform or not depending on a parameter)
    bool canDistort;

    TreeRenderNodeArgsPrivate(const TreeRenderPtr& render, const NodePtr& node)
    : parentRender(render)
    , node(node)
    , frames()
    , mappedScale(1.)
    , valuesCache(new RenderValuesCache)
    , currentThreadSafety(node->getCurrentRenderThreadSafety())
    , currentOpenglSupport(node->getCurrentOpenGLRenderSupport())
    , currentSequentialPref(node->getCurrentSequentialRenderSupport())
    , nVisits(0)
    , tilesSupported(node->getCurrentSupportTiles())
    , canDistort(node->getCurrentCanDistort())
    {


        // Create a copy of the roto item if needed
        RotoDrawableItemPtr attachedRotoItem = node->getOriginalAttachedItem();
        if (attachedRotoItem && attachedRotoItem->isRenderCloneNeeded()) {
            attachedRotoItem->getOrCreateCachedDrawable(render);
        }


        {
#pragma message WARN("Meta datas should be cached like all other action results in the global cache")
            // Cache the meta-datas now
            NodeMetadata metadata;
            node->getEffectInstance()->copyMetaData(&metadata);
            valuesCache->setCachedNodeMetadatas(metadata);
        }
        {
            // Also cache all inputs
            for (int i = 0; i < node->getMaxInputCount(); ++i) {
                valuesCache->setCachedInput(i, node->getInput(i));
            }
            
        }

    }

};


TreeRenderNodeArgs::TreeRenderNodeArgs(const TreeRenderPtr& render, const NodePtr& node)
: _imp(new TreeRenderNodeArgsPrivate(render, node))
{
    
}

TreeRenderNodeArgs::~TreeRenderNodeArgs()
{

}

TreeRenderPtr
TreeRenderNodeArgs::getParentRender() const
{
    return _imp->parentRender.lock();
}

bool
TreeRenderNodeArgs::isAborted() const
{
    return _imp->parentRender.lock()->isAborted();
}

NodePtr
TreeRenderNodeArgs::getNode() const
{
    return _imp->node;
}

const FrameViewRequest*
TreeRenderNodeArgs::getFrameViewRequest(double time,
                                      ViewIdx view) const
{
    FrameViewPair p = {time,view};
    NodeFrameViewRequestData::const_iterator found = _imp->frames.find(p);
    if (found == _imp->frames.end()) {
        return 0;
    }
    return &found->second;
}

FrameViewRequest*
TreeRenderNodeArgs::getOrCreateFrameViewRequest(double time, ViewIdx view)
{
    FrameViewPair p = {time, view};
    return &_imp->frames[p];
}

void
TreeRenderNodeArgs::setMappedRenderScale(const RenderScale& scale)
{
    _imp->mappedScale = scale;
}

const RenderScale&
TreeRenderNodeArgs::getMappedRenderScale() const
{
    return _imp->mappedScale;
}

RenderSafetyEnum
TreeRenderNodeArgs::getCurrentRenderSafety() const
{
    return _imp->currentThreadSafety;
}


PluginOpenGLRenderSupport
TreeRenderNodeArgs::getCurrentRenderOpenGLSupport() const
{
    return _imp->currentOpenglSupport;
}

SequentialPreferenceEnum
TreeRenderNodeArgs::getCurrentRenderSequentialPreference() const
{
    return _imp->currentSequentialPref;
}

void
TreeRenderNodeArgs::setFrameViewHash(double time, ViewIdx view, U64 hash)
{
    FrameViewPair id = {roundImageTimeToEpsilon(time), view};
    FrameViewRequest& fv = _imp->frames[id];
    fv.setHash(hash);
}

bool
TreeRenderNodeArgs::getFrameViewCanonicalRoI(double time,
                                           ViewIdx view,
                                           RectD* roi) const
{
    const FrameViewRequest* fv = getFrameViewRequest(time, view);

    if (!fv) {
        return false;
    }
    *roi = fv->getCurrentRoI();

    return true;
}

bool
TreeRenderNodeArgs::getFrameViewHash(double time, ViewIdx view, U64* hash) const
{
    FrameViewPair p = {time,view};
    NodeFrameViewRequestData::const_iterator found = _imp->frames.find(p);
    if (found == _imp->frames.end()) {
        return 0;
    }

    return found->second.getHash(hash);
}



NATRON_NAMESPACE_EXIT;
