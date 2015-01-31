//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "NoOp.h"

#include "Engine/KnobTypes.h"
#include "Engine/NodeGroup.h"
#include "Engine/Transform.h"

NoOpBase::NoOpBase(boost::shared_ptr<Natron::Node> n)
    : Natron::OutputEffectInstance(n)
{
    setSupportsRenderScaleMaybe(eSupportsYes);
}

void
NoOpBase::addAcceptedComponents(int /*inputNb*/,
                                std::list<Natron::ImageComponentsEnum>* comps)
{
    comps->push_back(Natron::eImageComponentRGB);
    comps->push_back(Natron::eImageComponentRGBA);
    comps->push_back(Natron::eImageComponentAlpha);
}

void
NoOpBase::addSupportedBitDepth(std::list<Natron::ImageBitDepthEnum>* depths) const
{
    depths->push_back(Natron::eImageBitDepthByte);
    depths->push_back(Natron::eImageBitDepthShort);
    depths->push_back(Natron::eImageBitDepthFloat);
}

bool
NoOpBase::isIdentity(SequenceTime time,
                     const RenderScale & /*scale*/,
                     const RectD & /*rod*/,
                     const double /*par*/,
                     int /*view*/,
                     SequenceTime* inputTime,
                     int* inputNb)
{
    *inputTime = time;
    *inputNb = 0;

    return true;
}

std::string
Dot::getDescription() const
{
    return "Doesn't do anything to the input image, this is used in the node graph to make bends in the links.";
}

Natron::StatusEnum
NoOpBase::getTransform(SequenceTime /*time*/,
                       const RenderScale& /*renderScale*/,
                       int /*view*/,
                       Natron::EffectInstance** inputToTransform,
                       Transform::Matrix3x3* transform)
{
    *inputToTransform = getInput(0);
    if (!*inputToTransform) {
        return Natron::eStatusFailed;
    }
    transform->a = 1.; transform->b = 0.; transform->c = 0.;
    transform->d = 0.; transform->e = 1.; transform->f = 0.;
    transform->g = 0.; transform->h = 0.; transform->i = 1.;
    return Natron::eStatusOK;
}

std::string
GroupInput::getDescription() const
{
    return "This node can only be used within a Group. It adds an input arrow to the group.";
}

void
GroupInput::initializeKnobs()
{
    boost::shared_ptr<Page_Knob> page = Natron::createKnob<Page_Knob>(this, "Controls");
    page->setName("controls");
    
    optional = Natron::createKnob<Bool_Knob>(this, "Optional");
    optional->setHintToolTip("When checked, this input of the group will be optional, i.e it will not be required that it is connected "
                             "for the render to work. ");
    optional->setAnimationEnabled(false);
    optional->setName("optional");
    page->addKnob(optional);
    
    mask = Natron::createKnob<Bool_Knob>(this, "Mask");
    mask->setHintToolTip("When checked, this input of the group will be considered as a mask. A mask is always optional.");
    mask->setAnimationEnabled(false);
    mask->setName("isMask");
    page->addKnob(mask);

}

void
GroupInput::knobChanged(KnobI* k,
                 Natron::ValueChangedReasonEnum /*reason*/,
                 int /*view*/,
                 SequenceTime /*time*/,
                 bool /*originatedFromMainThread*/)
{
    if (k == optional.get()) {
        boost::shared_ptr<NodeCollection> group = getNode()->getGroup();
        group->notifyInputOptionalStateChanged(getNode());
    } else if (k == mask.get()) {
        bool isMask = mask->getValue();
        if (isMask) {
            optional->setValue(true, 0);
        } else {
            optional->setValue(false, 0);
        }
        boost::shared_ptr<NodeCollection> group = getNode()->getGroup();
        group->notifyInputMaskStateChanged(getNode());
        
    }
}

std::string
GroupOutput::getDescription() const
{
    return "This node can only be used within a Group. There can only be 1 Output node in the group. It defines the output of the group.";
}
