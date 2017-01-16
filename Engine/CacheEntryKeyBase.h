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

#include "Engine/EngineFwd.h"
#include "Engine/IPCCommon.h"
#include "Engine/TimeValue.h"
#include "Engine/ViewIdx.h"

NATRON_NAMESPACE_ENTER;

#define kCacheKeyUniqueIDImageTile 1
#define kCacheKeyUniqueIDGetRoDResults 2
#define kCacheKeyUniqueIDIsIdentityResults 3
#define kCacheKeyUniqueIDFramesNeededResults 4
#define kCacheKeyUniqueIDGetDistorsionResults 5
#define kCacheKeyUniqueIDGetTimeInvariantMetaDatasResults 6
#define kCacheKeyUniqueIDGetComponentsResults 7
#define kCacheKeyUniqueIDGetFrameRangeResults 8
#define kCacheKeyUniqueIDExpressionResult 9



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
     * The function writeMMObject can be used to simplify the serialization of objects to the
     * memory segment.
     **/
    virtual void toMemorySegment(ExternalSegmentType* segment) const;

    /**
     * @brief Reads this key from shared process memory segment.
     * Object names in the segment are the ones written to in toMemorySegment
     * Derived class should call the base class version AFTER their implementation.
     * The function readMMObject can be used to simplify the serialization of objects from the
     * memory segment.
     **/
    virtual void fromMemorySegment(ExternalSegmentType* segment);

    /**
     * @brief This should return exactly the size in bytes of memory taken in the
     * memory segment of the cache used to store the table of content.
     * Derived version should include the size of any element that is written
     * to the memory segment in the toMemorySegment function.
     * Make sure to call the base class version.
     **/
    virtual std::size_t getMetadataSize() const;

    /**
     * @brief Get the hash for this key.
     **/
    U64 getHash() const;

    static std::string hashToString(U64 hash);

    /**
     * @brief Return the plug-in ID to which this cache entry corresponds to.
     **/
    std::string getHolderPluginID() const;
    void setHolderPluginID(const std::string& holderID);

    /**
     * @brief Returns the key time
     **/
    virtual TimeValue getTime() const
    {
        return TimeValue(0.);
    }

    /**
     * @brief Returns the key view
     **/
    virtual ViewIdx getView() const
    {
        return ViewIdx(0);
    }

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

struct ImageTileKeyPrivate;
class ImageTileKey
: public CacheEntryKeyBase
{
public:

    ImageTileKey();

    ImageTileKey(U64 nodeTimeInvariantHash,
                 TimeValue time,
                 ViewIdx view,
                 const std::string& layerChannel,
                 const RenderScale& proxyScale,
                 unsigned int mipMapLevel,
                 bool draftMode,
                 ImageBitDepthEnum bitdepth,
                 int tileX,
                 int tileY);

    virtual ~ImageTileKey();

    U64 getNodeTimeInvariantHashKey() const;

    std::string getLayerChannel() const;

    int getTileX() const;

    int getTileY() const;

    RenderScale getProxyScale() const;

    unsigned int getMipMapLevel() const;

    bool isDraftMode() const;

    ImageBitDepthEnum getBitDepth() const;

    virtual TimeValue getTime() const OVERRIDE FINAL;

    virtual ViewIdx getView() const OVERRIDE FINAL;

    virtual std::size_t getMetadataSize() const OVERRIDE FINAL;

    virtual void toMemorySegment(ExternalSegmentType* segment) const OVERRIDE FINAL;

    virtual void fromMemorySegment(ExternalSegmentType* segment) OVERRIDE FINAL;

    virtual int getUniqueID() const OVERRIDE FINAL;

private:


    virtual void appendToHash(Hash64* hash) const OVERRIDE FINAL;

    boost::scoped_ptr<ImageTileKeyPrivate> _imp;
};

inline
ImageTileKeyPtr toImageTileKey(const CacheEntryKeyBasePtr& key)
{
    return boost::dynamic_pointer_cast<ImageTileKey>(key);
}

NATRON_NAMESPACE_EXIT;

#endif // Engine_CacheEntryKeyBase_h
