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

#ifndef Engine_CacheSerializationImpl_h
#define Engine_CacheSerializationImpl_h

#include "Serialization/CacheSerialization.h"

#include "Global/Macros.h"

GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
#include <yaml-cpp/yaml.h>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON

SERIALIZATION_NAMESPACE_ENTER;

template<typename EntryType>
void SerializedEntry<EntryType>::encode(YAML::Emitter& em) const
{
    em << YAML::Flow;
    em << YAML::BeginSeq;
    em << filePath;
    em << dataOffsetInFile;
    em << size;
    em << hash;
    key.encode(em);
    params.encode(em);
    if (!pluginID.empty()) {
        em << pluginID;
    }
    em << YAML::EndSeq;

}

template<typename EntryType>
void SerializedEntry<EntryType>::decode(const YAML::Node& node)
{
    if (!node.IsSequence() || node.size() < 6) {
        throw YAML::InvalidNode();
    }
    filePath = node[0].as<std::string>();
    dataOffsetInFile = node[1].as<std::size_t>();
    size = node[2].as<std::size_t>();
    hash = node[3].as<hash_type>();
    key.decode(node[4]);
    params.decode(node[5]);
    if (node.size() >= 7) {
        pluginID = node[6].as<std::string>();
    }
}

template<typename EntryType>
void CacheSerialization<EntryType>::encode(YAML::Emitter& em) const
{
    em << YAML::BeginMap;
    em << YAML::Key << "Version" << YAML::Value << cacheVersion;
    if (!entries.empty()) {
        em << YAML::Key << "Entries" << YAML::Value;
        em << YAML::BeginSeq;
        for (typename std::list<SerializedEntry<EntryType> >::const_iterator it = entries.begin(); it!=entries.end(); ++it) {
            it->encode(em);
        }
        em << YAML::EndSeq;
    }
    em << YAML::EndMap;
}

template<typename EntryType>
void CacheSerialization<EntryType>::decode(const YAML::Node& node)
{
    if (node["Version"]) {
        cacheVersion = node["Version"].as<int>();
    }
    if (node["Entries"]) {
        YAML::Node entriesNode = node["Entries"];
        for (std::size_t i = 0; i < entriesNode.size(); ++i) {
            SerializedEntry<EntryType> entry;
            entry.decode(entriesNode[i]);
            entries.push_back(entry);
        }
    }
}

SERIALIZATION_NAMESPACE_EXIT;

#endif // Engine_CacheSerializationImpl_h