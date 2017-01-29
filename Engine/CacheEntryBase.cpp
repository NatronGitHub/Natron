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

#include "CacheEntryBase.h"

#include <QDir>

#include "Engine/Cache.h"

NATRON_NAMESPACE_ENTER;

struct CacheEntryBasePrivate
{

    CacheWPtr cache;
    CacheEntryKeyBasePtr key;

    CacheEntryBasePrivate(const CachePtr& cache)
    : cache(cache)
    , key()
    {
        
    }
};

CacheEntryBase::CacheEntryBase(const CachePtr& cache)
: _imp(new CacheEntryBasePrivate(cache))
{

}

CacheEntryBase::~CacheEntryBase()
{

}

CachePtr
CacheEntryBase::getCache() const
{
    return _imp->cache.lock();
}


CacheEntryKeyBasePtr
CacheEntryBase::getKey() const
{
    return _imp->key;
}

void
CacheEntryBase::setKey(const CacheEntryKeyBasePtr& key)
{
    _imp->key = key;
}

U64
CacheEntryBase::getHashKey() const
{
    if (!_imp->key) {
        return 0;
    }
    return _imp->key->getHash();
}


TimeValue
CacheEntryBase::getTime() const
{
    if (!_imp->key) {
        return TimeValue(0.);
    }
    return _imp->key->getTime();

}

ViewIdx
CacheEntryBase::getView() const
{
    if (!_imp->key) {
        return ViewIdx(0);
    }
    return _imp->key->getView();
}

std::size_t
CacheEntryBase::getMetadataSize() const
{
    return _imp->key->getMetadataSize();
}

void
CacheEntryBase::toMemorySegment(ExternalSegmentType* segment, const std::string& objectNamesPrefix, ExternalSegmentTypeHandleList* objectPointers, void* /*tileDataPtr*/) const
{
    _imp->key->toMemorySegment(segment, objectNamesPrefix, objectPointers);
}

void
CacheEntryBase::fromMemorySegment(ExternalSegmentType* segment, const std::string& objectNamesPrefix, const void* /*tileDataPtr*/)
{
    _imp->key->fromMemorySegment(segment, objectNamesPrefix);
}


NATRON_NAMESPACE_EXIT;