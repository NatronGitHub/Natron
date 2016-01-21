/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
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

#ifndef CACHEENTRY_H
#define CACHEENTRY_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <iostream>
#include <cassert>
#include <cstdio> // for std::remove
#include <stdexcept>
#include <vector>
#include <fstream>

#include <QtCore/QFile>
#include <QtCore/QMutex>
#include <QtCore/QReadWriteLock>
#include <QtCore/QDir>
#include <QtCore/QDebug>
#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#endif
#include "Engine/Hash64.h"
#include "Engine/CacheEntryHolder.h"
#include "Engine/MemoryFile.h"
#include "Engine/NonKeyParams.h"
#include <SequenceParsing.h> // for removePath
#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER;

/////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////BUFFER////////////////////////////////////////////////////

template <typename T>
class RamBuffer
{
    T* data;
    U64 count;
    
public:
    
    RamBuffer()
    : data(0)
    , count(0)
    {
        
    }
    
    T* getData()
    {
        return data;
    }
    
    const T* getData() const
    {
        return data;
    }
    
    void swap(RamBuffer& other)
    {
        std::swap(data, other.data);
        std::swap(count, other.count);
    }
    
    U64 size() const
    {
        return count;
    }
    
    void resize(U64 size)
    {
        if (size == 0) {
            return;
        }
        count = size;
        if (data) {
            free(data);
            data = 0;
        }
        if (count == 0) {
            return;
        }
        data = (T*)malloc(size * sizeof(T));
        if (!data) {
            throw std::bad_alloc();
        }
    }
    
    void clear()
    {
        count = 0;
        if (data) {
            free(data);
            data = 0;
        }
    }
    
    ~RamBuffer()
    {
        if (data) {
            free(data);
            data = 0;
        }
    }
};

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
    , _storageMode(eStorageModeRAM)
    {
    }
    
    ~Buffer()
    {
        deallocate();
    }

    void allocate( U64 count,
                   StorageModeEnum storage,
                   std::string path = std::string() )
    {
        /*allocate should be called only once.*/
        assert( _path.empty() );
        assert( ( (_buffer.size() == 0) && !_backingFile ) || ( (_buffer.size() != 0) && !_backingFile ) || (!(_buffer.size() != 0) && _backingFile) );
        if ( (_buffer.size() > 0) || _backingFile ) {
            return;
        }


        if (storage == eStorageModeDisk) {
            _storageMode = eStorageModeDisk;
            _path = path;
            try {
                _backingFile.reset( new MemoryFile(_path,MemoryFile::eFileOpenModeEnumIfExistsKeepElseCreate) );
            } catch (const std::runtime_error & r) {
                std::cout << r.what() << std::endl;

                ///if opening the file mapping failed, just call allocate again, but this time on RA%!
                _backingFile.reset();
                _path.clear();
                allocate(count,eStorageModeRAM, path);

                return;
            }

            if ( !path.empty() && (count != 0) ) {
                //if the backing file has already the good size and we just wanted to re-open the mapping
                _backingFile->resize(count);
            }
        } else if (storage == eStorageModeRAM) {
            _storageMode = eStorageModeRAM;
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
        if (_storageMode == eStorageModeRAM) {
            assert(_buffer.size() > 0); // could be 0 if we allocate 0...
            _buffer.resize(count);
        } else if (_storageMode == eStorageModeDisk) {
            assert(_backingFile);
            _backingFile->resize( count * sizeof(DataType) );
        }
    }
    
    /**
     * @brief Beware this is not really a "swap" as other do not get the infos from this Buffer.
     **/
    void swap(Buffer& other)
    {
        if (_storageMode == eStorageModeRAM) {
            if (other._storageMode == eStorageModeRAM) {
                _buffer.swap(other._buffer);
            } else {
                _buffer.resize(other._backingFile->size() / sizeof(DataType));
                const char* src = other._backingFile->data();
                char* dst = (char*)_buffer.getData();
                memcpy(dst,src,other._backingFile->size());
            }
        } else if (_storageMode == eStorageModeDisk) {
            if (other._storageMode == eStorageModeDisk) {
                assert(_backingFile);
                _backingFile.swap(other._backingFile);
                _path = other._path;
            } else {
                _backingFile->resize(other._buffer.size() * sizeof(DataType));
				assert(_backingFile->data());
                const char* src = (const char*)other._buffer.getData();
                char* dst = (char*)_backingFile->data();
                memcpy(dst,src,other._buffer.size() * sizeof(DataType));
            }
        }
        
    }
    
    const std::string& getFilePath() const {
        return _path;
    }

    void reOpenFileMapping() const
    {
        assert(!_backingFile && _storageMode == eStorageModeDisk);
        try{
            _backingFile.reset( new MemoryFile(_path,MemoryFile::eFileOpenModeEnumIfExistsKeepElseCreate) );
        } catch (const std::exception & e) {
            _backingFile.reset();
            throw std::bad_alloc();
        }
    }

    void restoreBufferFromFile(const std::string & path)
    {
        _path = path;
        _storageMode = eStorageModeDisk;
    }

    void deallocate()
    {
        if (_storageMode == eStorageModeRAM) {
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
        if (_storageMode == eStorageModeDisk) {
            if (_backingFile) {
                _backingFile->remove();
                _backingFile.reset();
                return true;
            } else {
                int ret_code = std::remove( _path.c_str() );
                Q_UNUSED(ret_code);
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
        if (_storageMode == eStorageModeRAM) {
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
        if (_storageMode == eStorageModeDisk) {
            if (_backingFile) {
                return (DataType*)_backingFile->data();
            } else {
                return NULL;
            }
        } else {
            return _buffer.getData();
        }
    }

    const DataType* readable() const
    {
        if (_storageMode == eStorageModeDisk) {
            return _backingFile ? (const DataType*)_backingFile->data() : 0;
        } else {
            return _buffer.getData();
        }
    }

    StorageModeEnum getStorageMode() const
    {
        return _storageMode;
    }

private:

    std::string _path;
    RamBuffer<DataType> _buffer;

    /*mutable so the reOpenFileMapping function can reopen the mmaped file. It doesn't
       change the underlying data*/
    mutable boost::scoped_ptr<MemoryFile> _backingFile;
    StorageModeEnum _storageMode;
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
    virtual void notifyEntryAllocated(double time, size_t size, StorageModeEnum storage) const = 0;

    /**
     * @brief To be called by a CacheEntry on destruction.
     **/
    virtual void notifyEntryDestroyed(double time, size_t size, StorageModeEnum storage) const = 0;
    
    /**
     * @brief Called by the Cache deleter thread to wake up sleeping threads that were attempting to create a new iamge
     **/
    virtual void notifyMemoryDeallocated() const = 0;

    /**
     * @brief To be called when a backing file has been closed
     **/
    virtual void backingFileClosed() const = 0;

    /**
     * @brief To be called whenever an entry is deallocated from memory and put back on disk or whenever
     * it is reallocated in the RAM.
     **/
    virtual void notifyEntryStorageChanged(StorageModeEnum oldStorage,StorageModeEnum newStorage,
                                           double time,size_t size) const = 0;
    
    /**
     * @brief Remove from the cache all entries that matches the holderID and have a different nodeHash than the given one.
     * @param removeAll If true, remove even entries that match the nodeHash
     **/
    virtual void removeAllEntriesWithDifferentNodeHashForHolderPrivate(const std::string& holderID, U64 nodeHash, bool removeAll) = 0;
    
    
#ifdef DEBUG
    static bool checkFileNameMatchesHash(const std::string &originalFileName,U64 hash)
    {
        
        std::string filename = originalFileName;
        std::string path = SequenceParsing::removePath(filename);
        //remove extension from filename
        {
            size_t lastdot = filename.find_last_of('.');
            if (lastdot != std::string::npos) {
                filename.erase(lastdot, std::string::npos);
            }
        }
        
        //remove index if it has one
        {
            std::size_t foundSep = filename.find_last_of('_');
            if (foundSep != std::string::npos) {
                filename.erase(foundSep, std::string::npos);
            }
        }
        
        QString hashKeyStr(filename.c_str());
        
        //prepend the 2 digits of the containing directory
        {
            if (path.size() > 0) {
                if (path[path.size() - 1] == '\\' || path[path.size() -1] == '/') {
                    path.erase(path.size() - 1, 1);
                }
                
                std::size_t foundSep = path.find_last_of('/');
                if (foundSep == std::string::npos) {
                    foundSep = path.find_last_of('\\');
                }
                assert(foundSep != std::string::npos);
                std::string enclosingDirName = path.substr(foundSep + 1, std::string::npos);
                hashKeyStr.prepend(enclosingDirName.c_str());
            }
        }
        
        
        U64 hashKey = hashKeyStr.toULongLong(0,16); //< to hex (base 16)
        if (hashKey == hash) {
            return true;
        } else {
            return false;
        }
        
    }
#endif
    
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
    virtual double getTime() const = 0;
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
    , _requestedPath()
    , _entryLock(QReadWriteLock::Recursive)
    , _requestedStorage(eStorageModeNone)
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
                     StorageModeEnum storage,
                     const std::string & path)
    : _key(key)
    , _params(params)
    , _data()
    , _cache(cache)
    , _requestedPath(path)
    , _entryLock(QReadWriteLock::Recursive)
    , _requestedStorage(storage)
    , _removeBackingFileBeforeDestruction(false)
    {
    }

    virtual ~CacheEntryHelper()
    {
        if (_removeBackingFileBeforeDestruction) {
            removeAnyBackingFile();
        }
        deallocate();
    }
    
    const CacheAPI* getCacheAPI() const
    {
        return _cache;
    }

    void setCacheEntry(const KeyType & key,
                       const boost::shared_ptr<ParamsType> & params,
                       const CacheAPI* cache,
                       StorageModeEnum storage,
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
        if (_requestedStorage == eStorageModeNone) {
            return;
        }
        
        
        {
            {
                QReadLocker k(&_entryLock);
                if (_data.isAllocated()) {
                    return;
                }
            }
            QWriteLocker k(&_entryLock);
            allocate(_params->getElementsCount(),_requestedStorage,_requestedPath);
            onMemoryAllocated(false);
        }
        
        if (_cache) {
            _cache->notifyEntryAllocated( getTime(),size(),_data.getStorageMode() );
        }
    }
    
    /**
     * @brief To be called for disk-cached entries when restoring them from a file.
     * The file-path will be the one passed to the constructor
     **/
    void restoreMetaDataFromFile(std::size_t size)
    {
        if (!_cache || _requestedStorage != eStorageModeDisk) {
            return;
        }
        
        assert(!_requestedPath.empty());
        
        {
            QWriteLocker k(&_entryLock);
            
            restoreBufferFromFile(_requestedPath);
            
            onMemoryAllocated(true);

        }
        
        if (_cache) {
            _cache->notifyEntryStorageChanged(eStorageModeNone, eStorageModeDisk, getTime(),size);
        }
    }

    /**
     * @brief Called right away once the buffer is allocated. Used in debug mode to initialize image with a default color.
     * @param diskRestoration If true, this is called by restoreMetaDataFromFile() and the memory is in fact not allocated, this should
     * just restore meta-data
     **/
    virtual void onMemoryAllocated(bool /*diskRestoration*/)
    {
    }


    const KeyType & getKey() const OVERRIDE FINAL
    {
        return _key;
    }
    
    const std::string& getFilePath() const {
        return _data.getFilePath();
    }

    typename AbstractCacheEntry<KeyType>::hash_type getHashKey() const OVERRIDE FINAL
    {
        return _key.getHash();
    }

    std::string generateStringFromHash(const std::string & path,U64 hashKey) const
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
        QString hashKeyStr = QString::number(hashKey,16); //< hex is base 16
        for (int i = 0; i < 2; ++i) {
            if (i >= hashKeyStr.size()) {
                break;
            }
            name.push_back(hashKeyStr[i].toLatin1());
        }
        name.append("/");
        int i = 2;
        while (i < hashKeyStr.size()) {
            name.push_back(hashKeyStr[i].toLatin1());
            ++i;
        }
        
        name.append("." NATRON_CACHE_FILE_EXT);

        return name;
    }

    /** @brief This function is called by the get() function of the Cache when the entry is
     * living only in the disk portion of the cache. No locking is required here because the
     * caller is already preventing other threads to call this function.
     **/
    void reOpenFileMapping() const
    {
        {
            QWriteLocker k(&_entryLock);
            _data.reOpenFileMapping();
        }
        if (_cache) {
            _cache->notifyEntryStorageChanged( eStorageModeDisk, eStorageModeRAM,getTime(), size() );
        }
    }

    /**
     * @brief Can be called several times without harm
     **/
    void deallocate()
    {
        std::size_t sz = size();
        bool dataAllocated = _data.isAllocated();
        double time = getTime();
        {
            QWriteLocker k(&_entryLock);
            
            _data.deallocate();
        }
        if (_cache) {
            if ( isStoredOnDisk() ) {
                if (dataAllocated) {
                    _cache->notifyEntryStorageChanged( eStorageModeRAM, eStorageModeDisk, time, sz );
                }
            } else {
                if (dataAllocated) {
                    _cache->notifyEntryDestroyed(time, sz, eStorageModeRAM);
                }
            }
        }
    }

    /**
     * @brief Returns the size of the cache entry in bytes. This is made virtual
     * so derived class could add any extra size related to a buffer it may have (@see Image::size())
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
        bool got = _entryLock.tryLockForRead();
        std::size_t r = _data.size();
        if (got) {
            _entryLock.unlock();
        }
        return r;
    }

    bool isStoredOnDisk() const
    {
        return _data.getStorageMode() == eStorageModeDisk;
    }
    
    bool isAllocated() const
    {
        QReadLocker k(&_entryLock);
        return _data.isAllocated();
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
        bool hasRemovedFile;
        {
            QWriteLocker k(&_entryLock);
            hasRemovedFile = _data.removeAnyBackingFile();
        }
        
        if (hasRemovedFile) {
            _cache->backingFileClosed();
        }
        if ( isAlloc ) {
            _cache->notifyEntryDestroyed(getTime(), _params->getElementsCount() * sizeof(DataType),eStorageModeRAM);
        } else {
            ///size() will return 0 at this point, we have to recompute it
            _cache->notifyEntryDestroyed(getTime(), _params->getElementsCount() * sizeof(DataType),eStorageModeDisk);
        }
    }
    
    /**
     * @brief To be called when an entry is going to be removed from the cache entirely.
     **/
    void scheduleForDestruction() {
        QWriteLocker k(&_entryLock);
        _removeBackingFileBeforeDestruction = true;
    }

    virtual double getTime() const OVERRIDE FINAL
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
        size_t oldSize = size();
        _data.reallocate(elemCount);
        if (_cache) {
            _cache->notifyEntrySizeChanged( oldSize,size());
        }
    }

    void swapBuffer(CacheEntryHelper<DataType,KeyType,ParamsType>& other) {
        
        size_t oldSize = size();
        _data.swap(other._data);
        if (_cache) {
            _cache->notifyEntrySizeChanged( oldSize,size());
        }
    }

private:

    static bool fileExists(const std::string& filename)
    {
        std::ifstream f(filename.c_str());
        bool ret = f.good();
        f.close();
        return ret;
    }

    /** @brief This function is called in allocateMeory(...) and before the object is exposed
     * to other threads. Hence this function doesn't need locking mechanism at all.
     * We must ensure that this function is called ONLY by allocateMemory(), that's why
     * it is private.
     **/
    void allocate( U64 count,
                   StorageModeEnum storage,
                   std::string path = std::string() )
    {
        std::string fileName;
        
        if (storage == eStorageModeDisk) {
            
            typename AbstractCacheEntry<KeyType>::hash_type hashKey = getHashKey();
            try {
                fileName = generateStringFromHash(path,hashKey);
            } catch (const std::invalid_argument & e) {
                std::cout << "Path is empty but required for disk caching: " << e.what() << std::endl;
                return;
            }
            
            assert(!fileName.empty());
            //Check if the filename already exists, if so append a 0-based index after the hash (separated by a '_')
            //and try again
            int index = 0;
            if (fileExists(fileName)) {
                fileName.insert(fileName.size() - 4,"_0");
            }
            while (fileExists(fileName)) {
                ++index;
                std::stringstream ss;
                ss << index;
                fileName.replace(fileName.size() - 1,std::string::npos,ss.str());
            }
#ifdef DEBUG
            if (!CacheAPI::checkFileNameMatchesHash(fileName, hashKey)) {
                qDebug() << "WARNING: Cache entry filename is not the same as the serialized hash key";
            }
#endif
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
        
        if (!fileExists(path)) {
            throw std::runtime_error("Cache restore, no such file: " + path);
        }
        _data.restoreBufferFromFile(path);
       
    }

protected:

    KeyType _key;
    boost::shared_ptr<ParamsType> _params;
    Buffer<DataType> _data;
    const CacheAPI* _cache;
    std::string _requestedPath;
    mutable QReadWriteLock _entryLock;
    StorageModeEnum _requestedStorage;
    bool _removeBackingFileBeforeDestruction;
};

NATRON_NAMESPACE_EXIT;

#endif // CACHEENTRY_H
