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


// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "CacheEntryKeyBase.h"

#include <QMutex>
#include <QDebug>
#include "Engine/Hash64.h"

namespace bip = boost::interprocess;

NATRON_NAMESPACE_ENTER;

struct CacheEntryKeyBasePrivate
{
    mutable QMutex lock;
    std::string pluginID;
    mutable U64 hash;
    mutable bool hashComputed;

    CacheEntryKeyBasePrivate()
    : lock()
    , pluginID()
    , hash(0)
    , hashComputed(false)
    {

    }
};

CacheEntryKeyBase::CacheEntryKeyBase()
: _imp(new CacheEntryKeyBasePrivate)
{
}

CacheEntryKeyBase::CacheEntryKeyBase(const std::string& pluginID)
: _imp(new CacheEntryKeyBasePrivate)
{
    _imp->pluginID = pluginID;
}


CacheEntryKeyBase::~CacheEntryKeyBase()
{
}



U64
CacheEntryKeyBase::getHash() const
{
    QMutexLocker k(&_imp->lock);
    if (_imp->hashComputed) {
        return _imp->hash;
    }

    Hash64 hash;
    hash.append(getUniqueID());
    appendToHash(&hash);
    hash.computeHash();
    _imp->hash = hash.value();
    _imp->hashComputed = true;
    return _imp->hash;
    
}

std::string
CacheEntryKeyBase::hashToString(U64 hash)
{
    std::stringstream ss;
    ss << std::hex << hash;
    return ss.str();
}

std::string
CacheEntryKeyBase::getHolderPluginID() const
{
    QMutexLocker k(&_imp->lock);
    return _imp->pluginID;
}

void
CacheEntryKeyBase::setHolderPluginID(const std::string& holderID) {
    QMutexLocker k(&_imp->lock);
    _imp->pluginID = holderID;
}

std::size_t
CacheEntryKeyBase::getMetadataSize() const
{
    std::size_t ret = 0;

    // Also count the null character.
    ret += (_imp->pluginID.size() + 1) * sizeof(char);
    ret += sizeof(_imp->hash);

    return ret;
}

void
CacheEntryKeyBase::toMemorySegment(ExternalSegmentType* segment, const std::string& objectNamesPrefix, ExternalSegmentTypeHandleList* objectPointers) const
{

    // Write a hash as a magic number: the hash should be the last item wrote to the external memory segment.
    // When reading, if the hash could be recovered correctly, we know that the entry is valid.
    U64 hash = getHash();
    objectPointers->push_back(writeNamedSharedObject(hash, objectNamesPrefix + "magic", segment));
}


void
CacheEntryKeyBase::fromMemorySegment(ExternalSegmentType* segment, const std::string& objectNamesPrefix)
{
    U64 serializedHash;
    readNamedSharedObject(objectNamesPrefix + "magic", segment, &serializedHash);


    // We are done serializing, compute the hash and compare against the serialized hash
    _imp->hashComputed = false;
    U64 computedHash = getHash();
    if (computedHash != serializedHash) {

        // They are different: either the process writing this entry crashed or another object type
        // was associated to the hash.
        throw std::bad_alloc();
    }

}

struct ImageTileKeyShmData
{
    U64 nodeTimeViewVariantHash;
    RenderScale proxyScale;
    unsigned int mipMapLevel;
    bool draftMode;
    ImageBitDepthEnum bitdepth;
    RectI tileBounds;
    

    ImageTileKeyShmData()
    : nodeTimeViewVariantHash(0)
    , proxyScale(1.)
    , mipMapLevel(0)
    , draftMode(false)
    , bitdepth(eImageBitDepthNone)
    , tileBounds()
    {

    }
};


struct ImageTileKeyPrivate
{

    ImageTileKeyShmData data;
    std::string layerChannel;

    ImageTileKeyPrivate()
    : data()
    , layerChannel()
    {
    }
};


ImageTileKey::ImageTileKey(U64 nodeTimeViewVariantHash,
                           const std::string& layerChannel,
                           const RenderScale& scale,
                           unsigned int mipMapLevel,
                           bool draftMode,
                           ImageBitDepthEnum bitdepth,
                           const RectI& tileBounds)
: CacheEntryKeyBase()
, _imp(new ImageTileKeyPrivate())
{
    _imp->data.nodeTimeViewVariantHash = nodeTimeViewVariantHash;
    _imp->layerChannel = layerChannel;
    _imp->data.proxyScale = scale;
    _imp->data.mipMapLevel = mipMapLevel;
    _imp->data.draftMode = draftMode;
    _imp->data.bitdepth = bitdepth;
    _imp->data.tileBounds = tileBounds;
}

ImageTileKey::ImageTileKey()
: CacheEntryKeyBase()
, _imp(new ImageTileKeyPrivate())
{

}

ImageTileKey::~ImageTileKey()
{

}

U64
ImageTileKey::getNodeTimeInvariantHashKey() const
{
    return _imp->data.nodeTimeViewVariantHash;
}

int
ImageTileKey::getUniqueID() const
{
    return kCacheKeyUniqueIDImageTile;
}

void
ImageTileKey::appendToHash(Hash64* hash) const
{
    Hash64::appendQString(QString::fromUtf8(_imp->layerChannel.c_str()), hash);
    hash->append(_imp->data.nodeTimeViewVariantHash);
    hash->append(_imp->data.proxyScale.x);
    hash->append(_imp->data.proxyScale.y);
    hash->append(_imp->data.mipMapLevel);
    hash->append(_imp->data.draftMode);
    hash->append((int)_imp->data.bitdepth);
    hash->append(_imp->data.tileBounds.x1);
    hash->append(_imp->data.tileBounds.x2);
    hash->append(_imp->data.tileBounds.y1);
    hash->append(_imp->data.tileBounds.y2);
}

std::string
ImageTileKey::getLayerChannel() const
{
    return _imp->layerChannel;
}

const RectI&
ImageTileKey::getTileBounds() const
{
    return _imp->data.tileBounds;
}


const RenderScale &
ImageTileKey::getProxyScale() const
{
    return _imp->data.proxyScale;
}

unsigned int
ImageTileKey::getMipMapLevel() const
{
    return _imp->data.mipMapLevel;
}

bool
ImageTileKey::isDraftMode() const
{
    return _imp->data.draftMode;
}

ImageBitDepthEnum
ImageTileKey::getBitDepth() const
{
    return _imp->data.bitdepth;
}

std::size_t
ImageTileKey::getMetadataSize() const
{
    std::size_t ret = CacheEntryKeyBase::getMetadataSize();

    // Also count the null character.
    ret += sizeof(_imp->data.nodeTimeViewVariantHash);
    ret += (_imp->layerChannel.size() + 1) * sizeof(char);
    ret += sizeof(_imp->data.tileBounds);
    ret += sizeof(_imp->data.proxyScale);
    ret += sizeof(_imp->data.mipMapLevel);
    ret += sizeof(_imp->data.draftMode);
    ret += sizeof(_imp->data.bitdepth);

    return ret;
}

void
ImageTileKey::toMemorySegment(ExternalSegmentType* segment, const std::string& objectNamesPrefix, ExternalSegmentTypeHandleList* objectPointers) const
{
    
    ImageTileKeyShmData* data = segment->construct<ImageTileKeyShmData>(std::string(objectNamesPrefix + "KeyData").c_str())();
    if (!data) {
        throw std::bad_alloc();
    }
    objectPointers->push_back(segment->get_handle_from_address(data));


    CharAllocator_ExternalSegment allocator(segment->get_segment_manager());
    String_ExternalSegment* layerChannel = segment->construct<String_ExternalSegment>(std::string(objectNamesPrefix + "LayerChannel").c_str())(allocator);
    if (!layerChannel) {
        throw std::bad_alloc();
    }
    objectPointers->push_back(segment->get_handle_from_address(layerChannel));

    data->nodeTimeViewVariantHash = _imp->data.nodeTimeViewVariantHash;
    data->tileBounds = _imp->data.tileBounds;
    data->proxyScale = _imp->data.proxyScale;
    data->mipMapLevel = _imp->data.mipMapLevel;
    data->draftMode = _imp->data.draftMode;
    data->bitdepth = _imp->data.bitdepth;
    layerChannel->append(_imp->layerChannel.c_str());

    CacheEntryKeyBase::toMemorySegment(segment, objectNamesPrefix, objectPointers);
}


void
ImageTileKey::fromMemorySegment(ExternalSegmentType* segment, const std::string& objectNamesPrefix)
{
    ImageTileKeyShmData* data = segment->find<ImageTileKeyShmData>(std::string(objectNamesPrefix + "KeyData").c_str()).first;
    if (!data) {
        throw std::bad_alloc();
    }
    String_ExternalSegment* layersChannels = segment->find<String_ExternalSegment>(std::string(objectNamesPrefix + "LayerChannel").c_str()).first;
    if (!layersChannels) {
        throw std::bad_alloc();
    }

    _imp->data.nodeTimeViewVariantHash = data->nodeTimeViewVariantHash;
    _imp->data.tileBounds = data->tileBounds;
    _imp->data.proxyScale = data->proxyScale;
    _imp->data.mipMapLevel = data->mipMapLevel;
    _imp->data.draftMode = data->draftMode;
    _imp->data.bitdepth = data->bitdepth;
    _imp->layerChannel.clear();
    _imp->layerChannel.append(layersChannels->c_str());


    CacheEntryKeyBase::fromMemorySegment(segment, objectNamesPrefix);
}


NATRON_NAMESPACE_EXIT;
