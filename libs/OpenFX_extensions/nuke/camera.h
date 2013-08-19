#ifndef nukeOfxCamera_h
#define nukeOfxCamera_h

#include "ofxImageEffect.h"

/*
Software License :

Copyright (c) 2010, The Foundry Visionmongers Ltd. All rights reserved.

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

/** @file nukeOfxCamera.h

This file defines a custom OFX extension to NUKE that allows cameras to
be defined as nodal inputs to an image processing effect. It uses a 
custom suite that allows you to define camera inputs to an effect in a similar
manner as image clips are defined in the standard image processing API.

It uses the standard descriptor/instance split that is common to other OFX
objects. So during effect description you define a camera input by name
and set some properties on it.

You can define more than one camera input to the effect, they will need
different names however.

After instance creation you can fetch data from a 'live' camera. This is
done by via functions in the suite. As this is Nuke, you need to specify 
a frame and a view to fetch the value at. The view is available as a custom
nuke property passed into all actions from nuke.
*/

/** @brief string to identify the Camera suite, pass to OfxHost::fetchSuite */
#define kNukeOfxCameraSuite "NukeOfxCameraSuite"

typedef struct NukeOfxCameraStruct *NukeOfxCameraHandle;

#define kNukeOfxCameraProjectionModePerspective 0.0
#define kNukeOfxCameraProjectionModeOrthographic 1.0
#define kNukeOfxCameraProjectionModeUV 2.0
#define kNukeOfxCameraProjectionModeSpherical 3.0

/** @brief Name of the projection mode nuke camera parameter.

This has only one dimension and can be one of four values...
  - \ref kNukeOfxCameraProjectionPerspective
  - \ref kNukeOfxCameraProjectionOrthographic
  - \ref kNukeOfxCameraProjectionUV
  - \ref kNukeOfxCameraProjectionSpherical
*/
#define kNukeOfxCameraParamProjectionMode "projection_mode"

/** @brief Name of the focal length nuke camera parameter.

This has only one dimension.
*/
#define kNukeOfxCameraParamFocalLength "focal"

/** @brief Name of the horizontal aperture nuke camera parameter.

This has only one dimension.
*/
#define kNukeOfxCameraParamHorizontalAperture "haperture"

/** @brief Name of the vertical aperture nuke camera parameter.

This has only one dimension.
*/
#define kNukeOfxCameraParamVerticalAperture "vaperture"

/** @brief Name of the near clipping plane nuke camera parameter 

This has only one dimension.
*/
#define kNukeOfxCameraParamNear "near"

/** @brief Name of the far clipping plane nuke camera parameter 

This has only one dimension.
*/
#define kNukeOfxCameraParamFar "far"

/** @brief Name of the window translate nuke camera parameter 

This has two dimensions.
*/
#define kNukeOfxCameraParamWindowTranslate "win_translate"

/** @brief Name of the window scale nuke camera parameter 

This has two dimensions.
*/
#define kNukeOfxCameraParamWindowScale "win_scale"

/** @brief Name of the window roll scale nuke camera parameter 

This has one dimension.
*/
#define kNukeOfxCameraParamWindowRoll "winroll"

/** @brief Name of the focal point nuke camera parameter 

This has only one dimension.
*/
#define kNukeOfxCameraParamFocalPoint "focal_point"

/** @brief Name of the camera position parameter 

This represents a homogenous 4x4 transform matrix
that places the camera in 3D space. As such it has 16 values.
*/
#define kNukeOfxCameraParamPositionMatrix "position_matrix"

/** @brief A suite to use Nuke cameras.

This suite provides the functions needed by a plugin to define and use camera inputs for a Nuke image effect plugin.

Cameras are named and more than one camera can be attached as an input to an effect.

Camera descriptor property sets are defined to have the following properties
        - \ref kOfxPropType - (read only) set to "NukeCamera"
        - \ref kOfxPropName - (read only) the name the camera was created with
        - \ref kOfxPropLabel - (read/write) the user visible label for the camera
        - \ref kOfxPropShortLabel - (read/write)
        - \ref kOfxPropLongLabel - (read/write)
        - \ref kOfxImageClipPropOptional - (read/write)

Instance property sets have all the descriptor properties (as read only) and the following properties...
        - \ref kOfxImageClipPropConnected - (read only)
*/
typedef struct NukeOfxCameraSuiteV1 {
  /** @brief Define a camera input to an effect.

   \arg pluginHandle - the decriptor handle passed into the 'describeInContext' action
   \arg name - unique name of the camera to define
   \arg propertySet - a property handle for the camera descriptor will be returned here

   Called by the plugin when the 'describeInContext' action has been called.  Sets up a camera
   input with the given name.  If not NULL, then a pointer to a PropertySetHandle for the Camera
   may be put in *propertySet.

   More than one camera can be defined, they will need different names.
  */
  OfxStatus (*cameraDefine)        (OfxImageEffectHandle pluginHandle, const char *name, OfxPropertySetHandle *propertySet);

  /** @brief Get the handle of the named camera input to an effect.

  \arg pluginHandle - an instance handle to the plugin
  \arg name         - name of camera, previously used in a camera define call
  \arg camera       - where to return the camera handle
  \arg propertySet  - if not null, the descriptor handle for the camera's property set will be placed here.
  */
  OfxStatus (*cameraGetHandle)     (OfxImageEffectHandle pluginHandle, const char *name, NukeOfxCameraHandle *camera, OfxPropertySetHandle *propertySet);

  /** @brief Get the property set for the given camera
  
  \arg camera       - the handle of the camera, as obtained from cameraGetHandle
  \arg propertySet  - will be set with a pointer to the property set
  */
  OfxStatus (*cameraGetPropertySet)(NukeOfxCameraHandle camera, OfxPropertySetHandle *propHandle);

  /** @brief Get an arbitrary camera parameter for a given time and view
  
  \arg camera       - the handle of the camera, as obtained from cameraGetHandle
  \arg time         - the time to evaluate the parameter for
  \arg view         - the view to evaluate the parameter for
  \arg paramName    - parameter name to look up (matches name of knob in Nuke, see defines at top of \ref ofxCamera.h)
  \arg baseReturnAddress - base address to store the evaluated result
  \arg returnSize   - the number of doubles at the baseReturnAddress

  @returns
  - ::kOfxStatOK         - if the parameter was fetched succesfully
  - ::kOfxStatFailed     - if the camera was not connected
  - ::kOfxStatErrUnknown - if the named parameter could not be found
  */
  OfxStatus (*cameraGetParameter)  (NukeOfxCameraHandle camera, const char* paramName, double time, int view, double* baseReturnAddress, int returnSize);
} NukeOfxCameraSuiteV1;

#endif
