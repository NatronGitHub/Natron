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

#ifndef Engine_CacheSerialization_h
#define Engine_CacheSerialization_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Serialization/SerializationBase.h"
#include "Serialization/ImageKeySerialization.h"
#include "Serialization/ImageParamsSerialization.h"
#include "Serialization/FrameKeySerialization.h"
#include "Serialization/FrameParamsSerialization.h"
#include "Serialization/SerializationFwd.h"


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

    // The plugin ID of the node holding this image
    std::string pluginID;


    SerializedEntry()
    : hash(0)
    , key()
    , params()
    , size(0)
    , filePath()
    , dataOffsetInFile(0)
    , pluginID()
    {
    }

    virtual void encode(YAML::Emitter& em) const OVERRIDE FINAL;

    virtual void decode(const YAML::Node& node) OVERRIDE FINAL;
};

template<typename EntryType>
class CacheSerialization
: public SerializationObjectBase
{

public:

    int cacheVersion;
    std::list<SerializedEntry<EntryType> > entries;


    virtual void encode(YAML::Emitter& em) const OVERRIDE FINAL;

    virtual void decode(const YAML::Node& node) OVERRIDE FINAL;
};




SERIALIZATION_NAMESPACE_EXIT;


#endif // Engine_CacheSerialization_h
