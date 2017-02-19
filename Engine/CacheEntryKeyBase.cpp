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
CacheEntryKeyBase::getHash(bool forceComputation) const
{
    QMutexLocker k(&_imp->lock);
    if (!forceComputation && _imp->hashComputed) {
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
    return 0;
}

void
CacheEntryKeyBase::toMemorySegment(ExternalSegmentType* /*segment*/, ExternalSegmentTypeHandleList* /*objectPointers*/) const
{

}


void
CacheEntryKeyBase::fromMemorySegment(ExternalSegmentType* /*segment*/,
                                     ExternalSegmentTypeHandleList::const_iterator /*start*/,
                                     ExternalSegmentTypeHandleList::const_iterator /*end*/)

{

}

struct ImageTileKeyShmData
{
    U64 nodeTimeViewVariantHash;
    U64 layerChannelID;
    RenderScale proxyScale;
    unsigned int mipMapLevel;
    bool draftMode;
    ImageBitDepthEnum bitdepth;
    RectI tileBounds;
    

    ImageTileKeyShmData()
    : nodeTimeViewVariantHash(0)
    , layerChannelID(0)
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

    ImageTileKeyPrivate()
    : data()
    {
    }
};


ImageTileKey::ImageTileKey(U64 nodeTimeViewVariantHash,
                           U64 layerChannelID,
                           const RenderScale& scale,
                           unsigned int mipMapLevel,
                           bool draftMode,
                           ImageBitDepthEnum bitdepth,
                           const RectI& tileBounds,
                           const std::string& pluginID)
: CacheEntryKeyBase(pluginID)
, _imp(new ImageTileKeyPrivate())
{
    _imp->data.nodeTimeViewVariantHash = nodeTimeViewVariantHash;
    _imp->data.layerChannelID = layerChannelID;
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
    hash->append(_imp->data.nodeTimeViewVariantHash);
    hash->append(_imp->data.layerChannelID);
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

void
ImageTileKey::toMemorySegment(ExternalSegmentType* segment, ExternalSegmentTypeHandleList* objectPointers) const
{
    
    ImageTileKeyShmData* data = segment->construct<ImageTileKeyShmData>(boost::interprocess::anonymous_instance)();
    if (!data) {
        throw std::bad_alloc();
    }
    objectPointers->push_back(segment->get_handle_from_address(data));

    data->nodeTimeViewVariantHash = _imp->data.nodeTimeViewVariantHash;
    data->tileBounds = _imp->data.tileBounds;
    data->proxyScale = _imp->data.proxyScale;
    data->mipMapLevel = _imp->data.mipMapLevel;
    data->draftMode = _imp->data.draftMode;
    data->bitdepth = _imp->data.bitdepth;
    data->layerChannelID = _imp->data.layerChannelID;
 }


void
ImageTileKey::fromMemorySegment(ExternalSegmentType* segment,
                                ExternalSegmentTypeHandleList::const_iterator start,
                                ExternalSegmentTypeHandleList::const_iterator end)
{
    if (start == end) {
        throw std::bad_alloc();
    }
    ImageTileKeyShmData* data = (ImageTileKeyShmData*)segment->get_address_from_handle(*start);
    ++start;
  
    _imp->data.nodeTimeViewVariantHash = data->nodeTimeViewVariantHash;
    _imp->data.tileBounds = data->tileBounds;
    _imp->data.proxyScale = data->proxyScale;
    _imp->data.mipMapLevel = data->mipMapLevel;
    _imp->data.draftMode = data->draftMode;
    _imp->data.bitdepth = data->bitdepth;
    _imp->data.layerChannelID = data->layerChannelID;

}


NATRON_NAMESPACE_EXIT;
