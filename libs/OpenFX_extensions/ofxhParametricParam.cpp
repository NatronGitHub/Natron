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

#include <cassert>

#include "ofxhParametricParam.h"

// ofx
#include "ofxCore.h"
#include "ofxParam.h"


// parametric params
#include "ofxParametricParam.h"

#include "ofxhUtilities.h"

/** @brief The initial control points for a parametric params.
 A control point is a double triplet (curveID, key, value).
 1) If curveID is not in the range [0,kOfxParamPropParametricDimension]
 then the function parametricParamAddControlPoint will fail.
 
 
 - Type - double xN
 - Property Set - parametric param descriptor (read/write) and instance (read only)
 - Value Values - Any multiple of 3 doubles validating 1)
 
 This indicates the dimension of the parametric param.
 */
#define kOfxParamPropControlPoints "OfxParamPropControlPoints"

/** @brief How many control points is there in the property set.
 
 - Type - int x1
 - Property Set - parametric param descriptor (read/write) and instance (read only)
 - Value Values - Any number >= 0
 - Default value - 1. This property is added upon the addition of the 1st control point.
 This indicates the dimension of the parametric param.
 */
#define kOfxParamPropControlPointsCount "kOfxParamPropControlPointsCount"


namespace OFX {

namespace Host {

namespace ParametricParam{

ParametricInstance::ParametricInstance(Param::Descriptor& descriptor, Param::SetInstance* instance)
    : Param::Instance(descriptor,instance)
    , _curvesDefaultInitialized(false)
{

}
    
bool ParametricInstance::isInitialized() const{
    return _curvesDefaultInitialized;
}
    
OfxStatus ParametricInstance::defaultInitializeFromDescriptor(const Param::Descriptor& descriptor)
{
    int cpsCount = descriptor.getProperties().getIntProperty(kOfxParamPropControlPointsCount);
    double cps[cpsCount*3];
    descriptor.getProperties().getDoublePropertyN(kOfxParamPropControlPoints, cps, cpsCount*3);
    
    for(int i = 0; i < cpsCount;++i){
        OfxStatus stat = addControlPoint(cps[i*3], 0., cps[i*3+1], cps[i*3+2], false);
        if(stat == kOfxStatFailed){
            return stat;
        }
    }
    _curvesDefaultInitialized = true;
    return kOfxStatOK;
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

OfxStatus  ParametricInstance::deleteControlPoint(int   /*curveIndex*/,int/*  nthCtl*/)
{
    return kOfxStatErrMissingHostFeature;
}



OfxStatus  ParametricInstance::deleteAllControlPoints(int /*curveIndex*/)
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
    Param::Base *base = reinterpret_cast<Param::Base*>(param);
    if(!base || !base->verifyMagic()) {
#         ifdef OFX_DEBUG_PARAMETERS
        std::cout << ' ' << StatStr(kOfxStatErrBadHandle) << std::endl;
#         endif
        return kOfxStatErrBadHandle;
    }
    
    ParametricInstance* instance = dynamic_cast<ParametricInstance*>(base);
    if(!instance || !instance->isInitialized()) {
#         ifdef OFX_DEBUG_PARAMETERS
        std::cout << ' ' << StatStr(kOfxStatErrBadHandle) << std::endl;
#         endif
        return kOfxStatErrBadHandle;
    }


    OfxStatus stat = kOfxStatErrUnsupported;

    stat = instance->getValue(curveIndex, time,parametricPosition,returnValue);

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
    Param::Base *base = reinterpret_cast<Param::Base*>(param);
    if(!base || !base->verifyMagic()) {
#         ifdef OFX_DEBUG_PARAMETERS
        std::cout << ' ' << StatStr(kOfxStatErrBadHandle) << std::endl;
#         endif
        return kOfxStatErrBadHandle;
    }

    ParametricInstance* instance = dynamic_cast<ParametricInstance*>(base);
    if(!instance || !instance->isInitialized()) {
#         ifdef OFX_DEBUG_PARAMETERS
        std::cout << ' ' << StatStr(kOfxStatErrBadHandle) << std::endl;
#         endif
        return kOfxStatErrBadHandle;
    }

    OfxStatus stat = kOfxStatErrUnsupported;

    stat = instance->getNControlPoints(curveIndex, time,returnValue);

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
    Param::Base *base = reinterpret_cast<Param::Base*>(param);
    if(!base || !base->verifyMagic()) {
#         ifdef OFX_DEBUG_PARAMETERS
        std::cout << ' ' << StatStr(kOfxStatErrBadHandle) << std::endl;
#         endif
        return kOfxStatErrBadHandle;
    }

    ParametricInstance* instance = dynamic_cast<ParametricInstance*>(base);
    if(!instance || !instance->isInitialized()) {
#         ifdef OFX_DEBUG_PARAMETERS
        std::cout << ' ' << StatStr(kOfxStatErrBadHandle) << std::endl;
#         endif
        return kOfxStatErrBadHandle;
    }


    OfxStatus stat = kOfxStatErrUnsupported;

    stat = instance->getNthControlPoint(curveIndex, time,nthCtl,key,value);

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
    Param::Base *base = reinterpret_cast<Param::Base*>(param);
    if(!base || !base->verifyMagic()) {
#         ifdef OFX_DEBUG_PARAMETERS
        std::cout << ' ' << StatStr(kOfxStatErrBadHandle) << std::endl;
#         endif
        return kOfxStatErrBadHandle;
    }

    ParametricInstance* instance = dynamic_cast<ParametricInstance*>(base);
    if(!instance || !instance->isInitialized()) {
#         ifdef OFX_DEBUG_PARAMETERS
        std::cout << ' ' << StatStr(kOfxStatErrBadHandle) << std::endl;
#         endif
        return kOfxStatErrBadHandle;
    }


    OfxStatus stat = kOfxStatErrUnsupported;

    stat = instance->setNthControlPoint(curveIndex, time, nthCtl, key, value, addAnimationKey);

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
    Param::Base *base = reinterpret_cast<Param::Base*>(param);
    if(!base || !base->verifyMagic()) {
#         ifdef OFX_DEBUG_PARAMETERS
        std::cout << ' ' << StatStr(kOfxStatErrBadHandle) << std::endl;
#         endif
        return kOfxStatErrBadHandle;
    }


    OfxStatus stat = kOfxStatErrUnsupported;
    
    
    ParametricInstance* instance = dynamic_cast<ParametricInstance*>(base);
    ///if the handle is an instance call the virtual function, otherwise store a new double3D property
    /// to indicate a new control point was added.
    if(instance){
        stat = instance->addControlPoint(curveIndex, time, key, value, addAnimationKey);
    }else{
        
        Param::Descriptor* desc = dynamic_cast<Param::Descriptor*>(base);
        //if it's also not a descriptor this is a bad pointer then
        if(!desc){
            stat = kOfxStatErrBadHandle;
        }else{
            
            ///check whether the property already exists in the property set
            double initialCp[3];
            initialCp[0] = -1.; //< initialises to -1 the curve index, if it hasn't change after the
                                //call to getDoublePropertyN, that means the property didn't exist already.
            desc->getProperties().getDoublePropertyN(kOfxParamPropControlPoints,initialCp,3);
            
            ///the property didn't exist, add it
            if(initialCp[0] == -1){
                
                static const Property::PropSpec parametricControlPoints[] = {
                    { kOfxParamPropControlPoints, Property::eDouble, 0, false, ""},
                    { kOfxParamPropControlPointsCount, Property::eInt, 1,false, "1"},
                    Property::propSpecEnd
                };
                desc->getProperties().addProperties(parametricControlPoints);
                
                initialCp[0] = curveIndex;
                initialCp[1] = key;
                initialCp[2] = value;
                
                desc->getProperties().setDoublePropertyN(kOfxParamPropControlPoints, initialCp, 3);
            }else{
                //the property already exists, append a triplet
                //get the number of already existing cp's
                
                int cpCount = desc->getProperties().getIntProperty(kOfxParamPropControlPointsCount);
                
                //if the property exists it must be > 0 !
                assert(cpCount > 0);
                
                //get the existing cps to copy and make a new array greater that can contain a new control point.
                double existingCPs[cpCount * 3];
                desc->getProperties().getDoublePropertyN(kOfxParamPropControlPoints, existingCPs, cpCount*3);
                
                
                double newCps[(cpCount+1) *3];
                //copy over the existing values
                memcpy(newCps, existingCPs, sizeof(double) * cpCount * 3);
                
                //set the new control point
                newCps[cpCount*3] = curveIndex;
                newCps[cpCount*3+ 1] = key;
                newCps[cpCount*3 +2] = value;
                
                //set back the property
                desc->getProperties().setDoublePropertyN(kOfxParamPropControlPoints, newCps, (cpCount+1)*3);
                desc->getProperties().setIntProperty(kOfxParamPropControlPointsCount, cpCount+1);
                
            }
            
        }
        stat = kOfxStatOK;
        
    }

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
    Param::Base *base = reinterpret_cast<Param::Base*>(param);
    if(!base || !base->verifyMagic()) {
#         ifdef OFX_DEBUG_PARAMETERS
        std::cout << ' ' << StatStr(kOfxStatErrBadHandle) << std::endl;
#         endif
        return kOfxStatErrBadHandle;
    }
    
    ParametricInstance* instance = dynamic_cast<ParametricInstance*>(base);
    if(!instance || !instance->isInitialized()) {
#         ifdef OFX_DEBUG_PARAMETERS
        std::cout << ' ' << StatStr(kOfxStatErrBadHandle) << std::endl;
#         endif
        return kOfxStatErrBadHandle;
    }

    OfxStatus stat = kOfxStatErrUnsupported;

    stat = instance->deleteControlPoint(curveIndex, nthCtl);

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
    Param::Base *base = reinterpret_cast<Param::Base*>(param);
    if(!base || !base->verifyMagic()) {
#         ifdef OFX_DEBUG_PARAMETERS
        std::cout << ' ' << StatStr(kOfxStatErrBadHandle) << std::endl;
#         endif
        return kOfxStatErrBadHandle;
    }
    
    ParametricInstance* instance = dynamic_cast<ParametricInstance*>(base);
    if(!instance || !instance->isInitialized()) {
#         ifdef OFX_DEBUG_PARAMETERS
        std::cout << ' ' << StatStr(kOfxStatErrBadHandle) << std::endl;
#         endif
        return kOfxStatErrBadHandle;
    }



    OfxStatus stat = kOfxStatErrUnsupported;

    stat = instance->deleteAllControlPoints(curveIndex);

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
