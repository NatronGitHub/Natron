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
    em << YAML_NAMESPACE::BeginMap;
    em << YAML_NAMESPACE::Key << "ScriptName" << YAML_NAMESPACE::Value << name;
    em << YAML_NAMESPACE::Key << "Label" << YAML_NAMESPACE::Value << label;
    em << YAML_NAMESPACE::Key << "Activated" << YAML_NAMESPACE::Value << activated;
    em << YAML_NAMESPACE::Key << "Layer" << YAML_NAMESPACE::Value << parentLayerName;
    em << YAML_NAMESPACE::Key << "Locked" << YAML_NAMESPACE::Value << locked;
    em << YAML_NAMESPACE::EndMap;

}

void
RotoItemSerialization::decode(const YAML_NAMESPACE::Node& node)
{
    for (YAML_NAMESPACE::const_iterator it = node.begin(); it!=node.end(); ++it) {
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