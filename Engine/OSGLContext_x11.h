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

#ifndef OSGLCONTEXT_X11_H
#define OSGLCONTEXT_X11_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include "Global/GLIncludes.h"

#ifdef __NATRON_LINUX__


#include <dlfcn.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xmd.h>

typedef XID GLXWindow;
typedef XID GLXDrawable;
typedef struct __GLXFBConfig* GLXFBConfig;
typedef struct __GLXcontext* GLXContext;
typedef void (*__GLXextproc)(void);

typedef int (*PFNGLXGETFBCONFIGATTRIBPROC)(Display*,GLXFBConfig,int,int*);
typedef const char* (*PFNGLXGETCLIENTSTRINGPROC)(Display*,int);
typedef Bool (*PFNGLXQUERYEXTENSIONPROC)(Display*,int*,int*);
typedef Bool (*PFNGLXQUERYVERSIONPROC)(Display*,int*,int*);
typedef void (*PFNGLXDESTROYCONTEXTPROC)(Display*,GLXContext);
typedef Bool (*PFNGLXMAKECURRENTPROC)(Display*,GLXDrawable,GLXContext);
typedef void (*PFNGLXSWAPBUFFERSPROC)(Display*,GLXDrawable);
typedef const char* (*PFNGLXQUERYEXTENSIONSSTRINGPROC)(Display*,int);
typedef GLXFBConfig* (*PFNGLXGETFBCONFIGSPROC)(Display*,int,int*);
typedef GLXContext (*PFNGLXCREATENEWCONTEXTPROC)(Display*,GLXFBConfig,int,GLXContext,Bool);
typedef __GLXextproc (* PFNGLXGETPROCADDRESSPROC) (const GLubyte *procName);
typedef int (*PFNGLXSWAPINTERVALMESAPROC)(int);
typedef int (*PFNGLXSWAPINTERVALSGIPROC)(int);
typedef void (*PFNGLXSWAPINTERVALEXTPROC)(Display*,GLXDrawable,int);
typedef GLXContext (*PFNGLXCREATECONTEXTATTRIBSARBPROC)(Display*,GLXFBConfig,GLXContext,Bool,const int*);
typedef XVisualInfo* (*PFNGLXGETVISUALFROMFBCONFIGPROC)(Display*,GLXFBConfig);
typedef GLXWindow (*PFNGLXCREATEWINDOWPROC)(Display*,GLXFBConfig,Window,const int*);
typedef void (*PFNGLXDESTROYWINDOWPROC)(Display*,GLXWindow);
typedef GLXPBuffer (*PFNGLXCREATEPBUFFERPROC)(Display*,GLXFBConfig,const int*);
typedef void (*PFNGLXDESTROYPBUFFERPROC)(Display*,GLXPBuffer);
typedef void (*PFNGLXMAKECONTEXTCURRENTPROC)(Display*,GLXDrawable,GLXDrawable,GLXContext);

NATRON_NAMESPACE_ENTER;

// X11-specific global data
//
struct X11Data
{
    Display*        display;
    int             screen;
    Window          root;
    // Context for mapping window XIDs to _GLFWwindow pointers
    XContext        context;
    // Most recent error code received by X error handler
    int             errorCode;
};


// GLX-specific global data
struct OSGLContext_glx_data
{
    X11Data x11;

    int             major, minor;
    int             eventBase;
    int             errorBase;

    // dlopen handle for libGL.so.1
    void*           handle;

    // GLX 1.3 functions
    PFNGLXGETFBCONFIGSPROC              GetFBConfigs;
    PFNGLXGETFBCONFIGATTRIBPROC         GetFBConfigAttrib;
    PFNGLXGETCLIENTSTRINGPROC           GetClientString;
    PFNGLXQUERYEXTENSIONPROC            QueryExtension;
    PFNGLXQUERYVERSIONPROC              QueryVersion;
    PFNGLXDESTROYCONTEXTPROC            DestroyContext;
    PFNGLXMAKECURRENTPROC               MakeCurrent;
    PFNGLXSWAPBUFFERSPROC               SwapBuffers;
    PFNGLXQUERYEXTENSIONSSTRINGPROC     QueryExtensionsString;
    PFNGLXCREATENEWCONTEXTPROC          CreateNewContext;
    PFNGLXGETVISUALFROMFBCONFIGPROC     GetVisualFromFBConfig;
    PFNGLXCREATEWINDOWPROC              CreateWindow;
    PFNGLXDESTROYWINDOWPROC             DestroyWindow;
    PFNGLXCREATEPBUFFERPROC             CreatePBuffer;
    PFNGLXDESTROYPBUFFERPROC            DestroyPBuffer;
    PFNGLXMAKECONTEXTCURRENTPROC        MakeContextCurrent;

    // GLX 1.4 and extension functions
    PFNGLXGETPROCADDRESSPROC            GetProcAddress;
    PFNGLXGETPROCADDRESSPROC            GetProcAddressARB;
    PFNGLXSWAPINTERVALSGIPROC           SwapIntervalSGI;
    PFNGLXSWAPINTERVALEXTPROC           SwapIntervalEXT;
    PFNGLXSWAPINTERVALMESAPROC          SwapIntervalMESA;
    PFNGLXCREATECONTEXTATTRIBSARBPROC   CreateContextAttribsARB;
    GLboolean        SGI_swap_control;
    GLboolean        EXT_swap_control;
    GLboolean        MESA_swap_control;
    GLboolean        ARB_multisample;
    GLboolean        ARB_framebuffer_sRGB;
    GLboolean        EXT_framebuffer_sRGB;
    GLboolean        ARB_create_context;
    GLboolean        ARB_create_context_profile;
    GLboolean        ARB_create_context_robustness;
    GLboolean        EXT_create_context_es2_profile;
    GLboolean        ARB_context_flush_control;
};



class OSGLContext_x11
{
public:

    OSGLContext_x11(const FramebufferConfig& pixelFormatAttrs, int major, int minor, const OSGLContext_x11* shareContext);

    ~OSGLContext_x11();

    static void initGLXData(OSGLContext_glx_data* glxInfo);
    static void destroyGLXData(OSGLContext_glx_data* glxInfo);

    static bool makeContextCurrent(const OSGLContext_x11* context);

    void swapBuffers();

    void swapInterval(int interval);

private:

    void createContextGLX(OSGLContext_glx_data* glxInfo, const FramebufferConfig& fbconfig, int major, int minor, const OSGLContext_x11* shareContext);

    void createWindow(OSGLContext_glx_data* glxInfo,
                      Visual* visual, int depth);


    // X11-specific per-window data
    //
    struct X11Window {
        Colormap        colormap;
        Window          handle;
    };

    GLXContext _glxContextHandle;
    GLXWindow _glxWindowHandle;
    X11Window _x11Window;

};

NATRON_NAMESPACE_EXIT;

#endif // __NATRON_LINUX__

#endif // OSGLCONTEXT_X11_H
