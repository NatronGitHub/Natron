/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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
    // The list of other objects that need this hash as part of their result
    // and the list of dependencies (other hash objects) we need in order to provide our result
    std::set<HashableObjectWPtr> listeners, dependencies;

    // The hash cache
    mutable FrameViewHashMap timeViewVariantHashCache;

    U64 timeViewInvariantCache;

    bool timeViewInvariantCacheValid;

    U64 metadataSlaveCache;

    bool metadataSlaveCacheValid;

    // protects all members
    mutable QMutex hashCacheMutex;

    // Do we allow caching of the hash ?
    bool hashCacheEnabled;

    HashableObjectPrivate()
    : listeners()
    , dependencies()
    , timeViewVariantHashCache()
    , timeViewInvariantCache(0)
    , timeViewInvariantCacheValid(false)
    , metadataSlaveCache(0)
    , metadataSlaveCacheValid(false)
    , hashCacheMutex(QMutex::Recursive) // It might recurse when calling getValue on a knob with an expression because of randomSeed
    , hashCacheEnabled(true)
    {

    }

    HashableObjectPrivate(const HashableObjectPrivate& other)
    : listeners()
    , dependencies()
    , timeViewVariantHashCache()
    , timeViewInvariantCache()
    , timeViewInvariantCacheValid(false)
    , metadataSlaveCache()
    , metadataSlaveCacheValid(false)
    , hashCacheMutex(QMutex::Recursive)
    , hashCacheEnabled(true)
    {
        QMutexLocker k(&other.hashCacheMutex);
        timeViewVariantHashCache = other.timeViewVariantHashCache;
        timeViewInvariantCache = other.timeViewInvariantCache;
        timeViewInvariantCacheValid = other.timeViewInvariantCacheValid;
        metadataSlaveCache = other.metadataSlaveCache;
        metadataSlaveCacheValid = other.metadataSlaveCacheValid;
        hashCacheEnabled = other.hashCacheEnabled;
    }

    bool findCachedHashInternal(const HashableObject::FindHashArgs& args, U64 *hash) const;

    bool computeCachingEnabled() const;

    void computeCachingEnabledRecursive(std::set<HashableObjectPrivate*>* markedObjects);
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
    _imp->listeners.insert(parent);
}

void
HashableObject::removeListener(const HashableObjectPtr& parent)
{
    for (std::set<HashableObjectWPtr>::iterator it = _imp->listeners.begin(); it != _imp->listeners.end(); ++it) {
        if (it->lock() == parent) {
            _imp->listeners.erase(it);
            return;
        }
    }
}

void
HashableObject::addHashDependency(const HashableObjectPtr& dep)
{
    _imp->dependencies.insert(dep);
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
    assert(false);

    return false;
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
        if (_imp->hashCacheEnabled) {
            FindHashArgs findArgs;
            findArgs.time = args.time;
            findArgs.view = args.view;
            findArgs.hashType = args.hashType;
            U64 hashValue;
            if (_imp->findCachedHashInternal(findArgs, &hashValue)) {
                return hashValue;
            }
        }
    }
    {
        // Compute it
        Hash64 hash;

        // Identity the hash by the hash type in case for some coincendence 2 hash types are equal
        hash.append(args.hashType);

        // Unlock the mutex while calling derived implementation
        computeHash_noCache(args, &hash);
        hash.computeHash();
        U64 hashValue = hash.value();


        QMutexLocker k(&_imp->hashCacheMutex);

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

        // Edit: we cannot do this below: The main instance hash cache may be empty because nobody asked to compute the hash on the main thread
        // but some other object that depend on this hash may very well need to have their hash cache cleared.
#if 0
        // If the cache hash is empty, then all hash listeners must also have their hash empty.
        if (_imp->timeViewVariantHashCache.empty() && !_imp->timeViewInvariantCacheValid && !_imp->metadataSlaveCacheValid) {
            return false;
        }
#endif
        _imp->timeViewVariantHashCache.clear();
        _imp->timeViewInvariantCacheValid = false;
        _imp->metadataSlaveCacheValid = false;
    }
    for (std::set<HashableObjectWPtr>::const_iterator it = _imp->listeners.begin(); it != _imp->listeners.end(); ++it) {
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

void
HashableObject::setHashCachingEnabled(bool enabled)
{
    {
        QMutexLocker k(&_imp->hashCacheMutex);
        _imp->hashCacheEnabled = enabled;
    }
    std::set<HashableObjectPrivate*> markedObjects;
    _imp->computeCachingEnabledRecursive(&markedObjects);
}

bool
HashableObjectPrivate::computeCachingEnabled() const
{

    if (!hashCacheEnabled) {
        return false;
    }
    for (std::set<HashableObjectWPtr>::const_iterator it = dependencies.begin(); it != dependencies.end(); ++it) {
        HashableObjectPtr listener = it->lock();
        if (listener) {
            if (!listener->isHashCachingEnabled()) {
                return false;
            }
        }
    }
    return true;
}

void
HashableObjectPrivate::computeCachingEnabledRecursive(std::set<HashableObjectPrivate*>* markedObjects)
{
    if (markedObjects->find(this) != markedObjects->end()) {
        return;
    }
    QMutexLocker k(&hashCacheMutex);

    hashCacheEnabled = computeCachingEnabled();

    for (std::set<HashableObjectWPtr>::const_iterator it = listeners.begin(); it != listeners.end(); ++it) {
        HashableObjectPtr listener = it->lock();
        if (listener) {
            listener->_imp->computeCachingEnabledRecursive(markedObjects);
        }
    }
}

bool
HashableObject::isHashCachingEnabled() const
{
    QMutexLocker k(&_imp->hashCacheMutex);
    return _imp->hashCacheEnabled;

}

NATRON_NAMESPACE_EXIT
