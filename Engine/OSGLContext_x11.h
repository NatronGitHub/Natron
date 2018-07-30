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

#ifndef OSGLCONTEXT_X11_H
#define OSGLCONTEXT_X11_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#ifdef __NATRON_LINUX__

#include "Global/GLIncludes.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#endif

#include "Global/GlobalDefines.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER


// GLX-specific global data
struct OSGLContext_glx_dataPrivate;
class OSGLContext_glx_data
{
public:

    boost::scoped_ptr<OSGLContext_glx_dataPrivate> _imp;


    OSGLContext_glx_data();

    ~OSGLContext_glx_data();
};

struct OSGLContext_x11Private;
class OSGLContext_x11
{
public:

    OSGLContext_x11(const FramebufferConfig& pixelFormatAttrs,
                    int major,
                    int minor,
                    bool coreProfile,
                    const GLRendererID& rendererID,
                    const OSGLContext_x11* shareContext);

    ~OSGLContext_x11();

    static void initGLXData(OSGLContext_glx_data* glxInfo);
    static void destroyGLXData(OSGLContext_glx_data* glxInfo);
    static bool makeContextCurrent(const OSGLContext_x11* context);
    static void getGPUInfos(std::list<OpenGLRendererInfo>& renderers);

    void swapBuffers();

    void swapInterval(int interval);

private:
    friend struct OSGLContext_x11Private;
    boost::scoped_ptr<OSGLContext_x11Private> _imp;
};

NATRON_NAMESPACE_EXIT

#endif // __NATRON_LINUX__

#endif // OSGLCONTEXT_X11_H
