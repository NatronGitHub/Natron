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

#include "Global/AppManager.h"

#include "Engine/CurvePrivate.h"
#include "Engine/Interpolation.h"
#include "Engine/KnobTypes.h"

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
    QMutexLocker l(&_imp->_lock);
    const KeyFrameSet& otherKeys = other.getKeyFrames();
    std::transform(otherKeys.begin(), otherKeys.end(), std::inserter(_imp->keyFrames, _imp->keyFrames.begin()), KeyFrameCloner());
    _imp->curveType = other._imp->curveType;
    _imp->mustSetCurveType = false;
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

bool Curve::addKeyFrame(KeyFrame key){
    
    QMutexLocker l(&_imp->_lock);
    
    {
        ///lock the project if it is saving
        QMutexLocker pl(&_imp->owner->getHolder()->getApp()->projectMutex());


        if(_imp->mustSetCurveType) {
            if(_imp->owner->typeName() == Int_Knob::typeNameStatic() ||
               _imp->owner->typeName() == Choice_Knob::typeNameStatic()){
                _imp->curveType = CurvePrivate::INT_CURVE;
            }else if(_imp->owner->typeName() == String_Knob::typeNameStatic()){
                _imp->curveType = CurvePrivate::STRING_CURVE;
            }else if(_imp->owner->typeName() == Bool_Knob::typeNameStatic()){
                _imp->curveType = CurvePrivate::BOOL_CURVE;
            }else{
                _imp->curveType = CurvePrivate::DOUBLE_CURVE;
            }
            _imp->mustSetCurveType = false;
        }

        if (_imp->curveType == CurvePrivate::BOOL_CURVE || _imp->curveType == CurvePrivate::STRING_CURVE) {
            key.setInterpolation(Natron::KEYFRAME_CONSTANT);
        }
        
        std::pair<KeyFrameSet::iterator,bool> it = addKeyFrameNoUpdate(key);
    }
    
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


double Curve::getValueAt(double t) const {
    
    QMutexLocker l(&_imp->_lock);
    
    if(_imp->keyFrames.empty()){
        throw std::runtime_error("Curve has no control points!");
    }
    if (_imp->keyFrames.size() == 1) {
        //if there's only 1 keyframe, don't bother interpolating
        return (*_imp->keyFrames.begin()).getValue();
    }
    double tcur,tnext;
    double vcurDerivRight ,vnextDerivLeft ,vcur ,vnext ;
    Natron::KeyframeType interp ,interpNext;
    
    //Replicate of std::set::lower_bound
    //we can't use that function because KeyFrame's member holding the time is an int, whereas
    //we're looking for a double which can be at any time, so we can't use the container's comparator.
    KeyFrameSet::const_iterator upper = _imp->keyFrames.end();
    for(KeyFrameSet::const_iterator it = _imp->keyFrames.begin();it!=_imp->keyFrames.end();++it){
        if((*it).getTime() > t){
            upper = it;
            break;
        }else if((*it).getTime() == t){
            //if the time is exactly the time of a keyframe, return its value
            return (*it).getValue();
        }
    }

    KeyFrameSet::const_iterator prev = upper;
    --prev;

    if (upper == _imp->keyFrames.begin()) {
        //if all keys have a greater time (i.e: we search after the last keyframe)
        tnext = upper->getTime();
        vnext = upper->getValue();
        vnextDerivLeft = upper->getLeftDerivative();
        interpNext = upper->getInterpolation();
        tcur = tnext - 1.;
        vcur = vnext;
        vcurDerivRight = 0.;
        interp = Natron::KEYFRAME_NONE;

    } else if (upper == _imp->keyFrames.end()) {
        //if we found no key that has a greater time (i.e: we search before the 1st keyframe)
        tcur = prev->getTime();
        vcur = prev->getValue();
        vcurDerivRight = prev->getRightDerivative();
        interp = prev->getInterpolation();
        tnext = tcur + 1.;
        vnext = vcur;
        vnextDerivLeft = 0.;
        interpNext = Natron::KEYFRAME_NONE;
    } else {
        tcur = prev->getTime();
        vcur = prev->getValue();
        vcurDerivRight = prev->getRightDerivative();
        interp = prev->getInterpolation();
        tnext = upper->getTime();
        vnext = upper->getValue();
        vnextDerivLeft = upper->getLeftDerivative();
        interpNext = upper->getInterpolation();
    }

    double v = Natron::interpolate<double>(tcur,vcur,
                                           vcurDerivRight,
                                           vnextDerivLeft,
                                           tnext,vnext,
                                           t,
                                           interp,
                                           interpNext);

    if (_imp->owner) {
        v = clampValueToCurveYRange(v);
    }

    switch (_imp->curveType) {
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
    return _imp->keyFrames.size() > 1;
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

int Curve::keyFramesCount() const
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
    
    ///if the curve is a string_curve or bool_curve the interpolation is bound to be constant.
    if ((_imp->curveType == CurvePrivate::STRING_CURVE || _imp->curveType == CurvePrivate::BOOL_CURVE)
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
    Natron::autoComputeDerivatives<double>(prevType,
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

KeyFrameSet::const_iterator Curve::begin() const {
    return _imp->keyFrames.begin();
}

KeyFrameSet::const_iterator Curve::end() const {
    return _imp->keyFrames.end();
}

bool Curve::isYComponentMovable() const{
    return _imp->curveType != CurvePrivate::STRING_CURVE;
}

bool Curve::areKeyFramesValuesClampedToIntegers() const{
    return _imp->curveType == CurvePrivate::INT_CURVE;
}

bool Curve::areKeyFramesValuesClampedToBooleans() const{
    return _imp->curveType == CurvePrivate::BOOL_CURVE;
}
