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


#include "CreateNodeArgs.h"
#include "Engine/NodeGroup.h"

NATRON_NAMESPACE_ENTER


CreateNodeArgs::CreateNodeArgs()
: PropertiesHolder()
{

}


CreateNodeArgs::~CreateNodeArgs()
{

}

CreateNodeArgsPtr
CreateNodeArgs::create(const std::string& pluginID, const NodeCollectionPtr& group)
{
    CreateNodeArgsPtr ret = boost::make_shared<CreateNodeArgs>();
    ret->setProperty(kCreateNodeArgsPropPluginID, pluginID);
    if (group) {
        ret->setProperty(kCreateNodeArgsPropGroupContainer, group);
    }
    return ret;
}

void
CreateNodeArgs::initializeProperties() const
{
    createProperty<std::string>(kCreateNodeArgsPropPluginID, std::string());
    createProperty<int>(kCreateNodeArgsPropPluginVersion, -1, -1);
    createProperty<double>(kCreateNodeArgsPropNodeInitialPosition, (double)INT_MIN, (double)INT_MIN);
    createProperty<std::string>(kCreateNodeArgsPropNodeInitialName, std::string());
    createProperty<std::string>(kCreateNodeArgsPropPreset, std::string());
    createProperty<std::string>(kCreateNodeArgsPropPyPlugID, std::string());
    createProperty<std::string>(kCreateNodeArgsPropNodeInitialParamValues, std::vector<std::string>());
    createProperty<SERIALIZATION_NAMESPACE::NodeSerializationPtr>(kCreateNodeArgsPropNodeSerialization, SERIALIZATION_NAMESPACE::NodeSerializationPtr());
    createProperty<bool>(kCreateNodeArgsPropVolatile, false);
    createProperty<bool>(kCreateNodeArgsPropNoNodeGUI, false);
    createProperty<bool>(kCreateNodeArgsPropSettingsOpened, true);
    createProperty<bool>(kCreateNodeArgsPropSubGraphOpened, true);
    createProperty<bool>(kCreateNodeArgsPropAutoConnect, true);
    createProperty<bool>(kCreateNodeArgsPropAllowNonUserCreatablePlugins, false);
    createProperty<bool>(kCreateNodeArgsPropSilent, false);
    createProperty<bool>(kCreateNodeArgsPropNodeGroupDisableCreateInitialNodes, false);
    createProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, true);
    createProperty<NodeCollectionPtr>(kCreateNodeArgsPropGroupContainer, NodeCollectionPtr());
    createProperty<NodePtr>(kCreateNodeArgsPropMetaNodeContainer, NodePtr());
}

NATRON_NAMESPACE_EXIT
