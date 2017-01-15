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
    QMutex lock;

    CacheWPtr cache;
    CacheEntryKeyBasePtr key;
    int cacheBucketIndex;
    bool cacheSignalRequired;

    CacheEntryBasePrivate(const CachePtr& cache)
    : lock()
    , cache(cache)
    , key()
    , cacheBucketIndex(-1)
    , cacheSignalRequired(false)
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

void
CacheEntryBase::setCacheBucketIndex(int index)
{
    QMutexLocker (&_imp->lock);
    _imp->cacheBucketIndex = index;
}


int
CacheEntryBase::getCacheBucketIndex() const
{
    QMutexLocker (&_imp->lock);
    return _imp->cacheBucketIndex;
}


CacheEntryKeyBasePtr
CacheEntryBase::getKey() const
{
    QMutexLocker (&_imp->lock);
    return _imp->key;
}

void
CacheEntryBase::setKey(const CacheEntryKeyBasePtr& key)
{
    QMutexLocker (&_imp->lock);
    _imp->key = key;
}

U64
CacheEntryBase::getHashKey() const
{
    QMutexLocker (&_imp->lock);
    if (!_imp->key) {
        return 0;
    }
    return _imp->key->getHash();
}

bool
CacheEntryBase::isCacheSignalRequired() const
{
    QMutexLocker (&_imp->lock);
    return _imp->cacheSignalRequired;
}

void
CacheEntryBase::setCacheSignalRequired(bool required)
{
    QMutexLocker (&_imp->lock);
    _imp->cacheSignalRequired = required;
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
CacheEntryBase::toMemorySegment(ExternalSegmentType* segment) const
{
    _imp->key->toMemorySegment(segment);
}

void
CacheEntryBase::fromMemorySegment(const ExternalSegmentType& segment)
{
    _imp->key->fromMemorySegment(segment);
}


NATRON_NAMESPACE_EXIT;