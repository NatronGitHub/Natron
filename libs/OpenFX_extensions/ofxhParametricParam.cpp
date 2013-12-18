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

namespace OFX {

namespace Host {

namespace ParametricParam{


CurvesHolder::CurvesHolder()
    : _curves()
{

}

void CurvesHolder::setDimension(int dimension)
{
    _curves.clear();
    for(int i = 0; i < dimension;++i)
    {
        ControlPointSet emptySet;
        _curves.push_back(emptySet);
    }
}

const CurvesHolder::Curves& CurvesHolder::getCurves() const
{
    return _curves;
}

OfxStatus CurvesHolder::getNControlPoints(int curveIndex,double /*time*/,int *returnValue)
{
    assert(curveIndex < (int)_curves.size());

    *returnValue = (int)_curves[curveIndex].size();

    return kOfxStatOK;
}

OfxStatus CurvesHolder::getNthControlPoint(int curveIndex,
                                           double /*time*/,
                                           int    nthCtl,
                                           double *key,
                                           double *value)
{
    assert(curveIndex < (int)_curves.size());

    const ControlPointSet& set = _curves[curveIndex];
    int index = 0;
    for(ControlPointSet::const_iterator it = set.begin();it!=set.end();++it){
        if (index == nthCtl) {
            *key = it->key;
            *value = it->value;
            return kOfxStatOK;
        }
        ++index;
    }

    return kOfxStatErrBadIndex;
}

OfxStatus CurvesHolder::setNthControlPoint(int   curveIndex,
                                           double time,
                                           int   nthCtl,
                                           double key,
                                           double value,
                                           bool addAnimationKey
                                           )
{
    assert(curveIndex < (int)_curves.size());

    ControlPointSet& set = _curves[curveIndex];
    int index = 0;
    for(ControlPointSet::iterator it = set.begin();it!=set.end();++it){
        if (index == nthCtl) {
            ControlPoint newCp;
            newCp.time = time;
            newCp.key = key;
            newCp.value = value;
            newCp.createAnimationKey = addAnimationKey;
            set.erase(it);
            set.insert(newCp);
            onNthControlPointSet(curveIndex,newCp);
            return kOfxStatOK;
        }
        ++index;
    }

    return kOfxStatErrBadIndex;
}

void CurvesHolder::onNthControlPointSet(int /*curveIndex*/,const ControlPoint& /*cp*/)
{
    //nothing to do here
}

#define OFX_CONTROL_POINTS_EQUALITY_EPSILON 1e-2

OfxStatus CurvesHolder::addControlPoint(int   curveIndex,
                                        double time,
                                        double key,
                                        double value,
                                        bool addAnimationKey)
{
    assert(curveIndex < (int)_curves.size());

    ControlPoint newCp;
    newCp.time = time;
    newCp.key = key;
    newCp.value = value;
    newCp.createAnimationKey = addAnimationKey;

    ControlPointSet& set = _curves[curveIndex];
    for(ControlPointSet::iterator it = set.begin();it!=set.end();++it){
        if (std::abs(it->key - key) < OFX_CONTROL_POINTS_EQUALITY_EPSILON) {
            set.erase(it);
            set.insert(newCp);
            onNthControlPointSet(curveIndex,newCp);
            return kOfxStatOK;
        }
    }

    ///we found no close control point so far, add a new one
    set.insert(newCp);
    onControlPointAdded(curveIndex,newCp);
    return kOfxStatOK;
}

void CurvesHolder::onControlPointAdded(int /*curveIndex*/,const ControlPoint& /*cp*/)
{
    //nothing to do here
}

OfxStatus  CurvesHolder::deleteControlPoint(int   curveIndex,int  nthCtl)
{
    assert(curveIndex < (int)_curves.size());

    ControlPointSet& set = _curves[curveIndex];
    int index = 0;
    for(ControlPointSet::const_iterator it = set.begin();it!=set.end();++it){
        if (index == nthCtl) {
            ControlPoint cp = *it;
            set.erase(it);
            onControlPointDeleted(curveIndex,cp);
            return kOfxStatOK;
        }
        ++index;
    }

    return kOfxStatErrBadIndex;
}

void CurvesHolder::onControlPointDeleted(int /*curveIndex*/,const ControlPoint& /*cp*/)
{
    //nothing to do here
}

OfxStatus  CurvesHolder::deleteAllControlPoints(int curveIndex)
{

    assert(curveIndex < (int)_curves.size());
    _curves[curveIndex].clear();
    onCurveCleared(curveIndex);
    return kOfxStatOK;
}

void CurvesHolder::onCurveCleared(int /*curveIndex*/)
{
    //nothing to do here
}

/////////////////////////////////////////ParametricDescriptor/////////////////////////////////////////


ParametricDescriptor::ParametricDescriptor(const std::string &type, const std::string &name)
    : Descriptor(type,name)
    , CurvesHolder()
{
    
    ///the properties must be added first before being able to set a notify hook
    static const Property::PropSpec allParametric[] = {
        { kOfxParamPropParametricDimension,         Property::eInt,     1,  false, "1" },
        { kOfxParamPropParametricUIColour,          Property::eDouble,  0,  false, ""  },
        { kOfxParamPropParametricInteractBackground,Property::ePointer, 1,  false, 0   },
        { kOfxParamPropParametricRange,             Property::eDouble,  2,  false, "0" },
        Property::propSpecEnd
    };
    
    getProperties().addProperties(allParametric);
    
    getProperties().setDoubleProperty(kOfxParamPropParametricRange, 0., 0);
    getProperties().setDoubleProperty(kOfxParamPropParametricRange, 1., 1);
    
    ///listen to the changes made to this property so we can add curves in response to the change of dimension.
    getProperties().addNotifyHook(kOfxParamPropParametricDimension, this);
}
    
void ParametricDescriptor::notify(const std::string &name, bool /*single*/, int /*num*/) OFX_EXCEPTION_SPEC
{
    if(name == kOfxParamPropParametricDimension){
        setDimension(getProperties().getIntProperty(kOfxParamPropParametricDimension));
    }
}

OfxStatus ParametricDescriptor::getValue(int /*curveIndex*/,OfxTime /*time*/,double /*parametricPosition*/,double */*returnValue*/)
{
    //nothing to do
    return kOfxStatErrMissingHostFeature;
}

/////////////////////////////////////////ParametricInstance/////////////////////////////////////////

ParametricInstance::ParametricInstance(Param::Descriptor& descriptor, Param::SetInstance* instance)
    : Param::Instance(descriptor,instance)
{
    ParametricDescriptor &parametricDesc = dynamic_cast<ParametricDescriptor&>(descriptor);

    ///copy all the curves defined in the descriptor to initialize them
    _curves = parametricDesc.getCurves();

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

void ParametricInstance::onNthControlPointSet(int /*curveIndex*/,const ControlPoint& /*cp*/)
{
    //nothing do to here
}

void ParametricInstance::onControlPointAdded(int /*curveIndex*/,const ControlPoint& /*cp*/)
{
    //nothing do to here
}

void ParametricInstance::onControlPointDeleted(int /*curveIndex*/,const ControlPoint& /*cp*/)
{
    //nothing do to here
}

void ParametricInstance::onCurveCleared(int /*curveIndex*/)
{
    //nothing do to here
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
    CurvesHolder *curveHolder = reinterpret_cast<CurvesHolder*>(param);
    if(!curveHolder) {
#         ifdef OFX_DEBUG_PARAMETERS
        std::cout << ' ' << StatStr(kOfxStatErrBadHandle) << std::endl;
#         endif
        return kOfxStatErrBadHandle;
    }


    OfxStatus stat = kOfxStatErrUnsupported;

    stat = curveHolder->getValue(curveIndex, time,parametricPosition,returnValue);

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
    CurvesHolder *curveHolder = reinterpret_cast<CurvesHolder*>(param);
    if(!curveHolder) {
#         ifdef OFX_DEBUG_PARAMETERS
        std::cout << ' ' << StatStr(kOfxStatErrBadHandle) << std::endl;
#         endif
        return kOfxStatErrBadHandle;
    }


    OfxStatus stat = kOfxStatErrUnsupported;

    stat = curveHolder->getNControlPoints(curveIndex, time,returnValue);

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
    CurvesHolder *curveHolder = reinterpret_cast<CurvesHolder*>(param);
    if(!curveHolder) {
#         ifdef OFX_DEBUG_PARAMETERS
        std::cout << ' ' << StatStr(kOfxStatErrBadHandle) << std::endl;
#         endif
        return kOfxStatErrBadHandle;
    }


    OfxStatus stat = kOfxStatErrUnsupported;

    stat = curveHolder->getNthControlPoint(curveIndex, time,nthCtl,key,value);

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
    CurvesHolder *curveHolder = reinterpret_cast<CurvesHolder*>(param);
    if(!curveHolder) {
#         ifdef OFX_DEBUG_PARAMETERS
        std::cout << ' ' << StatStr(kOfxStatErrBadHandle) << std::endl;
#         endif
        return kOfxStatErrBadHandle;
    }


    OfxStatus stat = kOfxStatErrUnsupported;

    stat = curveHolder->setNthControlPoint(curveIndex, time, nthCtl, key, value, addAnimationKey);

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
    CurvesHolder *curveHolder = reinterpret_cast<CurvesHolder*>(param);
    if(!curveHolder) {
#         ifdef OFX_DEBUG_PARAMETERS
        std::cout << ' ' << StatStr(kOfxStatErrBadHandle) << std::endl;
#         endif
        return kOfxStatErrBadHandle;
    }


    OfxStatus stat = kOfxStatErrUnsupported;

    stat = curveHolder->addControlPoint(curveIndex,time,key,value,addAnimationKey);

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
    CurvesHolder *curveHolder= reinterpret_cast<CurvesHolder*>(param);
    if(!curveHolder) {
#         ifdef OFX_DEBUG_PARAMETERS
        std::cout << ' ' << StatStr(kOfxStatErrBadHandle) << std::endl;
#         endif
        return kOfxStatErrBadHandle;
    }


    OfxStatus stat = kOfxStatErrUnsupported;

    stat = curveHolder->deleteControlPoint(curveIndex, nthCtl);

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
    CurvesHolder *curveHolder = reinterpret_cast<CurvesHolder*>(param);
    if(!curveHolder) {
#         ifdef OFX_DEBUG_PARAMETERS
        std::cout << ' ' << StatStr(kOfxStatErrBadHandle) << std::endl;
#         endif
        return kOfxStatErrBadHandle;
    }


    OfxStatus stat = kOfxStatErrUnsupported;

    stat = curveHolder->deleteAllControlPoints(curveIndex);

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
