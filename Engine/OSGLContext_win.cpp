/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
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

#include "OSGLContext_win.h"

#ifdef __NATRON_WIN32__
#include <sstream> // stringstream

#include <QtCore/QtGlobal>      // for Q_UNUSED

#include "Engine/AppManager.h"
#include "Engine/MemoryInfo.h" // isApplication32Bits
#include "Engine/OSGLContext.h"

#ifdef NATRON_USE_OPTIMUS_HPG

// Applications exporting this symbol with this value will be automatically
// directed to the high-performance GPU on Nvidia Optimus systems with
// up-to-date drivers
//
__declspec(dllexport) DWORD NvOptimusEnablement = 1;

// Applications exporting this symbol with this value will be automatically
// directed to the high-performance GPU on AMD PowerXpress systems with
// up-to-date drivers
//
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;

#endif // NATRON_USE_OPTIMUS_HPG


#define NATRON_WNDCLASSNAME L"NATRON30"


NATRON_NAMESPACE_ENTER

bool
OSGLContext_win::extensionSupported(const char* extension,
                                    OSGLContext_wgl_data* data)
{
    const char* extensions;

    if (data->GetExtensionsStringEXT) {
        extensions = data->GetExtensionsStringEXT();
        if (extensions) {
            if ( OSGLContext::stringInExtensionString(extension, extensions) ) {
                return true;
            }
        }
    }

    if (data->GetExtensionsStringARB) {
        extensions = data->GetExtensionsStringARB(_dc);
        if (extensions) {
            if ( OSGLContext::stringInExtensionString(extension, extensions) ) {
                return true;
            }
        }
    }

    return false;
}

void
OSGLContext_win::initWGLData(OSGLContext_wgl_data* wglInfo)
{
    memset( wglInfo, 0, sizeof(OSGLContext_wgl_data) );
    wglInfo->instance = LoadLibraryW(L"opengl32.dll");
    if (!wglInfo->instance) {
        throw std::runtime_error("WGL: Failed to load opengl32.dll");
    }

    wglInfo->CreateContext = (WGLCREATECONTEXT_T)GetProcAddress(wglInfo->instance, "wglCreateContext");
    wglInfo->DeleteContext = (WGLDELETECONTEXT_T)GetProcAddress(wglInfo->instance, "wglDeleteContext");
    wglInfo->GetProcAddress = (WGLGETPROCADDRESS_T)GetProcAddress(wglInfo->instance, "wglGetProcAddress");
    wglInfo->MakeCurrent = (WGLMAKECURRENT_T)GetProcAddress(wglInfo->instance, "wglMakeCurrent");
    wglInfo->ShareLists = (WGLSHARELISTS_T)GetProcAddress(wglInfo->instance, "wglShareLists");

    WNDCLASSEXW wc;

    ZeroMemory( &wc, sizeof(wc) );
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc   = (WNDPROC) DefWindowProcW;
    wc.hInstance     = GetModuleHandleW(NULL);
    wc.hCursor       = 0;//LoadCursorW(NULL, IDC_ARROW);
    wc.lpszClassName = NATRON_WNDCLASSNAME;
    bool ok = (bool)RegisterClassExW(&wc);
    assert(ok);
    Q_UNUSED(ok);
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
    wglInfo->NV_gpu_affinity = extensionSupported("WGL_NV_gpu_affinity", wglInfo);
    wglInfo->AMD_gpu_association = extensionSupported("WGL_AMD_gpu_association", wglInfo);

    if (wglInfo->NV_gpu_affinity) {
        wglInfo->EnumGpusNV = (PFNWGLENUMGPUSNV)wglInfo->GetProcAddress("wglEnumGpusNV");
        wglInfo->EnumGpuDevicesNV = (PFNWGLENUMGPUDEVICESNV)wglInfo->GetProcAddress("wglEnumGpuDevicesNV");
        wglInfo->CreateAffinityDCNV = (PFNWGLCREATEAFFINITYDCNV)wglInfo->GetProcAddress("wglCreateAffinityDCNV");
        wglInfo->EnumGpusFromAffinityDCNV = (PFNWGLENUMGPUSFROMAFFINITYDCNV)wglInfo->GetProcAddress("wglEnumGpusFromAffinityDCNV");
        wglInfo->DeleteDCNV = (PFNWGLDELETEDCNV)wglInfo->GetProcAddress("wglDeleteDCNV");
    }

    if (wglInfo->AMD_gpu_association) {
        wglInfo->GetGpuIDAMD = (PFNWGLGETGPUIDSAMD)wglInfo->GetProcAddress("wglGetGPUIDsAMD");
        wglInfo->GetGPUInfoAMD = (PFNWGLGETGPUINFOAMD)wglInfo->GetProcAddress("wglGetGPUInfoAMD");
        wglInfo->GetContextGpuIDAMD = (PFNWGLGETCONTEXTGPUIDAMD)wglInfo->GetProcAddress("wglGetContextGPUIDAMD");
        wglInfo->CreateAssociatedContextAMD = (PFNWGLCREATEASSOCIATEDCONTEXTAMD)wglInfo->GetProcAddress("wglCreateAssociatedContextAMD");
        wglInfo->CreateAssociatedContextAttribsAMD = (PFNWGLCREATEASSOCIATEDCONTEXTATTRIBSAMD)wglInfo->GetProcAddress("wglCreateAssociatedContextAttribsAMD");
        wglInfo->DeleteAssociatedContextAMD = (PFNWGLDELETEASSOCIATEDCONTEXTAMD)wglInfo->GetProcAddress("wglDeleteAssociatedContextAMD");
        wglInfo->MakeAssociatedContetCurrentAMD = (PFNWGLMAKEASSOCIATEDCONTEXTCURRENTAMD)wglInfo->GetProcAddress("wglMakeAssociatedContextCurrentAMD");
        wglInfo->GetCurrentAssociatedContextAMD = (PFNWGLGETCURRENTASSOCIATEDCONTEXTAMD)wglInfo->GetProcAddress("wglGetCurrentAssociatedContextAMD");
        wglInfo->BlitContextFrameBufferAMD = (PFNWGLBLITCONTEXTFRAMEBUFFERAMD)wglInfo->GetProcAddress("wglBlitContextFramebufferAMD");
    }

    wglInfo->extensionsLoaded = GL_TRUE;

    return true;
} // OSGLContext_win::loadWGLExtensions

void
OSGLContext_win::destroyWGLData(OSGLContext_wgl_data* wglInfo)
{
    if (wglInfo->instance) {
        FreeLibrary(wglInfo->instance);
        wglInfo->instance = 0;
    }
    UnregisterClassW( NATRON_WNDCLASSNAME, GetModuleHandleW(NULL) );
}

// Creates the GLFW window and rendering context
//
static bool
createWindow(HWND* window)
{
    int xpos = CW_USEDEFAULT;
    int ypos = CW_USEDEFAULT;
    const int fullWidth = 32;
    const int fullHeight = 32;

    *window = CreateWindowExW(/*exStyle*/ 0,
                                          NATRON_WNDCLASSNAME,
                                          L"",
                                          /*style*/ 0,
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
OSGLContext_win::getPixelFormatAttrib(const OSGLContext_wgl_data* wglInfo,
                                      int pixelFormat,
                                      int attrib)
{
    int value = 0;

    assert(wglInfo->ARB_pixel_format);

    if ( !wglInfo->GetPixelFormatAttribivARB(_dc, pixelFormat, 0, 1, &attrib, &value) ) {
        std::stringstream ss;
        ss << "WGL: Failed to retrieve pixel format attribute ";
        ss << attrib;
        throw std::runtime_error( ss.str() );
    }

    return value;
}

static void
setAttribs(int /*major*/,
           int /*minor*/,
           bool coreProfile,
           std::vector<int>& attribs)
{
    int mask = 0;

    // Note that we do NOT request a specific version. Sometimes a driver might report that its maximum compatibility
    //profile is 3.0 but we ask for 2.0, hence it will fail context creation whereas it should not. Instead we check
    //OpenGL version once context is created and check that we have at least 2.0
    /*
       if ( (major != 1) || (minor != 0) ) {
        attribs.push_back(WGL_CONTEXT_MAJOR_VERSION_ARB);
        attribs.push_back(major);
        attribs.push_back(WGL_CONTEXT_MINOR_VERSION_ARB);
        attribs.push_back(minor);
       }*/

    if (coreProfile) {
        mask |= WGL_CONTEXT_CORE_PROFILE_BIT_ARB;
    } else {
        mask |= WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB;
    }
    if (mask) {
        attribs.push_back(WGL_CONTEXT_PROFILE_MASK_ARB);
        attribs.push_back(mask);
    }
    attribs.push_back(0);
    attribs.push_back(0);
}

void
OSGLContext_win::createGLContext(const FramebufferConfig& pixelFormatAttrs,
                                 int major,
                                 int minor,
                                 bool coreProfile,
                                 const GLRendererID &rendererID,
                                 const OSGLContext_win* shareContext)
{
    const OSGLContext_wgl_data* wglInfo = appPTR->getWGLData();

    assert(wglInfo);
    if (!wglInfo) {
        throw std::invalid_argument("No wgl info");
    }
    bool useNVGPUAffinity = rendererID.rendererHandle && wglInfo->NV_gpu_affinity;
    if (useNVGPUAffinity) {
        HGPUNV GpuMask[2] = {(HGPUNV)rendererID.rendererHandle, NULL};
        _dc = wglInfo->CreateAffinityDCNV(GpuMask);
    } else {
        _dc = GetDC(_windowHandle);
    }
    if (!_dc) {
        throw std::runtime_error("WGL: Failed to retrieve DC for window");
    }


    std::vector<FramebufferConfig> usableConfigs;
    int nativeCount = 0, usableCount = 0;

    if (wglInfo->ARB_pixel_format) {
        nativeCount = getPixelFormatAttrib(wglInfo, 1, WGL_NUMBER_PIXEL_FORMATS_ARB);
    } else {
        nativeCount = DescribePixelFormat(_dc, 1, sizeof(PIXELFORMATDESCRIPTOR), NULL);
    }

    usableConfigs.resize(nativeCount);

    for (int i = 0; i < nativeCount; ++i) {
        const int n = i + 1;
        FramebufferConfig& u = usableConfigs[usableCount];

        if (wglInfo->ARB_pixel_format) {
            // Get pixel format attributes through "modern" extension

            if ( !getPixelFormatAttrib(wglInfo, n, WGL_SUPPORT_OPENGL_ARB) ||
                 !getPixelFormatAttrib(wglInfo, n, WGL_DRAW_TO_WINDOW_ARB) ) {
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

            if ( getPixelFormatAttrib(wglInfo, n, WGL_STEREO_ARB) ) {
                u.stereo = GL_TRUE;
            }
            if ( getPixelFormatAttrib(wglInfo, n, WGL_DOUBLE_BUFFER_ARB) ) {
                u.doublebuffer = GL_TRUE;
            }

            if (wglInfo->ARB_multisample) {
                u.samples = getPixelFormatAttrib(wglInfo, n, WGL_SAMPLES_ARB);
            }

            if (wglInfo->ARB_framebuffer_sRGB ||  wglInfo->EXT_framebuffer_sRGB) {
                if ( getPixelFormatAttrib(wglInfo, n, WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB) ) {
                    u.sRGB = GL_TRUE;
                }
            }
        } else {
            PIXELFORMATDESCRIPTOR pfd;

            // Get pixel format attributes through legacy PFDs

            if ( !DescribePixelFormat(_dc, n, sizeof(PIXELFORMATDESCRIPTOR), &pfd) ) {
                continue;
            }

            if ( !(pfd.dwFlags & PFD_DRAW_TO_WINDOW) || !(pfd.dwFlags & PFD_SUPPORT_OPENGL) ) {
                continue;
            }

            if ( !(pfd.dwFlags & PFD_GENERIC_ACCELERATED) && (pfd.dwFlags & PFD_GENERIC_FORMAT) ) {
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

            if (pfd.dwFlags & PFD_STEREO) {
                u.stereo = GL_TRUE;
            }
            if (pfd.dwFlags & PFD_DOUBLEBUFFER) {
                u.doublebuffer = GL_TRUE;
            }
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
    HGLRC share = shareContext ? shareContext->_handle : 0;

    if ( !DescribePixelFormat(_dc, pixelFormat, sizeof(pfd), &pfd) ) {
        throw std::runtime_error("WGL: Failed to retrieve PFD for selected pixel format");
    }

    if ( !SetPixelFormat(_dc, pixelFormat, &pfd) ) {
        throw std::runtime_error("WGL: Failed to set selected pixel format");
    }


    if (useNVGPUAffinity) {
        _handle = wglInfo->CreateContext(_dc);
    } else if ( (rendererID.renderID > 0) && wglInfo->AMD_gpu_association ) {
        std::vector<int> attribs;
        setAttribs(major, minor, coreProfile, attribs);
        _handle = wglInfo->CreateAssociatedContextAttribsAMD( (UINT)rendererID.renderID, share, &attribs[0] );
    } else if (wglInfo->ARB_create_context) {
        std::vector<int> attribs;
        setAttribs(major, minor, coreProfile, attribs);
        _handle = wglInfo->CreateContextAttribsARB(_dc, share, &attribs[0]);
    } else {
        _handle = wglInfo->CreateContext(_dc);
    }

    if (!_handle) {
        throw std::runtime_error("WGL: Failed to create OpenGL context");
    }
} // createGLContext

#undef setWGLattrib


// Analyzes the specified context for possible recreation
//
bool
OSGLContext_win::analyzeContextWGL(const FramebufferConfig& pixelFormatAttrs,
                                   int major,
                                   int minor)
{
    bool required = false;
    OSGLContext_wgl_data* wglInfo = const_cast<OSGLContext_wgl_data*>( appPTR->getWGLData() );

    assert(wglInfo);

    makeContextCurrent(this);
    if ( !loadWGLExtensions(wglInfo) ) {
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


    if ( (major != 1) || (minor != 0) ) {
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
        if (wglInfo->ARB_multisample && wglInfo->ARB_pixel_format) {
            required = true;
        }
    }

    if (pixelFormatAttrs.sRGB) {
        // sRGB is not a hard constraint, so do nothing if it's not supported
        if ( (wglInfo->ARB_framebuffer_sRGB ||
              wglInfo->EXT_framebuffer_sRGB) &&
             wglInfo->ARB_pixel_format ) {
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

OSGLContext_win::OSGLContext_win(const FramebufferConfig& pixelFormatAttrs,
                                 int major,
                                 int minor,
                                 bool coreProfile,
                                 const GLRendererID &rendererID,
                                 const OSGLContext_win* shareContext)
    : _dc(0)
    , _handle(0)
    , _interval(0)
    , _windowHandle(0)
{
    if ( !createWindow(&_windowHandle) ) {
        throw std::runtime_error("WGL: Failed to create window");
    }

    createGLContext(pixelFormatAttrs, major, minor, coreProfile, rendererID, shareContext);

    if ( analyzeContextWGL(pixelFormatAttrs, major, minor) ) {
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
        if ( !createWindow(&_windowHandle) ) {
            throw std::runtime_error("WGL: Failed to create window");
        }

        createGLContext(pixelFormatAttrs, major, minor, coreProfile, rendererID, shareContext);
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

// A valid context must be bound
// http://developer.download.nvidia.com/opengl/specs/GL_NVX_gpu_memory_info.txt
static int
nvx_get_GPU_mem_info()
{
    const OSGLContext_wgl_data* wglInfo = appPTR->getWGLData();

    assert(wglInfo);
    if (!wglInfo->NVX_gpu_memory_info) {
        return 0;
    }

    int v = 0;
    // Clear error
    GLenum _glerror_ = glGetError();
    glGetIntegerv(GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX, &v);

    // If error, return 0
    _glerror_ = glGetError();
    if (_glerror_ != GL_NO_ERROR) {
        return 0;
    }

    return v * 1e3;
}

static std::string GetGPUInfoAMDInternal_string(const OSGLContext_wgl_data* wglInfo, UINT gpuID, int info)
{
    std::vector<char> data;
    int totalSize = 0;

    INT numVals;
    int safeCounter = 0;
    do {
        totalSize += 1024;
        if ((int)data.size() < totalSize) {
            data.resize(totalSize);
        }

        numVals = wglInfo->GetGPUInfoAMD(gpuID, info, GL_UNSIGNED_BYTE, data.size(), &data[0]);
        ++safeCounter;
    } while (numVals > 0 && numVals == (INT)data.size() && safeCounter < 1000);
    assert(numVals > 0);
    if (numVals <= 0) {
        return std::string();
    }

    return std::string(&data[0], numVals);
}

static bool GetGPUInfoAMDInternal_int(const OSGLContext_wgl_data* wglInfo, UINT gpuID, int info, int* value)
{

    std::vector<unsigned int> data;
    int totalSize = 0;

    INT numVals;
    int safeCounter = 0;
    do {
        totalSize += 10;
        if ((int)data.size() < totalSize) {
            data.resize(totalSize);
        }

        // AMD drivers are f*** up in 32 bits, they read a wrong buffer size.
        // It works fine in 64 bits mode
        int arrayDepth = data.size();
        numVals = wglInfo->GetGPUInfoAMD(gpuID, info, GL_UNSIGNED_INT, arrayDepth, &data[0]);
        ++safeCounter;
    } while (numVals > 0 && numVals == (INT)data.size() && safeCounter < 1000);
    assert(numVals > 0);
    if (numVals <= 0) {
        return false;
    }
    *value = (int)data[0];
    return true;
}

void
OSGLContext_win::getGPUInfos(std::list<OpenGLRendererInfo>& renderers)
{

    const OSGLContext_wgl_data* wglInfo = appPTR->getWGLData();
    assert(wglInfo);
    if (!wglInfo) {
        return;
    }
    bool defaultFallback = false;
    if (wglInfo->NV_gpu_affinity) {
        // https://www.opengl.org/registry/specs/NV/gpu_affinity.txt
        std::vector<HGPUNV> gpuHandles;
        int gpuIndex = 0;
        HGPUNV gpuHandle;
        bool gotGPU = true;
        do {
            gotGPU = wglInfo->EnumGpusNV(gpuIndex, &gpuHandle);
            if (gotGPU) {
                gpuHandles.push_back(gpuHandle);
            }
            ++gpuIndex;
        } while (gotGPU);

        if (gpuHandles.empty()) {
            defaultFallback = true;
        }
        for (std::size_t i = 0; i < gpuHandles.size(); ++i) {
            OpenGLRendererInfo info;
            info.rendererID.rendererHandle = (void*)gpuHandles[i];

            boost::scoped_ptr<OSGLContext_win> context;
            try {
                GLRendererID gid;
                gid.rendererHandle = info.rendererID.rendererHandle;
                context.reset( new OSGLContext_win(FramebufferConfig(), GLVersion.major, GLVersion.minor, false, gid, 0) );
            } catch (const std::exception& e) {
                continue;
            }

            if ( !makeContextCurrent( context.get() ) ) {
                continue;
            }

            try {
                OSGLContext::checkOpenGLVersion();
            } catch (const std::exception& e) {
                std::cerr << e.what() << std::endl;
                continue;
            }

            info.vendorName = std::string( (const char *) glGetString(GL_VENDOR) );
            info.rendererName = std::string( (const char *) glGetString(GL_RENDERER) );
            info.glVersionString = std::string( (const char *) glGetString(GL_VERSION) );
            info.glslVersionString = std::string( (const char*)glGetString (GL_SHADING_LANGUAGE_VERSION) );
            info.maxMemBytes = nvx_get_GPU_mem_info();
            glGetIntegerv(GL_MAX_TEXTURE_SIZE, &info.maxTextureSize);
            renderers.push_back(info);

            makeContextCurrent(0);
        }
    } else if (wglInfo->AMD_gpu_association && !isApplication32Bits()) {
        //https://www.opengl.org/registry/specs/AMD/wgl_gpu_association.txt
        UINT getGpuIDMaxCount = wglInfo->GetGpuIDAMD(0, 0);
        UINT maxCount = getGpuIDMaxCount;
        std::vector<UINT> gpuIDs(maxCount);
        if (maxCount == 0) {
            defaultFallback = true;
        } else {
            UINT gpuCount = wglInfo->GetGpuIDAMD(maxCount, &gpuIDs[0]);
            if (gpuCount > maxCount) {
                gpuIDs.resize(gpuCount);
            }
            for (UINT index = 0; index < gpuCount; ++index) {
                assert(index < gpuIDs.size());
                UINT gpuID = gpuIDs[index];

                OpenGLRendererInfo info;
                info.rendererName = GetGPUInfoAMDInternal_string(wglInfo, gpuID, WGL_GPU_RENDERER_STRING_AMD);
                if (info.rendererName.empty()) {
                    continue;
                }

                info.vendorName = GetGPUInfoAMDInternal_string(wglInfo, gpuID, WGL_GPU_VENDOR_AMD);
                if (info.vendorName.empty()) {
                    continue;
                }

                info.glVersionString = GetGPUInfoAMDInternal_string(wglInfo, gpuID, WGL_GPU_OPENGL_VERSION_STRING_AMD);
                if (info.glVersionString.empty()) {
                    continue;
                }

                // note: cannot retrieve GL_SHADING_LANGUAGE_VERSION

                info.maxMemBytes = 0;
                if (!isApplication32Bits()) {
                    int ramMB = 0;
                    // AMD drivers are f*** up in 32 bits, they read a wrong buffer size.
                    // It works fine in 64 bits mode
                    if (!GetGPUInfoAMDInternal_int(wglInfo, gpuID, WGL_GPU_RAM_AMD, &ramMB)) {
                        continue;
                    }
                    info.maxMemBytes = ramMB * 1e6;
                }

                info.rendererID.renderID = gpuID;
                
                boost::scoped_ptr<OSGLContext_win> context;

                GLRendererID gid;
                gid.renderID = info.rendererID.renderID;
                try {
                    context.reset( new OSGLContext_win(FramebufferConfig(), GLVersion.major, GLVersion.minor, false, gid, 0) );
                } catch (const std::exception& e) {
                    continue;
                }

                if ( !makeContextCurrent( context.get() ) ) {
                    continue;
                }

                try {
                    OSGLContext::checkOpenGLVersion();
                } catch (const std::exception& e) {
                    std::cerr << e.what() << std::endl;
                    continue;
                }

                glGetIntegerv(GL_MAX_TEXTURE_SIZE, &info.maxTextureSize);
                renderers.push_back(info);
                
                makeContextCurrent(0);
            }
        }
    }

    if (renderers.empty()) {
        defaultFallback = true;
    }
    if (defaultFallback) {
        // No extension, use default
        boost::scoped_ptr<OSGLContext_win> context;
        try {
            context.reset( new OSGLContext_win(FramebufferConfig(), GLVersion.major, GLVersion.minor, false, GLRendererID(), 0) );
        } catch (const std::exception& e) {
            std::cerr << e.what() << std::endl;

            return;
        }

        if ( !makeContextCurrent( context.get() ) ) {
            return;
        }

        try {
            OSGLContext::checkOpenGLVersion();
        } catch (const std::exception& e) {
            std::cerr << e.what() << std::endl;

            return;
        }

        OpenGLRendererInfo info;
        info.vendorName = std::string( (const char *) glGetString(GL_VENDOR) );
        info.rendererName = std::string( (const char *) glGetString(GL_RENDERER) );
        info.glVersionString = std::string( (const char *) glGetString(GL_VERSION) );
        info.glslVersionString = std::string( (const char *) glGetString (GL_SHADING_LANGUAGE_VERSION) );
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &info.maxTextureSize);
        // We don't have any way to get memory size, set it to 0
        info.maxMemBytes = nvx_get_GPU_mem_info();
        renderers.push_back(info);

        makeContextCurrent(0);
    }
} // OSGLContext_win::getGPUInfos

NATRON_NAMESPACE_EXIT

#endif // __NATRON_WIN32__
