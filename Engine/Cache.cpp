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


#include <boost/unordered_set.hpp>
#include <boost/format.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp> // regular mutex
#include <boost/interprocess/sync/scoped_lock.hpp> // scoped lock a regular mutex
#include <boost/interprocess/sync/interprocess_upgradable_mutex.hpp> // r-w mutex that can upgrade read right to write
#include <boost/interprocess/sync/interprocess_sharable_mutex.hpp> // r-w mutex
#include <boost/interprocess/sync/sharable_lock.hpp> // scope lock a r-w mutex
#include <boost/interprocess/sync/upgradable_lock.hpp> // scope lock a r-w upgradable mutex
#include <boost/interprocess/sync/interprocess_condition.hpp> // wait cond
#include <boost/interprocess/file_mapping.hpp>

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


// Each cache file on disk that is created by MMAP will have a multiple of the tile size.
// Each time the cache file has to grow, it will be resized to contain 1000 more tiles
#define NATRON_CACHE_FILE_GROW_N_TILES 1024

// Used to prevent loading older caches when we change the serialization scheme
#define NATRON_CACHE_SERIALIZATION_VERSION 5


NATRON_NAMESPACE_ENTER;




// Typedef our interprocess types
typedef bip::allocator<int, MemorySegmentType::segment_manager> MM_allocator_int;
typedef bip::allocator<char, MemorySegmentType::segment_manager> MM_allocator_char;


// The unordered set of free tiles indices in a bucket
typedef boost::unordered_set<int, boost::hash<int>, std::equal_to<int>, MM_allocator_int> MM_unordered_set_int;

// Our LRU container: internally an allocator for the key and value type will be used
typedef LRUHashTable<U64, CacheEntryBasePtr, MemorySegmentType::segment_manager> CacheContainer;

inline CacheEntryBasePtr& getValueFromIterator(CacheContainer::iterator it)
{
    return it->value;
}

struct CacheBucket;
/**
 * @brief Struct used to prevent multiple threads from computing the same value associated to a hash
 **/
struct LockedHash
{
    // Raw pointer to the bucket, lives in process memory
    CacheBucket* bucket;

    // [IPC] This is the wait condition into which waiters will wait. This is protected by the corresponding bucketLock
    bip::interprocess_condition lockedHashCond;

    // [IPC] Indicates whether at least one CacheEntryLocke has the status eCacheEntryStatusMustCompute that is locking the entry.
    //
    // Protected by bucketLock accross processes.
    bool* computeLocker;


    LockedHash(const std::string& hashString, CacheBucket* bucket);

    ~LockedHash();
};


typedef bip::deleter<LockedHash, MemorySegmentType::segment_manager>  LockedHash_deleter;
typedef bip::shared_ptr<LockedHash, void_allocator_type, LockedHash_deleter> LockedHashPtr;

typedef bip::allocator<LockedHashPtr, MemorySegmentType::segment_manager> MM_allocator_LockedHashPtr;

/**
 * @brief The cache is split up into 256 buckets. Each bucket is identified by 2 hexadecimal digits (16*16)
 * This allows concurrent access to the cache without taking a lock: threads are less likely to access to the same bucket.
 * We could also split into 4096 buckets but that's too many data structures to allocate to be worth it.
 *
 * The cache bucket is implemented only using interprocess safe data structures so that it can be shared across Natron processes.
 **/
struct CacheBucket
{
    // [IPC] Protects container with an upgradable read-write mutex
    bip::interprocess_upgradable_mutex bucketLock;

    // [IPC] Internal LRU container
    CacheContainer container;

    // [IPC] Storage for tiled entries: the size of this file is a multiple of the tile byte size.
    bip::file_mapping tileAlignedFile;

    // [IPC] All-purpose Storage: the size of this file can be anything.
    bip::file_mapping allPurposeFile;

    // [IPC] Indices of the chunks of memory available in the cache file.
    MM_unordered_set_int freeTiles;

    // Raw pointer to the memory segment held by the CachePrivate class.
    // This is ensured to be always valid and lives in process memory.
    MemorySegmentType* memorySegment;

    CacheBucket()
    : bucketLock()
    , container()
    , tileAlignedFile()
    , allPurposeFile()
    , freeTiles()
    , memorySegment(0)
    {

    }
};

struct CacheEntryLockerPrivate
{
    // Raw pointer to the public interface: lives in process memory
    CacheEntryLocker* _publicInterface;

    // A smart pointer to the cache: lives in process memory
    CachePtr cache;

    // The key of the entry: lives in process memory
    CacheEntryKeyBasePtr key;

    // [IPC] A strong reference to the locked object shared amongst CacheEntryLocker (accross processes)
    // that needs the value associated to the given key.
    LockedHashPtr* lockObject;

    // Holding a pointer to the bucket is safe since they are statically allocated on the cache.
    CacheBucket* bucket;

    // The entry result
    CacheEntryBasePtr entry;

    CacheEntryLocker::CacheEntryStatusEnum status;

    CacheEntryLockerPrivate(CacheEntryLocker* publicInterface, const CachePtr& cache, const CacheEntryKeyBasePtr& key)
    : _publicInterface(publicInterface)
    , cache(cache)
    , key(key)
    , lockObject(0)
    , bucket(0)
    , entry()
    , status(CacheEntryLocker::eCacheEntryStatusMustCompute)
    {

    }

    /**
     * @brief Lookup the cache. If found, set the entry member and updates the status to 
     * eCacheEntryStatusCached.
     * If not found, set the status to eCacheEntryStatusMustCompute.
     * In output, if the status is eCacheEntryStatusMustCompute and and iterator other than end()
     * is returned, that means an entry with a matching hash was found but the key did not match.
     * The caller should then remove erase the iterator from the container (taking a write lock first)
     * and then compute the entry.
     *
     * This function assumes that a read locker is taken.
     **/
    CacheContainer::iterator tryCacheLookup();

};

struct CachePrivate
{
    // Raw pointer to the public interface: lives in process memory
    Cache* _publicInterface;

    // The maximum size allowed for the MMAP cache
    // This is local to the process as it does not have to be shared necessarily:
    // if different accross processes then the process with the minimum size will
    // regulate the cache size.
    std::size_t maximumDiskSize;

    // The maximum size of the in RAM memory in bytes the cache can grow to
    // This is local to the process as it does not have to be shared necessarily:
    // if different accross processes then the process with the minimum size will
    // regulate the cache size.
    std::size_t maximumInMemorySize;

    // The maximum memory that can be taken by GL textures
    // This is local to the process as it does not have to be shared necessarily:
    // if different accross processes then the process with the minimum size will
    // regulate the cache size.
    std::size_t maximumGLTextureSize;

    // Protects all maximum*Size values above.
    // Since all they are all in process memory, this mutex
    // only protects against threads.
    QMutex maximumSizesMutex;

    // [IPC] Current size in RAM of the cache in bytes:
    // this is a single std::size_t that lives in shared memory.
    // It is protected by sizeLock
    std::size_t* memorySize;

    // [IPC] OpenGL textures memory taken so far in bytes
    // this is a single std::size_t that lives in shared memory.
    // It is protected by sizeLock
    std::size_t* glTextureSize;

    // [IPC] Current size in MMAP in bytes
    // this is a single std::size_t that lives in shared memory.
    // It is protected by sizeLock
    std::size_t* diskSize;

    // [IPC] Protects memorySize, glTextureSize and diskSize
    bip::interprocess_mutex sizeLock;

    // This is the memory segment where all cache state is written to and shared amongst processes.
    // The scoped_ptr itself lives in memory but the memory segment itself is shared.
    boost::scoped_ptr<MemorySegmentType> segment;

    // Each bucket handle entries with the 2 first hexadecimal numbers of the hash
    // This allows to hopefully dispatch threads and processes in 256 different buckets so that they are less likely
    // to take the same lock.
    // Each bucket is interprocess safe by itself.
    CacheBucket buckets[NATRON_CACHE_BUCKETS_COUNT];

    // Path of the directory that should contain the cache directory itself.
    // This is controled by a Natron setting. By default it points to a standard system dependent
    // location.
    std::string directoryContainingCachePath;

    // Thread internal to the cache that is used to remove unused entries.
    // This is local to the process.
    boost::scoped_ptr<CacheCleanerThread> cleanerThread;

    // Store the system physical total RAM in a member
    std::size_t maxPhysicalRAMAttainable;

    // True when clearing the cache, protected by tileCacheMutex
    bool clearingCache;

    // True when we are about to destroy the cache.
    bool tearingDown;

    CachePrivate(Cache* publicInterface)
    : _publicInterface(publicInterface)
    , maximumDiskSize((std::size_t)10 * 1024 * 1024 * 1024) // 10GB max by default (RAM + Disk)
    , maximumInMemorySize((std::size_t)4 * 1024 * 1024 * 1024) // 4GB in RAM max by default
    , maximumGLTextureSize(0) // This is updated once we get GPU infos
    , maximumSizesMutex()
    , memorySize(0)
    , glTextureSize(0)
    , diskSize(0)
    , sizeLock()
    , segment()
    , buckets()
    , directoryContainingCachePath()
    , cleanerThread()
    , maxPhysicalRAMAttainable(0)
    , clearingCache(false)
    , tearingDown(false)
    {


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


    void ensureCacheDirectoryExists();

    void incrementCacheSize(std::size_t size, StorageModeEnum storage)
    {
        // Private - should not lock
        assert(!sizeLock.try_lock());

        switch (storage) {
            case eStorageModeDisk:
                *diskSize = *diskSize + size;
                break;
            case eStorageModeGLTex:
                *glTextureSize = *glTextureSize + size;
                break;
            case eStorageModeRAM:
                *memorySize = *memorySize + size;
                break;
            case eStorageModeNone:
                break;
        }
    }

    void decrementCacheSize(std::size_t size, StorageModeEnum storage)
    {
        // Private - should not lock
        assert(!sizeLock.try_lock());

        switch (storage) {
            case eStorageModeDisk:
                *diskSize = *diskSize - size;
                break;
            case eStorageModeGLTex:
                *glTextureSize = *glTextureSize - size;
                break;
            case eStorageModeRAM:
                *memorySize = *memorySize - size;
                break;
            case eStorageModeNone:
                break;
        }
    }

    QString getBucketAbsoluteDirPath(int bucketIndex) const;

};

LockedHash::LockedHash(const std::string& hashString, CacheBucket* bucket)
: bucket(bucket)
, lockedHashCond()
, computeLocker(0)
{
    // The bucket should be locked at this point!
    assert(!bucket->bucketLock.try_lock());

    std::string computeLockerName = hashString + "ComputeLocker";
    computeLocker = bucket->memorySegment->construct<bool>(computeLockerName.c_str())(false);
}

LockedHash::~LockedHash()
{
    // The bucket should be locked at this point!
    assert(!bucket->bucketLock.try_lock());

    bucket->memorySegment->deallocate(computeLocker);
}

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

CacheContainer::iterator
CacheEntryLockerPrivate::tryCacheLookup()
{
     // Private - should not lock

    U64 keyHash = key->getHash();

    // Check for a matching entry
    CacheContainer::iterator foundEntry = bucket->container.find(keyHash);

    if (foundEntry != bucket->container.end()) {

        CacheEntryBasePtr& ret = getValueFromIterator(foundEntry);
        // Ok found one
        // Check if the key is really equal, not just the hash
        if (key->equals(*ret->getKey())) {
            entry = ret;
            status = CacheEntryLocker::eCacheEntryStatusCached;
        }
    }
    status = CacheEntryLocker::eCacheEntryStatusMustCompute;
    return foundEntry;

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
    _imp->bucket = &_imp->cache->_imp->buckets[Cache::getBucketCacheBucketIndex(hash)];

    {
        // Take the read lock: many threads/processes can try read at the same time
        bip::sharable_lock<bip::interprocess_upgradable_mutex> lock(_imp->bucket->bucketLock);

        CacheContainer::iterator foundEntry = _imp->tryCacheLookup();

        if (_imp->status == eCacheEntryStatusCached) {
            // We found in cache, nothing to do
            assert(foundEntry != _imp->bucket->container.end());
            Q_UNUSED(foundEntry);
            return;
        }
    }

    assert(_imp->status == eCacheEntryStatusMustCompute);


    // Ok we either did not find a matching hash or we found a matching hash but the key did not match.
    //
    // In any cases, first take an upgradable lock and repeat the find
    // Only a single thread/process can take the upgradable lock.

    bip::upgradable_lock<bip::interprocess_upgradable_mutex> readLock(_imp->bucket->bucketLock);
    CacheContainer::iterator foundEntry = _imp->tryCacheLookup();

    if (_imp->status == eCacheEntryStatusCached) {
        // We found in cache, nothing to do: it was probably cached in between the 2 locks.
        assert(foundEntry != _imp->bucket->container.end());
        return;
    }

    assert(_imp->status == eCacheEntryStatusMustCompute);


    {
        // Ok we either did not find a matching hash or we found a matching hash but the key did not match.
        // We need to upgrade the lock to a write lock. This will wait until all other threads have released their
        // read lock.
        bip::scoped_lock<bip::interprocess_upgradable_mutex> writeLock(boost::move(readLock));
        
        
        // Now we are the only thread in this portion.

        // If we had a hash match but invalid key, erase it now.
        _imp->bucket->container.erase(foundEntry);


        // Instead of taking the write lock until this thread/process has computed the entry we do things differently.
        // If we were to take the lock until the entry was computed, we would lock out the entire bucket even for other
        // objects that may be totally unrelated.
        //
        // Instead we register this CacheEntryLocker in a lockedHashes map. Thus on the bucket, we can
        // track the CacheEntryLocker's interested in the result of the value associated to the hash.
        // We can then wait into a wait condition specific to that hash.
        //
        // If somebody is already computing the entry we mark ourselves with the status eCacheEntryStatusComputationPending
        // and the caller is expected to call waitForPendingEntry() when needed.


        // Create a locker object associated to the provided hash.
        std::string hashStr;
        {
            std::stringstream ss;
            ss << std::hex << hash;
            hashStr = ss.str();
        }
        std::string sharedPtrName = hashStr + "LockedHashPtr";
        {
            std::pair<LockedHashPtr*, MemorySegmentType::size_type> found = _imp->bucket->memorySegment->find<LockedHashPtr>(sharedPtrName.c_str());
            if (found.first) {
                // There's already somebody computing the hash
                // Set the status to pending: caller is expected to call waitForPendingEntry()

                _imp->lockObject = found.first;

                // The compute locker flag should be set to true
                assert(*(*_imp->lockObject)->computeLocker);
                _imp->status = eCacheEntryStatusComputationPending;

            } else {

                // We are the first thread/process to call Cache::get() for this hash: we must create the shared lock object.

                std::string objectName = hashStr + "LockedHash";

                _imp->lockObject = _imp->bucket->memorySegment->construct<LockedHashPtr>(sharedPtrName.c_str())
                ( _imp->bucket->memorySegment->construct<LockedHash>(objectName.c_str())(hashStr, _imp->bucket)      //object to own
                 , void_allocator_type(_imp->bucket->memorySegment->get_segment_manager())  //allocator
                 , LockedHash_deleter(_imp->bucket->memorySegment->get_segment_manager())         //deleter
                 );

                // Set the compute locker flag to true.
                *(*_imp->lockObject)->computeLocker = true;

            }
        }

    } // writeLock

    // Concurrency resumes here!

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
CacheEntryLocker::insertInCache(const CacheEntryBasePtr& value)
{
    // The entry should only be computed and inserted in the cache if the status
    // of the object was eCacheEntryStatusMustCompute
    assert(_imp->status == eCacheEntryStatusMustCompute);
    _imp->status = eCacheEntryStatusCached;

    _imp->entry = value;

    U64 hash = _imp->key->getHash();

    // Set the entry cache bucket index
    value->setCacheBucketIndex(Cache::getBucketCacheBucketIndex(hash));


    {
        // Take a write lock on the bucket
        bip::scoped_lock<bip::interprocess_upgradable_mutex> writeLock(_imp->bucket->bucketLock);

        // The entry must not yet exist in the cache. This is ensured by the locker object that was returned from the init()
        // function.
        assert(_imp->bucket->container.find(hash) == _imp->bucket->container.end());

        // Insert the value in the container
        _imp->bucket->container.insert(hash, value);

        assert(_imp->lockObject && *_imp->lockObject);

        // The computeLocker flag should be set to true
        assert(*(*_imp->lockObject)->computeLocker);

        // Reset it to false
        *(*_imp->lockObject)->computeLocker = false;

        // Wake up any thread waiting in waitForPendingEntry()
        (*_imp->lockObject)->lockedHashCond.notify_all();

        // We no longer need to hold a reference to the lock object since we cache it
        _imp->bucket->memorySegment->deallocate(_imp->lockObject);
        _imp->lockObject = 0;

    } // writeLock

    _imp->cache->onEntryInsertedInCache(value);


    // Concurrency resumes!
    
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

    {
        // Take a write lock on the bucket
        bip::scoped_lock<bip::interprocess_upgradable_mutex> writeLock(_imp->bucket->bucketLock);

        do {
            assert(_imp->lockObject && *_imp->lockObject);
            (*_imp->lockObject)->lockedHashCond.wait(_imp->bucket->bucketLock);

            // We have been woken up by a thread that either called insertInCache() or was in the
            // destructor (i.e: it was aborted and did not inserted result in cache).

            // If the original compute thread was aborted, we have to compute now, otherwise results
            // are available.

            CacheContainer::iterator foundEntry = _imp->tryCacheLookup();

            assert(_imp->status == eCacheEntryStatusMustCompute || _imp->status == eCacheEntryStatusCached);

            if (_imp->status == eCacheEntryStatusCached) {

                // Remove the reference to the lock object
                (*_imp->lockObject).reset();

                // We no longer need to hold a reference to the lock object since we cache it
                _imp->bucket->memorySegment->deallocate(_imp->lockObject);
                _imp->lockObject = 0;

            } else {

                // We found an entry in the cache with the same hash but that did not match our key, re-compute.
                if (foundEntry != _imp->bucket->container.end()) {
                    _imp->bucket->container.erase(foundEntry);
                }

                if (!*(*_imp->lockObject)->computeLocker) {
                    // We got notified and there's no computeLocker set,
                    // set the flag so that we are the thread that is supposed
                    // to compute the entry.
                    *(*_imp->lockObject)->computeLocker = true;
                } else {
                    // Another thread got woken up first, go to sleep again.
                    _imp->status = eCacheEntryStatusComputationPending;
                }
            }

        } while(_imp->status == eCacheEntryStatusComputationPending);
    } // writeLock

    // Concurrency resumes!

    if (hasReleasedThread) {
        QThreadPool::globalInstance()->reserveThread();
    }
    
    
    return _imp->status;
} // waitForPendingEntry

int
CacheEntryLocker::getNumberOfLockersInterestedByPendingResults() const
{
    assert(_imp->status != eCacheEntryStatusCached);

    if (!_imp->lockObject) {
        return 0;
    }
    if (!(*_imp->lockObject)) {
        return 0;
    }

    int useCount = (*_imp->lockObject).use_count();
    assert(useCount >= 1);
    return useCount;
}


CacheEntryLocker::~CacheEntryLocker()
{
    // If no lock object, there's nothing to do.
    if (!_imp->lockObject) {
        return;
    }

    if (!*_imp->lockObject) {
        return;
    }

    // There's still a locker object:
    // We are either in the eCacheEntryStatusComputationPending or eCacheEntryStatusMustCompute status.
    // If we are in compute status, make sure other threads get notified we are no longer computing it.

    // Take the bucket lock

    bip::scoped_lock<bip::interprocess_upgradable_mutex> writeLock(_imp->bucket->bucketLock);
    if (_imp->status == eCacheEntryStatusMustCompute) {
        (*_imp->lockObject)->lockedHashCond.notify_all();
        *(*_imp->lockObject)->computeLocker = false;
    }
    // Drop the reference of the lock object
    _imp->bucket->memorySegment->deallocate(_imp->lockObject);
}


Cache::Cache()
: QObject()
, boost::enable_shared_from_this<Cache>()
, _imp(new CachePrivate(this))
{
    // Default to a system specific location
    setDirectoryContainingCachePath(std::string());

    _imp->segment.reset(new MemorySegmentType(bip::open_or_create, NATRON_CACHE_DIRECTORY_NAME, SIZE));
    for (int i = 0; i < NATRON_CACHE_BUCKETS_COUNT; ++i) {
        _imp->buckets[i].memorySegment = _imp->segment.get();
    }

    // Lock out other processes/threads to retrieve the size parameters
    bip::scoped_lock<bip::interprocess_mutex> sizeLocker(_imp->sizeLock);

    {
        std::pair<std::size_t*, MemorySegmentType::size_type> found = _imp->segment->find<std::size_t>("DiskSize");
        if (found.first) {
            _imp->diskSize = found.first;
        } else {
            _imp->diskSize = _imp->segment->construct<std::size_t>("DiskSize")(0);
        }
    }
    {
        std::pair<std::size_t*, MemorySegmentType::size_type> found = _imp->segment->find<std::size_t>("MemorySize");
        if (found.first) {
            _imp->memorySize = found.first;
        } else {
            _imp->memorySize = _imp->segment->construct<std::size_t>("MemorySize")(0);
        }
    }
    {
        std::pair<std::size_t*, MemorySegmentType::size_type> found = _imp->segment->find<std::size_t>("GLTextureSize");
        if (found.first) {
            _imp->glTextureSize = found.first;
        } else {
            _imp->glTextureSize = _imp->segment->construct<std::size_t>("GLTextureSize")(0);
        }
    }
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


int
Cache::getBucketCacheBucketIndex(U64 hash)
{
    int index = hash >> (64 - NATRON_CACHE_BUCKETS_N_DIGITS * 4);
    assert(index >= 0 && index < NATRON_CACHE_BUCKETS_COUNT);
    return index;
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
        QMutexLocker k(&_imp->maximumSizesMutex);
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
                _imp->maximumGLTextureSize = size;
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
        QMutexLocker k(&_imp->maximumSizesMutex);
        switch (storage) {
            case eStorageModeDisk:
                return _imp->maximumDiskSize;
            case eStorageModeGLTex:
                return _imp->maximumInMemorySize;
            case eStorageModeRAM:
                return _imp->maximumGLTextureSize;
            case eStorageModeNone:
                return 0;
        }
    }
}

std::size_t
Cache::getCurrentSize(StorageModeEnum storage) const
{
    {
        // Lock out all thread/process
        bip::scoped_lock<bip::interprocess_mutex> locker(_imp->sizeLock);
        switch (storage) {
            case eStorageModeDisk:
                return *_imp->diskSize;
            case eStorageModeGLTex:
                return *_imp->memorySize;
            case eStorageModeRAM:
                return *_imp->glTextureSize;
            case eStorageModeNone:
                return 0;
        }
    }
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

static std::string getBucketDirName(int bucketIndex)
{
    std::string dirName;
    {
        std::string formatStr;
        {

            std::stringstream ss;
            ss << "%0";
            ss << NATRON_CACHE_BUCKETS_N_DIGITS;
            ss << "x";
            formatStr = ss.str();
        }

        std::ostringstream oss;
        oss <<  boost::format(formatStr) % bucketIndex;
        dirName = oss.str();
    }
    return dirName;
}

static void createIfNotExistBucketDirs(const QDir& d)
{
    // Create a directory for each bucket with the index as name
    for (int i = 0; i < NATRON_CACHE_BUCKETS_COUNT; ++i) {
        QString qDirName = QString::fromUtf8( getBucketDirName(i).c_str() );
        if (!d.exists(qDirName)) {
            d.mkdir(qDirName);
        }
    }

}

void
CachePrivate::ensureCacheDirectoryExists()
{
    QString userDirectoryCache = QString::fromUtf8(directoryContainingCachePath.c_str());
    QDir d(userDirectoryCache);
    if (d.exists()) {
        QString cacheDirName = QString::fromUtf8(NATRON_CACHE_DIRECTORY_NAME);
        if (!d.exists(cacheDirName)) {
            d.mkdir(cacheDirName);
        }
        d.cd(cacheDirName);
        createIfNotExistBucketDirs(d);

    }
} // ensureCacheDirectoryExists

void
Cache::clearAndRecreateCacheDirectory()
{
    clear();
    
    QString userDirectoryCache = QString::fromUtf8(_imp->directoryContainingCachePath.c_str());
    QDir d(userDirectoryCache);
    if (d.exists()) {
        QString cacheName = QString::fromUtf8(NATRON_CACHE_DIRECTORY_NAME);
        QString absoluteCacheFilePath = d.absolutePath() + QLatin1String("/") + cacheName;
        QtCompat::removeRecursively(absoluteCacheFilePath);

        d.mkdir(cacheName);
        d.cd(cacheName);

        // Create a directory for each bucket with the index as name
        for (int i = 0; i < NATRON_CACHE_BUCKETS_COUNT; ++i) {

            std::string dirName;
            {
                std::string formatStr;
                {

                    std::stringstream ss;
                    ss << "%0"; // fill prefix with 0
                    ss << NATRON_CACHE_BUCKETS_N_DIGITS; // number should be presented with this number of digits
                    ss << "x"; // number should be displayed as hexadecimal
                    formatStr = ss.str();
                }

                std::ostringstream oss;
                oss <<  boost::format(formatStr) % i;
                dirName = oss.str();
            }
            d.mkdir( QString::fromUtf8( dirName.c_str() ) );
        }
    }
} // clearAndRecreateCacheDirectory


std::string
Cache::getCacheDirectoryPath() const
{
    QString cacheFolderName;
    cacheFolderName = QString::fromUtf8(_imp->directoryContainingCachePath.c_str());
    StrUtils::ensureLastPathSeparator(cacheFolderName);
    cacheFolderName.append( QString::fromUtf8(NATRON_CACHE_DIRECTORY_NAME) );
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
Cache::getTileSizePx(ImageBitDepthEnum bitdepth, int *tx, int *ty)
{
    switch (bitdepth) {
        case eImageBitDepthByte:
            *tx = NATRON_TILE_SIZE_X_8_BIT;
            *ty = NATRON_TILE_SIZE_Y_8_BIT;
            break;
        case eImageBitDepthShort:
        case eImageBitDepthHalf:
            *tx = NATRON_TILE_SIZE_X_16_BIT;
            *ty = NATRON_TILE_SIZE_Y_16_BIT;
            break;
        case eImageBitDepthFloat:
            *tx = NATRON_TILE_SIZE_X_32_BIT;
            *ty = NATRON_TILE_SIZE_Y_32_BIT;
            break;
        case eImageBitDepthNone:
            *tx = *ty = 0;
            break;
    }
}

QString
CachePrivate::getBucketAbsoluteDirPath(int bucketIndex) const
{
    QString bucketDirPath;
    bucketDirPath = QString::fromUtf8(directoryContainingCachePath.c_str());
    StrUtils::ensureLastPathSeparator(bucketDirPath);
    bucketDirPath += QString::fromUtf8(NATRON_CACHE_DIRECTORY_NAME);
    StrUtils::ensureLastPathSeparator(bucketDirPath);
    bucketDirPath += QString::fromUtf8(getBucketDirName(bucketIndex).c_str());
    StrUtils::ensureLastPathSeparator(bucketDirPath);
    return bucketDirPath;
}


MemoryFilePtr
Cache::getTileCacheFile(int bucketIndex, const std::string& filename, std::size_t cacheFileChunkIndex)
{
    assert(bucketIndex >= 0 && bucketIndex < NATRON_CACHE_BUCKETS_COUNT);
    if (bucketIndex < 0 && bucketIndex >= NATRON_CACHE_BUCKETS_COUNT) {
        return MemoryFilePtr();
    }

    CacheBucket& bucket = _imp->buckets[bucketIndex];

    // Lock the bucket
    QMutexLocker k(&bucket.bucketLock);

    QString bucketDirPath = _imp->getBucketAbsoluteDirPath(bucketIndex);

    std::string absoluteFileName = bucketDirPath.toStdString() + filename;

    // Check if the file exists on disk, if not fail
    if (!fileExists(absoluteFileName)) {
        return MemoryFilePtr();
    }

    if (!bucket.cacheFile) {
        // If the memory file object was not created, open the mapping now.
        bucket.cacheFile.reset(new MemoryFile(absoluteFileName, MemoryFile::eFileOpenModeEnumIfExistsKeepElseFail));

        // Ensure the file has a multiple of the tile size in bytes
        std::size_t curFileSize = bucket.cacheFile->size();
        if ((curFileSize % NATRON_TILE_SIZE_BYTES) != 0) {

            // Delete the file, it is not valid anyway.
            bucket.cacheFile->remove();
            bucket.cacheFile.reset();

            return MemoryFilePtr();
        }

        // Fill the free tiles list
        std::size_t nTiles = curFileSize / NATRON_TILE_SIZE_BYTES;
        for (std::size_t i = 0; i < nTiles; ++i) {
            bucket.freeTiles.insert(i);
        }
    }

    {
        // Ensure cacheFileChunkIndex is a valid offset in the file.
        std::size_t curFileSize = bucket.cacheFile->size();
        if (cacheFileChunkIndex * NATRON_TILE_SIZE_BYTES > (curFileSize - NATRON_TILE_SIZE_BYTES)) {
            // If the file does not contain this tile, fail
            return MemoryFilePtr();
        }
    }
    // Remove the tile index from the freeTiles set
    boost::unordered_set<int>::const_iterator found = bucket.freeTiles.find(cacheFileChunkIndex);
    assert(found != bucket.freeTiles.end());
    bucket.freeTiles.erase(found);

    return bucket.cacheFile;
} // getTileCacheFile


MemoryFilePtr
Cache::allocTile(int bucketIndex, std::size_t *cacheFileChunkIndex)
{

    assert(bucketIndex >= 0 && bucketIndex < NATRON_CACHE_BUCKETS_COUNT);
    if (bucketIndex < 0 && bucketIndex >= NATRON_CACHE_BUCKETS_COUNT) {
        return MemoryFilePtr();
    }

    CacheBucket& bucket = _imp->buckets[bucketIndex];

    QMutexLocker k(&bucket.bucketLock);

    QString bucketDirPath = _imp->getBucketAbsoluteDirPath(bucketIndex);

    std::string absoluteFileName = bucketDirPath.toStdString() + ".storage";


    if (!bucket.cacheFile) {

        // If a file already exists on disk, remove it because it is untracked
        if (fileExists(absoluteFileName)) {
            ::remove(absoluteFileName.c_str());
        }

        // If the memory file object was not created, open the mapping now.
        bucket.cacheFile.reset(new MemoryFile(absoluteFileName, MemoryFile::eFileOpenModeEnumIfExistsFailElseCreate));
    }

    std::size_t curFileSize = bucket.cacheFile->size();

    if (!foundAvailableFile) {

        // Create a file if all space is taken

        foundAvailableFile.reset(new TileCacheFile());

        std::string cacheFilePath;
        {
            int nCacheFiles = (int)bucket.cacheFiles.size();
            std::stringstream cacheFilePathSs;
            cacheFilePathSs << _imp->getBucketAbsoluteDirPath(bucketIndex).toStdString() << nCacheFiles;
            cacheFilePath = cacheFilePathSs.str();
        }

        foundAvailableFile->file.reset(new MemoryFile(cacheFilePath, MemoryFile::eFileOpenModeEnumIfExistsKeepElseCreate));
        
        std::size_t cacheFileSize = getCacheFileSizeFromTileSize(_imp->tileByteSize);
        std::size_t nTilesPerFile = std::floor((double)cacheFileSize / _imp->tileByteSize);

        foundAvailableFile->file->resize(cacheFileSize);
        foundAvailableFile->usedTiles.resize(nTilesPerFile, false);
        *dataOffset = 0;
        foundTileIndex = 0;
        bucket.cacheFiles.push_back(foundAvailableFile);

        // Since we just created a file we can safely ensure that the next tile is free if the file contains at least 2 tiles
        if (nTilesPerFile > 1) {
            bucket.nextAvailableCacheFile = foundAvailableFile;
            bucket.nextAvailableCacheFileIndex = 1;
        }
    }

    // Notify the memory file that this portion of the file is valid
    foundAvailableFile->usedTiles[foundTileIndex] = true;

    return foundAvailableFile;

} // allocTile

void
Cache::freeTile(int bucketIndex, std::size_t cacheFileChunkIndex)
{
    assert(bucketIndex >= 0 && bucketIndex < NATRON_CACHE_BUCKETS_COUNT);
    if (bucketIndex < 0 && bucketIndex >= NATRON_CACHE_BUCKETS_COUNT) {
        return;
    }

    CacheBucket& bucket = _imp->buckets[bucketIndex];

    QMutexLocker k(&bucket.bucketLock);


    std::list<TileCacheFilePtr>::iterator foundTileFile = std::find(bucket.cacheFiles.begin(), bucket.cacheFiles.end(), file);
    assert(foundTileFile != bucket.cacheFiles.end());
    if (foundTileFile == bucket.cacheFiles.end()) {
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
            bucket.cacheFiles.erase(foundTileFile);
        } else {
            // Invalidate this portion of the cache
            (*foundTileFile)->file->flush(MemoryFile::eFlushTypeInvalidate, (*foundTileFile)->file->data() + dataOffset, _imp->tileByteSize);
        }
    } else {
        // We just freed this tile, mark it available to speed up the next allocTile call.
        bucket.nextAvailableCacheFile = *foundTileFile;
        bucket.nextAvailableCacheFileIndex = index;
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
Cache::onEntryInsertedInCache(const CacheEntryBasePtr& entry)
{

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
    // We cannot block here until the memory stays contained in the user requested memory portion:
    // if we would do so, then it could deadlock: Natron could require more memory than what
    // the user requested to render just one node.
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

        // Clear underlying tile storage files.
        bucket.cacheFiles.clear();
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
                    serialization = s;
                }

                serialization->hash = keyBase->getHash();
                serialization->time = keyBase->getTime();
                serialization->view = keyBase->getView();
                serialization->pluginID = keyBase->getHolderPluginID();
                serialization->dataOffsetInFile = mmapEntry->getOffsetInFile();

                // Do not save the absolute file-path, instead save a relative file-path to the bucket dir.
                QString filePath = QString::fromUtf8(mmapEntry->getCacheFileAbsolutePath().c_str());
                {
                    int foundLastSep = filePath.lastIndexOf(QLatin1Char('/'));
                    if (foundLastSep != -1) {
                        filePath = filePath.mid(foundLastSep + 1);
                    }
                }
                serialization->filePath = filePath.toStdString();

                assert(serialization);

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

        // Set the entry cache bucket index
        int bucket_i = Cache::getBucketCacheBucketIndex((*it)->hash);
        value->setCacheBucketIndex(bucket_i);

        CacheBucket& bucket = _imp->buckets[bucket_i];

        try {
            value->allocateMemory(allocArgs);
        } catch (const std::exception& /*e*/) {
            continue;
        }

        usedFilePaths.insert(QString::fromUtf8((*it)->filePath.c_str()));


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
