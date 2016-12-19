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

#ifndef SERIALIZATION_CACHEENTRYSERIALIZATION_H
#define SERIALIZATION_CACHEENTRYSERIALIZATION_H

#include "Serialization/SerializationBase.h"
#include "Serialization/SerializationFwd.h"

#define kBitDepthSerializationByte "B"
#define kBitDepthSerializationShort "S"
#define kBitDepthSerializationHalf "H"
#define kBitDepthSerializationFloat "F"

#define kSerializationImageTileTag "ImageTile"

SERIALIZATION_NAMESPACE_ENTER;


class CacheEntrySerializationBase
: public SerializationObjectBase
{
public:

    // Tag type to uniquely identify the item type to YAML
    std::string verbatimTag;

    // The hash can be recovered from the key, but it is also serialized to double check that the entry still
    // has a valid hash
    unsigned long long hash;

    // Even though entries have their filename given by their hash,
    // we need to serialize it as multiple entries can have the same hash.
    // Each entry with the same hash has an index appended
    std::string filePath;

    // At which byte in the cache file does this entry start. For Image cache generally each file is one entry so this is 0
    // but for a Tile cache, it is an offset in the cache memory files.
    std::size_t dataOffsetInFile;

    // The plugin ID of the node holding this image
    std::string pluginID;

    CacheEntrySerializationBase()
    : hash(0)
    , filePath()
    , dataOffsetInFile(0)
    , pluginID()
    {
    }

    virtual void encode(YAML::Emitter& em) const OVERRIDE;

    virtual void decode(const YAML::Node& node) OVERRIDE;
};

class MMAPBufferedEntrySerialization
: public CacheEntrySerializationBase
{
public:

    // The index of the tile
    int tileX, tileY;

    // The bitdepth of the tile
    std::string bitdepth;

    MMAPBufferedEntrySerialization()
    : CacheEntrySerializationBase()
    , tileX(0)
    , tileY(0)
    , bitdepth()
    {
    }

    virtual void encode(YAML::Emitter& em) const OVERRIDE;

    virtual void decode(const YAML::Node& node) OVERRIDE;
};

class ImageTileSerialization
: public MMAPBufferedEntrySerialization
{
public:

    unsigned long long nodeHashKey; // the hash of the node this image corresponds to
    std::string layerChannelName; // e.g: Disparity.X
    unsigned int mipMapLevel; // the mipmap level of the tile
    bool draft; // was it computed in draft mode ?

    ImageTileSerialization()
    : MMAPBufferedEntrySerialization()
    , nodeHashKey(0)
    , layerChannelName()
    , mipMapLevel(0)
    , draft(false)
    {
        verbatimTag = kSerializationImageTileTag;
    }

    virtual ~ImageTileSerialization()
    {
    }

    virtual void encode(YAML::Emitter& em) const OVERRIDE FINAL;

    virtual void decode(const YAML::Node& node) OVERRIDE FINAL;
};


SERIALIZATION_NAMESPACE_EXIT;

#endif // SERIALIZATION_CACHEENTRYSERIALIZATION_H
