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

#include "ParallelRenderArgs.h"

#include <cassert>
#include <stdexcept>

#include <boost/scoped_ptr.hpp>

#include "Engine/AppInstance.h"
#include "Engine/AbortableRenderInfo.h"
#include "Engine/AppManager.h"
#include "Engine/Settings.h"
#include "Engine/Hash64.h"
#include "Engine/EffectInstance.h"
#include "Engine/Image.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/GPUContextPool.h"
#include "Engine/OSGLContext.h"
#include "Engine/RenderValuesCache.h"
#include "Engine/RotoPaint.h"
#include "Engine/RotoStrokeItem.h"
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

EffectInstancePtr
EffectInstance::resolveInputEffectForFrameNeeded(const int inputNb, const EffectInstance* thisEffect, const InputMatrixMapPtr& reroutesMap)
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
    if (reroutesMap) {
        InputMatrixMap::const_iterator foundReroute = reroutesMap->find(inputNb);
        if ( foundReroute != reroutesMap->end() ) {
            inputEffect = foundReroute->second.newInputEffect->getInput(foundReroute->second.newInputNbToFetchFrom);
        }
    }

    // We still did not get an effect, then get the real input
    if (!inputEffect) {
        inputEffect = thisEffect->getInput(inputNb);
    }

    // Redirect the mask input
    if (maskInput) {
        inputEffect = maskInput->getEffectInstance();
    }

    return inputEffect;
}

//Same as FramesNeededMap but we also get a pointer to EffectInstancePtr as key
typedef std::map<EffectInstancePtr, std::pair</*inputNb*/ int, FrameRangesMap> > PreRenderFrames;

RenderRoIRetCode
EffectInstance::treeRecurseFunctor(bool isRenderFunctor,
                                   const NodePtr& node,
                                   const FramesNeededMap& framesNeeded,
                                   const RoIMap* inputRois, // roi functor specific
                                   const InputMatrixMapPtr& reroutesMap,
                                   bool useTransforms, // roi functor specific
                                   StorageModeEnum renderStorageMode, // if the render of this node is in OpenGL
                                   unsigned int originalMipMapLevel, // roi functor specific
                                   double time,
                                   ViewIdx /*view*/,
                                   const NodePtr& treeRoot,
                                   EffectInstance::InputImagesMap* inputImages, // render functor specific
                                   const EffectInstance::ComponentsNeededMap* neededComps, // render functor specific
                                   bool useScaleOneInputs, // render functor specific
                                   bool byPassCache) // render functor specific
{
    ///For all frames/views needed, call recursively on inputs with the appropriate RoI

    EffectInstancePtr effect = node->getEffectInstance();

    PreRenderFrames framesToRender;


    //Add frames needed to the frames to render
    for (FramesNeededMap::const_iterator it = framesNeeded.begin(); it != framesNeeded.end(); ++it) {
        int inputNb = it->first;
        EffectInstancePtr inputEffect = resolveInputEffectForFrameNeeded(inputNb, effect.get(), reroutesMap);
        if (inputEffect) {

            framesToRender[inputEffect] = std::make_pair(inputNb, it->second);
        }
    }


    for (PreRenderFrames::const_iterator it = framesToRender.begin(); it != framesToRender.end(); ++it) {
        const EffectInstancePtr& inputEffect = it->first;
        NodePtr inputNode = inputEffect->getNode();
        assert(inputNode);

        int inputNb = it->second.first;
        if ( (inputNb == -1) && isRenderFunctor ) {
            /*
               We use inputNb=-1 for the RotoPaint node to recurse the RoI computations on the internal rotopaint tree.
               Note that when we are in the render functor, we do not pre-render the rotopaint tree, instead we wait for
               the getImage() call on the bottom Merge node from the RotoPaint::render function
             */
            continue;
        }

        ImageList* inputImagesList = 0;
        if (isRenderFunctor) {
            EffectInstance::InputImagesMap::iterator foundInputImages = inputImages->find(inputNb);
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
        ParallelRenderArgsPtr frameArgs;
        if (isRenderFunctor) {
            frameArgs = inputEffect->getParallelRenderArgsTLS();
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
            EffectInstance::ComponentsNeededMap::const_iterator foundCompsNeeded = neededComps->find(inputNb);
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

                            StatusEnum stat = EffectInstance::getInputsRoIsFunctor(useTransforms,
                                                                                   f,
                                                                                   viewIt->first,
                                                                                   originalMipMapLevel,
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

                            ///Render the input image with the bit depth of its preference
                            ImageBitDepthEnum inputPrefDepth = inputEffect->getBitDepth(-1);
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

                            assert(frameArgs && frameArgs->request);
                            if (frameArgs && frameArgs->request) {
                                // Use the final roi (merged from all branches leading to that node) for that frame/view pair
                                frameArgs->request->getFrameViewCanonicalRoI(f, viewIt->first, &roi);
                            }

                            if (roi.isNull()) {
                                continue;
                            }

                            RectI inputRoIPixelCoords;
                            const unsigned int upstreamMipMapLevel = useScaleOneInputs ? 0 : originalMipMapLevel;
                            const RenderScale & upstreamScale = useScaleOneInputs ? scaleOne : scale;
                            roi.toPixelEnclosing(upstreamMipMapLevel, inputPar, &inputRoIPixelCoords);

                            std::map<ImageComponents, ImagePtr> inputImgs;
                            {
                                boost::scoped_ptr<EffectInstance::RenderRoIArgs> renderArgs;
                                renderArgs.reset( new EffectInstance::RenderRoIArgs( f, //< time
                                                                                    upstreamScale, //< scale
                                                                                    upstreamMipMapLevel, //< mipmapLevel (redundant with the scale)
                                                                                    viewIt->first, //< view
                                                                                    byPassCache,
                                                                                    inputRoIPixelCoords, //< roi in pixel coordinates
                                                                                    componentsToRender, //< requested comps
                                                                                    inputPrefDepth,
                                                                                    false,
                                                                                    effect,
                                                                                    renderStorageMode /*returnStorage*/,
                                                                                    time /*callerRenderTime*/) );

                                RenderRoIRetCode ret;
                                ret = inputEffect->renderRoI(*renderArgs, &inputImgs); //< requested bitdepth
                                if (ret != eRenderRoIRetCodeOk) {
                                    return ret;
                                }
                            }
                            for (std::map<ImageComponents, ImagePtr>::iterator it3 = inputImgs.begin(); it3 != inputImgs.end(); ++it3) {
                                if (inputImagesList && it3->second) {
                                    inputImagesList->push_back(it3->second);
                                }
                            }

                            if ( effect->aborted() ) {
                                return eRenderRoIRetCodeAborted;
                            }

                            if ( !inputImgs.empty() ) {
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
EffectInstance::getInputsRoIsFunctor(bool useTransforms,
                                     double inArgsTime,
                                     ViewIdx view,
                                     unsigned originalMipMapLevel,
                                     const NodePtr& node,
                                     const NodePtr& /*callerNode*/,
                                     const NodePtr& treeRoot,
                                     const RectD& canonicalRenderWindow)
{

    EffectInstancePtr effect = node->getEffectInstance();

    assert(effect);

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


    ParallelRenderArgsPtr frameArgs = effect->getParallelRenderArgsTLS();
    if (!frameArgs) {
        // TLS not set on the node! The ParallelRenderArgsSetter should have been created before calling this.
        qDebug() << node->getScriptName_mt_safe().c_str() << "getInputsRoIsFunctor called but this node did not have TLS created. This is a bug! Please investigate";
        assert(false);
        return eStatusFailed;
    }

    NodeFrameRequestPtr nodeRequest = frameArgs->request;
    assert(nodeRequest);

    assert(effect->supportsRenderScaleMaybe() == EffectInstance::eSupportsNo ||
           effect->supportsRenderScaleMaybe() == EffectInstance::eSupportsYes);
    bool supportsRs = effect->supportsRenderScale();
    unsigned int mappedLevel = supportsRs ? originalMipMapLevel : 0;


    if ( !nodeRequest->firstRequestMade ) {
        nodeRequest->firstRequestMade = true;
        
        // Setup global data for the node for the whole frame render if the frames map is empty
        nodeRequest->mappedScale.x = nodeRequest->mappedScale.y = Image::getScaleFromMipMapLevel(mappedLevel);
    }

    assert(nodeRequest);

    // Okay now we concentrate on this particular frame/view pair

    FrameViewPair frameView;

    // Requested time is rounded to an epsilon so we can be sure to find it again in getImage, accounting for precision
    frameView.time = roundImageTimeToEpsilon(time);
    frameView.view = view;

    FrameViewRequest* fvRequest = &nodeRequest->frames[frameView];

    double par = effect->getAspectRatio(-1);
    ViewInvarianceLevel viewInvariance = effect->isViewInvariant();

    // Did we already request something on this node from another branch ?
    if ( !fvRequest->requestValid ) {


        // Set up global data specific for this frame view, this is the first time it has been requested so far


        // Get the hash from the thread local storage
        U64 frameViewHash;
        bool gotHash = effect->getRenderHash(time, view, &frameViewHash);
        (void)gotHash;

        ///Check identity
        fvRequest->identityInputNb = -1;
        fvRequest->inputIdentityTime = 0.;
        fvRequest->identityView = view;


        RectI identityRegionPixel;
        canonicalRenderWindow.toPixelEnclosing(mappedLevel, par, &identityRegionPixel);

        if ( (view != 0) && (viewInvariance == eViewInvarianceAllViewsInvariant) ) {
            fvRequest->isIdentity = true;
            fvRequest->identityInputNb = -2;
            fvRequest->inputIdentityTime = time;
        } else {
            try {
                fvRequest->isIdentity = effect->isIdentity_public(true, frameViewHash, time, nodeRequest->mappedScale, identityRegionPixel, view, &fvRequest->inputIdentityTime, &fvRequest->identityView, &fvRequest->identityInputNb);
            } catch (...) {
                return eStatusFailed;
            }
        }

        if (fvRequest->identityInputNb == -2) {
            // If identity on itself, add to the hash cache
            effect->addHashToCache(fvRequest->inputIdentityTime, fvRequest->identityView, frameViewHash);
        }

        /*
           Do NOT call getRegionOfDefinition on the identity time, if the plug-in returns an identity time different from
           this time, we expect that it handles getRegionOfDefinition itself correctly.
         */
        double rodTime = time;
        ViewIdx rodView = view;

        // Get the RoD
        StatusEnum stat = effect->getRegionOfDefinition_public(frameViewHash, rodTime, nodeRequest->mappedScale, rodView, &fvRequest->rod);
        // If failed it should have failed earlier
        if ( (stat == eStatusFailed) && !fvRequest->rod.isNull() ) {
            return stat;
        }


        // Get the frame/views needed for this frame/view.
        // This is cached because we computed the hash in the ParallelRenderArgs constructor before.

        fvRequest->frameViewsNeeded = effect->getFramesNeeded_public(time, view, false, AbortableRenderInfoPtr(), &fvRequest->frameViewHash);

        fvRequest->hashValid = true;
        
        // Concatenate transforms if needed
        if (useTransforms) {
            fvRequest->transforms.reset(new InputMatrixMap);
            effect->tryConcatenateTransforms( time, view, nodeRequest->mappedScale, fvRequest->frameViewHash, fvRequest->transforms.get() );
        }


        fvRequest->requestValid = true;

    } // if (foundFrameView != nodeRequest->frames.end()) {

    assert(fvRequest);


    bool finalRoIEmpty = fvRequest->finalRoi.isNull();
    if (!finalRoIEmpty && fvRequest->finalRoi.contains(canonicalRenderWindow)) {
        // Do not recurse if the roi did not add anything new to render
        return eStatusOK;
    }
    // Upon first request, set the finalRoI, otherwise merge it.
    if (finalRoIEmpty) {
        fvRequest->finalRoi = canonicalRenderWindow;
    } else {
        fvRequest->finalRoi.merge(canonicalRenderWindow);
    }

    if (fvRequest->identityInputNb == -2) {
        assert(fvRequest->inputIdentityTime != time || viewInvariance == eViewInvarianceAllViewsInvariant);
        // be safe in release mode otherwise we hit an infinite recursion
        if ( (fvRequest->inputIdentityTime != time) || (viewInvariance == eViewInvarianceAllViewsInvariant) ) {
            //fvRequest->requests.push_back( std::make_pair( canonicalRenderWindow, FrameViewPerRequestData() ) );

            ViewIdx inputView = (view != 0 && viewInvariance == eViewInvarianceAllViewsInvariant) ? ViewIdx(0) : view;
            StatusEnum stat = getInputsRoIsFunctor(useTransforms,
                                                   fvRequest->inputIdentityTime,
                                                   inputView,
                                                   originalMipMapLevel,
                                                   node,
                                                   node,
                                                   treeRoot,
                                                   canonicalRenderWindow);

            return stat;
        }

        //Should fail on the assert above
        return eStatusFailed;
    } else if (fvRequest->identityInputNb != -1) {
        EffectInstancePtr inputEffectIdentity = effect->getInput(fvRequest->identityInputNb);
        if (inputEffectIdentity) {
            //fvRequest->requests.push_back( std::make_pair( canonicalRenderWindow, FrameViewPerRequestData() ) );

            NodePtr inputIdentityNode = inputEffectIdentity->getNode();
            StatusEnum stat = getInputsRoIsFunctor(useTransforms,
                                                   fvRequest->inputIdentityTime,
                                                   fvRequest->identityView,
                                                   originalMipMapLevel,
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
    effect->getRegionsOfInterest_public(time, nodeRequest->mappedScale, fvRequest->rod, canonicalRenderWindow, view, &inputsRoi);


    // Transform Rois and get the reroutes map
    if (useTransforms) {
        if (fvRequest->transforms) {
            fvRequest->reroutesMap.reset( new ReRoutesMap() );
            transformInputRois( effect, fvRequest->transforms, par, nodeRequest->mappedScale, &inputsRoi, fvRequest->reroutesMap );
        }
    }


    // Append the request
    //fvRequest->requests.push_back( std::make_pair(canonicalRenderWindow, fvPerRequestData) );




    RenderRoIRetCode ret = treeRecurseFunctor(false,
                                              node,
                                              fvRequest->frameViewsNeeded,
                                              &inputsRoi,
                                              fvRequest->transforms,
                                              useTransforms,
                                              eStorageModeRAM /*returnStorage*/,
                                              originalMipMapLevel,
                                              time,
                                              view,
                                              treeRoot,
                                              0,
                                              0,
                                              false,
                                              false);
    if (ret == eRenderRoIRetCodeFailed) {
        return eStatusFailed;
    }

    return eStatusOK;
} // EffectInstance::getInputsRoIsFunctor

StatusEnum
EffectInstance::optimizeRoI(double time,
                            ViewIdx view,
                            unsigned int mipMapLevel,
                            const RectD& renderWindow,
                            const NodePtr& treeRoot)
{
    bool doTransforms = appPTR->getCurrentSettings()->isTransformConcatenationEnabled();
    StatusEnum stat = getInputsRoIsFunctor(doTransforms,
                                           time,
                                           view,
                                           mipMapLevel,
                                           treeRoot,
                                           treeRoot,
                                           treeRoot,
                                           renderWindow);

    if (stat == eStatusFailed) {
        return stat;
    }

    return eStatusOK;
}

static const FrameViewRequest*
getFrameViewRequestInternal(double time, ViewIdx view, bool failIfNotValid, const NodeFrameViewRequestData& frames)
{
    for (NodeFrameViewRequestData::const_iterator it = frames.begin(); it != frames.end(); ++it) {

        // Compare floating times with an epsilon
        if (std::abs(it->first.time - time) < NATRON_IMAGE_TIME_EQUALITY_EPS) {
            if ( (it->first.view == -1) || (it->first.view == view) ) {
                if (it->second.requestValid || !failIfNotValid) {
                    return &it->second;
                }
            }
        }
    }

    return 0;
}

const FrameViewRequest*
NodeFrameRequest::getFrameViewRequest(double time,
                                      ViewIdx view) const
{
    return getFrameViewRequestInternal(time, view, true, frames);
}

void
NodeFrameRequest::setFrameViewHash(double time, ViewIdx view, U64 hash)
{
    FrameViewPair id = {roundImageTimeToEpsilon(time), view};
    FrameViewRequest& fv = frames[id];
    fv.hashValid = true;
    fv.frameViewHash = hash;
}

bool
NodeFrameRequest::getFrameViewCanonicalRoI(double time,
                                           ViewIdx view,
                                           RectD* roi) const
{
    const FrameViewRequest* fv = getFrameViewRequest(time, view);

    if (!fv) {
        return false;
    }
    *roi = fv->finalRoi;

    return true;
}

bool
NodeFrameRequest::getFrameViewHash(double time, ViewIdx view, U64* hash) const
{
    const FrameViewRequest* fv = getFrameViewRequestInternal(time, view, false, frames);

    if (!fv) {
        return false;
    }
    if (!fv->hashValid) {
        return false;
    }
    *hash = fv->frameViewHash;
    return true;
}

struct FindDependenciesNode
{
    // Did we already recurse on this node (i.e: did we call upstream on input nodes)
    bool recursed;

    // How many times did we visit this node
    int visitCounter;

    // A map of the hash for each frame/view pair
    FrameViewHashMap frameViewHash;


    FindDependenciesNode()
    : recursed(false)
    , visitCounter(0)
    , frameViewHash()
    {}
};


typedef std::map<NodePtr,FindDependenciesNode> FindDependenciesMap;

struct ParallelRenderArgsSetterPrivate
{

    ParallelRenderArgsSetter* _publicInterface;

    // A map of the TLS set on each node
    boost::shared_ptr<std::map<NodePtr, ParallelRenderArgsPtr > > argsMap;

    // The node list
    NodesList nodes;
    NodePtr treeRoot;
    double time;
    ViewIdx view;
    OSGLContextWPtr openGLContext, cpuOpenGLContext;

    ParallelRenderArgsSetterPrivate(ParallelRenderArgsSetter* publicInterface)
    : _publicInterface(publicInterface)
    ,argsMap()
    , nodes()
    , treeRoot()
    , time(0)
    , view()
    , openGLContext()
    , cpuOpenGLContext()
    {
        
    }

    void fetchOpenGLContext(const ParallelRenderArgsSetter::CtorArgsPtr& inArgs);

    void getDependenciesRecursive_internal(const ParallelRenderArgsSetter::CtorArgsPtr& inArgs,
                                           const bool doNansHandling,
                                           const OSGLContextPtr& glContext,
                                           const OSGLContextPtr& cpuContext,
                                           const NodePtr& node,
                                           const NodePtr& treeRoot,
                                           double time,
                                           ViewIdx view,
                                           FindDependenciesMap& finalNodes,
                                           U64* nodeHash);


    void setNodeTLSInternal(const ParallelRenderArgsSetter::CtorArgsPtr& inArgs,
                            bool doNansHandling,
                            const NodePtr& node,
                            bool initTLS,
                            int visitsCounter,
                            bool addFrameViewHash,
                            double time,
                            ViewIdx view,
                            U64 hash,
                            const OSGLContextPtr& gpuContext,
                            const OSGLContextPtr& cpuContext);

};


void
ParallelRenderArgsSetterPrivate::setNodeTLSInternal(const ParallelRenderArgsSetter::CtorArgsPtr& inArgs,
                                             bool doNansHandling,
                                             const NodePtr& node,
                                             bool initTLS,
                                             int visitsCounter,
                                             bool addFrameViewHash,
                                             double time,
                                             ViewIdx view,
                                             U64 hash,
                                             const OSGLContextPtr& gpuContext,
                                             const OSGLContextPtr& cpuContext)
{
    EffectInstancePtr effect = node->getEffectInstance();
    assert(effect);

    if (!initTLS) {
        // Find a TLS object that was created in a call to this function before
        ParallelRenderArgsPtr curTLS = effect->getParallelRenderArgsTLS();
        if (!curTLS) {
            // Not found, create it
            initTLS = true;
        } else {
            assert(curTLS->request);
            // Found: add the hash for this frame/view pair and also increase the visitsCount
            if (addFrameViewHash) {
                curTLS->request->setFrameViewHash(time, view, hash);
            }
            curTLS->visitsCount = visitsCounter;

        }
    }

    // Create the TLS object for this frame render
    if (initTLS) {
        RenderSafetyEnum safety = node->getCurrentRenderThreadSafety();
        PluginOpenGLRenderSupport glSupport = node->getCurrentOpenGLRenderSupport();

        EffectInstance::SetParallelRenderTLSArgsPtr tlsArgs(new EffectInstance::SetParallelRenderTLSArgs);
        tlsArgs->time = inArgs->time;
        tlsArgs->view = inArgs->view;
        tlsArgs->isRenderUserInteraction = inArgs->isRenderUserInteraction;
        tlsArgs->isSequential = inArgs->isSequential;
        tlsArgs->treeRoot = inArgs->treeRoot;
        tlsArgs->glContext = gpuContext;
        tlsArgs->cpuGlContext = cpuContext;
        tlsArgs->textureIndex = inArgs->textureIndex;
        tlsArgs->timeline = inArgs->timeline;
        tlsArgs->isAnalysis = inArgs->isAnalysis;
        tlsArgs->currentThreadSafety = safety;
        tlsArgs->currentOpenGLSupport = glSupport;
        tlsArgs->doNanHandling = doNansHandling;
        tlsArgs->draftMode = inArgs->draftMode;
        tlsArgs->stats = inArgs->stats;
        tlsArgs->visitsCount = visitsCounter;
        tlsArgs->parent = _publicInterface->shared_from_this();

        ParallelRenderArgsPtr curTLS = effect->initParallelRenderArgsTLS(tlsArgs);
        assert(curTLS);
        if (addFrameViewHash) {
            curTLS->request->setFrameViewHash(time, view, hash);
        }
    }
}


/**
 * @brief Builds the internal render tree (including this node) and all its dependencies through expressions as well (which
 * also may be recursive).
 * The hash is also computed for each frame/view pair of each node.
 * This function throw exceptions upon error.
 **/
void
ParallelRenderArgsSetterPrivate::getDependenciesRecursive_internal(const ParallelRenderArgsSetter::CtorArgsPtr& inArgs,
                                                                   const bool doNansHandling,
                                                                   const OSGLContextPtr& glContext,
                                                                   const OSGLContextPtr& cpuContext,
                                                                   const NodePtr& node,
                                                                   const NodePtr& treeRoot,
                                                                   double inArgsTime,
                                                                   ViewIdx view,
                                                                   FindDependenciesMap& finalNodes,
                                                                   U64* nodeHash)
{



    // There may be cases where nodes gets added to the finalNodes in getAllExpressionDependenciesRecursive(), but we still
    // want to recurse upstream for them too
    bool foundInMap = false;
    bool alreadyRecursedUpstream = false;
    
    // Sanity check
    if ( !node || !node->isNodeCreated() ) {
        return;
    }

    EffectInstancePtr effect = node->getEffectInstance();
    if (!effect) {
        return;
    }

    // When building the render tree, the actual graph is flattened and groups no longer exist!
    assert(!dynamic_cast<NodeGroup*>(effect.get()));

    double time = inArgsTime;
    {
        int roundedTime = std::floor(time + 0.5);
        if (roundedTime != time && !effect->canRenderContinuously()) {
            time = roundedTime;
        }
    }

    FindDependenciesNode *nodeData = 0;

    // Check if we visited the node already
    {
        FindDependenciesMap::iterator found = finalNodes.find(node);
        if (found != finalNodes.end()) {
            nodeData = &found->second;
            foundInMap = true;
            if (found->second.recursed) {
                // We already called getAllUpstreamNodesRecursiveWithDependencies on its inputs
                // Increment the number of time we visited this node
                ++found->second.visitCounter;
                alreadyRecursedUpstream = true;
            } else {
                // Now we set the recurse flag
                nodeData->recursed = true;
                nodeData->visitCounter = 1;
            }

        }
    }

    if (!nodeData) {
        // Add this node to the set
        nodeData = &finalNodes[node];
        nodeData->recursed = true;
        nodeData->visitCounter = 1;
    }



    // Compute frames-needed, which will also give us the hash for this node.
    // This action will most likely make getValue calls on knobs, so we need to create the TLS object
    // on the effect with at least the render values cache even if we don't have yet other informations.
    U64 hashValue;
    FramesNeededMap framesNeeded = effect->getFramesNeeded_public(time, view, nodeData->visitCounter == 1 /*createTLS*/, inArgs->abortInfo, &hashValue);


    // Now update the TLS with the hash value and initialize data if needed
    setNodeTLSInternal(inArgs, doNansHandling, node, nodeData->visitCounter == 1 /*initTLS*/, nodeData->visitCounter, true /*addFrameViewHash*/, time, view, hashValue, glContext, cpuContext);

    // If the node is within a group, also apply TLS on the group so that knobs have a consistant state throughout the render, even though
    // the group node will not actually render anything.
    {
        NodeGroupPtr isContainerAGroup = toNodeGroup(effect->getNode()->getGroup());
        if (isContainerAGroup) {
            isContainerAGroup->createFrameRenderTLS(inArgs->abortInfo);
            setNodeTLSInternal(inArgs, doNansHandling, isContainerAGroup->getNode(), true /*createTLS*/, 0 /*visitCounter*/, false /*addFrameViewHash*/, time, view, hashValue, glContext, cpuContext);
        }
    }


    // If we already got this node from expressions dependencies, don't do it again
    if (!foundInMap || !alreadyRecursedUpstream) {

        std::set<NodePtr> expressionsDeps;
        effect->getAllExpressionDependenciesRecursive(expressionsDeps);

        // Also add all expression dependencies but mark them as we did not recursed on them yet
        for (std::set<NodePtr>::iterator it = expressionsDeps.begin(); it != expressionsDeps.end(); ++it) {
            FindDependenciesNode n;
            n.recursed = false;
            n.visitCounter = 0;
            std::pair<FindDependenciesMap::iterator,bool> insertOK = finalNodes.insert(std::make_pair(node, n));

            // Set the TLS on the expression dependency
            if (insertOK.second) {
                (*it)->getEffectInstance()->createFrameRenderTLS(inArgs->abortInfo);
                setNodeTLSInternal(inArgs, doNansHandling, *it, true /*createTLS*/, 0 /*visitCounter*/, false /*addFrameViewHash*/, time, view, hashValue, glContext, cpuContext);
            }
        }
    }


    // Recurse on frames needed
    for (FramesNeededMap::const_iterator it = framesNeeded.begin(); it != framesNeeded.end(); ++it) {

        // No need to use transform redirections to compute the hash
        EffectInstancePtr inputEffect = EffectInstance::resolveInputEffectForFrameNeeded(it->first, effect.get(), InputMatrixMapPtr());
        if (!inputEffect) {
            continue;
        }

        // For all views requested in input
        for (FrameRangesMap::const_iterator viewIt = it->second.begin(); viewIt != it->second.end(); ++viewIt) {

            // For all ranges in this view
            for (U32 range = 0; range < viewIt->second.size(); ++range) {

                // For all frames in the range
                for (double f = viewIt->second[range].min; f <= viewIt->second[range].max; f += 1.) {
                    U64 inputHash;
                    getDependenciesRecursive_internal(inArgs, doNansHandling, glContext, cpuContext, inputEffect->getNode(), treeRoot, f, viewIt->first, finalNodes, &inputHash);
                }
            }

        }
    }

    // For the viewer, since it has no hash of its own, do NOT set the frameViewHash.
    ViewerInstance* isViewerInstance = dynamic_cast<ViewerInstance*>(effect.get());
    if (!isViewerInstance) {
        FrameViewPair fv = {time, view};
        nodeData->frameViewHash[fv] = hashValue;
    }


    if (nodeHash) {
        *nodeHash = hashValue;
    }

} // getDependenciesRecursive_internal


void
ParallelRenderArgsSetterPrivate::fetchOpenGLContext(const ParallelRenderArgsSetter::CtorArgsPtr& inArgs)
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

ParallelRenderArgsSetterPtr
ParallelRenderArgsSetter::create(const CtorArgsPtr& inArgs)
{
    ParallelRenderArgsSetterPtr ret(new ParallelRenderArgsSetter());
    ret->init(inArgs);
    return ret;
}

void
ParallelRenderArgsSetter::init(const CtorArgsPtr& inArgs)
{
    assert(inArgs->treeRoot);

    _imp->time = inArgs->time;
    _imp->view = inArgs->view;
    _imp->treeRoot = inArgs->treeRoot;

    _imp->fetchOpenGLContext(inArgs);

    OSGLContextPtr glContext = _imp->openGLContext.lock(), cpuContext = _imp->cpuOpenGLContext.lock();

    bool doNanHandling = appPTR->getCurrentSettings()->isNaNHandlingEnabled();


    // Get dependencies node tree and apply TLS
    FindDependenciesMap dependenciesMap;
    _imp->getDependenciesRecursive_internal(inArgs, doNanHandling, glContext, cpuContext, inArgs->treeRoot, inArgs->treeRoot, inArgs->time, inArgs->view, dependenciesMap, 0);

    for (FindDependenciesMap::iterator it = dependenciesMap.begin(); it != dependenciesMap.end(); ++it) {
        _imp->nodes.push_back(it->first);
    }

}

ParallelRenderArgsSetter::ParallelRenderArgsSetter()
: boost::enable_shared_from_this<ParallelRenderArgsSetter>()
, _imp(new ParallelRenderArgsSetterPrivate(this))
{

}


StatusEnum
ParallelRenderArgsSetter::optimizeRoI(unsigned int mipMapLevel, const RectD& canonicalRoI)
{
    StatusEnum stat = EffectInstance::optimizeRoI(_imp->time, _imp->view, mipMapLevel, canonicalRoI, _imp->treeRoot);
    if (stat != eStatusOK) {
        return stat;
    }
    return stat;
}


ParallelRenderArgsSetter::~ParallelRenderArgsSetter()
{
    // Invalidate the TLS on all nodes that had it set
    for (NodesList::iterator it = _imp->nodes.begin(); it != _imp->nodes.end(); ++it) {
        if ( !(*it) || !(*it)->getEffectInstance() ) {
            continue;
        }
        (*it)->getEffectInstance()->invalidateParallelRenderArgsTLS();
    }

}

ParallelRenderArgs::ParallelRenderArgs()
: time(0)
, timeline()
, request()
, view(0)
, abortInfo()
, treeRoot()
, visitsCount(0)
, stats()
, openGLContext()
, textureIndex(0)
, currentThreadSafety(eRenderSafetyInstanceSafe)
, currentOpenglSupport(ePluginOpenGLRenderSupportNone)
, isRenderResponseToUserInteraction(false)
, isSequentialRender(false)
, isAnalysis(false)
, doNansHandling(true)
, draftMode(false)
, tilesSupported(false)
, activeStrokeLastMovementBboxBitmapCleared(false)
{
}

bool
ParallelRenderArgs::isCurrentFrameRenderNotAbortable() const
{
    AbortableRenderInfoPtr info = abortInfo.lock();

    return isRenderResponseToUserInteraction && ( !info || !info->canAbort() );
}


NATRON_NAMESPACE_EXIT;
