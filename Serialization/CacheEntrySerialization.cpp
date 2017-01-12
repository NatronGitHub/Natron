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

#include "CacheEntrySerialization.h"

GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
#include <yaml-cpp/yaml.h>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON

SERIALIZATION_NAMESPACE_ENTER



void CacheEntrySerializationBase::encode(YAML::Emitter& em) const
{
    em << YAML::Key << "File" << YAML::Value;
    em << filePath;

    em << YAML::Key << "Offset" << YAML::Value;
    em << dataOffsetInFile;

    em << YAML::Key << "Hash" << YAML::Value;
    em << hash;

    em << YAML::Key << "Time" << YAML::Value;
    em << time;

    em << YAML::Key << "View" << YAML::Value;
    em << view;



    if (!pluginID.empty()) {
        em << YAML::Key << "Plugin" << YAML::Value;
        em << pluginID;
    }


}

void CacheEntrySerializationBase::decode(const YAML::Node& node)
{

    if (node["File"]) {
        filePath = node["File"].as<std::string>();
    }
    if (node["Offset"]) {
        dataOffsetInFile = node["Offset"].as<std::size_t>();
    }
    if (node["Hash"]) {
        hash = node["Hash"].as<unsigned long long>();
    }
    if (node["Time"]) {
        time = node["Time"].as<double>();
    }
    if (node["View"]) {
        view = node["View"].as<int>();
    }
    if (node["Plugin"]) {
        pluginID = node["Plugin"].as<std::string>();
    }
}


void MMAPBufferedEntrySerialization::encode(YAML::Emitter& em) const
{
    CacheEntrySerializationBase::encode(em);
    em << YAML::Key << "TileCoords" << YAML::Value;
    em << tileX << tileY;
    em << YAML::Key << "Depth" << YAML::Value;
    em << bitdepth;
}

void MMAPBufferedEntrySerialization::decode(const YAML::Node& node)
{
    CacheEntrySerializationBase::decode(node);
    if (node["TileCoords"]) {
        YAML::Node tileNode = node["TileCords"];
        if (tileNode.size() != 2) {
            throw YAML::InvalidNode();
        }
        tileX = tileNode[0].as<int>();
        tileY = tileNode[1].as<int>();
    }
    if (node["Depth"]) {
        bitdepth = node["Depth"].as<std::string>();
    }

}


void
ImageTileSerialization::encode(YAML::Emitter& em) const
{
    em << YAML::Flow << YAML::BeginMap;
    MMAPBufferedEntrySerialization::encode(em);
    em << YAML::Key << "NodeHash" << YAML::Value;
    em << nodeHashKey;
    em << YAML::Key << "Channel" << YAML::Value;
    em << layerChannelName;
    em << YAML::Key << "Scale" << YAML::Value;
    em << YAML::Flow << YAML::BeginSeq;
    em << scale[0] << scale[1];
    em << YAML::EndSeq;
    em << YAML::Key << "MipMapLevel" << YAML::Value;
    em << mipMapLevel;
    em << YAML::Key << "Draft" << YAML::Value;
    em << draft;
    em << YAML::EndMap;
}

void
ImageTileSerialization::decode(const YAML::Node& node)
{
    if (!node.IsMap()) {
        throw YAML::InvalidNode();
    }
    MMAPBufferedEntrySerialization::decode(node);

    if (node["NodeHash"]) {
        nodeHashKey = node["NodeHash"].as<unsigned long long>();
    }
    if (node["Channel"]) {
        layerChannelName = node["Channel"].as<std::string>();
    }
    if (node["Scale"]) {
        YAML::Node scaleNode = node["Scale"];
        if (scaleNode.size() != 2) {
            throw YAML::InvalidNode();
        }
        scale[0] = scaleNode[0].as<double>();
        scale[1] = scaleNode[1].as<double>();
    }
    if (node["MipMapLevel"]) {
        mipMapLevel = node["MipMapLevel"].as<unsigned int>();
    }
    if (node["Draft"]) {
        draft = node["Draft"].as<bool>();
    }
}

SERIALIZATION_NAMESPACE_EXIT


