/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 * Copyright (C) 2018-2020 The Natron developers
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


NATRON_NAMESPACE_ENTER


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
    return bool(_imp->allocArgs);
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
        _imp->allocArgs.reset();
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

    {
        QMutexLocker k(&_imp->allocatedLock);
        _imp->allocated = true;
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

bool
RAMImageStorage::canSoftCopy(const ImageStorageBase& other)
{
    const RAMImageStorage* isRamStorage = dynamic_cast<const RAMImageStorage*>(&other);
    if (isRamStorage) {
        if (!isRamStorage->_imp->buffer.get() || !_imp->buffer.get()) {
            return false;
        }
        return isRamStorage->_imp->buffer->size() == _imp->buffer->size();
    } else {
        return false;
    }
}

void
RAMImageStorage::softCopy(const ImageStorageBase& other)
{
    const RAMImageStorage* isRamStorage = dynamic_cast<const RAMImageStorage*>(&other);
    if (isRamStorage) {
        _imp->buffer.swap(isRamStorage->_imp->buffer);
    } else {
        assert(false);
    }

}

struct GLImageStoragePrivate
{

    OSGLContextWPtr glContext;
    GLTexturePtr texture;

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
    if (_imp->texture) {
        OSGLContextPtr context = _imp->glContext.lock();
        assert(context);
        OSGLContextSaver contextSaver;
        {
            OSGLContextAttacherPtr attacher;
            if (context) {
                attacher = OSGLContextAttacher::create(context);
                attacher->attach();
            }
            _imp->texture.reset();
        }
    }
}

OSGLContextPtr
GLImageStorage::getOpenGLContext() const
{
    return _imp->glContext.lock();
}

GLTexturePtr
GLImageStorage::getTexture() const
{
    return _imp->texture;
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

    OSGLContextAttacherPtr contextLocker = OSGLContextAttacher::create(glArgs->glContext);
    contextLocker->attach();

    // We are about to make GL calls, ensure that the context is current.
    assert(glArgs->glContext->getCurrentThread() == QThread::currentThread());

    _imp->glContext = glArgs->glContext;


    int glType, internalFormat;
    int format = GL_RGBA;

    // For now, only use RGBA fp OpenGL textures, let glReadPixels do the conversion for us
    internalFormat = GL_RGBA32F_ARB;
    glType = GL_FLOAT;


    _imp->texture = boost::make_shared<Texture>(glArgs->textureTarget,
                                                GL_NONE,
                                                GL_NONE,
                                                GL_NONE,
                                                eImageBitDepthFloat,
                                                format,
                                                internalFormat,
                                                glType,
                                                glArgs->glContext->isGPUContext() /*useOpenGL*/);


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




NATRON_NAMESPACE_EXIT
