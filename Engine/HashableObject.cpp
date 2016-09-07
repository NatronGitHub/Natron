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

#include "HashableObject.h"

#include <QMutex>

#include "Engine/Hash64.h"
#include "Engine/ParallelRenderArgs.h"


NATRON_NAMESPACE_ENTER

struct HashableObjectPrivate
{
    // The parent if any
    HashableObjectWPtr parent;

    // The hash cache
    mutable FrameViewHashMap hashCache;

    // protects hashCache
    mutable QMutex hashCacheMutex;

    HashableObjectPrivate()
    : parent()
    {

    }
};

HashableObject::HashableObject()
: _imp(new HashableObjectPrivate())
{
    
}

HashableObject::~HashableObject()
{

}

void
HashableObject::setHashParent(const HashableObjectPtr& parent)
{
    _imp->parent = parent;
}

HashableObjectPtr
HashableObject::getHashParent() const
{
    return _imp->parent.lock();
}



U64
HashableObject::findCachedHash(double time, ViewIdx view) const
{
    QMutexLocker k(&_imp->hashCacheMutex);
    U64 foundCached = findFrameViewHash(time, view, _imp->hashCache);
    if (foundCached != 0) {
        return foundCached;
    }
    return 0;
}


void
HashableObject::computeHash_noCache(double time, ViewIdx view, Hash64* hash)
{

    // Call the virtual portion to let implementation add extra fields to the hash
    appendToHash(time, view, hash);
}


void
HashableObject::addHashToCache(double time, ViewIdx view, U64 hashValue)
{
    QMutexLocker k(&_imp->hashCacheMutex);
    FrameViewPair fv = {time, view};
    _imp->hashCache[fv] = hashValue;
}

U64
HashableObject::computeHash(double time, ViewIdx view)
{
    {
        // Find a hash in the cache..
        QMutexLocker k(&_imp->hashCacheMutex);
        U64 foundCached = findFrameViewHash(time, view, _imp->hashCache);
        if (foundCached != 0) {
            return foundCached;
        }

        // Compute it
        Hash64 hash;
        computeHash_noCache(time, view, &hash);
        hash.computeHash();
        U64 ret = hash.value();

        // Cache it
        FrameViewPair fv = {time, view};
        _imp->hashCache[fv] = ret;
        return ret;

    }

}

void
HashableObject::invalidateHashCache()
{
    {
        QMutexLocker k(&_imp->hashCacheMutex);
        _imp->hashCache.clear();
    }
    HashableObjectPtr parent = getHashParent();
    if (parent) {
        parent->invalidateHashCache();
    }
}


NATRON_NAMESPACE_EXIT