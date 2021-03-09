/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2021 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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
#include <cstring> // for std::memcpy
#include <stdexcept>
#include <vector>
#ifndef _WIN32
#include <fstream>
#endif
#include <sstream> // stringstream
#include <algorithm>
#include <utility>

#ifdef __NATRON_WIN32__
#include <windows.h>
#endif

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#endif

#include <QtCore/QFile>
#include <QtCore/QMutex>
#include <QtCore/QReadWriteLock>
#include <QtCore/QDir>
#include <QtCore/QDebug>

#ifdef DEBUG
#include <SequenceParsing.h> // for removePath
#endif

#include "Engine/Hash64.h"
#include "Engine/CacheEntryHolder.h"
#include "Engine/MemoryFile.h"
#include "Engine/NonKeyParams.h"
#include "Engine/Texture.h"
#include "Engine/EngineFwd.h"
#include "Global/GlobalDefines.h"
#include "Global/StrUtils.h"

NATRON_NAMESPACE_ENTER

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
        data = (T*)malloc( size * sizeof(T) );
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

// This is a cache file with a fixed size that is a multiple of the tileByteSize.
// A bitset represents the allocated tiles in the file.
// A value of true means that a tile is used by a cache entry.
class TileCacheFile
{
public:
    MemoryFilePtr file;
    std::vector<bool> usedTiles;
};

typedef TileCacheFilePtr TileCacheFilePtr;

/**
 * @brief Defines the API of the Cache as seen by the cache entries
 **/
class CacheAPI
{
public:
    virtual ~CacheAPI() {}

    /**
     * @brief Returns the path to the cache location on disk
     **/
    virtual QString getCachePath() const = 0;

    /**
     * @brief Return true if the cache is tiled
     **/
    virtual bool isTileCache() const = 0;

    /**
     * @brief Returns the number of bytes occupied by a tile in the cache
     **/
    virtual std::size_t getTileSizeBytes() const = 0;

    /**
     * @brief To be called by a CacheEntry whenever it's size is changed.
     * This way the cache can keep track of the real memory footprint.
     **/
    virtual void notifyEntrySizeChanged(size_t oldSize, size_t newSize) const = 0;

    /**
     * @brief To be called by a CacheEntry on allocation.
     **/
    virtual void notifyEntryAllocated(double time, size_t size, StorageModeEnum storage) const = 0;

    /**
     * @brief To be called by a CacheEntry on destruction.
     **/
    virtual void notifyEntryDestroyed(double time, size_t size, StorageModeEnum storage) const = 0;

    /**
     * @brief Called by the Cache deleter thread to wake up sleeping threads that were attempting to create a new image
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
    virtual void notifyEntryStorageChanged(StorageModeEnum oldStorage, StorageModeEnum newStorage,
                                           double time, size_t size) const = 0;

    /**
     * @brief Remove from the cache all entries that matches the holderID and have a different nodeHash than the given one.
     * @param removeAll If true, remove even entries that match the nodeHash
     **/
    virtual void removeAllEntriesWithDifferentNodeHashForHolderPrivate(const std::string& holderID, U64 nodeHash, bool removeAll) = 0;

    /**
     * @brief Relevant only for tiled caches. This will allocate the memory required for a tile in the cache and lock it.
     * Note that the calling entry should have exactly the size of a tile in the cache.
     * In return, a pointer to a memory file is returned and the output parameter dataOffset will be set to the offset - in bytes - where the
     * contiguous memory block for this tile begin relative to the start of the data of the memory file.
     * This function may throw exceptions in case of failure.
     * To retrieve the exact pointer of the block of memory for this tile use tileFile->file->data() + dataOffset
     **/
    virtual TileCacheFilePtr allocTile(std::size_t *dataOffset) = 0;

    /**
     * @brief Return a pointer to the tile cache file from its filepath
     **/
    virtual TileCacheFilePtr getTileCacheFile(const std::string& filepath,std::size_t dataOffset) = 0;

    /**
     * @brief Free a tile from the cache that was previously allocated with allocTile. It will be made available again for other entries.
     **/
    virtual void freeTile(const TileCacheFilePtr& file, std::size_t dataOffset) = 0;

#ifdef DEBUG
    static bool checkFileNameMatchesHash(const std::string &originalFileName,
                                         U64 hash)
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
        QString hashKeyStr = QString::fromUtf8( filename.c_str() );

        //prepend the 2 digits of the containing directory
        {
            if (path.size() > 0) {
                if ( (path[path.size() - 1] == '\\') || (path[path.size() - 1] == '/') ) {
                    path.erase(path.size() - 1, 1);
                }

                std::size_t foundSep = path.find_last_of('/');
                if (foundSep == std::string::npos) {
                    foundSep = path.find_last_of('\\');
                }
                assert(foundSep != std::string::npos);
                std::string enclosingDirName = path.substr(foundSep + 1, std::string::npos);
                hashKeyStr.prepend( QString::fromUtf8( enclosingDirName.c_str() ) );
            }
        }
        U64 hashKey = hashKeyStr.toULongLong(0, 16); //< to hex (base 16)

        if (hashKey == hash) {
            return true;
        } else {
            return false;
        }
    }
    
#endif

    static bool fileExists(const std::string& filename)
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
};

class AbstractCacheEntryBase : boost::noncopyable
{
public:

    AbstractCacheEntryBase()
    {

    }

    virtual ~AbstractCacheEntryBase()
    {

    }

    virtual TileCacheFilePtr allocTile(std::size_t *dataOffset) = 0;
    virtual void freeTile(const TileCacheFilePtr& file, std::size_t dataOffset) = 0;
    virtual TileCacheFilePtr getTileCacheFile(const std::string& filepath, std::size_t dataOffset) = 0;
    virtual std::size_t getCacheTileSizeBytes() const = 0;

    virtual size_t size() const = 0;
    virtual double getTime() const = 0;
    virtual U64 getElementsCountFromParams() const = 0;

    virtual void syncBackingFile() const = 0;
};


/** @brief Abstract interface for cache entries.
 * KeyType must inherit KeyHelper
 **/
template<typename KeyType>
class AbstractCacheEntry : public AbstractCacheEntryBase
{
public:

    typedef typename KeyType::hash_type hash_type;
    typedef KeyType key_type;

    AbstractCacheEntry()
    : AbstractCacheEntryBase()
    {
    };

    virtual ~AbstractCacheEntry()
    {
    }

    virtual const KeyType & getKey() const = 0;
    virtual hash_type getHashKey() const = 0;


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
        , _entry(0)
        , _cacheFile()
        , _cacheFileDataOffset(0)
        , _storageMode(eStorageModeRAM)
    {
    }

    ~Buffer()
    {
        deallocate();
    }

    void allocateRAM(U64 count)
    {
        if ( _buffer && (_buffer->size() > 0) ) {
            return;
        }
        _storageMode = eStorageModeRAM;
        if (!_buffer) {
            _buffer.reset( new RamBuffer<DataType>() );
        }
        _buffer->resize(count);
    }

    void allocateMMAP(U64 count,
                      const std::string& path)
    {
        assert( _path.empty() );
        if (_backingFile) {
            return;
        }
        _storageMode = eStorageModeDisk;
        _path = path;
        try {
            _backingFile.reset( new MemoryFile(_path, MemoryFile::eFileOpenModeEnumIfExistsKeepElseCreate) );
        } catch (const std::runtime_error & r) {
            qDebug() << r.what();
            // if opening the file mapping failed, just call allocate again, but this time on RAM!
            _backingFile.reset();
            _path.clear();
            allocateRAM(count);

            return;
        }

        assert(_backingFile);

        if ( !path.empty() && (count != 0) ) {
            //if the backing file has already the good size and we just wanted to re-open the mapping
            _backingFile->resize(count);
        }
    }

    void allocateGLTexture(const RectI& rectangle,
                           U32 target)
    {
        if (_glTexture) {
            return;
        }

        _storageMode = eStorageModeGLTex;
        assert(!_glTexture);


        int glType, internalFormat;
        int format = GL_RGBA;
        Texture::DataTypeEnum type = Texture::eDataTypeFloat;
        /*if (sizeOfData == 1) {
            type = Texture::eDataTypeByte;
            internalFormat = GL_RGBA8;
            glType = GL_UNSIGNED_INT_8_8_8_8_REV;
           } else if (sizeOfData == 2) {
            internalFormat = Texture::eDataTypeUShort;
            internalFormat = GL_RGBA16;
            glType = GL_UNSIGNED_SHORT;
           } else {*/

        // EDIT: for now, only use RGBA fp OpenGL textures, let glReadPixels do the conversion for us
        internalFormat = GL_RGBA32F_ARB;
        glType = GL_FLOAT;
        //}

        _glTexture.reset( new Texture(target,
                                      GL_NONE,
                                      GL_NONE,
                                      GL_NONE,
                                      type,
                                      format,
                                      internalFormat,
                                      glType) );


        TextureRect r(rectangle.x1, rectangle.y1, rectangle.x2, rectangle.y2, 1., 1.);

        // This calls glTexImage2D and allocates a RGBA image
        _glTexture->ensureTextureHasSize(r, 0);
    }

    void allocateTileCache(AbstractCacheEntryBase* entry)
    {
        _storageMode = eStorageModeDisk;
        _entry = entry;
        try {
            _cacheFile = entry->allocTile(&_cacheFileDataOffset);
            _path = _cacheFile->file->path();
        } catch (...) {
            allocateRAM(entry->getElementsCountFromParams());
        }
    }

    /**
     * @brief Beware this is not really a "swap" as other do not get the infos from this Buffer.
     **/
    void swap(Buffer& other)
    {
        if (_storageMode == eStorageModeRAM) {
            if (other._storageMode == eStorageModeRAM) {
                if (other._buffer) {
                    if (!_buffer) {
                        _buffer.reset( new RamBuffer<DataType>() );
                    }
                    _buffer.swap(other._buffer);
                }
            } else {
                if (!_buffer) {
                    _buffer.reset( new RamBuffer<DataType>() );
                }
                _buffer->resize( other._backingFile->size() / sizeof(DataType) );
                const char* src = other._backingFile->data();
                char* dst = (char*)_buffer->getData();
                std::memcpy( dst, src, other._backingFile->size() );
            }
        } else if (_storageMode == eStorageModeDisk) {
            if (other._storageMode == eStorageModeDisk) {
                assert(_backingFile);
                _backingFile.swap(other._backingFile);
                _path = other._path;
            } else {
                _backingFile->resize( other._buffer->size() * sizeof(DataType) );
                assert( _backingFile->data() );
                const char* src = (const char*)other._buffer->getData();
                char* dst = (char*)_backingFile->data();
                std::memcpy( dst, src, other._buffer->size() * sizeof(DataType) );
            }
        }
    }

    const std::string& getFilePath() const
    {
        return _path;
    }

    std::size_t getOffsetInFile() const
    {
        return _cacheFileDataOffset;
    }

    void reOpenFileMapping() const
    {
        assert(!_backingFile && _storageMode == eStorageModeDisk);
        try{
            _backingFile.reset( new MemoryFile(_path, MemoryFile::eFileOpenModeEnumIfExistsKeepElseCreate) );
        } catch (const std::exception & e) {
            _backingFile.reset();
            throw std::bad_alloc();
        }
    }

    void restoreBufferFromFile(const std::string & path, std::size_t dataOffset, AbstractCacheEntryBase* entry, bool isTileCache)
    {
        _entry = entry;
        if (isTileCache) {
            _cacheFile = entry->getTileCacheFile(path, dataOffset);
            if (!_cacheFile) {
                throw std::runtime_error("Unexisting file " + path);
            }
            _cacheFileDataOffset = dataOffset;
        }
        _path = path;
        _storageMode = eStorageModeDisk;
    }

    void deallocate()
    {
        if (_storageMode == eStorageModeRAM) {
            if (_buffer) {
                _buffer->clear();
            }
        } else if (_storageMode == eStorageModeDisk) {
            if (_backingFile) {
                bool flushOk = _backingFile->flush(MemoryFile::eFlushTypeAsync, 0, 0);
                _backingFile.reset();
                if (!flushOk) {
                    throw std::runtime_error("Failed to flush RAM data to backing file.");
                }
            } else if (_cacheFile) {
                assert(_entry);
                _entry->freeTile(_cacheFile, _cacheFileDataOffset);
                _cacheFile.reset();
            }
        } else if (_storageMode == eStorageModeGLTex) {
            if (_glTexture) {
                _glTexture.reset();
            }
        }
    }

    void syncBackingFile() const
    {
        if (_backingFile) {
            _backingFile->flush(MemoryFile::eFlushTypeAsync, 0, 0);
        } else if (_cacheFile && _entry) {
            _cacheFile->file->flush(MemoryFile::eFlushTypeAsync, _cacheFile->file->data() + _cacheFileDataOffset, _entry->getCacheTileSizeBytes());
        }
    }

    bool removeAnyBackingFile() const
    {
        if (_storageMode == eStorageModeDisk && !_cacheFile) {
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
            return _buffer ? _buffer->size() * sizeof(DataType) : 0;
        } else if (_storageMode == eStorageModeDisk) {
            if (_backingFile) {
                return _backingFile->size();
            } else if (_cacheFile) {
                assert(_entry);
                return _entry->getCacheTileSizeBytes();
            } else {
                return 0;
            }
        } else if (_storageMode == eStorageModeGLTex) {
            return _glTexture ? _glTexture->getSize() : 0;
        }

        return 0;
    }

    bool isAllocated() const
    {
        return (_buffer && _buffer->size() > 0) || ( _backingFile && _backingFile->data() ) || _cacheFile || _glTexture;
    }

    DataType* writable()
    {
        if (_storageMode == eStorageModeDisk) {
            if (_backingFile) {
                return (DataType*)_backingFile->data();
            } else if (_cacheFile) {
                return (DataType*)(_cacheFile->file->data() + _cacheFileDataOffset);
            } else {
                return NULL;
            }
        } else if (_storageMode == eStorageModeRAM) {
            return _buffer ? _buffer->getData() : NULL;
        } else {
            // Other storage modes don't provide direct access to RAM handle
            return NULL;
        }
    }

    const DataType* readable() const
    {
        if (_storageMode == eStorageModeDisk) {
            if (_backingFile) {
                return (const DataType*)_backingFile->data();
            } else if (_cacheFile) {
                return (const DataType*)(_cacheFile->file->data() + _cacheFileDataOffset);
            } else {
                return 0;
            }
        } else if (_storageMode == eStorageModeRAM) {
            return _buffer ? _buffer->getData() : NULL;
        } else {
            // Other storage modes don't provide direct access to RAM handle
            return NULL;
        }
    }

    StorageModeEnum getStorageMode() const
    {
        return _storageMode;
    }

    U32 getGLTextureID() const
    {
        return _glTexture ? _glTexture->getTexID() : 0;
    }

    int getGLTextureTarget() const
    {
        return _glTexture ? _glTexture->getTexTarget() : 0;
    }

    int getGLTextureFormat() const
    {
        return _glTexture ? _glTexture->getFormat() : 0;
    }

    int getGLTextureInternalFormat() const
    {
        return _glTexture ? _glTexture->getInternalFormat() : 0;
    }

    int getGLTextureType() const
    {
        return _glTexture ? _glTexture->getGLType() : 0;
    }

private:

    std::string _path;
    boost::scoped_ptr<RamBuffer<DataType> > _buffer;

    /*mutable so the reOpenFileMapping function can reopen the mapped file. It doesn't
       change the underlying data*/
    mutable boost::scoped_ptr<MemoryFile> _backingFile;

    // Set if the cache is a tile cache
    AbstractCacheEntryBase* _entry;
    TileCacheFilePtr _cacheFile;
    std::size_t _cacheFileDataOffset;

    // Used when we store images as OpenGL textures
    boost::scoped_ptr<Texture> _glTexture;
    StorageModeEnum _storageMode;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////CACHE ENTRY////////////////////////////////////////////////////




/** @brief Implements AbstractCacheEntry. This class represents a combinaison of
 * a set of metadata called 'Key' and a buffer.
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
    typedef boost::shared_ptr<ParamsType> ParamsTypePtr;

    /**
     * @brief Ctor
     * the cache entry needs to be set afterwards using setCacheEntry()
     **/
    CacheEntryHelper()
        : _key()
        , _params()
        , _data()
        , _cache()
        , _entryLock(QReadWriteLock::Recursive)
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
                     const ParamsTypePtr & params,
                     const CacheAPI* cache)
        : _key(key)
        , _params(params)
        , _data()
        , _cache(cache)
        , _entryLock(QReadWriteLock::Recursive)
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
                       const ParamsTypePtr & params,
                       const CacheAPI* cache)
    {
        assert(!_params && _cache == NULL);
        _key = key;
        _params = params;
        _cache = cache;
    }

    void setKey(const KeyType& key) {
        _key = key;
    }

    /**
     * @brief Allocates the memory required by the cache entry. It allocates enough memory to contain at least the
     * memory specified by the key.
     * WARNING: This function throws a std::bad_alloc if the allocation fails.
     *
     **/
    void allocateMemory()
    {
        const CacheEntryStorageInfo& storageInfo = _params->getStorageInfo();

        if (storageInfo.mode == eStorageModeNone) {
            return;
        }


        {
            {
                QReadLocker k(&_entryLock);
                if ( _data.isAllocated() ) {
                    return;
                }
            }
            QWriteLocker k(&_entryLock);
            if ( _data.isAllocated() ) {
                return;
            }
            allocate();
            onMemoryAllocated(false);
        }

        if (_cache) {
            _cache->notifyEntryAllocated( getTime(), size(), storageInfo.mode );
        }
    }

    /**
     * @brief To be called for disk-cached entries when restoring them from a file.
     **/
    void restoreMetadataFromFile(std::size_t size, const std::string& filePath, std::size_t dataOffset)
    {
        const CacheEntryStorageInfo& storageInfo = _params->getStorageInfo();

        if ( !_cache || (storageInfo.mode != eStorageModeDisk) ) {
            return;
        }

        {
            QWriteLocker k(&_entryLock);

            restoreBufferFromFile(filePath, dataOffset);

            onMemoryAllocated(true);
        }

        if (_cache) {
            if (_cache->isTileCache()) {
                _cache->notifyEntryAllocated(getTime(), size, eStorageModeDisk);
            } else {
                _cache->notifyEntryStorageChanged(eStorageModeNone, eStorageModeDisk, getTime(), size);
            }
        }
    }

    /**
     * @brief Called right away once the buffer is allocated. Used in debug mode to initialize image with a default color.
     * @param diskRestoration If true, this is called by restoreMetadataFromFile() and the memory is in fact not allocated, this should
     * just restore meta-data
     **/
    virtual void onMemoryAllocated(bool /*diskRestoration*/)
    {
    }

    const KeyType & getKey() const OVERRIDE FINAL
    {
        return _key;
    }

    const std::string& getFilePath() const
    {
        return _data.getFilePath();
    }

    typename AbstractCacheEntry<KeyType>::hash_type getHashKey() const OVERRIDE FINAL
    {
        return _key.getHash();
    }

    std::string generateStringFromHash(const std::string & path,
                                       U64 hashKey) const
    {
        std::string name(path);

        if ( path.empty() ) {
            QDir subfolder( QString::fromUtf8( path.c_str() ) );
            if ( !subfolder.exists() ) {
                std::cout << "(" << std::hex <<
                    this << ") " <<   "Something is wrong in cache... couldn't find : " << path << std::endl;
                throw std::invalid_argument(path);
            }
        }
        QString hashKeyStr = QString::number(hashKey, 16); //< hex is base 16
        for (int i = 0; i < 2; ++i) {
            if ( i >= hashKeyStr.size() ) {
                break;
            }
            name.push_back( hashKeyStr[i].toLatin1() );
        }
        name.append("/");
        int i = 2;
        while ( i < hashKeyStr.size() ) {
            name.push_back( hashKeyStr[i].toLatin1() );
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
        if (_cache && _cache->isTileCache()) {
            return;
        }
        {
            QWriteLocker k(&_entryLock);
            _data.reOpenFileMapping();
        }
        if (_cache) {
            _cache->notifyEntryStorageChanged( eStorageModeDisk, eStorageModeRAM, getTime(), size() );
        }
    }

    /**
     * @brief Can be called several times without harm
     **/
    void deallocate()
    {
        std::size_t sz = size();
        bool dataAllocated;
        double time = getTime();
        {
            QWriteLocker k(&_entryLock);
            dataAllocated = _data.isAllocated();
            _data.deallocate();
        }

        if (_cache) {
            const CacheEntryStorageInfo& info = _params->getStorageInfo();
            if (info.mode == eStorageModeDisk) {
                if (dataAllocated) {
                    if (_cache->isTileCache()) {
                         _cache->notifyEntryDestroyed(time, sz, eStorageModeDisk);
                    } else {
                        _cache->notifyEntryStorageChanged( eStorageModeRAM, eStorageModeDisk, time, sz );
                    }
                }
            } else if (info.mode == eStorageModeRAM) {
                if (dataAllocated) {
                    _cache->notifyEntryDestroyed(time, sz, eStorageModeRAM);
                }
            } else if (info.mode == eStorageModeGLTex) {
                if (dataAllocated) {
                    _cache->notifyEntryDestroyed(time, sz, eStorageModeGLTex);
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

    std::size_t getOffsetInFile() const
    {
        return _data.getOffsetInFile();
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

    virtual void syncBackingFile() const OVERRIDE FINAL
    {
        QWriteLocker k(&_entryLock);
        return _data.syncBackingFile();
    }

    /**
     * @brief An entry stored on disk is effectively destroyed when its backing file is removed.
     **/
    void removeAnyBackingFile() const
    {
        if ( !isStoredOnDisk() || _cache->isTileCache()) {
            return;
        }

        bool isAlloc;
        bool hasRemovedFile;
        {
            QWriteLocker k(&_entryLock);
            isAlloc = _data.isAllocated();
            hasRemovedFile = _data.removeAnyBackingFile();
        }

        if (hasRemovedFile) {
            _cache->backingFileClosed();
        }
        if (isAlloc) {
            _cache->notifyEntryDestroyed(getTime(), getElementsCountFromParams(), eStorageModeRAM);
        } else {
            ///size() will return 0 at this point, we have to recompute it
            _cache->notifyEntryDestroyed(getTime(), getElementsCountFromParams(), eStorageModeDisk);
        }
    }

    /**
     * @brief To be called when an entry is going to be removed from the cache entirely.
     **/
    void scheduleForDestruction()
    {
        QWriteLocker k(&_entryLock);

        _removeBackingFileBeforeDestruction = true;
    }

    virtual double getTime() const OVERRIDE FINAL
    {
        return _key.getTime();
    }

    ParamsTypePtr getParams() const WARN_UNUSED_RETURN
    {
        return _params;
    }

    std::size_t getSizeInBytesFromParams() const
    {
        return getElementsCountFromParams() * sizeof(DataType);
    }

    virtual U64 getElementsCountFromParams() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        const CacheEntryStorageInfo& info = _params->getStorageInfo();

        return info.dataTypeSize * info.numComponents * info.bounds.area();
    }

    U32 getGLTextureID() const
    {
        return _data.getGLTextureID();
    }

    int getGLTextureFormat() const
    {
        return _data.getGLTextureFormat();
    }

    int getGLTextureInternalFormat() const
    {
        return _data.getGLTextureInternalFormat();
    }

    int getGLTextureType() const
    {
        return _data.getGLTextureType();
    }

    int getGLTextureTarget() const
    {
        return _data.getGLTextureTarget();
    }

    virtual std::size_t getCacheTileSizeBytes() const OVERRIDE FINAL
    {
        assert(_cache);
        return _cache->getTileSizeBytes();
    }

protected:


    void swapBuffer(CacheEntryHelper<DataType, KeyType, ParamsType>& other)
    {
        size_t oldSize = size();

        _data.swap(other._data);
        if (_cache) {
            _cache->notifyEntrySizeChanged( oldSize, size() );
        }
    }

private:

    virtual TileCacheFilePtr allocTile(std::size_t *dataOffset) OVERRIDE FINAL
    {
        assert(_cache);
        return const_cast<CacheAPI*>(_cache)->allocTile(dataOffset);
    }

    virtual void freeTile(const TileCacheFilePtr& file, std::size_t dataOffset) OVERRIDE FINAL
    {
        assert(_cache);
        const_cast<CacheAPI*>(_cache)->freeTile(file,dataOffset);
    }

    virtual TileCacheFilePtr getTileCacheFile(const std::string& filepath, std::size_t dataOffset) OVERRIDE FINAL
    {
        assert(_cache);
        return const_cast<CacheAPI*>(_cache)->getTileCacheFile(filepath, dataOffset);
    }



    /** @brief This function is called in allocateMeory(...) and before the object is exposed
     * to other threads. Hence this function doesn't need locking mechanism at all.
     * We must ensure that this function is called ONLY by allocateMemory(), that's why
     * it is private.
     **/
    void allocate()
    {
        CacheEntryStorageInfo& info = _params->getStorageInfo();

        // _cache should be non-NULL for eStorageModeDisk
        if (info.mode == eStorageModeDisk && _cache) {
            if (_cache && _cache->isTileCache()) {
                _data.allocateTileCache(this);
            } else {
                std::string fileName;

                typename AbstractCacheEntry<KeyType>::hash_type hashKey = getHashKey();

                assert(_cache);
                std::string cachePath = _cache->getCachePath().toStdString();
                cachePath += '/';

                try {
                    fileName = generateStringFromHash(cachePath, hashKey);
                } catch (const std::invalid_argument & e) {
                    std::cout << "Path is empty but required for disk caching: " << e.what() << std::endl;

                    return;
                }

                assert( !fileName.empty() );
                //Check if the filename already exists, if so append a 0-based index after the hash (separated by a '_')
                //and try again
                int index = 0;
                if ( CacheAPI::fileExists(fileName) ) {
                    fileName.insert(fileName.size() - 4, "_0");
                }
                while ( CacheAPI::fileExists(fileName) ) {
                    ++index;
                    std::stringstream ss;
                    ss << index;
                    fileName.replace( fileName.size() - 1, std::string::npos, ss.str() );
                }
#ifdef DEBUG
                if ( !CacheAPI::checkFileNameMatchesHash(fileName, hashKey) ) {
                    qDebug() << "WARNING: Cache entry filename is not the same as the serialized hash key";
                }
#endif
                U64 count = getElementsCountFromParams();
                _data.allocateMMAP(count, fileName);
            }
        } else if (info.mode == eStorageModeRAM) {
            U64 count = getElementsCountFromParams();
            _data.allocateRAM(count);
        } else if (info.mode == eStorageModeGLTex) {
            _data.allocateGLTexture(info.bounds, info.textureTarget);
        }
    }

    /** @brief This function is called in allocateMeory() and before the object is exposed
     * to other threads. Hence this function doesn't need locking mechanism at all.
     * We must ensure that this function is called ONLY by allocateMemory(), that's why
     * it is private.
     **/
    void restoreBufferFromFile(const std::string & path, std::size_t offset)
    {

        bool isTileCache = _cache && _cache->isTileCache();
        if (!isTileCache && !CacheAPI::fileExists(path) ) {
            throw std::runtime_error("Cache restore, no such file: " + path);
        }
        _data.restoreBufferFromFile(path, offset, this, isTileCache);
    }

protected:

    friend class Buffer<DataType>;

    KeyType _key;
    ParamsTypePtr _params;
    Buffer<DataType> _data;
    const CacheAPI* _cache;
    mutable QReadWriteLock _entryLock;
    bool _removeBackingFileBeforeDestruction;
};

NATRON_NAMESPACE_EXIT

#endif // CACHEENTRY_H
