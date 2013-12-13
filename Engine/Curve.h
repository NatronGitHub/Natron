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
/**
 * @brief A KeyFrame is a lightweight pair <time,value>. These are the values that are used
 * to interpolate a Curve. The _leftTangent and _rightTangent can be
 * used by the interpolation method of the curve.
**/
class Curve;
class KeyFrame  {


public:

    KeyFrame();

    KeyFrame(int time, double initialValue);

    KeyFrame(const KeyFrame& other);

    ~KeyFrame();
    
    void operator=(const KeyFrame& o);
    
    bool operator==(const KeyFrame& o) const {
        return o._time == _time &&
        o._value == _value &&
        o._interpolation == _interpolation &&
        o._leftTangent == _leftTangent &&
        o._rightTangent == _rightTangent;
    }

    double getValue() const;
    
    int getTime() const ;

    double getLeftTangent() const;

    double getRightTangent() const ;

    void setLeftTangent(double v);

    void setRightTangent(double v);

    void setValue(double v);

    void setTime(int time);

    void setInterpolation(Natron::KeyframeType interp) ;

    Natron::KeyframeType getInterpolation() const;


private:

    int _time;
    double _value;
    double _leftTangent;
    double _rightTangent;
    Natron::KeyframeType _interpolation;
    
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar,const unsigned int version)
    {
        (void)version;
        ar & boost::serialization::make_nvp("Time",_time);
        ar & boost::serialization::make_nvp("Value",_value);
        ar & boost::serialization::make_nvp("InterpolationMethod",_interpolation);
        ar & boost::serialization::make_nvp("LeftTangent",_leftTangent);
        ar & boost::serialization::make_nvp("RightTangent",_rightTangent);
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
        TANGENT_CHANGED = 0,
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

    KeyFrameSet::iterator addKeyFrame(const KeyFrame& cp);

    void removeKeyFrame(int time);

    int keyFramesCount() const ;

    double getMinimumTimeCovered() const;

    double getMaximumTimeCovered() const;

    double getValueAt(double t) const;

    const RectD& getBoundingBox() const ;

    const KeyFrameSet& getKeyFrames() const;

    void clearKeyFrames();
    
    void setKeyFrameValue(double value,int index);
    
    void setKeyFrameTime(double time,int index);
    
    void setKeyFrameValueAndTime(double time,double value,int index);
    
    void setKeyFrameLeftTangent(double value,int index);
    
    void setKeyFrameRightTangent(double value,int index);
    
    void setKeyFrameTangents(double left, double right,int index);
    
    void setKeyFrameInterpolation(Natron::KeyframeType interp,int index);

    KeyFrameSet::iterator find(int time);

    KeyFrameSet::iterator end();

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
private:

    /**
     * @brief Called when a keyframe/tangent is modified, indicating that the curve has changed and we must
     * evaluate any change (i.e: force a new render)
     **/
    void evaluateCurveChanged(CurveChangedReason reason,KeyFrameSet::iterator key);
    
    KeyFrameSet::iterator refreshTangents(CurveChangedReason reason, KeyFrameSet::iterator key);

    KeyFrameSet::iterator setKeyFrameValueAndTimeNoUpdate(double value,double time, KeyFrameSet::iterator k);

    boost::scoped_ptr<CurvePrivate> _imp;
};



#endif // NATRON_ENGINE_CURVE_H_
