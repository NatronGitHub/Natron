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


#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
#include <boost/scoped_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
#endif


#include "Engine/CacheEntryKeyBase.h"
#include "Engine/EngineFwd.h"
#include "Engine/RectI.h"

#include "Global/GlobalDefines.h"

NATRON_NAMESPACE_ENTER;

inline int
getSizeOfForBitDepth(ImageBitDepthEnum bitdepth)
{
    switch (bitdepth) {
        case eImageBitDepthByte: {
            return sizeof(unsigned char);
        }
        case eImageBitDepthShort: {
            return sizeof(unsigned short);
        }
        case eImageBitDepthHalf: {
            return sizeof(unsigned short);
        }
        case eImageBitDepthFloat: {
            return sizeof(float);
        }
        case eImageBitDepthNone:
            break;
    }
    
    return 0;
}

/**
 * @brief Base class for any cached item: At the very least each cache entry has a corresponding key from
 * which a 64 bit hash may be computed.
 **/
struct CacheEntryBasePrivate;
class CacheEntryBase : public boost::enable_shared_from_this<CacheEntryBase>
{

public:

    CacheEntryBase(const CachePtr& cache);

    virtual ~CacheEntryBase();

    /**
     * @brief Returns the cache owning this entry.
     **/
    CachePtr getCache() const;

    /**
     * @brief When inserted in the cache, this sets the index of the cache bucket into which this entry is inserted.
     **/
    void setCacheBucketIndex(int index);

    /**
     * @brief Get the index of the cache bucket into which this entry belong. This returns -1 if the entry does not belong
     * to the cache.
     **/
    int getCacheBucketIndex() const;

    /**
     * @brief Get the key object for this entry.
     **/
    CacheEntryKeyBasePtr getKey() const;

    /**
     * @brief Set the key object for this entry.
     * Note that this should be done prior to inserting this entry in the cache.
     **/
    void setKey(const CacheEntryKeyBasePtr& key);

    /**
     * @brief Get the hash key for this entry
     **/
    U64 getHashKey() const;

    /**
     * @brief Must return the size in bytes of the entry, so that the cache is aware
     * of the amount of memory taken by this entry.
     **/
    virtual size_t getSize() const = 0;

    /**
     * @brief If this returns true, the cache will emit a cacheChanged() signal whenever an entry of this type is 
     * inserted or removed from the cache.
     **/
    virtual bool isCacheSignalRequired() const
    {
        return false;
    }

private:

    boost::scoped_ptr<CacheEntryBasePrivate> _imp;

};


/**
 * @brief Sub-class this class to pass custom args to the allocateMemory() function of
 * your derivative class of MemoryBufferedCacheEntryBase
 **/
class AllocateMemoryArgs
{
public:

    AllocateMemoryArgs()
    : bitDepth(eImageBitDepthNone)
    {

    }

    virtual ~AllocateMemoryArgs()
    {

    }

    // The bitdpeth of the memory buffer. This information is needed for the cache
    // in order to know what memory chunk is allocated
    ImageBitDepthEnum bitDepth;
};

/**
 * @brief The base class for a cache entry that holds a memory buffer. 
 * The memory itself can be held on different storage types (OpenGL texture, RAM, MMAP'ed file...).
 **/
struct MemoryBufferedCacheEntryBasePrivate;
class MemoryBufferedCacheEntryBase : public CacheEntryBase
{
public:

    MemoryBufferedCacheEntryBase(const CachePtr& cache);

    virtual ~MemoryBufferedCacheEntryBase();

    /**
     * @brief Returns the bitdepth of the buffer
     **/
    ImageBitDepthEnum getBitDepth() const;

    /**
     * @brief Allocates the memory for this entry using the custom args.
     * Note that this function may throw an std::bad_alloc exception if it could not
     * allocate the required memory.
     **/
    void allocateMemory(const AllocateMemoryArgs& args);

    /**
     * @brief Remove any memory held by this entry. Persistent storage should not remove any
     * file associated to this memory.
     **/
    void deallocateMemory();

    /**
     * @brief Returns whether the memory of this entry is allocated or not
     **/
    bool isAllocated() const;

    /**
     * @brief Returns the internal storage that your entry uses
     **/
    virtual StorageModeEnum getStorageMode() const = 0;


protected:

    /**
     * @brief Implement to allocate the memory for the entry.
     * Note that this function may throw an std::bad_alloc exception if it could not
     * allocate the required memory.
     **/
    virtual void allocateMemoryImpl(const AllocateMemoryArgs& args) = 0;

    /**
     * @brief Implement to free the memory held by the entry
     **/
    virtual void deallocateMemoryImpl() = 0;

private:

    boost::scoped_ptr<MemoryBufferedCacheEntryBasePrivate> _imp;
};

class MMAPAllocateMemoryArgs : public AllocateMemoryArgs
{
public:

    MMAPAllocateMemoryArgs()
    : AllocateMemoryArgs()
    , cacheFilePath()
    , cacheFileDataOffset(0)
    {

    }

    virtual ~MMAPAllocateMemoryArgs()
    {

    }

    // If not null, this will pick up the tile of the given file at the given offset.
    std::string cacheFilePath;
    std::size_t cacheFileDataOffset;
};

/**
 * @brief A memory buffered entry back with MMAP.
 * The bounds of a memory mapped entry cover exactly the size of 1 tile in the cache.
 **/
struct MemoryMappedCacheEntryPrivate;
class MemoryMappedCacheEntry : public MemoryBufferedCacheEntryBase
{
public:
    MemoryMappedCacheEntry(const CachePtr& cache);

    virtual ~MemoryMappedCacheEntry();

    virtual StorageModeEnum getStorageMode() const OVERRIDE FINAL;

    virtual std::size_t getSize() const OVERRIDE FINAL;

    RectI getBounds() const;

    const char* getData() const;

    std::size_t getRowSize() const;

    char* getData();

    std::size_t getOffsetInFile() const;

    std::string getCacheFileAbsolutePath() const;

    /**
     * @brief Sync the backing file with the virtual memory currently in RAM
     **/
    void syncBackingFile();


private:

    virtual void allocateMemoryImpl(const AllocateMemoryArgs& args) OVERRIDE FINAL;

    virtual void deallocateMemoryImpl() OVERRIDE FINAL;

    boost::scoped_ptr<MemoryMappedCacheEntryPrivate> _imp;
};

inline
MemoryMappedCacheEntryPtr
toMemoryMappedCacheEntry(const CacheEntryBasePtr& entry)
{
    return boost::dynamic_pointer_cast<MemoryMappedCacheEntry>(entry);
}

class RAMAllocateMemoryArgs : public AllocateMemoryArgs
{
public:

    RAMAllocateMemoryArgs()
    : AllocateMemoryArgs()
    , numComponents(0)
    , externalBuffer(0)
    , externalBufferSize(0)
    , externalBufferFreeFunc(0)
    {

    }

    virtual ~RAMAllocateMemoryArgs()
    {

    }

    // The bounds of the image buffer
    RectI bounds;

    // When allocating an image that is not to be in the cache, this is the number of packed components (e.g: RGB) to allocate the buffer for
    std::size_t numComponents;

    // This is possible to create a RAMCacheEntry above an existing buffer. This enable caching of external data
    // If not set, a buffer covering the area of the bounds time the num components and the bitdepth will be allocated.
    void* externalBuffer;
    std::size_t externalBufferSize;

    typedef void (*ExternalBufferFreeFunction)(void* externalBuffer);

    // Ptr to a func to delete the external buffer
    ExternalBufferFreeFunction externalBufferFreeFunc;

};

/**
 * @brief A memory buffered entry backed with RAM
 * Since the storage is not handled by the cache directly, the portion covered may not necessarily be a cache tile.
 * In fact this class even supports holding an external memory buffer that is passed to allocateMemory()
 **/
struct RAMCacheEntryPrivate;
class RAMCacheEntry : public MemoryBufferedCacheEntryBase
{
public:

    RAMCacheEntry(const CachePtr& cache);

    virtual ~RAMCacheEntry();

    RectI getBounds() const;

    std::size_t getNumComponents() const;

    std::size_t getRowSize() const;

    virtual StorageModeEnum getStorageMode() const OVERRIDE FINAL;

    virtual std::size_t getSize() const OVERRIDE FINAL;

    const char* getData() const;

    char* getData();

private:

    virtual void allocateMemoryImpl(const AllocateMemoryArgs& args) OVERRIDE FINAL;

    virtual void deallocateMemoryImpl() OVERRIDE FINAL;

    boost::scoped_ptr<RAMCacheEntryPrivate> _imp;
};

inline
RAMCacheEntryPtr
toRAMCacheEntry(const CacheEntryBasePtr& entry)
{
    return boost::dynamic_pointer_cast<RAMCacheEntry>(entry);
}

class GLAllocateMemoryArgs : public AllocateMemoryArgs
{
public:

    GLAllocateMemoryArgs()
    : AllocateMemoryArgs()
    {

    }

    virtual ~GLAllocateMemoryArgs()
    {

    }

    RectI bounds;
    U32 textureTarget;
    OSGLContextPtr glContext;
};

/**
 * @brief A memory buffered entry back with an OpenGL texture.
 * The rectangle portion may be different than a tile in the cache
 **/
struct GLCacheEntryPrivate;
class GLCacheEntry : public MemoryBufferedCacheEntryBase
{
public:

    GLCacheEntry(const CachePtr& cache);

    virtual ~GLCacheEntry();

    virtual StorageModeEnum getStorageMode() const OVERRIDE FINAL;

    virtual std::size_t getSize() const OVERRIDE FINAL;

    RectI getBounds() const;

    OSGLContextPtr getOpenGLContext() const;

    U32 getGLTextureID() const;

    int getGLTextureTarget() const;

    int getGLTextureFormat() const;

    int getGLTextureInternalFormat() const;

    int getGLTextureType() const;

private:

    virtual void allocateMemoryImpl(const AllocateMemoryArgs& args) OVERRIDE FINAL;

    virtual void deallocateMemoryImpl() OVERRIDE FINAL;

    boost::scoped_ptr<GLCacheEntryPrivate> _imp;
};

inline
GLCacheEntryPtr
toGLCacheEntry(const CacheEntryBasePtr& entry)
{
    return boost::dynamic_pointer_cast<GLCacheEntry>(entry);
}

NATRON_NAMESPACE_EXIT;

#endif // CACHEENTRY_H
