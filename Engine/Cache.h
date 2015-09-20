/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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

#ifndef NATRON_ENGINE_ABSTRACTCACHE_H_
#define NATRON_ENGINE_ABSTRACTCACHE_H_

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <vector>
#include <sstream>
#include <fstream>
#include <functional>
#include <list>
#include <cstddef>
#include <utility>
#include <algorithm> // min, max

#include "Global/GlobalDefines.h"
#include "Global/MemoryInfo.h"
GCC_DIAG_OFF(deprecated)
#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>
#include <QtCore/QMutexLocker>
#include <QtCore/QObject>
#include <QtCore/QTextStream>
#include <QtCore/QBuffer>
#include <QtCore/QThreadPool>
#include <QtCore/QRunnable>
GCC_DIAG_ON(deprecated)
#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#endif

#include "Engine/AppManager.h" //for access to settings
#include "Engine/Settings.h"
#include "Engine/CacheEntry.h"
#include "Engine/LRUHashTable.h"
#include "Engine/StandardPaths.h"
#include "Engine/ImageLocker.h"
#include "Global/MemoryInfo.h"

//Beyond that percentage of occupation, the cache will start evicting LRU entries
#define NATRON_CACHE_LIMIT_PERCENT 0.9

///When defined, number of opened files, memory size and disk size of the cache are printed whenever there's activity.
//#define NATRON_DEBUG_CACHE

namespace Natron {
/**
 * @brief The point of this thread is to delete the content of the list in a separate thread so the thread calling
 * get() doesn't wait for all the entries to be deleted (which can be expensive for large images)
 **/
template <typename T>
class DeleterThread
    : public QThread
{
    mutable QMutex _entriesQueueMutex;
    std::list<boost::shared_ptr<T> >_entriesQueue;
    QWaitCondition _entriesQueueNotEmptyCond;
    bool mustQuit;
    QMutex mustQuitMutex;
    QWaitCondition mustQuitCond;
    CacheAPI* cache;

public:

    DeleterThread(CacheAPI* cache)
        : QThread()
        , _entriesQueueMutex()
        , _entriesQueue()
        , _entriesQueueNotEmptyCond()
        , mustQuit(false)
        , mustQuitMutex()
        , mustQuitCond()
        , cache(cache)
    {
        setObjectName("CacheDeleter");
    }

    virtual ~DeleterThread()
    {
    }

    void appendToQueue(const std::list<boost::shared_ptr<T> > & entriesToDelete)
    {
        if ( entriesToDelete.empty() ) {
            return;
        }

        {
            QMutexLocker k(&_entriesQueueMutex);
            _entriesQueue.insert( _entriesQueue.begin(), entriesToDelete.begin(), entriesToDelete.end() );
        }
        if ( !isRunning() ) {
            start();
        } else {
            QMutexLocker k(&_entriesQueueMutex);
            _entriesQueueNotEmptyCond.wakeOne();
        }
    }

    void quitThread()
    {
        if ( !isRunning() ) {
            return;
        }
        QMutexLocker k(&mustQuitMutex);
        mustQuit = true;

        {
            QMutexLocker k2(&_entriesQueueMutex);
            _entriesQueue.push_back( boost::shared_ptr<T>() );
            _entriesQueueNotEmptyCond.wakeOne();
        }
        while (mustQuit) {
            mustQuitCond.wait(&mustQuitMutex);
        }
    }

    bool isWorking() const
    {
        QMutexLocker k(&_entriesQueueMutex);

        return !_entriesQueue.empty();
    }

private:

    virtual void run() OVERRIDE FINAL
    {
        for (;; ) {
            bool quit;
            {
                QMutexLocker k(&mustQuitMutex);
                quit = mustQuit;
            }

            {
                boost::shared_ptr<T> front;
                {
                    QMutexLocker k(&_entriesQueueMutex);
                    if ( quit && _entriesQueue.empty() ) {
                        _entriesQueueMutex.unlock();
                        QMutexLocker k(&mustQuitMutex);
                        mustQuit = false;
                        mustQuitCond.wakeAll();

                        return;
                    }
                    while ( _entriesQueue.empty() ) {
                        _entriesQueueNotEmptyCond.wait(&_entriesQueueMutex);
                    }

                    assert( !_entriesQueue.empty() );
                    front = _entriesQueue.front();
                    _entriesQueue.pop_front();
                }
                if (front) {
                    front->scheduleForDestruction();
                }
            } // front. After this scope, the image is guarenteed to be freed
            cache->notifyMemoryDeallocated();
        }
    }
};


/**
 * @brief The point of this thread is to remove entries that we are sure are no longer needed
 * e.g: they may have a hash that can no longer be produced
 **/
class CacheCleanerThread
    : public QThread
{
    mutable QMutex _requestQueueMutex;
    struct CleanRequest
    {
        std::string holderID;
        U64 nodeHash;
        bool removeAll;
    };

    std::list<CleanRequest> _requestsQueues;
    QWaitCondition _requestsQueueNotEmptyCond;
    bool mustQuit;
    QMutex mustQuitMutex;
    QWaitCondition mustQuitCond;
    CacheAPI* cache;

public:

    CacheCleanerThread(CacheAPI* cache)
        : QThread()
        , _requestQueueMutex()
        , _requestsQueues()
        , _requestsQueueNotEmptyCond()
        , mustQuit(false)
        , mustQuitMutex()
        , mustQuitCond()
        , cache(cache)
    {
        setObjectName("CacheCleaner");
    }

    virtual ~CacheCleanerThread()
    {
    }

    void appendToQueue(const std::string & holderID,
                       U64 nodeHash,
                       bool removeAll)
    {
        {
            QMutexLocker k(&_requestQueueMutex);
            CleanRequest r;
            r.holderID = holderID;
            r.nodeHash = nodeHash;
            r.removeAll = removeAll;
            _requestsQueues.push_back(r);
        }
        if ( !isRunning() ) {
            start();
        } else {
            QMutexLocker k(&_requestQueueMutex);
            _requestsQueueNotEmptyCond.wakeOne();
        }
    }

    void quitThread()
    {
        if ( !isRunning() ) {
            return;
        }
        QMutexLocker k(&mustQuitMutex);
        mustQuit = true;

        {
            QMutexLocker k2(&_requestQueueMutex);
            CleanRequest r;
            _requestsQueues.push_back(r);
            _requestsQueueNotEmptyCond.wakeOne();
        }
        while (mustQuit) {
            mustQuitCond.wait(&mustQuitMutex);
        }
    }

    bool isWorking() const
    {
        QMutexLocker k(&_requestQueueMutex);

        return !_requestsQueues.empty();
    }

private:

    virtual void run() OVERRIDE FINAL
    {
        for (;; ) {
            bool quit;
            {
                QMutexLocker k(&mustQuitMutex);
                quit = mustQuit;
            }

            {
                CleanRequest front;
                {
                    QMutexLocker k(&_requestQueueMutex);
                    if ( quit && _requestsQueues.empty() ) {
                        _requestQueueMutex.unlock();
                        QMutexLocker k(&mustQuitMutex);
                        mustQuit = false;
                        mustQuitCond.wakeAll();

                        return;
                    }
                    while ( _requestsQueues.empty() ) {
                        _requestsQueueNotEmptyCond.wait(&_requestQueueMutex);
                    }

                    assert( !_requestsQueues.empty() );
                    front = _requestsQueues.front();
                    _requestsQueues.pop_front();
                }
                cache->removeAllEntriesWithDifferentNodeHashForHolderPrivate(front.holderID, front.nodeHash, front.removeAll);
            }
        }
    }
};


class CacheSignalEmitter
    : public QObject
{
    Q_OBJECT

public:
    CacheSignalEmitter()
    {
    }

    ~CacheSignalEmitter()
    {
    }

    void emitSignalClearedInMemoryPortion()
    {
        Q_EMIT clearedInMemoryPortion();
    }

    void emitClearedDiskPortion()
    {
        Q_EMIT clearedDiskPortion();
    }

    void emitAddedEntry(SequenceTime time)
    {
        Q_EMIT addedEntry(time);
    }

    void emitRemovedEntry(SequenceTime time,
                          int storage)
    {
        Q_EMIT removedEntry(time, storage);
    }

    void emitEntryStorageChanged(SequenceTime time,
                                 int oldStorage,
                                 int newStorage)
    {
        Q_EMIT entryStorageChanged(time, oldStorage, newStorage);
    }

Q_SIGNALS:

    void clearedInMemoryPortion();

    void clearedDiskPortion();

    void addedEntry(SequenceTime);
    void removedEntry(SequenceTime, int);
    void entryStorageChanged(SequenceTime, int, int);
};


/*
 * ValueType must be derived of CacheEntryHelper
 */
template<typename EntryType>
class Cache
    : public CacheAPI
{
    friend class CacheCleanerThread;

public:

    typedef typename EntryType::hash_type hash_type;
    typedef typename EntryType::data_t data_t;
    typedef typename EntryType::key_t key_t;
    typedef typename EntryType::param_t param_t;
    typedef boost::shared_ptr<param_t> ParamsTypePtr;
    typedef boost::shared_ptr<EntryType> EntryTypePtr;

    struct SerializedEntry;

    typedef std::list< SerializedEntry > CacheTOC;

public:


#ifdef USE_VARIADIC_TEMPLATES

#ifdef NATRON_CACHE_USE_BOOST
#ifdef NATRON_CACHE_USE_HASH
    typedef BoostLRUHashTable<hash_type, EntryTypePtr>, boost::bimaps::unordered_set_of > CacheContainer;
#else
    typedef BoostLRUHashTable<hash_type, EntryTypePtr >, boost::bimaps::set_of > CacheContainer;
#endif
    typedef typename CacheContainer::container_type::left_iterator CacheIterator;
    typedef typename CacheContainer::container_type::left_const_iterator ConstCacheIterator;
    static std::list<CachedValue> &  getValueFromIterator(CacheIterator it)
    {
        return it->second;
    }

#else // cache use STL

#ifdef NATRON_CACHE_USE_HASH
    typedef StlLRUHashTable<hash_type, EntryTypePtr >, std::unordered_map > CacheContainer;
#else
    typedef StlLRUHashTable<hash_type, EntryTypePtr >, std::map > CacheContainer;
#endif
    typedef typename CacheContainer::key_to_value_type::iterator CacheIterator;
    typedef typename CacheContainer::key_to_value_type::const_iterator ConstCacheIterator;
    static std::list<EntryTypePtr> &  getValueFromIterator(CacheIterator it)
    {
        return it->second;
    }

#endif // NATRON_CACHE_USE_BOOST

#else // !USE_VARIADIC_TEMPLATES

#ifdef NATRON_CACHE_USE_BOOST
#ifdef NATRON_CACHE_USE_HASH
    typedef BoostLRUHashTable<hash_type, EntryTypePtr> CacheContainer;
#else
    typedef BoostLRUHashTable<hash_type, EntryTypePtr > CacheContainer;
#endif
    typedef typename CacheContainer::container_type::left_iterator CacheIterator;
    typedef typename CacheContainer::container_type::left_const_iterator ConstCacheIterator;
    static std::list<EntryTypePtr> &  getValueFromIterator(CacheIterator it)
    {
        return it->second;
    }

#else // cache use STL and tree (std map)

    typedef StlLRUHashTable<hash_type, EntryTypePtr > CacheContainer;
    typedef typename CacheContainer::key_to_value_type::iterator CacheIterator;
    typedef typename CacheContainer::key_to_value_type::const_iterator ConstCacheIterator;
    static std::list<EntryTypePtr> &   getValueFromIterator(CacheIterator it)
    {
        return it->second.first;
    }

#endif // NATRON_CACHE_USE_BOOST

#endif // USE_VARIADIC_TEMPLATES

private:


    std::size_t _maximumInMemorySize;     // the maximum size of the in-memory portion of the cache.(in % of the maximum cache size)
    std::size_t _maximumCacheSize;     // maximum size allowed for the cache

    /*mutable because we need to change modify it in the sealEntryInternal function which
         is called by an external object that have a const ref to the cache.
     */
    mutable std::size_t _memoryCacheSize;     // current size of the cache in bytes
    mutable std::size_t _diskCacheSize;
    mutable QMutex _sizeLock; // protects _memoryCacheSize & _diskCacheSize & _maximumInMemorySize & _maximumCacheSize
    mutable QMutex _lock; //protects _memoryCache & _diskCache
    mutable QMutex _getLock;  //prevents get() and getOrCreate() to be called simultaneously

    /*These 2 are mutable because we need to modify the LRU list even
         when we call get() and we want this function to be const.*/
    mutable CacheContainer _memoryCache;
    mutable CacheContainer _diskCache;
    const std::string _cacheName;
    const unsigned int _version;

    /*mutable because it doesn't hold any data, it just emits signals but signals cannot
         be const somehow .*/
    mutable CacheSignalEmitter* _signalEmitter;

    ///Store the system physical total RAM in a member
    std::size_t _maxPhysicalRAM;
    bool _tearingDown;
    mutable Natron::DeleterThread<EntryType> _deleterThread;
    mutable QWaitCondition _memoryFullCondition; //< protected by _sizeLock
    mutable Natron::CacheCleanerThread _cleanerThread;

public:


    Cache(const std::string & cacheName
          ,
          unsigned int version
          ,
          U64 maximumCacheSize      // total size
          ,
          double maximumInMemoryPercentage)      //how much should live in RAM
        : CacheAPI()
        , _maximumInMemorySize(maximumCacheSize * maximumInMemoryPercentage)
        , _maximumCacheSize(maximumCacheSize)
        , _memoryCacheSize(0)
        , _diskCacheSize(0)
        , _sizeLock()
        , _lock()
        , _getLock()
        , _memoryCache()
        , _diskCache()
        , _cacheName(cacheName)
        , _version(version)
        , _signalEmitter(new CacheSignalEmitter)
        , _maxPhysicalRAM( getSystemTotalRAM() )
        , _tearingDown(false)
        , _deleterThread(this)
        , _memoryFullCondition()
        , _cleanerThread(this)
    {
    }

    virtual ~Cache()
    {
        QMutexLocker locker(&_lock);

        _tearingDown = true;
        _memoryCache.clear();
        _diskCache.clear();
        delete _signalEmitter;
    }

    void waitForDeleterThread()
    {
        _deleterThread.quitThread();
        _cleanerThread.quitThread();
    }

    /**
     * @brief Look-up the cache for an entry whose key matches the params.
     * @param params The key identifying the entry we're looking for.
     * @param [out] params A pointer to the params that go along the returnValue.
     * They do not help to identify they entry, rather
     * this class can be used to cache other parameters along with the value_type.
     * @param [out] returnValue The returnValue, contains the cache entry if the return value
     * of the function is true, otherwise the pointer is left untouched.
     * @returns True if the cache successfully found an entry matching the params.
     * False otherwise.
     **/
    bool get(const typename EntryType::key_type & key,
             std::list<EntryTypePtr>* returnValue) const
    {
        ///Be atomic, so it cannot be created by another thread in the meantime
        QMutexLocker getlocker(&_getLock);

        ///lock the cache before reading it.
        QMutexLocker locker(&_lock);

        return getInternal(key, returnValue);
    } // get

private:

    void createInternal(const typename EntryType::key_type & key,
                        const ParamsTypePtr & params,
                        EntryTypePtr* returnValue) const
    {
        //_lock must not be taken here

        ///Before allocating the memory check that there's enough space to fit in memory
        appPTR->checkCacheFreeMemoryIsGoodEnough();


        ///Just in case, we don't allow more than X files to be removed at once.
        int safeCounter = 0;
        ///If too many files are opened, fall-back on RAM storage.
        while (appPTR->isNCacheFilesOpenedCapped() && safeCounter < 1000) {
#ifdef NATRON_DEBUG_CACHE
            qDebug() << "Reached maximum cache files opened limit,clearing last recently used one...";
#endif
            if ( !evictLRUDiskEntry() ) {
                break;
            }
            ++safeCounter;
        }

        U64 memoryCacheSize, maximumInMemorySize;
        {
            QMutexLocker k(&_sizeLock);
            memoryCacheSize = _memoryCacheSize;
            maximumInMemorySize = std::max( (std::size_t)1, _maximumInMemorySize );
        }
        {
            QMutexLocker locker(&_lock);
            std::list<EntryTypePtr> entriesToBeDeleted;
            double occupationPercentage = (double)memoryCacheSize / maximumInMemorySize;
            ///While the current cache size can't fit the new entry, erase the last recently used entries.
            ///Also if the total free RAM is under the limit of the system free RAM to keep free, erase LRU entries.
            while (occupationPercentage > NATRON_CACHE_LIMIT_PERCENT) {
                std::list<EntryTypePtr> deleted;
                if ( !tryEvictEntry(deleted) ) {
                    break;
                }

                for (typename std::list<EntryTypePtr>::iterator it = deleted.begin(); it != deleted.end(); ++it) {
                    if ( !(*it)->isStoredOnDisk() ) {
                        memoryCacheSize -= (*it)->size();
                    }
                    entriesToBeDeleted.push_back(*it);
                }

                occupationPercentage = (double)memoryCacheSize / maximumInMemorySize;
            }

            if ( !entriesToBeDeleted.empty() ) {
                ///Launch a separate thread whose function will be to delete all the entries to be deleted
                _deleterThread.appendToQueue(entriesToBeDeleted);

                ///Clearing the list here will not delete the objects pointing to by the shared_ptr's because we made a copy
                ///that the separate thread will delete
                entriesToBeDeleted.clear();
            }
        }
        {
            //If _maximumcacheSize == 0 we don't return 1 otherwise we would cause a deadlock
            QMutexLocker k(&_sizeLock);
            double occupationPercentage =  _maximumCacheSize == 0 ? 0.99 : (double)_memoryCacheSize / _maximumCacheSize;

            //_memoryCacheSize member will get updated while images are being destroyed by the parallel thread.
            //we wait for cache memory occupation to be < 100% to be sure we don't hit swap here
            while ( occupationPercentage >= 1. && _deleterThread.isWorking() ) {
                _memoryFullCondition.wait(&_sizeLock);
                occupationPercentage =  _maximumCacheSize == 0 ? 0.99 : (double)_memoryCacheSize / _maximumCacheSize;
            }
        }
        {
            QMutexLocker locker(&_lock);
            Natron::StorageModeEnum storage;
            if (params->getCost() == 0) {
                storage = Natron::eStorageModeRAM;
            } else if (params->getCost() >= 1) {
                storage = Natron::eStorageModeDisk;
            } else {
                storage = Natron::eStorageModeNone;
            }


            try {
                std::string filePath;
                if (storage == Natron::eStorageModeDisk) {
                    filePath = getCachePath().toStdString();
                    filePath += '/';
                }
                returnValue->reset( new EntryType(key, params, this, storage, filePath) );

                ///Don't call allocateMemory() here because we're still under the lock and we might force tons of threads to wait unnecesserarily
            } catch (const std::bad_alloc & e) {
                *returnValue = EntryTypePtr();
            }

            if (*returnValue) {
                sealEntry(*returnValue, true);
            }
        }
    } // createInternal

public:


    /**
     * @brief Look-up the cache for an entry whose key matches the 'key' and 'params'.
     * Unlike get(...) this function creates a new entry if it couldn't be found.
     * Note that this function also takes extra parameters which go along the value_type
     * and will be cached. These parameters aren't taken into account for the computation of
     * the hash key. It is a safe place to cache any extra data that is relative to an entry,
     * but doesn't make it an identifier of that entry. The base class just adds the necessary
     * info for the cache to be able to instantiate a new entry (that is the cost and the elements count).
     *
     * @param key The key identifying the entry we're looking for.
     *
     * @param params The non unique parameters. They do not help to identify they entry, rather
     * this class can be used to cache other parameters along with the value_type.
     *
     * @param imageLocker A pointer to an ImageLockerI which will lock the image if it was freshly
     * created so that you can call allocateMemory() safely without another thread accessing it.
     *
     * @param [out] returnValue The returnValue, contains the cache entry.
     * Internally the allocation of the new entry might fail on the requested device,
     * e.g: if you ask for an entry with a large cost, the cache will try to put the
     * entry on disk to preserve it, but if the allocation failed it will fallback
     * on RAM instead.
     *
     * Either way the returnValue parameter can never be NULL.
     * @returns True if the cache successfully found an entry matching the key.
     * False otherwise.
     **/
    bool getOrCreate(const typename EntryType::key_type & key,
                     const ParamsTypePtr & params,
                     EntryTypePtr* returnValue) const
    {
        ///Make sure the shared_ptrs live in this list and are destroyed not while under the lock
        ///so that the memory freeing (which might be expensive for large images) doesn't happen while under the lock

        {
            ///Be atomic, so it cannot be created by another thread in the meantime
            QMutexLocker getlocker(&_getLock);
            std::list<EntryTypePtr> entries;
            bool didGetSucceed;
            {
                QMutexLocker locker(&_lock);
                didGetSucceed = getInternal(key, &entries);
            }
            if (didGetSucceed) {
                for (typename std::list<EntryTypePtr>::iterator it = entries.begin(); it != entries.end(); ++it) {
                    if (*(*it)->getParams() == *params) {
                        *returnValue = *it;

                        return true;
                    }
                }
            }

            createInternal(key, params, returnValue);

            return false;
        } // getlocker
    }

    /**
     * @brief Clears entirely the disk portion and memory portion.
     **/
    void clear()
    {
        clearDiskPortion();


        if (_signalEmitter) {
            ///block signals otherwise the we would be spammed of notifications
            _signalEmitter->blockSignals(true);
        }
        QMutexLocker locker(&_lock);
        std::pair<hash_type, EntryTypePtr> evictedFromMemory = _memoryCache.evict();
        while (evictedFromMemory.second) {
            if ( evictedFromMemory.second->isStoredOnDisk() ) {
                evictedFromMemory.second->removeAnyBackingFile();
            }
            evictedFromMemory = _memoryCache.evict();
        }

        if (_signalEmitter) {
            _signalEmitter->blockSignals(false);
            _signalEmitter->emitSignalClearedInMemoryPortion();
        }
    }

    /**
     * @brief Clears all entries on disk that are not actively being used somewhere else in the application.
     * Any entry being used by the application will be left in the cache.
     **/
    void clearDiskPortion()
    {
        if (_signalEmitter) {
            ///block signals otherwise the we would be spammed of notifications
            _signalEmitter->blockSignals(true);
        }
        QMutexLocker locker(&_lock);

        /// An entry which has a use_count greater than 1 is not removable:
        /// The backing file must not be removed because it might be read/written to
        /// at the same time. The best we can do is just let it here in the cache.
        std::pair<hash_type, EntryTypePtr> evictedFromDisk = _diskCache.evict();
        //if the cache couldn't evict that means all entries are used somewhere and we shall not remove them!
        //we'll let the user of these entries purge the extra entries left in the cache later on
        while (evictedFromDisk.second) {
            evictedFromDisk.second->removeAnyBackingFile();
            evictedFromDisk = _diskCache.evict();
        }


        _signalEmitter->blockSignals(false);
        _signalEmitter->emitClearedDiskPortion();
    }

    /**
     * @brief Clears the memory portion and moves it to the disk portion if possible
     **/
    void clearInMemoryPortion(bool emitSignals = true)
    {
        if (_signalEmitter) {
            ///block signals otherwise the we would be spammed of notifications
            _signalEmitter->blockSignals(true);
        }
        QMutexLocker locker(&_lock);
        std::pair<hash_type, EntryTypePtr> evictedFromMemory = _memoryCache.evict();
        while (evictedFromMemory.second) {
            ///move back the entry on disk if it can be store on disk
            if ( evictedFromMemory.second->isStoredOnDisk() ) {
                evictedFromMemory.second->deallocate();
                /*insert it back into the disk portion */

                U64 diskCacheSize, maximumCacheSize;
                {
                    QMutexLocker k(&_sizeLock);
                    diskCacheSize = _diskCacheSize;
                    maximumCacheSize = _maximumCacheSize;
                }

                /*before that we need to clear the disk cache if it exceeds the maximum size allowed*/
                while (diskCacheSize + evictedFromMemory.second->size() >= maximumCacheSize) {
                    {
                        std::pair<hash_type, EntryTypePtr> evictedFromDisk = _diskCache.evict();
                        //if the cache couldn't evict that means all entries are used somewhere and we shall not remove them!
                        //we'll let the user of these entries purge the extra entries left in the cache later on
                        if (!evictedFromDisk.second) {
                            break;
                        }
                        ///Erase the file from the disk if we reach the limit.
                        evictedFromDisk.second->removeAnyBackingFile();
                    }
                    {
                        QMutexLocker k(&_sizeLock);
                        diskCacheSize = _diskCacheSize;
                        maximumCacheSize = _maximumCacheSize;
                    }
                }

                /*update the disk cache size*/
                CacheIterator existingDiskCacheEntry = _diskCache( evictedFromMemory.second->getHashKey() );
                /*if the entry doesn't exist on the disk cache,make a new list and insert it*/
                if ( existingDiskCacheEntry == _diskCache.end() ) {
                    _diskCache.insert(evictedFromMemory.second->getHashKey(), evictedFromMemory.second);
                }
            }

            evictedFromMemory = _memoryCache.evict();
        }

        _signalEmitter->blockSignals(false);
        if (emitSignals) {
            _signalEmitter->emitSignalClearedInMemoryPortion();
        }
    } // clearInMemoryPortion

    void clearExceedingEntries()
    {
        ///Make sure the shared_ptrs live in this list and are destroyed not while under the lock
        ///so that the memory freeing (which might be expensive for large images) doesn't happen while under the lock
        std::list<EntryTypePtr> entriesToBeDeleted;

        {
            QMutexLocker locker(&_lock);
            U64 memoryCacheSize, maximumInMemorySize;
            {
                QMutexLocker k(&_sizeLock);
                memoryCacheSize = _memoryCacheSize;
                maximumInMemorySize = std::max( (std::size_t)1, _maximumInMemorySize );
            }
            double occupationPercentage = (double)memoryCacheSize / maximumInMemorySize;
            while (occupationPercentage >= NATRON_CACHE_LIMIT_PERCENT) {
                std::list<EntryTypePtr> deleted;
                if ( !tryEvictEntry(deleted) ) {
                    break;
                }

                for (typename std::list<EntryTypePtr>::iterator it = deleted.begin(); it != deleted.end(); ++it) {
                    if ( !(*it)->isStoredOnDisk() ) {
                        memoryCacheSize -= (*it)->size();
                    }
                    entriesToBeDeleted.push_back(*it);
                }
                occupationPercentage = (double)memoryCacheSize / maximumInMemorySize;
            }
        }
    }

    /**
     * @brief Get a copy of the cache at the moment it gets the lock for reading.
     * Returning this function, the caller can assume the entries will not be removed
     * from the cache because their use_count is > 1
     **/
    void getCopy(std::list<EntryTypePtr>* copy) const
    {
        QMutexLocker locker(&_lock);

        for (CacheIterator it = _memoryCache.begin(); it != _memoryCache.end(); ++it) {
            const std::list<EntryTypePtr> & entries = getValueFromIterator(it);
            copy->insert( copy->end(), entries.begin(), entries.end() );
        }
        for (CacheIterator it = _diskCache.begin(); it != _diskCache.end(); ++it) {
            const std::list<EntryTypePtr> & entries = getValueFromIterator(it);
            copy->insert( copy->end(), entries.begin(), entries.end() );
        }
    }

    /**
     * @brief Removes the last recently used entry from the in-memory cache.
     * This is expensive since it takes the lock. Returns false
     * if there's nothing left to evict.
     **/
    bool evictLRUInMemoryEntry() const
    {
        ///Make sure the shared_ptrs live in this list and are destroyed not while under the lock
        ///so that the memory freeing (which might be expensive for large images) doesn't happen while under the lock
        std::list<EntryTypePtr> entriesToBeDeleted;
        bool ret;
        {
            QMutexLocker locker(&_lock);
            ret = tryEvictEntry(entriesToBeDeleted);
        }

        return ret;
    }

    /**
     * @brief Removes the last recently used entry from the disk cache.
     * This is expensive since it takes the lock. Returns false
     * if there's nothing left to evict.
     **/
    bool evictLRUDiskEntry() const
    {
        QMutexLocker locker(&_lock);
        std::pair<hash_type, EntryTypePtr> evicted = _diskCache.evict();

        //if the cache couldn't evict that means all entries are used somewhere and we shall not remove them!
        //we'll let the user of these entries purge the extra entries left in the cache later on
        if (!evicted.second) {
            return false;
        }
        /*if it is stored on disk, remove it from memory*/

        assert( evicted.second.unique() );
        evicted.second->removeAnyBackingFile();

        return true;
    }

    /**
     * @brief To be called by a CacheEntry whenever it's size changes.
     * This way the cache can keep track of the real memory footprint.
     **/
    virtual void notifyEntrySizeChanged(std::size_t oldSize,
                                        std::size_t newSize) const OVERRIDE FINAL
    {
        ///The entry has notified it's memory layout has changed, it must have been due to an action from the cache
        QMutexLocker k(&_sizeLock);

        ///This function can only be called for RAM buffers or while a memory mapped file is mapped into the RAM, so
        ///we just have to modify the RAM size.

        ///Avoid overflows, _memoryCacheSize may not always fallback to 0
        qint64 diff = (qint64)newSize - (qint64)oldSize;

        if (diff < 0) {
            _memoryCacheSize = diff > (qint64)_memoryCacheSize ? 0 : _memoryCacheSize + diff;
        } else {
            _memoryCacheSize += diff;
        }
#ifdef NATRON_DEBUG_CACHE
        qDebug() << cacheName().c_str() << " memory size: " << printAsRAM(_memoryCacheSize);
#endif
    }

    /**
     * @brief To be called by a CacheEntry on allocation.
     **/
    virtual void notifyEntryAllocated(int time,
                                      std::size_t size,
                                      Natron::StorageModeEnum storage) const OVERRIDE FINAL
    {
        ///The entry has notified it's memory layout has changed, it must have been due to an action from the cache, hence the
        ///lock should already be taken.
        QMutexLocker k(&_sizeLock);

        _memoryCacheSize += size;
        _signalEmitter->emitAddedEntry(time);

        if (storage == Natron::eStorageModeDisk) {
            appPTR->increaseNCacheFilesOpened();
        }
#ifdef NATRON_DEBUG_CACHE
        qDebug() << cacheName().c_str() << " memory size: " << printAsRAM(_memoryCacheSize);
#endif
    }

    /**
     * @brief To be called by a CacheEntry on destruction.
     **/
    virtual void notifyEntryDestroyed(int time,
                                      std::size_t size,
                                      Natron::StorageModeEnum storage) const OVERRIDE FINAL
    {
        QMutexLocker k(&_sizeLock);

        if (storage == Natron::eStorageModeRAM) {
            _memoryCacheSize = size > _memoryCacheSize ? 0 : _memoryCacheSize - size;
#ifdef NATRON_DEBUG_CACHE
            qDebug() << cacheName().c_str() << " memory size: " << printAsRAM(_memoryCacheSize);
#endif
        } else if (storage == Natron::eStorageModeDisk) {
            _diskCacheSize = size > _diskCacheSize ? 0 : _diskCacheSize - size;
#ifdef NATRON_DEBUG_CACHE
            qDebug() << cacheName().c_str() << " disk size: " << printAsRAM(_diskCacheSize);
#endif
        }


        _signalEmitter->emitRemovedEntry(time, (int)storage);
    }

    virtual void notifyMemoryDeallocated() const OVERRIDE FINAL
    {
        QMutexLocker k(&_sizeLock);

        _memoryFullCondition.wakeAll();
    }

    /**
     * @brief To be called whenever an entry is deallocated from memory and put back on disk or whenever
     * it is reallocated in the RAM.
     **/
    virtual void notifyEntryStorageChanged(Natron::StorageModeEnum oldStorage,
                                           Natron::StorageModeEnum newStorage,
                                           int time,
                                           std::size_t size) const OVERRIDE FINAL
    {
        if (_tearingDown) {
            return;
        }
        QMutexLocker k(&_sizeLock);

        assert(oldStorage != newStorage);
        assert(newStorage != Natron::eStorageModeNone);
        if (oldStorage == Natron::eStorageModeRAM) {
            _memoryCacheSize = size > _memoryCacheSize ? 0 : _memoryCacheSize - size;
            _diskCacheSize += size;
#ifdef NATRON_DEBUG_CACHE
            qDebug() << cacheName().c_str() << " memory size: " << printAsRAM(_memoryCacheSize);
            qDebug() << cacheName().c_str() << " disk size: " << printAsRAM(_diskCacheSize);
#endif
            ///We switched from RAM to DISK that means the MemoryFile object has been destroyed hence the file has been closed.
            appPTR->decreaseNCacheFilesOpened();
        } else if (oldStorage == Natron::eStorageModeDisk) {
            _memoryCacheSize += size;
            _diskCacheSize = size > _diskCacheSize ? 0 : _diskCacheSize - size;
#ifdef NATRON_DEBUG_CACHE
            qDebug() << cacheName().c_str() << " memory size: " << printAsRAM(_memoryCacheSize);
            qDebug() << cacheName().c_str() << " disk size: " << printAsRAM(_diskCacheSize);
#endif
            ///We switched from DISK to RAM that means the MemoryFile object has been created and the file opened
            appPTR->increaseNCacheFilesOpened();
        } else {
            if (newStorage == Natron::eStorageModeRAM) {
                _memoryCacheSize += size;
            } else if (newStorage == Natron::eStorageModeDisk) {
                _diskCacheSize += size;
            }
        }

        _signalEmitter->emitEntryStorageChanged(time, (int)oldStorage, (int)newStorage);
    }

    virtual void backingFileClosed() const OVERRIDE FINAL
    {
        appPTR->decreaseNCacheFilesOpened();
    }

    // const data member: no need to take the lock
    const std::string & cacheName() const
    {
        return _cacheName;
    }

    /*Returns the cache version as a string. This is
         used to know whether a cache is still valid when
         restoring*/
    // const data member: no need to take the lock
    unsigned int cacheVersion() const
    {
        return _version;
    }

    /*Returns the name of the cache with its path preprended*/
    QString getCachePath() const
    {
        QString cacheFolderName( appPTR->getDiskCacheLocation() );

        if ( !cacheFolderName.endsWith('\\') && !cacheFolderName.endsWith('/') ) {
            cacheFolderName.append('/');
        }
        cacheFolderName.append( cacheName().c_str() );

        return cacheFolderName;
    }

    std::string getRestoreFilePath() const
    {
        QString newCachePath( getCachePath() );

        newCachePath.append( QDir::separator() );
        newCachePath.append("restoreFile." NATRON_CACHE_FILE_EXT);

        return newCachePath.toStdString();
    }

    void setMaximumCacheSize(U64 newSize)
    {
        QMutexLocker k(&_sizeLock);

        _maximumCacheSize = newSize;
    }

    void setMaximumInMemorySize(double percentage)
    {
        QMutexLocker k(&_sizeLock);

        _maximumInMemorySize = _maximumCacheSize * percentage;
    }

    std::size_t getMaximumSize() const
    {
        QMutexLocker k(&_sizeLock); return _maximumCacheSize;
    }

    std::size_t getMaximumMemorySize() const
    {
        QMutexLocker k(&_sizeLock); return _maximumInMemorySize;
    }

    std::size_t getMemoryCacheSize() const
    {
        QMutexLocker k(&_sizeLock); return _memoryCacheSize;
    }

    std::size_t getDiskCacheSize() const
    {
        QMutexLocker k(&_sizeLock); return _diskCacheSize;
    }

    CacheSignalEmitter* activateSignalEmitter() const
    {
        return _signalEmitter;
    }

    /** @brief This function can be called to remove a specific entry from the cache. For example a frame
     * that has had its render aborted but already belong to the cache.
     **/
    void removeEntry(EntryTypePtr entry)
    {
        ///early return if entry is NULL
        if (!entry) {
            return;
        }
        std::list<EntryTypePtr> toRemove;

        {
            QMutexLocker l(&_lock);
            CacheIterator existingEntry = _memoryCache( entry->getHashKey() );
            if ( existingEntry != _memoryCache.end() ) {
                std::list<EntryTypePtr> & ret = getValueFromIterator(existingEntry);
                for (typename std::list<EntryTypePtr>::iterator it = ret.begin(); it != ret.end(); ++it) {
                    if ( (*it)->getKey() == entry->getKey() ) {
                        toRemove.push_back(*it);
                        //(*it)->scheduleForDestruction();
                        ret.erase(it);
                        break;
                    }
                }
                if ( ret.empty() ) {
                    _memoryCache.erase(existingEntry);
                }
            } else {
                existingEntry = _diskCache( entry->getHashKey() );
                if ( existingEntry != _diskCache.end() ) {
                    std::list<EntryTypePtr> & ret = getValueFromIterator(existingEntry);
                    for (typename std::list<EntryTypePtr>::iterator it = ret.begin(); it != ret.end(); ++it) {
                        if ( (*it)->getKey() == entry->getKey() ) {
                            //(*it)->scheduleForDestruction();
                            toRemove.push_back(*it);
                            ret.erase(it);
                            break;
                        }
                    }
                    if ( ret.empty() ) {
                        _diskCache.erase(existingEntry);
                    }
                }
            }
        } // QMutexLocker l(&_lock);
        if ( !toRemove.empty() ) {
            _deleterThread.appendToQueue(toRemove);

            ///Clearing the list here will not delete the objects pointing to by the shared_ptr's because we made a copy
            ///that the separate thread will delete
            toRemove.clear();
        }
    } // removeEntry

    void removeEntry(U64 hash)
    {
        std::list<EntryTypePtr> toRemove;
        {
            QMutexLocker l(&_lock);
            CacheIterator existingEntry = _memoryCache( hash);
            if ( existingEntry != _memoryCache.end() ) {
                std::list<EntryTypePtr> & ret = getValueFromIterator(existingEntry);
                for (typename std::list<EntryTypePtr>::iterator it = ret.begin(); it != ret.end(); ++it) {
                    //(*it)->scheduleForDestruction();
                    toRemove.push_back(*it);
                }
                _memoryCache.erase(existingEntry);
            } else {
                existingEntry = _diskCache( hash );
                if ( existingEntry != _diskCache.end() ) {
                    std::list<EntryTypePtr> & ret = getValueFromIterator(existingEntry);
                    for (typename std::list<EntryTypePtr>::iterator it = ret.begin(); it != ret.end(); ++it) {
                        //(*it)->scheduleForDestruction();
                        toRemove.push_back(*it);
                    }
                    _diskCache.erase(existingEntry);
                }
            }
        } // QMutexLocker l(&_lock);

        if ( !toRemove.empty() ) {
            _deleterThread.appendToQueue(toRemove);

            ///Clearing the list here will not delete the objects pointing to by the shared_ptr's because we made a copy
            ///that the separate thread will delete
            toRemove.clear();
        }
    }

    /*Saves cache to disk as a settings file.
     */
    void save(CacheTOC* tableOfContents);


    /*Restores the cache from disk.*/
    void restore(const CacheTOC & tableOfContents);

    void removeAllEntriesWithDifferentNodeHashForHolderPublic(const CacheEntryHolder* holder,
                                                              U64 nodeHash)
    {
        _cleanerThread.appendToQueue(holder->getCacheID(), nodeHash, false);
    }

    void removeAllEntriesForHolderPublic(const CacheEntryHolder* holder)
    {
        _cleanerThread.appendToQueue(holder->getCacheID(), 0, true);
    }
    
    void getMemoryStatsForCacheEntryHolder(const CacheEntryHolder* holder,
                                      std::size_t* ramOccupied,
                                      std::size_t* diskOccupied) const
    {
        *ramOccupied = 0;
        *diskOccupied= 0;
        
        std::string holderID = holder->getCacheID();
        
        QMutexLocker locker(&_lock);
        
        for (CacheIterator memIt = _memoryCache.begin(); memIt != _memoryCache.end(); ++memIt) {
            std::list<EntryTypePtr> & entries = getValueFromIterator(memIt);
            if ( !entries.empty() ) {
                
                const EntryTypePtr & front = entries.front();
                
                if (front->getKey().getCacheHolderID() == holderID) {
                    for (typename std::list<EntryTypePtr>::iterator it = entries.begin(); it != entries.end(); ++it) {
                        *ramOccupied += (*it)->size();
                    }
                }
            }
        }
        
        for (CacheIterator memIt = _diskCache.begin(); memIt != _diskCache.end(); ++memIt) {
            std::list<EntryTypePtr> & entries = getValueFromIterator(memIt);
            if ( !entries.empty() ) {
                
                const EntryTypePtr & front = entries.front();
                
                if (front->getKey().getCacheHolderID() == holderID) {
                    for (typename std::list<EntryTypePtr>::iterator it = entries.begin(); it != entries.end(); ++it) {
                        *diskOccupied += (*it)->size();
                    }
                }
            }
        }
    }

private:

    virtual void removeAllEntriesWithDifferentNodeHashForHolderPrivate(const std::string & holderID,
                                                                       U64 nodeHash,
                                                                       bool removeAll) OVERRIDE FINAL
    {
        std::list<EntryTypePtr> toDelete;
        CacheContainer newMemCache, newDiskCache;
        {
            QMutexLocker locker(&_lock);

            for (CacheIterator memIt = _memoryCache.begin(); memIt != _memoryCache.end(); ++memIt) {
                std::list<EntryTypePtr> & entries = getValueFromIterator(memIt);
                if ( !entries.empty() ) {
                    const EntryTypePtr & front = entries.front();

                    if ( (front->getKey().getCacheHolderID() == holderID) &&
                         ( ( front->getKey().getTreeVersion() != nodeHash) || removeAll ) ) {
                        for (typename std::list<EntryTypePtr>::iterator it = entries.begin(); it != entries.end(); ++it) {
                            //(*it)->scheduleForDestruction();
                            toDelete.push_back(*it);
                        }
                    } else {
                        typename EntryType::hash_type hash = front->getHashKey();
                        newMemCache.insert(hash, entries);
                    }
                }
            }

            for (CacheIterator dIt = _diskCache.begin(); dIt != _diskCache.end(); ++dIt) {
                std::list<EntryTypePtr> & entries = getValueFromIterator(dIt);
                if ( !entries.empty() ) {
                    const EntryTypePtr & front = entries.front();

                    if ( (front->getKey().getCacheHolderID() == holderID) &&
                         ( ( front->getKey().getTreeVersion() != nodeHash) || removeAll ) ) {
                        for (typename std::list<EntryTypePtr>::iterator it = entries.begin(); it != entries.end(); ++it) {
                            //(*it)->scheduleForDestruction();
                            toDelete.push_back(*it);
                        }
                    } else {
                        typename EntryType::hash_type hash = front->getHashKey();
                        newDiskCache.insert(hash, entries);
                    }
                }
            }

            _memoryCache = newMemCache;
            _diskCache = newDiskCache;
        } // QMutexLocker locker(&_lock);

        if ( !toDelete.empty() ) {
            _deleterThread.appendToQueue(toDelete);

            ///Clearing the list here will not delete the objects pointing to by the shared_ptr's because we made a copy
            ///that the separate thread will delete
            toDelete.clear();
        }
    } // removeAllEntriesWithDifferentNodeHashForHolderPrivate

    bool getInternal(const typename EntryType::key_type & key,
                     std::list<EntryTypePtr>* returnValue) const
    {
        ///Private should be locked
        assert( !_lock.tryLock() );

        ///find a matching value in the internal memory container
        CacheIterator memoryCached = _memoryCache( key.getHash() );

        if ( memoryCached != _memoryCache.end() ) {
            ///we found something with a matching hash key. There may be several entries linked to
            ///this key, we need to find one with matching params
            std::list<EntryTypePtr> & ret = getValueFromIterator(memoryCached);
            for (typename std::list<EntryTypePtr>::const_iterator it = ret.begin(); it != ret.end(); ++it) {
                if ( (*it)->getKey() == key ) {
                    returnValue->push_back(*it);

                    ///Q_EMIT te added signal otherwise when first reading something that's already cached
                    ///the timeline wouldn't update
                    if (_signalEmitter) {
                        _signalEmitter->emitAddedEntry( key.getTime() );
                    }
                }
            }

            return returnValue->size() > 0;
        } else {
            ///fallback on the disk cache internal container
            CacheIterator diskCached = _diskCache( key.getHash() );

            if ( diskCached == _diskCache.end() ) {
                /*the entry was neither in memory or disk, just allocate a new one*/
                return false;
            } else {
                /*we found something with a matching hash key. There may be several entries linked to
                   this key, we need to find one with matching values(operator ==)*/
                std::list<EntryTypePtr> & ret = getValueFromIterator(diskCached);

                for (typename std::list<EntryTypePtr>::iterator it = ret.begin();
                     it != ret.end(); ++it) {
                    if ( (*it)->getKey() == key ) {
                        /*If we found 1 entry in the list that has exactly the same key params,
                           we re-open the mapping to the RAM put the entry
                           back into the memoryCache.*/

                        if ( ret.empty() ) {
                            _diskCache.erase(diskCached);
                        }

                        try {
                            (*it)->reOpenFileMapping();
                        } catch (const std::exception & e) {
                            qDebug() << "Error while reopening cache file: " << e.what();
                            ret.erase(it);

                            return false;
                        } catch (...) {
                            qDebug() << "Error while reopening cache file";
                            ret.erase(it);

                            return false;
                        }

                        //put it back into the RAM
                        _memoryCache.insert( (*it)->getHashKey(), *it );

                        U64 memoryCacheSize, maximumInMemorySize;
                        {
                            QMutexLocker k(&_sizeLock);
                            memoryCacheSize = _memoryCacheSize;
                            maximumInMemorySize = _maximumInMemorySize;
                        }
                        std::list<EntryTypePtr> entriesToBeDeleted;

                        //now clear extra entries from the disk cache so it doesn't exceed the RAM limit.
                        while (memoryCacheSize > maximumInMemorySize) {
                            if ( !tryEvictEntry(entriesToBeDeleted) ) {
                                break;
                            }

                            {
                                QMutexLocker k(&_sizeLock);
                                memoryCacheSize = _memoryCacheSize;
                                maximumInMemorySize = _maximumInMemorySize;
                            }
                        }

                        returnValue->push_back(*it);
                        ret.erase(it);
                        ///Q_EMIT te added signal otherwise when first reading something that's already cached
                        ///the timeline wouldn't update
                        if (_signalEmitter) {
                            _signalEmitter->emitAddedEntry( key.getTime() );
                        }

                        return true;
                    }
                }

                /*if we reache here it means no entries linked to the hash key matches the params,then
                   we allocate a new one*/
                return false;
            }
        }
    } // getInternal

    /** @brief Inserts into the cache an entry that was previously allocated by the createInternal()
     * function. This is called directly by createInternal() if the allocation was successful
     **/
    void sealEntry(const EntryTypePtr & entry,
                   bool inMemory) const
    {
        assert( !_lock.tryLock() );   // must be locked
        typename EntryType::hash_type hash = entry->getHashKey();

        if (inMemory) {
            /*if the entry doesn't exist on the memory cache,make a new list and insert it*/
            CacheIterator existingEntry = _memoryCache(hash);
            if ( existingEntry == _memoryCache.end() ) {
                _memoryCache.insert(hash, entry);
            } else {
                /*append to the existing list*/
                getValueFromIterator(existingEntry).push_back(entry);
            }
        } else {
            CacheIterator existingEntry = _diskCache(hash);
            if ( existingEntry == _diskCache.end() ) {
                _diskCache.insert(hash, entry);
            } else {
                /*append to the existing list*/
                getValueFromIterator(existingEntry).push_back(entry);
            }
        }
    }

    bool tryEvictEntry(std::list<EntryTypePtr> & entriesToBeDeleted) const
    {
        assert( !_lock.tryLock() );
        std::pair<hash_type, EntryTypePtr> evicted = _memoryCache.evict();
        //if the cache couldn't evict that means all entries are used somewhere and we shall not remove them!
        //we'll let the user of these entries purge the extra entries left in the cache later on
        if (!evicted.second) {
            return false;
        }
        /*if it is stored on disk, remove it from memory*/

        if ( evicted.second->isStoredOnDisk() ) {
            assert( evicted.second.unique() );

            ///This is EXPENSIVE! it calls msync
            evicted.second->deallocate();

            /*insert it back into the disk portion */

            U64 diskCacheSize, maximumCacheSize, maximumInMemorySize;
            {
                QMutexLocker k(&_sizeLock);
                diskCacheSize = _diskCacheSize;
                maximumInMemorySize = _maximumInMemorySize;
                maximumCacheSize = _maximumCacheSize;
            }

            /*before that we need to clear the disk cache if it exceeds the maximum size allowed*/
            while ( ( diskCacheSize  + evicted.second->size() ) >= (maximumCacheSize - maximumInMemorySize) ) {
                {
                    std::pair<hash_type, EntryTypePtr> evictedFromDisk = _diskCache.evict();
                    //if the cache couldn't evict that means all entries are used somewhere and we shall not remove them!
                    //we'll let the user of these entries purge the extra entries left in the cache later on
                    if (!evictedFromDisk.second) {
                        break;
                    }

                    ///Erase the file from the disk if we reach the limit.
                    //evictedFromDisk.second->scheduleForDestruction();


                    entriesToBeDeleted.push_back(evictedFromDisk.second);
                }
                {
                    QMutexLocker k(&_sizeLock);
                    diskCacheSize = _diskCacheSize;
                    maximumInMemorySize = _maximumInMemorySize;
                    maximumCacheSize = _maximumCacheSize;
                }
            }

            CacheIterator existingDiskCacheEntry = _diskCache(evicted.first);
            /*if the entry doesn't exist on the disk cache,make a new list and insert it*/
            if ( existingDiskCacheEntry == _diskCache.end() ) {
                _diskCache.insert(evicted.first, evicted.second);
            } else {   /*append to the existing list*/
                getValueFromIterator(existingDiskCacheEntry).push_back(evicted.second);
            }
        } else {
            entriesToBeDeleted.push_back(evicted.second);
        }

        return true;
    } // tryEvictEntry
};
} // namespace Natron


#endif /*NATRON_ENGINE_ABSTRACTCACHE_H_ */
