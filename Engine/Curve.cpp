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
#include "Engine/CurvePrivate.h"
#include "Engine/Interpolation.h"

namespace {
    struct KeyFrameCloner {
        KeyFrame operator()(const KeyFrame& kf) const { return KeyFrame(kf); }
    };

    class KeyFrameTimePredicate
    {
    public:
        KeyFrameTimePredicate(int t) : _t(t) {};

        bool operator()(const KeyFrame& f) { return (f.getTime() == _t); }
    private:
        int _t;
    };
}

/************************************KEYFRAME************************************/

KeyFrame::KeyFrame()
    : _time(0.)
    , _value(0.)
    , _leftTangent(0.)
    , _rightTangent(0.)
    , _interpolation(Natron::KEYFRAME_SMOOTH)
{}


KeyFrame::KeyFrame(int time, double initialValue)
    : _time(time)
    , _value(initialValue)
    , _leftTangent(0.)
    , _rightTangent(0.)
    , _interpolation(Natron::KEYFRAME_SMOOTH)
{
}

KeyFrame::KeyFrame(const KeyFrame& other)
    : _time(other._time)
    , _value(other._value)
    , _leftTangent(other._leftTangent)
    , _rightTangent(other._rightTangent)
    , _interpolation(other._interpolation)
{
}

void KeyFrame::operator=(const KeyFrame& o)
{
    _time = o._time;
    _value = o._value;
    _leftTangent = o._leftTangent;
    _rightTangent = o._rightTangent;
    _interpolation = o._interpolation;
}

KeyFrame::~KeyFrame(){}


void KeyFrame::setLeftTangent(double v){
    _leftTangent = v;
}


void KeyFrame::setRightTangent(double v){
    _rightTangent = v;
}


void KeyFrame::setValue(double v){
    _value = v;
}

void KeyFrame::setTime(int time){
     _time = time;
}


void KeyFrame::setInterpolation(Natron::KeyframeType interp) { _interpolation = interp; }

Natron::KeyframeType KeyFrame::getInterpolation() const { return _interpolation; }


double KeyFrame::getValue() const { return _value; }

int KeyFrame::getTime() const { return _time; }

double KeyFrame::getLeftTangent() const { return _leftTangent; }

double KeyFrame::getRightTangent() const { return _rightTangent; }

/************************************CURVEPATH************************************/

Curve::Curve()
    : _imp(new CurvePrivate)
{

}

Curve::Curve(Knob *owner)
    : _imp(new CurvePrivate)
{
    
    _imp->owner = owner;
    const Variant& value = owner->getValue();
    switch (value.type()) {
        case QVariant::Int:
            _imp->curveType = CurvePrivate::INT_CURVE;
            break;
        case QVariant::Double :
            _imp->curveType = CurvePrivate::DOUBLE_CURVE;
            break;
        case QVariant::Bool:
            _imp->curveType = CurvePrivate::BOOL_CURVE;
            break;
        case QVariant::String:
            _imp->curveType = CurvePrivate::INT_CURVE;
            break;
        case QVariant::StringList:
            _imp->curveType = CurvePrivate::STRING_CURVE;
            break;
        default:
            _imp->curveType = CurvePrivate::DOUBLE_CURVE;
            break;
    }
}

Curve::~Curve(){ clearKeyFrames(); }

void Curve::clearKeyFrames(){
    _imp->keyFrames.clear();
}



void Curve::clone(const Curve& other){
    clearKeyFrames();
    const KeyFrameSet& otherKeys = other.getKeyFrames();
    std::transform(otherKeys.begin(), otherKeys.end(), std::inserter(_imp->keyFrames, _imp->keyFrames.begin()), KeyFrameCloner());
}


double Curve::getMinimumTimeCovered() const{
    assert(!_imp->keyFrames.empty());
    return (*_imp->keyFrames.begin()).getTime();
}

double Curve::getMaximumTimeCovered() const{
    assert(!_imp->keyFrames.empty());
    return (*_imp->keyFrames.rbegin()).getTime();
}

void Curve::addKeyFrame(const KeyFrame key){
    KeyFrameSet::iterator it = addKeyFrameNoUpdate(key);
    evaluateCurveChanged(KEYFRAME_CHANGED,it);
}

KeyFrameSet::iterator Curve::addKeyFrameNoUpdate(const KeyFrame& cp)
{
    std::pair<KeyFrameSet::iterator,bool> newKey = _imp->keyFrames.insert(cp);
    
    // keyframe at this time exists, erase and insert again
    if(!newKey.second){
        _imp->keyFrames.erase(newKey.first);
        newKey = _imp->keyFrames.insert(cp);
        assert(newKey.second);
    }
    return newKey.first;
}

void Curve::removeKeyFrame(int time) {
    
    KeyFrameSet::iterator it = find(time);
    assert(it != _imp->keyFrames.end());
    
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
        refreshTangents(TANGENT_CHANGED,find(prevKey.getTime()));
    }
    if(mustRefreshNext){
        refreshTangents(TANGENT_CHANGED,find(nextKey.getTime()));
    }
    
}


double Curve::getValueAt(double t) const {
    assert(!_imp->keyFrames.empty());
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

    //if all keys have a greater time (i.e: we search after the last keyframe)
    KeyFrameSet::const_iterator prev = upper;
    --prev;

    //if we found no key that has a greater time (i.e: we search before the 1st keyframe)
    if (upper == _imp->keyFrames.begin()) {
        tnext = upper->getTime();
        vnext = upper->getValue();
        vnextDerivLeft = upper->getLeftTangent();
        interpNext = upper->getInterpolation();
        tcur = tnext - 1.;
        vcur = vnext;
        vcurDerivRight = 0.;
        interp = Natron::KEYFRAME_NONE;

    } else if (upper == _imp->keyFrames.end()) {
        tcur = prev->getTime();
        vcur = prev->getValue();
        vcurDerivRight = prev->getRightTangent();
        interp = prev->getInterpolation();
        tnext = tcur + 1.;
        vnext = vcur;
        vnextDerivLeft = 0.;
        interpNext = Natron::KEYFRAME_NONE;
    } else {
        tcur = prev->getTime();
        vcur = prev->getValue();
        vcurDerivRight = prev->getRightTangent();
        interp = prev->getInterpolation();
        tnext = upper->getTime();
        vnext = upper->getValue();
        vnextDerivLeft = upper->getLeftTangent();
        interpNext = upper->getInterpolation();
    }

    double v = Natron::interpolate<double>(tcur,vcur,
                                           vcurDerivRight,
                                           vnextDerivLeft,
                                           tnext,vnext,
                                           t,
                                           interp,
                                           interpNext);

    switch (_imp->curveType) {
        case CurvePrivate::STRING_CURVE:
        case CurvePrivate::INT_CURVE:
            return std::floor(v + 0.5);
        case CurvePrivate::DOUBLE_CURVE:
            return v;
        case CurvePrivate::BOOL_CURVE:
            return v >= 0.5 ? 1. : 0.;
        default:
            break;
    }
}


bool Curve::isAnimated() const { return _imp->keyFrames.size() > 1; }

int Curve::keyFramesCount() const { return (int)_imp->keyFrames.size(); }

const KeyFrameSet& Curve::getKeyFrames() const { return _imp->keyFrames; }

KeyFrameSet::iterator Curve::setKeyFrameValueAndTimeNoUpdate(double value,double time, KeyFrameSet::iterator k) {
    KeyFrame newKey(*k);
    if (k->getInterpolation() == Natron::KEYFRAME_FREE ||
        k->getInterpolation() == Natron::KEYFRAME_BROKEN) {
        const KeyFrameSet& ks = getKeyFrames();

        assert(k != ks.end());

        double oldTime = k->getTime();

        // DON'T REMOVE THE FOLLOWING ASSERTS, if there is a problem it has to be fixed upstream
        // (probably in CurveWidgetPrivate::moveSelectedKeyFrames()).
        // If only one keyframe is moved, oldTime and time should be between prevTime and nextTime,
        // the GUI should make sure of this.
        // If several frames are moved, the order in which they are updated depends on the direction
        // of motion:
        // - if the motion if towards positive time, the the keyframes should be updated in decreasing
        //   time order
        // - if the motion if towards negative time, the the keyframes should be updated in increasing
        //   time order

        if (k != ks.begin()) {
            KeyFrameSet::const_iterator prev = k;
            --prev;
            double prevTime = prev->getTime();
            assert(prevTime < oldTime);
            assert(prevTime < time); // may break if the keyframe is moved fast
            double oldLeftTan = k->getLeftTangent();
            // denormalize the derivatives (which are for a [0,1] time interval)
            // and renormalize them
            double newLeftTan = (oldLeftTan / (oldTime - prevTime)) * (time - prevTime);
            newKey.setLeftTangent(newLeftTan);
        }
        KeyFrameSet::const_iterator next = k;
        ++next;
        if (next != ks.end()) {
            double nextTime = next->getTime();
            assert(oldTime < nextTime);
            assert(time < nextTime); // may break if the keyframe is moved fast
            double oldRightTan = k->getRightTangent();
            // denormalize the derivatives (which are for a [0,1] time interval)
            // and renormalize them
            double newRightTan = (oldRightTan / (nextTime - oldTime)) * (nextTime - time);
            newKey.setRightTangent(newRightTan);
        }
   }
    newKey.setTime(time);
    newKey.setValue(value);
    _imp->keyFrames.erase(k);
    return addKeyFrameNoUpdate(newKey);
}

const KeyFrame &Curve::setKeyFrameValue(double value,int index){
    KeyFrameSet::iterator it = find(index);
    assert(it != _imp->keyFrames.end());
    KeyFrame newKey(*it);
    newKey.setValue(value);
    it = addKeyFrameNoUpdate(newKey);
    
    evaluateCurveChanged(KEYFRAME_CHANGED,it);
    return *it;
}


const KeyFrame &Curve::setKeyFrameTime(double time, int index) {
    KeyFrameSet::iterator it = find(index);
    assert(it != _imp->keyFrames.end());

    if(time != it->getTime()) {
        it = setKeyFrameValueAndTimeNoUpdate(it->getValue(),time, it);
        evaluateCurveChanged(KEYFRAME_CHANGED,it);
    }
    return *it;
}


const KeyFrame &Curve::setKeyFrameValueAndTime(double time, double value, int index) {
    KeyFrameSet::iterator it = find(index);
    assert(it != _imp->keyFrames.end());

    bool setTime = (time != it->getTime());
    bool setValue = (value != it->getValue());
    
    if(setTime || setValue){
        it = setKeyFrameValueAndTimeNoUpdate(value,time, it);
        evaluateCurveChanged(KEYFRAME_CHANGED,it);
    }
    return *it;
}

const KeyFrame& Curve::setKeyFrameLeftTangent(double value, int index){
    
    KeyFrameSet::iterator it = find(index);
    assert(it != _imp->keyFrames.end());
    
    if(value != it->getLeftTangent()) {
        KeyFrame newKey(*it);
        newKey.setLeftTangent(value);
        it = addKeyFrameNoUpdate(newKey);
        evaluateCurveChanged(TANGENT_CHANGED,it);
    }
    return *it;
}

const KeyFrame &Curve::setKeyFrameRightTangent(double value, int index){
    KeyFrameSet::iterator it = find(index);
    assert(it != _imp->keyFrames.end());
    
    if(value != it->getRightTangent()) {
        KeyFrame newKey(*it);
        newKey.setRightTangent(value);
        it = addKeyFrameNoUpdate(newKey);
        evaluateCurveChanged(TANGENT_CHANGED,it);
    }
    return *it;
}

const KeyFrame &Curve::setKeyFrameTangents(double left, double right, int index){
    KeyFrameSet::iterator it = find(index);
    assert(it != _imp->keyFrames.end());
    
    if(left != it->getLeftTangent() || right != it->getRightTangent()) {
        KeyFrame newKey(*it);
        newKey.setLeftTangent(left);
        newKey.setRightTangent(right);
        it = addKeyFrameNoUpdate(newKey);
        evaluateCurveChanged(TANGENT_CHANGED,it);
    }
    return *it;
}


const KeyFrame &Curve::setKeyFrameInterpolation(Natron::KeyframeType interp,int index){
    KeyFrameSet::iterator it = find(index);
    assert(it != _imp->keyFrames.end());
    
    if(interp != it->getInterpolation()) {
        KeyFrame newKey(*it);
        newKey.setInterpolation(interp);
        it = addKeyFrameNoUpdate(newKey);
        evaluateCurveChanged(KEYFRAME_CHANGED,it);
    }
    return *it;
}



KeyFrameSet::iterator Curve::refreshTangents(Curve::CurveChangedReason reason, KeyFrameSet::iterator key){
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
        vprevDerivRight = prev->getRightTangent();
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
        vnextDerivLeft = next->getLeftTangent();
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
    Natron::autoComputeTangents<double>(prevType,
                                        key->getInterpolation(),
                                        nextType,
                                        tprev, vprev,
                                        tcur, vcur,
                                        tnext, vnext,
                                        vprevDerivRight,
                                        vnextDerivLeft,
                                        &vcurDerivLeft, &vcurDerivRight);

    
    KeyFrame newKey(*key);
    newKey.setLeftTangent(vcurDerivLeft);
    newKey.setRightTangent(vcurDerivRight);

    std::pair<KeyFrameSet::iterator,bool> newKeyIt = _imp->keyFrames.insert(newKey);

    // keyframe at this time exists, erase and insert again
    if(!newKeyIt.second){
        _imp->keyFrames.erase(newKeyIt.first);
        newKeyIt = _imp->keyFrames.insert(newKey);
        assert(newKeyIt.second);
    }
    key = newKeyIt.first;
    
    if(reason != TANGENT_CHANGED){
        evaluateCurveChanged(TANGENT_CHANGED,key);
    }
    return key;
}



void Curve::evaluateCurveChanged(CurveChangedReason reason, KeyFrameSet::iterator key){

    assert(key!=_imp->keyFrames.end());

    if(key->getInterpolation()!= Natron::KEYFRAME_BROKEN && key->getInterpolation() != Natron::KEYFRAME_FREE
       && reason != TANGENT_CHANGED ){
        refreshTangents(TANGENT_CHANGED,key);
    }
    KeyFrameSet::iterator prev = key;
    if(key != _imp->keyFrames.begin()){
        --prev;
        if(prev->getInterpolation()!= Natron::KEYFRAME_BROKEN &&
           prev->getInterpolation()!= Natron::KEYFRAME_FREE &&
           prev->getInterpolation()!= Natron::KEYFRAME_NONE){
            refreshTangents(TANGENT_CHANGED,prev);
        }
    }
    KeyFrameSet::iterator next = key;
    ++next;
    if(next != _imp->keyFrames.end()){
        if(next->getInterpolation()!= Natron::KEYFRAME_BROKEN &&
           next->getInterpolation()!= Natron::KEYFRAME_FREE &&
           next->getInterpolation()!= Natron::KEYFRAME_NONE){
            refreshTangents(TANGENT_CHANGED,next);
        }
    }
    
    _imp->owner->evaluateAnimationChange();
}

KeyFrameSet::const_iterator Curve::find(int time) const {
    return _imp->keyFrames.find(KeyFrame(time,0.));
}

KeyFrameSet::iterator Curve::end(){
    return _imp->keyFrames.end();
}
