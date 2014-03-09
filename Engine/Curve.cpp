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

#include "Curve.h"

#include <algorithm>
#include <boost/math/special_functions/fpclassify.hpp>

#include "Engine/AppManager.h"

#include "Engine/CurvePrivate.h"
#include "Engine/Interpolation.h"
#include "Engine/KnobTypes.h"
#include "Engine/KnobFile.h"

namespace {
    struct KeyFrameCloner {
        KeyFrame operator()(const KeyFrame& kf) const { return KeyFrame(kf); }
    };

    class KeyFrameTimePredicate
    {
    public:
        KeyFrameTimePredicate(double t) : _t(t) {};

        bool operator()(const KeyFrame& f) { return (f.getTime() == _t); }
    private:
        double _t;
    };
}

/************************************KEYFRAME************************************/

KeyFrame::KeyFrame()
    : _time(0.)
    , _value(0.)
    , _leftDerivative(0.)
    , _rightDerivative(0.)
    , _interpolation(Natron::KEYFRAME_SMOOTH)
{}


KeyFrame::KeyFrame(double time, double initialValue)
    : _time(time)
    , _value(initialValue)
    , _leftDerivative(0.)
    , _rightDerivative(0.)
    , _interpolation(Natron::KEYFRAME_SMOOTH)
{
}

KeyFrame::KeyFrame(const KeyFrame& other)
    : _time(other._time)
    , _value(other._value)
    , _leftDerivative(other._leftDerivative)
    , _rightDerivative(other._rightDerivative)
    , _interpolation(other._interpolation)
{
}

void KeyFrame::operator=(const KeyFrame& o)
{
    _time = o._time;
    _value = o._value;
    _leftDerivative = o._leftDerivative;
    _rightDerivative = o._rightDerivative;
    _interpolation = o._interpolation;
}

KeyFrame::~KeyFrame(){}


void KeyFrame::setLeftDerivative(double v){
    assert(!boost::math::isnan(v));
    _leftDerivative = v;
}


void KeyFrame::setRightDerivative(double v){
    assert(!boost::math::isnan(v));
    _rightDerivative = v;
}


void KeyFrame::setValue(double v){
    assert(!boost::math::isnan(v));
    _value = v;
}

void KeyFrame::setTime(double time){
     _time = time;
}


void KeyFrame::setInterpolation(Natron::KeyframeType interp) { _interpolation = interp; }

Natron::KeyframeType KeyFrame::getInterpolation() const { return _interpolation; }


double KeyFrame::getValue() const { return _value; }

double KeyFrame::getTime() const { return _time; }

double KeyFrame::getLeftDerivative() const { return _leftDerivative; }

double KeyFrame::getRightDerivative() const { return _rightDerivative; }

/************************************CURVEPATH************************************/

Curve::Curve()
    : _imp(new CurvePrivate)
{

}

Curve::Curve(Knob *owner)
    : _imp(new CurvePrivate)
{
    
    _imp->owner = owner;
    
}

Curve::~Curve(){ clearKeyFrames(); }

void Curve::clearKeyFrames(){
    
    QMutexLocker l(&_imp->_lock);
    _imp->keyFrames.clear();
}


bool Curve::areKeyFramesTimeClampedToIntegers() const{
    return _imp->owner->typeName() != Parametric_Knob::typeNameStatic();
}

void Curve::clone(const Curve& other){
    clearKeyFrames();
    const KeyFrameSet& otherKeys = other.getKeyFrames();
    std::transform(otherKeys.begin(), otherKeys.end(), std::inserter(_imp->keyFrames, _imp->keyFrames.begin()), KeyFrameCloner());
}


double Curve::getMinimumTimeCovered() const {
    QMutexLocker l(&_imp->_lock);
    assert(!_imp->keyFrames.empty());
    return (*_imp->keyFrames.begin()).getTime();
}

double Curve::getMaximumTimeCovered() const {
    QMutexLocker l(&_imp->_lock);
    assert(!_imp->keyFrames.empty());
    return (*_imp->keyFrames.rbegin()).getTime();
}

bool Curve::addKeyFrame(KeyFrame key)
{
    
    QMutexLocker l(&_imp->_lock);
    
    CurvePrivate::CurveType type = _imp->getCurveType();
    
    if (type == CurvePrivate::BOOL_CURVE || type == CurvePrivate::STRING_CURVE) {
        key.setInterpolation(Natron::KEYFRAME_CONSTANT);
    }
    
    std::pair<KeyFrameSet::iterator,bool> it = addKeyFrameNoUpdate(key);
    
    evaluateCurveChanged(KEYFRAME_CHANGED,it.first);
    return it.second;
}


std::pair<KeyFrameSet::iterator,bool> Curve::addKeyFrameNoUpdate(const KeyFrame& cp)
{
    
    if(areKeyFramesTimeClampedToIntegers()){
        std::pair<KeyFrameSet::iterator,bool> newKey = _imp->keyFrames.insert(cp);
        // keyframe at this time exists, erase and insert again
        bool addedKey = true;
        if(!newKey.second){
            _imp->keyFrames.erase(newKey.first);
            newKey = _imp->keyFrames.insert(cp);
            assert(newKey.second);
            addedKey = false;
        }
        return std::make_pair(newKey.first,addedKey);
    }else{
        bool addedKey = true;
        double paramEps = 1e-4 * std::abs(_imp->curveMax - _imp->curveMin);
        for (KeyFrameSet::iterator it = _imp->keyFrames.begin(); it!= _imp->keyFrames.end(); ++it) {
            if (std::abs(it->getTime() - cp.getTime()) < paramEps) {
                _imp->keyFrames.erase(it);
                addedKey = false;
                break;
            }
        }
        std::pair<KeyFrameSet::iterator,bool> newKey = _imp->keyFrames.insert(cp);
        newKey.second = addedKey;
        return newKey;
    }
}

void Curve::removeKeyFrameWithIndex(int index) {
    QMutexLocker l(&_imp->_lock);
    int i = 0;
    for (KeyFrameSet::iterator it = _imp->keyFrames.begin(); it!= _imp->keyFrames.end(); ++it) {
        if(i == index){
            KeyFrame prevKey;
            bool mustRefreshPrev = false;
            KeyFrame nextKey;
            bool mustRefreshNext = false;
            
            if(it != _imp->keyFrames.begin()){
                KeyFrameSet::iterator prev = it;
                --prev;
                prevKey = *prev;
                mustRefreshPrev = prevKey.getInterpolation() != Natron::KEYFRAME_BROKEN &&
                prevKey.getInterpolation() != Natron::KEYFRAME_FREE &&
                prevKey.getInterpolation() != Natron::KEYFRAME_NONE;
            }
            KeyFrameSet::iterator next = it;
            ++next;
            if(next != _imp->keyFrames.end()){
                nextKey = *next;
                mustRefreshNext = nextKey.getInterpolation() != Natron::KEYFRAME_BROKEN &&
                nextKey.getInterpolation() != Natron::KEYFRAME_FREE &&
                nextKey.getInterpolation() != Natron::KEYFRAME_NONE;
            }
            
            _imp->keyFrames.erase(it);
            
            
            if(mustRefreshPrev){
                refreshDerivatives(DERIVATIVES_CHANGED,find(prevKey.getTime()));
            }
            if(mustRefreshNext){
                refreshDerivatives(DERIVATIVES_CHANGED,find(nextKey.getTime()));
            }
            break;
        }
        ++i;
    }
}

void Curve::removeKeyFrame(double time) {
    QMutexLocker l(&_imp->_lock);
    KeyFrameSet::iterator it = find(time);
    
    if(it == _imp->keyFrames.end()){
        return;
    }
    
    KeyFrame prevKey;
    bool mustRefreshPrev = false;
    KeyFrame nextKey;
    bool mustRefreshNext = false;
    
    if(it != _imp->keyFrames.begin()){
        KeyFrameSet::iterator prev = it;
        --prev;
        prevKey = *prev;
        mustRefreshPrev = prevKey.getInterpolation() != Natron::KEYFRAME_BROKEN &&
                 prevKey.getInterpolation() != Natron::KEYFRAME_FREE &&
                 prevKey.getInterpolation() != Natron::KEYFRAME_NONE;
    }
    KeyFrameSet::iterator next = it;
    ++next;
    if(next != _imp->keyFrames.end()){
        nextKey = *next;
        mustRefreshNext = nextKey.getInterpolation() != Natron::KEYFRAME_BROKEN &&
                nextKey.getInterpolation() != Natron::KEYFRAME_FREE &&
                nextKey.getInterpolation() != Natron::KEYFRAME_NONE;
    }
    
    _imp->keyFrames.erase(it);

    
    if(mustRefreshPrev){
        refreshDerivatives(DERIVATIVES_CHANGED,find(prevKey.getTime()));
    }
    if(mustRefreshNext){
        refreshDerivatives(DERIVATIVES_CHANGED,find(nextKey.getTime()));
    }
    
}


double Curve::getValueAt(double t) const
{
    QMutexLocker l(&_imp->_lock);
    
    if (_imp->keyFrames.empty()) {
        throw std::runtime_error("Curve has no control points!");
    }

    // even when there is only one keyframe, there may be tangents!
    //if (_imp->keyFrames.size() == 1) {
    //    //if there's only 1 keyframe, don't bother interpolating
    //    return (*_imp->keyFrames.begin()).getValue();
    //}
    double tcur,tnext;
    double vcurDerivRight ,vnextDerivLeft ,vcur ,vnext ;
    Natron::KeyframeType interp ,interpNext;

    KeyFrame k(t,0.);
    // find the first keyframe with time greater than t
    KeyFrameSet::const_iterator itup;
    itup = _imp->keyFrames.upper_bound(k);
    assert(itup == _imp->keyFrames.end() || t < itup->getTime());
    if (itup == _imp->keyFrames.begin()) {
        //if all keys have a greater time
        tnext = itup->getTime();
        vnext = itup->getValue();
        vnextDerivLeft = itup->getLeftDerivative();
        interpNext = itup->getInterpolation();
        tcur = tnext - 1.;
        vcur = vnext;
        vcurDerivRight = 0.;
        interp = Natron::KEYFRAME_NONE;

    } else if (itup == _imp->keyFrames.end()) {
        //if we found no key that has a greater time
        KeyFrameSet::const_reverse_iterator itlast = _imp->keyFrames.rbegin();
        tcur = itlast->getTime();
        vcur = itlast->getValue();
        vcurDerivRight = itlast->getRightDerivative();
        interp = itlast->getInterpolation();
        tnext = tcur + 1.;
        vnext = vcur;
        vnextDerivLeft = 0.;
        interpNext = Natron::KEYFRAME_NONE;
    } else {
        // between two keyframes
        // get the last keyframe with time <= t
        KeyFrameSet::const_iterator itcur = itup;
        --itcur;
        assert(itcur->getTime() <= t);
        tcur = itcur->getTime();
        vcur = itcur->getValue();
        vcurDerivRight = itcur->getRightDerivative();
        interp = itcur->getInterpolation();
        tnext = itup->getTime();
        vnext = itup->getValue();
        vnextDerivLeft = itup->getLeftDerivative();
        interpNext = itup->getInterpolation();
    }

    double v = Natron::interpolate(tcur,vcur,
                                   vcurDerivRight,
                                   vnextDerivLeft,
                                   tnext,vnext,
                                   t,
                                   interp,
                                   interpNext);

    if (_imp->owner) {
        v = clampValueToCurveYRange(v);
    }

    CurvePrivate::CurveType type = _imp->getCurveType();
    switch (type) {
        case CurvePrivate::STRING_CURVE:
        case CurvePrivate::INT_CURVE:
            return std::floor(v + 0.5);
        case CurvePrivate::DOUBLE_CURVE:
            return v;
        case CurvePrivate::BOOL_CURVE:
            return v >= 0.5 ? 1. : 0.;
        default:
            return v;
    }
}

double Curve::getDerivativeAt(double t) const
{
    QMutexLocker l(&_imp->_lock);

    if (_imp->keyFrames.empty()) {
        throw std::runtime_error("Curve has no control points!");
    }
    assert(_imp->getCurveType() == CurvePrivate::DOUBLE_CURVE); // only real-valued curves can be derived
    
    // even when there is only one keyframe, there may be tangents!
    //if (_imp->keyFrames.size() == 1) {
    //    //if there's only 1 keyframe, don't bother interpolating
    //    return (*_imp->keyFrames.begin()).getValue();
    //}
    double tcur,tnext;
    double vcurDerivRight ,vnextDerivLeft ,vcur ,vnext ;
    Natron::KeyframeType interp ,interpNext;

    KeyFrame k(t,0.);
    // find the first keyframe with time greater than t
    KeyFrameSet::const_iterator itup;
    itup = _imp->keyFrames.upper_bound(k);
    assert(itup == _imp->keyFrames.end() || t < itup->getTime());
    if (itup == _imp->keyFrames.begin()) {
        //if all keys have a greater time
        tnext = itup->getTime();
        vnext = itup->getValue();
        vnextDerivLeft = itup->getLeftDerivative();
        interpNext = itup->getInterpolation();
        tcur = tnext - 1.;
        vcur = vnext;
        vcurDerivRight = 0.;
        interp = Natron::KEYFRAME_NONE;

    } else if (itup == _imp->keyFrames.end()) {
        //if we found no key that has a greater time
        // get the last keyframe
        KeyFrameSet::const_reverse_iterator itlast = _imp->keyFrames.rbegin();
        tcur = itlast->getTime();
        vcur = itlast->getValue();
        vcurDerivRight = itlast->getRightDerivative();
        interp = itlast->getInterpolation();
        tnext = tcur + 1.;
        vnext = vcur;
        vnextDerivLeft = 0.;
        interpNext = Natron::KEYFRAME_NONE;
    } else {
        // between two keyframes
        // get the last keyframe with time <= t
        KeyFrameSet::const_iterator itcur = itup;
        --itcur;
        assert(itcur->getTime() <= t);
        tcur = itcur->getTime();
        vcur = itcur->getValue();
        vcurDerivRight = itcur->getRightDerivative();
        interp = itcur->getInterpolation();
        tnext = itup->getTime();
        vnext = itup->getValue();
        vnextDerivLeft = itup->getLeftDerivative();
        interpNext = itup->getInterpolation();
    }

    double d;

    if (_imp->owner) {
        std::pair<double,double> minmax = getCurveYRange();
        d = Natron::derive_clamp(tcur,vcur,
                                 vcurDerivRight,
                                 vnextDerivLeft,
                                 tnext,vnext,
                                 t,
                                 minmax.first, minmax.second,
                                 interp,
                                 interpNext);
    } else {
        d = Natron::derive(tcur,vcur,
                           vcurDerivRight,
                           vnextDerivLeft,
                           tnext,vnext,
                           t,
                           interp,
                           interpNext);
    }

    return d;
}

// integrateFromTo is a bit more complicated to deal with clamping: the intersections of the curve with y=min and y=max have to be computed and used in the integral
#pragma message WARN("TODO: add Curve::integrateFromTo(t1,t2), see Natron::integrate")

std::pair<double,double>  Curve::getCurveYRange() const
{
    
    assert(_imp->owner);
    if (!_imp->owner)
        throw std::logic_error("Curve::getCurveYRange() called for a curve without owner");
    if(_imp->owner->typeName() == Double_Knob::typeNameStatic()) {
        Double_Knob* dbKnob = dynamic_cast<Double_Knob*>(_imp->owner);
        assert(dbKnob);
        return dbKnob->getMinMaxForCurve(this);

    }else if(_imp->owner->typeName() == Int_Knob::typeNameStatic()) {
        Int_Knob* intK = dynamic_cast<Int_Knob*>(_imp->owner);
        assert(intK);
        return intK->getMinMaxForCurve(this);
    } else {
        return std::make_pair(INT_MIN, INT_MAX);
    }
}

double Curve::clampValueToCurveYRange(double v) const
{
    assert(_imp->owner);
    if (!_imp->owner)
        throw std::logic_error("Curve::clampValueToCurveYRange() called for a curve without owner");
    ////clamp to min/max if the owner of the curve is a Double or Int knob.
    std::pair<double,double> minmax = getCurveYRange();

    if (v > minmax.second) {
        return minmax.second;
    } else if (v < minmax.first) {
        return minmax.first;
    }
    return v;
}

bool Curve::isAnimated() const
{
    QMutexLocker l(&_imp->_lock);
    // even when there is only one keyframe, there may be tangents!
    return _imp->keyFrames.size() > 0;
}

void Curve::setParametricRange(double a,double b) {
    QMutexLocker l(&_imp->_lock);
    _imp->curveMin = a;
    _imp->curveMax = b;
}

std::pair<double,double> Curve::getParametricRange() const
{
    QMutexLocker l(&_imp->_lock);
    return std::make_pair(_imp->curveMin, _imp->curveMax);
}

int Curve::getKeyFramesCount() const
{
    QMutexLocker l(&_imp->_lock);
    return (int)_imp->keyFrames.size();
}

const KeyFrameSet& Curve::getKeyFrames() const
{
    QMutexLocker l(&_imp->_lock);
    return _imp->keyFrames;
}

KeyFrameSet::iterator Curve::setKeyFrameValueAndTimeNoUpdate(double value,double time, KeyFrameSet::iterator k) {
    KeyFrame newKey(*k);
    
    // nothing special has to be done, since the derivatives are with respect to t
    newKey.setTime(time);
    newKey.setValue(value);
    _imp->keyFrames.erase(k);
    return addKeyFrameNoUpdate(newKey).first;
}


const KeyFrame& Curve::setKeyFrameValue(double value,int index,int* newIndex) {
    
    QMutexLocker l(&_imp->_lock);
    KeyFrameSet::iterator it = keyframeAt(index);
    assert(it != _imp->keyFrames.end());
    
    KeyFrame newKey(*it);
    newKey.setValue(value);
    it = addKeyFrameNoUpdate(newKey).first;
    
    evaluateCurveChanged(KEYFRAME_CHANGED,it);
    
    if (newIndex) {
        *newIndex = std::distance(_imp->keyFrames.begin(),it);
    }
    return *it;

}



const KeyFrame& Curve::setKeyFrameTime(double time,int index,int* newIndex) {
    
    QMutexLocker l(&_imp->_lock);
    KeyFrameSet::iterator it = keyframeAt(index);
    assert(it != _imp->keyFrames.end());
    
    if(time != it->getTime()) {
        it = setKeyFrameValueAndTimeNoUpdate(it->getValue(),time, it);
        evaluateCurveChanged(KEYFRAME_CHANGED,it);
    }
    if (newIndex) {
        *newIndex = std::distance(_imp->keyFrames.begin(),it);
    }

    return *it;
}



const KeyFrame& Curve::setKeyFrameValueAndTime(double time,double value,int index,int* newIndex) {
    
    QMutexLocker l(&_imp->_lock);
    KeyFrameSet::iterator it = keyframeAt(index);
    assert(it != _imp->keyFrames.end());
    
    bool setTime = (time != it->getTime());
    bool setValue = (value != it->getValue());
    
    if(setTime || setValue){
        it = setKeyFrameValueAndTimeNoUpdate(value,time, it);
        evaluateCurveChanged(KEYFRAME_CHANGED,it);
    }
    if (newIndex) {
        *newIndex = std::distance(_imp->keyFrames.begin(),it);
    }

    return *it;

}


const KeyFrame& Curve::setKeyFrameLeftDerivative(double value,int index,int* newIndex) {
    
    QMutexLocker l(&_imp->_lock);
    KeyFrameSet::iterator it = keyframeAt(index);
    assert(it != _imp->keyFrames.end());

    if(value != it->getLeftDerivative()) {
        KeyFrame newKey(*it);
        newKey.setLeftDerivative(value);
        it = addKeyFrameNoUpdate(newKey).first;
        evaluateCurveChanged(DERIVATIVES_CHANGED,it);
    }
    if (newIndex) {
        *newIndex = std::distance(_imp->keyFrames.begin(),it);
    }

    return *it;
}


const KeyFrame& Curve::setKeyFrameRightDerivative(double value,int index,int* newIndex) {
    
    QMutexLocker l(&_imp->_lock);
    KeyFrameSet::iterator it = keyframeAt(index);
    assert(it != _imp->keyFrames.end());
    
    if(value != it->getRightDerivative()) {
        KeyFrame newKey(*it);
        newKey.setRightDerivative(value);
        it = addKeyFrameNoUpdate(newKey).first;
        evaluateCurveChanged(DERIVATIVES_CHANGED,it);
    }
    if (newIndex) {
        *newIndex = std::distance(_imp->keyFrames.begin(),it);
    }

    return *it;
}


const KeyFrame& Curve::setKeyFrameDerivatives(double left, double right,int index,int* newIndex) {
    
    QMutexLocker l(&_imp->_lock);
    KeyFrameSet::iterator it = keyframeAt(index);
    assert(it != _imp->keyFrames.end());
    
    if(left != it->getLeftDerivative() || right != it->getRightDerivative()) {
        KeyFrame newKey(*it);
        newKey.setLeftDerivative(left);
        newKey.setRightDerivative(right);
        it = addKeyFrameNoUpdate(newKey).first;
        evaluateCurveChanged(DERIVATIVES_CHANGED,it);
    }
    if (newIndex) {
        *newIndex = std::distance(_imp->keyFrames.begin(),it);
    }

    return *it;
}

const KeyFrame& Curve::setKeyFrameInterpolation(Natron::KeyframeType interp,int index,int* newIndex){
    
    QMutexLocker l(&_imp->_lock);
    KeyFrameSet::iterator it = keyframeAt(index);
    assert(it != _imp->keyFrames.end());
    
    CurvePrivate::CurveType type = _imp->getCurveType();
    ///if the curve is a string_curve or bool_curve the interpolation is bound to be constant.
    if ((type == CurvePrivate::STRING_CURVE || type == CurvePrivate::BOOL_CURVE)
        && interp != Natron::KEYFRAME_CONSTANT) {
        return *it;
    }

    
    if(interp != it->getInterpolation()) {
        KeyFrame newKey(*it);
        newKey.setInterpolation(interp);
        it = addKeyFrameNoUpdate(newKey).first;
        evaluateCurveChanged(KEYFRAME_CHANGED,it);
    }
    if (newIndex) {
        *newIndex = std::distance(_imp->keyFrames.begin(),it);
    }

    return *it;
}

KeyFrameSet::iterator Curve::refreshDerivatives(Curve::CurveChangedReason reason, KeyFrameSet::iterator key){
    double tcur = key->getTime();
    double vcur = key->getValue();
    
    double tprev, vprev, tnext, vnext, vprevDerivRight, vnextDerivLeft;
    Natron::KeyframeType prevType, nextType;
    if (key == _imp->keyFrames.begin()) {
        tprev = tcur;
        vprev = vcur;
        vprevDerivRight = 0.;
        prevType = Natron::KEYFRAME_NONE;
    } else {
        KeyFrameSet::const_iterator prev = key;
        --prev;
        tprev = prev->getTime();
        vprev = prev->getValue();
        vprevDerivRight = prev->getRightDerivative();
        prevType = prev->getInterpolation();
        
        //if prev is the first keyframe, and not edited by the user then interpolate linearly
        if(prev == _imp->keyFrames.begin() && prevType != Natron::KEYFRAME_FREE &&
           prevType != Natron::KEYFRAME_BROKEN){
            prevType = Natron::KEYFRAME_LINEAR;
        }
    }
    
    KeyFrameSet::const_iterator next = key;
    ++next;
    if (next == _imp->keyFrames.end()) {
        tnext = tcur;
        vnext = vcur;
        vnextDerivLeft = 0.;
        nextType = Natron::KEYFRAME_NONE;
    } else {
        tnext = next->getTime();
        vnext = next->getValue();
        vnextDerivLeft = next->getLeftDerivative();
        nextType = next->getInterpolation();
        
        KeyFrameSet::const_iterator nextnext = next;
        ++nextnext;
        //if next is thelast keyframe, and not edited by the user then interpolate linearly
        if(nextnext == _imp->keyFrames.end() && nextType != Natron::KEYFRAME_FREE &&
           nextType != Natron::KEYFRAME_BROKEN){
            nextType = Natron::KEYFRAME_LINEAR;
        }
    }
    
    double vcurDerivLeft,vcurDerivRight;

    assert(key->getInterpolation() != Natron::KEYFRAME_NONE &&
            key->getInterpolation() != Natron::KEYFRAME_BROKEN &&
            key->getInterpolation() != Natron::KEYFRAME_FREE);
    Natron::autoComputeDerivatives(prevType,
                                   key->getInterpolation(),
                                   nextType,
                                   tprev, vprev,
                                   tcur, vcur,
                                   tnext, vnext,
                                   vprevDerivRight,
                                   vnextDerivLeft,
                                   &vcurDerivLeft, &vcurDerivRight);


    KeyFrame newKey(*key);
    newKey.setLeftDerivative(vcurDerivLeft);
    newKey.setRightDerivative(vcurDerivRight);

    std::pair<KeyFrameSet::iterator,bool> newKeyIt = _imp->keyFrames.insert(newKey);

    // keyframe at this time exists, erase and insert again
    if(!newKeyIt.second){
        _imp->keyFrames.erase(newKeyIt.first);
        newKeyIt = _imp->keyFrames.insert(newKey);
        assert(newKeyIt.second);
    }
    key = newKeyIt.first;
    
    if(reason != DERIVATIVES_CHANGED){
        evaluateCurveChanged(DERIVATIVES_CHANGED,key);
    }
    return key;
}



void Curve::evaluateCurveChanged(CurveChangedReason reason, KeyFrameSet::iterator key){

    assert(key!=_imp->keyFrames.end());

    if(key->getInterpolation()!= Natron::KEYFRAME_BROKEN && key->getInterpolation() != Natron::KEYFRAME_FREE
       && reason != DERIVATIVES_CHANGED ){
        key = refreshDerivatives(DERIVATIVES_CHANGED,key);
    }
    KeyFrameSet::iterator prev = key;
    if(key != _imp->keyFrames.begin()){
        --prev;
        if(prev->getInterpolation()!= Natron::KEYFRAME_BROKEN &&
           prev->getInterpolation()!= Natron::KEYFRAME_FREE &&
           prev->getInterpolation()!= Natron::KEYFRAME_NONE){
            prev = refreshDerivatives(DERIVATIVES_CHANGED,prev);
        }
    }
    KeyFrameSet::iterator next = key;
    ++next;
    if(next != _imp->keyFrames.end()){
        if(next->getInterpolation()!= Natron::KEYFRAME_BROKEN &&
           next->getInterpolation()!= Natron::KEYFRAME_FREE &&
           next->getInterpolation()!= Natron::KEYFRAME_NONE){
            next = refreshDerivatives(DERIVATIVES_CHANGED,next);
        }
    }
    
    _imp->owner->evaluateAnimationChange();
}

KeyFrameSet::const_iterator Curve::find(double time) const {
    return std::find_if(_imp->keyFrames.begin(), _imp->keyFrames.end(), KeyFrameTimePredicate(time));
}

KeyFrameSet::const_iterator Curve::keyframeAt(int index) const {
    int i = 0;
    for (KeyFrameSet::const_iterator it = _imp->keyFrames.begin(); it!=_imp->keyFrames.end(); ++it) {
        if(i == index){
            return it;
        }
        ++i;
    }
    return _imp->keyFrames.end();
}

int Curve::keyFrameIndex(double time) const {
    
    QMutexLocker l(&_imp->_lock);
    int i = 0;
    double paramEps;
    if (_imp->curveMax != INT_MAX && _imp->curveMin != INT_MIN) {
        paramEps = 1e-4 * std::abs(_imp->curveMax - _imp->curveMin);
    } else {
        paramEps = 1e-4;
    }
    for (KeyFrameSet::const_iterator it = _imp->keyFrames.begin(); it!=_imp->keyFrames.end(); ++it) {
        if(std::abs(it->getTime() - time) < paramEps){
            return i;
        }
        ++i;
    }
    throw std::runtime_error("There's no keyframe at such time");

}

bool Curve::getKeyFrameByIndex(int index,KeyFrame* ret) const {
    QMutexLocker l(&_imp->_lock);
    int c = 0;
    for (KeyFrameSet::const_iterator it = _imp->keyFrames.begin(); it!=_imp->keyFrames.end(); ++it) {
        if (c == index) {
            *ret = *it;
            return true;
        }
        ++c;
    }
    return false;
}

KeyFrameSet::const_iterator Curve::begin() const {
    return _imp->keyFrames.begin();
}

KeyFrameSet::const_iterator Curve::end() const {
    return _imp->keyFrames.end();
}

bool Curve::isYComponentMovable() const{
    return _imp->getCurveType() != CurvePrivate::STRING_CURVE;
}

bool Curve::areKeyFramesValuesClampedToIntegers() const{
    return _imp->getCurveType() == CurvePrivate::INT_CURVE;
}

bool Curve::areKeyFramesValuesClampedToBooleans() const{
    return _imp->getCurveType() == CurvePrivate::BOOL_CURVE;
}
