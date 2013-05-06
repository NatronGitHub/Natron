
#ifndef _ofxOpenGLRender_h_
#define _ofxOpenGLRender_h_

/*
Software License :

Copyright (c) 2010, The Open Effects Association Ltd. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.
    * Neither the name The Foundry Visionmongers Ltd, nor the names of its
      contributors may be used to endorse or promote products derived from this
      software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#ifdef __cplusplus
extern "C" {
#endif

/** @file ofxOpenGLRender.h

	This file contains an optional suite for performing OpenGL accelerated
	rendering of OpenFX Image Effect Plug-ins.  For details see
        \ref ofxOpenGLRender.
*/

/** @brief The name of the OpenGL render suite, used to fetch from a host
via OfxHost::fetchSuite
*/
#define kOfxOpenGLRenderSuite			"OfxImageEffectOpenGLRenderSuite"
//#define kOfxOpenGLRenderSuite_ext		"OfxImageEffectOpenGLRenderSuite_ext"


#ifndef kOfxBitDepthHalf
/** @brief String used to label the OpenGL half float (16 bit floating
point) sample format */
  #define kOfxBitDepthHalf "OfxBitDepthHalf"
#endif

/** @brief Indicates whether a host or plugin can support OpenGL accelerated
rendering

   - Type - C string X 1
   - Property Set - plugin descriptor (read/write), host descriptor (read
only)
   - Default - "false" for a plugin
   - Valid Values - This must be one of
     - "false"  - in which case the host or plugin does not support OpenGL
                  accelerated rendering
     - "true"   - which means a host or plugin can support OpenGL accelerated
                  rendering, in the case of plug-ins this also means that it
                  is capable of CPU based rendering in the absence of a GPU
     - "needed" - only for plug-ins, this means that an effect has to have
                  OpenGL support, without which it cannot work.
*/
#define kOfxImageEffectPropOpenGLRenderSupported "OfxImageEffectPropOpenGLRenderSupported"


/** @brief Indicates the bit depths supported by a plug-in during OpenGL renders.

    This is analogous to ::kOfxImageEffectPropSupportedPixelDepths. When a
    plug-in sets this property, the host will try to provide buffers/textures
    in one of the supported formats. Additionally, the target buffers where
    the plug-in renders to will be set to one of the supported formats.

    Unlike ::kOfxImageEffectPropSupportedPixelDepths, this property is
    optional. Shader-based effects might not really care about any
    format specifics when using OpenGL textures, so they can leave this unset
    and allow the host the decide the format.


   - Type - string X N
   - Property Set - plugin descriptor (read only)
   - Default - none set
   - Valid Values - This must be one of
       - ::kOfxBitDepthNone (implying a clip is unconnected, not valid for an
         image)
       - ::kOfxBitDepthByte
       - ::kOfxBitDepthShort
       - ::kOfxBitDepthHalf
       - ::kOfxBitDepthFloat
*/
#define kOfxOpenGLPropPixelDepth "OfxOpenGLPropPixelDepth"


/** @brief Indicates that an image effect SHOULD use OpenGL acceleration in
the current action

   When a plugin and host have established they can both use OpenGL renders
   then when this property has been set the host expects the plugin to render
   its result into the buffer it has setup before calling the render.  The
   plugin can then also safely use the 'OfxImageEffectOpenGLRenderSuite'

   - Type - int X 1
   - Property Set - inArgs property set of the following actions...
      - ::kOfxImageEffectActionRender
      - ::kOfxImageEffectActionBeginSequenceRender
      - ::kOfxImageEffectActionEndSequenceRender
   - Valid Values
      - 0 indicates that the effect cannot use the OpenGL suite
      - 1 indicates that the effect should render into the texture,
        and may use the OpenGL suite functions.

\note Once this property is set, the host and plug-in have agreed to
use OpenGL, so the effect SHOULD access all its images through the
OpenGL suite.

*/
#define kOfxImageEffectPropOpenGLEnabled "OfxImageEffectPropOpenGLEnabled"


/** @brief Indicates the texture index of an image turned into an OpenGL
texture by the host

   - Type - int X 1
   - Property Set - texture handle returned by
`        OfxImageEffectOpenGLRenderSuiteV1::clipLoadTexture (read only)

	This value should be cast to a GLuint and used as the texture index when
        performing OpenGL texture operations.

   The property set of the following actions should contain this property:
      - ::kOfxImageEffectActionRender
      - ::kOfxImageEffectActionBeginSequenceRender
      - ::kOfxImageEffectActionEndSequenceRender
*/
#define kOfxImageEffectPropOpenGLTextureIndex "OfxImageEffectPropOpenGLTextureIndex"


/** @brief Indicates the texture target enumerator of an image turned into
    an OpenGL texture by the host

   - Type - int X 1
   - Property Set - texture handle returned by
        OfxImageEffectOpenGLRenderSuiteV1::clipLoadTexture (read only)
	This value should be cast to a GLenum and used as the texture target
	when performing OpenGL texture operations.

   The property set of the following actions should contain this property:
      - ::kOfxImageEffectActionRender
      - ::kOfxImageEffectActionBeginSequenceRender
      - ::kOfxImageEffectActionEndSequenceRender
*/
#define kOfxImageEffectPropOpenGLTextureTarget "OfxImageEffectPropOpenGLTextureTarget"


/** @name StatusReturnValues
OfxStatus returns indicating that a OpenGL render error has occurred:

 - If a plug-in returns ::kOfxStatGLRenderFailed, the host should retry the
   render with OpenGL rendering disabled.

 - If a plug-in returns ::kOfxStatGLOutOfMemory, the host may choose to free
   resources on the GPU and retry the OpenGL render, rather than immediately
   falling back to CPU rendering.
 */
/** @{ */
/** @brief render ran out of memory */
#define kOfxStatGLOutOfMemory  ((int) 1001)
/** @brief render failed in a non-memory-related way */
#define kOfxStatGLRenderFailed ((int) 1002)
/** @} */

/** @brief OFX suite that provides image to texture conversion for OpenGL
    processing
 */

typedef struct OfxImageEffectOpenGLRenderSuiteV1
{
  /** @brief loads an image from an OFX clip as a texture into OpenGL

      \arg clip   - the clip to load the image from
      \arg time   - effect time to load the image from
      \arg format - the requested texture format (As in
            none,byte,word,half,float, etc..)
            When set to NULL, the host decides the format based on the
	    plug-in's ::kOfxOpenGLPropPixelDepth setting.
      \arg region - region of the image to load (optional, set to NULL to
            get a 'default' region)
	    this is in the \ref CanonicalCoordinates.
      \arg textureHandle - a property set containing information about the
            texture

  An image is fetched from a clip at the indicated time for the given region
  and loaded into an OpenGL texture. When a specific format is requested, the
  host ensures it gives the requested format.
  When the clip specified is the "Output" clip, the format is ignored and
  the host must bind the resulting texture as the current color buffer
  (render target). This may also be done prior to calling the
  ::kOfxImageEffectActionRender action.
  If the \em region parameter is set to non-NULL, then it will be clipped to
  the clip's Region of Definition for the given time.
  The returned image will be \em at \em least as big as this region.
  If the region parameter is not set or is NULL, then the region fetched will be at
  least the Region of Interest the effect has previously specified, clipped to
  the clip's Region of Definition.
  Information about the texture, including the texture index, is returned in
  the \em textureHandle argument.
  The properties on this handle will be...
    - ::kOfxImageEffectPropOpenGLTextureIndex
    - ::kOfxImageEffectPropOpenGLTextureTarget
    - ::kOfxImageEffectPropPixelDepth
    - ::kOfxImageEffectPropComponents
    - ::kOfxImageEffectPropPreMultiplication
    - ::kOfxImageEffectPropRenderScale
    - ::kOfxImagePropPixelAspectRatio
    - ::kOfxImagePropBounds
    - ::kOfxImagePropRegionOfDefinition
    - ::kOfxImagePropRowBytes
    - ::kOfxImagePropField
    - ::kOfxImagePropUniqueIdentifier

  With the exception of the OpenGL specifics, these properties are the same
  as the properties in an image handle returned by clipGetImage in the image
  effect suite.
\pre
 - clip was returned by clipGetHandle
 - Format property in the texture handle

\post
 - texture handle to be disposed of by clipFreeTexture before the action
returns
 - when the clip specified is the "Output" clip, the format is ignored and
   the host must bind the resulting texture as the current color buffer
   (render target).
   This may also be done prior to calling the render action.

@returns
  - ::kOfxStatOK           - the image was successfully fetched and returned
                             in the handle,
  - ::kOfxStatFailed       - the image could not be fetched because it does
                             not exist in the clip at the indicated
                             time and/or region, the plugin should continue
                             operation, but assume the image was black and
			     transparent.
  - ::kOfxStatErrBadHandle - the clip handle was invalid,
  - ::kOfxStatErrMemory    - not enough OpenGL memory was available for the
                             effect to load the texture.
                             The plugin should abort the GL render and
			     return ::kOfxStatErrMemory, after which the host can
			     decide to retry the operation with CPU based processing.

\note
  - this is the OpenGL equivalent of clipGetImage from OfxImageEffectSuiteV1


*/

  OfxStatus (*clipLoadTexture)(OfxImageClipHandle clip,
                               OfxTime       time,
			       const char   *format,
                               OfxRectD     *region,
                               OfxPropertySetHandle   *textureHandle);

  /** @brief Releases the texture handle previously returned by
clipLoadTexture

  For input clips, this also deletes the texture from OpenGL.
  This should also be called on the output clip; for the Output
  clip, it just releases the handle but does not delete the
  texture (since the host will need to read it).

  \pre
    - textureHandle was returned by clipGetImage

  \post
    - all operations on textureHandle will be invalid, and the OpenGL texture
      it referred to has been deleted (for source clips)

  @returns
    - ::kOfxStatOK - the image was successfully fetched and returned in the
         handle,
    - ::kOfxStatFailed - general failure for some reason,
    - ::kOfxStatErrBadHandle - the image handle was invalid,
*/
  OfxStatus (*clipFreeTexture)(OfxPropertySetHandle   textureHandle);


  /** @brief Request the host to minimize its GPU resource load

  When a plugin fails to allocate GPU resources, it can call this function to
  request the host to flush it's GPU resources if it holds any.
  After the function the plugin can try again to allocate resources which then
  might succeed if the host actually has released anything.

  \pre
  \post
    - No changes to the plugin GL state should have been made.

  @returns
    - ::kOfxStatOK           - the host has actually released some
resources,
    - ::kOfxStatReplyDefault - nothing the host could do..
 */
  OfxStatus (*flushResources)( );

} OfxImageEffectOpenGLRenderSuiteV1;


/** @brief Action called when an effect has just been attached to an OpenGL
context.

The purpose of this action is to allow a plugin to set up any data it may need
to do OpenGL rendering in an instance. For example...
   - allocate a lookup table on a GPU,
   - create an openCL or CUDA context that is bound to the host's OpenGL
     context so it can share buffers.

The plugin will be responsible for deallocating any such shared resource in the
\ref ::kOfxActionOpenGLContextDetached action.

A host cannot call ::kOfxActionOpenGLContextAttached on the same instance
without an intervening ::kOfxActionOpenGLContextDetached. A host can have a
plugin swap OpenGL contexts by issuing a attach/detach for the first context
then another attach for the next context.

The arguments to the action are...
  \arg handle - handle to the plug-in instance, cast to an
  \ref OfxImageEffectHandle
  \arg inArgs - is redundant and set to null
  \arg outArgs - is redundant and set to null

A plugin can return...
  - ::kOfxStatOK, the action was trapped and all was well
  - ::kOfxStatReplyDefault, the action was ignored, but all was well anyway
  - ::kOfxStatErrMemory, in which case this may be called again after a memory
    purge
  - ::kOfxStatFailed, something went wrong, but no error code appropriate,
    the plugin should to post a message if possible and the host should not
    attempt to run the plugin in OpenGL render mode.
*/
#define kOfxActionOpenGLContextAttached "OfxActionOpenGLContextAttached"

/** @brief Action called when an effect is about to be detached from an
OpenGL context

The purpose of this action is to allow a plugin to deallocate any resource
allocated in \ref ::kOfxActionOpenGLContextAttached just before the host
decouples a plugin from an OpenGL context.
The host must call this with the same OpenGL context active as it
called with the corresponding ::kOfxActionOpenGLContextAttached.

The arguments to the action are...
  \arg handle - handle to the plug-in instance, cast to an
  \ref OfxImageEffectHandle
  \arg inArgs - is redundant and set to null
  \arg outArgs - is redundant and set to null

A plugin can return...
  - ::kOfxStatOK, the action was trapped and all was well
  - ::kOfxStatReplyDefault, the action was ignored, but all was well anyway
  - ::kOfxStatErrMemory, in which case this may be called again after a memory
    purge
  - ::kOfxStatFailed, something went wrong, but no error code appropriate,
    the plugin should to post a message if possible and the host should not
    attempt to run the plugin in OpenGL render mode.
*/
#define kOfxActionOpenGLContextDetached "kOfxActionOpenGLContextDetached"


/** @page ofxOpenGLRender OpenGL Acceleration of Rendering

@section ofxOpenGLRenderIntro Introduction

The OfxOpenGLRenderSuite allows image effects to use OpenGL commands
(hopefully backed by a GPU) to accelerate rendering
of their outputs. The basic scheme is simple....
  - An effect indicates it wants to use OpenGL acceleration by setting the
    ::kOfxImageEffectOpenGLRenderSupported flag on it's descriptor
  - A host indicates it supports OpenGL acceleration by setting
    ::kOfxImageEffectOpenGLRenderSupported on it's descriptor
  - In an effect's ::kOfxImageEffectActionGetClipPreferences action, an
    effect indicates what clips it will be loading images from onto the GPU's
    memory during an effect's ::kOfxImageEffectActionRender action.

@section ofxOpenGLRenderHouseKeeping OpenGL House Keeping

If a host supports OpenGL rendering then it flags this with the string
property ::kOfxImageEffectOpenGLRenderSupported on its descriptor property
set. Effects that cannot run without OpenGL support should examine this in
::kOfxActionDescribe action and return a ::kOfxStatErrMissingHostFeature
status flag if it is not set to "true".

Effects flag to a host that they support OpenGL rendering by setting the
string property ::kOfxImageEffectOpenGLRenderSupported on their effect
descriptor during the ::kOfxActionDescribe action. Effects can work in three
ways....
  - purely on CPUs without any OpenGL support at all, in which case they
    should set ::kOfxImageEffectOpenGLRenderSupported to be "false" (the
    default),
  - on CPUs but with optional OpenGL support, in which case they should set
    ::kOfxImageEffectOpenGLRenderSupported to be "true",
  - only with OpenGL support, in which case they should set
    ::kOfxImageEffectOpenGLRenderSupported to be "needed".

Hosts can examine this flag and respond to it appropriately.

Effects can use OpenGL accelerated rendering during the following
action...
  - ::kOfxImageEffectActionRender

If an effect has indicated that it optionally supports OpenGL acceleration,
it should check the property ::kOfxImageEffectPropOpenGLEnabled
passed as an in argument to the following actions,
  - ::kOfxImageEffectActionRender
  - ::kOfxImageEffectActionBeginSequenceRender
  - ::kOfxImageEffectActionEndSequenceRender

If this property is set to 0, then it should not attempt to use any calls to
the OpenGL suite or OpenGL calls whilst rendering.


@section ofxOpenGLRenderGettingTextures Getting Images as Textures

An effect could fetch an image into memory from a host via the standard
Image Effect suite "clipGetImage" call, then create an OpenGL
texture from that. However as several buffer copies and various other bits
of house keeping may need to happen to do this, it is more
efficient for a host to create the texture directly.

The OfxOpenGLRenderSuiteV1::clipLoadTexture function does this. The
arguments and semantics are similar to the
OfxImageEffectSuiteV2::clipGetImage function, with a few minor changes.

The effect is passed back a property handle describing the texture. Once the
texture is finished with, this should be disposed
of via the OfxOpenGLRenderSuiteV1::clipFreeTexture function, which will also
delete the associated OpenGL texture (for source clips).

The returned handle has a set of properties on it, analogous to the
properties returned on the image handle by
OfxImageEffectSuiteV2::clipGetImage. These are:
    - ::kOfxImageEffectPropOpenGLTextureIndex
    - ::kOfxImageEffectPropOpenGLTextureTarget
    - ::kOfxImageEffectPropPixelDepth
    - ::kOfxImageEffectPropComponents
    - ::kOfxImageEffectPropPreMultiplication
    - ::kOfxImageEffectPropRenderScale
    - ::kOfxImagePropPixelAspectRatio
    - ::kOfxImagePropBounds
    - ::kOfxImagePropRegionOfDefinition
    - ::kOfxImagePropRowBytes
    - ::kOfxImagePropField
    - ::kOfxImagePropUniqueIdentifier

The main difference between this and an image handle is that the
::kOfxImagePropData property is replaced by the
kOfxImageEffectPropOpenGLTextureIndex property.
This integer property should be cast to a GLuint and is the index to use for
the OpenGL texture.
Next to texture handle the texture target enumerator is given in
kOfxImageEffectPropOpenGLTextureTarget

Note, because the image is being directly loaded into a texture by the host
it need not obey the Clip Preferences action to remap the image to the pixel
depth the effect requested.

@section ofxOpenGLRenderOutput Render Output Directly with OpenGL

Effects can use the graphics context as they see fit. They may be doing
several render passes with fetch back from the card to main memory
via 'render to texture' mechanisms interleaved with passes performed on the
CPU. The effect must leave output on the graphics card in the provided output
image texture buffer.

The host will create a default OpenGL viewport that is the size of the
render window passed to the render action. The following
code snippet shows how the viewport should be rooted at the bottom left of
the output texture.

\verbatim
  // set up the OpenGL context for the render to texture
  ...

  // figure the size of the render window
  int dx = renderWindow.x2 - renderWindow.x1;
  int dy = renderWindow.y2 - renderWindow.y2;

  // setup the output viewport
  glViewport(0, 0, dx, dy);

\endverbatim

Prior to calling the render action the host may also choose to
bind the output texture as the current color buffer (render target), or they
may defer doing this until clipLoadTexture is called for the output clip.

After this, it is completely up to the effect to choose what OpenGL
operations to render with, including projections and so on.

@section ofxOpenGLRenderContext OpenGL Current Context

The host is only required to make the OpenGL context current (e.g.,
using wglMakeCurrent, for Windows) during the following actions:

   - ::kOfxImageEffectActionRender
   - ::kOfxImageEffectActionBeginSequenceRender
   - ::kOfxImageEffectActionEndSequenceRender
   - ::kOfxActionOpenGLContextAttached
   - ::kOfxActionOpenGLContextDetached

For the first 3 actions, Render through EndSequenceRender, the host is only
required to set the OpenGL context if ::kOfxImageEffectPropOpenGLEnabled is
set.  In other words, a plug-in should not expect the OpenGL context to be
current for other OFX calls, such as ::kOfxImageEffectActionDescribeInContext.

*/

#ifdef __cplusplus
}
#endif


#endif
