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

#ifndef Engine_CacheEntryKeyBase_h
#define Engine_CacheEntryKeyBase_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <string>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/noncopyable.hpp>

#endif

#include "Global/GlobalDefines.h"

#include "Engine/IPCCommon.h"
#include "Engine/TimeValue.h"
#include "Engine/ViewIdx.h"
#include "Engine/RectI.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

#define kCacheKeyUniqueIDCacheImage 1
#define kCacheKeyUniqueIDGetRoDResults 2
#define kCacheKeyUniqueIDIsIdentityResults 3
#define kCacheKeyUniqueIDFramesNeededResults 4
#define kCacheKeyUniqueIDGetTimeInvariantMetadataResults 5
#define kCacheKeyUniqueIDGetComponentsResults 6
#define kCacheKeyUniqueIDGetFrameRangeResults 7
#define kCacheKeyUniqueIDGetDistortionResults 9



/**
 * @brief The base class for a key of an item in the cache
 **/
struct CacheEntryKeyBasePrivate;
class CacheEntryKeyBase : public boost::noncopyable
{
public:


    CacheEntryKeyBase();
    
    CacheEntryKeyBase(const std::string& pluginID);

    virtual ~CacheEntryKeyBase();

    /**
     * @brief Write this key to the process shared memory segment.
     * This is thread-safe and this function is only called by the cache.
     * Derived class should call the base class version AFTER their implementation.
     * Each member should have a unique name in the segment, prefixed with the hash string.
     * The function writeSharedObject can be used to simplify the serialization of objects to the
     * memory segment.
     **/
    virtual void toMemorySegment(IPCPropertyMap* properties) const;

    enum FromMemorySegmentRetCodeEnum
    {
        // Everything went ok
        eFromMemorySegmentRetCodeOk,

        // Something failed, the entry should be removed from the cache
        eFromMemorySegmentRetCodeFailed,
    };

    /**
     * @brief Reads this key from shared process memory segment.
     * Objects to be deserialized are given from the start iterator to the end.
     * They are to be read in reverse order of what was written to in toMemorySegment.
     **/
    virtual FromMemorySegmentRetCodeEnum fromMemorySegment(const IPCPropertyMap& properties);

    /**
     * @brief This should return an approximate size in bytes of memory taken in the
     * memory segment of the cache used to store the table of content.
     * Implement if your cache entry is going to write a significant amount of memory
     * in the toMemorySegment function, otherwise you may leave to the default 
     * implementation that returns 0. If the cache fails to allocate enough memory
     * for the entry, it will grow the table of content in any cases.
     **/
    virtual std::size_t getMetadataSize() const;

    /**
     * @brief Get the hash for this key.
     **/
    U64 getHash(bool forceComputation = false) const;

    /**
     * @brief Invalidate the hash to force its computation upon the next call of gethash()
     **/
    void invalidateHash();

    static std::string hashToString(U64 hash);

    /**
     * @brief Return the plug-in ID to which this cache entry corresponds to.
     **/
    std::string getHolderPluginID() const;
    void setHolderPluginID(const std::string& holderID);

    
    /**
     * @brief Must return a unique string identifying this class.
     * Imagine 2 key derived class having the same parameters in the same order, the hash
     * produced could be the same but the value associated to the key could be of a different type.
     * To ensure that a hash is unique to an item type, this function must return something unique
     * for each type.
     **/
    virtual int getUniqueID() const = 0;
    
    
protected:

    /**
     * @brief Must append anything that should identify uniquely the cache entry.
     **/
    virtual void appendToHash(Hash64* hash) const = 0;


private:

    boost::scoped_ptr<CacheEntryKeyBasePrivate> _imp;
   
};

NATRON_NAMESPACE_EXIT

#endif // Engine_CacheEntryKeyBase_h
