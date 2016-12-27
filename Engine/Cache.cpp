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

#include "Cache.h"

#include <cassert>
#include <stdexcept>
#include <set>
#include <list>

#include <QMutex>
#include <QDir>
#include <QWaitCondition>
#include <QDebug>

#include <SequenceParsing.h>

#include "Global/GlobalDefines.h"
#include "Global/MemoryInfo.h"
#include "Global/StrUtils.h"
#include "Global/QtCompat.h"

#include "Engine/AppManager.h"
#include "Engine/LRUHashTable.h"
#include "Engine/CacheDeleterThread.h"
#include "Engine/MemoryFile.h"
#include "Engine/StandardPaths.h"


#include "Serialization/CacheEntrySerialization.h"

// The number of buckets. This must be a power of 16 since the buckets will be identified by a digit of a hash
// which is an hexadecimal number.
#define NATRON_CACHE_BUCKETS_N_DIGITS 2
#define NATRON_CACHE_BUCKETS_COUNT 256


// Each cache file on disk that is created by MMAP will have a multiple of the tile size so
// it is the closest to 1GB.
#define NATRON_TILE_CACHE_FILE_SIZE_BYTES 1073741824

// Used to prevent loading older caches when we change the serialization scheme
#define NATRON_CACHE_SERIALIZATION_VERSION 5


NATRON_NAMESPACE_ENTER;

// The 3 defines below configure the internal LRU storage for the cache.
// For now we use
#ifdef USE_VARIADIC_TEMPLATES

    #ifdef NATRON_CACHE_USE_BOOST
        #ifdef NATRON_CACHE_USE_HASH
            typedef BoostLRUHashTable<U64, CacheEntryBasePtr>, boost::bimaps::unordered_set_of > CacheContainer;
        #else
            typedef BoostLRUHashTable<U64, CacheEntryBasePtr >, boost::bimaps::set_of > CacheContainer;
        #endif

        typedef CacheContainer::container_type::left_iterator CacheIterator;
        typedef CacheContainer::container_type::left_const_iterator ConstCacheIterator;
        static CacheEntryBasePtr &  getValueFromIterator(CacheIterator it)
        {
            return it->second;
        }

    #else // cache use STL

        #ifdef NATRON_CACHE_USE_HASH
            typedef StlLRUHashTable<U64, CacheEntryBasePtr >, std::unordered_map > CacheContainer;
        #else
            typedef StlLRUHashTable<U64, CacheEntryBasePtr >, std::map > CacheContainer;
        #endif

        typedef CacheContainer::key_to_value_type::iterator CacheIterator;
        typedef CacheContainer::key_to_value_type::const_iterator ConstCacheIterator;
        static CacheEntryBasePtr& getValueFromIterator(CacheIterator it)
        {
            return it->second;
        }
    #endif // NATRON_CACHE_USE_BOOST

#else // !USE_VARIADIC_TEMPLATES

    #ifdef NATRON_CACHE_USE_BOOST
        typedef BoostLRUHashTable<U64, CacheEntryBasePtr> CacheContainer;
        typedef CacheContainer::container_type::left_iterator CacheIterator;
        typedef CacheContainer::container_type::left_const_iterator ConstCacheIterator;
        static CacheEntryBasePtr& getValueFromIterator(CacheIterator it)
        {
            return it->second;
        }
    #else // cache use STL and tree (std map)

        typedef StlLRUHashTable<U64, CacheEntryBasePtr > CacheContainer;
        typedef CacheContainer::key_to_value_type::iterator CacheIterator;
        typedef CacheContainer::key_to_value_type::const_iterator ConstCacheIterator;
        static CacheEntryBasePtr& getValueFromIterator(CacheIterator it)
        {
            return it->second.first;
        }
    #endif // NATRON_CACHE_USE_BOOST
#endif // USE_VARIADIC_TEMPLATES


typedef std::set<U64> LockedHashSet;


/**
 * @brief The cache is split up into 256 buckets. Each bucket is identified by 2 hexadecimal digits (16*16)
 * This allows concurrent access to the cache without taking a lock: threads are less likely to access to the same bucket.
 * We could also split into 4096 buckets but that's too many data structures to allocate to be worth it.
 **/
struct CacheBucket
{
    // Protects container
    QMutex bucketLock;

    // Internal LRU container
    CacheContainer container;

    // To ensure concurrent threads do not compute concurrently multiple stuff, we lock cache keys
    // of objects that are currently being computed by a thread and that will be inserted later on in the cache.
    // This way, another thread may wait until the computation is done to return the results.
    LockedHashSet lockedHashes;

    // Threads waiting for an entry to be computed wait in this condition. Protected by bucketLock
    QWaitCondition lockedHashesCond;
};

struct CacheEntryLockerPrivate
{
    // The locked hash
    U64 hash;

    // Holding a pointer to the bucket is safe since they are statically allocated on the cache.
    CacheBucket* bucket;

    CacheEntryLockerPrivate(U64 hash, CacheBucket* bucket)
    : hash(hash)
    , bucket(bucket)
    {

    }

};

CacheEntryLocker::CacheEntryLocker(U64 hash, CacheBucket* bucket)
: _imp(new CacheEntryLockerPrivate(hash, bucket))
{

    // When calling this, the mutex should already be taken
    assert(!bucket->bucketLock.tryLock());

}

CacheEntryLocker::~CacheEntryLocker()
{
    // Erase the locked hash
    QMutexLocker k(&_imp->bucket->bucketLock);
    LockedHashSet::iterator foundLockedHash = _imp->bucket->lockedHashes.find(_imp->hash);
    assert(foundLockedHash != _imp->bucket->lockedHashes.end());
    if (foundLockedHash != _imp->bucket->lockedHashes.end()) {
        _imp->bucket->lockedHashes.erase(foundLockedHash);
    }

    // And wake up all other threads
    _imp->bucket->lockedHashesCond.wakeAll();
}

struct CachePrivate
{
    Cache* _publicInterface;

    // maximum size allowed for the cache RAM + MMAP
    std::size_t maximumDiskSize;

    // the maximum size of the in RAM memory in bytes the cache can grow to
    std::size_t maximumInMemorySize;

    // The maximum memory that can be taken by GL textures
    std::size_t maximumGLTextureCacheSize;

    // current size in RAM of the cache in bytes
    std::size_t memoryCacheSize;

    // OpenGL textures memory taken so far in bytes
    std::size_t glTextureCacheSize;

    // current size in MMAP of the cache in bytes
    std::size_t diskCacheSize;

    // protects all size bits
    QMutex sizeLock;

    // All buckets. Each bucket handle entries with the 2 first hexadecimal numbers of the hash
    // This allows to hopefully dispatch threads in 256 different buckets so that they are less likely
    // to take the same lock.
    CacheBucket buckets[NATRON_CACHE_BUCKETS_COUNT];

    // The name of the cache (as it appears on Disk)
    std::string cacheName;

    // Path of the directory that should contain the cache directory itself.
    std::string directoryContainingCachePath;

    // Thread internal to the cache that is used to remove unused entries
    boost::scoped_ptr<CacheCleanerThread> cleanerThread;

    // Store the system physical total RAM in a member
    std::size_t maxPhysicalRAMAttainable;

    // Protected by sizeLock. This is used to wait the thread trying to insert something in the cache whilst the memory is still
    // too high
    QWaitCondition memoryFullCondition;

    // To support tiles, the cache will consist only of a few large files that each contain tiles of the same size.
    // This is useful to cache chunks of data that always have the same size.
    mutable QMutex tileCacheMutex;

    // Each 8 bit tile will have pow(2, tileSizePo2) pixels in each dimension.
    // pow(2, tileSizePo2 - 1) for 16 bit
    // pow(2, tileSizePo2 - 2) for 32 bit
    int tileSizePo2For8bit;

    std::size_t tileByteSize;

    // True when clearing the cache, protected by tileCacheMutex
    bool clearingCache;

    // Storage for tiled entries
    std::list<TileCacheFilePtr> cacheFiles;

    // When set these are used for fast search of a free tile
    TileCacheFileWPtr nextAvailableCacheFile;
    int nextAvailableCacheFileIndex;

    // True when we are about to destroy the cache.
    bool tearingDown;

    CachePrivate(Cache* publicInterface)
    : _publicInterface(publicInterface)
    , maximumDiskSize((std::size_t)10 * 1024 * 1024 * 1024) // 10GB max by default (RAM + Disk)
    , maximumInMemorySize((std::size_t)4 * 1024 * 1024 * 1024) // 4GB in RAM max by default
    , maximumGLTextureCacheSize(0) // This is updated once we get GPU infos
    , memoryCacheSize(0)
    , glTextureCacheSize(0)
    , diskCacheSize(0)
    , sizeLock()
    , buckets()
    , cacheName()
    , directoryContainingCachePath()
    , cleanerThread()
    , maxPhysicalRAMAttainable(0)
    , memoryFullCondition()
    , tileCacheMutex()
    , tileSizePo2For8bit(9) // 512*512 by default for 8 bit
    , tileByteSize(0)
    , clearingCache(false)
    , cacheFiles()
    , nextAvailableCacheFile()
    , nextAvailableCacheFileIndex(-1)
    , tearingDown(false)
    {
        tileByteSize = std::pow(2, tileSizePo2For8bit);
        tileByteSize *= tileByteSize;

        // Make the system RAM appear as 90% of the RAM so we leave some room for other stuff
        maxPhysicalRAMAttainable = getSystemTotalRAM() * 0.9;

    }

    ~CachePrivate()
    {
        cleanerThread->quitThread();
    }

    void initCleanerThread()
    {
        cleanerThread.reset(new CacheCleanerThread(_publicInterface->shared_from_this()));
    }

    std::size_t getTileSizeBytesInternal() const;

    int getBucketCacheBucketIndex(U64 hash) const
    {
        int index = hash >> (64 - NATRON_CACHE_BUCKETS_N_DIGITS * 4);
        assert(index >= 0 && index < NATRON_CACHE_BUCKETS_COUNT);
        return index;
    }

    void ensureCacheDirectoryExists();

    void incrementCacheSize(std::size_t size, StorageModeEnum storage)
    {
        // Private - should not lock
        assert(!sizeLock.tryLock());

        switch (storage) {
            case eStorageModeDisk:
                diskCacheSize += size;
                break;
            case eStorageModeGLTex:
                glTextureCacheSize += size;
                break;
            case eStorageModeRAM:
                memoryCacheSize += size;
                break;
            case eStorageModeNone:
                break;
        }
    }

    void decrementCacheSize(std::size_t size, StorageModeEnum storage)
    {
        // Private - should not lock
        assert(!sizeLock.tryLock());

        switch (storage) {
            case eStorageModeDisk:
                diskCacheSize -= size;
                break;
            case eStorageModeGLTex:
                glTextureCacheSize -= size;
                break;
            case eStorageModeRAM:
                memoryCacheSize -= size;
                break;
            case eStorageModeNone:
                break;
        }
    }

};


Cache::Cache()
: QObject()
, SERIALIZATION_NAMESPACE::SerializableObjectBase()
, boost::enable_shared_from_this<Cache>()
, _imp(new CachePrivate(this))
{
    // Default to a system specific location
    setDirectoryContainingCachePath(std::string());
}

Cache::~Cache()
{

    if (_imp->cleanerThread) {
        _imp->cleanerThread->quitThread();
    }
    _imp->tearingDown = true;

}

CachePtr
Cache::create()
{
    CachePtr ret(new Cache);
    ret->_imp->initCleanerThread();
    return ret;
}

bool
Cache::fileExists(const std::string& filename)
{
#ifdef _WIN32
    WIN32_FIND_DATAW FindFileData;
    std::wstring wpath = StrUtils::utf8_to_utf16 (filename);
    HANDLE handle = FindFirstFileW(wpath.c_str(), &FindFileData);
    if (handle != INVALID_HANDLE_VALUE) {
        FindClose(handle);

        return true;
    }

    return false;
#else
    // on Unix platforms passing in UTF-8 works
    std::ifstream fs( filename.c_str() );

    return fs.is_open() && fs.good();
#endif
}



void
Cache::setMaximumCacheSize(StorageModeEnum storage, std::size_t size)
{
    std::size_t curSize = getMaximumCacheSize(storage);
    {
        QMutexLocker k(&_imp->sizeLock);
        switch (storage) {
            case eStorageModeDisk:
                _imp->maximumDiskSize = size;
                break;
            case eStorageModeGLTex:
                _imp->maximumInMemorySize = size;
                break;
            case eStorageModeRAM: {
                if (size == 0) {
                    size = _imp->maxPhysicalRAMAttainable;
                }
                _imp->maximumGLTextureCacheSize = size;
            }   break;
            case eStorageModeNone:
                assert(false);
                break;
        }
    }

    // Clear exceeding entries if we are shrinking the cache.
    if (size < curSize) {
        evictLRUEntries(0, storage);
    }
}


std::size_t
Cache::getMaximumCacheSize(StorageModeEnum storage) const
{
    {
        QMutexLocker k(&_imp->sizeLock);
        switch (storage) {
            case eStorageModeDisk:
                return _imp->maximumDiskSize;
            case eStorageModeGLTex:
                return _imp->maximumInMemorySize;
            case eStorageModeRAM:
                return _imp->maximumGLTextureCacheSize;
            case eStorageModeNone:
                return 0;
        }
    }
}

std::size_t
Cache::getCurrentSize(StorageModeEnum storage) const
{
    {
        QMutexLocker k(&_imp->sizeLock);
        switch (storage) {
            case eStorageModeDisk:
                return _imp->diskCacheSize;
            case eStorageModeGLTex:
                return _imp->memoryCacheSize;
            case eStorageModeRAM:
                return _imp->glTextureCacheSize;
            case eStorageModeNone:
                return 0;
        }
    }
}

void
Cache::setCacheName(const std::string& name)
{
    _imp->cacheName = name;
}

const std::string &
Cache::getCacheName() const
{
    return _imp->cacheName;
}

void
Cache::setDirectoryContainingCachePath(const std::string& cachePath)
{
    // Check that the user provided path exists otherwise fallback on default.
    bool userDirExists;
    if (cachePath.empty()) {
        userDirExists = false;
    } else {
        QString userDirectoryCache = QString::fromUtf8(cachePath.c_str());
        QDir d(userDirectoryCache);
        userDirExists = d.exists();
    }
    if (userDirExists) {
        _imp->directoryContainingCachePath = cachePath;
    } else {
        _imp->directoryContainingCachePath = StandardPaths::writableLocation(StandardPaths::eStandardLocationCache).toStdString();
    }

    _imp->ensureCacheDirectoryExists();
}

void
CachePrivate::ensureCacheDirectoryExists()
{
    QString userDirectoryCache = QString::fromUtf8(directoryContainingCachePath.c_str());
    QDir d(userDirectoryCache);
    if (d.exists()) {
        QString name = QString::fromUtf8(cacheName.c_str());
        if (d.exists(name)) {
            d.mkdir(name);
        }
    }
} // ensureCacheDirectoryExists

void
Cache::clearAndRecreateCacheDirectory()
{
    clear();
    
    QString userDirectoryCache = QString::fromUtf8(_imp->directoryContainingCachePath.c_str());
    QDir d(userDirectoryCache);
    if (d.exists()) {
        QString cacheName = d.absolutePath() + QLatin1String("/") + QString::fromUtf8(getCacheName().c_str());
        QtCompat::removeRecursively(cacheName);
        d.mkdir(QString::fromUtf8(getCacheName().c_str()));
    }
} // clearAndRecreateCacheDirectory


std::string
Cache::getCacheDirectoryPath() const
{
    QString cacheFolderName;
    cacheFolderName = QString::fromUtf8(_imp->directoryContainingCachePath.c_str());
    StrUtils::ensureLastPathSeparator(cacheFolderName);
    cacheFolderName.append( QString::fromUtf8( getCacheName().c_str() ) );
    return cacheFolderName.toStdString();
} // getCacheDirectoryPath

std::string
Cache::getRestoreFilePath() const
{
    QString newCachePath( QString::fromUtf8(getCacheDirectoryPath().c_str()) );
    StrUtils::ensureLastPathSeparator(newCachePath);

    newCachePath.append( QString::fromUtf8("toc." NATRON_CACHE_FILE_EXT) );

    return newCachePath.toStdString();
}



void
Cache::set8bitTileSizePo2(int tileSizePo2)
{
    {
        QMutexLocker k(&_imp->tileCacheMutex);
        if (_imp->tileSizePo2For8bit == tileSizePo2) {
            return;
        }
    }
    // Clear the cache of entries that don't have the appropriate size
    clear();

    QMutexLocker k(&_imp->tileCacheMutex);
    _imp->tileSizePo2For8bit = tileSizePo2;
    _imp->tileByteSize = std::pow(2, _imp->tileSizePo2For8bit);
    _imp->tileByteSize *= _imp->tileByteSize;

}

int
Cache::get8bitTileSizePo2() const
{
    QMutexLocker k(&_imp->tileCacheMutex);
    return _imp->tileSizePo2For8bit;
}

std::size_t
CachePrivate::getTileSizeBytesInternal() const
{
    // Private - should not lock
    assert(!tileCacheMutex.tryLock());
    std::size_t tileSizePx = std::pow(2, tileSizePo2For8bit);
    // A tile is a mono-channel buffer of NxN pixels
    return tileSizePx * tileSizePx;
} // getTileSizeBytesInternal

std::size_t
Cache::getTileSizeBytes() const
{
    QMutexLocker k(&_imp->tileCacheMutex);
    return _imp->getTileSizeBytesInternal();
}

int
Cache::getTileSizePx(ImageBitDepthEnum bitdepth) const
{
    QMutexLocker k(&_imp->tileCacheMutex);
    switch (bitdepth) {
        case eImageBitDepthByte:
            return std::pow(2, _imp->tileSizePo2For8bit);
        case eImageBitDepthShort:
        case eImageBitDepthHalf:
            return std::pow(2, _imp->tileSizePo2For8bit - 1);
        case eImageBitDepthFloat:
            return std::pow(2, _imp->tileSizePo2For8bit - 2);
        case eImageBitDepthNone:
            return 0;
    }
}

/**
 * @brief Returns a cache size that rounds the cache size to the multiple of the tile size 
 * nearest enclosing of NATRON_TILE_CACHE_FILE_SIZE_BYTES.
 **/
static std::size_t getCacheFileSizeFromTileSize(std::size_t tileSizeInBytes)
{
    return std::ceil(NATRON_TILE_CACHE_FILE_SIZE_BYTES / (double)tileSizeInBytes) * tileSizeInBytes;
}

TileCacheFilePtr
Cache::getTileCacheFile(const std::string& filepath, std::size_t dataOffset)
{
    QMutexLocker k(&_imp->tileCacheMutex);

    // Find an existing cache file
    for (std::list<TileCacheFilePtr>::iterator it = _imp->cacheFiles.begin(); it != _imp->cacheFiles.end(); ++it) {
        if ((*it)->file->path() == filepath) {
            int index = dataOffset / _imp->tileByteSize;

            // The dataOffset should be a multiple of the tile size
            assert(_imp->tileByteSize * index == dataOffset);
            assert(!(*it)->usedTiles[index]);

            // Mark the tile as used
            (*it)->usedTiles[index] = true;
            return *it;
        }
    }

    // Check if the file exists on disk, if not create it.
    if (!fileExists(filepath)) {
        return TileCacheFilePtr();
    } else {

        TileCacheFilePtr ret(new TileCacheFile);
        ret->file.reset(new MemoryFile(filepath, MemoryFile::eFileOpenModeEnumIfExistsKeepElseFail) );
        
        std::size_t nTilesPerFile = std::floor( ( (double)getCacheFileSizeFromTileSize(_imp->tileByteSize)) / _imp->tileByteSize );
        ret->usedTiles.resize(nTilesPerFile, false);
        int index = dataOffset / _imp->tileByteSize;

        // The dataOffset should be a multiple of the tile size
        assert(_imp->tileByteSize * index == dataOffset);
        assert(index >= 0 && index < (int)ret->usedTiles.size());
        assert(!ret->usedTiles[index]);

        // Mark the tile as used
        ret->usedTiles[index] = true;
        _imp->cacheFiles.push_back(ret);
        return ret;

    }

} // getTileCacheFile


TileCacheFilePtr
Cache::allocTile(std::size_t *dataOffset) 
{

    QMutexLocker k(&_imp->tileCacheMutex);

    // First, search for a file with available space.
    // If not found create one
    TileCacheFilePtr foundAvailableFile;
    int foundTileIndex = -1;
    {

        // If we just freed a tile before or allocated a new cache file, the nextAvailableCacheFile and nextAvailableCacheFileIndex are set.
        foundAvailableFile = _imp->nextAvailableCacheFile.lock();
        if (_imp->nextAvailableCacheFileIndex != -1 && foundAvailableFile) {
            foundTileIndex = _imp->nextAvailableCacheFileIndex;
            *dataOffset = foundTileIndex * _imp->tileByteSize;
            _imp->nextAvailableCacheFileIndex = -1;
            _imp->nextAvailableCacheFile.reset();
        } else {
            foundTileIndex = -1;
            foundAvailableFile.reset();
        }
    }
    if (foundTileIndex == -1) {

        // Cycle through each cache file and search for a non allocated tile
        for (std::list<TileCacheFilePtr>::iterator it = _imp->cacheFiles.begin(); it != _imp->cacheFiles.end(); ++it) {
            for (std::size_t i = 0; i < (*it)->usedTiles.size(); ++i) {
                if (!(*it)->usedTiles[i])  {
                    foundTileIndex = i;
                    *dataOffset = i * _imp->tileByteSize;
                    break;
                }
            }
            if (foundTileIndex != -1) {
                foundAvailableFile = *it;
                break;
            }
        }
    }

    if (!foundAvailableFile) {

        // Create a file if all space is taken

        foundAvailableFile.reset(new TileCacheFile());

        std::string cacheFilePath;
        {
            int nCacheFiles = (int)_imp->cacheFiles.size();
            std::stringstream cacheFilePathSs;
            cacheFilePathSs << getCacheDirectoryPath() << "/CachePart" << nCacheFiles;
            cacheFilePath = cacheFilePathSs.str();
        }

        foundAvailableFile->file.reset(new MemoryFile(cacheFilePath, MemoryFile::eFileOpenModeEnumIfExistsKeepElseCreate));
        
        std::size_t cacheFileSize = getCacheFileSizeFromTileSize(_imp->tileByteSize);
        std::size_t nTilesPerFile = std::floor((double)cacheFileSize / _imp->tileByteSize);

        foundAvailableFile->file->resize(cacheFileSize);
        foundAvailableFile->usedTiles.resize(nTilesPerFile, false);
        *dataOffset = 0;
        foundTileIndex = 0;
        _imp->cacheFiles.push_back(foundAvailableFile);

        // Since we just created a file we can safely ensure that the next tile is free if the file contains at least 2 tiles
        if (nTilesPerFile > 1) {
            _imp->nextAvailableCacheFile = foundAvailableFile;
            _imp->nextAvailableCacheFileIndex = 1;
        }
    }

    // Notify the memory file that this portion of the file is valid
    foundAvailableFile->usedTiles[foundTileIndex] = true;

    return foundAvailableFile;

} // allocTile

void
Cache::freeTile(const TileCacheFilePtr& file, std::size_t dataOffset)
{
    QMutexLocker k(&_imp->tileCacheMutex);


    std::list<TileCacheFilePtr>::iterator foundTileFile = std::find(_imp->cacheFiles.begin(), _imp->cacheFiles.end(), file);
    assert(foundTileFile != _imp->cacheFiles.end());
    if (foundTileFile == _imp->cacheFiles.end()) {
        return;
    }

    // The index of the tile can be directly retrieved from the offset in bytes in the file
    int index = dataOffset / _imp->tileByteSize;

    // The dataOffset should be a multiple of the tile size
    assert(_imp->tileByteSize * index == dataOffset);
    assert(index >= 0 && index < (int)(*foundTileFile)->usedTiles.size());
    assert((*foundTileFile)->usedTiles[index]);

    // Mark the tile as free
    (*foundTileFile)->usedTiles[index] = false;

    // If the file does not have any tile associated, remove it
    // A use_count of 2 means that the tile file is only referenced by the cache itself and the entry calling
    // the freeTile() function, hence once its freed, no tile should be using it anymore
    if ((*foundTileFile).use_count() <= 2) {
        // Do not remove the file except if we are clearing the cache
        if (_imp->clearingCache) {
            (*foundTileFile)->file->remove();
            _imp->cacheFiles.erase(foundTileFile);
        } else {
            // Invalidate this portion of the cache
            (*foundTileFile)->file->flush(MemoryFile::eFlushTypeInvalidate, (*foundTileFile)->file->data() + dataOffset, _imp->tileByteSize);
        }
    } else {
        // We just freed this tile, mark it available to speed up the next allocTile call.
        _imp->nextAvailableCacheFile = *foundTileFile;
        _imp->nextAvailableCacheFileIndex = index;
    }
} // freeTile

bool
Cache::get(const CacheEntryKeyBasePtr& key, CacheEntryBasePtr* returnValue, CacheEntryLockerPtr* locker) const
{
    assert(key && returnValue && locker);
    if (!key || !returnValue || !locker) {
        return false;
    }

    // Get the bucket corresponding to the hash. This will dispatch threads in (hopefully) different
    // buckets
    U64 hash = key->getHash();
    CacheBucket& bucket = _imp->buckets[_imp->getBucketCacheBucketIndex(hash)];

    {
        // Lock out threads
        QMutexLocker k(&bucket.bucketLock);

        // Check for a matching entry
        CacheIterator foundEntry = bucket.container(hash);
        if (foundEntry != bucket.container.end()) {

            CacheEntryBasePtr ret = getValueFromIterator(foundEntry);
            // Ok found one
            // Check if the key is really equal, not just the hash
            if (key->equals(*ret->getKey())) {
                *returnValue = ret;
                return true;
            }

            // If the key was not equal this may be because 2 hash computations returned the same results.
            // Erase the current entry
            bucket.container.erase(foundEntry);
            foundEntry = bucket.container.end();
        }

        // Not found. Since we have the lock, all other threads that want to check for the same entry
        // are waiting on the bucketLock.
        // We register the hash in the locked hash set so we ensure that
        // this thread will be the only thread computing the entry

        LockedHashSet::iterator foundLockedHash = bucket.lockedHashes.find(hash);

        // Whilst the hash is still locked by someone else, wait...
        bool hasWaitedOnce = false;
        while (foundLockedHash != bucket.lockedHashes.end()) {

            // This thread will go asleep and release the bucket lock.
            // it will be woken up once the thread locking the hash is done.
            bucket.lockedHashesCond.wait(&bucket.bucketLock);
            hasWaitedOnce = true;
        }

        // Ok now if we waited once, the entry may now be in the cache
        // because the thread that locked the hash is done with it.
        // If the entry is not in the cache but it was released it means that the
        // original thread aborted, in this case let this thread redo the computation.
        // It is likely that this thread gets in turn aborted.
        if (hasWaitedOnce) {
            foundEntry = bucket.container(hash);
            if (foundEntry != bucket.container.end()) {
                // Ok found one, return it
                *returnValue = getValueFromIterator(foundEntry);
                return true;
            }
        }


        // We did not find it in the cache. We must compute it:
        // Ensure that we are the only thread doing so.
        bucket.lockedHashes.insert(hash);
        locker->reset(new CacheEntryLocker(hash, &bucket));
    }

    return false;
} // get

void
Cache::insert(const CacheEntryBasePtr& value, const CacheEntryLockerPtr& locker)
{
    assert(value && locker);
    if (!value || !locker) {
        return;
    }
    insertInternal(value);

} // insert

void
Cache::insertInternal(const CacheEntryBasePtr& value)
{
    // Get the bucket corresponding to the hash. This will dispatch threads in (hopefully) different
    // buckets
    CacheEntryKeyBasePtr key = value->getKey();
    assert(key);
    if (!key) {
        throw std::runtime_error("Cache entry without a key!");
    }
    U64 hash = key->getHash();

    int cacheBucket_i = _imp->getBucketCacheBucketIndex(hash);
    CacheBucket& bucket = _imp->buckets[cacheBucket_i];

    // Set the entry cache bucket index
    onEntryInsertedInCache(value, cacheBucket_i);

    QMutexLocker k(&bucket.bucketLock);

    // The entry must not yet exist in the cache. This is ensured by the locker object that was returned from the get()
    // function.
    assert(bucket.container(hash) == bucket.container.end());

    bucket.container.insert(hash, value);
} // insertInternal

void
Cache::onEntryRemovedFromCache(const CacheEntryBasePtr& entry)
{
    entry->setCacheBucketIndex(-1);
}

void
Cache::onEntryInsertedInCache(const CacheEntryBasePtr& entry, int bucketIndex)
{
    entry->setCacheBucketIndex(bucketIndex);
}

void
Cache::notifyEntryAllocated(std::size_t size, StorageModeEnum storage)
{
    {
        QMutexLocker k(&_imp->sizeLock);
        _imp->incrementCacheSize(size, storage);
    }

    // We just allocateg something, ensure the cache size remains reasonable.
    evictLRUEntries(size, storage);
}


void
Cache::notifyEntryDestroyed(std::size_t size, StorageModeEnum storage)
{
    QMutexLocker k(&_imp->sizeLock);
    _imp->decrementCacheSize(size, storage);
}

void
Cache::removeEntry(const CacheEntryBasePtr& entry)
{
    if (!entry) {
        return;
    }

    int bucketIndex = entry->getCacheBucketIndex();
    if (bucketIndex < 0 || bucketIndex >= NATRON_CACHE_BUCKETS_COUNT) {
        // The bucket index does not correspond to a valid index.
        return;
    }

    CacheBucket& bucket = _imp->buckets[bucketIndex];

    U64 hash = entry->getHashKey();

    bool callCacheChanged = entry->isCacheSignalRequired();
    {
        QMutexLocker k(&bucket.bucketLock);
        onEntryRemovedFromCache(entry);


        CacheIterator found = bucket.container(hash);
        if (found == bucket.container.end()) {
            // The entry does not belong to the cache
            return;
        }
        bucket.container.erase(found);

        // If we are the only user of this entry, delete it
        if (entry.use_count() == 1) {
            std::list<CacheEntryBasePtr> entriesToDelete;
            entriesToDelete.push_back(entry);
            appPTR->deleteCacheEntriesInSeparateThread(entriesToDelete);
        }
    }
    if (callCacheChanged) {
        Q_EMIT cacheChanged();
    }
} // removeEntry


void
Cache::clear()
{
    std::list<CacheEntryBasePtr> entriesToDelete;

    bool callCacheChanged = false;

    for (int bucket_i = 0; bucket_i < NATRON_CACHE_BUCKETS_COUNT; ++bucket_i) {
        CacheBucket& bucket = _imp->buckets[bucket_i];

        QMutexLocker k(&bucket.bucketLock);

        // Copy the bucket container into a temp list and clear the bucket
        std::list<CacheEntryBasePtr> allBucketEntries;
        for (CacheIterator it = bucket.container.begin(); it != bucket.container.end(); ++it) {
            CacheEntryBasePtr entry = getValueFromIterator(it);
            allBucketEntries.push_back(entry);

            onEntryRemovedFromCache(entry);
            if (entry->isCacheSignalRequired()) {
                callCacheChanged = true;
            }
        }
        bucket.container.clear();

        if (allBucketEntries.empty()) {
            continue;
        }

        for (std::list<CacheEntryBasePtr>::const_iterator it = allBucketEntries.begin(); it!=allBucketEntries.end(); ++it) {
            // If we are the last thread with a reference to the entry, delete it in a separate thread.
            if (it->use_count() == 1) {
                entriesToDelete.push_back(*it);
            }
        }
    } // for each bucket

    if (!entriesToDelete.empty()) {
        appPTR->deleteCacheEntriesInSeparateThread(entriesToDelete);
    }
    if (callCacheChanged) {
        Q_EMIT cacheChanged();
    }



} // clear()

void
Cache::evictLRUEntries(std::size_t nBytesToFree, StorageModeEnum storage)
{
    std::size_t maxSize = getMaximumCacheSize(storage);

    // If max size == 0 then there's no limit.
    if (maxSize == 0) {
        return;
    }

    if (nBytesToFree >= maxSize) {
        maxSize = 0;
    } else {
        maxSize = maxSize - nBytesToFree;
    }


    bool callCacheChanged = false;

    std::list<CacheEntryBasePtr> entriesToDelete;

    std::size_t curSize = getCurrentSize(storage);


    bool mustEvictEntries = curSize > maxSize;

    // When allocating on RAM, ensure we do not hit the swap
    // because Natron might not be alone running on the system.
    if (!mustEvictEntries && storage == eStorageModeRAM) {
        std::size_t residentSetBytes = getCurrentRSS();
        mustEvictEntries = residentSetBytes >= _imp->maxPhysicalRAMAttainable;
    }
    while (mustEvictEntries) {
        bool foundBucketThatCanEvict = false;
        // Check each bucket
        for (int bucket_i = 0; bucket_i < NATRON_CACHE_BUCKETS_COUNT; ++bucket_i) {
            CacheBucket& bucket = _imp->buckets[bucket_i];

            CacheEntryBasePtr evictedEntry;
            {
                QMutexLocker k(&bucket.bucketLock);

                // Clear the LRU entry of this bucket.
                // Note that if a thread holds a pointer to it somewhere it won't be evicted
                std::pair<U64, CacheEntryBasePtr> evicted = bucket.container.evict(true /*checkUseCount*/);
                if (!evicted.second) {
                    continue;
                }
                evictedEntry = evicted.second;
            }
            onEntryRemovedFromCache(evictedEntry);

            if (evictedEntry->isCacheSignalRequired()) {
                callCacheChanged = true;
            }

            foundBucketThatCanEvict = true;

            // We evicted one, decrease the size
            curSize -= evictedEntry->getSize();

            // If we are the last thread with a reference to the entry, delete it in a separate thread.
            if (evictedEntry.use_count() == 1) {
                entriesToDelete.push_back(evictedEntry);
            }
            
        } // for each bucket

        // No bucket can be evicted anymore, exit.
        if (!foundBucketThatCanEvict) {
            break;
        }

        // Update mustEvictEntries for next iteration
        mustEvictEntries = curSize > maxSize;

        // When allocating on RAM, ensure we do not hit the swap
        // because Natron might not be alone running on the system.
        if (!mustEvictEntries && storage == eStorageModeRAM) {
            std::size_t residentSetBytes = getCurrentRSS();
            mustEvictEntries = residentSetBytes >= _imp->maxPhysicalRAMAttainable;
        }

    } // curSize > maxSize

    if (!entriesToDelete.empty()) {
        appPTR->deleteCacheEntriesInSeparateThread(entriesToDelete);
    }
    if (callCacheChanged) {
        Q_EMIT cacheChanged();
    }
} // evictLRUEntries

void
Cache::removeAllEntriesForPlugin(const std::string& pluginID,  bool blocking)
{
    if (blocking) {
        removeAllEntriesForPluginBlocking(pluginID);
    } else {
        _imp->cleanerThread->appendToQueue(pluginID);
    }
}

void
Cache::removeAllEntriesForPluginBlocking(const std::string& pluginID)
{
    bool callCacheChanged = false;

    std::list<CacheEntryBasePtr> entriesToDelete;
    for (int bucket_i = 0; bucket_i < NATRON_CACHE_BUCKETS_COUNT; ++bucket_i) {
        CacheBucket& bucket = _imp->buckets[bucket_i];

        CacheContainer newBucketContainer;
        QMutexLocker k(&bucket.bucketLock);
        for (CacheIterator it = bucket.container.begin(); it != bucket.container.end(); ++it) {
            CacheEntryBasePtr& entry = getValueFromIterator(it);
            if (entry->getKey()->getHolderPluginID() != pluginID) {
                newBucketContainer.insert(entry->getHashKey(), entry);
            } else {

                onEntryRemovedFromCache(entry);

                if (entry->isCacheSignalRequired()) {
                    callCacheChanged = true;
                }

                // If we are the only thread referencing this entry, delete it in a separate thread
                if (entry.use_count() == 1) {
                    entriesToDelete.push_back(entry);
                }
            }
        }
        bucket.container = newBucketContainer;
    } // for each bucket

    if (!entriesToDelete.empty()) {
        appPTR->deleteCacheEntriesInSeparateThread(entriesToDelete);
    }
    if (callCacheChanged) {
        Q_EMIT cacheChanged();
    }
} // removeAllEntriesForPluginBlocking

void
Cache::getMemoryStats(std::map<std::string, CacheReportInfo>* infos) const
{

    for (int bucket_i = 0; bucket_i < NATRON_CACHE_BUCKETS_COUNT; ++bucket_i) {
        CacheBucket& bucket = _imp->buckets[bucket_i];

        QMutexLocker k(&bucket.bucketLock);
        for (CacheIterator it = bucket.container.begin(); it != bucket.container.end(); ++it) {
            CacheEntryBasePtr& entry = getValueFromIterator(it);

            std::string plugID = entry->getKey()->getHolderPluginID();
            CacheReportInfo& entryData = (*infos)[plugID];
            ++entryData.nEntries;

            std::size_t entrySize = entry->getSize();

            MemoryBufferedCacheEntryBase* isMemoryEntry = dynamic_cast<MemoryBufferedCacheEntryBase*>(entry.get());
            if (!isMemoryEntry) {
                // A non buffered entry is always ram
                entryData.ramBytes += entrySize;
                continue;
            }


            switch (isMemoryEntry->getStorageMode()) {
                case eStorageModeDisk:
                    entryData.diskBytes += entrySize;
                    break;
                case eStorageModeGLTex:
                    entryData.glTextureBytes += entrySize;
                    break;
                case eStorageModeRAM:
                    entryData.ramBytes += entrySize;
                    break;
                case eStorageModeNone:
                    assert(false);
                    break;
            }
        }
    } // for each bucket
} // getMemoryStats



void
Cache::toSerialization(SERIALIZATION_NAMESPACE::SerializationObjectBase* obj) 
{
    SERIALIZATION_NAMESPACE::CacheSerialization* s = dynamic_cast<SERIALIZATION_NAMESPACE::CacheSerialization*>(obj);
    if (!s) {
        return;
    }

    s->cacheVersion = NATRON_CACHE_SERIALIZATION_VERSION;

        for (int bucket_i = 0; bucket_i < NATRON_CACHE_BUCKETS_COUNT; ++bucket_i) {
            CacheBucket& bucket = _imp->buckets[bucket_i];

            QMutexLocker k(&bucket.bucketLock);
            for (CacheIterator it = bucket.container.begin(); it != bucket.container.end(); ++it) {
                CacheEntryBasePtr& entry = getValueFromIterator(it);

                // Only entries backed with mmap are serialized
                MemoryMappedCacheEntry* mmapEntry = dynamic_cast<MemoryMappedCacheEntry*>(entry.get());
                if (!mmapEntry) {
                    continue;
                }


                // Ensure the file on disk is synced
                mmapEntry->syncBackingFile();
                
                CacheEntryKeyBasePtr keyBase = entry->getKey();

                SERIALIZATION_NAMESPACE::CacheEntrySerializationBasePtr serialization;

                ImageTileKey* isImageKey = dynamic_cast<ImageTileKey*>(keyBase.get());
                if (isImageKey) {
                    SERIALIZATION_NAMESPACE::ImageTileSerializationPtr s(new SERIALIZATION_NAMESPACE::ImageTileSerialization);
                    isImageKey->toSerialization(s.get());
                }


                s->entries.push_back(serialization);


            } // for each bucket entry

        } // for each bucket


} // toSerialization

void
Cache::fromSerialization(const SERIALIZATION_NAMESPACE::SerializationObjectBase& obj)
{
    const SERIALIZATION_NAMESPACE::CacheSerialization* s = dynamic_cast<const SERIALIZATION_NAMESPACE::CacheSerialization*>(&obj);
    if (!s) {
        return;
    }

    CachePtr thisShared = shared_from_this();

    if (s->cacheVersion != NATRON_CACHE_SERIALIZATION_VERSION) {
        clearAndRecreateCacheDirectory();
        return;
    }

    std::set<QString> usedFilePaths;
    for (std::list<SERIALIZATION_NAMESPACE::CacheEntrySerializationBasePtr >::const_iterator it = s->entries.begin(); it != s->entries.end(); ++it) {

        SERIALIZATION_NAMESPACE::ImageTileSerializationPtr isImageTileKeySerialization = boost::dynamic_pointer_cast<SERIALIZATION_NAMESPACE::ImageTileSerialization>(*it);

        CacheEntryKeyBasePtr key;
        ImageBitDepthEnum bitDepth = eImageBitDepthNone;
        if (isImageTileKeySerialization) {
            ImageTileKeyPtr imgKey(new ImageTileKey);
            imgKey->fromSerialization(*isImageTileKeySerialization);
            bitDepth = imgKey->getBitDepth();
            key = imgKey;
        }
        if (!key) {
            continue;
        }

        if ((*it)->hash != key->getHash() ) {
            /*
             * If this warning is printed this means that some of the fields required to compute the hash key
             * were not serialized.
             */
            qDebug() << "WARNING: serialized hash key different than the restored one";
        }


        key->setHolderPluginID((*it)->pluginID);
        
        MemoryMappedCacheEntryPtr value(new MemoryMappedCacheEntry(thisShared));
        value->setKey(key);
        
        MMAPAllocateMemoryArgs allocArgs;
        allocArgs.bitDepth = bitDepth;
        allocArgs.cacheFilePath = (*it)->filePath;
        allocArgs.cacheFileDataOffset = (*it)->dataOffsetInFile;

        try {
            value->allocateMemory(allocArgs);
        } catch (const std::exception& /*e*/) {
            continue;
        }

        usedFilePaths.insert(QString::fromUtf8((*it)->filePath.c_str()));

        insertInternal(value);
    } // for each serialized entry

    // Remove from the cache all files that are not referenced by the table of contents
    QString cachePath = QString::fromUtf8(getCacheDirectoryPath().c_str());
    QDir cacheFolder(cachePath);
    cachePath = cacheFolder.absolutePath();

    QStringList etr = cacheFolder.entryList(QDir::NoDotAndDotDot);

    for (QStringList::iterator it = etr.begin(); it != etr.end(); ++it) {
        QString entryFilePath = cachePath + QLatin1Char('/') + *it;

        std::set<QString>::iterator foundUsed = usedFilePaths.find(entryFilePath);
        if (foundUsed == usedFilePaths.end()) {
            cacheFolder.remove(*it);
        }
    }


} // fromSerialization



struct CacheFetcherPrivate
{
    CacheEntryLockerPtr entryLocker;
    CacheEntryBasePtr entry;

    CacheFetcherPrivate()
    : entryLocker()
    {

    }
};

CacheFetcher::CacheFetcher(const CacheEntryKeyBasePtr& key)
: _imp(new CacheFetcherPrivate())
{
    CachePtr cache = appPTR->getCache();
    bool cached = cache->get(key, &_imp->entry, &_imp->entryLocker);
    (void)cached;
}

CacheEntryBasePtr
CacheFetcher::isCached() const
{
    return _imp->entry;
}

void
CacheFetcher::setEntry(const CacheEntryBasePtr& entry)
{
    // The entry should only be computed if isCached retured NULL.
    assert(!_imp->entry && _imp->entryLocker);
    _imp->entry = entry;
}

CacheFetcher::~CacheFetcher()
{
    // If we created the item, push it to the cache.
    if (_imp->entryLocker && _imp->entry) {
        CachePtr cache = appPTR->getCache();
        cache->insert(_imp->entry, _imp->entryLocker);
    }
}


NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_Cache.cpp"
