//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#ifndef NATRON_ENGINE_ABSTRACTCACHE_H_
#define NATRON_ENGINE_ABSTRACTCACHE_H_

#include <vector>
#include <sstream>
#include <fstream>
#include <functional>
#include <list>

#include "Global/GlobalDefines.h"
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

#include "Engine/ImageSerialization.h"
#include "Engine/ImageParamsSerialization.h"
#include "Engine/FrameEntrySerialization.h"
#include "Engine/FrameParamsSerialization.h"
#include "Engine/CacheEntry.h"
#include "Engine/LRUHashTable.h"
#include "Engine/StandardPaths.h"

namespace Natron {
    
    class CacheSignalEmitter : public QObject {
        Q_OBJECT

    public:
        CacheSignalEmitter(){}

        ~CacheSignalEmitter(){}

        void emitSignalClearedInMemoryPortion() {emit clearedInMemoryPortion();}

        void emitAddedEntry() {emit addedEntry();}

        void emitRemovedLRUEntry() {emit removedLRUEntry();}
    signals:

        void clearedInMemoryPortion();

        void addedEntry();

        void removedLRUEntry();

    };

    /*
         * ValueType must be derived of CacheEntryHelper
         */
    template<typename EntryType>
    class Cache {

        
    public:

      
        
        typedef boost::shared_ptr<const NonKeyParams> NonKeyParamsPtr;
        typedef boost::shared_ptr<EntryType> EntryTypePtr;
        typedef typename EntryType::hash_type hash_type;
        typedef typename EntryType::data_t data_t;
        typedef typename EntryType::key_t key_t;

        struct SerializedEntry {
            
            hash_type hash;
            typename EntryType::key_type key;
            NonKeyParamsPtr params;
            
            
            template<class Archive>
            void serialize(Archive & ar, const unsigned int /*version*/)
            {
                ar & boost::serialization::make_nvp("Hash",hash);
                ar & boost::serialization::make_nvp("Key",key);
                ar & boost::serialization::make_nvp("Params",params);
            }
            
        };
        
        typedef std::list< SerializedEntry > CacheTOC;

    private:

        
        
        

        struct CachedValue {
            EntryTypePtr _entry;
            NonKeyParamsPtr _params;
        };

    public:
        


#ifdef USE_VARIADIC_TEMPLATES

#ifdef NATRON_CACHE_USE_BOOST
#ifdef NATRON_CACHE_USE_HASH
        typedef BoostLRUHashTable<hash_type, CachedValue>, boost::bimaps::unordered_set_of> CacheContainer;
#else
        typedef BoostLRUHashTable<hash_type, CachedValue > , boost::bimaps::set_of> CacheContainer;
#endif
        typedef typename CacheContainer::container_type::left_iterator CacheIterator;
        typedef typename CacheContainer::container_type::left_const_iterator ConstCacheIterator;
        static std::list<CachedValue>&  getValueFromIterator(CacheIterator it){return it->second;}

#else // cache use STL

#ifdef NATRON_CACHE_USE_HASH
        typedef StlLRUHashTable<hash_type,CachedValue >, std::unordered_map> CacheContainer;
#else
        typedef StlLRUHashTable<hash_type,CachedValue >, std::map> CacheContainer;
#endif
        typedef typename CacheContainer::key_to_value_type::iterator CacheIterator;
        typedef typename CacheContainer::key_to_value_type::const_iterator ConstCacheIterator;
        static std::list<CachedValue>&  getValueFromIterator(CacheIterator it){return it->second;}
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
        static std::list<CachedValue>&  getValueFromIterator(CacheIterator it){return it->second;}

#else // cache use STL and tree (std map)

        typedef StlLRUHashTable<hash_type, CachedValue > CacheContainer;
        typedef typename CacheContainer::key_to_value_type::iterator CacheIterator;
        typedef typename CacheContainer::key_to_value_type::const_iterator ConstCacheIterator;
        static std::list<CachedValue>&   getValueFromIterator(CacheIterator it){return it->second.first;}
#endif // NATRON_CACHE_USE_BOOST

#endif // USE_VARIADIC_TEMPLATES
    private:
     


        U64 _maximumInMemorySize; // the maximum size of the in-memory portion of the cache.(in % of the maximum cache size)

        U64 _maximumCacheSize; // maximum size allowed for the cache

        /*mutable because we need to change modify it in the sealEntryInternal function which
             is called by an external object that have a const ref to the cache.
             */
        mutable U64 _memoryCacheSize; // current size of the cache in bytes
        mutable U64 _diskCacheSize;

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

    public:



        Cache(const std::string& cacheName
              ,unsigned int version
              ,U64 maximumCacheSize // total size
              ,double maximumInMemoryPercentage) //how much should live in RAM
            :_maximumInMemorySize(maximumCacheSize*maximumInMemoryPercentage)
            ,_maximumCacheSize(maximumCacheSize)
            ,_memoryCacheSize(0)
            ,_diskCacheSize(0)
            ,_lock()
            ,_memoryCache()
            ,_diskCache()
            ,_cacheName(cacheName)
            ,_version(version)
            ,_signalEmitter(NULL)
        {
        }

        ~Cache() {
            QMutexLocker locker(&_lock);
            _memoryCache.clear();
            _diskCache.clear();
            if(_signalEmitter)
                delete _signalEmitter;
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
        bool get(const typename EntryType::key_type& key,NonKeyParamsPtr* params,EntryTypePtr* returnValue) const {

            ///lock the cache before reading it.
            QMutexLocker locker(&_lock);

            ///find a matching value in the internal memory container
            CacheIterator memoryCached = _memoryCache(key.getHash());

            if (memoryCached != _memoryCache.end()) {
                /*we found something with a matching hash key. There may be several entries linked to
                    this key, we need to find one with matching params*/
                std::list<CachedValue>& ret = getValueFromIterator(memoryCached);
                for (typename std::list<CachedValue>::const_iterator it = ret.begin(); it!=ret.end(); ++it) {
                    if (it->_entry->getKey() == key) {
                        if(_signalEmitter) {
                            ///emit the signal add entry so it can sync any external structure.
                            _signalEmitter->emitAddedEntry();
                        }

                        *returnValue = it->_entry;
                        *params = it->_params;
                        return true;
                    }
                }
                return false;
            } else {

                ///fallback on the disk cache internal container
                CacheIterator diskCached = _diskCache(key.getHash());

                if (diskCached == _diskCache.end()) {
                    /*the entry was neither in memory or disk, just allocate a new one*/
                    return false;
                } else {
                    /*we found something with a matching hash key. There may be several entries linked to
                         this key, we need to find one with matching values(operator ==)*/
                    std::list<CachedValue>& ret = getValueFromIterator(diskCached);

                    for (typename std::list<CachedValue>::iterator it = ret.begin();
                         it!=ret.end(); ++it) {
                        if (it->_entry->getKey() == key) {
                            /*If we found 1 entry in the list that has exactly the same key params,
                         we re-open the mapping to the RAM put the entry
                         back into the memoryCache.*/

                            // remove it from the disk cache
                            ret.erase(it);
                            _diskCacheSize -= it->_entry->size();

                            if(ret.empty()){
                                _diskCache.erase(diskCached);
                            }

                            try {
                                it->_entry->reOpenFileMapping();
                            } catch (const std::exception& e) {
                                qDebug() << "Error while reopening cache file: " << e.what();
                                return false;
                            } catch (...) {
                                qDebug() << "Error while reopening cache file";
                                return false;
                            }

                            //put it back into the RAM
                            _memoryCache.insert(it->_entry->getHashKey(),*it);
                            _memoryCacheSize += it->_entry->size();


                            if(_signalEmitter)
                                _signalEmitter->emitAddedEntry();
                            *returnValue = it->_entry;
                            *params = it->_params;
                            return true;

                        }
                    }
                    /*if we reache here it means no entries linked to the hash key matches the params,then
                         we allocate a new one*/
                    return false;
                }
            }

        }


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
        bool getOrCreate(const typename EntryType::key_type& key,NonKeyParamsPtr params,EntryTypePtr* returnValue) const {
            NonKeyParamsPtr cachedParams;
            if (!get(key,&cachedParams,returnValue)) {
                ///lock the cache before writing it.
                QMutexLocker locker(&_lock);
                *returnValue = newEntry(key,params);
                return false;
            } else {
                if (*cachedParams != *params) {
                    qDebug() << "WARNING: A cache entry was found in the cache for the given key, but the cached parameters that "
                                " go along the entry do not match what's expected. This is a bug.";
                }
                return true;
            }
        }

        void clear() {
            clearInMemoryPortion();
            QMutexLocker locker(&_lock);

            for (CacheIterator it = _diskCache.begin(); it != _diskCache.end(); ++it) {
                std::list<CachedValue>& values = getValueFromIterator(it);
                for (typename std::list<CachedValue>::iterator it2 = values.begin(); it2!=values.end(); ++it2) {
                    _diskCacheSize -= it2->_entry->size();
                    it2->_entry->removeAnyBackingFile();
                }
            }
            _diskCache.clear();

        }


        void clearInMemoryPortion() {
            QMutexLocker locker(&_lock);
            for (CacheIterator it = _memoryCache.begin(); it != _memoryCache.end(); ++it) {
                std::list<CachedValue>& values = getValueFromIterator(it);
                for (typename std::list<CachedValue>::iterator it2 = values.begin(); it2!=values.end(); ++it2) {
                    _memoryCacheSize -= it2->_entry->size();
                    
                    ///move back the entry on disk if it can be store on disk
                    if (it2->_entry->isStoredOnDisk()) {
                        
                        it2->_entry->deallocate();
                        /*insert it back into the disk portion */
                        
                        /*before that we need to clear the disk cache if it exceeds the maximum size allowed*/
                        while (_diskCacheSize + it2->_entry->size() >= _maximumCacheSize) {
                            
                            std::pair<hash_type,CachedValue> evictedFromDisk = _diskCache.evict();
                            //if the cache couldn't evict that means all entries are used somewhere and we shall not remove them!
                            //we'll let the user of these entries purge the extra entries left in the cache later on
                            if (!evictedFromDisk.second._entry) {
                                break;
                            }
                            _diskCacheSize -= evictedFromDisk.second._entry->size();
                        }
                        
                        /*update the disk cache size*/
                        _diskCacheSize += it2->_entry->size();
                        CacheIterator existingDiskCacheEntry = _diskCache(it2->_entry->getHashKey());
                        /*if the entry doesn't exist on the disk cache,make a new list and insert it*/
                        if(existingDiskCacheEntry == _diskCache.end()){
                            _diskCache.insert(it2->_entry->getHashKey(),*it2);
                        }
                        
                    }

                }
                
            }
            _memoryCache.clear();
            if (_signalEmitter) {
                _signalEmitter->emitSignalClearedInMemoryPortion();
            }
        }

        void clearExceedingEntries(){
            QMutexLocker locker(&_lock);
            while (_memoryCacheSize >= _maximumInMemorySize) {
                if (!tryEvictEntry()) {
                    break;
                }
            }
        }

        // const data member: no need to take the lock
        const std::string& cacheName() const { return _cacheName;}

        /*Returns the cache version as a string. This is
             used to know whether a cache is still valid when
             restoring*/
        // const data member: no need to take the lock
        unsigned int cacheVersion() const { return _version;}

        /*Returns the name of the cache with its path preprended*/
        QString getCachePath() const {
            QString cacheFolderName(Natron::StandardPaths::writableLocation(Natron::StandardPaths::CacheLocation) + QDir::separator());
            cacheFolderName.append(QDir::separator());
            QString str(cacheFolderName);
            str.append(cacheName().c_str());
            return str;

        }
        
        std::string getRestoreFilePath() const {
            QString newCachePath(getCachePath());
            newCachePath.append(QDir::separator());
            newCachePath.append("restoreFile." NATRON_CACHE_FILE_EXT);
            return newCachePath.toStdString();
        }

        void setMaximumCacheSize(U64 newSize) { _maximumCacheSize = newSize;}

        void setMaximumInMemorySize(double percentage) { _maximumInMemorySize = _maximumCacheSize * percentage; }

        U64 getMaximumSize() const  { QMutexLocker locker(&_lock); return _maximumCacheSize;}

        U64 getMaximumMemorySize() const { QMutexLocker locker(&_lock); return _maximumInMemorySize;}

        U64 getMemoryCacheSize() const  { QMutexLocker locker(&_lock); return _memoryCacheSize;}

        U64 getDiskCacheSize() const { QMutexLocker locker(&_lock); return _diskCacheSize;}

        CacheSignalEmitter* activateSignalEmitter() const {
            QMutexLocker locker(&_lock);
            if(!_signalEmitter)
                _signalEmitter = new CacheSignalEmitter;
            return _signalEmitter;
        }

        /** @brief This function can be called to remove a specific entry from the cache. For example a frame
         * that has had its render aborted but already belong to the cache.
         **/
        void removeEntry(EntryTypePtr entry) {

            ///early return if entry is NULL
            if (!entry) {
                return;
            }

            QMutexLocker l(&_lock);
            CacheIterator existingEntry = _memoryCache(entry->getHashKey());
            if (existingEntry != _memoryCache.end()) {
                std::list<CachedValue>& ret = getValueFromIterator(existingEntry);
                for (typename std::list<CachedValue>::iterator it = ret.begin(); it!=ret.end(); ++it) {
                    if(it->_entry->getKey() == entry->getKey()){
                        ret.erase(it);
                        _memoryCacheSize -= entry->size();
                        break;
                    }
                }
                if (ret.empty()) {
                    _memoryCache.erase(existingEntry);
                }
            } else {
                existingEntry = _diskCache(entry->getHashKey());
                if (existingEntry != _diskCache.end()) {
                    std::list<CachedValue>& ret = getValueFromIterator(existingEntry);
                    for (typename std::list<CachedValue>::iterator it = ret.begin(); it!=ret.end(); ++it) {
                        if (it->_entry->getKey() == entry->getKey()) {
                            ret.erase(it);
                            _diskCacheSize -= entry->size();
                            break;
                        }
                    }
                    if (ret.empty()) {
                        _diskCache.erase(existingEntry);
                    }
                }
            }
        }

        
        
        /*Saves cache to disk as a settings file.
         */
        void save(CacheTOC* tableOfContents) {
            clearInMemoryPortion();
            QMutexLocker l(&_lock); // must be locked
            
            for (CacheIterator it = _diskCache.begin(); it!= _diskCache.end() ; ++it) {
                std::list<CachedValue>& listOfValues  = getValueFromIterator(it);
                for (typename std::list<CachedValue>::const_iterator it2 = listOfValues.begin() ;it2 != listOfValues.end(); ++it2) {
                    if (it2->_entry->isStoredOnDisk()) {
                        SerializedEntry serialization;
                        serialization.hash = it2->_entry->getHashKey();
                        serialization.params = it2->_params;
                        serialization.key = it2->_entry->getKey();
                        tableOfContents->push_back(serialization);
                    }
                }
                
            }
            
            
        }
        
        /*Restores the cache from disk.*/
        void restore(const CacheTOC& tableOfContents) {
            QMutexLocker locker(&_lock);
            for (typename CacheTOC::const_iterator it =
                 tableOfContents.begin(); it!=tableOfContents.end(); ++it) {
                if (it->hash != it->key.getHash()) {
                    /*
                     * If this warning is printed this means that the value computed by it->key()
                     * is different than the value stored prior to serialiazing this entry. In other words there're
                     * 2 possibilities:
                     * 1) The key has changed since it has been added to the cache.
                     * 2) The hash key computation is unreliable and is depending upon changing or non-deterministic
                     * parameters which is wrong.
                     */
                    qDebug() << "WARNING: serialized hash key different than the restored one";
                }
                EntryType* value = NULL;
                try {
                    value = new EntryType(it->key,*(it->params),true,QString(getCachePath()+QDir::separator()).toStdString());
                } catch (const std::bad_alloc& e) {
                    qDebug() << e.what();
                    continue;
                }
                CachedValue cachedValue;
                cachedValue._entry = EntryTypePtr(value);
                cachedValue._params = it->params;
                sealEntry(cachedValue);
            }
            
        }
    private:

   


        /** @brief Allocates a new entry by the cache. The storage is then handled by
     * the cache solely.
     **/
        EntryTypePtr newEntry(const typename EntryType::key_type& key,const NonKeyParamsPtr& params) const {
            assert(!_lock.tryLock()); // must be locked
            EntryTypePtr entryptr(new EntryType(key,*params, false , QString(getCachePath()+QDir::separator()).toStdString()));
            CachedValue cachedValue;
            cachedValue._entry = entryptr;
            cachedValue._params = params;
            sealEntry(cachedValue);
            return entryptr;

        }

        /** @brief Inserts into the cache an entry that was previously allocated by the newEntry()
     * function. This is called directly by newEntry() if the allocation was successful
     **/
        void sealEntry(const CachedValue& entry) const {
            assert(!_lock.tryLock()); // must be locked
            /*If the cache size exceeds the maximum size allowed, try to make some space*/
            while (_memoryCacheSize+entry._entry->size() >= _maximumInMemorySize) {
                if (!tryEvictEntry()) {
                    break;
                }
            }
            if(_signalEmitter) {
                _signalEmitter->emitAddedEntry();
            }
            typename EntryType::hash_type hash = entry._entry->getHashKey();
            /*if the entry doesn't exist on the memory cache,make a new list and insert it*/
            CacheIterator existingEntry = _memoryCache(hash);
            if (existingEntry == _memoryCache.end()) {
                _memoryCache.insert(hash,entry);
            } else {
                /*append to the existing list*/
                getValueFromIterator(existingEntry).push_back(entry);
            }
            _memoryCacheSize += entry._entry->size();
        }

        bool tryEvictEntry() const {
            assert(!_lock.tryLock());
            std::pair<hash_type,CachedValue> evicted = _memoryCache.evict();
            //if the cache couldn't evict that means all entries are used somewhere and we shall not remove them!
            //we'll let the user of these entries purge the extra entries left in the cache later on
            if (!evicted.second._entry) {
                return false;
            }
            _memoryCacheSize -= evicted.second._entry->size();

            if (_signalEmitter) {
                _signalEmitter->emitRemovedLRUEntry();
            }

            /*if it is stored on disk, remove it from memory*/

            if (evicted.second._entry->isStoredOnDisk()) {

                assert(evicted.second._entry.unique());
                evicted.second._entry->deallocate();
                /*insert it back into the disk portion */

                /*before that we need to clear the disk cache if it exceeds the maximum size allowed*/
                while (_diskCacheSize + evicted.second._entry->size() >= _maximumCacheSize) {

                    std::pair<hash_type,CachedValue> evictedFromDisk = _diskCache.evict();
                    //if the cache couldn't evict that means all entries are used somewhere and we shall not remove them!
                    //we'll let the user of these entries purge the extra entries left in the cache later on
                    if (!evictedFromDisk.second._entry) {
                        break;
                    }
                    _diskCacheSize -= evictedFromDisk.second._entry->size();
                }

                /*update the disk cache size*/
                _diskCacheSize += evicted.second._entry->size();
                CacheIterator existingDiskCacheEntry = _diskCache(evicted.first);
                /*if the entry doesn't exist on the disk cache,make a new list and insert it*/
                if(existingDiskCacheEntry == _diskCache.end()){
                    _diskCache.insert(evicted.first,evicted.second);
                }else{ /*append to the existing list*/
                    getValueFromIterator(existingDiskCacheEntry).push_back(evicted.second);
                }
            }

            return true;
        }

    };



}


#endif /*NATRON_ENGINE_ABSTRACTCACHE_H_ */
