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


#ifndef CREATENODEARGS_H
#define CREATENODEARGS_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <map>
#include <vector>
#include "Engine/EngineFwd.h"

/**
 * @brief x1 std::string property indicating the ID of the plug-in instance to create
 * Must be specified
 **/
#define kCreateNodeArgsPropPluginID "CreateNodeArgsPropPluginID"

/**
 * @brief optional x2 int property of the form (major, minor) indicating the version of the plug-in instance to create
 * By default Natron will load the highest possible version available for that plug-in.
 **/
#define kCreateNodeArgsPropPluginVersion "CreateNodeArgsPropPluginVersion"

/**
 * @brief optional x2 double property indicating the position in the Node-Graph of the given group where to position the node.
 * By default Natron will position the node according to the state of the interface (current selection, position of the viewport, etc...)
 **/
#define kCreateNodeArgsPropNodeInitialPosition "CreateNodeArgsPropNodeInitialPosition"

/**
 * @brief optional x1 std::string property indicating the name that the Node will have initially when created.
 * By default Natron will name the node according to the plug-in label and will add a digit afterwards dependending on the current number of instances of that plug-in.
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
 * @brief optional x1 boost::shared_ptr<NodeSerialization> A pointer to a node serialization object.
 * If specified, Natron will load the node state from this object.
 **/
#define kCreateNodeArgsPropNodeSerialization "CreateNodeArgsPropNodeSerialization"

/**
 * @brief optional x1 int property
 * If copy/pasting, we don't want to paste a PyPlug and create copy from the Python script,
 * we want it to copy the group directly and keep the modifications the user may have made.
 * This is only relevant and valid if the property kCreateNodeArgsPropNodeSerialization has been set.
 **/
#define kCreateNodeArgsPropDoNotLoadPyPlugFromScript "CreateNodeArgsPropDoNotLoadPyPlugFromScript"

/**
 * @brief optional x1 int property
 * When set the node will not be part of the project. The node can be used for internal used, e.g in a Python script.
 * kCreateNodeArgsPropNoNodeGUI is implicitly set to 1 when this property is set
 **/
#define kCreateNodeArgsPropOutOfProject "CreateNodeArgsPropOutOfProject"

/**
 * @brief optional x1 int property
 * If 1, the node will not have any GUI created.
 * By default Natron will always create the GUI for a node, except if the property kCreateNodeArgsPropOutOfProject is set to 1
 **/
#define kCreateNodeArgsPropNoNodeGUI "CreateNodeArgsPropNoNodeGUI"

/**
 * @brief optional x1 int property
 * If 1, the createNode function will not fail if passing the plug-in ID of a plug-in that is flagged as not allowed for user creation
 **/
#define kCreateNodeArgsPropAllowNonUserCreatablePlugins "CreateNodeArgsPropAllowNonUserCreatablePlugins"

/**
 * @brief optional x1 int property
 * If 1, this is a hint indicating that the node was created directly following a user action and that the node should be made visible for the user
 * By default, the node is not assumed to be user created
 **/
#define kCreateNodeArgsPropUserCreated "CreateNodeArgsPropUserCreated"

/**
 * @brief optional x1 int property
 * If 1, Natron will not attempt to convert the plug-in ID to another known plug-in ID. For example if trying to create an instance of PLUGINID_OFX_ROTO, then Natron will stick to it.
 * If not set Natron will try to convert the plug-in ID to a known value, such as PLUGINID_NATRON_ROTO
 **/
#define kCreateNodeArgsPropTrustPluginID "CreateNodeArgsPropTrustPluginID"

/**
 * @brief optional x1 boost::shared_ptr<NodeCollection> A pointer to the group in which the node will be created. If not set, Natron
 * will create the node in the top-level node-graph of the project.
 **/
#define kCreateNodeArgsPropGroupContainer "CreateNodeArgsPropGroupContainer"

/**
 * @brief optional x1 boost::shared_ptr<Node> A Pointer to a node that contains this node. This is used internally by the Read and Write nodes
 * which are meta-bundles to the internal decoders/encoders.
 **/
#define kCreateNodeArgsPropMetaNodeContainer "CreateNodeArgsPropMetaNodeContainer"

/**
 * @brief [DEPRECATED] optional x1 std::string For the old Tracker_PM node, we used this to indicate when creating a track (which was represented by a node internally)
 * what was the script-name of the parent node.
 * This is meaningless in Natron >= 2.1
 **/
#define kCreateNodeArgsPropMultiInstanceParentName "CreateNodeArgsPropMultiInstanceParentName"

NATRON_NAMESPACE_ENTER;

struct CreateNodeArgsPrivate;
class CreateNodeArgs
{

    class PropertyBase
    {
    public:


        PropertyBase()
        {

        }

        virtual int getDimension() const = 0;

        virtual ~PropertyBase()
        {

        }
    };

    template<typename T>
    class Property : public PropertyBase
    {
    public:

        std::vector<T> value;

        Property()
        : PropertyBase()
        , value()
        {}

        virtual int getDimension() const OVERRIDE FINAL
        {
            return (int)value.size();
        }

        virtual ~Property() {

        }
    };


    template <typename T>
    boost::shared_ptr<Property<T> > getOrCreateProp(const std::string& name, bool fail)
    {
        boost::shared_ptr<PropertyBase>* propPtr = 0;
        if (!fail) {
            propPtr = &_properties[name];
        } else {
            std::map<std::string, boost::shared_ptr<PropertyBase> >::iterator found = _properties.find(name);
            if (found == _properties.end()) {
                throw std::invalid_argument("Invalid property " + name);
            }
            propPtr = &(found->second);
        }
        boost::shared_ptr<Property<T> > propTemplate;
        if (!propPtr) {
            propTemplate.reset(new Property<T>);
            propPtr = propTemplate;
        } else {
            propTemplate = boost::dynamic_pointer_cast<Property<T> >(propPtr);
        }
        assert(propTemplate);
        return propTemplate;
    }

public:

    /**
     * @brief The constructor, taking values for all non-optional properties and the group
     **/
    CreateNodeArgs(const std::string& pluginID,
                   const boost::shared_ptr<NodeCollection>& group = boost::shared_ptr<NodeCollection>());

    ~CreateNodeArgs();

    template <typename T>
    void setProperty(const std::string& name, const T& value, int index = 0)
    {
        boost::shared_ptr<Property<T> > propTemplate = getOrCreateProp<T>(name, false);
        if (index >= (int)propTemplate->value.size()) {
            propTemplate->value.resize(index + 1);
        }
        propTemplate->value[index] = value;

    }

    template <typename T>
    void setPropertyN(const std::string& name, const std::vector<T>& values)
    {
        boost::shared_ptr<Property<T> > propTemplate = getOrCreateProp<T>(name, false);
        propTemplate->value = values;
    }

    int getPropertyDimension(const std::string& name, bool throwIfFailed) const
    {
        std::map<std::string, boost::shared_ptr<PropertyBase> >::const_iterator found = _properties.find(name);
        if (found == _properties.end()) {
            if (throwIfFailed) {
                throw std::invalid_argument("Invalid property " + name);
            } else {
                return 0;
            }
        }
        return found->second->getDimension();
    }


    template <typename T>
    void addParamDefaultValue(const std::string& paramName, const T& value)
    {
        int n = getPropertyDimension(kCreateNodeArgsPropNodeInitialParamValues, false);

        setProperty<std::string>(kCreateNodeArgsPropNodeInitialParamValues, paramName, n);
        std::string propertyName(kCreateNodeArgsPropParamValue);
        propertyName += "_";
        propertyName += paramName;
        setProperty(propertyName, value);
    }

    template <typename T>
    void addParamDefaultValueN(const std::string& paramName, const std::vector<T>& values)
    {
        int n = getPropertyDimension(kCreateNodeArgsPropNodeInitialParamValues, false);

        setProperty<std::string>(kCreateNodeArgsPropNodeInitialParamValues, paramName, n);
        std::string propertyName(kCreateNodeArgsPropParamValue);
        propertyName += "_";
        propertyName += paramName;
        setPropertyN(propertyName, values);
    }


    template<typename T>
    const T& getProperty(const std::string& name, const T& defaultValueIffailed, int index = 0) const
    {
        try {
            boost::shared_ptr<Property<T> > propTemplate = getOrCreateProp<T>(name, true);
            if (index < 0 || index >= (int)propTemplate->value.size()) {
                return defaultValueIffailed;
            }
            return propTemplate->value[index];
        } catch (const std::exception& e) {
            return defaultValueIffailed;
        }
        
    }

    template<typename T>
    const std::vector<T>& getPropertyN(const std::string& name, const std::vector<T>& defaultValueIfFailed) const
    {
        try {
            boost::shared_ptr<Property<T> > propTemplate = getOrCreateProp<T>(name, true);
            return propTemplate->value;

        } catch (const std::exception& e) {
            return defaultValueIfFailed;
        }
    }
private:

    std::map<std::string, boost::shared_ptr<PropertyBase> > _properties;
};

NATRON_NAMESPACE_EXIT;

#endif // CREATENODEARGS_H
