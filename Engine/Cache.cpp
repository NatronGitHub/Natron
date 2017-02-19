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

#ifdef __NATRON_UNIX__
#include <time.h>
#endif

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
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/containers/flat_map.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp> // IPC regular mutex
#include <boost/interprocess/sync/scoped_lock.hpp> // IPC  scoped lock a regular mutex
#include <boost/interprocess/sync/interprocess_upgradable_mutex.hpp> // IPC  r-w mutex that can upgrade read right to write
#include <boost/interprocess/sync/interprocess_sharable_mutex.hpp> // IPC  r-w mutex
#include <boost/interprocess/sync/sharable_lock.hpp> // IPC  scoped lock a r-w mutex
#include <boost/interprocess/sync/upgradable_lock.hpp> // IPC  scope lock a r-w upgradable mutex
#include <boost/interprocess/sync/interprocess_condition_any.hpp> // IPC  wait cond with a r-w mutex
#include <boost/interprocess/sync/file_lock.hpp> // IPC  file lock
#include <boost/interprocess/sync/named_semaphore.hpp> // IPC  named semaphore
#include <boost/thread/mutex.hpp> // local mutex
#include <boost/thread/shared_mutex.hpp> // local r-w mutex
#include <boost/thread/locks.hpp>
#include <boost/thread/condition_variable.hpp>
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

// If we change the MemorySegmentEntryHeader struct, we must increment this version so we do not attempt to read an invalid structure.
#define NATRON_MEMORY_SEGMENT_ENTRY_HEADER_VERSION 1

// If defined, the cache can handle multiple processes accessing to the same cache concurrently, however
// the cache may not be placed in a network drive.
// If not defined, the cache supports only a single process writing/reading from the cache concurrently, other processes will resort
// in a process-local cache.
//#define NATRON_CACHE_INTERPROCESS_ROBUST


// After this amount of milliseconds, if a thread is not able to access a mutex, the cache is assumed to be inconsistent
#ifdef NATRON_CACHE_INTERPROCESS_ROBUST
#define NATRON_CACHE_INTERPROCESS_MUTEX_TIMEOUT_MS 10000
#endif

//#define CACHE_TRACE_ENTRY_ACCESS
//#define CACHE_TRACE_TIMEOUTS
//#define CACHE_TRACE_FILE_MAPPING
//#define CACHE_TRACE_TILES_ALLOCATION

namespace bip = boost::interprocess;


NATRON_NAMESPACE_ENTER;



// Cache integrity:
// ----------------
//
// Exposing the cache to multiple process can be harmful in multiple ways: a process can die
// in any instruction and may leave the program in an incoherent state. Other process have to deal
// with that. Hopefully this kind of situation is rare.
// E.G:
// A Natron process could very well crash whilst an interprocess mutex is taken: any subsequent attempt to lock
// the mutex would deadlock because of an abandonned mutex.
//
// Databases generally overcome this issue by using a file lock instead of a named mutex.
// The file lock has the interesting property that it lives as long as the process lives:
// From: http://www.boost.org/doc/libs/1_63_0/doc/html/interprocess/synchronization_mechanisms.html#interprocess.synchronization_mechanisms.file_lock.file_lock_whats_a_file_lock
/*A file locking is a class that has process lifetime.
 This means that if a process holding a file lock ends or crashes,
 the operating system will automatically unlock it.
 This feature is very useful in some situations where we want to assure
 automatic unlocking even when the process crashes and avoid leaving blocked resources in the system.
 */
//
// Databases generally also use a journal to log each action operated on the database and rollback
// the journal if in an incoherent state.
//
// In our case we have a cache split up in 256 buckets to lower concurrent accesses.
// That means 256 mutex: one for each bucket.
//
// Since the cache is accessed largely over thousands of times per second, using I/O based solution
// to ensure thread/process safety of the cache is probably overkill thus we did not explore this solution.
// However if in the future we want it to be shared accross a network, we would need to
// fallback on the file lock solution.
//
// Instead we use 256 interprocess mutex, all of them embedded in a single shared memory segment.
// The trick is then to detect any abandonned mutex and to recover from it.
// Instead of locking a mutex and wait until it is obtained, any mutex in the cache is taken by doing a
// timed lock: after a certain amount of time, if the lock could not be taken, we assume the mutex was
// abandonned.
//
// In a situation of abandonnement, we cannot assume any state on the cache, thus we wipe it and recreate it.
// Since we don't hold any information as precious as a database would, we are safe to do so anyway.
//
// Algorithm to detect and recover from abandonnement in a inter process cache:
//
// In addition to the interprocess mutex, we add a global file lock to monitor process access to the cache.
//
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

// Typedef our interprocess types
typedef bip::allocator<std::size_t, ExternalSegmentType::segment_manager> Size_t_Allocator_ExternalSegment;

// The unordered set of free tiles indices in a bucket
typedef bip::set<std::size_t, std::less<std::size_t>, Size_t_Allocator_ExternalSegment> set_size_t_ExternalSegment;


// A process local storage holder
typedef RamBuffer<char> ProcessLocalBuffer;
typedef boost::shared_ptr<ProcessLocalBuffer> ProcessLocalBufferPtr;

#ifdef NATRON_CACHE_INTERPROCESS_ROBUST
/**
 * @brief Implementation of the timed_lock which timesout after timeoutMilliseconds milliseconds.
 * The implementation is taken from boost, but we repliced the micro_clock::universal_time() by our 
 * own timestamps which bypass gmtime_r on unix systems which is horribly slow.
 **/
template <class Mutex, void(Mutex::*lock_func)(), bool(Mutex::*try_lock_func) ()>
bool timed_lock_impl(Mutex* m, std::size_t timeoutMilliseconds, double frequency)
{
    //Same as lock()
    if (timeoutMilliseconds == 0){
        (m->*lock_func)();
        return true;
    }
    //Always try to lock to achieve POSIX guarantees:
    // "Under no circumstance shall the function fail with a timeout if the mutex
    //  can be locked immediately. The validity of the abs_timeout parameter need not
    //  be checked if the mutex can be locked immediately."
    else if((m->*try_lock_func)()) {
        return true;
    } else {

        TimestampVal startTime = getTimestampInSeconds();
        TimestampVal now = startTime;

        double timeElapsedMS = 0.;
        do {
            if ((m->*try_lock_func)()) {
                return true;
            }
            timeElapsedMS = getTimeElapsed(startTime, now, frequency) * 1000.;
        } while(timeElapsedMS < timeoutMilliseconds);
    }
    return false;
} // timed_lock_impl

/**
 * @brief A base class for all scoped timed locks
 **/
template <class Mutex, void(Mutex::*lock_func)(), bool(Mutex::*try_lock_func) (), void(Mutex::*unlock_func)()>
class scoped_timed_lock_impl
{
public:

    typedef Mutex mutex_type;

    scoped_timed_lock_impl()
    : mp_mutex(0), m_locked(false), m_frequency(1.)
    {

    }

    // The mutex is not locked in the ctor
    scoped_timed_lock_impl(mutex_type& m, double frequency)
    : mp_mutex(&m), m_locked(false), m_frequency(frequency)
    {}

    // Unlocks the mutex if locked
    ~scoped_timed_lock_impl()
    {
        try{  if(m_locked && mp_mutex)   (mp_mutex->*unlock_func)();  }
        catch(...){}
    }

    //!Effects: If mutex() == 0 or if already locked, throws a lock_exception()
    //!   exception. Calls lock() on the referenced mutex.
    //!Postconditions: owns() == true.
    //!Notes: The scoped_lock changes from a state of not owning the mutex, to
    //!   owning the mutex, blocking if necessary.
    void lock()
    {
        assert(mp_mutex && !m_locked);
        mp_mutex->lock();
        m_locked = true;
    }

    // Try to take the lock and bail out if it failed to do
    // so after timeoutMilliseconds milliseconds.
    // If timeoutMilliseconds is 0, this is the same as lock()
    bool timed_lock(std::size_t timeoutMilliseconds = NATRON_CACHE_INTERPROCESS_MUTEX_TIMEOUT_MS)
    {
        assert(mp_mutex && !m_locked);
        m_locked = timed_lock_impl<Mutex, lock_func, try_lock_func>(mp_mutex, timeoutMilliseconds, m_frequency);
        return m_locked;
    }

    void unlock()
    {
        assert(mp_mutex && m_locked);
        mp_mutex->unlock();
        m_locked = false;
    }

    bool owns() const
    {
        return m_locked && mp_mutex;
    }

    operator bool() const
    {  return m_locked;   }


    mutex_type* mutex() const
    {  return  mp_mutex;  }

    mutex_type* release()
    {
        mutex_type *mut = mp_mutex;
        mp_mutex = 0;
        m_locked = false;
        return mut;
    }

protected:
    mutex_type *mp_mutex;
    bool m_locked;
    double m_frequency;
};

/**
 * @brief Base class for all locks that can be shared (read lock)
 **/
template <class Mutex>
class scoped_timed_sharable_lock : public scoped_timed_lock_impl<Mutex, &Mutex::lock_sharable, &Mutex::try_lock_sharable, &Mutex::unlock_sharable>
{
public:
    scoped_timed_sharable_lock(Mutex& m, double frequency)
    : scoped_timed_lock_impl<Mutex, &Mutex::lock_sharable, &Mutex::try_lock_sharable, &Mutex::unlock_sharable>(m, frequency)
    {

    }
};

typedef scoped_timed_sharable_lock<bip::interprocess_upgradable_mutex> Upgradable_ReadLock;
typedef scoped_timed_sharable_lock<bip::interprocess_sharable_mutex> Sharable_ReadLock;

/**
 * @brief Base class for all locks that can be upgraded (read lock)
 **/
class UpgradableLock : public scoped_timed_lock_impl<bip::interprocess_upgradable_mutex, &bip::interprocess_upgradable_mutex::lock_upgradable, &bip::interprocess_upgradable_mutex::try_lock_upgradable, &bip::interprocess_upgradable_mutex::unlock_upgradable>
{

    BOOST_MOVABLE_BUT_NOT_COPYABLE(UpgradableLock)


public:
    UpgradableLock(bip::interprocess_upgradable_mutex& m, double frequency)
    : scoped_timed_lock_impl<bip::interprocess_upgradable_mutex, &bip::interprocess_upgradable_mutex::lock_upgradable, &bip::interprocess_upgradable_mutex::try_lock_upgradable, &bip::interprocess_upgradable_mutex::unlock_upgradable>(m, frequency)
    {

    }
};


/**
 * @brief Base class for all locks that are exclusive
 **/
template <class Mutex>
class scoped_timed_lock : public scoped_timed_lock_impl<Mutex, &Mutex::lock, &Mutex::try_lock, &Mutex::unlock>
{


public:

    scoped_timed_lock()
    : scoped_timed_lock_impl<Mutex, &Mutex::lock, &Mutex::try_lock, &Mutex::unlock>()
    {
    }

    scoped_timed_lock(Mutex& m, double frequency)
    : scoped_timed_lock_impl<Mutex, &Mutex::lock, &Mutex::try_lock, &Mutex::unlock>(m, frequency)
    {
    }
};

typedef scoped_timed_lock<bip::interprocess_upgradable_mutex> Upgradable_WriteLock;
typedef scoped_timed_lock<bip::interprocess_sharable_mutex> Sharable_WriteLock;
typedef scoped_timed_lock<bip::interprocess_mutex> MutexLock;

#define scoped_lock_type scoped_timed_lock


/**
 * @brief A kind of scoped_timed_lock that is constructed from an upgradable lock
 **/
class scoped_upgraded_lock : public scoped_timed_lock<bip::interprocess_upgradable_mutex>
{
public:


    typedef bip::interprocess_upgradable_mutex mutex_type;


    //!Effects: If upgr.owns() then calls unlock_upgradable_and_lock() on the
    //!   referenced mutex. upgr.release() is called.
    //!Postconditions: mutex() == the value upgr.mutex() had before the construction.
    //!   upgr.mutex() == 0. owns() == upgr.owns() before the construction.
    //!   upgr.owns() == false after the construction.
    //!Notes: If upgr is locked, this constructor will lock this scoped_lock while
    //!   unlocking upgr. If upgr is unlocked, then this scoped_lock will be
    //!   unlocked as well. Only a moved upgradable_lock's will match this
    //!   signature. An non-moved upgradable_lock can be moved with
    //!   the expression: "boost::move(lock);" This constructor may block if
    //!   other threads hold a sharable_lock on this mutex (sharable_lock's can
    //!   share ownership with an upgradable_lock).
    explicit scoped_upgraded_lock(BOOST_RV_REF(UpgradableLock) upgr)
    : scoped_timed_lock<bip::interprocess_upgradable_mutex>()
    {
        UpgradableLock &u_lock = upgr;
        if (u_lock.owns()) {
            u_lock.mutex()->unlock_upgradable_and_lock();
            m_locked = true;
        }
        mp_mutex = u_lock.release();
    }
};


#else // !NATRON_CACHE_INTERPROCESS_ROBUST

typedef boost::shared_lock<boost::shared_mutex> Sharable_ReadLock;
typedef boost::shared_lock<boost::upgrade_mutex> Upgradable_ReadLock;
typedef boost::upgrade_lock<boost::upgrade_mutex> UpgradableLock;

typedef boost::unique_lock<boost::upgrade_mutex> scoped_upgraded_lock;
typedef boost::unique_lock<boost::upgrade_mutex> Upgradable_WriteLock;
typedef boost::unique_lock<boost::shared_mutex> Sharable_WriteLock;
typedef boost::unique_lock<boost::mutex> MutexLock;

#define scoped_lock_type boost::unique_lock

#endif // NATRON_CACHE_INTERPROCESS_ROBUST




// Maintain the lru with a list of hash: more recents hash are inserted at the end of the list
// The least recently used hash is the first item of the list.

/**
 * @brief A node of the linked list used to implement the LRU.
 * We need a custom list here, because we want to be able to hold 
 * an offset_ptr of a node directly inside a MemorySegmentEntry
 * for efficiency.
 **/
struct MemorySegmentEntryHeader;
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

typedef std::pair<const U64, bip::offset_ptr<MemorySegmentEntryHeader> > EntriesMapValueType;
typedef bip::allocator<EntriesMapValueType, ExternalSegmentType::segment_manager> EntriesMapValueType_Allocator_ExternalSegment;

typedef bip::map<U64, bip::offset_ptr<MemorySegmentEntryHeader>, std::less<U64>, EntriesMapValueType_Allocator_ExternalSegment> EntriesMap_ExternalSegment;


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

class SharedMemoryProcessLocalReadLocker;

/**
 * @brief This struct represents the minimum required data for a cache entry in the global bucket memory segment.
 * It is associated to a hash in the LRU linked list.
 * This struct lives in the ToC memory mapped file
 **/
struct MemorySegmentEntryHeader
{

    // If this entry has data in the tile aligned memory mapped file, this is the
    // index of the tile allocated. If not allocated, this is  -1.
    int tileCacheIndex;

    // The size of the memorySegmentPortion, in bytes. This is stored in the main cache memory segment.
    std::size_t size;

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

    // The corresponding node in the LRU list
    LRUListNode lruNode;

    MemorySegmentEntryHeader(const void_allocator& allocator)
    : tileCacheIndex(-1)
    , size(0)
    , status(eEntryStatusNull)
    , pluginID(allocator)
    , entryDataPointerList(allocator)
    , lruNode()
    {

    }

    void operator=(const MemorySegmentEntryHeader& other)
    {
        tileCacheIndex = other.tileCacheIndex;
        size = other.size;
        status = other.status;
        pluginID = other.pluginID;
        entryDataPointerList = other.entryDataPointerList;
        lruNode = other.lruNode;
    }
};

/**
 * @brief An enum indicating the state of the bucket. This enables corrupted cache detection in case
 * NATRON_CACHE_INTERPROCESS_ROBUST is not defined. If NATRON_CACHE_INTERPROCESS_ROBUST is defined,
 * the inconsistency of the cache is detected by a mutex timeout.
 **/
enum BucketStateEnum
{
    // Nothing is happening currently, we can safely operate
    eBucketStateOk,

    // An operation is under progress, when entering a public function
    // if we find this status, this means the bucket is inconsistent.
    eBucketStateInconsistent
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
     * @brief All IPC data that are shared accross processes for this bucket. This object lives in the ToC memory mapped file.
     **/
    struct IPCData
    {

        // Indices of the chunks of memory available in the tileAligned memory-mapped file.
        set_size_t_ExternalSegment freeTiles;

        // Pointers in shared memory to the lru list from node and back node
        bip::offset_ptr<LRUListNode> lruListFront, lruListBack;

        // A flat map for fast lookup of elements
        // See http://www.boost.org/doc/libs/1_59_0/doc/html/container/non_standard_containers.html#container.non_standard_containers.flat_xxx
        // for a comparison over regular maps
        EntriesMap_ExternalSegment entriesMap;

        // A version indicator for the serialization. If the cache version doesn't correspond
        // to NATRON_MEMORY_SEGMENT_ENTRY_HEADER_VERSION, we wipe it.
        unsigned int version;

        // What operation is done on the bucket. When obtaining a write lock on the bucket,
        // if the state is other than eBucketStateOk we detected an inconsistency.
        // The bucket state is protected by the toc segmentMutex
        BucketStateEnum bucketState;

        IPCData(const void_allocator& allocator)
        : freeTiles(allocator)
        , lruListFront(0)
        , lruListBack(0)
        , entriesMap(allocator)
        , version(NATRON_MEMORY_SEGMENT_ENTRY_HEADER_VERSION)
        , bucketState(eBucketStateOk)
        {

        }
    };


    // Memory mapped file for tiled entries: the size of this file is a multiple of the tile byte size.
    // Any access to the file should be protected by the tileData.segmentMutex mutex located in
    // CachePrivate::IPCData::PerBucketData
    // This is only valid if the cache is persistent
    MemoryFilePtr tileAlignedFile;

    // If the cache non persitent, this replaces tileAlignedFile
    ProcessLocalBufferPtr tileAlignedLocalBuf;


    // Memory mapped file used to store interprocess table of contents (IPCData)
    // It contains for each entry:
    // - A LRUListNode
    // - A MemorySegmentEntry
    // - A memory buffer of arbitrary size
    // Any access to the file should be protected by the tocData.segmentMutex mutex located in
    // CachePrivate::IPCData::PerBucketData
    // This is only valid if the cache is persistent
    MemoryFilePtr tocFile;

    // If the cache non persitent, this replaces tocFile
    ProcessLocalBufferPtr tocLocalBuf;

    // A memory manager of the tocFile. It is only valid when the tocFile is memory mapped.
    boost::shared_ptr<ExternalSegmentType> tocFileManager;

    // Pointer to the IPC data that live in tocFile memory mapped file. This is valid
    // as long as tocFile is mapped.
    IPCData *ipc;

    // Weak pointer to the cache
    CacheWPtr cache;

    // The index of this bucket in the cache
    int bucketIndex;

    CacheBucket()
    : tileAlignedFile()
    , tileAlignedLocalBuf()
    , tocFile()
    , tocLocalBuf()
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
    void deallocateCacheEntryImpl(EntriesMap_ExternalSegment::iterator entryIt,
                                  boost::scoped_ptr<SharedMemoryProcessLocalReadLocker>& shmAccess);

    /**
     * @brief Lookup the cache for a MemorySegmentEntry matching the hash key.
     * If found, the cacheEntry member will be set.
     * This function assumes that the tocData.segmentMutex is taken at least in read mode.
     **/
    EntriesMap_ExternalSegment::iterator tryCacheLookupImpl(U64 hash);

    enum ShmEntryReadRetCodeEnum
    {
        eShmEntryReadRetCodeOk,
        eShmEntryReadRetCodeDeserializationFailed,
        eShmEntryReadRetCodeLockTimeout
    };

    /**
     * @brief Reads the cacheEntry into the processLocalEntry.
     * This function updates the status member.
     * This function assumes that the bucketLock of the bucket is taken at least in read mode.
     * @returns True if ok, false if the MemorySegmentEntry cannot be read properly.
     * it should be deallocated from the segment.
     **/
    ShmEntryReadRetCodeEnum readFromSharedMemoryEntryImpl(MemorySegmentEntryHeader* entry,
                                                          const CacheEntryBasePtr& processLocalEntry,
                                                          U64 hash,
                                                          boost::scoped_ptr<SharedMemoryProcessLocalReadLocker>& shmAccess);

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
    template <typename Mutex>
    void remapToCMemoryFile(scoped_lock_type<Mutex>& lock, std::size_t minFreeSize);

    /**
     * @brief Returns whether the tile aligned memory mapped file mapping is still valid.
     * The tileData.segmentMutex is assumed to be taken for read-lock
     **/
    bool isTileFileMappingValid() const;

    /**
     * @brief Ensures that the tile aligned memory mapped file mapping is still valid and re-open it if not.
     * The tileData.segmentMutex AND tocData.segmentMutex are assumed to be taken for write-lock.
     * remapToCMemoryFile must have been called first because this function needs to access
     * the freeTiles data.
     * @param minFreeSize Indicates that the file should have at least this amount of free bytes.
     * If not, this function will call growTileFile.
     * If the file is empty and minFreeSize is 0, the file will at least be grown to a size of
     * NATRON_TILE_SIZE_BYTES * NATRON_CACHE_FILE_GROW_N_TILES
     **/
    template <typename Mutex>
    void remapTileMemoryFile(scoped_lock_type<Mutex>& lock, std::size_t minFreeSize);

    /**
     * @brief Grow the ToC memory mapped file. 
     * This will first wait all other processes accessing to the mapping before resizing.
     * Any process trying to access the mapping during resizing will wait.
     * The tileData.segmentMutex is assumed to be taken for write lock
     *
     * This function is called internally by remapToCMemoryFile()
     **/
    template <typename Mutex>
    void growToCFile(scoped_lock_type<Mutex>& lock, std::size_t bytesToAdd);

    /**
     * @brief Grow the tile memory mapped file. 
     * This will first wait all other processes accessing to the mapping before resizing.
     * Any process trying to access the mapping during resizing will wait.
     * The tileData.segmentMutex is assumed to be taken for write lock
     *
     * This function is called internally by remapTileMemoryFile()
     **/
    template <typename Mutex>
    void growTileFile(scoped_lock_type<Mutex>& lock, std::size_t bytesToAdd);
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

    CacheEntryLockerPrivate(CacheEntryLocker* publicInterface, const CachePtr& cache, const CacheEntryBasePtr& entry);

};

struct CachePrivate
{
    // Raw pointer to the public interface: lives in process memory
    Cache* _publicInterface;

    // The maximum size in bytes the cache can grow to
    // This is local to the process as it does not have to be shared necessarily:
    // if different accross processes then the process with the minimum size will
    // regulate the cache size.
    std::size_t maximumSize;

    // Protects all maximumSize.
    // Since it lives in process memory, this mutex
    // only protects against threads.
    boost::mutex maximumSizeMutex;

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
#ifdef NATRON_CACHE_INTERPROCESS_ROBUST
            bip::interprocess_upgradable_mutex segmentMutex;
#else
            boost::upgrade_mutex segmentMutex;
#endif

            // True whilst the mapping is valid.
            // Any time the memory mapped file needs to be accessed, the caller
            // is supposed to call isTileFileMappingValid() to check if the mapping is still valid.
            // If this function returns false, the caller needs to take a write lock and call
            // remapToCMemoryFile or remapTileMemoryFile depending on the memory file accessed.
            bool mappingValid;

            // The number of processes with a valid mapping: use to count processes with a valid mapping
            // See pseudo code below, this is used in combination with the wait conditions.
            int nProcessWithMappingValid;

            // Threads wait on this condition whilst the mappingValid flag is false
#ifdef NATRON_CACHE_INTERPROCESS_ROBUST
            bip::interprocess_condition_any mappingInvalidCond;

#else
            boost::condition_variable_any mappingInvalidCond;
#endif

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
#ifdef NATRON_CACHE_INTERPROCESS_ROBUST
            bip::interprocess_condition_any mappedProcessesNotEmpty;
#else
            boost::condition_variable_any mappedProcessesNotEmpty;
#endif

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

            // Protects the LRU list in the toc memory file.
            // This is separate to the mutex protecting the ToC memory mapped file
            // because even if we just access
            // the cache in read mode (in the get() function) we still need to update the LRU list, thus
            // protect it from being written by multiple concurrent threads.
#ifdef NATRON_CACHE_INTERPROCESS_ROBUST
            bip::interprocess_mutex lruListMutex;

#else
            boost::mutex lruListMutex;
#endif

            // Protects the bucketState
#ifdef NATRON_CACHE_INTERPROCESS_ROBUST
            bip::interprocess_mutex bucketStateMutex;

#else
            boost::mutex bucketStateMutex;
#endif

        };
        

        PerBucketData bucketsData[NATRON_CACHE_BUCKETS_COUNT];


        IPCData()
        : bucketsData()
        {

        }

    };

#ifdef NATRON_CACHE_INTERPROCESS_ROBUST
    // Pointer to the memory segment used to store bucket independent data accross processes.
    // This is ensured to be always valid and lives in process memory.
    // The global memory segment is of a fixed size and only contains one instance of CachePrivate::IPCData.
    // This is the only shared memory segment that has a fixed size.
    boost::scoped_ptr<bip::managed_shared_memory> globalMemorySegment;
#endif

    // The global file lock to monitor process access to the cache.
    // Only valid if the cache is persistent.
    boost::scoped_ptr<bip::file_lock> globalMemorySegmentFileLock;

#ifdef NATRON_CACHE_INTERPROCESS_ROBUST
    // Used in the implementation of ensureSharedMemoryIntegrity()
    boost::scoped_ptr<bip::named_semaphore> nSHMInvalidSem, nSHMValidSem;

    // A mutex used in the algorithm described above to lock the process local threads.
    // Protects nThreadsTimedOutFailed
    // It should be taken for reading anytime a process use an object in shared memory
    // and locked for writing when unmapping/remapping the shared memory.
    boost::shared_mutex nThreadsTimedOutFailedMutex;

    // Counts how many threads in this process timed out on the segmentMutex, to avoid
    // remapping multiple times the shared memory.
    int nThreadsTimedOutFailed;

    // Protected by nThreadsTimedOutFailedMutex
    boost::condition_variable_any nThreadsTimedOutFailedCond;
#endif

#ifdef NATRON_CACHE_INTERPROCESS_ROBUST
    // The IPC data object created in globalMemorySegment shared memory
    IPCData* ipc;
#else
    // If not IPC, this lives in memory
    boost::scoped_ptr<IPCData> ipc;
#endif

    // Path of the directory that should contain the cache directory itself.
    // This is controled by a Natron setting. By default it points to a standard system dependent
    // location.
    // Only valid for a persistent cache.
    std::string directoryContainingCachePath;

#ifdef NATRON_CACHE_INTERPROCESS_ROBUST
    // In windows, times returned by getTimestampInSeconds() must be divided by this value
    double timerFrequency;
#endif

    // If true the cache is persitent and all buckets use memory mapped files instead of
    // process local storage.
    bool persistent;

    CachePrivate(Cache* publicInterface, bool persistent)
    : _publicInterface(publicInterface)
    , maximumSize((std::size_t)8 * 1024 * 1024 * 1024) // 8GB max by default
    , maximumSizeMutex()
    , buckets()
#ifdef NATRON_CACHE_INTERPROCESS_ROBUST
    , globalMemorySegment()
#endif
    , globalMemorySegmentFileLock()
#ifdef NATRON_CACHE_INTERPROCESS_ROBUST
    , nSHMInvalidSem()
    , nSHMValidSem()
    , nThreadsTimedOutFailedMutex()
    , nThreadsTimedOutFailed(0)
    , nThreadsTimedOutFailedCond()
#endif
#ifdef NATRON_CACHE_INTERPROCESS_ROBUST
    , ipc(0)
#else
    , ipc()
#endif
    , directoryContainingCachePath()
#ifdef NATRON_CACHE_INTERPROCESS_ROBUST
    , timerFrequency(getPerformanceFrequency())
#endif
    , persistent(persistent)
    {

    }

    ~CachePrivate()
    {
    }

    void initializeCacheDirPath();

    void ensureCacheDirectoryExists();

    QString getBucketAbsoluteDirPath(int bucketIndex) const;

    std::string getSharedMemoryName() const;

    std::size_t getSharedMemorySize() const;

#ifdef NATRON_CACHE_INTERPROCESS_ROBUST
    /**
     * @brief Unmaps the global shared memory segment holding all bucket interprocess mutex
     * and re-create it. This function ensures that all process connected to the shared memory
     * are correctly remapped when exiting this function.
     **/
    void ensureSharedMemoryIntegrity();
#endif

    /**
     * @brief Clear all buckets by re-creating their underlying storage. No lock should be taken
     * when entering this function, except the shmAccess.
     **/
    void clearCacheInternal(boost::scoped_ptr<SharedMemoryProcessLocalReadLocker>& shmAccess);
    void clearCacheBucket(int bucket_i, boost::scoped_ptr<SharedMemoryProcessLocalReadLocker>& shmAccess);


    /**
     * @brief Ensure the cache returns to a correct state. Currently it wipes the cache.
     **/
    void recoverFromInconsistentState(int bucket_i, boost::scoped_ptr<SharedMemoryProcessLocalReadLocker>& shmAccess);

};

/**
 * @brief A small RAII object that should be instanciated whenever taking a write lock on the bucket
 **/
class BucketStateHandler_RAII
{
    CachePrivate* _imp;
    bool _valid;
    int _bucket_i;
#ifdef NATRON_CACHE_INTERPROCESS_ROBUST
    boost::scoped_ptr<MutexLock> locker;
#endif
public:

    BucketStateHandler_RAII(CachePrivate* imp, int bucket_i, boost::scoped_ptr<SharedMemoryProcessLocalReadLocker>& shmAccess)
    : _imp(imp)
    , _valid(true)
    , _bucket_i(bucket_i)
    {

#ifndef NATRON_CACHE_INTERPROCESS_ROBUST
        _imp->ipc->bucketsData[bucket_i].bucketStateMutex.lock();
#else
        locker.reset(new MutexLock(_imp->ipc->bucketsData[bucket_i].bucketStateMutex, imp->timerFrequency));
        if (!locker->timed_lock()) {
            _imp->recoverFromInconsistentState(bucket_i, shmAccess);
            _valid = false;
            return;
        }
#endif

        if (_imp->buckets[bucket_i].ipc->bucketState != eBucketStateOk) {
            _imp->ipc->bucketsData[bucket_i].bucketStateMutex.unlock();
            _imp->recoverFromInconsistentState(bucket_i, shmAccess);
            _valid = false;
            return;
        }

        _imp->buckets[bucket_i].ipc->bucketState = eBucketStateInconsistent;
    }

    bool isValid() const
    {
        return _valid;
    }

    ~BucketStateHandler_RAII()
    {
        if (_valid) {
            assert(_imp->buckets[_bucket_i].ipc->bucketState == eBucketStateInconsistent);
            _imp->buckets[_bucket_i].ipc->bucketState = eBucketStateOk;
            _imp->ipc->bucketsData[_bucket_i].bucketStateMutex.unlock();
        }
        assert(_imp->buckets[_bucket_i].ipc->bucketState == eBucketStateOk);
    }
};

/**
 * @brief Small RAII style class that should be used before using anything that is in the cache global shared memory
 * segment.
 * This prevents any other threads to call ensureSharedMemoryIntegrity() whilst this object is active.
 **/
class SharedMemoryProcessLocalReadLocker
{
#ifdef NATRON_CACHE_INTERPROCESS_ROBUST
    boost::scoped_ptr<boost::shared_lock<boost::shared_mutex> > processLocalLocker;
#endif
public:

    SharedMemoryProcessLocalReadLocker(CachePrivate* imp)
    {
#ifdef NATRON_CACHE_INTERPROCESS_ROBUST

        // A thread may enter ensureSharedMemoryIntegrity(), thus any other threads must ensure that the shared memory mapping
        // is valid before doing anything else.
        processLocalLocker.reset(new boost::shared_lock<boost::shared_mutex>(imp->nThreadsTimedOutFailedMutex));
        while (imp->nThreadsTimedOutFailed > 0) {
            imp->nThreadsTimedOutFailedCond.wait(*processLocalLocker);
        }
#else
        (void)imp;
#endif
    }

    ~SharedMemoryProcessLocalReadLocker()
    {
        // Release the processLocalLocker, allowing other threads to call ensureSharedMemoryIntegrity()
    }

};

#ifdef NATRON_CACHE_INTERPROCESS_ROBUST
/**
 * @brief Creates a locker object around the given process shared mutex.
 * If after the given lockTimeOutMs the lock could not be taken, the function
 * ensureSharedMemoryIntegrity() will be called. The mutex could be very well taken by 
 * a dead process. This function unmaps the shared memory, re-creates it and remap it.
 * @param shmReader Before calling this function, the shared memory where the interprocess lock resides
 * must be lock for reading.
 **/
template <typename LOCK>
bool createTimedLockAndHandleInconsistentStateIfFailed(CachePrivate* imp,
                                                       boost::scoped_ptr<SharedMemoryProcessLocalReadLocker>& shmReader,
                                                       int bucket_i,
                                                       boost::scoped_ptr<LOCK>& lock,
                                                       typename LOCK::mutex_type* mutex)
{
    // Take the lock. After lockTimeOutMS milliseconds, if the locks is not taken, we check the integrity of the
    // shared memory segment and retry the lock.
    // Another process could have taken the lock and crashed, leaving the shared memory in a bad state.
    bool unmappedOnce = false;
    for (;;) {

        lock.reset(new LOCK(*mutex, imp->timerFrequency));

        if (lock->timed_lock()) {
            return !unmappedOnce;
        } else {

            unmappedOnce = true;
            lock.reset();

#ifdef CACHE_TRACE_TIMEOUTS
            qDebug() << QThread::currentThread() << "Lock timeout, checking cache integrity";
#endif
            imp->recoverFromInconsistentState(bucket_i, shmReader);
        }
    }
    return false;
} // createTimedLockAndHandleInconsistentStateIfFailed
#endif // #ifdef NATRON_CACHE_INTERPROCESS_ROBUST

CacheEntryLockerPrivate::CacheEntryLockerPrivate(CacheEntryLocker* publicInterface, const CachePtr& cache, const CacheEntryBasePtr& entry)
: _publicInterface(publicInterface)
, cache(cache)
, processLocalEntry(entry)
, bucket(0)
, status(CacheEntryLocker::eCacheEntryStatusMustCompute)
{

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
    boost::scoped_ptr<SharedMemoryProcessLocalReadLocker> shmAccess(new SharedMemoryProcessLocalReadLocker(cache->_imp.get()));

    // Lookup and find an existing entry.
    // Never take over an entry upon timeout.
    std::size_t timeSpentWaiting = 0;
    ret->lookupAndSetStatus(shmAccess, &timeSpentWaiting, 0);
    return ret;
}

bool
CacheBucket::isToCFileMappingValid() const
{
    // Private - the tocData.segmentMutex is assumed to be taken for read lock
    CachePtr c = cache.lock();
    if (!c->_imp->ipc->bucketsData[bucketIndex].tocData.mappingValid) {
        return false;
    }
    return true;

} // isToCFileMappingValid

bool
CacheBucket::isTileFileMappingValid() const
{
    // Private - the tileData.segmentMutex is assumed to be taken for read lock
    CachePtr c = cache.lock();
    if (!c->_imp->ipc->bucketsData[bucketIndex].tileData.mappingValid) {
        return false;
    }
    return true;
} // isTileFileMappingValid

template <typename Mutex>
static void ensureMappingValidInternal(scoped_lock_type<Mutex>& lock,
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
        char* data;
        std::size_t dataNumBytes;
        if (bucket->tocFile) {
            data = bucket->tocFile->data();
            dataNumBytes = bucket->tocFile->size();
        } else {
            assert(bucket->tocLocalBuf);
            data = bucket->tocLocalBuf->getData();
            dataNumBytes = bucket->tocLocalBuf->size();
        }
        if (create) {
            bucket->tocFileManager.reset(new ExternalSegmentType(bip::create_only, data, dataNumBytes));
        } else {
            bucket->tocFileManager.reset(new ExternalSegmentType(bip::open_only, data, dataNumBytes));
        }
        // The ipc data pointer must be re-fetched
        void_allocator allocator(bucket->tocFileManager->get_segment_manager());
        bucket->ipc = bucket->tocFileManager->find_or_construct<CacheBucket::IPCData>("BucketData")(allocator);

        // If the version of the data is different than this build, wipe it and re-create it
        if (bucket->ipc->version != NATRON_MEMORY_SEGMENT_ENTRY_HEADER_VERSION) {
            std::string tileFilePath = bucket->tocFile->path();
            bucket->tocFile->remove();
            bucket->tocFile->open(tileFilePath, MemoryFile::eFileOpenModeOpenTruncateOrCreate);
            bucket->tocFile->resize(NATRON_CACHE_BUCKET_TOC_FILE_GROW_N_BYTES, false);
            reOpenToCData(bucket, true /*create*/);
        }

    } catch (...) {
        assert(false);
        throw std::runtime_error("Not enough space to allocate bucket table of content!");
    }
}

template <typename Mutex>
void
CacheBucket::remapToCMemoryFile(scoped_lock_type<Mutex>& lock, std::size_t minFreeSize)
{
    // Private - the tocData.segmentMutex is assumed to be taken for write lock
    CachePtr c = cache.lock();
    if (c->_imp->persistent) {
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
    }
    // Ensure the size of the ToC file is reasonable
    std::size_t curNumBytes;
    if (tocFile) {
        curNumBytes = tocFile->size();
    } else {
        curNumBytes = tocLocalBuf->size();
    }
    if (curNumBytes == 0) {
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

} // remapToCMemoryFile

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

template <typename Mutex>
void
CacheBucket::remapTileMemoryFile(scoped_lock_type<Mutex>& lock, std::size_t minFreeSize)
{
    // Private - the tileData.segmentMutex is assumed to be taken for write lock
    CachePtr c = cache.lock();
    if (c->_imp->persistent) {
        if (!c->_imp->ipc->bucketsData[bucketIndex].tileData.mappingValid) {
            // The number of memory free requested must be a multiple of the tile size.
            assert(minFreeSize == 0 || minFreeSize % NATRON_TILE_SIZE_BYTES == 0);

            flushTileMapping(tileAlignedFile, ipc->freeTiles);
        }

#ifdef CACHE_TRACE_FILE_MAPPING
        qDebug() << "Checking ToC mapping:" << c->_imp->ipc->bucketsData[bucketIndex].tileData.mappingValid;
#endif

        ensureMappingValidInternal(lock, tileAlignedFile, &c->_imp->ipc->bucketsData[bucketIndex].tileData);
    }

    // Ensure the size of the ToC file is reasonable
    std::size_t curNumBytes;
    if (tileAlignedFile) {
        curNumBytes = tileAlignedFile->size();
    } else {
        curNumBytes = tileAlignedLocalBuf->size();
    }
    if (curNumBytes == 0) {
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

} // remapTileMemoryFile

template <typename Mutex>
void
CacheBucket::growToCFile(scoped_lock_type<Mutex>& lock, std::size_t bytesToAdd)
{
    // Private - the tocData.segmentMutex is assumed to be taken for write lock

    CachePtr c = cache.lock();

    if (c->_imp->persistent) {
        c->_imp->ipc->bucketsData[bucketIndex].tocData.mappingValid = false;

        --c->_imp->ipc->bucketsData[bucketIndex].tocData.nProcessWithMappingValid;
        while (c->_imp->ipc->bucketsData[bucketIndex].tocData.nProcessWithMappingValid > 0) {
            c->_imp->ipc->bucketsData[bucketIndex].tocData.mappedProcessesNotEmpty.wait(lock);
        }
        // Save the entire file
        tocFile->flush(MemoryFile::eFlushTypeSync, NULL, 0);

    }


    std::size_t oldSize;
    if (tocFile) {
        oldSize = tocFile->size();
    } else {
        oldSize = tocLocalBuf->size();
    }
    std::size_t newSize = oldSize + bytesToAdd;
    // Round to the nearest next multiple of NATRON_CACHE_BUCKET_TOC_FILE_GROW_N_BYTES
    newSize = std::max((std::size_t)1, (std::size_t)std::ceil(newSize / (double) NATRON_CACHE_BUCKET_TOC_FILE_GROW_N_BYTES)) * NATRON_CACHE_BUCKET_TOC_FILE_GROW_N_BYTES;

    if (c->_imp->persistent) {
        // we pass preserve=false since we flushed the portion we know is valid just above
        tocFile->resize(newSize, false /*preserve*/);
    } else {
        assert(tocLocalBuf);
        tocLocalBuf->resizeAndPreserve(newSize);
    }

#ifdef CACHE_TRACE_FILE_MAPPING
    qDebug() << "Growing ToC file to " << printAsRAM(newSize);
#endif


    reOpenToCData(this, oldSize == 0 /*create*/);

    if (c->_imp->persistent) {
        ++c->_imp->ipc->bucketsData[bucketIndex].tocData.nProcessWithMappingValid;

        // Flag that the mapping is valid again and notify all other threads waiting
        c->_imp->ipc->bucketsData[bucketIndex].tocData.mappingValid = true;

        c->_imp->ipc->bucketsData[bucketIndex].tocData.mappingInvalidCond.notify_all();
    }

} // growToCFile

template <typename Mutex>
void
CacheBucket::growTileFile(scoped_lock_type<Mutex>& lock, std::size_t bytesToAdd)
{
    // Private - the tileData.segmentMutex is assumed to be taken for write lock
    // the tocData.segmentMutex is assumed to be taken for write lock because we need to read/write the free tiles

    CachePtr c = cache.lock();

    if (c->_imp->persistent) {
        c->_imp->ipc->bucketsData[bucketIndex].tileData.mappingValid = false;

        --c->_imp->ipc->bucketsData[bucketIndex].tileData.nProcessWithMappingValid;
        while (c->_imp->ipc->bucketsData[bucketIndex].tileData.nProcessWithMappingValid > 0) {
            c->_imp->ipc->bucketsData[bucketIndex].tileData.mappedProcessesNotEmpty.wait(lock);
        }
        // Update free tiles
        flushTileMapping(tileAlignedFile, ipc->freeTiles);

    }

    {


        // Resize the file
        std::size_t curSize;
        if (tileAlignedFile) {
            curSize = tileAlignedFile->size();
        } else {
            curSize = tileAlignedLocalBuf->size();
        }
        // The current size must be a multiple of the tile size
        assert(curSize % NATRON_TILE_SIZE_BYTES == 0);

        const std::size_t minTilesToAllocSize = NATRON_CACHE_FILE_GROW_N_TILES * NATRON_TILE_SIZE_BYTES;

        std::size_t newSize = curSize + bytesToAdd;
        // Round to the nearest next multiple of minTilesToAllocSize
        newSize = std::max((std::size_t)1, (std::size_t)std::ceil(newSize / (double) minTilesToAllocSize)) * minTilesToAllocSize;

        if (c->_imp->persistent) {
            // we pass preserve=false since we flushed the portion we know is valid just above
            tileAlignedFile->resize(newSize, false /*preserve*/);
        } else {
            assert(tileAlignedLocalBuf);
            tileAlignedLocalBuf->resizeAndPreserve(newSize);
        }

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

    if (c->_imp->persistent) {
        ++c->_imp->ipc->bucketsData[bucketIndex].tileData.nProcessWithMappingValid;

        // Flag that the mapping is valid again and notify all other threads waiting
        c->_imp->ipc->bucketsData[bucketIndex].tileData.mappingValid = true;

        c->_imp->ipc->bucketsData[bucketIndex].tileData.mappingInvalidCond.notify_all();
    }
    

} // growTileFile

EntriesMap_ExternalSegment::iterator
CacheBucket::tryCacheLookupImpl(U64 hash)
{
    // Private - the tocData.segmentMutex is assumed to be taken at least in read lock mode
#ifndef CACHE_TRACE_ENTRY_ACCESS
    return ipc->entriesMap.find(hash);
#else
    EntriesMap_ExternalSegment::iterator found = ipc->entriesMap.find(hash);
    if (found == ipc->entriesMap.end()) {
        qDebug() << hash << "look-up: entry not found";
    } else {
        qDebug() << hash << "look-up: entry found";
    }
    return found;
#endif

} // tryCacheLookupImpl

CacheBucket::ShmEntryReadRetCodeEnum
CacheBucket::readFromSharedMemoryEntryImpl(MemorySegmentEntryHeader* cacheEntry,
                                           const CacheEntryBasePtr& processLocalEntry,
                                           U64 hash,
                                           boost::scoped_ptr<SharedMemoryProcessLocalReadLocker>& shmAccess)
{
    // Private - the tocData.segmentMutex is assumed to be taken at least in read lock mode

#ifndef NATRON_CACHE_INTERPROCESS_ROBUST
    (void)shmAccess;
#endif

    // The entry must have been looked up in tryCacheLookup()
    assert(cacheEntry);

    assert(cacheEntry->status == MemorySegmentEntryHeader::eEntryStatusReady);

    CachePtr c = cache.lock();

    // If the entry is tiled, read from the tile buffer

    boost::scoped_ptr<Upgradable_ReadLock> tileReadLock;
    boost::scoped_ptr<Upgradable_WriteLock> tileWriteLock;
    char* tileDataPtr = 0;
    if (cacheEntry->tileCacheIndex != -1) {

        // First try to check if the tile aligned mapping is valid with a readlock
        bool tileMappingValid;


        {
#ifndef NATRON_CACHE_INTERPROCESS_ROBUST
            tileReadLock.reset(new Upgradable_ReadLock(c->_imp->ipc->bucketsData[bucketIndex].tileData.segmentMutex));
#else
            if (!createTimedLockAndHandleInconsistentStateIfFailed<Upgradable_ReadLock>(c->_imp.get(), shmAccess, bucketIndex, tileReadLock, &c->_imp->ipc->bucketsData[bucketIndex].tileData.segmentMutex)) {
                return eShmEntryReadRetCodeLockTimeout;
            }
#endif
            tileMappingValid = isTileFileMappingValid();
        }

        // mapping invalid, remap
        if (!tileMappingValid) {
            // If the tile mapping is invalid, take a write lock on the tile mapping and ensure it is valid
            tileReadLock.reset();
#ifndef NATRON_CACHE_INTERPROCESS_ROBUST
            tileWriteLock.reset(new Upgradable_WriteLock(c->_imp->ipc->bucketsData[bucketIndex].tileData.segmentMutex));
#else
            tileWriteLock.reset(new Upgradable_WriteLock(c->_imp->ipc->bucketsData[bucketIndex].tileData.segmentMutex, c->_imp->timerFrequency));
            if (!tileWriteLock->timed_lock()) {
                return eShmEntryReadRetCodeLockTimeout;
            }
#endif
            remapTileMemoryFile(*tileWriteLock, 0);
        }

        char* data;
        if (c->_imp->persistent) {
            data = tileAlignedFile->data();
        } else {
            data = tileAlignedLocalBuf->getData();
        }
        tileDataPtr = data + cacheEntry->tileCacheIndex * NATRON_TILE_SIZE_BYTES;
    }

    try {
        // Deserialize the entry. This may throw an exception if it cannot be deserialized properly
        // or out of memory
        if (cacheEntry->entryDataPointerList.empty()) {
            return eShmEntryReadRetCodeDeserializationFailed;
        }


        // First check the hash: it was the last object written in the memory segment, check that it is valid
        U64 serializedHash = 0;
        readAnonymousSharedObject(*cacheEntry->entryDataPointerList.rbegin(), tocFileManager.get(), &serializedHash);
        if (serializedHash != hash) {
            // The serialized hash is not the same, the entry was probably not written properly.
            return eShmEntryReadRetCodeDeserializationFailed;
        }

        ExternalSegmentTypeHandleList::const_iterator it = cacheEntry->entryDataPointerList.begin();
        ExternalSegmentTypeHandleList::const_iterator end = cacheEntry->entryDataPointerList.end();
        --end; // the last element was the hash
        processLocalEntry->fromMemorySegment(tocFileManager.get(), it, end, tileDataPtr);

        // Now compute the hash from the deserialized entry and check that it matches the given hash
        serializedHash = processLocalEntry->getHashKey(true /*forceComputation*/);

        // The entry changed, probably it was something of another type
        if (serializedHash != hash) {
            return eShmEntryReadRetCodeDeserializationFailed;
        }
    } catch (...) {
        return eShmEntryReadRetCodeDeserializationFailed;
    }


    // Update LRU record if this item is not already at the tail of the list
    //
    // Take the LRU list mutex
    {
        boost::scoped_ptr<MutexLock> lruWriteLock;
#ifndef NATRON_CACHE_INTERPROCESS_ROBUST
        lruWriteLock.reset(new MutexLock(c->_imp->ipc->bucketsData[bucketIndex].lruListMutex));
#else
        if (!createTimedLockAndHandleInconsistentStateIfFailed<MutexLock>(c->_imp.get(), shmAccess, bucketIndex, lruWriteLock, &c->_imp->ipc->bucketsData[bucketIndex].lruListMutex)) {
            return eShmEntryReadRetCodeLockTimeout;
        }
#endif

        assert(ipc->lruListBack && !ipc->lruListBack->next);
        if (ipc->lruListBack.get() != &cacheEntry->lruNode) {

            bip::offset_ptr<LRUListNode> entryNode(&cacheEntry->lruNode);
            disconnectLinkedListNode(entryNode);

            // And push_back to the tail of the list...
            insertLinkedListNode(entryNode, ipc->lruListBack, bip::offset_ptr<LRUListNode>(0));
            ipc->lruListBack = entryNode;
        }
    } // lruWriteLock

    return eShmEntryReadRetCodeOk;

} // readFromSharedMemoryEntryImpl

void
CacheBucket::deallocateCacheEntryImpl(EntriesMap_ExternalSegment::iterator entryIt,
                                      boost::scoped_ptr<SharedMemoryProcessLocalReadLocker>& shmAccess)
{

    // The tocData.segmentMutex must be taken in write mode

#ifndef NATRON_CACHE_INTERPROCESS_ROBUST
    (void)shmAccess;
#endif

    assert(entryIt != ipc->entriesMap.end());

    // Does not throw any exception
    for (ExternalSegmentTypeHandleList::const_iterator it = entryIt->second->entryDataPointerList.begin(); it != entryIt->second->entryDataPointerList.end(); ++it) {
        void* bufPtr = tocFileManager->get_address_from_handle(*it);
        if (bufPtr) {
            tocFileManager->destroy_ptr(bufPtr);
        }
    }
    entryIt->second->entryDataPointerList.clear();

    CachePtr c = cache.lock();


    if (entryIt->second->tileCacheIndex != -1) {
        // Free the tile
        // Take the tileData.segmentMutex in write mode

        if (c->_imp->persistent) {
            boost::scoped_ptr<Upgradable_WriteLock> writeLock;
#ifndef NATRON_CACHE_INTERPROCESS_ROBUST
            writeLock.reset(new Upgradable_WriteLock(c->_imp->ipc->bucketsData[bucketIndex].tileData.segmentMutex));
#else
            if (!createTimedLockAndHandleInconsistentStateIfFailed<Upgradable_WriteLock>(c->_imp.get(), shmAccess, bucketIndex, writeLock,  &c->_imp->ipc->bucketsData[bucketIndex].tileData.segmentMutex)) {
                return;
            }
#endif
            if (isTileFileMappingValid()) {
                remapTileMemoryFile(*writeLock, 0);
            }

            // Invalidate this portion of the memory mapped file
            std::size_t dataOffset = entryIt->second->tileCacheIndex * NATRON_TILE_SIZE_BYTES;
            tileAlignedFile->flush(MemoryFile::eFlushTypeInvalidate, tileAlignedFile->data() + dataOffset, NATRON_TILE_SIZE_BYTES);
        }
        

        // Make this tile free again
#ifdef CACHE_TRACE_TILES_ALLOCATION
        qDebug() << "Bucket" << bucketIndex << ": tile freed" << entryIt->second.tileCacheIndex << " Nb free tiles left:" << ipc->freeTiles.size();
#endif
        std::pair<set_size_t_ExternalSegment::iterator, bool>  insertOk = ipc->freeTiles.insert(entryIt->second->tileCacheIndex);
        assert(insertOk.second);
        (void)insertOk;
        entryIt->second->tileCacheIndex = -1;
    }



    // Remove this entry from the LRU list
    {
        // Take the lock of the LRU list.
        boost::scoped_ptr<MutexLock> lruWriteLock;
#ifndef NATRON_CACHE_INTERPROCESS_ROBUST
        lruWriteLock.reset(new MutexLock(c->_imp->ipc->bucketsData[bucketIndex].lruListMutex));
#else
        if (!createTimedLockAndHandleInconsistentStateIfFailed<MutexLock>(c->_imp.get(), shmAccess, bucketIndex, lruWriteLock, &c->_imp->ipc->bucketsData[bucketIndex].lruListMutex)) {
            return;
        }
#endif
        // Ensure the back and front pointers do not point to this entry
        if (&entryIt->second->lruNode == ipc->lruListBack.get()) {
            ipc->lruListBack = entryIt->second->lruNode.next ? entryIt->second->lruNode.next  : entryIt->second->lruNode.prev;
        }
        if (&entryIt->second->lruNode == ipc->lruListFront.get()) {
            ipc->lruListFront = entryIt->second->lruNode.next;
        }

        // Remove this entry's node from the list
        disconnectLinkedListNode(&entryIt->second->lruNode);
    }

    tocFileManager->destroy_ptr<MemorySegmentEntryHeader>(entryIt->second.get());

    // deallocate the entry
#ifdef CACHE_TRACE_ENTRY_ACCESS
    qDebug() << entryIt->first << ": destroy entry";
#endif
    ipc->entriesMap.erase(entryIt);
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
CacheEntryLocker::lookupAndSetStatusInternal(bool hasWriteRights, boost::scoped_ptr<SharedMemoryProcessLocalReadLocker>& shmAccess, std::size_t *timeSpentWaitingForPendingEntryMS, std::size_t timeout)
{

    // Look-up the cache
    U64 hash = _imp->processLocalEntry->getHashKey();
    EntriesMap_ExternalSegment::iterator foundEntry = _imp->bucket->tryCacheLookupImpl(hash);

    if (foundEntry == _imp->bucket->ipc->entriesMap.end()) {
        // No entry matching the hash could be found.
        return false;
    }


    if (foundEntry->second->status == MemorySegmentEntryHeader::eEntryStatusNull) {
        // The entry was aborted by a thread and nobody is computing it yet.
        // If we have write rights, takeover the entry
        // otherwise, wait for the 2nd look-up under the Write lock to do it.
        if (!hasWriteRights) {
            return false;
        }
#ifdef CACHE_TRACE_ENTRY_ACCESS
        qDebug() << hash << ": entry found but NULL, thread" << QThread::currentThread() << "is taking over the entry";
#endif
    }

    if (foundEntry->second->status == MemorySegmentEntryHeader::eEntryStatusPending) {

        // After a certain number of lookups, if the cache entry is still locked by another thread,
        // we take-over the entry, to ensure the entry was not left abandonned.
        if (timeout == 0 || *timeSpentWaitingForPendingEntryMS < timeout) {
            _imp->status = eCacheEntryStatusComputationPending;
#ifdef CACHE_TRACE_ENTRY_ACCESS
            qDebug() << hash << ": entry pending, sleeping...";
#endif

            return true;
        }

        // We need write rights to take over the entry
        if (!hasWriteRights) {
            return false;
        }
#ifdef CACHE_TRACE_ENTRY_ACCESS
        qDebug() << hash << ": entry pending timeout, thread" << QThread::currentThread() << "is taking over the entry";
#endif
    }

    if (foundEntry->second->status == MemorySegmentEntryHeader::eEntryStatusReady) {
        // Deserialize the entry and update the status
        CacheBucket::ShmEntryReadRetCodeEnum readStatus = _imp->bucket->readFromSharedMemoryEntryImpl(foundEntry->second.get(), _imp->processLocalEntry, hash, shmAccess);

        // By default we must compute
        _imp->status = CacheEntryLocker::eCacheEntryStatusMustCompute;
        switch (readStatus) {
            case CacheBucket::eShmEntryReadRetCodeOk:
                _imp->status = CacheEntryLocker::eCacheEntryStatusCached;
                break;
            case CacheBucket::eShmEntryReadRetCodeDeserializationFailed:
                // If the entry failed to deallocate or is not of the type of the process local entry
                // we have to remove it from the cache.
                // However we cannot do so under the read lock, we must take the write lock.
                // So do it in the 2nd lookup attempt.
                if (hasWriteRights) {
                    _imp->bucket->deallocateCacheEntryImpl(foundEntry, shmAccess);
                }
                return false;
            case CacheBucket::eShmEntryReadRetCodeLockTimeout:
                // Something went wrong: fail
                return false;
        }

    } else {
        // Either the entry was eEntryStatusNull and we have to compute it or the entry was still marked eEntryStatusPending
        // but we timed out and took over the entry computation.
        assert(hasWriteRights);
        foundEntry->second->status = MemorySegmentEntryHeader::eEntryStatusPending;
        _imp->status = eCacheEntryStatusMustCompute;
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
            qDebug() << hash << ": got entry but it has to be computed";
#endif
        }   break;
        case eCacheEntryStatusCached:
        {
            // We found in cache, nothing to do
#ifdef CACHE_TRACE_ENTRY_ACCESS
            qDebug() << hash << ": entry cached";
#endif
        }   break;
    } // switch(_imp->status)
    return true;
} // lookupAndSetStatusInternal

void
CacheEntryLocker::lookupAndSetStatus(boost::scoped_ptr<SharedMemoryProcessLocalReadLocker>& shmAccess, std::size_t *timeSpentWaitingForPendingEntryMS, std::size_t timeout)
{
    
    // Get the bucket corresponding to the hash. This will dispatch threads in (hopefully) different
    // buckets
    U64 hash = _imp->processLocalEntry->getHashKey();
    if (!_imp->bucket) {
        _imp->bucket = &_imp->cache->_imp->buckets[Cache::getBucketCacheBucketIndex(hash)];
    }

    // At least account for twice the struct size since

    {
        // Take the read lock: many threads/processes can try read at the same time.
        // If we timeout, clear the cache and retry.
        boost::scoped_ptr<Upgradable_ReadLock> readLock;
#ifndef NATRON_CACHE_INTERPROCESS_ROBUST
        readLock.reset(new Upgradable_ReadLock(_imp->cache->_imp->ipc->bucketsData[_imp->bucket->bucketIndex].tocData.segmentMutex));
#else
        createTimedLockAndHandleInconsistentStateIfFailed<Upgradable_ReadLock>(_imp->cache->_imp.get(), shmAccess, _imp->bucket->bucketIndex, readLock, &_imp->cache->_imp->ipc->bucketsData[_imp->bucket->bucketIndex].tocData.segmentMutex);
#endif

        boost::scoped_ptr<Upgradable_WriteLock> writeLock;
        // Every time we take the lock, we must ensure the memory mapping is ok because the
        // memory mapped file might have been resized to fit more entries.
        if (!_imp->bucket->isToCFileMappingValid()) {
            // Remove the read lock, and take a write lock.
            // This could allow other threads to run in-between, but we don't care since nothing happens.
            readLock.reset();

#ifndef NATRON_CACHE_INTERPROCESS_ROBUST
            writeLock.reset(new Upgradable_WriteLock(_imp->cache->_imp->ipc->bucketsData[_imp->bucket->bucketIndex].tocData.segmentMutex));
#else
            createTimedLockAndHandleInconsistentStateIfFailed<Upgradable_WriteLock>(_imp->cache->_imp.get(), shmAccess, _imp->bucket->bucketIndex, writeLock, &_imp->cache->_imp->ipc->bucketsData[_imp->bucket->bucketIndex].tocData.segmentMutex);
#endif

            _imp->bucket->remapToCMemoryFile(*writeLock, 0);
        }

        // This function succeeds either if
        // 1) The entry is cached and could be deserialized
        // 2) The entry is pending and thus the caller should call waitForPendingEntry
        // 3) The entry is not computed and thus the caller should compute the entry and call insertInCache
        //
        // This function returns false if the thread must take over the entry computation or the deserialization failed.
        // In any case, it should do so under the write lock below.
        if (lookupAndSetStatusInternal(false /*hasWriteRights*/, shmAccess, timeSpentWaitingForPendingEntryMS, timeout)) {
            return;
        }
    } // ReadLock(tocData.segmentMutex)

    // Concurrency resumes!

    assert(_imp->status == eCacheEntryStatusMustCompute ||
           _imp->status == eCacheEntryStatusComputationPending);

    // Either we failed to deserialize an entry or the caller timedout.
    // Take an upgradable lock and repeat the look-up.
    // Only a single thread/process can take the upgradable lock.
    {
        boost::scoped_ptr<UpgradableLock> upgradableLock;

        // If we timed out, take the lock once more and upong timeout, ensure the cache integrity and clear it.
#ifndef NATRON_CACHE_INTERPROCESS_ROBUST
        upgradableLock.reset(new UpgradableLock(_imp->cache->_imp->ipc->bucketsData[_imp->bucket->bucketIndex].tocData.segmentMutex));
#else
        createTimedLockAndHandleInconsistentStateIfFailed<UpgradableLock>(_imp->cache->_imp.get(), shmAccess, _imp->bucket->bucketIndex, upgradableLock, &_imp->cache->_imp->ipc->bucketsData[_imp->bucket->bucketIndex].tocData.segmentMutex);
#endif

        boost::scoped_ptr<scoped_upgraded_lock> writeLock;
        // Every time we take the lock, we must ensure the memory mapping is ok
        if (!_imp->bucket->isToCFileMappingValid()) {
            writeLock.reset(new scoped_upgraded_lock(boost::move(*upgradableLock)));
            _imp->bucket->remapToCMemoryFile(*writeLock, 0);
        }

        // This function only fails if the entry must be computed anyway.
        if (lookupAndSetStatusInternal(true /*hasWriteRights*/, shmAccess, timeSpentWaitingForPendingEntryMS, timeout)) {
            return;
        }
        assert(_imp->status == eCacheEntryStatusMustCompute);
        {
            // We need to upgrade the lock to a write lock. This will wait until all other threads have released their
            // read lock.
            if (!writeLock) {
                writeLock.reset(new scoped_upgraded_lock(boost::move(*upgradableLock)));
            }

            // Ensure the bucket is in a valid state.
            BucketStateHandler_RAII bucketStateHandler(_imp->cache->_imp.get(), _imp->bucket->bucketIndex, shmAccess);
            if (!bucketStateHandler.isValid()) {
                return;
            }

            // Now we are the only thread in this portion.

            // Create the MemorySegmentEntry if it does not exist
            void_allocator allocator(_imp->bucket->tocFileManager->get_segment_manager());
#ifdef CACHE_TRACE_ENTRY_ACCESS
            qDebug() << hash << ": construct entry";
#endif
            bip::offset_ptr<MemorySegmentEntryHeader> cacheEntry = 0;

            // the construction of the object may fail if the segment is out of memory. Upon failure, grow the ToC file and retry to allocate.
            {
                int attempt_i = 0;
                while (attempt_i < 10) {
                    try {
                        cacheEntry = _imp->bucket->tocFileManager->construct<MemorySegmentEntryHeader>(bip::anonymous_instance)(allocator);

                        EntriesMapValueType pair = std::make_pair(hash, cacheEntry);
                        std::pair<EntriesMap_ExternalSegment::iterator, bool> ok = _imp->bucket->ipc->entriesMap.insert(boost::move(pair));
                        assert(ok.first->second->entryDataPointerList.get_allocator().get_segment_manager() == allocator.get_segment_manager());
                        assert(ok.second);
                    } catch (const bip::bad_alloc& /*e*/) {
                        _imp->bucket->growToCFile(*writeLock, sizeof(MemorySegmentEntryHeader));
                    }
                    if (cacheEntry) {
                        break;
                    }
                    ++attempt_i;
                }
            }
            cacheEntry->pluginID.append(_imp->processLocalEntry->getKey()->getHolderPluginID().c_str());

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
    boost::scoped_ptr<SharedMemoryProcessLocalReadLocker> shmAccess(new SharedMemoryProcessLocalReadLocker(_imp->cache->_imp.get()));

    // Ensure the memory mapping is ok. We grow the file so it contains at least the size needed by the entry
    // plus some metadatas required management algorithm store its own memory housekeeping data.
    std::size_t entryToCSize = _imp->processLocalEntry->getMetadataSize();

    U64 hash = _imp->processLocalEntry->getHashKey();
    {
        // Take write lock on the bucket, if timeout, wipe the cache and fail.
        boost::scoped_ptr<Upgradable_WriteLock> writeLock;
#ifndef NATRON_CACHE_INTERPROCESS_ROBUST
        writeLock.reset(new Upgradable_WriteLock(_imp->cache->_imp->ipc->bucketsData[_imp->bucket->bucketIndex].tocData.segmentMutex));
#else
        if (!createTimedLockAndHandleInconsistentStateIfFailed<Upgradable_WriteLock>(_imp->cache->_imp.get(), shmAccess, _imp->bucket->bucketIndex, writeLock, &_imp->cache->_imp->ipc->bucketsData[_imp->bucket->bucketIndex].tocData.segmentMutex)) {
            return;
        }
#endif

        // Ensure the bucket is in a valid state.
        BucketStateHandler_RAII bucketStateHandler(_imp->cache->_imp.get(), _imp->bucket->bucketIndex, shmAccess);
        if (!bucketStateHandler.isValid()) {
            return;
        }


        if (!_imp->bucket->isToCFileMappingValid()) {
            _imp->bucket->remapToCMemoryFile(*writeLock, entryToCSize);
        }
        

        // Fetch the entry. It should be here unless the cache was wiped in between the lookupAndSetStatus and this function.
        EntriesMap_ExternalSegment::iterator entryIt = _imp->bucket->tryCacheLookupImpl(hash);
        if (entryIt == _imp->bucket->ipc->entriesMap.end()) {
            return;
        }

        // The status of the memory segment entry should be pending because we are the thread computing it.
        // All other threads are waiting.
        assert(entryIt->second->status == MemorySegmentEntryHeader::eEntryStatusPending);

        // The cacheEntry fields should be uninitialized
        // This may throw an exception if out of memory or if the getMetadataSize function does not return
        // enough memory to encode all the data.
        try {

            // Allocate memory for the entry metadatas
            entryIt->second->size = entryToCSize;


            // Serialize the meta-datas in the memory segment
            // If the entry also requires tile aligned data storage, allocate a tile now
            {
                boost::scoped_ptr<Upgradable_ReadLock> tileReadLock;
                boost::scoped_ptr<Upgradable_WriteLock> tileWriteLock;
                char* tileDataPtr = 0;
                if (_imp->processLocalEntry->isStorageTiled()) {
                    // First try to check if the tile aligned mapping is valid with a readlock

                    {
#ifndef NATRON_CACHE_INTERPROCESS_ROBUST
                        tileReadLock.reset(new Upgradable_ReadLock(_imp->cache->_imp->ipc->bucketsData[_imp->bucket->bucketIndex].tileData.segmentMutex));
#else
                         // Take read lock on the tile data, if timeout, wipe the cache and fail.
                        if (!createTimedLockAndHandleInconsistentStateIfFailed<Upgradable_ReadLock>(_imp->cache->_imp.get(), shmAccess, _imp->bucket->bucketIndex, tileReadLock, &_imp->cache->_imp->ipc->bucketsData[_imp->bucket->bucketIndex].tileData.segmentMutex)) {
                            return;
                        }
#endif
                        bool tileMappingValid = _imp->bucket->isTileFileMappingValid();
                        if (!tileMappingValid) {
                            // If the tile mapping is invalid, take a write lock on the tile mapping and ensure it is valid
                            tileReadLock.reset();
#ifndef NATRON_CACHE_INTERPROCESS_ROBUST
                            tileWriteLock.reset(new Upgradable_WriteLock(_imp->cache->_imp->ipc->bucketsData[_imp->bucket->bucketIndex].tileData.segmentMutex));
#else
                            if (!createTimedLockAndHandleInconsistentStateIfFailed<Upgradable_WriteLock>(_imp->cache->_imp.get(), shmAccess, _imp->bucket->bucketIndex, tileWriteLock, &_imp->cache->_imp->ipc->bucketsData[_imp->bucket->bucketIndex].tileData.segmentMutex)) {
                                return;
                            }
#endif
                            _imp->bucket->remapTileMemoryFile(*tileWriteLock, NATRON_TILE_SIZE_BYTES);

                        }
                    }

                    // Check that there's at least one free tile.
                    // No free tile: grow the file if necessary.
                    if (_imp->bucket->ipc->freeTiles.empty()) {
                        if (!tileWriteLock) {
#ifndef NATRON_CACHE_INTERPROCESS_ROBUST
                            tileWriteLock.reset(new Upgradable_WriteLock(_imp->cache->_imp->ipc->bucketsData[_imp->bucket->bucketIndex].tileData.segmentMutex));
#else
                            if (!createTimedLockAndHandleInconsistentStateIfFailed<Upgradable_WriteLock>(_imp->cache->_imp.get(), shmAccess, _imp->bucket->bucketIndex, tileWriteLock, &_imp->cache->_imp->ipc->bucketsData[_imp->bucket->bucketIndex].tileData.segmentMutex)) {
                                return;
                            }
#endif
                        }
                        _imp->bucket->growTileFile(*tileWriteLock, NATRON_TILE_SIZE_BYTES);
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


                    char* data;
                    if (_imp->cache->_imp->persistent) {
                        data = _imp->bucket->tileAlignedFile->data();
                    } else {
                        assert(_imp->bucket->tileAlignedLocalBuf);
                        data = _imp->bucket->tileAlignedLocalBuf->getData();
                    }
                    tileDataPtr = data + freeTileIndex * NATRON_TILE_SIZE_BYTES;

                    // Set the tile index on the entry so we can free it afterwards.
                    entryIt->second->tileCacheIndex = freeTileIndex;
                } // isStorageTiled
                

                // the construction of the object may fail if the segment is out of memory. Upon failure, grow the ToC file and retry to allocate.
                {
                    int attempt_i = 0;
                    while (attempt_i < 10) {
                        try {
                            assert(entryIt->second->entryDataPointerList.get_allocator().get_segment_manager() == _imp->bucket->tocFileManager->get_segment_manager());
                            _imp->processLocalEntry->toMemorySegment(_imp->bucket->tocFileManager.get(), &entryIt->second->entryDataPointerList, tileDataPtr);

                            // Add at the end the hash of the entry so that when deserializing we can check if everything was written correctly first
                            entryIt->second->entryDataPointerList.push_back(writeAnonymousSharedObject(hash, _imp->bucket->tocFileManager.get()));
                        } catch (const bip::bad_alloc& /*e*/) {

                            // Clear stuff that was already allocated by the entry
                            for (ExternalSegmentTypeHandleList::const_iterator it = entryIt->second->entryDataPointerList.begin(); it != entryIt->second->entryDataPointerList.end(); ++it) {
                                void* bufPtr = _imp->bucket->tocFileManager->get_address_from_handle(*it);
                                if (bufPtr) {
                                    _imp->bucket->tocFileManager->destroy_ptr(bufPtr);
                                }
                            }
                            entryIt->second->entryDataPointerList.clear();

                            _imp->bucket->growToCFile(*writeLock, entryToCSize);
                            ++attempt_i;
                            continue;
                        }
                        break;
                    }
                }

            } // tileWriteLock


            // Insert the hash in the LRU linked list
            // Lock the LRU list mutex
            {
                boost::scoped_ptr<MutexLock> lruWriteLock;
#ifndef NATRON_CACHE_INTERPROCESS_ROBUST
                lruWriteLock.reset(new MutexLock(_imp->cache->_imp->ipc->bucketsData[_imp->bucket->bucketIndex].lruListMutex));
#else
                if (!createTimedLockAndHandleInconsistentStateIfFailed<MutexLock>(_imp->cache->_imp.get(), shmAccess, _imp->bucket->bucketIndex, lruWriteLock, &_imp->cache->_imp->ipc->bucketsData[_imp->bucket->bucketIndex].lruListMutex)) {
                    return;
                }
#endif

                entryIt->second->lruNode.prev = 0;
                entryIt->second->lruNode.next = 0;
                entryIt->second->lruNode.hash = hash;

                bip::offset_ptr<LRUListNode> thisNodePtr = bip::offset_ptr<LRUListNode>(&entryIt->second->lruNode);
                if (!_imp->bucket->ipc->lruListBack) {
                    assert(!_imp->bucket->ipc->lruListFront);
                    // The list is empty, initialize to this node
                    _imp->bucket->ipc->lruListFront = thisNodePtr;
                    _imp->bucket->ipc->lruListBack = thisNodePtr;
                    assert(!_imp->bucket->ipc->lruListFront->prev && !_imp->bucket->ipc->lruListFront->next);
                    assert(!_imp->bucket->ipc->lruListBack->prev && !_imp->bucket->ipc->lruListBack->next);
                } else {
                    // Append to the tail of the list
                    assert(_imp->bucket->ipc->lruListFront && _imp->bucket->ipc->lruListBack);

                    insertLinkedListNode(thisNodePtr, _imp->bucket->ipc->lruListBack, bip::offset_ptr<LRUListNode>(0));
                    // Update back node
                    _imp->bucket->ipc->lruListBack = thisNodePtr;
                    
                }
            } // lruWriteLock
            entryIt->second->status = MemorySegmentEntryHeader::eEntryStatusReady;

            _imp->status = eCacheEntryStatusCached;

#ifdef CACHE_TRACE_ENTRY_ACCESS
            qDebug() << hash << ": entry inserted in cache";
#endif

        } catch (...) {

            // Set the status to eCacheEntryStatusMustCompute so that the destructor deallocates the entry.
            _imp->status = eCacheEntryStatusMustCompute;
        }


    } // writeLock

    // We just allocated something, ensure the cache size remains reasonable.
    // We cannot block here until the memory stays contained in the user requested memory portion:
    // if we would do so, then it could deadlock: Natron could require more memory than what
    // the user requested to render just one node.
    appPTR->checkCachesMemory();

    // Concurrency resumes!
    
} // insertInCache

CacheEntryLocker::CacheEntryStatusEnum
CacheEntryLocker::waitForPendingEntry(std::size_t timeout)
{
    // Public function, the SHM must not be locked.
    boost::scoped_ptr<SharedMemoryProcessLocalReadLocker> shmAccess(new SharedMemoryProcessLocalReadLocker(_imp->cache->_imp.get()));

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
    static const std::size_t timeToWaitMS = 50;

    do {
        // Look up the cache, but first take the lock on the MemorySegmentEntry
        // that will be released once another thread unlocked it in insertInCache
        // or the destructor.
        lookupAndSetStatus(shmAccess, &timeSpentWaitingForPendingEntryMS, timeout);

        if (_imp->status == eCacheEntryStatusComputationPending) {
            timeSpentWaitingForPendingEntryMS += timeToWaitMS;
            if (timeSpentWaitingForPendingEntryMS < timeout) {
                sleep_milliseconds(timeToWaitMS);
            }
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
        
        boost::scoped_ptr<SharedMemoryProcessLocalReadLocker> shmAccess(new SharedMemoryProcessLocalReadLocker(_imp->cache->_imp.get()));

        boost::scoped_ptr<Upgradable_WriteLock> writeLock;
#ifndef NATRON_CACHE_INTERPROCESS_ROBUST
        writeLock.reset(new Upgradable_WriteLock(_imp->cache->_imp->ipc->bucketsData[_imp->bucket->bucketIndex].tocData.segmentMutex));
#else
        if (!createTimedLockAndHandleInconsistentStateIfFailed<Upgradable_WriteLock>(_imp->cache->_imp.get(), shmAccess, _imp->bucket->bucketIndex, writeLock, &_imp->cache->_imp->ipc->bucketsData[_imp->bucket->bucketIndex].tocData.segmentMutex)) {
            return;
        }
#endif
        // Every time we take the lock, we must ensure the memory mapping is ok
        if (!_imp->bucket->isToCFileMappingValid()) {
            _imp->bucket->remapToCMemoryFile(*writeLock, 0);
        }

        U64 hash = _imp->processLocalEntry->getHashKey();
        EntriesMap_ExternalSegment::iterator entryIt = _imp->bucket->tryCacheLookupImpl(hash);
        if (entryIt == _imp->bucket->ipc->entriesMap.end()) {
            // The cache may have been wiped in between
            return;
        }
        _imp->bucket->deallocateCacheEntryImpl(entryIt, shmAccess);

    }
} // ~CacheEntryLocker


Cache::Cache(bool persistent)
: boost::enable_shared_from_this<Cache>()
, _imp(new CachePrivate(this, persistent))
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
Cache::create(bool persistent)
{
    CachePtr ret(new Cache(persistent));

    // Open or create the file lock
    if (persistent) {

        ret->_imp->initializeCacheDirPath();
        ret->_imp->ensureCacheDirectoryExists();

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
    } // persistent

    // Take the file lock in write mode:
    //      - If it succeeds, that means no other process is active: We remove the globalMemorySegment shared memory segment
    //        and create a new one, to ensure no lock was left in a bad state. Then we release the file lock
    //      - If it fails, another process is still actively using the globalMemorySegment shared memory: it must still be valid
    bool gotFileLock = true;
    if (persistent) {
        gotFileLock = ret->_imp->globalMemorySegmentFileLock->try_lock();
    }

#ifndef NATRON_CACHE_INTERPROCESS_ROBUST

    // If we did not get the file lock
    if (!gotFileLock) {
        qDebug() << "Another" << NATRON_APPLICATION_NAME << "is active, this process will fallback on a process local cache instead of a persistent cache";
        persistent = false;
        ret->_imp->persistent = false;
        ret->_imp->globalMemorySegmentFileLock.reset();
    }
#else
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
#endif // NATRON_CACHE_INTERPROCESS_ROBUST

    // Create the main memory segment containing the CachePrivate::IPCData
    {
#ifndef NATRON_CACHE_INTERPROCESS_ROBUST
        ret->_imp->ipc.reset(new CachePrivate::IPCData);
#else
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
#endif
        
    }

    if (persistent && gotFileLock) {
        ret->_imp->globalMemorySegmentFileLock->unlock();
    }

    // Indicate that we use the shared memory by taking the file lock in read mode.
    if (ret->_imp->globalMemorySegmentFileLock) {
        ret->_imp->globalMemorySegmentFileLock->lock_sharable();
    }
    
    // Open each bucket individual memory segment.
    // They are not created in shared memory but in a memory mapped file instead
    // to be persistent when the OS shutdown.
    // Each segment controls the table of content of the bucket.

    boost::scoped_ptr<SharedMemoryProcessLocalReadLocker> shmReader(new SharedMemoryProcessLocalReadLocker(ret->_imp.get()));
    for (int i = 0; i < NATRON_CACHE_BUCKETS_COUNT; ++i) {

        // Hold a weak pointer to the cache on the bucket
        ret->_imp->buckets[i].cache = ret;
        ret->_imp->buckets[i].bucketIndex = i;

        // Get the bucket directory path. It ends with a separator.
        QString bucketDirPath = ret->_imp->getBucketAbsoluteDirPath(i);

        // Open the cache ToC shared memory segment
        {
            // Take the ToC mapping mutex to ensure that the ToC file is valid
            boost::scoped_ptr<Upgradable_WriteLock> writeLock;
#ifndef NATRON_CACHE_INTERPROCESS_ROBUST
            writeLock.reset(new Upgradable_WriteLock(ret->_imp->ipc->bucketsData[i].tocData.segmentMutex));
#else
            if (!createTimedLockAndHandleInconsistentStateIfFailed<Upgradable_WriteLock>(ret->_imp.get(), shmReader, i, writeLock, &ret->_imp->ipc->bucketsData[i].tocData.segmentMutex)) {
                continue;
            }
#endif

            if (ret->_imp->persistent) {
                std::string tocFilePath = bucketDirPath.toStdString() + "Index";
                ret->_imp->buckets[i].tocFile.reset(new MemoryFile);
                ret->_imp->buckets[i].tocFile->open(tocFilePath, MemoryFile::eFileOpenModeOpenOrCreate);

                // Ensure the mapping is valid. This will grow the file the first time.
            } else {
                ret->_imp->buckets[i].tocLocalBuf.reset(new ProcessLocalBuffer);
            }
            ret->_imp->buckets[i].remapToCMemoryFile(*writeLock, 0);


        }

        // Open the memory-mapped file used for tiled entries data storage.
        {

            // Take the ToC mapping mutex and register this process amongst the valid mapping
            boost::scoped_ptr<Upgradable_WriteLock> writeLock;
#ifndef NATRON_CACHE_INTERPROCESS_ROBUST
            writeLock.reset(new Upgradable_WriteLock(ret->_imp->ipc->bucketsData[i].tileData.segmentMutex));
#else
            if (!createTimedLockAndHandleInconsistentStateIfFailed<Upgradable_WriteLock>(ret->_imp.get(), shmReader, i, writeLock, &ret->_imp->ipc->bucketsData[i].tileData.segmentMutex)) {
                continue;
            }
#endif
            if (ret->_imp->persistent) {
                std::string tileCacheFilePath = bucketDirPath.toStdString() + "TileCache";
                ret->_imp->buckets[i].tileAlignedFile.reset(new MemoryFile);
                ret->_imp->buckets[i].tileAlignedFile->open(tileCacheFilePath, MemoryFile::eFileOpenModeOpenOrCreate);

                // Ensure the mapping is valid. This will grow the file the first time.
            } else {
                ret->_imp->buckets[i].tileAlignedLocalBuf.reset(new ProcessLocalBuffer);
            }
            ret->_imp->buckets[i].remapTileMemoryFile(*writeLock, 0);

        }

    } // for each bucket


    return ret;
} // create

#ifdef NATRON_CACHE_INTERPROCESS_ROBUST
void
CachePrivate::ensureSharedMemoryIntegrity()
{
    // Any operation taking the segmentMutex in the shared memory, must do so with a timeout so we can avoid deadlocks:
    // If a process crashes whilst the segmentMutex is taken, the file lock is ensured to be released but the
    // segmentMutex will remain taken, deadlocking any other process.

    // Multiple threads in this process can time-out, however we just need to remap the shared memory once.
    // Ensure that no other thread is reading
    boost::unique_lock<boost::shared_mutex> processLocalLocker(nThreadsTimedOutFailedMutex);

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
        nThreadsTimedOutFailedCond.wait(processLocalLocker);
    }

} // ensureSharedMemoryIntegrity
#endif // #ifdef NATRON_CACHE_INTERPROCESS_ROBUST

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
Cache::setMaximumCacheSize(std::size_t size)
{
    std::size_t curSize = getMaximumCacheSize();
    {
        boost::unique_lock<boost::mutex> k(_imp->maximumSizeMutex);
        _imp->maximumSize = size;
    }

    // Clear exceeding entries if we are shrinking the cache.
    if (size < curSize) {
        evictLRUEntries(0);
    }
}


std::size_t
Cache::getMaximumCacheSize() const
{
    {
        boost::unique_lock<boost::mutex> k(_imp->maximumSizeMutex);
        return _imp->maximumSize;
    }
}

std::size_t
Cache::getCurrentSize() const
{
    boost::scoped_ptr<SharedMemoryProcessLocalReadLocker> shmReader(new SharedMemoryProcessLocalReadLocker(_imp.get()));

    std::size_t ret = 0;
    for (int i = 0; i < NATRON_CACHE_BUCKETS_COUNT; ++i) {
#ifndef NATRON_CACHE_INTERPROCESS_ROBUST
        boost::scoped_ptr<Upgradable_ReadLock> locker(new Upgradable_ReadLock(_imp->ipc->bucketsData[i].tileData.segmentMutex));
#else
        boost::scoped_ptr<Upgradable_ReadLock> locker(new Upgradable_ReadLock(_imp->ipc->bucketsData[i].tileData.segmentMutex, _imp->timerFrequency));
        if (!locker->timed_lock(500)) {
            return 0;
        }
#endif
        std::size_t bucketSize = 0;

        // Add the tile storage
        {
            std::size_t totalTileStorageSize;
            if (_imp->persistent) {
                totalTileStorageSize = _imp->buckets[i].tileAlignedFile->size();
            } else {
                totalTileStorageSize = _imp->buckets[i].tileAlignedLocalBuf->size();
            }
            bucketSize += (totalTileStorageSize - _imp->buckets[i].ipc->freeTiles.size() * NATRON_TILE_SIZE_BYTES);
        }

        // Add the table of contents storage
        {
            std::size_t totalToCStorageSize;
            if (_imp->persistent) {
                totalToCStorageSize = _imp->buckets[i].tocFile->size();
            } else {
                totalToCStorageSize = _imp->buckets[i].tocLocalBuf->size();
            }
            bucketSize += (totalToCStorageSize - _imp->buckets[i].tocFileManager->get_free_memory());
        }
        ret += bucketSize;

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


bool
Cache::isPersistent() const
{
    return _imp->persistent;
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

CacheEntryLockerPtr
Cache::get(const CacheEntryBasePtr& entry) const
{
    CachePtr thisShared = boost::const_pointer_cast<Cache>(shared_from_this());
    return CacheEntryLocker::create(thisShared, entry);
} // get

bool
Cache::hasCacheEntryForHash(U64 hash) const
{

    int bucketIndex = Cache::getBucketCacheBucketIndex(hash);
    CacheBucket& bucket = _imp->buckets[bucketIndex];

    boost::scoped_ptr<SharedMemoryProcessLocalReadLocker> shmReader(new SharedMemoryProcessLocalReadLocker(_imp.get()));

#ifndef NATRON_CACHE_INTERPROCESS_ROBUST
    boost::scoped_ptr<Upgradable_ReadLock> readLock(new Upgradable_ReadLock(_imp->ipc->bucketsData[bucketIndex].tocData.segmentMutex));
#else
    boost::scoped_ptr<Upgradable_ReadLock> readLock(new Upgradable_ReadLock(_imp->ipc->bucketsData[bucketIndex].tocData.segmentMutex, _imp->timerFrequency));
    if (!readLock->timed_lock()) {
        return false;
    }
#endif
    boost::scoped_ptr<Upgradable_WriteLock> writeLock;


    // First take a read lock and check if the mapping is valid. Otherwise take a write lock
    if (!bucket.isToCFileMappingValid()) {
        readLock.reset();

#ifndef NATRON_CACHE_INTERPROCESS_ROBUST
        writeLock.reset(new Upgradable_WriteLock(_imp->ipc->bucketsData[bucketIndex].tocData.segmentMutex));
#else
        if (!createTimedLockAndHandleInconsistentStateIfFailed<Upgradable_WriteLock>(_imp.get(), shmReader, bucketIndex, writeLock, &_imp->ipc->bucketsData[bucketIndex].tocData.segmentMutex)) {
            return false;
        }
#endif
        bucket.remapToCMemoryFile(*writeLock, 0);
    }

    EntriesMap_ExternalSegment::iterator entryIt = _imp->buckets[bucketIndex].tryCacheLookupImpl(hash);
    return entryIt != _imp->buckets[bucketIndex].ipc->entriesMap.end();


} // hasCacheEntryForHash

void
Cache::removeEntry(const CacheEntryBasePtr& entry)
{
    if (!entry) {
        return;
    }


    U64 hash = entry->getHashKey();
    int bucketIndex = Cache::getBucketCacheBucketIndex(hash);

    CacheBucket& bucket = _imp->buckets[bucketIndex];

    boost::scoped_ptr<SharedMemoryProcessLocalReadLocker> shmReader(new SharedMemoryProcessLocalReadLocker(_imp.get()));

    // Take the bucket lock in write mode
    {
        boost::scoped_ptr<Upgradable_WriteLock> writeLock;
#ifndef NATRON_CACHE_INTERPROCESS_ROBUST
        writeLock.reset(new Upgradable_WriteLock(_imp->ipc->bucketsData[bucketIndex].tocData.segmentMutex));
#else
        if (!createTimedLockAndHandleInconsistentStateIfFailed<Upgradable_WriteLock>(_imp.get(), shmReader, bucketIndex, writeLock, &_imp->ipc->bucketsData[bucketIndex].tocData.segmentMutex)) {
            return;
        }
#endif
        // Ensure the file mapping is OK
        bucket.remapToCMemoryFile(*writeLock, 0);

        // Ensure the bucket is in a valid state.
        BucketStateHandler_RAII bucketStateHandler(_imp.get(), bucketIndex, shmReader);
        if (!bucketStateHandler.isValid()) {
            return;
        }

        // Deallocate the memory taken by the cache entry in the ToC
        {
            EntriesMap_ExternalSegment::iterator entryIt = _imp->buckets[bucketIndex].tryCacheLookupImpl(hash);
            if (entryIt != _imp->buckets[bucketIndex].ipc->entriesMap.end()) {
                _imp->buckets[bucketIndex].deallocateCacheEntryImpl(entryIt, shmReader);
            }
        }
    }

} // removeEntry


void
CachePrivate::recoverFromInconsistentState(int bucket_i, boost::scoped_ptr<SharedMemoryProcessLocalReadLocker>& shmAccess)
{
    // Release the read lock on the SHM
    shmAccess.reset();

#ifdef NATRON_CACHE_INTERPROCESS_ROBUST
    // Create and remap the SHM
    ensureSharedMemoryIntegrity();
#endif

    // Flag that we are reading it
    shmAccess.reset(new SharedMemoryProcessLocalReadLocker(this));

    // Clear the cache: it could be corrupted
    clearCacheBucket(bucket_i, shmAccess);

} // recoverFromInconsistentState

void
CachePrivate::clearCacheBucket(int bucket_i, boost::scoped_ptr<SharedMemoryProcessLocalReadLocker>& shmAccess)
{
    // The SHM must be locked for reading.
    assert(shmAccess);
    (void)shmAccess;

    CacheBucket& bucket = buckets[bucket_i];

    // Close and re-create the memory mapped files
    {
#ifndef NATRON_CACHE_INTERPROCESS_ROBUST
        boost::scoped_ptr<Upgradable_WriteLock> writeLock (new Upgradable_WriteLock(ipc->bucketsData[bucket_i].tocData.segmentMutex));
#else
        boost::scoped_ptr<Upgradable_WriteLock> writeLock(new Upgradable_WriteLock(ipc->bucketsData[bucket_i].tocData.segmentMutex, timerFrequency));
        if (!writeLock->timed_lock()) {
            return;
        }
#endif
        if (persistent) {
            std::string tocFilePath = bucket.tocFile->path();
            bucket.tocFile->remove();
            bucket.tocFile->open(tocFilePath, MemoryFile::eFileOpenModeOpenTruncateOrCreate);
        } else {
            bucket.tocLocalBuf->clear();
        }
        bucket.remapToCMemoryFile(*writeLock, 0);

    }
    {
#ifndef NATRON_CACHE_INTERPROCESS_ROBUST
        boost::scoped_ptr<Upgradable_WriteLock> writeLock (new Upgradable_WriteLock(ipc->bucketsData[bucket_i].tileData.segmentMutex));
#else
        boost::scoped_ptr<Upgradable_WriteLock> writeLock(new Upgradable_WriteLock(ipc->bucketsData[bucket_i].tileData.segmentMutex, timerFrequency));
        if (!writeLock->timed_lock()) {
            return;
        }
#endif
        if (persistent) {
            std::string tileFilePath = bucket.tileAlignedFile->path();
            bucket.tileAlignedFile->remove();
            bucket.tileAlignedFile->open(tileFilePath, MemoryFile::eFileOpenModeOpenTruncateOrCreate);

        } else {
            bucket.tileAlignedLocalBuf->clear();
        }
        bucket.remapTileMemoryFile(*writeLock, 0);

    }

} // clearCacheBucket

void
CachePrivate::clearCacheInternal(boost::scoped_ptr<SharedMemoryProcessLocalReadLocker>& shmAccess)
{

    for (int bucket_i = 0; bucket_i < NATRON_CACHE_BUCKETS_COUNT; ++bucket_i) {
        clearCacheBucket(bucket_i, shmAccess);
    } // for each bucket

}

void
Cache::clear()
{

#ifdef NATRON_CACHE_INTERPROCESS_ROBUST
    _imp->ensureSharedMemoryIntegrity();
#endif

    boost::scoped_ptr<SharedMemoryProcessLocalReadLocker> shmReader(new SharedMemoryProcessLocalReadLocker(_imp.get()));
    _imp->clearCacheInternal(shmReader);

} // clear()

void
Cache::evictLRUEntries(std::size_t nBytesToFree)
{
    std::size_t maxSize = getMaximumCacheSize();

    // If max size == 0 then there's no limit.
    if (maxSize == 0) {
        return;
    }

    if (nBytesToFree >= maxSize) {
        maxSize = 0;
    } else {
        maxSize = maxSize - nBytesToFree;
    }

    std::size_t curSize = getCurrentSize();

    bool mustEvictEntries = curSize > maxSize;

    while (mustEvictEntries) {
        
        bool foundBucketThatCanEvict = false;

        boost::scoped_ptr<SharedMemoryProcessLocalReadLocker> shmReader(new SharedMemoryProcessLocalReadLocker(_imp.get()));

        // Check each bucket
        for (int bucket_i = 0; bucket_i < NATRON_CACHE_BUCKETS_COUNT; ++bucket_i) {
            CacheBucket& bucket = _imp->buckets[bucket_i];

            {
                // Lock for writing
#ifndef NATRON_CACHE_INTERPROCESS_ROBUST
                boost::scoped_ptr<Upgradable_WriteLock> writeLock (new Upgradable_WriteLock(_imp->ipc->bucketsData[bucket_i].tocData.segmentMutex));
#else
                boost::scoped_ptr<Upgradable_WriteLock> writeLock;
                if (!createTimedLockAndHandleInconsistentStateIfFailed<Upgradable_WriteLock>(_imp.get(), shmReader, bucket_i, writeLock, &_imp->ipc->bucketsData[bucket_i].tocData.segmentMutex)) {
                    return;
                }
#endif
                // Ensure the mapping
                bucket.remapToCMemoryFile(*writeLock, 0);

                // Ensure the bucket is in a valid state.
                BucketStateHandler_RAII bucketStateHandler(_imp.get(), bucket_i, shmReader);
                if (!bucketStateHandler.isValid()) {
                    return;
                }

                U64 hash = 0;
                {
                    // Lock the LRU list
#ifndef NATRON_CACHE_INTERPROCESS_ROBUST
                    boost::scoped_ptr<MutexLock> writeLock (new MutexLock(_imp->ipc->bucketsData[bucket_i].lruListMutex));
#else
                    boost::scoped_ptr<MutexLock> lruWriteLock;
                    if (!createTimedLockAndHandleInconsistentStateIfFailed<MutexLock>(_imp.get(), shmReader, bucket_i, lruWriteLock, &_imp->ipc->bucketsData[bucket_i].lruListMutex)) {
                        return;
                    }
#endif
                    // The least recently used entry is the one at the front of the linked list
                    if (bucket.ipc->lruListFront) {
                        hash = bucket.ipc->lruListFront->hash;
                    }
                }
                if (hash == 0) {
                    continue;
                }

                // Deallocate the memory taken by the cache entry in the ToC
                EntriesMap_ExternalSegment::iterator cacheEntryIt = bucket.tryCacheLookupImpl(hash);
                if (cacheEntryIt == bucket.ipc->entriesMap.end()) {
                    continue;
                }
                // We evicted one, decrease the size
                curSize -= cacheEntryIt->second->size;

                // Also decrease the size if this entry held a tile
                if (cacheEntryIt->second->tileCacheIndex != -1) {
                    curSize -= NATRON_TILE_SIZE_BYTES;
                }
                bucket.deallocateCacheEntryImpl(cacheEntryIt, shmReader);



            }
            
            foundBucketThatCanEvict = true;
            
        } // for each bucket

        // No bucket can be evicted anymore, exit.
        if (!foundBucketThatCanEvict) {
            break;
        }

        // Update mustEvictEntries for next iteration
        mustEvictEntries = curSize > maxSize;

    } // while(mustEvictEntries)

} // evictLRUEntries

void
Cache::getMemoryStats(std::map<std::string, CacheReportInfo>* infos) const
{

    boost::scoped_ptr<SharedMemoryProcessLocalReadLocker> shmReader(new SharedMemoryProcessLocalReadLocker(_imp.get()));

    for (int bucket_i = 0; bucket_i < NATRON_CACHE_BUCKETS_COUNT; ++bucket_i) {
        CacheBucket& bucket = _imp->buckets[bucket_i];

        boost::scoped_ptr<Upgradable_ReadLock> readLock;
        boost::scoped_ptr<Upgradable_WriteLock> writeLock;
#ifndef NATRON_CACHE_INTERPROCESS_ROBUST
        readLock.reset(new Upgradable_ReadLock(_imp->ipc->bucketsData[bucket_i].tocData.segmentMutex));
#else
        if (!createTimedLockAndHandleInconsistentStateIfFailed<Upgradable_ReadLock>(_imp.get(), shmReader, bucket_i, readLock, &_imp->ipc->bucketsData[bucket_i].tocData.segmentMutex)) {
            return;
        }
#endif

        // First take a read lock and check if the mapping is valid. Otherwise take a write lock
        if (!bucket.isToCFileMappingValid()) {
            readLock.reset();
#ifndef NATRON_CACHE_INTERPROCESS_ROBUST
            writeLock.reset(new Upgradable_WriteLock(_imp->ipc->bucketsData[bucket_i].tocData.segmentMutex));
#else
            if (!createTimedLockAndHandleInconsistentStateIfFailed<Upgradable_WriteLock>(_imp.get(), shmReader, bucket_i, writeLock, &_imp->ipc->bucketsData[bucket_i].tocData.segmentMutex)) {
                return;
            }
#endif
            bucket.remapToCMemoryFile(*writeLock, 0);
        }

        // Cycle through the whole map
        for (EntriesMap_ExternalSegment::const_iterator it = bucket.ipc->entriesMap.begin(); it != bucket.ipc->entriesMap.end(); ++it) {
            if (!it->second->pluginID.empty()) {

                std::string pluginID(it->second->pluginID.c_str());
                CacheReportInfo& entryData = (*infos)[pluginID];
                ++entryData.nEntries;
                entryData.nBytes += it->second->size;
                if (it->second->tileCacheIndex != -1) {
                    entryData.nBytes += NATRON_TILE_SIZE_BYTES;
                }
            }
        }

    } // for each bucket
} // getMemoryStats

void
Cache::flushCacheOnDisk(bool async)
{
    if (!_imp->persistent) {
        return;
    }
    boost::scoped_ptr<SharedMemoryProcessLocalReadLocker> shmReader(new SharedMemoryProcessLocalReadLocker(_imp.get()));
    for (int bucket_i = 0; bucket_i < NATRON_CACHE_BUCKETS_COUNT; ++bucket_i) {
        CacheBucket& bucket = _imp->buckets[bucket_i];

        {
            boost::scoped_ptr<Upgradable_ReadLock> readLock;
            boost::scoped_ptr<Upgradable_WriteLock> writeLock;
#ifndef NATRON_CACHE_INTERPROCESS_ROBUST
            readLock.reset(new Upgradable_ReadLock(_imp->ipc->bucketsData[bucket_i].tocData.segmentMutex));
#else
            if (!createTimedLockAndHandleInconsistentStateIfFailed<Upgradable_ReadLock>(_imp.get(), shmReader, bucket_i, readLock, &_imp->ipc->bucketsData[bucket_i].tocData.segmentMutex)) {
                return;
            }
#endif
            // First take a read lock and check if the mapping is valid. Otherwise take a write lock
            if (!bucket.isToCFileMappingValid()) {
                readLock.reset();
#ifndef NATRON_CACHE_INTERPROCESS_ROBUST
                writeLock.reset(new Upgradable_WriteLock(_imp->ipc->bucketsData[bucket_i].tocData.segmentMutex));
#else
                if (!createTimedLockAndHandleInconsistentStateIfFailed<Upgradable_WriteLock>(_imp.get(), shmReader, bucket_i, writeLock, &_imp->ipc->bucketsData[bucket_i].tocData.segmentMutex)) {
                    return;
                }
#endif
                // This function will flush for us.
                bucket.remapToCMemoryFile(*writeLock, 0);
            } else {
                bucket.tocFile->flush(async ? MemoryFile::eFlushTypeAsync : MemoryFile::eFlushTypeSync, NULL, 0);
            }



#ifndef NATRON_CACHE_INTERPROCESS_ROBUST
            boost::scoped_ptr<Upgradable_ReadLock> readLockTile(new Upgradable_ReadLock(_imp->ipc->bucketsData[bucket_i].tileData.segmentMutex));
#else
            boost::scoped_ptr<Upgradable_ReadLock> readLockTile(new Upgradable_ReadLock(_imp->ipc->bucketsData[bucket_i].tileData.segmentMutex, _imp->timerFrequency));
            if (!readLockTile->timed_lock()) {
                return;
            }
#endif
            // First take a read lock and check if the mapping is valid. Otherwise take a write lock
            if (!bucket.isTileFileMappingValid()) {
                readLockTile.reset();

#ifndef NATRON_CACHE_INTERPROCESS_ROBUST
                boost::scoped_ptr<Upgradable_WriteLock> writeLockTile(new Upgradable_WriteLock(_imp->ipc->bucketsData[bucket_i].tileData.segmentMutex));
#else
                boost::scoped_ptr<Upgradable_WriteLock> writeLockTile(new Upgradable_WriteLock(_imp->ipc->bucketsData[bucket_i].tileData.segmentMutex, _imp->timerFrequency));;
                if (!writeLockTile->timed_lock()) {
                    return;
                }
#endif
                 // This function will flush for us.
                bucket.remapTileMemoryFile(*writeLock, 0);
            } else {
                flushTileMapping(bucket.tileAlignedFile, bucket.ipc->freeTiles);
            }
        } // scoped lock

    } // for each bucket
} // flushCacheOnDisk

NATRON_NAMESPACE_EXIT;

