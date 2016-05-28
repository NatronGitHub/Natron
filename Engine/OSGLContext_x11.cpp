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


#include "OSGLContext_x11.h"

#ifdef __NATRON_LINUX__

#include "Engine/OSGLContext.h"


static bool extensionSupported(const OSGLContext_glx_data* data, const char* extension)
{
    const char* extensions = glXQueryExtensionsString(data->x11.display, data->x11.screen);
    if (extensions) {
        if (OSGLContext::(extension, extensions)) {
            return true;
        }
    }

    return false;
}

static GLFWglproc getProcAddress(const OSGLContext_glx_data* data, const char* procname)
{
    if (data->GetProcAddress)
        return data->GetProcAddress((const GLubyte*) procname);
    else if (data->GetProcAddressARB)
        return data->GetProcAddressARB((const GLubyte*) procname);
    else
        return dlsym(data->handle, procname);
}

void
OSGLContext_x11::initGLXData(const OSGLContext_glx_data* glxInfo)
{

    XInitThreads();
    glxInfo->x11.display = XOpenDisplay(NULL);
    if (!glxInfo->x11.display) {
        assert(false);
        throw std::runtime_error("X11: Failed to open display");
    }
    glxInfo->x11.screen = DefaultScreen(glxInfo->x11.display);
    glxInfo->x11.root = RootWindow(glxInfo->x11.display, glxInfo->x11.screen);
    glxInfo->x11.context = XUniqueContext();


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

    for (int i = 0;  sonames[i];  ++i) {
        glxInfo->handle = dlopen(sonames[i], RTLD_LAZY | RTLD_GLOBAL);
        if (glxInfo->handle) {
            break;
        }
    }

    if (!glxInfo->handle) {
        assert(false);
        throw std::runtime_error("GLX: Failed to load GLX");
    }

    glxInfo->GetFBConfigs = dlsym(glxInfo->handle, "glXGetFBConfigs");
    glxInfo->GetFBConfigAttrib = dlsym(glxInfo->handle, "glXGetFBConfigAttrib");
    glxInfo->GetClientString = dlsym(glxInfo->handle, "glXGetClientString");
    glxInfo->QueryExtension = dlsym(glxInfo->handle, "glXQueryExtension");
    glxInfo->QueryVersion = dlsym(glxInfo->handle, "glXQueryVersion");
    glxInfo->DestroyContext = dlsym(glxInfo->handle, "glXDestroyContext");
    glxInfo->MakeCurrent = dlsym(glxInfo->handle, "glXMakeCurrent");
    glxInfo->SwapBuffers = dlsym(glxInfo->handle, "glXSwapBuffers");
    glxInfo->QueryExtensionsString = dlsym(glxInfo->handle, "glXQueryExtensionsString");
    glxInfo->CreateNewContext = dlsym(glxInfo->handle, "glXCreateNewContext");
    glxInfo->CreateWindow = dlsym(glxInfo->handle, "glXCreateWindow");
    glxInfo->DestroyWindow = dlsym(glxInfo->handle, "glXDestroyWindow");
    glxInfo->GetProcAddress = dlsym(glxInfo->handle, "glXGetProcAddress");
    glxInfo->GetProcAddressARB = dlsym(glxInfo->handle, "glXGetProcAddressARB");
    glxInfo->GetVisualFromFBConfig = dlsym(glxInfo->handle, "glXGetVisualFromFBConfig");
    glxInfo->CreatePBuffer  = dlsym(glxInfo->handle, "glXCreatePbuffer");
    glxInfo->DestroyPBuffer  = dlsym(glxInfo->handle, "glXDestroyPbuffer");
    glxInfo->MakeContextCurrent = dlsym(glxInfo->handle, "glXMakeContextCurrent");

    if (!glxInfo->QueryExtension(glxInfo->x11.display, &glxInfo->errorBase, &glxInfo->eventBase)) {
        throw std::runtime_error("GLX: GLX extension not found");
    }

    if (!glXQueryVersion(glxInfo->x11.display, &glxInfo->major, &glxInfo->minor)) {
        throw std::runtime_error("GLX: Failed to query GLX version");
    }

    if (glxInfo->major == 1 && glxInfo->minor < 3) {
        throw std::runtime_error("GLX: GLX version 1.3 is required");
    }

    if (extensionSupported("GLX_EXT_swap_control")) {
        glxInfo->SwapIntervalEXT = (PFNGLXSWAPINTERVALEXTPROC)getProcAddress("glXSwapIntervalEXT");
        if (glxInfo->SwapIntervalEXT) {
            glxInfo->EXT_swap_control = GL_TRUE;
        }
    }

    if (extensionSupported("GLX_SGI_swap_control"))
    {
        glxInfo->SwapIntervalSGI = (PFNGLXSWAPINTERVALSGIPROC)getProcAddress("glXSwapIntervalSGI");

        if (glxInfo->SwapIntervalSGI) {
            glxInfo->SGI_swap_control = GL_TRUE;
        }
    }

    if (extensionSupported("GLX_MESA_swap_control"))
    {
        glxInfo->SwapIntervalMESA = (PFNGLXSWAPINTERVALMESAPROC)getProcAddress("glXSwapIntervalMESA");

        if (glxInfo->SwapIntervalMESA) {
            glxInfo->MESA_swap_control = GL_TRUE;
        }
    }

    if (extensionSupported("GLX_ARB_multisample"))
        glxInfo->ARB_multisample = GL_TRUE;

    if (extensionSupported("GLX_ARB_framebuffer_sRGB"))
        glxInfo->ARB_framebuffer_sRGB = GL_TRUE;

    if (extensionSupported("GLX_EXT_framebuffer_sRGB"))
        glxInfo->EXT_framebuffer_sRGB = GL_TRUE;

    if (extensionSupported("GLX_ARB_create_context"))
    {
        glxInfo->CreateContextAttribsARB = (PFNGLXCREATECONTEXTATTRIBSARBPROC)getProcAddress("glXCreateContextAttribsARB");

        if (glxInfo->CreateContextAttribsARB) {
            glxInfo->ARB_create_context = GL_TRUE;
        }
    }

    if (extensionSupported("GLX_ARB_create_context_robustness"))
        glxInfo->ARB_create_context_robustness = GL_TRUE;

    if (extensionSupported("GLX_ARB_create_context_profile"))
        glxInfo->ARB_create_context_profile = GL_TRUE;

    if (extensionSupported("GLX_EXT_create_context_es2_profile"))
        glxInfo->EXT_create_context_es2_profile = GL_TRUE;

    if (extensionSupported("GLX_ARB_context_flush_control"))
        glxInfo->ARB_context_flush_control = GL_TRUE;
}

void
OSGLContext_x11::destroyGLXData(OSGLContext_glx_data* glxInfo)
{
    if (glxInfo->handle) {
        dlclose(glxInfo->handle);
        glxInfo->handle = 0;
    }
}

// Returns the specified attribute of the specified GLXFBConfig
//
static int getFBConfigAttrib(const OSGLContext_glx_data* glxInfo, const GLXFBConfig& fbconfig, int attrib)
{
    int value;
    glxInfo->GetFBConfigAttrib(glxInfo->x11.display, fbconfig, attrib, &value);
    return value;
}

// Return a list of available and usable framebuffer configs
//
static void chooseFBConfig(const OSGLContext_glx_data* glxInfo, const FramebufferConfig& desired, GLXFBConfig* result)
{

    bool trustWindowBit = true;
    // HACK: This is a (hopefully temporary) workaround for Chromium
    //       (VirtualBox GL) not setting the window bit on any GLXFBConfigs
    const char* vendor = glxInfo->GetClientString(glxInfo->x11.display, GLX_VENDOR);
    if (strcmp(vendor, "Chromium") == 0) {
        trustWindowBit = false;
    }

    int nativeCount;
    GLXFBConfig* nativeConfigs = glxInfo->GetFBConfigs(glxInfo->x11.display, glxInfo->x11.screen, &nativeCount);
    if (!nativeCount) {
        throw std::runtime_error("GLX: No GLXFBConfigs returned");
    }

    std::vector<FramebufferConfig> usableConfigs(nativeCount);
    int usableCount = 0;

    for (int i = 0;  i < nativeCount;  ++i) {
        const GLXFBConfig& n = nativeConfigs[i];
        FramebufferConfig& u = usableConfigs[usableCount];

        // Only consider RGBA GLXFBConfigs
        if (!(getFBConfigAttrib(glxInfo, n, GLX_RENDER_TYPE) & GLX_RGBA_BIT))
            continue;

        // Only consider window GLXFBConfigs
        if (!(getFBConfigAttrib(glxInfo, n, GLX_DRAWABLE_TYPE) & GLX_WINDOW_BIT)) {
            if (trustWindowBit)
                continue;
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

        if (getFBConfigAttrib(glxInfo, n, GLX_STEREO))
            u.stereo = GLFW_TRUE;
        if (getFBConfigAttrib(glxInfo, n, GLX_DOUBLEBUFFER))
            u.doublebuffer = GLFW_TRUE;

        if (_glfw.glx.ARB_multisample)
            u.samples = getFBConfigAttrib(glxInfo, n, GLX_SAMPLES);

        if (_glfw.glx.ARB_framebuffer_sRGB || _glfw.glx.EXT_framebuffer_sRGB)
            u.sRGB = getFBConfigAttrib(glxInfo, n, GLX_FRAMEBUFFER_SRGB_CAPABLE_ARB);

        u.handle = (uintptr_t) n;
        usableCount++;
    }

    const FramebufferConfig& closest = OSGLContext::chooseFBConfig(desired, usableConfigs, usableCount);
    *result = (GLXFBConfig) closest->handle;
    
    XFree(nativeConfigs);

}

// Returns the Visual and depth of the chosen GLXFBConfig
//
static void ChooseVisualGLX(OSGLContext_glx_data* glxInfo,
                              const FramebufferConfig& fbconfig,
                              Visual** visual, int* depth)
{
    GLXFBConfig native;
    chooseFBConfig(glxInfo, fbconfig, &native);

    XVisualInfo* result = glxInfo->GetVisualFromFBConfig(glxInfo->x11.display, native);
    if (!result) {
        throw std::runtime_error("GLX: Failed to retrieve Visual for GLXFBConfig");
    }

    *visual = result->visual;
    *depth = result->depth;

    XFree(result);
}



// Sets the X error handler callback
//
static void GrabErrorHandlerX11(OSGLContext_glx_data* glxInfo)
{
    glxInfo->x11.errorCode = Success;
    XSetErrorHandler(errorHandler);
}

// Clears the X error handler callback
//
static void ReleaseErrorHandlerX11(OSGLContext_glx_data* glxInfo)
{
    // Synchronize to make sure all commands are processed
    XSync(glxInfo->x11.display, False);
    XSetErrorHandler(NULL);
}

// Create the X11 window (and its colormap)
//
void
OSGLContext_x11::createWindow(OSGLContext_glx_data* glxInfo,
                         Visual* visual, int depth)
{

    const int width = 32;
    const int height = 32;

    // Every window needs a colormap
    // Create one based on the visual used by the current context
    // TODO: Decouple this from context creation

    _x11Window.colormap = XCreateColormap(glxInfo->x11.display,  glxInfo->x11.root, visual, AllocNone);

    // Create the actual window
    {
        XSetWindowAttributes wa;
        const unsigned long wamask = CWBorderPixel | CWColormap | CWEventMask;

        wa.colormap = _x11Window.colormap;
        wa.border_pixel = 0;
        wa.event_mask = StructureNotifyMask | KeyPressMask | KeyReleaseMask |
        PointerMotionMask | ButtonPressMask | ButtonReleaseMask |
        ExposureMask | FocusChangeMask | VisibilityChangeMask |
        EnterWindowMask | LeaveWindowMask | PropertyChangeMask;

        GrabErrorHandlerX11(glxInfo);


        _x11Window.handle = XCreateWindow(glxInfo->x11.display,
                                          glxInfo->x11.root,
                                          0, 0,
                                          width, height,
                                          0,      // Border width
                                          depth,  // Color depth
                                          InputOutput,
                                          visual,
                                          wamask,
                                          &wa);

        ReleaseErrorHandlerX11(glxInfo);

        if (!_x11Window.handle) {
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
}



#define setGLXattrib(attribName, attribValue) \
{ \
attribs[index++] = attribName; \
attribs[index++] = attribValue; \
assert((size_t) index < sizeof(attribs) / sizeof(attribs[0])); \
}

// Create the OpenGL context using legacy API
//
static GLXContext createLegacyContext(OSGLContext_glx_data* glxInfo,
                                      GLXFBConfig fbconfig,
                                      GLXContext share)
{
    return glxInfo->CreateNewContext(glxInfo->x11.display, fbconfig, GLX_RGBA_TYPE, share, True);
}



void
OSGLContext_x11::createContextGLX(OSGLContext_glx_data* glxInfo, const FramebufferConfig& fbconfig, int major, int minor)
{
    int attribs[40];
    GLXFBConfig native = NULL;
    GLXContext share = NULL;
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

    if (!glxInfo->ARB_create_context) {
        _glxContextHandle = createLegacyContext(glxInfo, native, share);
    } else {

        int index = 0, mask = 0, flags = 0;

        /*if (ctxconfig->forward)
            flags |= GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB;

        if (ctxconfig->profile == GLFW_OPENGL_CORE_PROFILE)
            mask |= GLX_CONTEXT_CORE_PROFILE_BIT_ARB;*/
        //else if (ctxconfig->profile == GLFW_OPENGL_COMPAT_PROFILE)
        mask |= GLX_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB;
        
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
        if (major != 1 || minor != 0)
        {
            setGLXattrib(GLX_CONTEXT_MAJOR_VERSION_ARB, major);
            setGLXattrib(GLX_CONTEXT_MINOR_VERSION_ARB, minor);
        }

        if (mask)
            setGLXattrib(GLX_CONTEXT_PROFILE_MASK_ARB, mask);

        if (flags)
            setGLXattrib(GLX_CONTEXT_FLAGS_ARB, flags);

        setGLXattrib(None, None);

        _glxContextHandle = glxInfo->CreateContextAttribsARB(glxInfo->x11.display, native, share, True, attribs);

        // HACK: This is a fallback for broken versions of the Mesa
        //       implementation of GLX_ARB_create_context_profile that fail
        //       default 1.0 context creation with a GLXBadProfileARB error in
        //       violation of the extension spec
        if (!_glxContextHandle) {
            if (glxInfo->x11.errorCode == glxInfo->errorBase + GLXBadProfileARB
                /*&& ctxconfig->client == GLFW_OPENGL_API &&
                ctxconfig->profile == GLFW_OPENGL_ANY_PROFILE &&
                ctxconfig->forward == GLFW_FALSE*/)
            {
                _glxContextHandle = createLegacyContext(glxInfo, native, share);
            }
        }
    }


    ReleaseErrorHandlerX11(glxInfo);

    if (!_glxContextHandle) {
        throw std::runtime_error("GLX: Failed to create context");
    }


    _glxWindowHandle = glxInfo->CreateWindow(glxInfo->x11.display, native, _x11Window.handle, NULL);
    if (!_glxWindowHandle) {
        throw std::runtime_error("GLX: Failed to create window");
    }


    /*_window = glxInfo->CreateWindow(glxInfo->display, native, window->x11.handle, NULL);
    if (!_window) {
        throw std::runtime_error("GLX: Failed to create window");
    }*/


}

#undef setGLXattrib



OSGLContext_x11::OSGLContext_x11(const FramebufferConfig& pixelFormatAttrs, int major, int minor)
: _glxContextHandle(0)
, _glxWindowHandle(0)
, _x11Window(0)
{

    const OSGLContext_glx_data* glxInfo = appPTR->getGLXData();
    assert(glxInfo);

    Visual* visual;
    int depth;

    ChooseVisualGLX(glxInfo, pixelFormatAttrs, &visual, &depth);
    createWindow(glxInfo, visual, depth)
    createContextGLX(glxInfo, pixelFormatAttrs, major, minor);





}

OSGLContext_x11::~OSGLContext_x11()
{
    const OSGLContext_glx_data* glxInfo = appPTR->getGLXData();
    assert(glxInfo);


    if (_glxWindowHandle) {
        glxInfo->DestroyWindow(glxInfo->x11.display, _glxWindowHandle);
        _glxWindowHandle = 0;
    }
    if (_glxContextHandle) {
        glxInfo->DestroyContext(glxInfo->x11.display,_glxContextHandle);
        _glxContextHandle = 0;
    }
}

bool
OSGLContext_x11::makeContextCurrent(const OSGLContext_x11* context)
{
    const OSGLContext_glx_data* glxInfo = appPTR->getGLXData();
    assert(glxInfo);

    if (context) {
        return glxInfo->MakeCurrent(glxInfo->x11.display, _glxWindowHandle, _glxContextHandle);
    } else {
        return glxInfo->MakeCurrent(glxInfo->x11.display, None, NULL);
    }
    

}

void
OSGLContext_x11::swapBuffers()
{
    const OSGLContext_glx_data* glxInfo = appPTR->getGLXData();
    assert(glxInfo);
    glxInfo->SwapBuffers(glxInfo->x11.display, _glxWindowHandle);
}

void
OSGLContext_x11::swapInterval(int interval)
{
    const OSGLContext_glx_data* glxInfo = appPTR->getGLXData();
    assert(glxInfo);

    if (glxInfo->EXT_swap_control) {
        glxInfo->SwapIntervalEXT(glxInfo->x11.display, _glxWindowHandle, interval);
    } else if (glxInfo->MESA_swap_control) {
        glxInfo->SwapIntervalMESA(interval);
    } else if (glxInfo->SGI_swap_control) {
        if (interval > 0) {
            glxInfo->SwapIntervalSGI(interval);
        }
    }

}


#endif // __NATRON_LINUX__
