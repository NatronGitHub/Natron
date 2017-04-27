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

#include "AddPlaneNode.h"

#include "Engine/KnobTypes.h"

NATRON_NAMESPACE_ENTER;

struct AddPlaneNodePrivate
{

    AddPlaneNodePrivate()
    {

    }
};


PluginPtr
AddPlaneNode::createPlugin()
{
    std::vector<std::string> grouping;
    grouping.push_back(PLUGIN_GROUP_CHANNEL);
    PluginPtr ret = Plugin::create((void*)AddPlaneNode::create, (void*)AddPlaneNode::createRenderClone, PLUGINID_NATRON_ADD_PLANE, "AddPlane", 1, 0, grouping);

    QString desc = tr("This node acts as a pass-through for the input image, but allows to add new empty plane(s) to the image");
    ret->setProperty<std::string>(kNatronPluginPropDescription, desc.toStdString());
    ret->setProperty<int>(kNatronPluginPropRenderSafety, (int)eRenderSafetyFullySafe);
    return ret;
}


AddPlaneNode::AddPlaneNode(const NodePtr& node)
: NoOpBase(node)
, _imp(new AddPlaneNodePrivate())
{
}

AddPlaneNode::AddPlaneNode(const EffectInstancePtr& mainInstance, const FrameViewRenderKey& key)
: NoOpBase(mainInstance, key)
, _imp(new AddPlaneNodePrivate())
{
}

AddPlaneNode::~AddPlaneNode()
{

}

std::string
AddPlaneNode::getInputLabel(int /*inputNb*/) const
{
    return tr("Source").toStdString();
}

void
AddPlaneNode::initializeKnobs()
{
    // Get the knob created by default on the main page, and make it visible
    KnobPagePtr mainPage = createKnob<KnobPage>("controlsPage");
    mainPage->setLabel(tr("Controls"));

    KnobLayersPtr param = getOrCreateUserPlanesKnob(mainPage);
    param->setSecret(false);
}

NATRON_NAMESPACE_EXIT;
