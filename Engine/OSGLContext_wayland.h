/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2021 The Natron developers
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

#ifndef OSGLCONTEXT_WAYLAND_H
#define OSGLCONTEXT_WAYLAND_H

#include "Global/Macros.h"

#include "Engine/EngineFwd.h"
#include "Engine/OSGLContext_xdg.h"
#include "Global/GLIncludes.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#endif

#ifdef __NATRON_LINUX__

#include "Global/GlobalDefines.h"

NATRON_NAMESPACE_ENTER

// EGL-specific global data
struct OSGLContext_egl_dataPrivate;
class OSGLContext_egl_data
{

public:

    std::unique_ptr<OSGLContext_egl_dataPrivate> _imp;

    GLADloadproc getProcAddress;

    OSGLContext_egl_data();

    ~OSGLContext_egl_data();
};

struct OSGLContext_waylandPrivate;
class OSGLContext_wayland : public OSGLContext_xdg
{
public:

    OSGLContext_wayland(const FramebufferConfig& pixelFormatAttrs,
                        int major,
                        int minor,
                        bool coreProfile,
                        const GLRendererID& rendererID,
                        const OSGLContext_wayland* shareContext);

    ~OSGLContext_wayland();

    static void initEGLData(OSGLContext_egl_data* eglInfo);
    static void destroyEGLData(OSGLContext_egl_data* eglInfo);
    static bool makeContextCurrent(const OSGLContext_wayland* context);
    static void getGPUInfos(std::list<OpenGLRendererInfo>& renderers);

    void swapBuffers();

    void swapInterval(int interval);

private:

    friend struct OSGLContext_waylandPrivate;
    std::unique_ptr<OSGLContext_waylandPrivate> _imp;
};

NATRON_NAMESPACE_EXIT

#endif // __NATRON_LINUX__

#endif // OSGLCONTEXT_WAYLAND_H
