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


#include <map>

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>

#include "Global/Enums.h"
#include "Global/Macros.h"

/**
 * @brief A KeyFrame is a lightweight pair <time,value>. These are the values that are used
 * to interpolate a Curve. The _leftDerivative and _rightDerivative can be
 * used by the interpolation method of the curve.
**/
class Curve;

class KeyFrame  {


public:

    KeyFrame();

    KeyFrame(double time, double initialValue);

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
class Knob;
struct CurvePrivate;
class RectD;
class Curve {
    
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
    Curve(Knob* owner);

    ~Curve();

    /**
     * @brief Copies all the keyframes held by other, but does not change the pointer to the owner.
    **/
    void clone(const Curve& other);

    bool isAnimated() const;
    
    /**
     * @brief Returns true if the Curve represents a string animation, in which case the user cannot
     * modify the Y component of the curve.
     **/
    bool isYComponentMovable() const;
    
    /**whether the curve will clamp possible keyframe X values to integers or not.**/
    bool areKeyFramesTimeClampedToIntegers() const;
    
    bool areKeyFramesValuesClampedToIntegers() const;
    
    bool areKeyFramesValuesClampedToBooleans() const;

    void setParametricRange(double a,double b);
    
    void getParametricRange(double* a,double* b);
    
    ///returns true if a keyframe was successfully added, false if it just replaced an already
    ///existing key at this time.
    bool addKeyFrame(KeyFrame key);

    void removeKeyFrame(double time);
    
    void removeKeyFrameWithIndex(int index);

    int keyFramesCount() const ;

    double getMinimumTimeCovered() const;

    double getMaximumTimeCovered() const;

    double getValueAt(double t) const;

    const RectD& getBoundingBox() const ;

    const KeyFrameSet& getKeyFrames() const;

    void clearKeyFrames();

    
    /**
     * @brief  Set the value of the keyframe positioned at index index and returns the new  keyframe.
     * Also the index of the new keyframe is returned in newIndex.
     **/
    const KeyFrame& setKeyFrameValue(double value,int index,int* newIndex = NULL);

    /**
     * @brief Set the new time of the keyframe positioned at index index and returns the new keyframe.
     * Also the index of the new keyframe is returned in newIndex.
     **/
    const KeyFrame& setKeyFrameTime(double time,int index,int* newIndex = NULL);

    /**
     * @brief Set the new value and time of the keyframe positioned at index index and returns the new  keyframe.
     * Also the index of the new keyframe is returned in newIndex.
     **/
    const KeyFrame& setKeyFrameValueAndTime(double time,double value,int index,int* newIndex = NULL);

    /**
     * @brief Set the left derivative  of the keyframe positioned at index index and returns the new  keyframe.
     * Also the index of the new keyframe is returned in newIndex.
     **/
    const KeyFrame& setKeyFrameLeftDerivative(double value,int index,int* newIndex = NULL);
    
    /**
     * @brief Set the right derivative  of the keyframe positioned at index index and returns the new keyframe.
     * Also the index of the new keyframe is returned in newIndex.
     **/
    const KeyFrame& setKeyFrameRightDerivative(double value,int index,int* newIndex = NULL);

    /**
     * @brief Set the right and left derivatives  of the keyframe positioned at index index and returns the new  keyframe.
     * Also the index of the new keyframe is returned in newIndex.
     **/
    const KeyFrame& setKeyFrameDerivatives(double left, double right,int index,int* newIndex = NULL) ;
    
    /**
     * @brief  Set the interpolation method of the keyframe positioned at index index and returns the new  keyframe.
     * Also the index of the new keyframe is returned in newIndex.
     **/
    const KeyFrame& setKeyFrameInterpolation(Natron::KeyframeType interp,int index,int* newIndex = NULL);

    void getCurveYRange(double &min,double &max) const;
    
    void clampValueToCurveYRange(double &v) const;
    
    KeyFrameSet::const_iterator find(double time) const;
    
    KeyFrameSet::const_iterator keyframeAt(int index) const;
    
    int keyFrameIndex(double time) const;
    
    KeyFrameSet::const_iterator begin() const;

    KeyFrameSet::const_iterator end() const;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
private:

    ///returns an iterator to the new keyframe in the keyframe set and
    ///a boolean indicating whether it removed a keyframe already existing at this time or not
    std::pair<KeyFrameSet::iterator,bool> addKeyFrameNoUpdate(const KeyFrame& cp);

    
    /**
     * @brief Called when a keyframe/derivative is modified, indicating that the curve has changed and we must
     * evaluate any change (i.e: force a new render)
     **/
    void evaluateCurveChanged(CurveChangedReason reason,KeyFrameSet::iterator key);
    
    KeyFrameSet::iterator refreshDerivatives(CurveChangedReason reason, KeyFrameSet::iterator key);

    KeyFrameSet::iterator setKeyFrameValueAndTimeNoUpdate(double value,double time, KeyFrameSet::iterator k);

    boost::scoped_ptr<CurvePrivate> _imp;
};



#endif // NATRON_ENGINE_CURVE_H_
