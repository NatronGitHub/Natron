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


// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "ImageStorage.h"

#include <QMutex>
#include <QThread>
#include <QCoreApplication>

#include "Engine/AppManager.h"
#include "Engine/Cache.h"
#include "Engine/OSGLContext.h"
#include "Engine/RamBuffer.h"
#include "Engine/Texture.h"


NATRON_NAMESPACE_ENTER;


struct ImageStorageBasePrivate
{
    bool allocated;
    QMutex allocatedLock;
    ImageBitDepthEnum bitdepth;
    boost::shared_ptr<AllocateMemoryArgs> allocArgs;

    ImageStorageBasePrivate()
    : allocated(false)
    , allocatedLock()
    , bitdepth()
    , allocArgs()
    {

    }
};

ImageStorageBase::ImageStorageBase()
: _imp(new ImageStorageBasePrivate())
{

}

ImageStorageBase::~ImageStorageBase()
{

}

ImageBitDepthEnum
ImageStorageBase::getBitDepth() const
{
    QMutexLocker k(&_imp->allocatedLock);
    return _imp->bitdepth;
}

bool
ImageStorageBase::isAllocated() const
{
    QMutexLocker k(&_imp->allocatedLock);
    return _imp->allocated;
}

void
ImageStorageBase::setAllocateMemoryArgs(const boost::shared_ptr<AllocateMemoryArgs>& args)
{
    QMutexLocker k(&_imp->allocatedLock);
    if (_imp->allocated) {
        return;
    }
    _imp->allocArgs = args;
}


bool
ImageStorageBase::hasAllocateMemoryArgs() const
{
    QMutexLocker k(&_imp->allocatedLock);
    return _imp->allocArgs;
}

void
ImageStorageBase::allocateMemoryFromSetArgs()
{
    boost::shared_ptr<AllocateMemoryArgs> args;
    {
        QMutexLocker k(&_imp->allocatedLock);
        if (!_imp->allocArgs) {
            return;
        }
        args = _imp->allocArgs;
        if (_imp->allocated) {
            return;
        }
    }
    allocateMemory(*args);
}

void
ImageStorageBase::allocateMemory(const AllocateMemoryArgs& args)
{
    if (isAllocated()) {
        return;
    }

    _imp->bitdepth = args.bitDepth;
    allocateMemoryImpl(args);

    CachePtr cache = appPTR->getCache();
    if (cache) {
        // Notify the cache about memory changes
        std::size_t size = getBufferSize();
        if (size > 0) {
            cache->notifyMemoryAllocated( size, getStorageMode() );
        }
    }
}

void
ImageStorageBase::deallocateMemory()
{
    bool dataAllocated = isAllocated();
    if (!dataAllocated) {
        return;
    }

    deallocateMemoryImpl();

    CachePtr cache = appPTR->getCache();
    if (cache) {
        // Notify the cache about memory changes
        std::size_t size = getBufferSize();
        if (size > 0) {
            cache->notifyMemoryDeallocated( size, getStorageMode() );
        }
    }
}



struct RAMImageStoragePrivate
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

    RAMImageStoragePrivate()
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


RAMImageStorage::RAMImageStorage()
: ImageStorageBase()
, _imp(new RAMImageStoragePrivate())
{

}

RAMImageStorage::~RAMImageStorage()
{

}

RectI
RAMImageStorage::getBounds() const
{
    return _imp->bounds;
}

std::size_t
RAMImageStorage::getNumComponents() const
{
    return _imp->numComps;
}

std::size_t
RAMImageStorage::getRowSize() const
{
    return _imp->bounds.width() * _imp->numComps * getSizeOfForBitDepth(_imp->bitDepth);
}

StorageModeEnum
RAMImageStorage::getStorageMode() const
{
    return eStorageModeRAM;
}

std::size_t
RAMImageStorage::getBufferSize() const
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
RAMImageStorage::getData() const
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
RAMImageStorage::getData()
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
RAMImageStorage::allocateMemoryImpl(const AllocateMemoryArgs& args)
{
    const RAMAllocateMemoryArgs* ramArgs = dynamic_cast<const RAMAllocateMemoryArgs*>(&args);
    assert(ramArgs);

    assert(!_imp->buffer && !_imp->externalBuffer);

    _imp->externalBuffer = ramArgs->externalBuffer;
    _imp->bitDepth = ramArgs->bitDepth;
    _imp->numComps = ramArgs->numComponents;
    _imp->externalBufferSize = ramArgs->externalBufferSize;
    _imp->externalBufferFreeFunc = ramArgs->externalBufferFreeFunc;
    _imp->bounds = ramArgs->bounds;

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
RAMImageStorage::deallocateMemoryImpl()
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

struct GLImageStoragePrivate
{

    OSGLContextWPtr glContext;
    boost::scoped_ptr<Texture> texture;

    GLImageStoragePrivate()
    : glContext()
    , texture()
    {

    }
};


GLImageStorage::GLImageStorage()
: ImageStorageBase()
, _imp(new GLImageStoragePrivate())
{

}

GLImageStorage::~GLImageStorage()
{

}

OSGLContextPtr
GLImageStorage::getOpenGLContext() const
{
    return _imp->glContext.lock();
}

RectI
GLImageStorage::getBounds() const
{
    if (!_imp->texture) {
        return RectI();
    }
    return _imp->texture->getBounds();
}

StorageModeEnum
GLImageStorage::getStorageMode() const
{
    return eStorageModeGLTex;
}

std::size_t
GLImageStorage::getBufferSize() const
{
    if (!_imp->texture) {
        return 0;
    }
    return _imp->texture->getSize();
}

void
GLImageStorage::allocateMemoryImpl(const AllocateMemoryArgs& args)
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
GLImageStorage::deallocateMemoryImpl()
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
GLImageStorage::getGLTextureID() const
{
    return _imp->texture ? _imp->texture->getTexID() : 0;
}

int
GLImageStorage::getGLTextureTarget() const
{
    return _imp->texture ? _imp->texture->getTexTarget() : 0;
}

int
GLImageStorage::getGLTextureFormat() const
{
    return _imp->texture ? _imp->texture->getFormat() : 0;
}

int
GLImageStorage::getGLTextureInternalFormat() const
{
    return _imp->texture ? _imp->texture->getInternalFormat() : 0;
}

int
GLImageStorage::getGLTextureType() const
{
    return _imp->texture ? _imp->texture->getGLType() : 0;
}


struct CacheImageTileStoragePrivate
{


    // Process local buffer: instead of working directly on the memory portion in the memory mapped file
    // returned by the cache, work on a local copy that is copied from/to the cache to avoid locking
    // the cache.
    boost::scoped_ptr<RamBuffer<char> > localBuffer;
    ImageBitDepthEnum bitdepth;

    CacheImageTileStoragePrivate()
    : localBuffer()
    , bitdepth(eImageBitDepthNone)
    {

    }
};

CacheImageTileStorage::CacheImageTileStorage(const CachePtr& cache)
: ImageStorageBase()
, CacheEntryBase(cache)
, _imp(new CacheImageTileStoragePrivate())
{

}

CacheImageTileStorage::~CacheImageTileStorage()
{

}

void
CacheImageTileStorage::toMemorySegment(ExternalSegmentType* segment, const std::string& objectNamesPrefix, ExternalSegmentTypeHandleList* objectPointers, void* tileDataPtr) const
{
    assert(tileDataPtr && _imp->localBuffer);
    memcpy(tileDataPtr, _imp->localBuffer->getData(), NATRON_TILE_SIZE_BYTES);
    CacheEntryBase::toMemorySegment(segment, objectNamesPrefix, objectPointers, tileDataPtr);
}

void
CacheImageTileStorage::fromMemorySegment(ExternalSegmentType* segment, const std::string& objectNamesPrefix, const void* tileDataPtr)
{
    CacheEntryBase::fromMemorySegment(segment, objectNamesPrefix, tileDataPtr);

    // The memory might not have been pre-allocated, but at least AllocateMemoryArgs should have been set if the
    // delayAllocation flag was set in Image::InitStorageArgs
    if (!isAllocated()) {
        assert(hasAllocateMemoryArgs());

        // This may throw a std::bad_alloc
        allocateMemoryFromSetArgs();
    }
    assert(tileDataPtr && _imp->localBuffer);
    memcpy(_imp->localBuffer->getData(), tileDataPtr, NATRON_TILE_SIZE_BYTES);
}

StorageModeEnum
CacheImageTileStorage::getStorageMode() const
{
    return eStorageModeDisk;
}

std::size_t
CacheImageTileStorage::getBufferSize() const
{
    return NATRON_TILE_SIZE_BYTES;
}

std::size_t
CacheImageTileStorage::getMetadataSize() const
{
    return CacheEntryBase::getMetadataSize();
}

RectI
CacheImageTileStorage::getBounds() const
{
    RectI ret;

    int tileSizeX, tileSizeY;
    Cache::getTileSizePx(getBitDepth(), &tileSizeX, &tileSizeY);
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
CacheImageTileStorage::getRowSize() const
{

    ImageBitDepthEnum bitDepth = getBitDepth();
    int tileSizeX, tileSizeY;
    Cache::getTileSizePx(bitDepth,&tileSizeX, &tileSizeY);

    return tileSizeX * getSizeOfForBitDepth(bitDepth);

}

bool
CacheImageTileStorage::isStorageTiled() const
{
    return true;
}

const char*
CacheImageTileStorage::getData() const
{
    if (!_imp->localBuffer) {
        return 0;
    }

    return _imp->localBuffer->getData();
}

char*
CacheImageTileStorage::getData()
{
    if (!_imp->localBuffer) {
        return 0;
    }
    return _imp->localBuffer->getData();
}


void
CacheImageTileStorage::allocateMemoryImpl(const AllocateMemoryArgs& args)
{
    assert(!_imp->localBuffer);
    _imp->localBuffer.reset(new RamBuffer<char>);
    _imp->localBuffer->resize(NATRON_TILE_SIZE_BYTES);
    _imp->bitdepth = args.bitDepth;
}

void
CacheImageTileStorage::deallocateMemoryImpl()
{
    _imp->localBuffer.reset();
}



NATRON_NAMESPACE_EXIT;
