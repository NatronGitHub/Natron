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

#ifndef Engine_CreateNodeArgs_h
#define Engine_CreateNodeArgs_h


// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <map>
#include <vector>
#include <stdexcept>
#include <string>

#include "Engine/PropertiesHolder.h"

#include "Engine/EngineFwd.h"


NATRON_NAMESPACE_ENTER

/**
 * @brief x1 std::string property indicating the ID of the plug-in instance to create
 * Default value - Empty
 * Must be specified
 **/
#define kCreateNodeArgsPropPluginID "CreateNodeArgsPropPluginID"

/**
 * @brief optional x2 int property of the form (major, minor) indicating the version of the plug-in instance to create
 * Default value - (-1,-1)
 * With the value (-1,-1) Natron will load the highest possible version available for that plug-in.
 **/
#define kCreateNodeArgsPropPluginVersion "CreateNodeArgsPropPluginVersion"

/**
 * @brief optional x2 double property indicating the position in the Node-Graph of the given group where to position the node.
 * Default value - (INT_MIN,INT_MIN)
 * By default Natron will position the node according to the state of the interface (current selection, position of the viewport, etc...)*
 * If the property kCreateNodeArgsPropNodeSerialization is set, this has no effect.
 **/
#define kCreateNodeArgsPropNodeInitialPosition "CreateNodeArgsPropNodeInitialPosition"

/**
 * @brief optional x1 std::string property indicating the name that the Node will have initially when created.
 * Default Value - Empty
 * By default Natron will name the node according to the plug-in label and will add a digit afterwards dependending on the current number of instances of that plug-in.
 * If the property kCreateNodeArgsPropNodeSerialization is set, this has no effect
 **/
#define kCreateNodeArgsPropNodeInitialName "CreateNodeArgsPropNodeInitialName"

/**
 * @brief optional xN std::string property containing parameter script-name for which the CreateNodeArgs struct contains special default values.
 * Those values must be set by setting the property "CreateNodeArgsPropParamValue_paramName" where paramName gets replaced by the script-name of the param-name and
 * the type of the property is the type of the Knob value (i.e: bool, int, double or std::string).
 * E.g the property could be : kCreateNodeArgsPropParamValue_filename to indicate the default value for a Read node.
 * By default, no parameters is set and the parameters are left to the default value specified by the plug-in itself.
 **/
#define kCreateNodeArgsPropNodeInitialParamValues "CreateNodeArgsPropNodeInitialParamValues"

/**
 * @brief optional xN property of unknown type (i.e: either int, bool, double or std::string) which may be used to specify a parameter's default value. @see kCreateNodeArgsPropNodeInitialParamValues
 **/
#define kCreateNodeArgsPropParamValue "CreateNodeArgsPropParamValue"

/**
 * @brief optional x1 NodeSerializationPtr A pointer to a node serialization object.
 * Default value - NULL
 * If non null, Natron will load the node state from this object.
 **/
#define kCreateNodeArgsPropNodeSerialization "CreateNodeArgsPropNodeSerialization"

/**
 * @brief optional x1 bool If set to true when creating a node with the plug-in PLUGINID_NATRON_GROUP the initial Input and Output nodes
 * will NOT be created. If the property kCreateNodeArgsPropNodeSerialization is set to a non null value it acts as if this property is set to true
 * Default value - false
 **/
#define kCreateNodeArgsPropNodeGroupDisableCreateInitialNodes "CreateNodeArgsPropNodeGroupDisableCreateInitialNodes"

/**
 * @brief optional x1 std::string property indicating the label of the presets to use to load the node.
 * The preset label must correspond to a valid label of a a preset file (.nps) that was found by Natron.
 * The preset label is NOT the filename of the preset file, but the string in the file found next to the key "Label"
 * If the preset cannot be found, the presets will not be loaded
 * Default value - Empty
 **/
#define kCreateNodeArgsPropPreset "CreateNodeArgsPropPreset"

/**
 * @brief optional x1 std::string property indicating the pyplug ID to use to load the node.
 * This is only relevant to Group nodes.
 * Default value - Empty
 **/
#define kCreateNodeArgsPropPyPlugID "CreateNodeArgsPropPyPlugID"


/**
 * @brief optional x1 bool property
 * Default Value - false
 * When set to true the node will not be visible and will not be serialized into the user project.
 * The node can be used for internal use, e.g in a Python script.
 **/
#define kCreateNodeArgsPropVolatile "CreateNodeArgsPropVolatile"

/**
 * @brief optional x1 bool property
 * Default Value - false
 * If true, the node will not have any GUI created.
 * By default Natron will always create the GUI for a node, except if the property kCreateNodeArgsPropVolatile is set to true
 **/
#define kCreateNodeArgsPropNoNodeGUI "CreateNodeArgsPropNoNodeGUI"

/**
 * @brief optional x1 bool property
 * Default Value - true
 * If true, the node will have its properties panel opened when created. If the property kCreateNodeArgsPropNodeSerialization is set to a non null
 * serialization, this property has no effect.
 **/
#define kCreateNodeArgsPropSettingsOpened "CreateNodeArgsPropSettingsOpened"

/**
 * @brief optional x1 bool property
 * Default Value - true
 * If true, if the node is a group, its sub-graph panel will be visible when created. If the property kCreateNodeArgsPropNodeSerialization is set to a non null
 * serialization, this property has no effect.
 **/
#define kCreateNodeArgsPropSubGraphOpened "CreateNodeArgsPropSubGraphOpened"

/**
 * @brief optional x1 bool property
 * Default Value - true
 * If true, Natron will try to automatically connect the node to others depending on the user selection. If the property kCreateNodeArgsPropNodeSerialization is set, this has no effect.
 **/
#define kCreateNodeArgsPropAutoConnect "CreateNodeArgsPropAutoConnect"

/**
 * @brief optional x1 bool property
 * Default Value - true
 * If true, Natron will push a undo/redo command to the stack when creating this node. If the property kCreateNodeArgsPropNoNodeGUI is set to true or kCreateNodeArgsPropVolatile
 * is set to true, this property has no effet
 **/
#define kCreateNodeArgsPropAddUndoRedoCommand "CreateNodeArgsPropAddUndoRedoCommand"

/**
 * @brief optional x1 bool property
 * Default value - false
 * If true, the createNode function will not fail if passing the plug-in ID of a plug-in that is flagged as not allowed for user creation
 **/
#define kCreateNodeArgsPropAllowNonUserCreatablePlugins "CreateNodeArgsPropAllowNonUserCreatablePlugins"


/**
 * @brief optional x1 bool property
 * Default value - false
 * If true, Natron will not show any information, error, warning, question or file dialog when creating the node.
 **/
#define kCreateNodeArgsPropSilent "CreateNodeArgsPropSilent"


/**
 * @brief optional x1 NodeCollectionPtr A pointer to the group in which the node will be created. If not set, Natron
 * will create the node in the top-level node-graph of the project.
 **/
#define kCreateNodeArgsPropGroupContainer "CreateNodeArgsPropGroupContainer"

/**
 * @brief optional x1 NodePtr A Pointer to a node that contains this node. This is used internally by the Read and Write nodes
 * which are meta-bundles to the internal decoders/encoders.
 * Default value - null
 **/
#define kCreateNodeArgsPropMetaNodeContainer "CreateNodeArgsPropMetaNodeContainer"


struct CreateNodeArgsPrivate;
class CreateNodeArgs : public PropertiesHolder
{


public:

    static CreateNodeArgsPtr create(const std::string& pluginID, const NodeCollectionPtr& group = NodeCollectionPtr());


    CreateNodeArgs();


    virtual ~CreateNodeArgs();

    template <typename T>
    void addParamDefaultValue(const std::string& paramName, const T& value)
    {
        int n = getPropertyDimension(kCreateNodeArgsPropNodeInitialParamValues);
        setProperty<std::string>(kCreateNodeArgsPropNodeInitialParamValues, paramName, n);
        std::string propertyName(kCreateNodeArgsPropParamValue);
        propertyName += "_";
        propertyName += paramName;
        setProperty(propertyName, value, 0, false);
    }

    template <typename T>
    void addParamDefaultValueN(const std::string& paramName, const std::vector<T>& values)
    {
        int n = getPropertyDimension(kCreateNodeArgsPropNodeInitialParamValues);

        setProperty<std::string>(kCreateNodeArgsPropNodeInitialParamValues, paramName, n);
        std::string propertyName(kCreateNodeArgsPropParamValue);
        propertyName += "_";
        propertyName += paramName;
        setPropertyN(propertyName, values, false);
    }

private:


    virtual void initializeProperties() const OVERRIDE FINAL;
};

NATRON_NAMESPACE_EXIT

#endif // Engine_CreateNodeArgs_h
