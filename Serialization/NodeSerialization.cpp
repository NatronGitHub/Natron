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

SERIALIZATION_NAMESPACE_ENTER

void
NodeSerialization::encode(YAML_NAMESPACE::Emitter& em) const
{
    em << YAML_NAMESPACE::BeginMap;
    em << YAML_NAMESPACE::Key << "PluginID" << YAML_NAMESPACE::Value << _pluginID;
    em << YAML_NAMESPACE::Key << "ScriptName" << YAML_NAMESPACE::Value << _nodeScriptName;
    if (_nodeLabel != _nodeScriptName) {
        em << YAML_NAMESPACE::Key << "Label" << YAML_NAMESPACE::Value << _nodeLabel;
    }

    std::string fullyQualifiedName = _groupFullyQualifiedScriptName;
    if (!fullyQualifiedName.empty()) {
        fullyQualifiedName += ".";
    }
    fullyQualifiedName += _nodeScriptName;
    if (_cacheID != fullyQualifiedName) {
        em << YAML_NAMESPACE::Key << "CacheID" << YAML_NAMESPACE::Value << _cacheID;
    }

    // If version is 1.0 do not serialize
    if ((_pluginMajorVersion != 1 && _pluginMajorVersion != -1) || (_pluginMinorVersion != 0 && _pluginMinorVersion != -1)) {
        em << YAML_NAMESPACE::Key << "Version" << YAML_NAMESPACE::Value << YAML_NAMESPACE::Flow << YAML_NAMESPACE::BeginSeq;
        em << _pluginMajorVersion << _pluginMinorVersion;
        em << YAML_NAMESPACE::EndSeq;
    }

    bool hasInput = false;
    for (std::map<std::string, std::string>::const_iterator it = _inputs.begin(); it!=_inputs.end(); ++it) {
        if (!it->second.empty()) {
            hasInput = true;
            break;
        }
    }
    if (hasInput) {
        em << YAML_NAMESPACE::Key << "Inputs" << YAML_NAMESPACE::Value << YAML_NAMESPACE::Flow << YAML_NAMESPACE::BeginMap;
        for (std::map<std::string, std::string>::const_iterator it = _inputs.begin(); it!=_inputs.end(); ++it) {
            if (!it->second.empty()) {
                em << YAML_NAMESPACE::Key << it->first << YAML_NAMESPACE::Value << it->second;
            }
        }
        em << YAML_NAMESPACE::EndMap;
    }

    if (_knobsAge != 0) {
        em << YAML_NAMESPACE::Key << "Age" << YAML_NAMESPACE::Value << _knobsAge;
    }

    if (!_knobsValues.empty()) {
        em << YAML_NAMESPACE::Key << "Params" << YAML_NAMESPACE::Value << YAML_NAMESPACE::BeginSeq;
        for (KnobSerializationList::const_iterator it = _knobsValues.begin(); it!=_knobsValues.end(); ++it) {
            (*it)->encode(em);
        }
        em << YAML_NAMESPACE::EndSeq;
    }

    if (!_userPages.empty()) {
        em << YAML_NAMESPACE::Key << "UserPages" << YAML_NAMESPACE::Value << YAML_NAMESPACE::BeginSeq;
        for (std::list<boost::shared_ptr<GroupKnobSerialization> >::const_iterator it = _userPages.begin(); it!=_userPages.end(); ++it) {
            (*it)->encode(em);
        }
        em << YAML_NAMESPACE::EndSeq;
    }

    if (!_pagesIndexes.empty()) {
        em << YAML_NAMESPACE::Key << "PagesOrder" << YAML_NAMESPACE::Value <<  YAML_NAMESPACE::Flow << YAML_NAMESPACE::BeginSeq;
        for (std::list<std::string>::const_iterator it = _pagesIndexes.begin(); it!=_pagesIndexes.end(); ++it) {
            em << *it;
        }
        em << YAML_NAMESPACE::EndSeq;
    }

    if (!_children.empty()) {
        em << YAML_NAMESPACE::Key << "Children" << YAML_NAMESPACE::Value << YAML_NAMESPACE::BeginSeq;
        for (NodeSerializationList::const_iterator it = _children.begin(); it!=_children.end(); ++it) {
            (*it)->encode(em);
        }
        em << YAML_NAMESPACE::EndSeq;
    }

    if (_rotoContext) {
        em << YAML_NAMESPACE::Key << "Roto" << YAML_NAMESPACE::Value;
        _rotoContext->encode(em);
    }

    if (_trackerContext) {
        em << YAML_NAMESPACE::Key << "Tracks" << YAML_NAMESPACE::Value;
        _trackerContext->encode(em);
    }

    if (!_masterNodeFullyQualifiedScriptName.empty()) {
        em << YAML_NAMESPACE::Key << "CloneMaster" << YAML_NAMESPACE::Value << _masterNodeFullyQualifiedScriptName;
    }

    if (!_pythonModule.empty()) {
        em << YAML_NAMESPACE::Key << "PyPlug" << YAML_NAMESPACE::Value << _pythonModule;
        if (((int)_pythonModuleVersion != 1) && ((int)_pythonModuleVersion != -1)) {
            em << YAML_NAMESPACE::Key << "PyPlugVersion" << YAML_NAMESPACE::Value << _pythonModuleVersion;
        }
    }
    
    if (!_presetLabel.empty()) {
        em << YAML_NAMESPACE::Key << "Preset" << YAML_NAMESPACE::Value << _presetLabel;
    }

    if (!_userComponents.empty()) {
        em << YAML_NAMESPACE::Key << "NewLayers" << YAML_NAMESPACE::Value << YAML_NAMESPACE::Flow << YAML_NAMESPACE::BeginSeq;
        for (std::list<ImageComponentsSerialization>::const_iterator it = _userComponents.begin(); it!=_userComponents.end(); ++it) {
            it->encode(em);
        }
        em << YAML_NAMESPACE::EndSeq;
    }

    if (_nodePositionCoords[0] != INT_MIN && _nodePositionCoords[1] != INT_MIN) {
        em << YAML_NAMESPACE::Key << "Pos" << YAML_NAMESPACE::Value << YAML_NAMESPACE::Flow << YAML_NAMESPACE::BeginSeq << _nodePositionCoords[0] << _nodePositionCoords[1] << YAML_NAMESPACE::EndSeq;
    }
    if (_nodeSize[0] != -1 && _nodeSize[1] != -1) {
        em << YAML_NAMESPACE::Key << "Size" << YAML_NAMESPACE::Value << YAML_NAMESPACE::Flow << YAML_NAMESPACE::BeginSeq << _nodeSize[0] << _nodeSize[1] << YAML_NAMESPACE::EndSeq;
    }
    if (_nodeColor[0] != -1 && _nodeColor[1] != -1 && _nodeColor[2] != -1) {
        em << YAML_NAMESPACE::Key << "Color" << YAML_NAMESPACE::Value << YAML_NAMESPACE::Flow << YAML_NAMESPACE::BeginSeq << _nodeColor[0] << _nodeColor[1] << _nodeColor[2] << YAML_NAMESPACE::EndSeq;
    }
    if (_overlayColor[0] != -1 && _overlayColor[1] != -1 && _overlayColor[2] != -1) {
        em << YAML_NAMESPACE::Key << "OverlayColor" << YAML_NAMESPACE::Value << YAML_NAMESPACE::Flow << YAML_NAMESPACE::BeginSeq << _overlayColor[0] << _overlayColor[1] << _overlayColor[2] << YAML_NAMESPACE::EndSeq;
    }

    if (!_viewerUIKnobsOrder.empty()) {
        em << YAML_NAMESPACE::Key << "ViewerParamsOrder" << YAML_NAMESPACE::Value << YAML_NAMESPACE::Flow << YAML_NAMESPACE::BeginSeq;
        for (std::list<std::string>::const_iterator it = _viewerUIKnobsOrder.begin(); it!=_viewerUIKnobsOrder.end(); ++it) {
            em << *it;
        }
        em << YAML_NAMESPACE::EndSeq;
    }
    em << YAML_NAMESPACE::EndMap;
} // NodeSerialization::encode

void
NodeSerialization::decode(const YAML_NAMESPACE::Node& node)
{
    if (!node.IsMap()) {
        throw YAML_NAMESPACE::InvalidNode();
    }

    _pluginID = node["PluginID"].as<std::string>();
    _nodeScriptName = node["ScriptName"].as<std::string>();
    if (node["Label"]) {
        _nodeLabel = node["Label"].as<std::string>();
    } else {
        _nodeLabel = _nodeScriptName;
    }
    if (node["CacheID"]) {
        _cacheID = node["CacheID"].as<std::string>();
    } else {
        _cacheID = _nodeScriptName;
    }
    if (node["Version"]) {
        YAML_NAMESPACE::Node versionNode = node["Version"];
        if (versionNode.size() != 2) {
            throw YAML_NAMESPACE::InvalidNode();
        }
        _pluginMajorVersion = versionNode[0].as<int>();
        _pluginMinorVersion = versionNode[1].as<int>();
    }

    if (node["Inputs"]) {
        YAML_NAMESPACE::Node inputsNode = node["Inputs"];
        for (YAML_NAMESPACE::const_iterator it = inputsNode.begin(); it!=inputsNode.end(); ++it) {
            _inputs.insert(std::make_pair(it->first.as<std::string>(), it->second.as<std::string>()));
        }
    }

    if (node["Age"]) {
        _knobsAge = node["Age"].as<unsigned long long>();
    }

    if (node["Params"]) {
        YAML_NAMESPACE::Node paramsNode = node["Params"];
        for (std::size_t i = 0; i < paramsNode.size(); ++i) {
            KnobSerializationPtr s(new KnobSerialization);
            s->decode(paramsNode[i]);
            _knobsValues.push_back(s);
        }
    }
    if (node["UserPages"]) {
        YAML_NAMESPACE::Node pagesNode = node["UserPages"];
        for (std::size_t i = 0; i < pagesNode.size(); ++i) {
            GroupKnobSerializationPtr s(new GroupKnobSerialization);
            s->decode(pagesNode[i]);
            _userPages.push_back(s);
        }
    }
    if (node["PagesOrder"]) {
        YAML_NAMESPACE::Node pagesOrder = node["PagesOrder"];
        for (std::size_t i = 0; i < pagesOrder.size(); ++i) {
            _pagesIndexes.push_back(pagesOrder[i].as<std::string>());
        }
    }
    if (node["Children"]) {
        YAML_NAMESPACE::Node childrenNode = node["Children"];
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
        _masterNodeFullyQualifiedScriptName = node["CloneMaster"].as<std::string>();
    }
    if (node["PyPlug"]) {
        _pythonModule = node["PyPlug"].as<std::string>();
        if (node["PyPlugVersion"]) {
            _pythonModuleVersion = node["PyPlugVersion"].as<int>();
        }
    }

    if (node["Preset"]) {
        _presetLabel = node["Preset"].as<std::string>();
    }
    
    if (node["NewLayers"]) {
        YAML_NAMESPACE::Node layersNode = node["NewLayers"];
        for (std::size_t i = 0; i < layersNode.size(); ++i) {
            ImageComponentsSerialization s;
            s.decode(layersNode[i]);
            _userComponents.push_back(s);
        }
    }
    if (node["Pos"]) {
        YAML_NAMESPACE::Node posNode = node["Pos"];
        if (posNode.size() != 2) {
            throw YAML_NAMESPACE::InvalidNode();
        }
        _nodePositionCoords[0] = posNode[0].as<double>();
        _nodePositionCoords[1] = posNode[1].as<double>();
    }
    if (node["Size"]) {
        YAML_NAMESPACE::Node sizeNode = node["Size"];
        if (sizeNode.size() != 2) {
            throw YAML_NAMESPACE::InvalidNode();
        }
        _nodeSize[0] = sizeNode[0].as<double>();
        _nodeSize[1] = sizeNode[1].as<double>();
    }
    if (node["Color"]) {
        YAML_NAMESPACE::Node colorNode = node["Color"];
        if (colorNode.size() != 3) {
            throw YAML_NAMESPACE::InvalidNode();
        }
        _nodeColor[0] = colorNode[0].as<double>();
        _nodeColor[1] = colorNode[1].as<double>();
        _nodeColor[2] = colorNode[2].as<double>();
    }
    if (node["OverlayColor"]) {
        YAML_NAMESPACE::Node colorNode = node["OverlayColor"];
        if (colorNode.size() != 3) {
            throw YAML_NAMESPACE::InvalidNode();
        }
        _overlayColor[0] = colorNode[0].as<double>();
        _overlayColor[1] = colorNode[1].as<double>();
        _overlayColor[2] = colorNode[2].as<double>();
    }
    if (node["ViewerParamsOrder"]) {
        YAML_NAMESPACE::Node viewerParamsOrderNode = node["ViewerParamsOrder"];
        for (std::size_t i = 0; i < viewerParamsOrderNode.size(); ++i) {
            _viewerUIKnobsOrder.push_back(viewerParamsOrderNode[i].as<std::string>());
        }
    }


} // NodeSerialization::decode

void
NodePresetSerialization::encode(YAML_NAMESPACE::Emitter& em) const
{
    em << YAML_NAMESPACE::BeginMap;
    em << YAML_NAMESPACE::Key << "PluginID" << YAML_NAMESPACE::Value << pluginID;
    em << YAML_NAMESPACE::Key << "PresetLabel" << YAML_NAMESPACE::Value << presetLabel;
    if (!presetIcon.empty()) {
        em << YAML_NAMESPACE::Key << "Icon" << YAML_NAMESPACE::Value << presetIcon;
    }
    if (presetSymbol != 0) {
        em << YAML_NAMESPACE::Key << "Key" << YAML_NAMESPACE::Value << presetSymbol;
    }
    if (presetModifiers != 0) {
        em << YAML_NAMESPACE::Key << "Modifiers" << YAML_NAMESPACE::Value << presetModifiers;
    }
    em << YAML_NAMESPACE::Key << "Node" << YAML_NAMESPACE::Value;
    node.encode(em);
    em << YAML_NAMESPACE::EndMap;
};

void
NodePresetSerialization::decode(const YAML_NAMESPACE::Node& node)
{
    if (!node.IsMap()) {
        throw YAML_NAMESPACE::InvalidNode();
    }
    pluginID = node["PluginID"].as<std::string>();
    presetLabel = node["PresetLabel"].as<std::string>();
    if (node["Icon"]) {
        presetIcon = node["Icon"].as<std::string>();
    }
    if (node["Key"]) {
        presetSymbol = node["Key"].as<int>();
    }
    if (node["Modifiers"]) {
        presetModifiers = node["Modifiers"].as<int>();
    }
    presetLabel = node["PresetLabel"].as<std::string>();
    if (decodeMetaDataOnly) {
        return;
    }
    this->node.decode(node["Node"]);
}

SERIALIZATION_NAMESPACE_EXIT



