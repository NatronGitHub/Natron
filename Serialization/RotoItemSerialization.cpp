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

SERIALIZATION_NAMESPACE_ENTER

void
RotoItemSerialization::encode(YAML_NAMESPACE::Emitter& em) const
{
    // This assumes that a map is already created

    em << YAML_NAMESPACE::Key << "ScriptName" << YAML_NAMESPACE::Value << name;
    if (label != name) {
        em << YAML_NAMESPACE::Key << "Label" << YAML_NAMESPACE::Value << label;
    }
    std::list<std::string> props;
    if (!activated) {
        props.push_back("Disabled");
    }
    if (locked) {
        props.push_back("Locked");
    }
    if (!parentLayerName.empty()) {
        em << YAML_NAMESPACE::Key << "Layer" << YAML_NAMESPACE::Value << parentLayerName;
    }
    if (!props.empty()) {
        em << YAML_NAMESPACE::Key << "Props" << YAML_NAMESPACE::Value << YAML_NAMESPACE::Flow << YAML_NAMESPACE::BeginSeq;
        for (std::list<std::string>::const_iterator it = props.begin(); it!=props.end(); ++it) {
            em << *it;
        }
        em << YAML_NAMESPACE::EndSeq;
    }

}

void
RotoItemSerialization::decode(const YAML_NAMESPACE::Node& node)
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
        YAML_NAMESPACE::Node props = node["Props"];
        for (std::size_t i = 0; i < props.size(); ++i) {
            std::string prop = props[i].as<std::string>();
            if (prop == "Disabled") {
                activated = true;
            } else if (prop == "Locked") {
                locked = true;
            } else {
                throw std::invalid_argument("RotoItemSerialization: unknown property: " + prop);
            }
        }
    }
}


SERIALIZATION_NAMESPACE_EXIT