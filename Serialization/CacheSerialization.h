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

#ifndef Engine_CacheSerialization_h
#define Engine_CacheSerialization_h


GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
#include <yaml-cpp/yaml.h>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON

#include "Serialization/SerializationBase.h"
#include "Serialization/ImageKeySerialization.h"
#include "Serialization/ImageParamsSerialization.h"
#include "Serialization/FrameKeySerialization.h"
#include "Serialization/FrameParamsSerialization.h"


SERIALIZATION_NAMESPACE_ENTER;


template<typename EntryType>
class SerializedEntry
: public SerializationObjectBase
{
public:

    typedef typename EntryType::hash_type hash_type;
    typedef typename EntryType::key_serialization_t key_serialization_t;
    typedef typename EntryType::params_serialization_t params_serialization_t;

    // The hash can be recovered from the key, but it is also serialized to double check that the entry still
    // has a valid hash
    hash_type hash;

    // The unique identifier of the cache entry
    key_serialization_t key;

    // The parameters that go along the cache entry but that do not necessarilly uniquely identify it
    params_serialization_t params;

    // The size in bytes
    std::size_t size;

    // Even though entries have their filename given by their hash,
    // we need to serialize it as multiple entries can have the same hash.
    // Each entry with the same hash has an index appended
    std::string filePath;

    // At which byte in the cache file does this entry start. For Image cache generally each file is one entry so this is 0
    // but for a Tile cache, it is an offset in the cache memory files.
    std::size_t dataOffsetInFile;

    SerializedEntry()
    : hash(0)
    , key()
    , params()
    , size(0)
    , filePath()
    , dataOffsetInFile(0)
    {
    }

    virtual void encode(YAML::Emitter& em) const OVERRIDE FINAL
    {
        em << YAML::BeginSeq;
        em << YAML::Flow;
        em << filePath;
        em << dataOffsetInFile;
        em << size;
        em << hash;
        key.encode(em);
        params.encode(em);
        em << YAML::EndSeq;

    }

    virtual void decode(const YAML::Node& node) OVERRIDE FINAL
    {
        if (!node.IsSequence() || node.size() != 6) {
            throw YAML::InvalidNode();
        }
        filePath = node[0].as<std::string>();
        dataOffsetInFile = node[1].as<std::size_t>();
        size = node[2].as<std::size_t>();
        hash = node[3].as<hash_type>();
        key.decode(node[4]);
        params.decode(node[5]);
    }


};

template<typename EntryType>
class CacheSerialization
: public SerializationObjectBase
{

public:

    int cacheVersion;
    std::list<SerializedEntry<EntryType> > entries;

    virtual void encode(YAML::Emitter& em) const OVERRIDE FINAL
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

    virtual void decode(const YAML::Node& node) OVERRIDE FINAL
    {
        if (node["Version"]) {
            cacheVersion = node["Version"].as<int>();
        }
        if (node["Entries"]) {
            YAML::Node entriesNode = node["Entries"];
            for (YAML::const_iterator it = entriesNode.begin(); it!=entriesNode.end(); ++it) {
                SerializedEntry<EntryType> entry;
                entry.decode(it->second);
                entries.push_back(entry);
            }
        }
    }
};




SERIALIZATION_NAMESPACE_EXIT;


#endif // Engine_CacheSerialization_h
