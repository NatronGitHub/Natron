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

#ifdef __NATRON_WIN32__
#include "Engine/OSGLContext_win.h"
#elif defined(__NATRON_OSX__)
#include "Engine/OSGLContext_mac.h"
#elif defined(__NATRON_LINUX__)
#include "Engine/OSGLContext_x11.h"
#endif

NATRON_NAMESPACE_ENTER;

const FramebufferConfig&
OSGLContext::chooseFBConfig(const FramebufferConfig& desired, const std::vector<FramebufferConfig>& alternatives, int count)
{

    if (alternatives.empty() || count > (int)alternatives.size()) {
        throw std::logic_error("alternatives empty");
    }
    unsigned int missing, leastMissing = UINT_MAX;
    unsigned int colorDiff, leastColorDiff = UINT_MAX;
    unsigned int extraDiff, leastExtraDiff = UINT_MAX;
    int closest = -1;

    for (int i = 0;  i < count;  ++i)
    {
        const FramebufferConfig& current = alternatives[i];

        if (desired.stereo > 0 && current.stereo == 0)
        {
            // Stereo is a hard constraint
            continue;
        }

        if (desired.doublebuffer != current.doublebuffer)
        {
            // Double buffering is a hard constraint
            continue;
        }

        // Count number of missing buffers
        {
            missing = 0;

            if (desired.alphaBits > 0 && current.alphaBits == 0)
                missing++;

            if (desired.depthBits > 0 && current.depthBits == 0)
                missing++;

            if (desired.stencilBits > 0 && current.stencilBits == 0)
                missing++;

            if (desired.auxBuffers > 0 &&
                current.auxBuffers < desired.auxBuffers)
            {
                missing += desired.auxBuffers - current.auxBuffers;
            }

            if (desired.samples > 0 && current.samples == 0)
            {
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

            if (desired.redBits != FramebufferConfig::ATTR_DONT_CARE)
            {
                colorDiff += (desired.redBits - current.redBits) *
                (desired.redBits - current.redBits);
            }

            if (desired.greenBits != FramebufferConfig::ATTR_DONT_CARE)
            {
                colorDiff += (desired.greenBits - current.greenBits) *
                (desired.greenBits - current.greenBits);
            }

            if (desired.blueBits != FramebufferConfig::ATTR_DONT_CARE)
            {
                colorDiff += (desired.blueBits - current.blueBits) *
                (desired.blueBits - current.blueBits);
            }
        }

        // Calculate non-color channel size difference value
        {
            extraDiff = 0;

            if (desired.alphaBits != FramebufferConfig::ATTR_DONT_CARE)
            {
                extraDiff += (desired.alphaBits - current.alphaBits) *
                (desired.alphaBits - current.alphaBits);
            }

            if (desired.depthBits != FramebufferConfig::ATTR_DONT_CARE)
            {
                extraDiff += (desired.depthBits - current.depthBits) *
                (desired.depthBits - current.depthBits);
            }

            if (desired.stencilBits != FramebufferConfig::ATTR_DONT_CARE)
            {
                extraDiff += (desired.stencilBits - current.stencilBits) *
                (desired.stencilBits - current.stencilBits);
            }

            if (desired.accumRedBits != FramebufferConfig::ATTR_DONT_CARE)
            {
                extraDiff += (desired.accumRedBits - current.accumRedBits) *
                (desired.accumRedBits - current.accumRedBits);
            }

            if (desired.accumGreenBits != FramebufferConfig::ATTR_DONT_CARE)
            {
                extraDiff += (desired.accumGreenBits - current.accumGreenBits) *
                (desired.accumGreenBits - current.accumGreenBits);
            }

            if (desired.accumBlueBits != FramebufferConfig::ATTR_DONT_CARE)
            {
                extraDiff += (desired.accumBlueBits - current.accumBlueBits) *
                (desired.accumBlueBits - current.accumBlueBits);
            }

            if (desired.accumAlphaBits != FramebufferConfig::ATTR_DONT_CARE)
            {
                extraDiff += (desired.accumAlphaBits - current.accumAlphaBits) *
                (desired.accumAlphaBits - current.accumAlphaBits);
            }

            if (desired.samples != FramebufferConfig::ATTR_DONT_CARE)
            {
                extraDiff += (desired.samples - current.samples) *
                (desired.samples - current.samples);
            }

            if (desired.sRGB && !current.sRGB)
                extraDiff++;
        }

        // Figure out if the current one is better than the best one found so far
        // Least number of missing buffers is the most important heuristic,
        // then color buffer size match and lastly size match for other buffers

        if (missing < leastMissing) {
            closest = i;
        } else if (missing == leastMissing) {
            if ((colorDiff < leastColorDiff) || (colorDiff == leastColorDiff && extraDiff < leastExtraDiff)) {
                closest = i;
            }
        }

        if ((int)i == closest) {
            leastMissing = missing;
            leastColorDiff = colorDiff;
            leastExtraDiff = extraDiff;
        }
    } // for alternatives
    if (closest == -1) {
        throw std::logic_error("closest = -1");
    }
    return alternatives[closest];
}

struct OSGLContextPrivate
{
#ifdef __NATRON_WIN32__
    boost::scoped_ptr<OSGLContext_win> _platformContext;
#elif defined(__NATRON_OSX__)
    boost::scoped_ptr<OSGLContext_mac> _platformContext;
#elif defined(__NATRON_LINUX__)
    boost::scoped_ptr<OSGLContext_x11> _platformContext;
#endif

    OSGLContextPrivate()
    : _platformContext()
    {
       
    }
};

OSGLContext::OSGLContext(const FramebufferConfig& pixelFormatAttrs, int major, int minor)
: _imp(new OSGLContextPrivate())
{

#ifdef __NATRON_WIN32__
    _imp->_platformContext.reset(new OSGLContext_win(pixelFormatAttrs, major, minor));
#elif defined(__NATRON_OSX__)
    _imp->_platformContext.reset(new OSGLContext_mac(pixelFormatAttrs, major, minor));
#elif defined(__NATRON_LINUX__)
    _imp->_platformContext.reset(new OSGLContext_x11(pixelFormatAttrs, major, minor));
#endif
}

OSGLContext::~OSGLContext()
{

}

void
OSGLContext::makeContextCurrent()
{
#ifdef __NATRON_WIN32__
    OSGLContext_win::makeContextCurrent(_imp->_platformContext.get());
#elif defined(__NATRON_OSX__)
    OSGLContext_mac::makeContextCurrent(_imp->_platformContext.get());
#elif defined(__NATRON_LINUX__)
    OSGLContext_x11::makeContextCurrent(_imp->_platformContext.get());
#endif
}

bool
OSGLContext::stringInExtensionString(const char* string, const char* extensions)
{
    const char* start = extensions;

    for (;;)
    {
        const char* where;
        const char* terminator;

        where = strstr(start, string);
        if (!where)
            return false;

        terminator = where + strlen(string);
        if (where == start || *(where - 1) == ' ')
        {
            if (*terminator == ' ' || *terminator == '\0')
                break;
        }

        start = terminator;
    }
    
    return true;
}

NATRON_NAMESPACE_EXIT;
