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
    : _imp(new KeyFramePrivate)
{}


KeyFrame::KeyFrame(double time, const Variant& initialValue)
    : _imp(new KeyFramePrivate(time,initialValue))
{
}

KeyFrame::KeyFrame(const KeyFrame& other)
    : _imp(new KeyFramePrivate(*(other._imp)))
{
}

void KeyFrame::operator=(const KeyFrame& o)
{
    _imp->time = o._imp->time;
    _imp->value = o._imp->value;
    _imp->leftTangent = o._imp->leftTangent;
    _imp->rightTangent = o._imp->rightTangent;
    _imp->interpolation = o._imp->interpolation;
}

KeyFrame::~KeyFrame(){}

bool KeyFrame::operator==(const KeyFrame& o) const {
    return _imp->value == o._imp->value &&
            _imp->time == o._imp->time;
}


void KeyFrame::setLeftTangent(const Variant& v){
    _imp->leftTangent = v;
}


void KeyFrame::setRightTangent(const Variant& v){
    _imp->rightTangent = v;
}


void KeyFrame::setValue(const Variant& v){
    _imp->value = v;
}

void KeyFrame::setTime(double time){
    _imp->time = time;
}


void KeyFrame::setInterpolation(Natron::KeyframeType interp) { _imp->interpolation = interp; }

Natron::KeyframeType KeyFrame::getInterpolation() const { return _imp->interpolation; }


const Variant& KeyFrame::getValue() const { return _imp->value; }

double KeyFrame::getTime() const { return _imp->time; }

const Variant& KeyFrame::getLeftTangent() const { return _imp->leftTangent; }

const Variant& KeyFrame::getRightTangent() const { return _imp->rightTangent; }

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
    KeyFrameSet::iterator it = std::find_if(_imp->keyFrames.begin(), _imp->keyFrames.end(), KeyFrameTimePredicate(cp->getTime()));
    if (it != _imp->keyFrames.end()) {
        // keyframe at same time exists, just modify it
        *(*it) = *cp;
        refreshTangents(KEYFRAME_CHANGED, it);
    } else {
        std::pair<KeyFrameSet::iterator,bool> newKey = _imp->keyFrames.insert(cp);
        assert(newKey.second);
        refreshTangents(KEYFRAME_CHANGED, newKey.first);
    }else{
        (*newKey.first)->setValue(cp->getValue());
    }
}

void Curve::removeKeyFrame(boost::shared_ptr<KeyFrame> cp) {
    KeyFrameSet::iterator it = std::find(_imp->keyFrames.begin(),_imp->keyFrames.end(),cp);
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
        vnextDerivLeft = (*upper)->getLeftTangent().value<double>();
        interpNext = (*upper)->getInterpolation();
        tcur = tnext - 1.;
        vcur = vnext;
        vcurDerivRight = 0.;
        interp = Natron::KEYFRAME_NONE;

    } else if (upper == _imp->keyFrames.end()) {
        tcur = (*prev)->getTime();
        vcur = (*prev)->getValue().value<double>();
        vcurDerivRight = (*prev)->getRightTangent().value<double>();
        interp = (*prev)->getInterpolation();
        tnext = tcur + 1.;
        vnext = vcur;
        vnextDerivLeft = 0.;
        interpNext = Natron::KEYFRAME_NONE;
    } else {
        tcur = (*prev)->getTime();
        vcur = (*prev)->getValue().value<double>();
        vcurDerivRight = (*prev)->getRightTangent().value<double>();
        interp = (*prev)->getInterpolation();
        tnext = (*upper)->getTime();
        vnext = (*upper)->getValue().value<double>();
        vnextDerivLeft = (*upper)->getLeftTangent().value<double>();
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

void Curve::setKeyFrameValue(const Variant& value,boost::shared_ptr<KeyFrame> k){
    if(value.toDouble() != k->getValue().toDouble()){
        k->setValue(value);
        evaluateCurveChanged(KEYFRAME_CHANGED,k);
    }

}

void Curve::setKeyFrameTime(double time,boost::shared_ptr<KeyFrame> k){
    if(time != k->getTime()){
        k->setTime(time);
        evaluateCurveChanged(KEYFRAME_CHANGED,k);
    }
}


void Curve::setKeyFrameValueAndTime(double time,const Variant& value,boost::shared_ptr<KeyFrame> k){
    if(time != k->getTime() || value.toDouble() != k->getValue().toDouble()){
        k->setTime(time);
        k->setValue(value);
        evaluateCurveChanged(KEYFRAME_CHANGED,k);
    }

}

void Curve::setKeyFrameLeftTangent(const Variant& value,boost::shared_ptr<KeyFrame> k){
    if(value.toDouble() != k->getRightTangent().toDouble()){
        k->setLeftTangent(value);
        evaluateCurveChanged(TANGENT_CHANGED,k);
    }
    
}

void Curve::setKeyFrameRightTangent(const Variant& value,boost::shared_ptr<KeyFrame> k){
    if(k->getLeftTangent().toDouble()  != value.toDouble()){
        k->setRightTangent(value);
        evaluateCurveChanged(TANGENT_CHANGED,k);
    }
}

void Curve::setKeyFrameTangents(const Variant& left,const Variant& right,boost::shared_ptr<KeyFrame> k){
    if(k->getLeftTangent().toDouble()  != left.toDouble() || right.toDouble() != k->getRightTangent().toDouble()){
        k->setLeftTangent(left);
        k->setRightTangent(right);
        evaluateCurveChanged(TANGENT_CHANGED,k);
    }
    
}


void Curve::setKeyFrameInterpolation(Natron::KeyframeType interp,boost::shared_ptr<KeyFrame> k){
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
        vprevDerivRight = (*prev)->getRightTangent().value<double>();
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
        vnextDerivLeft = (*next)->getLeftTangent().value<double>();
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
    (*key)->setLeftTangent(Variant(vcurDerivLeft));
    (*key)->setRightTangent(Variant(vcurDerivRight));
    if(reason != TANGENT_CHANGED){
        evaluateCurveChanged(TANGENT_CHANGED,*key);
    }

}

void Curve::refreshTangents(CurveChangedReason reason, boost::shared_ptr<KeyFrame> k){
    KeyFrameSet::iterator it = std::find(_imp->keyFrames.begin(),_imp->keyFrames.end(),k);
    assert(it!=_imp->keyFrames.end());
    refreshTangents(reason,it);
}

void Curve::evaluateCurveChanged(CurveChangedReason reason,boost::shared_ptr<KeyFrame> k){
    KeyFrameSet::iterator found = _imp->keyFrames.end();
    for(KeyFrameSet::iterator it =  _imp->keyFrames.begin();it!=_imp->keyFrames.end();++it){
        if((*it) == k){
            found = it;
            break;
        }
    }
    assert(found!=_imp->keyFrames.end());
    
    if(k->getInterpolation()!= Natron::KEYFRAME_BROKEN && k->getInterpolation() != Natron::KEYFRAME_FREE
       && reason != TANGENT_CHANGED ){
        refreshTangents(TANGENT_CHANGED,found);
    }
    KeyFrameSet::iterator prev = found;
    if(found != _imp->keyFrames.begin()){
        --prev;
        if((*prev)->getInterpolation()!= Natron::KEYFRAME_BROKEN &&
           (*prev)->getInterpolation()!= Natron::KEYFRAME_FREE){
            refreshTangents(TANGENT_CHANGED,prev);
        }
    }
    KeyFrameSet::iterator next = found;
    ++next;
    if(next != _imp->keyFrames.end()){
        if((*next)->getInterpolation()!= Natron::KEYFRAME_BROKEN &&
           (*next)->getInterpolation()!= Natron::KEYFRAME_FREE){
            refreshTangents(TANGENT_CHANGED,next);
        }
    }
    
    _imp->owner->evaluateAnimationChange();
}
