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

#include "RotoItemSerialization.h"

GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
#include <yaml-cpp/yaml.h>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON

#include "Serialization/KnobSerialization.h"

SERIALIZATION_NAMESPACE_ENTER

void
RotoItemSerialization::encode(YAML::Emitter& em) const
{
    // This assumes that a map is already created

    em << YAML::Key << "ScriptName" << YAML::Value << name;
    if (label != name) {
        em << YAML::Key << "Label" << YAML::Value << label;
    }
    std::list<std::string> props;
    if (locked) {
        props.push_back("Locked");
    }
    if (!parentLayerName.empty()) {
        em << YAML::Key << "Layer" << YAML::Value << parentLayerName;
    }
    if (!props.empty()) {
        em << YAML::Key << "Props" << YAML::Value << YAML::Flow << YAML::BeginSeq;
        for (std::list<std::string>::const_iterator it = props.begin(); it!=props.end(); ++it) {
            em << *it;
        }
        em << YAML::EndSeq;
    }

}

void
RotoItemSerialization::decode(const YAML::Node& node)
{

    name = node["ScriptName"].as<std::string>();

    if (node["Label"]) {
        label = node["Label"].as<std::string>();
    } else {
        label = name;
    }
    if (node["Layer"]) {
        parentLayerName = node["Layer"].as<std::string>();
    }
    if (node["Props"]) {
        YAML::Node props = node["Props"];
        for (std::size_t i = 0; i < props.size(); ++i) {
            std::string prop = props[i].as<std::string>();
            if (prop == "Locked") {
                locked = true;
            } else {
                throw std::invalid_argument("RotoItemSerialization: unknown property: " + prop);
            }
        }
    }
}


SERIALIZATION_NAMESPACE_EXIT