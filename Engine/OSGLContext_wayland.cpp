/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2023 The Natron developers
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

#include "OSGLContext_wayland.h"

#ifdef __NATRON_LINUX__

#ifdef __NATRON_WAYLAND__

#include <array>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <sstream> // stringstream
#include <stdexcept>

#include <dlfcn.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <wayland-client.h>
#include <wayland-egl.h>

#include "Engine/AppManager.h"
#include "Engine/OSGLContext.h"
#include "Global/GLIncludes.h"

NATRON_NAMESPACE_ENTER

// Wayland-specific global data
//
struct WaylandData
{
    wl_display* display;

    wl_compositor* compositor;
};

struct OSGLContext_egl_dataPrivate
{
    WaylandData wayland;
    EGLDisplay dpy;
    int major, minor;
    int eventBase;
    int errorBase;
    int err;

    // dlopen handle for libGL.so.1
    void* handle;

    // EGL
    PFNEGLGETDISPLAYPROC GetDisplay;
    PFNEGLGETCONFIGSPROC GetConfigs;
    PFNEGLGETCONFIGATTRIBPROC GetConfigAttrib;
    PFNEGLINITIALIZEPROC Initialize;
    PFNEGLBINDAPIPROC BindAPI;
    PFNEGLCHOOSECONFIGPROC ChooseConfig;
    PFNEGLCREATECONTEXTPROC CreateContext;
    PFNEGLGETCURRENTCONTEXTPROC GetCurrentContext;
    PFNEGLDESTROYCONTEXTPROC DestroyContext;
    PFNEGLCREATEWINDOWSURFACEPROC CreateWindowSurface;
    PFNEGLDESTROYSURFACEPROC DestroySurface;
    PFNEGLMAKECURRENTPROC MakeCurrent;
    PFNEGLSWAPBUFFERSPROC SwapBuffers;
    PFNEGLSWAPINTERVALPROC SwapInterval;
    PFNEGLQUERYSTRINGPROC QueryString;
    PFNEGLGETPROCADDRESSPROC GetProcAddress;
    PFNEGLTERMINATEPROC Terminate;
    PFNEGLRELEASETHREADPROC ReleaseThread;

    PFNEGLCREATEPLATFORMWINDOWSURFACEEXTPROC CreatePlatformWindowSurfaceEXT;

    EGLBoolean KHR_create_context;
    EGLBoolean EXT_platform_wayland;
    EGLBoolean KHR_gl_colorspace;
    EGLBoolean EXT_create_context_robustness;
    EGLBoolean KHR_context_flush_control;
};

OSGLContext_egl_data::OSGLContext_egl_data()
    : _imp(new OSGLContext_egl_dataPrivate())
{
}

OSGLContext_egl_data::~OSGLContext_egl_data()
{
}

// Wayland-specific per-window data
//
struct WaylandWindow
{
    wl_surface* handle;
    wl_egl_window* native;
    int width;
    int height;

    WaylandWindow()
        : handle(nullptr)
        , native(nullptr)
        , width(0)
        , height(0)
    {
    }
};

struct OSGLContext_waylandPrivate
{
    EGLContext eglContextHandle;
    EGLDeviceEXT eglDeviceHandle;
    EGLSurface eglSurfaceHandle;
    WaylandWindow waylandWindow;

    OSGLContext_waylandPrivate()
        : eglContextHandle(EGL_NO_CONTEXT)
        , eglDeviceHandle(EGL_NO_DEVICE_EXT)
        , eglSurfaceHandle(EGL_NO_SURFACE)
        , waylandWindow()
    {
    }

    void createContextEGL(OSGLContext_egl_data* eglInfo, const FramebufferConfig& fbconfig, int major, int minor, bool coreProfile, int rendererID, const OSGLContext_wayland* shareContext);

    void createWindow(OSGLContext_egl_data* eglInfo,
                      const EGLConfig& native,
                      int depth);
};

static bool
extensionSupported(const OSGLContext_egl_data* data,
                   const char* extension)
{
    const char* extensions = data->_imp->QueryString(data->_imp->dpy, EGL_EXTENSIONS);

    if (extensions) {
        if (OSGLContext::stringInExtensionString(extension, extensions)) {
            return true;
        }
    }

    return false;
}

typedef void (*GLPFNglproc)(void);
static GLPFNglproc
getProcAddress(const OSGLContext_egl_data* data,
               const char* procname)
{
    if (data->_imp->GetProcAddress) {
        return data->_imp->GetProcAddress(procname);
    } else {
        return (GLPFNglproc)dlsym(data->_imp->handle, procname);
    }
}

static void
registry_handle_global(void* data, wl_registry* registry, uint32_t id, const char* interface, uint32_t version)
{
    auto state = reinterpret_cast<WaylandData*>(data);
    if (std::strcmp(interface, wl_compositor_interface.name) == 0) {
        state->compositor = reinterpret_cast<wl_compositor*>(wl_registry_bind(registry, id, &wl_compositor_interface, 4));
    }
}

static void
registry_handle_global_remove(void* data, wl_registry* registry, uint32_t id)
{
}

static const wl_registry_listener registry_listener = {
    registry_handle_global,
    registry_handle_global_remove,
};

void
OSGLContext_wayland::initEGLData(OSGLContext_egl_data* eglInfo)
{
    // Sets all bits to 0
    memset(eglInfo->_imp.get(), 0, sizeof(OSGLContext_egl_dataPrivate));

    eglInfo->_imp->wayland.display = wl_display_connect(nullptr);
    if (!eglInfo->_imp->wayland.display) {
        throw std::runtime_error("Wayland: Failed to open display");
    }

    wl_registry* registry = wl_display_get_registry(eglInfo->_imp->wayland.display);
    if (registry == nullptr) {
        throw std::runtime_error("Wayland: Failed to get registry from display");
    }

    wl_registry_add_listener(registry, &registry_listener, &eglInfo->_imp->wayland);

    wl_display_roundtrip(eglInfo->_imp->wayland.display);

    wl_registry_destroy(registry);

    const char* sonames[] = {
#if defined(__CYGWIN__)
        "libEGL-1.so",
#else
        "libEGL.so.1",
        "libEGL.so",
#endif
        nullptr
    };

    for (int i = 0; sonames[i]; ++i) {
        eglInfo->_imp->handle = dlopen(sonames[i], RTLD_LAZY | RTLD_GLOBAL);
        if (eglInfo->_imp->handle) {
            break;
        }
    }

    if (!eglInfo->_imp->handle) {
        throw std::runtime_error("EGL: Failed to load EGL");
    }

    // EGL
    eglInfo->_imp->GetDisplay = (PFNEGLGETDISPLAYPROC)dlsym(eglInfo->_imp->handle, "eglGetDisplay");
    eglInfo->_imp->GetConfigs = (PFNEGLGETCONFIGSPROC)dlsym(eglInfo->_imp->handle, "eglGetConfigs");
    eglInfo->_imp->GetConfigAttrib = (PFNEGLGETCONFIGATTRIBPROC)dlsym(eglInfo->_imp->handle, "eglGetConfigAttrib");
    eglInfo->_imp->Initialize = (PFNEGLINITIALIZEPROC)dlsym(eglInfo->_imp->handle, "eglInitialize");
    eglInfo->_imp->BindAPI = (PFNEGLBINDAPIPROC)dlsym(eglInfo->_imp->handle, "eglBindAPI");
    eglInfo->_imp->ChooseConfig = (PFNEGLCHOOSECONFIGPROC)dlsym(eglInfo->_imp->handle, "eglChooseConfig");
    eglInfo->_imp->CreateContext = (PFNEGLCREATECONTEXTPROC)dlsym(eglInfo->_imp->handle, "eglCreateContext");
    eglInfo->_imp->GetCurrentContext = (PFNEGLGETCURRENTCONTEXTPROC)dlsym(eglInfo->_imp->handle, "eglGetCurrentContext");
    eglInfo->_imp->DestroyContext = (PFNEGLDESTROYCONTEXTPROC)dlsym(eglInfo->_imp->handle, "eglDestroyContext");
    eglInfo->_imp->CreateWindowSurface = (PFNEGLCREATEWINDOWSURFACEPROC)dlsym(eglInfo->_imp->handle, "eglCreateWindowSurface");
    eglInfo->_imp->DestroySurface = (PFNEGLDESTROYSURFACEPROC)dlsym(eglInfo->_imp->handle, "eglDestroySurface");
    eglInfo->_imp->MakeCurrent = (PFNEGLMAKECURRENTPROC)dlsym(eglInfo->_imp->handle, "eglMakeCurrent");
    eglInfo->_imp->SwapBuffers = (PFNEGLSWAPBUFFERSPROC)dlsym(eglInfo->_imp->handle, "eglSwapBuffers");
    eglInfo->_imp->SwapInterval = (PFNEGLSWAPINTERVALPROC)getProcAddress(eglInfo, "eglSwapInterval");
    eglInfo->_imp->GetProcAddress = (PFNEGLGETPROCADDRESSPROC)dlsym(eglInfo->_imp->handle, "eglGetProcAddress");
    eglInfo->_imp->QueryString = (PFNEGLQUERYSTRINGPROC)dlsym(eglInfo->_imp->handle, "eglQueryString");
    eglInfo->_imp->Terminate = (PFNEGLTERMINATEPROC)dlsym(eglInfo->_imp->handle, "eglTerminate");
    eglInfo->_imp->ReleaseThread = (PFNEGLRELEASETHREADPROC)dlsym(eglInfo->_imp->handle, "eglReleaseThread");

    eglInfo->getProcAddress = (GLADloadproc)eglInfo->_imp->GetProcAddress;

    eglInfo->_imp->dpy = eglInfo->_imp->GetDisplay(eglInfo->_imp->wayland.display);
    if (eglInfo->_imp->dpy == EGL_NO_DISPLAY) {
        throw std::runtime_error("EGL: No available EGL display");
    }

    EGLBoolean ret = eglInfo->_imp->Initialize(eglInfo->_imp->dpy, &eglInfo->_imp->major, &eglInfo->_imp->minor);
    if (!ret) {
        throw std::runtime_error("EGL: Could not initialize EGL");
    }

    if ((eglInfo->_imp->major == 1) && (eglInfo->_imp->minor < 4)) {
        throw std::runtime_error("EGL: EGL version 1.4 or higher is required");
    }

    if ((eglInfo->_imp->major > 1) || (eglInfo->_imp->minor >= 5) || extensionSupported(eglInfo, "EGL_KHR_create_context")) {
        eglInfo->_imp->KHR_create_context = EGL_TRUE;
    }

    if (extensionSupported(eglInfo, "EGL_EXT_platform_wayland")) {
        eglInfo->_imp->CreatePlatformWindowSurfaceEXT = (PFNEGLCREATEPLATFORMWINDOWSURFACEEXTPROC)getProcAddress(eglInfo, "eglCreatePlatformWindowSurfaceEXT");

        if (eglInfo->_imp->CreatePlatformWindowSurfaceEXT) {
            eglInfo->_imp->EXT_platform_wayland = EGL_TRUE;
        }
    }

    if (extensionSupported(eglInfo, "EGL_KHR_gl_colorspace")) {
        eglInfo->_imp->KHR_gl_colorspace = EGL_TRUE;
    }

    if (extensionSupported(eglInfo, "EGL_EXT_create_context_robustness")) {
        eglInfo->_imp->EXT_create_context_robustness = EGL_TRUE;
    }

    if (extensionSupported(eglInfo, "EGL_KHR_context_flush_control")) {
        eglInfo->_imp->KHR_context_flush_control = EGL_TRUE;
    }
} // initGLXData

void
OSGLContext_wayland::destroyEGLData(OSGLContext_egl_data* eglInfo)
{
    if (eglInfo->_imp->dpy != EGL_NO_DISPLAY) {
        eglInfo->_imp->Terminate(eglInfo->_imp->dpy);
        eglInfo->_imp->ReleaseThread();
    }
    if (eglInfo->_imp->handle != nullptr) {
        dlclose(eglInfo->_imp->handle);
        eglInfo->_imp->handle = nullptr;
    }
    if (eglInfo->_imp->wayland.compositor != nullptr) {
        wl_compositor_destroy(eglInfo->_imp->wayland.compositor);
        eglInfo->_imp->wayland.compositor = nullptr;
    }
    if (eglInfo->_imp->wayland.display != nullptr) {
        wl_display_flush(eglInfo->_imp->wayland.display);
        wl_display_disconnect(eglInfo->_imp->wayland.display);
        eglInfo->_imp->wayland.display = nullptr;
    }
}

// Returns the specified attribute of the specified EGLConfig
//
static int
getFBConfigAttrib(const OSGLContext_egl_data* eglInfo,
                  const EGLConfig& fbconfig,
                  int attrib)
{
    int value;

    eglInfo->_imp->GetConfigAttrib(eglInfo->_imp->dpy, fbconfig, attrib, &value);

    return value;
}

// Return a list of available and usable framebuffer configs
//
static void
chooseFBConfig(const OSGLContext_egl_data* eglInfo,
               const FramebufferConfig& desired,
               EGLConfig* result)
{
    if (!eglInfo->_imp->dpy) {
        throw std::runtime_error("EGL: No DISPLAY available");
    }

    int nativeCount;
    eglInfo->_imp->GetConfigs(eglInfo->_imp->dpy, nullptr, 0, &nativeCount);
    if (nativeCount == 0) {
        throw std::runtime_error("EGL: No EGLConfigs returned");
    }
    std::vector<EGLConfig> nativeConfigs(nativeCount);
    eglInfo->_imp->GetConfigs(eglInfo->_imp->dpy, nativeConfigs.data(), nativeCount, &nativeCount);

    std::vector<FramebufferConfig> usableConfigs(nativeCount);
    int usableCount = 0;

    for (int i = 0; i < nativeCount; ++i) {
        const EGLConfig& n = nativeConfigs[i];
        FramebufferConfig& u = usableConfigs[usableCount];

        // Only consider window EGLConfigs
        if (!(getFBConfigAttrib(eglInfo, n, EGL_SURFACE_TYPE) & EGL_WINDOW_BIT)) {
            continue;
        }

        u.redBits = getFBConfigAttrib(eglInfo, n, EGL_RED_SIZE);
        u.greenBits = getFBConfigAttrib(eglInfo, n, EGL_GREEN_SIZE);
        u.blueBits = getFBConfigAttrib(eglInfo, n, EGL_BLUE_SIZE);

        u.alphaBits = getFBConfigAttrib(eglInfo, n, EGL_ALPHA_SIZE);
        u.depthBits = getFBConfigAttrib(eglInfo, n, EGL_DEPTH_SIZE);
        u.stencilBits = getFBConfigAttrib(eglInfo, n, EGL_STENCIL_SIZE);

        u.accumRedBits = FramebufferConfig::ATTR_DONT_CARE;
        u.accumGreenBits = FramebufferConfig::ATTR_DONT_CARE;
        u.accumBlueBits = FramebufferConfig::ATTR_DONT_CARE;
        u.accumAlphaBits = FramebufferConfig::ATTR_DONT_CARE;

        u.auxBuffers = desired.auxBuffers;

        u.stereo = desired.stereo;

        u.doublebuffer = desired.doublebuffer;

        u.samples = getFBConfigAttrib(eglInfo, n, EGL_SAMPLES);

        u.sRGB = desired.sRGB;

        u.handle = (uintptr_t)n;
        usableCount++;
    }

    const FramebufferConfig& closest = OSGLContext::chooseFBConfig(desired, usableConfigs, usableCount);
    *result = (EGLConfig)closest.handle;
} // chooseFBConfig

// Create the Wayland window (and its colormap)
//
void
OSGLContext_waylandPrivate::createWindow(OSGLContext_egl_data* eglInfo, const EGLConfig& native, int depth)
{
    const int width = 32;
    const int height = 32;

    // Create the actual window
    waylandWindow.handle = wl_compositor_create_surface(eglInfo->_imp->wayland.compositor);

    if (!waylandWindow.handle) {
        throw std::runtime_error("Wayland: Failed to create surface");
    }

    waylandWindow.width = width;
    waylandWindow.height = height;

    waylandWindow.native = wl_egl_window_create(waylandWindow.handle, waylandWindow.width, waylandWindow.height);
    if (waylandWindow.native == nullptr) {
        throw std::runtime_error("EGL: Failed to create native window");
    }

    wl_surface_set_opaque_region(waylandWindow.handle, nullptr);

    wl_surface_commit(waylandWindow.handle);

    if (eglInfo->_imp->EXT_platform_wayland) {
        eglSurfaceHandle = eglInfo->_imp->CreatePlatformWindowSurfaceEXT(eglInfo->_imp->dpy, native, waylandWindow.native, nullptr);
    } else {
        eglSurfaceHandle = eglInfo->_imp->CreateWindowSurface(eglInfo->_imp->dpy, native, reinterpret_cast<EGLNativeWindowType>(waylandWindow.native), nullptr);
    }
    if (eglSurfaceHandle == EGL_NO_SURFACE) {
        throw std::runtime_error("EGL: Failed to create surface");
    }
} // createWindow

static inline void
setEGLattrib(std::vector<int>& attribs, int attribName, int attribValue)
{
    attribs.push_back(attribName);
    attribs.push_back(attribValue);
}

void
OSGLContext_waylandPrivate::createContextEGL(OSGLContext_egl_data* eglInfo,
                                             const FramebufferConfig& fbconfig,
                                             int major,
                                             int minor,
                                             bool coreProfile,
                                             int rendererID,
                                             const OSGLContext_wayland* shareContext)
{
    EGLConfig native;
    EGLContext share = shareContext ? shareContext->_imp->eglContextHandle : EGL_NO_CONTEXT;

    chooseFBConfig(eglInfo, fbconfig, &native);

    if (!eglInfo->_imp->BindAPI(EGL_OPENGL_API)) {
        throw std::runtime_error("EGL: Could not bind with OpenGL API");
    }

    std::vector<int> attribs;
    std::vector<int> ctxAttribs;
    int mask = 0, n;

    if (coreProfile) {
        mask |= EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR;
    } else {
        mask |= EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT_KHR;
    }

    // For EGL contexts we have to request for specific API versions, in our case is OpenGL 2.0
    if ((major < 2) || (minor < 0)) {
        major = 2;
        minor = 0;
    }

    if (eglInfo->_imp->KHR_create_context) {
        setEGLattrib(ctxAttribs, EGL_CONTEXT_MAJOR_VERSION_KHR, major);
        setEGLattrib(ctxAttribs, EGL_CONTEXT_MINOR_VERSION_KHR, minor);

        if (mask) {
            setEGLattrib(ctxAttribs, EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR, mask);
        }
    } else {
        setEGLattrib(ctxAttribs, EGL_CONTEXT_CLIENT_VERSION, major);
    }

    if ((rendererID != -1) && (rendererID != 0)) {
        setEGLattrib(attribs, EGL_CONFIG_ID, rendererID);
    }

    setEGLattrib(attribs, EGL_SURFACE_TYPE, EGL_WINDOW_BIT);
    setEGLattrib(attribs, EGL_RED_SIZE, fbconfig.redBits);
    setEGLattrib(attribs, EGL_GREEN_SIZE, fbconfig.greenBits);
    setEGLattrib(attribs, EGL_BLUE_SIZE, fbconfig.blueBits);
    setEGLattrib(attribs, EGL_ALPHA_SIZE, fbconfig.alphaBits);
    setEGLattrib(attribs, EGL_DEPTH_SIZE, fbconfig.depthBits);
    setEGLattrib(attribs, EGL_STENCIL_SIZE, fbconfig.stencilBits);
    setEGLattrib(attribs, EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT);

    setEGLattrib(attribs, EGL_NONE, EGL_NONE);
    setEGLattrib(ctxAttribs, EGL_NONE, EGL_NONE);

    eglInfo->_imp->ChooseConfig(eglInfo->_imp->dpy, attribs.data(), &native, 1, &n);
    if (n != 1) {
        throw std::runtime_error("EGL: Failed to choose config");
    }

    eglContextHandle = eglInfo->_imp->CreateContext(eglInfo->_imp->dpy, native, share, ctxAttribs.data());
    if (eglContextHandle == EGL_NO_CONTEXT) {
        throw std::runtime_error("EGL: Failed to create context");
    }
} // createContextEGL

OSGLContext_wayland::OSGLContext_wayland(const FramebufferConfig& pixelFormatAttrs,
                                         int major,
                                         int minor,
                                         bool coreProfile,
                                         const GLRendererID& rendererID,
                                         const OSGLContext_wayland* shareContext)
    : OSGLContext_xdg()
    , _imp(new OSGLContext_waylandPrivate())
{
    auto eglInfo = const_cast<OSGLContext_egl_data*>(appPTR->getEGLData());

    assert(eglInfo);

    EGLConfig native;

    int depth;

    chooseFBConfig(eglInfo, pixelFormatAttrs, &native);
    _imp->createWindow(eglInfo, native, depth);
    _imp->createContextEGL(eglInfo, pixelFormatAttrs, major, minor, coreProfile, rendererID.renderID, shareContext);
}

OSGLContext_wayland::~OSGLContext_wayland()
{
    const OSGLContext_egl_data* eglInfo = appPTR->getEGLData();

    assert(eglInfo);

    if (_imp->eglSurfaceHandle != EGL_NO_SURFACE) {
        eglInfo->_imp->DestroySurface(eglInfo->_imp->dpy, _imp->eglSurfaceHandle);
        _imp->eglSurfaceHandle = EGL_NO_SURFACE;
    }
    if (_imp->waylandWindow.native != nullptr) {
        wl_egl_window_destroy(_imp->waylandWindow.native);
        _imp->waylandWindow.native = nullptr;
    }
    if (_imp->eglContextHandle != EGL_NO_CONTEXT) {
        if (eglInfo->_imp->GetCurrentContext() == _imp->eglContextHandle) {
            OSGLContext_wayland::makeContextCurrent(nullptr);
        }
        eglInfo->_imp->DestroyContext(eglInfo->_imp->dpy, _imp->eglContextHandle);
        _imp->eglContextHandle = EGL_NO_CONTEXT;
    }
    if (_imp->waylandWindow.handle != nullptr) {
        wl_surface_destroy(_imp->waylandWindow.handle);
        _imp->waylandWindow.handle = nullptr;
    }
}

bool
OSGLContext_wayland::makeContextCurrent(const OSGLContext_wayland* context)
{
    const OSGLContext_egl_data* eglInfo = appPTR->getEGLData();

    assert(eglInfo);

    if (eglInfo->_imp->dpy == nullptr) {
        return false;
    }

    if (context != nullptr) {
        if (!eglInfo->_imp->BindAPI(EGL_OPENGL_API)) {
            throw std::runtime_error("EGL: Could not bind with OpenGL API");
        }
        return eglInfo->_imp->MakeCurrent(eglInfo->_imp->dpy, context->_imp->eglSurfaceHandle, context->_imp->eglSurfaceHandle, context->_imp->eglContextHandle);
    } else {
        return eglInfo->_imp->MakeCurrent(eglInfo->_imp->dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    }
}

bool
OSGLContext_wayland::threadHasACurrentContext() {
    const OSGLContext_egl_data* eglInfo = appPTR->getEGLData();

    assert(eglInfo);

    return eglInfo != nullptr && eglInfo->_imp->GetCurrentContext() != nullptr;
}

void
OSGLContext_wayland::swapBuffers()
{
    const OSGLContext_egl_data* eglInfo = appPTR->getEGLData();

    assert(eglInfo);
    eglInfo->_imp->SwapBuffers(eglInfo->_imp->dpy, _imp->eglSurfaceHandle);
}

void
OSGLContext_wayland::swapInterval(int interval)
{
    const OSGLContext_egl_data* eglInfo = appPTR->getEGLData();

    assert(eglInfo);

    eglInfo->_imp->SwapInterval(eglInfo->_imp->dpy, interval);
}

void
OSGLContext_wayland::getGPUInfos(std::list<OpenGLRendererInfo>& renderers)
{
    const OSGLContext_egl_data* eglInfo = appPTR->getEGLData();

    assert(eglInfo);
    if (!eglInfo) {
        return;
    }
    if (!eglInfo->_imp->dpy) {
        return;
    }

    std::unique_ptr<OSGLContext_wayland> context;
    try {
        context.reset(new OSGLContext_wayland(FramebufferConfig(), GLVersion.major, GLVersion.minor, false, GLRendererID(), nullptr));
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;

        return;
    }

    if (!makeContextCurrent(context.get())) {
        return;
    }

    try {
        OSGLContext::checkOpenGLVersion();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;

        return;
    }

    OpenGLRendererInfo info;
    info.vendorName = std::string(reinterpret_cast<const char*>(glGetString(GL_VENDOR)));
    info.rendererName = std::string(reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
    info.glVersionString = std::string(reinterpret_cast<const char*>(glGetString(GL_VERSION)));
    info.glslVersionString = std::string(reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION)));
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &info.maxTextureSize);
    // We don't have any way to get memory size, set it to 0
    info.maxMemBytes = 0;
    info.rendererID.renderID = -1;
    renderers.push_back(info);
} // OSGLContext_wayland::getGPUInfos

NATRON_NAMESPACE_EXIT

#else // __NATRON_WAYLAND__

NATRON_NAMESPACE_ENTER

// Place stubs in case that Wayland wasn't enabled
struct OSGLContext_egl_dataPrivate
{
    void* moot;
};

OSGLContext_egl_data::OSGLContext_egl_data()
{
    throw std::runtime_error("EGL: Wayland contexts are not supported and need to be enabled at build time");
}

OSGLContext_egl_data::~OSGLContext_egl_data()
{
}

struct OSGLContext_waylandPrivate
{
    void* moot;
};

OSGLContext_wayland::OSGLContext_wayland(const FramebufferConfig&,
                                         int,
                                         int,
                                         bool,
                                         const GLRendererID&,
                                         const OSGLContext_wayland*)
    : OSGLContext_xdg()
{
}

OSGLContext_wayland::~OSGLContext_wayland()
{
}

void
OSGLContext_wayland::initEGLData(OSGLContext_egl_data*)
{
}

void
OSGLContext_wayland::destroyEGLData(OSGLContext_egl_data*)
{
}

void
OSGLContext_wayland::swapBuffers()
{
}

void
OSGLContext_wayland::swapInterval(int)
{
}

bool
OSGLContext_wayland::makeContextCurrent(const OSGLContext_wayland*)
{
    return false;
}

void
OSGLContext_wayland::getGPUInfos(std::list<OpenGLRendererInfo>&)
{
}

NATRON_NAMESPACE_EXIT

#endif // __NATRON_WAYLAND__

#endif // __NATRON_LINUX__
