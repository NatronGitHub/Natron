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

SERIALIZATION_NAMESPACE_ENTER

void
RotoItemSerialization::encode(YAML::Emitter& em) const
{
    em << YAML::BeginMap;
    em << YAML::Key << "ScriptName" << YAML::Value << name;
    em << YAML::Key << "Label" << YAML::Value << label;
    em << YAML::Key << "Activated" << YAML::Value << activated;
    em << YAML::Key << "Layer" << YAML::Value << parentLayerName;
    em << YAML::Key << "Locked" << YAML::Value << locked;
    em << YAML::EndMap;

}

void
RotoItemSerialization::decode(const YAML::Node& node)
{
    for (YAML::const_iterator it = node.begin(); it!=node.end(); ++it) {
        std::string key = it->first.as<std::string>();
        if (key == "ScriptName") {
            name = it->second.as<std::string>();
        } else if (key == "Label") {
            label = it->second.as<std::string>();
        } else if (key == "Activated") {
            activated = it->second.as<bool>();
        } else if (key == "Layer") {
            parentLayerName = it->second.as<std::string>();
        } else if (key == "Locked") {
            locked = it->second.as<bool>();
        } else {
            std::ostringstream os;
            os << "Unrecognized item " << key;
            throw std::runtime_error(os.str());
        }
    }
}


SERIALIZATION_NAMESPACE_EXIT