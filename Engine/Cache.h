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

#ifndef NATRON_ENGINE_ABSTRACTCACHE_H
#define NATRON_ENGINE_ABSTRACTCACHE_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <vector>
#include <sstream>
#include <fstream>
#include <functional>
#include <list>
#include <set>
#include <cstddef>
#include <utility>
#include <algorithm> // min, max

#include "Global/GlobalDefines.h"

GCC_DIAG_OFF(deprecated)
#include <QtCore/QMutex>
#include <QtCore/QThread>
#include <QtCore/QWaitCondition>
#include <QtCore/QMutexLocker>
#include <QtCore/QObject>
#include <QtCore/QBuffer>
#include <QtCore/QRunnable>
GCC_DIAG_ON(deprecated)
#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#endif

#include "Serialization/CacheSerialization.h"

#include "Engine/CacheEntryBase.h"
#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER;


struct CacheReportInfo
{
    int nEntries;
    std::size_t ramBytes, diskBytes, glTextureBytes;

    CacheReportInfo()
    : nEntries(0)
    , ramBytes(0)
    , diskBytes(0)
    , glTextureBytes(0)
    {

    }
};

// This is a cache file with a fixed size that is a multiple of the tileByteSize.
// A bitset represents the allocated tiles in the file.
// A value of true means that a tile is used by a cache entry.
struct TileCacheFile
{
    MemoryFilePtr file;
    std::vector<bool> usedTiles;
};

struct CacheBucket;

/**
 * @brief Small RAII style class used to lock an entry corresponding to a hash key to ensure
 * only a single thread can work on it at once.
 * Mainly this is to avoid multiple threads from computing the same image at once.
 **/
struct CacheEntryLockerPrivate;
class CacheEntryLocker
{
public:

    CacheEntryLocker(U64 hash, CacheBucket* bucket);

    ~CacheEntryLocker();

private:

    boost::scoped_ptr<CacheEntryLockerPrivate> _imp;
};

typedef boost::shared_ptr<CacheEntryLocker> CacheEntryLockerPtr;

struct CachePrivate;
class Cache
: public QObject
, public SERIALIZATION_NAMESPACE::SerializableObjectBase
, public boost::enable_shared_from_this<Cache>
{
    GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
    GCC_DIAG_SUGGEST_OVERRIDE_ON

    friend class CacheDeleterThread;
    friend class MemoryMappedCacheEntry;
    friend class MemoryBufferedCacheEntryBase;
    
private:

    Cache();

public:

    static CachePtr create();
    
    virtual ~Cache();

    /**
     * @brief Set the cache name as it appears on disk and its version (for serialization purposes)
     **/
    void setCacheName(const std::string& name);

    /**
     * @brief Returns the cache name set in setCacheNameAndVersion
     **/
    const std::string & getCacheName() const;

    /**
     * @brief Set the path to the directory that should contain the Cache directory itself. 
     * If empty or an invalid location is provided, the default location is used which is  system dependant.
     **/
    void setDirectoryContainingCachePath(const std::string& cachePath);


    /**
     * @brief Returns the absolute path to the cache directory
     **/
    std::string getCacheDirectoryPath() const;

    /**
     * @brief Returns the absolute file path to the cache table of contents file
     **/
    std::string getRestoreFilePath() const;

    /**
     * @brief Set the size of a 8bit tile in cache. 
     * Each 8-bit tile will have pow(2, tileSizePo2) pixels in each dimension.
     * A 16-bit tile will have pow(2, tileSizePo2 - 1)
     * A 32-bit tile will have pow(2, tileSizePo2 - 2)
     **/
    void set8bitTileSizePo2(int tileSizePo2);
    int get8bitTileSizePo2() const;
    std::size_t getTileSizeBytes() const;

    /**
     * @brief Returns the tile size (of one dimension) in pixels for the given bitdepth/
     **/
    int getTileSizePx(ImageBitDepthEnum bitdepth) const;

    /**
     * @brief Set the maximum cache size available for the given storage.
     * Note that if shrinking, this will clear from the cache exceeding entries.
     **/
    void setMaximumCacheSize(StorageModeEnum storage, std::size_t size);

    /**
     * @brief Returns the maximum cache size for the given storage.
     **/
    std::size_t getMaximumCacheSize(StorageModeEnum storage) const;

    /**
     * @breif Returns the actual size taken in memory for the given storagE.
     **/
    std::size_t getCurrentSize(StorageModeEnum storage) const;

    /**
     * @brief Check if the given file exists on disk
     **/
    static bool fileExists(const std::string& filename);

    /**
     * @brief Look-up the cache for an entry whose key matches the given key.
     * @returnValue This is set in output to the cache entry if it was found.
     * The pointer must be valid otherwise this function always return false.
     * If not found, the cache will lock the hash of
     * the given key to this thread: if other threads try to call the get() function they 
     * will wait until this thread releases the locker. 
     * This way it is possible to let a chance to this thread to create the cache entry and
     * to insert it in the cache so that others don't have to compute it.

     * @returns True if found, false otherwise.
     **/
    bool get(const CacheEntryKeyBasePtr& key, CacheEntryBasePtr* returnValue, CacheEntryLockerPtr* locker) const;

    /**
     * @brief Insert an entry in the cache with the given key. The caller should have first called the get() function
     * which should have returned a locker if the entry was not found. This same locker should be given to the insert functino
     * and should be deleted as soon as this function returns.
     **/
    void insert(const CacheEntryKeyBasePtr& key, CacheEntryBasePtr& value, const CacheEntryLockerPtr& locker);

    /**
     * @brief Clears the cache of its last recently used entries so at least nBytesToFree are available for the given storage.
     * This should be called before allocating any buffer in the application to ensure we do not hit the swap.
     *
     * This function is not blocking and it is not guaranteed that the memory is available when returning. 
     * Evicted entries will be deleted in a separate thread so this thread can continue its own work.
     **/
    void evictLRUEntries(std::size_t nBytesToFree, StorageModeEnum storage);

    /**
     * @brief Clear the cache of entries that can be purged.
     **/
    void clear();

    /**
     * @brief Clear the cache directory and recreates it
     **/
    void clearAndRecreateCacheDirectory();

    /**
     * @brief Removes all cache entries for the given pluginID.
     * @param blocking If true, this function will not return until all entries for the plug-in are removed from the cache,
     * otherwise they are removed from a separate thread.
     **/
    void removeAllEntriesForPlugin(const std::string& pluginID,  bool blocking);

    /**
     * @brief Removes this entry from the cache
     **/
    void removeEntry(const CacheEntryBasePtr& entry);

    /**
     * @brief Returns cache stats for each plug-in
     **/
    void getMemoryStats(std::map<std::string, CacheReportInfo>* infos) const;

    /**
     * @brief Must serialize the cache table of contents
     **/
    virtual void toSerialization(SERIALIZATION_NAMESPACE::SerializationObjectBase* obj) OVERRIDE FINAL;

    /**
     * @brief Restores the cache from the table of contents
     **/
    virtual void fromSerialization(const SERIALIZATION_NAMESPACE::SerializationObjectBase& obj) OVERRIDE FINAL;

Q_SIGNALS:

    /**
     * @brief Emitted whenever an entry that has its isCacheSignalRequired() return true is inserted or
     * removed from the cache.
     **/
    void cacheChanged();
    
private:

    /**
     * @brief Used internally in the implementation of insert()
     **/
    void insertInternal(const CacheEntryKeyBasePtr& key, const CacheEntryBasePtr& value);

    /**
     * @brief Removes all cache entries for the given pluginID.
     * This function will not return until all entries for the plug-in are removed from the cache
     **/
    void removeAllEntriesForPluginBlocking(const std::string& pluginID);


    /**
     * @brief Return the tile cache file associated to the given filePath and mark the tile at the given
     * dataOffset as used (not free).
     * This function is useful when deserializing the cache.
     **/
    TileCacheFilePtr getTileCacheFile(const std::string& filepath, std::size_t dataOffset);

    /**
     * @brief Relevant only for tiled caches. This will allocate the memory required for a tile in the cache and lock it.
     * Note that the calling entry should have exactly the size of a tile in the cache.
     * In return, a pointer to a memory file is returned and the output parameter dataOffset will be set to the offset - in bytes - where the
     * contiguous memory block for this tile begin relative to the start of the data of the memory file.
     * This function may throw exceptions in case of failure.
     * To retrieve the exact pointer of the block of memory for this tile use tileFile->file->data() + dataOffset
     **/
    TileCacheFilePtr allocTile(std::size_t *dataOffset);

    /**
     * @brief Free a tile from the cache that was previously allocated with allocTile. It will be made available again for other entries.
     **/
    void freeTile(const TileCacheFilePtr& file, std::size_t dataOffset);

    /**
     * @brief Called when an entry is removed from the cache. The bucket mutex may still be locked.
     **/
    void onEntryRemovedFromCache(const CacheEntryBasePtr& entry);

    /**
     * @brief Called when an entry is inserted from the cache. The bucket mutex may still be locked.
     **/
    void onEntryInsertedInCache(const CacheEntryBasePtr& entry, int bucketIndex);

    /**
     * @brief Called when an entry is allocated
     **/
    void notifyEntryAllocated(std::size_t size, StorageModeEnum storage);

    /**
     * @brief Called when an entry is deallocated
     **/
    void notifyEntryDestroyed(std::size_t size, StorageModeEnum storage);

private:

    boost::scoped_ptr<CachePrivate> _imp;

};

NATRON_NAMESPACE_EXIT;


#endif /*NATRON_ENGINE_ABSTRACTCACHE_H_ */
