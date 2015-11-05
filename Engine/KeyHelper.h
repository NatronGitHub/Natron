/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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

#ifndef KEYHELPER_H
#define KEYHELPER_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <string>

#include "Engine/Hash64.h"
#include "Engine/CacheEntryHolder.h"
#include "Engine/EngineFwd.h"


/** @brief Helper class that represents a Key in the cache. A key is a set
 * of 1 or more parameters that represent a "unique" element in the cache.
 * To create your own key you must derive this class and provide a HashType.
 * The HashType is a value that will serve as a hash key and it will be computed
 * the first time the getHash() function is called.
 * If you need to notify the cache that the key have changed, call resetHash()
 * and the next call to the getHash() function will compute the hash for you.
 *
 * Thread safety: This function is not thread-safe itself but it is used only
 * by the CacheEntryHelper class which itself is used only by the cache which
 * is thread-safe.
 *
 * Important: In order to compile this class must be boost::serializable, see
 * FrameEntry.h or Image.h for an example on how to serialize a KeyHelper instance.
 *
 * Maybe should we move this class as an internal class of CacheEntryHelper to prevent elsewhere
 * usages.
 **/
template<typename HashType>
class KeyHelper
{
public:
    typedef HashType hash_type;

    /**
     * @brief Constructs an empty key. This constructor is used by boost::serialization.
     **/
    KeyHelper()
        : _hashComputed(false), _hash(), _holderID()
    {
    }

    KeyHelper(const CacheEntryHolder* holder)
    : _hashComputed(false), _hash(), _holderID()
    {
        if (holder) {
            _holderID = holder->getCacheID();
        }
    }

    
    /**
     * @brief Constructs a key from an already existing hash key.
     **/
    // KeyHelper(hash_type hashValue):  _hashComputed(true), _hash(hashValue){}

    /**
     * @brief Constructs a key from an already existing hash key. This is similar than the
     * constructor above but takes in parameter another key.
     **/
    KeyHelper(const KeyHelper & other)
        : _hashComputed(true), _hash( other.getHash() ), _holderID( other.getCacheHolderID() )
    {
    }

    virtual ~KeyHelper()
    {
    }

    hash_type getHash() const
    {
        if (!_hashComputed) {
            computeHashKey();
            _hashComputed = true;
        }

        return _hash;
    }

    void resetHash() const
    {
        _hashComputed = false;
    }
    
    const std::string& getCacheHolderID() const
    {
        return _holderID;
    }


protected:

    /*for now HashType can only be 64 bits...the implementation should
       fill the Hash64 using the append function with the values contained in the
       derived class.*/
    virtual void fillHash(Hash64* hash) const = 0;

private:
    void computeHashKey() const
    {
        Hash64 hash;

        fillHash(&hash);
        hash.computeHash();
        _hash = hash.value();
    }

    mutable bool _hashComputed;
    mutable hash_type _hash;
    
public:
    
    std::string _holderID;

};


#endif // KEYHELPER_H
