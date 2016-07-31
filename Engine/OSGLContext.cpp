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
#include "Engine/GPUContextPool.h"

#include "Global/GLIncludes.h"

NATRON_NAMESPACE_ENTER;


static  const char* fillConstant_FragmentShader =
"uniform vec4 fillColor;\n"
"\n"
"void main() {\n"
"	gl_FragColor = fillColor;\n"
"}";

static  const char* applyMaskMix_FragmentShader =
"uniform sampler2D originalImageTex;\n"
"uniform sampler2D outputImageTex;\n"
"uniform sampler2D maskImageTex;\n"
"uniform float mixValue;\n"
"\n"
"void main() {\n"
"   vec4 srcColor = texture2D(originalImageTex,gl_TexCoord[0].st);\n"
"   vec4 dstColor = texture2D(outputImageTex,gl_TexCoord[0].st);\n"
"   float alpha;\n"
"#ifdef MASK_ENABLED \n"
"       vec4 maskColor = texture2D(maskImageTex,gl_TexCoord[0].st);\n"
"#ifdef MASK_INVERT \n"
"       maskColor.a = -maskColor.a;\n"
"#endif"
"       alpha = mixValue * maskColor.a;\n"
"#else\n"
"       alpha = mixValue;\n"
"#endif\n"
"   gl_FragColor = dstColor * alpha + (1.0 - alpha) * srcColor;\n"
"}";

static const char* copyUnprocessedChannels_FragmentShader =
"uniform sampler2D originalImageTex;\n"
"uniform sampler2D outputImageTex;\n"
"\n"
"void main() {\n"
"   vec4 srcColor = texture2D(originalImageTex,gl_TexCoord[0].st);\n"
"   vec4 dstColor = texture2D(outputImageTex,gl_TexCoord[0].st);\n"
"#ifdef DO_R\n"
"       gl_FragColor.r = srcColor.r;\n"
"#else\n"
"       gl_FragColor.r = dstColor.r;\n"
"#endif\n"
"#ifdef DO_G\n"
"       gl_FragColor.g = srcColor.g;\n"
"#else\n"
"       gl_FragColor.g = dstColor.g;\n"
"#endif\n"
"#ifdef DO_B\n"
"       gl_FragColor.b = srcColor.b;\n"
"#else\n"
"       gl_FragColor.b = dstColor.b;\n"
"#endif\n"
"#ifdef DO_A\n"
"       gl_FragColor.a = srcColor.a;\n"
"#else\n"
"       gl_FragColor.a = dstColor.a;\n"
"#endif\n"
"}";

static  const char* copyTex_FragmentShader =
"uniform sampler2D srcTex;\n"
"void main() {\n"
"   gl_FragColor = texture2D(srcTex,gl_TexCoord[0].st);\n"
"}";



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

    boost::shared_ptr<GLShaderBase> fillImageShader;
    boost::shared_ptr<GLShaderBase> copyTexShader;
    std::vector<boost::shared_ptr<GLShaderBase> > applyMaskMixShader;
    std::vector<boost::shared_ptr<GLShaderBase> > copyUnprocessedChannelsShader;

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
        , fillImageShader()
        , applyMaskMixShader(4)
        , copyUnprocessedChannelsShader(16)
    {

    }


};

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

int
OSGLContext::getMaxOpenGLWidth()
{
    static int maxCPUWidth = 0;
    static int maxGPUWidth = 0;
    if (_imp->useGPUContext) {
        if (maxGPUWidth == 0) {
            setContextCurrentNoRender();
            GL_GPU::glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxGPUWidth);
        }
        return maxGPUWidth;
    } else {
#ifdef HAVE_OSMESA
        if (maxCPUWidth == 0) {
            setContextCurrentNoRender();
            GL_CPU::glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxCPUWidth);
            int osmesaMaxWidth = OSGLContext_osmesa::getMaxWidth();
            maxCPUWidth = std::min(maxCPUWidth, osmesaMaxWidth);
        }
#endif
        return maxCPUWidth;
    }
}

int
OSGLContext::getMaxOpenGLHeight()
{
    static int maxCPUHeight = 0;
    static int maxGPUHeight = 0;
    if (_imp->useGPUContext) {
        if (maxGPUHeight == 0) {
            setContextCurrentNoRender();
            GL_GPU::glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxGPUHeight);
        }
        return maxGPUHeight;
    } else {
#ifdef HAVE_OSMESA
        if (maxCPUHeight == 0) {
            setContextCurrentNoRender();
            GL_CPU::glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxCPUHeight);
            int osmesaMaxHeight = OSGLContext_osmesa::getMaxHeight();
            maxCPUHeight = std::min(maxCPUHeight, osmesaMaxHeight);
        }
#endif
        return maxCPUHeight;
    }
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


void
OSGLContext::setContextCurrentNoRender(int width, int height, int rowWidth, void* buffer)
{

    if (_imp->useGPUContext) {
#     ifdef __NATRON_WIN32__
        OSGLContext_win::makeContextCurrent( _imp->_platformContext.get() );
#     elif defined(__NATRON_OSX__)
        OSGLContext_mac::makeContextCurrent( _imp->_platformContext.get() );
#     elif defined(__NATRON_LINUX__)
        OSGLContext_x11::makeContextCurrent( _imp->_platformContext.get() );
#     endif
    } else {
#     ifdef HAVE_OSMESA
        if (buffer) {
            OSGLContext_osmesa::makeContextCurrent(_imp->_osmesaContext.get(), GL_FLOAT, width, height, rowWidth, buffer);
        } else {
            // Make the context current with a stub buffer
            OSGLContext_osmesa::makeContextCurrent(_imp->_osmesaContext.get(), GL_FLOAT, 1, 1, 1, &_imp->_osmesaStubBuffer[0]);
        }
#     else
        assert(false);
        Q_UNUSED(width);
        Q_UNUSED(height);
        Q_UNUSED(rowWidth);
        Q_UNUSED(buffer);
#     endif
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
    setContextCurrentInternal(abortInfo, frameTime, 0, 0, 0, 0);
}

void
OSGLContext::setContextCurrent_CPU(const AbortableRenderInfoPtr& abortInfo
#ifdef DEBUG
                                   , double frameTime
#endif
                                   , int width
                                   , int height
                                   , int rowWidth
                                   , void* buffer)
{
    setContextCurrentInternal(abortInfo, frameTime, width, height, rowWidth, buffer);
}

void
OSGLContext::setContextCurrentInternal(const AbortableRenderInfoPtr& abortInfo
#ifdef DEBUG
                                       , double frameTime
#endif
                                       , int width
                                       , int height
                                       , int rowWidth
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

    setContextCurrentNoRender(width, height, rowWidth, buffer);

}

void
OSGLContext::unsetCurrentContextNoRenderInternal(bool useGPU, const Natron::OSGLContext* context)
{
    if (useGPU) {
#     ifdef __NATRON_WIN32__
        OSGLContext_win::makeContextCurrent(0);
#     elif defined(__NATRON_OSX__)
        OSGLContext_mac::makeContextCurrent(0);
#     elif defined(__NATRON_LINUX__)
        OSGLContext_x11::makeContextCurrent(0);
#     endif
    } else {
#     ifdef HAVE_OSMESA
        OSGLContext_osmesa::unSetContext(context ? context->_imp->_osmesaContext.get() : 0);
#     else
        assert(false);
        Q_UNUSED(context);
#     endif
    }
}

void
OSGLContext::unsetCurrentContextNoRender(bool useGPU)
{
    unsetCurrentContextNoRenderInternal(useGPU, this);
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


template <typename GL>
static boost::shared_ptr<GLShader<GL> >
getOrCreateFillShaderInternal()
{

    boost::shared_ptr<GLShader<GL> > shader( new GLShader<GL>() );
#ifdef DEBUG
    std::string error;
    bool ok = shader->addShader(GLShader<GL>::eShaderTypeFragment, fillConstant_FragmentShader, &error);
    if (!ok) {
        qDebug() << error.c_str();
    }
#else
    bool ok = shader->addShader(GLShader<GL>::eShaderTypeFragment, fillConstant_FragmentShader, 0);
#endif

    assert(ok);
#ifdef DEBUG
    ok = shader->link(&error);
    if (!ok) {
        qDebug() << error.c_str();
    }
#else
    ok = shader->link();
#endif
    assert(ok);
    Q_UNUSED(ok);

    return shader;
}

template <typename GL>
static boost::shared_ptr<GLShader<GL> >
getOrCreateCopyTexShaderInternal()
{

    boost::shared_ptr<GLShader<GL> > shader( new GLShader<GL>() );
#ifdef DEBUG
    std::string error;
    bool ok = shader->addShader(GLShader<GL>::eShaderTypeFragment, copyTex_FragmentShader, &error);
    if (!ok) {
        qDebug() << error.c_str();
    }
#else
    bool ok = shader->addShader(GLShader<GL>::eShaderTypeFragment, copyTex_FragmentShader, 0);
#endif

    assert(ok);
#ifdef DEBUG
    ok = shader->link(&error);
    if (!ok) {
        qDebug() << error.c_str();
    }
#else
    ok = shader->link();
#endif
    assert(ok);
    Q_UNUSED(ok);

    return shader;
}


template <typename GL>
static boost::shared_ptr<GLShader<GL> >
getOrCreateMaskMixShaderInternal(bool maskEnabled, bool maskInvert)
{
    boost::shared_ptr<GLShader<GL> > shader( new GLShader<GL>() );( new GLShader<GL>() );

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
    bool ok = shader->addShader(GLShader<GL>::eShaderTypeFragment, fragmentSource.c_str(), &error);
    if (!ok) {
        qDebug() << error.c_str();
    }
#else
    bool ok = shader->addShader(GLShader<GL>::eShaderTypeFragment, fragmentSource.c_str(), 0);
#endif
    assert(ok);
#ifdef DEBUG
    ok = shader->link(&error);
    if (!ok) {
        qDebug() << error.c_str();
    }
#else
    ok = shader->link();
#endif
    assert(ok);
    Q_UNUSED(ok);

    return shader;
}


template <typename GL>
static boost::shared_ptr<GLShader<GL> >
getOrCreateCopyUnprocessedChannelsShaderInternal(bool doR,
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

    boost::shared_ptr<GLShader<GL> > shader( new GLShader<GL>() );( new GLShader<GL>() );( new GLShader<GL>() );

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
    bool ok = shader->addShader(GLShader<GL>::eShaderTypeFragment, fragmentSource.c_str(), &error);
    if (!ok) {
        qDebug() << error.c_str();
    }
#else
    bool ok = shader->addShader(GLShader<GL>::eShaderTypeFragment, fragmentSource.c_str(), 0);
#endif
    assert(ok);
#ifdef DEBUG
    ok = shader->link(&error);
    if (!ok) {
        qDebug() << error.c_str();
    }
#else
    ok = shader->link();
#endif
    assert(ok);
    Q_UNUSED(ok);

    return shader;
} // OSGLContext::getOrCreateCopyUnprocessedChannelsShader


GLShaderBasePtr
OSGLContext::getOrCreateCopyTexShader()
{
    if (_imp->copyTexShader) {
        return _imp->copyTexShader;
    }
    if (_imp->useGPUContext) {
        _imp->copyTexShader = getOrCreateCopyTexShaderInternal<GL_GPU>();
    } else {
        _imp->copyTexShader = getOrCreateCopyTexShaderInternal<GL_CPU>();
    }
    return _imp->copyTexShader;

}

GLShaderBasePtr
OSGLContext::getOrCreateFillShader()
{
    if (_imp->fillImageShader) {
        return _imp->fillImageShader;
    }
    if (_imp->useGPUContext) {
        _imp->fillImageShader = getOrCreateFillShaderInternal<GL_GPU>();
    } else {
        _imp->fillImageShader = getOrCreateFillShaderInternal<GL_CPU>();
    }
    return _imp->fillImageShader;
}

GLShaderBasePtr
OSGLContext::getOrCreateMaskMixShader(bool maskEnabled, bool maskInvert)
{
    int shader_i = int(maskEnabled) << 1 | int(maskInvert);
    if (_imp->applyMaskMixShader[shader_i]) {
        return _imp->applyMaskMixShader[shader_i];
    }
    if (_imp->useGPUContext) {
        _imp->applyMaskMixShader[shader_i] = getOrCreateMaskMixShaderInternal<GL_GPU>(maskEnabled, maskInvert);
    } else {
        _imp->applyMaskMixShader[shader_i] = getOrCreateMaskMixShaderInternal<GL_CPU>(maskEnabled, maskInvert);
    }

    return _imp->applyMaskMixShader[shader_i];
}

GLShaderBasePtr
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

    if (_imp->copyUnprocessedChannelsShader[index]) {
        return _imp->copyUnprocessedChannelsShader[index];
    }
    if (_imp->useGPUContext) {
        _imp->copyUnprocessedChannelsShader[index] = getOrCreateCopyUnprocessedChannelsShaderInternal<GL_GPU>(doR, doG, doB, doA);
    } else {
        _imp->copyUnprocessedChannelsShader[index] = getOrCreateCopyUnprocessedChannelsShaderInternal<GL_CPU>(doR, doG, doB, doA);
    }
    
    return _imp->copyUnprocessedChannelsShader[index];
    
}

NATRON_NAMESPACE_EXIT;
