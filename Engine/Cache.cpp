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
#include "Engine/ThreadPool.h"


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

/**
 * @brief Struct used to prevent multiple threads from computing the same value associated to a hash
 **/
struct LockedHash
{

    // This is the wait condition into which waiters will wait. This is protected by the corresponding bucketLock
    QWaitCondition lockedHashCond;

    // This is the locker that has the status eCacheEntryStatusMustCompute that is locking the entry and supposed to
    // compute it.
    // Protected by bucketLock
    CacheEntryLockerPtr computeLocker;

};


typedef boost::shared_ptr<LockedHash> LockedHashPtr;


typedef std::map<U64, LockedHashPtr> LockedHashMap;


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
    LockedHashMap lockedHashes;

};

struct CacheEntryLockerPrivate
{
    CacheEntryLocker* _publicInterface;

    // A pointer to the cache that returned this object in Cache::get()
    CachePtr cache;

    // The key of the entry
    CacheEntryKeyBasePtr key;

    // A strong reference to the locked object shared amongst CacheEntryLocker that need the value associated
    // to the hash
    LockedHashPtr lockObject;

    // Protects the lockObject pointer
    QMutex lockObjectMutex;

    // Holding a pointer to the bucket is safe since they are statically allocated on the cache.
    CacheBucket* bucket;

    // The entry result
    CacheEntryBasePtr entry;

    CacheEntryLocker::CacheEntryStatusEnum status;

    CacheEntryLockerPrivate(CacheEntryLocker* publicInterface, const CachePtr& cache, const CacheEntryKeyBasePtr& key)
    : _publicInterface(publicInterface)
    , cache(cache)
    , key(key)
    , lockObject()
    , lockObjectMutex()
    , bucket(0)
    , entry()
    , status(CacheEntryLocker::eCacheEntryStatusMustCompute)
    {

    }

    void tryCacheLookup();

    void removeLockObjectIfUnique();

};

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
    , tileSizePo2For8bit(7) // 128*128 by default for 8 bit
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

    static int getBucketCacheBucketIndex(U64 hash)
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


CacheEntryLocker::CacheEntryLocker(const CachePtr& cache, const CacheEntryKeyBasePtr& key)
: _imp(new CacheEntryLockerPrivate(this, cache, key))
{


}

CacheEntryLockerPtr
CacheEntryLocker::create(const CachePtr& cache, const CacheEntryKeyBasePtr& key)
{
    CacheEntryLockerPtr ret(new CacheEntryLocker(cache, key));
    ret->init();
    return ret;
}

void
CacheEntryLockerPrivate::tryCacheLookup()
{
     // Private - should not lock
    assert(!bucket->bucketLock.tryLock());

    U64 keyHash = key->getHash();

    // Check for a matching entry
    CacheIterator foundEntry = bucket->container(keyHash);
    if (foundEntry != bucket->container.end()) {

        CacheEntryBasePtr ret = getValueFromIterator(foundEntry);
        // Ok found one
        // Check if the key is really equal, not just the hash
        if (key->equals(*ret->getKey())) {
            entry = ret;
            status = CacheEntryLocker::eCacheEntryStatusCached;
        }

        // If the key was not equal this may be because 2 hash computations returned the same results.
        // Erase the current entry
        bucket->container.erase(foundEntry);
        foundEntry = bucket->container.end();
    }
    status = CacheEntryLocker::eCacheEntryStatusMustCompute;

} // tryCacheLookup

void
CacheEntryLocker::init()
{
    if (!_imp->key) {
        throw std::runtime_error("CacheEntryLocker::init(): no key");
    }

    U64 hash = _imp->key->getHash();


    // Get the bucket corresponding to the hash. This will dispatch threads in (hopefully) different
    // buckets
    _imp->bucket = &_imp->cache->_imp->buckets[CachePrivate::getBucketCacheBucketIndex(hash)];

    // Lock out threads
    QMutexLocker k(&_imp->bucket->bucketLock);

    _imp->tryCacheLookup();

    if (_imp->status == eCacheEntryStatusCached) {
        // We found in cache, nothing to do
        return;
    }

    assert(_imp->status == eCacheEntryStatusMustCompute);

    LockedHashMap::iterator foundLockedHash = _imp->bucket->lockedHashes.find(hash);
    if (foundLockedHash == _imp->bucket->lockedHashes.end()) {

        // We are the first call to Cache::get() that was made: it is expected that
        // we compute the value
        LockedHashPtr hashLock(new LockedHash);
        _imp->bucket->lockedHashes.insert(std::make_pair(hash, hashLock));
        {
            QMutexLocker k(&_imp->lockObjectMutex);
            _imp->lockObject = hashLock;
        }
        hashLock->computeLocker = shared_from_this();
    } else {

        // There's already somebody computing the hash
        LockedHashPtr &hashLock = foundLockedHash->second;
        assert(hashLock->computeLocker);
        {
            QMutexLocker k(&_imp->lockObjectMutex);
            _imp->lockObject = hashLock;
        }


        _imp->status = eCacheEntryStatusComputationPending;
    }
} // init

CacheEntryKeyBasePtr
CacheEntryLocker::getKey() const
{
    return _imp->key;
}

CacheEntryLocker::CacheEntryStatusEnum
CacheEntryLocker::getStatus() const
{
    return _imp->status;
}


CacheEntryBasePtr
CacheEntryLocker::getCachedEntry() const
{
    return _imp->entry;
}

void
CacheEntryLockerPrivate::removeLockObjectIfUnique()
{
    // Private - should not lock
    assert(!bucket->bucketLock.tryLock());

    U64 hash = key->getHash();

    // Find the lock object, it should still be around
    LockedHashMap::iterator foundLockedHash = bucket->lockedHashes.find(hash);
    assert(foundLockedHash != bucket->lockedHashes.end());

    if (foundLockedHash != bucket->lockedHashes.end()) {

        // If the use count of the lock objet is 1 that means only this thread is still referencing this hash
        // hence remove the lock object
        if (foundLockedHash->second.use_count() == 1) {
            bucket->lockedHashes.erase(foundLockedHash);
        }
    }
}

void
CacheEntryLocker::insertInCache(const CacheEntryBasePtr& value)
{
    // The entry should only be computed and inserted in the cache if the status
    // of the object was eCacheEntryStatusMustCompute
    assert(_imp->status == eCacheEntryStatusMustCompute);
    _imp->status = eCacheEntryStatusCached;

    _imp->entry = value;


    U64 hash = _imp->key->getHash();

    // Set the entry cache bucket index
    _imp->cache->onEntryInsertedInCache(value, CachePrivate::getBucketCacheBucketIndex(hash));

    QMutexLocker locker(&_imp->bucket->bucketLock);

    // The entry must not yet exist in the cache. This is ensured by the locker object that was returned from the get()
    // function.
    assert(_imp->bucket->container(hash) == _imp->bucket->container.end());

    _imp->bucket->container.insert(hash, value);

    assert(_imp->lockObject);
    assert(_imp->lockObject->computeLocker.get() == this);
    _imp->lockObject->computeLocker.reset();


    // Wake up any thread waiting in waitForPendingEntry()
    _imp->lockObject->lockedHashCond.wakeAll();

    // We no longer need to hold a reference to the lock object since we cache it
    {
        QMutexLocker k(&_imp->lockObjectMutex);
        _imp->lockObject.reset();
    }

    // Remove the lock object from the cache if we were the only thread using it
    _imp->removeLockObjectIfUnique();

} // insertInCache

CacheEntryLocker::CacheEntryStatusEnum
CacheEntryLocker::waitForPendingEntry()
{
    // The thread can only wait if the status was set to eCacheEntryStatusComputationPending
    assert(_imp->status == eCacheEntryStatusComputationPending);

    // If this thread is a threadpool thread, it may wait for a while that results gets available.
    // Release the thread to the thread pool so that it may use this thread for other runnables
    // and reserve it back when done waiting.
    bool hasReleasedThread = false;
    if (isRunningInThreadPoolThread()) {
        QThreadPool::globalInstance()->releaseThread();
        hasReleasedThread = true;
    }


    // Lock out threads
    QMutexLocker k(&_imp->bucket->bucketLock);
    do {


        _imp->lockObject->lockedHashCond.wait(&_imp->bucket->bucketLock);

        // We have been woken up by a thread that either called insertInCache() or was in the
        // destructor (hence was aborted).

        // If the original compute thread was aborted, we have to compute now, else we are now
        // also cached

        _imp->tryCacheLookup();

        assert(_imp->status == eCacheEntryStatusMustCompute || _imp->status == eCacheEntryStatusCached);

        if (_imp->status == eCacheEntryStatusMustCompute) {
            assert(_imp->lockObject);
            if (!_imp->lockObject->computeLocker) {
                _imp->lockObject->computeLocker = shared_from_this();
            } else {
                _imp->status = eCacheEntryStatusComputationPending;
            }
        } else {
            // Remove the reference to the lock object
            {
                QMutexLocker k(&_imp->lockObjectMutex);
                _imp->lockObject.reset();
            }
            _imp->removeLockObjectIfUnique();
        }


    } while(_imp->status == eCacheEntryStatusComputationPending);

    if (hasReleasedThread) {
        QThreadPool::globalInstance()->reserveThread();
    }


    return _imp->status;
} // waitForPendingEntry

int
CacheEntryLocker::getNumberOfLockersInterestedByPendingResults() const
{

    QMutexLocker k(&_imp->lockObjectMutex);
    if (!_imp->lockObject) {
        return 0;
    }
    
    assert(_imp->status != eCacheEntryStatusCached);

    int useCount = _imp->lockObject.use_count();

    // There must be at least 2 persons referencing it: the Cache and us
    assert(useCount >= 2);

    // Do not count the cache in the interested items
    return useCount - 1;
}


CacheEntryLocker::~CacheEntryLocker()
{

    // Lock the bucket
    QMutexLocker k(&_imp->bucket->bucketLock);

    // There may still be threads waiting in waitForPendingEntry. If this entry status
    // is set to eCacheEntryStatusMustCompute, this means that the entry was not computed
    // or CacheEntryLocker::insertInCache was not called. We need to wake pending threads
    // and tell them to actually compute
    if (_imp->status == eCacheEntryStatusMustCompute) {
        assert(_imp->lockObject);
        _imp->lockObject->lockedHashCond.wakeAll();
    } else {

        // The cache entry is still pending but this thread is no longer interested in it
        // or it was already cached
        assert(_imp->status == eCacheEntryStatusCached || _imp->status == eCacheEntryStatusComputationPending);
    }

    // Remove the reference to the lock object
    {
        QMutexLocker k(&_imp->lockObjectMutex);
        if (_imp->lockObject) {
            _imp->lockObject->computeLocker.reset();
            _imp->lockObject.reset();
        }
    }

    _imp->removeLockObjectIfUnique();
}


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

void
Cache::getTileSizePx(ImageBitDepthEnum bitdepth, int *tx, int *ty) const
{
    QMutexLocker k(&_imp->tileCacheMutex);
    switch (bitdepth) {
        case eImageBitDepthByte:
            *tx = std::pow(2, _imp->tileSizePo2For8bit);
            *ty = *tx;
            break;
        case eImageBitDepthShort:
        case eImageBitDepthHalf:
            *tx = std::pow(2, _imp->tileSizePo2For8bit);
            *ty  = *tx / 2;
            break;
        case eImageBitDepthFloat:
            *tx = std::pow(2, _imp->tileSizePo2For8bit - 1);
            *ty = *tx;
            break;
        case eImageBitDepthNone:
            *tx = *ty = 0;
            break;
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

#pragma message WARN("Also split the tileCacheMutex across all cache buckets and handle the cache files allocated chunks by bucket")
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

CacheEntryLockerPtr
Cache::get(const CacheEntryKeyBasePtr& key) const
{
    assert(key);
    CachePtr thisShared = boost::const_pointer_cast<Cache>(shared_from_this());
    return CacheEntryLocker::create(thisShared, key);
} // get


void
Cache::onEntryRemovedFromCache(const CacheEntryBasePtr& entry)
{
    entry->setCacheBucketIndex(-1);
}

void
Cache::onEntryInsertedInCache(const CacheEntryBasePtr& entry, int bucketIndex)
{
    entry->setCacheBucketIndex(bucketIndex);

    if (entry->isCacheSignalRequired()) {
        Q_EMIT cacheChanged();
    }
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
                // Note that we don't compare the use count of the entry: if it is used somewhere else it will
                // be evicted from the cache anyway.
                std::pair<U64, CacheEntryBasePtr> evicted = bucket.container.evict(false /*checkUseCount*/);
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
Cache::getAllEntriesByKeyIDWithCacheSignalEnabled(int uniqueID, std::list<CacheEntryBasePtr>* entries) const
{
    for (int bucket_i = 0; bucket_i < NATRON_CACHE_BUCKETS_COUNT; ++bucket_i) {
        CacheBucket& bucket = _imp->buckets[bucket_i];

        QMutexLocker k(&bucket.bucketLock);
        for (CacheIterator it = bucket.container.begin(); it != bucket.container.end(); ++it) {
            CacheEntryBasePtr& entry = getValueFromIterator(it);
            if (entry->isCacheSignalRequired()) {
                if (entry->getKey()->getUniqueID() == uniqueID) {
                    entries->push_back(entry);
                }
            }
        }
    } // for each bucket
} // getAllEntriesByKeyIDWithCacheSignalEnabled

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


        // Set the entry cache bucket index
        int bucket_i = CachePrivate::getBucketCacheBucketIndex((*it)->hash);
        CacheBucket& bucket = _imp->buckets[bucket_i];
        onEntryInsertedInCache(value, bucket_i);

        QMutexLocker locker(&bucket.bucketLock);

        assert(bucket.container((*it)->hash) == bucket.container.end());
        bucket.container.insert((*it)->hash, value);


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



NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_Cache.cpp"
