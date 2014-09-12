//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#ifndef NATRON_ENGINE_ABSTRACTCACHE_H_
#define NATRON_ENGINE_ABSTRACTCACHE_H_

#include <vector>
#include <sstream>
#include <fstream>
#include <functional>
#include <list>
#include <cstddef>

#include "Global/GlobalDefines.h"
#include "Global/MemoryInfo.h"
CLANG_DIAG_OFF(deprecated)
#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtCore/QObject>
#include <QtCore/QDebug>
#include <QtCore/QTextStream>
#include <QtCore/QBuffer>
CLANG_DIAG_ON(deprecated)
#include <boost/shared_ptr.hpp>
CLANG_DIAG_OFF(unused-parameter)
// /opt/local/include/boost/serialization/smart_cast.hpp:254:25: warning: unused parameter 'u' [-Wunused-parameter]
#include <boost/archive/binary_iarchive.hpp>
CLANG_DIAG_ON(unused-parameter)
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/export.hpp>

#include "Engine/AppManager.h" //for access to settings
#include "Engine/Settings.h"
#include "Engine/ImageSerialization.h"
#include "Engine/ImageParamsSerialization.h"
#include "Engine/FrameEntrySerialization.h"
#include "Engine/FrameParamsSerialization.h"
#include "Engine/CacheEntry.h"
#include "Engine/LRUHashTable.h"
#include "Engine/StandardPaths.h"
#include "Global/MemoryInfo.h"
///When defined, number of opened files, memory size and disk size of the cache are printed whenever there's activity.
//#define NATRON_DEBUG_CACHE

namespace Natron {
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
        emit clearedInMemoryPortion();
    }

    void emitClearedDiskPortion()
    {
        emit clearedDiskPortion();
    }

    void emitAddedEntry(SequenceTime time)
    {
        emit addedEntry(time);
    }

    void emitRemovedEntry(SequenceTime time,
                          int storage)
    {
        emit removedEntry(time,storage);
    }

    void emitEntryStorageChanged(SequenceTime time,
                                 int oldStorage,
                                 int newStorage)
    {
        emit entryStorageChanged(time,oldStorage,newStorage);
    }

signals:

    void clearedInMemoryPortion();

    void clearedDiskPortion();

    void addedEntry(SequenceTime);
    void removedEntry(SequenceTime,int);
    void entryStorageChanged(SequenceTime,int,int);
};


/*
 * ValueType must be derived of CacheEntryHelper
 */
template<typename EntryType>
class Cache
    : public CacheAPI
{
public:


    typedef boost::shared_ptr<NonKeyParams> NonKeyParamsPtr;
    typedef boost::shared_ptr<EntryType> EntryTypePtr;
    typedef typename EntryType::hash_type hash_type;
    typedef typename EntryType::data_t data_t;
    typedef typename EntryType::key_t key_t;

    struct SerializedEntry
    {
        hash_type hash;
        typename EntryType::key_type key;
        NonKeyParamsPtr params;


        template<class Archive>
        void serialize(Archive & ar,
                       const unsigned int /*version*/)
        {
            ar & boost::serialization::make_nvp("Hash",hash);
            ar & boost::serialization::make_nvp("Key",key);
            ar & boost::serialization::make_nvp("Params",params);
        }
    };

    typedef std::list< SerializedEntry > CacheTOC;

private:


    struct CachedValue
    {
        EntryTypePtr entry;
        NonKeyParamsPtr params;
    };

public:


#ifdef USE_VARIADIC_TEMPLATES

#ifdef NATRON_CACHE_USE_BOOST
#ifdef NATRON_CACHE_USE_HASH
    typedef BoostLRUHashTable<hash_type, CachedValue>, boost::bimaps::unordered_set_of > CacheContainer;
#else
    typedef BoostLRUHashTable<hash_type, CachedValue >, boost::bimaps::set_of > CacheContainer;
#endif
    typedef typename CacheContainer::container_type::left_iterator CacheIterator;
    typedef typename CacheContainer::container_type::left_const_iterator ConstCacheIterator;
    static std::list<CachedValue> &  getValueFromIterator(CacheIterator it)
    {
        return it->second;
    }

#else // cache use STL

#ifdef NATRON_CACHE_USE_HASH
    typedef StlLRUHashTable<hash_type,CachedValue >, std::unordered_map > CacheContainer;
#else
    typedef StlLRUHashTable<hash_type,CachedValue >, std::map > CacheContainer;
#endif
    typedef typename CacheContainer::key_to_value_type::iterator CacheIterator;
    typedef typename CacheContainer::key_to_value_type::const_iterator ConstCacheIterator;
    static std::list<CachedValue> &  getValueFromIterator(CacheIterator it)
    {
        return it->second;
    }

#endif // NATRON_CACHE_USE_BOOST

#else // !USE_VARIADIC_TEMPLATES

#ifdef NATRON_CACHE_USE_BOOST
#ifdef NATRON_CACHE_USE_HASH
    typedef BoostLRUHashTable<hash_type,CachedValue> CacheContainer;
#else
    typedef BoostLRUHashTable<hash_type, CachedValue > CacheContainer;
#endif
    typedef typename CacheContainer::container_type::left_iterator CacheIterator;
    typedef typename CacheContainer::container_type::left_const_iterator ConstCacheIterator;
    static std::list<CachedValue> &  getValueFromIterator(CacheIterator it)
    {
        return it->second;
    }

#else // cache use STL and tree (std map)

    typedef StlLRUHashTable<hash_type, CachedValue > CacheContainer;
    typedef typename CacheContainer::key_to_value_type::iterator CacheIterator;
    typedef typename CacheContainer::key_to_value_type::const_iterator ConstCacheIterator;
    static std::list<CachedValue> &   getValueFromIterator(CacheIterator it)
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
    mutable QMutex _lock;

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

public:


    Cache(const std::string & cacheName
          ,
          unsigned int version
          ,
          U64 maximumCacheSize      // total size
          ,
          double maximumInMemoryPercentage)      //how much should live in RAM
        : _maximumInMemorySize(maximumCacheSize * maximumInMemoryPercentage)
          ,_maximumCacheSize(maximumCacheSize)
          ,_memoryCacheSize(0)
          ,_diskCacheSize(0)
          ,_lock()
          ,_memoryCache()
          ,_diskCache()
          ,_cacheName(cacheName)
          ,_version(version)
          ,_signalEmitter(NULL)
          ,_maxPhysicalRAM( getSystemTotalRAM() )
    {
    }

    virtual ~Cache()
    {
        QMutexLocker locker(&_lock);

        _memoryCache.clear();
        _diskCache.clear();
        if (_signalEmitter) {
            delete _signalEmitter;
        }
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
             NonKeyParamsPtr* params,
             EntryTypePtr* returnValue) const
    {
        ///lock the cache before reading it.
        QMutexLocker locker(&_lock);

        ///find a matching value in the internal memory container
        CacheIterator memoryCached = _memoryCache( key.getHash() );

        if ( memoryCached != _memoryCache.end() ) {
            /*we found something with a matching hash key. There may be several entries linked to
                this key, we need to find one with matching params*/
            std::list<CachedValue> & ret = getValueFromIterator(memoryCached);
            for (typename std::list<CachedValue>::const_iterator it = ret.begin(); it != ret.end(); ++it) {
                if (it->entry->getKey() == key) {
                    *returnValue = it->entry;
                    *params = it->params;

                    ///emit te added signal otherwise when first reading something that's already cached
                    ///the timeline wouldn't update
                    if (_signalEmitter) {
                        _signalEmitter->emitAddedEntry( key.getTime() );
                    }

                    return true;
                }
            }

            return false;
        } else {
            ///fallback on the disk cache internal container
            CacheIterator diskCached = _diskCache( key.getHash() );

            if ( diskCached == _diskCache.end() ) {
                /*the entry was neither in memory or disk, just allocate a new one*/
                return false;
            } else {
                /*we found something with a matching hash key. There may be several entries linked to
                     this key, we need to find one with matching values(operator ==)*/
                std::list<CachedValue> & ret = getValueFromIterator(diskCached);

                for (typename std::list<CachedValue>::iterator it = ret.begin();
                     it != ret.end(); ++it) {
                    if (it->entry->getKey() == key) {
                        /*If we found 1 entry in the list that has exactly the same key params,
                           we re-open the mapping to the RAM put the entry
                           back into the memoryCache.*/

                        if ( ret.empty() ) {
                            _diskCache.erase(diskCached);
                        }

                        try {
                            it->entry->reOpenFileMapping();
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
                        _memoryCache.insert(it->entry->getHashKey(),*it);

                        //now clear extra entries from the disk cache so it doesn't exceed the RAM limit.
                        while (_memoryCacheSize > _maximumInMemorySize) {
                            if ( !tryEvictEntry() ) {
                                break;
                            }
                        }

                        *returnValue = it->entry;
                        *params = it->params;
                        ret.erase(it);
                        ///emit te added signal otherwise when first reading something that's already cached
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
    } // get

    /**
     * @brief Look-up the cache for an entry whose key matches the params.
     * Unlike get(...) this function creates a new entry if it couldn't be found.
     * Note that this function also takes extra parameters which go along the value_type
     * and will be cached. These parameters aren't taken into account for the computation of
     * the hash key. It is a safe place to cache any extra data that is relative to an entry,
     * but doesn't make it an identifier of that entry. The base class just adds the necessary
     * infos for the cache to be able to instantiate a new entry (that is the cost and the elements count).
     * @param key The key identifying the entry we're looking for.
     * @param params The non unique parameters. They do not help to identify they entry, rather
     * this class can be used to cache other parameters along with the value_type.
     * @param [out] returnValue The returnValue, contains the cache entry.
     * Internally the allocation of the new entry might fail on the requested device,
     * e.g: if you ask for an entry with a large cost, the cache will try to put the
     * entry on disk to preserve it, but if the allocation failed it will fallback
     * on RAM instead.
     * Either way the returnValue parameter can never be NULL.
     * @returns True if the cache successfully found an entry matching the key.
     * False otherwise.
     **/
    bool getOrCreate(const typename EntryType::key_type & key,
                     NonKeyParamsPtr params,
                     EntryTypePtr* returnValue) const
    {
        NonKeyParamsPtr cachedParams;

        if ( !get(key,&cachedParams,returnValue) ) {
            
            ///Before allocating the memory check that there's enough space to fit in memory
            appPTR->checkCacheFreeMemoryIsGoodEnough();
            
            
            ///Just in case, we don't allow more than X files to be removed at once.
            int safeCounter = 0;
            ///If too many files are opened, fall-back on RAM storage.
            while ( appPTR->isNCacheFilesOpenedCapped() && safeCounter < 1000 ) {
#ifdef NATRON_DEBUG_CACHE
                qDebug() << "Reached maximum cache files opened limit,clearing last recently used one...";
#endif
                if ( !evictLRUDiskEntry() ) {
                    break;
                }
                ++safeCounter;
            }
            
            ///lock the cache before writing it.
            QMutexLocker locker(&_lock);
            *returnValue = newEntry(key,params);

            return false;
        } else {
#ifdef DEBUG
            if (*cachedParams != *params) {
                qDebug() << "WARNING: A cache entry was found in the cache for the given key, but the cached parameters that "
                    " go along the entry do not match what's expected. This is a bug.";
            }
#endif
            return true;
        }
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
        std::pair<hash_type,CachedValue> evictedFromMemory = _memoryCache.evict();
        while (evictedFromMemory.second.entry) {
            if ( evictedFromMemory.second.entry->isStoredOnDisk() ) {
                evictedFromMemory.second.entry->removeAnyBackingFile();
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
        std::pair<hash_type,CachedValue> evictedFromDisk = _diskCache.evict();
        //if the cache couldn't evict that means all entries are used somewhere and we shall not remove them!
        //we'll let the user of these entries purge the extra entries left in the cache later on
        while (evictedFromDisk.second.entry) {
            evictedFromDisk.second.entry->removeAnyBackingFile();
            evictedFromDisk = _diskCache.evict();
        }

        
        if (_signalEmitter) {
            _signalEmitter->blockSignals(false);
            _signalEmitter->emitClearedDiskPortion();
        }
    }

    /**
     * @brief Clears the memory portion and moves it to the disk portion if possible
     **/
    void clearInMemoryPortion()
    {
        if (_signalEmitter) {
            ///block signals otherwise the we would be spammed of notifications
            _signalEmitter->blockSignals(true);
        }
        QMutexLocker locker(&_lock);
        std::pair<hash_type,CachedValue> evictedFromMemory = _memoryCache.evict();
        while (evictedFromMemory.second.entry) {
            ///move back the entry on disk if it can be store on disk
            if ( evictedFromMemory.second.entry->isStoredOnDisk() ) {
                evictedFromMemory.second.entry->deallocate();
                /*insert it back into the disk portion */

                /*before that we need to clear the disk cache if it exceeds the maximum size allowed*/
                while (_diskCacheSize + evictedFromMemory.second.entry->size() >= _maximumCacheSize) {
                    std::pair<hash_type,CachedValue> evictedFromDisk = _diskCache.evict();
                    //if the cache couldn't evict that means all entries are used somewhere and we shall not remove them!
                    //we'll let the user of these entries purge the extra entries left in the cache later on
                    if (!evictedFromDisk.second.entry) {
                        break;
                    }
                    ///Erase the file from the disk if we reach the limit.
                    evictedFromDisk.second.entry->removeAnyBackingFile();
                }

                /*update the disk cache size*/
                CacheIterator existingDiskCacheEntry = _diskCache( evictedFromMemory.second.entry->getHashKey() );
                /*if the entry doesn't exist on the disk cache,make a new list and insert it*/
                if ( existingDiskCacheEntry == _diskCache.end() ) {
                    _diskCache.insert(evictedFromMemory.second.entry->getHashKey(),evictedFromMemory.second);
                }
            }

            evictedFromMemory = _memoryCache.evict();
        }

        if (_signalEmitter) {
            _signalEmitter->blockSignals(false);
            _signalEmitter->emitSignalClearedInMemoryPortion();
        }
    }

    void clearExceedingEntries()
    {
        QMutexLocker locker(&_lock);

        while (_memoryCacheSize >= _maximumInMemorySize) {
            if ( !tryEvictEntry() ) {
                break;
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
            const std::list<CachedValue> & entries = getValueFromIterator(it);
            for (typename std::list<CachedValue>::const_iterator it2 = entries.begin(); it2 != entries.end(); ++it2) {
                copy->push_back(it2->entry);
            }
        }
        for (CacheIterator it = _diskCache.begin(); it != _diskCache.end(); ++it) {
            const std::list<CachedValue> & entries = getValueFromIterator(it);
            for (typename std::list<CachedValue>::const_iterator it2 = entries.begin(); it2 != entries.end(); ++it2) {
                copy->push_back(it2->entry);
            }
        }
    }
    
    /**
     * @brief Removes the last recently used entry from the in-memory cache.
     * This is expensive since it takes the lock. Returns false
     * if there's nothing left to evict.
     **/
    bool evictLRUInMemoryEntry() const
    {
        QMutexLocker locker(&_lock);
        return tryEvictEntry();
    }
    
    /**
     * @brief Removes the last recently used entry from the disk cache.
     * This is expensive since it takes the lock. Returns false
     * if there's nothing left to evict.
     **/
    bool evictLRUDiskEntry() const {
        QMutexLocker locker(&_lock);
        
        std::pair<hash_type,CachedValue> evicted = _diskCache.evict();
        //if the cache couldn't evict that means all entries are used somewhere and we shall not remove them!
        //we'll let the user of these entries purge the extra entries left in the cache later on
        if (!evicted.second.entry) {
            return false;
        }
        /*if it is stored on disk, remove it from memory*/
        
        assert( evicted.second.entry.unique() );
        evicted.second.entry->removeAnyBackingFile();
        
        return true;
    }

    /**
     * @brief To be called by a CacheEntry whenever it's size changes.
     * This way the cache can keep track of the real memory footprint.
     **/
    virtual void notifyEntrySizeChanged(std::size_t oldSize,
                                        std::size_t newSize) const OVERRIDE FINAL
    {
        ///The entry has notified it's memory layout has changed, it must have been due to an action from the cache, hence the
        ///lock should already be taken.
        assert( !_lock.tryLock() );

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
                                      Natron::StorageMode storage) const OVERRIDE FINAL
    {
        ///The entry has notified it's memory layout has changed, it must have been due to an action from the cache, hence the
        ///lock should already be taken.
        assert( !_lock.tryLock() );
        _memoryCacheSize += size;
        if (_signalEmitter) {
            _signalEmitter->emitAddedEntry(time);
        }

        if (storage == Natron::DISK) {
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
                                      Natron::StorageMode storage) const OVERRIDE FINAL
    {
        ///The entry could be destoryed at any time when the boost shared ptr use count reaches 0.
        ///This might not be while the cache is still under control of the thread safety
        ///make sure we lock this part by trying to lock
        bool gotLock = _lock.tryLock();

        if (storage == Natron::RAM) {
            _memoryCacheSize = size > _memoryCacheSize ? 0 : _memoryCacheSize - size;
#ifdef NATRON_DEBUG_CACHE
            qDebug() << cacheName().c_str() << " memory size: " << printAsRAM(_memoryCacheSize);
#endif
        } else if (storage == Natron::DISK) {
            _diskCacheSize = size > _diskCacheSize ? 0 : _diskCacheSize - size;
#ifdef NATRON_DEBUG_CACHE
            qDebug() << cacheName().c_str() << " disk size: " << printAsRAM(_diskCacheSize);
#endif
        }
        if (gotLock) {
            _lock.unlock();
        }
        if (_signalEmitter) {
            _signalEmitter->emitRemovedEntry(time,(int)storage);
        }
    }

    /**
     * @brief To be called whenever an entry is deallocated from memory and put back on disk or whenever
     * it is reallocated in the RAM.
     **/
    virtual void notifyEntryStorageChanged(Natron::StorageMode oldStorage,
                                           Natron::StorageMode newStorage,
                                           int time,
                                           std::size_t size) const OVERRIDE FINAL
    {
        ///The entry could be destoryed at any time when the boost shared ptr use count reaches 0.
        ///This might not be while the cache is still under control of the thread safety
        ///make sure we lock this part by trying to lock
        bool gotLock = _lock.tryLock();

        assert(oldStorage != newStorage);

        if (oldStorage == Natron::RAM) {
            _memoryCacheSize = size > _memoryCacheSize ? 0 : _memoryCacheSize - size;
            _diskCacheSize += size;
#ifdef NATRON_DEBUG_CACHE
            qDebug() << cacheName().c_str() << " memory size: " << printAsRAM(_memoryCacheSize);
            qDebug() << cacheName().c_str() << " disk size: " << printAsRAM(_diskCacheSize);
#endif
            ///We switched from RAM to DISK that means the MemoryFile object has been destroyed hence the file has been closed.
            appPTR->decreaseNCacheFilesOpened();
        } else {
            _memoryCacheSize += size;
            _diskCacheSize = size > _diskCacheSize ? 0 : _diskCacheSize - size;
#ifdef NATRON_DEBUG_CACHE
            qDebug() << cacheName().c_str() << " memory size: " << printAsRAM(_memoryCacheSize);
            qDebug() << cacheName().c_str() << " disk size: " << printAsRAM(_diskCacheSize);
#endif
            ///We switched from DISK to RAM that means the MemoryFile object has been created and the file opened
            appPTR->increaseNCacheFilesOpened();
        }
        if (gotLock) {
            _lock.unlock();
        }
        if (_signalEmitter) {
            _signalEmitter->emitEntryStorageChanged(time, (int)oldStorage, (int)newStorage);
        }
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
        QString cacheFolderName( Natron::StandardPaths::writableLocation(Natron::StandardPaths::CacheLocation) + QDir::separator() );

        cacheFolderName.append( QDir::separator() );
        QString str(cacheFolderName);
        str.append( cacheName().c_str() );

        return str;
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
        _maximumCacheSize = newSize;
    }

    void setMaximumInMemorySize(double percentage)
    {
        _maximumInMemorySize = _maximumCacheSize * percentage;
    }

    std::size_t getMaximumSize() const
    {
        QMutexLocker locker(&_lock); return _maximumCacheSize;
    }

    std::size_t getMaximumMemorySize() const
    {
        QMutexLocker locker(&_lock); return _maximumInMemorySize;
    }

    std::size_t getMemoryCacheSize() const
    {
        QMutexLocker locker(&_lock); return _memoryCacheSize;
    }

    std::size_t getDiskCacheSize() const
    {
        QMutexLocker locker(&_lock); return _diskCacheSize;
    }

    CacheSignalEmitter* activateSignalEmitter() const
    {
        QMutexLocker locker(&_lock);

        if (!_signalEmitter) {
            _signalEmitter = new CacheSignalEmitter;
        }

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

        QMutexLocker l(&_lock);
        CacheIterator existingEntry = _memoryCache( entry->getHashKey() );
        if ( existingEntry != _memoryCache.end() ) {
            std::list<CachedValue> & ret = getValueFromIterator(existingEntry);
            for (typename std::list<CachedValue>::iterator it = ret.begin(); it != ret.end(); ++it) {
                if ( it->entry->getKey() == entry->getKey() ) {
                    if ( it->entry->isStoredOnDisk() ) {
                        it->entry->removeAnyBackingFile();
                    }
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
                std::list<CachedValue> & ret = getValueFromIterator(existingEntry);
                for (typename std::list<CachedValue>::iterator it = ret.begin(); it != ret.end(); ++it) {
                    if ( it->entry->getKey() == entry->getKey() ) {
                        if ( it->entry->isStoredOnDisk() ) {
                            it->entry->removeAnyBackingFile();
                        }
                        ret.erase(it);
                        break;
                    }
                }
                if ( ret.empty() ) {
                    _diskCache.erase(existingEntry);
                }
            }
        }
    }

    /*Saves cache to disk as a settings file.
     */
    void save(CacheTOC* tableOfContents)
    {
        clearInMemoryPortion();
        QMutexLocker l(&_lock);     // must be locked

        for (CacheIterator it = _diskCache.begin(); it != _diskCache.end(); ++it) {
            std::list<CachedValue> & listOfValues  = getValueFromIterator(it);
            for (typename std::list<CachedValue>::const_iterator it2 = listOfValues.begin(); it2 != listOfValues.end(); ++it2) {
                if ( it2->entry->isStoredOnDisk() ) {
                    SerializedEntry serialization;
                    serialization.hash = it2->entry->getHashKey();
                    serialization.params = it2->params;
                    serialization.key = it2->entry->getKey();
                    tableOfContents->push_back(serialization);
                }
            }
        }
    }

    /*Restores the cache from disk.*/
    void restore(const CacheTOC & tableOfContents)
    {

        for (typename CacheTOC::const_iterator it =
                 tableOfContents.begin(); it != tableOfContents.end(); ++it) {
            if ( it->hash != it->key.getHash() ) {
                /*
                 * If this warning is printed this means that the value computed by it->key()
                 * is different than the value stored prior to serialiazing this entry. In other words there're
                 * 2 possibilities:
                 * 1) The key has changed since it has been added to the cache: maybe you forgot to serialize some
                 * members of the key or you didn't save them correctly.
                 * 2) The hash key computation is unreliable and is depending upon changing or non-deterministic
                 * parameters which is wrong.
                 */
                qDebug() << "WARNING: serialized hash key different than the restored one";
            }
            EntryType* value = NULL;

            ///Before allocating the memory check that there's enough space to fit in memory
            appPTR->checkCacheFreeMemoryIsGoodEnough();
           
            size_t entryDataSize = it->params->getElementsCount() * sizeof(data_t);
            
            {
                QMutexLocker locker(&_lock);
                
                while ( (_memoryCacheSize + entryDataSize >= _maximumInMemorySize) ) {
                    if ( !tryEvictEntry() ) {
                        break;
                    }
                }
            }
            Natron::StorageMode storage = Natron::DISK;

            
            ///Just in case, we don't allow more than X files to be removed at once.
            int safeCounter = 0;
            ///If too many files are opened, fall-back on RAM storage.
            while ( appPTR->isNCacheFilesOpenedCapped() && safeCounter < 1000 ) {
#ifdef NATRON_DEBUG_CACHE
            qDebug() << "Reached maximum cache files opened limit,clearing last recently used one...";
#endif
                if ( !evictLRUDiskEntry() ) {
                    break;
                }
                ++safeCounter;
            }
            
            {
                QMutexLocker locker(&_lock);
                
                try {
                    value = new EntryType(it->key,it->params,this);
                    value->allocateMemory( true, storage, QString( getCachePath() + QDir::separator() ).toStdString() );
                } catch (const std::bad_alloc & e) {
                    qDebug() << e.what();
                    continue;
                }
                
                CachedValue cachedValue;
                cachedValue.entry = EntryTypePtr(value);
                cachedValue.params = it->params;
                
                sealEntry(cachedValue);
            }
        }
    }

private:


    /** @brief Allocates a new entry by the cache. On failure a NULL pointer is returned.
     * The storage is then handled by the cache solely.
     **/
    EntryTypePtr newEntry(const typename EntryType::key_type & key,
                          const NonKeyParamsPtr & params) const
    {
        assert( !_lock.tryLock() );   // must be locked
        EntryTypePtr entryptr;
        Natron::StorageMode storage;
        if (params->getCost() == 0) {
            storage = Natron::RAM;
        } else if (params->getCost() >= 1) {
            storage = Natron::DISK;
        } else {
            storage = Natron::NO_STORAGE;
        }

    
        size_t entryDataSize = params->getElementsCount() * sizeof(data_t);

        ///While the current cache size can't fit the new entry, erase the last recently used entries.
        ///Also if the total free RAM is under the limit of the system free RAM to keep free, erase LRU entries.
        while ( (_memoryCacheSize + entryDataSize >= _maximumInMemorySize)) {
            if ( !tryEvictEntry() ) {
                break;
            }
        }

        ///At this point there could be nothing in the cache but the condition totalFreeRAM <= systemRAMToKeepFree would still be
        ///true. This is likely because the other cache is taking all the memory.
        // We still allocate the entry because otherwise we would never be able to continue the render.
        try {
            entryptr.reset( new EntryType(key,params,this) );
            entryptr->allocateMemory( false, storage, QString( getCachePath() + QDir::separator() ).toStdString() );
        } catch (const std::bad_alloc & e) {
            return EntryTypePtr();
        }

        CachedValue cachedValue;
        cachedValue.entry = entryptr;
        cachedValue.params = params;
        sealEntry(cachedValue);

        return entryptr;
    }

    /** @brief Inserts into the cache an entry that was previously allocated by the newEntry()
     * function. This is called directly by newEntry() if the allocation was successful
     **/
    void sealEntry(const CachedValue & entry) const
    {
        assert( !_lock.tryLock() );   // must be locked
        typename EntryType::hash_type hash = entry.entry->getHashKey();
        /*if the entry doesn't exist on the memory cache,make a new list and insert it*/
        CacheIterator existingEntry = _memoryCache(hash);
        if ( existingEntry == _memoryCache.end() ) {
            _memoryCache.insert(hash,entry);
        } else {
            /*append to the existing list*/
            getValueFromIterator(existingEntry).push_back(entry);
        }
    }

    bool tryEvictEntry() const
    {
        assert( !_lock.tryLock() );
        std::pair<hash_type,CachedValue> evicted = _memoryCache.evict();
        //if the cache couldn't evict that means all entries are used somewhere and we shall not remove them!
        //we'll let the user of these entries purge the extra entries left in the cache later on
        if (!evicted.second.entry) {
            return false;
        }
        /*if it is stored on disk, remove it from memory*/

        if ( evicted.second.entry->isStoredOnDisk() ) {
            assert( evicted.second.entry.unique() );
            
            ///This is EXPENSIVE! it calls msync
            evicted.second.entry->deallocate();
            
            /*insert it back into the disk portion */

            /*before that we need to clear the disk cache if it exceeds the maximum size allowed*/
            while ( ( _diskCacheSize  + evicted.second.entry->size() ) >= (_maximumCacheSize - _maximumInMemorySize) ) {
                std::pair<hash_type,CachedValue> evictedFromDisk = _diskCache.evict();
                //if the cache couldn't evict that means all entries are used somewhere and we shall not remove them!
                //we'll let the user of these entries purge the extra entries left in the cache later on
                if (!evictedFromDisk.second.entry) {
                    break;
                }

                ///Erase the file from the disk if we reach the limit.
                evictedFromDisk.second.entry->removeAnyBackingFile();
            }

            CacheIterator existingDiskCacheEntry = _diskCache(evicted.first);
            /*if the entry doesn't exist on the disk cache,make a new list and insert it*/
            if ( existingDiskCacheEntry == _diskCache.end() ) {
                _diskCache.insert(evicted.first,evicted.second);
            } else {   /*append to the existing list*/
                getValueFromIterator(existingDiskCacheEntry).push_back(evicted.second);
            }
        }

        return true;
    }
};
}


#endif /*NATRON_ENGINE_ABSTRACTCACHE_H_ */
