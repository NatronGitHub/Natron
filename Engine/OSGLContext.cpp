/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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
#include <sstream> // stringstream
#include <cstring> // strlen

#include <QtCore/QDebug>
#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>
#include <QtCore/QThread>

#include "Engine/OSGLFunctions.h"
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
#include "Engine/GPUContextPool.h"

#include "Global/GLIncludes.h"

NATRON_NAMESPACE_ENTER


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
    QMutex threadOwningContextMutex;
    QWaitCondition threadOwningContextCond;
    int threadOwningContextCount;
    QThread* threadOwningContext;
#ifdef DEBUG
    double renderOwningContextFrameTime;
#endif

    // This pbo is used to call glTexSubImage2D and glReadPixels asynchronously
    unsigned int pboID;

    // The main FBO onto which we do all renders
    unsigned int fboID;

    GLShaderBasePtr fillImageShader;
    GLShaderBasePtr copyTexShader;
    std::vector<GLShaderBasePtr> applyMaskMixShader;
    std::vector<GLShaderBasePtr> copyUnprocessedChannelsShader;

    OSGLContextPrivate(bool useGPUContext)
        : useGPUContext(useGPUContext)
        , _platformContext()
#ifdef HAVE_OSMESA
        , _osmesaContext()
        , _osmesaStubBuffer()
#endif
        , threadOwningContextMutex()
        , threadOwningContextCond()
        , threadOwningContextCount(0)
        , threadOwningContext(0)
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

OSGLContext::OSGLContext(bool useGPUContext)
: _imp(new OSGLContextPrivate(useGPUContext))
{

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
        // scoped_ptr
        _imp->_platformContext.reset( new OSGLContext_win(pixelFormatAttrs, major, minor, coreProfile, rendererID, shareContext ? shareContext->_imp->_platformContext.get() : 0) );
#elif defined(__NATRON_OSX__)
        // scoped_ptr
        _imp->_platformContext.reset( new OSGLContext_mac(pixelFormatAttrs, major, minor, coreProfile, rendererID, shareContext ? shareContext->_imp->_platformContext.get() : 0) );
#elif defined(__NATRON_LINUX__)
        // scoped_ptr
        _imp->_platformContext.reset( new OSGLContext_x11(pixelFormatAttrs, major, minor, coreProfile, rendererID, shareContext ? shareContext->_imp->_platformContext.get() : 0) );
#endif
    } else {
#ifdef HAVE_OSMESA
        // scoped_ptr
        _imp->_osmesaContext.reset( new OSGLContext_osmesa(pixelFormatAttrs, major, minor, coreProfile, rendererID, shareContext ? shareContext->_imp->_osmesaContext.get() : 0) );
        _imp->_osmesaStubBuffer.resize(4);
#else
        throw std::invalid_argument("Cannot create OSMesa context when not compiled with HAVE_OSMESA");
#endif
    }
}

OSGLContext::~OSGLContext()
{
    setContextCurrentInternal(0, 0, 0, 0);

    if (_imp->pboID) {
        if (_imp->useGPUContext) {
            GL_GPU::DeleteBuffers(1, &_imp->pboID);
        } else {
            GL_CPU::DeleteBuffers(1, &_imp->pboID);
        }
    }
    if (_imp->fboID) {
        if (_imp->useGPUContext) {
            GL_GPU::DeleteFramebuffers(1, &_imp->fboID);
        } else {
            GL_CPU::DeleteFramebuffers(1, &_imp->fboID);
        }
    }

}

void
OSGLContext::checkOpenGLVersion(bool gpuAPI)
{

    const char *verstr;
    if (gpuAPI) {
        verstr = (const char *) GL_GPU::GetString(GL_VERSION);
    } else {
        verstr = (const char *) GL_CPU::GetString(GL_VERSION);
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
            OSGLContextSaver contextSaver;
            {
                boost::shared_ptr<OSGLContextAttacher> attacher;
                if (getCurrentThread() != QThread::currentThread()) {
                    attacher = OSGLContextAttacher::create(shared_from_this());
                    attacher->attach();
                }
                GL_GPU::GetIntegerv(GL_MAX_TEXTURE_SIZE, &maxGPUWidth);
            }
        }
        return maxGPUWidth;
    } else {
#ifdef HAVE_OSMESA
        if (maxCPUWidth == 0) {
            OSGLContextSaver contextSaver;
            {
                boost::shared_ptr<OSGLContextAttacher> attacher;
                if (getCurrentThread() != QThread::currentThread()) {
                    attacher = OSGLContextAttacher::create(shared_from_this());
                    attacher->attach();
                }
                GL_CPU::GetIntegerv(GL_MAX_TEXTURE_SIZE, &maxCPUWidth);
                int osmesaMaxWidth = OSGLContext_osmesa::getMaxWidth();
                maxCPUWidth = std::min(maxCPUWidth, osmesaMaxWidth);
            }
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
            OSGLContextSaver contextSaver;
            {
                boost::shared_ptr<OSGLContextAttacher> attacher;
                if (getCurrentThread() != QThread::currentThread()) {
                    attacher = OSGLContextAttacher::create(shared_from_this());
                    attacher->attach();
                }
                GL_GPU::GetIntegerv(GL_MAX_TEXTURE_SIZE, &maxGPUHeight);
            }
        }
        return maxGPUHeight;
    } else {
#ifdef HAVE_OSMESA
        if (maxCPUHeight == 0) {
            OSGLContextSaver contextSaver;
            {
                boost::shared_ptr<OSGLContextAttacher> attacher;
                if (getCurrentThread() != QThread::currentThread()) {
                    attacher = OSGLContextAttacher::create(shared_from_this());
                    attacher->attach();
                }
                GL_CPU::GetIntegerv(GL_MAX_TEXTURE_SIZE, &maxCPUHeight);
                int osmesaMaxHeight = OSGLContext_osmesa::getMaxHeight();
                maxCPUHeight = std::min(maxCPUHeight, osmesaMaxHeight);
            }
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
            GL_GPU::GenBuffers(1, &_imp->pboID);
        } else {
            GL_CPU::GenBuffers(1, &_imp->pboID);
        }
    }
    return _imp->pboID;
}

unsigned int
OSGLContext::getOrCreateFBOId()
{
    if (!_imp->fboID) {
        if (_imp->useGPUContext) {
            GL_GPU::GenFramebuffers(1, &_imp->fboID);
        } else {
            GL_CPU::GenFramebuffers(1, &_imp->fboID);
        }
    }
    return _imp->fboID;
}

void
OSGLContext::makeGPUContextCurrent()
{
    if (_imp->_platformContext
#ifdef HAVE_OSMESA
        || _imp->_osmesaContext
#endif
        ) {

#     ifdef __NATRON_WIN32__
        OSGLContext_win::makeContextCurrent( _imp->_platformContext.get() );
#     elif defined(__NATRON_OSX__)
        OSGLContext_mac::makeContextCurrent( _imp->_platformContext.get() );
#     elif defined(__NATRON_LINUX__)
        OSGLContext_x11::makeContextCurrent( _imp->_platformContext.get() );
#     endif
    }
}

void
OSGLContext::setContextCurrentInternal(int width, int height, int rowWidth, void* buffer)
{


    QThread* curThread = QThread::currentThread();

    QMutexLocker k(&_imp->threadOwningContextMutex);
    while (_imp->threadOwningContext && _imp->threadOwningContext != curThread) {
        _imp->threadOwningContextCond.wait(&_imp->threadOwningContextMutex);
    }
    _imp->threadOwningContext = curThread;

    ++_imp->threadOwningContextCount;

    if (_imp->useGPUContext) {
        makeGPUContextCurrent();
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

static void unsetGPUContextStatic()
{
#     ifdef __NATRON_WIN32__
    OSGLContext_win::makeContextCurrent(0);
#     elif defined(__NATRON_OSX__)
    OSGLContext_mac::makeContextCurrent(0);
#     elif defined(__NATRON_LINUX__)
    OSGLContext_x11::makeContextCurrent(0);
#     endif
}

void
OSGLContext::unsetGPUContext()
{
    unsetGPUContextStatic();
}

void
OSGLContext::unsetCurrentContextNoRenderInternal(bool useGPU, Natron::OSGLContext* context)
{
    if (useGPU) {
        if (context) {
            context->unsetGPUContext();
        } else {
            unsetGPUContextStatic();
        }
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
OSGLContext::unsetCurrentContext()
{
    QThread* curThread = QThread::currentThread();

    QMutexLocker k(&_imp->threadOwningContextMutex);
    if (curThread != _imp->threadOwningContext) {
        qDebug() << "Thread" << curThread << "is trying to call unsetCurrentContext but it does not own the OpenGL context.";
        return;
    }
    --_imp->threadOwningContextCount;
    if (!_imp->threadOwningContextCount) {
        _imp->threadOwningContext = 0;

        unsetCurrentContextNoRenderInternal(_imp->useGPUContext, this);

        // Wake-up only one thread waiting, since each thread that is waiting in setContextCurrent() will actually call this function.
        _imp->threadOwningContextCond.wakeOne();
    }
}

bool
OSGLContext::hasCreatedContext() const
{
#ifdef HAVE_OSMESA
    return bool(_imp->_platformContext) || bool(_imp->_osmesaContext);
#else
    return bool(_imp->_platformContext);
#endif
}

QThread*
OSGLContext::getCurrentThread() const
{
    QMutexLocker k(&_imp->threadOwningContextMutex);
    return _imp->threadOwningContext;
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

        terminator = where + std::strlen(string);
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

    boost::shared_ptr<GLShader<GL> > shader = boost::make_shared<GLShader<GL> >();
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

    boost::shared_ptr<GLShader<GL> > shader = boost::make_shared<GLShader<GL> >();
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
    boost::shared_ptr<GLShader<GL> > shader = boost::make_shared<GLShader<GL> >();

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

    boost::shared_ptr<GLShader<GL> > shader = boost::make_shared<GLShader<GL> >();

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


OSGLContextAttacher::OSGLContextAttacher(const OSGLContextPtr& c)
: _c(c)
, _attached(0)
, _width(0)
, _height(0)
, _rowWidth(0)
, _buffer(0)
, _dettachOnDtor(true)
{
    assert(c);
}

OSGLContextAttacher::OSGLContextAttacher(const OSGLContextPtr& c, int width, int height, int rowWidth, void* buffer)
: _c(c)
, _attached(0)
, _width(width)
, _height(height)
, _rowWidth(rowWidth)
, _buffer(buffer)
, _dettachOnDtor(true)
{
    assert(c && !c->isGPUContext());
}


OSGLContextAttacherPtr
OSGLContextAttacher::create(const OSGLContextPtr& c)
{
    // Check if the thread already has a context attached
    OSGLContextAttacherPtr curAttacher = appPTR->getGPUContextPool()->getThreadLocalContext();
    if (curAttacher) {
        // If the OpenGL context is already attached, use this attacher
        if (curAttacher->getContext() == c) {
            return curAttacher;
        } else {
            // Another context is already bound, dettach it and return a new attacher
            curAttacher->dettach();
        }
    }
    OSGLContextAttacherPtr ret(new OSGLContextAttacher(c));
    return ret;
}

OSGLContextAttacherPtr
OSGLContextAttacher::create(const OSGLContextPtr& c, int width, int height, int rowWidth, void* buffer)
{
    OSGLContextAttacherPtr curAttacher = appPTR->getGPUContextPool()->getThreadLocalContext();
    if (curAttacher) {
        if (curAttacher->getContext() == c) {
            // Deattach and re-attach since width & height may have changed
            if (!curAttacher->_c->isGPUContext() && (curAttacher->_width != width || curAttacher->_height != height || curAttacher->_rowWidth != rowWidth || curAttacher->_buffer != buffer)) {
                curAttacher->_attached = 0;
                curAttacher->_c->unsetCurrentContext();
                curAttacher->_width = width;
                curAttacher->_height = height;
                curAttacher->_rowWidth = rowWidth;
                curAttacher->_buffer = buffer;
            }
            return curAttacher;
        } else {
            curAttacher->dettach();
        }
    }
    OSGLContextAttacherPtr ret(new OSGLContextAttacher(c, width, height, rowWidth, buffer));
    return ret;
}


OSGLContextPtr
OSGLContextAttacher::getContext() const
{
    return _c;
}

void
OSGLContextAttacher::setDettachOnDtor(bool enabled)
{
    _dettachOnDtor = enabled;
}

void
OSGLContextAttacher::attach()
{
    if (!_attached) {
        // Register the context to this thread globally so we can track for each thread the active context
        appPTR->getGPUContextPool()->registerContextForThread(shared_from_this());

        // Make the context current to the thread, after this line the context is guaranteed to be bound and can no longer
        // be used by any other thread until dettach() is called
        _c->setContextCurrentInternal(_width, _height, _rowWidth, _buffer);
        ++_attached;
    }
}

bool
OSGLContextAttacher::isAttached() const
{
    return _attached > 0;
}

void
OSGLContextAttacher::dettach()
{

    if (_attached == 1) {
        appPTR->getGPUContextPool()->unregisterContextForThread();
        _c->unsetCurrentContext();
        assert(_attached > 0);
        --_attached;
    }
}


OSGLContextAttacher::~OSGLContextAttacher()
{
    if (_dettachOnDtor) {
        dettach();
    }
}

OSGLContextSaver::OSGLContextSaver()
: savedContext()
{
    // Remember the active context
    savedContext = appPTR->getGPUContextPool()->getThreadLocalContext();

}

OSGLContextSaver::~OSGLContextSaver()
{
    // Dettach whichever context was made current in the middle
    OSGLContextAttacherPtr curContext = appPTR->getGPUContextPool()->getThreadLocalContext();
    if (curContext) {
        if (curContext != savedContext) {
            curContext->dettach();
        }
    }

    // re-attach the old one
    if (savedContext) {
        savedContext->attach();
    }
}

NATRON_NAMESPACE_EXIT
