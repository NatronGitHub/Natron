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

#ifndef Engine_CacheEntryBase_h
#define Engine_CacheEntryBase_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
#include <boost/scoped_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
#endif

#include "Global/GlobalDefines.h"
#include "Engine/CacheEntryKeyBase.h"
#include "Engine/RectI.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER;

inline int
getSizeOfForBitDepth(ImageBitDepthEnum bitdepth)
{
    switch (bitdepth) {
        case eImageBitDepthByte: {
            return sizeof(unsigned char);
        }
        case eImageBitDepthShort: {
            return sizeof(unsigned short);
        }
        case eImageBitDepthHalf: {
            return sizeof(unsigned short);
        }
        case eImageBitDepthFloat: {
            return sizeof(float);
        }
        case eImageBitDepthNone:
            break;
    }
    
    return 0;
}


/**
 * @brief Base class for any cached item: At the very least each cache entry has a corresponding key from
 * which a 64 bit hash may be computed.
 **/
struct CacheEntryBasePrivate;
class CacheEntryBase : public boost::enable_shared_from_this<CacheEntryBase>
{

public:

    CacheEntryBase(const CachePtr& cache);

    virtual ~CacheEntryBase();

    /**
     * @brief Returns the cache owning this entry.
     **/
    CachePtr getCache() const;

    /**
     * @brief Same as getCache()->get()
     **/
    CacheEntryLockerPtr getFromCache() const;

    /**
     * @brief Get the key object for this entry.
     **/
    CacheEntryKeyBasePtr getKey() const;

    /**
     * @brief Set the key object for this entry.
     * Note that this should be done prior to inserting this entry in the cache.
     **/
    void setKey(const CacheEntryKeyBasePtr& key);

    /**
     * @brief Get the hash key for this entry
     **/
    U64 getHashKey(bool forceComputation = false) const;

    /**
     * @brief This should return exactly the size in bytes of memory taken in the 
     * memory segment of the cache used to store the table of content.
     * The base class version returns the size of the key.
     * Derived version should include the size of any element that is written
     * to the memory segment in the toMemorySegment function.
     * Make sure to call the base class version.
     **/
    virtual std::size_t getMetadataSize() const;

    /**
     * @brief Returns the number of tiles data storage this entry needs. Each tile is exactly the size of NATRON_TILE_SIZE_BYTES.
     * In this case, Natron optimizes the storage of the entry in a tile aligned memory mapped file.
     * If true the toMemorySegment and fromMemorySegment function will have their tileDataPtr set to 
     * a non null value. The implementation should then copy from/to the data exactly NATRON_TILE_SIZE_BYTES bytes.
     **/
    virtual std::size_t getNumTiles() const
    {
        return 0;
    }

    /**
     * @brief Write this key to the process shared memory segment.
     * Each object written to the memory segment must have its handle appended 
     * to the objectPointers list.
     * This is thread-safe and this function is only called by the cache.
     * Derived class should call the base class version AFTER its implementation.
     * The function writeNamedSharedObject can be used to simplify the serialization of objects to the
     * memory segment.
     **/
    virtual void toMemorySegment(ExternalSegmentType* segment, ExternalSegmentTypeHandleList* objectPointers) const;

    /**
     * @brief Reads this key from shared process memory segment.
     * Objects can be retrieved from their process local handle. This handle points
     * to the object copy in shared memory, in the same order that was inserted in the 
     * objectPointers list out of toMemorySegment.
     * Derived class should call the base class version AFTER its implementation.
     * The function readNamedSharedObject can be used to simplify the serialization of objects from the
     * memory segment.
     **/
    virtual void fromMemorySegment(ExternalSegmentType* segment,
                                   ExternalSegmentTypeHandleList::const_iterator start,
                                   ExternalSegmentTypeHandleList::const_iterator end);

    /**
     * @brief Must return whether attempting to call Cache::get() recursively on the same hash is allowed
     * or not for this kind of entry.
     **/
    virtual bool allowMultipleFetchForThread() const
    {
        return false;
    }
private:

    boost::scoped_ptr<CacheEntryBasePrivate> _imp;

};


NATRON_NAMESPACE_EXIT;

#endif // Engine_CacheEntryBase_h
