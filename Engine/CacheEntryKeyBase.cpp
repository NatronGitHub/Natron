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
#include "Serialization/CacheEntrySerialization.h"

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


void
CacheEntryKeyBase::copy(const CacheEntryKeyBase& other)
{
    QMutexLocker k(&_imp->lock);
    QMutexLocker k2(&other._imp->lock);
    _imp->pluginID = other._imp->pluginID;
    _imp->hash = other._imp->hash;
    _imp->hashComputed = other._imp->hashComputed;
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

struct ImageTileKeyPrivate
{
    U64 nodeFrameViewHashKey;
    std::string layerChannel;
    RenderScale proxyScale;
    unsigned int mipMapLevel;
    bool draftMode;
    ImageBitDepthEnum bitdepth;
    int tileX;
    int tileY;

    ImageTileKeyPrivate()
    : nodeFrameViewHashKey(0)
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


ImageTileKey::ImageTileKey(U64 nodeFrameViewHashKey,
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
    _imp->nodeFrameViewHashKey = nodeFrameViewHashKey;
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
ImageTileKey::getNodeFrameViewHashKey() const
{
    return _imp->nodeFrameViewHashKey;
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
    hash->append(_imp->proxyScale.x);
    hash->append(_imp->proxyScale.y);
    hash->append(_imp->mipMapLevel);
    hash->append(_imp->draftMode);
    hash->append((int)_imp->bitdepth);
    hash->append(_imp->tileX);
    hash->append(_imp->tileY);
}

bool
ImageTileKey::equals(const CacheEntryKeyBase& other)
{
    const ImageTileKey* otherKey = dynamic_cast<const ImageTileKey*>(&other);
    assert(otherKey);
    if (!otherKey) {
        return false;
    }
    if (!CacheEntryKeyBase::equals(other)) {
        return false;
    }
    if (_imp->nodeFrameViewHashKey != otherKey->_imp->nodeFrameViewHashKey) {
        return false;
    }
    if (_imp->layerChannel != otherKey->_imp->layerChannel) {
        return false;
    }
    if (_imp->proxyScale.x != otherKey->_imp->proxyScale.x) {
        return false;
    }
    if (_imp->proxyScale.y != otherKey->_imp->proxyScale.y) {
        return false;
    }
    if (_imp->mipMapLevel != otherKey->_imp->mipMapLevel) {
        return false;
    }
    if (_imp->draftMode != otherKey->_imp->draftMode) {
        return false;
    }
    if (_imp->bitdepth != otherKey->_imp->bitdepth) {
        return false;
    }
    if (_imp->tileX != otherKey->_imp->tileX) {
        return false;
    }
    if (_imp->tileY != otherKey->_imp->tileY) {
        return false;
    }
    return true;
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

static std::string bitDepthToString(ImageBitDepthEnum depth)
{
    switch (depth) {
        case eImageBitDepthByte:
            return kBitDepthSerializationByte;
        case eImageBitDepthFloat:
            return kBitDepthSerializationFloat;
        case eImageBitDepthShort:
            return kBitDepthSerializationShort;
        case eImageBitDepthHalf:
            return kBitDepthSerializationHalf;
        case eImageBitDepthNone:
            return "";
    }
}

static ImageBitDepthEnum bitDepthFromString(const std::string& depth)
{
    if (depth == kBitDepthSerializationByte) {
        return eImageBitDepthByte;
    } else if (depth == kBitDepthSerializationFloat) {
        return eImageBitDepthFloat;
    } else if (depth == kBitDepthSerializationShort) {
        return eImageBitDepthShort;
    } else if (depth == kBitDepthSerializationHalf) {
        return eImageBitDepthHalf;
    } else {
        return eImageBitDepthNone;
    }
}

void
ImageTileKey::toSerialization(SERIALIZATION_NAMESPACE::SerializationObjectBase* serializationBase)
{
    SERIALIZATION_NAMESPACE::ImageTileSerialization* serialization = dynamic_cast<SERIALIZATION_NAMESPACE::ImageTileSerialization*>(serializationBase);
    if (!serialization) {
        return;
    }
    serialization->nodeHashKey = _imp->nodeFrameViewHashKey;
    serialization->layerChannelName = _imp->layerChannel;
    serialization->tileX = _imp->tileX;
    serialization->tileY = _imp->tileY;
    serialization->scale[0] = _imp->proxyScale.x;
    serialization->scale[1] = _imp->proxyScale.y;
    serialization->mipMapLevel = _imp->mipMapLevel;
    serialization->draft = _imp->draftMode;
    serialization->bitdepth = bitDepthToString(_imp->bitdepth);
}

void
ImageTileKey::fromSerialization(const SERIALIZATION_NAMESPACE::SerializationObjectBase& serializationBase)
{
    const SERIALIZATION_NAMESPACE::ImageTileSerialization* serialization = dynamic_cast<const SERIALIZATION_NAMESPACE::ImageTileSerialization*>(&serializationBase);
    if (!serialization) {
        return;
    }
    _imp->nodeFrameViewHashKey = serialization->nodeHashKey;
    _imp->layerChannel = serialization->layerChannelName;
    _imp->tileX = serialization->tileX;
    _imp->tileY = serialization->tileY;
    _imp->proxyScale.x = serialization->scale[0];
    _imp->proxyScale.y = serialization->scale[1];
    _imp->mipMapLevel = serialization->mipMapLevel;
    _imp->draftMode = serialization->draft;
    _imp->bitdepth = bitDepthFromString(serialization->bitdepth);
}

NATRON_NAMESPACE_EXIT;