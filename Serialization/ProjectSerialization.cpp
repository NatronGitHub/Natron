/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2020 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#include "ProjectSerialization.h"

#include <boost/make_shared.hpp>

GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
#include <yaml-cpp/yaml.h>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON

SERIALIZATION_NAMESPACE_ENTER


void
ProjectBeingLoadedInfo::encode(YAML::Emitter& em) const
{
    em << YAML::Flow << YAML::BeginMap;
    em << YAML::Key << "Version" << YAML::Value << YAML::Flow << YAML::BeginSeq << vMajor << vMinor << vRev << YAML::EndSeq;
    em << YAML::Key << "Branch" << YAML::Value << gitBranch;
    em << YAML::Key << "Commit" << YAML::Value << gitCommit;
    em << YAML::Key << "OS" << YAML::Value << osStr;
    em << YAML::Key << "Bits" << YAML::Value << bits;
    em << YAML::EndMap;
}

void
ProjectBeingLoadedInfo::decode(const YAML::Node& node)
{
    {
        const YAML::Node& n = node["Version"];
        if (n.size() != 3) {
            throw YAML::InvalidNode();
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
ProjectSerialization::encode(YAML::Emitter& em) const
{
    em << YAML::BeginMap;

    if (!_nodes.empty()) {
        em << YAML::Key << "Nodes" << YAML::Value << YAML::BeginSeq;
        for (NodeSerializationList::const_iterator it = _nodes.begin(); it!=_nodes.end(); ++it) {
            (*it)->encode(em);
        }
        em << YAML::EndSeq;
    }

    if (!_additionalFormats.empty()) {
        em << YAML::Key << "Formats" << YAML::Value << YAML::Flow  << YAML::BeginSeq;
        for (std::list<FormatSerialization>::const_iterator it = _additionalFormats.begin(); it!=_additionalFormats.end(); ++it) {
            it->encode(em);
        }
        em << YAML::EndSeq;
    }

    if (!_projectKnobs.empty()) {
        em << YAML::Key << "Params" << YAML::Value << YAML::BeginSeq;
        for (KnobSerializationList::const_iterator it = _projectKnobs.begin(); it!=_projectKnobs.end(); ++it) {
            (*it)->encode(em);
        }
        em << YAML::EndSeq;
    }

    em << YAML::Key << "Frame" << YAML::Value << _timelineCurrent;
    em << YAML::Key << "NatronVersion" << YAML::Value;
    _projectLoadedInfo.encode(em);

    if (!_openedPanelsOrdered.empty()) {
        em << YAML::Key << "OpenedPanels" << YAML::Value << YAML::Flow << YAML::BeginSeq;
        for (std::list<std::string>::const_iterator it = _openedPanelsOrdered.begin(); it!=_openedPanelsOrdered.end(); ++it) {
            em << *it;
        }
        em << YAML::EndSeq;
    }
    if (_projectWorkspace) {
        em << YAML::Key << "Workspace" << YAML::Value;
        _projectWorkspace->encode(em);
    }
    if (!_viewportsData.empty()) {
        em << YAML::Key << "Viewports" << YAML::Value << YAML::BeginSeq;
        for (std::map<std::string, ViewportData>::const_iterator it = _viewportsData.begin(); it!=_viewportsData.end(); ++it) {
            em << YAML::Flow << YAML::BeginSeq;
            em << it->first;
            it->second.encode(em);
            em << YAML::EndSeq;
        }
        em << YAML::EndSeq;
    }
    em << YAML::EndMap;
} // ProjectSerialization::encode

void
ProjectSerialization::decode(const YAML::Node& node)
{
    if (node["Nodes"]) {
        const YAML::Node& n = node["Nodes"];
        for (std::size_t i = 0; i < n.size(); ++i) {
            NodeSerializationPtr ns = boost::make_shared<NodeSerialization>();
            ns->decode(n[i]);
            _nodes.push_back(ns);
        }
    }
    if (node["Formats"]) {
        const YAML::Node& n = node["Formats"];
        for (std::size_t i = 0; i < n.size(); ++i) {
            FormatSerialization s;
            s.decode(n[i]);
            _additionalFormats.push_back(s);
        }
    }
    if (node["Params"]) {
        const YAML::Node& n = node["Params"];
        for (std::size_t i = 0; i < n.size(); ++i) {
            KnobSerializationPtr s = boost::make_shared<KnobSerialization>();
            s->decode(n[i]);
            _projectKnobs.push_back(s);
        }
    }
    _timelineCurrent = node["Frame"].as<int>();
    _projectLoadedInfo.decode(node["NatronVersion"]);
    if (node["OpenedPanels"]) {
        const YAML::Node& n = node["OpenedPanels"];
        for (std::size_t i = 0; i < n.size(); ++i) {
            _openedPanelsOrdered.push_back(n[i].as<std::string>());
        }
    }
    if (node["Workspace"]) {
        _projectWorkspace = boost::make_shared<WorkspaceSerialization>();
        _projectWorkspace->decode(node["Workspace"]);
    }
    if (node["Viewports"]) {
        const YAML::Node& n = node["Viewports"];
        for (std::size_t i = 0; i < n.size(); ++i) {
            if (!n[i].IsSequence() || n[i].size() != 2) {
                throw YAML::InvalidNode();
            }
            std::string name = n[i][0].as<std::string>();
            ViewportData data;
            data.decode(n[i][1]);
            _viewportsData.insert(std::make_pair(name, data));
        }
    }

} // ProjectSerialization::decode

SERIALIZATION_NAMESPACE_EXIT



