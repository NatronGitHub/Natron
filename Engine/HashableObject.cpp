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

#include "HashableObject.h"
#include <list>
#include <QMutex>

#include "Engine/Hash64.h"
#include "Engine/FrameViewRequest.h"

NATRON_NAMESPACE_ENTER

struct HashableObjectPrivate
{
    // The parent if any
    std::list<HashableObjectWPtr> hashListeners;

    // The hash cache
    mutable FrameViewHashMap timeViewVariantHashCache;

    U64 timeViewInvariantCache;

    bool timeViewInvariantCacheValid;

    U64 metadataSlaveCache;

    bool metadataSlaveCacheValid;

    // protects hashCache
    mutable QMutex hashCacheMutex;

    HashableObjectPrivate()
    : hashListeners()
    , timeViewVariantHashCache()
    , timeViewInvariantCache(0)
    , timeViewInvariantCacheValid(false)
    , metadataSlaveCache(0)
    , metadataSlaveCacheValid(false)
    , hashCacheMutex(QMutex::Recursive) // It might recurse when calling getValue on a knob with an expression because of randomSeed
    {

    }

    HashableObjectPrivate(const HashableObjectPrivate& other)
    : hashListeners(other.hashListeners)
    , timeViewVariantHashCache() // do not copy the cache! During a render we need a hash that corresponds exactly to the render values.
    , timeViewInvariantCache(0)
    , timeViewInvariantCacheValid(false)
    , metadataSlaveCache(0)
    , metadataSlaveCacheValid(false)
    , hashCacheMutex(QMutex::Recursive)
    {

    }

    bool findCachedHashInternal(const HashableObject::FindHashArgs& args, U64 *hash) const;
};

HashableObject::HashableObject()
: _imp(new HashableObjectPrivate())
{
    
}

HashableObject::~HashableObject()
{

}

HashableObject::HashableObject(const HashableObject& other)
: _imp(new HashableObjectPrivate(*other._imp))
{

}

void
HashableObject::addHashListener(const HashableObjectPtr& parent)
{
    _imp->hashListeners.push_back(parent);
}

bool
HashableObjectPrivate::findCachedHashInternal(const HashableObject::FindHashArgs& args, U64 *hash) const
{
    switch (args.hashType) {
        case HashableObject::eComputeHashTypeTimeViewVariant:
            return findFrameViewHash(args.time, args.view, timeViewVariantHashCache, hash);
        case HashableObject::eComputeHashTypeTimeViewInvariant:
            if (!timeViewInvariantCacheValid) {
                return false;
            }
            *hash = timeViewInvariantCache;
            return true;
        case HashableObject::eComputeHashTypeOnlyMetadataSlaves:
            if (!metadataSlaveCacheValid) {
                return false;
            }
            *hash = metadataSlaveCache;
            return true;

    }
}

bool
HashableObject::findCachedHash(const FindHashArgs& args, U64 *hash) const
{
    QMutexLocker k(&_imp->hashCacheMutex);
    return _imp->findCachedHashInternal(args, hash);
}


void
HashableObject::computeHash_noCache(const ComputeHashArgs& args, Hash64* hash)
{

    // Call the virtual portion to let implementation add extra fields to the hash
    appendToHash(args, hash);
}


U64
HashableObject::computeHash(const ComputeHashArgs& args)
{
    {
        // Find a hash in the cache.
        QMutexLocker k(&_imp->hashCacheMutex);
        U64 hashValue;
        {
            FindHashArgs findArgs;
            findArgs.time = args.time;
            findArgs.view = args.view;
            findArgs.hashType = args.hashType;
            if (_imp->findCachedHashInternal(findArgs, &hashValue)) {
                return hashValue;
            }
        }


        // Compute it
        Hash64 hash;

        // Identity the hash by the hash type in case for some coincendence 2 hash types are equal
        hash.append(args.hashType);
        computeHash_noCache(args, &hash);
        hash.computeHash();
        hashValue = hash.value();

        switch (args.hashType) {
            case eComputeHashTypeTimeViewInvariant:
                _imp->timeViewInvariantCache = hashValue;
                _imp->timeViewInvariantCacheValid = true;
                break;
            case eComputeHashTypeOnlyMetadataSlaves:
                _imp->metadataSlaveCache = hashValue;
                _imp->metadataSlaveCacheValid = true;
                break;
            case eComputeHashTypeTimeViewVariant:
                FrameViewPair fv = {roundImageTimeToEpsilon(args.time), args.view};
                _imp->timeViewVariantHashCache[fv] = hashValue;
                break;
        }

        return hashValue;
        

    }

} // computeHash

bool
HashableObject::invalidateHashCacheInternal(std::set<HashableObject*>* invalidatedObjects)
{
    assert(invalidatedObjects);

    if (invalidatedObjects->find(this) != invalidatedObjects->end()) {
        // Already invalidated
        return false;
    }
    invalidatedObjects->insert(this);

    {
        QMutexLocker k(&_imp->hashCacheMutex);

        // If the cache hash is empty, then all hash listeners must also have their hash empty. 
        if (_imp->timeViewVariantHashCache.empty() && !_imp->timeViewInvariantCacheValid && !_imp->metadataSlaveCacheValid) {
            return false;
        }
        _imp->timeViewVariantHashCache.clear();
        _imp->timeViewInvariantCacheValid = false;
        _imp->metadataSlaveCacheValid = false;
    }
    for (std::list<HashableObjectWPtr>::const_iterator it = _imp->hashListeners.begin(); it != _imp->hashListeners.end(); ++it) {
        HashableObjectPtr listener = it->lock();
        if (listener) {
            listener->invalidateHashCacheInternal(invalidatedObjects);
        }
    }
    return true;
}

void
HashableObject::invalidateHashCache()
{
    std::set<HashableObject*> objs;
    invalidateHashCacheInternal(&objs);
}


NATRON_NAMESPACE_EXIT
