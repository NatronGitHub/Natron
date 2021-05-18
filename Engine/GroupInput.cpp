/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2021 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#include <cassert>
#include <stdexcept>

#include "Engine/NodeMetadata.h"
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h" // kNatronGroupInputIsOptionalParamName, kNatronGroupInputIsMaskParamName
#include "Engine/ViewIdx.h"

NATRON_NAMESPACE_ENTER

std::string
GroupInput::getPluginDescription() const
{
    return "This node can only be used within a Group. It adds an input arrow to the group.";
}

void
GroupInput::initializeKnobs()
{
    KnobPagePtr page = AppManager::createKnob<KnobPage>( this, tr("Controls") );

    page->setName("controls");

    KnobBoolPtr optKnob = AppManager::createKnob<KnobBool>( this, tr("Optional") );
    optKnob->setHintToolTip( tr("When checked, this input of the group will be optional, i.e. it will not be required that it is connected "
                                "for the render to work.") );
    optKnob->setAnimationEnabled(false);
    optKnob->setName(kNatronGroupInputIsOptionalParamName);
    page->addKnob(optKnob);
    optional = optKnob;

    KnobBoolPtr maskKnob = AppManager::createKnob<KnobBool>( this, tr("Mask") );
    maskKnob->setHintToolTip( tr("When checked, this input of the group will be considered as a mask. A mask is always optional.") );
    maskKnob->setAnimationEnabled(false);
    maskKnob->setName(kNatronGroupInputIsMaskParamName);
    page->addKnob(maskKnob);
    mask = maskKnob;
}

bool
GroupInput::knobChanged(KnobI* k,
                        ValueChangedReasonEnum /*reason*/,
                        ViewSpec /*view*/,
                        double /*time*/,
                        bool /*originatedFromMainThread*/)
{
    bool ret = true;

    if ( k == optional.lock().get() ) {
        NodeCollectionPtr group = getNode()->getGroup();
        group->notifyInputOptionalStateChanged( getNode() );
    } else if ( k == mask.lock().get() ) {
        bool isMask = mask.lock()->getValue();
        if (isMask) {
            optional.lock()->setValue(true);
        } else {
            optional.lock()->setValue(false);
        }
        NodeCollectionPtr group = getNode()->getGroup();
        group->notifyInputMaskStateChanged( getNode() );
    } else {
        ret = false;
    }

    return ret;
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_GroupInput.cpp"
