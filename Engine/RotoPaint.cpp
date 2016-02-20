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

#include "RotoPaint.h"

#include <sstream>
#include <cassert>
#include <stdexcept>

#include "Engine/AppInstance.h"
#include "Engine/Image.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/NodeMetadata.h"
#include "Engine/RotoContext.h"
#include "Engine/KnobTypes.h"
#include "Engine/RotoDrawableItem.h"
#include "Engine/TimeLine.h"
#include "Engine/ViewIdx.h"

#define ROTOPAINT_MASK_INPUT_INDEX 10

NATRON_NAMESPACE_ENTER;

struct RotoPaintPrivate
{
    bool isPaintByDefault;
    boost::weak_ptr<KnobBool> premultKnob;
    boost::weak_ptr<KnobBool> enabledKnobs[4];
    
    RotoPaintPrivate(bool isPaintByDefault)
    : isPaintByDefault(isPaintByDefault)
    , premultKnob()
    , enabledKnobs()
    {
        
    }
};

std::string
RotoPaint::getPluginDescription() const
{
    return "RotoPaint is a vector based free-hand drawing node that helps for tasks such as rotoscoping, matting, etc...";
}

RotoPaint::RotoPaint(NodePtr node, bool isPaintByDefault)
: EffectInstance(node)
, _imp(new RotoPaintPrivate(isPaintByDefault))
{
    setSupportsRenderScaleMaybe(eSupportsYes);
}


RotoPaint::~RotoPaint()
{
    
}

bool
RotoPaint::isDefaultBehaviourPaintContext() const {
    return _imp->isPaintByDefault;
}

std::string
RotoPaint::getPluginID() const
{
    return PLUGINID_NATRON_ROTOPAINT;
}

std::string
RotoPaint::getPluginLabel() const
{
    return "RotoPaint";
}

std::string
RotoNode::getPluginID() const
{
    return PLUGINID_NATRON_ROTO;
}

std::string
RotoNode::getPluginLabel() const
{
    return "Roto";
}

std::string
RotoNode::getPluginDescription() const
{
    return "Create masks and shapes";
}

bool
RotoPaint::isHostChannelSelectorSupported(bool* defaultR,bool* defaultG, bool* defaultB, bool* defaultA) const
{
    //Use our own selectors, we don't want Natron to copy back channels
    *defaultR = true;
    *defaultG = true;
    *defaultB = true;
    *defaultA = true;
    return false;
}

bool
RotoNode::isHostChannelSelectorSupported(bool* defaultR,bool* defaultG, bool* defaultB, bool* defaultA) const
{
    *defaultR = true;
    *defaultG = true;
    *defaultB = true;
    *defaultA = false;
    return false;
}

std::string
RotoPaint::getInputLabel (int inputNb) const
{
    if (inputNb == ROTOPAINT_MASK_INPUT_INDEX) {
        return "Mask";
    } else if (inputNb == 0) {
        return "Bg";
    } else {
        std::stringstream ss;
        ss << "Bg" << inputNb + 1;
        return ss.str();
    }
}

bool
RotoPaint::isInputMask(int inputNb) const
{
    return inputNb == ROTOPAINT_MASK_INPUT_INDEX;
}

void
RotoPaint::addAcceptedComponents(int inputNb,std::list<ImageComponents>* comps)
{
    
    if (inputNb != ROTOPAINT_MASK_INPUT_INDEX) {
        comps->push_back(ImageComponents::getRGBAComponents());
        comps->push_back(ImageComponents::getRGBComponents());
        comps->push_back(ImageComponents::getXYComponents());
    }
    comps->push_back(ImageComponents::getAlphaComponents());
}

void
RotoPaint::addSupportedBitDepth(std::list<ImageBitDepthEnum>* depths) const
{
    depths->push_back(eImageBitDepthFloat);
}

void
RotoPaint::initializeKnobs()
{
    //This page is created in the RotoContext, before initializeKnobs() is called.
    boost::shared_ptr<KnobPage> generalPage = boost::dynamic_pointer_cast<KnobPage>(getKnobByName("General"));
    assert(generalPage);
    

    boost::shared_ptr<KnobSeparator> sep = AppManager::createKnob<KnobSeparator>(this, "Output", 1, false);
    generalPage->addKnob(sep);
    
    
    std::string channelNames[4] = {"doRed","doGreen","doBlue","doAlpha"};
    std::string channelLabels[4] = {"R","G","B","A"};
    
    bool defaultValues[4];
    (void)isHostChannelSelectorSupported(&defaultValues[0], &defaultValues[1], &defaultValues[2], &defaultValues[3]);
    for (int i = 0; i < 4; ++i) {
        
        boost::shared_ptr<KnobBool> enabled =  AppManager::createKnob<KnobBool>(this, channelLabels[i], 1, false);
        enabled->setName(channelNames[i]);
        enabled->setAnimationEnabled(false);
        enabled->setAddNewLine(i == 3);
        enabled->setDefaultValue(defaultValues[i]);
        enabled->setHintToolTip("Enable drawing onto this channel");
        generalPage->addKnob(enabled);
        _imp->enabledKnobs[i] = enabled;
    }

    
    boost::shared_ptr<KnobBool> premultKnob = AppManager::createKnob<KnobBool>(this, "Premultiply", 1, false);
    premultKnob->setName("premultiply");
    premultKnob->setHintToolTip("When checked, the red, green and blue channels in output of this node are premultiplied by the alpha channel."
                                " This will result in the pixels outside of the shapes and paint strokes "
                                "being black and transparant.");
    premultKnob->setDefaultValue(false);
    premultKnob->setAnimationEnabled(false);
    premultKnob->setIsMetadataSlave(true);
    _imp->premultKnob = premultKnob;
    generalPage->addKnob(premultKnob);
    
}

void
RotoPaint::knobChanged(KnobI* k,
                       ValueChangedReasonEnum reason,
                       ViewSpec view,
                       double time,
                       bool originatedFromMainThread)
{
    boost::shared_ptr<RotoContext> ctx = getNode()->getRotoContext();
    if (!ctx) {
        return;
    }
    ctx->knobChanged(k, reason, view, time, originatedFromMainThread);
}

StatusEnum
RotoPaint::getPreferredMetaDatas(NodeMetadata& metadata)
{
    metadata.setImageComponents(-1, ImageComponents::getRGBAComponents());
    boost::shared_ptr<KnobBool> premultKnob = _imp->premultKnob.lock();
    assert(premultKnob);
    bool premultiply = premultKnob->getValue();
    if (premultiply) {
        metadata.setOutputPremult(eImagePremultiplicationPremultiplied);
    } else {
        ImagePremultiplicationEnum srcPremult = eImagePremultiplicationOpaque;
        
        EffectInstPtr input = getInput(0);
        if (input) {
            srcPremult = input->getPremult();
        }
        bool processA = getNode()->getProcessChannel(3);
        if (srcPremult == eImagePremultiplicationOpaque && processA) {
            metadata.setOutputPremult(eImagePremultiplicationUnPremultiplied);
        } else {
            metadata.setOutputPremult(eImagePremultiplicationPremultiplied);
        }
    }
    return eStatusOK;
}



void
RotoPaint::onInputChanged(int inputNb)
{
    
    NodePtr inputNode = getNode()->getInput(0);
    getNode()->getRotoContext()->onRotoPaintInputChanged(inputNode);
    EffectInstance::onInputChanged(inputNb);
    
}

StatusEnum
RotoPaint::getRegionOfDefinition(U64 hash,double time, const RenderScale & scale, ViewIdx view, RectD* rod)
{
    StatusEnum st = EffectInstance::getRegionOfDefinition(hash, time, scale, view, rod);
    if (st != eStatusOK) {
        rod->x1 = rod->y1 = rod->x2 = rod->y2 = 0.;
    }
    RectD maskRod;
    getNode()->getRotoContext()->getMaskRegionOfDefinition(time, view, &maskRod);
    if (rod->isNull()) {
        *rod = maskRod;
    } else {
        rod->merge(maskRod);
    }
    return eStatusOK;
}

FramesNeededMap
RotoPaint::getFramesNeeded(double time, ViewIdx view)
{

    FramesNeededMap ret;
    FrameRangesMap views;
    OfxRangeD range;
    range.min = range.max = time;
    views[view].push_back(range);
    ret.insert(std::make_pair(0,views));
    return ret;
}

void
RotoPaint::getRegionsOfInterest(double time,
                          const RenderScale & scale,
                          const RectD & outputRoD, //!< the RoD of the effect, in canonical coordinates
                          const RectD & renderWindow, //!< the region to be rendered in the output image, in Canonical Coordinates
                          ViewIdx view,
                          RoIMap* ret)
{
    boost::shared_ptr<RotoContext> roto = getNode()->getRotoContext();
    NodePtr bottomMerge = roto->getRotoPaintBottomMergeNode();
    if (bottomMerge) {
        ret->insert(std::make_pair(bottomMerge->getEffectInstance(), renderWindow));
    }
    EffectInstance::getRegionsOfInterest(time, scale, outputRoD, renderWindow, view, ret);
}

bool
RotoPaint::isIdentity(double time,
                      const RenderScale & scale,
                      const RectI & roi,
                      ViewIdx view,
                      double* inputTime,
                      int* inputNb)
{
    NodePtr node = getNode();
    EffectInstPtr maskInput = getInput(ROTOPAINT_MASK_INPUT_INDEX);
    if (maskInput) {
        
        RectD maskRod;
        bool isProjectFormat;
        StatusEnum s = maskInput->getRegionOfDefinition_public(maskInput->getRenderHash(), time, scale, view, &maskRod, &isProjectFormat);
        Q_UNUSED(s);
        RectI maskPixelRod;
        maskRod.toPixelEnclosing(scale, getAspectRatio(ROTOPAINT_MASK_INPUT_INDEX), &maskPixelRod);
        if (!maskPixelRod.intersects(roi)) {
            *inputTime = time;
            *inputNb = 0;
            return true;
        }
    }
    
    std::list<boost::shared_ptr<RotoDrawableItem> > items = node->getRotoContext()->getCurvesByRenderOrder();
    if (items.empty()) {
        
        *inputNb = 0;
        *inputTime = time;
        return true;

    }
    return false;
}


StatusEnum
RotoPaint::render(const RenderActionArgs& args)
{
    boost::shared_ptr<RotoContext> roto = getNode()->getRotoContext();
    std::list<boost::shared_ptr<RotoDrawableItem> > items = roto->getCurvesByRenderOrder();
    
    ImageBitDepthEnum bgDepth = getBitDepth(0);

    
    std::list<ImageComponents> neededComps;
    for (std::list<std::pair<ImageComponents,boost::shared_ptr<Image> > >::const_iterator plane = args.outputPlanes.begin();
         plane != args.outputPlanes.end(); ++plane) {
        neededComps.push_back(plane->first);
    }
    
    boost::shared_ptr<KnobBool> premultKnob = _imp->premultKnob.lock();
    assert(premultKnob);
    bool premultiply = premultKnob->getValueAtTime(args.time);
    
    if (items.empty()) {
        
        RectI bgImgRoI;
        ImagePtr bgImg = getImage(0, args.time, args.mappedScale, args.view, 0, 0,false, false, &bgImgRoI);
        
        for (std::list<std::pair<ImageComponents,boost::shared_ptr<Image> > >::const_iterator plane = args.outputPlanes.begin();
             plane != args.outputPlanes.end(); ++plane) {
            
            if (bgImg) {
                if (bgImg->getComponents() != plane->second->getComponents()) {
                    
                    bgImg->convertToFormat(args.roi,
                                           getApp()->getDefaultColorSpaceForBitDepth(bgImg->getBitDepth()),
                                           getApp()->getDefaultColorSpaceForBitDepth(plane->second->getBitDepth()), 3
                                           , false, false, plane->second.get());
                } else {
                    plane->second->pasteFrom(*bgImg, args.roi, false);
                }
                
                
                if (premultiply && plane->second->getComponents() == ImageComponents::getRGBAComponents()) {
                    plane->second->premultImage(args.roi);
                }
            } else {
                plane->second->fillZero(args.roi);
            }
            
        }
    } else {
        
        
        NodesList rotoPaintNodes;
        {
            bool ok = getThreadLocalRotoPaintTreeNodes(&rotoPaintNodes);
            if (!ok) {
                throw std::logic_error("RotoPaint::render(): getThreadLocalRotoPaintTreeNodes() failed");
            }
        }
        
        NodePtr bottomMerge = roto->getRotoPaintBottomMergeNode();
        
        
        RenderingFlagSetter flagIsRendering(bottomMerge.get());

        std::bitset<4> copyChannels;
        for (int i = 0; i < 4; ++i) {
            copyChannels[i] = _imp->enabledKnobs[i].lock()->getValue();
        }
        
        unsigned int mipMapLevel = Image::getLevelFromScale(args.mappedScale.x);
        RenderRoIArgs rotoPaintArgs(args.time,
                                    args.mappedScale,
                                    mipMapLevel,
                                    args.view,
                                    args.byPassCache,
                                    args.roi,
                                    RectD(),
                                    neededComps,
                                    bgDepth,
                                    false,
                                    this);
        std::map<ImageComponents,ImagePtr> rotoPaintImages;
        RenderRoIRetCode code = bottomMerge->getEffectInstance()->renderRoI(rotoPaintArgs, &rotoPaintImages);
        if (code == eRenderRoIRetCodeFailed) {
            return eStatusFailed;
        } else if (code == eRenderRoIRetCodeAborted) {
            return eStatusOK;
        } else if (rotoPaintImages.empty()) {
            for (std::list<std::pair<ImageComponents,boost::shared_ptr<Image> > >::const_iterator plane = args.outputPlanes.begin();
                 plane != args.outputPlanes.end(); ++plane) {
                
                plane->second->fillZero(args.roi);
                
                
            }
            return eStatusOK;
        }
        assert(rotoPaintImages.size() == args.outputPlanes.size());
        
        RectI bgImgRoI;
        ImagePtr bgImg;
        
        ImagePremultiplicationEnum outputPremult = getPremult();
        
        bool triedGetImage = false;
        
        for (std::list<std::pair<ImageComponents,boost::shared_ptr<Image> > >::const_iterator plane = args.outputPlanes.begin();
             plane != args.outputPlanes.end(); ++plane) {
            
            std::map<ImageComponents,ImagePtr>::iterator rotoImagesIt = rotoPaintImages.find(plane->first);
            assert(rotoImagesIt != rotoPaintImages.end());
            if (rotoImagesIt == rotoPaintImages.end()) {
                continue;
            }
            if (!bgImg) {
                if (!triedGetImage) {
                    bgImg = getImage(0, args.time, args.mappedScale, args.view, 0, 0, false, false, &bgImgRoI);
                    triedGetImage = true;
                }
            }
            if (!rotoImagesIt->second->getBounds().contains(args.roi)) {
                
                ///We first fill with the bg image because the bounds of the image produced by the last merge of the rotopaint tree
                ///might not be equal to the bounds of the image produced by the rotopaint. This is because the RoD of the rotopaint is the
                ///union of all the mask strokes bounds, whereas all nodes inside the rotopaint tree don't take the mask RoD into account.
                if (bgImg) {
                    
                    RectI bgBounds = bgImg->getBounds();
                    
                    ///The bg bounds might not be inside the roi, but yet we need to fill the whole roi, so just fill borders
                    ///with black and transparant, e.g:
                    /*
                        AAAAAAAAA
                        DDXXXXXBB
                        DDXXXXXBB
                        DDXXXXXBB
                        CCCCCCCCC
                     */
                    RectI merge = bgBounds;
                    merge.merge(args.roi);
                    RectI aRect;
                    aRect.x1 = merge.x1;
                    aRect.y1 = bgBounds.y2;
                    aRect.y2 = merge.y2;
                    aRect.x2 = merge.x2;
                    
                    RectI bRect;
                    bRect.x1 = bgBounds.x2;
                    bRect.y1 = bgBounds.y1;
                    bRect.x2 = merge.x2;
                    bRect.y2 = bgBounds.y2;
                    
                    RectI cRect;
                    cRect.x1 = merge.x1;
                    cRect.y1 = merge.y1;
                    cRect.x2 = merge.x2;
                    cRect.y2 = bgBounds.y1;
                    
                    RectI dRect;
                    dRect.x1 = merge.x1;
                    dRect.y1 = bgBounds.y1;
                    dRect.x2 = bgBounds.x1;
                    dRect.y2 = bgBounds.y2;
                    
                    plane->second->fillZero(aRect);
                    plane->second->fillZero(bRect);
                    plane->second->fillZero(cRect);
                    plane->second->fillZero(dRect);
                    
                    if (bgImg->getComponents() != plane->second->getComponents()) {
                        RectI intersection;
                        args.roi.intersect(bgImg->getBounds(), &intersection);
                        bgImg->convertToFormat(intersection,
                                               getApp()->getDefaultColorSpaceForBitDepth(rotoImagesIt->second->getBitDepth()),
                                               getApp()->getDefaultColorSpaceForBitDepth(plane->second->getBitDepth()), 3
                                               , false, false, plane->second.get());
                    } else {
                        plane->second->pasteFrom(*bgImg, args.roi, false);
                    }
                    
                } else {
                    plane->second->fillZero(args.roi);
                }
            }
           
            
           
            if (rotoImagesIt->second->getComponents() != plane->second->getComponents()) {
                
                rotoImagesIt->second->convertToFormat(args.roi,
                                                 getApp()->getDefaultColorSpaceForBitDepth(rotoImagesIt->second->getBitDepth()),
                                                 getApp()->getDefaultColorSpaceForBitDepth(plane->second->getBitDepth()), 3
                                                 , false, false, plane->second.get());
            } else {
                plane->second->pasteFrom(*(rotoImagesIt->second), args.roi, false);
            }
            if (premultiply && plane->second->getComponents() == ImageComponents::getRGBAComponents()) {
                plane->second->premultImage(args.roi);
            }
            if (bgImg) {
                plane->second->copyUnProcessedChannels(args.roi, outputPremult, bgImg->getPremultiplication(), copyChannels, bgImg, false);
            }
            
        }
    } // RenderingFlagSetter flagIsRendering(bottomMerge.get());
    
    return eStatusOK;
}

void
RotoPaint::clearLastRenderedImage()
{
    EffectInstance::clearLastRenderedImage();
    NodesList rotoPaintNodes;
    boost::shared_ptr<RotoContext> roto = getNode()->getRotoContext();
    assert(roto);
    roto->getRotoPaintTreeNodes(&rotoPaintNodes);
    for (NodesList::iterator it = rotoPaintNodes.begin(); it!=rotoPaintNodes.end(); ++it) {
        (*it)->clearLastRenderedImage();
    }
}

NATRON_NAMESPACE_EXIT;
