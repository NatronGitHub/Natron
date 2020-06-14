/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2020 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

NATRON_NAMESPACE_ENTER


struct ImageCacheKeyShmData
{
    U64 nodeTimeViewVariantHash;
    U64 layerIDHash;
    RenderScale proxyScale;


    ImageCacheKeyShmData()
    : nodeTimeViewVariantHash(0)
    , layerIDHash(0)
    , proxyScale(1.)
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
                             U64 layerIDHash,
                             const RenderScale& scale,
                             const std::string& pluginID)
: CacheEntryKeyBase(pluginID)
, _imp(new ImageCacheKeyPrivate())
{
    _imp->data.nodeTimeViewVariantHash = nodeTimeViewVariantHash;
    _imp->data.layerIDHash = layerIDHash;
    _imp->data.proxyScale = scale;
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
ImageCacheKey::getNodeTimeVariantHashKey() const
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
    std::vector<U64> elements(4);
    elements[0] = Hash64::toU64(_imp->data.nodeTimeViewVariantHash);
    elements[1] = Hash64::toU64(_imp->data.layerIDHash);
    elements[2] = Hash64::toU64(_imp->data.proxyScale.x);
    elements[3] = Hash64::toU64(_imp->data.proxyScale.y);
    hash->insert(elements);
}


const RenderScale &
ImageCacheKey::getProxyScale() const
{
    return _imp->data.proxyScale;
}

void
ImageCacheKey::toMemorySegment(IPCPropertyMap* properties) const
{
    properties->setIPCProperty("NodeHash", _imp->data.nodeTimeViewVariantHash);
    properties->setIPCProperty("ChannelID", _imp->data.layerIDHash);
    std::vector<double> scaleVec(2);
    scaleVec[0] = _imp->data.proxyScale.x;
    scaleVec[1] = _imp->data.proxyScale.y;
    properties->setIPCPropertyN("Scale", scaleVec);
}


CacheEntryKeyBase::FromMemorySegmentRetCodeEnum
ImageCacheKey::fromMemorySegment(const IPCPropertyMap& properties)
{
    if (!properties.getIPCProperty("NodeHash", 0, &_imp->data.nodeTimeViewVariantHash)) {
        return eFromMemorySegmentRetCodeFailed;
    }
    if (!properties.getIPCProperty("ChannelID", 0, &_imp->data.layerIDHash)) {
        return eFromMemorySegmentRetCodeFailed;
    }
    std::vector<double> scaleVec;
    if (!properties.getIPCPropertyN("Scale", &scaleVec)) {
        return eFromMemorySegmentRetCodeFailed;
    }
    _imp->data.proxyScale.x = scaleVec[0];
    _imp->data.proxyScale.y = scaleVec[1];

    return eFromMemorySegmentRetCodeOk;
}


NATRON_NAMESPACE_EXIT
