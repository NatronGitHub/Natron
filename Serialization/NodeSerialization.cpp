/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "NodeSerialization.h"

#include <cmath>

#include <boost/make_shared.hpp>

GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
#include <yaml-cpp/yaml.h>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON

SERIALIZATION_NAMESPACE_ENTER

static double roundDecimals(double value, int decimals)
{
    int exp = std::pow(10, decimals);
    return std::floor(value * exp + 0.5) / exp;
}

static void serializeInputsMap(const std::map<std::string, std::string>& inputs, const std::string& tokenName, YAML::Emitter& em)
{
    bool hasInput = false;
    for (std::map<std::string, std::string>::const_iterator it = inputs.begin(); it!=inputs.end(); ++it) {
        if (!it->second.empty()) {
            hasInput = true;
            break;
        }
    }
    if (hasInput) {
        if (inputs.size() == 1) {
            em << YAML::Key << tokenName << YAML::Value << inputs.begin()->second;
        } else {
            em << YAML::Key << tokenName << YAML::Value << YAML::Flow << YAML::BeginMap;
            for (std::map<std::string, std::string>::const_iterator it = inputs.begin(); it!=inputs.end(); ++it) {
                if (!it->second.empty()) {
                    em << YAML::Key << it->first << YAML::Value << it->second;
                }
            }
            em << YAML::EndMap;
        }
    }
} // serializeInputsMap

void
ImagePlaneDescSerialization::encode(YAML::Emitter& em) const
{
    em << YAML::Flow << YAML::BeginMap;
    em << YAML::Key << "PlaneID" << YAML::Value << planeID;
    if (!planeLabel.empty()) {
        em << YAML::Key << "PlaneLabel" << YAML::Value << planeLabel;
    }
    if (!channelsLabel.empty()) {
        em << YAML::Key << "ChannelsLabel" << YAML::Value << channelsLabel;
    }
    em << YAML::Key << "Channels" << YAML::Value << YAML::Flow << YAML::BeginSeq;
    for (std::size_t i = 0; i < channelNames.size(); ++i) {
        em << channelNames[i];
    }
    em << YAML::EndSeq;
    em << YAML::EndMap;
}

void
ImagePlaneDescSerialization::decode(const YAML::Node& node)
{

    planeID = node["PlaneID"].as<std::string>();
    if (node["PlaneLabel"]) {
        planeLabel = node["PlaneLabel"].as<std::string>();
    }
    if (node["ChannelsLabel"]) {
        channelsLabel = node["ChannelsLabel"].as<std::string>();
    }
    const YAML::Node& channelsNode = node["Channels"];
    for (std::size_t i = 0; i < channelsNode.size(); ++i) {
        channelNames.push_back(channelsNode[i].as<std::string>());
    }
}


void
NodeSerialization::encode(YAML::Emitter& em) const
{
    em << YAML::BeginMap;

    assert(!_pluginID.empty());
    em << YAML::Key << "PluginID" << YAML::Value << _pluginID;

    //  Presets specific
    if (_encodeFlags & eNodeSerializationFlagsPreset) {
        em << YAML::Key << "PresetName" << YAML::Value << _presetsIdentifierLabel;
        if (!_presetsIconFilePath.empty()) {
            em << YAML::Key << "PresetIcon" << YAML::Value << _presetsIconFilePath;
        }
        if (_presetShortcutSymbol != 0) {
            em << YAML::Key << "PresetShortcutKey" << YAML::Value << _presetShortcutSymbol;
        }
        if (_presetShortcutPresetModifiers != 0) {
            em << YAML::Key << "PresetShortcutModifiers" << YAML::Value << _presetShortcutPresetModifiers;
        }

    }


    // Only serialize this for non pyplugs and non presets
    if ((_encodeFlags & (eNodeSerializationFlagsPreset | eNodeSerializationFlagsPyPlug)) == 0) {
        em << YAML::Key << "Name" << YAML::Value << _nodeScriptName;
        if (_nodeLabel != _nodeScriptName) {
            em << YAML::Key << "Label" << YAML::Value << _nodeLabel;
        }
    }


    // If version is 1.0 do not serialize
    if ((_encodeFlags & eNodeSerializationFlagsPyPlug) == 0  && ((_pluginMajorVersion != 1 && _pluginMajorVersion != -1) || (_pluginMinorVersion != 0 && _pluginMinorVersion != -1))) {
        em << YAML::Key << "Version" << YAML::Value << YAML::Flow << YAML::BeginSeq;
        em << _pluginMajorVersion << _pluginMinorVersion;
        em << YAML::EndSeq;
    }

    // When PyPlug or preset, no need to serialize inputs
    if ((_encodeFlags & (eNodeSerializationFlagsPreset | eNodeSerializationFlagsPyPlug)) == 0) {
        serializeInputsMap(_inputs, "Inputs", em);
        serializeInputsMap(_masks, "Masks", em);
    }

    if (!_outputs.empty()) {
        em << YAML::Key << "Outputs" << YAML::Value << YAML::BeginSeq;
        for (std::list<OutputNodeConnection>::const_iterator it = _outputs.begin(); it != _outputs.end(); ++it) {
            if (it->outputNodeIndices.size() == 0) {
                continue;
            }
            em << it->outputNodeScriptName;
            if (it->outputNodeIndices.size() == 1) {
                em << it->outputNodeIndices.front();
            } else {
                em << YAML::BeginSeq;
                for (std::list<int>::const_iterator it2 = it->outputNodeIndices.begin(); it2 != it->outputNodeIndices.end(); ++it2) {
                    em << *it2;
                }
                em << YAML::EndSeq;
            }
        }
        em << YAML::EndSeq;
    }

    if (!_knobsValues.empty()) {
        em << YAML::Key << "Params" << YAML::Value << YAML::BeginSeq;
        for (KnobSerializationList::const_iterator it = _knobsValues.begin(); it!=_knobsValues.end(); ++it) {
            (*it)->encode(em);
        }
        em << YAML::EndSeq;
    }

    if (!_userPages.empty()) {
        em << YAML::Key << "UserPages" << YAML::Value << YAML::BeginSeq;
        for (std::list<boost::shared_ptr<GroupKnobSerialization> >::const_iterator it = _userPages.begin(); it!=_userPages.end(); ++it) {
            (*it)->encode(em);
        }
        em << YAML::EndSeq;
    }

    if (!_pagesIndexes.empty()) {
        em << YAML::Key << "PagesOrder" << YAML::Value <<  YAML::Flow << YAML::BeginSeq;
        for (std::list<std::string>::const_iterator it = _pagesIndexes.begin(); it!=_pagesIndexes.end(); ++it) {
            em << *it;
        }
        em << YAML::EndSeq;
    }

    if (!_children.empty()) {
        em << YAML::Key << "Children" << YAML::Value << YAML::BeginSeq;
        for (NodeSerializationList::const_iterator it = _children.begin(); it!=_children.end(); ++it) {
            (*it)->encode(em);
        }
        em << YAML::EndSeq;
    }

    if (!_tables.empty()) {
        em << YAML::Key << "Tables" << YAML::Value;
        em << YAML::BeginSeq;
        for (std::list<KnobItemsTableSerializationPtr>::const_iterator it = _tables.begin(); it != _tables.end(); ++it) {
            (*it)->encode(em);
        }
        em << YAML::EndSeq;
    }

    if (!_presetInstanceLabel.empty()) {
        em << YAML::Key << "Preset" << YAML::Value << _presetInstanceLabel;
    }

    // Only serialize UI stuff for non pyplug/non presets
    if ((_encodeFlags & (eNodeSerializationFlagsPreset | eNodeSerializationFlagsPyPlug)) == 0) {
        if (_nodePositionCoords[0] != INT_MIN && _nodePositionCoords[1] != INT_MIN) {
            em << YAML::Key << "Pos" << YAML::Value << YAML::Flow << YAML::BeginSeq << roundDecimals(_nodePositionCoords[0], 1) << roundDecimals(_nodePositionCoords[1], 1) << YAML::EndSeq;
        }
        if (_nodeSize[0] != -1 && _nodeSize[1] != -1) {
            em << YAML::Key << "Size" << YAML::Value << YAML::Flow << YAML::BeginSeq << roundDecimals(_nodeSize[0], 1) << roundDecimals(_nodeSize[1], 1) << YAML::EndSeq;
        }
        if (_nodeColor[0] != -1 && _nodeColor[1] != -1 && _nodeColor[2] != -1) {
            em << YAML::Key << "Color" << YAML::Value << YAML::Flow << YAML::BeginSeq << _nodeColor[0] << _nodeColor[1] << _nodeColor[2] << YAML::EndSeq;
        }
        if (_overlayColor[0] != -1 && _overlayColor[1] != -1 && _overlayColor[2] != -1) {
            em << YAML::Key << "OverlayColor" << YAML::Value << YAML::Flow << YAML::BeginSeq << _overlayColor[0] << _overlayColor[1] << _overlayColor[2] << YAML::EndSeq;
        }
    }
    if (!_viewerUIKnobsOrder.empty()) {
        em << YAML::Key << "ViewerParamsOrder" << YAML::Value << YAML::Flow << YAML::BeginSeq;
        for (std::list<std::string>::const_iterator it = _viewerUIKnobsOrder.begin(); it!=_viewerUIKnobsOrder.end(); ++it) {
            em << *it;
        }
        em << YAML::EndSeq;
    }
    em << YAML::EndMap;
} // NodeSerialization::encode

static void tryDecodeInputsMap(const YAML::Node& node, const std::string& token, std::map<std::string, std::string>* container)
{
    if (node[token]) {
        const YAML::Node& inputsNode = node[token];
        if (inputsNode.IsMap()) {
            for (YAML::const_iterator it = inputsNode.begin(); it!=inputsNode.end(); ++it) {
                container->insert(std::make_pair(it->first.as<std::string>(), it->second.as<std::string>()));
            }
        } else {
            // When single input, just use the index as key
            container->insert(std::make_pair("0", inputsNode.as<std::string>()));
        }
    }

}

void
NodeSerialization::decode(const YAML::Node& node)
{
    if (!node.IsMap()) {
        throw YAML::InvalidNode();
    }


    _pluginID = node["PluginID"].as<std::string>();

    if (node["PresetName"]) {
        _encodeFlags = eNodeSerializationFlagsPreset;

        // This is a presets or pyplug
        _presetsIdentifierLabel = node["PresetName"].as<std::string>();
        if (node["PresetIcon"]) {
            _presetsIconFilePath = node["PresetIcon"].as<std::string>();
        }
        if (node["PresetShortcutKey"]) {
            _presetShortcutSymbol = node["PresetShortcutKey"].as<int>();
        }
        if (node["PresetShortcutModifiers"]) {
            _presetShortcutPresetModifiers = node["PresetShortcutModifiers"].as<int>();
        }
    }

    if (node["Name"]) {
        _nodeScriptName = node["Name"].as<std::string>();
    }
    
    if (node["Label"]) {
        _nodeLabel = node["Label"].as<std::string>();
    } else {
        _nodeLabel = _nodeScriptName;
    }

    if (node["Version"]) {
        const YAML::Node& versionNode = node["Version"];
        if (versionNode.size() != 2) {
            throw YAML::InvalidNode();
        }
        _pluginMajorVersion = versionNode[0].as<int>();
        _pluginMinorVersion = versionNode[1].as<int>();
    }

    tryDecodeInputsMap(node, "Inputs", &_inputs);
    tryDecodeInputsMap(node, "Masks", &_masks);

    if (node["Outputs"]) {
        const YAML::Node& outputsNode = node["Outputs"];
        bool expectName = true;
        OutputNodeConnection connection;
        for (std::size_t i = 0; i < outputsNode.size(); ++i) {
            if (expectName) {
                connection.outputNodeScriptName = outputsNode[i].as<std::string>();
                expectName = false;
            } else {
                if (outputsNode[i].IsSequence()) {
                    for (std::size_t j = 0; j < outputsNode[i].size(); ++j) {
                        connection.outputNodeIndices.push_back(outputsNode[i][j].as<int>());
                    }
                } else {
                    connection.outputNodeIndices.push_back(outputsNode[i].as<int>());
                }
                _outputs.push_back(connection);
                expectName = true;
            }
        }
    }

    
    if (node["Params"]) {
        const YAML::Node& paramsNode = node["Params"];
        for (std::size_t i = 0; i < paramsNode.size(); ++i) {
            KnobSerializationPtr s = boost::make_shared<KnobSerialization>();
            s->decode(paramsNode[i]);
            _knobsValues.push_back(s);
        }
    }
    if (node["UserPages"]) {
        const YAML::Node& pagesNode = node["UserPages"];
        for (std::size_t i = 0; i < pagesNode.size(); ++i) {
            GroupKnobSerializationPtr s = boost::make_shared<GroupKnobSerialization>();
            s->decode(pagesNode[i]);
            _userPages.push_back(s);
        }
    }
    if (node["PagesOrder"]) {
        const YAML::Node& pagesOrder = node["PagesOrder"];
        for (std::size_t i = 0; i < pagesOrder.size(); ++i) {
            _pagesIndexes.push_back(pagesOrder[i].as<std::string>());
        }
    }
    if (node["Children"]) {
        const YAML::Node& childrenNode = node["Children"];
        for (std::size_t i = 0; i < childrenNode.size(); ++i) {
            NodeSerializationPtr s = boost::make_shared<NodeSerialization>();
            s->decode(childrenNode[i]);
            _children.push_back(s);
        }
    }
    if (node["Tables"]) {
        const YAML::Node& tablesNode = node["Tables"];
        for (std::size_t i = 0; i < tablesNode.size(); ++i) {
            KnobItemsTableSerializationPtr table = boost::make_shared<KnobItemsTableSerialization>();
            table->decode(tablesNode[i]);
            _tables.push_back(table);
        }
    }
    
    if (node["Preset"]) {
        _presetInstanceLabel = node["Preset"].as<std::string>();
    }
    

    if (node["Pos"]) {
        const YAML::Node& posNode = node["Pos"];
        if (posNode.size() != 2) {
            throw YAML::InvalidNode();
        }
        _nodePositionCoords[0] = posNode[0].as<double>();
        _nodePositionCoords[1] = posNode[1].as<double>();
    }
    if (node["Size"]) {
        const YAML::Node& sizeNode = node["Size"];
        if (sizeNode.size() != 2) {
            throw YAML::InvalidNode();
        }
        _nodeSize[0] = sizeNode[0].as<double>();
        _nodeSize[1] = sizeNode[1].as<double>();
    }
    if (node["Color"]) {
        const YAML::Node& colorNode = node["Color"];
        if (colorNode.size() != 3) {
            throw YAML::InvalidNode();
        }
        _nodeColor[0] = colorNode[0].as<double>();
        _nodeColor[1] = colorNode[1].as<double>();
        _nodeColor[2] = colorNode[2].as<double>();
    }
    if (node["OverlayColor"]) {
        const YAML::Node& colorNode = node["OverlayColor"];
        if (colorNode.size() != 3) {
            throw YAML::InvalidNode();
        }
        _overlayColor[0] = colorNode[0].as<double>();
        _overlayColor[1] = colorNode[1].as<double>();
        _overlayColor[2] = colorNode[2].as<double>();
    }
    if (node["ViewerParamsOrder"]) {
        const YAML::Node& viewerParamsOrderNode = node["ViewerParamsOrder"];
        for (std::size_t i = 0; i < viewerParamsOrderNode.size(); ++i) {
            _viewerUIKnobsOrder.push_back(viewerParamsOrderNode[i].as<std::string>());
        }
    }


} // NodeSerialization::decode


SERIALIZATION_NAMESPACE_EXIT



