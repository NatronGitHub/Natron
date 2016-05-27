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


#include "OSGLContext_mac.h"

#include <stdexcept>
#ifdef __NATRON_OSX__

#import <Cocoa/Cocoa.h>

#include "Engine/OSGLContext.h"

NATRON_NAMESPACE_ENTER;

OSGLContext_mac::OSGLContext_mac(const FramebufferConfig& pixelFormatAttrs,int major, int /*minor*/)
: _pixelFormat(0)
, _object(0)
{

    unsigned int attributeCount = 0;

    // Context robustness modes (GL_KHR_robustness) are not yet supported on
    // OS X but are not a hard constraint, so ignore and continue

    // Context release behaviors (GL_KHR_context_flush_control) are not yet
    // supported on OS X but are not a hard constraint, so ignore and continue

#define ADD_ATTR(x) { attributes[attributeCount++] = x; }
#define ADD_ATTR2(x, y) { ADD_ATTR(x); ADD_ATTR(y); }

    // Arbitrary array size here
    NSOpenGLPixelFormatAttribute attributes[40];

    ADD_ATTR(NSOpenGLPFAAccelerated);
    ADD_ATTR(NSOpenGLPFAClosestPolicy);

#if MAC_OS_X_VERSION_MAX_ALLOWED >= 101000
    if (major >= 4)
    {
        ADD_ATTR2(NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion4_1Core);
    }
    else
#endif /*MAC_OS_X_VERSION_MAX_ALLOWED*/
        if (major >= 3)
        {
            ADD_ATTR2(NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion3_2Core);
        }

    if (major <= 2)
    {
        if (pixelFormatAttrs.auxBuffers != FramebufferConfig::ATTR_DONT_CARE)
            ADD_ATTR2(NSOpenGLPFAAuxBuffers, pixelFormatAttrs.auxBuffers);

        if (pixelFormatAttrs.accumRedBits != FramebufferConfig::ATTR_DONT_CARE &&
            pixelFormatAttrs.accumGreenBits != FramebufferConfig::ATTR_DONT_CARE &&
            pixelFormatAttrs.accumBlueBits != FramebufferConfig::ATTR_DONT_CARE &&
            pixelFormatAttrs.accumAlphaBits != FramebufferConfig::ATTR_DONT_CARE)
        {
            const int accumBits = pixelFormatAttrs.accumRedBits +
            pixelFormatAttrs.accumGreenBits +
            pixelFormatAttrs.accumBlueBits +
            pixelFormatAttrs.accumAlphaBits;

            ADD_ATTR2(NSOpenGLPFAAccumSize, accumBits);
        }
    }

    if (pixelFormatAttrs.redBits != FramebufferConfig::ATTR_DONT_CARE &&
        pixelFormatAttrs.greenBits != FramebufferConfig::ATTR_DONT_CARE &&
        pixelFormatAttrs.blueBits != FramebufferConfig::ATTR_DONT_CARE)
    {
        int colorBits = pixelFormatAttrs.redBits +
        pixelFormatAttrs.greenBits +
        pixelFormatAttrs.blueBits;

        // OS X needs non-zero color size, so set reasonable values
        if (colorBits == 0)
            colorBits = 24;
        else if (colorBits < 15)
            colorBits = 15;

        ADD_ATTR2(NSOpenGLPFAColorSize, colorBits);
    }

    if (pixelFormatAttrs.alphaBits != FramebufferConfig::ATTR_DONT_CARE)
        ADD_ATTR2(NSOpenGLPFAAlphaSize, pixelFormatAttrs.alphaBits);

    if (pixelFormatAttrs.depthBits != FramebufferConfig::ATTR_DONT_CARE)
        ADD_ATTR2(NSOpenGLPFADepthSize, pixelFormatAttrs.depthBits);

    if (pixelFormatAttrs.stencilBits != FramebufferConfig::ATTR_DONT_CARE)
        ADD_ATTR2(NSOpenGLPFAStencilSize, pixelFormatAttrs.stencilBits);

    if (pixelFormatAttrs.stereo)
        ADD_ATTR(NSOpenGLPFAStereo);

    if (pixelFormatAttrs.doublebuffer)
        ADD_ATTR(NSOpenGLPFADoubleBuffer);

    if (pixelFormatAttrs.samples != FramebufferConfig::ATTR_DONT_CARE)
    {
        if (pixelFormatAttrs.samples == 0)
        {
            ADD_ATTR2(NSOpenGLPFASampleBuffers, 0);
        }
        else
        {
            ADD_ATTR2(NSOpenGLPFASampleBuffers, 1);
            ADD_ATTR2(NSOpenGLPFASamples, pixelFormatAttrs.samples);
        }
    }

    // NOTE: All NSOpenGLPixelFormats on the relevant cards support sRGB
    //       framebuffer, so there's no need (and no way) to request it

    ADD_ATTR(0);

#undef ADD_ATTR
#undef ADD_ATTR2

    _pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attributes];


    if (_pixelFormat == nil) {
        throw std::runtime_error("NSGL: Failed to find a suitable pixel format");
    }

    // Never share the OpenGL context for offscreen rendering
    NSOpenGLContext* share = NULL;
    _object = [[NSOpenGLContext alloc] initWithFormat:_pixelFormat shareContext:share];

    if (_object == nil) {
        throw std::runtime_error("NSGL: Failed to create OpenGL context");
    }

    //[_object setView:window->ns.view];
}

void
OSGLContext_mac::makeContextCurrent(const OSGLContext_mac* context)
{
    if (context) {
        [context->_object makeCurrentContext];
    } else {
        [NSOpenGLContext clearCurrentContext];
    }
}

void
OSGLContext_mac::swapBuffers()
{
    // ARP appears to be unnecessary, but this is future-proof
    [_object flushBuffer];
}

void
OSGLContext_mac::swapInterval(int interval)
{
    GLint sync = interval;
    [_object setValues:&sync forParameter:NSOpenGLCPSwapInterval];
}

OSGLContext_mac::~OSGLContext_mac()
{
    [_pixelFormat release];
    _pixelFormat = nil;

    [_object release];
    _object = nil;
}

NATRON_NAMESPACE_EXIT;

#endif // __NATRON_OSX__