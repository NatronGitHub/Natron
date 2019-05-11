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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****


#include "FrameKeySerialization.h"

GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
#include <yaml-cpp/yaml.h>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON

SERIALIZATION_NAMESPACE_ENTER

void
FrameKeySerialization::encode(YAML::Emitter& em) const
{
    em << YAML::Flow << YAML::BeginMap;
    em << YAML::Key << "Frame" << YAML::Value << frame;
    em << YAML::Key << "View" << YAML::Value << view;
    em << YAML::Key << "Hash" << YAML::Value << treeHash;
    if (bitdepth != kBitDepthSerializationByte) {
        em << YAML::Key << "Depth" << YAML::Value << bitdepth;
    }
    em << YAML::Key << "Rect" << YAML::Value;
    textureRect.encode(em);
    if (draftMode) {
        em << YAML::Key << "Draft" << YAML::Value << draftMode;
    }
    if (useShader) {
        em << YAML::Key << "Shaders" << YAML::Value << useShader;
    }
    em << YAML::EndMap;
}

void
FrameKeySerialization::decode(const YAML::Node& node)
{
    if (!node.IsMap()) {
        throw YAML::InvalidNode();
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


