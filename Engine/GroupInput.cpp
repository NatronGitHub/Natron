//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "GroupInput.h"

#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h" // kNatronGroupInputIsOptionalParamName, kNatronGroupInputIsMaskParamName

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
    
    boost::shared_ptr<Bool_Knob> optKnob = Natron::createKnob<Bool_Knob>(this, "Optional");
    optKnob->setHintToolTip("When checked, this input of the group will be optional, i.e it will not be required that it is connected "
                             "for the render to work. ");
    optKnob->setAnimationEnabled(false);
    optKnob->setName(kNatronGroupInputIsOptionalParamName);
    page->addKnob(optKnob);
    optional = optKnob;
    
    boost::shared_ptr<Bool_Knob> maskKnob = Natron::createKnob<Bool_Knob>(this, "Mask");
    maskKnob->setHintToolTip("When checked, this input of the group will be considered as a mask. A mask is always optional.");
    maskKnob->setAnimationEnabled(false);
    maskKnob->setName(kNatronGroupInputIsMaskParamName);
    page->addKnob(maskKnob);
    mask = maskKnob;

}

void
GroupInput::knobChanged(KnobI* k,
                 Natron::ValueChangedReasonEnum /*reason*/,
                 int /*view*/,
                 SequenceTime /*time*/,
                 bool /*originatedFromMainThread*/)
{
    if (k == optional.lock().get()) {
        boost::shared_ptr<NodeCollection> group = getNode()->getGroup();
        group->notifyInputOptionalStateChanged(getNode());
    } else if (k == mask.lock().get()) {
        bool isMask = mask.lock()->getValue();
        if (isMask) {
            optional.lock()->setValue(true, 0);
        } else {
            optional.lock()->setValue(false, 0);
        }
        boost::shared_ptr<NodeCollection> group = getNode()->getGroup();
        group->notifyInputMaskStateChanged(getNode());
        
    }
}
