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
        boost::shared_ptr<KeyFrame> operator()(const boost::shared_ptr<KeyFrame>& kf) const { return boost::shared_ptr<KeyFrame>(new KeyFrame(*kf)); }
    };

    class KeyFrameTimePredicate
    {
    public:
        KeyFrameTimePredicate(double t) : _t(t) {};

        bool operator()(const boost::shared_ptr<KeyFrame>& f) { return (f->getTime() == _t); }
    private:
        double _t;
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


void KeyFrame::setValue(const Variant& v){
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
    return (*_imp->keyFrames.begin())->getTime();
}

double Curve::getMaximumTimeCovered() const{
    assert(!_imp->keyFrames.empty());
    return (*_imp->keyFrames.rbegin())->getTime();
}

void Curve::addKeyFrame(boost::shared_ptr<KeyFrame> cp)
{
    KeyFrameSet::iterator it = _imp->keyFrames.lower_bound(cp);
    if (it != _imp->keyFrames.end() && (*it)->getTime() == cp->getTime()) {
        // keyframe at same time exists, just modify it
        *(*it) = *cp;
        refreshTangents(KEYFRAME_CHANGED, it);
    } else {
        std::pair<KeyFrameSet::iterator,bool> newKey = _imp->keyFrames.insert(cp);
        assert(newKey.second);
        refreshTangents(KEYFRAME_CHANGED, newKey.first);
    }
}

void Curve::removeKeyFrame(boost::shared_ptr<KeyFrame> cp) {
    KeyFrameSet::iterator it = _imp->keyFrames.find(cp);
    assert(it!=_imp->keyFrames.end());
    KeyFrameSet::iterator prev = it;
    boost::shared_ptr<KeyFrame> prevCp;
    boost::shared_ptr<KeyFrame> nextCp;
    if(it != _imp->keyFrames.begin()){
        --prev;
        prevCp = (*prev);
    }
    KeyFrameSet::iterator next = it;
    ++next;
    if(next != _imp->keyFrames.end()){
        nextCp = (*next);
    }
    _imp->keyFrames.erase(it);
    if(prevCp){
        refreshTangents(TANGENT_CHANGED,prevCp);
    }
    if(nextCp){
        refreshTangents(TANGENT_CHANGED,nextCp);
    }

}

void Curve::removeKeyFrame(double time) {
    KeyFrameSet::iterator it = std::find_if(_imp->keyFrames.begin(), _imp->keyFrames.end(), KeyFrameTimePredicate(time));
    if (it != _imp->keyFrames.end()) {
        removeKeyFrame(*it);
    }
}




Variant Curve::getValueAt(double t) const {
    assert(!_imp->keyFrames.empty());
    if (_imp->keyFrames.size() == 1) {
        //if there's only 1 keyframe, don't bother interpolating
        return (*_imp->keyFrames.begin())->getValue();
    }
    double tcur,tnext;
    double vcurDerivRight ,vnextDerivLeft ,vcur ,vnext ;
    Natron::KeyframeType interp ,interpNext;
    KeyFrameSet::const_iterator upper = _imp->keyFrames.end();
    for(KeyFrameSet::const_iterator it = _imp->keyFrames.begin();it!=_imp->keyFrames.end();++it){
        if((*it)->getTime() > t){
            upper = it;
            break;
        }else if((*it)->getTime() == t){
            //if the time is exactly the time of a keyframe, return its value
            return (*it)->getValue();
        }
    }


    //if all keys have a greater time (i.e: we search after the last keyframe)
    KeyFrameSet::const_iterator prev = upper;
    --prev;


    //if we found no key that has a greater time (i.e: we search before the 1st keyframe)
    if (upper == _imp->keyFrames.begin()) {
        tnext = (*upper)->getTime();
        vnext = (*upper)->getValue().value<double>();
        vnextDerivLeft = (*upper)->getLeftTangent();
        interpNext = (*upper)->getInterpolation();
        tcur = tnext - 1.;
        vcur = vnext;
        vcurDerivRight = 0.;
        interp = Natron::KEYFRAME_NONE;

    } else if (upper == _imp->keyFrames.end()) {
        tcur = (*prev)->getTime();
        vcur = (*prev)->getValue().value<double>();
        vcurDerivRight = (*prev)->getRightTangent();
        interp = (*prev)->getInterpolation();
        tnext = tcur + 1.;
        vnext = vcur;
        vnextDerivLeft = 0.;
        interpNext = Natron::KEYFRAME_NONE;
    } else {
        tcur = (*prev)->getTime();
        vcur = (*prev)->getValue().value<double>();
        vcurDerivRight = (*prev)->getRightTangent();
        interp = (*prev)->getInterpolation();
        tnext = (*upper)->getTime();
        vnext = (*upper)->getValue().value<double>();
        vnextDerivLeft = (*upper)->getLeftTangent();
        interpNext = (*upper)->getInterpolation();
    }

    double v = Natron::interpolate<double>(tcur,vcur,
                                           vcurDerivRight,
                                           vnextDerivLeft,
                                           tnext,vnext,
                                           t,
                                           interp,
                                           interpNext);

    
    const Variant& firstKeyValue = (*_imp->keyFrames.begin())->getValue();
    switch(firstKeyValue.type()){
    case QVariant::Int :
        return Variant((int)std::floor(v+0.5));
    case QVariant::Double :
        return Variant(v);
    case QVariant::Bool:
        return Variant((bool)std::floor(v+0.5));
    default:
        std::string exc("The type requested ( ");
        exc.append(firstKeyValue.typeName());
        exc.append(") is not interpolable, it cannot animate!");
        throw std::invalid_argument(exc);
    }



}


bool Curve::isAnimated() const { return _imp->keyFrames.size() > 1; }

int Curve::keyFramesCount() const { return (int)_imp->keyFrames.size(); }

const KeyFrameSet& Curve::getKeyFrames() const { return _imp->keyFrames; }

void Curve::setKeyFrameValue(double value,int index){
    if(value.toDouble() != k->getValue().toDouble()){
        k->setValue(value);
        evaluateCurveChanged(KEYFRAME_CHANGED,k);
    }

}

void Curve::setKeyFrameTimeNoUpdate(double time, KeyFrameSet::iterator k) {
    if ((*k)->getInterpolation() == Natron::KEYFRAME_FREE ||
        (*k)->getInterpolation() == Natron::KEYFRAME_BROKEN) {
        const KeyFrameSet& ks = getKeyFrames();

        assert(k != ks.end());

        double oldTime = (*k)->getTime();

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
            double prevTime = (*prev)->getTime();
            assert(prevTime < oldTime);
            assert(prevTime < time); // may break if the keyframe is moved fast
            double oldLeftTan = (*k)->getLeftTangent();
            // denormalize the derivatives (which are for a [0,1] time interval)
            // and renormalize them
            double newLeftTan = (oldLeftTan / (oldTime - prevTime)) * (time - prevTime);
            (*k)->setLeftTangent(newLeftTan);
        }
        KeyFrameSet::const_iterator next = k;
        ++next;
        if (next != ks.end()) {
            double nextTime = (*next)->getTime();
            assert(oldTime < nextTime);
            assert(time < nextTime); // may break if the keyframe is moved fast
            double oldRightTan = (*k)->getRightTangent();
            // denormalize the derivatives (which are for a [0,1] time interval)
            // and renormalize them
            double newRightTan = (oldRightTan / (nextTime - oldTime)) * (nextTime - time);
            (*k)->setRightTangent(newRightTan);
        }
   }
    (*k)->setTime(time);
}

void Curve::setKeyFrameTime(double time, int index) {
    if(time != index->getTime()) {
        KeyFrameSet::iterator it = _imp->keyFrames.find(index);
        assert(it!=_imp->keyFrames.end());
        setKeyFrameTimeNoUpdate(time, it);
        evaluateCurveChanged(KEYFRAME_CHANGED,it);
    }
}


void Curve::setKeyFrameValueAndTime(double time, double value, int index) {
    bool setTime = (time != index->getTime());
    bool setValue = (value.toDouble() != index->getValue().toDouble());
    KeyFrameSet::iterator it = _imp->keyFrames.find(index);
    assert(it!=_imp->keyFrames.end());
    if(setTime) {
        setKeyFrameTimeNoUpdate(time, it);
    }
    if (setValue) {
        index->setValue(value);
    }
    if (setTime || setValue) {
        evaluateCurveChanged(KEYFRAME_CHANGED,it);
    }
}

void Curve::setKeyFrameLeftTangent(double value, int index){
    if(value != index->getLeftTangent()) {
        index->setLeftTangent(value);
        evaluateCurveChanged(TANGENT_CHANGED,index);
    }
    
}

void Curve::setKeyFrameRightTangent(double value, int time){
    if(value != time->getRightTangent()) {
        time->setRightTangent(value);
        evaluateCurveChanged(TANGENT_CHANGED,time);
    }
}

void Curve::setKeyFrameTangents(double left, double right, int time){
    if(time->getLeftTangent() != left || right != time->getRightTangent()){
        time->setLeftTangent(left);
        time->setRightTangent(right);
        evaluateCurveChanged(TANGENT_CHANGED,time);
    }
    
}


void Curve::setKeyFrameInterpolation(Natron::KeyframeType interp,,int time){
    if(k->getInterpolation() != interp){
        k->setInterpolation(interp);
        evaluateCurveChanged(KEYFRAME_CHANGED,k);
    }
}



void Curve::refreshTangents(Curve::CurveChangedReason reason, KeyFrameSet::iterator key){
    double tcur = (*key)->getTime();
    double vcur = (*key)->getValue().value<double>();
    
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
        tprev = (*prev)->getTime();
        vprev = (*prev)->getValue().value<double>();
        vprevDerivRight = (*prev)->getRightTangent();
        prevType = (*prev)->getInterpolation();
        
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
        tnext = (*next)->getTime();
        vnext = (*next)->getValue().value<double>();
        vnextDerivLeft = (*next)->getLeftTangent();
        nextType = (*next)->getInterpolation();
        
        KeyFrameSet::const_iterator nextnext = next;
        ++nextnext;
        //if next is thelast keyframe, and not edited by the user then interpolate linearly
        if(nextnext == _imp->keyFrames.end() && nextType != Natron::KEYFRAME_FREE &&
           nextType != Natron::KEYFRAME_BROKEN){
            nextType = Natron::KEYFRAME_LINEAR;
        }
    }
    
    double vcurDerivLeft,vcurDerivRight;
    try{
        Natron::autoComputeTangents<double>(prevType,
                                            (*key)->getInterpolation(),
                                            nextType,
                                            tprev, vprev,
                                            tcur, vcur,
                                            tnext, vnext,
                                            vprevDerivRight,
                                            vnextDerivLeft,
                                            &vcurDerivLeft, &vcurDerivRight);
    }catch(const std::exception& e){
        std::cout << e.what() << std::endl;
        assert(false);
    }
    (*key)->setLeftTangent(vcurDerivLeft);
    (*key)->setRightTangent(vcurDerivRight);
    if(reason != TANGENT_CHANGED){
        evaluateCurveChanged(TANGENT_CHANGED,key);
    }

}

void Curve::refreshTangents(CurveChangedReason reason, boost::shared_ptr<KeyFrame> k){
    KeyFrameSet::iterator it = _imp->keyFrames.find(k);
    assert(it!=_imp->keyFrames.end());
    refreshTangents(reason,it);
}

void Curve::evaluateCurveChanged(CurveChangedReason reason,boost::shared_ptr<KeyFrame> k){
    KeyFrameSet::iterator it = _imp->keyFrames.find(k);
    assert(it!=_imp->keyFrames.end());
    evaluateCurveChanged(reason,it);
}

void Curve::evaluateCurveChanged(CurveChangedReason reason, KeyFrameSet::iterator key){

    assert(key!=_imp->keyFrames.end());

    if((*key)->getInterpolation()!= Natron::KEYFRAME_BROKEN && (*key)->getInterpolation() != Natron::KEYFRAME_FREE
       && reason != TANGENT_CHANGED ){
        refreshTangents(TANGENT_CHANGED,key);
    }
    KeyFrameSet::iterator prev = key;
    if(key != _imp->keyFrames.begin()){
        --prev;
        if((*prev)->getInterpolation()!= Natron::KEYFRAME_BROKEN &&
           (*prev)->getInterpolation()!= Natron::KEYFRAME_FREE){
            refreshTangents(TANGENT_CHANGED,prev);
        }
    }
    KeyFrameSet::iterator next = key;
    ++next;
    if(next != _imp->keyFrames.end()){
        if((*next)->getInterpolation()!= Natron::KEYFRAME_BROKEN &&
           (*next)->getInterpolation()!= Natron::KEYFRAME_FREE){
            refreshTangents(TANGENT_CHANGED,next);
        }
    }
    
    _imp->owner->evaluateAnimationChange();
}

KeyFrameSet::const_iterator Curve::find(int time) const {
    return _imp->keyFrames.find(KeyFrame(time,0.));
}

KeyFrameSet::iterator Curve::find(int time){

}

KeyFrameSet::const_iterator Curve::end() const{
    return _imp->keyFrames.end();
}

KeyFrameSet::iterator Curve::end(){
    return _imp->keyFrames.end();
}
