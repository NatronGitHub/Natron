/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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
/*
 *Created by Alexandre Gauthier-Foichat-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#include <cassert>
#include <sstream>
#include <cmath>
#include <iostream>
#include "ofxhParametricParam.h"

// ofx
#include "ofxCore.h"
#include "ofxParam.h"


// parametric params
#include "ofxParametricParam.h"

#include "ofxhPropertySuite.h"
#include "ofxProperty.h"
#include "ofxhUtilities.h"

/** @brief The initial control points for a parametric params.
 A control point is a double pair (key, value).
 The properties are each named "OfxParamPropControlPoints_" with the curveID post pended
 1) If curveID is not in the range [0,kOfxParamPropParametricDimension]
 then the function parametricParamAddControlPoint will fail.
 
 
 - Type - double xN
 - Property Set - parametric param descriptor (read/write) and instance (read only)
 - Value Values - Any multiple of 2 doubles validating 1)
 
 This indicates the dimension of the parametric param.
 */
#define kOfxParamPropControlPoints "OfxParamPropControlPoints"

namespace {
struct BezierCP {
    double key;
    double value;
};
typedef std::vector<BezierCP> ControlPointV;
struct ControlPoint_LessThan {
    bool operator() (const BezierCP & left, const BezierCP & right)
    {
        return left.key < right.key;
    }
};
struct ControlPoint_IsClose {
    const BezierCP& _refcp;
    double _eps;
    ControlPoint_IsClose(const BezierCP& refcp, double eps) : _refcp(refcp), _eps(eps){}

    bool operator() (const BezierCP& cp) const {
        return std::abs(cp.key - _refcp.key) < _eps;
    }
};
struct ControlPoint_MuchLessThan {
    double _eps;
    ControlPoint_MuchLessThan(double eps) : _eps(eps){}
    bool operator() (const BezierCP & left, const BezierCP & right)
    {
        return left.key < (right.key - _eps);
    }
};

}

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

OfxStatus ParametricInstance::defaultInitializeFromDescriptor(int curveIndex,const Param::Descriptor& descriptor)
{
    const Property::Set &descProps = descriptor.getProperties();
    
    int curveCount = descProps.getIntProperty(kOfxParamPropParametricDimension);
    if(curveIndex >= curveCount){
        return kOfxStatFailed;
    }

    std::stringstream name;
    name << kOfxParamPropControlPoints << '_' << curveIndex;
    if (descProps.fetchProperty(name.str())) {
        // there is a curve for dimension curveIndex
        int cpsCount = descProps.getDimension(name.str()) / 2;
        ControlPointV cps(cpsCount);
        descProps.getDoublePropertyN(name.str(), &cps[0].key, cpsCount*2);
        for (int i = 0; i < cpsCount; ++i) {
            OfxStatus stat = addControlPoint(curveIndex, 0., cps[i].key, cps[i].value, false);
            if (stat == kOfxStatFailed) {
                return stat;
            }
        }
    }
    return kOfxStatOK;
}
    
OfxStatus ParametricInstance::defaultInitializeAllCurves(const Param::Descriptor& descriptor){
    int curveCount = descriptor.getProperties().getIntProperty(kOfxParamPropParametricDimension);
    for(int i = 0; i < curveCount; ++i){
        OfxStatus stat = defaultInitializeFromDescriptor(i, descriptor);
        if(stat == kOfxStatFailed){
            return stat;
        }
    }
    _curvesDefaultInitialized = true;
    return kOfxStatOK;
}

// copy one parameter to another, with a range (NULL means to copy all animation)
OfxStatus ParametricInstance::copyFrom(const Param::Instance &/*instance*/, OfxTime /*offset*/, const OfxRangeD* /*range*/)
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
#   ifdef OFX_DEBUG_PARAMETERS
    std::cout << "OFX: parametricParamGetValue - " << param << " ...";
#   endif
    Param::Base *base = reinterpret_cast<Param::Base*>(param);
    if(!base || !base->verifyMagic()) {
#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << ' ' << StatStr(kOfxStatErrBadHandle) << std::endl;
#       endif
        return kOfxStatErrBadHandle;
    }

    ParametricInstance* instance = dynamic_cast<ParametricInstance*>(base);
    if(!instance || !instance->isInitialized()) {
#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << ' ' << StatStr(kOfxStatErrBadHandle) << std::endl;
#       endif
        return kOfxStatErrBadHandle;
    }


    OfxStatus stat = instance->getValue(curveIndex, time,parametricPosition,returnValue);

#   ifdef OFX_DEBUG_PARAMETERS
    std::cout << ' ' << StatStr(stat) << std::endl;
#   endif
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
#   ifdef OFX_DEBUG_PARAMETERS
    std::cout << "OFX: parametricParamGetNControlPoints - " << param << " ...";
#   endif
    Param::Base *base = reinterpret_cast<Param::Base*>(param);
    if(!base || !base->verifyMagic()) {
#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << ' ' << StatStr(kOfxStatErrBadHandle) << std::endl;
#       endif
        return kOfxStatErrBadHandle;
    }

    ParametricInstance* instance = dynamic_cast<ParametricInstance*>(base);
    if (instance) {
        if(!instance->isInitialized()) {
#           ifdef OFX_DEBUG_PARAMETERS
            std::cout << ' ' << StatStr(kOfxStatErrBadHandle) << std::endl;
#           endif
            return kOfxStatErrBadHandle;
        }

        OfxStatus stat = instance->getNControlPoints(curveIndex, time,returnValue);
#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << ' ' << StatStr(stat) << std::endl;
#       endif
        return stat;
    }

    Param::Descriptor* descriptor = dynamic_cast<Param::Descriptor*>(base);
    //if it's also not a descriptor this is a bad pointer then
    if (!descriptor) {
#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << ' ' << StatStr(kOfxStatErrBadHandle) << std::endl;
#       endif
        return kOfxStatErrBadHandle;
    }

    Property::Set &descProps = descriptor->getProperties();
    int curveCount = descProps.getIntProperty(kOfxParamPropParametricDimension);
    if (curveIndex < 0 || curveIndex >= curveCount ) {
#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << ' ' << StatStr(kOfxStatErrBadIndex) << std::endl;
#       endif
        return kOfxStatErrBadIndex;
    }

    ///check whether the property already exists in the property set
    std::stringstream name;
    name << kOfxParamPropControlPoints << '_' << curveIndex;
    std::string namestr = name.str();
    // getDimension returns 0 if the property does not exist
    *returnValue = descProps.getDimension(name.str()) / 2;
    OfxStatus stat = kOfxStatOK;

#   ifdef OFX_DEBUG_PARAMETERS
    std::cout << ' ' << StatStr(stat) << std::endl;
#   endif
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
             - ::kOfxStatErrBadIndex   - the curve index or the control point index was invalid
             */
static  OfxStatus parametricParamGetNthControlPoint(OfxParamHandle param,
                                                    int    curveIndex,
                                                    double time,
                                                    int    nthCtl,
                                                    double *key,
                                                    double *value){
#   ifdef OFX_DEBUG_PARAMETERS
    std::cout << "OFX: parametricParamGetNthControlPoint - " << param << " ...";
#   endif
    Param::Base *base = reinterpret_cast<Param::Base*>(param);
    if(!base || !base->verifyMagic()) {
#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << ' ' << StatStr(kOfxStatErrBadHandle) << std::endl;
#       endif
        return kOfxStatErrBadHandle;
    }

    ParametricInstance* instance = dynamic_cast<ParametricInstance*>(base);
    if (instance) {
        if(!instance->isInitialized()) {
#           ifdef OFX_DEBUG_PARAMETERS
            std::cout << ' ' << StatStr(kOfxStatErrBadHandle) << std::endl;
#           endif
            return kOfxStatErrBadHandle;
        }

        OfxStatus stat = instance->getNthControlPoint(curveIndex, time,nthCtl,key,value);

#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << ' ' << StatStr(stat) << std::endl;
#       endif
        return stat;
    }

    Param::Descriptor* descriptor = dynamic_cast<Param::Descriptor*>(base);
    //if it's also not a descriptor this is a bad pointer then
    if (!descriptor) {
#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << ' ' << StatStr(kOfxStatErrBadHandle) << std::endl;
#       endif
        return kOfxStatErrBadHandle;
    }

    Property::Set &descProps = descriptor->getProperties();
    int curveCount = descProps.getIntProperty(kOfxParamPropParametricDimension);
    if (curveIndex < 0 || curveIndex >= curveCount ) {
#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << ' ' << StatStr(kOfxStatErrBadIndex) << std::endl;
#       endif
        return kOfxStatErrBadIndex;
    }

    ///check whether the property already exists in the property set
    std::stringstream name;
    name << kOfxParamPropControlPoints << '_' << curveIndex;
    std::string namestr = name.str();
    // getDimension returns 0 if the property does not exist
    int cpsCount = descProps.getDimension(name.str()) / 2;
    if (nthCtl < 0 || nthCtl >= cpsCount ) {
#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << ' ' << StatStr(kOfxStatErrBadIndex) << std::endl;
#       endif
        return kOfxStatErrBadIndex;
    }

    *key = descProps.getDoubleProperty(namestr, 2*nthCtl);
    *value = descProps.getDoubleProperty(namestr, 2*nthCtl+1);

    OfxStatus stat = kOfxStatOK;

#   ifdef OFX_DEBUG_PARAMETERS
    std::cout << ' ' << StatStr(stat) << std::endl;
#   endif
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
#   ifdef OFX_DEBUG_PARAMETERS
    std::cout << "OFX: parametricParamSetNthControlPoint - " << param << " ...";
#   endif
    Param::Base *base = reinterpret_cast<Param::Base*>(param);
    if(!base || !base->verifyMagic()) {
#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << ' ' << StatStr(kOfxStatErrBadHandle) << std::endl;
#       endif
        return kOfxStatErrBadHandle;
    }

    ParametricInstance* instance = dynamic_cast<ParametricInstance*>(base);
    if (instance) {
        if(!instance->isInitialized()) {
#           ifdef OFX_DEBUG_PARAMETERS
            std::cout << ' ' << StatStr(kOfxStatErrBadHandle) << std::endl;
#           endif
            return kOfxStatErrBadHandle;
        }

        OfxStatus stat = instance->setNthControlPoint(curveIndex, time, nthCtl, key, value, addAnimationKey);

#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << ' ' << StatStr(stat) << std::endl;
#       endif
        return stat;
    }

    Param::Descriptor* descriptor = dynamic_cast<Param::Descriptor*>(base);
    //if it's also not a descriptor this is a bad pointer then
    if (!descriptor) {
#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << ' ' << StatStr(kOfxStatErrBadHandle) << std::endl;
#       endif
        return kOfxStatErrBadHandle;
    }

    Property::Set &descProps = descriptor->getProperties();
    int curveCount = descProps.getIntProperty(kOfxParamPropParametricDimension);
    if (curveIndex < 0 || curveIndex >= curveCount ) {
#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << ' ' << StatStr(kOfxStatErrBadIndex) << std::endl;
#       endif
        return kOfxStatErrBadIndex;
    }

    ///check whether the property already exists in the property set
    std::stringstream name;
    name << kOfxParamPropControlPoints << '_' << curveIndex;
    std::string namestr = name.str();
    // getDimension returns 0 if the property does not exist
    int cpsCount = descProps.getDimension(name.str()) / 2;
    if (nthCtl < 0 || nthCtl >= cpsCount ) {
#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << ' ' << StatStr(kOfxStatErrBadIndex) << std::endl;
#       endif
        return kOfxStatErrBadIndex;
    }

    descProps.setDoubleProperty(namestr, key, 2*nthCtl);
    descProps.setDoubleProperty(namestr, value, 2*nthCtl+1);

    OfxStatus stat = kOfxStatOK;

#   ifdef OFX_DEBUG_PARAMETERS
    std::cout << ' ' << StatStr(stat) << std::endl;
#   endif
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
#   ifdef OFX_DEBUG_PARAMETERS
    std::cout << "OFX: parametricParamAddControlPoint - " << param << " ...";
#   endif
    Param::Base *base = reinterpret_cast<Param::Base*>(param);
    if(!base || !base->verifyMagic()) {
#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << ' ' << StatStr(kOfxStatErrBadHandle) << std::endl;
#       endif
        return kOfxStatErrBadHandle;
    }


    OfxStatus stat = kOfxStatErrUnsupported;


    ParametricInstance* instance = dynamic_cast<ParametricInstance*>(base);
    ///if the handle is an instance call the virtual function, otherwise store a new double3D property
    /// to indicate a new control point was added.
    if (instance) {
        if (!instance->isInitialized()) {
#           ifdef OFX_DEBUG_PARAMETERS
            std::cout << ' ' << StatStr(kOfxStatErrBadHandle) << std::endl;
#           endif
            return kOfxStatErrBadHandle;
        }

        stat = instance->addControlPoint(curveIndex, time, key, value, addAnimationKey);
#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << ' ' << StatStr(stat) << std::endl;
#       endif
        return stat;
    }

    Param::Descriptor* descriptor = dynamic_cast<Param::Descriptor*>(base);
    //if it's also not a descriptor this is a bad pointer then
    if (!descriptor) {
#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << ' ' << StatStr(kOfxStatErrBadHandle) << std::endl;
#       endif
        return kOfxStatErrBadHandle;
    }

    Property::Set &descProps = descriptor->getProperties();
    int curveCount = descProps.getIntProperty(kOfxParamPropParametricDimension);
    if (curveIndex < 0 || curveIndex >= curveCount ) {
#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << ' ' << StatStr(kOfxStatErrBadHandle) << std::endl;
#       endif
        return kOfxStatErrBadIndex;
    }

    ///check whether the property already exists in the property set
    std::stringstream name;
    name << kOfxParamPropControlPoints << '_' << curveIndex;
    std::string namestr = name.str();
    BezierCP cp = { key, value };

    if (!descProps.fetchProperty(namestr)) {
        // the property does not exist, create it
        const Property::PropSpec parametricControlPoints = {namestr.c_str(), Property::eDouble, 0, false, ""};
        descProps.createProperty(parametricControlPoints);
        descProps.setDoublePropertyN(namestr, &cp.key, 2);
    } else {
        //the property already exists
        int cpsCount = descProps.getDimension(name.str()) / 2;
        //if the property exists it must be > 0 !
        assert(cpsCount > 0);

        //get the existing cps
        ControlPointV cps(cpsCount);
        descProps.getDoublePropertyN(namestr, &cps[0].key, cps.size()*2);

        // Note: an optimal implementation could work on a std::set (which is an ordered container),
        // but we propose this suboptimal implementation based on std::vector, given the fact that
        // there are usually not many control points.

        // if this key or the next one is almost equal, then replace it:
        // http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#OfxParametricParameterSuiteV1_parametricParamAddControlPoint
        // "If a key exists sufficiently close to 'key', then it will be set to the indicated control point."
        // As a definition of "sufficiently close", we use:
        // "the distance to the closest key is less than 1/10000 of the parametric range"
        double paramMin = descProps.getDoubleProperty(kOfxParamPropParametricRange, 0);
        double paramMax = descProps.getDoubleProperty(kOfxParamPropParametricRange, 1);
        double paramEps = 1e-4 * std::abs(paramMax - paramMin);
        // std::lower_bound finds the element in a sorted vector in logarithmic time
        ControlPointV::iterator it = std::lower_bound(cps.begin(), cps.end(), cp, ControlPoint_MuchLessThan(paramEps));
        // lower_bound returned the first element for which the key is >= cp.key-paramEps.
        // now check that its key is also <= cp.key+paramEps
        if (it != cps.end() && it->key <= cp.key + paramEps) {
            // found a "sufficiently close" element, replace it
            *it = cp;
        } else {
            // insert it
            cps.insert(it, cp);
        }
        //set back the property
        descProps.setDoublePropertyN(namestr, &cps[0].key, cps.size()*2);
    }

    stat = kOfxStatOK;

#   ifdef OFX_DEBUG_PARAMETERS
    std::cout << ' ' << StatStr(stat) << std::endl;
#   endif
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
#   ifdef OFX_DEBUG_PARAMETERS
    std::cout << "OFX: parametricParamDeleteControlPoint - " << param << " ...";
#   endif
    Param::Base *base = reinterpret_cast<Param::Base*>(param);
    if(!base || !base->verifyMagic()) {
#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << ' ' << StatStr(kOfxStatErrBadHandle) << std::endl;
#       endif
        return kOfxStatErrBadHandle;
    }

    ParametricInstance* instance = dynamic_cast<ParametricInstance*>(base);
    if (instance) {
        if (!instance->isInitialized()) {
#           ifdef OFX_DEBUG_PARAMETERS
            std::cout << ' ' << StatStr(kOfxStatErrBadHandle) << std::endl;
#           endif
            return kOfxStatErrBadHandle;
        }

        OfxStatus stat = instance->deleteControlPoint(curveIndex, nthCtl);
#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << ' ' << StatStr(stat) << std::endl;
#       endif
        return stat;
    }

    Param::Descriptor* descriptor = dynamic_cast<Param::Descriptor*>(base);
    //if it's also not a descriptor this is a bad pointer then
    if (!descriptor) {
#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << ' ' << StatStr(kOfxStatErrBadHandle) << std::endl;
#       endif
        return kOfxStatErrBadHandle;
    }

    Property::Set &descProps = descriptor->getProperties();
    int curveCount = descProps.getIntProperty(kOfxParamPropParametricDimension);
    if (curveIndex < 0 || curveIndex >= curveCount ) {
#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << ' ' << StatStr(kOfxStatErrBadIndex) << std::endl;
#       endif
        return kOfxStatErrBadIndex;
    }

    ///check whether the property already exists in the property set
    std::stringstream name;
    name << kOfxParamPropControlPoints << '_' << curveIndex;
    std::string namestr = name.str();
    // getDimension returns 0 if the property does not exist
    int cpsCount = descProps.getDimension(name.str()) / 2;
    if (nthCtl < 0 || nthCtl >= cpsCount ) {
#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << ' ' << StatStr(kOfxStatErrBadIndex) << std::endl;
#       endif
        return kOfxStatErrBadIndex;
    }

    //get the existing cps
    ControlPointV cps(cpsCount);
    descProps.getDoublePropertyN(namestr, &cps[0].key, cps.size()*2);

    // delete the control point
    cps.erase(cps.begin()+nthCtl);

    //set back the property
    descProps.setDoublePropertyN(namestr, &cps[0].key, cps.size()*2);

    OfxStatus stat = kOfxStatOK;

#   ifdef OFX_DEBUG_PARAMETERS
    std::cout << ' ' << StatStr(stat) << std::endl;
#   endif
    return stat;
}


/** @brief Delete all curve control points on the given param.

             \arg param                 handle to the parametric parameter
             \arg curveIndex            which dimension to clear
             */
static OfxStatus parametricParamDeleteAllControlPoints(OfxParamHandle param,
                                                       int   curveIndex){
#   ifdef OFX_DEBUG_PARAMETERS
    std::cout << "OFX: parametricParamDeleteAllControlPoints - " << param << " ...";
#   endif
    Param::Base *base = reinterpret_cast<Param::Base*>(param);
    if(!base || !base->verifyMagic()) {
#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << ' ' << StatStr(kOfxStatErrBadHandle) << std::endl;
#       endif
        return kOfxStatErrBadHandle;
    }

    ParametricInstance* instance = dynamic_cast<ParametricInstance*>(base);
    if (instance) {
        if (!instance->isInitialized()) {
#           ifdef OFX_DEBUG_PARAMETERS
            std::cout << ' ' << StatStr(kOfxStatErrBadHandle) << std::endl;
#           endif
            return kOfxStatErrBadHandle;
        }

        OfxStatus stat = instance->deleteAllControlPoints(curveIndex);
#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << ' ' << StatStr(stat) << std::endl;
#       endif
        return stat;
    }

    Param::Descriptor* descriptor = dynamic_cast<Param::Descriptor*>(base);
    //if it's also not a descriptor this is a bad pointer then
    if (!descriptor) {
#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << ' ' << StatStr(kOfxStatErrBadHandle) << std::endl;
#       endif
        return kOfxStatErrBadHandle;
    }

    Property::Set &descProps = descriptor->getProperties();
    int curveCount = descProps.getIntProperty(kOfxParamPropParametricDimension);
    if (curveIndex < 0 || curveIndex >= curveCount ) {
#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << ' ' << StatStr(kOfxStatErrBadHandle) << std::endl;
#       endif
        return kOfxStatErrBadIndex;
    }

    std::stringstream name;
    name << kOfxParamPropControlPoints << '_' << curveIndex;
    std::string namestr = name.str();
    // set the property dimension to 0
    descProps.setDoublePropertyN(namestr, NULL, 0);

    OfxStatus stat = kOfxStatOK;

#   ifdef OFX_DEBUG_PARAMETERS
    std::cout << ' ' << StatStr(stat) << std::endl;
#   endif
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
