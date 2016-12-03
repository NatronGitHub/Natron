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

#ifndef NODESERIALIZATION_H
#define NODESERIALIZATION_H

#include <climits>

#include "Serialization/KnobSerialization.h"
#include "Serialization/KnobTableItemSerialization.h"
#include "Serialization/ImageParamsSerialization.h"
#include "Serialization/SerializationBase.h"
#include "Serialization/SerializationFwd.h"


SERIALIZATION_NAMESPACE_ENTER;

/**
 * @class This is the main class for everything related to the serialization of nodes.
 * It use for the purpose of 3 serialization kinds:
 * - Serialization of a node in a project
 * - Serialization of a PyPlug
 * - Serialization of a Preset of a node
 **/
class NodeSerialization : public SerializationObjectBase
{
public:

    enum NodeSerializationTypeEnum
    {
        eNodeSerializationTypeRegular,
        eNodeSerializationTypePyPlug,
        eNodeSerializationTypePresets
    };

    explicit NodeSerialization()
    : _encodeType(eNodeSerializationTypeRegular)
    , _presetsIdentifierLabel()
    , _presetsIconFilePath()
    , _presetShortcutSymbol(0)
    , _presetShortcutPresetModifiers(0)
    , _presetInstanceLabel()
    , _knobsValues()
    , _groupFullyQualifiedScriptName()
    , _nodeLabel()
    , _nodeScriptName()
    , _pluginID()
    , _pluginMajorVersion(-1)
    , _pluginMinorVersion(-1)
    , _masterNodecriptName()
    , _inputs()
    , _tableModel()
    , _userPages()
    , _pagesIndexes()
    , _children()
    , _userComponents()
    , _nodePositionCoords()
    , _nodeSize()
    , _nodeColor()
    , _overlayColor()
    , _viewerUIKnobsOrder()
    {
        _nodePositionCoords[0] = _nodePositionCoords[1] = INT_MIN;
        _nodeSize[0] = _nodeSize[1] = -1;
        _nodeColor[0] = _nodeColor[1] = _nodeColor[2] = -1;
        _overlayColor[0] = _overlayColor[1] = _overlayColor[2] = -1;

    }


    virtual ~NodeSerialization()
    {
    }

    // This is a hint for the encode() function so it skips
    // serializing unneeded stuff given the type
    NodeSerializationTypeEnum _encodeType;

    //////// Presets only
    ///////////////////////////////////////////////////////
    // Preset name
    std::string _presetsIdentifierLabel;

    // Optionally the filename of an icon file (without path) that should be located next to the preset file
    std::string _presetsIconFilePath;

    // Optionally the default keyboard symbol (as in Natron::Key) and modifiers (as in Natron::KeyboardModifiers)
    // that should by default be used to instantiate the plug-in. (Note that the user may still change it afterwards
    // from the Shortcut Editor)
    int _presetShortcutSymbol;
    int _presetShortcutPresetModifiers;
    ///////////////////////////////////////////////////////
    ////////// End presets related

    // If this node is using a preset, this is the label of the preset it was instantiated in
    std::string _presetInstanceLabel;

    // Knobs serialization
    KnobSerializationList _knobsValues;

    // The group full script-name or empty if the node is part of the top-level group
    std::string _groupFullyQualifiedScriptName;

    // The node label as seen in the nodegraph
    std::string _nodeLabel;

    // The node script-name as used in Python
    std::string _nodeScriptName;

    // The ID of the plug-in embedded into the node
    std::string _pluginID;

    // Plugin version used by the node, -1 means highest loaded in the application
    int _pluginMajorVersion;
    int _pluginMinorVersion;

    // If this node is a clone of another one, this is the script-name of the master node
    std::string _masterNodecriptName;

    // Serialization of inputs, this is a map of the input label to the script-name (not full) of the input node
    std::map<std::string, std::string> _inputs, _masks;

    // If this node has an item model (Roto, tracker...), this points to its serialization
    KnobItemsTableSerializationPtr _tableModel;

    // The serialization of the pages created by the user
    std::list<boost::shared_ptr<GroupKnobSerialization> > _userPages;

    // The pages order in the node by script-name
    std::list<std::string> _pagesIndexes;

    // If this node is a group or a multi-instance, this is the children
    NodeSerializationList _children;

    // This is the user created components on the node
    std::list<ImageComponentsSerialization> _userComponents;

    // UI stuff
    double _nodePositionCoords[2]; // x,y  X=Y=INT_MIN if there is no position info
    double _nodeSize[2]; // width, height, W=H=-1 if there is no size info
    double _nodeColor[3]; // node color (RGB), between 0. and 1. If R=G=B=-1 then no color
    double _overlayColor[3]; // overlay color (RGB), between 0. and 1. If R=G=B=-1 then no color

    // Ordering of the knobs in the viewer UI for this node
    std::list<std::string> _viewerUIKnobsOrder;

    virtual void encode(YAML::Emitter& em) const OVERRIDE;

    virtual void decode(const YAML::Node& node) OVERRIDE;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
};


SERIALIZATION_NAMESPACE_EXIT;

#endif // NODESERIALIZATION_H
