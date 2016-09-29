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

#include "NodeSerialization.h"

GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
#include <yaml-cpp/yaml.h>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON

SERIALIZATION_NAMESPACE_ENTER

void
NodeSerialization::encode(YAML::Emitter& em) const
{
    em << YAML::BeginMap;


    // For pyplugs, don't serialize it, this is always a group anyway
    if (_encodeType != eNodeSerializationTypePyPlug) {
        em << YAML::Key << "PluginID" << YAML::Value << _pluginID;
    }
    //  Presets specific
    if (_encodeType == eNodeSerializationTypePresets) {
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
    if (_encodeType == eNodeSerializationTypeRegular) {
        em << YAML::Key << "ScriptName" << YAML::Value << _nodeScriptName;
        if (_nodeLabel != _nodeScriptName) {
            em << YAML::Key << "Label" << YAML::Value << _nodeLabel;
        }
    }


    // If version is 1.0 do not serialize
    if (_encodeType != eNodeSerializationTypePyPlug  && ((_pluginMajorVersion != 1 && _pluginMajorVersion != -1) || (_pluginMinorVersion != 0 && _pluginMinorVersion != -1))) {
        em << YAML::Key << "Version" << YAML::Value << YAML::Flow << YAML::BeginSeq;
        em << _pluginMajorVersion << _pluginMinorVersion;
        em << YAML::EndSeq;
    }

    bool hasInput = false;
    for (std::map<std::string, std::string>::const_iterator it = _inputs.begin(); it!=_inputs.end(); ++it) {
        if (!it->second.empty()) {
            hasInput = true;
            break;
        }
    }
    if (hasInput) {
        em << YAML::Key << "Inputs" << YAML::Value << YAML::Flow << YAML::BeginMap;
        for (std::map<std::string, std::string>::const_iterator it = _inputs.begin(); it!=_inputs.end(); ++it) {
            if (!it->second.empty()) {
                em << YAML::Key << it->first << YAML::Value << it->second;
            }
        }
        em << YAML::EndMap;
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

    if (_rotoContext) {
        em << YAML::Key << "Roto" << YAML::Value;
        _rotoContext->encode(em);
    }

    if (_trackerContext) {
        em << YAML::Key << "Tracks" << YAML::Value;
        _trackerContext->encode(em);
    }

     // Only serialize clone stuff for non pyplug/non presets
    if (_encodeType == eNodeSerializationTypeRegular && !_masterNodecriptName.empty()) {
        em << YAML::Key << "CloneMaster" << YAML::Value << _masterNodecriptName;
    }

    if (!_presetInstanceLabel.empty()) {
        em << YAML::Key << "Preset" << YAML::Value << _presetInstanceLabel;
    }

    // Only serialize created components for non pyplugs/non presets
    if (_encodeType == eNodeSerializationTypeRegular && !_userComponents.empty()) {
        em << YAML::Key << "NewLayers" << YAML::Value << YAML::Flow << YAML::BeginSeq;
        for (std::list<ImageComponentsSerialization>::const_iterator it = _userComponents.begin(); it!=_userComponents.end(); ++it) {
            it->encode(em);
        }
        em << YAML::EndSeq;
    }

    // Only serialize UI stuff for non pyplug/non presets
    if (_encodeType == eNodeSerializationTypeRegular) {
        if (_nodePositionCoords[0] != INT_MIN && _nodePositionCoords[1] != INT_MIN) {
            em << YAML::Key << "Pos" << YAML::Value << YAML::Flow << YAML::BeginSeq << _nodePositionCoords[0] << _nodePositionCoords[1] << YAML::EndSeq;
        }
        if (_nodeSize[0] != -1 && _nodeSize[1] != -1) {
            em << YAML::Key << "Size" << YAML::Value << YAML::Flow << YAML::BeginSeq << _nodeSize[0] << _nodeSize[1] << YAML::EndSeq;
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

void
NodeSerialization::decode(const YAML::Node& node)
{
    if (!node.IsMap()) {
        throw YAML::InvalidNode();
    }

    if (node["PluginID"]) {
        _pluginID = node["PluginID"].as<std::string>();
        _encodeType = eNodeSerializationTypeRegular;
    } else {
        // PyPlugs are uniquely identified by the fact that they don't serialize the script name of the plug-in directly
        _encodeType = eNodeSerializationTypePyPlug;
    }

    if (node["PresetName"]) {
        _encodeType = eNodeSerializationTypePresets;

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

    if (node["ScriptName"]) {
        _nodeScriptName = node["ScriptName"].as<std::string>();
    }
    
    if (node["Label"]) {
        _nodeLabel = node["Label"].as<std::string>();
    } else {
        _nodeLabel = _nodeScriptName;
    }

    if (node["Version"]) {
        YAML::Node versionNode = node["Version"];
        if (versionNode.size() != 2) {
            throw YAML::InvalidNode();
        }
        _pluginMajorVersion = versionNode[0].as<int>();
        _pluginMinorVersion = versionNode[1].as<int>();
    }

    if (node["Inputs"]) {
        YAML::Node inputsNode = node["Inputs"];
        for (YAML::const_iterator it = inputsNode.begin(); it!=inputsNode.end(); ++it) {
            _inputs.insert(std::make_pair(it->first.as<std::string>(), it->second.as<std::string>()));
        }
    }
    
    if (node["Params"]) {
        YAML::Node paramsNode = node["Params"];
        for (std::size_t i = 0; i < paramsNode.size(); ++i) {
            KnobSerializationPtr s(new KnobSerialization);
            s->decode(paramsNode[i]);
            _knobsValues.push_back(s);
        }
    }
    if (node["UserPages"]) {
        YAML::Node pagesNode = node["UserPages"];
        for (std::size_t i = 0; i < pagesNode.size(); ++i) {
            GroupKnobSerializationPtr s(new GroupKnobSerialization);
            s->decode(pagesNode[i]);
            _userPages.push_back(s);
        }
    }
    if (node["PagesOrder"]) {
        YAML::Node pagesOrder = node["PagesOrder"];
        for (std::size_t i = 0; i < pagesOrder.size(); ++i) {
            _pagesIndexes.push_back(pagesOrder[i].as<std::string>());
        }
    }
    if (node["Children"]) {
        YAML::Node childrenNode = node["Children"];
        for (std::size_t i = 0; i < childrenNode.size(); ++i) {
            NodeSerializationPtr s(new NodeSerialization);
            s->decode(childrenNode[i]);
            _children.push_back(s);
        }
    }
    if (node["Roto"]) {
        _rotoContext.reset(new RotoContextSerialization);
        _rotoContext->decode(node["Roto"]);
    }
    if (node["Tracks"]) {
        _trackerContext.reset(new TrackerContextSerialization);
        _trackerContext->decode(node["Tracks"]);
    }
    if (node["CloneMaster"]) {
        _masterNodecriptName = node["CloneMaster"].as<std::string>();
    }
    
    if (node["Preset"]) {
        _presetInstanceLabel = node["Preset"].as<std::string>();
    }
    
    if (node["NewLayers"]) {
        YAML::Node layersNode = node["NewLayers"];
        for (std::size_t i = 0; i < layersNode.size(); ++i) {
            ImageComponentsSerialization s;
            s.decode(layersNode[i]);
            _userComponents.push_back(s);
        }
    }
    if (node["Pos"]) {
        YAML::Node posNode = node["Pos"];
        if (posNode.size() != 2) {
            throw YAML::InvalidNode();
        }
        _nodePositionCoords[0] = posNode[0].as<double>();
        _nodePositionCoords[1] = posNode[1].as<double>();
    }
    if (node["Size"]) {
        YAML::Node sizeNode = node["Size"];
        if (sizeNode.size() != 2) {
            throw YAML::InvalidNode();
        }
        _nodeSize[0] = sizeNode[0].as<double>();
        _nodeSize[1] = sizeNode[1].as<double>();
    }
    if (node["Color"]) {
        YAML::Node colorNode = node["Color"];
        if (colorNode.size() != 3) {
            throw YAML::InvalidNode();
        }
        _nodeColor[0] = colorNode[0].as<double>();
        _nodeColor[1] = colorNode[1].as<double>();
        _nodeColor[2] = colorNode[2].as<double>();
    }
    if (node["OverlayColor"]) {
        YAML::Node colorNode = node["OverlayColor"];
        if (colorNode.size() != 3) {
            throw YAML::InvalidNode();
        }
        _overlayColor[0] = colorNode[0].as<double>();
        _overlayColor[1] = colorNode[1].as<double>();
        _overlayColor[2] = colorNode[2].as<double>();
    }
    if (node["ViewerParamsOrder"]) {
        YAML::Node viewerParamsOrderNode = node["ViewerParamsOrder"];
        for (std::size_t i = 0; i < viewerParamsOrderNode.size(); ++i) {
            _viewerUIKnobsOrder.push_back(viewerParamsOrderNode[i].as<std::string>());
        }
    }


} // NodeSerialization::decode


SERIALIZATION_NAMESPACE_EXIT



