//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#ifndef KEYHELPER_H
#define KEYHELPER_H

#include "Engine/Hash64.h"

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
        : _hashComputed(false), _hash()
    {
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
        : _hashComputed(true), _hash( other.getHash() )
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
};


#endif // KEYHELPER_H
