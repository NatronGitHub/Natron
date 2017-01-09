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



// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "CacheEntryBase.h"

#include <QDir>

#include "Engine/Cache.h"
#include "Engine/MemoryFile.h"
#include "Engine/OSGLContext.h"
#include "Engine/RamBuffer.h"
#include "Engine/Texture.h"

NATRON_NAMESPACE_ENTER;

struct CacheEntryBasePrivate
{
    QMutex lock;

    CacheWPtr cache;
    CacheEntryKeyBasePtr key;
    int cacheBucketIndex;

    CacheEntryBasePrivate(const CachePtr& cache)
    : lock()
    , cache(cache)
    , key()
    , cacheBucketIndex(-1)
    {
        
    }
};

CacheEntryBase::CacheEntryBase(const CachePtr& cache)
: _imp(new CacheEntryBasePrivate(cache))
{

}

CacheEntryBase::~CacheEntryBase()
{

}

CachePtr
CacheEntryBase::getCache() const
{
    return _imp->cache.lock();
}

void
CacheEntryBase::setCacheBucketIndex(int index)
{
    QMutexLocker (&_imp->lock);
    _imp->cacheBucketIndex = index;
}


int
CacheEntryBase::getCacheBucketIndex() const
{
    QMutexLocker (&_imp->lock);
    return _imp->cacheBucketIndex;
}


CacheEntryKeyBasePtr
CacheEntryBase::getKey() const
{
    QMutexLocker (&_imp->lock);
    return _imp->key;
}

void
CacheEntryBase::setKey(const CacheEntryKeyBasePtr& key)
{
    QMutexLocker (&_imp->lock);
    _imp->key = key;
}

U64
CacheEntryBase::getHashKey() const
{
    QMutexLocker (&_imp->lock);
    if (!_imp->key) {
        return 0;
    }
    return _imp->key->getHash();
}

struct MemoryBufferedCacheEntryBasePrivate
{
    bool allocated;
    QMutex allocatedLock;
    RectI bounds;
    ImageBitDepthEnum bitdepth;

    MemoryBufferedCacheEntryBasePrivate()
    : allocated(false)
    , allocatedLock()
    , bounds()
    , bitdepth()
    {

    }
};

MemoryBufferedCacheEntryBase::MemoryBufferedCacheEntryBase(const CachePtr& cache)
: CacheEntryBase(cache)
{

}

MemoryBufferedCacheEntryBase::~MemoryBufferedCacheEntryBase()
{

}

ImageBitDepthEnum
MemoryBufferedCacheEntryBase::getBitDepth() const
{
    QMutexLocker k(&_imp->allocatedLock);
    return _imp->bitdepth;
}

bool
MemoryBufferedCacheEntryBase::isAllocated() const
{
    QMutexLocker k(&_imp->allocatedLock);
    return _imp->allocated;
}

void
MemoryBufferedCacheEntryBase::allocateMemory(const AllocateMemoryArgs& args)
{
    if (isAllocated()) {
        return;
    }

    allocateMemoryImpl(args);

    CachePtr cache = getCache();
    if (cache) {
        // Update the cache size
        cache->notifyEntryAllocated( getSize(), getStorageMode() );
    }
}

void
MemoryBufferedCacheEntryBase::deallocateMemory()
{
    bool dataAllocated = isAllocated();
    if (!dataAllocated) {
        return;
    }

    deallocateMemoryImpl();
    
    CachePtr cache = getCache();
    if (cache) {
        cache->notifyEntryDestroyed(getSize(), getStorageMode());
    }
}

struct MemoryMappedCacheEntryPrivate
{
    // The cache file used
    TileCacheFilePtr cacheFile;

    // The offset in the cache file at which starts this buffer
    std::size_t cacheFileDataOffset;

    MemoryMappedCacheEntryPrivate()
    : cacheFile()
    , cacheFileDataOffset(0)
    {

    }
};

MemoryMappedCacheEntry::MemoryMappedCacheEntry(const CachePtr& cache)
: MemoryBufferedCacheEntryBase(cache)
, _imp(new MemoryMappedCacheEntryPrivate())
{

}

MemoryMappedCacheEntry::~MemoryMappedCacheEntry()
{

}

StorageModeEnum
MemoryMappedCacheEntry::getStorageMode() const
{
    return eStorageModeDisk;
}

std::size_t
MemoryMappedCacheEntry::getSize() const
{
    CachePtr cache = getCache();
    if (!cache) {
        return 0;
    }
    return cache->getTileSizeBytes();
}

RectI
MemoryMappedCacheEntry::getBounds() const
{
    RectI ret;
    CachePtr cache = getCache();
    if (!cache) {
        return ret;
    }
    int tileSizeX, tileSizeY;
    cache->getTileSizePx(getBitDepth(), &tileSizeX, &tileSizeY);
    // Recover the bottom left corner from the tile coords
    {
        CacheEntryKeyBasePtr key = getKey();
        ImageTileKey* isImageTile = dynamic_cast<ImageTileKey*>(key.get());
        if (isImageTile) {
            ret.x1 = isImageTile->getTileX() * tileSizeX;
            ret.y1 = isImageTile->getTileY() * tileSizeY;
        }
    }
    ret.x2 = ret.x1 + tileSizeX;
    ret.y2 = ret.y1 + tileSizeY;
    return ret;
}

std::size_t
MemoryMappedCacheEntry::getRowSize() const
{
    CachePtr cache = getCache();
    if (!cache) {
        return 0;
    }
    ImageBitDepthEnum bitDepth = getBitDepth();
    int tileSizeX, tileSizeY;
    cache->getTileSizePx(bitDepth,&tileSizeX, &tileSizeY);

    return tileSizeX * getSizeOfForBitDepth(bitDepth);

}

const char*
MemoryMappedCacheEntry::getData() const
{
    if (!_imp->cacheFile) {
        return 0;
    }
    return (const char*)(_imp->cacheFile->file->data() + _imp->cacheFileDataOffset);
}

char*
MemoryMappedCacheEntry::getData()
{
    if (!_imp->cacheFile) {
        return 0;
    }
    return (char*)(_imp->cacheFile->file->data() + _imp->cacheFileDataOffset);
}

std::size_t
MemoryMappedCacheEntry::getOffsetInFile() const
{
    return _imp->cacheFileDataOffset;
}

std::string
MemoryMappedCacheEntry::getCacheFileAbsolutePath() const
{
    return _imp->cacheFile->file->path();
}


void
MemoryMappedCacheEntry::syncBackingFile()
{
    if (!_imp->cacheFile) {
        return;
    }

    _imp->cacheFile->file->flush(MemoryFile::eFlushTypeAsync, _imp->cacheFile->file->data() + _imp->cacheFileDataOffset, getSize());
}


void
MemoryMappedCacheEntry::allocateMemoryImpl(const AllocateMemoryArgs& args)
{
    const MMAPAllocateMemoryArgs* mmapArgs = dynamic_cast<const MMAPAllocateMemoryArgs*>(&args);
    assert(mmapArgs);
    
    CachePtr cache = getCache();
    if (!cache) {
        throw std::bad_alloc();
    }

    if (!mmapArgs->cacheFilePath.empty()) {
        // Check that the provided file exists
        if (!Cache::fileExists(mmapArgs->cacheFilePath)) {
            throw std::bad_alloc();
        }
        try {
            _imp->cacheFile = cache->getTileCacheFile(mmapArgs->cacheFilePath, mmapArgs->cacheFileDataOffset);
        } catch (...) {
            throw std::bad_alloc();
        }
        if (!_imp->cacheFile) {
            throw std::bad_alloc();
        }
        _imp->cacheFileDataOffset = mmapArgs->cacheFileDataOffset;
    } else {
        try {
            cache->allocTile(&_imp->cacheFileDataOffset);
        } catch (...) {
            throw std::bad_alloc();
        }
    }

}

void
MemoryMappedCacheEntry::deallocateMemoryImpl()
{
    CachePtr cache = getCache();
    if (!cache) {
        return;
    }
    
    cache->freeTile(_imp->cacheFile, _imp->cacheFileDataOffset);
}


struct RAMCacheEntryPrivate
{

    // Set if externalBuffer is not set
    boost::scoped_ptr<RamBuffer<char> > buffer;

    // Set if buffer is not set
    void* externalBuffer;
    std::size_t externalBufferSize;
    RAMAllocateMemoryArgs::ExternalBufferFreeFunction externalBufferFreeFunc;

    ImageBitDepthEnum bitDepth;
    std::size_t numComps;

    RectI bounds;

    RAMCacheEntryPrivate()
    : buffer()
    , externalBuffer(0)
    , externalBufferSize(0)
    , externalBufferFreeFunc(0)
    , bitDepth(eImageBitDepthFloat)
    , numComps(1)
    , bounds()
    {

    }
};


RAMCacheEntry::RAMCacheEntry(const CachePtr& cache)
: MemoryBufferedCacheEntryBase(cache)
, _imp(new RAMCacheEntryPrivate())
{

}

RAMCacheEntry::~RAMCacheEntry()
{

}

RectI
RAMCacheEntry::getBounds() const
{
    return _imp->bounds;
}

std::size_t
RAMCacheEntry::getNumComponents() const
{
    return _imp->numComps;
}

std::size_t
RAMCacheEntry::getRowSize() const
{
    return _imp->bounds.width() * _imp->numComps * getSizeOfForBitDepth(_imp->bitDepth);
}

StorageModeEnum
RAMCacheEntry::getStorageMode() const
{
    return eStorageModeRAM;
}

std::size_t
RAMCacheEntry::getSize() const
{
    if (_imp->buffer) {
        return _imp->buffer->size();
    } else if (_imp->externalBuffer) {
        return _imp->externalBufferSize;
    } else {
        return 0;
    }

}

const char*
RAMCacheEntry::getData() const
{
    if (_imp->buffer) {
        return _imp->buffer->getData();
    } else if (_imp->externalBuffer) {
        return (const char*)_imp->externalBuffer;
    } else {
        return 0;
    }
}

char*
RAMCacheEntry::getData()
{
    if (_imp->buffer) {
        return _imp->buffer->getData();
    } else if (_imp->externalBuffer) {
        return (char*)_imp->externalBuffer;
    } else {
        return 0;
    }

}


void
RAMCacheEntry::allocateMemoryImpl(const AllocateMemoryArgs& args)
{
    const RAMAllocateMemoryArgs* ramArgs = dynamic_cast<const RAMAllocateMemoryArgs*>(&args);
    assert(ramArgs);

    assert(!_imp->buffer && !_imp->externalBuffer);

    _imp->externalBuffer = ramArgs->externalBuffer;
    _imp->bitDepth = ramArgs->bitDepth;
    _imp->numComps = ramArgs->numComponents;
    _imp->externalBufferSize = ramArgs->externalBufferSize;
    _imp->externalBufferFreeFunc = ramArgs->externalBufferFreeFunc;

    assert(!_imp->externalBuffer || _imp->externalBufferFreeFunc);

    if (!_imp->externalBuffer) {
        _imp->buffer.reset(new RamBuffer<char>);
        std::size_t nBytes = getSizeOfForBitDepth(_imp->bitDepth) * _imp->numComps;

        nBytes *= ramArgs->bounds.width();
        nBytes *= ramArgs->bounds.height();

        _imp->buffer->resize(nBytes);
    }
}

void
RAMCacheEntry::deallocateMemoryImpl()
{
    if (_imp->buffer) {
        _imp->buffer.reset();
    } else if (_imp->externalBuffer) {
        if (_imp->externalBufferFreeFunc) {
            // Call the user provided delete func
            _imp->externalBufferFreeFunc(_imp->externalBuffer);
        }
    }
}

struct GLCacheEntryPrivate
{

    OSGLContextWPtr glContext;
    boost::scoped_ptr<Texture> texture;

    GLCacheEntryPrivate()
    : glContext()
    , texture()
    {

    }
};


GLCacheEntry::GLCacheEntry(const CachePtr& cache)
: MemoryBufferedCacheEntryBase(cache)
, _imp(new GLCacheEntryPrivate())
{

}

GLCacheEntry::~GLCacheEntry()
{

}

OSGLContextPtr
GLCacheEntry::getOpenGLContext() const
{
    return _imp->glContext.lock();
}

RectI
GLCacheEntry::getBounds() const
{
    if (!_imp->texture) {
        return RectI();
    }
    return _imp->texture->getBounds();
}

StorageModeEnum
GLCacheEntry::getStorageMode() const
{
    return eStorageModeGLTex;
}

std::size_t
GLCacheEntry::getSize() const
{
    if (!_imp->texture) {
        return 0;
    }
    return _imp->texture->getSize();
}

void
GLCacheEntry::allocateMemoryImpl(const AllocateMemoryArgs& args)
{
    const GLAllocateMemoryArgs* glArgs = dynamic_cast<const GLAllocateMemoryArgs*>(&args);
    assert(glArgs);

    assert(!_imp->texture);

    // We are about to make GL calls, ensure that the context is current.
    assert(glArgs->glContext->getCurrentThread() == QThread::currentThread());

    _imp->glContext = glArgs->glContext;


    int glType, internalFormat;
    int format = GL_RGBA;

    // For now, only use RGBA fp OpenGL textures, let glReadPixels do the conversion for us
    internalFormat = GL_RGBA32F_ARB;
    glType = GL_FLOAT;


    _imp->texture.reset( new Texture(glArgs->textureTarget,
                                     GL_NONE,
                                     GL_NONE,
                                     GL_NONE,
                                     eImageBitDepthFloat,
                                     format,
                                     internalFormat,
                                     glType,
                                     glArgs->glContext->isGPUContext() /*useOpenGL*/) );


    // This calls glTexImage2D and allocates a RGBA image
    _imp->texture->ensureTextureHasSize(glArgs->bounds, 0);
}

void
GLCacheEntry::deallocateMemoryImpl()
{
    assert(_imp->texture);

    // This will make current and OpenGL context which may not be the current context used during a render
    // hence save the current context.
    OSGLContextSaver saveCurrentContext;

    OSGLContextPtr glContext = _imp->glContext.lock();

    // The context must still be alive while textures are created in this context!
    assert(glContext);
    if (glContext) {
        // Ensure the context is current to the thread
        OSGLContextAttacherPtr attacher = OSGLContextAttacher::create(glContext);
        attacher->attach();
        _imp->texture.reset();
    }

}

U32
GLCacheEntry::getGLTextureID() const
{
    return _imp->texture ? _imp->texture->getTexID() : 0;
}

int
GLCacheEntry::getGLTextureTarget() const
{
    return _imp->texture ? _imp->texture->getTexTarget() : 0;
}

int
GLCacheEntry::getGLTextureFormat() const
{
    return _imp->texture ? _imp->texture->getFormat() : 0;
}

int
GLCacheEntry::getGLTextureInternalFormat() const
{
    return _imp->texture ? _imp->texture->getInternalFormat() : 0;
}

int
GLCacheEntry::getGLTextureType() const
{
    return _imp->texture ? _imp->texture->getGLType() : 0;
}


NATRON_NAMESPACE_EXIT;