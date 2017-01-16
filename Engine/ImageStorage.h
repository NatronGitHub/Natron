/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2017 INRIA and Alexandre Gauthier-Foichat
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

#ifndef NATRON_ENGINE_IMAGESTORAGE_H
#define NATRON_ENGINE_IMAGESTORAGE_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/GlobalDefines.h"

#include "Engine/EngineFwd.h"

#include "Engine/CacheEntryBase.h"

NATRON_NAMESPACE_ENTER;


/**
 * @brief Sub-class this class to pass custom args to the allocateMemory() function of
 * your derivative class of ImageStorageBase
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
 * @brief The base class for image storage.
 * The memory itself can be held on different storage types (OpenGL texture, RAM, shared memory...).
 **/
struct ImageStorageBasePrivate;
class ImageStorageBase
{
public:

    ImageStorageBase();

    virtual ~ImageStorageBase();

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

    /**
     * @brief Returns the size in bytes occupied by the buffer.
     **/
    virtual std::size_t getBufferSize() const = 0;


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

    boost::scoped_ptr<ImageStorageBasePrivate> _imp;
};



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

    // This is possible to create a RAMImageStorage above an existing buffer. This enable caching of external data
    // If not set, a buffer covering the area of the bounds time the num components and the bitdepth will be allocated.
    void* externalBuffer;
    std::size_t externalBufferSize;

    typedef void (*ExternalBufferFreeFunction)(void* externalBuffer);

    // Ptr to a func to delete the external buffer
    ExternalBufferFreeFunction externalBufferFreeFunc;

};

/**
 * @brief Image storaged based in RAM process memory.
 * The allocate() args must be of RAMAllocateMemoryArgs type.
 **/
struct RAMImageStoragePrivate;
class RAMImageStorage : public ImageStorageBase
{
public:

    RAMImageStorage();

    virtual ~RAMImageStorage();

    RectI getBounds() const;

    std::size_t getNumComponents() const;

    std::size_t getRowSize() const;

    virtual StorageModeEnum getStorageMode() const OVERRIDE FINAL;

    virtual std::size_t getBufferSize() const OVERRIDE FINAL;

    const char* getData() const;

    char* getData();

private:

    virtual void allocateMemoryImpl(const AllocateMemoryArgs& args) OVERRIDE FINAL;

    virtual void deallocateMemoryImpl() OVERRIDE FINAL;

    boost::scoped_ptr<RAMImageStoragePrivate> _imp;
};

inline
RAMImageStoragePtr
toRAMImageStorage(const ImageStorageBasePtr& entry)
{
    return boost::dynamic_pointer_cast<RAMImageStorage>(entry);
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
 * @brief Image storage based on an OpenGL texture.
 * The allocate() args must be of GLAllocateMemoryArgs type.
 **/
struct GLImageStoragePrivate;
class GLImageStorage : public ImageStorageBase
{
public:

    GLImageStorage();

    virtual ~GLImageStorage();

    virtual StorageModeEnum getStorageMode() const OVERRIDE FINAL;

    virtual std::size_t getBufferSize() const OVERRIDE FINAL;

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

    boost::scoped_ptr<GLImageStoragePrivate> _imp;
};

inline
GLImageStoragePtr
toGLImageStorage(const ImageStorageBasePtr& entry)
{
    return boost::dynamic_pointer_cast<GLImageStorage>(entry);
}


/**
 * @brief Image storage based on the cache shared memory.
 * Unlike other storage modes, the size of such an image storage is 
 * exactly the size of a tile in the cache: NATRON_TILE_SIZE_BYTES
 * The allocate() args must be of CacheAllocateMemoryArgs type.
 **/
struct CacheImageTileStoragePrivate;
class CacheImageTileStorage
: public ImageStorageBase
, public CacheEntryBase
{
public:
    CacheImageTileStorage(const CachePtr& cache);

    virtual ~CacheImageTileStorage();

    virtual StorageModeEnum getStorageMode() const OVERRIDE FINAL;

    virtual std::size_t getMetadataSize() const OVERRIDE FINAL;

    virtual std::size_t getBufferSize() const OVERRIDE FINAL;

    RectI getBounds() const;

    const char* getData() const;

    std::size_t getRowSize() const;

    char* getData();

    virtual bool isStorageTiled() const OVERRIDE FINAL;

    virtual void toMemorySegment(ExternalSegmentType* segment, void* tileDataPtr) const OVERRIDE FINAL;

    virtual void fromMemorySegment(const ExternalSegmentType& segment, const void* tileDataPtr) OVERRIDE FINAL;

private:

    virtual void allocateMemoryImpl(const AllocateMemoryArgs& args) OVERRIDE FINAL;

    virtual void deallocateMemoryImpl() OVERRIDE FINAL;

    boost::scoped_ptr<CacheImageTileStoragePrivate> _imp;
};

inline
CacheImageTileStoragePtr
toCacheImageTileStorage(const ImageStorageBasePtr& entry)
{
    return boost::dynamic_pointer_cast<CacheImageTileStorage>(entry);
}

inline
CacheImageTileStoragePtr
toCacheImageTileStorage(const CacheEntryBasePtr& entry)
{
    return boost::dynamic_pointer_cast<CacheImageTileStorage>(entry);
}

NATRON_NAMESPACE_EXIT;

#endif // NATRON_ENGINE_IMAGESTORAGE_H
