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

#include "GroupInput.h"

#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h" // kNatronGroupInputIsOptionalParamName, kNatronGroupInputIsMaskParamName

NATRON_NAMESPACE_USING

std::string
GroupInput::getPluginDescription() const
{
    return "This node can only be used within a Group. It adds an input arrow to the group.";
}

void
GroupInput::initializeKnobs()
{
    boost::shared_ptr<KnobPage> page = Natron::createKnob<KnobPage>(this, "Controls");
    page->setName("controls");
    
    boost::shared_ptr<KnobBool> optKnob = Natron::createKnob<KnobBool>(this, "Optional");
    optKnob->setHintToolTip("When checked, this input of the group will be optional, i.e it will not be required that it is connected "
                             "for the render to work. ");
    optKnob->setAnimationEnabled(false);
    optKnob->setName(kNatronGroupInputIsOptionalParamName);
    page->addKnob(optKnob);
    optional = optKnob;
    
    boost::shared_ptr<KnobBool> maskKnob = Natron::createKnob<KnobBool>(this, "Mask");
    maskKnob->setHintToolTip("When checked, this input of the group will be considered as a mask. A mask is always optional.");
    maskKnob->setAnimationEnabled(false);
    maskKnob->setName(kNatronGroupInputIsMaskParamName);
    page->addKnob(maskKnob);
    mask = maskKnob;

}

void
GroupInput::knobChanged(KnobI* k,
                 ValueChangedReasonEnum /*reason*/,
                 int /*view*/,
                 double /*time*/,
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

ImagePremultiplicationEnum
GroupInput::getOutputPremultiplication() const
{
    NodePtr thisNode = getNode();
    boost::shared_ptr<NodeCollection> group = thisNode->getGroup();
    NodeGroup* isGroup = dynamic_cast<NodeGroup*>(group.get());
    assert(isGroup);
    if (!isGroup) {
        return Natron::eImagePremultiplicationPremultiplied;
    }
    int inputNb = -1;
    std::vector<NodePtr> groupInputs;
    isGroup->getInputs(&groupInputs, false);
    for (std::size_t i = 0; i < groupInputs.size(); ++i) {
        if (groupInputs[i] == thisNode) {
            inputNb = i;
            break;
        }
    }
    assert(inputNb != -1);
    if (inputNb != -1) {
        Natron::EffectInstance* input = isGroup->getInput(inputNb);
        if (input) {
            return input->getOutputPremultiplication();
        }
    }
    return Natron::eImagePremultiplicationPremultiplied;
}
