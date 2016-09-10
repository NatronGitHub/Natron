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

#include "FrameKeySerialization.h"

SERIALIZATION_NAMESPACE_ENTER

void
FrameKeySerialization::encode(YAML_NAMESPACE::Emitter& em) const
{
    em << YAML_NAMESPACE::Flow << YAML_NAMESPACE::BeginMap;
    em << YAML_NAMESPACE::Key << "Frame" << YAML_NAMESPACE::Value << frame;
    em << YAML_NAMESPACE::Key << "View" << YAML_NAMESPACE::Value << view;
    em << YAML_NAMESPACE::Key << "Hash" << YAML_NAMESPACE::Value << treeHash;
    if (bitdepth != kBitDepthSerializationByte) {
        em << YAML_NAMESPACE::Key << "Depth" << YAML_NAMESPACE::Value << bitdepth;
    }
    em << YAML_NAMESPACE::Key << "Rect" << YAML_NAMESPACE::Value;
    textureRect.encode(em);
    if (draftMode) {
        em << YAML_NAMESPACE::Key << "Draft" << YAML_NAMESPACE::Value << draftMode;
    }
    if (useShader) {
        em << YAML_NAMESPACE::Key << "Shaders" << YAML_NAMESPACE::Value << useShader;
    }
    em << YAML_NAMESPACE::EndMap;
}

void
FrameKeySerialization::decode(const YAML_NAMESPACE::Node& node)
{
    if (!node.IsMap()) {
        throw YAML_NAMESPACE::InvalidNode();
    }
    frame = node["Frame"].as<int>();
    view = node["View"].as<int>();
    treeHash = node["Hash"].as<unsigned long long>();
    if (node["Depth"]) {
        bitdepth = node["Depth"].as<std::string>();
    } else {
        bitdepth = kBitDepthSerializationByte;
    }
    textureRect.decode(node["Rect"]);
    if (node["Draft"]) {
        draftMode = node["Draft"].as<bool>();
    }
    if (node["Shaders"]) {
        useShader = node["Shaders"].as<bool>();
    }
   
}

SERIALIZATION_NAMESPACE_EXIT


