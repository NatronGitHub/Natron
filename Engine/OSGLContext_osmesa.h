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

#ifndef OSGLCONTEXT_OSMESA_H
#define OSGLCONTEXT_OSMESA_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#endif

#include "Global/GlobalDefines.h"

#include "Engine/EngineFwd.h"

#ifdef HAVE_OSMESA

NATRON_NAMESPACE_ENTER

struct OSGLContext_osmesaPrivate;
class OSGLContext_osmesa
{
public:

    
    OSGLContext_osmesa(const FramebufferConfig& pixelFormatAttrs,
                       int major,
                       int minor,
                       bool coreProfile,
                       const GLRendererID& rendererID,
                       const OSGLContext_osmesa* shareContext);

    ~OSGLContext_osmesa();

    /**
     * @brief Make the offscreen rendering context current. 
     * @param type must be either GL_UNSIGNED_BYTE, GL_UNSIGNED_SHORT or GL_FLOAT
     * @param width The width of the buffer
     * @param height the height of the buffer
     * @param buffer a pointer to the buffer data where the context must write
     **/
    static bool makeContextCurrent(const OSGLContext_osmesa* context,
                                   int type,
                                   int width,
                                   int height,
                                   int rowWidth,
                                   void* buffer);

    static int getMaxWidth();
    static int getMaxHeight();

    static bool unSetContext(const OSGLContext_osmesa* context);

    static void getOSMesaVersion(int* major, int* minor, int* rev);

private:
    friend struct OSGLContext_osmesaPrivate;
    boost::scoped_ptr<OSGLContext_osmesaPrivate> _imp;
};

NATRON_NAMESPACE_EXIT

#endif // HAVE_OSMESA
#endif // OSGLCONTEXT_OSMESA_H
