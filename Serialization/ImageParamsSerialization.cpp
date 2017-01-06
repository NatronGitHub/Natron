/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2017 INRIA and Alexandre Gauthier-Foichat
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

#include "ImageParamsSerialization.h"

GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
#include <yaml-cpp/yaml.h>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON

SERIALIZATION_NAMESPACE_ENTER

void
ImageComponentsSerialization::encode(YAML::Emitter& em) const
{
    em << YAML::Flow << YAML::BeginSeq << layerName << globalCompsName << YAML::Flow << channelNames << YAML::EndSeq;
}

void
ImageComponentsSerialization::decode(const YAML::Node& node)
{
    if (!node.IsSequence() || node.size() != 3) {
        throw YAML::InvalidNode();
    }
    layerName = node[0].as<std::string>();
    globalCompsName = node[1].as<std::string>();
    channelNames = node[2].as<std::vector<std::string> >();
}

void
ImageParamsSerialization::encode(YAML::Emitter& em) const
{
    em << YAML::Flow << YAML::BeginSeq;
    NonKeyParamsSerialization::encode(em);
    rod.encode(em);
    em << par;
    components.encode(em);
    em << bitdepth;
    em << fielding;
    em << premult;
    em << mipMapLevel;
    em << YAML::EndSeq;
}

void
ImageParamsSerialization::decode(const YAML::Node& node)
{
    if (!node.IsSequence() || node.size() != 8) {
        throw YAML::InvalidNode();
    }
    NonKeyParamsSerialization::decode(node[0]);
    rod.decode(node[1]);
    par = node[2].as<double>();
    components.decode(node[3]);
    bitdepth = node[4].as<int>();
    fielding = node[5].as<int>();
    premult = node[6].as<int>();
    mipMapLevel = node[7].as<unsigned int>();
}

SERIALIZATION_NAMESPACE_EXIT


