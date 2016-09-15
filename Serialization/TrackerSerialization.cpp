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

#include "TrackerSerialization.h"

GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
#include <yaml-cpp/yaml.h>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON

SERIALIZATION_NAMESPACE_ENTER

void
TrackSerialization::encode(YAML::Emitter& em) const
{
    em << YAML::BeginMap;

    std::list<std::string> props;
    if (!_enabled) {
        props.push_back("Disabled");
    }
    if (_isPM) {
        props.push_back("PM");
    }

    em << YAML::Key << "ScriptName" << YAML::Value << _scriptName;
    if (_label != _scriptName) {
        em << YAML::Key << "Label" << YAML::Value << _label;
    }
    if (!_knobs.empty()) {
        em << YAML::Key << "Params" << YAML::Value;
        em << YAML::BeginSeq;
        for (std::list<KnobSerializationPtr>::const_iterator it = _knobs.begin(); it!=_knobs.end(); ++it) {
            (*it)->encode(em);
        }
        em << YAML::EndSeq;
    }
    if (!_userKeys.empty()) {
        em << YAML::Key << "UserKeyframes" << YAML::Value << YAML::Flow << YAML::BeginSeq;
        for (std::list<int>::const_iterator it = _userKeys.begin(); it != _userKeys.end(); ++it) {
            em << *it;
        }
        em << YAML::EndSeq;
    }

    if (!props.empty()) {
        em << YAML::Key << "Properties" << YAML::Value;
        em << YAML::Flow << YAML::BeginSeq;
        for (std::list<std::string>::const_iterator it = props.begin(); it!=props.end(); ++it) {
            em << *it;
        }
        em << YAML::EndSeq;

    }

    em << YAML::EndMap;
}

void
TrackSerialization::decode(const YAML::Node& node)
{
    if (!node.IsMap()) {
        throw YAML::InvalidNode();
    }

    if (node["Properties"]) {
        YAML::Node propsNode = node["Properties"];
        for (std::size_t i = 0; i < propsNode.size(); ++i) {
            std::string key = propsNode[i].as<std::string>();
            if (key == "Disabled") {
                _enabled = false;
            } else if (key == "PM") {
                _isPM = true;
            }
        }
    }
    _scriptName = node["ScriptName"].as<std::string>();
    if (node["Label"]) {
        _label = node["Label"].as<std::string>();
    } else {
        _label = _scriptName;
    }

    if (node["UserKeyframes"]) {
        YAML::Node keys = node["UserKeyframes"];
        for (std::size_t i = 0; i < keys.size(); ++i) {
            _userKeys.push_back(keys[i].as<int>());
        }
    }

    if (node["Params"]) {
        YAML::Node params = node["Params"];
        for (std::size_t i = 0; i < params.size(); ++i) {
            KnobSerializationPtr k(new KnobSerialization);
            k->decode(params[i]);
            _knobs.push_back(k);
        }
    }
}

void
TrackerContextSerialization::encode(YAML::Emitter& em) const
{
    if (_tracks.empty()) {
        return;
    }
    em << YAML::BeginSeq;
    for (std::list<TrackSerialization>::const_iterator it = _tracks.begin(); it!=_tracks.end(); ++it) {
        it->encode(em);
    }
    em << YAML::EndSeq;
}

void
TrackerContextSerialization::decode(const YAML::Node& node)
{
    for (std::size_t i = 0; i < node.size(); ++i) {
        TrackSerialization t;
        t.decode(node[i]);
        _tracks.push_back(t);
    }
}

SERIALIZATION_NAMESPACE_EXIT



