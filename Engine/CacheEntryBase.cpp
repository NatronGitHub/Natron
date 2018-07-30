/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
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


#include "Engine/Cache.h"

NATRON_NAMESPACE_ENTER

struct CacheEntryBasePrivate
{

    CacheBaseWPtr cache;
    CacheEntryKeyBasePtr key;

    CacheEntryBasePrivate(const CacheBasePtr& cache)
    : cache(cache)
    , key()
    {
        
    }
};

CacheEntryBase::CacheEntryBase(const CacheBasePtr& cache)
: _imp(new CacheEntryBasePrivate(cache))
{

}

CacheEntryBase::~CacheEntryBase()
{

}

CacheBasePtr
CacheEntryBase::getCache() const
{
    return _imp->cache.lock();
}

CacheEntryLockerBasePtr
CacheEntryBase::getFromCache() const
{
    CacheBasePtr cache = getCache();
    CacheEntryBasePtr thisShared = boost::const_pointer_cast<CacheEntryBase>(shared_from_this());
    return cache->get(thisShared);
}

bool
CacheEntryBase::isPersistent() const
{
    return getCache()->isPersistent();
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
CacheEntryBase::getHashKey(bool forceComputation) const
{
    if (!_imp->key) {
        return 0;
    }
    return _imp->key->getHash(forceComputation);
}

std::size_t
CacheEntryBase::getMetadataSize() const
{
    return _imp->key->getMetadataSize();
}

void
CacheEntryBase::toMemorySegment(IPCPropertyMap* properties) const
{
    _imp->key->toMemorySegment(properties);
}

CacheEntryBase::FromMemorySegmentRetCodeEnum
CacheEntryBase::fromMemorySegment(bool /*isLockedForWriting*/,
                                  const IPCPropertyMap& properties)

{
    CacheEntryKeyBase::FromMemorySegmentRetCodeEnum stat = _imp->key->fromMemorySegment(properties);
    if (stat == CacheEntryKeyBase::eFromMemorySegmentRetCodeFailed) {
        return eFromMemorySegmentRetCodeFailed;
    }
    return eFromMemorySegmentRetCodeOk;
}


NATRON_NAMESPACE_EXIT
