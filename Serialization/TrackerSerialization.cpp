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
    bool hasProperties = _enabled || _isPM;
    if (hasProperties) {
        em << YAML::Key << "Properties" << YAML::Value;
        em << YAML::Flow << YAML::BeginSeq;
        if (!_enabled) {
            em << "Disabled";
        }
        if (_isPM) {
            em << "PM";
        }
        em << YAML::EndSeq;
    }

    em << YAML::Key << "ScriptName" << YAML::Value << _scriptName;
    if (!_label.empty()) {
        em << YAML::Key << "Label" << YAML::Value << _label;
    }
    em << YAML::Key << "Params" << YAML::Value;
    em << YAML::BeginSeq;
    for (std::list<KnobSerializationPtr>::const_iterator it = _knobs.begin(); it!=_knobs.end(); ++it) {
        (*it)->encode(em);
    }
    em << YAML::EndSeq;
    if (!_userKeys.empty()) {
        em << YAML::Key << "UserKeyframes" << YAML::Value << YAML::Flow << YAML::BeginSeq;
        for (std::list<int>::const_iterator it = _userKeys.begin(); it != _userKeys.end(); ++it) {
            em << *it;
        }
        em << YAML::EndSeq;
    }
    em << YAML::EndSeq;
}

void
TrackSerialization::decode(const YAML::Node& node)
{
    if (!node.IsMap()) {
        throw YAML::InvalidNode();
    }

    if (node["Properties"]) {
        YAML::Node propsNode = node["Properties"];
        for (YAML::const_iterator it = propsNode.begin(); it!=propsNode.end(); ++it) {
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
        YAML::Node keys = node["UserKeyframes"];
        for (YAML::const_iterator it = keys.begin(); it!=keys.end(); ++it) {
            _userKeys.push_back(it->second.as<int>());
        }
    }

    YAML::Node params = node["Params"];
    for (YAML::const_iterator it = params.begin(); it!=params.end(); ++it) {
        KnobSerializationPtr k(new KnobSerialization);
        k->decode(it->second);
        _knobs.push_back(k);
    }
}

void
TrackerContextSerialization::encode(YAML::Emitter& em) const
{
    em << YAML::BeginSeq;
    for (std::list<TrackSerialization>::const_iterator it = _tracks.begin(); it!=_tracks.end(); ++it) {
        it->encode(em);
    }
    em << YAML::EndSeq;
}

void
TrackerContextSerialization::decode(const YAML::Node& node)
{
    for (YAML::const_iterator it = node.begin(); it!=node.end(); ++it) {
        TrackSerialization t;
        t.decode(it->second);
        _tracks.push_back(t);
    }
}

SERIALIZATION_NAMESPACE_EXIT



