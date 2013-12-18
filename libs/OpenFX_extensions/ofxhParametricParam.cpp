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


#include "ofxhParametricParam.h"

// ofx
#include "ofxCore.h"
#include "ofxParam.h"


// parametric params
#include "ofxParametricParam.h"

namespace OFX {

namespace Host {

namespace ParametricParam{

    //to be moved to host support.
void addParametricProperties(Param::Base& propertySetHolder){

    bool isDescriptor = dynamic_cast<Param::Descriptor*>(&propertySetHolder) != NULL;
    Property::PropSpec parametricProps[] = {
        /* name                                         type            dim. r/o default value */
        { kOfxParamPropAnimates,                    Property::eInt,     1,  !isDescriptor, "1" },
        { kOfxParamPropPersistant,                  Property::eInt,     1,  !isDescriptor, "1" },
        { kOfxParamPropEvaluateOnChange,            Property::eInt,     1,  !isDescriptor, "1" },
        { kOfxParamPropPluginMayWrite,              Property::eInt,     1,  !isDescriptor, "0" },
        { kOfxParamPropCacheInvalidation,           Property::eString,  1,  !isDescriptor, kOfxParamInvalidateValueChange },
        { kOfxParamPropCanUndo,                     Property::eInt,     1,  !isDescriptor, "1" },
        { kOfxParamPropParametricDimension,         Property::eInt,     1,  !isDescriptor, "1" },
        { kOfxParamPropParametricUIColour,          Property::eDouble,  0,  !isDescriptor, ""  },
        { kOfxParamPropParametricInteractBackground,Property::ePointer, 1,  !isDescriptor, 0   },
        { kOfxParamPropParametricRange,             Property::eDouble,  2,  !isDescriptor, "0"},
        Property::propSpecEnd
    };

    propertySetHolder.getProperties().addProperties(parametricProps);
    propertySetHolder.getProperties().setDoubleProperty(kOfxParamPropParametricRange, 0., 0);
    propertySetHolder.getProperties().setDoubleProperty(kOfxParamPropParametricRange, 1., 1);
}


ParametricInstance::ParametricInstance(Param::Descriptor& descriptor, Param::SetInstance* instance)
    : Param::Instance(descriptor,instance)
{
    addParametricProperties(*this);
}
// copy one parameter to another
OfxStatus ParametricInstance::copy(const Param::Instance &/*instance*/, OfxTime /*offset*/)
{
    return kOfxStatErrMissingHostFeature;
}

// copy one parameter to another, with a range
OfxStatus ParametricInstance::copy(const Param::Instance &/*instance*/, OfxTime /*offset*/, OfxRangeD /*range*/)
{
    return kOfxStatErrMissingHostFeature;
}

// callback which should set enabled state as appropriate
void ParametricInstance::setEnabled()
{

}

// callback which should set secret state as appropriate
void ParametricInstance::setSecret()
{

}

/// callback which should update label
void ParametricInstance::setLabel()
{

}

/// callback which should set
void ParametricInstance::setDisplayRange()
{

}

OfxStatus ParametricInstance::getNumKeys(unsigned int &/*nKeys*/) const
{
    return kOfxStatErrMissingHostFeature;
}

OfxStatus ParametricInstance::getKeyTime(int /*nth*/, OfxTime& /*time*/) const
{
    return kOfxStatErrMissingHostFeature;
}

OfxStatus ParametricInstance::getKeyIndex(OfxTime /*time*/, int /*direction*/, int & /*index*/) const
{
    return kOfxStatErrMissingHostFeature;
}

OfxStatus ParametricInstance::deleteKey(OfxTime /*time*/)
{
    return kOfxStatErrMissingHostFeature;
}

OfxStatus ParametricInstance::deleteAllKeys()
{
    return kOfxStatErrMissingHostFeature;
}

OfxStatus ParametricInstance::getValue(int /*curveIndex*/,OfxTime /*time*/,double /*parametricPosition*/,double */*returnValue*/)
{
    return kOfxStatErrMissingHostFeature;
}

OfxStatus ParametricInstance::getNControlPoints(int /*curveIndex*/,double /*time*/,int */*returnValue*/)
{
    return kOfxStatErrMissingHostFeature;
}

OfxStatus ParametricInstance::getNthControlPoint(int /*curveIndex*/,
                                                 double /*time*/,
                                                 int    /*nthCtl*/,
                                                 double */*key*/,
                                                 double */*value*/)
{
    return kOfxStatErrMissingHostFeature;
}

OfxStatus ParametricInstance::setNthControlPoint(int   /*curveIndex*/,
                                                 double /*time*/,
                                                 int   /*nthCtl*/,
                                                 double /*key*/,
                                                 double /*value*/,
                                                 bool /*addAnimationKey*/
                                                 )
{
    return kOfxStatErrMissingHostFeature;
}

OfxStatus ParametricInstance::addControlPoint(int   /*curveIndex*/,
                                              double /*time*/,
                                              double /*key*/,
                                              double /*value*/,
                                              bool /*addAnimationKey*/)
{
    return kOfxStatErrMissingHostFeature;
}

OfxStatus  ParametricInstance::deleteControlPoint(int   /*curveIndex*/,int  /* nthCtl*/)
{
    return kOfxStatErrMissingHostFeature;
}

OfxStatus  ParametricInstance::deleteAllControlPoints(int   /*curveIndex*/)
{
    return kOfxStatErrMissingHostFeature;
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
#       ifdef OFX_DEBUG_PARAMETERS
    std::cout << "OFX: parametricParamGetValue - " << param << " ...";
#       endif
    ParametricInstance *paramInstance = reinterpret_cast<ParametricInstance*>(param);
    if(!paramInstance || !paramInstance->verifyMagic()) {
#         ifdef OFX_DEBUG_PARAMETERS
        std::cout << ' ' << StatStr(kOfxStatErrBadHandle) << std::endl;
#         endif
        return kOfxStatErrBadHandle;
    }
    
    
    OfxStatus stat = kOfxStatErrUnsupported;
    
    stat = paramInstance->getValue(curveIndex, time,parametricPosition,returnValue);
    
#       ifdef OFX_DEBUG_PARAMETERS
    std::cout << ' ' << StatStr(stat) << std::endl;
#       endif
    return stat;

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
#       ifdef OFX_DEBUG_PARAMETERS
    std::cout << "OFX: parametricParamGetNControlPoints - " << param << " ...";
#       endif
    ParametricInstance *paramInstance = reinterpret_cast<ParametricInstance*>(param);
    if(!paramInstance || !paramInstance->verifyMagic()) {
#         ifdef OFX_DEBUG_PARAMETERS
        std::cout << ' ' << StatStr(kOfxStatErrBadHandle) << std::endl;
#         endif
        return kOfxStatErrBadHandle;
    }
    
    
    OfxStatus stat = kOfxStatErrUnsupported;
    
    stat = paramInstance->getNControlPoints(curveIndex, time,returnValue);
    
#       ifdef OFX_DEBUG_PARAMETERS
    std::cout << ' ' << StatStr(stat) << std::endl;
#       endif
    return stat;
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
#       ifdef OFX_DEBUG_PARAMETERS
    std::cout << "OFX: parametricParamGetNthControlPoint - " << param << " ...";
#       endif
    ParametricInstance *paramInstance = reinterpret_cast<ParametricInstance*>(param);
    if(!paramInstance || !paramInstance->verifyMagic()) {
#         ifdef OFX_DEBUG_PARAMETERS
        std::cout << ' ' << StatStr(kOfxStatErrBadHandle) << std::endl;
#         endif
        return kOfxStatErrBadHandle;
    }
    
    
    OfxStatus stat = kOfxStatErrUnsupported;
    
    stat = paramInstance->getNthControlPoint(curveIndex, time,nthCtl,key,value);
    
#       ifdef OFX_DEBUG_PARAMETERS
    std::cout << ' ' << StatStr(stat) << std::endl;
#       endif
    return stat;
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
#       ifdef OFX_DEBUG_PARAMETERS
    std::cout << "OFX: parametricParamSetNthControlPoint - " << param << " ...";
#       endif
    ParametricInstance *paramInstance = reinterpret_cast<ParametricInstance*>(param);
    if(!paramInstance || !paramInstance->verifyMagic()) {
#         ifdef OFX_DEBUG_PARAMETERS
        std::cout << ' ' << StatStr(kOfxStatErrBadHandle) << std::endl;
#         endif
        return kOfxStatErrBadHandle;
    }
    
    
    OfxStatus stat = kOfxStatErrUnsupported;
    
    stat = paramInstance->setNthControlPoint(curveIndex, time, nthCtl, key, value, addAnimationKey);
    
#       ifdef OFX_DEBUG_PARAMETERS
    std::cout << ' ' << StatStr(stat) << std::endl;
#       endif
    return stat;
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
#       ifdef OFX_DEBUG_PARAMETERS
    std::cout << "OFX: parametricParamAddControlPoint - " << param << " ...";
#       endif
    ParametricInstance *paramInstance = reinterpret_cast<ParametricInstance*>(param);
    if(!paramInstance || !paramInstance->verifyMagic()) {
#         ifdef OFX_DEBUG_PARAMETERS
        std::cout << ' ' << StatStr(kOfxStatErrBadHandle) << std::endl;
#         endif
        return kOfxStatErrBadHandle;
    }
    
    
    OfxStatus stat = kOfxStatErrUnsupported;
    
    stat = paramInstance->addControlPoint(curveIndex,time,key,value,addAnimationKey);
    
#       ifdef OFX_DEBUG_PARAMETERS
    std::cout << ' ' << StatStr(stat) << std::endl;
#       endif
    return stat;

}

/** @brief Deletes the nth control point from a parametric param.

      \arg param                 handle to the parametric parameter
      \arg curveIndex            which dimension to delete
      \arg nthCtl                the control point to delete
   */
static OfxStatus parametricParamDeleteControlPoint(OfxParamHandle param,
                                                   int   curveIndex,
                                                   int   nthCtl){
#       ifdef OFX_DEBUG_PARAMETERS
    std::cout << "OFX: parametricParamDeleteControlPoint - " << param << " ...";
#       endif
    ParametricInstance *paramInstance = reinterpret_cast<ParametricInstance*>(param);
    if(!paramInstance || !paramInstance->verifyMagic()) {
#         ifdef OFX_DEBUG_PARAMETERS
        std::cout << ' ' << StatStr(kOfxStatErrBadHandle) << std::endl;
#         endif
        return kOfxStatErrBadHandle;
    }
    
    
    OfxStatus stat = kOfxStatErrUnsupported;
    
    stat = paramInstance->deleteControlPoint(curveIndex, nthCtl);
    
#       ifdef OFX_DEBUG_PARAMETERS
    std::cout << ' ' << StatStr(stat) << std::endl;
#       endif
    return stat;
}


/** @brief Delete all curve control points on the given param.

      \arg param                 handle to the parametric parameter
      \arg curveIndex            which dimension to clear
   */
static OfxStatus parametricParamDeleteAllControlPoints(OfxParamHandle param,
                                                       int   curveIndex){
#       ifdef OFX_DEBUG_PARAMETERS
    std::cout << "OFX: parametricParamDeleteAllControlPoints - " << param << " ...";
#       endif
    ParametricInstance *paramInstance = reinterpret_cast<ParametricInstance*>(param);
    if(!paramInstance || !paramInstance->verifyMagic()) {
#         ifdef OFX_DEBUG_PARAMETERS
        std::cout << ' ' << StatStr(kOfxStatErrBadHandle) << std::endl;
#         endif
        return kOfxStatErrBadHandle;
    }
    
    
    OfxStatus stat = kOfxStatErrUnsupported;
    
    stat = paramInstance->deleteAllControlPoints(curveIndex);
    
#       ifdef OFX_DEBUG_PARAMETERS
    std::cout << ' ' << StatStr(stat) << std::endl;
#       endif
    return stat;
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
