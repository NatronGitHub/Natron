/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
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

#include "OSGLContext_osmesa.h"

#ifdef HAVE_OSMESA

#include <GL/gl_mangle.h>
#include <GL/glu_mangle.h>
#include <GL/osmesa.h>
#include <stdexcept>

#include "Engine/OSGLFramebufferConfig.h"

NATRON_NAMESPACE_ENTER

struct OSGLContext_osmesaPrivate
{

    OSMesaContext ctx;
    
    OSGLContext_osmesaPrivate()
    : ctx(0)
    {

    }
};

void
OSGLContext_osmesa::getOSMesaVersion(int* major, int* minor, int* rev)
{
    *major = OSMESA_MAJOR_VERSION;
    *minor = OSMESA_MINOR_VERSION;
    *rev = OSMESA_PATCH_VERSION;
}

OSGLContext_osmesa::OSGLContext_osmesa(const FramebufferConfig& pixelFormatAttrs,
                                       int /*major*/,
                                       int /*minor*/,
                                       bool /*coreProfile*/,
                                       const GLRendererID& rendererID,
                                       const OSGLContext_osmesa* shareContex)
: _imp(new OSGLContext_osmesaPrivate())
{
    GLenum format = GL_RGBA;
    int depthBits = pixelFormatAttrs.depthBits;
    int stencilBits = pixelFormatAttrs.stencilBits;
    int accumBits = pixelFormatAttrs.accumRedBits + pixelFormatAttrs.accumGreenBits + pixelFormatAttrs.accumBlueBits + pixelFormatAttrs.accumAlphaBits;
    /* Create an RGBA-mode context */
#if defined(OSMESA_GALLIUM_DRIVER) && (OSMESA_MAJOR_VERSION * 100 + OSMESA_MINOR_VERSION >= 1102)
    /* specify Z, stencil, accum sizes */
    {
        int cpuDriver = rendererID.renderID;

        int attribs[100], n = 0;

        attribs[n++] = OSMESA_FORMAT;
        attribs[n++] = format;
        attribs[n++] = OSMESA_DEPTH_BITS;
        attribs[n++] = depthBits;
        attribs[n++] = OSMESA_STENCIL_BITS;
        attribs[n++] = stencilBits;
        attribs[n++] = OSMESA_ACCUM_BITS;
        attribs[n++] = accumBits;
        attribs[n++] = OSMESA_GALLIUM_DRIVER;
        attribs[n++] = (int)cpuDriver;
        attribs[n++] = 0;
        _imp->ctx = OSMesaCreateContextAttribs( attribs, shareContex ? shareContex->_imp->ctx : 0);
    }
#else
    Q_UNUSED(rendererID);
#if OSMESA_MAJOR_VERSION * 100 + OSMESA_MINOR_VERSION >= 305
    /* specify Z, stencil, accum sizes */
    _imp->ctx = OSMesaCreateContextExt( format, depthBits, stencilBits, accumBits, shareContex ? shareContex->_imp->ctx : 0);
#else
    _imp->ctx = OSMesaCreateContext( format, shareContex ? shareContex->_imp->ctx : 0);
#endif
#endif
    
    if (!_imp->ctx) {
        throw std::runtime_error("OSMesaCreateContext failed");
    }
}

OSGLContext_osmesa::~OSGLContext_osmesa()
{
    unSetContext(this);
    assert( !OSMesaGetCurrentContext() );
    OSMesaDestroyContext( _imp->ctx );

}

int
OSGLContext_osmesa::getMaxWidth()
{
    GLint value;
    OSMesaGetIntegerv(OSMESA_MAX_WIDTH, &value);
    return (int)value;
}

int
OSGLContext_osmesa::getMaxHeight()
{
    GLint value;
    OSMesaGetIntegerv(OSMESA_MAX_HEIGHT, &value);
    return (int)value;
}


bool
OSGLContext_osmesa::makeContextCurrent(const OSGLContext_osmesa* context,
                                       int type,
                                       int width,
                                       int height,
                                       int rowWidth,
                                       void* buffer)
{
    bool ret = OSMesaMakeCurrent( context ? context->_imp->ctx : 0, buffer, type, width, height );
    OSMesaPixelStore(OSMESA_ROW_LENGTH, rowWidth);
    return ret;
}

bool
OSGLContext_osmesa::unSetContext(const OSGLContext_osmesa* context)
{
    bool ret = OSMesaMakeCurrent( context ? context->_imp->ctx : NULL, NULL, 0, 0, 0 ); // detach buffer from context
    OSMesaMakeCurrent(NULL, NULL, 0, 0, 0); // deactivate the context (not really recessary)
    assert(OSMesaGetCurrentContext() == 0);
    return ret;

}
NATRON_NAMESPACE_EXIT

#endif //HAVE_OSMESA
