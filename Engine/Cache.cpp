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
#include <QReadWriteLock>

GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
GCC_DIAG_OFF(unused-parameter)
#include <boost/unordered_set.hpp>
#include <boost/format.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp> // regular mutex
#include <boost/interprocess/sync/scoped_lock.hpp> // scoped lock a regular mutex
#include <boost/interprocess/sync/interprocess_upgradable_mutex.hpp> // r-w mutex that can upgrade read right to write
#include <boost/interprocess/sync/interprocess_sharable_mutex.hpp> // r-w mutex
#include <boost/interprocess/sync/sharable_lock.hpp> // scope lock a r-w mutex
#include <boost/interprocess/sync/upgradable_lock.hpp> // scope lock a r-w upgradable mutex
#include <boost/interprocess/sync/interprocess_condition_any.hpp> // wait cond with a r-w mutex
#include <boost/interprocess/sync/file_lock.hpp> //  file lock
#include <boost/interprocess/sync/named_semaphore.hpp> //  named semaphore
#include <boost/date_time/posix_time/posix_time.hpp> // time for timed lock
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
GCC_DIAG_ON(unused-parameter)

#include <SequenceParsing.h>

#include "Global/GlobalDefines.h"
#include "Global/StrUtils.h"
#include "Global/QtCompat.h"

#include "Engine/AppManager.h"
#include "Engine/StorageDeleterThread.h"
#include "Engine/FStreamsSupport.h"
#include "Engine/MemoryFile.h"
#include "Engine/MemoryInfo.h"
#include "Engine/Settings.h"
#include "Engine/StandardPaths.h"
#include "Engine/RamBuffer.h"
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

// If we change the MemorySegmentEntryHeader struct, we must increment this version so we do not attempt to read an invalid structure.
#define NATRON_MEMORY_SEGMENT_ENTRY_HEADER_VERSION 1

//#define CACHE_TRACE_ENTRY_ACCESS
//#define CACHE_TRACE_TIMEOUTS
//#define CACHE_TRACE_FILE_MAPPING
//#define CACHE_TRACE_TILES_ALLOCATION

namespace bip = boost::interprocess;


NATRON_NAMESPACE_ENTER;




// Typedef our interprocess types
typedef bip::allocator<std::size_t, ExternalSegmentType::segment_manager> Size_t_Allocator_ExternalSegment;



// The unordered set of free tiles indices in a bucket
typedef bip::set<std::size_t, std::less<std::size_t>, Size_t_Allocator_ExternalSegment> set_size_t_ExternalSegment;

typedef bip::sharable_lock<bip::interprocess_upgradable_mutex> ReadLock;
typedef bip::upgradable_lock<bip::interprocess_upgradable_mutex> UpgradableLock;
typedef bip::scoped_lock<bip::interprocess_upgradable_mutex> WriteLock;

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
    node->prev = 0;

    // Make the next item predecessor point to this item predecessor
    if (node->next) {
        node->next->prev = node->prev;
    }
    node->next = 0;
}

inline
void insertLinkedListNode(const bip::offset_ptr<LRUListNode>& node, const bip::offset_ptr<LRUListNode>& prev, const bip::offset_ptr<LRUListNode>& next)
{
    assert(node);
    if (prev) {
        prev->next = node;
        assert(prev->next);
    }
    node->prev = prev;

    if (next) {
        next->prev = node;
        assert(next->prev);
    }
    node->next = next;
}

class SharedMemoryReader;

/**
 * @brief This struct represents the minimum required data for a cache entry in the global bucket memory segment.
 * It is associated to a hash in the LRU linked list.
 * This struct lives in memory
 **/
struct MemorySegmentEntryHeader
{

    // If this entry has data in the tile aligned memory mapped file, this is the
    // index of the tile allocated. If not allocated, this is  -1.
    int tileCacheIndex;

    // The size of the memorySegmentPortion, in bytes. This is stored in the main cache memory segment.
    std::size_t size;

    // Hold an iterator pointing to this entry
    //
    // From http://www.sgi.com/tech/stl/List.html :
    // Lists have the important property that insertion and splicing do not invalidate
    // iterators to list elements, and that even removal invalidates
    // only the iterators that point to the elements that are removed.
    bip::offset_ptr<LRUListNode> lruIterator;


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

    // The status of the entry
    EntryStatusEnum status;

    // The ID of the plug-in holding this entry
    String_ExternalSegment pluginID;

    // List of pointers to entry data allocated in the bucket memory segment
    ExternalSegmentTypeHandleList entryDataPointerList;

    // A version indicator for the serialization. If and entry version doesn't correspond
    // to NATRON_MEMORY_SEGMENT_ENTRY_HEADER_VERSION
    unsigned int version;

    MemorySegmentEntryHeader(const void_allocator& allocator)
    : tileCacheIndex(-1)
    , size(0)
    , lruIterator(0)
    , status(eEntryStatusNull)
    , pluginID(allocator)
    , entryDataPointerList(allocator)
    , version(0)
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
        set_size_t_ExternalSegment freeTiles;

        // Protects the LRU list. This is separate to the bucketLock because even if we just access
        // the cache in read mode (in the get() function) we still need to update the LRU list, thus
        // protect it from being written by multiple concurrent threads.
        bip::interprocess_mutex lruListMutex;

        // Pointers in shared memory to the lru list from node and back node
        bip::offset_ptr<LRUListNode> lruListFront, lruListBack;


        IPCData(const Size_t_Allocator_ExternalSegment& freeTilesAllocator)
        : freeTiles(freeTilesAllocator)
        , lruListMutex()
        , lruListFront(0)
        , lruListBack(0)
        {

        }
    };


    // Memory mapped file for tiled entries: the size of this file is a multiple of the tile byte size.
    // Any access to the file should be protected by the tileData.segmentMutex mutex located in
    // CachePrivate::IPCData::PerBucketData
    MemoryFilePtr tileAlignedFile;

    // Memory mapped file used to store interprocess table of contents (IPCData)
    // It contains for each entry:
    // - A LRUListNode
    // - A MemorySegmentEntry
    // - A memory buffer of arbitrary size
    // Any access to the file should be protected by the tocData.segmentMutex mutex located in
    // CachePrivate::IPCData::PerBucketData
    MemoryFilePtr tocFile;

    // A memory manager of the tocFile. It is only valid when the tocFile is memory mapped.
    boost::shared_ptr<ExternalSegmentType> tocFileManager;

    // Pointer to the IPC data that live in tocFile memory mapped file
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
     * This function assumes that tocData.segmentMutex must be taken in write mode
     * This function may take the tileData.segmentMutex in write mode.
     **/
    void deallocateCacheEntryImpl(MemorySegmentEntryHeader* entry,
                                  const std::string& hashStr);

    /**
     * @brief Lookup the cache for a MemorySegmentEntry matching the hash key.
     * If found, the cacheEntry member will be set.
     * This function assumes that the tocData.segmentMutex is taken at least in read mode.
     **/
    MemorySegmentEntryHeader* tryCacheLookupImpl(const std::string& hashStr);

    /**
     * @brief Reads the cacheEntry into the processLocalEntry.
     * This function updates the status member.
     * This function assumes that the bucketLock of the bucket is taken at least in read mode.
     * @returns True if ok, false if the MemorySegmentEntry cannot be read properly.
     * it should be deallocated from the segment.
     **/
    bool readFromSharedMemoryEntryImpl(MemorySegmentEntryHeader* entry,
                                       const CacheEntryBasePtr& processLocalEntry,
                                       const std::string &hashStr,
                                       CacheEntryLocker::CacheEntryStatusEnum* status);

    /**
     * @brief Returns whether the ToC memory mapped file mapping is still valid.
     * The tocData.segmentMutex is assumed to be taken for read-lock
     **/
    bool isToCFileMappingValid() const;

    /**
     * @brief Ensures that the ToC memory mapped file mapping is still valid and re-open it if not.
     * The tocData.segmentMutex is assumed to be taken for write-lock
     * @param minFreeSize Indicates that the file should have at least this amount of free bytes.
     * If not, this function will call growTileFile.
     * If the file is empty and minFreeSize is 0, the file will at least be grown to a size of
     * NATRON_CACHE_BUCKET_TOC_FILE_GROW_N_BYTES
     **/
    void ensureToCFileMappingValid(WriteLock& lock, std::size_t minFreeSize);

    /**
     * @brief Returns whether the tile aligned memory mapped file mapping is still valid.
     * The tileData.segmentMutex is assumed to be taken for read-lock
     **/
    bool isTileFileMappingValid() const;

    /**
     * @brief Ensures that the tile aligned memory mapped file mapping is still valid and re-open it if not.
     * The tileData.segmentMutex AND tocData.segmentMutex are assumed to be taken for write-lock.
     * ensureToCFileMappingValid must have been called first because this function needs to access
     * the freeTiles data.
     * @param minFreeSize Indicates that the file should have at least this amount of free bytes.
     * If not, this function will call growTileFile.
     * If the file is empty and minFreeSize is 0, the file will at least be grown to a size of
     * NATRON_TILE_SIZE_BYTES * NATRON_CACHE_FILE_GROW_N_TILES
     **/
    void ensureTileMappingValid(WriteLock& lock, std::size_t minFreeSize);

    /**
     * @brief Grow the ToC memory mapped file. 
     * This will first wait all other processes accessing to the mapping before resizing.
     * Any process trying to access the mapping during resizing will wait.
     * The tileData.segmentMutex is assumed to be taken for write lock
     *
     * This function is called internally by ensureToCFileMappingValid()
     **/
    void growToCFile(WriteLock& lock, std::size_t bytesToAdd);

    /**
     * @brief Grow the tile memory mapped file. 
     * This will first wait all other processes accessing to the mapping before resizing.
     * Any process trying to access the mapping during resizing will wait.
     * The tileData.segmentMutex is assumed to be taken for write lock
     *
     * This function is called internally by ensureTileMappingValid()
     **/
    void growTileFile(WriteLock& lock, std::size_t bytesToAdd);
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

    CacheEntryLockerPrivate(CacheEntryLocker* publicInterface, const CachePtr& cache, const CacheEntryBasePtr& entry);

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

        struct SharedMemorySegmentData
        {
            // Lock protecting the memory file read/write access.
            // Whenever a process/thread reads the memory segment, it takes the lock in read mode.
            // Whenever a process/thread needs to write to or grow or shrink the memory segment, it takes this lock
            // in write mode.
            bip::interprocess_upgradable_mutex segmentMutex;

            // True whilst the mapping is valid.
            // Any time the memory mapped file needs to be accessed, the caller
            // is supposed to call isTileFileMappingValid() to check if the mapping is still valid.
            // If this function returns false, the caller needs to take a write lock and call
            // ensureToCFileMappingValid or ensureTileMappingValid depending on the memory file accessed.
            bool mappingValid;

            // The number of processes with a valid mapping: use to count processes with a valid mapping
            // See pseudo code below, this is used in combination with the wait conditions.
            int nProcessWithMappingValid;

            // Threads wait on this condition whilst the mappingValid flag is false
            bip::interprocess_condition_any mappingInvalidCond;

            // The thread that wants to grow the memory portion just waits in this condition
            // until nProcessWithToCMappingValid is 0.
            //
            // To resize the segment portion, it does as follow:
            //
            // writeLock(segmentMutex);
            // mappingValid = false;
            // segment.unmap();
            // --nProcessWithMappingValid;
            // while(nProcessWithMappingValid > 0) {
            //      mappedProcessesNotEmpty.wait(segmentMutex);
            // }
            // segment.grow();
            // segment.remap();
            // ++nProcessWithMappingValid;
            // mappingValid = true;
            // mappingInvalidCond.notifyAll();
            //
            // In other threads, before accessing the mapped region:
            //
            // thisMappingValid = true;
            // {
            //  readLock(segmentMutex);
            //  if (!mappingValid) {
            //      thisMappingValid = false;
            //  }
            // }
            // if (!thisMappingValid) {
            //    writeLock(segmentMutex);
            //    if (!mappingValid) {
            //          segment.unmap()
            //          --nProcessWithMappingValid;
            //          mappedProcessesNotEmpty.notifyOne();
            //          while(!mappingValid) {
            //              mappingInvalidCond.wait(segmentMutex);
            //          }
            //
            //          segment.remap();
            //          ++nProcessWithMappingValid;
            //    }
            // }
            bip::interprocess_condition_any mappedProcessesNotEmpty;

            SharedMemorySegmentData()
            : segmentMutex()
            , mappingValid(true)
            , nProcessWithMappingValid(0)
            , mappingInvalidCond()
            , mappedProcessesNotEmpty()
            {

            }
        };

        struct PerBucketData
        {

            // Data related to the table of content memory mapped file
            SharedMemorySegmentData tocData;

            // Data related to the tiled memory mapped file
            SharedMemorySegmentData tileData;
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

    // Pointer to the memory segment used to store bucket independent data accross processes.
    // This is ensured to be always valid and lives in process memory.
    // The global memory segment is of a fixed size and only contains one instance of CachePrivate::IPCData.
    // This is the only shared memory segment that has a fixed size.
    boost::scoped_ptr<bip::managed_shared_memory> globalMemorySegment;

    // A file lock used to ensure the integrity of the globalMemorySegment shared memory.
    // A Natron process could very well crash whilst a mutex is taken: any subsequent attempt to lock
    // the mutex would deadlock.
    // To ensure the shared memory does not get corrupted as such, we use a file lock.
    // The file lock has the interesting property that it lives as long as the process lives:
    // From: http://www.boost.org/doc/libs/1_63_0/doc/html/interprocess/synchronization_mechanisms.html#interprocess.synchronization_mechanisms.file_lock.file_lock_whats_a_file_lock
    /*A file locking is a class that has process lifetime.
     This means that if a process holding a file lock ends or crashes, 
     the operating system will automatically unlock it.
     This feature is very useful in some situations where we want to assure
     automatic unlocking even when the process crashes and avoid leaving blocked resources in the system.
     */
    //
    // We apply the following steps:
    // When starting up a new Natron process: globalMemorySegmentFileLock.try_lock()
    //      - If it succeeds, that means no other process is active: We remove the globalMemorySegment shared memory segment
    //        and create a new one, to ensure no lock was left in a bad state. Then we release the file lock
    //      - If it fails, another process is still actively using the globalMemorySegment shared memory: it must still be valid
    //
    // We then take the file lock in read mode, indicating that we use the shared memory:
    //      globalMemorySegmentFileLock.lock_sharable()
    //
    // Any operation taking the segmentMutex in the shared memory, must do so with a timeout so we can avoid deadlocks:
    // If a process crashes whilst the segmentMutex is taken, the file lock is ensured to be released but the
    // segmentMutex will remain taken, deadlocking any other process.
    //
    // To overcome the deadlock, we add 2 named semaphores (nSHMValid, nSHMInvalid).
    //
    // If the segmentMutex times out, we apply the following steps:
    //
    // 0 - Lock the nThreadsTimedOutFailedMutex mutex to ensure only 1 thread in this process
    //     does the following operations.
    //     ++nThreadsTimedOutFailed
    //     If nThreadsTimedOutFailed == 1, do steps 1 to 9 (included), otherwise
    //     skip directly to step 10
    // 1 - unmap the globalMemorySegment shared memory
    //
    // 2 - nSHMInvalid.post() --> The mapping for this process is no longer invalid
    //
    // 3 - We release the read lock taken on the globalMemorySegmentFileLock: globalMemorySegmentFileLock.unlock()
    //
    // 4 - We take the file lock in write mode: globalMemorySegmentFileLock.lock():
    //   The lock is guaranteed to be taken at some point since any active process will eventually timeout on the segmentMutex and release
    //   their read lock on the globalMemorySegmentFileLock in step 3. We are sure that when the lock is taken, nobody else is still in step 3.
    //
    //  Now that we have the file lock in write mode, we may not be the first process to have it:
    //     5 -  nSHMValid.try_wait() --> If this returns false, we are the first process to take the write lock.
    //                               We know at this point that any other process has released its read lock on the globalMemorySegmentFileLock
    //                               and that the globalMemorySegment is no longer mapped anywhere.
    //                               We thus remove the globalMemorySegment and re-create it and remap it.
    //
    //                           --> If this returns true, we are not the first process to take the write lock, hence the globalMemorySegment
    //                               has been re-created already, so just map it.
    //
    //      6 - nSHMValid.post() --> Indicate that we mapped the shared memory segment
    //
    //      7 - nSHMInvalid.wait() --> Decrement the post() that we made earlier
    //
    //      8 - Release the write lock: globalMemorySegmentFileLock.unlock()
    //
    // 9 - When the write lock is released we cannot take the globalMemorySegmentFileLock in read mode yet, we could block other processes that
    // are still waiting for the write lock in 4.
    // We must wait that every other process has a valid mapping:
    //
    //
    //      while(nSHMInvalid.try_wait()) {
    //          nSHMInvalid.post()
    //      }
    //
    //  nSHMInvalid.try_wait() will return false when all processes have been remapped.
    //  If it returns true, that means another process is still in-between steps 4 and 8, thus we post
    //  what we decremented in try_wait and re-try again.
    //
    // 10 - Now wait that all timed out threads are finished
    //      --nThreadsTimedOutFailed
    //      while(nThreadsTimedOutFailed > 0) {
    //          nThreadsTimedOutFailedCond.wait(&nThreadsTimedOutFailedMutex)
    //      }
    boost::scoped_ptr<bip::file_lock> globalMemorySegmentFileLock;

    // Used in the implementation of the algorithm described above.
    boost::scoped_ptr<bip::named_semaphore> nSHMInvalidSem, nSHMValidSem;

    // A mutex used in the algorithm described above to lock the process local threads.
    // Protects nThreadsTimedOutFailed
    // It should be taken for reading anytime a process use an object in shared memory
    // and locked for writing when unmapping/remapping the shared memory.
    QReadWriteLock nThreadsTimedOutFailedMutex;

    // Counts how many threads in this process timed out on the segmentMutex, to avoid
    // remapping multiple times the shared memory.
    int nThreadsTimedOutFailed;

    // Protected by nThreadsTimedOutFailedMutex
    QWaitCondition nThreadsTimedOutFailedCond;


    // The IPC data object created in globalMemorySegment shared memory
    IPCData* ipc;

    // Path of the directory that should contain the cache directory itself.
    // This is controled by a Natron setting. By default it points to a standard system dependent
    // location.
    std::string directoryContainingCachePath;


    CachePrivate(Cache* publicInterface)
    : _publicInterface(publicInterface)
    , maximumDiskSize((std::size_t)10 * 1024 * 1024 * 1024) // 10GB max by default (RAM + Disk)
    , maximumInMemorySize((std::size_t)4 * 1024 * 1024 * 1024) // 4GB in RAM max by default
    , maximumGLTextureSize(0) // This is updated once we get GPU infos
    , maximumSizesMutex()
    , buckets()
    , globalMemorySegment()
    , globalMemorySegmentFileLock()
    , nSHMInvalidSem()
    , nSHMValidSem()
    , nThreadsTimedOutFailedMutex()
    , nThreadsTimedOutFailed(0)
    , nThreadsTimedOutFailedCond()
    , ipc(0)
    , directoryContainingCachePath()
    {


    }

    ~CachePrivate()
    {
    }

    void initializeCacheDirPath();

    void ensureCacheDirectoryExists();

    void incrementCacheSize(long long size, StorageModeEnum storage);

    QString getBucketAbsoluteDirPath(int bucketIndex) const;

    std::string getSharedMemoryName() const;

    std::size_t getSharedMemorySize() const;

    void ensureSharedMemoryIntegrity();

};

/**
 * @brief Small RAII style class that should be used before using anything that is in the cache global shared memory
 * segment.
 * This prevents any other threads to call ensureSharedMemoryIntegrity() whilst this object is active.
 **/
class SharedMemoryReader
{
    boost::scoped_ptr<QReadLocker> processLocalLocker;
public:

    SharedMemoryReader(CachePrivate* imp)
    {
        // A thread may enter ensureSharedMemoryIntegrity(), thus any other threads must ensure that the shared memory mapping
        // is valid before doing anything else.
        processLocalLocker.reset(new QReadLocker(&imp->nThreadsTimedOutFailedMutex));
        while (imp->nThreadsTimedOutFailed > 0) {
            imp->nThreadsTimedOutFailedCond.wait(&imp->nThreadsTimedOutFailedMutex);
        }
    }

    ~SharedMemoryReader()
    {
        // Release the processLocalLocker, allowing other threads to call ensureSharedMemoryIntegrity()
    }

};

template <typename LOCK>
bool createTimedLock(boost::scoped_ptr<LOCK>& lock,
                        typename LOCK::mutex_type* mutex,
                        std::size_t lockTimeOutMs = 300) WARN_UNUSED_RETURN;

template <typename LOCK>
bool createTimedLock(boost::scoped_ptr<LOCK>& lock,
                        typename LOCK::mutex_type* mutex,
                        std::size_t lockTimeOutMs)
{
    boost::posix_time::ptime abs_time = boost::posix_time::microsec_clock::universal_time() +  boost::posix_time::milliseconds(lockTimeOutMs);
    lock.reset(new LOCK(*mutex, abs_time));
    return lock->owns();
}

template <typename LOCK>
void createLockNoTimeout(boost::scoped_ptr<LOCK>& lock,
                        typename LOCK::mutex_type* mutex)
{
    lock.reset(new LOCK(*mutex));
}


/**
 * @brief Creates a locker object around the given process shared mutex.
 * If after the given lockTimeOutMs the lock could not be taken, the function
 * ensureSharedMemoryIntegrity() will be called. The mutex could be very well taken by 
 * a dead process. This function unmaps the shared memory, re-creates it and remap it.
 * @param shmReader Before calling this function, the shared memory where the interprocess lock resides
 * must be lock for reading.
 **/
template <typename LOCK>
void createLockAndEnsureSHM(CachePrivate* imp,
                boost::scoped_ptr<SharedMemoryReader>& shmReader,
                boost::scoped_ptr<LOCK>& lock,
                typename LOCK::mutex_type* mutex,
                std::size_t lockTimeOutMs = 300)
{
    // Take the lock. After lockTimeOutMS milliseconds, if the locks is not taken, we check the integrity of the
    // shared memory segment and retry the lock.
    // Another process could have taken the lock and crashed, leaving the shared memory in a bad state.
    for (;;) {
        boost::posix_time::ptime abs_time = boost::posix_time::microsec_clock::universal_time() +  boost::posix_time::milliseconds(lockTimeOutMs);
        lock.reset(new LOCK(*mutex, abs_time));
        if (lock->owns()) {
            break;
        } else {

            lock.reset();

#ifdef CACHE_TRACE_TIMEOUTS
            qDebug() << QThread::currentThread() << "Lock timeout, checking cache integrity";
#endif
            // Release the read lock on the SHM
            shmReader.reset();

            // Create and remap the SHM
            imp->ensureSharedMemoryIntegrity();

            // Flag that we are reading it
            shmReader.reset(new SharedMemoryReader(imp));
        }
    }
} // createLock


CacheEntryLockerPrivate::CacheEntryLockerPrivate(CacheEntryLocker* publicInterface, const CachePtr& cache, const CacheEntryBasePtr& entry)
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

    // Lock the SHM for reading to ensure all process shared mutexes and other IPC structures remains valid.
    // This will prevent any other thread from calling ensureSharedMemoryIntegrity()
    boost::scoped_ptr<SharedMemoryReader> shmAccess(new SharedMemoryReader(cache->_imp.get()));

    // Lookup and find an existing entry.
    // Never take over an entry upon timeout.
    ret->lookupAndSetStatus(shmAccess, 0, INT_MAX);
    return ret;
}

bool
CacheBucket::isToCFileMappingValid() const
{
    // Private - the tocData.segmentMutex is assumed to be taken for read lock
    CachePtr c = cache.lock();
    return c->_imp->ipc->bucketsData[bucketIndex].tocData.mappingValid;

} // isToCFileMappingValid

bool
CacheBucket::isTileFileMappingValid() const
{
    // Private - the tileData.segmentMutex is assumed to be taken for read lock
    CachePtr c = cache.lock();
    return c->_imp->ipc->bucketsData[bucketIndex].tileData.mappingValid;
} // isTileFileMappingValid

static void ensureMappingValidInternal(WriteLock& lock,
                                       const MemoryFilePtr& memoryMappedFile,
                                       CachePrivate::IPCData::SharedMemorySegmentData* segment)
{
    memoryMappedFile->close();
    std::string filePath = memoryMappedFile->path();

    // Decrement nProcessWithMappingValid and notify the thread that is resizing
    if (segment->nProcessWithMappingValid > 0) {
        --segment->nProcessWithMappingValid;
    }
    segment->mappedProcessesNotEmpty.notify_one();

    // Wait until the mapping becomes valid again
    while(!segment->mappingValid) {
        segment->mappingInvalidCond.wait(lock);
    }

    memoryMappedFile->open(filePath, MemoryFile::eFileOpenModeOpenOrCreate);
    ++segment->nProcessWithMappingValid;
} // ensureMappingValidInternal

static void reOpenToCData(CacheBucket* bucket, bool create)
{
    // Re-create the manager on the new mapped buffer
    try {
        if (create) {
            bucket->tocFileManager.reset(new ExternalSegmentType(bip::create_only, bucket->tocFile->data(), bucket->tocFile->size()));
        } else {
            bucket->tocFileManager.reset(new ExternalSegmentType(bip::open_only, bucket->tocFile->data(), bucket->tocFile->size()));
        }
    } catch (...) {
        assert(false);
        throw std::runtime_error("Not enough space to allocate bucket table of content!");
    }

    // The ipc data pointer must be re-fetched
    Size_t_Allocator_ExternalSegment freeTilesAllocator(bucket->tocFileManager->get_segment_manager());
    bucket->ipc = bucket->tocFileManager->find_or_construct<CacheBucket::IPCData>("BucketData")(freeTilesAllocator);
}

void
CacheBucket::ensureToCFileMappingValid(WriteLock& lock, std::size_t minFreeSize)
{
    // Private - the tocData.segmentMutex is assumed to be taken for write lock
    CachePtr c = cache.lock();

    if (!c->_imp->ipc->bucketsData[bucketIndex].tocData.mappingValid) {
        // Save the entire file
        tocFile->flush(MemoryFile::eFlushTypeSync, NULL, 0);

    }

#ifdef CACHE_TRACE_FILE_MAPPING
    qDebug() << "Checking ToC mapping:" << c->_imp->ipc->bucketsData[bucketIndex].tocData.mappingValid;
#endif

    ensureMappingValidInternal(lock,
                               tocFile,
                               &c->_imp->ipc->bucketsData[bucketIndex].tocData);

    // Ensure the size of the ToC file is reasonable
    if (tocFile->size() == 0) {
        growToCFile(lock, minFreeSize);
    } else {
        reOpenToCData(this, false /*create*/);

        // Check that there's enough memory, if not grow the file
        ExternalSegmentType::size_type freeMem = tocFileManager->get_free_memory();
        if (freeMem < minFreeSize) {
            std::size_t minbytesToGrow = minFreeSize - freeMem;
            growToCFile(lock, minbytesToGrow);
        }
    }
    assert(tocFileManager->get_free_memory() >= minFreeSize);

} // ensureToCFileMappingValid

static void flushTileMapping(const MemoryFilePtr& tileAlignedFile, const set_size_t_ExternalSegment& freeTiles)
{

    // Save only allocated tiles portion
    assert(tileAlignedFile->size() % NATRON_TILE_SIZE_BYTES == 0);
    std::size_t nTiles = tileAlignedFile->size() / NATRON_TILE_SIZE_BYTES;

    std::vector<bool> allocatedTiles(nTiles, true);

    // Mark all free tiles as not allocated
    for (set_size_t_ExternalSegment::const_iterator it = freeTiles.begin(); it != freeTiles.end(); ++it) {
        allocatedTiles[*it] = false;
    }

    for (std::size_t i = 0; i < allocatedTiles.size(); ++i) {
        std::size_t dataOffset = i * NATRON_TILE_SIZE_BYTES;
        if (allocatedTiles[i]) {
            tileAlignedFile->flush(MemoryFile::eFlushTypeSync, tileAlignedFile->data() + dataOffset, NATRON_TILE_SIZE_BYTES);
        } else {
            tileAlignedFile->flush(MemoryFile::eFlushTypeInvalidate, tileAlignedFile->data() + dataOffset, NATRON_TILE_SIZE_BYTES);
        }
    }
} // flushTileMapping

void
CacheBucket::ensureTileMappingValid(WriteLock& lock, std::size_t minFreeSize)
{
    // Private - the tileData.segmentMutex is assumed to be taken for write lock
    CachePtr c = cache.lock();
    if (!c->_imp->ipc->bucketsData[bucketIndex].tileData.mappingValid) {
        // The number of memory free requested must be a multiple of the tile size.
        assert(minFreeSize == 0 || minFreeSize % NATRON_TILE_SIZE_BYTES == 0);

        flushTileMapping(tileAlignedFile, ipc->freeTiles);
    }

#ifdef CACHE_TRACE_FILE_MAPPING
    qDebug() << "Checking ToC mapping:" << c->_imp->ipc->bucketsData[bucketIndex].tileData.mappingValid;
#endif

    ensureMappingValidInternal(lock, tileAlignedFile, &c->_imp->ipc->bucketsData[bucketIndex].tileData);

    // Ensure the size of the ToC file is reasonable
    if (tileAlignedFile->size() == 0) {
        growTileFile(lock, minFreeSize);
    } else {

        std::size_t freeMem = ipc->freeTiles.size() * NATRON_TILE_SIZE_BYTES;

        // Check that there's enough memory, if not grow the file
        if (freeMem < minFreeSize) {
            std::size_t minbytesToGrow = minFreeSize - freeMem;
            growTileFile(lock, minbytesToGrow);
        }
    }
    assert(ipc->freeTiles.size() * NATRON_TILE_SIZE_BYTES >= minFreeSize);

} // ensureTileMappingValid

void
CacheBucket::growToCFile(WriteLock& lock, std::size_t bytesToAdd)
{
    // Private - the tocData.segmentMutex is assumed to be taken for write lock

    CachePtr c = cache.lock();

    c->_imp->ipc->bucketsData[bucketIndex].tocData.mappingValid = false;

    --c->_imp->ipc->bucketsData[bucketIndex].tocData.nProcessWithMappingValid;
    while (c->_imp->ipc->bucketsData[bucketIndex].tocData.nProcessWithMappingValid > 0) {
        c->_imp->ipc->bucketsData[bucketIndex].tocData.mappedProcessesNotEmpty.wait(lock);
    }

    // Save the entire file
    tocFile->flush(MemoryFile::eFlushTypeSync, NULL, 0);

    std::size_t oldSize = tocFile->size();
    std::size_t newSize = oldSize + bytesToAdd;
    // Round to the nearest next multiple of NATRON_CACHE_BUCKET_TOC_FILE_GROW_N_BYTES
    newSize = std::max((std::size_t)1, (std::size_t)std::ceil(newSize / (double) NATRON_CACHE_BUCKET_TOC_FILE_GROW_N_BYTES)) * NATRON_CACHE_BUCKET_TOC_FILE_GROW_N_BYTES;
    tocFile->resize(newSize, false /*preserve*/);

#ifdef CACHE_TRACE_FILE_MAPPING
    qDebug() << "Growing ToC file to " << printAsRAM(newSize);
#endif


    reOpenToCData(this, oldSize == 0 /*create*/);

    ++c->_imp->ipc->bucketsData[bucketIndex].tocData.nProcessWithMappingValid;

    // Flag that the mapping is valid again and notify all other threads waiting
    c->_imp->ipc->bucketsData[bucketIndex].tocData.mappingValid = true;

    c->_imp->ipc->bucketsData[bucketIndex].tocData.mappingInvalidCond.notify_all();

} // growToCFile

void
CacheBucket::growTileFile(WriteLock& lock, std::size_t bytesToAdd)
{
    // Private - the tileData.segmentMutex is assumed to be taken for write lock
    // the tocData.segmentMutex is assumed to be taken for write lock because we need to read/write the free tiles

    CachePtr c = cache.lock();

    c->_imp->ipc->bucketsData[bucketIndex].tileData.mappingValid = false;

    --c->_imp->ipc->bucketsData[bucketIndex].tileData.nProcessWithMappingValid;
    while (c->_imp->ipc->bucketsData[bucketIndex].tileData.nProcessWithMappingValid > 0) {
        c->_imp->ipc->bucketsData[bucketIndex].tileData.mappedProcessesNotEmpty.wait(lock);
    }


    {
        // Update free tiles
        flushTileMapping(tileAlignedFile, ipc->freeTiles);

        {
            // Resize the file
            std::size_t curSize = tileAlignedFile->size();
            // The current size must be a multiple of the tile size
            assert(curSize % NATRON_TILE_SIZE_BYTES == 0);

            const std::size_t minTilesToAllocSize = NATRON_CACHE_FILE_GROW_N_TILES * NATRON_TILE_SIZE_BYTES;

            std::size_t newSize = curSize + bytesToAdd;
            // Round to the nearest next multiple of minTilesToAllocSize
            newSize = std::max((std::size_t)1, (std::size_t)std::ceil(newSize / (double) minTilesToAllocSize)) * minTilesToAllocSize;
            tileAlignedFile->resize(newSize, false /*preserve*/);

#ifdef CACHE_TRACE_FILE_MAPPING
            qDebug() << "Growing tile file to " << printAsRAM(newSize);
#endif


            int nTilesSoFar = curSize / NATRON_TILE_SIZE_BYTES;
            int newNTiles = newSize / NATRON_TILE_SIZE_BYTES;

            // Insert the new available tiles in the freeTiles set
            for (int i = nTilesSoFar; i < newNTiles; ++i) {
                ipc->freeTiles.insert((std::size_t)i);
            }
#ifdef CACHE_TRACE_TILES_ALLOCATION
            qDebug() << "Bucket" << bucketIndex << ": adding tiles from" << nTilesSoFar << "to" << newNTiles << " Nb free tiles left:" << ipc->freeTiles.size();
#endif
        }
    }

    ++c->_imp->ipc->bucketsData[bucketIndex].tileData.nProcessWithMappingValid;

    // Flag that the mapping is valid again and notify all other threads waiting
    c->_imp->ipc->bucketsData[bucketIndex].tileData.mappingValid = true;

    c->_imp->ipc->bucketsData[bucketIndex].tileData.mappingInvalidCond.notify_all();


} // growTileFile

MemorySegmentEntryHeader*
CacheBucket::tryCacheLookupImpl(const std::string& hashStr)
{
    // Private - the tocData.segmentMutex is assumed to be taken at least in read lock mode
    std::pair<MemorySegmentEntryHeader*, ExternalSegmentType::size_type> found = tocFileManager->find<MemorySegmentEntryHeader>(hashStr.c_str());
#ifdef CACHE_TRACE_ENTRY_ACCESS
    if (found.first) {
        qDebug() << hashStr.c_str() << "look-up: entry found";
    } else {
        qDebug() << hashStr.c_str() << "look-up: entry not found";
    }
#endif
    return found.first;
} // tryCacheLookupImpl

bool
CacheBucket::readFromSharedMemoryEntryImpl(MemorySegmentEntryHeader* cacheEntry,
                                           const CacheEntryBasePtr& processLocalEntry,
                                           const std::string &hashStr,
                                           CacheEntryLocker::CacheEntryStatusEnum* status)
{
    // Private - the tocData.segmentMutex is assumed to be taken at least in read lock mode

    // The entry must have been looked up in tryCacheLookup()
    assert(cacheEntry);

    // By default we are in mustComputeMode
    *status = CacheEntryLocker::eCacheEntryStatusMustCompute;

    assert(cacheEntry->status == MemorySegmentEntryHeader::eEntryStatusReady);

    if (cacheEntry->version != NATRON_MEMORY_SEGMENT_ENTRY_HEADER_VERSION) {
        // The structure has changed since, do not attempt to read it.
        return false;
    }

    CachePtr c = cache.lock();

    // If the entry is tiled, read from the tile buffer

    boost::scoped_ptr<ReadLock> tileReadLock;
    boost::scoped_ptr<WriteLock> tileWriteLock;
    char* tileDataPtr = 0;
    if (cacheEntry->tileCacheIndex != -1) {

        // First try to check if the tile aligned mapping is valid with a readlock
        bool tileMappingValid;

        {
            if (!createTimedLock<ReadLock>(tileReadLock, &c->_imp->ipc->bucketsData[bucketIndex].tileData.segmentMutex)) {
                *status = CacheEntryLocker::eCacheEntryStatusComputationPending;
                return true;
            }
            tileMappingValid = isTileFileMappingValid();
        }

        // mapping invalid, remap
        if (!tileMappingValid) {
            // If the tile mapping is invalid, take a write lock on the tile mapping and ensure it is valid
            tileReadLock.reset();

            if (!createTimedLock<WriteLock>(tileWriteLock, &c->_imp->ipc->bucketsData[bucketIndex].tileData.segmentMutex)) {
                *status = CacheEntryLocker::eCacheEntryStatusComputationPending;
                return true;
            }
            ensureTileMappingValid(*tileWriteLock, 0);
        }


        tileDataPtr = tileAlignedFile->data() + cacheEntry->tileCacheIndex * NATRON_TILE_SIZE_BYTES;
    }

    try {
        // Deserialize the entry. This may throw an exception if it cannot be deserialized properly
        // or out of memory
        processLocalEntry->fromMemorySegment(tocFileManager.get(), hashStr + "Data", tileDataPtr);
    } catch (...) {
        return false;
    }

    // Deserialization went ok, set the status
    *status = CacheEntryLocker::eCacheEntryStatusCached;

    // Update LRU record if this item is not already at the tail of the list
    //
    // Take the LRU list mutex
    {
        boost::scoped_ptr<bip::scoped_lock<bip::interprocess_mutex> > lruWriteLock;
        createLockNoTimeout<bip::scoped_lock<bip::interprocess_mutex> >(lruWriteLock, &ipc->lruListMutex);

        assert(ipc->lruListBack && !ipc->lruListBack->next);
        if (ipc->lruListBack != cacheEntry->lruIterator) {

            disconnectLinkedListNode(cacheEntry->lruIterator);

            // And push_back to the tail of the list...
            insertLinkedListNode(cacheEntry->lruIterator, ipc->lruListBack, bip::offset_ptr<LRUListNode>(0));
            ipc->lruListBack = cacheEntry->lruIterator;
        }
    } // lruWriteLock

    return true;

} // readFromSharedMemoryEntryImpl

void
CacheBucket::deallocateCacheEntryImpl(MemorySegmentEntryHeader* cacheEntry,
                                      const std::string& hashStr)
{

    // The tocData.segmentMutex must be taken in write mode

    // Does not throw any exception
    for (ExternalSegmentTypeHandleList::const_iterator it = cacheEntry->entryDataPointerList.begin(); it != cacheEntry->entryDataPointerList.end(); ++it) {
        void* bufPtr = tocFileManager->get_address_from_handle(*it);
        if (bufPtr) {
            tocFileManager->deallocate(bufPtr);
        }
    }
    cacheEntry->entryDataPointerList.clear();

    CachePtr c = cache.lock();


    if (cacheEntry->tileCacheIndex != -1) {
        // Free the tile
        // Take the tileData.segmentMutex in write mode

        {
            boost::scoped_ptr<WriteLock> writeLock;
            createLockNoTimeout<WriteLock>(writeLock, &c->_imp->ipc->bucketsData[bucketIndex].tileData.segmentMutex);

            ensureTileMappingValid(*writeLock, 0);

            // Invalidate this portion of the memory mapped file
            std::size_t dataOffset = cacheEntry->tileCacheIndex * NATRON_TILE_SIZE_BYTES;
            tileAlignedFile->flush(MemoryFile::eFlushTypeInvalidate, tileAlignedFile->data() + dataOffset, NATRON_TILE_SIZE_BYTES);
        }
        

        // Make this tile free again
#ifdef CACHE_TRACE_TILES_ALLOCATION
        qDebug() << "Bucket" << bucketIndex << ": tile freed" << cacheEntry->tileCacheIndex << " Nb free tiles left:" << ipc->freeTiles.size();
#endif
        std::pair<set_size_t_ExternalSegment::iterator, bool>  insertOk = ipc->freeTiles.insert(cacheEntry->tileCacheIndex);
        assert(insertOk.second);
        (void)insertOk;
        cacheEntry->tileCacheIndex = -1;
    }

    if (cacheEntry->lruIterator) {

        // Remove this entry from the LRU list
        {
            // Take the lock of the LRU list.
            boost::scoped_ptr<bip::scoped_lock<bip::interprocess_mutex> > lruWriteLock;
            createLockNoTimeout<bip::scoped_lock<bip::interprocess_mutex> >(lruWriteLock, &ipc->lruListMutex);

            // Ensure the back and front pointers do not point to this entry
            if (cacheEntry->lruIterator == ipc->lruListBack) {
                ipc->lruListBack = cacheEntry->lruIterator->next ? cacheEntry->lruIterator->next  : cacheEntry->lruIterator->prev;
            }
            if (cacheEntry->lruIterator == ipc->lruListFront) {
                ipc->lruListFront = cacheEntry->lruIterator->next;
            }

            // Remove this entry's node from the list
            disconnectLinkedListNode(cacheEntry->lruIterator);

            tocFileManager->deallocate(cacheEntry->lruIterator.get());
        }
        cacheEntry->lruIterator = 0;
    }

    // deallocate the entry
#ifdef CACHE_TRACE_ENTRY_ACCESS
    qDebug() << hashStr.c_str() << ": destroy entry";
#endif
    tocFileManager->destroy<MemorySegmentEntryHeader>(hashStr.c_str());
} // deallocateCacheEntryImpl

/*
 helper function to do thread sleeps, since usleep()/nanosleep()
 aren't reliable enough (in terms of behavior and availability)
 */
#ifdef __NATRON_UNIX__
static void thread_sleep(struct timespec *ti)
{
    pthread_mutex_t mtx;
    pthread_cond_t cnd;

    pthread_mutex_init(&mtx, 0);
    pthread_cond_init(&cnd, 0);

    pthread_mutex_lock(&mtx);
    (void) pthread_cond_timedwait(&cnd, &mtx, ti);
    pthread_mutex_unlock(&mtx);

    pthread_cond_destroy(&cnd);
    pthread_mutex_destroy(&mtx);
}
#endif

static void sleep_milliseconds(std::size_t amountMS)
{
#ifdef __NATRON_WIN32__
     ::Sleep(amountMS);
#elif defined(__NATRON_UNIX__)
    struct timeval tv;
    gettimeofday(&tv, 0);
    struct timespec ti;

    ti.tv_nsec = (tv.tv_usec + (amountMS % 1000) * 1000) * 1000;
    ti.tv_sec = tv.tv_sec + (amountMS / 1000) + (ti.tv_nsec / 1000000000);
    ti.tv_nsec %= 1000000000;
    thread_sleep(&ti);
#else
#error "unsupported OS"
#endif

}

bool
CacheEntryLocker::lookupAndSetStatusInternal(bool hasWriteRights, std::size_t timeSpentWaitingForPendingEntryMS, std::size_t timeout)
{

    // Look-up the cache
    MemorySegmentEntryHeader* cacheEntry = _imp->bucket->tryCacheLookupImpl(_imp->hashStr);

    // Flag set to true if readFromSharedMemoryEntry failed
    bool deserializeFailed = false;

    if (!cacheEntry) {
        return false;
    }


    if (cacheEntry->status == MemorySegmentEntryHeader::eEntryStatusNull) {
        // The entry was aborted by a thread. If we have write rights, takeover the entry
        // otherwise, wait for the 2nd look-up under the Write lock to do it.
        if (!hasWriteRights) {
            return false;
        }
#ifdef CACHE_TRACE_ENTRY_ACCESS
        qDebug() << _imp->hashStr.c_str() << ": entry found but NULL, thread" << QThread::currentThread() << "is taking over the entry";
#endif
    }

    if (cacheEntry->status == MemorySegmentEntryHeader::eEntryStatusPending) {

        // After a certain number of lookups, if the cache entry is still locked by another thread,
        // we take-over the entry, to ensure the entry was not left abandonned.
        // ensure cache entry was not left abandonned.
        if (timeSpentWaitingForPendingEntryMS < timeout || timeout == INT_MAX) {
            _imp->status = eCacheEntryStatusComputationPending;
#ifdef CACHE_TRACE_ENTRY_ACCESS
            qDebug() << _imp->hashStr.c_str() << ": entry pending, sleeping...";
#endif

            return true;
        }

        // We need write rights to take over the entry
        if (!hasWriteRights) {
            return false;
        }
#ifdef CACHE_TRACE_ENTRY_ACCESS
        qDebug() << _imp->hashStr.c_str() << ": entry pending timeout, thread" << QThread::currentThread() << "is taking over the entry";
#endif
    }

    // Deserialize the entry and update the status
    if (cacheEntry->status == MemorySegmentEntryHeader::eEntryStatusReady) {
        deserializeFailed = !_imp->bucket->readFromSharedMemoryEntryImpl(cacheEntry, _imp->processLocalEntry, _imp->hashStr, &_imp->status);
    } else {
        assert(hasWriteRights);
        cacheEntry->status = MemorySegmentEntryHeader::eEntryStatusPending;
        _imp->status = eCacheEntryStatusMustCompute;
    }

    if (deserializeFailed) {
        // If the entry failed to deallocate or is not of the type of the process local entry
        // we have to remove it from the cache.
        // However we cannot do so under the read lock, we must take the write lock.
        // So do it below in the 2nd lookup attempt.
        if (hasWriteRights) {
            _imp->bucket->deallocateCacheEntryImpl(cacheEntry,  _imp->hashStr);
        }
        return false;
    }

    // If the entry is still pending, that means the thread that originally should have computed this entry failed to do so.
    // If we were in waitForPendingEntry(), we now have the lock on the entry, thus change the status
    // to eCacheEntryStatusMustCompute to indicate that we must compute the entry now.
    // If we are looking up the first time, then we keep the status to pending, the caller will
    // just have to call waitForPendingEntry()
    switch (_imp->status) {
        case eCacheEntryStatusComputationPending:
        case eCacheEntryStatusMustCompute: {
#ifdef CACHE_TRACE_ENTRY_ACCESS
            qDebug() << _imp->hashStr.c_str() << ": got entry but it has to be computed";
#endif
        }   break;
        case eCacheEntryStatusCached:
        {
            assert(!deserializeFailed);
            // We found in cache, nothing to do
#ifdef CACHE_TRACE_ENTRY_ACCESS
            qDebug() << _imp->hashStr.c_str() << ": entry cached";
#endif
        }   break;
    } // switch(_imp->status)
    return true;
} // lookupAndSetStatusInternal

void
CacheEntryLocker::lookupAndSetStatus(boost::scoped_ptr<SharedMemoryReader>& shmAccess, std::size_t timeSpentWaitingForPendingEntryMS, std::size_t timeout)
{
    
    // Get the bucket corresponding to the hash. This will dispatch threads in (hopefully) different
    // buckets
    if (!_imp->bucket) {
        U64 hash = _imp->processLocalEntry->getHashKey();
        _imp->bucket = &_imp->cache->_imp->buckets[Cache::getBucketCacheBucketIndex(hash)];
    }

    {
        // Take the read lock: many threads/processes can try read at the same time
        boost::scoped_ptr<ReadLock> readLock;
        if (!createTimedLock<ReadLock>(readLock, &_imp->cache->_imp->ipc->bucketsData[_imp->bucket->bucketIndex].tocData.segmentMutex)) {
            _imp->status = eCacheEntryStatusComputationPending;
            return;
        }

        boost::scoped_ptr<WriteLock> writeLock;
        // Every time we take the lock, we must ensure the memory mapping is ok
        if (!_imp->bucket->isToCFileMappingValid()) {
            // Remove the read lock, and take a write lock.
            // This could allow other threads to run in-between, but we don't care since nothing happens.
            readLock.reset();

            if (!createTimedLock<WriteLock>(writeLock, &_imp->cache->_imp->ipc->bucketsData[_imp->bucket->bucketIndex].tocData.segmentMutex)) {
                _imp->status = eCacheEntryStatusComputationPending;
                return;
            }

            _imp->bucket->ensureToCFileMappingValid(*writeLock, 0);
        }

        // This function succeeds either if
        // 1) The entry is cached and could be deserialized
        // 2) The entry is pending and thus the caller should call waitForPendingEntry
        // 3) The entry is not computed and thus the caller should compute the entry and call insertInCache
        //
        // This function returns false if the thread must take over the entry computation or the deserialization failed.
        // In any case, it should do so under the write lock below.
        if (lookupAndSetStatusInternal(false /*hasWriteRights*/, timeSpentWaitingForPendingEntryMS, timeout)) {
            return;
        }
    } // ReadLock(tocData.segmentMutex)

    // Concurrency resumes!

    assert(_imp->status == eCacheEntryStatusMustCompute ||
           _imp->status == eCacheEntryStatusComputationPending);


    // Ok we are in one of those 2 cases
    // - we did not find an entry matching the hash
    // - we found a matching entry but the object was invalid
    //
    // In any cases, first take an upgradable lock and repeat the look-up.
    // Only a single thread/process can take the upgradable lock.
    {
        boost::scoped_ptr<UpgradableLock> upgradableLock;
        createLockAndEnsureSHM<UpgradableLock>(_imp->cache->_imp.get(), shmAccess, upgradableLock, &_imp->cache->_imp->ipc->bucketsData[_imp->bucket->bucketIndex].tocData.segmentMutex);


        boost::scoped_ptr<WriteLock> writeLock;

        // Every time we take the lock, we must ensure the memory mapping is ok
        if (!_imp->bucket->isToCFileMappingValid()) {
            writeLock.reset(new WriteLock(boost::move(*upgradableLock)));
            _imp->bucket->ensureToCFileMappingValid(*writeLock, 0);
        }

        // This function only fails if the entry must be computed anyway.
        if (lookupAndSetStatusInternal(true /*hasWriteRights*/, timeSpentWaitingForPendingEntryMS, timeout)) {
            return;
        }
        assert(_imp->status == eCacheEntryStatusMustCompute);
        {
            // We need to upgrade the lock to a write lock. This will wait until all other threads have released their
            // read lock.
            if (!writeLock) {
                writeLock.reset(new WriteLock(boost::move(*upgradableLock)));
            }

            // Now we are the only thread in this portion.

            // Create the MemorySegmentEntry if it does not exist
            void_allocator allocator(_imp->bucket->tocFileManager->get_segment_manager());
#ifdef CACHE_TRACE_ENTRY_ACCESS
            qDebug() << _imp->hashStr.c_str() << ": construct entry";
#endif
            MemorySegmentEntryHeader* cacheEntry = _imp->bucket->tocFileManager->construct<MemorySegmentEntryHeader>(_imp->hashStr.c_str())(allocator);
            cacheEntry->pluginID.append(_imp->processLocalEntry->getKey()->getHolderPluginID().c_str());

            cacheEntry->version = NATRON_MEMORY_SEGMENT_ENTRY_HEADER_VERSION;

            // Lock the statusMutex: this will lock-out other threads interested in this entry.
            // This mutex is unlocked in deallocateCacheEntryImpl() or in insertInCache()
            // We must get the lock since we are the first thread to create it and we own the write lock on the segmentMutex


            assert(cacheEntry->status == MemorySegmentEntryHeader::eEntryStatusNull);

            // Set the status of the entry to pending because we (this thread) are going to compute it.
            // Other fields of the entry will be set once it is done computed in insertInCache()
            cacheEntry->status = MemorySegmentEntryHeader::eEntryStatusPending;

            
        } // writeLock
    } // upgradableLock
    // Concurrency resumes here!

} // lookupAndSetStatus

CacheEntryBasePtr
CacheEntryLocker::getProcessLocalEntry() const
{
    return _imp->processLocalEntry;
}

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

    // Public function, the SHM must not be locked.
    boost::scoped_ptr<SharedMemoryReader> shmAccess(new SharedMemoryReader(_imp->cache->_imp.get()));

    {
        // Take write lock on the bucket
        boost::scoped_ptr<WriteLock> writeLock;
        createLockAndEnsureSHM<WriteLock>(_imp->cache->_imp.get(), shmAccess, writeLock, &_imp->cache->_imp->ipc->bucketsData[_imp->bucket->bucketIndex].tocData.segmentMutex);

        // Ensure the memory mapping is ok. We grow the file so it contains at least the size needed by the entry
        // plus some metadatas required management algorithm store its own memory housekeeping data.
        const std::size_t entrySize = _imp->processLocalEntry->getMetadataSize();
        if (!_imp->bucket->isToCFileMappingValid()) {
            _imp->bucket->ensureToCFileMappingValid(*writeLock, entrySize);
        }
        

        // Fetch the entry. It must be here!
        MemorySegmentEntryHeader* cacheEntry = _imp->bucket->tryCacheLookupImpl(_imp->hashStr);
        assert(cacheEntry);
        if (!cacheEntry) {
            throw std::logic_error("CacheEntryLocker::insertInCache");
        }

        // The status of the memory segment entry should be pending because we are the thread computing it.
        // All other threads are waiting.
        assert(cacheEntry->status == MemorySegmentEntryHeader::eEntryStatusPending);

        // The cacheEntry fields should be uninitialized
        // This may throw an exception if out of memory or if the getMetadataSize function does not return
        // enough memory to encode all the data.
        try {

            // Allocate memory for the entry metadatas
            cacheEntry->size = entrySize;


            // Serialize the meta-datas in the memory segment
            // If the entry also requires tile aligned data storage, allocate a tile now
            {
                boost::scoped_ptr<ReadLock> tileReadLock;
                boost::scoped_ptr<WriteLock> tileWriteLock;
                char* tileDataPtr = 0;
                if (_imp->processLocalEntry->isStorageTiled()) {
                    // First try to check if the tile aligned mapping is valid with a readlock
                    bool tileMappingValid;

                    {
                        createLockNoTimeout<ReadLock>(tileReadLock, &_imp->cache->_imp->ipc->bucketsData[_imp->bucket->bucketIndex].tileData.segmentMutex);

                        tileMappingValid = _imp->bucket->isTileFileMappingValid();
                        if (tileMappingValid) {
                            // Check that there's at least one free tile
                            tileMappingValid = _imp->bucket->ipc->freeTiles.size() > 0;
                        }
                    }

                    // No free tiles or mapping invalid, remap and grow if necessary.
                    if (!tileMappingValid) {
                        // If the tile mapping is invalid, take a write lock on the tile mapping and ensure it is valid
                        tileReadLock.reset();
                        createLockNoTimeout<WriteLock>(tileWriteLock, &_imp->cache->_imp->ipc->bucketsData[_imp->bucket->bucketIndex].tileData.segmentMutex);


                        _imp->bucket->ensureTileMappingValid(*tileWriteLock, NATRON_TILE_SIZE_BYTES);
                    }
                    assert(_imp->bucket->ipc->freeTiles.size() > 0);
                    int freeTileIndex;
                    {
                        set_size_t_ExternalSegment::iterator freeTileIt = _imp->bucket->ipc->freeTiles.begin();
                        freeTileIndex = *freeTileIt;
                        _imp->bucket->ipc->freeTiles.erase(freeTileIt);
#ifdef CACHE_TRACE_TILES_ALLOCATION
                        qDebug() << "Bucket" << _imp->bucket->bucketIndex << ": removing tile" << freeTileIndex << " Nb free tiles left:" << _imp->bucket->ipc->freeTiles.size();
#endif
                    }
                    tileDataPtr = _imp->bucket->tileAlignedFile->data() + freeTileIndex * NATRON_TILE_SIZE_BYTES;

                    // Set the tile index on the entry so we can free it afterwards.
                    cacheEntry->tileCacheIndex = freeTileIndex;
                } // tileWriteLock
                
                
                _imp->processLocalEntry->toMemorySegment(_imp->bucket->tocFileManager.get(), _imp->hashStr + "Data", &cacheEntry->entryDataPointerList, tileDataPtr);

            }


            // Insert the hash in the LRU linked list
            // Lock the LRU list mutex
            {
                boost::scoped_ptr<bip::scoped_lock<bip::interprocess_mutex> > lruWriteLock;
                createLockNoTimeout<bip::scoped_lock<bip::interprocess_mutex> >(lruWriteLock, &_imp->bucket->ipc->lruListMutex);


                cacheEntry->lruIterator = static_cast<LRUListNode*>(_imp->bucket->tocFileManager->allocate(sizeof(LRUListNode)));
                cacheEntry->lruIterator->prev = 0;
                cacheEntry->lruIterator->next = 0;
                cacheEntry->lruIterator->hash = _imp->processLocalEntry->getHashKey();

                if (!_imp->bucket->ipc->lruListBack) {
                    assert(!_imp->bucket->ipc->lruListFront);
                    // The list is empty, initialize to this node
                    _imp->bucket->ipc->lruListFront = cacheEntry->lruIterator;
                    _imp->bucket->ipc->lruListBack = cacheEntry->lruIterator;
                    assert(!_imp->bucket->ipc->lruListFront->prev && !_imp->bucket->ipc->lruListFront->next);
                    assert(!_imp->bucket->ipc->lruListBack->prev && !_imp->bucket->ipc->lruListBack->next);
                } else {
                    // Append to the tail of the list
                    assert(_imp->bucket->ipc->lruListFront && _imp->bucket->ipc->lruListBack);

                    insertLinkedListNode(cacheEntry->lruIterator, _imp->bucket->ipc->lruListBack, bip::offset_ptr<LRUListNode>(0));
                    // Update back node
                    _imp->bucket->ipc->lruListBack = cacheEntry->lruIterator;
                    
                }
            } // lruWriteLock
            cacheEntry->status = MemorySegmentEntryHeader::eEntryStatusReady;

            _imp->status = eCacheEntryStatusCached;

#ifdef CACHE_TRACE_ENTRY_ACCESS
            qDebug() << _imp->hashStr.c_str() << ": entry inserted in cache";
#endif

        } catch (...) {

            // Set the status to eCacheEntryStatusMustCompute so that the destructor deallocates the entry.
            _imp->status = eCacheEntryStatusMustCompute;
        }


    } // writeLock

    // Concurrency resumes!
    
} // insertInCache

CacheEntryLocker::CacheEntryStatusEnum
CacheEntryLocker::waitForPendingEntry(std::size_t timeout)
{
    // Public function, the SHM must not be locked.
    boost::scoped_ptr<SharedMemoryReader> shmAccess(new SharedMemoryReader(_imp->cache->_imp.get()));

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

    std::size_t timeSpentWaitingForPendingEntryMS = 0;
    static const std::size_t timeToWaitMS = 200;

    do {
        // Look up the cache, but first take the lock on the MemorySegmentEntry
        // that will be released once another thread unlocked it in insertInCache
        // or the destructor.
        lookupAndSetStatus(shmAccess, timeSpentWaitingForPendingEntryMS, timeout);

        if (_imp->status == eCacheEntryStatusComputationPending) {
            timeSpentWaitingForPendingEntryMS += timeToWaitMS;
            sleep_milliseconds(timeToWaitMS);
        }

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
    // insertInCache().
    // Release the entry by setting its status to MemorySegmentEntryHeader::eEntryStatusNull, indicating
    // that another thread has to take over and compute it.

    if (_imp->status == eCacheEntryStatusMustCompute) {
        
        boost::scoped_ptr<SharedMemoryReader> shmAccess(new SharedMemoryReader(_imp->cache->_imp.get()));

        boost::scoped_ptr<WriteLock> writeLock;
        createLockAndEnsureSHM<WriteLock>(_imp->cache->_imp.get(), shmAccess, writeLock, &_imp->cache->_imp->ipc->bucketsData[_imp->bucket->bucketIndex].tocData.segmentMutex);

        // Every time we take the lock, we must ensure the memory mapping is ok
        if (!_imp->bucket->isToCFileMappingValid()) {
            _imp->bucket->ensureToCFileMappingValid(*writeLock, 0);
        }


        MemorySegmentEntryHeader* cacheEntry = _imp->bucket->tryCacheLookupImpl(_imp->hashStr);
        assert(cacheEntry);
        if (!cacheEntry) {
            throw std::logic_error("CacheEntryLocker::insertInCache");
        }
        assert(cacheEntry->status == MemorySegmentEntryHeader::eEntryStatusPending);

#ifdef CACHE_TRACE_ENTRY_ACCESS
        qDebug() << _imp->hashStr.c_str() << ": entry aborted";
#endif
        cacheEntry->status = MemorySegmentEntryHeader::eEntryStatusNull;

    }
} // ~CacheEntryLocker


Cache::Cache()
: boost::enable_shared_from_this<Cache>()
, _imp(new CachePrivate(this))
{

}

Cache::~Cache()
{

}

std::string
CachePrivate::getSharedMemoryName() const
{

    std::stringstream ss;
    ss << NATRON_APPLICATION_NAME << NATRON_CACHE_DIRECTORY_NAME  << "SHM";
    return ss.str();

}

std::size_t
CachePrivate::getSharedMemorySize() const
{
    // Allocate 500KB rounded to page size for the global data.
    // This gives the global memory segment a little bit of room for its own housekeeping of memory.
    std::size_t pageSize = bip::mapped_region::get_page_size();
    std::size_t desiredSize = 500 * 1024;
    desiredSize = std::ceil(desiredSize / (double)pageSize) * pageSize;
    return desiredSize;
}

CachePtr
Cache::create()
{
    CachePtr ret(new Cache);

    ret->_imp->initializeCacheDirPath();
    ret->_imp->ensureCacheDirectoryExists();


    // Open or create the file lock
    {

        std::string cacheDir;
        {
            std::stringstream ss;
            ss << ret->_imp->directoryContainingCachePath << "/" << NATRON_CACHE_DIRECTORY_NAME << "/";
            cacheDir = ss.str();
        }
        std::string fileLockFile = cacheDir + "Lock";

        // Ensure the file lock file exists in read/write mode
        {
            FStreamsSupport::ofstream ofile;
            FStreamsSupport::open(&ofile, fileLockFile);
            if (!ofile || fileLockFile.empty()) {
                assert(false);
                std::string message = "Failed to open file: " + fileLockFile;
                throw std::runtime_error(message);
            }

            try {
                ret->_imp->globalMemorySegmentFileLock.reset(new bip::file_lock(fileLockFile.c_str()));
            } catch (...) {
                assert(false);
                throw std::runtime_error("Failed to initialize shared memory file lock, exiting.");
            }
        }
    }

    // Take the file lock in write mode:
    //      - If it succeeds, that means no other process is active: We remove the globalMemorySegment shared memory segment
    //        and create a new one, to ensure no lock was left in a bad state. Then we release the file lock
    //      - If it fails, another process is still actively using the globalMemorySegment shared memory: it must still be valid
    bool gotFileLock = ret->_imp->globalMemorySegmentFileLock->try_lock();

    // Create 2 semaphores used to ensure the integrity of the shared memory segment holding interprocess mutexes.
    std::string semValidStr, semInvalidStr;
    {
        std::string semBaseName;
        {
            std::stringstream ss;
            ss << NATRON_APPLICATION_NAME << NATRON_CACHE_DIRECTORY_NAME;
            semBaseName = ss.str();
        }
        semValidStr = std::string(semBaseName + "nSHMValidSem");
        semInvalidStr = std::string(semBaseName + "nSHMInvalidSem");
    }
    try {
        if (gotFileLock) {
            // Remove the semaphore if we are the only process alive to ensure its state.
            bip::named_semaphore::remove(semValidStr.c_str());
        }
        ret->_imp->nSHMValidSem.reset(new bip::named_semaphore(bip::open_or_create,
                                                               semValidStr.c_str(),
                                                               0));
        if (gotFileLock) {
            // Remove the semaphore if we are the only process alive to ensure its state.
            bip::named_semaphore::remove(semInvalidStr.c_str());
        }
        ret->_imp->nSHMInvalidSem.reset(new bip::named_semaphore(bip::open_or_create,
                                                                 semInvalidStr.c_str(),
                                                                 0));
    } catch (...) {
        assert(false);
        throw std::runtime_error("Failed to initialize named semaphores, exiting.");
    }


    // Create the main memory segment containing the CachePrivate::IPCData
    {

        std::size_t desiredSize = ret->_imp->getSharedMemorySize();
        std::string sharedMemoryName = ret->_imp->getSharedMemoryName();
        try {
            if (gotFileLock) {
                bip::shared_memory_object::remove(sharedMemoryName.c_str());
            }
            ret->_imp->globalMemorySegment.reset(new bip::managed_shared_memory(bip::open_or_create, sharedMemoryName.c_str(), desiredSize));
            ret->_imp->ipc = ret->_imp->globalMemorySegment->find_or_construct<CachePrivate::IPCData>("CacheData")();
        } catch (...) {
            assert(false);
            bip::shared_memory_object::remove(sharedMemoryName.c_str());
            throw std::runtime_error("Failed to initialize managed shared memory, exiting.");
        }
        
    }

    if (gotFileLock) {
        ret->_imp->globalMemorySegmentFileLock->unlock();
    }

    // Indicate that we use the shared memory by taking the file lock in read mode.
    ret->_imp->globalMemorySegmentFileLock->try_lock_sharable();
    
    // Open each bucket individual memory segment.
    // They are not created in shared memory but in a memory mapped file instead
    // to be persistent when the OS shutdown.
    // Each segment controls the table of content of the bucket.

    boost::scoped_ptr<SharedMemoryReader> shmReader(new SharedMemoryReader(ret->_imp.get()));
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

            // Take the ToC mapping mutex to ensure that the ToC file is valid
            boost::scoped_ptr<WriteLock> writeLock;
            createLockAndEnsureSHM<WriteLock>(ret->_imp.get(), shmReader, writeLock, &ret->_imp->ipc->bucketsData[i].tocData.segmentMutex);


            ret->_imp->buckets[i].tocFile->open(tocFilePath, MemoryFile::eFileOpenModeOpenOrCreate);

            // Ensure the mapping is valid. This will grow the file the first time.
            ret->_imp->buckets[i].ensureToCFileMappingValid(*writeLock, 0);
        }

        // Open the memory-mapped file used for tiled entries data storage.
        {
            std::string tileCacheFilePath = bucketDirPath.toStdString() + "TileCache";

            ret->_imp->buckets[i].tileAlignedFile.reset(new MemoryFile);

            // Take the ToC mapping mutex and register this process amongst the valid mapping
            boost::scoped_ptr<WriteLock> writeLock;
            createLockAndEnsureSHM<WriteLock>(ret->_imp.get(), shmReader, writeLock, &ret->_imp->ipc->bucketsData[i].tileData.segmentMutex);

            ret->_imp->buckets[i].tileAlignedFile->open(tileCacheFilePath, MemoryFile::eFileOpenModeOpenOrCreate);

            // Ensure the mapping is valid. This will grow the file the first time.
            ret->_imp->buckets[i].ensureTileMappingValid(*writeLock, 0);

        }

    } // for each bucket


    return ret;
} // create

void
CachePrivate::ensureSharedMemoryIntegrity()
{
    // Any operation taking the segmentMutex in the shared memory, must do so with a timeout so we can avoid deadlocks:
    // If a process crashes whilst the segmentMutex is taken, the file lock is ensured to be released but the
    // segmentMutex will remain taken, deadlocking any other process.

    // Multiple threads in this process can time-out, however we just need to remap the shared memory once.
    // Ensure that no other thread is reading
    QWriteLocker processLocalLocker(&nThreadsTimedOutFailedMutex);

    // Mark that this thread is in a timeout operation.
    ++nThreadsTimedOutFailed;

    if (nThreadsTimedOutFailed == 1) {
        // If we are the first thread in a timeout, handle it.

        // Unmap the shared memory segment. This is safe to do since we have the nThreadsTimedOutFailedMutex write lock
        globalMemorySegment.reset();

        // The mapping for this process is no longer invalid
        nSHMInvalidSem->post();

        // We release the read lock taken on the globalMemorySegmentFileLock
        globalMemorySegmentFileLock->unlock();

        {
            // We take the file lock in write mode.
            // The lock is guaranteed to be taken at some point since any active process will eventually timeout on the segmentMutex and release
            // their read lock on the globalMemorySegmentFileLock in the unlock call above.
            // We are sure that when the lock is taken, every process has its shared memory segment unmapped.
            bip::scoped_lock<bip::file_lock> writeLocker(*globalMemorySegmentFileLock);

            std::string sharedMemoryName = getSharedMemoryName();
            std::size_t sharedMemorySize = getSharedMemorySize();

            if (!nSHMValidSem->try_wait()) {
                // We are the first process to take the write lock.
                // We know at this point that any other process has released its read lock on the globalMemorySegmentFileLock
                // and that the globalMemorySegment is no longer mapped anywhere.
                // We thus remove the globalMemorySegment and re-create it and remap it.
                bool ok = bip::shared_memory_object::remove(sharedMemoryName.c_str());

                // The call should succeed since no one else should be using it
                assert(ok);
                (void)ok;

            } else {
                // We are not the first process to take the write lock, hence the globalMemorySegment
                // has been re-created already, so just map it.
                // Increment what was removed in try_wait()
                nSHMValidSem->post();
            }

            try {
                globalMemorySegment.reset(new bip::managed_shared_memory(bip::open_or_create, sharedMemoryName.c_str(), sharedMemorySize));
                ipc = globalMemorySegment->find_or_construct<CachePrivate::IPCData>("CacheData")();
            } catch (...) {
                assert(false);
                bip::shared_memory_object::remove(sharedMemoryName.c_str());
                throw std::runtime_error("Failed to initialize managed shared memory, exiting.");
            }


            // Indicate that we mapped the shared memory segment
            nSHMValidSem->post();

            // Decrement the post() that we made earlier
            nSHMInvalidSem->wait();
            
            // Unlock the file lock
        } // writeLocker

        // When the write lock is released we cannot take the globalMemorySegmentFileLock in read mode yet, we could block other processes that
        // are still waiting for the write lock.
        // We must wait that every other process has a valid mapping.

        //  nSHMInvalid.try_wait() will return false when all processes have been remapped.
        //  If it returns true, that means another process is still in-between steps 4 and 8, thus we post
        //  what we decremented in try_wait and re-try again.
        while(nSHMInvalidSem->try_wait()) {
            nSHMInvalidSem->post();
        }

    } // nThreadsTimedOutFailed == 1


    // Wait for all timed out threads to go through the the timed out process.
    --nThreadsTimedOutFailed;
    while (nThreadsTimedOutFailed > 0) {
        nThreadsTimedOutFailedCond.wait(&nThreadsTimedOutFailedMutex);
    }

} // ensureSharedMemoryIntegrity

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
                _imp->maximumGLTextureSize = size;
            }   break;
            case eStorageModeNone:
                assert(false);
                break;
        }
    }

    // Clear exceeding entries if we are shrinking the cache.
    if (size < curSize) {
        evictLRUEntries(0);
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

    boost::scoped_ptr<SharedMemoryReader> shmReader(new SharedMemoryReader(_imp.get()));

    // Lock for read the size lock
    boost::scoped_ptr<bip::sharable_lock<bip::interprocess_sharable_mutex> > locker;
    createLockAndEnsureSHM<bip::sharable_lock<bip::interprocess_sharable_mutex> >(_imp.get(), shmReader, locker, &_imp->ipc->sizeLock);
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
    boost::scoped_ptr<SharedMemoryReader> shmReader(new SharedMemoryReader(this));

    // Lock for writing
    boost::scoped_ptr<bip::scoped_lock<bip::interprocess_sharable_mutex> > locker;
    createLockAndEnsureSHM<bip::scoped_lock<bip::interprocess_sharable_mutex> >(this, shmReader, locker, &ipc->sizeLock);

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

    {
        QDir d = QDir::root();
        d.mkpath(userDirectoryCache);
    }

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

CacheEntryLockerPtr
Cache::get(const CacheEntryBasePtr& entry) const
{
    CachePtr thisShared = boost::const_pointer_cast<Cache>(shared_from_this());
    return CacheEntryLocker::create(thisShared, entry);
} // get

bool
Cache::hasCacheEntryForHash(U64 hash) const
{

    std::string hashStr = CacheEntryKeyBase::hashToString(hash);

    int bucketIndex = Cache::getBucketCacheBucketIndex(hash);
    CacheBucket& bucket = _imp->buckets[bucketIndex];

    boost::scoped_ptr<SharedMemoryReader> shmReader(new SharedMemoryReader(_imp.get()));

    boost::scoped_ptr<ReadLock> readLock;
    boost::scoped_ptr<WriteLock> writeLock;

    createLockAndEnsureSHM<ReadLock>(_imp.get(), shmReader, readLock, &_imp->ipc->bucketsData[bucketIndex].tocData.segmentMutex);

    // First take a read lock and check if the mapping is valid. Otherwise take a write lock
    if (!bucket.isToCFileMappingValid()) {
        readLock.reset();

        createLockNoTimeout<WriteLock>(writeLock, &_imp->ipc->bucketsData[bucketIndex].tocData.segmentMutex);

        bucket.ensureToCFileMappingValid(*writeLock, 0);
    }

    std::pair<MemorySegmentEntryHeader*, ExternalSegmentType::size_type> found = bucket.tocFileManager->find<MemorySegmentEntryHeader>(hashStr.c_str());
    return found.second != 0;

} // hasCacheEntryForHash

void
Cache::notifyMemoryAllocated(std::size_t size, StorageModeEnum storage)
{
    _imp->incrementCacheSize(size, storage);

    // We just allocateg something, ensure the cache size remains reasonable.
    // We cannot block here until the memory stays contained in the user requested memory portion:
    // if we would do so, then it could deadlock: Natron could require more memory than what
    // the user requested to render just one node.
    evictLRUEntries(size);
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


    U64 hash = entry->getHashKey();
    int bucketIndex = Cache::getBucketCacheBucketIndex(hash);
    std::string hashStr = CacheEntryKeyBase::hashToString(hash);

    CacheBucket& bucket = _imp->buckets[bucketIndex];

    boost::scoped_ptr<SharedMemoryReader> shmReader(new SharedMemoryReader(_imp.get()));

    // Take the bucket lock in write mode
    {
        boost::scoped_ptr<WriteLock> writeLock;
        createLockAndEnsureSHM<WriteLock>(_imp.get(), shmReader, writeLock, &_imp->ipc->bucketsData[bucketIndex].tocData.segmentMutex);

        // Ensure the file mapping is OK
        bucket.ensureToCFileMappingValid(*writeLock, 0);

        // Deallocate the memory taken by the cache entry in the ToC
        {
            MemorySegmentEntryHeader* cacheEntry = bucket.tryCacheLookupImpl(hashStr);
            if (cacheEntry) {
                bucket.deallocateCacheEntryImpl(cacheEntry, hashStr);
            }
        }
    }

} // removeEntry


void
Cache::clear()
{


    _imp->ensureSharedMemoryIntegrity();

    boost::scoped_ptr<SharedMemoryReader> shmReader(new SharedMemoryReader(_imp.get()));

    for (int bucket_i = 0; bucket_i < NATRON_CACHE_BUCKETS_COUNT; ++bucket_i) {
        CacheBucket& bucket = _imp->buckets[bucket_i];

        // Close and re-create the memory mapped files
        {
            boost::scoped_ptr<WriteLock> writeLock;
            createLockAndEnsureSHM<WriteLock>(_imp.get(), shmReader, writeLock, &_imp->ipc->bucketsData[bucket_i].tocData.segmentMutex);

            std::string tocFilePath = bucket.tocFile->path();
            bucket.tocFile->remove();
            bucket.tocFile->open(tocFilePath, MemoryFile::eFileOpenModeOpenTruncateOrCreate);

            bucket.ensureToCFileMappingValid(*writeLock, 0);
        }
        {
            boost::scoped_ptr<WriteLock> writeLock;
            createLockAndEnsureSHM<WriteLock>(_imp.get(), shmReader, writeLock, &_imp->ipc->bucketsData[bucket_i].tileData.segmentMutex);

            std::string tileFilePath = bucket.tileAlignedFile->path();
            bucket.tileAlignedFile->remove();
            bucket.tileAlignedFile->open(tileFilePath, MemoryFile::eFileOpenModeOpenTruncateOrCreate);


            bucket.ensureTileMappingValid(*writeLock, 0);
        }

    } // for each bucket

} // clear()

void
Cache::evictLRUEntries(std::size_t nBytesToFree)
{
    std::size_t maxSize = getMaximumCacheSize(eStorageModeDisk);

    // If max size == 0 then there's no limit.
    if (maxSize == 0) {
        return;
    }

    if (nBytesToFree >= maxSize) {
        maxSize = 0;
    } else {
        maxSize = maxSize - nBytesToFree;
    }

    std::size_t curSize = getCurrentSize(eStorageModeDisk);

    bool mustEvictEntries = curSize > maxSize;

    while (mustEvictEntries) {
        
        bool foundBucketThatCanEvict = false;

        boost::scoped_ptr<SharedMemoryReader> shmReader(new SharedMemoryReader(_imp.get()));

        // Check each bucket
        for (int bucket_i = 0; bucket_i < NATRON_CACHE_BUCKETS_COUNT; ++bucket_i) {
            CacheBucket& bucket = _imp->buckets[bucket_i];

            {
                // Lock for writing
                boost::scoped_ptr<WriteLock> writeLock;
                createLockAndEnsureSHM<WriteLock>(_imp.get(), shmReader, writeLock, &_imp->ipc->bucketsData[bucket_i].tocData.segmentMutex);

                // Ensure the mapping
                bucket.ensureToCFileMappingValid(*writeLock, 0);

                U64 entryHash = 0;
                {
                    // Lock the LRU list
                    boost::scoped_ptr<bip::scoped_lock<bip::interprocess_mutex> > lruWriteLock;
                    createLockNoTimeout<bip::scoped_lock<bip::interprocess_mutex> >(lruWriteLock, &bucket.ipc->lruListMutex);

                    // The least recently used entry is the one at the front of the linked list
                    if (bucket.ipc->lruListFront) {
                        entryHash = bucket.ipc->lruListFront->hash;
                    }
                }
                if (!entryHash) {
                    continue;
                }

                // Deallocate the memory taken by the cache entry in the ToC
                {
                    std::string hashStr = CacheEntryKeyBase::hashToString(entryHash);
                    MemorySegmentEntryHeader* cacheEntry = bucket.tryCacheLookupImpl(hashStr);
                    if (cacheEntry) {
                        // We evicted one, decrease the size
                        curSize -= cacheEntry->size;

                        // Also decrease the size if this entry held a tile
                        if (cacheEntry->tileCacheIndex != -1) {
                            curSize -= NATRON_TILE_SIZE_BYTES;
                        }
                        bucket.deallocateCacheEntryImpl(cacheEntry, hashStr);
                    }
                }

            }

            foundBucketThatCanEvict = true;
            
        } // for each bucket

        // No bucket can be evicted anymore, exit.
        if (!foundBucketThatCanEvict) {
            break;
        }

        // Update mustEvictEntries for next iteration
        mustEvictEntries = curSize > maxSize;

    } // curSize > maxSize

} // evictLRUEntries

void
Cache::getMemoryStats(std::map<std::string, CacheReportInfo>* infos) const
{

    boost::scoped_ptr<SharedMemoryReader> shmReader(new SharedMemoryReader(_imp.get()));

    for (int bucket_i = 0; bucket_i < NATRON_CACHE_BUCKETS_COUNT; ++bucket_i) {
        CacheBucket& bucket = _imp->buckets[bucket_i];

        boost::scoped_ptr<ReadLock> readLock;
        boost::scoped_ptr<WriteLock> writeLock;
        createLockAndEnsureSHM<ReadLock>(_imp.get(), shmReader, readLock, &_imp->ipc->bucketsData[bucket_i].tocData.segmentMutex);

        // First take a read lock and check if the mapping is valid. Otherwise take a write lock
        if (!bucket.isToCFileMappingValid()) {
            readLock.reset();
            createLockAndEnsureSHM<WriteLock>(_imp.get(), shmReader, writeLock, &_imp->ipc->bucketsData[bucket_i].tocData.segmentMutex);

            bucket.ensureToCFileMappingValid(*writeLock, 0);
        }

        try {
            // Cycle through the whole LRU list
            bip::offset_ptr<LRUListNode> it = bucket.ipc->lruListFront;
            while (it) {
                std::string hashStr = CacheEntryKeyBase::hashToString(it->hash);
                MemorySegmentEntryHeader* cacheEntry = bucket.tryCacheLookupImpl(hashStr);
                assert(cacheEntry);
                if (cacheEntry && !cacheEntry->pluginID.empty()) {

                    std::string pluginID(cacheEntry->pluginID.c_str());
                    CacheReportInfo& entryData = (*infos)[pluginID];
                    ++entryData.nEntries;
                    entryData.nBytes += cacheEntry->size;
                    if (cacheEntry->tileCacheIndex != -1) {
                        entryData.nBytes += NATRON_TILE_SIZE_BYTES;
                    }
                    
                }
                it = it->next;
            }
        } catch (...) {
            // Exceptions can be thrown from the ExternalSegmentType ctor or readNamedSharedObject()
        }
    } // for each bucket
} // getMemoryStats

void
Cache::flushCacheOnDisk(bool async)
{
    boost::scoped_ptr<SharedMemoryReader> shmReader(new SharedMemoryReader(_imp.get()));
    for (int bucket_i = 0; bucket_i < NATRON_CACHE_BUCKETS_COUNT; ++bucket_i) {
        CacheBucket& bucket = _imp->buckets[bucket_i];

        {
            boost::scoped_ptr<ReadLock> readLock;
            boost::scoped_ptr<WriteLock> writeLock;
            createLockAndEnsureSHM<ReadLock>(_imp.get(), shmReader, readLock, &_imp->ipc->bucketsData[bucket_i].tocData.segmentMutex);

            // First take a read lock and check if the mapping is valid. Otherwise take a write lock
            if (!bucket.isToCFileMappingValid()) {
                readLock.reset();
                createLockAndEnsureSHM<WriteLock>(_imp.get(), shmReader, writeLock, &_imp->ipc->bucketsData[bucket_i].tocData.segmentMutex);

                // This function will flush for us.
                bucket.ensureToCFileMappingValid(*writeLock, 0);
            } else {
                bucket.tocFile->flush(async ? MemoryFile::eFlushTypeAsync : MemoryFile::eFlushTypeSync, NULL, 0);
            }



            boost::scoped_ptr<ReadLock> readLockTile;
            boost::scoped_ptr<WriteLock> writeLockTile;
            readLockTile.reset(new ReadLock(_imp->ipc->bucketsData[bucket_i].tileData.segmentMutex));

            // First take a read lock and check if the mapping is valid. Otherwise take a write lock
            if (!bucket.isTileFileMappingValid()) {
                readLockTile.reset();
                writeLockTile.reset(new WriteLock(_imp->ipc->bucketsData[bucket_i].tileData.segmentMutex));

                 // This function will flush for us.
                bucket.ensureTileMappingValid(*writeLock, 0);
            } else {
                flushTileMapping(bucket.tileAlignedFile, bucket.ipc->freeTiles);
            }
        }

    }
} // flushCacheOnDisk

NATRON_NAMESPACE_EXIT;

