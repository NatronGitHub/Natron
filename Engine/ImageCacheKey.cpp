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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "ImageCacheKey.h"

#include "Engine/Hash64.h"

NATRON_NAMESPACE_ENTER;

struct ImageCacheKeyShmData
{
    U64 nodeTimeViewVariantHash;
    U64 layerChannelID;
    RenderScale proxyScale;
    unsigned int mipMapLevel;
    bool draftMode;
    ImageBitDepthEnum bitdepth;


    ImageCacheKeyShmData()
    : nodeTimeViewVariantHash(0)
    , layerChannelID(0)
    , proxyScale(1.)
    , mipMapLevel(0)
    , draftMode(false)
    , bitdepth(eImageBitDepthNone)
    {

    }
};


struct ImageCacheKeyPrivate
{

    ImageCacheKeyShmData data;

    ImageCacheKeyPrivate()
    : data()
    {
    }
};


ImageCacheKey::ImageCacheKey(U64 nodeTimeViewVariantHash,
                             U64 layerChannelID,
                             const RenderScale& scale,
                             unsigned int mipMapLevel,
                             bool draftMode,
                             ImageBitDepthEnum bitdepth,
                             const std::string& pluginID)
: CacheEntryKeyBase(pluginID)
, _imp(new ImageCacheKeyPrivate())
{
    _imp->data.nodeTimeViewVariantHash = nodeTimeViewVariantHash;
    _imp->data.layerChannelID = layerChannelID;
    _imp->data.proxyScale = scale;
    _imp->data.mipMapLevel = mipMapLevel;
    _imp->data.draftMode = draftMode;
    _imp->data.bitdepth = bitdepth;
}

ImageCacheKey::ImageCacheKey()
: CacheEntryKeyBase()
, _imp(new ImageCacheKeyPrivate())
{

}

ImageCacheKey::~ImageCacheKey()
{

}

U64
ImageCacheKey::getNodeTimeInvariantHashKey() const
{
    return _imp->data.nodeTimeViewVariantHash;
}

int
ImageCacheKey::getUniqueID() const
{
    return kCacheKeyUniqueIDCacheImage;
}

void
ImageCacheKey::appendToHash(Hash64* hash) const
{
    std::vector<U64> elements(11);
    elements[0] = Hash64::toU64(_imp->data.nodeTimeViewVariantHash);
    elements[1] = Hash64::toU64(_imp->data.layerChannelID);
    elements[2] = Hash64::toU64(_imp->data.proxyScale.x);
    elements[3] = Hash64::toU64(_imp->data.proxyScale.y);
    elements[4] = Hash64::toU64(_imp->data.mipMapLevel);
    elements[5] = Hash64::toU64(_imp->data.draftMode);
    elements[6] = Hash64::toU64((int)_imp->data.bitdepth);
    hash->insert(elements);
}


const RenderScale &
ImageCacheKey::getProxyScale() const
{
    return _imp->data.proxyScale;
}

unsigned int
ImageCacheKey::getMipMapLevel() const
{
    return _imp->data.mipMapLevel;
}

bool
ImageCacheKey::isDraftMode() const
{
    return _imp->data.draftMode;
}

ImageBitDepthEnum
ImageCacheKey::getBitDepth() const
{
    return _imp->data.bitdepth;
}


void
ImageCacheKey::toMemorySegment(ExternalSegmentType* segment, ExternalSegmentTypeHandleList* objectPointers) const
{

    ImageCacheKeyShmData* data = segment->construct<ImageCacheKeyShmData>(boost::interprocess::anonymous_instance)();
    if (!data) {
        throw std::bad_alloc();
    }
    objectPointers->push_back(segment->get_handle_from_address(data));

    data->nodeTimeViewVariantHash = _imp->data.nodeTimeViewVariantHash;
    data->proxyScale = _imp->data.proxyScale;
    data->mipMapLevel = _imp->data.mipMapLevel;
    data->draftMode = _imp->data.draftMode;
    data->bitdepth = _imp->data.bitdepth;
    data->layerChannelID = _imp->data.layerChannelID;
}


void
ImageCacheKey::fromMemorySegment(ExternalSegmentType* segment,
                            ExternalSegmentTypeHandleList::const_iterator start,
                            ExternalSegmentTypeHandleList::const_iterator end)
{
    if (start == end) {
        throw std::bad_alloc();
    }
    ImageCacheKeyShmData* data = (ImageCacheKeyShmData*)segment->get_address_from_handle(*start);
    ++start;

    _imp->data.nodeTimeViewVariantHash = data->nodeTimeViewVariantHash;
    _imp->data.proxyScale = data->proxyScale;
    _imp->data.mipMapLevel = data->mipMapLevel;
    _imp->data.draftMode = data->draftMode;
    _imp->data.bitdepth = data->bitdepth;
    _imp->data.layerChannelID = data->layerChannelID;
    
}


NATRON_NAMESPACE_EXIT;
