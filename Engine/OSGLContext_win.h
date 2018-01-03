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

#ifndef OSGLCONTEXT_WIN_H
#define OSGLCONTEXT_WIN_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#ifdef __NATRON_WIN32__

#include "Global/GLIncludes.h"
#include <windows.h>

#include "Engine/EngineFwd.h"


NATRON_NAMESPACE_ENTER


#define WGL_NUMBER_PIXEL_FORMATS_ARB 0x2000
#define WGL_SUPPORT_OPENGL_ARB 0x2010
#define WGL_DRAW_TO_WINDOW_ARB 0x2001
#define WGL_PIXEL_TYPE_ARB 0x2013
#define WGL_TYPE_RGBA_ARB 0x202b
#define WGL_ACCELERATION_ARB 0x2003
#define WGL_NO_ACCELERATION_ARB 0x2025
#define WGL_RED_BITS_ARB 0x2015
#define WGL_RED_SHIFT_ARB 0x2016
#define WGL_GREEN_BITS_ARB 0x2017
#define WGL_GREEN_SHIFT_ARB 0x2018
#define WGL_BLUE_BITS_ARB 0x2019
#define WGL_BLUE_SHIFT_ARB 0x201a
#define WGL_ALPHA_BITS_ARB 0x201b
#define WGL_ALPHA_SHIFT_ARB 0x201c
#define WGL_ACCUM_BITS_ARB 0x201d
#define WGL_ACCUM_RED_BITS_ARB 0x201e
#define WGL_ACCUM_GREEN_BITS_ARB 0x201f
#define WGL_ACCUM_BLUE_BITS_ARB 0x2020
#define WGL_ACCUM_ALPHA_BITS_ARB 0x2021
#define WGL_DEPTH_BITS_ARB 0x2022
#define WGL_STENCIL_BITS_ARB 0x2023
#define WGL_AUX_BUFFERS_ARB 0x2024
#define WGL_STEREO_ARB 0x2012
#define WGL_DOUBLE_BUFFER_ARB 0x2011
#define WGL_SAMPLES_ARB 0x2042
#define WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB 0x20a9
#define WGL_CONTEXT_DEBUG_BIT_ARB 0x00000001
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB 0x00000002
#define WGL_CONTEXT_PROFILE_MASK_ARB 0x9126
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB 0x00000001
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x00000002
#define WGL_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB 0x2092
#define WGL_CONTEXT_FLAGS_ARB 0x2094
#define WGL_CONTEXT_ES2_PROFILE_BIT_EXT 0x00000004
#define WGL_CONTEXT_ROBUST_ACCESS_BIT_ARB 0x00000004
#define WGL_LOSE_CONTEXT_ON_RESET_ARB 0x8252
#define WGL_CONTEXT_RESET_NOTIFICATION_STRATEGY_ARB 0x8256
#define WGL_NO_RESET_NOTIFICATION_ARB 0x8261
#define WGL_CONTEXT_RELEASE_BEHAVIOR_ARB 0x2097
#define WGL_CONTEXT_RELEASE_BEHAVIOR_NONE_ARB 0
#define WGL_CONTEXT_RELEASE_BEHAVIOR_FLUSH_ARB 0x2098


typedef BOOL (WINAPI * PFNWGLSWAPINTERVALEXTPROC)(int);
typedef BOOL (WINAPI * PFNWGLGETPIXELFORMATATTRIBIVARBPROC)(HDC, int, int, UINT, const int*, int*);
typedef const char* (WINAPI * PFNWGLGETEXTENSIONSSTRINGEXTPROC)(void);
typedef const char* (WINAPI * PFNWGLGETEXTENSIONSSTRINGARBPROC)(HDC);
typedef HGLRC (WINAPI * PFNWGLCREATECONTEXTATTRIBSARBPROC)(HDC, HGLRC, const int*);
typedef HGLRC (WINAPI * WGLCREATECONTEXT_T)(HDC);
typedef BOOL (WINAPI * WGLDELETECONTEXT_T)(HGLRC);
typedef PROC (WINAPI * WGLGETPROCADDRESS_T)(LPCSTR);
typedef BOOL (WINAPI * WGLMAKECURRENT_T)(HDC, HGLRC);
typedef BOOL (WINAPI * WGLSHARELISTS_T)(HGLRC, HGLRC);

////////// https://www.opengl.org/registry/specs/NV/gpu_affinity.txt

DECLARE_HANDLE(HGPUNV);

typedef struct _GPU_DEVICE
{
    DWORD cb;
    CHAR DeviceName[32];
    CHAR DeviceString[128];
    DWORD Flags;
    RECT rcVirtualScreen;
} GPU_DEVICE, *PGPU_DEVICE;
typedef BOOL (*PFNWGLENUMGPUSNV)(UINT, HGPUNV*);
typedef BOOL (*PFNWGLENUMGPUDEVICESNV)(HGPUNV, UINT, PGPU_DEVICE);
typedef HDC (*PFNWGLCREATEAFFINITYDCNV)(const HGPUNV*);
typedef BOOL (*PFNWGLENUMGPUSFROMAFFINITYDCNV)(HDC, UINT, HGPUNV*);
typedef BOOL (*PFNWGLDELETEDCNV)(HDC);


///////// https://www.opengl.org/registry/specs/AMD/wgl_gpu_association.txt

#define WGL_GPU_VENDOR_AMD                 0x1F00
#define WGL_GPU_RENDERER_STRING_AMD        0x1F01
#define WGL_GPU_OPENGL_VERSION_STRING_AMD  0x1F02
#define WGL_GPU_FASTEST_TARGET_GPUS_AMD    0x21A2
#define WGL_GPU_RAM_AMD                    0x21A3
#define WGL_GPU_CLOCK_AMD                  0x21A4
#define WGL_GPU_NUM_PIPES_AMD              0x21A5
#define WGL_GPU_NUM_SIMD_AMD               0x21A6
#define WGL_GPU_NUM_RB_AMD                 0x21A7
#define WGL_GPU_NUM_SPI_AMD                0x21A8


typedef UINT (*PFNWGLGETGPUIDSAMD)(UINT maxCount, UINT *ids);
typedef INT (*PFNWGLGETGPUINFOAMD)(UINT id, INT property, GLenum dataType,
                                   UINT size, void *data);
typedef UINT (*PFNWGLGETCONTEXTGPUIDAMD)(HGLRC hglrc);
typedef HGLRC (*PFNWGLCREATEASSOCIATEDCONTEXTAMD)(UINT id);
typedef HGLRC (*PFNWGLCREATEASSOCIATEDCONTEXTATTRIBSAMD)(UINT id, HGLRC hShareContext,
                                                         const int *attribList);
typedef BOOL (*PFNWGLDELETEASSOCIATEDCONTEXTAMD)(HGLRC hglrc);
typedef BOOL (*PFNWGLMAKEASSOCIATEDCONTEXTCURRENTAMD)(HGLRC hglrc);
typedef HGLRC (*PFNWGLGETCURRENTASSOCIATEDCONTEXTAMD)(void);
typedef VOID (*PFNWGLBLITCONTEXTFRAMEBUFFERAMD)(HGLRC dstCtx, GLint srcX0, GLint srcY0,
                                                GLint srcX1, GLint srcY1, GLint dstX0,
                                                GLint dstY0, GLint dstX1, GLint dstY1,
                                                GLbitfield mask, GLenum filter);


/////////// http://developer.download.nvidia.com/opengl/specs/GL_NVX_gpu_memory_info.txt

#define GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX          0x9047
#define GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX    0x9048
#define GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX  0x9049
#define GPU_MEMORY_INFO_EVICTION_COUNT_NVX            0x904A
#define GPU_MEMORY_INFO_EVICTED_MEMORY_NVX            0x904B


struct OSGLContext_wgl_data
{
    HINSTANCE instance;
    WGLCREATECONTEXT_T CreateContext;
    WGLDELETECONTEXT_T DeleteContext;
    WGLGETPROCADDRESS_T GetProcAddress;
    WGLMAKECURRENT_T MakeCurrent;
    WGLSHARELISTS_T ShareLists;
    GLboolean extensionsLoaded;
    PFNWGLSWAPINTERVALEXTPROC SwapIntervalEXT;
    PFNWGLGETPIXELFORMATATTRIBIVARBPROC GetPixelFormatAttribivARB;
    PFNWGLGETEXTENSIONSSTRINGEXTPROC GetExtensionsStringEXT;
    PFNWGLGETEXTENSIONSSTRINGARBPROC GetExtensionsStringARB;
    PFNWGLCREATECONTEXTATTRIBSARBPROC CreateContextAttribsARB;
    PFNWGLENUMGPUSNV EnumGpusNV;
    PFNWGLENUMGPUDEVICESNV EnumGpuDevicesNV;
    PFNWGLCREATEAFFINITYDCNV CreateAffinityDCNV;
    PFNWGLENUMGPUSFROMAFFINITYDCNV EnumGpusFromAffinityDCNV;
    PFNWGLDELETEDCNV DeleteDCNV;
    PFNWGLGETGPUIDSAMD GetGpuIDAMD;
    PFNWGLGETGPUINFOAMD GetGPUInfoAMD;
    PFNWGLGETCONTEXTGPUIDAMD GetContextGpuIDAMD;
    PFNWGLCREATEASSOCIATEDCONTEXTAMD CreateAssociatedContextAMD;
    PFNWGLCREATEASSOCIATEDCONTEXTATTRIBSAMD CreateAssociatedContextAttribsAMD;
    PFNWGLDELETEASSOCIATEDCONTEXTAMD DeleteAssociatedContextAMD;
    PFNWGLMAKEASSOCIATEDCONTEXTCURRENTAMD MakeAssociatedContetCurrentAMD;
    PFNWGLGETCURRENTASSOCIATEDCONTEXTAMD GetCurrentAssociatedContextAMD;
    PFNWGLBLITCONTEXTFRAMEBUFFERAMD BlitContextFrameBufferAMD;
    GLboolean NV_gpu_affinity;
    GLboolean AMD_gpu_association;
    GLboolean NVX_gpu_memory_info;
    GLboolean EXT_swap_control;
    GLboolean ARB_multisample;
    GLboolean ARB_framebuffer_sRGB;
    GLboolean EXT_framebuffer_sRGB;
    GLboolean ARB_pixel_format;
    GLboolean ARB_create_context;
    GLboolean ARB_create_context_profile;
    GLboolean EXT_create_context_es2_profile;
    GLboolean ARB_create_context_robustness;
    GLboolean ARB_context_flush_control;
};

class OSGLContext_win
{
public:

    OSGLContext_win(const FramebufferConfig& pixelFormatAttrs,
                    int major,
                    int minor,
                    bool coreProfile,
                    const GLRendererID &rendererID,
                    const OSGLContext_win* shareContext);

    ~OSGLContext_win();

    static bool makeContextCurrent(const OSGLContext_win* context);

    void swapBuffers();

    void swapInterval(int interval);

    static void initWGLData(OSGLContext_wgl_data* wglInfo);
    bool loadWGLExtensions(OSGLContext_wgl_data* wglInfo);
    static void destroyWGLData(OSGLContext_wgl_data* wglInfo);
    static void getGPUInfos(std::list<OpenGLRendererInfo>& renderers);

private:

    int getPixelFormatAttrib(const OSGLContext_wgl_data* wglInfo, int pixelFormat, int attrib);

    void createGLContext(const FramebufferConfig& pixelFormatAttrs, int major, int minor, bool coreProfile, const GLRendererID &rendererID, const OSGLContext_win* shareContext);

    bool analyzeContextWGL(const FramebufferConfig& pixelFormatAttrs, int major, int minor);

    void destroyWindow();

    void destroyContext();

    bool extensionSupported(const char* extension, OSGLContext_wgl_data* data);

    // WGL-specific per-context data
    HDC _dc;
    HGLRC _handle;
    int _interval;
    HWND _windowHandle;
};


NATRON_NAMESPACE_EXIT

#endif // __NATRON_WIN32__

#endif // OSGLCONTEXT_WIN_H
