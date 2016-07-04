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

#include "OSGLContext.h"

#include <stdexcept>
#include <QDebug>
#include <QMutex>
#include <QWaitCondition>

#ifdef __NATRON_WIN32__
#include "Engine/OSGLContext_win.h"
#elif defined(__NATRON_OSX__)
#include "Engine/OSGLContext_mac.h"
#elif defined(__NATRON_LINUX__)
#include "Engine/OSGLContext_x11.h"
#endif

#ifdef HAVE_OSMESA
#include "Engine/OSGLContext_osmesa.h"
#endif

#include "Engine/AppManager.h"
#include "Engine/AbortableRenderInfo.h"
#include "Engine/DefaultShaders.h"
#include "Engine/GPUContextPool.h"

#include "Global/GLIncludes.h"

NATRON_NAMESPACE_ENTER;


const FramebufferConfig&
OSGLContext::chooseFBConfig(const FramebufferConfig& desired,
                            const std::vector<FramebufferConfig>& alternatives,
                            int count)
{
    if ( alternatives.empty() || ( count > (int)alternatives.size() ) ) {
        throw std::logic_error("alternatives empty");
    }
    unsigned int missing, leastMissing = UINT_MAX;
    unsigned int colorDiff, leastColorDiff = UINT_MAX;
    unsigned int extraDiff, leastExtraDiff = UINT_MAX;
    int closest = -1;

    for (int i = 0; i < count; ++i) {
        const FramebufferConfig& current = alternatives[i];

        if ( (desired.stereo > 0) && (current.stereo == 0) ) {
            // Stereo is a hard constraint
            continue;
        }

        if (desired.doublebuffer != current.doublebuffer) {
            // Double buffering is a hard constraint
            continue;
        }

        // Count number of missing buffers
        {
            missing = 0;

            if ( (desired.alphaBits > 0) && (current.alphaBits == 0) ) {
                missing++;
            }

            if ( (desired.depthBits > 0) && (current.depthBits == 0) ) {
                missing++;
            }

            if ( (desired.stencilBits > 0) && (current.stencilBits == 0) ) {
                missing++;
            }

            if ( (desired.auxBuffers > 0) &&
                 ( current.auxBuffers < desired.auxBuffers) ) {
                missing += desired.auxBuffers - current.auxBuffers;
            }

            if ( (desired.samples > 0) && (current.samples == 0) ) {
                // Technically, several multisampling buffers could be
                // involved, but that's a lower level implementation detail and
                // not important to us here, so we count them as one
                missing++;
            }
        }

        // These polynomials make many small channel size differences matter
        // less than one large channel size difference

        // Calculate color channel size difference value
        {
            colorDiff = 0;

            if (desired.redBits != FramebufferConfig::ATTR_DONT_CARE) {
                colorDiff += (desired.redBits - current.redBits) *
                             (desired.redBits - current.redBits);
            }

            if (desired.greenBits != FramebufferConfig::ATTR_DONT_CARE) {
                colorDiff += (desired.greenBits - current.greenBits) *
                             (desired.greenBits - current.greenBits);
            }

            if (desired.blueBits != FramebufferConfig::ATTR_DONT_CARE) {
                colorDiff += (desired.blueBits - current.blueBits) *
                             (desired.blueBits - current.blueBits);
            }
        }

        // Calculate non-color channel size difference value
        {
            extraDiff = 0;

            if (desired.alphaBits != FramebufferConfig::ATTR_DONT_CARE) {
                extraDiff += (desired.alphaBits - current.alphaBits) *
                             (desired.alphaBits - current.alphaBits);
            }

            if (desired.depthBits != FramebufferConfig::ATTR_DONT_CARE) {
                extraDiff += (desired.depthBits - current.depthBits) *
                             (desired.depthBits - current.depthBits);
            }

            if (desired.stencilBits != FramebufferConfig::ATTR_DONT_CARE) {
                extraDiff += (desired.stencilBits - current.stencilBits) *
                             (desired.stencilBits - current.stencilBits);
            }

            if (desired.accumRedBits != FramebufferConfig::ATTR_DONT_CARE) {
                extraDiff += (desired.accumRedBits - current.accumRedBits) *
                             (desired.accumRedBits - current.accumRedBits);
            }

            if (desired.accumGreenBits != FramebufferConfig::ATTR_DONT_CARE) {
                extraDiff += (desired.accumGreenBits - current.accumGreenBits) *
                             (desired.accumGreenBits - current.accumGreenBits);
            }

            if (desired.accumBlueBits != FramebufferConfig::ATTR_DONT_CARE) {
                extraDiff += (desired.accumBlueBits - current.accumBlueBits) *
                             (desired.accumBlueBits - current.accumBlueBits);
            }

            if (desired.accumAlphaBits != FramebufferConfig::ATTR_DONT_CARE) {
                extraDiff += (desired.accumAlphaBits - current.accumAlphaBits) *
                             (desired.accumAlphaBits - current.accumAlphaBits);
            }

            if (desired.samples != FramebufferConfig::ATTR_DONT_CARE) {
                extraDiff += (desired.samples - current.samples) *
                             (desired.samples - current.samples);
            }

            if (desired.sRGB && !current.sRGB) {
                extraDiff++;
            }
        }

        // Figure out if the current one is better than the best one found so far
        // Least number of missing buffers is the most important heuristic,
        // then color buffer size match and lastly size match for other buffers

        if (missing < leastMissing) {
            closest = i;
        } else if (missing == leastMissing) {
            if ( (colorDiff < leastColorDiff) || ( (colorDiff == leastColorDiff) && (extraDiff < leastExtraDiff) ) ) {
                closest = i;
            }
        }

        if ( (int)i == closest ) {
            leastMissing = missing;
            leastColorDiff = colorDiff;
            leastExtraDiff = extraDiff;
        }
    } // for alternatives
    if (closest == -1) {
        throw std::logic_error("closest = -1");
    }

    return alternatives[closest];
} // chooseFBConfig

template <typename GL>
class ShadersContainer
{
public:

    boost::shared_ptr<GLShader<GL> > fillImageShader;

    boost::shared_ptr<GLShader<GL> > applyMaskMixShader[4];
    boost::shared_ptr<GLShader<GL> > copyUnprocessedChannelsShader[16];

    boost::shared_ptr<GLShader<GL> > rampShader[5];

};

struct OSGLContextPrivate
{

    bool useGPUContext;

#ifdef __NATRON_WIN32__
    boost::scoped_ptr<OSGLContext_win> _platformContext;
#elif defined(__NATRON_OSX__)
    boost::scoped_ptr<OSGLContext_mac> _platformContext;
#elif defined(__NATRON_LINUX__)
    boost::scoped_ptr<OSGLContext_x11> _platformContext;
#endif

#ifdef HAVE_OSMESA
    boost::scoped_ptr<OSGLContext_osmesa> _osmesaContext;

    // Used when we want to make the osmesa context current not for rendering (e.g: just to call functions like glGetInteger, etc...) so the context
    // does not fail
    std::vector<float> _osmesaStubBuffer;
#endif

    // When we use a single GL context, renders are all sharing the same context and lock it when trying to render
    QMutex renderOwningContextMutex;
    QWaitCondition renderOwningContextCond;
    AbortableRenderInfoPtr renderOwningContext;
    int renderOwningContextCount;
#ifdef DEBUG
    double renderOwningContextFrameTime;
#endif

    // This pbo is used to call glTexSubImage2D and glReadPixels asynchronously
    unsigned int pboID;

    // The main FBO onto which we do all renders
    unsigned int fboID;

    // A vbo used to upload vertices (for now only used in Roto)
    unsigned int vboVerticesID;

    // a vbo used to upload vertices colors (for now only used in Roto)
    unsigned int vboColorsID;

    // an ibo used to call glDrawElements (for now only used in Roto)
    unsigned int iboID;

    boost::scoped_ptr<ShadersContainer<GL_GPU> > gpuShaders;
    boost::scoped_ptr<ShadersContainer<GL_CPU> > cpuShaders;

    OSGLContextPrivate(bool useGPUContext)
        : useGPUContext(useGPUContext)
        , _platformContext()
#ifdef HAVE_OSMESA
        , _osmesaContext()
        , _osmesaStubBuffer()
#endif
        , renderOwningContextMutex()
        , renderOwningContextCond()
        , renderOwningContext()
        , renderOwningContextCount(0)
#ifdef DEBUG
        , renderOwningContextFrameTime(0)
#endif
        , pboID(0)
        , fboID(0)
        , vboVerticesID(0)
        , vboColorsID(0)
        , iboID(0)
        , gpuShaders()
        , cpuShaders()
    {
        if (useGPUContext) {
            gpuShaders.reset(new ShadersContainer<GL_GPU>);
        } else {
            cpuShaders.reset(new ShadersContainer<GL_CPU>);
        }

    }

    template <typename GL> ShadersContainer<GL>& getShadersContainer() const;


};


template <>
ShadersContainer<GL_GPU>&
OSGLContextPrivate::getShadersContainer() const
{
    return *gpuShaders;

}

template <>
ShadersContainer<GL_CPU>&
OSGLContextPrivate::getShadersContainer() const
{
    return *cpuShaders;
}

OSGLContext::OSGLContext(const FramebufferConfig& pixelFormatAttrs,
                         const OSGLContext* shareContext,
                         bool useGPUContext,
                         int major,
                         int minor,
                         const GLRendererID &rendererID,
                         bool coreProfile)
    : _imp( new OSGLContextPrivate(useGPUContext) )
{



    if (major == -1) {
        major = appPTR->getOpenGLVersionMajor();
    }
    if (minor == -1) {
        minor = appPTR->getOpenGLVersionMinor();
    }

    if (coreProfile) {
        // Don't bother with core profile with OpenGL < 3
        if (major < 3) {
            coreProfile = false;
        }
    }

    if (useGPUContext) {
#ifdef __NATRON_WIN32__
        _imp->_platformContext.reset( new OSGLContext_win(pixelFormatAttrs, major, minor, coreProfile, rendererID, shareContext ? shareContext->_imp->_platformContext.get() : 0) );
#elif defined(__NATRON_OSX__)
        _imp->_platformContext.reset( new OSGLContext_mac(pixelFormatAttrs, major, minor, coreProfile, rendererID, shareContext ? shareContext->_imp->_platformContext.get() : 0) );
#elif defined(__NATRON_LINUX__)
        _imp->_platformContext.reset( new OSGLContext_x11(pixelFormatAttrs, major, minor, coreProfile, rendererID, shareContext ? shareContext->_imp->_platformContext.get() : 0) );
#endif
    } else {
#ifdef HAVE_OSMESA
        _imp->_osmesaContext.reset( new OSGLContext_osmesa(pixelFormatAttrs, major, minor, coreProfile, rendererID, shareContext ? shareContext->_imp->_osmesaContext.get() : 0) );
        _imp->_osmesaStubBuffer.resize(4);
#endif
    }
}

OSGLContext::~OSGLContext()
{
    setContextCurrentNoRender();
    if (_imp->pboID) {
        if (_imp->useGPUContext) {
            GL_GPU::glDeleteBuffers(1, &_imp->pboID);
        } else {
            GL_CPU::glDeleteBuffers(1, &_imp->pboID);
        }
    }
    if (_imp->fboID) {
        if (_imp->useGPUContext) {
            GL_GPU::glDeleteFramebuffers(1, &_imp->fboID);
        } else {
            GL_CPU::glDeleteFramebuffers(1, &_imp->fboID);
        }
    }

    if (_imp->vboVerticesID) {
        if (_imp->useGPUContext) {
            GL_GPU::glDeleteBuffers(1, &_imp->vboVerticesID);
        } else {
            GL_CPU::glDeleteBuffers(1, &_imp->vboVerticesID);
        }
    }

    if (_imp->vboColorsID) {
        if (_imp->useGPUContext) {
            GL_GPU::glDeleteBuffers(1, &_imp->vboColorsID);
        } else {
            GL_CPU::glDeleteBuffers(1, &_imp->vboColorsID);
        }
    }

    if (_imp->iboID) {
        if (_imp->useGPUContext) {
            GL_GPU::glDeleteBuffers(1, &_imp->iboID);
        } else {
            GL_CPU::glDeleteBuffers(1, &_imp->iboID);
        }
    }
}

void
OSGLContext::checkOpenGLVersion(bool gpuAPI)
{

    const char *verstr;
    if (gpuAPI) {
        verstr = (const char *) GL_GPU::glGetString(GL_VERSION);
    } else {
        verstr = (const char *) GL_CPU::glGetString(GL_VERSION);
    }
    int major, minor;

    if ( (verstr == NULL) || (std::sscanf(verstr, "%d.%d", &major, &minor) != 2) ) {
        major = minor = 0;
        //fprintf(stderr, "Invalid GL_VERSION format!!!\n");
    }

    if ( (major < appPTR->getOpenGLVersionMajor()) || ( (major == appPTR->getOpenGLVersionMajor()) && (minor < appPTR->getOpenGLVersionMinor()) ) ) {
        std::stringstream ss;
        ss << NATRON_APPLICATION_NAME << " requires at least OpenGL " << appPTR->getOpenGLVersionMajor() << "." << appPTR->getOpenGLVersionMinor() << "to perform OpenGL accelerated rendering." << std::endl;
        ss << "Your system only has OpenGL " << major << "." << minor << "available.";
        throw std::runtime_error( ss.str() );
    }
}

bool
OSGLContext::isGPUContext() const
{
    return _imp->useGPUContext;
}

unsigned int
OSGLContext::getOrCreatePBOId()
{
    if (!_imp->pboID) {
        if (_imp->useGPUContext) {
            GL_GPU::glGenBuffers(1, &_imp->pboID);
        } else {
            GL_CPU::glGenBuffers(1, &_imp->pboID);
        }
    }
    return _imp->pboID;
}

unsigned int
OSGLContext::getOrCreateFBOId()
{
    if (!_imp->fboID) {
        if (_imp->useGPUContext) {
            GL_GPU::glGenFramebuffers(1, &_imp->fboID);
        } else {
            GL_CPU::glGenFramebuffers(1, &_imp->fboID);
        }
    }
    return _imp->fboID;
}

unsigned int
OSGLContext::getOrCreateVBOVerticesId()
{
    if (!_imp->vboVerticesID) {
        if (_imp->useGPUContext) {
            GL_GPU::glGenBuffers(1, &_imp->vboVerticesID);
        } else {
            GL_CPU::glGenBuffers(1, &_imp->vboVerticesID);
        }
    }
    return _imp->vboVerticesID;
}

unsigned int
OSGLContext::getOrCreateVBOColorsId()
{
    if (!_imp->vboColorsID) {
        if (_imp->useGPUContext) {
            GL_GPU::glGenBuffers(1, &_imp->vboColorsID);
        } else {
            GL_CPU::glGenBuffers(1, &_imp->vboColorsID);
        }
    }
    return _imp->vboColorsID;
}

unsigned int
OSGLContext::getOrCreateIBOId()
{
    if (!_imp->iboID) {
        if (_imp->useGPUContext) {
            GL_GPU::glGenBuffers(1, &_imp->iboID);
        } else {
            GL_CPU::glGenBuffers(1, &_imp->iboID);
        }
    }
    return _imp->iboID;
}

void
OSGLContext::setContextCurrentNoRender(int width, int height, void* buffer)
{

    if (_imp->useGPUContext) {
#ifdef __NATRON_WIN32__
        OSGLContext_win::makeContextCurrent( _imp->_platformContext.get() );
#elif defined(__NATRON_OSX__)
        OSGLContext_mac::makeContextCurrent( _imp->_platformContext.get() );
#elif defined(__NATRON_LINUX__)
        OSGLContext_x11::makeContextCurrent( _imp->_platformContext.get() );
#endif
    } else {
#ifdef HAVE_OSMESA

        if (buffer) {
            OSGLContext_osmesa::makeContextCurrent(_imp->_osmesaContext.get(), GL_FLOAT, width, height, buffer);
        } else {
            // Make the context current with a stub buffer
            OSGLContext_osmesa::makeContextCurrent(_imp->_osmesaContext.get(), GL_FLOAT, 1, 1, &_imp->_osmesaStubBuffer[0]);
        }
#endif
    }
}

void
OSGLContext::setContextCurrent_GPU(const AbortableRenderInfoPtr& abortInfo
#ifdef DEBUG
                               ,
                               double frameTime
#endif
                               )
{
    setContextCurrentInternal(abortInfo, frameTime, 0, 0, 0);
}

void
OSGLContext::setContextCurrent_CPU(const AbortableRenderInfoPtr& abortInfo
#ifdef DEBUG
                           , double frameTime
#endif
                           , int width
                           , int height
                           , void* buffer)
{
    setContextCurrentInternal(abortInfo, frameTime, width, height, buffer);
}

void
OSGLContext::setContextCurrentInternal(const AbortableRenderInfoPtr& abortInfo
#ifdef DEBUG
                               , double frameTime
#endif
                               , int width
                               , int height
                               , void* buffer)
{
    assert(_imp && (_imp->_platformContext
#ifdef HAVE_OSMESA
                    || _imp->_osmesaContext
#endif
                    ));

    QMutexLocker k(&_imp->renderOwningContextMutex);
    while (_imp->renderOwningContext && _imp->renderOwningContext != abortInfo) {
        _imp->renderOwningContextCond.wait(&_imp->renderOwningContextMutex);
    }
    _imp->renderOwningContext = abortInfo;
#ifdef DEBUG
    if (!_imp->renderOwningContextCount) {
        _imp->renderOwningContextFrameTime = frameTime;
        //qDebug() << "Attaching" << this << "to render frame" << frameTime;
    }
#endif
    ++_imp->renderOwningContextCount;

    setContextCurrentNoRender(width, height, buffer);

}


void
OSGLContext::unsetCurrentContextNoRender(bool useGPU)
{
    if (useGPU) {
#ifdef __NATRON_WIN32__
        OSGLContext_win::makeContextCurrent( 0 );
#elif defined(__NATRON_OSX__)
        OSGLContext_mac::makeContextCurrent( 0 );
#elif defined(__NATRON_LINUX__)
        OSGLContext_x11::makeContextCurrent( 0 );
#endif
    } else {
#ifdef HAVE_OSMESA
        OSGLContext_osmesa::makeContextCurrent(0, 0, 0, 0, 0);
#endif
    }
}

void
OSGLContext::unsetCurrentContext(const AbortableRenderInfoPtr& abortInfo)
{
    QMutexLocker k(&_imp->renderOwningContextMutex);
    if (abortInfo != _imp->renderOwningContext) {
        return;
    }
    --_imp->renderOwningContextCount;
    if (!_imp->renderOwningContextCount) {
#ifdef DEBUG
        //qDebug() << "Dettaching" << this << "from frame" << _imp->renderOwningContextFrameTime;
#endif
        _imp->renderOwningContext.reset();
        unsetCurrentContextNoRender(_imp->useGPUContext);
        //Wake-up only one thread waiting, since each thread that is waiting in setContextCurrent() will actually call this function.
        _imp->renderOwningContextCond.wakeOne();
    }
}


bool
OSGLContext::stringInExtensionString(const char* string,
                                     const char* extensions)
{
    const char* start = extensions;

    for (;; ) {
        const char* where;
        const char* terminator;

        where = strstr(start, string);
        if (!where) {
            return false;
        }

        terminator = where + strlen(string);
        if ( (where == start) || (*(where - 1) == ' ') ) {
            if ( (*terminator == ' ') || (*terminator == '\0') ) {
                break;
            }
        }

        start = terminator;
    }

    return true;
}

template <typename GL>
boost::shared_ptr<GLShader<GL> >
OSGLContext::getOrCreateFillShader()
{

    ShadersContainer<GL>& shaders = _imp->getShadersContainer<GL>();

    if (shaders.fillImageShader) {
        return shaders.fillImageShader;
    }
    shaders.fillImageShader.reset( new GLShader<GL>() );
#ifdef DEBUG
    std::string error;
    bool ok = shaders.fillImageShader->addShader(GLShader<GL>::eShaderTypeFragment, fillConstant_FragmentShader, &error);
    if (!ok) {
        qDebug() << error.c_str();
    }
#else
    bool ok = shaders.fillImageShader->addShader(GLShader<GL>::eShaderTypeFragment, fillConstant_FragmentShader, 0);
#endif

    assert(ok);
#ifdef DEBUG
    ok = shaders.fillImageShader->link(&error);
    if (!ok) {
        qDebug() << error.c_str();
    }
#else
    ok = shaders.fillImageShader->link();
#endif
    assert(ok);
    Q_UNUSED(ok);

    return shaders.fillImageShader;
}

template boost::shared_ptr<GLShader<GL_GPU> > OSGLContext::getOrCreateFillShader<GL_GPU>();
template boost::shared_ptr<GLShader<GL_CPU> > OSGLContext::getOrCreateFillShader<GL_CPU>();

template <typename GL>
boost::shared_ptr<GLShader<GL> >
OSGLContext::getOrCreateMaskMixShader(bool maskEnabled, bool maskInvert)
{
    int shader_i = int(maskEnabled) << 1 | int(maskInvert);

    ShadersContainer<GL>& shaders = _imp->getShadersContainer<GL>();
    if (shaders.applyMaskMixShader[shader_i]) {
        return shaders.applyMaskMixShader[shader_i];
    }
    shaders.applyMaskMixShader[shader_i].reset( new GLShader<GL>() );

    std::string fragmentSource;
    if (maskEnabled) {
        fragmentSource += "#define MASK_ENABLED\n";
    }
    if (maskInvert) {
        fragmentSource += "#define MASK_INVERT\n";
    }

    fragmentSource += std::string(applyMaskMix_FragmentShader);

#ifdef DEBUG
    std::string error;
    bool ok = shaders.applyMaskMixShader[shader_i]->addShader(GLShader<GL>::eShaderTypeFragment, fragmentSource.c_str(), &error);
    if (!ok) {
        qDebug() << error.c_str();
    }
#else
    bool ok = shaders.applyMaskMixShader[shader_i]->addShader(GLShader<GL>::eShaderTypeFragment, fragmentSource.c_str(), 0);
#endif
    assert(ok);
#ifdef DEBUG
    ok = shaders.applyMaskMixShader[shader_i]->link(&error);
    if (!ok) {
        qDebug() << error.c_str();
    }
#else
    ok = shaders.applyMaskMixShader[shader_i]->link();
#endif
    assert(ok);
    Q_UNUSED(ok);

    return shaders.applyMaskMixShader[shader_i];
}

template boost::shared_ptr<GLShader<GL_GPU> > OSGLContext::getOrCreateMaskMixShader<GL_GPU>(bool,bool);
template boost::shared_ptr<GLShader<GL_CPU> > OSGLContext::getOrCreateMaskMixShader<GL_CPU>(bool,bool);

template <typename GL>
boost::shared_ptr<GLShader<GL> >
OSGLContext::getOrCreateRampShader(RampTypeEnum type)
{
    int shader_i = (int)type;

    ShadersContainer<GL>& shaders = _imp->getShadersContainer<GL>();
    if (shaders.rampShader[shader_i]) {
        return shaders.rampShader[shader_i];
    }
    shaders.rampShader[shader_i].reset( new GLShader<GL>() );

    std::string fragmentSource;

    switch (type) {
        case eRampTypeLinear:
            break;
        case eRampTypePLinear:
            fragmentSource += "#define RAMP_P_LINEAR\n";
            break;
        case eRampTypeEaseIn:
            fragmentSource += "#define RAMP_EASE_IN\n";
            break;
        case eRampTypeEaseOut:
            fragmentSource += "#define RAMP_EASE_OUT\n";
            break;
        case eRampTypeSmooth:
            fragmentSource += "#define RAMP_SMOOTH\n";
            break;
    }


    fragmentSource += std::string(rotoRamp_FragmentShader);

#ifdef DEBUG
    std::string error;
    bool ok = shaders.rampShader[shader_i]->addShader(GLShader<GL>::eShaderTypeFragment, fragmentSource.c_str(), &error);
    if (!ok) {
        qDebug() << error.c_str();
    }
#else
    bool ok = shaders.rampShader[shader_i]->addShader(GLShader<GL>::eShaderTypeFragment, fragmentSource.c_str(), 0);
#endif
    assert(ok);
#ifdef DEBUG
    ok = shaders.rampShader[shader_i]->link(&error);
    if (!ok) {
        qDebug() << error.c_str();
    }
#else
    ok = shaders.rampShader[shader_i]->link();
#endif
    assert(ok);
    Q_UNUSED(ok);

    return shaders.rampShader[shader_i];
}

template boost::shared_ptr<GLShader<GL_GPU> > OSGLContext::getOrCreateRampShader<GL_GPU>(RampTypeEnum);
template boost::shared_ptr<GLShader<GL_CPU> > OSGLContext::getOrCreateRampShader<GL_CPU>(RampTypeEnum);


template <typename GL>
boost::shared_ptr<GLShader<GL> >
OSGLContext::getOrCreateCopyUnprocessedChannelsShader(bool doR,
                                                      bool doG,
                                                      bool doB,
                                                      bool doA)
{
    int index = 0x0;

    if (doR) {
        index |= 0x01;
    }
    if (doG) {
        index |= 0x02;
    }
    if (doB) {
        index |= 0x04;
    }
    if (doA) {
        index |= 0x08;
    }

    ShadersContainer<GL>& shaders = _imp->getShadersContainer<GL>();
    if (shaders.copyUnprocessedChannelsShader[index]) {
        return shaders.copyUnprocessedChannelsShader[index];
    }
    shaders.copyUnprocessedChannelsShader[index].reset( new GLShader<GL>() );

    std::string fragmentSource;
    if (!doR) {
        fragmentSource += "#define DO_R\n";
    }
    if (!doG) {
        fragmentSource += "#define DO_G\n";
    }
    if (!doB) {
        fragmentSource += "#define DO_B\n";
    }
    if (!doA) {
        fragmentSource += "#define DO_A\n";
    }
    fragmentSource += std::string(copyUnprocessedChannels_FragmentShader);


#ifdef DEBUG
    std::string error;
    bool ok = shaders.copyUnprocessedChannelsShader[index]->addShader(GLShader<GL>::eShaderTypeFragment, fragmentSource.c_str(), &error);
    if (!ok) {
        qDebug() << error.c_str();
    }
#else
    bool ok = shaders.copyUnprocessedChannelsShader[index]->addShader(GLShader<GL>::eShaderTypeFragment, fragmentSource.c_str(), 0);
#endif
    assert(ok);
#ifdef DEBUG
    ok = shaders.copyUnprocessedChannelsShader[index]->link(&error);
    if (!ok) {
        qDebug() << error.c_str();
    }
#else
    ok = shaders.copyUnprocessedChannelsShader[index]->link();
#endif
    assert(ok);
    Q_UNUSED(ok);

    return shaders.copyUnprocessedChannelsShader[index];
} // OSGLContext::getOrCreateCopyUnprocessedChannelsShader

template boost::shared_ptr<GLShader<GL_GPU> > OSGLContext::getOrCreateCopyUnprocessedChannelsShader<GL_GPU>(bool,bool,bool,bool);
template boost::shared_ptr<GLShader<GL_CPU> > OSGLContext::getOrCreateCopyUnprocessedChannelsShader<GL_CPU>(bool,bool,bool,bool);


void
OSGLContext::getGPUInfos(std::list<OpenGLRendererInfo>& renderers)
{
#ifdef __NATRON_WIN32__
    OSGLContext_win::getGPUInfos(renderers);
#elif defined(__NATRON_OSX__)
    OSGLContext_mac::getGPUInfos(renderers);
#elif defined(__NATRON_LINUX__)
    OSGLContext_x11::getGPUInfos(renderers);
#endif
}

NATRON_NAMESPACE_EXIT;
