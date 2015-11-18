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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "DiskCacheNode.h"

#include <stdexcept>

#include "Engine/Node.h"
#include "Engine/Image.h"
#include "Engine/AppInstance.h"
#include "Engine/KnobTypes.h"
#include "Engine/TimeLine.h"

using namespace Natron;

struct DiskCacheNodePrivate
{
    boost::weak_ptr<KnobChoice> frameRange;
    boost::weak_ptr<KnobInt> firstFrame;
    boost::weak_ptr<KnobInt> lastFrame;
    boost::weak_ptr<KnobButton> preRender;
    
    DiskCacheNodePrivate()
    {
        
    }
};

DiskCacheNode::DiskCacheNode(boost::shared_ptr<Node> node)
: OutputEffectInstance(node)
, _imp(new DiskCacheNodePrivate())
{
    setSupportsRenderScaleMaybe(eSupportsYes);
}


void
DiskCacheNode::addAcceptedComponents(int /*inputNb*/,std::list<Natron::ImageComponents>* comps)
{
    comps->push_back(ImageComponents::getRGBAComponents());
    comps->push_back(ImageComponents::getRGBComponents());
    comps->push_back(ImageComponents::getAlphaComponents());
}
void
DiskCacheNode::addSupportedBitDepth(std::list<Natron::ImageBitDepthEnum>* depths) const
{
    depths->push_back(Natron::eImageBitDepthFloat);
}

bool
DiskCacheNode::shouldCacheOutput(bool /*isFrameVaryingOrAnimated*/,double /*time*/, int /*view*/) const
{
    return true;
}

void
DiskCacheNode::initializeKnobs()
{
    boost::shared_ptr<KnobPage> page = Natron::createKnob<KnobPage>(this, "Controls");
    
    boost::shared_ptr<KnobChoice> frameRange = Natron::createKnob<KnobChoice>(this, "Frame range");
    frameRange->setName("frameRange");
    frameRange->setAnimationEnabled(false);
    std::vector<std::string> choices;
    choices.push_back("Input frame range");
    choices.push_back("Project frame range");
    choices.push_back("Manual");
    frameRange->populateChoices(choices);
    frameRange->setEvaluateOnChange(false);
    frameRange->setDefaultValue(0);
    page->addKnob(frameRange);
    _imp->frameRange = frameRange;
    
    boost::shared_ptr<KnobInt> firstFrame = Natron::createKnob<KnobInt>(this, "First frame");
    firstFrame->setAnimationEnabled(false);
    firstFrame->setName("firstFrame");
    firstFrame->disableSlider();
    firstFrame->setEvaluateOnChange(false);
    firstFrame->setAddNewLine(false);
    firstFrame->setDefaultValue(1);
    firstFrame->setSecretByDefault(true);
    page->addKnob(firstFrame);
    _imp->firstFrame = firstFrame;
    
    boost::shared_ptr<KnobInt> lastFrame = Natron::createKnob<KnobInt>(this, "Last frame");
    lastFrame->setAnimationEnabled(false);
    lastFrame->setName("LastFrame");
    lastFrame->disableSlider();
    lastFrame->setEvaluateOnChange(false);
    lastFrame->setDefaultValue(100);
    lastFrame->setSecretByDefault(true);
    page->addKnob(lastFrame);
    _imp->lastFrame = lastFrame;
    
    boost::shared_ptr<KnobButton> preRender = Natron::createKnob<KnobButton>(this, "Pre-cache");
    preRender->setName("preRender");
    preRender->setEvaluateOnChange(false);
    preRender->setHintToolTip("Cache the frame range specified by rendering images at zoom-level 100% only.");
    page->addKnob(preRender);
    _imp->preRender = preRender;
    
    
}

void
DiskCacheNode::knobChanged(KnobI* k, Natron::ValueChangedReasonEnum /*reason*/, int /*view*/, double /*time*/,
                           bool /*originatedFromMainThread*/)
{
    if (_imp->frameRange.lock().get() == k) {
        int idx = _imp->frameRange.lock()->getValue();
        switch (idx) {
            case 0:
            case 1:
                _imp->firstFrame.lock()->setSecret(true);
                _imp->lastFrame.lock()->setSecret(true);
                break;
            case 2:
                _imp->firstFrame.lock()->setSecret(false);
                _imp->lastFrame.lock()->setSecret(false);
                break;
            default:
                break;
        }
    } else if (_imp->preRender.lock().get() == k) {
        AppInstance::RenderWork w;
        w.writer = this;
        w.firstFrame = INT_MIN;
        w.lastFrame = INT_MAX;
        w.frameStep = 1;
        std::list<AppInstance::RenderWork> works;
        works.push_back(w);
        getApp()->startWritersRendering(getApp()->isRenderStatsActionChecked(), false, works);
    }
}

void
DiskCacheNode::getFrameRange(double *first,double *last)
{
    int idx = _imp->frameRange.lock()->getValue();
    switch (idx) {
        case 0: {
            EffectInstance* input = getInput(0);
            if (input) {
                input->getFrameRange_public(input->getHash(), first, last);
            }
        } break;
        case 1: {
            getApp()->getFrameRange(first, last);
        } break;
        case 2: {
            *first = _imp->firstFrame.lock()->getValue();
            *last = _imp->lastFrame.lock()->getValue();
        };
        default:
            break;
    }
}

void
DiskCacheNode::getPreferredDepthAndComponents(int /*inputNb*/,std::list<Natron::ImageComponents>* comp,Natron::ImageBitDepthEnum* depth) const
{
    EffectInstance* input = getInput(0);
    if (input) {
        return input->getPreferredDepthAndComponents(-1, comp, depth);
    } else {
        comp->push_back(ImageComponents::getRGBAComponents());
        *depth = eImageBitDepthFloat;
    }
}


Natron::ImagePremultiplicationEnum
DiskCacheNode::getOutputPremultiplication() const
{
    EffectInstance* input = getInput(0);
    if (input) {
        return input->getOutputPremultiplication();
    } else {
        return eImagePremultiplicationPremultiplied;
    }

}

double
DiskCacheNode::getPreferredAspectRatio() const
{
    EffectInstance* input = getInput(0);
    if (input) {
        return input->getPreferredAspectRatio();
    } else {
        return 1.;
    }

}

Natron::StatusEnum
DiskCacheNode::render(const RenderActionArgs& args)
{
    
    assert(args.outputPlanes.size() == 1);
    
    EffectInstance* input = getInput(0);
    if (!input) {
        return eStatusFailed;
    }
    
    ImageBitDepthEnum bitdepth;
    std::list<ImageComponents> components;
    input->getPreferredDepthAndComponents(-1, &components, &bitdepth);
    double par = input->getPreferredAspectRatio();
    
    const std::pair<ImageComponents,ImagePtr>& output = args.outputPlanes.front();
    
    for (std::list<ImageComponents> ::const_iterator it =components.begin(); it != components.end(); ++it) {
        RectI roiPixel;
        
        ImagePtr srcImg = getImage(0, args.time, args.originalScale, args.view, NULL, *it, bitdepth, par, false, &roiPixel);
        if (!srcImg) {
            return eStatusFailed;
        }
        if (srcImg->getMipMapLevel() != output.second->getMipMapLevel()) {
            throw std::runtime_error("Host gave image with wrong scale");
        }
        if (srcImg->getComponents() != output.second->getComponents() || srcImg->getBitDepth() != output.second->getBitDepth()) {
            
            
            srcImg->convertToFormat(args.roi, getApp()->getDefaultColorSpaceForBitDepth( srcImg->getBitDepth() ),
                                    getApp()->getDefaultColorSpaceForBitDepth(output.second->getBitDepth()), 3, true, false, output.second.get());
        } else {
            output.second->pasteFrom(*srcImg, args.roi, output.second->usesBitMap() && srcImg->usesBitMap());
        }

    }
    
    return eStatusOK;
}

