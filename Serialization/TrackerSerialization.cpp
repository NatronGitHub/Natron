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

SERIALIZATION_NAMESPACE_ENTER

void
TrackSerialization::encode(YAML_NAMESPACE::Emitter& em) const
{
    em << YAML_NAMESPACE::BeginMap;

    std::list<std::string> props;
    if (!_enabled) {
        props.push_back("Disabled");
    }
    if (_isPM) {
        props.push_back("PM");
    }

    em << YAML_NAMESPACE::Key << "ScriptName" << YAML_NAMESPACE::Value << _scriptName;
    if (_label != _scriptName) {
        em << YAML_NAMESPACE::Key << "Label" << YAML_NAMESPACE::Value << _label;
    }
    if (!_knobs.empty()) {
        em << YAML_NAMESPACE::Key << "Params" << YAML_NAMESPACE::Value;
        em << YAML_NAMESPACE::BeginSeq;
        for (std::list<KnobSerializationPtr>::const_iterator it = _knobs.begin(); it!=_knobs.end(); ++it) {
            (*it)->encode(em);
        }
        em << YAML_NAMESPACE::EndSeq;
    }
    if (!_userKeys.empty()) {
        em << YAML_NAMESPACE::Key << "UserKeyframes" << YAML_NAMESPACE::Value << YAML_NAMESPACE::Flow << YAML_NAMESPACE::BeginSeq;
        for (std::list<int>::const_iterator it = _userKeys.begin(); it != _userKeys.end(); ++it) {
            em << *it;
        }
        em << YAML_NAMESPACE::EndSeq;
    }

    if (!props.empty()) {
        em << YAML_NAMESPACE::Key << "Properties" << YAML_NAMESPACE::Value;
        em << YAML_NAMESPACE::Flow << YAML_NAMESPACE::BeginSeq;
        for (std::list<std::string>::const_iterator it = props.begin(); it!=props.end(); ++it) {
            em << *it;
        }
        em << YAML_NAMESPACE::EndSeq;

    }

    em << YAML_NAMESPACE::EndMap;
}

void
TrackSerialization::decode(const YAML_NAMESPACE::Node& node)
{
    if (!node.IsMap()) {
        throw YAML_NAMESPACE::InvalidNode();
    }

    if (node["Properties"]) {
        YAML_NAMESPACE::Node propsNode = node["Properties"];
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
        YAML_NAMESPACE::Node keys = node["UserKeyframes"];
        for (std::size_t i = 0; i < keys.size(); ++i) {
            _userKeys.push_back(keys[i].as<int>());
        }
    }

    if (node["Params"]) {
        YAML_NAMESPACE::Node params = node["Params"];
        for (std::size_t i = 0; i < params.size(); ++i) {
            KnobSerializationPtr k(new KnobSerialization);
            k->decode(params[i]);
            _knobs.push_back(k);
        }
    }
}

void
TrackerContextSerialization::encode(YAML_NAMESPACE::Emitter& em) const
{
    if (_tracks.empty()) {
        return;
    }
    em << YAML_NAMESPACE::BeginSeq;
    for (std::list<TrackSerialization>::const_iterator it = _tracks.begin(); it!=_tracks.end(); ++it) {
        it->encode(em);
    }
    em << YAML_NAMESPACE::EndSeq;
}

void
TrackerContextSerialization::decode(const YAML_NAMESPACE::Node& node)
{
    for (std::size_t i = 0; i < node.size(); ++i) {
        TrackSerialization t;
        t.decode(node[i]);
        _tracks.push_back(t);
    }
}

SERIALIZATION_NAMESPACE_EXIT



