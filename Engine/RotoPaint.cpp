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
#include <stdexcept>

#include "Engine/AppInstance.h"
#include "Engine/Image.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/RotoContext.h"
#include "Engine/KnobTypes.h"
#include "Engine/RotoDrawableItem.h"
#include "Engine/TimeLine.h"

#define ROTOPAINT_MASK_INPUT_INDEX 10

NATRON_NAMESPACE_ENTER;

struct RotoPaintPrivate
{
    bool isPaintByDefault;
    boost::weak_ptr<KnobBool> premultKnob;
    
    RotoPaintPrivate(bool isPaintByDefault)
    : isPaintByDefault(isPaintByDefault)
    , premultKnob()
    {
        
    }
};

std::string
RotoPaint::getPluginDescription() const
{
    return "RotoPaint is a vector based free-hand drawing node that helps for tasks such as rotoscoping, matting, etc...";
}

RotoPaint::RotoPaint(boost::shared_ptr<Node> node, bool isPaintByDefault)
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
    *defaultR = true;
    *defaultG = true;
    *defaultB = true;
    *defaultA = true;
    return true;
}

bool
RotoNode::isHostChannelSelectorSupported(bool* defaultR,bool* defaultG, bool* defaultB, bool* defaultA) const
{
    *defaultR = false;
    *defaultG = false;
    *defaultB = false;
    *defaultA = true;
    return true;
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

    boost::shared_ptr<KnobSeparator> sep = natronCreateKnob<KnobSeparator>(this, "Output", 1, false);
    generalPage->addKnob(sep);
    
    boost::shared_ptr<KnobBool> premultKnob = natronCreateKnob<KnobBool>(this, "Premultiply", 1, false);
    premultKnob->setName("premultiply");
    premultKnob->setHintToolTip("When checked, the red, green and blue channels in output of this node are premultiplied by the alpha channel."
                                " This will result in the pixels outside of the shapes and paint strokes "
                                "being black and transparant.");
    premultKnob->setDefaultValue(false);
    premultKnob->setAnimationEnabled(false);
    _imp->premultKnob = premultKnob;
    generalPage->addKnob(premultKnob);
    
}

void
RotoPaint::knobChanged(KnobI* k,
                 ValueChangedReasonEnum reason,
                 int view,
                 double time,
                 bool originatedFromMainThread)
{
    boost::shared_ptr<RotoContext> ctx = getNode()->getRotoContext();
    if (!ctx) {
        return;
    }
    ctx->knobChanged(k, reason, view, time, originatedFromMainThread);
}

void
RotoPaint::getPreferredDepthAndComponents(int inputNb,std::list<ImageComponents>* comp,ImageBitDepthEnum* depth) const
{

    if (inputNb != ROTOPAINT_MASK_INPUT_INDEX) {
        EffectInstance* input = getInput(inputNb);
        if (input) {
            std::list<ImageComponents> ic;
            ImageBitDepthEnum id;
            input->getPreferredDepthAndComponents(-1, &ic, &id);
            *comp = ic;
        } else {
            comp->push_back(ImageComponents::getRGBAComponents());
        }
    } else {
        comp->push_back(ImageComponents::getAlphaComponents());
    }
    *depth = eImageBitDepthFloat;
}


ImagePremultiplicationEnum
RotoPaint::getOutputPremultiplication() const
{
  
    EffectInstance* input = getInput(0);
    
    boost::shared_ptr<KnobBool> premultKnob = _imp->premultKnob.lock();
    assert(premultKnob);
    bool premultiply = premultKnob->getValue();
    if (premultiply) {
        return eImagePremultiplicationPremultiplied;
    }
    ImagePremultiplicationEnum srcPremult = eImagePremultiplicationOpaque;
    if (input) {
        srcPremult = input->getOutputPremultiplication();
    }
    bool processA = getNode()->getProcessChannel(3);
    if (srcPremult == eImagePremultiplicationOpaque && processA) {
        return eImagePremultiplicationUnPremultiplied;
    }
    return eImagePremultiplicationPremultiplied;
}

double
RotoPaint::getPreferredAspectRatio() const
{
    EffectInstance* input = getInput(0);
    if (input) {
        return input->getPreferredAspectRatio();
    } else {
        return 1.;
    }
    
}

void
RotoPaint::onInputChanged(int inputNb)
{
    
    boost::shared_ptr<Node> inputNode = getNode()->getInput(0);
    getNode()->getRotoContext()->onRotoPaintInputChanged(inputNode);
    EffectInstance::onInputChanged(inputNb);
    
}

StatusEnum
RotoPaint::getRegionOfDefinition(U64 hash,double time, const RenderScale & scale, int view, RectD* rod)
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
RotoPaint::getFramesNeeded(double time, int view)
{

    FramesNeededMap ret;
    std::map<int, std::vector<OfxRangeD> > views;
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
                          int view,
                          RoIMap* ret)
{
    boost::shared_ptr<RotoContext> roto = getNode()->getRotoContext();
    boost::shared_ptr<Node> bottomMerge = roto->getRotoPaintBottomMergeNode();
    if (bottomMerge) {
        ret->insert(std::make_pair(bottomMerge->getLiveInstance(), renderWindow));
    }
    EffectInstance::getRegionsOfInterest(time, scale, outputRoD, renderWindow, view, ret);
}

bool
RotoPaint::isIdentity(double time,
                      const RenderScale & scale,
                      const RectI & roi,
                      int view,
                      double* inputTime,
                      int* inputNb)
{
    boost::shared_ptr<Node> node = getNode();
    EffectInstance* maskInput = getInput(ROTOPAINT_MASK_INPUT_INDEX);
    if (maskInput) {
        
        RectD maskRod;
        bool isProjectFormat;
        StatusEnum s = maskInput->getRegionOfDefinition_public(maskInput->getRenderHash(), time, scale, view, &maskRod, &isProjectFormat);
        Q_UNUSED(s);
        RectI maskPixelRod;
        maskRod.toPixelEnclosing(scale, getPreferredAspectRatio(), &maskPixelRod);
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
    
    std::list<ImageComponents> bgComps;
    ImageBitDepthEnum bgDepth;
    getPreferredDepthAndComponents(0, &bgComps, &bgDepth);
    assert(!bgComps.empty());

    
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
        ImagePtr bgImg = getImage(0, args.time, args.mappedScale, args.view, 0, bgComps.front(), bgDepth, getPreferredAspectRatio(), false, true, &bgImgRoI);
        
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
        
        
        NodeList rotoPaintNodes;
        {
            bool ok = getThreadLocalRotoPaintTreeNodes(&rotoPaintNodes);
            if (!ok) {
                throw std::logic_error("RotoPaint::render(): getThreadLocalRotoPaintTreeNodes() failed");
            }
        }
        
        boost::shared_ptr<Node> bottomMerge = roto->getRotoPaintBottomMergeNode();
        
        
        RenderingFlagSetter flagIsRendering(bottomMerge.get());

        
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
        ImageList rotoPaintImages;
        RenderRoIRetCode code = bottomMerge->getLiveInstance()->renderRoI(rotoPaintArgs, &rotoPaintImages);
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
        
        bool triedGetImage = false;
        //bgImg = getImage(0, args.time, args.mappedScale, args.view, 0, bgComps.front(), bgDepth, getPreferredAspectRatio(), false, &bgImgRoI);
        
        ImageList::iterator rotoImagesIt = rotoPaintImages.begin();
        for (std::list<std::pair<ImageComponents,boost::shared_ptr<Image> > >::const_iterator plane = args.outputPlanes.begin();
             plane != args.outputPlanes.end(); ++plane, ++rotoImagesIt) {
            
            if (!(*rotoImagesIt)->getBounds().contains(args.roi)) {
                if (!bgImg) {
                    if (!triedGetImage) {
                        bgImg = getImage(0, args.time, args.mappedScale, args.view, 0, bgComps.front(), bgDepth, getPreferredAspectRatio(), false, true, &bgImgRoI);
                        triedGetImage = true;
                    }
                }
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
                                               getApp()->getDefaultColorSpaceForBitDepth((*rotoImagesIt)->getBitDepth()),
                                               getApp()->getDefaultColorSpaceForBitDepth(plane->second->getBitDepth()), 3
                                               , false, false, plane->second.get());
                    } else {
                        plane->second->pasteFrom(*bgImg, args.roi, false);
                    }
                    
                } else {
                    plane->second->fillZero(args.roi);
                }
            }
           
            
           
            if ((*rotoImagesIt)->getComponents() != plane->second->getComponents()) {
                
                (*rotoImagesIt)->convertToFormat(args.roi,
                                                 getApp()->getDefaultColorSpaceForBitDepth((*rotoImagesIt)->getBitDepth()),
                                                 getApp()->getDefaultColorSpaceForBitDepth(plane->second->getBitDepth()), 3
                                                 , false, false, plane->second.get());
            } else {
                plane->second->pasteFrom(**rotoImagesIt, args.roi, false);
            }
            if (premultiply && plane->second->getComponents() == ImageComponents::getRGBAComponents()) {
                plane->second->premultImage(args.roi);
            }
        }
    } // RenderingFlagSetter flagIsRendering(bottomMerge.get());
    
    return eStatusOK;
}

void
RotoPaint::clearLastRenderedImage()
{
    EffectInstance::clearLastRenderedImage();
    NodeList rotoPaintNodes;
    boost::shared_ptr<RotoContext> roto = getNode()->getRotoContext();
    assert(roto);
    roto->getRotoPaintTreeNodes(&rotoPaintNodes);
    for (NodeList::iterator it = rotoPaintNodes.begin(); it!=rotoPaintNodes.end(); ++it) {
        (*it)->clearLastRenderedImage();
    }
}

NATRON_NAMESPACE_EXIT;
