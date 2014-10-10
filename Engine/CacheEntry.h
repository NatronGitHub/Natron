#ifndef CACHEENTRY_H
#define CACHEENTRY_H

#include <iostream>
#include <cassert>
#include <stdexcept>
#include <vector>

#include <QtCore/QFile>
#include <QtCore/QDir>

#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>

#include "Engine/Hash64.h"
#include "Engine/MemoryFile.h"
#include "Engine/NonKeyParams.h"

namespace Natron {
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
class Buffer
{
public:


    Buffer()
        : _path()
          , _buffer()
          , _backingFile()
          , _storageMode(RAM)
    {
    }

    ~Buffer()
    {
        deallocate();
    }

    void allocate( U64 count,
                   Natron::StorageMode storage,
                   std::string path = std::string() )
    {
        /*allocate should be called only once.*/
        assert( _path.empty() );
        assert( ( (_buffer.size() == 0) && !_backingFile ) || ( (_buffer.size() != 0) && !_backingFile ) || (!(_buffer.size() != 0) && _backingFile) );
        if ( (_buffer.size() > 0) || _backingFile ) {
            return;
        }


        if (storage == Natron::DISK) {
            _storageMode = DISK;
            _path = path;
            try {
                _backingFile.reset( new MemoryFile(_path,MemoryFile::if_exists_keep_if_dont_exists_create) );
            } catch (const std::runtime_error & r) {
                std::cout << r.what() << std::endl;

                ///if opening the file mapping failed, just call allocate again, but this time on RA%!
                _backingFile.reset();
                _path.clear();
                allocate(count,Natron::RAM,path);

                return;
            }

            if ( !path.empty() && (count != 0) ) {
                //if the backing file has already the good size and we just wanted to re-open the mapping
                _backingFile->resize( count * sizeof(DataType) );
            }
        } else if (storage == Natron::RAM) {
            _storageMode = RAM;
            _buffer.resize(count);
        }
    }

    /**
     * @brief Reallocates the internal buffer so that it countains "count" elements of the DataType.
     * Content defined in the previous portions of the buffer will be kept.
     *
     * Pre-condition: allocate(..) must have been called already.
     **/
    void reallocate(U64 count)
    {
        if (_storageMode == RAM) {
            assert(_buffer.size() > 0); // could be 0 if we allocate 0...
            _buffer.resize(count);
        } else if (_storageMode == DISK) {
            assert(_backingFile);
            _backingFile->resize( count * sizeof(DataType) );
        }
    }

    void reOpenFileMapping() const
    {
        assert(!_backingFile && _storageMode == DISK);
        try{
            _backingFile.reset( new MemoryFile(_path,MemoryFile::if_exists_keep_if_dont_exists_create) );
        } catch (const std::exception & e) {
            _backingFile.reset();
            throw std::bad_alloc();
        }
    }

    void restoreBufferFromFile(const std::string & path)
    {
        try {
            _backingFile.reset( new MemoryFile(path,MemoryFile::if_exists_keep_if_dont_exists_create) );
        } catch (const std::exception & e) {
            _backingFile.reset();
            throw std::bad_alloc();
        }

        _path = path;
        _storageMode = DISK;
    }

    void deallocate()
    {
        if (_storageMode == RAM) {
            _buffer.clear();
        } else {
            if (_backingFile) {
                bool flushOk = _backingFile->flush();
                _backingFile.reset();
                if (!flushOk) {
                    throw std::runtime_error("Failed to flush RAM data to backing file.");
                }
            }
        }
    }

    bool removeAnyBackingFile() const
    {
        if (_storageMode == DISK) {
            if (_backingFile) {
                _backingFile->remove();
                _backingFile.reset();
                return true;
            } else {
                ::remove( _path.c_str() ) ;
                return false;
            }
        }
        return false;
    }

    /**
     * @brief Returns the size of the buffer in bytes.
     **/
    size_t size() const
    {
        if (_storageMode == RAM) {
            return _buffer.size() * sizeof(DataType);
        } else {
            return _backingFile ? _backingFile->size() : 0;
        }
    }

    bool isAllocated() const
    {
        return (_buffer.size() > 0) || ( _backingFile && _backingFile->data() );
    }

    DataType* writable()
    {
        if (_storageMode == DISK) {
            if (_backingFile) {
                return (DataType*)_backingFile->data();
            } else {
                return NULL;
            }
        } else {
            return &_buffer.front();
        }
    }

    const DataType* readable() const
    {
        if (_storageMode == DISK) {
            return (const DataType*)_backingFile->data();
        } else {
            return &_buffer.front();
        }
    }

    Natron::StorageMode getStorageMode() const
    {
        return _storageMode;
    }

private:

    std::string _path;
    std::vector<DataType> _buffer;

    /*mutable so the reOpenFileMapping function can reopen the mmaped file. It doesn't
       change the underlying data*/
    mutable boost::scoped_ptr<MemoryFile> _backingFile;
    Natron::StorageMode _storageMode;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////CACHE ENTRY////////////////////////////////////////////////////
/**
 * @brief Defines the API of the Cache as seen by the cache entries
 **/
class CacheAPI
{
public:
    virtual ~CacheAPI() {}

    /**
     * @brief To be called by a CacheEntry whenever it's size is changed.
     * This way the cache can keep track of the real memory footprint.
     **/
    virtual void notifyEntrySizeChanged(size_t oldSize,size_t newSize) const = 0;

    /**
     * @brief To be called by a CacheEntry on allocation.
     **/
    virtual void notifyEntryAllocated(int time,size_t size,Natron::StorageMode storage) const = 0;

    /**
     * @brief To be called by a CacheEntry on destruction.
     **/
    virtual void notifyEntryDestroyed(int time,size_t size,Natron::StorageMode storage) const = 0;

    /**
     * @brief To be called when a backing file has been closed
     **/
    virtual void backingFileClosed() const = 0;

    /**
     * @brief To be called whenever an entry is deallocated from memory and put back on disk or whenever
     * it is reallocated in the RAM.
     **/
    virtual void notifyEntryStorageChanged(Natron::StorageMode oldStorage,Natron::StorageMode newStorage,
                                           int time,size_t size) const = 0;
};


/** @brief Abstract interface for cache entries.
 * KeyType must inherit KeyHelper
 **/
template<typename KeyType>
class AbstractCacheEntry
    : boost::noncopyable
{
public:

    typedef typename KeyType::hash_type hash_type;
    typedef KeyType key_type;

    AbstractCacheEntry()
    {
    };

    virtual ~AbstractCacheEntry()
    {
    }

    virtual const KeyType & getKey() const = 0;
    virtual hash_type getHashKey() const = 0;
    virtual size_t size() const = 0;
    virtual SequenceTime getTime() const = 0;
};

/** @brief Implements AbstractCacheEntry. This class represents a combinaison of
 * a set of metadatas called 'Key' and a buffer.
 *
 **/

template <typename DataType, typename KeyType, typename ParamsType>
class CacheEntryHelper
    : public AbstractCacheEntry<KeyType>
{
public:
    
    typedef DataType data_t;
    typedef KeyType key_t;
    typedef ParamsType param_t;
    
    /**
     * @brief Ctor
     * the cache entry needs to be set afterwards using setCacheEntry()
     **/
    CacheEntryHelper()
    : _key()
    , _params()
    , _data()
    , _cache()
    , _removeBackingFileBeforeDestruction(false)
    {
    }

    /**
     * @brief Allocates a new cache entry. This function does not allocate the memory required by the entry,
     * the storage will not be available until allocateMemory(...) has been called.
     * @param params The key associated to this cache entry, this is the object containing all the parameters.
     * @param cache The cache managing this entry. Can be NULL when the entry has been allocated outside the cache
     **/
    CacheEntryHelper(const KeyType & key,
                     const boost::shared_ptr<ParamsType> & params,
                     const CacheAPI* cache,
                     Natron::StorageMode storage,
                     const std::string & path)
        : _key(key)
          , _params(params)
          , _data()
          , _cache(cache)
          , _removeBackingFileBeforeDestruction(false)
          , _requestedPath(path)
          , _requestedStorage(storage)
    {
    }

    virtual ~CacheEntryHelper()
    {
        if (_removeBackingFileBeforeDestruction) {
            removeAnyBackingFile();
        }
        deallocate();
    }

    void setCacheEntry(const KeyType & key,
                       const boost::shared_ptr<ParamsType> & params,
                       const CacheAPI* cache,
                       Natron::StorageMode storage,
                       const std::string & path)
    {
        assert(!_params && _cache == NULL);
        _key = key;
        _params = params;
        _cache = cache;
        _requestedPath = path;
        _requestedStorage = storage;
    }

    /**
     * @brief Allocates the memory required by the cache entry. It allocates enough memory to contain at least the
     * memory specified by the key.
     * WARNING: This function throws a std::bad_alloc if the allocation fails.
     **/
    void allocateMemory()
    {
        if (_requestedStorage == Natron::NO_STORAGE) {
            return;
        }

        allocate(_params->getElementsCount(),_requestedStorage,_requestedPath);
        onMemoryAllocated();

        if (_cache) {
            _cache->notifyEntryAllocated( getTime(),size(),_data.getStorageMode() );
        }
    }
    
    /**
     * @brief To be called for disk-cached entries when restoring them from a file.
     * The file-path will be the one passed to the constructor
     * WARNING: This function throws a std::bad_alloc if the allocation fails.
     **/
    void restoreMemory()
    {
        if (_requestedStorage == Natron::NO_STORAGE) {
            return;
        }
        
        assert(!_requestedPath.empty());
        
        restoreBufferFromFile(_requestedPath);
        
        if (_cache) {
            _cache->notifyEntryAllocated( getTime(),size(),_data.getStorageMode() );
        }
        onMemoryAllocated();
    }

    /**
     * @brief Called right away once the buffer is allocated. Used in debug mode to initialize image with a default color.
     **/
    virtual void onMemoryAllocated()
    {
    }


    const KeyType & getKey() const OVERRIDE FINAL
    {
        return _key;
    }

    typename AbstractCacheEntry<KeyType>::hash_type getHashKey() const OVERRIDE FINAL
    {
        return _key.getHash();
    }

    std::string generateStringFromHash(const std::string & path) const
    {
        std::string name(path);

        if ( path.empty() ) {
            QDir subfolder( path.c_str() );
            if ( !subfolder.exists() ) {
                std::cout << "(" << std::hex <<
                    this << ") " <<   "Something is wrong in cache... couldn't find : " << path << std::endl;
                throw std::invalid_argument(path);
            }
        }
        std::ostringstream oss1;
        typename AbstractCacheEntry<KeyType>::hash_type _hashKey = getHashKey();
        oss1 << std::hex << ( _hashKey >> (sizeof(typename AbstractCacheEntry<KeyType>::hash_type) * 8 - 4) );
        oss1 << std::hex << ( (_hashKey << 4) >> (sizeof(typename AbstractCacheEntry<KeyType>::hash_type) * 8 - 4) );
        name.append( oss1.str() );
        std::ostringstream oss2;
        oss2 << std::hex << ( (_hashKey << 8) >> 8 );
        name.append("/");
        name.append( oss2.str() );
        name.append("." NATRON_CACHE_FILE_EXT);

        return name;
    }

    /** @brief This function is called by the get() function of the Cache when the entry is
     * living only in the disk portion of the cache. No locking is required here because the
     * caller is already preventing other threads to call this function.
     **/
    void reOpenFileMapping() const
    {
        _data.reOpenFileMapping();
        if (_cache) {
            _cache->notifyEntryStorageChanged( Natron::DISK, Natron::RAM,getTime(), size() );
        }
    }

    /**
     * @brief Can be called several times without harm
     **/
    void deallocate()
    {
        if (_cache) {
            if ( isStoredOnDisk() ) {
                if ( _data.isAllocated() ) {
                    _cache->notifyEntryStorageChanged( Natron::RAM, Natron::DISK,getTime(), size() );
                }
            } else {
                if ( _data.isAllocated() ) {
                    _cache->notifyEntryDestroyed(getTime(),size(),Natron::RAM);
                }
            }
        }
        _data.deallocate();
    }

    /**
     * @brief Returns the size of the cache entry in bytes. This is made virtual
     * so derived class could add any extra size related to a buffer it may have (@see Natron::Image::size())
     *
     * WARNING: When overloading this, make sure you call then the deallocate() function in your destructor right prior
     * anything else is destroyed, to make sure the good amount of memory to be destroyed is notified to the cache.
     **/
    virtual size_t size() const OVERRIDE
    {
        return dataSize();
    }

    /**
     * @brief Returns the size of the buffer in bytes.
     **/
    size_t dataSize() const
    {
        return _data.size();
    }

    bool isStoredOnDisk() const
    {
        return _data.getStorageMode() == Natron::DISK;
    }

    /**
     * @brief An entry stored on disk is effectively destroyed when its backing file is removed.
     **/
    void removeAnyBackingFile() const
    {
        if (!isStoredOnDisk()) {
            return;
        }
        
        bool isAlloc = _data.isAllocated();
        bool hasRemovedFile = _data.removeAnyBackingFile();
        if (hasRemovedFile) {
            _cache->backingFileClosed();
        }
        if ( isAlloc ) {
            _cache->notifyEntryDestroyed(getTime(), _params->getElementsCount() * sizeof(DataType),Natron::RAM);
        } else {
            ///size() will return 0 at this point, we have to recompute it
            _cache->notifyEntryDestroyed(getTime(), _params->getElementsCount() * sizeof(DataType),Natron::DISK);
        }
    }
    
    /**
     * @brief To be called when an entry is going to be removed from the cache entirely.
     **/
    void scheduleForDestruction() {
        _removeBackingFileBeforeDestruction = true;
    }

    virtual SequenceTime getTime() const OVERRIDE FINAL
    {
        return _key.getTime();
    }

    boost::shared_ptr<ParamsType> getParams() const WARN_UNUSED_RETURN
    {
        return _params;
    }

protected:


    void reallocate(U64 elemCount)
    {
        _params->setElementsCount(elemCount);
        _data.reallocate(elemCount);
        if (_cache) {
            size_t oldSize = size();
            _cache->notifyEntrySizeChanged( oldSize,size(),_data.getStorageMode() );
        }
    }

private:
    /** @brief This function is called in allocateMeory(...) and before the object is exposed
     * to other threads. Hence this function doesn't need locking mechanism at all.
     * We must ensure that this function is called ONLY by allocateMemory(), that's why
     * it is private.
     **/
    void allocate( U64 count,
                   Natron::StorageMode storage,
                   std::string path = std::string() )
    {
        std::string fileName;

        if (storage == Natron::DISK) {
            try {
                fileName = generateStringFromHash(path);
            } catch (const std::invalid_argument & e) {
                std::cout << "Path is empty but required for disk caching: " << e.what() << std::endl;
                return;
            }
        }
        _data.allocate(count, storage, fileName);
    }

    /** @brief This function is called in allocateMeory() and before the object is exposed
     * to other threads. Hence this function doesn't need locking mechanism at all.
     * We must ensure that this function is called ONLY by allocateMemory(), that's why
     * it is private.
     **/
    void restoreBufferFromFile(const std::string & path)
    {
        std::string fileName;

        try {
            fileName = generateStringFromHash(path);
        } catch (const std::invalid_argument & e) {
            std::cout << "Path is empty but required for disk caching: " << e.what() << std::endl;
            return;
        }

        _data.restoreBufferFromFile(fileName);
       
    }

protected:

    KeyType _key;
    boost::shared_ptr<ParamsType> _params;
    Buffer<DataType> _data;
    const CacheAPI* _cache;
    bool _removeBackingFileBeforeDestruction;
    std::string _requestedPath;
    Natron::StorageMode _requestedStorage;
};
}

#endif // CACHEENTRY_H
