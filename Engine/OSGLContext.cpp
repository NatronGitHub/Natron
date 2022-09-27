/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2022 The Natron developers
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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "OSGLContext.h"

#include <limits>
#include <stdexcept>
#include <sstream> // stringstream

#include <QtCore/QDebug>
#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>

#include "Engine/AppManager.h"

#ifdef __NATRON_WIN32__
#include "Engine/OSGLContext_win.h"
#elif defined(__NATRON_OSX__)
#include "Engine/OSGLContext_mac.h"
#elif defined(__NATRON_LINUX__)
#include "Engine/OSGLContext_wayland.h"
#include "Engine/OSGLContext_x11.h"
#endif

#include "Engine/AbortableRenderInfo.h"
#include "Engine/DefaultShaders.h"
#include "Engine/GPUContextPool.h"
#include "Engine/GLShader.h"

NATRON_NAMESPACE_ENTER

FramebufferConfig::FramebufferConfig()
    : redBits(8)
    , greenBits(8)
    , blueBits(8)
    , alphaBits(8)
    , depthBits(24)
    , stencilBits(8)
    , accumRedBits(0)
    , accumGreenBits(0)
    , accumBlueBits(0)
    , accumAlphaBits(0)
    , auxBuffers(0)
    , stereo(GL_FALSE)
    , samples(0)
    , sRGB(GL_FALSE)
    , doublebuffer(GL_FALSE)
    , handle(0)
{
}

const FramebufferConfig&
OSGLContext::chooseFBConfig(const FramebufferConfig& desired,
                            const std::vector<FramebufferConfig>& alternatives,
                            int count)
{
    if ( alternatives.empty() || ( count > (int)alternatives.size() ) ) {
        throw std::logic_error("alternatives empty");
    }
    unsigned int missing, leastMissing = std::numeric_limits<unsigned int>::max();
    unsigned int colorDiff, leastColorDiff = std::numeric_limits<unsigned int>::max();
    unsigned int extraDiff, leastExtraDiff = std::numeric_limits<unsigned int>::max();
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
#ifdef __NATRON_WIN32__
    std::unique_ptr<OSGLContext_win> _platformContext;
#elif defined(__NATRON_OSX__)
    std::unique_ptr<OSGLContext_mac> _platformContext;
#elif defined(__NATRON_LINUX__)
    std::unique_ptr<OSGLContext_xdg> _platformContext;
#endif

#ifdef NATRON_RENDER_SHARED_CONTEXT
    // When we use a single GL context, renders are all sharing the same context and lock it when trying to render
    QMutex renderOwningContextMutex;
    QWaitCondition renderOwningContextCond;
    AbortableRenderInfoPtr renderOwningContext;
    int renderOwningContextCount;
#ifdef DEBUG
    double renderOwningContextFrameTime;
#endif
#endif

    // This pbo is used to call glTexSubImage2D and glReadPixels asynchronously
    GLuint pboID;

    // The main FBO onto which we do all renders
    GLuint fboID;
    GLShaderPtr fillImageShader;

    // One for enabled, one for disabled
    GLShaderPtr applyMaskMixShader[2];
    GLShaderPtr copyUnprocessedChannelsShader[16];

    OSGLContextPrivate()
        : _platformContext()
#ifdef NATRON_RENDER_SHARED_CONTEXT
        , renderOwningContextMutex()
        , renderOwningContextCond()
        , renderOwningContext()
        , renderOwningContextCount(0)
#ifdef DEBUG
        , renderOwningContextFrameTime(0)
#endif
#endif
        , pboID(0)
        , fboID(0)
        , fillImageShader()
        , applyMaskMixShader()
        , copyUnprocessedChannelsShader()
    {
    }

    // Create PBO and FBO if needed
    void init();
};

OSGLContext::OSGLContext(const FramebufferConfig& pixelFormatAttrs,
                         const OSGLContext* shareContext,
                         int major,
                         int minor,
                         const GLRendererID &rendererID,
                         bool coreProfile)
    : _imp( new OSGLContextPrivate() )
{
    if (coreProfile) {
        // Don't bother with core profile with OpenGL < 3
        if (major < 3) {
            coreProfile = false;
        }
    }
#ifdef __NATRON_WIN32__
    _imp->_platformContext.reset( new OSGLContext_win(pixelFormatAttrs, major, minor, coreProfile, rendererID, shareContext ? shareContext->_imp->_platformContext.get() : nullptr) );
#elif defined(__NATRON_OSX__)
    _imp->_platformContext.reset( new OSGLContext_mac(pixelFormatAttrs, major, minor, coreProfile, rendererID, shareContext ? shareContext->_imp->_platformContext.get() : nullptr) );
#elif defined(__NATRON_LINUX__)
    if (appPTR->isOnWayland()) {
        _imp->_platformContext.reset( new OSGLContext_wayland(pixelFormatAttrs, major, minor, coreProfile, rendererID, shareContext ? static_cast<const OSGLContext_wayland *>(shareContext->_imp->_platformContext.get()) : nullptr) );
    } else {
        _imp->_platformContext.reset( new OSGLContext_x11(pixelFormatAttrs, major, minor, coreProfile, rendererID, shareContext ? static_cast<const OSGLContext_x11 *>(shareContext->_imp->_platformContext.get()) : nullptr) );
    }
#endif
}

OSGLContext::~OSGLContext()
{
    setContextCurrentNoRender();
    if (_imp->pboID) {
        glDeleteBuffers(1, &_imp->pboID);
    }
    if (_imp->fboID) {
        glDeleteFramebuffers(1, &_imp->fboID);
    }
    unsetCurrentContextNoRender();
}

void
OSGLContext::checkOpenGLVersion()
{
    const char *verstr = (const char *) glGetString(GL_VERSION);
    int major, minor;

    if ( (verstr == NULL) || (std::sscanf(verstr, "%d.%d", &major, &minor) != 2) ) {
        major = minor = 0;
        //fprintf(stderr, "Invalid GL_VERSION format!!!\n");
    }

    if ( (major < GLVersion.major) || ( (major == GLVersion.major) && (minor < GLVersion.minor) ) ) {
        std::stringstream ss;
        ss << NATRON_APPLICATION_NAME << " requires at least OpenGL " << GLVersion.major << "." << GLVersion.minor << "to perform OpenGL accelerated rendering." << std::endl;
        ss << "Your system only has OpenGL " << major << "." << minor << "available.";
        throw std::runtime_error( ss.str() );
    }
}

GLuint
OSGLContext::getPBOId() const
{
    return _imp->pboID;
}

GLuint
OSGLContext::getFBOId() const
{
    return _imp->fboID;
}

void
OSGLContext::setContextCurrentNoRender()
{
#ifdef __NATRON_WIN32__
    OSGLContext_win::makeContextCurrent( _imp->_platformContext.get() );
#elif defined(__NATRON_OSX__)
    OSGLContext_mac::makeContextCurrent( _imp->_platformContext.get() );
#elif defined(__NATRON_LINUX__)
    if (appPTR->isOnWayland()) {
        OSGLContext_wayland::makeContextCurrent( static_cast<const OSGLContext_wayland *>( _imp->_platformContext.get()) );
    } else {
        OSGLContext_x11::makeContextCurrent( static_cast<const OSGLContext_x11 *>( _imp->_platformContext.get()) );
    }
#endif
}

void
OSGLContext::setContextCurrent(const AbortableRenderInfoPtr& abortInfo
#ifdef DEBUG
                               ,
                               double frameTime
#endif
                               )
{
    assert(_imp && _imp->_platformContext);

#ifdef NATRON_RENDER_SHARED_CONTEXT
    QMutexLocker k(&_imp->renderOwningContextMutex);
    while (_imp->renderOwningContext && _imp->renderOwningContext != abortInfo) {
        _imp->renderOwningContextCond.wait(k.mutex());
    }
    _imp->renderOwningContext = abortInfo;
#ifdef DEBUG
    if (!_imp->renderOwningContextCount) {
        _imp->renderOwningContextFrameTime = frameTime;
        //qDebug() << "Attaching" << this << "to render frame" << frameTime;
    }
#endif
    ++_imp->renderOwningContextCount;
#endif

    setContextCurrentNoRender();

    // The first time this context is made current, allocate the PBO and FBO
    // This is thread-safe
    if (!_imp->pboID) {
        _imp->init();
    }
}

void
OSGLContext::unsetCurrentContextNoRender()
{
#ifdef __NATRON_WIN32__
    OSGLContext_win::makeContextCurrent( nullptr );
#elif defined(__NATRON_OSX__)
    OSGLContext_mac::makeContextCurrent( nullptr );
#elif defined(__NATRON_LINUX__)
    if (appPTR->isOnWayland()) {
        OSGLContext_wayland::makeContextCurrent( nullptr );
    } else {
        OSGLContext_x11::makeContextCurrent( nullptr );
    }
#endif
}

void
OSGLContext::unsetCurrentContext(const AbortableRenderInfoPtr& abortInfo)
{
#ifdef NATRON_RENDER_SHARED_CONTEXT
    QMutexLocker k(&_imp->renderOwningContextMutex);
    if (abortInfo != _imp->renderOwningContext) {
        return;
    }
    --_imp->renderOwningContextCount;
    if (!_imp->renderOwningContextCount) {
#ifdef DEBUG
        //qDebug() << "Detaching" << this << "from frame" << _imp->renderOwningContextFrameTime;
#endif
        _imp->renderOwningContext.reset();
        unsetCurrentContextNoRender();
        //Wake-up only one thread waiting, since each thread that is waiting in setContextCurrent() will actually call this function.
        _imp->renderOwningContextCond.wakeOne();
    }
#else
    unsetCurrentContextNoRender();
#endif
}

void
OSGLContextPrivate::init()
{
    glGenBuffers(1, &pboID);
    glGenFramebuffers(1, &fboID);
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

GLShaderPtr
OSGLContext::getOrCreateFillShader()
{
    if (_imp->fillImageShader) {
        return _imp->fillImageShader;
    }
    _imp->fillImageShader = std::make_shared<GLShader>();
#ifdef DEBUG
    std::string error;
    bool ok = _imp->fillImageShader->addShader(GLShader::eShaderTypeFragment, fillConstant_FragmentShader, &error);
    if (!ok) {
        qDebug() << error.c_str();
    }
#else
    bool ok = _imp->fillImageShader->addShader(GLShader::eShaderTypeFragment, fillConstant_FragmentShader, nullptr);
#endif

    assert(ok);
#ifdef DEBUG
    ok = _imp->fillImageShader->link(&error);
    if (!ok) {
        qDebug() << error.c_str();
    }
#else
    ok = _imp->fillImageShader->link();
#endif
    assert(ok);
    Q_UNUSED(ok);

    return _imp->fillImageShader;
}

GLShaderPtr
OSGLContext::getOrCreateMaskMixShader(bool maskEnabled)
{
    int shader_i = maskEnabled ? 1 : 0;

    if (_imp->applyMaskMixShader[shader_i]) {
        return _imp->applyMaskMixShader[shader_i];
    }
    _imp->applyMaskMixShader[shader_i] = std::make_shared<GLShader>();

    std::string fragmentSource;
    if (maskEnabled) {
        fragmentSource += "#define MASK_ENABLED\n";
    }

    fragmentSource += std::string(applyMaskMix_FragmentShader);

#ifdef DEBUG
    std::string error;
    bool ok = _imp->applyMaskMixShader[shader_i]->addShader(GLShader::eShaderTypeFragment, fragmentSource.c_str(), &error);
    if (!ok) {
        qDebug() << error.c_str();
    }
#else
    bool ok = _imp->applyMaskMixShader[shader_i]->addShader(GLShader::eShaderTypeFragment, fragmentSource.c_str(), nullptr);
#endif
    assert(ok);
#ifdef DEBUG
    ok = _imp->applyMaskMixShader[shader_i]->link(&error);
    if (!ok) {
        qDebug() << error.c_str();
    }
#else
    ok = _imp->applyMaskMixShader[shader_i]->link();
#endif
    assert(ok);
    Q_UNUSED(ok);

    return _imp->applyMaskMixShader[shader_i];
}

GLShaderPtr
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
    _imp->copyUnprocessedChannelsShader[index] = std::make_shared<GLShader>();

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
    bool ok = _imp->copyUnprocessedChannelsShader[index]->addShader(GLShader::eShaderTypeFragment, fragmentSource.c_str(), &error);
    if (!ok) {
        qDebug() << error.c_str();
    }
#else
    bool ok = _imp->copyUnprocessedChannelsShader[index]->addShader(GLShader::eShaderTypeFragment, fragmentSource.c_str(), nullptr);
#endif
    assert(ok);
#ifdef DEBUG
    ok = _imp->copyUnprocessedChannelsShader[index]->link(&error);
    if (!ok) {
        qDebug() << error.c_str();
    }
#else
    ok = _imp->copyUnprocessedChannelsShader[index]->link();
#endif
    assert(ok);
    Q_UNUSED(ok);

    return _imp->copyUnprocessedChannelsShader[index];
} // OSGLContext::getOrCreateCopyUnprocessedChannelsShader

void
OSGLContext::getGPUInfos(std::list<OpenGLRendererInfo>& renderers)
{
#ifdef __NATRON_WIN32__
    OSGLContext_win::getGPUInfos(renderers);
#elif defined(__NATRON_OSX__)
    OSGLContext_mac::getGPUInfos(renderers);
#elif defined(__NATRON_LINUX__)
    if (appPTR->isOnWayland()) {
        OSGLContext_wayland::getGPUInfos(renderers);
    } else {
        OSGLContext_x11::getGPUInfos(renderers);
    }
#endif
}

NATRON_NAMESPACE_EXIT
