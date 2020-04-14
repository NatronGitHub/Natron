#ifndef _ofxParametricParam_h_
#define _ofxParametricParam_h_

/*
Software License :

Copyright (c) 2009-15, The Open Effects Association Ltd. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.
    * Neither the name The Open Effects Association Ltd, nor the names of its 
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

/** @file ofxParametricParam.h
 
This header file defines the optional OFX extension to define and manipulate parametric
parameters.

*/

#include "ofxParam.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief string value to the ::kOfxPropType property for all parameters */
#define kOfxParametricParameterSuite "OfxParametricParameterSuite"


/**
   \defgroup ParamTypeDefines Parameter Type definitions 

These strings are used to identify the type of the parameter when it is defined, they are also on the ::kOfxParamPropType in any parameter instance.
*/
/*@{*/

/** @brief String to identify a param as a single valued integer */
#define kOfxParamTypeParametric "OfxParamTypeParametric"

/*@}*/

/**
   \addtogroup PropertiesAll
*/
/*@{*/
/**
   \defgroup ParamPropDefines Parameter Property Definitions 

These are the list of properties used by the parameters suite.
*/
/*@{*/

/** @brief The dimension of a parametric param

    - Type - int X 1
    - Property Set - parametric param descriptor (read/write) and instance (read only)
    - default - 1
    - Value Values - greater than 0

This indicates the dimension of the parametric param.
*/
#define kOfxParamPropParametricDimension "OfxParamPropParametricDimension"

/** @brief The colour of parametric param curve interface in any UI.

    - Type - double X N
    - Property Set - parametric param descriptor (read/write) and instance (read only)
    - default - unset, 
    - Value Values - three values for each dimension (see ::kOfxParamPropParametricDimension)
      being interpretted as R, G and B of the colour for each curve drawn in the UI.

This sets the colour of a parametric param curve drawn a host user interface. A colour triple
is needed for each dimension of the oparametric param. 

If not set, the host should generally draw these in white.
*/
#define kOfxParamPropParametricUIColour "OfxParamPropParametricUIColour"

/** @brief Interact entry point to draw the background of a parametric parameter.
 
    - Type - pointer X 1
    - Property Set - plug-in parametric parameter descriptor (read/write) and instance (read only),
    - Default - NULL, which implies the host should draw its default background.
 
Defines a pointer to an interact which will be used to draw the background of a parametric 
parameter's user interface.  None of the pen or keyboard actions can ever be called on the interact.
 
The openGL transform will be set so that it is an orthographic transform that maps directly to the
'parametric' space, so that 'x' represents the parametric position and 'y' represents the evaluated
value.
*/
#define kOfxParamPropParametricInteractBackground "OfxParamPropParametricInteractBackground"

/** @brief Property on the host to indicate support for parametric parameter animation.
 
    - Type - int X 1
    - Property Set - host descriptor (read only)
    - Valid Values 
      - 0 indicating the host does not support animation of parmetric params,
      - 1 indicating the host does support animation of parmetric params,
*/
#define kOfxParamHostPropSupportsParametricAnimation "OfxParamHostPropSupportsParametricAnimation"

/** @brief Property to indicate the min and max range of the parametric input value.
 
    - Type - double X 2
    - Property Set - parameter descriptor (read/write only), and instance (read only)
    - Default Value - (0, 1)
    - Valid Values - any pair of numbers so that  the first is less than the second.

This controls the min and max values that the parameter will be evaluated at.
*/
#define kOfxParamPropParametricRange "OfxParamPropParametricRange"

/*@}*/
/*@}*/


/** @brief The OFX suite used to define and manipulate 'parametric' parameters.

This is an optional suite.

Parametric parameters are in effect 'functions' a plug-in can ask a host to arbitrarily 
evaluate for some value 'x'. A classic use case would be for constructing look-up tables, 
a plug-in would ask the host to evaluate one at multiple values from 0 to 1 and use that 
to fill an array.

A host would probably represent this to a user as a cubic curve in a standard curve editor 
interface, or possibly through scripting. The user would then use this to define the 'shape'
of the parameter.

The evaluation of such params is not the same as animation, they are returning values based 
on some arbitrary argument orthogonal to time, so to evaluate such a param, you need to pass
a parametric position and time.

Often, you would want such a parametric parameter to be multi-dimensional, for example, a
colour look-up table might want three values, one for red, green and blue. Rather than 
declare three separate parametric parameters, it would be better to have one such parameter 
with multiple values in it.

The major complication with these parameters is how to allow a plug-in to set values, and 
defaults. The default default value of a parametric curve is to be an identity lookup. If
a plugin wishes to set a different default value for a curve, it can use the suite to set
key/value pairs on the \em descriptor of the param. When a new instance is made, it will
have these curve values as a default.
*/
typedef struct OfxParametricParameterSuiteV1 {
  
  /** @brief Evaluates a parametric parameter
 
      \arg param                 handle to the parametric parameter
      \arg curveIndex            which dimension to evaluate
      \arg time                  the time to evaluate to the parametric param at
      \arg parametricPosition    the position to evaluate the parametric param at
      \arg returnValue           pointer to a double where a value is returned
 
      @returns
        - ::kOfxStatOK            - all was fine 
        - ::kOfxStatErrBadHandle  - if the paramter handle was invalid
        - ::kOfxStatErrBadIndex   - the curve index was invalid
  */
  OfxStatus (*parametricParamGetValue)(OfxParamHandle param,
                                       int   curveIndex,
                                       OfxTime time,
                                       double parametricPosition,
                                       double *returnValue);


  /** @brief Returns the number of control points in the parametric param.

      \arg param                 handle to the parametric parameter
      \arg curveIndex            which dimension to check
      \arg time                  the time to check
      \arg returnValue           pointer to an integer where the value is returned.
      
      @returns
        - ::kOfxStatOK            - all was fine 
        - ::kOfxStatErrBadHandle  - if the paramter handle was invalid
        - ::kOfxStatErrBadIndex   - the curve index was invalid
   */
  OfxStatus (*parametricParamGetNControlPoints)(OfxParamHandle param,
                                                int   curveIndex,
                                                double time,
                                                int *returnValue);
 
  /** @brief Returns the key/value pair of the nth control point.

      \arg param                 handle to the parametric parameter
      \arg curveIndex            which dimension to check
      \arg time                  the time to check
      \arg nthCtl                the nth control point to get the value of
      \arg key                   pointer to a double where the key will be returned
      \arg value                 pointer to a double where the value will be returned 
            
      @returns
        - ::kOfxStatOK            - all was fine 
        - ::kOfxStatErrBadHandle  - if the paramter handle was invalid
        - ::kOfxStatErrUnknown    - if the type is unknown
   */
  OfxStatus (*parametricParamGetNthControlPoint)(OfxParamHandle param,
                                                 int    curveIndex,
                                                 double time,
                                                 int    nthCtl,
                                                 double *key,
                                                 double *value);
 
  /** @brief Modifies an existing control point on a curve

      \arg param                 handle to the parametric parameter
      \arg curveIndex            which dimension to set
      \arg time                  the time to set the value at 
      \arg nthCtl                the control point to modify
      \arg key                   key of the control point
      \arg value                 value of the control point
      \arg addAnimationKey       if the param is an animatable, setting this to true will 
                                 force an animation keyframe to be set as well as a curve key,
                                 otherwise if false, a key will only be added if the curve is already
                                 animating.

      @returns
        - ::kOfxStatOK            - all was fine 
        - ::kOfxStatErrBadHandle  - if the paramter handle was invalid
        - ::kOfxStatErrUnknown    - if the type is unknown

      This modifies an existing control point. Note that by changing key, the order of the
      control point may be modified (as you may move it before or after anther point). So be
      careful when iterating over a curves control points and you change a key.
   */
  OfxStatus (*parametricParamSetNthControlPoint)(OfxParamHandle param,
                                                 int   curveIndex,
                                                 double time,
                                                 int   nthCtl,
                                                 double key,
                                                 double value,
                                                 bool addAnimationKey);

  /** @brief Adds a control point to the curve.

      \arg param                 handle to the parametric parameter
      \arg curveIndex            which dimension to set
      \arg time                  the time to set the value at
      \arg key                   key of the control point
      \arg value                 value of the control point
      \arg addAnimationKey       if the param is an animatable, setting this to true will 
                                 force an animation keyframe to be set as well as a curve key,
                                 otherwise if false, a key will only be added if the curve is already
                                 animating.

      @returns
        - ::kOfxStatOK            - all was fine 
        - ::kOfxStatErrBadHandle  - if the paramter handle was invalid
        - ::kOfxStatErrUnknown    - if the type is unknown

      This will add a new control point to the given dimension of a parametric parameter. If a key exists
      sufficiently close to 'key', then it will be set to the indicated control point.
   */
  OfxStatus (*parametricParamAddControlPoint)(OfxParamHandle param,
                                              int   curveIndex,
                                              double time,
                                              double key,
                                              double value,
                                              bool addAnimationKey);
  
  /** @brief Deletes the nth control point from a parametric param.

      \arg param                 handle to the parametric parameter
      \arg curveIndex            which dimension to delete
      \arg nthCtl                the control point to delete
   */
  OfxStatus (*parametricParamDeleteControlPoint)(OfxParamHandle param,
                                                 int   curveIndex,
                                                 int   nthCtl);

  /** @brief Delete all curve control points on the given param.

      \arg param                 handle to the parametric parameter
      \arg curveIndex            which dimension to clear
   */
  OfxStatus (*parametricParamDeleteAllControlPoints)(OfxParamHandle param,
                                                     int   curveIndex);
 } OfxParametricParameterSuiteV1;

#ifdef OFX_SUPPORTS_PARAMETRIC_V2
/** @brief The version 2 of the suite adds support for derivatives on control points. Each control point
 has a left and right derivative which are computed by the host given an interpolation mode set on the control point.
 The interpolation mode is valid for a control point up to another. The first control point left derivative
 and the last control point right derivative cannot have a specific interpolation mode.
 The interpolation mode can be controled by the function parametricParamSetNthControlPoint and parametricParamAddControlPoint.
 In return the host will set the derivatives in the function parametricParamGetNControlPoints.
 A host can advertise the types of interpolation it supports among default interpolation modes.
 A plug-in is then free to give the host any interpolation that it recognizes. Failure to provide a known interpolation
 mode to a host should result in an error.
 **/

/** @brief Property on a host descriptor to indicate which interpolation modes are supported by the host.
   - Type - string X N
   - Property Set - host descriptor (read only)
   - Default Value - This property should at least contain the value of kOfxHostParametricInterpolationDefault
   - Valid Values - Any default value specified below

   This controls the min and max values that the parameter will be evaluated at.
  */
#define kOfxHostPropSupportedParametricInterpolations "OfxHostPropSupportedParametricInterpolations"

/** @brief Interpolation mode that can be passed to the kOfxHostPropSupportedParametricInterpolations property.
   The curve should use the default interpolation mode provided by the host
*/
#define kOfxHostParametricInterpolationDefault "OfxHostParametricInterpolationDefault"

/** @brief Interpolation mode that can be passed to the kOfxHostPropSupportedParametricInterpolations property. 
 The value of the parametric curve should remain the same up until the next control point.
 */
#define kOfxHostParametricInterpolationConstant "OfxHostParametricInterpolationConstant"

/** @brief Interpolation mode that can be passed to the kOfxHostPropSupportedParametricInterpolations property.
   The value of the parametric curve should evolve linearly up until the next control point.
  */
#define kOfxHostParametricInterpolationLinear "OfxHostParametricInterpolationLinear"

/** @brief Interpolation mode that can be passed to the kOfxHostPropSupportedParametricInterpolations property.
   The tangents of the control point should be horizontal.
  */
#define kOfxHostParametricInterpolationHorizontal "OfxHostParametricInterpolationHorizontal"

/** @brief Interpolation mode that can be passed to the kOfxHostPropSupportedParametricInterpolations property.
   The interpolation should follow the CatmullRom one if the value of the curve at the given parametric t is in the range
 [vprev,vnext] or horizontal if outside the range. vprev is the value of the previous keyframe relative to point t and 
 vnext the value of the next keyframe after t.
*/
#define kOfxHostParametricInterpolationSmooth "OfxHostParametricInterpolationSmooth"

/** @brief Interpolation mode that can be passed to the kOfxHostPropSupportedParametricInterpolations property.
  The curve should be cubic, i.e:  the 2nd derivative of the curve at the point t are equal.
*/
#define kOfxHostParametricInterpolationCubic "OfxHostParametricInterpolationCubic"

/** @brief Interpolation mode that can be passed to the kOfxHostPropSupportedParametricInterpolations property.
   The curve should be a catmull-rom spline, see https://en.wikipedia.org/wiki/Cubic_Hermite_spline#Catmull.E2.80.93Rom_spline
*/
#define kOfxHostParametricInterpolationCatmullRom "OfxHostParametricInterpolationCatmullRom"

/** @brief Interpolation mode that can be passed to the kOfxHostPropSupportedParametricInterpolations property.
  The curve should be interpolated by a custom interpolator passed by the plug-in. In this mode all control points must 
  use the custom interpolator.
*/
#define kOfxHostParametricInterpolationCustom "OfxHostParametricInterpolationCustom"


/** @brief Function prototype for parametric parameter interpolation callback functions

   \arg instance   the plugin instance that this parameter occurs in
   \arg inArgs     handle holding the following properties...
   - kOfxPropName - the name of the parametric parameter to interpolate
   - kOfxPropTime - absolute time the interpolation is ocurring at
   - kOfxParametricParamCustomInterpInterpolationTime - 2D double property that gives the time of the two control points we are interpolating
   - kOfxParametricParamCustomInterpInterpolationValue - 2D double property that gives the value of the two control points we are interpolating
   - kOfxParametricParamCustomInterpControlPointIndex - 1D int property that gives the index of the previous control point relative to kOfxPropTime
   - kOfxParametricParamCustomInterpCurveIndex - 1D int property that gives the index of the curve we are interpolating

   \arg outArgs handle holding the following properties to be set
   - kOfxParamPropCustomValue - the value of the interpolated custom parameter, in this case 1D

   This function allows custom parameters to animate by performing interpolation between keys.

   The plugin needs to parse the two strings encoding keyframes on either side of the time
   we need a value for. It should then interpolate a new value for it, encode it into a string and set
   the ::kOfxParamPropCustomValue property with this on the outArgs handle.

   The interp value is a linear interpolation amount, however his may be derived from a cubic (or other) curve.
   */
typedef OfxStatus (OfxParametricParamCustomInterpFuncV1)(OfxParamSetHandle instance,
                                                         OfxPropertySetHandle inArgs,
                                                         OfxPropertySetHandle outArgs);

/** @brief A double 2D indicating the time of the previous and next control points surrounding the parametric time at which the interpolation is occuring.
  - Type - Double X 2
  - Property Set - inArgs of the OfxParametricParamCustomInterpFuncV1 function
 */
#define kOfxParametricParamCustomInterpInterpolationTime "OfxParametricParamCustomInterpInterpolationTime"

/** @brief A double 2D indicating the value of the previous and next control points surrounding the parametric time at which the interpolation is occuring.
   - Type - Double X 2
   - Property Set - inArgs of the OfxParametricParamCustomInterpFuncV1 function
*/
#define kOfxParametricParamCustomInterpInterpolationValue "OfxParametricParamCustomInterpInterpolationValue"

/** @brief A int 1D indicating the index of the previous control point relative to the parametric time at which the interpolation is occuring.
  - Type - int X 1
  - Property Set - inArgs of the OfxParametricParamCustomInterpFuncV1 function
*/
#define kOfxParametricParamCustomInterpControlPointIndex "OfxParametricParamCustomInterpControlPointIndex"

/** @brief A int 1D indicating the curve index
  - Type - int X 1
  - Property Set - inArgs of the OfxParametricParamCustomInterpFuncV1 function
*/
#define kOfxParametricParamCustomInterpCurveIndex "OfxParametricParamCustomInterpCurveIndex"

  /** @brief A pointer to an interpolation function that will be used by a parametric parameter.

   - Type - pointer X 1
   - Property Set - parametric parameter descriptor (read/write) and instance (read only),
   - Default - NULL
   - Valid Values - must point to a ::OfxParametricParamCustomInterpFuncV1

   */
#define kOfxParamParametricInterpolationCustomInterpCallbackV1 "OfxParamParametricInterpolationCustomInterpCallbackV1"

typedef struct OfxParametricParameterSuiteV2 {
  /** @brief Evaluates a parametric parameter
   This function does the same as V1, except if the parametric param was passed a kOfxParamParametricInterpolationCustomInterpCallbackV1

   \arg param                 handle to the parametric parameter
   \arg curveIndex            which dimension to evaluate
   \arg time                  the time to evaluate to the parametric param at
   \arg parametricPosition    the position to evaluate the parametric param at
   \arg returnValue           pointer to a double where a value is returned

   @returns
   - ::kOfxStatOK            - all was fine
   - ::kOfxStatErrBadHandle  - if the paramter handle was invalid
   - ::kOfxStatErrBadIndex   - the curve index was invalid
   */
  OfxStatus (*parametricParamGetValue)(OfxParamHandle param,
                                       int   curveIndex,
                                       OfxTime time,
                                       double parametricPosition,
                                       double *returnValue);


  /** @brief Returns the number of control points in the parametric param.
   This function does the same as V1
   \arg param                 handle to the parametric parameter
   \arg curveIndex            which dimension to check
   \arg time                  the time to check
   \arg returnValue           pointer to an integer where the value is returned.

   @returns
   - ::kOfxStatOK            - all was fine
   - ::kOfxStatErrBadHandle  - if the paramter handle was invalid
   - ::kOfxStatErrBadIndex   - the curve index was invalid
   */
  OfxStatus (*parametricParamGetNControlPoints)(OfxParamHandle param,
                                                int   curveIndex,
                                                double time,
                                                int *returnValue);

  /** @brief Returns the key/value pair of the nth control point.
   This function does the same as V1
   \arg param                 handle to the parametric parameter
   \arg curveIndex            which dimension to check
   \arg time                  the time to check
   \arg nthCtl                the nth control point to get the value of
   \arg key                   pointer to a double where the key will be returned
   \arg value                 pointer to a double where the value will be returned

   @returns
   - ::kOfxStatOK            - all was fine
   - ::kOfxStatErrBadHandle  - if the paramter handle was invalid
   - ::kOfxStatErrUnknown    - if the type is unknown
   */
  OfxStatus (*parametricParamGetNthControlPoint)(OfxParamHandle param,
                                                 int    curveIndex,
                                                 double time,
                                                 int    nthCtl,
                                                 double *key,
                                                 double *value);

  /** @brief Returns the left and right derivatives pair of the nth control point.
   This is an ERROR to call this function if a custom interpolator has been set on the parametric param with the
   kOfxParamParametricInterpolationCustomInterpCallbackV1 property.
   
   NEW in V2

   \arg param                 handle to the parametric parameter
   \arg curveIndex            which dimension to check
   \arg nthCtl                the nth control point to get the value of
   \arg leftDerivative        pointer to a double where the value of the left derivative of the nthCtl control point will be returned
   \arg rightDerivative       pointer to a double where the value of the right derivative of the nthCtl control point will be returned

   @returns
   - ::kOfxStatOK            - all was fine
   - ::kOfxStatErrBadHandle  - if the paramter handle was invalid
   - ::kOfxStatErrUnknown    - if the type is unknown
   */
  OfxStatus (*parametricParamGetNthControlPointDerivatives)(OfxParamHandle param,
                                                 int    curveIndex,
                                                 double time,
                                                 int    nthCtl,
                                                 double *leftDerivative,
                                                 double *rightDerivative);


  /** @brief Modifies an existing control point on a curve
    This function does the same as V1
   \arg param                 handle to the parametric parameter
   \arg curveIndex            which dimension to set
   \arg time                  the time to set the value at
   \arg nthCtl                the control point to modify
   \arg key                   key of the control point
   \arg value                 value of the control point
   \arg addAnimationKey       if the param is an animatable, setting this to true will
   force an animation keyframe to be set as well as a curve key,
   otherwise if false, a key will only be added if the curve is already
   animating.

   @returns
   - ::kOfxStatOK            - all was fine
   - ::kOfxStatErrBadHandle  - if the paramter handle was invalid
   - ::kOfxStatErrUnknown    - if the type is unknown

   This modifies an existing control point. Note that by changing key, the order of the
   control point may be modified (as you may move it before or after anther point). So be
   careful when iterating over a curves control points and you change a key.
   */
  OfxStatus (*parametricParamSetNthControlPoint)(OfxParamHandle param,
                                                 int   curveIndex,
                                                 double time,
                                                 int   nthCtl,
                                                 double key,
                                                 double value,
                                                 bool addAnimationKey);

  /** @brief Modifies an existing control point on a curve

   NEW in V2

   \arg param                 handle to the parametric parameter
   \arg curveIndex            which dimension to set
   \arg time                  the time to set the value at
   \arg nthCtl                the control point to modify
   \arg interpolation         the interpolation of the control point

   @returns
   - ::kOfxStatOK            - all was fine
   - ::kOfxStatErrValue      - the host does not know this interpolation type. This must be one of the interpolation modes advertised by the host with the kOfxHostPropSupportedParametricInterpolations property
   - ::kOfxStatErrBadHandle  - if the paramter handle was invalid
   - ::kOfxStatErrUnknown    - if the type is unknown
   */
  OfxStatus (*parametricParamSetNthControlPointInterpolation)(OfxParamHandle param,
                                                              int   curveIndex,
                                                              double time,
                                                              int   nthCtl,
                                                              const char* interpolationMode);

  /** @brief Adds a control point to the curve.
   
   This was modified in V2 with the introduction of the interpolationMode parameter

   \arg param                 handle to the parametric parameter
   \arg curveIndex            which dimension to set
   \arg time                  the time to set the value at
   \arg key                   key of the control point
   \arg value                 value of the control point
   \arg interpolationMode     the interpolation of the control point, By default the value should be kOfxHostParametricInterpolationDefault
   \arg addAnimationKey       if the param is an animatable, setting this to true will
   force an animation keyframe to be set as well as a curve key,
   otherwise if false, a key will only be added if the curve is already
   animating.

   @returns
   - ::kOfxStatOK            - all was fine
   - ::kOfxStatErrValue      - the host does not know this interpolation type. This must be one of the interpolation modes advertised by the host with the kOfxHostPropSupportedParametricInterpolations property
   - ::kOfxStatErrBadHandle  - if the paramter handle was invalid
   - ::kOfxStatErrUnknown    - if the type is unknown

   This will add a new control point to the given dimension of a parametric parameter. If a key exists
   sufficiently close to 'key', then it will be set to the indicated control point.
   */
  OfxStatus (*parametricParamAddControlPoint)(OfxParamHandle param,
                                              int   curveIndex,
                                              double time,
                                              double key,
                                              double value,
                                              const char* interpolationMode,
                                              bool addAnimationKey);

  /** @brief Deletes the nth control point from a parametric param.

   \arg param                 handle to the parametric parameter
   \arg curveIndex            which dimension to delete
   \arg nthCtl                the control point to delete
   */
  OfxStatus (*parametricParamDeleteControlPoint)(OfxParamHandle param,
                                                 int   curveIndex,
                                                 int   nthCtl);

  /** @brief Delete all curve control points on the given param.

   \arg param                 handle to the parametric parameter
   \arg curveIndex            which dimension to clear
   */
  OfxStatus (*parametricParamDeleteAllControlPoints)(OfxParamHandle param,
                                                     int   curveIndex);
} OfxParametricParameterSuiteV2;
#endif // OFX_SUPPORTS_PARAMETRIC_V2

#ifdef __cplusplus
}
#endif




#endif
