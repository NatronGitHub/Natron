/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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
#include <stdexcept>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#endif

#include "Engine/EngineFwd.h"

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
 * @brief optional x1 boost::shared_ptr<NodeSerialization> A pointer to a node serialization object.
 * Default value - NULL
 * If non null, Natron will load the node state from this object.
 **/
#define kCreateNodeArgsPropNodeSerialization "CreateNodeArgsPropNodeSerialization"

/**
 * @brief optional x1 bool If set to true when creating a node with the plug-in PLUGINID_NATRON_GROUP the inital Input and Output nodes
 * will NOT be created. If the property kCreateNodeArgsPropNodeSerialization is set to a non null value it acts as if this property is set to true
 * Default value - false
 **/
#define kCreateNodeArgsPropNodeGroupDisableCreateInitialNodes "CreateNodeArgsPropNodeGroupDisableCreateInitialNodes"

/**
 * @brief optional x1 bool property
 * Default Value - false
 * If copy/pasting, we don't want to paste a PyPlug and create copy from the Python script,
 * we want it to copy the group directly and keep the modifications the user may have made.
 * This is only relevant and valid if the property kCreateNodeArgsPropNodeSerialization has been set.
 **/
#define kCreateNodeArgsPropDoNotLoadPyPlugFromScript "CreateNodeArgsPropDoNotLoadPyPlugFromScript"

/**
 * @brief optional x1 bool property
 * Default Value - false
 * When set the node will not be part of the project. The node can be used for internal used, e.g in a Python script.
 **/
#define kCreateNodeArgsPropOutOfProject "CreateNodeArgsPropOutOfProject"

/**
 * @brief optional x1 bool property
 * Default Value - false
 * If true, the node will not have any GUI created.
 * By default Natron will always create the GUI for a node, except if the property kCreateNodeArgsPropOutOfProject is set to true
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
 * If true, Natron will try to automatically connect the node to others depending on the user selection. If the property kCreateNodeArgsPropNodeSerialization is set, this has no effect.
 **/
#define kCreateNodeArgsPropAutoConnect "CreateNodeArgsPropAutoConnect"

/**
 * @brief optional x1 bool property
 * Default Value - true
 * If true, Natron will push a undo/redo command to the stack when creating this node. If the property kCreateNodeArgsPropNoNodeGUI is set to true or kCreateNodeArgsPropOutOfProject
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
 * @brief optional x1 bool property
 * Default value - false
 * If true, Natron will not attempt to convert the plug-in ID to another known plug-in ID. For example if trying to create an instance of PLUGINID_OFX_ROTO, then Natron will stick to it.
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
 * Defalt value - null
 **/
#define kCreateNodeArgsPropMetaNodeContainer "CreateNodeArgsPropMetaNodeContainer"

/**
 * @brief [DEPRECATED] optional x1 std::string For the old Tracker_PM node, we used this to indicate when creating a track (which was represented by a node internally)
 * what was the script-name of the parent node.
 * This is meaningless in Natron >= 2.1
 * Default Value - Empty
 **/
#define kCreateNodeArgsPropMultiInstanceParentName "CreateNodeArgsPropMultiInstanceParentName"

NATRON_NAMESPACE_ENTER

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
    boost::shared_ptr<Property<T> > getProp(const std::string& name, bool throwOnFailure = true) const
    {
        const boost::shared_ptr<PropertyBase>* propPtr = 0;

        std::map<std::string, boost::shared_ptr<PropertyBase> >::const_iterator found = _properties.find(name);
        if (found == _properties.end()) {
            if (throwOnFailure) {
                throw std::invalid_argument("CreateNodeArgs::getProp(): Invalid property " + name);
            }
            return boost::shared_ptr<Property<T> >();
        }
        propPtr = &(found->second);
        boost::shared_ptr<Property<T> > propTemplate;
        propTemplate = boost::dynamic_pointer_cast<Property<T> >(*propPtr);
        assert(propPtr);
        if (!propTemplate) {
            if (throwOnFailure) {
                throw std::invalid_argument("CreateNodeArgs::getProp(): Invalid property type for " + name);
            }
            return boost::shared_ptr<Property<T> >();
        }

        assert(propTemplate);
        return propTemplate;
    }


    template <typename T>
    boost::shared_ptr<Property<T> > createPropertyInternal(const std::string& name)
    {
        boost::shared_ptr<PropertyBase>* propPtr = 0;
        propPtr = &_properties[name];
        
        boost::shared_ptr<Property<T> > propTemplate;
        if (!*propPtr) {
            propTemplate.reset(new Property<T>);
            *propPtr = propTemplate;
        } else {
            propTemplate = boost::dynamic_pointer_cast<Property<T> >(*propPtr);
        }
        assert(propTemplate);
        return propTemplate;
    }

    template <typename T>
    boost::shared_ptr<Property<T> > createProperty(const std::string& name, const T& defaultValue)
    {
        boost::shared_ptr<Property<T> > p = createPropertyInternal<T>(name);
        p->value.push_back(defaultValue);
        return p;
    }

    template <typename T>
    boost::shared_ptr<Property<T> > createProperty(const std::string& name, const T& defaultValue1, const T& defaultValue2)
    {
        boost::shared_ptr<Property<T> > p = createPropertyInternal<T>(name);
        p->value.push_back(defaultValue1);
        p->value.push_back(defaultValue2);
        return p;
    }

    template <typename T>
    boost::shared_ptr<Property<T> > createProperty(const std::string& name, const std::vector<T>& defaultValue)
    {
        boost::shared_ptr<Property<T> > p = createPropertyInternal<T>(name);
        p->value = defaultValue;
        return p;
    }

    void createProperties()
    {
        createProperty<std::string>(kCreateNodeArgsPropPluginID, std::string());
        createProperty<int>(kCreateNodeArgsPropPluginVersion, -1, -1);
        createProperty<double>(kCreateNodeArgsPropNodeInitialPosition, (double)INT_MIN, (double)INT_MIN);
        createProperty<std::string>(kCreateNodeArgsPropNodeInitialName, std::string());
        createProperty<std::string>(kCreateNodeArgsPropNodeInitialParamValues, std::vector<std::string>());
        createProperty<boost::shared_ptr<NodeSerialization> >(kCreateNodeArgsPropNodeSerialization, boost::shared_ptr<NodeSerialization>());
        createProperty<bool>(kCreateNodeArgsPropDoNotLoadPyPlugFromScript, false);
        createProperty<bool>(kCreateNodeArgsPropOutOfProject, false);
        createProperty<bool>(kCreateNodeArgsPropNoNodeGUI, false);
        createProperty<bool>(kCreateNodeArgsPropSettingsOpened, true);
        createProperty<bool>(kCreateNodeArgsPropAutoConnect, true);
        createProperty<bool>(kCreateNodeArgsPropAllowNonUserCreatablePlugins, false);
        createProperty<bool>(kCreateNodeArgsPropSilent, false);
        createProperty<bool>(kCreateNodeArgsPropNodeGroupDisableCreateInitialNodes, false);
        createProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, true);
        createProperty<bool>(kCreateNodeArgsPropTrustPluginID, false);
        createProperty<boost::shared_ptr<NodeCollection> >(kCreateNodeArgsPropGroupContainer, boost::shared_ptr<NodeCollection>());
        createProperty<NodePtr>(kCreateNodeArgsPropMetaNodeContainer, NodePtr());
        createProperty<std::string>(kCreateNodeArgsPropMultiInstanceParentName, std::string());


    }

public:

    CreateNodeArgs();

    /**
     * @brief The constructor, taking values for all non-optional properties and the group
     **/
    CreateNodeArgs(const std::string& pluginID,
                   const boost::shared_ptr<NodeCollection>& group = boost::shared_ptr<NodeCollection>());

    ~CreateNodeArgs();

    template <typename T>
    void setProperty(const std::string& name, const T& value, int index = 0, bool failIfNotExisting = true)
    {
        boost::shared_ptr<Property<T> > propTemplate;
        propTemplate = getProp<T>(name, failIfNotExisting);
        if (!propTemplate) {
            propTemplate = createProperty<T>(name, value);
        }
        if (index >= (int)propTemplate->value.size()) {
            propTemplate->value.resize(index + 1);
        }
        propTemplate->value[index] = value;

    }

    template <typename T>
    void setPropertyN(const std::string& name, const std::vector<T>& values, bool failIfNotExisting = true)
    {
        boost::shared_ptr<Property<T> > propTemplate;
        propTemplate = getProp<T>(name, failIfNotExisting);
        if (!propTemplate) {
            propTemplate = createProperty<T>(name, values);
        }
        propTemplate->value = values;
    }

    int getPropertyDimension(const std::string& name, bool throwIfFailed = true) const
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


    template<typename T>
    T getProperty(const std::string& name, int index = 0) const
    {
        boost::shared_ptr<Property<T> > propTemplate = getProp<T>(name);
        if (index < 0 || index >= (int)propTemplate->value.size()) {
            throw std::invalid_argument("CreateNodeArgs::getProperty(): index out of range for " + name);
        }
        return propTemplate->value[index];

    }

    template<typename T>
    std::vector<T> getPropertyN(const std::string& name) const
    {
        boost::shared_ptr<Property<T> > propTemplate = getProp<T>(name);
        return propTemplate->value;

    }
private:

    std::map<std::string, boost::shared_ptr<PropertyBase> > _properties;
};

NATRON_NAMESPACE_EXIT

#endif // CREATENODEARGS_H
