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

#include <QDebug>

#ifdef __NATRON_WIN32__
#include "Engine/OSGLContext_win.h"
#elif defined(__NATRON_OSX__)
#include "Engine/OSGLContext_mac.h"
#elif defined(__NATRON_LINUX__)
#include "Engine/OSGLContext_x11.h"
#endif

#include "Engine/DefaultShaders.h"
#include "Engine/GLShader.h"

NATRON_NAMESPACE_ENTER;

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
#ifdef __NATRON_WIN32__
    boost::scoped_ptr<OSGLContext_win> _platformContext;
#elif defined(__NATRON_OSX__)
    boost::scoped_ptr<OSGLContext_mac> _platformContext;
#elif defined(__NATRON_LINUX__)
    boost::scoped_ptr<OSGLContext_x11> _platformContext;
#endif

    // This pbo is used to call glTexSubImage2D and glReadPixels asynchronously
    GLuint pboID;

    // The main FBO onto which we do all renders
    GLuint fboID;
    boost::shared_ptr<GLShader> fillImageShader, applyMaskMixShader, copyUnprocessedChannelsShader;

    OSGLContextPrivate()
        : _platformContext()
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
                         int minor)
    : _imp( new OSGLContextPrivate() )
{
#ifdef __NATRON_WIN32__
    _imp->_platformContext.reset( new OSGLContext_win(pixelFormatAttrs, major, minor, shareContext ? shareContext->_imp->_platformContext.get() : 0) );
#elif defined(__NATRON_OSX__)
    _imp->_platformContext.reset( new OSGLContext_mac(pixelFormatAttrs, major, minor, shareContext ? shareContext->_imp->_platformContext.get() : 0) );
#elif defined(__NATRON_LINUX__)
    _imp->_platformContext.reset( new OSGLContext_x11(pixelFormatAttrs, major, minor, shareContext ? shareContext->_imp->_platformContext.get() : 0) );
#endif
}

OSGLContext::~OSGLContext()
{
    if (_imp->pboID) {
        glDeleteBuffers(1, &_imp->pboID);
    }
    if (_imp->fboID) {
        glDeleteFramebuffers(1, &_imp->fboID);
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
OSGLContext::makeContextCurrent()
{
#ifdef __NATRON_WIN32__
    OSGLContext_win::makeContextCurrent( _imp->_platformContext.get() );
#elif defined(__NATRON_OSX__)
    OSGLContext_mac::makeContextCurrent( _imp->_platformContext.get() );
#elif defined(__NATRON_LINUX__)
    OSGLContext_x11::makeContextCurrent( _imp->_platformContext.get() );
#endif

    // The first time this context is made current, allocate the PBO and FBO
    // This is thread-safe because we actually call this under the GPUContextPool
    // before we make this context available to all threads
    if (!_imp->pboID) {
        _imp->init();
    }
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

boost::shared_ptr<GLShader>
OSGLContext::getOrCreateDefaultShader(DefaultGLShaderEnum type)
{
    switch (type) {
    case eDefaultGLShaderFillConstant: {
        if (_imp->fillImageShader) {
            return _imp->fillImageShader;
        }
        _imp->fillImageShader.reset( new GLShader() );
#ifdef DEBUG
        std::string error;
        bool ok = _imp->fillImageShader->addShader(GLShader::eShaderTypeFragment, fillConstant_FragmentShader, &error);
        if (!ok) {
            qDebug() << error.c_str();
        }
#else
        bool ok = _imp->fillImageShader->addShader(GLShader::eShaderTypeFragment, fillConstant_FragmentShader, 0);
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

        return _imp->fillImageShader;
    }

    case eDefaultGLShaderApplyMaskMix: {
        if (_imp->applyMaskMixShader) {
            return _imp->applyMaskMixShader;
        }
        _imp->applyMaskMixShader.reset( new GLShader() );
#ifdef DEBUG
        std::string error;
        bool ok = _imp->applyMaskMixShader->addShader(GLShader::eShaderTypeFragment, applyMaskMix_FragmentShader, &error);
        if (!ok) {
            qDebug() << error.c_str();
        }
#else
        bool ok = _imp->applyMaskMixShader->addShader(GLShader::eShaderTypeFragment, applyMaskMix_FragmentShader, 0);
#endif
#ifdef DEBUG
        ok = _imp->applyMaskMixShader->link(&error);
        if (!ok) {
            qDebug() << error.c_str();
        }
#else
        ok = _imp->applyMaskMixShader->link();
#endif

        return _imp->applyMaskMixShader;
    }

    case eDefaultGLShaderCopyUnprocessedChannels: {
        if (_imp->copyUnprocessedChannelsShader) {
            return _imp->copyUnprocessedChannelsShader;
        }
        _imp->copyUnprocessedChannelsShader.reset( new GLShader() );
#ifdef DEBUG
        std::string error;
        bool ok = _imp->copyUnprocessedChannelsShader->addShader(GLShader::eShaderTypeFragment, copyUnprocessedChannels_FragmentShader, &error);
        if (!ok) {
            qDebug() << error.c_str();
        }
#else
        bool ok = _imp->copyUnprocessedChannelsShader->addShader(GLShader::eShaderTypeFragment, copyUnprocessedChannels_FragmentShader, 0);
#endif
#ifdef DEBUG
        ok = _imp->copyUnprocessedChannelsShader->link(&error);
        if (!ok) {
            qDebug() << error.c_str();
        }
#else
        ok = _imp->copyUnprocessedChannelsShader->link();
#endif

        return _imp->copyUnprocessedChannelsShader;
    }
    } // switch
    assert(false);

    return boost::shared_ptr<GLShader>();
} // getOrCreateDefaultShader

NATRON_NAMESPACE_EXIT;
