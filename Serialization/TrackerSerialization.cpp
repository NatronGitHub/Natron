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
    bool hasProperties = _enabled || _isPM;
    if (hasProperties) {
        em << YAML_NAMESPACE::Key << "Properties" << YAML_NAMESPACE::Value;
        em << YAML_NAMESPACE::Flow << YAML_NAMESPACE::BeginSeq;
        if (!_enabled) {
            em << "Disabled";
        }
        if (_isPM) {
            em << "PM";
        }
        em << YAML_NAMESPACE::EndSeq;
    }

    em << YAML_NAMESPACE::Key << "ScriptName" << YAML_NAMESPACE::Value << _scriptName;
    if (!_label.empty()) {
        em << YAML_NAMESPACE::Key << "Label" << YAML_NAMESPACE::Value << _label;
    }
    em << YAML_NAMESPACE::Key << "Params" << YAML_NAMESPACE::Value;
    em << YAML_NAMESPACE::BeginSeq;
    for (std::list<KnobSerializationPtr>::const_iterator it = _knobs.begin(); it!=_knobs.end(); ++it) {
        (*it)->encode(em);
    }
    em << YAML_NAMESPACE::EndSeq;
    if (!_userKeys.empty()) {
        em << YAML_NAMESPACE::Key << "UserKeyframes" << YAML_NAMESPACE::Value << YAML_NAMESPACE::Flow << YAML_NAMESPACE::BeginSeq;
        for (std::list<int>::const_iterator it = _userKeys.begin(); it != _userKeys.end(); ++it) {
            em << *it;
        }
        em << YAML_NAMESPACE::EndSeq;
    }
    em << YAML_NAMESPACE::EndSeq;
}

void
TrackSerialization::decode(const YAML_NAMESPACE::Node& node)
{
    if (!node.IsMap()) {
        throw YAML_NAMESPACE::InvalidNode();
    }

    if (node["Properties"]) {
        YAML_NAMESPACE::Node propsNode = node["Properties"];
        for (YAML_NAMESPACE::const_iterator it = propsNode.begin(); it!=propsNode.end(); ++it) {
            std::string key = it->first.as<std::string>();
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
    }

    if (node["UserKeyframes"]) {
        YAML_NAMESPACE::Node keys = node["UserKeyframes"];
        for (YAML_NAMESPACE::const_iterator it = keys.begin(); it!=keys.end(); ++it) {
            _userKeys.push_back(it->second.as<int>());
        }
    }

    YAML_NAMESPACE::Node params = node["Params"];
    for (YAML_NAMESPACE::const_iterator it = params.begin(); it!=params.end(); ++it) {
        KnobSerializationPtr k(new KnobSerialization);
        k->decode(it->second);
        _knobs.push_back(k);
    }
}

void
TrackerContextSerialization::encode(YAML_NAMESPACE::Emitter& em) const
{
    em << YAML_NAMESPACE::BeginSeq;
    for (std::list<TrackSerialization>::const_iterator it = _tracks.begin(); it!=_tracks.end(); ++it) {
        it->encode(em);
    }
    em << YAML_NAMESPACE::EndSeq;
}

void
TrackerContextSerialization::decode(const YAML_NAMESPACE::Node& node)
{
    for (YAML_NAMESPACE::const_iterator it = node.begin(); it!=node.end(); ++it) {
        TrackSerialization t;
        t.decode(it->second);
        _tracks.push_back(t);
    }
}

SERIALIZATION_NAMESPACE_EXIT



