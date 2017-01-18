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

#include "Engine/Hash64.h"

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
        _imp->hashComputed = true;
        return _imp->hash;
    }

    Hash64 hash;
    hash.append(getUniqueID());
    appendToHash(&hash);
    hash.computeHash();
    _imp->hash = hash.value();
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
CacheEntryKeyBase::toMemorySegment(ExternalSegmentType* segment) const
{
    writeMMObject(_imp->pluginID, "pluginID", segment);

    // Write a hash as a magic number: the hash should be the last item wrote to the external memory segment.
    // When reading, if the hash could be recovered correctly, we know that the entry is valid.
    writeMMObject(_imp->hash, "magic", segment);
}


void
CacheEntryKeyBase::fromMemorySegment(ExternalSegmentType* segment)
{
    readMMObject("pluginID", segment, &_imp->pluginID);

    U64 serializedHash;
    readMMObject("magic", segment, &serializedHash);

    // The hash should not have been computed
    assert(!_imp->hashComputed);

    // We are done serializing, compute the hash and compare against the serialized hash
    U64 computedHash = getHash();
    if (computedHash != serializedHash) {

        // They are different: either the process writing this entry crashed or another object type
        // was associated to the hash.
        throw std::bad_alloc();
    }

}


struct ImageTileKeyPrivate
{
    U64 nodeTimeInvariantHash;
    TimeValue time;
    ViewIdx view;
    std::string layerChannel;
    RenderScale proxyScale;
    unsigned int mipMapLevel;
    bool draftMode;
    ImageBitDepthEnum bitdepth;
    int tileX;
    int tileY;

    ImageTileKeyPrivate()
    : nodeTimeInvariantHash(0)
    , time(0)
    , view(0)
    , layerChannel()
    , proxyScale(1.)
    , mipMapLevel(0)
    , draftMode(false)
    , bitdepth(eImageBitDepthNone)
    , tileX(0)
    , tileY(0)
    {

    }
};


ImageTileKey::ImageTileKey(U64 nodeTimeInvariantHash,
                           TimeValue time,
                           ViewIdx view,
                           const std::string& layerChannel,
                           const RenderScale& scale,
                           unsigned int mipMapLevel,
                           bool draftMode,
                           ImageBitDepthEnum bitdepth,
                           int tileX,
                           int tileY)
: CacheEntryKeyBase()
, _imp(new ImageTileKeyPrivate())
{
    _imp->nodeTimeInvariantHash = nodeTimeInvariantHash;
    _imp->time = time;
    _imp->view = view;
    _imp->layerChannel = layerChannel;
    _imp->proxyScale = scale;
    _imp->mipMapLevel = mipMapLevel;
    _imp->draftMode = draftMode;
    _imp->bitdepth = bitdepth;
    _imp->tileX = tileX;
    _imp->tileY = tileY;
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
    return _imp->nodeTimeInvariantHash;
}

TimeValue
ImageTileKey::getTime() const
{
    return _imp->time;
}

ViewIdx
ImageTileKey::getView() const
{
    return _imp->view;
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
    hash->append(_imp->nodeTimeInvariantHash);
    hash->append((double)_imp->time);
    hash->append((int)_imp->view);
    hash->append(_imp->proxyScale.x);
    hash->append(_imp->proxyScale.y);
    hash->append(_imp->mipMapLevel);
    hash->append(_imp->draftMode);
    hash->append((int)_imp->bitdepth);
    hash->append(_imp->tileX);
    hash->append(_imp->tileY);
}

std::string
ImageTileKey::getLayerChannel() const
{
    return _imp->layerChannel;
}

int
ImageTileKey::getTileX() const
{
    return _imp->tileX;
}

int
ImageTileKey::getTileY() const
{
    return _imp->tileY;
}

RenderScale
ImageTileKey::getProxyScale() const
{
    return _imp->proxyScale;
}

unsigned int
ImageTileKey::getMipMapLevel() const
{
    return _imp->mipMapLevel;
}

bool
ImageTileKey::isDraftMode() const
{
    return _imp->draftMode;
}

ImageBitDepthEnum
ImageTileKey::getBitDepth() const
{
    return _imp->bitdepth;
}

std::size_t
ImageTileKey::getMetadataSize() const
{
    std::size_t ret = CacheEntryKeyBase::getMetadataSize();

    // Also count the null character.
    ret += sizeof(_imp->nodeTimeInvariantHash);
    ret += (_imp->layerChannel.size() + 1) * sizeof(char);
    ret += sizeof(_imp->time);
    ret += sizeof(_imp->view);
    ret += sizeof(_imp->tileX);
    ret += sizeof(_imp->tileY);
    ret += sizeof(_imp->proxyScale.x);
    ret += sizeof(_imp->proxyScale.y);
    ret += sizeof(_imp->mipMapLevel);
    ret += sizeof(_imp->draftMode);
    ret += sizeof(_imp->bitdepth);

    return ret;
}


void
ImageTileKey::toMemorySegment(ExternalSegmentType* segment) const
{
    writeMMObject(_imp->nodeTimeInvariantHash, "hash", segment);
    writeMMObject(_imp->layerChannel, "channel", segment);
    writeMMObject(_imp->time, "time", segment);
    writeMMObject(_imp->view, "view", segment);
    writeMMObject(_imp->tileX, "tx", segment);
    writeMMObject(_imp->tileY, "ty", segment);
    writeMMObject(_imp->proxyScale.x, "sx", segment);
    writeMMObject(_imp->proxyScale.y, "sy", segment);
    writeMMObject(_imp->mipMapLevel, "lod", segment);
    writeMMObject(_imp->draftMode, "draft", segment);
    writeMMObject(_imp->bitdepth, "depth", segment);
    CacheEntryKeyBase::toMemorySegment(segment);
}


void
ImageTileKey::fromMemorySegment(ExternalSegmentType* segment)
{
    readMMObject("hash", segment, &_imp->nodeTimeInvariantHash);
    readMMObject("channel", segment, &_imp->layerChannel);
    readMMObject("time", segment, &_imp->time);
    readMMObject("view", segment, &_imp->view);
    readMMObject("tx", segment, &_imp->tileX);
    readMMObject("ty", segment, &_imp->tileY);
    readMMObject("sx", segment, &_imp->proxyScale.x);
    readMMObject("sy", segment, &_imp->proxyScale.y);
    readMMObject("lod", segment, &_imp->mipMapLevel);
    readMMObject("draft", segment, &_imp->draftMode);
    readMMObject("depth", segment, &_imp->bitdepth);
    CacheEntryKeyBase::fromMemorySegment(segment);
}


NATRON_NAMESPACE_EXIT;