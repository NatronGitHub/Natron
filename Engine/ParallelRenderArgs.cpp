/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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

#include "ParallelRenderArgs.h"

#include "Engine/AppManager.h"
#include "Engine/Settings.h"
#include "Engine/EffectInstance.h"
#include "Engine/Image.h"
#include "Engine/Node.h"

using namespace Natron;

Natron::EffectInstance::RenderRoIRetCode EffectInstance::treeRecurseFunctor(bool isRenderFunctor,
                                                                            const boost::shared_ptr<Natron::Node>& node,
                                                                            const FramesNeededMap& framesNeeded,
                                                                            const RoIMap& inputRois,
                                                                            const std::map<int, Natron::EffectInstance*>& reroutesMap,
                                                                            bool useTransforms, // roi functor specific
                                                                            const RectI* originalRenderWindow, // roi functor specific
                                                                            unsigned int originalMipMapLevel, // roi functor specific
                                                                            const boost::shared_ptr<Natron::Node>& treeRoot,
                                                                            FrameRequestMap* requests,  // roi functor specific
                                                                            EffectInstance::InputImagesMap* inputImages, // render functor specific
                                                                            const EffectInstance::ComponentsNeededMap* neededComps, // render functor specific
                                                                            bool useScaleOneInputs, // render functor specific
                                                                            bool byPassCache) // render functor specific
{
    EffectInstance* effect = node->getLiveInstance();
    
    ///For all frames/views needed, call recursively on inputs with the appropriate RoI
    for (FramesNeededMap::const_iterator it = framesNeeded.begin(); it != framesNeeded.end(); ++it) {
        
        bool inputIsMask = effect->isInputMask(it->first);
        
        Natron::ImageComponents maskComps;
        int channelForAlphaInput;
        boost::shared_ptr<Node> maskInput;
        if (inputIsMask) {
            if (!effect->isMaskEnabled(it->first)) {
                continue;
            }
            channelForAlphaInput = effect->getMaskChannel(it->first,&maskComps,&maskInput);
        } else {
            channelForAlphaInput = -1;
        }
        
        //No mask
        if (inputIsMask && (channelForAlphaInput == -1 || maskComps.getNumComponents() == 0)) {
            continue;
        }
        
        //Redirect for transforms if needed
        EffectInstance* inputEffect;
        std::map<int, EffectInstance*>::const_iterator foundReroute = reroutesMap.find(it->first);
        if (foundReroute != reroutesMap.end()) {
            inputEffect = foundReroute->second->getInput(it->first);
        } else {
            inputEffect = node->getLiveInstance()->getInput(it->first);
        }
        
        //Redirect the mask input
        if (maskInput) {
            inputEffect = maskInput->getLiveInstance();
        }
        
        //Never pre-render the mask if we are rendering a node of the rotopaint tree
        if (node->getAttachedRotoItem() && inputEffect && inputEffect->isRotoPaintNode()) {
            continue;
        }
        
        if (inputEffect) {
            
            boost::shared_ptr<Node> inputNode = inputEffect->getNode();
            assert(inputNode);
            
            ImageList* inputImagesList = 0;
            if (isRenderFunctor) {
                EffectInstance::InputImagesMap::iterator foundInputImages = inputImages->find(it->first);
                if (foundInputImages == inputImages->end()) {
                    std::pair<EffectInstance::InputImagesMap::iterator,bool> ret = inputImages->insert(std::make_pair(it->first, ImageList()));
                    inputImagesList = &ret.first->second;
                    assert(ret.second);
                }
            }
            
            ///What region are we interested in for this input effect ? (This is in Canonical coords)
            RoIMap::const_iterator foundInputRoI = inputRois.find(inputEffect);
            if(foundInputRoI == inputRois.end()) {
                continue;
            }
            
            ///There cannot be frames needed without components needed.
            const std::vector<Natron::ImageComponents>* compsNeeded = 0;
            
            if (neededComps) {
                EffectInstance::ComponentsNeededMap::const_iterator foundCompsNeeded = neededComps->find(it->first);
                if (foundCompsNeeded == neededComps->end()) {
                    continue;
                } else {
                    compsNeeded = &foundCompsNeeded->second;
                }
            }

            
            if (foundInputRoI->second.isInfinite()) {
                std::stringstream ss;
                ss << node->getScriptName_mt_safe();
                ss << QObject::tr(" asked for an infinite region of interest upstream").toStdString();
                effect->setPersistentMessage(Natron::eMessageTypeError, ss.str());
                return EffectInstance::eRenderRoIRetCodeFailed;
            }
            
            if (foundInputRoI->second.isNull()) {
                continue;
            }
            
            RectI inputRoIPixelCoords;
            if (isRenderFunctor) {
                const double inputPar = inputEffect->getPreferredAspectRatio();
                foundInputRoI->second.toPixelEnclosing(useScaleOneInputs ? 0 : originalMipMapLevel, inputPar, &inputRoIPixelCoords);
            }
            
            
            ///Notify the node that we're going to render something with the input
            assert(it->first != -1); //< see getInputNumber
            
            {
                boost::shared_ptr<EffectInstance::NotifyInputNRenderingStarted_RAII> inputNIsRendering_RAII;
                if (isRenderFunctor) {
                    inputNIsRendering_RAII.reset(new EffectInstance::NotifyInputNRenderingStarted_RAII(node.get(),it->first));
                }
                
                ///For all views requested in input
                for (std::map<int, std::vector<OfxRangeD> >::const_iterator viewIt = it->second.begin(); viewIt != it->second.end(); ++viewIt) {
                    
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
                                    Natron::StatusEnum stat = EffectInstance::getInputsRoIsFunctor(useTransforms,
                                                                                                   f,
                                                                                                   viewIt->first,
                                                                                                   originalMipMapLevel,
                                                                                                   inputNode,
                                                                                                   treeRoot,
                                                                                                   *originalRenderWindow,
                                                                                                   foundInputRoI->second,
                                                                                                   *requests);
                                    
                                    if (stat == eStatusFailed) {
                                        return EffectInstance::eRenderRoIRetCodeFailed;
                                    }
                                    
                                    ///Do not count frames pre-fetched in RoI functor mode, it is harmless and may
                                    ///limit calculations that will be done later on anyway.
                                    
                                } else {
                                    
                                    RenderScale scaleOne;
                                    scaleOne.x = scaleOne.y = 1.;
                                    
                                    RenderScale scale;
                                    scale.x = scale.y = Natron::Image::getScaleFromMipMapLevel(originalMipMapLevel);
                                    
                                    ///Render the input image with the bit depth of its preference
                                    std::list<Natron::ImageComponents> inputPrefComps;
                                    Natron::ImageBitDepthEnum inputPrefDepth;
                                    inputEffect->getPreferredDepthAndComponents(-1/*it2->first*/, &inputPrefComps, &inputPrefDepth);
                                    std::list<ImageComponents> componentsToRender;
                                    
                                    assert(compsNeeded);
                                    
                                    for (U32 k = 0; k < compsNeeded->size(); ++k) {
                                        if (compsNeeded->at(k).getNumComponents() > 0) {
                                            componentsToRender.push_back(compsNeeded->at(k));
                                        }
                                    }
                                    
                                    EffectInstance::RenderRoIArgs inArgs(f, //< time
                                                                         useScaleOneInputs ? scaleOne : scale, //< scale
                                                                         useScaleOneInputs ? 0 : originalMipMapLevel, //< mipmapLevel (redundant with the scale)
                                                                         viewIt->first, //< view
                                                                         byPassCache,
                                                                         inputRoIPixelCoords, //< roi in pixel coordinates
                                                                         RectD(), // < did we precompute any RoD to speed-up the call ?
                                                                         componentsToRender, //< requested comps
                                                                         inputPrefDepth,
                                                                         effect);
                                    
                                    
                                    
                                    ImageList inputImgs;
                                    EffectInstance::RenderRoIRetCode ret = inputEffect->renderRoI(inArgs, &inputImgs); //< requested bitdepth
                                    if (ret != EffectInstance::eRenderRoIRetCodeOk) {
                                        return ret;
                                    }
                                    
                                    for (ImageList::iterator it3 = inputImgs.begin(); it3 != inputImgs.end(); ++it3) {
                                        if (*it3) {
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
            
        } // if (inputEffect) {
        
    } // for all inputs
    return EffectInstance::eRenderRoIRetCodeOk;
}

Natron::StatusEnum Natron::EffectInstance::getInputsRoIsFunctor(bool useTransforms,
                                                                double time,
                                                                int view,
                                                                unsigned originalMipMapLevel,
                                                                const boost::shared_ptr<Natron::Node>& node,
                                                                const boost::shared_ptr<Natron::Node>& treeRoot,
                                                                const RectI& originalRenderWindow,
                                                                const RectD& canonicalRenderWindow,
                                                                FrameRequestMap& requests)
{
    
    boost::shared_ptr<NodeFrameRequest> nodeRequest;
    
    EffectInstance* effect = node->getLiveInstance();
    assert(effect);
    
    bool supportsRs = effect->supportsRenderScale();
    unsigned int mappedLevel = supportsRs ? originalMipMapLevel : 0;
    
    FrameRequestMap::iterator foundNode = requests.find(node);
    if (foundNode != requests.end()) {
        nodeRequest = foundNode->second;
    } else {
        
        ///Setup global data for the node for the whole frame render
        
        boost::shared_ptr<NodeFrameRequest> tmp(new NodeFrameRequest);
        tmp->mappedScale.x = tmp->mappedScale.y = Natron::Image::getScaleFromMipMapLevel(mappedLevel);
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
    
    if (foundFrameView != nodeRequest->frames.end()) {
        fvRequest = &foundFrameView->second;
    } else {
        
        ///Set up global data specific for this frame view, this is the first time it has been requested so far
        
        FrameViewRequest tmpFvRequest;
        
        ///Get the RoD
        Natron::StatusEnum stat = effect->getRegionOfDefinition_public(nodeRequest->nodeHash, time, nodeRequest->mappedScale, view, &tmpFvRequest.globalData.rod, &tmpFvRequest.globalData.isProjectFormat);
        //If failed it should have failed earlier
        if (stat == eStatusFailed) {
            return stat;
        }
        
        ///Concatenate transforms if needed
        if (useTransforms) {
            effect->tryConcatenateTransforms(time, view, nodeRequest->mappedScale, &tmpFvRequest.globalData.transforms);
        }
        
        ///Get the frame/views needed for this frame/view
        tmpFvRequest.globalData.frameViewsNeeded = effect->getFramesNeeded_public(nodeRequest->nodeHash, time, view, mappedLevel);
        
        std::pair<NodeFrameViewRequestData::iterator,bool> ret = nodeRequest->frames.insert(std::make_pair(frameView, tmpFvRequest));
        assert(ret.second);
        fvRequest = &ret.first->second;
        
        
    } // if (foundFrameView != nodeRequest->frames.end()) {
    
    assert(fvRequest);
    RectD realCanonicalRoI;
    double par = effect->getPreferredAspectRatio();
    
    if (node != treeRoot) {
        realCanonicalRoI = canonicalRenderWindow;
    } else {
        originalRenderWindow.toCanonical(mappedLevel, par , fvRequest->globalData.rod, &realCanonicalRoI);
    }
    
    ///Compute the regions of interest in input for this RoI
    
    FrameViewPerRequestData fvPerRequestData;
    effect->getRegionsOfInterest_public(time, nodeRequest->mappedScale, fvRequest->globalData.rod, realCanonicalRoI, view, &fvPerRequestData.inputsRoi);
    
    ///Transform Rois and get the reroutes map
    std::map<int, Natron::EffectInstance*> reroutesMap;
    if (useTransforms) {
        transformInputRois(effect,fvRequest->globalData.transforms,par,nodeRequest->mappedScale,&fvPerRequestData.inputsRoi,&reroutesMap);
    }
    
    
    ///Append the request
    fvRequest->requests.push_back(std::make_pair(canonicalRenderWindow, fvPerRequestData));
    
    EffectInstance::RenderRoIRetCode ret = treeRecurseFunctor(false,
                                                              node,
                                                              fvRequest->globalData.frameViewsNeeded,
                                                              fvPerRequestData.inputsRoi,
                                                              reroutesMap,
                                                              useTransforms,
                                                              &originalRenderWindow,
                                                              originalMipMapLevel,
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


Natron::StatusEnum
EffectInstance::computeRequestPass(double time,
                                   int view,
                                   unsigned int mipMapLevel,
                                   const RectI& renderWindow,
                                   const boost::shared_ptr<Natron::Node>& treeRoot,
                                   FrameRequestMap& request)
{
    bool doTransforms = appPTR->getCurrentSettings()->isTransformConcatenationEnabled();
    Natron::StatusEnum stat = getInputsRoIsFunctor(doTransforms,
                                                   time,
                                                   view,
                                                   mipMapLevel,
                                                   treeRoot,
                                                   treeRoot,
                                                   renderWindow,
                                                   RectD(),
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
