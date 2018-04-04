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

#include "OSGLContext_mac.h"

#ifdef __NATRON_OSX__

#include <stdexcept>

#include <QDebug>

#include "Global/GLIncludes.h"
#ifdef DEBUG
#include "Global/FloatingPointExceptions.h"
#endif

//#import <Cocoa/Cocoa.h>
#include <AvailabilityMacros.h>

/* Silence deprecated OpenGL warnings. */
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090
#include <OpenGL/OpenGLAvailability.h>
#undef OPENGL_DEPRECATED
#undef OPENGL_DEPRECATED_MSG
#define OPENGL_DEPRECATED(from, to)
#define OPENGL_DEPRECATED_MSG(from, to, msg)
#endif

#include <OpenGL/OpenGL.h> // necessary for CGL, even if glad was included just above
#include <OpenGL/CGLTypes.h>
#include <OpenGL/CGLRenderers.h>

#include "Engine/OSGLFunctions.h"
#include "Engine/OSGLContext.h"
#include "Engine/AppManager.h"

// see Availability.h
#ifdef __MAC_OS_X_VERSION_MIN_REQUIRED
// code only compiled when targeting Mac OS X and not iPhone
// note use of 1070 instead of __MAC_10_7
#if __MAC_OS_X_VERSION_MIN_REQUIRED < 1070
// code in here might run on pre-Lion OS
#ifndef kCGLPFATripleBuffer
#define kCGLPFATripleBuffer ((CGLPixelFormatAttribute)3)
#endif
#ifndef kCGLRPVideoMemoryMegabytes
#define kCGLRPVideoMemoryMegabytes ((CGLRendererProperty)131)
#endif
#ifndef kCGLRPTextureMemoryMegabytes
#define kCGLRPTextureMemoryMegabytes ((CGLRendererProperty)132)
#endif
#ifndef kCGLCECrashOnRemovedFunctions
#define kCGLCECrashOnRemovedFunctions ((CGLContextEnable)316)
#endif
#ifndef kCGLPFAOpenGLProfile
#define kCGLPFAOpenGLProfile ((CGLPixelFormatAttribute)99)
#endif
#ifndef kCGLOGLPVersion_Legacy
#define kCGLOGLPVersion_Legacy 0x1000
#endif
#ifndef kCGLOGLPVersion_3_2_Core
#define kCGLOGLPVersion_3_2_Core 0x3200
#endif
#else
// code here can assume Lion or later
#endif

#if __MAC_OS_X_VERSION_MIN_REQUIRED < 1080
#ifndef kCGLPFABackingVolatile
#define kCGLPFABackingVolatile ((CGLPixelFormatAttribute)77)
#endif
#ifndef kCGLPFASupportsAutomaticGraphicsSwitching
#define kCGLPFASupportsAutomaticGraphicsSwitching ((CGLPixelFormatAttribute)101)
#endif
#endif

#if __MAC_OS_X_VERSION_MIN_REQUIRED < 1090
#ifndef kCGLOGLPVersion_GL3_Core
#define kCGLOGLPVersion_GL3_Core 0x3200
#endif
#ifndef kCGLOGLPVersion_GL4_Core
#define kCGLOGLPVersion_GL4_Core 0x4100
#endif
#ifndef kCGLRPMajorGLVersion
#define kCGLRPMajorGLVersion ((CGLRendererProperty)133)
#endif
#endif

// OpenGL/CGLProfiler.h seems to no longer be included with XCode
#define kCGLCPComment                        ((CGLContextParameter)1232)
#define kCGLCPDumpState                      ((CGLContextParameter)1233)
#define kCGLCPEnableForceFlush               ((CGLContextParameter)1234)
#define kCGLGOComment                        ((CGLGlobalOption)1506)
#define kCGLGOEnableFunctionTrace            ((CGLGlobalOption)1507)
#define kCGLGOEnableFunctionStatistics       ((CGLGlobalOption)1508)
#define kCGLGOResetFunctionTrace             ((CGLGlobalOption)1509)
#define kCGLGOPageBreak                      ((CGLGlobalOption)1510)
#define kCGLGOResetFunctionStatistics        ((CGLGlobalOption)1511)
#define kCGLGOEnableDebugAttach              ((CGLGlobalOption)1512)
#define kCGLGOHideObjects                    ((CGLGlobalOption)1513)
#define kCGLGOEnableBreakpoint               ((CGLGlobalOption)1514)
#define kCGLGOForceSlowRenderingPath         ((CGLGlobalOption)1609)
#define kCGLGODisableImmediateRenderPath     ((CGLGlobalOption)1610)
#define kCGLGODisableCVARenderPath           ((CGLGlobalOption)1611)
#define kCGLGODisableVARRenderPath           ((CGLGlobalOption)1612)
#define kCGLGOForceWireframeRendering        ((CGLGlobalOption)1613)
#define kCGLGOSubmitOnImmediateRenderCommand ((CGLGlobalOption)1614)
#define kCGLGOSubmitOnCVARenderCommand       ((CGLGlobalOption)1615)
#define kCGLGOSubmitOnVAORenderCommand       ((CGLGlobalOption)1616)
#define kCGLGOSubmitOnClearCommand           ((CGLGlobalOption)1617)
#define kCGLGOForceSoftwareTransformLighting ((CGLGlobalOption)1618)
#define kCGLGOForceSoftwareTexgen            ((CGLGlobalOption)1619)
#define kCGLGOForceSoftwareTRUFORM_ATI       ((CGLGlobalOption)1620)
#define kCGLGOForceSoftwareVertexShaders     ((CGLGlobalOption)1621)
#define kCGLGODisableFragmentShaders_ATI     ((CGLGlobalOption)1622)
#define kCGLGODisableTexturing               ((CGLGlobalOption)1623)
#define kCGLGOOutlineTexture                 ((CGLGlobalOption)1624)
#define kCGLGOOutlineTextureColor            ((CGLGlobalOption)1625)
#define kCGLGOForceSlowBitmapPath            ((CGLGlobalOption)1626)
#define kCGLGODisableBitmap                  ((CGLGlobalOption)1627)
#define kCGLGOForceSlowReadPixelsPath        ((CGLGlobalOption)1630)
#define kCGLGODisableReadPixels              ((CGLGlobalOption)1631)
#define kCGLGOOutlineReadPixelsBuffer        ((CGLGlobalOption)1632)
#define kCGLGOOutlineReadPixelsBufferColor   ((CGLGlobalOption)1633)
#define kCGLGOForceSlowDrawPixelsPath        ((CGLGlobalOption)1634)
#define kCGLGODisableDrawPixels              ((CGLGlobalOption)1635)
#define kCGLGOOutlineDrawPixelsBuffer        ((CGLGlobalOption)1636)
#define kCGLGOOutlineDrawPixelsBufferColor   ((CGLGlobalOption)1637)
#define kCGLGOForceSlowCopyPixelsPath        ((CGLGlobalOption)1638)
#define kCGLGODisableCopyPixels              ((CGLGlobalOption)1639)
#define kCGLGOOutlineCopyPixelsBuffer        ((CGLGlobalOption)1640)
#define kCGLGOOutlineCopyPixelsBufferColor   ((CGLGlobalOption)1641)
#define kCGLGOMakeAllGLObjectsRequireUpdate  ((CGLGlobalOption)1642)
#define kCGLGOMakeAllGLStateRequireUpdate    ((CGLGlobalOption)1643)

#endif // __MAC_OS_X_VERSION_MIN_REQUIRED

NATRON_NAMESPACE_ENTER

class OSGLContext_mac::Implementation
{
public:
    CGLContextObj context;
};

#if 0
void
OSGLContext_mac::createWindow()
{
    /*_nsWindow.delegate = [[GLFWWindowDelegate alloc] initWithGlfwWindow:window];
       if (window->ns.delegate == nil)
       {
        _glfwInputError(GLFW_PLATFORM_ERROR,
                        "Cocoa: Failed to create window delegate");
        return GLFW_FALSE;
       }*/

    const int width = 32;
    const int height = 32;
    NSRect contentRect = NSMakeRect(0, 0, width, height);

    _nsWindow.object = [[GLFWWindow alloc]
                        initWithContentRect:contentRect
                        styleMask:getStyleMask(window)
                        backing:NSBackingStoreBuffered
                        defer:NO];

    if (_nsWindow.object == nil) {
        throw std::runtime_error("Cocoa: Failed to create window");
    }

    [_nsWindow.object center];

    _nsWindow.view = [[GLFWContentView alloc] initWithGlfwWindow:window];

//#if defined(_GLFW_USE_RETINA)
    [_nsWindow.view setWantsBestResolutionOpenGLSurface:YES];
//#endif /*_GLFW_USE_RETINA*/

    [_nsWindow.object makeFirstResponder:_nsWindow.view];
    //[_nsWindow.object setDelegate:window->ns.delegate];
    [_nsWindow.object setContentView:_nsWindow.view];
    [_nsWindow.object setRestorable:NO];
}

#endif

static void
makeAttribsFromFBConfig(const FramebufferConfig& pixelFormatAttrs,
                        int major,
                        int /*minor*/,
                        bool coreProfile,
                        int rendererID,
                        std::vector<CGLPixelFormatAttribute> &attributes)
{
    // See https://developer.apple.com/library/mac/documentation/GraphicsImaging/Reference/CGL_OpenGL/#//apple_ref/c/tdef/CGLPixelFormatAttribute
    // for a reference to attributes
    /*
     * The program chose a config based on the fbconfigs or visuals.
     * Those are based on the attributes from CGL, so we probably
     * do want the closest match for the color, depth, and accum.
     */
    if (rendererID != -1) {
        attributes.push_back(kCGLPFARendererID);
        attributes.push_back( (CGLPixelFormatAttribute)rendererID );
    }
    attributes.push_back(kCGLPFAClosestPolicy);
    attributes.push_back(kCGLPFAAccelerated);
    /* Request an OpenGL 3.2 profile if one is available */
    GLint version_major, version_minor;
    CGLGetVersion(&version_major, &version_minor);
    if (version_major > 1 || (version_major == 1 && version_minor >= 3)) {
        attributes.push_back(kCGLPFAOpenGLProfile);
        if (coreProfile) {
            attributes.push_back( (CGLPixelFormatAttribute) kCGLOGLPVersion_3_2_Core );
        } else {
            attributes.push_back( (CGLPixelFormatAttribute) kCGLOGLPVersion_Legacy );
        }
    }
    if (pixelFormatAttrs.doublebuffer) {
        attributes.push_back(kCGLPFADoubleBuffer);
    }
    /*if (pixelFormatAttrs.stereo) {
       attributes.push_back(kCGLPFAStereo);
       }*/

    if (major <= 2) {
        if ( pixelFormatAttrs.auxBuffers && (pixelFormatAttrs.auxBuffers != FramebufferConfig::ATTR_DONT_CARE) ) {
            attributes.push_back(kCGLPFAAuxBuffers);
            attributes.push_back( (CGLPixelFormatAttribute)pixelFormatAttrs.auxBuffers );
        }
        if ( (pixelFormatAttrs.accumRedBits != FramebufferConfig::ATTR_DONT_CARE) &&
             ( pixelFormatAttrs.accumGreenBits != FramebufferConfig::ATTR_DONT_CARE) &&
             ( pixelFormatAttrs.accumBlueBits != FramebufferConfig::ATTR_DONT_CARE) &&
             ( pixelFormatAttrs.accumAlphaBits != FramebufferConfig::ATTR_DONT_CARE) ) {
            const int accumBits = pixelFormatAttrs.accumRedBits +
                                  pixelFormatAttrs.accumGreenBits +
                                  pixelFormatAttrs.accumBlueBits +
                                  pixelFormatAttrs.accumAlphaBits;

            attributes.push_back(kCGLPFAAccumSize);
            attributes.push_back( (CGLPixelFormatAttribute)accumBits );
        }

        /*
           Deprecated in OS X v10.7. This attribute must not be specified if the attributes array also requests a profile other than a legacy OpenGL profile; if present, pixel format creation fails.
         */

        //attributes.push_back(kCGLPFAOffScreen);
    }
    if ( (pixelFormatAttrs.redBits != FramebufferConfig::ATTR_DONT_CARE) &&
         ( pixelFormatAttrs.greenBits != FramebufferConfig::ATTR_DONT_CARE) &&
         ( pixelFormatAttrs.blueBits != FramebufferConfig::ATTR_DONT_CARE) ) {
        int colorBits = pixelFormatAttrs.redBits +
                        pixelFormatAttrs.greenBits +
                        pixelFormatAttrs.blueBits;

        // OS X needs non-zero color size, so set reasonable values
        if (colorBits == 0) {
            colorBits = 24;
        } else if (colorBits < 15) {
            colorBits = 15;
        }

        attributes.push_back(kCGLPFAColorSize);
        attributes.push_back( (CGLPixelFormatAttribute)colorBits );
    }


    if (pixelFormatAttrs.alphaBits != FramebufferConfig::ATTR_DONT_CARE) {
        attributes.push_back(kCGLPFAAlphaSize);
        attributes.push_back( (CGLPixelFormatAttribute)pixelFormatAttrs.alphaBits );
    }

    if (pixelFormatAttrs.depthBits != FramebufferConfig::ATTR_DONT_CARE) {
        attributes.push_back(kCGLPFADepthSize);
        attributes.push_back( (CGLPixelFormatAttribute)pixelFormatAttrs.depthBits );
    }

    if (pixelFormatAttrs.stencilBits != FramebufferConfig::ATTR_DONT_CARE) {
        attributes.push_back(kCGLPFAStencilSize);
        attributes.push_back( (CGLPixelFormatAttribute)pixelFormatAttrs.stencilBits );
    }

    // Use float buffers
    attributes.push_back(kCGLPFAColorFloat);

    if (pixelFormatAttrs.samples != FramebufferConfig::ATTR_DONT_CARE) {
        if (pixelFormatAttrs.samples == 0) {
            attributes.push_back(kCGLPFASampleBuffers);
            attributes.push_back( (CGLPixelFormatAttribute)0 );
        } else {
            attributes.push_back(kCGLPFAMultisample);
            attributes.push_back(kCGLPFASampleBuffers);
            attributes.push_back( (CGLPixelFormatAttribute)1 );
            attributes.push_back(kCGLPFASamples);
            attributes.push_back( (CGLPixelFormatAttribute)pixelFormatAttrs.samples );
        }
    }
    attributes.push_back( (CGLPixelFormatAttribute)0 );
} // makeAttribsFromFBConfig

OSGLContext_mac::OSGLContext_mac(const FramebufferConfig& pixelFormatAttrs,
                                 int major,
                                 int minor,
                                 bool coreProfile,
                                 const GLRendererID& rendererID,
                                 const OSGLContext_mac* shareContext)
    : _imp(new Implementation)
{
    std::vector<CGLPixelFormatAttribute> attributes;

    makeAttribsFromFBConfig(pixelFormatAttrs, major, minor, coreProfile, rendererID.renderID, attributes);

    CGLPixelFormatObj nativePixelFormat;
    GLint num; // stores the number of possible pixel formats
    CGLError errorCode;
    {
#ifdef DEBUG
        boost_adaptbx::floating_point::exception_trapping trap(0);
#endif
        errorCode = CGLChoosePixelFormat( &attributes[0], &nativePixelFormat, &num );
    }
    if (errorCode != kCGLNoError) {
        throw std::runtime_error("CGL: Failed to choose pixel format");
    }

    errorCode = CGLCreateContext( nativePixelFormat, shareContext ? shareContext->_imp->context : 0, &_imp->context );
    if (errorCode != kCGLNoError) {
        throw std::runtime_error("CGL: Failed to create context");
    }
    CGLDestroyPixelFormat( nativePixelFormat );

    //errorCode = CGLSetCurrentContext( context );

#if 0
    unsigned int attributeCount = 0;

    // Context robustness modes (GL_KHR_robustness) are not yet supported on
    // OS X but are not a hard constraint, so ignore and continue

    // Context release behaviors (GL_KHR_context_flush_control) are not yet
    // supported on OS X but are not a hard constraint, so ignore and continue

#define ADD_ATTR(x) { attributes[attributeCount++] = x; }
#define ADD_ATTR2(x, y) { ADD_ATTR(x); ADD_ATTR(y); }

    // Arbitrary array size here
    NSOpenGLPixelFormatAttribute attributes[40];

    ADD_ATTR(NSOpenGLPFAAccelerated);
    ADD_ATTR(NSOpenGLPFAClosestPolicy);

#if MAC_OS_X_VERSION_MAX_ALLOWED >= 101000
    if (major >= 4) {
        ADD_ATTR2(NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion4_1Core);
    } else
#endif /*MAC_OS_X_VERSION_MAX_ALLOWED*/
    {
        if (major >= 3) {
            ADD_ATTR2(NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion3_2Core);
        }
    }

    if (major <= 2) {
        if (pixelFormatAttrs.auxBuffers != FramebufferConfig::ATTR_DONT_CARE) {
            ADD_ATTR2(NSOpenGLPFAAuxBuffers, pixelFormatAttrs.auxBuffers);
        }

        if ( (pixelFormatAttrs.accumRedBits != FramebufferConfig::ATTR_DONT_CARE) &&
             ( pixelFormatAttrs.accumGreenBits != FramebufferConfig::ATTR_DONT_CARE) &&
             ( pixelFormatAttrs.accumBlueBits != FramebufferConfig::ATTR_DONT_CARE) &&
             ( pixelFormatAttrs.accumAlphaBits != FramebufferConfig::ATTR_DONT_CARE) ) {
            const int accumBits = pixelFormatAttrs.accumRedBits +
                                  pixelFormatAttrs.accumGreenBits +
                                  pixelFormatAttrs.accumBlueBits +
                                  pixelFormatAttrs.accumAlphaBits;

            ADD_ATTR2(NSOpenGLPFAAccumSize, accumBits);
        }
    }

    if ( (pixelFormatAttrs.redBits != FramebufferConfig::ATTR_DONT_CARE) &&
         ( pixelFormatAttrs.greenBits != FramebufferConfig::ATTR_DONT_CARE) &&
         ( pixelFormatAttrs.blueBits != FramebufferConfig::ATTR_DONT_CARE) ) {
        int colorBits = pixelFormatAttrs.redBits +
                        pixelFormatAttrs.greenBits +
                        pixelFormatAttrs.blueBits;

        // OS X needs non-zero color size, so set reasonable values
        if (colorBits == 0) {
            colorBits = 24;
        } else if (colorBits < 15) {
            colorBits = 15;
        }

        ADD_ATTR2(NSOpenGLPFAColorSize, colorBits);
    }

    if (pixelFormatAttrs.alphaBits != FramebufferConfig::ATTR_DONT_CARE) {
        ADD_ATTR2(NSOpenGLPFAAlphaSize, pixelFormatAttrs.alphaBits);
    }

    if (pixelFormatAttrs.depthBits != FramebufferConfig::ATTR_DONT_CARE) {
        ADD_ATTR2(NSOpenGLPFADepthSize, pixelFormatAttrs.depthBits);
    }

    if (pixelFormatAttrs.stencilBits != FramebufferConfig::ATTR_DONT_CARE) {
        ADD_ATTR2(NSOpenGLPFAStencilSize, pixelFormatAttrs.stencilBits);
    }

    if (pixelFormatAttrs.stereo) {
        ADD_ATTR(NSOpenGLPFAStereo);
    }

    if (pixelFormatAttrs.doublebuffer) {
        ADD_ATTR(NSOpenGLPFADoubleBuffer);
    }

    if (pixelFormatAttrs.samples != FramebufferConfig::ATTR_DONT_CARE) {
        if (pixelFormatAttrs.samples == 0) {
            ADD_ATTR2(NSOpenGLPFASampleBuffers, 0);
        } else   {
            ADD_ATTR2(NSOpenGLPFASampleBuffers, 1);
            ADD_ATTR2(NSOpenGLPFASamples, pixelFormatAttrs.samples);
        }
    }

    // NOTE: All NSOpenGLPixelFormats on the relevant cards support sRGB
    //       framebuffer, so there's no need (and no way) to request it

    ADD_ATTR(0);

#undef ADD_ATTR
#undef ADD_ATTR2

    _pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attributes];


    if (_pixelFormat == nil) {
        throw std::runtime_error("NSGL: Failed to find a suitable pixel format");
    }

    // Never share the OpenGL context for offscreen rendering
    NSOpenGLContext* share = NULL;
    _object = [[NSOpenGLContext alloc] initWithFormat:_pixelFormat shareContext:share];

    if (_object == nil) {
        throw std::runtime_error("NSGL: Failed to create OpenGL context");
    }

    //[_object setView:window->ns.view];
#endif // if 0
}

GCC_DIAG_OFF(deprecated-declarations)
static const char *RendererIDString(GLint ID)
{
    static char holder[256] = {0};
    // should compile on OSX 10.6 (we don't use defines or enums that wer introduced later)
    switch (ID & kCGLRendererIDMatchingMask) {
        case kCGLRendererGenericID: return "kCGLRendererGenericID";
        case kCGLRendererGenericFloatID: return "kCGLRendererGenericFloatID";
        case kCGLRendererAppleSWID: return "kCGLRendererAppleSWID";
        case kCGLRendererATIRage128ID: return "kCGLRendererATIRage128ID";
        case kCGLRendererATIRadeonID: return "kCGLRendererATIRadeonID";
        case kCGLRendererATIRageProID: return "kCGLRendererATIRageProID";
        case kCGLRendererATIRadeon8500ID: return "kCGLRendererATIRadeon8500ID";
        case kCGLRendererATIRadeon9700ID: return "kCGLRendererATIRadeon9700ID";
        case kCGLRendererATIRadeonX1000ID: return "kCGLRendererATIRadeonX1000ID"; // Radeon X1xxx
        case kCGLRendererATIRadeonX2000ID: return "kCGLRendererATIRadeonX2000ID"; // Radeon HD 2xxx and 4xxx
        //case kCGLRendererATIRadeonX3000ID:
        case 0x00021B00: return "kCGLRendererATIRadeonX3000ID"; // Radeon HD 5xxx and 6xxx
        //case kCGLRendererATIRadeonX4000ID:
        case 0x00021C00: return "kCGLRendererATIRadeonX4000ID"; // Radeon HD 7xxx
        case kCGLRendererGeForce2MXID: return "kCGLRendererGeForce2MXID"; // GeForce 2MX and 4MX
        case kCGLRendererGeForce3ID: return "kCGLRendererGeForce3ID"; // GeForce 3 and 4Ti
        case kCGLRendererGeForceFXID: return "kCGLRendererGeForceFXID"; // GeForce 5xxx, 6xxx and 7xxx, and Quadro FX 4500
        case kCGLRendererGeForce8xxxID: return "kCGLRendererGeForce8xxxID"; // GeForce 8xxx, 9xxx, 1xx, 2xx, and 3xx, and Quadro 4800
        //case kCGLRendererGeForceID: return "kCGLRendererGeForceID"; // GeForce 6xx, and Quadro 4000, K5000
        case kCGLRendererVTBladeXP2ID: return "kCGLRendererVTBladeXP2ID";
        case kCGLRendererIntel900ID: return "kCGLRendererIntel900ID"; // GMA 950
        case kCGLRendererIntelX3100ID: return "kCGLRendererIntelX3100ID";
        //case kCGLRendererIntelHDID:
        case 0x00024300: return "kCGLRendererIntelHDID"; // HD Graphics 3000
        //case kCGLRendererIntelHD4000ID:
        case 0x00024400: return "kCGLRendererIntelHD4000ID";
        //case kCGLRendererIntelHD5000ID:
        case 0x00024500: return "kCGLRendererIntelHD5000ID"; // Iris
        case kCGLRendererMesa3DFXID: return "kCGLRendererMesa3DFXID";
        default: {
            sprintf(holder, "Unknown(0x%x)", ID);
            return (const char *) holder;
        }
    }
}
GCC_DIAG_ON(deprecated-declarations)

void
OSGLContext_mac::getGPUInfos(std::list<OpenGLRendererInfo>& renderers)
{
    CGLContextObj curr_ctx = 0;

    curr_ctx = CGLGetCurrentContext (); // get current CGL context

    CGLRendererInfoObj rend;
    GLint nrend = 0;
    CGLQueryRendererInfo(0xffffffff, &rend, &nrend);
    for (GLint i = 0; i < nrend; ++i) {
        OpenGLRendererInfo info;
        GLint rendererID, haccelerated;
        if (CGLDescribeRenderer(rend, i,  kCGLRPRendererID, &rendererID) != kCGLNoError) {
            qDebug() << "Could not describe OpenGL renderer" << i;
            continue;
        }
        CGLDescribeRenderer(rend, i,  kCGLRPAccelerated, &haccelerated);

        // We don't allow renderers that are not hardware accelerated
        if (!haccelerated) {
            qDebug() << "OpenGL renderer" << RendererIDString(rendererID) << "is not accelerated";
            continue;
        }

        info.rendererID.renderID = rendererID;

#if !defined(MAC_OS_X_VERSION_10_7) || \
        MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_7
        GLint nBytesMem;
        CGLDescribeRenderer(rend, i,  kCGLRPVideoMemory, &nBytesMem);
        GLint nBytesTexMem;
        CGLDescribeRenderer(rend, i,  kCGLRPTextureMemory, &nBytesTexMem);
        qDebug() << "OpenGL renderer" << RendererIDString(rendererID) << "has" << nBytesMem/(1024.*1024.) << "MB of VRAM and" << nBytesTexMem/(1024.*1024.) << "MB of texture";

        info.maxMemBytes = (std::size_t)nBytesMem;
#else
        //Available in OS X v10.0 and later.
        //Deprecated in OS X v10.7.
        GLint nMBMem;
        CGLDescribeRenderer(rend, i,  kCGLRPVideoMemoryMegabytes, &nMBMem);
        GLint nMBTexMem;
        CGLDescribeRenderer(rend, i,  kCGLRPTextureMemoryMegabytes, &nMBTexMem);

        info.maxMemBytes = (std::size_t)nMBMem * 1e6;
        qDebug() << "OpenGL renderer" << RendererIDString(rendererID) << "has" << nMBMem << "MB of VRAM and" << nMBTexMem << "MB of texture";

        // Available in OS X v10.9
        //GLint maxOpenGLMajorVersion;
        //CGLDescribeRenderer(rend, i,  kCGLRPMajorGLVersion, &maxOpenGLMajorVersion);

#endif
        /*GLint fragCapable, clCapable
           CGLDescribeRenderer(rend, i,  kCGLRPGPUFragProcCapable, &fragCapable);
           CGLDescribeRenderer(rend, i,  kCGLRPAcceleratedCompute, &clCapable);*/
        //info.fragCapable = (bool)fragCapable;
        //info.clCapable = (bool)clCapable;


        // Create a dummy OpenGL context for that specific renderer to retrieve more infos
        // See https://developer.apple.com/library/mac/technotes/tn2080/_index.html
        std::vector<CGLPixelFormatAttribute> attributes;
        makeAttribsFromFBConfig(FramebufferConfig(), appPTR->getOpenGLVersionMajor(), appPTR->getOpenGLVersionMinor(), false, rendererID, attributes);
        CGLPixelFormatObj pixelFormat;
        GLint numPixelFormats;
        CGLError errorCode = CGLChoosePixelFormat(&attributes[0], &pixelFormat, &numPixelFormats);
        if (errorCode != kCGLNoError) {
            qDebug() << "Failed to created pixel format for renderer " << RendererIDString(rendererID);
            continue;
        }
        if (pixelFormat == NULL) {
            qDebug() << "No matching pixel format exists for renderer " << RendererIDString(rendererID);
            continue;
        }
        CGLContextObj cglContext;
        errorCode = CGLCreateContext(pixelFormat, 0, &cglContext);
        CGLDestroyPixelFormat (pixelFormat);
        if (errorCode != kCGLNoError) {
            qDebug() << "Failed to create OpenGL context for renderer " << RendererIDString(rendererID);
            continue;
        }
        errorCode = CGLSetCurrentContext (cglContext);
        if (errorCode != kCGLNoError) {
            qDebug() << "Failed to make OpenGL context current for renderer " << RendererIDString(rendererID);
            continue;
        }
        // get renderer strings
        info.rendererName = std::string( (char*)GL_GPU::GetString (GL_RENDERER) );
        info.vendorName = std::string( (char*)GL_GPU::GetString (GL_VENDOR) );
        info.glVersionString = std::string( (char*)GL_GPU::GetString (GL_VERSION) );
        info.glslVersionString = std::string( (char*)GL_GPU::GetString (GL_SHADING_LANGUAGE_VERSION) );
        //std::string strExt((char*)glGetString (GL_EXTENSIONS));

        GL_GPU::GetIntegerv (GL_MAX_TEXTURE_SIZE,
                       &info.maxTextureSize);

        CGLDestroyContext (cglContext);

        renderers.push_back(info);
    }


    CGLDestroyRendererInfo(rend);
    if (curr_ctx) {
        CGLSetCurrentContext (curr_ctx); // reset current CGL context
    }
} // OSGLContext_mac::getGPUInfos

void
OSGLContext_mac::makeContextCurrent(const OSGLContext_mac* context)
{
    /*if (context) {
        [context->_object makeCurrentContext];
       } else {
        [NSOpenGLContext clearCurrentContext];
       }*/
    if (context) {
        CGLSetCurrentContext(context->_imp->context);
    } else {
        CGLSetCurrentContext(0);
    }
}

void
OSGLContext_mac::swapBuffers()
{
    //   [_object flushBuffer];
    CGLFlushDrawable(_imp->context);
}

void
OSGLContext_mac::swapInterval(int interval)
{
    /*GLint sync = interval;
       [_object setValues:&sync forParameter:NSOpenGLCPSwapInterval];*/
    GLint value = interval;

    CGLSetParameter(_imp->context, kCGLCPSwapInterval, &value);
}

OSGLContext_mac::~OSGLContext_mac()
{
    CGLDestroyContext(_imp->context);
    _imp->context = 0;
    /*[_pixelFormat release];
       _pixelFormat = nil;

       [_object release];
       _object = nil;*/
}

NATRON_NAMESPACE_EXIT

#endif // __NATRON_OSX__
