/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 * Copyright (C) 2018-2020 The Natron developers
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


PluginPtr
GroupInput::createPlugin()
{
    std::vector<std::string> grouping;
    grouping.push_back(PLUGIN_GROUP_OTHER);
    PluginPtr ret = Plugin::create(GroupInput::create, GroupInput::createRenderClone, PLUGINID_NATRON_INPUT, "Input", 1, 0, grouping);

    QString desc =  tr("This node can only be used within a Group. It adds an input arrow to the group.");
    ret->setProperty<std::string>(kNatronPluginPropDescription, desc.toStdString());
    EffectDescriptionPtr effectDesc = ret->getEffectDescriptor();
    effectDesc->setProperty<RenderSafetyEnum>(kEffectPropRenderThreadSafety, eRenderSafetyFullySafe);
    effectDesc->setProperty<bool>(kEffectPropSupportsTiles, true);
    ret->setProperty<std::string>(kNatronPluginPropIconFilePath,  "Images/input_icon.png");

    ret->setProperty<ImageBitDepthEnum>(kNatronPluginPropOutputSupportedBitDepths, eImageBitDepthFloat, 0);
    ret->setProperty<ImageBitDepthEnum>(kNatronPluginPropOutputSupportedBitDepths, eImageBitDepthByte, 1);
    ret->setProperty<ImageBitDepthEnum>(kNatronPluginPropOutputSupportedBitDepths, eImageBitDepthShort, 2);
    ret->setProperty<std::bitset<4> >(kNatronPluginPropOutputSupportedComponents, std::bitset<4>(std::string("1111")));
    return ret;
}



void
GroupInput::initializeKnobs()
{
    KnobPagePtr page = createKnob<KnobPage>("controlsPage");
    page->setLabel(tr("Controls"));

    KnobBoolPtr optKnob = createKnob<KnobBool>(kNatronGroupInputIsOptionalParamName);
    optKnob->setLabel(tr("Optional"));
    optKnob->setHintToolTip( tr("When checked, this input of the group will be optional, i.e. it will not be required that it is connected "
                                "for the render to work.") );
    optKnob->setAnimationEnabled(false);
    page->addKnob(optKnob);
    _optional = optKnob;

    KnobBoolPtr maskKnob = createKnob<KnobBool>(kNatronGroupInputIsMaskParamName);
    maskKnob->setHintToolTip( tr("When checked, this input of the group will be considered as a mask. A mask is always optional.") );
    maskKnob->setAnimationEnabled(false);
    maskKnob->setLabel(tr("Mask"));
    page->addKnob(maskKnob);
    _mask = maskKnob;
}

bool
GroupInput::knobChanged(const KnobIPtr& k,
                        ValueChangedReasonEnum /*reason*/,
                        ViewSetSpec /*view*/,
                        TimeValue /*time*/)
{
    bool ret = true;
    KnobBoolPtr optKnob = _optional.lock();
    KnobBoolPtr maskKnob = _mask.lock();

    if ( k == optKnob ) {
        NodeCollectionPtr group = getNode()->getGroup();
        group->notifyInputOptionalStateChanged( getNode() );
    } else if ( k == maskKnob ) {
        bool isMask = maskKnob->getValue();
        if (isMask) {
            optKnob->setValue(true);
        } else {
            optKnob->setValue(false);
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
