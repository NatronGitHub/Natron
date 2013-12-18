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

#ifndef OFXHPARAMETRICPARAMSUITE_H
#define OFXHPARAMETRICPARAMSUITE_H

#include <set>
#include <vector>

#include "ofxParametricParam.h"

#include "ofxhParam.h"

namespace OFX {

namespace Host {

namespace ParametricParam {

struct ControlPoint{
    OfxTime time;
    double key;
    double value;
    bool createAnimationKey;///< if true the host should make a keyframe for this control point
};

struct ControlPoint_key_compare {
    bool operator() (const ControlPoint& lhs, const ControlPoint& rhs) const {
        return lhs.key < rhs.key;
    }
};

typedef std::set<ControlPoint,ControlPoint_key_compare> ControlPointSet;


///a base class for handling control points for different curves.
///It avoids similar code in ParametricDescriptor and ParametricInstance
class CurvesHolder
{
public:

    typedef std::vector<ControlPointSet> Curves;

    CurvesHolder();
    
    void setDimension(int dimension);

    const Curves& getCurves() const;

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
    OfxStatus getNControlPoints(int curveIndex,double time,int *returnValue);


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
    OfxStatus getNthControlPoint(int curveIndex,
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
    OfxStatus setNthControlPoint(int   curveIndex,
                                 double time,
                                 int   nthCtl,
                                 double key,
                                 double value,
                                 bool addAnimationKey
                                 );

    /**
     * @brief Called when setNthControlPoint successfully set a control point.
     * It lets a chance to the host to update any graphical user interface it has
     * linked to this parameter before returning to the plugin.
     * WARNING: Even if the host doesn't find a control point on the graphical user interface
     * with a matching 'key' it must modify the closest one to this control point's key.
     * The function setNthControlPoint has previously made sure the control points were close enough.
     * @param cp The control point just added.
     */
    virtual void onNthControlPointSet(int curveIndex,const ControlPoint& cp);

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
    OfxStatus addControlPoint(int   curveIndex,
                              double time,
                              double key,
                              double value,
                              bool addAnimationKey);

    /**
     * @brief Called when addControlPoint successfully adds a control point.
     * The host should not attempt to merge it with any other keyframe, as otherwise
     * the function onNthControlPointSet would have been called.
     * It lets a chance to the host to update any graphical user interface it has
     * linked to this parameter before returning to the plugin.
     * @param cp The control point just added.
     */
    virtual void onControlPointAdded(int curveIndex,const ControlPoint& cp);


    /** @brief Deletes the nth control point from a parametric param.

                 \arg curveIndex            which dimension to delete
                 \arg nthCtl                the control point to delete
                 */
    OfxStatus  deleteControlPoint(int   curveIndex,int   nthCtl);

    /**
     * @brief Called when deleteControlPoint successfully deletes a control point.
     * This lets the host a change to update any graphical user interface it has
     * linked to this parameter before returning to the plugin.
     */
    virtual void onControlPointDeleted(int curveIndex,const ControlPoint& cp);

    /** @brief Delete all curve control points on the given param.

     \arg curveIndex            which dimension to clear
     */
    OfxStatus  deleteAllControlPoints(int   curveIndex);

    /**
     * @brief Called when deleteAllControlPoints is called.
     * This lets the host a change to update any graphical user interface it has
     * linked to this parameter before returning to the plugin.
     */
    virtual void onCurveCleared(int curveIndex);


protected:

   Curves _curves;

};

///a special descriptor for parametric parameters:
///It remembers the default shapes of the curves after the
/// plugin set them in describeInContext
class ParametricDescriptor : public Param::Descriptor, public CurvesHolder ,  private Property::NotifyHook
{
public:

    ///make a parametric parameter descriptor with given type and name
    ParametricDescriptor(const std::string &type, const std::string &name);

    ///does nothing as the describe in context function is not supposed to use this.
    virtual OfxStatus getValue(int curveIndex,OfxTime time,double parametricPosition,double *returnValue);
    
    /// overridden from Property::NotifyHook
    virtual void notify(const std::string &name, bool single, int num) OFX_EXCEPTION_SPEC;

};

class ParametricInstance : public Param::Instance, public CurvesHolder, public Param::KeyframeParam
{

public:

    /// make a parameter, with the given type and name
    explicit ParametricInstance(Param::Descriptor& descriptor, Param::SetInstance* instance = 0);

    // copy one parameter to another
    virtual OfxStatus copy(const Instance &instance, OfxTime offset);

    // copy one parameter to another, with a range
    virtual OfxStatus copy(const Instance &instance, OfxTime offset, OfxRangeD range);

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


    ///derived from CurveHolder
    virtual OfxStatus getValue(int curveIndex,OfxTime time,double parametricPosition,double *returnValue) = 0;

    virtual void onNthControlPointSet(int curveIndex,const ControlPoint& cp);

    virtual void onControlPointAdded(int curveIndex,const ControlPoint& cp);

    virtual void onControlPointDeleted(int curveIndex,const ControlPoint& cp);

    virtual void onCurveCleared(int curveIndex);

};



/// fetch the parametric params suite
void *GetSuite(int version);

} //namespace ParametricParam

} //namespace Host

} //namespace OFX

#endif // OFXHPARAMETRICPARAMSUITE_H
