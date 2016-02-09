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

#include "Engine/AppManager.h"
#include "Engine/Settings.h"
#include "Engine/EffectInstance.h"
#include "Engine/Image.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/RotoContext.h"
#include "Engine/RotoDrawableItem.h"

NATRON_NAMESPACE_ENTER;

EffectInstance::RenderRoIRetCode EffectInstance::treeRecurseFunctor(bool isRenderFunctor,
                                                                            const NodePtr& node,
                                                                            const FramesNeededMap& framesNeeded,
                                                                            const RoIMap& inputRois,
                                                                            const boost::shared_ptr<InputMatrixMap>& reroutesMap,
                                                                            bool useTransforms, // roi functor specific
                                                                            unsigned int originalMipMapLevel, // roi functor specific
                                                                            double time,
                                                                            int view,
                                                                            const NodePtr& treeRoot,
                                                                            FrameRequestMap* requests,  // roi functor specific
                                                                           EffectInstance::InputImagesMap* inputImages, // render functor specific
                                                                            const EffectInstance::ComponentsNeededMap* neededComps, // render functor specific
                                                                            bool useScaleOneInputs, // render functor specific
                                                                            bool byPassCache) // render functor specific
{
    ///For all frames/views needed, call recursively on inputs with the appropriate RoI

    EffectInstPtr effect = node->getEffectInstance();
    bool isRoto = node->isRotoPaintingNode();
    
    //Same as FramesNeededMap but we also get a pointer to EffectInstance* as key
    typedef std::map<EffectInstPtr, std::pair<int,std::map<int, std::vector<OfxRangeD> > > > PreRenderFrames;
    
    PreRenderFrames framesToRender;
    //Add frames needed to the frames to render
    for (FramesNeededMap::const_iterator it = framesNeeded.begin(); it != framesNeeded.end(); ++it) {
        int inputNb = it->first;
        
        bool inputIsMask = effect->isInputMask(inputNb);
        
        ImageComponents maskComps;
        int channelForAlphaInput;
        NodePtr maskInput;
        // if (inputIsMask) {
        if (!effect->isMaskEnabled(inputNb)) {
            continue;
        }
        channelForAlphaInput = effect->getMaskChannel(inputNb,&maskComps,&maskInput);
        // } else {
        //}
        
        //No mask
        if (inputIsMask && (channelForAlphaInput == -1 || maskComps.getNumComponents() == 0)) {
            continue;
        }
        
        //Redirect for transforms if needed
        EffectInstPtr inputEffect;
        if (reroutesMap) {
            InputMatrixMap::const_iterator foundReroute = reroutesMap->find(inputNb);
            if (foundReroute != reroutesMap->end()) {
                inputEffect = foundReroute->second.newInputEffect->getInput(foundReroute->second.newInputNbToFetchFrom);
            }
        }
        
        if (!inputEffect) {
            inputEffect = node->getEffectInstance()->getInput(inputNb);
        }
        
        //Redirect the mask input
        if (maskInput) {
            inputEffect = maskInput->getEffectInstance();
        }
        
        //Never pre-render the mask if we are rendering a node of the rotopaint tree
        if (node->getAttachedRotoItem() && inputEffect && inputEffect->isRotoPaintNode()) {
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
            std::map<int,std::vector<OfxRangeD> > frames;
            std::vector<OfxRangeD> vec;
            OfxRangeD r;
            r.min = r.max = time;
            vec.push_back(r);
            frames[view] = vec;
            framesToRender[btmMerge->getEffectInstance()] = std::make_pair(-1, frames);
        }
    }
    
    for (PreRenderFrames::const_iterator it = framesToRender.begin(); it != framesToRender.end(); ++it) {
        
        const EffectInstPtr& inputEffect = it->first;
        
        NodePtr inputNode = inputEffect->getNode();
        assert(inputNode);
        
        int inputNb = it->second.first;
        if (inputNb == -1 && isRenderFunctor) {
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
            if (foundInputImages == inputImages->end()) {
                std::pair<InputImagesMap::iterator,bool> ret = inputImages->insert(std::make_pair(inputNb, ImageList()));
                inputImagesList = &ret.first->second;
                assert(ret.second);
            }
        }
        
        ///What region are we interested in for this input effect ? (This is in Canonical coords)
        RectD roi;
        
        bool roiIsInRequestPass = false;
        boost::shared_ptr<ParallelRenderArgs> frameArgs;
        if (isRenderFunctor) {
            frameArgs = inputEffect->getParallelRenderArgsTLS();
            if (frameArgs && frameArgs->request) {
                roiIsInRequestPass = true;
            }
            
        }
        
        if (!roiIsInRequestPass) {
            RoIMap::const_iterator foundInputRoI = inputRois.find(inputEffect);
            if (foundInputRoI == inputRois.end()) {
                continue;
            }
            if (foundInputRoI->second.isInfinite()) {
                std::stringstream ss;
                ss << node->getScriptName_mt_safe();
                ss << QObject::tr(" asked for an infinite region of interest upstream").toStdString();
                effect->setPersistentMessage(eMessageTypeError, ss.str());
                return EffectInstance::eRenderRoIRetCodeFailed;
            }
            
            if (foundInputRoI->second.isNull()) {
                continue;
            }
            roi = foundInputRoI->second;
        }
        
        ///There cannot be frames needed without components needed.
        const std::vector<ImageComponents>* compsNeeded = 0;
        
        if (neededComps) {
           EffectInstance::ComponentsNeededMap::const_iterator foundCompsNeeded = neededComps->find(inputNb);
            if (foundCompsNeeded == neededComps->end()) {
                continue;
            } else {
                compsNeeded = &foundCompsNeeded->second;
            }
        }
        
        
        const double inputPar = inputEffect->getPreferredAspectRatio();
        
        
        
        {
            ///Notify the node that we're going to render something with the input
            boost::shared_ptr<EffectInstance::NotifyInputNRenderingStarted_RAII> inputNIsRendering_RAII;
            if (isRenderFunctor) {
                assert(it->second.first != -1); //< see getInputNumber
                inputNIsRendering_RAII.reset(new EffectInstance::NotifyInputNRenderingStarted_RAII(node.get(),inputNb));
            }
            
            ///For all views requested in input
            for (std::map<int, std::vector<OfxRangeD> >::const_iterator viewIt = it->second.second.begin(); viewIt != it->second.second.end(); ++viewIt) {
                
                ///For all frames in this view
                for (U32 range = 0; range < viewIt->second.size(); ++range) {
                    
                    int nbFramesPreFetched = 0;
                    
                    // if the range bounds are not ints, the fetched images will probably anywhere within this range - no need to pre-render
                    if (viewIt->second[range].min == (int)viewIt->second[range].min &&
                        viewIt->second[range].max == (int)viewIt->second[range].max) {
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
                                
                                RenderScale scale(Image::getScaleFromMipMapLevel(originalMipMapLevel));
                                
                                ///Render the input image with the bit depth of its preference
                                std::list<ImageComponents> inputPrefComps;
                                ImageBitDepthEnum inputPrefDepth;
                                inputEffect->getPreferredDepthAndComponents(-1/*it2->first*/, &inputPrefComps, &inputPrefDepth);
                                std::list<ImageComponents> componentsToRender;
                                
                                assert(compsNeeded);
                                if (!compsNeeded) {
                                    continue;
                                }
                                for (U32 k = 0; k < compsNeeded->size(); ++k) {
                                    if (compsNeeded->at(k).getNumComponents() > 0) {
                                        componentsToRender.push_back(compsNeeded->at(k));
                                    }
                                }
                                if (componentsToRender.empty()) {
                                    continue;
                                }
                                
                                if (roiIsInRequestPass) {
                                    frameArgs->request->getFrameViewCanonicalRoI(f, viewIt->first, &roi);
                                }
                                
                                RectI inputRoIPixelCoords;
                                const unsigned int upstreamMipMapLevel = useScaleOneInputs ? 0 : originalMipMapLevel;
                                const RenderScale & upstreamScale = useScaleOneInputs ? scaleOne : scale;
                                roi.toPixelEnclosing(upstreamMipMapLevel, inputPar, &inputRoIPixelCoords);

                                ImageList inputImgs;
                                {
                                    boost::scoped_ptr<EffectInstance::RenderRoIArgs> renderArgs;
                                    renderArgs.reset(new EffectInstance::RenderRoIArgs(f, //< time
                                                                                       upstreamScale, //< scale
                                                                                       upstreamMipMapLevel, //< mipmapLevel (redundant with the scale)
                                                                                       viewIt->first, //< view
                                                                                       byPassCache,
                                                                                       inputRoIPixelCoords, //< roi in pixel coordinates
                                                                                       RectD(), // < did we precompute any RoD to speed-up the call ?
                                                                                       componentsToRender, //< requested comps
                                                                                       inputPrefDepth,
                                                                                       false,
                                                                                       effect.get()));

                                    EffectInstance::RenderRoIRetCode ret;
                                    ret = inputEffect->renderRoI(*renderArgs, &inputImgs); //< requested bitdepth
                                    if (ret != EffectInstance::eRenderRoIRetCodeOk) {
                                        return ret;
                                    }
                                }
                                for (ImageList::iterator it3 = inputImgs.begin(); it3 != inputImgs.end(); ++it3) {
                                    if (inputImagesList && *it3) {
                                        inputImagesList->push_back(*it3);
                                    }
                                }
                                
                                if (effect->aborted()) {
                                    return EffectInstance::eRenderRoIRetCodeAborted;
                                }
                                
                                if (!inputImgs.empty()) {
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
}

StatusEnum EffectInstance::getInputsRoIsFunctor(bool useTransforms,
                                                double time,
                                                int view,
                                                unsigned originalMipMapLevel,
                                                const NodePtr& node,
                                                const NodePtr& callerNode,
                                                const NodePtr& treeRoot,
                                                const RectD& canonicalRenderWindow,
                                                FrameRequestMap& requests)
{
    
    boost::shared_ptr<NodeFrameRequest> nodeRequest;
    
    EffectInstPtr effect = node->getEffectInstance();
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
    if (foundNode != requests.end()) {
        nodeRequest = foundNode->second;
    } else {
        
        ///Setup global data for the node for the whole frame render
        
        boost::shared_ptr<NodeFrameRequest> tmp(new NodeFrameRequest);
        tmp->mappedScale.x = tmp->mappedScale.y = Image::getScaleFromMipMapLevel(mappedLevel);
        tmp->nodeHash = node->getHashValue();
        
        std::pair<FrameRequestMap::iterator,bool> ret = requests.insert(std::make_pair(node, tmp));
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
    
    double par = effect->getPreferredAspectRatio();
    ViewInvarianceLevel viewInvariance = effect->isViewInvariant();

    
    if (foundFrameView != nodeRequest->frames.end()) {
        fvRequest = &foundFrameView->second;
    } else {
        
        ///Set up global data specific for this frame view, this is the first time it has been requested so far
        
        
        fvRequest = &nodeRequest->frames[frameView];
        
        ///Get the RoD
        StatusEnum stat = effect->getRegionOfDefinition_public(nodeRequest->nodeHash, time, nodeRequest->mappedScale, view, &fvRequest->globalData.rod, &fvRequest->globalData.isProjectFormat);
        //If failed it should have failed earlier
        if (stat == eStatusFailed && !fvRequest->globalData.rod.isNull()) {
            return stat;
        }
        
        
        ///Check identity
        fvRequest->globalData.identityInputNb = -1;
        fvRequest->globalData.inputIdentityTime = 0.;
        
        RectI pixelRod;
        fvRequest->globalData.rod.toPixelEnclosing(mappedLevel, par, &pixelRod);
        
        
        if (view != 0 && viewInvariance == eViewInvarianceAllViewsInvariant) {
            fvRequest->globalData.isIdentity = true;
            fvRequest->globalData.identityInputNb = -2;
            fvRequest->globalData.inputIdentityTime = time;
        } else {
            try {
                fvRequest->globalData.isIdentity = effect->isIdentity_public(true, nodeRequest->nodeHash, time, nodeRequest->mappedScale, pixelRod, view, &fvRequest->globalData.inputIdentityTime, &fvRequest->globalData.identityInputNb);
            } catch (...) {
                return eStatusFailed;
            }
        }
        
        
        ///Concatenate transforms if needed
        if (useTransforms) {
            fvRequest->globalData.transforms.reset(new InputMatrixMap);
            effect->tryConcatenateTransforms(time, view, nodeRequest->mappedScale, fvRequest->globalData.transforms.get());
        }
        
        ///Get the frame/views needed for this frame/view
        fvRequest->globalData.frameViewsNeeded = effect->getFramesNeeded_public(nodeRequest->nodeHash, time, view, mappedLevel);
        
        
        
        
    } // if (foundFrameView != nodeRequest->frames.end()) {
    
    assert(fvRequest);
    
    
     if (fvRequest->globalData.identityInputNb == -2) {
        assert(fvRequest->globalData.inputIdentityTime != time || viewInvariance == eViewInvarianceAllViewsInvariant);
        // be safe in release mode otherwise we hit an infinite recursion
        if (fvRequest->globalData.inputIdentityTime != time || viewInvariance == eViewInvarianceAllViewsInvariant) {
            
            fvRequest->requests.push_back(std::make_pair(canonicalRenderWindow, FrameViewPerRequestData()));
            
            int inputView = (view != 0 && viewInvariance == eViewInvarianceAllViewsInvariant) ? 0 : view;
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
        
        EffectInstPtr inputEffectIdentity = effect->getInput(fvRequest->globalData.identityInputNb);
        if (inputEffectIdentity) {
            fvRequest->requests.push_back(std::make_pair(canonicalRenderWindow, FrameViewPerRequestData()));
            
            NodePtr inputIdentityNode = inputEffectIdentity->getNode();
            StatusEnum stat = getInputsRoIsFunctor(useTransforms,
                                                   fvRequest->globalData.inputIdentityTime,
                                                   view,
                                                   originalMipMapLevel,
                                                   inputIdentityNode,
                                                   node,
                                                   treeRoot,
                                                   canonicalRenderWindow,
                                                   requests);
            return stat;
        }
        
        //Identity but no input, if it's optional ignore, otherwise fail
        if (callerNode && callerNode != node) {
            
            int inputIndex = callerNode->getInputIndex(node.get());
            if (callerNode->getEffectInstance()->isInputOptional(inputIndex)
                || callerNode->getEffectInstance()->isInputMask(inputIndex)) {
                
                
                return eStatusOK;
            }
        }
        return eStatusFailed;
    }
    
    ///Compute the regions of interest in input for this RoI
    FrameViewPerRequestData fvPerRequestData;
    effect->getRegionsOfInterest_public(time, nodeRequest->mappedScale, fvRequest->globalData.rod, canonicalRenderWindow, view, &fvPerRequestData.inputsRoi);
    
    
    
    ///Transform Rois and get the reroutes map
    if (useTransforms) {
        if (fvRequest->globalData.transforms) {
            fvRequest->globalData.reroutesMap.reset(new std::map<int, EffectInstPtr>());
            transformInputRois(effect.get(),fvRequest->globalData.transforms,par,nodeRequest->mappedScale,&fvPerRequestData.inputsRoi,fvRequest->globalData.reroutesMap.get());
        }
    }
    
    /*qDebug() << node->getFullyQualifiedName().c_str() << "RoI request: x1="<<canonicalRenderWindow.x1<<"y1="<<canonicalRenderWindow.y1<<"x2="<<canonicalRenderWindow.x2<<"y2="<<canonicalRenderWindow.y2;
    qDebug() << "Input RoIs:";
    for (RoIMap::iterator it = fvPerRequestData.inputsRoi.begin(); it!=fvPerRequestData.inputsRoi.end(); ++it) {
        qDebug() << it->first->getNode()->getFullyQualifiedName().c_str()<<"x1="<<it->second.x1<<"y1="<<it->second.y1<<"x2="<<it->second.x2<<"y2"<<it->second.y2;
    }*/
    ///Append the request
    fvRequest->requests.push_back(std::make_pair(canonicalRenderWindow, fvPerRequestData));
    
    EffectInstance::RenderRoIRetCode ret = treeRecurseFunctor(false,
                                                              node,
                                                              fvRequest->globalData.frameViewsNeeded,
                                                              fvPerRequestData.inputsRoi,
                                                              fvRequest->globalData.transforms,
                                                              useTransforms,
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
}


StatusEnum
EffectInstance::computeRequestPass(double time,
                                   int view,
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
            
            const std::list<std::pair<RectD,FrameViewPerRequestData> >& rois = it2->second.requests;
            
            bool finalRoISet = false;
            for (std::list<std::pair<RectD,FrameViewPerRequestData> >::const_iterator it3 = rois.begin(); it3 != rois.end(); ++it3) {
                if (finalRoISet) {
                    if (!it3->first.isNull()) {
                        it2->second.finalData.finalRoi.merge(it3->first);
                    }
                } else {
                    finalRoISet = true;
                    if (!it3->first.isNull()) {
                        it2->second.finalData.finalRoi = it3->first;
                    }
                }
            }
        }
    }
    
    return eStatusOK;
}

const FrameViewRequest*
NodeFrameRequest::getFrameViewRequest(double time, int view) const
{

    for (NodeFrameViewRequestData::const_iterator it = frames.begin(); it != frames.end();++it) {
        if (it->first.time == time) {
            if (it->first.view == -1 || it->first.view == view) {
                return &it->second;
            }
        }
    }
    return 0;
}




bool
NodeFrameRequest::getFrameViewCanonicalRoI(double time, int view, RectD* roi) const
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
    NodePtr node;
    bool recursed;
    
    FindDependenciesNode() : node(), recursed(false) {}
};

struct FindDependenciesNode_compare
{
    bool operator() (const FindDependenciesNode & lhs,
                     const FindDependenciesNode & rhs) const
    {
        return lhs.node < rhs.node;
    }
};

typedef std::set<FindDependenciesNode,FindDependenciesNode_compare> FindDependenciesSet;

/**
 * @brief Builds a list with all nodes upstream of the given node (including this node) and all its dependencies through expressions as well (which
 * also may be recursive)
 **/
static void getAllUpstreamNodesRecursiveWithDependencies_internal(const NodePtr& node, FindDependenciesSet& finalNodes)
{
    //There may be cases where nodes gets added to the finalNodes in getAllExpressionDependenciesRecursive(), but we still
    //want to recurse upstream for them too
    bool foundButDidntRecursivelyCallUpstream = false;
    
    if (!node || !node->isNodeCreated()) {
        return;
    }
    
    for (FindDependenciesSet::iterator it = finalNodes.begin(); it!=finalNodes.end(); ++it) {
        if (it->node == node) {
            if ( it->recursed) {
                //We already called getAllUpstreamNodesRecursiveWithDependencies on its inputs
                return;
            } else {
                //Now we set the recurse flag below
                finalNodes.erase(it);
                foundButDidntRecursivelyCallUpstream = true;
                break;
            }
        }
    }
 
    {
        //Add this node to the set
        FindDependenciesNode n;
        n.node = node;
        n.recursed = true;
        finalNodes.insert(n);
    }
    
    //If we already called it, don't do it again
    if (!foundButDidntRecursivelyCallUpstream) {
        
        std::set<NodePtr> expressionsDeps;
        node->getEffectInstance()->getAllExpressionDependenciesRecursive(expressionsDeps);
        
        //Also add all expression dependencies but mark them as we did not recursed on them yet
        for (std::set<NodePtr>::iterator it = expressionsDeps.begin(); it!=expressionsDeps.end(); ++it) {
            FindDependenciesNode n;
            n.node = node;
            n.recursed = false;
            finalNodes.insert(n);
        }
    }
   
    int maxInputs = node->getMaxInputCount();
    for (int i = 0; i < maxInputs; ++i) {
        NodePtr inputNode = node->getInput(i);
        if (inputNode) {
            getAllUpstreamNodesRecursiveWithDependencies_internal(inputNode, finalNodes);
        }
    }
    
}

static void getAllUpstreamNodesRecursiveWithDependencies(const NodePtr& node, NodesList& finalNodes)
{
    FindDependenciesSet tmp;
    getAllUpstreamNodesRecursiveWithDependencies_internal(node, tmp);
    for (FindDependenciesSet::iterator it = tmp.begin(); it!=tmp.end(); ++it) {
        finalNodes.push_back(it->node);
    }
}


ParallelRenderArgsSetter::ParallelRenderArgsSetter(double time,
                                                   int view,
                                                   bool isRenderUserInteraction,
                                                   bool isSequential,
                                                   bool canAbort,
                                                   U64 renderAge,
                                                   const NodePtr& treeRoot,
                                                   const FrameRequestMap* request,
                                                   int textureIndex,
                                                   const TimeLine* timeline,
                                                   const NodePtr& activeRotoPaintNode,
                                                   bool isAnalysis,
                                                   bool draftMode,
                                                   bool viewerProgressReportEnabled,
                                                   const boost::shared_ptr<RenderStats>& stats)
:  argsMap()
{
    assert(treeRoot);
    
    bool doNanHandling = appPTR->getCurrentSettings()->isNaNHandlingEnabled();
    
    getAllUpstreamNodesRecursiveWithDependencies(treeRoot, nodes);
    
    for (NodesList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        assert(*it);
        
        EffectInstPtr liveInstance = (*it)->getEffectInstance();
        assert(liveInstance);
        bool duringPaintStrokeCreation = activeRotoPaintNode && (*it)->isDuringPaintStrokeCreation();
        RenderSafetyEnum safety = (*it)->getCurrentRenderThreadSafety();
        
        NodesList rotoPaintNodes;
        boost::shared_ptr<RotoContext> roto = (*it)->getRotoContext();
        if (roto) {
            roto->getRotoPaintTreeNodes(&rotoPaintNodes);
        }
        
        {
            U64 nodeHash = 0;
            bool hashSet = false;
            boost::shared_ptr<NodeFrameRequest> nodeRequest;
            if (request) {
                FrameRequestMap::const_iterator foundRequest = request->find(*it);
                if (foundRequest != request->end()) {
                    nodeRequest = foundRequest->second;
                    nodeHash = nodeRequest->nodeHash;
                    hashSet = true;
                }
            }
            if (!hashSet) {
                nodeHash = (*it)->getHashValue();
            }
            
            liveInstance->setParallelRenderArgsTLS(time, view, isRenderUserInteraction, isSequential, canAbort, nodeHash,
                                                   renderAge,treeRoot, nodeRequest,textureIndex, timeline, isAnalysis,duringPaintStrokeCreation, rotoPaintNodes, safety, doNanHandling, draftMode, viewerProgressReportEnabled, stats);
        }
        for (NodesList::iterator it2 = rotoPaintNodes.begin(); it2 != rotoPaintNodes.end(); ++it2) {
            
            boost::shared_ptr<NodeFrameRequest> childRequest;
            U64 nodeHash = 0;
            bool hashSet = false;
            if (request) {
                FrameRequestMap::const_iterator foundRequest = request->find(*it2);
                if (foundRequest != request->end()) {
                    childRequest = foundRequest->second;
                    nodeHash = childRequest->nodeHash;
                    hashSet = true;
                }
            }
            if (!hashSet) {
                nodeHash = (*it2)->getHashValue();
            }
            
            (*it2)->getEffectInstance()->setParallelRenderArgsTLS(time, view, isRenderUserInteraction, isSequential, canAbort, nodeHash, renderAge, treeRoot, childRequest, textureIndex, timeline, isAnalysis, activeRotoPaintNode && (*it2)->isDuringPaintStrokeCreation(), NodesList(), (*it2)->getCurrentRenderThreadSafety(), doNanHandling, draftMode, viewerProgressReportEnabled,stats);
        }
        
        if ((*it)->isMultiInstance()) {
            
            ///If the node has children, set the thread-local storage on them too, even if they do not render, it can be useful for expressions
            ///on parameters.
            NodesList children;
            (*it)->getChildrenMultiInstance(&children);
            for (NodesList::iterator it2 = children.begin(); it2!=children.end(); ++it2) {
                
                boost::shared_ptr<NodeFrameRequest> childRequest;
                U64 nodeHash = 0;
                bool hashSet = false;
                if (request) {
                    FrameRequestMap::const_iterator foundRequest = request->find(*it2);
                    if (foundRequest != request->end()) {
                        childRequest = foundRequest->second;
                        nodeHash = childRequest->nodeHash;
                        hashSet = true;
                    }
                }
                if (!hashSet) {
                    nodeHash = (*it2)->getHashValue();
                }
                
                assert(*it2);
                EffectInstPtr childLiveInstance = (*it2)->getEffectInstance();
                assert(childLiveInstance);
                RenderSafetyEnum childSafety = (*it2)->getCurrentRenderThreadSafety();
                childLiveInstance->setParallelRenderArgsTLS(time, view, isRenderUserInteraction, isSequential, canAbort, nodeHash, renderAge,treeRoot, childRequest, textureIndex, timeline, isAnalysis, false, NodesList(), childSafety, doNanHandling, draftMode, viewerProgressReportEnabled,stats);
                
            }
        }
        
        
       /* NodeGroup* isGrp = (*it)->isEffectGroup();
        if (isGrp) {
            isGrp->setParallelRenderArgs(time, view, isRenderUserInteraction, isSequential, canAbort,  renderAge, treeRoot, request, textureIndex, timeline, activeRotoPaintNode, isAnalysis, draftMode, viewerProgressReportEnabled,stats);
        }*/
        
    }
    
}

ParallelRenderArgsSetter::ParallelRenderArgsSetter(const boost::shared_ptr<std::map<NodePtr,boost::shared_ptr<ParallelRenderArgs> > >& args)
: argsMap(args)
{
    if (args) {
        for (std::map<NodePtr,boost::shared_ptr<ParallelRenderArgs> >::iterator it = argsMap->begin(); it != argsMap->end(); ++it) {
            it->first->getEffectInstance()->setParallelRenderArgsTLS(it->second);
        }
    }
}

ParallelRenderArgsSetter::~ParallelRenderArgsSetter()
{
    
    for (NodesList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        if (!(*it) || !(*it)->getEffectInstance()) {
            continue;
        }
        (*it)->getEffectInstance()->invalidateParallelRenderArgsTLS();
        
        if ((*it)->isMultiInstance()) {
            
            ///If the node has children, set the thread-local storage on them too, even if they do not render, it can be useful for expressions
            ///on parameters.
            NodesList children;
            (*it)->getChildrenMultiInstance(&children);
            for (NodesList::iterator it2 = children.begin(); it2!=children.end(); ++it2) {
                (*it2)->getEffectInstance()->invalidateParallelRenderArgsTLS();
                
            }
        }

       /* NodeGroup* isGrp = (*it)->isEffectGroup();
        if (isGrp) {
            isGrp->invalidateParallelRenderArgs();
        }*/
    }
    
    if (argsMap) {
        for (std::map<NodePtr,boost::shared_ptr<ParallelRenderArgs> >::iterator it = argsMap->begin(); it != argsMap->end(); ++it) {
            it->first->getEffectInstance()->invalidateParallelRenderArgsTLS();
        }
    }
}

NATRON_NAMESPACE_EXIT;
