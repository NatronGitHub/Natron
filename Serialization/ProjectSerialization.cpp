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

#include "ProjectSerialization.h"

SERIALIZATION_NAMESPACE_ENTER


void
ProjectBeingLoadedInfo::encode(YAML_NAMESPACE::Emitter& em) const
{
    em << YAML_NAMESPACE::Flow << YAML_NAMESPACE::BeginMap;
    em << YAML_NAMESPACE::Key << "Version" << YAML_NAMESPACE::Value << YAML_NAMESPACE::Flow << YAML_NAMESPACE::BeginSeq << vMajor << vMinor << vRev << YAML_NAMESPACE::EndSeq;
    em << YAML_NAMESPACE::Key << "Branch" << YAML_NAMESPACE::Value << gitBranch;
    em << YAML_NAMESPACE::Key << "Commit" << YAML_NAMESPACE::Value << gitCommit;
    em << YAML_NAMESPACE::Key << "OS" << YAML_NAMESPACE::Value << osStr;
    em << YAML_NAMESPACE::Key << "Bits" << YAML_NAMESPACE::Value << bits;
    em << YAML_NAMESPACE::EndMap;
}

void
ProjectBeingLoadedInfo::decode(const YAML_NAMESPACE::Node& node)
{
    {
        YAML_NAMESPACE::Node n = node["Version"];
        if (n.size() != 3) {
            throw YAML_NAMESPACE::InvalidNode();
        }
        vMajor = n[0].as<int>();
        vMinor = n[1].as<int>();
        vRev = n[2].as<int>();
    }
    gitBranch = node["Branch"].as<std::string>();
    gitCommit = node["Commit"].as<std::string>();
    osStr = node["OS"].as<std::string>();
    bits = node["Bits"].as<int>();
}

void
ProjectSerialization::encode(YAML_NAMESPACE::Emitter& em) const
{
    em << YAML_NAMESPACE::BeginMap;

    if (!_nodes.empty()) {
        em << YAML_NAMESPACE::Key << "Nodes" << YAML_NAMESPACE::Value << YAML_NAMESPACE::BeginSeq;
        for (NodeSerializationList::const_iterator it = _nodes.begin(); it!=_nodes.end(); ++it) {
            (*it)->encode(em);
        }
        em << YAML_NAMESPACE::EndSeq;
    }

    if (!_additionalFormats.empty()) {
        em << YAML_NAMESPACE::Key << "Formats" << YAML_NAMESPACE::Value << YAML_NAMESPACE::Flow  << YAML_NAMESPACE::BeginSeq;
        for (std::list<FormatSerialization>::const_iterator it = _additionalFormats.begin(); it!=_additionalFormats.end(); ++it) {
            it->encode(em);
        }
        em << YAML_NAMESPACE::EndSeq;
    }

    if (!_projectKnobs.empty()) {
        em << YAML_NAMESPACE::Key << "Params" << YAML_NAMESPACE::Value << YAML_NAMESPACE::BeginSeq;
        for (KnobSerializationList::const_iterator it = _projectKnobs.begin(); it!=_projectKnobs.end(); ++it) {
            (*it)->encode(em);
        }
        em << YAML_NAMESPACE::EndSeq;
    }

    em << YAML_NAMESPACE::Key << "Frame" << YAML_NAMESPACE::Value << _timelineCurrent;
    em << YAML_NAMESPACE::Key << "NatronVersion" << YAML_NAMESPACE::Value;
    _projectLoadedInfo.encode(em);

    if (!_openedPanelsOrdered.empty()) {
        em << YAML_NAMESPACE::Key << "OpenedPanels" << YAML_NAMESPACE::Value << YAML_NAMESPACE::Flow << YAML_NAMESPACE::BeginSeq;
        for (std::list<std::string>::const_iterator it = _openedPanelsOrdered.begin(); it!=_openedPanelsOrdered.end(); ++it) {
            em << *it;
        }
        em << YAML_NAMESPACE::EndSeq;
    }
    if (_projectWorkspace) {
        em << YAML_NAMESPACE::Key << "Workspace" << YAML_NAMESPACE::Value;
        _projectWorkspace->encode(em);
    }
    if (!_viewportsData.empty()) {
        em << YAML_NAMESPACE::Key << "Viewports" << YAML_NAMESPACE::Value << YAML_NAMESPACE::BeginSeq;
        for (std::map<std::string, ViewportData>::const_iterator it = _viewportsData.begin(); it!=_viewportsData.end(); ++it) {
            em << YAML_NAMESPACE::Flow << YAML_NAMESPACE::BeginSeq;
            em << it->first;
            it->second.encode(em);
            em << YAML_NAMESPACE::EndSeq;
        }
        em << YAML_NAMESPACE::EndSeq;
    }
    em << YAML_NAMESPACE::EndMap;
} // ProjectSerialization::encode

void
ProjectSerialization::decode(const YAML_NAMESPACE::Node& node)
{
    if (node["Nodes"]) {
        YAML_NAMESPACE::Node n = node["Nodes"];
        for (std::size_t i = 0; i < n.size(); ++i) {
            NodeSerializationPtr ns(new NodeSerialization);
            ns->decode(n[i]);
            _nodes.push_back(ns);
        }
    }
    if (node["Formats"]) {
        YAML_NAMESPACE::Node n = node["Formats"];
        for (std::size_t i = 0; i < n.size(); ++i) {
            FormatSerialization s;
            s.decode(n[i]);
            _additionalFormats.push_back(s);
        }
    }
    if (node["Params"]) {
        YAML_NAMESPACE::Node n = node["Params"];
        for (std::size_t i = 0; i < n.size(); ++i) {
            KnobSerializationPtr s(new KnobSerialization);
            s->decode(n[i]);
            _projectKnobs.push_back(s);
        }
    }
    _timelineCurrent = node["Frame"].as<int>();
    _projectLoadedInfo.decode(node["NatronVersion"]);
    if (node["OpenedPanels"]) {
        YAML_NAMESPACE::Node n = node["OpenedPanels"];
        for (std::size_t i = 0; i < n.size(); ++i) {
            _openedPanelsOrdered.push_back(n[i].as<std::string>());
        }
    }
    if (node["Workspace"]) {
        _projectWorkspace.reset(new WorkspaceSerialization);
        _projectWorkspace->decode(node["Workspace"]);
    }
    if (node["Viewports"]) {
        YAML_NAMESPACE::Node n = node["Viewports"];
        for (std::size_t i = 0; i < n.size(); ++i) {
            if (!n[i].IsSequence() || n[i].size() != 2) {
                throw YAML_NAMESPACE::InvalidNode();
            }
            std::string name = n[i][0].as<std::string>();
            ViewportData data;
            data.decode(n[i][1]);
            _viewportsData.insert(std::make_pair(name, data));
        }
    }

} // ProjectSerialization::decode

SERIALIZATION_NAMESPACE_EXIT



