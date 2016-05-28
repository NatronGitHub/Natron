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


#ifndef OSGLCONTEXT_WIN_H
#define OSGLCONTEXT_WIN_H

#include "Global/Macros.h"
#include "Global/GLIncludes.h"

#ifdef __NATRON_WIN32__

#include <windows.h>


typedef BOOL (WINAPI * PFNWGLSWAPINTERVALEXTPROC)(int);
typedef BOOL (WINAPI * PFNWGLGETPIXELFORMATATTRIBIVARBPROC)(HDC,int,int,UINT,const int*,int*);
typedef const char* (WINAPI * PFNWGLGETEXTENSIONSSTRINGEXTPROC)(void);
typedef const char* (WINAPI * PFNWGLGETEXTENSIONSSTRINGARBPROC)(HDC);
typedef HGLRC (WINAPI * PFNWGLCREATECONTEXTATTRIBSARBPROC)(HDC,HGLRC,const int*);

typedef HGLRC (WINAPI * WGLCREATECONTEXT_T)(HDC);
typedef BOOL (WINAPI * WGLDELETECONTEXT_T)(HGLRC);
typedef PROC (WINAPI * WGLGETPROCADDRESS_T)(LPCSTR);
typedef BOOL (WINAPI * WGLMAKECURRENT_T)(HDC,HGLRC);
typedef BOOL (WINAPI * WGLSHARELISTS_T)(HGLRC,HGLRC);

struct OSGLContext_wgl_data
{
    HINSTANCE                           instance;
    WGLCREATECONTEXT_T                  CreateContext;
    WGLDELETECONTEXT_T                  DeleteContext;
    WGLGETPROCADDRESS_T                 GetProcAddress;
    WGLMAKECURRENT_T                    MakeCurrent;
    WGLSHARELISTS_T                     ShareLists;

    GLFWbool                            extensionsLoaded;

    PFNWGLSWAPINTERVALEXTPROC           SwapIntervalEXT;
    PFNWGLGETPIXELFORMATATTRIBIVARBPROC GetPixelFormatAttribivARB;
    PFNWGLGETEXTENSIONSSTRINGEXTPROC    GetExtensionsStringEXT;
    PFNWGLGETEXTENSIONSSTRINGARBPROC    GetExtensionsStringARB;
    PFNWGLCREATECONTEXTATTRIBSARBPROC   CreateContextAttribsARB;
    GLboolean                            EXT_swap_control;
    GLboolean                            ARB_multisample;
    GLboolean                            ARB_framebuffer_sRGB;
    GLboolean                            EXT_framebuffer_sRGB;
    GLboolean                            ARB_pixel_format;
    GLboolean                            ARB_create_context;
    GLboolean                            ARB_create_context_profile;
    GLboolean                            EXT_create_context_es2_profile;
    GLboolean                            ARB_create_context_robustness;
    GLboolean                            ARB_context_flush_control;
    
};

class OSGLContext_win
{
public:

    OSGLContext_win(const FramebufferConfig& pixelFormatAttrs, int major, int minor);

    ~OSGLContext_win();

    static bool makeContextCurrent(const OSGLContext_win* context);

    void swapBuffers();

    void swapInterval(int interval);

    static void initWGLData(OSGLContext_wgl_data* wglInfo);
    static bool loadWGLExtensions(OSGLContext_wgl_data* wglInfo);
    static void destroyWGLData(OSGLContext_wgl_data* wglInfo);
private:

    int getPixelFormatAttrib(const OSGLContext_wgl_data* wglInfo, HWND window, int pixelFormat, int attrib);

    void createGLContext(const FramebufferConfig& pixelFormatAttrs, int major, int minor);

    bool analyzeContextWGL(const FramebufferConfig& pixelFormatAttrs, int major, int minor);

    void destroyWindow();

    // WGL-specific per-context data
    HDC       _dc;
    HGLRC     _handle;
    int       _interval;
    HWND _windowHandle;


};
#endif // __NATRON_WIN32__

#endif // OSGLCONTEXT_WIN_H
