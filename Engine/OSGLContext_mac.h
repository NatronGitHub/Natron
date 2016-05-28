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

#ifndef OSGLCONTEXT_MAC_H
#define OSGLCONTEXT_MAC_H

#include "Engine/EngineFwd.h"
#include "Global/Macros.h"
#ifdef __NATRON_OSX__

//#import <Cocoa/Cocoa.h>
#include <OpenGL/OpenGL.h>
#include <OpenGL/CGLTypes.h>


NATRON_NAMESPACE_ENTER;

class OSGLContext_mac
{
public:

    OSGLContext_mac(const FramebufferConfig& pixelFormatAttrs, int major, int minor);

    ~OSGLContext_mac();

    static void makeContextCurrent(const OSGLContext_mac* context);

    void swapBuffers();

    void swapInterval(int interval);

private:

    /*void createWindow();

    struct WindowNS
    {
        id object;
        id delegate;
        id view;
    };

    WindowNS _nsWindow;

    // NSGL-specific per-context data
    id _pixelFormat;
    id _object;*/
    CGLContextObj _context;

};

NATRON_NAMESPACE_EXIT;

#endif // __NATRON_OSX__

#endif // OSGLCONTEXT_MAC_H
