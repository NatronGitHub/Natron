/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier
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
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#ifndef OFXHPARAMETRICPARAMSUITE_H
#define OFXHPARAMETRICPARAMSUITE_H

#include <set>
#include <vector>

// parametric params
#include "ofxParametricParam.h"

#include "ofxhParam.h"

namespace OFX {

namespace Host {

namespace ParametricParam {



class ParametricInstance : public Param::Instance, public Param::KeyframeParam
{
    
    bool _curvesDefaultInitialized; //< true if defaultInitializeFromDescriptor() has been called
    
public:

    /// make a parameter, with the given type and name
    explicit ParametricInstance(Param::Descriptor& descriptor, Param::SetInstance* instance = 0);
    
    /// true if defaultInitializeFromDescriptor() has been called
    bool isInitialized() const;
    
    
    OfxStatus defaultInitializeFromDescriptor(int curveIndex,const Param::Descriptor& descriptor);
    
    ///To be called right away after the constructor. It will construct the curves
    ///to their default from the descriptor. This function calls the virtual function
    /// addControlPoint() hence we can't do this ourselves in the constructor.
    OfxStatus defaultInitializeAllCurves(const Param::Descriptor& descriptor);
    
    // copy one parameter to another, with a range (NULL means to copy all animation)
    virtual OfxStatus copyFrom(const Instance &instance, OfxTime offset, const OfxRangeD* range);

    // callback which should set enabled state as appropriate
    virtual void setEnabled();

    // callback which should set secret state as appropriate
    virtual void setSecret();

    /// callback which should update label
    virtual void setLabel();

    /// callback which should set
    virtual void setDisplayRange();



    ///derived from KeyFrameParam, does nothing
    virtual OfxStatus getNumKeys(unsigned int &nKeys) const ;
    virtual OfxStatus getKeyTime(int nth, OfxTime& time) const ;
    virtual OfxStatus getKeyIndex(OfxTime time, int direction, int & index) const ;
    virtual OfxStatus deleteKey(OfxTime time) ;
    virtual OfxStatus deleteAllKeys() ;


    /** @brief Evaluates a parametric parameter
     
     \arg curveIndex            which dimension to evaluate
     \arg time                  the time to evaluate to the parametric param at
     \arg parametricPosition    the position to evaluate the parametric param at
     \arg returnValue           pointer to a double where a value is returned
     
     @returns
     - ::kOfxStatOK            - all was fine
     - ::kOfxStatErrBadIndex   - the curve index was invalid
     */
    virtual OfxStatus getValue(int curveIndex,OfxTime time,double parametricPosition,double *returnValue) = 0;
    
    /** @brief Returns the number of control points in the parametric param.
     
     \arg curveIndex            which dimension to check
     \arg time                  the time to check
     \arg returnValue           pointer to an integer where the value is returned.
     
     @returns
     - ::kOfxStatOK            - all was fine
     - ::kOfxStatErrBadIndex   - the curve index was invalid
     */
    virtual OfxStatus getNControlPoints(int curveIndex,double time,int *returnValue);
    
    
    /** @brief Returns the key/value pair of the nth control point.
     
     \arg curveIndex            which dimension to check
     \arg time                  the time to check
     \arg nthCtl                the nth control point to get the value of
     \arg key                   pointer to a double where the key will be returned
     \arg value                 pointer to a double where the value will be returned
     
     @returns
     - ::kOfxStatOK            - all was fine
     - ::kOfxStatErrUnknown    - if the type is unknown
     */
    virtual OfxStatus getNthControlPoint(int curveIndex,
                                 double time,
                                 int    nthCtl,
                                 double *key,
                                 double *value);
    
    /** @brief Modifies an existing control point on a curve
     
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
     - ::kOfxStatErrUnknown    - if the type is unknown
     
     This modifies an existing control point. Note that by changing key, the order of the
     control point may be modified (as you may move it before or after anther point). So be
     careful when iterating over a curves control points and you change a key.
     */
    virtual OfxStatus setNthControlPoint(int   curveIndex,
                                 double time,
                                 int   nthCtl,
                                 double key,
                                 double value,
                                 bool addAnimationKey
                                 );

    
    /** @brief Adds a control point to the curve.
     
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
     - ::kOfxStatErrUnknown    - if the type is unknown
     
     This will add a new control point to the given dimension of a parametric parameter. If a key exists
     sufficiently close to 'key', then it will be set to the indicated control point.
     */
    virtual OfxStatus addControlPoint(int   curveIndex,
                              double time,
                              double key,
                              double value,
                              bool addAnimationKey);

    
    
    /** @brief Deletes the nth control point from a parametric param.
     
     \arg curveIndex            which dimension to delete
     \arg nthCtl                the control point to delete
     */
    virtual OfxStatus  deleteControlPoint(int   curveIndex,int   nthCtl);

    
    /** @brief Delete all curve control points on the given param.
     
     \arg curveIndex            which dimension to clear
     */
    virtual OfxStatus  deleteAllControlPoints(int   curveIndex);


};



/// fetch the parametric params suite
void *GetSuite(int version);

} //namespace ParametricParam

} //namespace Host

} //namespace OFX

#endif // OFXHPARAMETRICPARAMSUITE_H
