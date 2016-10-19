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
    if (_isPM) {
        props.push_back("PM");
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
            if (key == "PM") {
                _isPM = true;
            }
        }
    }

    if (node["UserKeyframes"]) {
        YAML::Node keys = node["UserKeyframes"];
        for (std::size_t i = 0; i < keys.size(); ++i) {
            _userKeys.push_back(keys[i].as<int>());
        }
    }


}


SERIALIZATION_NAMESPACE_EXIT



