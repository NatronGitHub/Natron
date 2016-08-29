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
    em << YAML::Key << "PluginID" << YAML::Value << _pluginID;
    em << YAML::Key << "ScriptName" << YAML::Value << _nodeScriptName;
    if (_nodeLabel != _nodeScriptName) {
        em << YAML::Key << "Label" << YAML::Value << _nodeLabel;
    }

    std::string fullyQualifiedName = _groupFullyQualifiedScriptName;
    if (!fullyQualifiedName.empty()) {
        fullyQualifiedName += ".";
    }
    fullyQualifiedName += _nodeScriptName;
    if (_cacheID != fullyQualifiedName) {
        em << YAML::Key << "CacheID" << YAML::Value << _cacheID;
    }

    // If version is 1.0 do not serialize
    if ((_pluginMajorVersion != 1 && _pluginMajorVersion != -1) || (_pluginMinorVersion != 0 && _pluginMinorVersion != -1)) {
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
            em << YAML::Key << it->first << YAML::Value << it->second;
        }
        em << YAML::EndMap;
    }

    if (_knobsAge != 0) {
        em << YAML::Key << "Age" << YAML::Value << _knobsAge;
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
        em << YAML::Key << "PagesOrder" << YAML::Value << YAML::BeginSeq;
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

    if (!_masterNodeFullyQualifiedScriptName.empty()) {
        em << YAML::Key << "CloneMaster" << YAML::Value << _masterNodeFullyQualifiedScriptName;
    }

    if (!_pythonModule.empty()) {
        em << YAML::Key << "PyPlug" << YAML::Value << _pythonModule;
        if (((int)_pythonModuleVersion != 1) && ((int)_pythonModuleVersion != -1)) {
            em << YAML::Key << "PyPlugVersion" << YAML::Value << _pythonModuleVersion;
        }
    }

    if (!_userComponents.empty()) {
        em << YAML::Key << "NewLayers" << YAML::Value << YAML::Flow << YAML::BeginSeq;
        for (std::list<ImageComponentsSerialization>::const_iterator it = _userComponents.begin(); it!=_userComponents.end(); ++it) {
            it->encode(em);
        }
        em << YAML::EndSeq;
    }

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

    if (node["Age"]) {
        _knobsAge = node["Age"].as<unsigned long long>();
    }

    if (node["Params"]) {
        YAML::Node paramsNode = node["Params"];
        for (YAML::const_iterator it = paramsNode.begin(); it!=paramsNode.end(); ++it) {
            KnobSerializationPtr s(new KnobSerialization);
            s->decode(it->second);
            _knobsValues.push_back(s);
        }
    }
    if (node["UserPages"]) {
        YAML::Node pagesNode = node["UserPages"];
        for (YAML::const_iterator it = pagesNode.begin(); it!=pagesNode.end(); ++it) {
            GroupKnobSerializationPtr s(new GroupKnobSerialization);
            s->decode(it->second);
            _userPages.push_back(s);
        }
    }
    if (node["PagesOrder"]) {
        YAML::Node pagesOrder = node["PagesOrder"];
        for (YAML::const_iterator it = pagesOrder.begin(); it!=pagesOrder.end(); ++it) {
            _pagesIndexes.push_back(it->second.as<std::string>());
        }
    }
    if (node["Children"]) {
        YAML::Node childrenNode = node["Children"];
        for (YAML::const_iterator it = childrenNode.begin(); it!=childrenNode.end(); ++it) {
            NodeSerializationPtr s(new NodeSerialization);
            s->decode(it->second);
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

    if (node["NewLayers"]) {
        YAML::Node layersNode = node["NewLayers"];
        for (YAML::const_iterator it = layersNode.begin(); it!=layersNode.end(); ++it) {
            ImageComponentsSerialization s;
            s.decode(it->second);
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
        if (colorNode.size() != 2) {
            throw YAML::InvalidNode();
        }
        _nodeColor[0] = colorNode[0].as<double>();
        _nodeColor[1] = colorNode[1].as<double>();
        _nodeColor[2] = colorNode[2].as<double>();
    }
    if (node["OverlayColor"]) {
        YAML::Node colorNode = node["OverlayColor"];
        if (colorNode.size() != 2) {
            throw YAML::InvalidNode();
        }
        _nodeColor[0] = colorNode[0].as<double>();
        _nodeColor[1] = colorNode[1].as<double>();
        _nodeColor[2] = colorNode[2].as<double>();
    }
    if (node["ViewerParamsOrder"]) {
        YAML::Node viewerParamsOrderNode = node["ViewerParamsOrder"];
        for (YAML::const_iterator it = viewerParamsOrderNode.begin(); it!=viewerParamsOrderNode.end(); ++it) {
            _viewerUIKnobsOrder.push_back(it->second.as<std::string>());
        }
    }


} // NodeSerialization::decode

void
NodePresetSerialization::encode(YAML::Emitter& em) const
{
    em << YAML::BeginMap;
    em << YAML::Key << "PluginID" << YAML::Value << pluginID;
    em << YAML::Key << "Version" << YAML::Value << version;
    em << YAML::Key << "PresetLabel" << YAML::Value << presetLabel;
    if (!presetIcon.empty()) {
        em << YAML::Key << "Icon" << YAML::Value << presetIcon;
    }
    if (presetSymbol != 0) {
        em << YAML::Key << "Key" << YAML::Value << presetSymbol;
    }
    if (presetModifiers != 0) {
        em << YAML::Key << "Modifiers" << YAML::Value << presetModifiers;
    }
    em << YAML::Key << "Node" << YAML::Value;
    node.encode(em);
    em << YAML::EndMap;
};

void
NodePresetSerialization::decode(const YAML::Node& node)
{
    if (!node.IsMap()) {
        throw YAML::InvalidNode();
    }
    pluginID = node["PluginID"].as<std::string>();
    version = node["Version"].as<int>();
    presetLabel = node["PresetLabel"].as<std::string>();
    if (node["Icon"]) {
        presetIcon = node["Icon"].as<std::string>();
    }
    if (node["Key"]) {
        presetSymbol = node["Key"].as<int>();
    }
    if (node["Modifiers"]) {
        presetSymbol = node["Modifiers"].as<int>();
    }
    presetLabel = node["PresetLabel"].as<std::string>();
    if (decodeMetaDataOnly) {
        return;
    }
    this->node.decode(node["Node"]);
}

SERIALIZATION_NAMESPACE_EXIT



