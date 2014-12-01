//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#include "DiskCacheNode.h"
#include "Engine/Node.h"
#include "Engine/Image.h"
#include "Engine/AppInstance.h"
#include "Engine/KnobTypes.h"
#include "Engine/TimeLine.h"

using namespace Natron;

struct DiskCacheNodePrivate
{
    boost::shared_ptr<Choice_Knob> frameRange;
    boost::shared_ptr<Int_Knob> firstFrame;
    boost::shared_ptr<Int_Knob> lastFrame;
    boost::shared_ptr<Button_Knob> preRender;
    
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
DiskCacheNode::addAcceptedComponents(int /*inputNb*/,std::list<Natron::ImageComponentsEnum>* comps)
{
    comps->push_back(Natron::eImageComponentRGBA);
    comps->push_back(Natron::eImageComponentRGB);
    comps->push_back(Natron::eImageComponentAlpha);
}
void
DiskCacheNode::addSupportedBitDepth(std::list<Natron::ImageBitDepthEnum>* depths) const
{
    depths->push_back(Natron::eImageBitDepthFloat);
}

bool
DiskCacheNode::shouldCacheOutput() const
{
    return true;
}

void
DiskCacheNode::initializeKnobs()
{
    boost::shared_ptr<Page_Knob> page = Natron::createKnob<Page_Knob>(this, "Controls");
    
    _imp->frameRange = Natron::createKnob<Choice_Knob>(this, "Frame range");
    _imp->frameRange->setName("frameRange");
    _imp->frameRange->setAnimationEnabled(false);
    std::vector<std::string> choices;
    choices.push_back("Input frame range");
    choices.push_back("Timeline bounds");
    choices.push_back("Manual");
    _imp->frameRange->populateChoices(choices);
    _imp->frameRange->setDefaultValue(0);
    page->addKnob(_imp->frameRange);
    
    _imp->firstFrame = Natron::createKnob<Int_Knob>(this, "First frame");
    _imp->firstFrame->setAnimationEnabled(false);
    _imp->firstFrame->setName("firstFrame");
    _imp->firstFrame->disableSlider();
    _imp->firstFrame->turnOffNewLine();
    _imp->firstFrame->setDefaultValue(1);
    _imp->firstFrame->setSecret(true);
    page->addKnob(_imp->firstFrame);
    
    _imp->lastFrame = Natron::createKnob<Int_Knob>(this, "Last frame");
    _imp->lastFrame->setAnimationEnabled(false);
    _imp->lastFrame->setName("LastFrame");
    _imp->lastFrame->disableSlider();
    _imp->lastFrame->setDefaultValue(100);
    _imp->lastFrame->setSecret(true);
    page->addKnob(_imp->lastFrame);
    
    _imp->preRender = Natron::createKnob<Button_Knob>(this, "Pre-cache");
    _imp->preRender->setName("preRender");
    _imp->preRender->setEvaluateOnChange(false);
    _imp->preRender->setHintToolTip("Cache the frame range specified by rendering images at zoom-level 100% only.");
    page->addKnob(_imp->preRender);
    
    
}

void
DiskCacheNode::knobChanged(KnobI* k, Natron::ValueChangedReasonEnum /*reason*/, int /*view*/, SequenceTime /*time*/)
{
    if (_imp->frameRange.get() == k) {
        int idx = _imp->frameRange->getValue();
        switch (idx) {
            case 0:
            case 1:
                _imp->firstFrame->setSecret(true);
                _imp->lastFrame->setSecret(true);
                break;
            case 2:
                _imp->firstFrame->setSecret(false);
                _imp->lastFrame->setSecret(false);
                break;
            default:
                break;
        }
    } else if (_imp->preRender.get() == k) {
        AppInstance::RenderWork w;
        w.writer = dynamic_cast<OutputEffectInstance*>(this);
        w.firstFrame = INT_MIN;
        w.lastFrame = INT_MAX;
        std::list<AppInstance::RenderWork> works;
        works.push_back(w);
        getApp()->startWritersRendering(works);
    }
}

void
DiskCacheNode::getFrameRange(SequenceTime *first,SequenceTime *last)
{
    int idx = _imp->frameRange->getValue();
    switch (idx) {
        case 0: {
            EffectInstance* input = getInput(0);
            if (input) {
                input->getFrameRange_public(input->getHash(), first, last);
            }
        } break;
        case 1: {
            boost::shared_ptr<TimeLine> tl = getApp()->getTimeLine();
            *first = tl->leftBound();
            *last = tl->rightBound();
        } break;
        case 2: {
            *first = _imp->firstFrame->getValue();
            *last = _imp->lastFrame->getValue();
        };
        default:
            break;
    }
}

void
DiskCacheNode::getPreferredDepthAndComponents(int inputNb,Natron::ImageComponentsEnum* comp,Natron::ImageBitDepthEnum* depth) const
{
    EffectInstance* input = getInput(0);
    if (input) {
        return input->getPreferredDepthAndComponents(-1, comp, depth);
    } else {
        *comp = eImageComponentRGBA;
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
DiskCacheNode::render(SequenceTime time,
                      const RenderScale& originalScale,
                      const RenderScale & /*mappedScale*/,
                      const RectI & roi, //!< renderWindow in pixel coordinates
                      int view,
                      bool /*isSequentialRender*/,
                      bool /*isRenderResponseToUserInteraction*/,
                      boost::shared_ptr<Natron::Image> output)
{
    
    EffectInstance* input = getInput(0);
    if (!input) {
        return eStatusFailed;
    }
    
    ImageBitDepthEnum bitdepth;
    ImageComponentsEnum components;
    input->getPreferredDepthAndComponents(-1, &components, &bitdepth);
    double par = input->getPreferredAspectRatio();
    
    RectI roiPixel;
    boost::shared_ptr<Image> srcImg = getImage(0, time, originalScale, view, NULL, components, bitdepth, par, false, &roiPixel);
    
    if (srcImg->getMipMapLevel() != output->getMipMapLevel()) {
        throw std::runtime_error("Host gave image with wrong scale");
    }
    if (srcImg->getComponents() != output->getComponents() || srcImg->getBitDepth() != output->getBitDepth()) {
        

        srcImg->convertToFormat(roi, getApp()->getDefaultColorSpaceForBitDepth( srcImg->getBitDepth() ),
                                getApp()->getDefaultColorSpaceForBitDepth(output->getBitDepth()), 3, false, true, false, output.get());
    } else {
        output->pasteFrom(*srcImg, roi, true);
    }
    
    return eStatusOK;
}

