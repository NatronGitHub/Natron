#ifndef CACHEENTRY_H
#define CACHEENTRY_H

#include <iostream>
#include <cassert>
#include <stdexcept>
#include <QtCore/QFile>
#include <QtCore/QDir>

#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>


#include "Engine/Hash64.h"
#include "Engine/MemoryFile.h"
#include "Engine/NonKeyParams.h"

namespace Natron {

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
 * FrameEntry.h or Image.h for an example on how to serialize a KeyHelper instance.
 *
 * Maybe should we move this class as an internal class of CacheEntryHelper to prevent elsewhere
 * usages.
 **/
template<typename HashType>
class KeyHelper {
    
public:
    typedef HashType hash_type;
    
    /**
     * @brief Constructs an empty key. This constructor is used by boost::serialization.
     **/
    KeyHelper(): _hashComputed(false), _hash(){}
    
    
    /**
     * @brief Constructs a key from an already existing hash key.
     **/
    // KeyHelper(hash_type hashValue):  _hashComputed(true), _hash(hashValue){}
    
    /**
     * @brief Constructs a key from an already existing hash key. This is similar than the
     * constructor above but takes in parameter another key.
     **/
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

/////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////BUFFER////////////////////////////////////////////////////



/** @brief Buffer represents  an internal buffer that can be allocated on different devices.
 * For now the class is simple and can only be either on disk using mmap or in RAM using malloc.
 * The cost parameter given to the allocate() function is a hint that the Buffer classes uses
 * to select a device to use. By default -1 means it should not allocate any memory,
 * 0 means RAM and >= 1 means the data will be stored on disk using mmap. We could see this
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
        
        /*allocate should be called only once.*/
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
                
                ///if opening the file mapping failed, just call allocate again, but this time on disk!
                delete _backingFile;
                _backingFile = 0;
                _path.clear();
                allocate(count,0,path);
                return;
            }
            if (!path.empty() && count != 0) {
                //if the backing file has already the good size and we just wanted to re-open the mapping
                _backingFile->resize(count*sizeof(DataType));
                if (!_backingFile->data()) {
                    throw std::bad_alloc();
                }
            }
        } else if(cost == 0) {
            _storageMode = RAM;
            _buffer = new DataType[count];
            if (!_buffer) {
                throw std::bad_alloc();
            }
        }
        _size = count * sizeof(DataType);
    }
    
    void reOpenFileMapping() const {
        assert(!_backingFile && _storageMode == DISK);
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

/////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////CACHE ENTRY////////////////////////////////////////////////////



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

/** @brief Implements AbstractCacheEntry. This class represents a combinaison of
 * a set of metadatas called 'Key' and a buffer.
 *
 **/

template <typename DataType, typename KeyType>
class CacheEntryHelper : public AbstractCacheEntry<KeyType> {
    
    
public:
    typedef DataType data_t;
    typedef KeyType key_t;
    /**
     * @brief Allocates a new cache entry. It allocates enough memory to contain at least the
     * memory specified by the key.
     * @param params The key associated to this cache entry, this is the object containing all the parameters.
     * @param restore If true then the entry will try to restore its buffer from a file pointed to
     * by path.
     * @param path The path of the file where to save/restore the buffer. If empty then it assumes
     * the buffer will be in RAM, hence volatile.
     **/
    CacheEntryHelper(const KeyType& key,const boost::shared_ptr<const NonKeyParams>& params,bool restore,const std::string& path)
    : _key(key)
    , _params(params)
    , _data()
    {
        try {
            if (restore) {
                restoreBufferFromFile(path);
            } else {
                allocate(params->getElementsCount(),params->getCost(),path);
            }
        } catch(const std::bad_alloc& e)
        {
            throw e;
        }
    }
    
    virtual ~CacheEntryHelper() { }
    
    const KeyType& getKey() const {return _key;}
    
    typename AbstractCacheEntry<KeyType>::hash_type getHashKey() const {return _key.getHash();}
    
    std::string generateStringFromHash(const std::string& path) const {
        std::string name(path);
        if (path.empty()) {
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
        _data.reOpenFileMapping();
    }
    
    
    void deallocate() {_data.deallocate();}
    
    virtual size_t size() const { return dataSize(); }

    size_t dataSize() const {return _data.size();}
    
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
        if(cost > 0){
            try {
                fileName = generateStringFromHash(path);
            } catch(const std::invalid_argument& e) {
                std::cout << "Path is empty but required for disk caching: " << e.what() << std::endl;
            }
        }
        _data.allocate(count, cost, fileName);
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
    
    KeyType _key;
    boost::shared_ptr<const NonKeyParams> _params;
    Buffer<DataType> _data;
};

}

#endif // CACHEENTRY_H
