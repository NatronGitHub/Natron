/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#include "Engine/AbortableRenderInfo.h"
#include "Engine/AppManager.h"
#include "Engine/Settings.h"
#include "Engine/EffectInstance.h"
#include "Engine/Image.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/GPUContextPool.h"
#include "Engine/OSGLContext.h"
#include "Engine/RotoContext.h"
#include "Engine/RotoDrawableItem.h"
#include "Engine/ViewIdx.h"

NATRON_NAMESPACE_ENTER

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
                                   ViewIdx view,
                                   const NodePtr& treeRoot,
                                   FrameRequestMap* requests,  // roi functor specific
                                   EffectInstance::InputImagesMap* inputImages, // render functor specific
                                   const EffectInstance::ComponentsNeededMap* neededComps, // render functor specific
                                   bool useScaleOneInputs, // render functor specific
                                   bool byPassCache) // render functor specific
{
    ///For all frames/views needed, call recursively on inputs with the appropriate RoI

    EffectInstancePtr effect = node->getEffectInstance();
    bool isRoto = node->isRotoPaintingNode();

    //Same as FramesNeededMap but we also get a pointer to EffectInstance* as key
    typedef std::map<EffectInstancePtr, std::pair</*inputNb*/ int, FrameRangesMap> > PreRenderFrames;

    PreRenderFrames framesToRender;
    //Add frames needed to the frames to render
    for (FramesNeededMap::const_iterator it = framesNeeded.begin(); it != framesNeeded.end(); ++it) {
        int inputNb = it->first;
        bool inputIsMask = effect->isInputMask(inputNb);
        ImagePlaneDesc maskComps;
        int channelForAlphaInput;
        // if (inputIsMask) {
        if ( !effect->isMaskEnabled(inputNb) ) {
            continue;
        }

        std::list<ImagePlaneDesc> availableLayers;
        effect->getAvailableLayers(time, view, inputNb, &availableLayers);

        channelForAlphaInput = effect->getMaskChannel(inputNb, availableLayers, &maskComps);
        // } else {
        //}

        //No mask
        if ( inputIsMask && ( (channelForAlphaInput == -1) || (maskComps.getNumComponents() == 0) ) ) {
            continue;
        }

        //Redirect for transforms if needed
        EffectInstancePtr inputEffect;
        if (reroutesMap) {
            InputMatrixMap::const_iterator foundReroute = reroutesMap->find(inputNb);
            if ( foundReroute != reroutesMap->end() ) {
                inputEffect = foundReroute->second.newInputEffect->getInput(foundReroute->second.newInputNbToFetchFrom);
            }
        }

        if (!inputEffect) {
            inputEffect = node->getEffectInstance()->getInput(inputNb);
        }

        //Never pre-render the mask if we are rendering a node of the rotopaint tree
        if ( node->getAttachedRotoItem() && inputEffect && inputEffect->isRotoPaintNode() ) {
            continue;
        }

        if (inputEffect) {
            framesToRender[inputEffect] = std::make_pair(inputNb, it->second);
        }
    }

    if (isRoto && !isRenderFunctor) {
        //Also add internal rotopaint tree RoIs
        NodePtr btmMerge = effect->getNode()->getRotoContext()->getRotoPaintBottomMergeNode();
        if (btmMerge) {
            FrameRangesMap frames;
            std::vector<OfxRangeD> vec;
            OfxRangeD r;
            r.min = r.max = time;
            vec.push_back(r);
            frames[view] = vec;
            framesToRender[btmMerge->getEffectInstance()] = std::make_pair(-1, frames);
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
        const std::list<ImagePlaneDesc>* compsNeeded = 0;

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
            EffectInstance::NotifyInputNRenderingStarted_RAIIPtr inputNIsRendering_RAII;
            if (isRenderFunctor) {
                assert(it->second.first != -1); //< see getInputNumber
                inputNIsRendering_RAII.reset( new EffectInstance::NotifyInputNRenderingStarted_RAII(node.get(), inputNb) );
            }

            ///For all views requested in input
            for (FrameRangesMap::const_iterator viewIt = it->second.second.begin(); viewIt != it->second.second.end(); ++viewIt) {
                ///For all frames in this view
                for (U32 range = 0; range < viewIt->second.size(); ++range) {
                    int nbFramesPreFetched = 0;

                    // if the range bounds are not ints, the fetched images will probably anywhere within this range - no need to pre-render
                    if ( (viewIt->second[range].min == (int)viewIt->second[range].min) &&
                         ( viewIt->second[range].max == (int)viewIt->second[range].max) ) {
                        for (double f = viewIt->second[range].min;
                             f <= viewIt->second[range].max  && nbFramesPreFetched < NATRON_MAX_FRAMES_NEEDED_PRE_FETCHING;
                             f += 1.) {
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
                                RenderScale scaleOne(1.);
                                RenderScale scale( Image::getScaleFromMipMapLevel(originalMipMapLevel) );

                                ///Render the input image with the bit depth of its preference
                                ImageBitDepthEnum inputPrefDepth = inputEffect->getBitDepth(-1);

                                if ( !compsNeeded || compsNeeded->empty() ) {
                                    continue;
                                }

                                if (roiIsInRequestPass) {
                                    frameArgs->request->getFrameViewCanonicalRoI(f, viewIt->first, &roi);
                                }

                                RectI inputRoIPixelCoords;
                                const unsigned int upstreamMipMapLevel = useScaleOneInputs ? 0 : originalMipMapLevel;
                                const RenderScale & upstreamScale = useScaleOneInputs ? scaleOne : scale;
                                roi.toPixelEnclosing(upstreamMipMapLevel, inputPar, &inputRoIPixelCoords);

                                std::map<ImagePlaneDesc, ImagePtr> inputImgs;
                                {
                                    boost::scoped_ptr<EffectInstance::RenderRoIArgs> renderArgs;
                                    renderArgs.reset( new EffectInstance::RenderRoIArgs( f, //< time
                                                                                         upstreamScale, //< scale
                                                                                         upstreamMipMapLevel, //< mipmapLevel (redundant with the scale)
                                                                                         viewIt->first, //< view
                                                                                         byPassCache,
                                                                                         inputRoIPixelCoords, //< roi in pixel coordinates
                                                                                         RectD(), // < did we precompute any RoD to speed-up the call ?
                                                                                         *compsNeeded, //< requested comps
                                                                                         inputPrefDepth,
                                                                                         false,
                                                                                         effect.get(),
                                                                                         renderStorageMode /*returnStorage*/,
                                                                                         time /*callerRenderTime*/) );

                                    EffectInstance::RenderRoIRetCode ret;
                                    ret = inputEffect->renderRoI(*renderArgs, &inputImgs); //< requested bitdepth
                                    if (ret != EffectInstance::eRenderRoIRetCodeOk) {
                                        return ret;
                                    }
                                }
                                for (std::map<ImagePlaneDesc, ImagePtr>::iterator it3 = inputImgs.begin(); it3 != inputImgs.end(); ++it3) {
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
                    }
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

        NodeFrameRequestPtr tmp = boost::make_shared<NodeFrameRequest>();
        tmp->mappedScale.x = tmp->mappedScale.y = Image::getScaleFromMipMapLevel(mappedLevel);
        tmp->nodeHash = effect->getRenderHash();

        std::pair<FrameRequestMap::iterator, bool> ret = requests.insert( std::make_pair(node, tmp) );
        assert(ret.second);
        nodeRequest = ret.first->second;
    }
    assert(nodeRequest);

    ///Okay now we concentrate on this particular frame/view pair

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
                fvRequest->globalData.isIdentity = effect->isIdentity_public(true, nodeRequest->nodeHash, time, nodeRequest->mappedScale, identityRegionPixel, view, &fvRequest->globalData.inputIdentityTime, &fvRequest->globalData.identityView, &fvRequest->globalData.identityInputNb);
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

        ///Get the RoD
        StatusEnum stat = effect->getRegionOfDefinition_public(nodeRequest->nodeHash, rodTime, nodeRequest->mappedScale, rodView, &fvRequest->globalData.rod, &fvRequest->globalData.isProjectFormat);
        //If failed it should have failed earlier
        if ( (stat == eStatusFailed) && !fvRequest->globalData.rod.isNull() ) {
            return stat;
        }


        ///Concatenate transforms if needed
        if (useTransforms) {
            fvRequest->globalData.transforms = boost::make_shared<InputMatrixMap>();
#pragma message WARN("TODO: can set draftRender properly here?")
            effect->tryConcatenateTransforms( time, /*draftRender=*/false, view, nodeRequest->mappedScale, fvRequest->globalData.transforms.get() );
        }

        ///Get the frame/views needed for this frame/view
        fvRequest->globalData.frameViewsNeeded = effect->getFramesNeeded_public(nodeRequest->nodeHash, time, view, mappedLevel);
    } // if (foundFrameView != nodeRequest->frames.end()) {

    assert(fvRequest);


    bool finalRoIEmpty = fvRequest->finalData.finalRoi.isNull();
    if (!finalRoIEmpty && fvRequest->finalData.finalRoi.contains(canonicalRenderWindow)) {
        // Do not recurse if the roi did not add anything new to render
        return eStatusOK;
    }
    if (finalRoIEmpty) {
        fvRequest->finalData.finalRoi = canonicalRenderWindow;
    } else {
        fvRequest->finalData.finalRoi.merge(canonicalRenderWindow);
    }

    if (fvRequest->globalData.identityInputNb == -2) {
        assert(fvRequest->globalData.inputIdentityTime != time || viewInvariance == eViewInvarianceAllViewsInvariant);
        // be safe in release mode otherwise we hit an infinite recursion
        if ( (fvRequest->globalData.inputIdentityTime != time) || (viewInvariance == eViewInvarianceAllViewsInvariant) ) {
            //fvRequest->requests.push_back( std::make_pair( canonicalRenderWindow, FrameViewPerRequestData() ) );

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
            //fvRequest->requests.push_back( std::make_pair( canonicalRenderWindow, FrameViewPerRequestData() ) );

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

    ///Compute the regions of interest in input for this RoI
    FrameViewPerRequestData fvPerRequestData;
    effect->getRegionsOfInterest_public(time, nodeRequest->mappedScale, fvRequest->globalData.rod, canonicalRenderWindow, view, &fvPerRequestData.inputsRoi);


    ///Transform Rois and get the reroutes map
    if (useTransforms) {
        if (fvRequest->globalData.transforms) {
            fvRequest->globalData.reroutesMap.reset( new std::map<int, EffectInstancePtr>() );
            transformInputRois( effect.get(), fvRequest->globalData.transforms, par, nodeRequest->mappedScale, &fvPerRequestData.inputsRoi, fvRequest->globalData.reroutesMap.get() );
        }
    }

    /*qDebug() << node->getFullyQualifiedName().c_str() << "RoI request: x1="<<canonicalRenderWindow.x1<<"y1="<<canonicalRenderWindow.y1<<"x2="<<canonicalRenderWindow.x2<<"y2="<<canonicalRenderWindow.y2;
       qDebug() << "Input RoIs:";
       for (RoIMap::iterator it = fvPerRequestData.inputsRoi.begin(); it!=fvPerRequestData.inputsRoi.end(); ++it) {
        qDebug() << it->first->getNode()->getFullyQualifiedName().c_str()<<"x1="<<it->second.x1<<"y1="<<it->second.y1<<"x2="<<it->second.x2<<"y2"<<it->second.y2;
       }*/



    // Append the request
    //fvRequest->requests.push_back( std::make_pair(canonicalRenderWindow, fvPerRequestData) );

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
    /*for (FrameRequestMap::iterator it = request.begin(); it != request.end(); ++it) {
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
    }*/

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

    FindDependenciesNode()
        : recursed(false), visitCounter(0) {}
};


typedef std::map<NodePtr,FindDependenciesNode> FindDependenciesMap;


/**
 * @brief Builds a list with all nodes upstream of the given node (including this node) and all its dependencies through expressions as well (which
 * also may be recursive)
 **/
static void
getAllUpstreamNodesRecursiveWithDependencies_internal(const NodePtr& node,
                                                      FindDependenciesMap& finalNodes)
{
    //There may be cases where nodes gets added to the finalNodes in getAllExpressionDependenciesRecursive(), but we still
    //want to recurse upstream for them too
    bool foundButDidntRecursivelyCallUpstream = false;

    if ( !node || !node->isNodeCreated() ) {
        return;
    }

    {
        FindDependenciesMap::iterator found = finalNodes.find(node);
        if (found != finalNodes.end()) {
            if (found->second.recursed) {
                ++found->second.visitCounter;
                //We already called getAllUpstreamNodesRecursiveWithDependencies on its inputs
                return;
            } else {
                //Now we set the recurse flag below
                finalNodes.erase(found);
                foundButDidntRecursivelyCallUpstream = true;
            }

        }
    }
    
    {
        //Add this node to the set
        FindDependenciesNode n;
        n.recursed = true;
        n.visitCounter = 1;
        finalNodes.insert(std::make_pair(node,n));
    }

    //If we already called it, don't do it again
    if (!foundButDidntRecursivelyCallUpstream) {
        std::set<NodePtr> expressionsDeps;
        node->getEffectInstance()->getAllExpressionDependenciesRecursive(expressionsDeps);

        //Also add all expression dependencies but mark them as we did not recursed on them yet
        for (std::set<NodePtr>::iterator it = expressionsDeps.begin(); it != expressionsDeps.end(); ++it) {
            FindDependenciesNode n;
            n.recursed = false;
            n.visitCounter = 0;
            finalNodes.insert(std::make_pair(node, n));
        }
    }

    int maxInputs = node->getNInputs();
    for (int i = 0; i < maxInputs; ++i) {
        NodePtr inputNode = node->getInput(i);
        if (inputNode) {
            getAllUpstreamNodesRecursiveWithDependencies_internal(inputNode, finalNodes);
        }
    }
} // getAllUpstreamNodesRecursiveWithDependencies_internal


ParallelRenderArgsSetter::ParallelRenderArgsSetter(double time,
                                                   ViewIdx view,
                                                   bool isRenderUserInteraction,
                                                   bool isSequential,
                                                   const AbortableRenderInfoPtr& abortInfo,
                                                   const NodePtr& treeRoot,
                                                   int textureIndex,
                                                   const TimeLine* timeline,
                                                   const NodePtr& activeRotoPaintNode,
                                                   bool isAnalysis,
                                                   bool draftMode,
                                                   const RenderStatsPtr& stats)
    :  argsMap()
{
    assert(treeRoot);

    // Ensure this thread gets an OpenGL context for the render of the frame
    OSGLContextPtr glContext;
    try {
        glContext = appPTR->getGPUContextPool()->attachGLContextToRender();
    } catch (const std::exception& /*e*/) {

    }

    _openGLContext = glContext;


    bool doNanHandling = appPTR->getCurrentSettings()->isNaNHandlingEnabled();

    FindDependenciesMap dependenciesMap;
    getAllUpstreamNodesRecursiveWithDependencies_internal(treeRoot, dependenciesMap);


    for (FindDependenciesMap::iterator it = dependenciesMap.begin(); it != dependenciesMap.end(); ++it) {

        const NodePtr& node = it->first;
        nodes.push_back(node);

        EffectInstancePtr liveInstance = node->getEffectInstance();
        assert(liveInstance);
        bool duringPaintStrokeCreation = activeRotoPaintNode && node->isDuringPaintStrokeCreation();
        RenderSafetyEnum safety = node->getCurrentRenderThreadSafety();
        PluginOpenGLRenderSupport glSupport = node->getCurrentOpenGLRenderSupport();
        NodesList rotoPaintNodes;
        RotoContextPtr roto = node->getRotoContext();
        if (roto) {
            roto->getRotoPaintTreeNodes(&rotoPaintNodes);
        }

        {
            U64 nodeHash = node->getHashValue();
            liveInstance->setParallelRenderArgsTLS(time, view, isRenderUserInteraction, isSequential, nodeHash,
                                                   abortInfo, treeRoot, it->second.visitCounter, NodeFrameRequestPtr(), glContext,  textureIndex, timeline, isAnalysis, duringPaintStrokeCreation, rotoPaintNodes, safety, glSupport, doNanHandling, draftMode, stats);
        }
        for (NodesList::iterator it2 = rotoPaintNodes.begin(); it2 != rotoPaintNodes.end(); ++it2) {
            U64 nodeHash = (*it2)->getHashValue();

            // For rotopaint nodes, since the tree internally is always the same for all renders (it doesn't depend where the viewer is connected) the visits count is the  number of output nodes
            NodesWList outputs;
            (*it2)->getOutputs_mt_safe(outputs);
            int visitsCounter = (int)outputs.size();

            (*it2)->getEffectInstance()->setParallelRenderArgsTLS(time, view, isRenderUserInteraction, isSequential, nodeHash, abortInfo, treeRoot, visitsCounter, NodeFrameRequestPtr(), glContext, textureIndex, timeline, isAnalysis, activeRotoPaintNode && (*it2)->isDuringPaintStrokeCreation(), NodesList(), (*it2)->getCurrentRenderThreadSafety(),  (*it2)->getCurrentOpenGLRenderSupport(),doNanHandling, draftMode, stats);
        }

        if ( node->isMultiInstance() ) {
            ///If the node has children, set the thread-local storage on them too, even if they do not render, it can be useful for expressions
            ///on parameters.
            NodesList children;
            node->getChildrenMultiInstance(&children);
            for (NodesList::iterator it2 = children.begin(); it2 != children.end(); ++it2) {
                U64 nodeHash = (*it2)->getHashValue();

                assert(*it2);
                EffectInstancePtr childLiveInstance = (*it2)->getEffectInstance();
                assert(childLiveInstance);
                RenderSafetyEnum childSafety = (*it2)->getCurrentRenderThreadSafety();
                PluginOpenGLRenderSupport childGlSupport = (*it2)->getCurrentOpenGLRenderSupport();
                childLiveInstance->setParallelRenderArgsTLS(time, view, isRenderUserInteraction, isSequential, nodeHash, abortInfo, treeRoot, 1, NodeFrameRequestPtr(), glContext, textureIndex, timeline, isAnalysis, false, NodesList(), childSafety, childGlSupport, doNanHandling, draftMode, stats);
            }
        }


        /* NodeGroup* isGrp = node->isEffectGroup();
           if (isGrp) {
             isGrp->setParallelRenderArgs(time, view, isRenderUserInteraction, isSequential, canAbort,  renderAge, treeRoot, request, textureIndex, timeline, activeRotoPaintNode, isAnalysis, draftMode,stats);
           }*/
    }
}

void
ParallelRenderArgsSetter::updateNodesRequest(const FrameRequestMap& request)
{
    for (NodesList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        {
            FrameRequestMap::const_iterator foundRequest = request.find(*it);
            if ( foundRequest != request.end() ) {
                (*it)->getEffectInstance()->setNodeRequestThreadLocal(foundRequest->second);
            }
        }

        NodesList rotoPaintNodes;
        RotoContextPtr roto = (*it)->getRotoContext();
        if (roto) {
            roto->getRotoPaintTreeNodes(&rotoPaintNodes);
        }

        for (NodesList::iterator it2 = rotoPaintNodes.begin(); it2 != rotoPaintNodes.end(); ++it2) {
            FrameRequestMap::const_iterator foundRequest = request.find(*it2);
            if ( foundRequest != request.end() ) {
                (*it2)->getEffectInstance()->setNodeRequestThreadLocal(foundRequest->second);
            }
        }

        if ( (*it)->isMultiInstance() ) {
            ///If the node has children, set the thread-local storage on them too, even if they do not render, it can be useful for expressions
            ///on parameters.
            NodesList children;
            (*it)->getChildrenMultiInstance(&children);
            for (NodesList::iterator it2 = children.begin(); it2 != children.end(); ++it2) {
                FrameRequestMap::const_iterator foundRequest = request.find(*it2);
                if ( foundRequest != request.end() ) {
                    (*it2)->getEffectInstance()->setNodeRequestThreadLocal(foundRequest->second);
                }
            }
        }
    }
}

ParallelRenderArgsSetter::ParallelRenderArgsSetter(const boost::shared_ptr<std::map<NodePtr, ParallelRenderArgsPtr> >& args)
    : argsMap(args)
{
    // Ensure this thread gets an OpenGL context for the render of the frame
    OSGLContextPtr glContext;

    try {
        glContext = appPTR->getGPUContextPool()->attachGLContextToRender();
        _openGLContext = glContext;
    } catch (...) {
    }

    if (args) {
        for (std::map<NodePtr, ParallelRenderArgsPtr>::iterator it = argsMap->begin(); it != argsMap->end(); ++it) {
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

        /* NodeGroup* isGrp = (*it)->isEffectGroup();
           if (isGrp) {
             isGrp->invalidateParallelRenderArgs();
           }*/
    }

    if (argsMap) {
        for (std::map<NodePtr, ParallelRenderArgsPtr>::iterator it = argsMap->begin(); it != argsMap->end(); ++it) {
            it->first->getEffectInstance()->invalidateParallelRenderArgsTLS();
        }
    }

    OSGLContextPtr glContext = _openGLContext.lock();
    if (glContext) {
        // This render is going to end, release the OpenGL context so that another frame render may use it
        appPTR->getGPUContextPool()->releaseGLContextFromRender(glContext);
    }
}

ParallelRenderArgs::ParallelRenderArgs()
    : time(0)
    , timeline(0)
    , nodeHash(0)
    , request()
    , view(0)
    , abortInfo()
    , treeRoot()
    , visitsCount(0)
    , rotoPaintNodes()
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

NATRON_NAMESPACE_EXIT
