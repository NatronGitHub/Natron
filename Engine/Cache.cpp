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
#include "Engine/CacheDeleterThread.h"
#include "Engine/MemoryFile.h"
#include "Engine/Settings.h"
#include "Engine/StandardPaths.h"
#include "Engine/Timer.h"
#include "Engine/ThreadPool.h"


// The number of buckets. This must be a power of 16 since the buckets will be identified by a digit of a hash
// which is an hexadecimal number.
#define NATRON_CACHE_BUCKETS_N_DIGITS 2
#define NATRON_CACHE_BUCKETS_COUNT 256


// Each cache file on disk that is created by MMAP will have a multiple of the tile size.
// Each time the cache file has to grow, it will be resized to contain 1000 more tiles
#define NATRON_CACHE_FILE_GROW_N_TILES 1024

// Grow the bucket ToC shared memory by 500Kb at once
#define NATRON_CACHE_BUCKET_TOC_FILE_GROW_N_BYTES 524288

// Used to prevent loading older caches when we change the serialization scheme
#define NATRON_CACHE_SERIALIZATION_VERSION 5

namespace bip = boost::interprocess;


NATRON_NAMESPACE_ENTER;




// Typedef our interprocess types
typedef bip::allocator<std::size_t, MemorySegmentType::segment_manager> MM_allocator_size_t;
typedef bip::allocator<char, MemorySegmentType::segment_manager> MM_allocator_char;


// The unordered set of free tiles indices in a bucket
typedef boost::unordered_set<std::size_t, boost::hash<std::size_t>, std::equal_to<std::size_t>, MM_allocator_size_t> MM_unordered_set_size_t;


// Maintain the lru with a list of hash: more recents hash are inserted at the end of the list
// The least recently used hash is the first item of the list.

/**
 * @brief A node of the linked list used to implement the LRU.
 * We need a custom list here, because we want to be able to hold 
 * an offset_ptr of a node directly inside a MemorySegmentEntry
 * for efficiency.
 **/
struct LRUListNode
{
    bip::offset_ptr<LRUListNode> prev, next;
    U64 hash;

    LRUListNode()
    : prev(0)
    , next(0)
    , hash(0)
    {

    }
};

inline
void disconnectLinkedListNode(const bip::offset_ptr<LRUListNode>& node)
{
    // Remove from the LRU linked list:

    // Make the previous item successor point to this item successor
    if (node->prev) {
        node->prev->next = node->next;
    }
    // Make the next item predecessor point to this item predecessor
    if (node->next) {
        node->next->prev = node->prev;
    }
}

inline
void insertLinkedListNode(const bip::offset_ptr<LRUListNode>& node, const bip::offset_ptr<LRUListNode>& prev, const bip::offset_ptr<LRUListNode>& next)
{

    if (prev) {
        prev->next = node;
    }
    node->prev = prev;

    if (next) {
        next->prev = node;
    }
    node->next = next;
}

/**
 * @brief This struct represents one memory segment associated to a hash in the main cache memory segment.
 * This object lives in shared memory
 **/
struct MemorySegmentEntry
{
    // Pointer to the start of the memory segment allocated for this entry.
    // The memory itself is handled with an external memory manager.
    MemorySegmentType::handle_t memorySegmentPortion;

    // The size of the memorySegmentPortion, in bytes. This is stored in the main cache memory segment.
    std::size_t size;

    // Hold an iterator pointing to this entry
    //
    // From http://www.sgi.com/tech/stl/List.html :
    // Lists have the important property that insertion and splicing do not invalidate
    // iterators to list elements, and that even removal invalidates
    // only the iterators that point to the elements that are removed.
    bip::offset_ptr<LRUListNode> lruIterator;

    // Protects the entry to prevent multiple threads from computing the entry.
    bip::interprocess_mutex lock;

    enum EntryStatusEnum
    {
        // The entry is ready (i.e: it was computed) and can be safely retrieved by other processes/threads
        eEntryStatusReady,

        // The entry is in the main memory segment of the cache bucket but has not been computed and not thread
        // is still computing it.
        eEntryStatusNull,

        // The entry is in the main memory segment of the cache bucket but still being computed by a thread
        // The caller should wait in statusCond before it gets ready.
        eEntryStatusPending
    };

    // The status of the entry, protected by the bucket bucketLock
    EntryStatusEnum status;

    MemorySegmentEntry()
    : memorySegmentPortion(0)
    , size(0)
    , lruIterator(0)
    , lock()
    , status(eEntryStatusNull)
    {

    }
};

/**
 * @brief The cache is split up into 256 buckets. Each bucket is identified by 2 hexadecimal digits (16*16)
 * This allows concurrent access to the cache without taking a lock: threads are less likely to access to the same bucket.
 * We could also split into 4096 buckets but that's too many data structures to allocate to be worth it.
 *
 * The cache bucket is implemented using interprocess safe data structures so that it can be shared across Natron processes.
 **/
struct CacheBucket
{

    /**
     * @brief All IPC data that are shared accross processes for this bucket. This object lives in shared memory.
     **/
    struct IPCData
    {

        // Indices of the chunks of memory available in the tileAligned memory-mapped file.
        MM_unordered_set_size_t freeTiles;

        // Protects the LRU list. This is separate to the bucketLock because even if we just access
        // the cache in read mode (in the get() function) we still need to update the LRU list, thus
        // protect it from being written by multiple concurrent threads.
        bip::interprocess_mutex lruListMutex;

        // Pointers in shared memory to the lru list from node and back node
        bip::offset_ptr<LRUListNode> lruListFront, lruListBack;


        IPCData()
        : freeTiles()
        , lruListMutex()
        , lruListFront(0)
        , lruListBack(0)
        {

        }
    };


    // Storage for tiled entries: the size of this file is a multiple of the tile byte size.
    // Any access to the file should be protected by the validTileMappingMutex mutex located in
    // CachePrivate::IPCData::PerBucketData
    MemoryFilePtr tileAlignedFile;

    // Raw pointer to the memory mapped file used to store interprocess table of contents (IPCData)
    // It contains for each entry:
    // - A LRUListNode
    // - A MemorySegmentEntry
    // - A memory buffer of arbitrary size
    // Any access to the file should be protected by the validToCMappingMutex mutex located in
    // CachePrivate::IPCData::PerBucketData
    MemoryFilePtr tocFile;

    // A memory manager of the tocFile
    boost::shared_ptr<ExternalSegmentType> tocFileManager;

    // [IPC] Pointer to the IPC data that live in shared memory (memorySegment)
    IPCData *ipc;

    // Weak pointer to the cache
    CacheWPtr cache;

    // The index of this bucket in the cache
    int bucketIndex;

    CacheBucket()
    : tileAlignedFile()
    , tocFile()
    , tocFileManager()
    , ipc(0)
    , cache()
    , bucketIndex(-1)
    {

    }


    /**
     * @brief Deallocates the cacheEntry from the ToC memory mapped file.
     **/
    void deallocateCacheEntryImpl(MemorySegmentEntry* entry, bool releaseLock);

    /**
     * @brief Lookup the cache for a MemorySegmentEntry matching the hash key.
     * If found, the cacheEntry member will be set.
     * This function assumes that the bucketLock of the bucket is taken at least in read mode.
     **/
    MemorySegmentEntry* tryCacheLookupImpl(const std::string& hashStr);

    /**
     * @brief Reads the cacheEntry into the processLocalEntry.
     * This function updates the status member.
     * This function assumes that the bucketLock of the bucket is taken at least in read mode.
     * @returns True if ok, false if the MemorySegmentEntry cannot be read properly.
     * it should be deallocated from the segment.
     **/
    bool readFromSharedMemoryEntryImpl(MemorySegmentEntry* entry,
                                       const CacheEntryBasePtr& processLocalEntry,
                                       CacheEntryLocker::CacheEntryStatusEnum* status);

    /**
     * @brief Ensures that the ToC memory mapped file mapping is still valid and re-open it if not.
     * The validToCMappingMutex is assumed to be taken for read-lock
     **/
    bool isToCFileMappingValid() const;
    void ensureToCFileMappingValid();

    /**
     * @brief Ensures that the tiled memory mapped file mapping is still valid and re-open it if not.
     * The validTileMappingMutex is assumed to be taken for read-lock
     **/
    bool isTileFileMappingValid() const;
    void ensureTileMappingValid();

    /**
     * @brief Grow the ToC memory mapped file. This will lock out other process/threads until 
     * the file is resized. The validToCMapping will be cleared and every process is supposed to
     * re-open the mapping using the ensureToCFileMappingValid() function.
     * The validTileMappingMutex is assumed to be taken for write lock
     **/
    void growToCFile();

    /**
     * @brief Grow the tile memory mapped file. This will lock out other process/threads until
     * the file is resized. The validTileMapping will be cleared and every process is supposed to
     * re-open the mapping using the ensureTileMappingValid() function.
     * The validTileMappingMutex is assumed to be taken for write lock
     **/
    void growTileFile();
};

struct CacheEntryLockerPrivate
{
    // Raw pointer to the public interface: lives in process memory
    CacheEntryLocker* _publicInterface;

    // A smart pointer to the cache: lives in process memory
    CachePtr cache;

    // A pointer to the entry to retrieve: lives in process memory
    CacheEntryBasePtr processLocalEntry;

    // Holding a pointer to the bucket is safe since they are statically allocated on the cache.
    CacheBucket* bucket;

    // The status of the entry, @see CacheEntryStatusEnum
    CacheEntryLocker::CacheEntryStatusEnum status;

    // A string version of the hash, uniquely identifying the MemorySegmentEntry in the memory mapped file
    std::string hashStr;

    CacheEntryLockerPrivate(CacheEntryLocker* publicInterface, const CachePtr& cache, const CacheEntryBasePtr& entry)
    : _publicInterface(publicInterface)
    , cache(cache)
    , processLocalEntry(entry)
    , bucket(0)
    , status(CacheEntryLocker::eCacheEntryStatusMustCompute)
    , hashStr()
    {

        U64 hash = entry->getHashKey();
        hashStr = CacheEntryKeyBase::hashToString(hash);
    }


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

    // Each bucket handle entries with the 2 first hexadecimal numbers of the hash
    // This allows to hopefully dispatch threads and processes in 256 different buckets so that they are less likely
    // to take the same lock.
    // Each bucket is interprocess safe by itself.
    CacheBucket buckets[NATRON_CACHE_BUCKETS_COUNT];

    struct IPCData
    {
        struct PerBucketData
        {
            // Locks the cache bucket's toc memory mapped file so we can grow/shrink it as needed.
            // Whenever a process/thread access a bucket memory segment, it takes the lock in read mode.
            // Whenever a process/thread needs to grow or shrink the memory segment, it takes this lock
            // in write mode.
            bip::interprocess_upgradable_mutex validToCMappingMutex;

            // Stores the PID of all processes that have a valid ToC. Whenever resizeing the ToC file,
            // this set is cleared. If a processes wishes to access a ToC file and it is not in this set,
            // that means it needs to re-map the file in memory.
            bip::set<long long> validToCMapping;

            // Locks the cache bucket's tile segment so we can grow/shrink it as needed.
            bip::interprocess_upgradable_mutex validTileMappingMutex;

            // Same as validToCMapping but indicates valid mapping of the tileAlignedFile
            bip::set<long long> validTileMapping;
        };
        

        PerBucketData bucketsData[NATRON_CACHE_BUCKETS_COUNT];


        // Current size in RAM of the cache in bytes:
        // this is a single std::size_t that lives in shared memory.
        // It is protected by sizeLock
        std::size_t memorySize;

        // OpenGL textures memory taken so far in bytes
        // this is a single std::size_t that lives in shared memory.
        // It is protected by sizeLock
        std::size_t glTextureSize;

        // Current size in MMAP in bytes
        // this is a single std::size_t that lives in shared memory.
        // It is protected by sizeLock
        std::size_t diskSize;

        // Protects memorySize, glTextureSize and diskSize
        bip::interprocess_sharable_mutex sizeLock;

        IPCData()
        : bucketsData()
        , memorySize(0)
        , glTextureSize(0)
        , diskSize(0)
        , sizeLock()
        {

        }

    };

    // Raw pointer to the memory segment used to store bucket independent data accross processes.
    // This is ensured to be always valid and lives in process memory.
    // The global memory segment is of a fixed size and only contains one instance of CachePrivate::IPCData.
    // This is the only shared memory segment that we know the size: it never grows.
    boost::scoped_ptr<bip::managed_shared_memory> globalMemorySegment;

    IPCData* ipc;

    // Path of the directory that should contain the cache directory itself.
    // This is controled by a Natron setting. By default it points to a standard system dependent
    // location.
    std::string directoryContainingCachePath;

    // Thread internal to the cache that is used to remove unused entries.
    // This is local to the process.
    boost::scoped_ptr<CacheCleanerThread> cleanerThread;

    // Store the system physical total RAM in a member
    std::size_t maxPhysicalRAMAttainable;

    // A unique ID identifying the process
    long long pid;

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
    , buckets()
    , globalMemorySegment()
    , ipc(0)
    , directoryContainingCachePath()
    , cleanerThread()
    , maxPhysicalRAMAttainable(0)
    , pid(0)
    , clearingCache(false)
    , tearingDown(false)
    {


        // Make the system RAM appear as 90% of the RAM so we leave some room for other stuff
        maxPhysicalRAMAttainable = getSystemTotalRAM() * 0.9;

        timeval now;
        gettimeofday(&now, 0);

        pid = (long long)(now.tv_sec + now.tv_usec * 1e-6f);

    }

    ~CachePrivate()
    {
        cleanerThread->quitThread();
    }

    void initCleanerThread()
    {
        cleanerThread.reset(new CacheCleanerThread(_publicInterface->shared_from_this()));
    }

    void initializeCacheDirPath();

    void ensureCacheDirectoryExists();

    void incrementCacheSize(long long size, StorageModeEnum storage);

    QString getBucketAbsoluteDirPath(int bucketIndex) const;

};

CacheEntryLocker::CacheEntryLocker(const CachePtr& cache, const CacheEntryBasePtr& entry)
: _imp(new CacheEntryLockerPrivate(this, cache, entry))
{

}

CacheEntryLockerPtr
CacheEntryLocker::create(const CachePtr& cache, const CacheEntryBasePtr& entry)
{
    assert(entry);
    if (!entry) {
        throw std::invalid_argument("CacheEntryLocker::create: no entry");
    }
    CacheEntryLockerPtr ret(new CacheEntryLocker(cache, entry));
    ret->lookupAndSetStatus(false /*takeEntryLock*/);
    return ret;
}

bool
CacheBucket::isToCFileMappingValid() const
{
    // Private - the validToCMappingMutex is assumed to be taken for read lock
    // Find this PID in the set.
    CachePtr c = cache.lock();
    bip::set<long long>::iterator foundInValidSet = c->_imp->ipc->bucketsData[bucketIndex].validToCMapping.find(c->_imp->pid);
    return foundInValidSet != c->_imp->ipc->bucketsData[bucketIndex].validToCMapping.end();

} // isToCFileMappingValid

bool
CacheBucket::isTileFileMappingValid() const
{
    // Private - the validTileMappingMutex is assumed to be taken for read lock
    // Find this PID in the set.
    CachePtr c = cache.lock();
    bip::set<long long>::iterator foundInValidSet = c->_imp->ipc->bucketsData[bucketIndex].validTileMapping.find(c->_imp->pid);
    return foundInValidSet != c->_imp->ipc->bucketsData[bucketIndex].validTileMapping.end();
} // isTileFileMappingValid

void
CacheBucket::ensureToCFileMappingValid()
{
    // Private - the validToCMappingMutex is assumed to be taken for write lock
    CachePtr c = cache.lock();

    // Find this PID in the set.

    bip::set<long long>::iterator foundInValidSet = c->_imp->ipc->bucketsData[bucketIndex].validToCMapping.find(c->_imp->pid);
    if (foundInValidSet == c->_imp->ipc->bucketsData[bucketIndex].validToCMapping.end()) {

        // Not found, the mapping is invalid, we must re-open it.
        // We upgrade the validToCMappingMutex to a write lock.
        bip::scoped_lock<bip::interprocess_upgradable_mutex> writeLock(c->_imp->ipc->bucketsData[bucketIndex].validToCMappingMutex);
        tocFile->remap();

        c->_imp->ipc->bucketsData[bucketIndex].validToCMapping.insert(c->_imp->pid);

        // Re-create the manager on the new mapped buffer
        tocFileManager.reset(new ExternalSegmentType(bip::open_only, tocFile->data(), tocFile->size()));

        // The ipc data pointer must be re-fetched
        ipc = tocFileManager->find_or_construct<CacheBucket::IPCData>("BucketData")();
    }
} // ensureToCFileMappingValid

void
CacheBucket::ensureTileMappingValid()
{
    // Private - the validTileMappingMutex is assumed to be taken for write lock
    CachePtr c = cache.lock();

    // Find this PID in the set.

    bip::set<long long>::iterator foundInValidSet = c->_imp->ipc->bucketsData[bucketIndex].validTileMapping.find(c->_imp->pid);
    if (foundInValidSet == c->_imp->ipc->bucketsData[bucketIndex].validTileMapping.end()) {

        // Not found, the mapping is invalid, we must re-open it.
        // We upgrade the validTileMappingMutex to a write lock.
        bip::scoped_lock<bip::interprocess_upgradable_mutex> writeLock(c->_imp->ipc->bucketsData[bucketIndex].validTileMappingMutex);
        tileAlignedFile->remap();

        c->_imp->ipc->bucketsData[bucketIndex].validTileMapping.insert(c->_imp->pid);

    }

} // ensureTileMappingValid

void
CacheBucket::growToCFile()
{
    // Private - the validToCMappingMutex is assumed to be taken for write lock

    CachePtr c = cache.lock();
    tocFile->resize(tocFile->size() + NATRON_CACHE_BUCKET_TOC_FILE_GROW_N_BYTES, true /*preserve*/);

    // Invalidate the mapping of all processes
    c->_imp->ipc->bucketsData[bucketIndex].validToCMapping.clear();

    // Re-open the mapping for this process
    ensureToCFileMappingValid();

} // growToCFile

void
CacheBucket::growTileFile()
{
    // Private - the validTileMappingMutex is assumed to be taken for write lock

    CachePtr c = cache.lock();

    std::size_t curSize = tileAlignedFile->size();
    // The current size must be a multiple of the tile size
    assert(curSize % NATRON_TILE_SIZE_BYTES == 0);

    std::size_t newSize = curSize + NATRON_CACHE_FILE_GROW_N_TILES * NATRON_TILE_SIZE_BYTES;
    tileAlignedFile->resize(newSize, true /*preserve*/);

    // Invalidate the mapping of all processes
    c->_imp->ipc->bucketsData[bucketIndex].validTileMapping.clear();

    int nTilesSoFar = curSize / NATRON_TILE_SIZE_BYTES;
    int newNTiles = newSize / NATRON_TILE_SIZE_BYTES;

    // Insert the new available tiles in the freeTiles set
    for (int i = nTilesSoFar; i < newNTiles; ++i) {
        ipc->freeTiles.insert((std::size_t)i);
    }

    // Re-open the mapping for this process
    ensureTileMappingValid();


} // growTileFile

MemorySegmentEntry*
CacheBucket::tryCacheLookupImpl(const std::string& hashStr)
{
    // Private - the bucket lock is assumed to be taken at least in read lock mode
    std::pair<MemorySegmentEntry*, MemorySegmentType::size_type> found = tocFileManager->find<MemorySegmentEntry>(hashStr.c_str());
    return found.first;
} // tryCacheLookupImpl

bool
CacheBucket::readFromSharedMemoryEntryImpl(MemorySegmentEntry* cacheEntry,
                                           const CacheEntryBasePtr& processLocalEntry,
                                           CacheEntryLocker::CacheEntryStatusEnum* status)
{
    // Private - the bucket lock is assumed to be taken at least in read lock mode

    // The entry must have been looked up in tryCacheLookup()
    assert(cacheEntry);

    // By default we are in mustComputeMode
    *status = CacheEntryLocker::eCacheEntryStatusMustCompute;

    if (cacheEntry->status == MemorySegmentEntry::eEntryStatusNull) {
        // We should never be in this situation:
        // The thread that created this cacheEntry should have marked the eEntryStatusPending
        // and if it failed to compute it should have removed it from memory.
        assert(false);
        return false;
    } else if (cacheEntry->status == MemorySegmentEntry::eEntryStatusPending) {
        *status = CacheEntryLocker::eCacheEntryStatusComputationPending;
        return true;
    }


    assert(cacheEntry->status == MemorySegmentEntry::eEntryStatusReady);

    // The shared pointer should always exist as long as the entry is still in the memory segment.
    assert(cacheEntry->memorySegmentPortion);

    bool mustRemoveEntry = false;
    if (!cacheEntry->memorySegmentPortion) {
        mustRemoveEntry = true;
    }

    void* bufPtr = 0;
    if (!mustRemoveEntry) {
        // Wrap the memory segment portion with a managed external buffer object.
        // This may throw an exception if the buffer is not large enough.
        bufPtr = tocFileManager->get_address_from_handle(cacheEntry->memorySegmentPortion);
        try {
            ExternalSegmentType managedMemoryPortion(bip::open_only,
                                                     bufPtr,
                                                     cacheEntry->size);

            // Deserialize the entry. This may throw an exception if it cannot be deserialized properly
            processLocalEntry->fromMemorySegment(managedMemoryPortion);

            // Deserialization went ok, set the status
            *status = CacheEntryLocker::eCacheEntryStatusCached;

            // Update LRU record if this item is not already at the tail of the list
            //
            // Take the LRU list mutex
            {
                bip::scoped_lock<bip::interprocess_mutex> lruWriteLock(ipc->lruListMutex);

                assert(ipc->lruListBack && !ipc->lruListBack->next);
                if (ipc->lruListBack != cacheEntry->lruIterator) {

                    disconnectLinkedListNode(cacheEntry->lruIterator);

                    // And push_back to the tail of the list...
                    insertLinkedListNode(cacheEntry->lruIterator, ipc->lruListBack, bip::offset_ptr<LRUListNode>(0));
                    ipc->lruListBack = cacheEntry->lruIterator;
                }
            } // lruWriteLock

        } catch (...) {
            mustRemoveEntry = true;
        }
    } // !mustRemoveEntry
    
    return !mustRemoveEntry;

} // readFromSharedMemoryEntryImpl

void
CacheBucket::deallocateCacheEntryImpl(MemorySegmentEntry* cacheEntry, bool releaseLock)
{
    // The bucketLock must be taken in write mode

    // Does not throw any exception
    if (cacheEntry->memorySegmentPortion) {
        void* bufPtr = tocFileManager->get_address_from_handle(cacheEntry->memorySegmentPortion);
        if (bufPtr) {
            tocFileManager->deallocate(bufPtr);
        }
    }

    if (cacheEntry->lruIterator) {

        // Remove this entry from the LRU list
        {
            bip::scoped_lock<bip::interprocess_mutex> lruWriteLock(ipc->lruListMutex);
            if (cacheEntry->lruIterator == ipc->lruListBack) {
                ipc->lruListBack = cacheEntry->lruIterator->next ? cacheEntry->lruIterator->next  : cacheEntry->lruIterator->prev;
            } else if (cacheEntry->lruIterator == ipc->lruListFront) {
                ipc->lruListFront = cacheEntry->lruIterator->next;
            }
            disconnectLinkedListNode(cacheEntry->lruIterator);
        }
        cacheEntry->lruIterator = 0;
    }

    if (releaseLock) {
        cacheEntry->lock.unlock();
    }

    // deallocate the entry
    tocFileManager->deallocate(cacheEntry);
} // deallocateCacheEntryImpl


void
CacheEntryLocker::lookupAndSetStatus(bool takeEntryLock)
{

    // Get the bucket corresponding to the hash. This will dispatch threads in (hopefully) different
    // buckets
    if (!_imp->bucket) {
        U64 hash = _imp->processLocalEntry->getHashKey();
        _imp->bucket = &_imp->cache->_imp->buckets[Cache::getBucketCacheBucketIndex(hash)];
    }

    {
        // Take the read lock: many threads/processes can try read at the same time
        boost::scoped_ptr<bip::sharable_lock<bip::interprocess_upgradable_mutex> > readLock;
        readLock.reset(new bip::sharable_lock<bip::interprocess_upgradable_mutex>(_imp->cache->_imp->ipc->bucketsData[_imp->bucket->bucketIndex].validToCMappingMutex));

        boost::scoped_ptr<bip::scoped_lock<bip::interprocess_upgradable_mutex> > writeLock;
        // Every time we take the lock, we must ensure the memory mapping is ok
        if (!_imp->bucket->isToCFileMappingValid()) {
            // Remove the read lock, and take a write lock.
            // This could allow other threads to run in-between, but we don't care since nothing happen.
            readLock.reset();
            writeLock.reset(new bip::scoped_lock<bip::interprocess_upgradable_mutex>(_imp->cache->_imp->ipc->bucketsData[_imp->bucket->bucketIndex].validToCMappingMutex));
            _imp->bucket->ensureToCFileMappingValid();
        }

        // Look-up the cache
        MemorySegmentEntry* cacheEntry = _imp->bucket->tryCacheLookupImpl(_imp->hashStr);

        // Flag set to true if readFromSharedMemoryEntry failed
        bool deserializeFailed = false;

        if (cacheEntry) {
            if (takeEntryLock) {
                cacheEntry->lock.lock();
            }

            // Deserialize the entry and update the status
            deserializeFailed = !_imp->bucket->readFromSharedMemoryEntryImpl(cacheEntry, _imp->processLocalEntry, &_imp->status);
        }

        if (_imp->status == eCacheEntryStatusCached) {
            assert(!deserializeFailed);
            // We found in cache, nothing to do
            return;
        }
    }

    // Concurrency resumes!

    assert(_imp->status == eCacheEntryStatusMustCompute ||
           _imp->status == eCacheEntryStatusComputationPending);


    // Ok we are in one of those 3 cases
    // - we did not find an entry matching the hash
    // - we found a matching entry but it is pending
    // - we found a matching entry but the object was invalid
    //
    // In any cases, first take an upgradable lock and repeat the look-up.
    // Only a single thread/process can take the upgradable lock.
    {
        bip::upgradable_lock<bip::interprocess_upgradable_mutex> readLock(_imp->cache->_imp->ipc->bucketsData[_imp->bucket->bucketIndex].validToCMappingMutex);

        boost::scoped_ptr< bip::scoped_lock<bip::interprocess_upgradable_mutex> > writeLock;

        // Every time we take the lock, we must ensure the memory mapping is ok
        if (!_imp->bucket->isToCFileMappingValid()) {
            writeLock.reset(new bip::scoped_lock<bip::interprocess_upgradable_mutex>(boost::move(readLock)));
            _imp->bucket->ensureToCFileMappingValid();
        }


        // Look-up again: a thread could have placed the entry in the cache in-between the two locks
        MemorySegmentEntry* cacheEntry = _imp->bucket->tryCacheLookupImpl(_imp->hashStr);


        // Flag set to true if readFromSharedMemoryEntry failed
        bool deserializeFailed = false;

        if (cacheEntry) {
            deserializeFailed = !_imp->bucket->readFromSharedMemoryEntryImpl(cacheEntry, _imp->processLocalEntry, &_imp->status);
        }

        if (_imp->status == eCacheEntryStatusCached) {
            assert(!deserializeFailed);
            // We found in cache, nothing to do: it was probably cached in between the 2 locks.
            return;
        }

        assert(_imp->status == eCacheEntryStatusMustCompute ||
               _imp->status == eCacheEntryStatusComputationPending);


        // If somebody is already computing the entry the caller is expected to call waitForPendingEntry() when needed.
        // This allows this thread to launch extra computation before waiting.
        if (_imp->status == eCacheEntryStatusComputationPending) {
            assert(!deserializeFailed);
            return;
        }


        {
            // We need to upgrade the lock to a write lock. This will wait until all other threads have released their
            // read lock.
            if (!writeLock) {
                writeLock.reset(new bip::scoped_lock<bip::interprocess_upgradable_mutex>(boost::move(readLock)));
            }

            // Now we are the only thread in this portion.

            // If the deserialization failed, deallocate the memory taken by the entry
            if (deserializeFailed) {
                _imp->bucket->deallocateCacheEntryImpl(cacheEntry, takeEntryLock /*releaseLock*/);
                cacheEntry = 0;
                deserializeFailed = false;
            }

            assert(!cacheEntry);



            // Create the MemorySegmentEntry if it does not exist
            cacheEntry = _imp->bucket->tocFileManager->construct<MemorySegmentEntry>(_imp->hashStr.c_str())();


            // Lock the statusMutex: this will lock-out other threads interested in this entry.
            // This mutex is unlocked in the destructor or in insertInCache()
            cacheEntry->lock.lock();


            assert(cacheEntry->status == MemorySegmentEntry::eEntryStatusNull);

            // Set the status of the entry to pending because we (this thread) are going to compute it.
            // Other fields of the entry will be set once it is done computed in insertInCache()
            cacheEntry->status = MemorySegmentEntry::eEntryStatusPending;

            
        } // writeLock
    } // upgradableLock
    // Concurrency resumes here!

} // lookupAndSetStatus

CacheEntryLocker::CacheEntryStatusEnum
CacheEntryLocker::getStatus() const
{
    return _imp->status;
}

void
CacheEntryLocker::insertInCache()
{
    // The entry should only be computed and inserted in the cache if the status
    // of the object was eCacheEntryStatusMustCompute
    assert(_imp->status == eCacheEntryStatusMustCompute);

    {
        // Take write lock on the bucket
        bip::scoped_lock<bip::interprocess_upgradable_mutex> writeLock(_imp->cache->_imp->ipc->bucketsData[_imp->bucket->bucketIndex].validToCMappingMutex);

        // Ensure the memory mapping is ok
        _imp->bucket->ensureToCFileMappingValid();


        // Fetch the entry. It must be here!
        MemorySegmentEntry* cacheEntry = _imp->bucket->tryCacheLookupImpl(_imp->hashStr);
        assert(cacheEntry);
        if (!cacheEntry) {
            throw std::logic_error("CacheEntryLocker::insertInCache");
        }

        // The status of the memory segment entry should be pending because we are the thread computing it.
        // All other threads are waiting.
        assert(cacheEntry->status == MemorySegmentEntry::eEntryStatusPending);

        // The cache entry mutex should have been taken in the lookupAndSetStatus function.
        assert(!cacheEntry->lock.try_lock());

        // The cacheEntry fields should be uninitialized
        // This may throw an exception if out of memory or if the getMetadataSize function does not return
        // enough memory to encode all the data.
        void* bufPtr = 0;
        try {

            // Allocate memory for the entry metadatas, rounded to the page size, + 1 page to let the memory
            // management algorithm store its own memory housekeeping data.
            std::size_t pageSize = bip::mapped_region::get_page_size();
            cacheEntry->size = _imp->processLocalEntry->getMetadataSize();
            cacheEntry->size = (std::ceil(cacheEntry->size / (double)pageSize) + 1) * pageSize;


            if (_imp->bucket->tocFileManager->get_free_memory() <= cacheEntry->size) {
                // We need to allocate more memory: resize the file.
                // This is safe to do it now because we have the write lock on the validToCMappingMutex
                _imp->bucket->growToCFile();

                // re-open the mapping
                _imp->bucket->ensureToCFileMappingValid();

                // We must re-lookup the entry
                cacheEntry = _imp->bucket->tryCacheLookupImpl(_imp->hashStr);
                assert(cacheEntry);
            }

            bufPtr = _imp->bucket->tocFileManager->allocate(cacheEntry->size);
            cacheEntry->memorySegmentPortion = _imp->bucket->tocFileManager->get_handle_from_address(bufPtr);

            // Serialize the meta-datas in the memory segment
            {

                ExternalSegmentType externalManager(bip::create_only, bufPtr, cacheEntry->size);
                _imp->processLocalEntry->toMemorySegment(&externalManager);
            }

            // Insert the hash in the LRU linked list
            // Lock the LRU list mutex
            {
                bip::scoped_lock<bip::interprocess_mutex> lruWriteLock(_imp->bucket->ipc->lruListMutex);
                cacheEntry->lruIterator = static_cast<LRUListNode*>(_imp->bucket->tocFileManager->allocate(sizeof(LRUListNode)));
                cacheEntry->lruIterator->hash = _imp->processLocalEntry->getHashKey();

                if (!_imp->bucket->ipc->lruListBack) {
                    assert(!_imp->bucket->ipc->lruListFront);
                    // The list is empty, initialize to this node
                    _imp->bucket->ipc->lruListFront = _imp->bucket->ipc->lruListBack = cacheEntry->lruIterator;
                } else {
                    // Append to the tail of the list
                    assert(_imp->bucket->ipc->lruListFront);

                    insertLinkedListNode(cacheEntry->lruIterator, _imp->bucket->ipc->lruListBack, bip::offset_ptr<LRUListNode>(0));
                    // Update back node
                    _imp->bucket->ipc->lruListBack = cacheEntry->lruIterator;
                    
                }
            } // lruWriteLock
            cacheEntry->status = MemorySegmentEntry::eEntryStatusReady;

            _imp->status = eCacheEntryStatusCached;

            // Notify other threads we are done with this entry by releasing the statusMutex lock
            cacheEntry->lock.unlock();

        } catch (...) {

            _imp->bucket->deallocateCacheEntryImpl(cacheEntry, true /*releaseLock*/);

        }


    } // writeLock


    // Set the entry cache bucket index
    _imp->processLocalEntry->setCacheBucketIndex(_imp->bucket->bucketIndex);

    _imp->cache->onEntryInsertedInCache(_imp->processLocalEntry);


    // Concurrency resumes!
    
} // insertInCache

CacheEntryLocker::CacheEntryStatusEnum
CacheEntryLocker::waitForPendingEntry()
{
    // The thread can only wait if the status was set to eCacheEntryStatusComputationPending
    assert(_imp->status == eCacheEntryStatusComputationPending);
    assert(_imp->processLocalEntry);

    // If this thread is a threadpool thread, it may wait for a while that results gets available.
    // Release the thread to the thread pool so that it may use this thread for other runnables
    // and reserve it back when done waiting.
    bool hasReleasedThread = false;
    if (isRunningInThreadPoolThread()) {
        QThreadPool::globalInstance()->releaseThread();
        hasReleasedThread = true;
    }

    do {

        // Look up the cache, but first take the lock on the MemorySegmentEntry
        // that will be released once another thread unlocked it in insertInCache
        // or the destructor.
        lookupAndSetStatus(true /*takeEntryLock*/);

    } while(_imp->status == eCacheEntryStatusComputationPending);

    // Concurrency resumes!

    if (hasReleasedThread) {
        QThreadPool::globalInstance()->reserveThread();
    }
    
    
    return _imp->status;
} // waitForPendingEntry

CacheEntryLocker::~CacheEntryLocker()
{

    // If cached, we don't have to release any data
    if (_imp->status == eCacheEntryStatusCached) {
        return;
    }

    // The cache entry is still pending: the caller thread did not call waitForPendingEntry() nor
    // insertInCache()
    bip::scoped_lock<bip::interprocess_upgradable_mutex> writeLock(_imp->cache->_imp->ipc->bucketsData[_imp->bucket->bucketIndex].validToCMappingMutex);
    // Every time we take the lock, we must ensure the memory mapping is ok
    _imp->bucket->ensureToCFileMappingValid();


    MemorySegmentEntry* cacheEntry = _imp->bucket->tryCacheLookupImpl(_imp->hashStr);
    assert(cacheEntry);
    if (!cacheEntry) {
        throw std::logic_error("CacheEntryLocker::~CacheEntryLocker");
    }
    assert(cacheEntry->status == MemorySegmentEntry::eEntryStatusPending);

    // If we are pending, this thread did not call waitForPendingEntry() thus did not
    // take the cacheEntry lock.
    // If we are eCacheEntryStatusMustCompute, the thread did not call insertInCache()
    // thus still have the lock. We have to unlock it.
    _imp->bucket->deallocateCacheEntryImpl(cacheEntry, _imp->status == eCacheEntryStatusMustCompute);
}


Cache::Cache()
: QObject()
, boost::enable_shared_from_this<Cache>()
, _imp(new CachePrivate(this))
{

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

    ret->_imp->initializeCacheDirPath();
    ret->_imp->ensureCacheDirectoryExists();


    // Create the main memory segment containing the CachePrivate::IPCData
    {
        // Give the global memory segment a little bit more room for its own internal state.
        std::size_t pageSize = bip::mapped_region::get_page_size();

        // Allocate 500KB for the global data. This shouldn't need more, unless too many processes
        // are inserted in the sets in CachePrivate::IPCData. (very unlikely to happen).
        std::size_t desiredSize = 1024 * 500;
        desiredSize = std::ceil(desiredSize / (double)pageSize) * pageSize;
        std::stringstream ss;
        ss << NATRON_CACHE_DIRECTORY_NAME  << "_GlobalData";
        ret->_imp->globalMemorySegment.reset(new bip::managed_shared_memory(bip::open_or_create, ss.str().c_str(), desiredSize));
        ret->_imp->ipc = ret->_imp->globalMemorySegment->find_or_construct<CachePrivate::IPCData>("CacheData")();

    }
    
    // Open each bucket individual memory segment.
    // They are not created in shared memory but in a memory mapped file instead
    // to be persistent when the OS shutdown.
    // Each segment controls the table of content of the bucket.

    for (int i = 0; i < NATRON_CACHE_BUCKETS_COUNT; ++i) {

        // Hold a weak pointer to the cache on the bucket
        ret->_imp->buckets[i].cache = ret;
        ret->_imp->buckets[i].bucketIndex = i;

        // Get the bucket directory path. It ends with a separator.
        QString bucketDirPath = ret->_imp->getBucketAbsoluteDirPath(i);

        // Open the cache ToC shared memory segment
        {
            std::string tocFilePath = bucketDirPath.toStdString() + "Index";
            ret->_imp->buckets[i].tocFile.reset(new MemoryFile);

            // Take the ToC mapping mutex and register this process amongst the valid mapping
            bip::scoped_lock<bip::interprocess_upgradable_mutex> writeLock(ret->_imp->ipc->bucketsData[i].validToCMappingMutex);
            ret->_imp->ipc->bucketsData[i].validToCMapping.insert(ret->_imp->pid);

            // If the file did not exist, resize it to at least contain NATRON_CACHE_BUCKET_TOC_FILE_GROW_N_BYTES minimum
            if (ret->_imp->buckets[i].tocFile->size() < NATRON_CACHE_BUCKET_TOC_FILE_GROW_N_BYTES) {
                ret->_imp->buckets[i].tocFile->resize(NATRON_CACHE_BUCKET_TOC_FILE_GROW_N_BYTES, false /*preserve*/);
            }

            // Create an externally managed data structure in our ToC
            ret->_imp->buckets[i].tocFileManager.reset(new ExternalSegmentType(bip::create_only, ret->_imp->buckets[i].tocFile->data(), ret->_imp->buckets[i].tocFile->size()));

            // Find or create the IPC data for this bucket.
            ret->_imp->buckets[i].ipc = ret->_imp->buckets[i].tocFileManager->find_or_construct<CacheBucket::IPCData>("BucketData")();
        }


        // Open the memory-mapped file used for tiled entries data storage.
        {
            std::string tileCacheFilePath = bucketDirPath.toStdString() + "TileCache";

            ret->_imp->buckets[i].tileAlignedFile.reset(new MemoryFile);

            // Take the ToC mapping mutex and register this process amongst the valid mapping
            bip::scoped_lock<bip::interprocess_upgradable_mutex> writeLock(ret->_imp->ipc->bucketsData[i].validTileMappingMutex);
            ret->_imp->ipc->bucketsData[i].validTileMapping.insert(ret->_imp->pid);

            ret->_imp->buckets[i].tileAlignedFile->open(tileCacheFilePath, MemoryFile::eFileOpenModeOpenOrCreate);

            // Ensure the tiled memory mapped file has some free tiles
            if (ret->_imp->buckets[i].tileAlignedFile->size() == 0) {
                ret->_imp->buckets[i].growTileFile();
            }

        }

    } // for each bucket


    return ret;
} // create


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
    std::size_t ret = 0;

    // Lock for read the size lock
    bip::sharable_lock<bip::interprocess_sharable_mutex> locker(_imp->ipc->sizeLock);
    switch (storage) {
        case eStorageModeDisk:
            return _imp->ipc->diskSize;
            break;
        case eStorageModeGLTex:
            return _imp->ipc->memorySize;
            break;
        case eStorageModeRAM:
            return _imp->ipc->glTextureSize;
            break;
        case eStorageModeNone:
            break;
    }

    return ret;
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
CachePrivate::incrementCacheSize(long long size, StorageModeEnum storage)
{

    // Lock for writing
    bip::scoped_lock<bip::interprocess_sharable_mutex> locker(ipc->sizeLock);

    switch (storage) {
        case eStorageModeDisk:
            assert(size >= 0 || ipc->diskSize >= (std::size_t)std::abs(size));
            ipc->diskSize += size;
            break;
        case eStorageModeGLTex:
            assert(size >= 0 || ipc->glTextureSize >= (std::size_t)std::abs(size));
            ipc->glTextureSize += size;
            break;
        case eStorageModeRAM:
            assert(size >= 0 || ipc->memorySize >= (std::size_t)std::abs(size));
            ipc->memorySize += size;
            break;
        case eStorageModeNone:
            break;
    }
}

void
CachePrivate::initializeCacheDirPath()
{
    std::string cachePath = appPTR->getCurrentSettings()->getDiskCachePath();
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
        directoryContainingCachePath = cachePath;
    } else {
        directoryContainingCachePath = StandardPaths::writableLocation(StandardPaths::eStandardLocationCache).toStdString();
    }
} // initializeCacheDirPath

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


std::string
Cache::getCacheDirectoryPath() const
{
    QString cacheFolderName;
    cacheFolderName = QString::fromUtf8(_imp->directoryContainingCachePath.c_str());
    StrUtils::ensureLastPathSeparator(cacheFolderName);
    cacheFolderName.append( QString::fromUtf8(NATRON_CACHE_DIRECTORY_NAME) );
    return cacheFolderName.toStdString();
} // getCacheDirectoryPath


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

std::size_t
Cache::allocTile(int bucketIndex)
{
    assert(bucketIndex >= 0 && bucketIndex < NATRON_CACHE_BUCKETS_COUNT);
    if (bucketIndex < 0 && bucketIndex >= NATRON_CACHE_BUCKETS_COUNT) {
        throw std::bad_alloc();
    }

    // Dispatch to the appropriate bucket
    CacheBucket& bucket = _imp->buckets[bucketIndex];

    // Lock for writing the tile mapping
    bip::scoped_lock<bip::interprocess_upgradable_mutex> writeLock(_imp->ipc->bucketsData[bucketIndex].validTileMappingMutex);

    // Ensure the mapping is still ok
    bucket.ensureTileMappingValid();
    bucket.ensureToCFileMappingValid();

    // Check if there's a free tile, otherwise grow the file
    if (bucket.ipc->freeTiles.empty()) {
        bucket.growTileFile();
    }
    assert(!bucket.ipc->freeTiles.empty());
    MM_unordered_set_size_t::iterator it = bucket.ipc->freeTiles.begin();
    std::size_t ret = *it;
    bucket.ipc->freeTiles.erase(it);
    return ret;
} // allocTile

void
Cache::freeTile(int bucketIndex, std::size_t cacheFileChunkIndex)
{
    assert(bucketIndex >= 0 && bucketIndex < NATRON_CACHE_BUCKETS_COUNT);
    if (bucketIndex < 0 && bucketIndex >= NATRON_CACHE_BUCKETS_COUNT) {
        return;
    }

    CacheBucket& bucket = _imp->buckets[bucketIndex];

    {
        // Lock for writing the ToC mapping
        bip::scoped_lock<bip::interprocess_upgradable_mutex> writeLock(_imp->ipc->bucketsData[bucketIndex].validTileMappingMutex);

        // Ensure the mapping is still ok
        bucket.ensureToCFileMappingValid();

        // Make this tile free again
        std::pair<MM_unordered_set_size_t::iterator, bool>  insertOk = bucket.ipc->freeTiles.insert(cacheFileChunkIndex);
        assert(insertOk.second);
        (void)insertOk;

    }

    {
        // Lock for writing the tile mapping
        bip::scoped_lock<bip::interprocess_upgradable_mutex> writeLock(_imp->ipc->bucketsData[bucketIndex].validTileMappingMutex);

        // Ensure the mapping is still ok
        bucket.ensureTileMappingValid();

        // Invalidate this portion of the memory mapped file
        std::size_t dataOffset = cacheFileChunkIndex * NATRON_TILE_SIZE_BYTES;
        bucket.tileAlignedFile->flush(MemoryFile::eFlushTypeInvalidate, bucket.tileAlignedFile->data() + dataOffset, NATRON_TILE_SIZE_BYTES);
    }
    
} // freeTile


CacheEntryLockerPtr
Cache::get(const CacheEntryBasePtr& entry) const
{
    CachePtr thisShared = boost::const_pointer_cast<Cache>(shared_from_this());
    return CacheEntryLocker::create(thisShared, entry);
} // get

void
Cache::onEntryInsertedInCache(const CacheEntryBasePtr& entry)
{

    if (entry->isCacheSignalRequired()) {
        Q_EMIT cacheChanged();
    }
}

void
Cache::notifyMemoryAllocated(std::size_t size, StorageModeEnum storage)
{
    _imp->incrementCacheSize(size, storage);

    // We just allocateg something, ensure the cache size remains reasonable.
    // We cannot block here until the memory stays contained in the user requested memory portion:
    // if we would do so, then it could deadlock: Natron could require more memory than what
    // the user requested to render just one node.
    evictLRUEntries(size, storage);
}


void
Cache::notifyMemoryDeallocated(std::size_t size, StorageModeEnum storage)
{
    long long decr = -(long long)size;
    _imp->incrementCacheSize(decr, storage);
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
    std::string hashStr = CacheEntryKeyBase::hashToString(hash);

    // Take the bucket lock in write mode
    {
        bip::scoped_lock<bip::interprocess_upgradable_mutex> writeLock(_imp->ipc->bucketsData[bucketIndex].validToCMappingMutex);

        entry->setCacheBucketIndex(-1);

        {
            MemorySegmentEntry* cacheEntry = bucket.tryCacheLookupImpl(hashStr);
            if (cacheEntry) {
                bucket.deallocateCacheEntryImpl(cacheEntry, false /*releaseLock*/);
            }
        }

        // If we are the only user of this entry, delete it
        if (entry.use_count() == 1) {
            std::list<CacheEntryBasePtr> entriesToDelete;
            entriesToDelete.push_back(entry);
            appPTR->deleteCacheEntriesInSeparateThread(entriesToDelete);
        }
    }

} // removeEntry


void
Cache::clear()
{


    for (int bucket_i = 0; bucket_i < NATRON_CACHE_BUCKETS_COUNT; ++bucket_i) {
        CacheBucket& bucket = _imp->buckets[bucket_i];

        // Close the files
        {
            bip::scoped_lock<bip::interprocess_upgradable_mutex> writeLock(_imp->ipc->bucketsData[bucket_i].validToCMappingMutex);
            std::string tocFilePath = bucket.tocFile->path();
            bucket.tocFile->remove();
            bucket.tocFile->open(tocFilePath, MemoryFile::eFileOpenModeOpenTruncateOrCreate);
            bucket.growToCFile();
        }
        {
            bip::scoped_lock<bip::interprocess_upgradable_mutex> writeLock(_imp->ipc->bucketsData[bucket_i].validTileMappingMutex);
            std::string tileFilePath = bucket.tileAlignedFile->path();
            bucket.tileAlignedFile->remove();
            bucket.tileAlignedFile->open(tileFilePath, MemoryFile::eFileOpenModeOpenTruncateOrCreate);
            bucket.growTileFile();
        }

    } // for each bucket


    Q_EMIT cacheChanged();

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
                // Lock for writing
                bip::scoped_lock<bip::interprocess_upgradable_mutex> writeLock(_imp->ipc->bucketsData[bucket_i].validToCMappingMutex);

                // Clear the LRU entry of this bucket.
                if (bucket.) {
                    <#statements#>
                }
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

            ImageStorageBase* isMemoryEntry = dynamic_cast<ImageStorageBase*>(entry.get());
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



NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_Cache.cpp"
