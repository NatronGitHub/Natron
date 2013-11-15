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

#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtCore/QObject>
#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtCore/QTextStream>
#include <QtCore/QBuffer>
#if QT_VERSION < 0x050000
#include <QtGui/QDesktopServices>
#else
#include <QStandardPaths>
#endif

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/list.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>

#include "Global/Macros.h"
#include "Global/GlobalDefines.h"
#include "Global/QtCompat.h" // for removeRecursively

#include "Engine/LRUHashTable.h"
#include "Engine/MemoryFile.h"
#include "Engine/Hash64.h"


namespace Natron{

/** @brief Helper class that represents a Key in the cache. A key is a set
         * of 1 or more parameters that represent a "unique" element in the cache.
         * To create your own key you must derive this class and provide a HashType.
         * The HashType is a value that will serve as a hash key and it will be computed
         * the first time the getHash() function is called.
         * If you need to notify the cache that the key have changed, call resetHash()
         * and the next call to the getHash() function will compute the hash for you.
         *
         * Thread safety: This function is not thread-safe itself but it is used only
         * by the CacheEntryHelper class which itself is used only by the cache which
         * is thread-safe.
         *
         * Important: In order to compile this class must be boost::serializable, see
         * FrameEntry.h or Row.h for an example on how to serialize a KeyHelper instance.
         *
         * Maybe should we move this class as an internal class of CacheEntryHelper to prevent elsewhere
         * usages.
         **/
template<typename HashType>
class KeyHelper {

public:
    typedef HashType hash_type;

    KeyHelper(): _hashComputed(false), _hash() {}

    KeyHelper(hash_type hashValue):  _hashComputed(true), _hash(hashValue){}

    KeyHelper(const KeyHelper& other) : _hashComputed(true), _hash(other.getHash()) {}

    virtual ~KeyHelper(){}

    hash_type getHash() const {
        if(!_hashComputed) {
            computeHashKey();
            _hashComputed = true;
        }
        return _hash;
    }

    void resetHash() const { _hashComputed = false;}

protected:

    /*for now HashType can only be 64 bits...the implementation should
             fill the Hash64 using the append function with the values contained in the
             derived class.*/
    virtual void fillHash(Hash64* hash) const = 0;

private:
    void computeHashKey() const {
        Hash64 hash;
        fillHash(&hash);
        hash.computeHash();
        _hash = hash.value();
    }
    mutable bool _hashComputed;
    mutable hash_type _hash;
};


template<typename ValueType> class Cache;

/** @brief Abstract interface for cache entries.
         * KeyType must inherit KeyHelper
         **/
template<typename KeyType>
class AbstractCacheEntry : boost::noncopyable {

public:

    typedef typename KeyType::hash_type hash_type;
    typedef KeyType key_type;

    AbstractCacheEntry() {};

    virtual ~AbstractCacheEntry() {}

    virtual const KeyType& getKey() const = 0;

    virtual hash_type getHashKey() const = 0;

    virtual size_t size() const = 0;
};

/** @brief Buffer represents  an internal buffer that can be allocated on different devices.
         * For now the class is simple and can only be either on disk using mmap or in RAM using malloc.
         * The cost parameter given to the allocate() function is a hint that the Buffer classes uses
         * to select a device to use. By default 0 means RAM and >= 1 means mmap. We could see this
         * scheme evolve in the future with other storage devices such as OpenGL textures, Cuda buffers,
         * ... etc
         *
         * Thread safety : This class is not thread-safe but is used ONLY by the CacheEntryHelper class
         * which is itself manipulated by the Cache which is thread-safe.
         *
         * Maybe should we move this class as an internal class of CacheEntryHelper to prevent elsewhere
         * usages.
         **/
template<typename DataType>
class Buffer {

public:

    enum StorageMode{RAM=0,DISK};

    Buffer():_path(),_size(0),_buffer(NULL),_backingFile(NULL),_storageMode(RAM){}

    ~Buffer(){deallocate();}

    void allocate(size_t count, int cost, std::string path = std::string()) {
        assert(_path.empty());
        assert((!_buffer && !_backingFile) || (_buffer && !_backingFile) || (!_buffer && _backingFile));
        if (_buffer || _backingFile) {
            return;
        }

        if (cost >= 1) {
            _storageMode = DISK;
            _path = path;
            try {
                _backingFile  = new MemoryFile(_path,Natron::if_exists_keep_if_dont_exists_create);
            } catch(const std::runtime_error& r) {
                std::cout << r.what() << std::endl;
                throw std::bad_alloc();
            }
            if (!path.empty() && count != 0) {
                //if the backing file has already the good size and we just wanted to re-open the mapping
                _backingFile->resize(count*sizeof(DataType));
            }
        } else {
            _storageMode = RAM;
            _buffer = new DataType[count];
        }
        _size = count;
    }

    void reOpenFileMapping() const {
        assert(!_backingFile && _storageMode == DISK && _size > 0);
        try{
            _backingFile  = new MemoryFile(_path,Natron::if_exists_keep_if_dont_exists_create);
        }catch(const std::runtime_error& r){
            delete _backingFile;
            _backingFile = NULL;
            std::cout << r.what() << std::endl;
            throw std::bad_alloc();
        }
    }

    void restoreBufferFromFile(const std::string& path)  {
        try{
            _backingFile  = new MemoryFile(path,Natron::if_exists_keep_if_dont_exists_create);
        }catch(const std::runtime_error& /*r*/){
            delete _backingFile;
            _backingFile = NULL;
            throw std::bad_alloc();
        }
        _path = path;
        _size = _backingFile->size();
        _storageMode = DISK;
    }

    void deallocate() {

        if (_storageMode == RAM) {
            delete [] _buffer;
            _buffer = NULL;
        } else {
            delete _backingFile;
            _backingFile = NULL;
        }
    }

    void removeAnyBackingFile() const{
        if(_storageMode == DISK){
            if(QFile::exists(_path.c_str())){
                QFile::remove(_path.c_str());
            }
        }

    }

    size_t size() const {return _size;}

    DataType* writable() const {
        if (_storageMode == DISK) {
            if(_backingFile) {
                return (DataType*)_backingFile->data();
            } else {
                return NULL;
            }
        } else {
            return _buffer;
        }
    }

    const DataType* readable() const {
        if (_storageMode == DISK) {
            return (DataType*)_backingFile->data();
        } else {
            return _buffer;
        }
    }

    StorageMode getStorageMode() const {return _storageMode;}

private:

    std::string _path;
    size_t _size;
    DataType* _buffer;

    /*mutable so the reOpenFileMapping function can reopen the mmaped file. It doesn't
             change the underlying data*/
    mutable MemoryFile* _backingFile;

    StorageMode _storageMode;
};

/** @brief Implements AbstractCacheEntry. This class represents a combinaison of
         * a set of metadatas called 'Key' and a buffer.
         *
         **/
template <typename DataType, typename KeyType>
class CacheEntryHelper : public AbstractCacheEntry<KeyType> {


public:
    typedef DataType data_t;
    typedef KeyType key_t;

    CacheEntryHelper(const KeyType& params,const std::string& path)
        : _params(params)
        , _data()
    {
        try{
            restoreBufferFromFile(path);
        }catch(const std::bad_alloc& e)
        {
            throw e;
        }
    }

    CacheEntryHelper(const KeyType& params, size_t count, int cost, std::string path = std::string())
        : _params(params)
        , _data()
    {
        try {
            allocate(count, cost, path);
        } catch(const std::bad_alloc& e) {
            throw e;
        }
    }
    virtual ~CacheEntryHelper() {}

    const KeyType& getKey() const {return _params;}

    typename AbstractCacheEntry<KeyType>::hash_type getHashKey() const {return _params.getHash();}

    std::string generateStringFromHash(const std::string& path) const {
        std::string name(path);
        if(path.empty()){
            QDir subfolder(path.c_str());
            if(!subfolder.exists()){
                std::cout << "("<< std::hex <<
                             this << ") "<<   "Something is wrong in cache... couldn't find : " << path << std::endl;
                throw std::invalid_argument(path);
            }
        }
        std::ostringstream oss1;
        typename AbstractCacheEntry<KeyType>::hash_type _hashKey = getHashKey();
        oss1 << std::hex << (_hashKey >> (sizeof(typename AbstractCacheEntry<KeyType>::hash_type)*8 - 4));
        oss1 << std::hex << ((_hashKey << 4) >> (sizeof(typename AbstractCacheEntry<KeyType>::hash_type)*8 - 4));
        name.append(oss1.str());
        std::ostringstream oss2;
        oss2 << std::hex << ((_hashKey << 8) >> 8);
        name.append("/");
        name.append(oss2.str());
        name.append("." NATRON_CACHE_FILE_EXT);
        return name;
    }

    /** @brief This function is called by the get() function of the Cache when the entry is
             * living only in the disk portion of the cache. No locking is required here because the
             * caller is already preventing other threads to call this function.
             **/
    void reOpenFileMapping() const {
        try{
            _data.reOpenFileMapping();
        }catch(const std::exception& e){
            throw e;
        }
    }


    void deallocate() {_data.deallocate();}

    size_t size() const {return _data.size();}

    bool isStoredOnDisk() const {return _data.getStorageMode() == Buffer<DataType>::DISK;}

    void removeAnyBackingFile() const {_data.removeAnyBackingFile();}

private:
    /** @brief This function is called upon the constructor and before the object is exposed
             * to other threads. Hence this function doesn't need locking mechanism at all.
             * We must ensure that this function is called ONLY by the constructor, that's why
             * it is private.
             **/
    void allocate(size_t count, int cost, std::string path = std::string()) {
        std::string fileName;
        if(cost != 0){
            try {
                fileName = generateStringFromHash(path);
            } catch(const std::invalid_argument& e) {
                std::cout << "Path is empty but required for disk caching: " << e.what() << std::endl;
            }
        }
        try {
            _data.allocate(count, cost, fileName);
        } catch(const std::bad_alloc& e) {
            throw e;
        }
    }

    /** @brief This function is called upon the constructor and before the object is exposed
             * to other threads. Hence this function doesn't need locking mechanism at all.
             * We must ensure that this function is called ONLY by the constructor, that's why
             * it is private.
             **/
    void restoreBufferFromFile(const std::string& path) {
        std::string fileName;
        try{
            fileName = generateStringFromHash(path);
        }catch(const std::invalid_argument& e){
            std::cout << "Path is empty but required for disk caching: " << e.what() << std::endl;
        }
        try{
            _data.restoreBufferFromFile(fileName);
        }catch(const std::bad_alloc& e){
            throw e;
        }
    }


protected:

    KeyType _params;
    Buffer<DataType> _data;
};

class CacheSignalEmitter : public QObject{
    Q_OBJECT

public:
    CacheSignalEmitter(){}

    ~CacheSignalEmitter(){}

    void emitSignalClearedInMemoryPortion() {emit clearedInMemoryPortion();}

    void emitAddedEntry() {emit addedEntry();}

    void emitRemovedEntry() {emit removedEntry();}
signals:

    void clearedInMemoryPortion();

    void addedEntry();

    void removedEntry();

};

/*
         * ValueType must be derived of CacheEntryHelper
         */
template<typename ValueType>
class Cache {

public:
    typedef boost::shared_ptr<const ValueType> value_type;
    typedef typename ValueType::hash_type hash_type;
    typedef typename ValueType::data_t data_t;
    typedef typename ValueType::key_t key_t;

#ifdef USE_VARIADIC_TEMPLATES

#ifdef NATRON_CACHE_USE_BOOST
#ifdef NATRON_CACHE_USE_HASH
    typedef BoostLRUHashTable<hash_type, value_type>, boost::bimaps::unordered_set_of> CacheContainer;
#else
    typedef BoostLRUHashTable<hash_type, value_type > , boost::bimaps::set_of> CacheContainer;
#endif
    typedef typename CacheContainer::container_type::left_iterator CacheIterator;
    typedef typename CacheContainer::container_type::left_const_iterator ConstCacheIterator;
    static std::list<value_type>&  getValueFromIterator(CacheIterator it){return it->second;}

#else // cache use STL

#ifdef NATRON_CACHE_USE_HASH
    typedef StlLRUHashTable<hash_type,value_type >, std::unordered_map> CacheContainer;
#else
    typedef StlLRUHashTable<hash_type,value_type >, std::map> CacheContainer;
#endif
    typedef typename CacheContainer::key_to_value_type::iterator CacheIterator;
    typedef typename CacheContainer::key_to_value_type::const_iterator ConstCacheIterator;
    static std::list<value_type>&  getValueFromIterator(CacheIterator it){return it->second;}
#endif // NATRON_CACHE_USE_BOOST

#else // !USE_VARIADIC_TEMPLATES

#ifdef NATRON_CACHE_USE_BOOST
#ifdef NATRON_CACHE_USE_HASH
    typedef BoostLRUHashTable<hash_type,value_type> CacheContainer;
#else
    typedef BoostLRUHashTable<hash_type, value_type > CacheContainer;
#endif
    typedef typename CacheContainer::container_type::left_iterator CacheIterator;
    typedef typename CacheContainer::container_type::left_const_iterator ConstCacheIterator;
    static std::list<value_type>&  getValueFromIterator(CacheIterator it){return it->second;}

#else // cache use STL and tree (std map)

    typedef StlLRUHashTable<hash_type, value_type > CacheContainer;
    typedef typename CacheContainer::key_to_value_type::iterator CacheIterator;
    typedef typename CacheContainer::key_to_value_type::const_iterator ConstCacheIterator;
    static std::list<value_type>&   getValueFromIterator(CacheIterator it){return it->second.first;}
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
        QMutexLocker locker(&_lock);
        restore();
    }

    ~Cache() {
        clearInMemoryPortion();
        QMutexLocker locker(&_lock);
        save();
        _memoryCache.clear();
        _diskCache.clear();
        if(_signalEmitter)
            delete _signalEmitter;
    }

    /* Allocates a new entry by the cache. The storage is then handled by
             * the cache solely.
             */
    boost::shared_ptr<ValueType> newEntry(const typename ValueType::key_type& params, size_t count, int cost) const {
        QMutexLocker locker(&_lock);
        ValueType* value = 0;
        try{
            value = new ValueType(params, count, cost, QString(getCachePath()+QDir::separator()).toStdString());
        } catch(const std::bad_alloc& /*e*/) {
            delete value;
            return boost::shared_ptr<ValueType>();
        }
        boost::shared_ptr<ValueType> cachedValue(value);
        sealEntry(value_type(cachedValue));
        return cachedValue;

    }

    value_type get(const typename ValueType::key_type& params) const {
        QMutexLocker locker(&_lock);
        CacheIterator memoryCached = _memoryCache(params.getHash());
        if(memoryCached != _memoryCache.end()){
            /*we found something with a matching hash key. There may be several entries linked to
                     this key, we need to find one with matching params*/
            std::list<value_type>& ret = getValueFromIterator(memoryCached);
            for (typename std::list<value_type>::const_iterator it = ret.begin(); it!=ret.end(); ++it) {
                if((*it)->getKey() == params){
                    if(_signalEmitter)
                        _signalEmitter->emitAddedEntry();

                    return *it;
                }
            }
            /*if we reach here it means no entries linked to the hash key matches the params,then
                     we return NULL.*/
            return value_type();
        }else{
            CacheIterator diskCached = _diskCache(params.getHash());
            if(diskCached == _diskCache.end()){
                return value_type();
            }else{
                /*we found something with a matching hash key. There may be several entries linked to
                         this key, we need to find one with matching params*/
                std::list<value_type>& ret = getValueFromIterator(diskCached);
                for (typename std::list<value_type>::iterator it = ret.begin();
                     it!=ret.end(); ++it) {
                    if ((*it)->getKey() == params) {
                        /*If we found 1 entry in the list that has exactly the same key params,
                         we re-open the mapping to the RAM put the entry
                         back into the memoryCache.*/
                        value_type entry = (*it);
                        
                        // remove it from the disk cache
                        ret.erase(it);
                        _diskCacheSize -= entry->size();
                        
                        if(ret.empty()){
                            _diskCache.erase(diskCached);
                        }
                        
                        try{
                            entry->reOpenFileMapping();
                        }catch(const std::exception& e){
                            std::cout << e.what() << std::endl;
                            return value_type();
                        }
                        
                        //put it back into the RAM
                        _memoryCache.insert(entry->getHashKey(),entry);
                        _memoryCacheSize += entry->size();
                        
                       
                        if(_signalEmitter)
                            _signalEmitter->emitAddedEntry();
                        return entry;

                    }
                }
                /*if we reache here it means no entries linked to the hash key matches the params,then
                         we return NULL.*/
                return value_type();
            }
        }

    }

    void clear(){
        clearInMemoryPortion();
        QMutexLocker locker(&_lock);
        while(_diskCache.size() > 0){
            std::pair<hash_type,value_type> evictedFromDisk = _diskCache.evict();
            //if the cache couldn't evict that means all entries are used somewhere and we shall not remove them!
            //we'll let the user of these entries purge the extra entries left in the cache later on
            if(!evictedFromDisk.second){
                break;
            }
            /*remove the sum of the size of all entries within the same hash key*/
            _diskCacheSize -= evictedFromDisk.second->size();
            evictedFromDisk.second->removeAnyBackingFile();

        }
    }


    void clearInMemoryPortion(){
        QMutexLocker locker(&_lock);
        while (_memoryCache.size() > 0) {
            if(!tryEvictEntry()){
                break;
            }
        }
        if(_signalEmitter){
            _signalEmitter->emitSignalClearedInMemoryPortion();
        }
    }

    void clearExceedingEntries(){
        QMutexLocker locker(&_lock);
        while(_memoryCacheSize >= _maximumInMemorySize){
            if(!tryEvictEntry()){
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
#if QT_VERSION < 0x050000
        QString cacheFolderName(QDesktopServices::storageLocation(QDesktopServices::CacheLocation));
#else
        QString cacheFolderName(QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QDir::separator());
#endif
        cacheFolderName.append(QDir::separator());
        QString str(cacheFolderName);
        str.append(cacheName().c_str());
        return str;

    }

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

private:
    typedef std::list<std::pair<hash_type,typename ValueType::key_type> > CacheTOC;


    /** @brief Inserts into the cache an entry that was previously allocated by the newEntry()
             * function. This is called directly by newEntry() if the allocation was successful
             **/
    void sealEntry(value_type entry) const {
        assert(!_lock.tryLock()); // must be locked
        /*If the cache size exceeds the maximum size allowed, try to make some space*/
        while(_memoryCacheSize+entry->size() >= _maximumInMemorySize){
            if(!tryEvictEntry()){
                break;
            }
        }
        if(_signalEmitter)
            _signalEmitter->emitAddedEntry();
        typename ValueType::hash_type hash = entry->getHashKey();
        /*if the entry doesn't exist on the memory cache,make a new list and insert it*/
        CacheIterator existingEntry = _memoryCache(hash);
        if(existingEntry == _memoryCache.end()){
            _memoryCache.insert(hash,entry);
        }else{
            /*append to the existing list*/
            getValueFromIterator(existingEntry).push_back(entry);
        }
        _memoryCacheSize += entry->size();
    }

    bool tryEvictEntry() const {
        assert(!_lock.tryLock());
        std::pair<hash_type,value_type> evicted = _memoryCache.evict();
        //if the cache couldn't evict that means all entries are used somewhere and we shall not remove them!
        //we'll let the user of these entries purge the extra entries left in the cache later on
        if(!evicted.second){
            return false;
        }
        _memoryCacheSize -= evicted.second->size();

        if(_signalEmitter)
            _signalEmitter->emitRemovedEntry();

        /*if it is stored using mmap, remove it from memory*/

        if(evicted.second->isStoredOnDisk()){
            boost::const_pointer_cast<ValueType>(evicted.second)->deallocate();
            /*insert it back into the disk portion */

            /*before that we need to clear the disk cache if it exceeds the maximum size allowed*/
            while (_diskCacheSize+evicted.second->size() >= _maximumCacheSize) {

                std::pair<hash_type,value_type> evictedFromDisk = _diskCache.evict();
                //if the cache couldn't evict that means all entries are used somewhere and we shall not remove them!
                //we'll let the user of these entries purge the extra entries left in the cache later on
                if(!evictedFromDisk.second){
                    break;
                }
                _diskCacheSize -= evictedFromDisk.second->size();
            }

            /*update the disk cache size*/
            _diskCacheSize += evicted.second->size();
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


    /*Saves cache to disk as a settings file.
             */
    void save(){
        assert(!_lock.tryLock()); // must be locked
        QString newCachePath(getCachePath());
        newCachePath.append(QDir::separator());
        newCachePath.append("restoreFile." NATRON_CACHE_FILE_EXT);
        std::ofstream ofile(newCachePath.toStdString().c_str(),std::ofstream::out);
        boost::archive::binary_oarchive oArchive(ofile);
        CacheTOC tableOfContents;
        {
            for(CacheIterator it = _diskCache.begin(); it!= _diskCache.end() ; ++it) {
                std::list<value_type>& listOfValues  = getValueFromIterator(it);
                for(typename std::list<value_type>::const_iterator it2 = listOfValues.begin() ;it2 != listOfValues.end(); ++it2){
                    if((*it2)->isStoredOnDisk()){
                        tableOfContents.push_back(std::make_pair((*it2)->getHashKey(),(*it2)->getKey()));
                    }
                }

            }
        }
        oArchive << tableOfContents;
        ofile.close();
    }

    /*Restores the cache from disk.*/
    void restore() {
        try { // if anything throws an exception, just recreate the cache
            QString newCachePath(getCachePath());
            QString settingsFilePath(newCachePath+QDir::separator()+"restoreFile." NATRON_CACHE_FILE_EXT);
            if(!QFile::exists(settingsFilePath)) {
                throw std::runtime_error("Cache does not exist");
            }
            QDir directory(newCachePath);
            QStringList files = directory.entryList();


            /*Now counting actual data files in the cache*/
            /*check if there's 256 subfolders, otherwise reset cache.*/
            int count = 0; // -1 because of the restoreFile
            int subFolderCount = 0;
            for (int i =0; i< files.size(); ++i) {
                QString subFolder(newCachePath);
                subFolder.append(QDir::separator());
                subFolder.append(files[i]);
                if(subFolder.right(1) == QString(".") || subFolder.right(2) == QString("..")) continue;
                QDir d(subFolder);
                if (d.exists()) {
                    ++subFolderCount;
                    QStringList items = d.entryList();
                    for (int j = 0; j < items.size(); ++j) {
                        if(items[j] != QString(".") && items[j] != QString("..")) {
                            ++count;
                        }
                    }
                }
            }
            if (subFolderCount<256) {
                std::cout << cacheName() << " doesn't contain sub-folders indexed from 00 to FF. Reseting." << std::endl;
                cleanUpDiskAndReset();
            }
            std::ifstream ifile(settingsFilePath.toStdString().c_str(),std::ifstream::in);
            boost::archive::binary_iarchive iArchive(ifile);
            CacheTOC tableOfContents;
            iArchive >> tableOfContents;


            for (typename CacheTOC::const_iterator it =
                 tableOfContents.begin(); it!=tableOfContents.end(); ++it) {
                if (it->first != it->second.getHash()) {
                    std::cout << "WARNING: serialized hash key different than the restored one" << std::endl;
                }
                ValueType* value = NULL;
                try {
                    value = new ValueType(it->second,QString(getCachePath()+QDir::separator()).toStdString());
                } catch (const std::bad_alloc& e) {
                    std::cout << e.what() << std::endl;
                    continue;
                }
                sealEntry(value_type(value));
            }

            ifile.close();
            QFile restoreFile(settingsFilePath);
            restoreFile.remove();
        } catch (...) {
            /*re-create cache*/

            QDir(getCachePath()).mkpath(".");
            cleanUpDiskAndReset();
        }

    }

    /*used by the restore func.*/
    void cleanUpDiskAndReset(){
        QString dirName(getCachePath());
#   if QT_VERSION < 0x050000
        removeRecursively(dirName);
#   else
        QDir dir(dirName);
        if(dir.exists()){
            dir.removeRecursively();
        }
#endif
        initializeSubDirectories();
    }

    /*Creates empty sub-directories for the cache, counting them
             from Ox00 to OxFF.
             Must be called explicitely after the constructor
             */
    void initializeSubDirectories(){
        QDir cacheFolder(getCachePath());
        cacheFolder.mkpath(".");

        QStringList etr = cacheFolder.entryList();
        // if not 256 subdirs, we re-create the cache
        if (etr.size() < 256) {
            foreach(QString e, etr){
                cacheFolder.rmdir(e);
            }
        }
        for(U32 i = 0x00 ; i <= 0xF; ++i) {
            for(U32 j = 0x00 ; j <= 0xF ; ++j) {
                std::ostringstream oss;
                oss << std::hex <<  i;
                oss << std::hex << j ;
                std::string str = oss.str();
                cacheFolder.mkdir(str.c_str());
            }
        }

    }

};



}


#endif /*NATRON_ENGINE_ABSTRACTCACHE_H_ */
