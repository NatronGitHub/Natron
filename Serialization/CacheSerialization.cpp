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


#include "CacheSerialization.h"
#include <iostream>

#include "Serialization/CacheEntrySerialization.h"

GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
#include <yaml-cpp/yaml.h>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON

SERIALIZATION_NAMESPACE_ENTER;

CacheEntrySerializationBasePtr
CacheSerialization::createSerializationObjectForEntryTag(const std::string& tag)
{
    if (tag == kSerializationImageTileTag) {
        ImageTileSerializationPtr ret(new ImageTileSerialization);
        return ret;
    } else {
        std::cerr << "Unknown YAML tag " << tag << std::endl;
        throw YAML::InvalidNode();
        assert(false);
        return CacheEntrySerializationBasePtr();
    }
}

void CacheSerialization::encode(YAML::Emitter& em) const
{
    em << YAML::BeginMap;
    em << YAML::Key << "Version" << YAML::Value << cacheVersion;
    em << YAML::Key << "TileSizePo2" << YAML::Value << tileSizePo2;
    if (!entries.empty()) {
        em << YAML::Key << "Entries" << YAML::Value;
        em << YAML::BeginSeq;
        for (std::list<CacheEntrySerializationBasePtr>::const_iterator it = entries.begin(); it!=entries.end(); ++it) {
            em << YAML::VerbatimTag((*it)->verbatimTag);
            (*it)->encode(em);
        }
        em << YAML::EndSeq;
    }
    em << YAML::EndMap;
}

void CacheSerialization::decode(const YAML::Node& node)
{
    if (node["Version"]) {
        cacheVersion = node["Version"].as<int>();
    }
    if (node["TileSizePo2"]) {
        tileSizePo2 = node["TileSizePo2"].as<int>();
    }
    if (node["Entries"]) {
        YAML::Node entriesNode = node["Entries"];
        for (std::size_t i = 0; i < entriesNode.size(); ++i) {

            const std::string& nodeTag = entriesNode[i].Tag();
            CacheEntrySerializationBasePtr s = createSerializationObjectForEntryTag(nodeTag);
            if (!s) {
                continue;
            }

            s->decode(entriesNode[i]);
            entries.push_back(s);
        }
    }
}


SERIALIZATION_NAMESPACE_EXIT;