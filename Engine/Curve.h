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

#include "Engine/Variant.h"

#include "Global/Enums.h"
/**
 * @brief A KeyFrame is a pair <time,value>. These are the value that are used
 * to interpolate a Curve. The _leftTangent and _rightTangent can be
 * used by the interpolation method of the curve.
**/
class Curve;
class Variant;
struct KeyFramePrivate;
class KeyFrame  {


public:

    KeyFrame();

    KeyFrame(double time, const Variant& initialValue);

    KeyFrame(const KeyFrame& other);

    ~KeyFrame();
    
    void operator=(const KeyFrame& o);

    bool operator==(const KeyFrame& o) const ;

    const Variant& getValue() const;
    
    double getTime() const ;

    double getLeftTangent() const;

    double getRightTangent() const ;

    void setLeftTangent(double v);

    void setRightTangent(double v);

    void setValue(const Variant& v);

    void setTime(double time);

    void setInterpolation(Natron::KeyframeType interp) ;

    Natron::KeyframeType getInterpolation() const;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);

private:


    boost::scoped_ptr<KeyFramePrivate> _imp;


};

struct KeyFrame_compare_time {
    bool operator() (const boost::shared_ptr<KeyFrame>& lhs, const boost::shared_ptr<KeyFrame>& rhs) const {
        return lhs->getTime() < rhs->getTime();
    }
};

typedef std::set<boost::shared_ptr<KeyFrame>, KeyFrame_compare_time> KeyFrameSet;


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

    void addKeyFrame(boost::shared_ptr<KeyFrame> cp);

    void removeKeyFrame(boost::shared_ptr<KeyFrame> cp);

    //removes a keyframe at this time, if any
    void removeKeyFrame(double time);

    int keyFramesCount() const ;

    double getMinimumTimeCovered() const;

    double getMaximumTimeCovered() const;

    Variant getValueAt(double t) const;

    const RectD& getBoundingBox() const ;

    const KeyFrameSet& getKeyFrames() const;

    void clearKeyFrames();
    
    void setKeyFrameValue(const Variant& value,boost::shared_ptr<KeyFrame> k);
    
    void setKeyFrameTime(double time,boost::shared_ptr<KeyFrame> k);
    
    void setKeyFrameValueAndTime(double time,const Variant& value,boost::shared_ptr<KeyFrame> k);
    
    void setKeyFrameLeftTangent(double value,boost::shared_ptr<KeyFrame> k);
    
    void setKeyFrameRightTangent(double value,boost::shared_ptr<KeyFrame> k);
    
    void setKeyFrameTangents(double left, double right, boost::shared_ptr<KeyFrame> k);
    
    void setKeyFrameInterpolation(Natron::KeyframeType interp,boost::shared_ptr<KeyFrame> k);


    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
private:

    /**
     * @brief Called when a keyframe/tangent is modified, indicating that the curve has changed and we must
     * evaluate any change (i.e: force a new render)
     **/
    void evaluateCurveChanged(CurveChangedReason reason, boost::shared_ptr<KeyFrame> k);
    
    void refreshTangents(CurveChangedReason reason, boost::shared_ptr<KeyFrame> k);
    
    void refreshTangents(CurveChangedReason reason, KeyFrameSet::iterator key);

    boost::scoped_ptr<CurvePrivate> _imp;
};



#endif // NATRON_ENGINE_CURVE_H_
