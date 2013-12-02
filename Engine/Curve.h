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

#ifndef CURVE_ANIMATION_H
#define CURVE_ANIMATION_H


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

    KeyFrame(double time,const Variant& initialValue,Curve* curve);

    KeyFrame(const KeyFrame& other);

    ~KeyFrame();

    void clone(const KeyFrame& other);

    bool operator==(const KeyFrame& o) const ;

    const Variant& getValue() const;
    
    double getTime() const ;

    const Variant& getLeftTangent() const;

    const Variant& getRightTangent() const ;

    void setLeftTangent(const Variant& v);

    void setRightTangent(const Variant& v);

    void setValue(const Variant& v);

    void setTime(double time);

    void setInterpolation(Natron::KeyframeType interp) ;

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
class AnimatingParam;
struct CurvePrivate;
class RectD;
class Curve {

public:


    //each segment of curve between a keyframe and the next can have a different interpolation method
    typedef std::list< KeyFrame* > KeyFrames;

    /**
     * @brief An empty curve, held by no one. This constructor is used by the serialization.
    **/
    Curve();

    /**
     * @brief An empty curve, held by owner. This is the "normal" constructor.
    **/
    Curve(AnimatingParam* owner);

    ~Curve();

    /**
     * @brief Copies all the keyframes held by other, but does not change the pointer to the owner.
    **/
    void clone(const Curve& other);

    bool isAnimated() const;

    void addControlPoint(KeyFrame* cp);

    int getControlPointsCount() const ;

    double getMinimumTimeCovered() const;

    double getMaximumTimeCovered() const;

    const KeyFrame* getStart() const;

    const KeyFrame* getEnd() const;

    Variant getValueAt(double t) const;

    const RectD& getBoundingBox() const ;

    const KeyFrames& getKeyFrames() const;

    void refreshTangents(KeyFrame* k);

    void refreshTangents(KeyFrames::iterator key);

    void clearKeyFrames();

    /**
     * @brief Called when a keyframe/tangent is modified, indicating that the curve has changed and we must
     * evaluate any change (i.e: force a new render)
    **/
    void evaluateCurveChanged();

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
private:


    boost::scoped_ptr<CurvePrivate> _imp;
};



/**
 * @brief A class based on Variant that can store a value across multiple dimension
 * and also have specific "keyframes".
 **/
struct AnimatingParamPrivate;
class AnimatingParam {


public:


    enum ValueChangedReason{USER_EDITED = 0,PLUGIN_EDITED = 1,TIME_CHANGED = 2};

    typedef std::map<int, boost::shared_ptr<Curve> > CurvesMap;

    AnimatingParam();
    
    explicit AnimatingParam(int dimension);
    
    AnimatingParam(const AnimatingParam& other);
    
    void operator=(const AnimatingParam& other);

    virtual ~AnimatingParam();

    const std::map<int,Variant>& getValueForEachDimension() const ;

    int getDimension() const ;

    const Variant& getValue(int dimension = 0) const;

    template <typename T>
    T getValue(int dimension = 0) const {
        return getValue(dimension).value<T>();
    }


    void setValue(const Variant& v,int dimension,AnimatingParam::ValueChangedReason reason);

    template<typename T>
    void setValue(const T &value,int dimension = 0) {
        setValue(Variant(value),dimension,AnimatingParam::PLUGIN_EDITED);
    }

    template<typename T>
    void setValue(T variant[],int count){
        for(int i = 0; i < count; ++i){
            setValue(Variant(variant[i]),i,AnimatingParam::PLUGIN_EDITED);
        }
    }

    /**
     * @brief Set the value for a specific dimension at a specific time. By default dimension
     * is 0. If there's a single dimension, it will set the dimension 0 regardless of the parameter dimension.
     * Otherwise, it will attempt to set a key for only the dimension 'dimensionIndex'.
     **/
    void setValueAtTime(double time,const Variant& v,int dimension);

    template<typename T>
    void setValueAtTime(double time,const T& value,int dimensionIndex = 0){
        assert(dimensionIndex < getDimension());
        setValueAtTime(time,Variant(value),dimensionIndex);
    }

    template<typename T>
    void setValueAtTime(double time,T variant[],int count){
        for(int i = 0; i < count; ++i){
            setValueAtTime(time,Variant(variant[i]),i);
        }
    }

    /**
     * @brief Returns the value  in a specific dimension at a specific time. If
     * there is no key in this dimension it will return the value at the requested dimension
     **/
    Variant getValueAtTime(double time, int dimension) const;

    template<typename T>
    T getValueAtTime(double time,int dimension = 0) const {
        return getValueAtTime(time,dimension).value<T>();
    }


    boost::shared_ptr<Curve> getCurve(int dimension) const;


    virtual void evaluateAnimationChange(){}

    /**
     * @brief Called when the curve has changed.
     * Can be implemented to evaluate any change (i.e: force a new render).
    **/
    virtual void evaluateValueChange(int dimension,AnimatingParam::ValueChangedReason reason) {(void)dimension;(void)reason;}

    /**
     * @brief Used to bracket calls to evaluateValueChange. This indicates than a series of calls will be made, and
     * the derived class can attempt to concatenate evaluations into a single one. For example to avoid multiple calls
     * to render.
    **/
    virtual void beginValueChange(AnimatingParam::ValueChangedReason reason){(void)reason;}

    /**
     * @brief Used to bracket calls to evaluateValueChange. This indicates than a series of calls will be made, and
     * the derived class can attempt to concatenate evaluations into a single one. For example to avoid multiple calls
     * to render.
    **/
    virtual void endValueChange(AnimatingParam::ValueChangedReason reason){(void)reason;}

    template<class Archive>
    void serialize(Archive & ar ,const unsigned int version);

protected:


    void clone(const AnimatingParam& other);

private:

    boost::scoped_ptr<AnimatingParamPrivate> _imp;

};


#endif // CURVE_ANIMATION_H
