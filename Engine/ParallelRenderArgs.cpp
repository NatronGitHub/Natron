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
#include "Engine/RotoContext.h"
#include "Engine/RotoPaint.h"
#include "Engine/RotoStrokeItem.h"
#include "Engine/ViewIdx.h"

NATRON_NAMESPACE_ENTER;

static EffectInstancePtr resolveInputEffectForFrameNeeded(const int inputNb, const EffectInstancePtr& thisEffect, const InputMatrixMapPtr& reroutesMap)
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

EffectInstance::RenderRoIRetCode
EffectInstance::treeRecurseFunctor(bool isRenderFunctor,
                                   const NodePtr& node,
                                   const FramesNeededMap& framesNeeded,
                                   const RoIMap& inputRois,
                                   const InputMatrixMapPtr& reroutesMap,
                                   bool useTransforms, // roi functor specific
                                   StorageModeEnum renderStorageMode, // if the render of this node is in OpenGL
                                   unsigned int originalMipMapLevel, // roi functor specific
                                   double time,
                                   ViewIdx /*view*/,
                                   const NodePtr& treeRoot,
                                   FrameRequestMap* requests,  // roi functor specific
                                   FrameViewRequest* frameViewRequestData, // roi functor specific
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
        EffectInstancePtr inputEffect = resolveInputEffectForFrameNeeded(inputNb, effect, reroutesMap);
        if (inputEffect) {

            // If this input is frame varying, the node is also frame varying
            if (!frameViewRequestData->globalData.isFrameVaryingRecursive && inputEffect->isFrameVaryingOrAnimated()) {
                frameViewRequestData->globalData.isFrameVaryingRecursive = true;
            }

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

        ///What region are we interested in for this input effect ? (This is in Canonical coords)
        RectD roi;
        bool roiIsInRequestPass = false;
        ParallelRenderArgsPtr frameArgs;
        if (isRenderFunctor) {
            frameArgs = inputEffect->getParallelRenderArgsTLS();
            if (frameArgs && frameArgs->request) {
                roiIsInRequestPass = true;
            }
        }

        if (!roiIsInRequestPass) {
            RoIMap::const_iterator foundInputRoI = inputRois.find(inputEffect);
            if ( foundInputRoI == inputRois.end() ) {
                continue;
            }
            if ( foundInputRoI->second.isInfinite() ) {
                effect->setPersistentMessage( eMessageTypeError, tr("%1 asked for an infinite region of interest upstream.").arg( QString::fromUtf8( node->getScriptName_mt_safe().c_str() ) ).toStdString() );

                return EffectInstance::eRenderRoIRetCodeFailed;
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


                    // if the range bounds are not ints, the fetched images will probably anywhere within this range - no need to pre-render
                    bool isIntegerFrame = (viewIt->second[range].min == (int)viewIt->second[range].min) &&
                    ( viewIt->second[range].max == (int)viewIt->second[range].max);

                    if (!isIntegerFrame) {
                        continue;
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
                                                                                   roi,
                                                                                   *requests);

                            if (stat == eStatusFailed) {
                                return EffectInstance::eRenderRoIRetCodeFailed;
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

                            if (roiIsInRequestPass) {
                                frameArgs->request->getFrameViewCanonicalRoI(f, viewIt->first, &roi);
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
                                                                                    RectD(), // < did we precompute any RoD to speed-up the call ?
                                                                                    componentsToRender, //< requested comps
                                                                                    inputPrefDepth,
                                                                                    false,
                                                                                    effect,
                                                                                    renderStorageMode /*returnStorage*/,
                                                                                    time /*callerRenderTime*/) );

                                EffectInstance::RenderRoIRetCode ret;
                                ret = inputEffect->renderRoI(*renderArgs, &inputImgs); //< requested bitdepth
                                if (ret != EffectInstance::eRenderRoIRetCodeOk) {
                                    return ret;
                                }
                            }
                            for (std::map<ImageComponents, ImagePtr>::iterator it3 = inputImgs.begin(); it3 != inputImgs.end(); ++it3) {
                                if (inputImagesList && it3->second) {
                                    inputImagesList->push_back(it3->second);
                                }
                            }

                            if ( effect->aborted() ) {
                                return EffectInstance::eRenderRoIRetCodeAborted;
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
    return EffectInstance::eRenderRoIRetCodeOk;
} // EffectInstance::treeRecurseFunctor

StatusEnum
EffectInstance::getInputsRoIsFunctor(bool useTransforms,
                                     double time,
                                     ViewIdx view,
                                     unsigned originalMipMapLevel,
                                     const NodePtr& node,
                                     const NodePtr& /*callerNode*/,
                                     const NodePtr& treeRoot,
                                     const RectD& canonicalRenderWindow,
                                     FrameRequestMap& requests)
{
    NodeFrameRequestPtr nodeRequest;
    EffectInstancePtr effect = node->getEffectInstance();

    assert(effect);

    if (effect->supportsRenderScaleMaybe() == EffectInstance::eSupportsMaybe) {
        /*
           If this flag was not set already that means it probably failed all calls to getRegionOfDefinition.
           We safely fail here
         */
        return eStatusFailed;
    }
    assert(effect->supportsRenderScaleMaybe() == EffectInstance::eSupportsNo ||
           effect->supportsRenderScaleMaybe() == EffectInstance::eSupportsYes);
    bool supportsRs = effect->supportsRenderScale();
    unsigned int mappedLevel = supportsRs ? originalMipMapLevel : 0;
    FrameRequestMap::iterator foundNode = requests.find(node);
    if ( foundNode != requests.end() ) {
        nodeRequest = foundNode->second;
    } else {
        ///Setup global data for the node for the whole frame render

        NodeFrameRequestPtr tmp(new NodeFrameRequest);
        tmp->mappedScale.x = tmp->mappedScale.y = Image::getScaleFromMipMapLevel(mappedLevel);

        std::pair<FrameRequestMap::iterator, bool> ret = requests.insert( std::make_pair(node, tmp) );
        assert(ret.second);
        nodeRequest = ret.first->second;
    }
    assert(nodeRequest);

    // Okay now we concentrate on this particular frame/view pair

    FrameViewPair frameView;
    frameView.time = time;
    frameView.view = view;

    FrameViewRequest* fvRequest = 0;
    NodeFrameViewRequestData::iterator foundFrameView = nodeRequest->frames.find(frameView);
    double par = effect->getAspectRatio(-1);
    ViewInvarianceLevel viewInvariance = effect->isViewInvariant();

    if ( foundFrameView != nodeRequest->frames.end() ) {
        fvRequest = &foundFrameView->second;
    } else {
        ///Set up global data specific for this frame view, this is the first time it has been requested so far


        fvRequest = &nodeRequest->frames[frameView];

        // Get the hash from the thread local storage
        U64 frameViewHash = effect->getRenderHash(time, view);

        ///Check identity
        fvRequest->globalData.identityInputNb = -1;
        fvRequest->globalData.inputIdentityTime = 0.;
        fvRequest->globalData.identityView = view;


        RectI identityRegionPixel;
        canonicalRenderWindow.toPixelEnclosing(mappedLevel, par, &identityRegionPixel);

        if ( (view != 0) && (viewInvariance == eViewInvarianceAllViewsInvariant) ) {
            fvRequest->globalData.isIdentity = true;
            fvRequest->globalData.identityInputNb = -2;
            fvRequest->globalData.inputIdentityTime = time;
        } else {
            try {
                // Pass a null hash because we cannot know the hash as of yet since we did not recurse on inputs already, meaning the action will not be cached.
                fvRequest->globalData.isIdentity = effect->isIdentity_public(true, frameViewHash, time, nodeRequest->mappedScale, identityRegionPixel, view, &fvRequest->globalData.inputIdentityTime, &fvRequest->globalData.identityView, &fvRequest->globalData.identityInputNb);
            } catch (...) {
                return eStatusFailed;
            }
        }

        /*
           Do NOT call getRegionOfDefinition on the identity time, if the plug-in returns an identity time different from
           this time, we expect that it handles getRegionOfDefinition itself correctly.
         */
        double rodTime = time; //fvRequest->globalData.isIdentity ? fvRequest->globalData.inputIdentityTime : time;
        ViewIdx rodView = view; //fvRequest->globalData.isIdentity ? fvRequest->globalData.identityView : view;

        // Get the RoD
        StatusEnum stat = effect->getRegionOfDefinition_public(frameViewHash, rodTime, nodeRequest->mappedScale, rodView, &fvRequest->globalData.rod, &fvRequest->globalData.isProjectFormat);
        // If failed it should have failed earlier
        if ( (stat == eStatusFailed) && !fvRequest->globalData.rod.isNull() ) {
            return stat;
        }


        // Concatenate transforms if needed
        if (useTransforms) {
            fvRequest->globalData.transforms.reset(new InputMatrixMap);
            effect->tryConcatenateTransforms( time, view, nodeRequest->mappedScale, fvRequest->globalData.transforms.get() );
        }

        // Get the frame/views needed for this frame/view
        // Pass a null hash because we cannot know the hash as of yet since we did not recurse on inputs already, meaning the action will not be cached.
        fvRequest->globalData.frameViewsNeeded = effect->getFramesNeeded_public(frameViewHash, time, view);
    } // if (foundFrameView != nodeRequest->frames.end()) {

    assert(fvRequest);

    if (fvRequest->globalData.identityInputNb == -2) {
        assert(fvRequest->globalData.inputIdentityTime != time || viewInvariance == eViewInvarianceAllViewsInvariant);
        // be safe in release mode otherwise we hit an infinite recursion
        if ( (fvRequest->globalData.inputIdentityTime != time) || (viewInvariance == eViewInvarianceAllViewsInvariant) ) {
            fvRequest->requests.push_back( std::make_pair( canonicalRenderWindow, FrameViewPerRequestData() ) );

            ViewIdx inputView = (view != 0 && viewInvariance == eViewInvarianceAllViewsInvariant) ? ViewIdx(0) : view;
            StatusEnum stat = getInputsRoIsFunctor(useTransforms,
                                                   fvRequest->globalData.inputIdentityTime,
                                                   inputView,
                                                   originalMipMapLevel,
                                                   node,
                                                   node,
                                                   treeRoot,
                                                   canonicalRenderWindow,
                                                   requests);

            return stat;
        }

        //Should fail on the assert above
        return eStatusFailed;
    } else if (fvRequest->globalData.identityInputNb != -1) {
        EffectInstancePtr inputEffectIdentity = effect->getInput(fvRequest->globalData.identityInputNb);
        if (inputEffectIdentity) {
            fvRequest->requests.push_back( std::make_pair( canonicalRenderWindow, FrameViewPerRequestData() ) );

            NodePtr inputIdentityNode = inputEffectIdentity->getNode();
            StatusEnum stat = getInputsRoIsFunctor(useTransforms,
                                                   fvRequest->globalData.inputIdentityTime,
                                                   fvRequest->globalData.identityView,
                                                   originalMipMapLevel,
                                                   inputIdentityNode,
                                                   node,
                                                   treeRoot,
                                                   canonicalRenderWindow,
                                                   requests);

            return stat;
        }

        //Identity but no input, if it's optional ignore, otherwise fail
        /*if (callerNode && callerNode != node) {

            int inputIndex = callerNode->getInputIndex(node.get());
            if (callerNode->getEffectInstance()->isInputOptional(inputIndex)
         || callerNode->getEffectInstance()->isInputMask(inputIndex)) {


                return eStatusOK;
            }
           }
           return eStatusFailed;*/

        // EDIT: always accept if identity has no input, it will produce a black image in the worst case scenario.
        return eStatusOK;
    }

    // Compute the regions of interest in input for this RoI
    FrameViewPerRequestData fvPerRequestData;
    effect->getRegionsOfInterest_public(time, nodeRequest->mappedScale, fvRequest->globalData.rod, canonicalRenderWindow, view, &fvPerRequestData.inputsRoi);


    // Transform Rois and get the reroutes map
    if (useTransforms) {
        if (fvRequest->globalData.transforms) {
            fvRequest->globalData.reroutesMap.reset( new ReRoutesMap() );
            transformInputRois( effect, fvRequest->globalData.transforms, par, nodeRequest->mappedScale, &fvPerRequestData.inputsRoi, fvRequest->globalData.reroutesMap );
        }
    }

    /*qDebug() << node->getFullyQualifiedName().c_str() << "RoI request: x1="<<canonicalRenderWindow.x1<<"y1="<<canonicalRenderWindow.y1<<"x2="<<canonicalRenderWindow.x2<<"y2="<<canonicalRenderWindow.y2;
       qDebug() << "Input RoIs:";
       for (RoIMap::iterator it = fvPerRequestData.inputsRoi.begin(); it!=fvPerRequestData.inputsRoi.end(); ++it) {
        qDebug() << it->first->getNode()->getFullyQualifiedName().c_str()<<"x1="<<it->second.x1<<"y1="<<it->second.y1<<"x2="<<it->second.x2<<"y2"<<it->second.y2;
       }*/
    ///Append the request
    fvRequest->requests.push_back( std::make_pair(canonicalRenderWindow, fvPerRequestData) );




    EffectInstance::RenderRoIRetCode ret = treeRecurseFunctor(false,
                                                              node,
                                                              fvRequest->globalData.frameViewsNeeded,
                                                              fvPerRequestData.inputsRoi,
                                                              fvRequest->globalData.transforms,
                                                              useTransforms,
                                                              eStorageModeRAM /*returnStorage*/,
                                                              originalMipMapLevel,
                                                              time,
                                                              view,
                                                              treeRoot,
                                                              &requests,
                                                              fvRequest,
                                                              0,
                                                              0,
                                                              false,
                                                              false);
    if (ret == EffectInstance::eRenderRoIRetCodeFailed) {
        return eStatusFailed;
    }

    return eStatusOK;
} // EffectInstance::getInputsRoIsFunctor

StatusEnum
EffectInstance::computeRequestPass(double time,
                                   ViewIdx view,
                                   unsigned int mipMapLevel,
                                   const RectD& renderWindow,
                                   const NodePtr& treeRoot,
                                   FrameRequestMap& request)
{
    bool doTransforms = appPTR->getCurrentSettings()->isTransformConcatenationEnabled();
    StatusEnum stat = getInputsRoIsFunctor(doTransforms,
                                           time,
                                           view,
                                           mipMapLevel,
                                           treeRoot,
                                           treeRoot,
                                           treeRoot,
                                           renderWindow,
                                           request);

    if (stat == eStatusFailed) {
        return stat;
    }

    //For all frame/view pair and for each node, compute the final roi as being the bounding box of all successive requests
    for (FrameRequestMap::iterator it = request.begin(); it != request.end(); ++it) {
        for (NodeFrameViewRequestData::iterator it2 = it->second->frames.begin(); it2 != it->second->frames.end(); ++it2) {
            const std::list<std::pair<RectD, FrameViewPerRequestData> >& rois = it2->second.requests;
            bool finalRoISet = false;
            for (std::list<std::pair<RectD, FrameViewPerRequestData> >::const_iterator it3 = rois.begin(); it3 != rois.end(); ++it3) {
                if (finalRoISet) {
                    if ( !it3->first.isNull() ) {
                        it2->second.finalData.finalRoi.merge(it3->first);
                    }
                } else {
                    finalRoISet = true;
                    if ( !it3->first.isNull() ) {
                        it2->second.finalData.finalRoi = it3->first;
                    }
                }
            }
        }
    }

    return eStatusOK;
}

const FrameViewRequest*
NodeFrameRequest::getFrameViewRequest(double time,
                                      ViewIdx view) const
{
    for (NodeFrameViewRequestData::const_iterator it = frames.begin(); it != frames.end(); ++it) {
        if (it->first.time == time) {
            if ( (it->first.view == -1) || (it->first.view == view) ) {
                return &it->second;
            }
        }
    }

    return 0;
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
    *roi = fv->finalData.finalRoi;

    return true;
}



struct FindDependenciesNode
{
    bool recursed;
    int visitCounter;
    FrameViewHashMap frameViewHash;

    FindDependenciesNode()
        : recursed(false), visitCounter(0), frameViewHash() {}
};


typedef std::map<NodePtr,FindDependenciesNode> FindDependenciesMap;


/**
 * @brief Builds a list with all nodes upstream of the given node (including this node) and all its dependencies through expressions as well (which
 * also may be recursive).
 * The hash is also computed for each frame/view pair of each node.
 * This function throw exceptions upon error.
 **/
static void
getDependenciesRecursive_internal(const NodePtr& node, double time, ViewIdx view, FindDependenciesMap& finalNodes, U64* nodeHash)
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
        //Add this node to the set
        nodeData = &finalNodes[node];
        nodeData->recursed = true;
        nodeData->visitCounter = 1;
    }


    //If we already got this node from expressions dependencies it, don't do it again
    if (!foundInMap || !alreadyRecursedUpstream) {

        std::set<NodePtr> expressionsDeps;
        effect->getAllExpressionDependenciesRecursive(expressionsDeps);

        //Also add all expression dependencies but mark them as we did not recursed on them yet
        for (std::set<NodePtr>::iterator it = expressionsDeps.begin(); it != expressionsDeps.end(); ++it) {
            FindDependenciesNode n;
            n.recursed = false;
            n.visitCounter = 0;
            finalNodes.insert(std::make_pair(node, n));
        }
    }

    // Compute the hash for this frame/view
    // First append the knobs to the hash then the hash of the inputs.
    // This function is virtual so derived implementation can influence the hash.
    boost::scoped_ptr<Hash64> hashObj;
    U64 hashValue = effect->findCachedHash(time ,view);
    bool isHashCached = hashValue != 0;
    if (!isHashCached) {
        // No hash in cache, compute it
        hashObj.reset(new Hash64);
        effect->computeHash_noCache(time, view, hashObj.get());
    }


    // Use getFramesNeeded to know where to recurse
    FramesNeededMap framesNeeded = effect->getFramesNeeded_public(0, time, view);
    for (FramesNeededMap::const_iterator it = framesNeeded.begin(); it != framesNeeded.end(); ++it) {

        // No need to use transform redirections to compute the hash
        EffectInstancePtr inputEffect = resolveInputEffectForFrameNeeded(it->first, effect, InputMatrixMapPtr());
        if (!inputEffect) {
            if (!effect->isInputOptional(it->first)) {
                effect->setPersistentMessage(eMessageTypeError, effect->tr("Input %1 is required to render").arg(QString::fromUtf8(effect->getInputLabel(it->first).c_str())).toStdString());
                throw std::invalid_argument("Input disconnected");
            }
            continue;
        }

        // For all views requested in input
        for (FrameRangesMap::const_iterator viewIt = it->second.begin(); viewIt != it->second.end(); ++viewIt) {

            // For all ranges in this view
            for (U32 range = 0; range < viewIt->second.size(); ++range) {

                // For all frames in the range
                for (double f = viewIt->second[range].min; f <= viewIt->second[range].max; f += 1.) {
                    U64 inputHash;
                    getDependenciesRecursive_internal(inputEffect->getNode(), f, viewIt->first, finalNodes, &inputHash);

                    // Append the input hash
                    if (!isHashCached) {
                        hashObj->append(inputHash);
                    }
                }
            }

        }
    }

    if (!isHashCached) {
        hashObj->computeHash();
        hashValue = hashObj->value();
    }

    FrameViewPair fv = {time, view};
    nodeData->frameViewHash[fv] = hashValue;

    if (!isHashCached) {
        effect->addHashToCache(time, view, hashValue);
    }

    if (nodeHash) {
        *nodeHash = hashValue;
    }


    // Now that we have the hash for this frame/view, cache actions results
    effect->cacheFramesNeeded(time, view, hashValue, framesNeeded);


} // getDependenciesRecursive_internal

#if 0
static bool
isRotoPaintNodeInputRecursive(const NodePtr& node,
                              const NodePtr& rotoPaintNode)
{
    if ( node == rotoPaintNode ) {
        return true;
    }
    int maxInputs = node->getMaxInputCount();
    for (int i = 0; i < maxInputs; ++i) {
        NodePtr input = node->getInput(i);
        if (input) {
            if (input == rotoPaintNode) {
                return true;
            } else {
                bool ret = isRotoPaintNodeInputRecursive(input, rotoPaintNode);
                if (ret) {
                    return true;
                }
            }
        }
    }

    return false;
}

/**
 * @brief The following is to flag that we are drawing on nodes in-between the Viewer and the RotoPaint node.
 * It is no longer needed since now we automatically connect the Viewer node to the RotoPaint when drawing.
 **/
static void
updateLastStrokeDataRecursively(const NodePtr& node,
                                const NodePtr& rotoPaintNode,
                                const RectD& lastStrokeBbox,
                                bool invalidate)
{
    if ( isRotoPaintNodeInputRecursive(node, rotoPaintNode) ) {
        if (invalidate) {
            node->invalidateLastPaintStrokeDataNoRotopaint();
        } else {
            node->setLastPaintStrokeDataNoRotopaint();
        }

        if ( node == rotoPaintNode ) {
            return;
        }
        int maxInputs = node->getMaxInputCount();
        for (int i = 0; i < maxInputs; ++i) {
            NodePtr input = node->getInput(i);
            if (input) {
                updateLastStrokeDataRecursively(input, rotoPaintNode, lastStrokeBbox, invalidate);
            }
        }
    }
}
#endif

/**
 * @brief While drawing, extract once all changes made by the user recently and keep this set of changes
 * throughout the render to keep things mt-safe.
 **/
static bool
setupRotoPaintDrawingData(const NodePtr& rotoPaintNode,
                          const RotoStrokeItemPtr& activeStroke,
                          const NodePtr& /*treeRoot*/,
                          double time)
{

    RotoPaintPtr rotoPaintEffect = toRotoPaint(rotoPaintNode->getEffectInstance());
    NodesList rotoPaintTreeNodes = rotoPaintEffect->getNodes();

    /*
     Take from the stroke all the points that were input by the user so far on the main thread and set them globally to the
     application. These data are the ones that are going to be used by any Roto related tool. We ensure that they all access
     the same data so we only access the real Roto datas now.
     */
    std::list<std::pair<Point, double> > lastStrokePoints;

    //The stroke RoD so far
    RectD wholeStrokeRod;

    //The bbox of the lastStrokePoints
    RectD lastStrokeBbox;

    //The index in the stroke of the last point we have rendered and up to the new point we have rendered
    int lastAge, newAge;

    //get on the app object the last point index we have rendered on this stroke
    lastAge = rotoPaintNode->getApp()->getStrokeLastIndex();

    //Get the active paint stroke so far and its multi-stroke index
    RotoStrokeItemPtr currentlyPaintedStroke;
    int currentlyPaintedStrokeMultiIndex;
    rotoPaintNode->getApp()->getStrokeAndMultiStrokeIndex(&currentlyPaintedStroke, &currentlyPaintedStrokeMultiIndex);


    //If this crashes here that means the user could start a new stroke while this one is not done rendering.
    assert(currentlyPaintedStroke == activeStroke);

    //the multi-stroke index in case of a stroke containing multiple strokes from the user
    int strokeIndex;
    bool isStrokeFirstTick;
    if ( activeStroke->getMostRecentStrokeChangesSinceAge(time, lastAge, currentlyPaintedStrokeMultiIndex, &lastStrokePoints, &lastStrokeBbox, &wholeStrokeRod, &isStrokeFirstTick, &newAge, &strokeIndex) ) {
        rotoPaintNode->getApp()->updateLastPaintStrokeData(isStrokeFirstTick, newAge, lastStrokePoints, lastStrokeBbox, strokeIndex);

        for (NodesList::iterator it = rotoPaintTreeNodes.begin(); it != rotoPaintTreeNodes.end(); ++it) {
            (*it)->prepareForNextPaintStrokeRender();
        }
        //updateLastStrokeDataRecursively(treeRoot, rotoPaintNode, lastStrokeBbox, false);
        return true;

    } else {
        return false;
    }
    
} // void setupRotoPaintDrawingData


static void setNodeTLSInternal(const ParallelRenderArgsSetter::CtorArgsPtr& inArgs,
                               bool doNansHandling,
                               const NodePtr& node,
                               int visitsCounter,
                               const FrameViewHashMap& frameViewHashes,
                               const OSGLContextPtr& gpuContext,
                               const OSGLContextPtr& cpuContext)
{
    EffectInstancePtr liveInstance = node->getEffectInstance();
    assert(liveInstance);


    {
        bool duringPaintStrokeCreation = !inArgs->isDoingRotoNeatRender && inArgs->activeRotoPaintNode && ((inArgs->activeRotoDrawableItem && node->getAttachedRotoItem() == inArgs->activeRotoDrawableItem) || node->isDuringPaintStrokeCreation()) ;
        RenderSafetyEnum safety = node->getCurrentRenderThreadSafety();
        PluginOpenGLRenderSupport glSupport = node->getCurrentOpenGLRenderSupport();

        EffectInstance::SetParallelRenderTLSArgsPtr tlsArgs(new EffectInstance::SetParallelRenderTLSArgs);
        tlsArgs->time = inArgs->time;
        tlsArgs->view = inArgs->view;
        tlsArgs->isRenderUserInteraction = inArgs->isRenderUserInteraction;
        tlsArgs->isSequential = inArgs->isSequential;
        tlsArgs->abortInfo = inArgs->abortInfo;
        tlsArgs->frameViewHash = frameViewHashes;
        tlsArgs->treeRoot = inArgs->treeRoot;
        tlsArgs->visitsCount = visitsCounter;
        tlsArgs->nodeRequest = NodeFrameRequestPtr();
        tlsArgs->glContext = gpuContext;
        tlsArgs->cpuGlContext = cpuContext;
        tlsArgs->textureIndex = inArgs->textureIndex;
        tlsArgs->timeline = inArgs->timeline;
        tlsArgs->isAnalysis = inArgs->isAnalysis;
        tlsArgs->isDuringPaintStrokeCreation = duringPaintStrokeCreation;
        tlsArgs->currentThreadSafety = safety;
        tlsArgs->currentOpenGLSupport = glSupport;
        tlsArgs->doNanHandling = doNansHandling;
        tlsArgs->draftMode = inArgs->draftMode;
        tlsArgs->stats = inArgs->stats;
        liveInstance->setParallelRenderArgsTLS(tlsArgs);

    }
}

void
ParallelRenderArgsSetter::fetchOpenGLContext(const CtorArgsPtr& inArgs)
{
    bool isPainting = false;
    if (inArgs->treeRoot) {
        isPainting = inArgs->treeRoot->getApp()->isDuringPainting();
    }

    // Ensure this thread gets an OpenGL context for the render of the frame
    OSGLContextPtr glContext, cpuContext;
    if (inArgs->activeRotoDrawableItem && (isPainting || inArgs->isDoingRotoNeatRender)) {

        if (isPainting) {
            assert(inArgs->activeRotoDrawableItem && inArgs->activeRotoPaintNode);
            setupRotoPaintDrawingData(inArgs->activeRotoPaintNode, boost::dynamic_pointer_cast<RotoStrokeItem>(inArgs->activeRotoDrawableItem), inArgs->treeRoot, inArgs->time);
        }

        // When painting, always use the same context since we paint over the same texture
        assert(inArgs->activeRotoDrawableItem);
        RotoStrokeItem* isStroke = dynamic_cast<RotoStrokeItem*>(inArgs->activeRotoDrawableItem.get());
        assert(isStroke);
        if (isStroke) {
            isStroke->getDrawingGLContext(&glContext, &cpuContext);
            if (!glContext && !cpuContext) {
                try {
                    glContext = appPTR->getGPUContextPool()->attachGLContextToRender(true/*retrieveLastContext*/);
                    cpuContext = appPTR->getGPUContextPool()->attachCPUGLContextToRender(true/*retrieveLastContext*/);
                    isStroke->setDrawingGLContext(glContext, cpuContext);
                } catch (const std::exception& /*e*/) {

                }
            }
        }
    } else {
        try {
            glContext = appPTR->getGPUContextPool()->attachGLContextToRender(false/*retrieveLastContext*/);
            cpuContext = appPTR->getGPUContextPool()->attachCPUGLContextToRender(false/*retrieveLastContext*/);
        } catch (const std::exception& /*e*/) {

        }
    }

    _openGLContext = glContext;
    _cpuOpenGLContext = cpuContext;
}

ParallelRenderArgsSetter::ParallelRenderArgsSetter(const CtorArgsPtr& inArgs)
: argsMap()
, nodes()
, _treeRoot(inArgs->treeRoot)
, _time(inArgs->time)
, _view(inArgs->view)
{
    assert(inArgs->treeRoot);




    fetchOpenGLContext(inArgs);
    OSGLContextPtr glContext = _openGLContext.lock(), cpuContext = _cpuOpenGLContext.lock();

    // Get dependencies node tree where to apply TLS
    FindDependenciesMap dependenciesMap;
    getDependenciesRecursive_internal(inArgs->treeRoot, inArgs->time, inArgs->view, dependenciesMap, 0);

    bool doNanHandling = appPTR->getCurrentSettings()->isNaNHandlingEnabled();
    for (FindDependenciesMap::iterator it = dependenciesMap.begin(); it != dependenciesMap.end(); ++it) {

        const NodePtr& node = it->first;
        nodes.push_back(node);

        setNodeTLSInternal(inArgs, doNanHandling, node, it->second.visitCounter, it->second.frameViewHash, glContext, cpuContext);
    }
}


StatusEnum
ParallelRenderArgsSetter::computeRequestPass(unsigned int mipMapLevel, const RectD& canonicalRoI)
{
    FrameRequestMap requestData;
    NodePtr treeRoot = _treeRoot.lock();
    assert(treeRoot);
    StatusEnum stat = EffectInstance::computeRequestPass(_time, _view, mipMapLevel, canonicalRoI, treeRoot, requestData);
    if (stat != eStatusOK) {
        return stat;
    }
    for (NodesList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        {
            FrameRequestMap::const_iterator foundRequest = requestData.find(*it);
            if ( foundRequest != requestData.end() ) {
                (*it)->getEffectInstance()->setNodeRequestThreadLocal(foundRequest->second);
            }
        }

        NodesList rotoPaintNodes;
        RotoContextPtr roto = (*it)->getRotoContext();
        if (roto) {
            roto->getRotoPaintTreeNodes(&rotoPaintNodes);
        }

        for (NodesList::iterator it2 = rotoPaintNodes.begin(); it2 != rotoPaintNodes.end(); ++it2) {
            FrameRequestMap::const_iterator foundRequest = requestData.find(*it2);
            if ( foundRequest != requestData.end() ) {
                (*it2)->getEffectInstance()->setNodeRequestThreadLocal(foundRequest->second);
            }
        }

        if ( (*it)->isMultiInstance() ) {
            ///If the node has children, set the thread-local storage on them too, even if they do not render, it can be useful for expressions
            ///on parameters.
            NodesList children;
            (*it)->getChildrenMultiInstance(&children);
            for (NodesList::iterator it2 = children.begin(); it2 != children.end(); ++it2) {
                FrameRequestMap::const_iterator foundRequest = requestData.find(*it2);
                if ( foundRequest != requestData.end() ) {
                    (*it2)->getEffectInstance()->setNodeRequestThreadLocal(foundRequest->second);
                }
            }
        }
    }
    return stat;
}

ParallelRenderArgsSetter::ParallelRenderArgsSetter(const boost::shared_ptr<std::map<NodePtr, ParallelRenderArgsPtr > >& args)
: argsMap(args)
, nodes()
, _treeRoot()
, _time(0)
, _view(0)
{
    bool isPainting = false;
    if (args && !args->empty()) {
        isPainting = args->begin()->first->getApp()->isDuringPainting();
    }

    // Ensure this thread gets an OpenGL context for the render of the frame
    OSGLContextPtr glContext;

    try {
        glContext = appPTR->getGPUContextPool()->attachGLContextToRender(isPainting /*retrieveLastContext*/);
        _openGLContext = glContext;
    } catch (...) {
    }

    OSGLContextPtr cpuContext;
    try {
        cpuContext = appPTR->getGPUContextPool()->attachCPUGLContextToRender(isPainting /*retrieveLastContext*/);
        _cpuOpenGLContext = cpuContext;
    } catch (const std::exception& e) {
    }


    if (args) {
        for (std::map<NodePtr, ParallelRenderArgsPtr >::iterator it = argsMap->begin(); it != argsMap->end(); ++it) {
            it->second->openGLContext = glContext;
            it->first->getEffectInstance()->setParallelRenderArgsTLS(it->second);
        }
    }
}

ParallelRenderArgsSetter::~ParallelRenderArgsSetter()
{
    for (NodesList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        if ( !(*it) || !(*it)->getEffectInstance() ) {
            continue;
        }
        (*it)->getEffectInstance()->invalidateParallelRenderArgsTLS();

        if ( (*it)->isMultiInstance() ) {
            ///If the node has children, set the thread-local storage on them too, even if they do not render, it can be useful for expressions
            ///on parameters.
            NodesList children;
            (*it)->getChildrenMultiInstance(&children);
            for (NodesList::iterator it2 = children.begin(); it2 != children.end(); ++it2) {
                (*it2)->getEffectInstance()->invalidateParallelRenderArgsTLS();
            }
        }

        /* NodeGroupPtr isGrp = (*it)->isEffectNodeGroup();
           if (isGrp) {
             isGrp->invalidateParallelRenderArgs();
           }*/
    }

    if (argsMap) {
        for (std::map<NodePtr, ParallelRenderArgsPtr >::iterator it = argsMap->begin(); it != argsMap->end(); ++it) {
            it->first->getEffectInstance()->invalidateParallelRenderArgsTLS();
        }
    }

    /*if (rotoNode) {
        updateLastStrokeDataRecursively(viewerNode, rotoNode, RectD(), true);
    }*/

    OSGLContextPtr glContext = _openGLContext.lock();
    if (glContext) {
        // This render is going to end, release the OpenGL context so that another frame render may use it
        appPTR->getGPUContextPool()->releaseGLContextFromRender(glContext);
    }

    OSGLContextPtr cpuContext = _cpuOpenGLContext.lock();
    if (cpuContext) {
        // This render is going to end, release the OpenGL context so that another frame render may use it
        appPTR->getGPUContextPool()->releaseCPUGLContextFromRender(cpuContext);
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
    , isDuringPaintStrokeCreation(false)
    , doNansHandling(true)
    , draftMode(false)
    , tilesSupported(false)
{
}

bool
ParallelRenderArgs::isCurrentFrameRenderNotAbortable() const
{
    AbortableRenderInfoPtr info = abortInfo.lock();

    return isRenderResponseToUserInteraction && ( !info || !info->canAbort() );
}

U64
ParallelRenderArgs::getFrameViewHash(double time, ViewIdx view) const
{
    return findFrameViewHash(time, view, frameViewHash);
}

NATRON_NAMESPACE_EXIT;
