#ifndef _ofxParametricParam_h_
#define _ofxParametricParam_h_

/*
Software License :

Copyright (c) 2009, The Open Effects Association Ltd. All rights reserved.

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

#ifdef __cplusplus
}
#endif




#endif
