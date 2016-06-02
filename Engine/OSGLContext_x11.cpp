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

#include "OSGLContext_x11.h"

#ifdef __NATRON_LINUX__

#include <stdexcept>
#include <iostream>
#include "Engine/AppManager.h"
#include "Engine/OSGLContext.h"
#include "Global/GLIncludes.h"

#include <dlfcn.h>

extern "C"
{
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xmd.h>
#include <X11/Xresource.h>
}

#define GLX_VENDOR 1
#define GLX_RGBA_BIT 0x00000001
#define GLX_WINDOW_BIT 0x00000001
#define GLX_DRAWABLE_TYPE 0x8010
#define GLX_RENDER_TYPE	0x8011
#define GLX_RGBA_TYPE 0x8014
#define GLX_DOUBLEBUFFER 5
#define GLX_STEREO 6
#define GLX_AUX_BUFFERS	7
#define GLX_RED_SIZE 8
#define GLX_GREEN_SIZE 9
#define GLX_BLUE_SIZE 10
#define GLX_ALPHA_SIZE 11
#define GLX_DEPTH_SIZE 12
#define GLX_STENCIL_SIZE 13
#define GLX_ACCUM_RED_SIZE 14
#define GLX_ACCUM_GREEN_SIZE 15
#define GLX_ACCUM_BLUE_SIZE	16
#define GLX_ACCUM_ALPHA_SIZE 17
#define GLX_SAMPLES 0x186a1
#define GLX_VISUAL_ID 0x800b

#define GLX_FRAMEBUFFER_SRGB_CAPABLE_ARB 0x20b2
#define GLX_CONTEXT_DEBUG_BIT_ARB 0x00000001
#define GLX_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x00000002
#define GLX_CONTEXT_CORE_PROFILE_BIT_ARB 0x00000001
#define GLX_CONTEXT_PROFILE_MASK_ARB 0x9126
#define GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB 0x00000002
#define GLX_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define GLX_CONTEXT_MINOR_VERSION_ARB 0x2092
#define GLX_CONTEXT_FLAGS_ARB 0x2094
#define GLX_CONTEXT_ES2_PROFILE_BIT_EXT 0x00000004
#define GLX_CONTEXT_ROBUST_ACCESS_BIT_ARB 0x00000004
#define GLX_LOSE_CONTEXT_ON_RESET_ARB 0x8252
#define GLX_CONTEXT_RESET_NOTIFICATION_STRATEGY_ARB 0x8256
#define GLX_NO_RESET_NOTIFICATION_ARB 0x8261
#define GLX_CONTEXT_RELEASE_BEHAVIOR_ARB 0x2097
#define GLX_CONTEXT_RELEASE_BEHAVIOR_NONE_ARB 0
#define GLX_CONTEXT_RELEASE_BEHAVIOR_FLUSH_ARB 0x2098


typedef XID GLXWindow;
typedef XID GLXDrawable;
typedef struct __GLXFBConfig* GLXFBConfig;
typedef struct __GLXcontext* GLXContext;
typedef void (*__GLXextproc)(void);
typedef int (*PFNGLXGETFBCONFIGATTRIBPROC)(Display*, GLXFBConfig, int, int*);
typedef const char* (*PFNGLXGETCLIENTSTRINGPROC)(Display*, int);
typedef Bool (*PFNGLXQUERYEXTENSIONPROC)(Display*, int*, int*);
typedef Bool (*PFNGLXQUERYVERSIONPROC)(Display*, int*, int*);
typedef void (*PFNGLXDESTROYCONTEXTPROC)(Display*, GLXContext);
typedef Bool (*PFNGLXMAKECURRENTPROC)(Display*, GLXDrawable, GLXContext);
typedef void (*PFNGLXSWAPBUFFERSPROC)(Display*, GLXDrawable);
typedef const char* (*PFNGLXQUERYEXTENSIONSSTRINGPROC)(Display*, int);
typedef GLXFBConfig* (*PFNGLXGETFBCONFIGSPROC)(Display*, int, int*);
typedef GLXContext (*PFNGLXCREATENEWCONTEXTPROC)(Display*, GLXFBConfig, int, GLXContext, Bool);
typedef __GLXextproc (* PFNGLXGETPROCADDRESSPROC) (const GLubyte *procName);
typedef int (*PFNGLXSWAPINTERVALMESAPROC)(int);
typedef int (*PFNGLXSWAPINTERVALSGIPROC)(int);
typedef void (*PFNGLXSWAPINTERVALEXTPROC)(Display*, GLXDrawable, int);
typedef GLXContext (*PFNGLXCREATECONTEXTATTRIBSARBPROC)(Display*, GLXFBConfig, GLXContext, Bool, const int*);
typedef XVisualInfo* (*PFNGLXGETVISUALFROMFBCONFIGPROC)(Display*, GLXFBConfig);
typedef GLXWindow (*PFNGLXCREATEWINDOWPROC)(Display*, GLXFBConfig, Window, const int*);
typedef void (*PFNGLXDESTROYWINDOWPROC)(Display*, GLXWindow);
typedef void (*PFNGLXMAKECONTEXTCURRENTPROC)(Display*, GLXDrawable, GLXDrawable, GLXContext);
typedef Bool (*PFNGLXISDIRECT)(Display*,GLXContext);
typedef Bool (*PFNGLXQUERYRENDERERINTEGERMESA)(Display*,int, int, int, unsigned int*);
typedef Bool (*PFNGLXQUERYCURRENTRENDERERINTEGERMESA)(int, unsigned int*);
typedef Bool (*PFNGLXQUERYRENDERERSTRINGMESA)(Display*,int, int, int);
typedef Bool (*PFNGLXQUERYCURRENTRENDERERSTRINGMESA)(int);

#ifndef GLXBadProfileARB
 #define GLXBadProfileARB 13
#endif

NATRON_NAMESPACE_ENTER;

// X11-specific global data
//
struct X11Data
{
    Display*        display;
    int screen;
    Window root;
    // Context for mapping window XIDs to _GLFWwindow pointers
    XContext context;
    // Most recent error code received by X error handler
    int errorCode;
};

struct OSGLContext_glx_dataPrivate
{
    X11Data x11;
    int major, minor;
    int eventBase;
    int errorBase;

    // dlopen handle for libGL.so.1
    void*           handle;

    // GLX 1.3 functions
    PFNGLXGETFBCONFIGSPROC GetFBConfigs;
    PFNGLXGETFBCONFIGATTRIBPROC GetFBConfigAttrib;
    PFNGLXGETCLIENTSTRINGPROC GetClientString;
    PFNGLXQUERYEXTENSIONPROC QueryExtension;
    PFNGLXQUERYVERSIONPROC QueryVersion;
    PFNGLXDESTROYCONTEXTPROC DestroyContext;
    PFNGLXMAKECURRENTPROC MakeCurrent;
    PFNGLXSWAPBUFFERSPROC SwapBuffers;
    PFNGLXQUERYEXTENSIONSSTRINGPROC QueryExtensionsString;
    PFNGLXCREATENEWCONTEXTPROC CreateNewContext;
    PFNGLXGETVISUALFROMFBCONFIGPROC GetVisualFromFBConfig;
    PFNGLXCREATEWINDOWPROC CreateWindow;
    PFNGLXDESTROYWINDOWPROC DestroyWindow;
    PFNGLXMAKECONTEXTCURRENTPROC MakeContextCurrent;
    PFNGLXISDIRECT IsDirect;

    // GLX 1.4 and extension functions
    PFNGLXGETPROCADDRESSPROC GetProcAddress;
    PFNGLXGETPROCADDRESSPROC GetProcAddressARB;
    PFNGLXSWAPINTERVALSGIPROC SwapIntervalSGI;
    PFNGLXSWAPINTERVALEXTPROC SwapIntervalEXT;
    PFNGLXSWAPINTERVALMESAPROC SwapIntervalMESA;
    PFNGLXCREATECONTEXTATTRIBSARBPROC CreateContextAttribsARB;
    PFNGLXQUERYRENDERERINTEGERMESA QueryRendererIntegerMESA;
    PFNGLXQUERYCURRENTRENDERERINTEGERMESA QueryCurrentRendererIntegerMESA;
    PFNGLXQUERYRENDERERSTRINGMESA QueryRendererStringMesa;
    PFNGLXQUERYCURRENTRENDERERSTRINGMESA QueryCurrentRendererStringMesa;

    GLboolean SGI_swap_control;
    GLboolean EXT_swap_control;
    GLboolean MESA_swap_control;
    GLboolean MESA_query_renderer;
    GLboolean ARB_multisample;
    GLboolean ARB_framebuffer_sRGB;
    GLboolean EXT_framebuffer_sRGB;
    GLboolean ARB_create_context;
    GLboolean ARB_create_context_profile;
    GLboolean ARB_create_context_robustness;
    GLboolean EXT_create_context_es2_profile;
    GLboolean ARB_context_flush_control;
};

OSGLContext_glx_data::OSGLContext_glx_data()
    : _imp(new OSGLContext_glx_dataPrivate())
{

}

OSGLContext_glx_data::~OSGLContext_glx_data()
{

}


struct OSGLContext_x11Private
{
    // X11-specific per-window data
    //
    struct X11Window
    {
        Colormap colormap;
        Window handle;

        X11Window()
            : colormap(0)
            , handle(0)
        {}
    };

    GLXContext glxContextHandle;
    GLXWindow glxWindowHandle;
    X11Window x11Window;

    OSGLContext_x11Private()
        : glxContextHandle(0)
          , glxWindowHandle(0)
          , x11Window()
    {

    }

    void createContextGLX(OSGLContext_glx_data* glxInfo, const FramebufferConfig& fbconfig, int major, int minor, bool coreProfile, int rendererID, const OSGLContext_x11* shareContext);

    void createWindow(OSGLContext_glx_data* glxInfo,
                      Visual* visual, int depth);
};


static bool
extensionSupported(const OSGLContext_glx_data* data,
                   const char* extension)
{
    const char* extensions = data->_imp->QueryExtensionsString(data->_imp->x11.display, data->_imp->x11.screen);

    if (extensions) {
        if ( OSGLContext::stringInExtensionString(extension, extensions) ) {
            return true;
        }
    }

    return false;
}

typedef void (*GLPFNglproc)(void);

static GLPFNglproc
getProcAddress(const OSGLContext_glx_data* data,
               const char* procname)
{
    if (data->_imp->GetProcAddress) {
        return data->_imp->GetProcAddress( (const GLubyte*) procname );
    } else if (data->_imp->GetProcAddressARB) {
        return data->_imp->GetProcAddressARB( (const GLubyte*) procname );
    } else {
        return (GLPFNglproc)dlsym(data->_imp->handle, procname);
    }
}

void
OSGLContext_x11::initGLXData(OSGLContext_glx_data* glxInfo)
{
    //Sets all bits to 0
    memset(glxInfo, 0, sizeof(OSGLContext_glx_data));

    XInitThreads();
    glxInfo->_imp->x11.display = XOpenDisplay(NULL);
    if (!glxInfo->_imp->x11.display) {
        assert(false);
        throw std::runtime_error("X11: Failed to open display");
    }
    glxInfo->_imp->x11.screen = DefaultScreen(glxInfo->_imp->x11.display);
    glxInfo->_imp->x11.root = RootWindow(glxInfo->_imp->x11.display, glxInfo->_imp->x11.screen);
    glxInfo->_imp->x11.context = XUniqueContext();


    const char* sonames[] =
    {
#if defined(__CYGWIN__)
        "libGL-1.so",
#else
        "libGL.so.1",
        "libGL.so",
#endif
        NULL
    };

    for (int i = 0; sonames[i]; ++i) {
        glxInfo->_imp->handle = dlopen(sonames[i], RTLD_LAZY | RTLD_GLOBAL);
        if (glxInfo->_imp->handle) {
            break;
        }
    }

    if (!glxInfo->_imp->handle) {
        assert(false);
        throw std::runtime_error("GLX: Failed to load GLX");
    }

    glxInfo->_imp->GetFBConfigs = (PFNGLXGETFBCONFIGSPROC)dlsym(glxInfo->_imp->handle, "glXGetFBConfigs");
    glxInfo->_imp->GetFBConfigAttrib = (PFNGLXGETFBCONFIGATTRIBPROC)dlsym(glxInfo->_imp->handle, "glXGetFBConfigAttrib");
    glxInfo->_imp->GetClientString = (PFNGLXGETCLIENTSTRINGPROC)dlsym(glxInfo->_imp->handle, "glXGetClientString");
    glxInfo->_imp->QueryExtension = (PFNGLXQUERYEXTENSIONPROC)dlsym(glxInfo->_imp->handle, "glXQueryExtension");
    glxInfo->_imp->QueryVersion = (PFNGLXQUERYVERSIONPROC)dlsym(glxInfo->_imp->handle, "glXQueryVersion");
    glxInfo->_imp->DestroyContext = (PFNGLXDESTROYCONTEXTPROC)dlsym(glxInfo->_imp->handle, "glXDestroyContext");
    glxInfo->_imp->MakeCurrent = (PFNGLXMAKECURRENTPROC)dlsym(glxInfo->_imp->handle, "glXMakeCurrent");
    glxInfo->_imp->SwapBuffers = (PFNGLXSWAPBUFFERSPROC)dlsym(glxInfo->_imp->handle, "glXSwapBuffers");
    glxInfo->_imp->QueryExtensionsString = (PFNGLXQUERYEXTENSIONSSTRINGPROC)dlsym(glxInfo->_imp->handle, "glXQueryExtensionsString");
    glxInfo->_imp->CreateNewContext = (PFNGLXCREATENEWCONTEXTPROC)dlsym(glxInfo->_imp->handle, "glXCreateNewContext");
    glxInfo->_imp->CreateWindow = (PFNGLXCREATEWINDOWPROC)dlsym(glxInfo->_imp->handle, "glXCreateWindow");
    glxInfo->_imp->DestroyWindow = (PFNGLXDESTROYWINDOWPROC)dlsym(glxInfo->_imp->handle, "glXDestroyWindow");
    glxInfo->_imp->GetProcAddress = (PFNGLXGETPROCADDRESSPROC)dlsym(glxInfo->_imp->handle, "glXGetProcAddress");
    glxInfo->_imp->GetProcAddressARB = (PFNGLXGETPROCADDRESSPROC)dlsym(glxInfo->_imp->handle, "glXGetProcAddressARB");
    glxInfo->_imp->GetVisualFromFBConfig = (PFNGLXGETVISUALFROMFBCONFIGPROC)dlsym(glxInfo->_imp->handle, "glXGetVisualFromFBConfig");
    glxInfo->_imp->MakeContextCurrent = (PFNGLXMAKECONTEXTCURRENTPROC)dlsym(glxInfo->_imp->handle, "glXMakeContextCurrent");
    glxInfo->_imp->IsDirect = (PFNGLXISDIRECT)dlsym(glxInfo->_imp->handle, "glXIsDirect");

    if ( !glxInfo->_imp->QueryExtension(glxInfo->_imp->x11.display, &glxInfo->_imp->errorBase, &glxInfo->_imp->eventBase) ) {
        throw std::runtime_error("GLX: GLX extension not found");
    }

    if ( !glxInfo->_imp->QueryVersion(glxInfo->_imp->x11.display, &glxInfo->_imp->major, &glxInfo->_imp->minor) ) {
        throw std::runtime_error("GLX: Failed to query GLX version");
    }

    if ( (glxInfo->_imp->major == 1) && (glxInfo->_imp->minor < 3) ) {
        throw std::runtime_error("GLX: GLX version 1.3 is required");
    }

    if ( extensionSupported(glxInfo, "GLX_EXT_swap_control") ) {
        glxInfo->_imp->SwapIntervalEXT = (PFNGLXSWAPINTERVALEXTPROC)getProcAddress(glxInfo, "glXSwapIntervalEXT");
        if (glxInfo->_imp->SwapIntervalEXT) {
            glxInfo->_imp->EXT_swap_control = GL_TRUE;
        }
    }

    if ( extensionSupported(glxInfo, "GLX_SGI_swap_control") ) {
        glxInfo->_imp->SwapIntervalSGI = (PFNGLXSWAPINTERVALSGIPROC)getProcAddress(glxInfo, "glXSwapIntervalSGI");

        if (glxInfo->_imp->SwapIntervalSGI) {
            glxInfo->_imp->SGI_swap_control = GL_TRUE;
        }
    }

    if ( extensionSupported(glxInfo, "GLX_MESA_swap_control") ) {
        glxInfo->_imp->SwapIntervalMESA = (PFNGLXSWAPINTERVALMESAPROC)getProcAddress(glxInfo, "glXSwapIntervalMESA");

        if (glxInfo->_imp->SwapIntervalMESA) {
            glxInfo->_imp->MESA_swap_control = GL_TRUE;
        }
    }

    if ( extensionSupported(glxInfo, "GLX_MESA_query_renderer") ) {
        glxInfo->_imp->QueryRendererIntegerMESA = (PFNGLXQUERYRENDERERINTEGERMESA)getProcAddress(glxInfo, "glXQueryRendererIntegerMESA");
        glxInfo->_imp->QueryCurrentRendererIntegerMESA = (PFNGLXQUERYCURRENTRENDERERINTEGERMESA)getProcAddress(glxInfo, "glXQueryCurrentRendererIntegerMESA");
        glxInfo->_imp->QueryRendererStringMesa = (PFNGLXQUERYRENDERERSTRINGMESA)getProcAddress(glxInfo, "glXQueryRendererStringMESA");
        glxInfo->_imp->QueryCurrentRendererStringMesa = (PFNGLXQUERYCURRENTRENDERERSTRINGMESA)getProcAddress(glxInfo, "glXQueryCurrentRendererStringMESA");
        if (glxInfo->_imp->QueryRendererIntegerMESA) {
            glxInfo->_imp->MESA_query_renderer = GL_TRUE;
        }
    }

    if ( extensionSupported(glxInfo, "GLX_ARB_multisample") ) {
        glxInfo->_imp->ARB_multisample = GL_TRUE;
    }

    if ( extensionSupported(glxInfo, "GLX_ARB_framebuffer_sRGB") ) {
        glxInfo->_imp->ARB_framebuffer_sRGB = GL_TRUE;
    }

    if ( extensionSupported(glxInfo, "GLX_EXT_framebuffer_sRGB") ) {
        glxInfo->_imp->EXT_framebuffer_sRGB = GL_TRUE;
    }

    if ( extensionSupported(glxInfo, "GLX_ARB_create_context") ) {
        glxInfo->_imp->CreateContextAttribsARB = (PFNGLXCREATECONTEXTATTRIBSARBPROC)getProcAddress(glxInfo, "glXCreateContextAttribsARB");

        if (glxInfo->_imp->CreateContextAttribsARB) {
            glxInfo->_imp->ARB_create_context = GL_TRUE;
        }
    }

    if ( extensionSupported(glxInfo, "GLX_ARB_create_context_robustness") ) {
        glxInfo->_imp->ARB_create_context_robustness = GL_TRUE;
    }

    if ( extensionSupported(glxInfo, "GLX_ARB_create_context_profile") ) {
        glxInfo->_imp->ARB_create_context_profile = GL_TRUE;
    }

    if ( extensionSupported(glxInfo, "GLX_EXT_create_context_es2_profile") ) {
        glxInfo->_imp->EXT_create_context_es2_profile = GL_TRUE;
    }

    if ( extensionSupported(glxInfo, "GLX_ARB_context_flush_control") ) {
        glxInfo->_imp->ARB_context_flush_control = GL_TRUE;
    }
} // initGLXData

void
OSGLContext_x11::destroyGLXData(OSGLContext_glx_data* glxInfo)
{
    if (glxInfo->_imp->handle) {
        dlclose(glxInfo->_imp->handle);
        glxInfo->_imp->handle = 0;
    }
}

// Returns the specified attribute of the specified GLXFBConfig
//
static int
getFBConfigAttrib(const OSGLContext_glx_data* glxInfo,
                  const GLXFBConfig& fbconfig,
                  int attrib)
{
    int value;

    glxInfo->_imp->GetFBConfigAttrib(glxInfo->_imp->x11.display, fbconfig, attrib, &value);

    return value;
}

// Return a list of available and usable framebuffer configs
//
static void
chooseFBConfig(const OSGLContext_glx_data* glxInfo,
               const FramebufferConfig& desired,
               GLXFBConfig* result)
{
    bool trustWindowBit = true;
    // HACK: This is a (hopefully temporary) workaround for Chromium
    //       (VirtualBox GL) not setting the window bit on any GLXFBConfigs
    const char* vendor = glxInfo->_imp->GetClientString(glxInfo->_imp->x11.display, GLX_VENDOR);

    if (strcmp(vendor, "Chromium") == 0) {
        trustWindowBit = false;
    }

    int nativeCount;
    GLXFBConfig* nativeConfigs = glxInfo->_imp->GetFBConfigs(glxInfo->_imp->x11.display, glxInfo->_imp->x11.screen, &nativeCount);
    if (!nativeCount) {
        throw std::runtime_error("GLX: No GLXFBConfigs returned");
    }

    std::vector<FramebufferConfig> usableConfigs(nativeCount);
    int usableCount = 0;

    for (int i = 0; i < nativeCount; ++i) {
        const GLXFBConfig& n = nativeConfigs[i];
        FramebufferConfig& u = usableConfigs[usableCount];

        // Only consider RGBA GLXFBConfigs
        if ( !(getFBConfigAttrib(glxInfo, n, GLX_RENDER_TYPE) & GLX_RGBA_BIT) ) {
            continue;
        }

        // Only consider window GLXFBConfigs
        if ( !(getFBConfigAttrib(glxInfo, n, GLX_DRAWABLE_TYPE) & GLX_WINDOW_BIT) ) {
            if (trustWindowBit) {
                continue;
            }
        }

        u.redBits = getFBConfigAttrib(glxInfo, n, GLX_RED_SIZE);
        u.greenBits = getFBConfigAttrib(glxInfo, n, GLX_GREEN_SIZE);
        u.blueBits = getFBConfigAttrib(glxInfo, n, GLX_BLUE_SIZE);

        u.alphaBits = getFBConfigAttrib(glxInfo, n, GLX_ALPHA_SIZE);
        u.depthBits = getFBConfigAttrib(glxInfo, n, GLX_DEPTH_SIZE);
        u.stencilBits = getFBConfigAttrib(glxInfo, n, GLX_STENCIL_SIZE);

        u.accumRedBits = getFBConfigAttrib(glxInfo, n, GLX_ACCUM_RED_SIZE);
        u.accumGreenBits = getFBConfigAttrib(glxInfo, n, GLX_ACCUM_GREEN_SIZE);
        u.accumBlueBits = getFBConfigAttrib(glxInfo, n, GLX_ACCUM_BLUE_SIZE);
        u.accumAlphaBits = getFBConfigAttrib(glxInfo, n, GLX_ACCUM_ALPHA_SIZE);

        u.auxBuffers = getFBConfigAttrib(glxInfo, n, GLX_AUX_BUFFERS);

        if ( getFBConfigAttrib(glxInfo, n, GLX_STEREO) ) {
            u.stereo = GL_TRUE;
        }
        if ( getFBConfigAttrib(glxInfo, n, GLX_DOUBLEBUFFER) ) {
            u.doublebuffer = GL_TRUE;
        }

        if (glxInfo->_imp->ARB_multisample) {
            u.samples = getFBConfigAttrib(glxInfo, n, GLX_SAMPLES);
        }

        if (glxInfo->_imp->ARB_framebuffer_sRGB || glxInfo->_imp->EXT_framebuffer_sRGB) {
            u.sRGB = getFBConfigAttrib(glxInfo, n, GLX_FRAMEBUFFER_SRGB_CAPABLE_ARB);
        }

        u.handle = (uintptr_t) n;
        usableCount++;
    }

    const FramebufferConfig& closest = OSGLContext::chooseFBConfig(desired, usableConfigs, usableCount);
    *result = (GLXFBConfig) closest.handle;

    XFree(nativeConfigs);
} // chooseFBConfig

// Returns the Visual and depth of the chosen GLXFBConfig
//
static void
ChooseVisualGLX(OSGLContext_glx_data* glxInfo,
                const FramebufferConfig& fbconfig,
                Visual** visual,
                int* depth)
{
    GLXFBConfig native;

    chooseFBConfig(glxInfo, fbconfig, &native);

    XVisualInfo* result = glxInfo->_imp->GetVisualFromFBConfig(glxInfo->_imp->x11.display, native);
    if (!result) {
        throw std::runtime_error("GLX: Failed to retrieve Visual for GLXFBConfig");
    }

    *visual = result->visual;
    *depth = result->depth;

    XFree(result);
}

// X error handler
//
static int errorHandler(Display */*display*/, XErrorEvent* event)
{
   // _glfw.x11.errorCode = event->error_code;
    return 0;
}


// Sets the X error handler callback
//
static void
GrabErrorHandlerX11(OSGLContext_glx_data* glxInfo)
{
    glxInfo->_imp->x11.errorCode = Success;
    XSetErrorHandler(errorHandler);
}

// Clears the X error handler callback
//
static void
ReleaseErrorHandlerX11(OSGLContext_glx_data* glxInfo)
{
    // Synchronize to make sure all commands are processed
    XSync(glxInfo->_imp->x11.display, False);
    XSetErrorHandler(NULL);
}

// Create the X11 window (and its colormap)
//
void
OSGLContext_x11Private::createWindow(OSGLContext_glx_data* glxInfo,
                              Visual* visual,
                              int depth)
{
    const int width = 32;
    const int height = 32;

    // Every window needs a colormap
    // Create one based on the visual used by the current context
    // TODO: Decouple this from context creation

    x11Window.colormap = XCreateColormap(glxInfo->_imp->x11.display,  glxInfo->_imp->x11.root, visual, AllocNone);

    // Create the actual window
    {
        XSetWindowAttributes wa;
        const unsigned long wamask = CWBorderPixel | CWColormap | CWEventMask;

        wa.colormap = x11Window.colormap;
        wa.border_pixel = 0;
        wa.event_mask = StructureNotifyMask | KeyPressMask | KeyReleaseMask |
                        PointerMotionMask | ButtonPressMask | ButtonReleaseMask |
                        ExposureMask | FocusChangeMask | VisibilityChangeMask |
                        EnterWindowMask | LeaveWindowMask | PropertyChangeMask;

        GrabErrorHandlerX11(glxInfo);


        x11Window.handle = XCreateWindow(glxInfo->_imp->x11.display,
                                          glxInfo->_imp->x11.root,
                                          0, 0,
                                          width, height,
                                          0,      // Border width
                                          depth,  // Color depth
                                          InputOutput,
                                          visual,
                                          wamask,
                                          &wa);

        ReleaseErrorHandlerX11(glxInfo);

        if (!x11Window.handle) {
            throw std::runtime_error("X11: Failed to create window");
        }

        /*XSaveContext(glxInfo->x11.display,
                     _x11Window.handle,
                     glxInfo->x11.context,
                     (XPointer) window);*/
    }

    /*if (!wndconfig->decorated)
       {
        struct
        {
            unsigned long flags;
            unsigned long functions;
            unsigned long decorations;
            long input_mode;
            unsigned long status;
        } hints;

        hints.flags = 2;       // Set decorations
        hints.decorations = 0; // No decorations

        XChangeProperty(_glfw.x11.display, window->x11.handle,
                        _glfw.x11.MOTIF_WM_HINTS,
                        _glfw.x11.MOTIF_WM_HINTS, 32,
                        PropModeReplace,
                        (unsigned char*) &hints,
       sizeof(hints) / sizeof(long));
       }*/

    /*{
        XSizeHints* hints = XAllocSizeHints();
        hints->flags |= (PMinSize | PMaxSize);
        hints->min_width  = hints->max_width  = width;
        hints->min_height = hints->max_height = height;
        XSetWMNormalHints(glxInfo->x11.display, _x11Window.handle, hints);
        XFree(hints);
       }*/
} // createWindow

#define setGLXattrib(attribName, attribValue) \
    { \
        attribs[index++] = attribName; \
        attribs[index++] = attribValue; \
        assert( (size_t) index < sizeof(attribs) / sizeof(attribs[0]) ); \
    }

// Create the OpenGL context using legacy API
//
static GLXContext
createLegacyContext(OSGLContext_glx_data* glxInfo,
                    GLXFBConfig fbconfig,
                    GLXContext share)
{
    return glxInfo->_imp->CreateNewContext(glxInfo->_imp->x11.display, fbconfig, GLX_RGBA_TYPE, share, True);
}

void
OSGLContext_x11Private::createContextGLX(OSGLContext_glx_data* glxInfo,
                                  const FramebufferConfig& fbconfig,
                                  int major,
                                  int minor,
                                  bool coreProfile,
                                  int rendererID,
                                  const OSGLContext_x11* shareContext)
{
    int attribs[40];
    GLXFBConfig native = NULL;
    GLXContext share = shareContext ? shareContext->_imp->glxContextHandle : 0;

    chooseFBConfig(glxInfo, fbconfig, &native);

    /*if (ctxconfig->client == GLFW_OPENGL_ES_API)
       {
        if (!_glfw.glx.ARB_create_context ||
            !_glfw.glx.ARB_create_context_profile ||
            !_glfw.glx.EXT_create_context_es2_profile)
        {
            _glfwInputError(GLFW_API_UNAVAILABLE,
                            "GLX: OpenGL ES requested but GLX_EXT_create_context_es2_profile is unavailable");
            return GLFW_FALSE;
        }
       }*/

    /*if (ctxconfig->forward)
       {
        if (!_glfw.glx.ARB_create_context)
        {
            _glfwInputError(GLFW_VERSION_UNAVAILABLE,
                            "GLX: Forward compatibility requested but GLX_ARB_create_context_profile is unavailable");
            return GLFW_FALSE;
        }
       }

       if (ctxconfig->profile)
       {
        if (!_glfw.glx.ARB_create_context ||
            !_glfw.glx.ARB_create_context_profile)
        {
            _glfwInputError(GLFW_VERSION_UNAVAILABLE,
                            "GLX: An OpenGL profile requested but GLX_ARB_create_context_profile is unavailable");
            return GLFW_FALSE;
        }
       }*/

    GrabErrorHandlerX11(glxInfo);

    if (!glxInfo->_imp->ARB_create_context) {
        glxContextHandle = createLegacyContext(glxInfo, native, share);
    } else {
        int index = 0, mask = 0, flags = 0;

        /*if (ctxconfig->forward)
            flags |= GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB;*/

        if (coreProfile) {
            mask |= GLX_CONTEXT_CORE_PROFILE_BIT_ARB;
        } else {
            mask |= GLX_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB;
        }

        /*if (ctxconfig->debug)
            flags |= GLX_CONTEXT_DEBUG_BIT_ARB;
           if (ctxconfig->noerror)
            flags |= GL_CONTEXT_FLAG_NO_ERROR_BIT_KHR;*/

        /*if (ctxconfig->robustness)
           {
            if (_glfw.glx.ARB_create_context_robustness)
            {
                if (ctxconfig->robustness == GLFW_NO_RESET_NOTIFICATION)
                {
                    setGLXattrib(GLX_CONTEXT_RESET_NOTIFICATION_STRATEGY_ARB,
                                 GLX_NO_RESET_NOTIFICATION_ARB);
                }
                else if (ctxconfig->robustness == GLFW_LOSE_CONTEXT_ON_RESET)
                {
                    setGLXattrib(GLX_CONTEXT_RESET_NOTIFICATION_STRATEGY_ARB,
                                 GLX_LOSE_CONTEXT_ON_RESET_ARB);
                }

                flags |= GLX_CONTEXT_ROBUST_ACCESS_BIT_ARB;
            }
           }*/

        /*if (ctxconfig->release)
           {
            if (_glfw.glx.ARB_context_flush_control)
            {
                if (ctxconfig->release == GLFW_RELEASE_BEHAVIOR_NONE)
                {
                    setGLXattrib(GLX_CONTEXT_RELEASE_BEHAVIOR_ARB,
                                 GLX_CONTEXT_RELEASE_BEHAVIOR_NONE_ARB);
                }
                else if (ctxconfig->release == GLFW_RELEASE_BEHAVIOR_FLUSH)
                {
                    setGLXattrib(GLX_CONTEXT_RELEASE_BEHAVIOR_ARB,
                                 GLX_CONTEXT_RELEASE_BEHAVIOR_FLUSH_ARB);
                }
            }
           }*/

        // NOTE: Only request an explicitly versioned context when necessary, as
        //       explicitly requesting version 1.0 does not always return the
        //       highest version supported by the driver
        if ( (major != 1) || (minor != 0) ) {
            setGLXattrib(GLX_CONTEXT_MAJOR_VERSION_ARB, major);
            setGLXattrib(GLX_CONTEXT_MINOR_VERSION_ARB, minor);
        }

        if (mask) {
            setGLXattrib(GLX_CONTEXT_PROFILE_MASK_ARB, mask);
        }

        if (flags) {
            setGLXattrib(GLX_CONTEXT_FLAGS_ARB, flags);
        }

        if (glxInfo->_imp->MESA_query_renderer && rendererID != -1) {
            setGLXattrib(GLX_RENDERER_ID_MESA, rendererID);
        }

        setGLXattrib(None, None);

        glxContextHandle = glxInfo->_imp->CreateContextAttribsARB(glxInfo->_imp->x11.display, native, share, True /*direct*/, attribs);

        // HACK: This is a fallback for broken versions of the Mesa
        //       implementation of GLX_ARB_create_context_profile that fail
        //       default 1.0 context creation with a GLXBadProfileARB error in
        //       violation of the extension spec
        if (!glxContextHandle) {
            if (glxInfo->_imp->x11.errorCode == glxInfo->_imp->errorBase + GLXBadProfileARB
                /*&& ctxconfig->client == GLFW_OPENGL_API &&
                   ctxconfig->profile == GLFW_OPENGL_ANY_PROFILE &&
                   ctxconfig->forward == GLFW_FALSE*/) {
                glxContextHandle = createLegacyContext(glxInfo, native, share);
            }
        }
    }


    ReleaseErrorHandlerX11(glxInfo);

    if (!glxContextHandle) {
        throw std::runtime_error("GLX: Failed to create context");
    }


    glxWindowHandle = glxInfo->_imp->CreateWindow(glxInfo->_imp->x11.display, native, x11Window.handle, NULL);
    if (!glxWindowHandle) {
        glxInfo->_imp->DestroyContext(glxInfo->_imp->x11.display, glxContextHandle);
        glxContextHandle = 0;
        throw std::runtime_error("GLX: Failed to create window");
    }

    if (!glxInfo->_imp->IsDirect(glxInfo->_imp->x11.display, glxContextHandle)) {
        glxInfo->_imp->DestroyContext(glxInfo->_imp->x11.display, glxContextHandle);
        glxContextHandle = 0;
        glxInfo->_imp->DestroyWindow(glxInfo->_imp->x11.display, glxWindowHandle);
        glxWindowHandle = 0;
        throw std::runtime_error("GLX: Created context is not doing direct rendering");
    }

    /*_window = glxInfo->CreateWindow(glxInfo->display, native, window->x11.handle, NULL);
       if (!_window) {
        throw std::runtime_error("GLX: Failed to create window");
       }*/
} // createContextGLX

#undef setGLXattrib


OSGLContext_x11::OSGLContext_x11(const FramebufferConfig& pixelFormatAttrs,
                                 int major,
                                 int minor,
                                 bool coreProfile,
                                 int rendererID,
                                 const OSGLContext_x11* shareContext)
    : _imp(new OSGLContext_x11Private())
{
    OSGLContext_glx_data* glxInfo = const_cast<OSGLContext_glx_data*>(appPTR->getGLXData());

    assert(glxInfo);

    Visual* visual;
    int depth;

    ChooseVisualGLX(glxInfo, pixelFormatAttrs, &visual, &depth);
    _imp->createWindow(glxInfo, visual, depth);
    _imp->createContextGLX(glxInfo, pixelFormatAttrs, major, minor, coreProfile, rendererID, shareContext);
}

OSGLContext_x11::~OSGLContext_x11()
{
    const OSGLContext_glx_data* glxInfo = appPTR->getGLXData();

    assert(glxInfo);


    if (_imp->glxWindowHandle) {
        glxInfo->_imp->DestroyWindow(glxInfo->_imp->x11.display, _imp->glxWindowHandle);
        _imp->glxWindowHandle = 0;
    }
    if (_imp->glxContextHandle) {
        glxInfo->_imp->DestroyContext(glxInfo->_imp->x11.display, _imp->glxContextHandle);
        _imp->glxContextHandle = 0;
    }
}

bool
OSGLContext_x11::makeContextCurrent(const OSGLContext_x11* context)
{
    const OSGLContext_glx_data* glxInfo = appPTR->getGLXData();

    assert(glxInfo);

    if (context) {
        return glxInfo->_imp->MakeCurrent(glxInfo->_imp->x11.display, context->_imp->glxWindowHandle, context->_imp->glxContextHandle);
    } else {
        return glxInfo->_imp->MakeCurrent(glxInfo->_imp->x11.display, None, NULL);
    }
}

void
OSGLContext_x11::swapBuffers()
{
    const OSGLContext_glx_data* glxInfo = appPTR->getGLXData();

    assert(glxInfo);
    glxInfo->_imp->SwapBuffers(glxInfo->_imp->x11.display, _imp->glxWindowHandle);
}

void
OSGLContext_x11::swapInterval(int interval)
{
    const OSGLContext_glx_data* glxInfo = appPTR->getGLXData();

    assert(glxInfo);

    if (glxInfo->_imp->EXT_swap_control) {
        glxInfo->_imp->SwapIntervalEXT(glxInfo->_imp->x11.display, _imp->glxWindowHandle, interval);
    } else if (glxInfo->_imp->MESA_swap_control) {
        glxInfo->_imp->SwapIntervalMESA(interval);
    } else if (glxInfo->_imp->SGI_swap_control) {
        if (interval > 0) {
            glxInfo->_imp->SwapIntervalSGI(interval);
        }
    }
}

void
OSGLContext_x11::getGPUInfos(std::list<OpenGLRendererInfo>& renderers)
{
    const OSGLContext_glx_data* glxInfo = appPTR->getGLXData();
    assert(glxInfo);
    if (!glxInfo->_imp->MESA_query_renderer) {
        boost::scoped_ptr<OSGLContext_x11> context;
        try {
            context.reset(new OSGLContext_x11(FramebufferConfig(), GLVersion.major, GLVersion.minor, false, -1, 0));
        } catch (const std::exception& e) {
            std::cerr << e.what() << std::endl;
        }
        if (!makeContextCurrent(context.get())) {
            return;
        }

        OpenGLRendererInfo info;
        info.vendorName = std::string((const char *) glGetString(GL_VENDOR));
        info.rendererName = std::string(const char *) glGetString(GL_RENDERER));
        info.glVersionString = std::string(const char *) glGetString(GL_VERSION));
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &info.maxTextureSize);
        // We don't have any way to get memory size, set it to 0
        info.maxMemBytes = 0;
        info.maxTexMemBytes = 0;
        info.rendererID = -1;
        renderers.push_back(info);
    } else {
        // Just use the first screen
        const int screen = 0;
        // The function QueryRendererIntegerMESA can return at most 3 values:  https://www.opengl.org/registry/specs/MESA/glx_query_renderer.txt
        unsigned int v[3];
        int renderer = 0;
        bool gotRenderer;
        do {
            gotRenderer = (bool)glxInfo->_imp->QueryRendererIntegerMESA(glxInfo->_imp->x11.display, screen, renderer, GLX_RENDERER_DEVICE_ID_MESA, v);
            ++renderer;
            if (gotRenderer) {
                int rendererID = v[0];
                if (rendererID == 0xFFFFFFFF) {
                    gotRenderer = false;
                } else {
                    bool ok = glxInfo->_imp->QueryRendererIntegerMESA(glxInfo->_imp->x11.display, screen, renderer, GLX_RENDERER_ACCELERATED_MESA, v)
                    assert(ok);
                    if (!v[0]) {
                        continue;
                    }

                    OpenGLRendererInfo info;

                    ok = glxInfo->_imp->QueryRendererIntegerMESA(glxInfo->_imp->x11.display, screen, renderer, GLX_RENDERER_VIDEO_MEMORY_MESA, v)
                    assert(ok);
                    info.maxMemBytes = info.maxTexMemBytes = v[0] * 1e6;

                    // Now create a context with the renderer ID
                    boost::scoped_ptr<OSGLContext_x11> context;
                    try {
                        context.reset(new OSGLContext_x11(FramebufferConfig(), GLVersion.major, GLVersion.minor, false, rendererID, 0));
                    } catch (const std::exception& e) {
                        std::cerr << e.what() << std::endl;
                    }
                    if (!makeContextCurrent(context.get())) {
                        continue;
                    }

                    info.rendererID = rendererID;
                    info.vendorName = std::string((const char *) glGetString(GL_VENDOR));
                    info.rendererName = std::string(const char *) glGetString(GL_RENDERER));
                    info.glVersionString = std::string(const char *) glGetString(GL_VERSION));
                    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &info.maxTextureSize);

                    renderers.push_back(info);
                }
            }
        } while (gotRenderer);
    }
}


NATRON_NAMESPACE_EXIT;

#endif // __NATRON_LINUX__
