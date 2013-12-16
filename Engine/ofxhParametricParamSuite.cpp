//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */


#include "ofxhParametricParamSuite.h"

// ofx
#include "ofxCore.h"
#include "ofxParam.h"

#include <ofxhParam.h>
#include <ofxhPropertySuite.h>

//ofx h
#include "ofxhPropertySuite.h"

// parametric params
#include "ofxParametricParam.h"

namespace OFX {

namespace Host {

namespace ParametricParam{

    
   
    ParametricDescriptor::ParametricDescriptor(const std::string &type, const std::string &name)
    : Param::Descriptor(type,name)
    {
        Property::PropSpec parametricProps[] = {
            /* name                                         type            dim. r/o default value */
            { kOfxParamPropAnimates,                    Property::eInt,     1,  false, "1" },
            { kOfxParamPropPersistant,                  Property::eInt,     1,  false, "1" },
            { kOfxParamPropEvaluateOnChange,            Property::eInt,     1,  false, "1" },
            { kOfxParamPropPluginMayWrite,              Property::eInt,     1,  false, "0" },
            { kOfxParamPropCacheInvalidation,           Property::eString,  1,  false, kOfxParamInvalidateValueChange },
            { kOfxParamPropCanUndo,                     Property::eInt,     1,  false, "1" },
            { kOfxParamPropParametricDimension,         Property::eInt,     1,  false, "1" },
            { kOfxParamPropParametricUIColour,          Property::eDouble,  0,  false, ""  },
            { kOfxParamPropParametricInteractBackground,Property::ePointer, 1,  false, 0   },
            { kOfxParamPropParametricRange,             Property::eDouble,  2,  false, "0"/*do not knob how to set default values
                                                                             across all dimensions*/ },
            Property::propSpecEnd
        };
        _properties.addProperties(parametricProps);
    }
    
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
static OfxStatus parametricParamGetValue(OfxParamHandle param,
                                         int   curveIndex,
                                         OfxTime time,
                                         double parametricPosition,
                                         double *returnValue){

}


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
static  OfxStatus parametricParamGetNControlPoints(OfxParamHandle param,
                                                   int   curveIndex,
                                                   double time,
                                                   int *returnValue){

}


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
static  OfxStatus parametricParamGetNthControlPoint(OfxParamHandle param,
                                                    int    curveIndex,
                                                    double time,
                                                    int    nthCtl,
                                                    double *key,
                                                    double *value){

}


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
static OfxStatus parametricParamSetNthControlPoint(OfxParamHandle param,
                                                   int   curveIndex,
                                                   double time,
                                                   int   nthCtl,
                                                   double key,
                                                   double value,
                                                   bool addAnimationKey){

}


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
static OfxStatus parametricParamAddControlPoint(OfxParamHandle param,
                                                int   curveIndex,
                                                double time,
                                                double key,
                                                double value,
                                                bool addAnimationKey){


}

/** @brief Deletes the nth control point from a parametric param.

      \arg param                 handle to the parametric parameter
      \arg curveIndex            which dimension to delete
      \arg nthCtl                the control point to delete
   */
static OfxStatus parametricParamDeleteControlPoint(OfxParamHandle param,
                                                   int   curveIndex,
                                                   int   nthCtl){

}


/** @brief Delete all curve control points on the given param.

      \arg param                 handle to the parametric parameter
      \arg curveIndex            which dimension to clear
   */
static OfxStatus parametricParamDeleteAllControlPoints(OfxParamHandle param,
                                                       int   curveIndex){

}

static OfxParametricParameterSuiteV1 gSuite = {
    parametricParamGetValue,
    parametricParamGetNControlPoints,
    parametricParamGetNthControlPoint,
    parametricParamSetNthControlPoint,
    parametricParamAddControlPoint,
    parametricParamDeleteControlPoint,
    parametricParamDeleteAllControlPoints
};



/// return the OFX function suite that manages parametric params
void *GetSuite(int version)
{
    if(version == 1)
        return (void *)(&gSuite);
    return NULL;
}

} //namespace ParametricParam

} //namespace Host

} //namespace OFX
