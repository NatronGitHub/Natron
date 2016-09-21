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

#ifndef HASHABLEOBJECT_H
#define HASHABLEOBJECT_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#endif

#include "Global/GlobalDefines.h"
#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

/**
 * @brief A HashableObject is an object that's used in the computation of the frame/view hash of a node.
 * This hash is used to identify specific images in the cache. 
 * Each time a hash is computed, the hash is cached against the frame/view as a key. 
 * The invalidate function removes all hashes from the hash cache and invalidates the parent as well recursively.
 **/
struct HashableObjectPrivate;
class HashableObject
{

public:

    HashableObject();

    virtual ~HashableObject();

    /**
     * @brief The parent is another hash object for which this hash object contributes to.
     * This is useful to have sub-hash values that may not change a lot be cached.
     * For instance, a Curve in a Knob might not change a lot hence we cache it's hash
     **/
    HashableObjectPtr getHashParent() const;
    void setHashParent(const HashableObjectPtr& parent);


    /**
     * @brief Look for a hash in the cache. Returns 0 if nothing is found
     **/
    bool findCachedHash(double time, ViewIdx view, U64 *hash) const;

    /**
     * @brief Compute has hash at the given time/view but does not use the hash cache at all.
     **/
    void computeHash_noCache(double time, ViewIdx view, Hash64* hash);

    /**
     * @brief Set the hash for the given frame view in the cache
     **/
    void addHashToCache(double time, ViewIdx view, U64 hashValue);


    /**
     * @brief Compute the hash at the given time/view. This may be cached.
     * Essentially this is the same as performing the 3 following functions calls
     * : findCachedHash, computeHash_noCache, addHashToCache except that everything
     * is protected under the same mutex for guarenteed atomicity.
     **/
    U64 computeHash(double time, ViewIdx view);
    
    /**
     * @brief Invalidate the hash cache and invalidate recursively the parent as well.
     * This should be called after anything that the hash computation relies on has changed.
     * Derived implementations must call the base class version.
     **/
    virtual void invalidateHashCache(bool invalidateParent = true);

protected:

    /**
     * @brief Must be implemented by deriving classes to add to the hash.
     * Call base implementation when finished to let it also append its stuff to the hash.
     * For any big object (such as a Knob) that should contribute to the hash, you can also
     * make them inherit this class so that their own hash is cached as well.
     * When adding a class inheriting HashableObject to the hash, make sure to call
     * computeHash to get the cache working.
     **/
    virtual void appendToHash(double time, ViewIdx view, Hash64* hash) = 0;

private:

    boost::scoped_ptr<HashableObjectPrivate> _imp;
};

NATRON_NAMESPACE_EXIT

#endif // HASHABLEOBJECT_H
