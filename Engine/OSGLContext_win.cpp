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


#include "OSGLContext_win.h"

#ifdef __NATRON_WIN32__

#include <sstream>
#include "Engine/AppManager.h"
#include "Engine/OSGLContext.h"

NATRON_NAMESPACE_ENTER;

bool
OSGLContext_win::extensionSupported(const char* extension, OSGLContext_wgl_data* data)
{
    const char* extensions;

    if (data->GetExtensionsStringEXT) {
        extensions = data->GetExtensionsStringEXT();
        if (extensions) {
            if (OSGLContext::stringInExtensionString(extension, extensions))
                return true;
        }
    }

    if (data->GetExtensionsStringARB) {
        extensions = data->GetExtensionsStringARB(_dc);
        if (extensions) {
            if (OSGLContext::stringInExtensionString(extension, extensions)) {
                return true;
            }
        }
    }

    return false;
}

void
OSGLContext_win::initWGLData(OSGLContext_wgl_data* wglInfo)
{
    wglInfo->instance = LoadLibraryA("opengl32.dll");
    if (!wglInfo->instance) {
        assert(false);
        throw std::runtime_error("WGL: Failed to load opengl32.dll");
    }

    wglInfo->CreateContext = (WGLCREATECONTEXT_T)GetProcAddress(wglInfo->instance, "wglCreateContext");
    wglInfo->DeleteContext = (WGLDELETECONTEXT_T)GetProcAddress(wglInfo->instance, "wglDeleteContext");
    wglInfo->GetProcAddress = (WGLGETPROCADDRESS_T)GetProcAddress(wglInfo->instance, "wglGetProcAddress");
    wglInfo->MakeCurrent = (WGLMAKECURRENT_T)GetProcAddress(wglInfo->instance, "wglMakeCurrent");
    wglInfo->ShareLists = (WGLSHARELISTS_T)GetProcAddress(wglInfo->instance, "wglShareLists");

}

bool
OSGLContext_win::loadWGLExtensions(OSGLContext_wgl_data* wglInfo)
{
    // This should be protected by a mutex externally
    if (wglInfo->extensionsLoaded) {
        return false;
    }
    // Functions for WGL_EXT_extension_string
    // NOTE: These are needed by extensionSupported
    wglInfo->GetExtensionsStringEXT = (PFNWGLGETEXTENSIONSSTRINGEXTPROC)wglInfo->GetProcAddress("wglGetExtensionsStringEXT");
    wglInfo->GetExtensionsStringARB = (PFNWGLGETEXTENSIONSSTRINGARBPROC)wglInfo->GetProcAddress("wglGetExtensionsStringARB");

    // Functions for WGL_ARB_create_context
    wglInfo->CreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglInfo->GetProcAddress("wglCreateContextAttribsARB");

    // Functions for WGL_EXT_swap_control
    wglInfo->SwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglInfo->GetProcAddress("wglSwapIntervalEXT");

    // Functions for WGL_ARB_pixel_format
    wglInfo->GetPixelFormatAttribivARB = (PFNWGLGETPIXELFORMATATTRIBIVARBPROC)wglInfo->GetProcAddress("wglGetPixelFormatAttribivARB");

    // This needs to include every extension used below except for
    // WGL_ARB_extensions_string and WGL_EXT_extensions_string
    wglInfo->ARB_multisample = extensionSupported("WGL_ARB_multisample", wglInfo);
    wglInfo->ARB_framebuffer_sRGB = extensionSupported("WGL_ARB_framebuffer_sRGB", wglInfo);
    wglInfo->EXT_framebuffer_sRGB = extensionSupported("WGL_EXT_framebuffer_sRGB", wglInfo);
    wglInfo->ARB_create_context = extensionSupported("WGL_ARB_create_context", wglInfo);
    wglInfo->ARB_create_context_profile = extensionSupported("WGL_ARB_create_context_profile", wglInfo);
    wglInfo->EXT_create_context_es2_profile = extensionSupported("WGL_EXT_create_context_es2_profile", wglInfo);
    wglInfo->ARB_create_context_robustness = extensionSupported("WGL_ARB_create_context_robustness", wglInfo);
    wglInfo->EXT_swap_control = extensionSupported("WGL_EXT_swap_control", wglInfo);
    wglInfo->ARB_pixel_format = extensionSupported("WGL_ARB_pixel_format", wglInfo);
    wglInfo->ARB_context_flush_control = extensionSupported("WGL_ARB_context_flush_control", wglInfo);

    wglInfo->extensionsLoaded = GL_TRUE;
    return true;
}

void
OSGLContext_win::destroyWGLData(OSGLContext_wgl_data* wglInfo)
{
    if (wglInfo->instance) {
        FreeLibrary(wglInfo->instance);
        wglInfo->instance = 0;
    }
}

// Creates the GLFW window and rendering context
//
static bool createWindow(HWND* window)
{

    int xpos = CW_USEDEFAULT;
    int ypos = CW_USEDEFAULT;
    const int fullWidth = 32;
    const int fullHeight = 32;

    *window = CreateWindowExW(/*exStyle*/0,
                              L"OffscreenWindow",
                              L"",
                              /*style*/0,
                              xpos, ypos,
                              fullWidth, fullHeight,
                              NULL, // No parent window
                              NULL, // No window menu
                              GetModuleHandleW(NULL),
                              NULL);


    if (!*window) {
        return false;
    }
    return true;
}

int
OSGLContext_win::getPixelFormatAttrib(const OSGLContext_wgl_data* wglInfo, int pixelFormat, int attrib)
{
    int value = 0;

    assert(wglInfo->ARB_pixel_format);

    if (!wglInfo->GetPixelFormatAttribivARB(_dc, pixelFormat, 0, 1, &attrib, &value)) {
        std::stringstream ss;
        ss << "WGL: Failed to retrieve pixel format attribute ";
        ss << attrib;
        throw std::runtime_error(ss.str());
    }

    return value;
}

#define setWGLattrib(attribName, attribValue) \
{ \
    attribs[index++] = attribName; \
    attribs[index++] = attribValue; \
    assert((std::size_t) index < sizeof(attribs) / sizeof(attribs[0])); \
}

void
OSGLContext_win::createGLContext(const FramebufferConfig& pixelFormatAttrs, int major, int minor)
{
    _dc = GetDC(_windowHandle);
    if (!_dc) {
        throw std::runtime_error("WGL: Failed to retrieve DC for window");
    }


    const OSGLContext_wgl_data* wglInfo = appPTR->getWGLData();
    assert(wglInfo);

    std::vector<FramebufferConfig> usableConfigs;

    int nativeCount = 0, usableCount = 0;

    if (wglInfo->ARB_pixel_format) {
        nativeCount = getPixelFormatAttrib(wglInfo, 1, WGL_NUMBER_PIXEL_FORMATS_ARB);
    } else {
        nativeCount = DescribePixelFormat(_dc, 1, sizeof(PIXELFORMATDESCRIPTOR), NULL);
    }

    usableConfigs.resize(nativeCount);

    for (int i = 0;  i < nativeCount; ++i)
    {
        const int n = i + 1;
        FramebufferConfig& u = usableConfigs[usableCount];

        if (wglInfo->ARB_pixel_format) {
            // Get pixel format attributes through "modern" extension

            if (!getPixelFormatAttrib(wglInfo, n, WGL_SUPPORT_OPENGL_ARB) ||
                !getPixelFormatAttrib(wglInfo, n, WGL_DRAW_TO_WINDOW_ARB)) {
                continue;
            }

            if (getPixelFormatAttrib(wglInfo, n, WGL_PIXEL_TYPE_ARB) != WGL_TYPE_RGBA_ARB) {
                continue;
            }

            if (getPixelFormatAttrib(wglInfo, n, WGL_ACCELERATION_ARB) == WGL_NO_ACCELERATION_ARB) {
                continue;
            }

            u.redBits = getPixelFormatAttrib(wglInfo, n, WGL_RED_BITS_ARB);
            u.greenBits = getPixelFormatAttrib(wglInfo, n, WGL_GREEN_BITS_ARB);
            u.blueBits = getPixelFormatAttrib(wglInfo, n, WGL_BLUE_BITS_ARB);
            u.alphaBits = getPixelFormatAttrib(wglInfo, n, WGL_ALPHA_BITS_ARB);

            u.depthBits = getPixelFormatAttrib(wglInfo, n, WGL_DEPTH_BITS_ARB);
            u.stencilBits = getPixelFormatAttrib(wglInfo, n, WGL_STENCIL_BITS_ARB);

            u.accumRedBits = getPixelFormatAttrib(wglInfo, n, WGL_ACCUM_RED_BITS_ARB);
            u.accumGreenBits = getPixelFormatAttrib(wglInfo, n, WGL_ACCUM_GREEN_BITS_ARB);
            u.accumBlueBits = getPixelFormatAttrib(wglInfo, n, WGL_ACCUM_BLUE_BITS_ARB);
            u.accumAlphaBits = getPixelFormatAttrib(wglInfo, n, WGL_ACCUM_ALPHA_BITS_ARB);

            u.auxBuffers = getPixelFormatAttrib(wglInfo, n, WGL_AUX_BUFFERS_ARB);

            if (getPixelFormatAttrib(wglInfo, n, WGL_STEREO_ARB)) {
                u.stereo = GL_TRUE;
            }
            if (getPixelFormatAttrib(wglInfo, n, WGL_DOUBLE_BUFFER_ARB)) {
                u.doublebuffer = GL_TRUE;
            }

            if (wglInfo->ARB_multisample) {
                u.samples = getPixelFormatAttrib(wglInfo, n, WGL_SAMPLES_ARB);
            }

            if (wglInfo->ARB_framebuffer_sRGB ||  wglInfo->EXT_framebuffer_sRGB) {
                if (getPixelFormatAttrib(wglInfo, n, WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB)) {
                    u.sRGB = GL_TRUE;
                }
            }
        } else {
            PIXELFORMATDESCRIPTOR pfd;

            // Get pixel format attributes through legacy PFDs

            if (!DescribePixelFormat(_dc, n, sizeof(PIXELFORMATDESCRIPTOR), &pfd)) {
                continue;
            }

            if (!(pfd.dwFlags & PFD_DRAW_TO_WINDOW) || !(pfd.dwFlags & PFD_SUPPORT_OPENGL)) {
                continue;
            }

            if (!(pfd.dwFlags & PFD_GENERIC_ACCELERATED) && (pfd.dwFlags & PFD_GENERIC_FORMAT)) {
                continue;
            }

            if (pfd.iPixelType != PFD_TYPE_RGBA) {
                continue;
            }

            u.redBits = pfd.cRedBits;
            u.greenBits = pfd.cGreenBits;
            u.blueBits = pfd.cBlueBits;
            u.alphaBits = pfd.cAlphaBits;

            u.depthBits = pfd.cDepthBits;
            u.stencilBits = pfd.cStencilBits;

            u.accumRedBits = pfd.cAccumRedBits;
            u.accumGreenBits = pfd.cAccumGreenBits;
            u.accumBlueBits = pfd.cAccumBlueBits;
            u.accumAlphaBits = pfd.cAccumAlphaBits;

            u.auxBuffers = pfd.cAuxBuffers;

            if (pfd.dwFlags & PFD_STEREO)
                u.stereo = GL_TRUE;
            if (pfd.dwFlags & PFD_DOUBLEBUFFER)
                u.doublebuffer = GL_TRUE;
        }

        u.handle = n;
        ++usableCount;
    }

    if (!usableCount) {
        throw std::runtime_error("WGL: The driver does not appear to support OpenGL");
    }
    FramebufferConfig closestConfig = OSGLContext::chooseFBConfig(pixelFormatAttrs, usableConfigs, usableCount);
    int pixelFormat = closestConfig.handle;

    PIXELFORMATDESCRIPTOR pfd;
    HGLRC share = NULL;

    if (!DescribePixelFormat(_dc, pixelFormat, sizeof(pfd), &pfd)) {
        throw std::runtime_error("WGL: Failed to retrieve PFD for selected pixel format");
    }

    if (!SetPixelFormat(_dc, pixelFormat, &pfd)) {
        throw std::runtime_error("WGL: Failed to set selected pixel format");
    }

    if (!wglInfo->ARB_create_context) {
        _handle = wglInfo->CreateContext(_dc);
        if (!_handle) {
            throw std::runtime_error("WGL: Failed to create OpenGL context");
        }
    } else {

        int attribs[40];

        int index = 0, mask = 0, flags = 0;
/*
        if (ctxconfig->client == GLFW_OPENGL_API)
        {
            if (ctxconfig->forward)
                flags |= WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB;

            if (ctxconfig->profile == GLFW_OPENGL_CORE_PROFILE)
                mask |= WGL_CONTEXT_CORE_PROFILE_BIT_ARB;
            else if (ctxconfig->profile == GLFW_OPENGL_COMPAT_PROFILE)
                mask |= WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB;
        }
        else
            mask |= WGL_CONTEXT_ES2_PROFILE_BIT_EXT;

        if (ctxconfig->debug)
            flags |= WGL_CONTEXT_DEBUG_BIT_ARB;
        if (ctxconfig->noerror)
            flags |= GL_CONTEXT_FLAG_NO_ERROR_BIT_KHR;

        if (ctxconfig->robustness)
        {
            if (_glfw.wgl.ARB_create_context_robustness)
            {
                if (ctxconfig->robustness == GLFW_NO_RESET_NOTIFICATION)
                {
                    setWGLattrib(WGL_CONTEXT_RESET_NOTIFICATION_STRATEGY_ARB,
                                 WGL_NO_RESET_NOTIFICATION_ARB);
                }
                else if (ctxconfig->robustness == GLFW_LOSE_CONTEXT_ON_RESET)
                {
                    setWGLattrib(WGL_CONTEXT_RESET_NOTIFICATION_STRATEGY_ARB,
                                 WGL_LOSE_CONTEXT_ON_RESET_ARB);
                }

                flags |= WGL_CONTEXT_ROBUST_ACCESS_BIT_ARB;
            }
        }

        if (ctxconfig->release)
        {
            if (_glfw.wgl.ARB_context_flush_control)
            {
                if (ctxconfig->release == GLFW_RELEASE_BEHAVIOR_NONE)
                {
                    setWGLattrib(WGL_CONTEXT_RELEASE_BEHAVIOR_ARB,
                                 WGL_CONTEXT_RELEASE_BEHAVIOR_NONE_ARB);
                }
                else if (ctxconfig->release == GLFW_RELEASE_BEHAVIOR_FLUSH)
                {
                    setWGLattrib(WGL_CONTEXT_RELEASE_BEHAVIOR_ARB,
                                 WGL_CONTEXT_RELEASE_BEHAVIOR_FLUSH_ARB);
                }
            }
        }*/

        // NOTE: Only request an explicitly versioned context when necessary, as
        //       explicitly requesting version 1.0 does not always return the
        //       highest version supported by the driver
        if (major != 1 || minor != 0)
        {
            setWGLattrib(WGL_CONTEXT_MAJOR_VERSION_ARB, major);
            setWGLattrib(WGL_CONTEXT_MINOR_VERSION_ARB, minor);
        }

        if (flags) {
            setWGLattrib(WGL_CONTEXT_FLAGS_ARB, flags);
        }

        if (mask) {
            setWGLattrib(WGL_CONTEXT_PROFILE_MASK_ARB, mask);
        }
        
        setWGLattrib(0, 0);
        
        _handle = wglInfo->CreateContextAttribsARB(_dc, share, attribs);
        if (!_handle) {
            throw std::runtime_error("WGL: Failed to create OpenGL context");
        }
        
    } // wglInfo->ARB_create_context
} // createGLContext

#undef setWGLattrib


// Analyzes the specified context for possible recreation
//
bool
OSGLContext_win::analyzeContextWGL(const FramebufferConfig& pixelFormatAttrs, int major, int minor)
{
    bool required = false;

    OSGLContext_wgl_data* wglInfo = const_cast<OSGLContext_wgl_data*>(appPTR->getWGLData());
    assert(wglInfo);

    makeContextCurrent(this);
    if (!loadWGLExtensions(wglInfo)) {
        return false;
    }



/*
    if (ctxconfig->forward)
    {
        if (!_glfw.wgl.ARB_create_context)
        {
            _glfwInputError(GLFW_VERSION_UNAVAILABLE,
                            "WGL: A forward compatible OpenGL context requested but WGL_ARB_create_context is unavailable");
            return _GLFW_RECREATION_IMPOSSIBLE;
        }

        required = GLFW_TRUE;
    }

    if (ctxconfig->profile)
    {
        if (!_glfw.wgl.ARB_create_context_profile)
        {
            _glfwInputError(GLFW_VERSION_UNAVAILABLE,
                            "WGL: OpenGL profile requested but WGL_ARB_create_context_profile is unavailable");
            return _GLFW_RECREATION_IMPOSSIBLE;
        }

        required = GLFW_TRUE;
    }

    if (ctxconfig->release)
    {
        if (_glfw.wgl.ARB_context_flush_control)
            required = GLFW_TRUE;
    }
*/


    if (major != 1 || minor != 0) {
        if (wglInfo->ARB_create_context) {
            required = true;
        }
    }

    /*if (ctxconfig->debug)
    {
        if (_glfw.wgl.ARB_create_context)
            required = GLFW_TRUE;
    }*/

    if (pixelFormatAttrs.samples > 0) {
        // MSAA is not a hard constraint, so do nothing if it's not supported
        if (wglInfo->ARB_multisample && wglInfo->ARB_pixel_format)
            required = true;
    }

    if (pixelFormatAttrs.sRGB) {
        // sRGB is not a hard constraint, so do nothing if it's not supported
        if ((wglInfo->ARB_framebuffer_sRGB ||
             wglInfo->EXT_framebuffer_sRGB) &&
            wglInfo->ARB_pixel_format) {
            required = true;
        }
    }

    return required;

}

// Destroys the window and rendering context
//
void
OSGLContext_win::destroyWindow()
{
    if (_windowHandle) {
        DestroyWindow(_windowHandle);
        _windowHandle = 0;
    }
}

void
OSGLContext_win::destroyContext()
{
    if (_handle) {
        const OSGLContext_wgl_data* wglInfo = appPTR->getWGLData();
        assert(wglInfo);
        wglInfo->DeleteContext(_handle);
        _handle = 0;
    }
}


OSGLContext_win::OSGLContext_win(const FramebufferConfig& pixelFormatAttrs, int major, int minor)
: _dc(0)
, _handle(0)
, _interval(0)
, _windowHandle(0)
{


    if (!createWindow(&_windowHandle)) {
        throw std::runtime_error("WGL: Failed to create window");
    }

    createGLContext(pixelFormatAttrs, major, minor);

    if (analyzeContextWGL(pixelFormatAttrs, major, minor)) {
        // Some window hints require us to re-create the context using WGL
        // extensions retrieved through the current context, as we cannot
        // check for WGL extensions or retrieve WGL entry points before we
        // have a current context (actually until we have implicitly loaded
        // the vendor ICD)

        // Yes, this is strange, and yes, this is the proper way on WGL

        // As Windows only allows you to set the pixel format once for
        // a window, we need to destroy the current window and create a new
        // one to be able to use the new pixel format

        // Technically, it may be possible to keep the old window around if
        // we're just creating an OpenGL 3.0+ context with the same pixel
        // format, but it's not worth the added code complexity

        // First we clear the current context (the one we just created)
        // This is usually done by glfwDestroyWindow, but as we're not doing
        // full GLFW window destruction, it's duplicated here
        makeContextCurrent(NULL);

        // Next destroy the Win32 window and WGL context (without resetting
        // or destroying the GLFW window object)
        destroyContext();
        destroyWindow();

        // ...and then create them again, this time with better APIs
        if (!createWindow(&_windowHandle)) {
            throw std::runtime_error("WGL: Failed to create window");
        }

        createGLContext(pixelFormatAttrs, major, minor);

    }

}


OSGLContext_win::~OSGLContext_win()
{

}

bool
OSGLContext_win::makeContextCurrent(const OSGLContext_win* context)
{
    const OSGLContext_wgl_data* wglInfo = appPTR->getWGLData();
    assert(wglInfo);
    if (context) {
        return wglInfo->MakeCurrent(context->_dc, context->_handle);
    } else {
        return wglInfo->MakeCurrent(0, 0);
    }
}

void
OSGLContext_win::swapBuffers()
{
    SwapBuffers(_dc);
}

void
OSGLContext_win::swapInterval(int interval)
{

    _interval = interval;

    const OSGLContext_wgl_data* wglInfo = appPTR->getWGLData();
    assert(wglInfo);
    if (wglInfo->EXT_swap_control) {
        wglInfo->SwapIntervalEXT(interval);
    }
}

NATRON_NAMESPACE_EXIT;

#endif // __NATRON_WIN32__
