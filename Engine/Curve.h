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

#ifndef NATRON_ENGINE_CURVE_H_
#define NATRON_ENGINE_CURVE_H_

#include <vector>
#include <map>

#include "Global/Macros.h"
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
CLANG_DIAG_OFF(unused-parameter)
// /opt/local/include/boost/serialization/smart_cast.hpp:254:25: warning: unused parameter 'u' [-Wunused-parameter]
#include <boost/archive/xml_iarchive.hpp>
CLANG_DIAG_ON(unused-parameter)
#include <boost/archive/xml_oarchive.hpp>

#include "Global/Macros.h"
#include "Global/GlobalDefines.h"

/**
 * @brief A KeyFrame is a lightweight pair <time,value>. These are the values that are used
 * to interpolate a Curve. The _leftDerivative and _rightDerivative can be
 * used by the interpolation method of the curve.
**/
class Curve;

class KeyFrame  {


public:

    KeyFrame();

    KeyFrame(double time, double initialValue, double leftDerivative = 0., double rightDerivative = 0., Natron::KeyframeType interpolation = Natron::KEYFRAME_SMOOTH);

    KeyFrame(const KeyFrame& other);

    ~KeyFrame();
    
    void operator=(const KeyFrame& o);
    
    bool operator==(const KeyFrame& o) const {
        return o._time == _time &&
        o._value == _value &&
        o._interpolation == _interpolation &&
        o._leftDerivative == _leftDerivative &&
        o._rightDerivative == _rightDerivative;
    }

    bool operator!=(const KeyFrame& o) const {
        return !(*this == o);
    }

    double getValue() const;
    
    double getTime() const ;

    double getLeftDerivative() const;

    double getRightDerivative() const ;

    void setLeftDerivative(double v);

    void setRightDerivative(double v);

    void setValue(double v);

    void setTime(double time);

    void setInterpolation(Natron::KeyframeType interp) ;

    Natron::KeyframeType getInterpolation() const;

private:

    double _time;
    double _value;
    double _leftDerivative;
    double _rightDerivative;
    Natron::KeyframeType _interpolation;
    
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar,const unsigned int version)
    {
        (void)version;
        ar & boost::serialization::make_nvp("Time",_time);
        ar & boost::serialization::make_nvp("Value",_value);
        ar & boost::serialization::make_nvp("InterpolationMethod",_interpolation);
        ar & boost::serialization::make_nvp("LeftDerivative",_leftDerivative);
        ar & boost::serialization::make_nvp("RightDerivative",_rightDerivative);
    }

};

struct KeyFrame_compare_time {
    bool operator() (const KeyFrame& lhs, const KeyFrame& rhs) const {
        return lhs.getTime() < rhs.getTime();
    }
};

typedef std::set<KeyFrame, KeyFrame_compare_time> KeyFrameSet;


/**
  * @brief A CurvePath is a list of chained curves. Each curve is a set of 2 keyFrames and has its
  * own interpolation method (that can differ from other curves).
**/
class KnobI;
struct CurvePrivate;
class RectD;

class Curve
{
    friend class boost::serialization::access;

    enum CurveChangedReason{
        DERIVATIVES_CHANGED = 0,
        KEYFRAME_CHANGED = 1
    };
    
public:


    /**
     * @brief An empty curve, held by no one. This constructor is used by the serialization.
    **/
    Curve();

    /**
     * @brief An empty curve, held by owner. This is the "normal" constructor.
    **/
    Curve(KnobI* owner);
    
    Curve(const Curve& other);

    ~Curve();
    
    void operator=(const Curve& other);
    
    /**
     * @brief Copies all the keyframes held by other, but does not change the pointer to the owner.
    **/
    void clone(const Curve& other);
    
    /**
     * @brief Same as the other version clone except that keyframes will be offset by the given offset
     * and only the keyframes lying in the given range will be copied.
     **/
    void clone(const Curve& other, SequenceTime offset, const RangeD* range);

    bool isAnimated() const WARN_UNUSED_RETURN;
    
    /**
     * @brief Returns true if the Curve represents a string animation, in which case the user cannot
     * modify the Y component of the curve.
     **/
    bool isYComponentMovable() const WARN_UNUSED_RETURN;
    
    /**whether the curve will clamp possible keyframe X values to integers or not.**/
    bool areKeyFramesTimeClampedToIntegers() const WARN_UNUSED_RETURN;

    bool areKeyFramesValuesClampedToIntegers() const WARN_UNUSED_RETURN;
    
    bool areKeyFramesValuesClampedToBooleans() const WARN_UNUSED_RETURN;

    void setXRange(double a,double b);

    std::pair<double,double> getXRange() const WARN_UNUSED_RETURN;

    ///returns true if a keyframe was successfully added, false if it just replaced an already
    ///existing key at this time.
    bool addKeyFrame(KeyFrame key);

    void removeKeyFrameWithTime(double time);

    void removeKeyFrameWithIndex(int index);
    
    bool getNearestKeyFrameWithTime(double time,KeyFrame* k) const WARN_UNUSED_RETURN;

    bool getKeyFrameWithTime(double time, KeyFrame* k) const WARN_UNUSED_RETURN;

    bool getKeyFrameWithIndex(int index, KeyFrame* k) const WARN_UNUSED_RETURN;
    
    int getKeyFramesCount() const WARN_UNUSED_RETURN;

    double getMinimumTimeCovered() const WARN_UNUSED_RETURN;

    double getMaximumTimeCovered() const WARN_UNUSED_RETURN;

    double getValueAt(double t) const WARN_UNUSED_RETURN;

    double getDerivativeAt(double t) const WARN_UNUSED_RETURN;

    double getIntegrateFromTo(double t1, double t2) const WARN_UNUSED_RETURN;
    
    KeyFrameSet getKeyFrames_mt_safe() const WARN_UNUSED_RETURN;

    void clearKeyFrames();

    /**
     * @brief Set the new value and time of the keyframe positioned at index index and returns the new  keyframe.
     * Also the index of the new keyframe is returned in newIndex.
     **/
    KeyFrame setKeyFrameValueAndTime(double time,double value,int index,int* newIndex = NULL);

    /**
     * @brief Set the left derivative  of the keyframe positioned at index index and returns the new  keyframe.
     * Also the index of the new keyframe is returned in newIndex.
     **/
    KeyFrame setKeyFrameLeftDerivative(double value,int index,int* newIndex = NULL);
    
    /**
     * @brief Set the right derivative  of the keyframe positioned at index index and returns the new keyframe.
     * Also the index of the new keyframe is returned in newIndex.
     **/
    KeyFrame setKeyFrameRightDerivative(double value,int index,int* newIndex = NULL);

    /**
     * @brief Set the right and left derivatives  of the keyframe positioned at index index and returns the new  keyframe.
     * Also the index of the new keyframe is returned in newIndex.
     **/
    KeyFrame setKeyFrameDerivatives(double left, double right,int index,int* newIndex = NULL) ;
    
    /**
     * @brief  Set the interpolation method of the keyframe positioned at index index and returns the new  keyframe.
     * Also the index of the new keyframe is returned in newIndex.
     **/
    KeyFrame setKeyFrameInterpolation(Natron::KeyframeType interp,int index,int* newIndex = NULL);
    
    void setCurveInterpolation(Natron::KeyframeType interp);

    std::pair<double,double> getCurveYRange() const WARN_UNUSED_RETURN;
    
    int keyFrameIndex(double time) const WARN_UNUSED_RETURN;
    
    /// set the curve Y range (used for testing, when the Curve his not owned by a Knob)
    void setYRange(double yMin, double yMax);
    
    void getKeyFramesWithinRect(double l,double b,double r,double t,std::vector<KeyFrame>* ret) const;

private:
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);

    ///////The following functions are not thread-safe
    KeyFrameSet::const_iterator find(double time) const WARN_UNUSED_RETURN;

    KeyFrameSet::const_iterator atIndex(int index) const WARN_UNUSED_RETURN;

    KeyFrameSet::const_iterator begin() const WARN_UNUSED_RETURN;

    KeyFrameSet::const_iterator end() const WARN_UNUSED_RETURN;
    
    std::pair<double,double> getCurveYRange_internal() const WARN_UNUSED_RETURN;
    
    void removeKeyFrame(KeyFrameSet::const_iterator it);

    double clampValueToCurveYRange(double v) const WARN_UNUSED_RETURN;

    ///returns an iterator to the new keyframe in the keyframe set and
    ///a boolean indicating whether it removed a keyframe already existing at this time or not
    std::pair<KeyFrameSet::iterator,bool> addKeyFrameNoUpdate(const KeyFrame& cp) WARN_UNUSED_RETURN;

    
    /**
     * @brief Called when a keyframe/derivative is modified, indicating that the curve has changed and we must
     * evaluate any change (i.e: force a new render)
     **/
    void evaluateCurveChanged(CurveChangedReason reason,KeyFrameSet::iterator key);
    
    KeyFrameSet::iterator refreshDerivatives(CurveChangedReason reason, KeyFrameSet::iterator key);

    KeyFrameSet::iterator setKeyFrameValueAndTimeNoUpdate(double value,double time, KeyFrameSet::iterator k) WARN_UNUSED_RETURN;

    bool hasYRange() const;

    bool mustClamp() const;
    
    KeyFrameSet::iterator setKeyframeInterpolation_internal(KeyFrameSet::iterator it,Natron::KeyframeType type);

private:
    boost::scoped_ptr<CurvePrivate> _imp;
};



#endif // NATRON_ENGINE_CURVE_H_
