//  Powiter
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#ifndef POWITER_ENGINE_ABSTRACTCACHE_H_
#define POWITER_ENGINE_ABSTRACTCACHE_H_

#include <vector>
#include <sstream>
#include <fstream>
#include <functional>
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

#include "Gui/SequenceFileDialog.h" // for removeRecursively

#include "Engine/LRUcache.h"
#include "Engine/MemoryFile.h"
#include "Engine/Hash64.h"


namespace Powiter{
    
        /*Calling getHash() the first time will force a computation of the hash key.
         Whenever you want that the hash value gets updated on the next getHash() call
         resetHash().
         */
        template<typename HashType>
        class KeyHelper{
            
        public:
            typedef HashType hash_type;
            
            KeyHelper():_hash(0){}
            
            KeyHelper(hash_type hashValue):_hash(hashValue){}
                        
            virtual ~KeyHelper(){}
            
            /*for now HashType can only be 64 bits...the implementation should
             fill the Hash64 using the append function with the values contained in the
             derived class.*/
            virtual void fillHash(Hash64* hash) const = 0;
            
            hash_type getHash() const {
                if(!_hash)
                    computeHashKey();
                return _hash;
            }
            
            void resetHash() const {_hash = 0;}
            
        private:
            
            void computeHashKey() const {
                Hash64 hash;
                fillHash(&hash);
                hash.computeHash();
                _hash = hash.value();
            }
            
            mutable hash_type _hash;
        };
        
        
        template<typename ValueType> class Cache;
        
        /*KeyType must inherit KeyHelper*/
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
    
        
        template<typename BitDepth>
        class Buffer{
            
        public:
            
            enum StorageMode{RAM=0,DISK};
            
            Buffer():_size(0),_buffer(NULL),_backingFile(NULL),_storageMode(RAM),_allocated(false){}
            
            ~Buffer(){deallocate();}
            
            void allocate(size_t size,int cost,std::string path = std::string()){
                if(_allocated){
                    return;
                }else{
                    _allocated = true;
                }
                if(cost >= 1){
                    _storageMode = DISK;
                    _path = path;
                    try{
                        _backingFile  = new MemoryFile(_path,Powiter::if_exists_keep_if_dont_exists_create);
                    }catch(const std::runtime_error& r){
                        std::cout << r.what() << std::endl;
                        throw std::bad_alloc();
                    }
                    if(!path.empty() && size != 0)//if the backing file has already the good size and we just wanted to re-open the mapping
                        _backingFile->resize(size);
                }else{
                    _storageMode = RAM;
                    _buffer = (BitDepth*)malloc(size);
                }
                _size = size;
            }
            
            void reOpenFileMapping() {
                if(_allocated || _storageMode != DISK || _size == 0){
                    throw std::logic_error("Buffer<T>::allocate(...) must have been called once before calling reOpenFileMapping()!");
                }
                try{
                    _backingFile  = new MemoryFile(_path,Powiter::if_exists_keep_if_dont_exists_create);
                }catch(const std::runtime_error& r){
                    std::cout << r.what() << std::endl;
                    throw std::bad_alloc();
                }
            }
            
            void restoreBufferFromFile(const std::string& path)  {
                try{
                    _backingFile  = new MemoryFile(path,Powiter::if_exists_keep_if_dont_exists_create);
                }catch(const std::runtime_error& r){
                    throw std::bad_alloc();
                }
                _path = path;
                _size = _backingFile->size();
                _storageMode = DISK;
            }
            
            void  deallocate(){
                if(!_allocated)
                    return;
                _allocated = false;
                if(_storageMode == RAM){
                    free(_buffer);
                }else{
                    delete _backingFile;
                }
                _buffer = 0;
                _allocated = false;
            }
            
            void removeAnyBackingFile() const{
                if(_storageMode == DISK){
                    if(QFile::exists(_path.c_str())){
                        QFile::remove(_path.c_str());
                    }
                }
                
            }
            
            size_t size() const {return _size;}
            
            BitDepth* writable() const {
                if(_storageMode == DISK){
                    if(_backingFile){
                        return (BitDepth*)_backingFile->data();
                    }else{
                        return NULL;
                    }
                }else{
                    return _buffer;
                }
            }
            
            const BitDepth* readable() const {
                if(_storageMode == DISK){
                    return (BitDepth*)_backingFile->data();
                }else{
                    return _buffer;
                }
            }
            
            StorageMode getStorageMode() const {return _storageMode;}
            
        private:
            
            std::string _path;
            size_t _size;
            BitDepth* _buffer;
            MemoryFile* _backingFile;
            StorageMode _storageMode;
            bool _allocated;
        };
        
        /* - KeyType must implement : void fillHash(Hash64* hash) which should fill the hash object
         * with all the params of the key using hash->append(U64 value)
         *
         * - KeyType must be boost::serializable
         *
         *
         * - To access the storage :
         *   + Call  _data.writable() to get a writable ptr
         *   + Call  _data.readable() to get a const ptr
         */
        
        template<typename BitDepth,typename KeyType>
        class CacheEntryHelper : public AbstractCacheEntry<KeyType> {
            
            
        public:
            
            CacheEntryHelper(const KeyType& params,const std::string& path):
            _params(params)
            ,_data()
            {
                try{
                    restoreBufferFromFile(path);
                }catch(const std::bad_alloc& e)
                {
                    throw e;
                }
            }
            
            CacheEntryHelper(const KeyType& params,size_t size,int cost,std::string path = std::string()):
            _params(params)
            ,_data(){
                try{
                    allocate(size, cost,path);
                }catch(const std::bad_alloc& e){
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
                name.append(".powc");
                return name;
            }
            
            void allocate(size_t size,int cost,std::string path = std::string()) const {
                std::string fileName;
                if(cost != 0){
                    try{
                        fileName = generateStringFromHash(path);
                    }catch(const std::invalid_argument& e){
                        std::cout << "Path is empty but required for disk caching: " << e.what() << std::endl;
                    }
                }
                try{
                    _data.allocate(size,cost,fileName);
                }catch(const std::bad_alloc& e){
                    throw e;
                }
            }
            
            void reOpenFileMapping() const {
                try{
                    _data.reOpenFileMapping();
                }catch(const std::logic_error& e){
                    throw e;
                }
            }
            
            void restoreBufferFromFile(const std::string& path) const {
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
            
            void deallocate() const {_data.deallocate();}
            
            size_t size() const {return _data.size();}
            
            bool isStoredOnDisk() const {return _data.getStorageMode() == Buffer<BitDepth>::DISK;}
            
            void removeAnyBackingFile() const {_data.removeAnyBackingFile();}
            
        protected:
            
            KeyType _params;
            mutable Buffer<BitDepth> _data;
        };
        
        template<typename ValueType>
        class CachedValue{
            
            boost::shared_ptr<ValueType> _value;
            const Cache<ValueType>* _owner;
        public:
            CachedValue(ValueType* value,const Cache<ValueType>* owner):_value(value),_owner(owner){}
            
            CachedValue(const CachedValue<ValueType>& other):_value(other._value),_owner(other._owner){}
            
            ~CachedValue(){_owner->sealEntry(_value);}
            
            boost::shared_ptr<ValueType> getObject() const {return _value;}
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
            
            friend class CachedValue<ValueType>;

            
        public:
            typedef boost::shared_ptr<const ValueType> value_type;
            typedef typename ValueType::hash_type hash_type;
#ifdef USE_VARIADIC_TEMPLATES
            
#ifdef POWITER_CACHE_USE_BOOST
#ifdef POWITER_CACHE_USE_HASH
            typedef BoostLRUCacheContainer<hash_type, value_type >, boost::bimaps::unordered_set_of> CacheContainer;
#else
            typedef BoostLRUCacheContainer<hash_type, value_type , boost::bimaps::set_of> CacheContainer;
#endif
            typedef typename CacheContainer::container_type::left_iterator CacheIterator;
            typedef typename CacheContainer::container_type::left_const_iterator ConstCacheIterator;
            static value_type getValueFromIterator(CacheIterator it){return it->second;}
            
#else // cache use STL
            
#ifdef POWITER_CACHE_USE_HASH
            typedef StlLRUCache<hash_type,value_type, std::unordered_map> CacheContainer;
#else
            typedef StlLRUCache<hash_type,value_type, std::map> CacheContainer;
#endif
            typedef typename CacheContainer::key_to_value_type::iterator CacheIterator;
            typedef typename CacheContainer::key_to_value_type::const_iterator ConstCacheIterator;
            static value_type  getValueFromIterator(CacheIterator it){return it->second;}
#endif // POWITER_CACHE_USE_BOOST
            
#else // !USE_VARIADIC_TEMPLATES
            
#ifdef POWITER_CACHE_USE_BOOST
#ifdef POWITER_CACHE_USE_HASH
            typedef BoostLRUHashCache<hash_type, value_type > CacheContainer;
#else
            typedef BoostLRUTreeCache<hash_type, value_type > CacheContainer;
#endif
            typedef typename CacheContainer::container_type::left_iterator CacheIterator;
            typedef typename CacheContainer::container_type::left_const_iterator ConstCacheIterator;
            static value_type  getValueFromIterator(CacheIterator it){return it->second;}
            
#else // cache use STL and tree (std map)
            
            typedef StlLRUTreeCache<hash_type, value_type> CacheContainer;
            typedef typename CacheContainer::key_to_value_type::iterator CacheIterator;
            typedef typename CacheContainer::key_to_value_type::const_iterator ConstCacheIterator;
            static value_type  getValueFromIterator(CacheIterator it){return it->second.first;}
#endif // POWITER_CACHE_USE_BOOST
            
#endif // USE_VARIADIC_TEMPLATES
        private:
            
            U64 _maximumInMemorySize; // the maximum size of the in-memory portion of the cache.(in % of the maximum cache size)
            
            U64 _maximumCacheSize; // maximum size allowed for the cache
            
            mutable U64 _memoryCacheSize; // current size of the cache in bytes
            
            mutable U64 _diskCacheSize;
            
            mutable QMutex _lock;
            
            mutable CacheContainer _memoryCache;
            mutable CacheContainer _diskCache;
            
            std::string _cacheName;
            
            unsigned int _version;
            
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
                restore();
            }
            
            ~Cache(){
                clearInMemoryPortion();
                save();
                _memoryCache.clear();
                _diskCache.clear();
                if(_signalEmitter)
                    delete _signalEmitter;
            }
            
            /* Allocates a new entry by the cache. The storage is then handled by
             * the cache solely.
             */
            boost::shared_ptr<CachedValue<ValueType> > newEntry(const typename ValueType::key_type& params,size_t size,int cost) const{
                ValueType* value;
                try{
                    value = new ValueType(params,size,cost,QString(getCachePath()+QDir::separator()).toStdString());
                }catch(const std::bad_alloc& e){
                    return boost::shared_ptr<CachedValue<ValueType> >();
                }
                return boost::shared_ptr<CachedValue<ValueType> >(new CachedValue<ValueType>(value,this));

            }
            
            value_type get(const typename ValueType::key_type& params) const{
                QMutexLocker locker(&_lock);
                value_type ret = _memoryCache(params.getHash());
                if(ret){
                    if(ret->getKey() == params){
                        if(_signalEmitter)
                            _signalEmitter->emitAddedEntry();
                            
                        return ret;
                    }else{
                        return value_type();
                    }
                }else{
                    ret = _diskCache(params.getHash());
                    if(!ret){
                        return value_type();
                    }else{
                        if (ret->getKey() == params) {
                            try{
                                ret->reOpenFileMapping();
                            }catch(const std::exception& e){
                                std::cout << e.what() << std::endl;
                                return value_type();
                            }
                            if(_signalEmitter)
                                _signalEmitter->emitAddedEntry();
                            // maybe we should move back the disk cache entry (ret) to the memory portion since
                            // we reallocated it in RAM?
                            return ret;
                            
                        }else{
                            return value_type();
                        }
                    }
                }
                
            }
            
            void clear(){
                clearInMemoryPortion();
                QMutexLocker locker(&_lock);
                while(_diskCache.size() > 0){
                    std::pair<hash_type,value_type> p = _diskCache.evict();
                    _diskCacheSize -= p.second->size();
                    p.second->removeAnyBackingFile();
                }
                cleanUpDiskAndReset();
            }
            
            void clearInMemoryPortion(){
                QMutexLocker locker(&_lock);
                while (_memoryCache.size() > 0) {
                    std::pair<hash_type,value_type> evicted = _memoryCache.evict();
                    evicted.second->deallocate();
                    _memoryCacheSize -= evicted.second->size();
                    /*insert it back into the disk portion */
                    while (_diskCacheSize+evicted.second->size() >= _maximumCacheSize) {
                        std::pair<hash_type,value_type> evictedFromDisk = _diskCache.evict();
                        _diskCacheSize -= evicted.second->size();
                        evictedFromDisk.second->removeAnyBackingFile();
                    }
                    _diskCacheSize += evicted.second->size();
                    _diskCache.insert(evicted.second->getHashKey(),evicted.second);
                }
                if(_signalEmitter){
                    _signalEmitter->emitSignalClearedInMemoryPortion();
                }
            }
            
            std::string cacheName() const {return _cacheName;}
            
            /*Returns the cache version as a string. This is
             used to know whether a cache is still valid when
             restoring*/
            unsigned int cacheVersion() const {return _version;}
            
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
            
            U64 getMaximumSize() const  {return _maximumCacheSize;}
            
            U64 getMaximumMemorySize() const {return _maximumInMemorySize;}
            
            U64 getMemoryCacheSize() const  {return _memoryCacheSize;}
            
            U64 getDiskCacheSize() const {return _diskCacheSize;}
            
            CacheSignalEmitter* activateSignalEmitter() const {
                if(!_signalEmitter)
                    _signalEmitter = new CacheSignalEmitter;
                return _signalEmitter;
            }
            
        private:
            
            /* Seal the entry into the cache: the entry must have been allocated
             * previously by this cache*/
            void sealEntry(value_type entry) const {
                QMutexLocker locker(&_lock);
                while(_memoryCacheSize+entry->size() >= _maximumInMemorySize){
                    std::pair<hash_type,value_type> evicted = _memoryCache.evict();
                    _memoryCacheSize -= evicted.second->size();
                    if(_signalEmitter)
                        _signalEmitter->emitRemovedEntry();
                    /*if it is stored using mmap, remove it from memory*/
                    if(evicted.second->isStoredOnDisk()){
                        evicted.second->deallocate();
                        /*insert it back into the disk portion */
                        while (_diskCacheSize+evicted.second->size() >= _maximumCacheSize) {
                            std::pair<hash_type,value_type> evictedFromDisk = _diskCache.evict();
                            _diskCacheSize -= evicted.second->size();
                        }
                        _diskCacheSize += evicted.second->size();
                        _diskCache.insert(evicted.second->getHashKey(),evicted.second);
                    }
                }
                if(_signalEmitter)
                    _signalEmitter->emitAddedEntry();
                _memoryCacheSize += entry->size();
                _memoryCache.insert(entry->getHashKey(),entry);
            }

            
            /*Saves cache to disk as a settings file.
             */
            void save(){
                QString newCachePath(getCachePath());
                newCachePath.append(QDir::separator());
                newCachePath.append("restoreFile.powc");
                std::ofstream ofile(newCachePath.toStdString().c_str(),std::ofstream::out);
                boost::archive::binary_oarchive oArchive(ofile);
                std::list<std::pair<hash_type,typename ValueType::key_type> > tableOfContents;
                {
                    QMutexLocker locker(&_lock);
                    for(CacheIterator it = _diskCache.begin(); it!= _diskCache.end() ; ++it) {
                        value_type value  = getValueFromIterator(it);
                        if(value->isStoredOnDisk()){
                            tableOfContents.push_back(std::make_pair(value->getHashKey(),value->getKey()));
                        }
                    }
                }
                oArchive << tableOfContents;
                ofile.close();
            }
            
            /*Restores the cache from disk.*/
            void restore(){
                QMutexLocker locker(&_lock);
                QString newCachePath(getCachePath());
                QString settingsFilePath(newCachePath);
                settingsFilePath.append(QDir::separator());
                settingsFilePath.append("restoreFile.powc");
                if(QFile::exists(settingsFilePath)){
                    QDir directory(newCachePath);
                    QStringList files = directory.entryList();
                    
                    
                    /*Now counting actual data files in the cache*/
                    /*check if there's 256 subfolders, otherwise reset cache.*/
                    int count = 0; // -1 because of the restoreFile
                    int subFolderCount = 0;
                    for(int i =0 ; i< files.size() ;++i) {
                        QString subFolder(newCachePath);
                        subFolder.append(QDir::separator());
                        subFolder.append(files[i]);
                        if(subFolder.right(1) == QString(".") || subFolder.right(2) == QString("..")) continue;
                        QDir d(subFolder);
                        if(d.exists()){
                            ++subFolderCount;
                            QStringList items = d.entryList();
                            for(int j = 0 ; j < items.size();++j) {
                                if(items[j] != QString(".") && items[j] != QString("..")) {
                                    ++count;
                                }
                            }
                        }
                    }
                    if(subFolderCount<256){
                        std::cout << cacheName() << " doesn't contain sub-folders indexed from 00 to FF. Reseting." << std::endl;
                        cleanUpDiskAndReset();
                    }
                    std::ifstream ifile(settingsFilePath.toStdString().c_str(),std::ifstream::in);
                    boost::archive::binary_iarchive iArchive(ifile);
                    std::list<std::pair<hash_type,typename ValueType::key_type> > tableOfContents;
                    iArchive >> tableOfContents;
                    
                    locker.unlock();
                    for (typename std::list<std::pair<hash_type,typename ValueType::key_type> >::const_iterator it =
                         tableOfContents.begin(); it!=tableOfContents.end(); ++it) {
                        if(it->first != it->second.getHash()){
                            std::cout << "WARNING: serialized hash key different than the restored one" << std::endl;
                        }
                        ValueType* value = NULL;
                        try{
                            value = new ValueType(it->second,QString(getCachePath()+QDir::separator()).toStdString());
                        }catch(const std::bad_alloc& e){
                            std::cout << e.what() << std::endl;
                            continue;
                        }
                        sealEntry(value_type(value));
                    }
                    locker.relock();
                    ifile.close();                    
                    QFile restoreFile(settingsFilePath);
                    restoreFile.remove();
                }else{ // restore file doesn't exist
                    /*re-create cache*/
                    
                    QDir(getCachePath()).mkpath(".");
                    cleanUpDiskAndReset();
                }

            }
            
            /*used by the restore func.*/
            void cleanUpDiskAndReset(){
                assert(!_lock.tryLock());
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
                assert(!_lock.tryLock()); // must be locked
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


#endif /* defined(POWITER_ENGINE_ABSTRACTCACHE_H_) */
