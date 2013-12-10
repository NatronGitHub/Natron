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
 * to interpolate an AnimationCurve. The _leftTangent and _rightTangent can be
 * used by the interpolation method of the curve.
**/
class Curve;
class Variant;
struct KeyFramePrivate;
class KeyFrame  {


public:

    KeyFrame();

    KeyFrame(double time, const Variant& initialValue, const boost::shared_ptr<Curve> &curve);

    KeyFrame(const KeyFrame& other);

    ~KeyFrame();

    void clone(const KeyFrame& other);

    bool operator==(const KeyFrame& o) const ;

    const Variant& getValue() const;
    
    double getTime() const ;

    const Variant& getLeftTangent() const;

    const Variant& getRightTangent() const ;

    void setTangents(const Variant& left,const Variant& right,bool evaluateNeighboors);

    void setLeftTangent(const Variant& v,bool evaluateNeighboors);

    void setRightTangent(const Variant& v,bool evaluateNeighboors);

    void setValue(const Variant& v);

    void setTime(double time);

    void setTimeAndValue(double time,const Variant& v);

    void setInterpolation(Natron::KeyframeType interp) ;

    void setInterpolationAndEvaluate(Natron::KeyframeType interp) ;

    Natron::KeyframeType getInterpolation() const;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);

private:


    boost::scoped_ptr<KeyFramePrivate> _imp;


};



/**
  * @brief A CurvePath is a list of chained curves. Each curve is a set of 2 keyFrames and has its
  * own interpolation method (that can differ from other curves).
**/
class Knob;
struct CurvePrivate;
class RectD;
class Curve {

public:

    enum CurveChangedReason{
        TANGENT_CHANGED = 0,
        KEYFRAME_CHANGED = 1
    };

    //each segment of curve between a keyframe and the next can have a different interpolation method
    typedef std::list< boost::shared_ptr<KeyFrame> > KeyFrames;

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

    const KeyFrames& getKeyFrames() const;

    void refreshTangents(CurveChangedReason reason, boost::shared_ptr<KeyFrame> k);

    void refreshTangents(CurveChangedReason reason, KeyFrames::iterator key);

    void clearKeyFrames();

    /**
     * @brief Called when a keyframe/tangent is modified, indicating that the curve has changed and we must
     * evaluate any change (i.e: force a new render)
    **/
    void evaluateCurveChanged(CurveChangedReason reason, KeyFrame *k);

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
private:


    boost::scoped_ptr<CurvePrivate> _imp;
};



#endif // NATRON_ENGINE_CURVE_H_
