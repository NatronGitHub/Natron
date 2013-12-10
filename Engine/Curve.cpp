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

/************************************KEYFRAME************************************/

KeyFrame::KeyFrame()
    : _imp(new KeyFramePrivate)
{}


KeyFrame::KeyFrame(double time, const Variant& initialValue,Curve* curve)
    : _imp(new KeyFramePrivate(time,initialValue,curve))
{
}

KeyFrame::KeyFrame(const KeyFrame& other)
    : _imp(new KeyFramePrivate(*other._imp.get()))
{
}

void KeyFrame::clone(const KeyFrame& other){
    _imp->clone(other._imp->_value,other._imp->_time,other._imp->_leftTangent,other._imp->_rightTangent,other._imp->_interpolation);
}

KeyFrame::~KeyFrame(){}

bool KeyFrame::operator==(const KeyFrame& o) const {
    return _imp->_value == o._imp->_value &&
            _imp->_time == o._imp->_time;
}



void KeyFrame::setTangents(const Variant& left,const Variant& right,bool evaluateNeighboors){
    assert(_imp->_curve);
     _imp->_leftTangent = left;
    _imp->_rightTangent = right;
    if(evaluateNeighboors){
        _imp->_curve->evaluateCurveChanged(Curve::TANGENT_CHANGED,this);
    }
}

void KeyFrame::setLeftTangent(const Variant& v,bool evaluateNeighboors){
    _imp->_leftTangent = v;
    assert(_imp->_curve);
    if(evaluateNeighboors){
        _imp->_curve->evaluateCurveChanged(Curve::TANGENT_CHANGED,this);
    }
}


void KeyFrame::setRightTangent(const Variant& v, bool evaluateNeighboors){
    _imp->_rightTangent = v;
    assert(_imp->_curve);
    if(evaluateNeighboors){
        _imp->_curve->evaluateCurveChanged(Curve::TANGENT_CHANGED,this);
    }
}

void KeyFrame::setValue(const Variant& v){
    _imp->_value = v;
    assert(_imp->_curve);
    _imp->_curve->evaluateCurveChanged(Curve::KEYFRAME_CHANGED,this);
}

void KeyFrame::setTime(double time){
    _imp->_time = time;
    assert(_imp->_curve);
    _imp->_curve->evaluateCurveChanged(Curve::KEYFRAME_CHANGED,this);
}

void KeyFrame::setTimeAndValue(double time,const Variant& v){
    _imp->_time = time;
    _imp->_value = v;
    assert(_imp->_curve);
    _imp->_curve->evaluateCurveChanged(Curve::KEYFRAME_CHANGED,this);
}

void KeyFrame::setInterpolation(Natron::KeyframeType interp) { _imp->_interpolation = interp; }

void KeyFrame::setInterpolationAndEvaluate(Natron::KeyframeType interp){
    _imp->_interpolation = interp;
     assert(_imp->_curve);
    _imp->_curve->evaluateCurveChanged(Curve::KEYFRAME_CHANGED,this);
}

Natron::KeyframeType KeyFrame::getInterpolation() const { return _imp->_interpolation; }


const Variant& KeyFrame::getValue() const { return _imp->_value; }

double KeyFrame::getTime() const { return _imp->_time; }

const Variant& KeyFrame::getLeftTangent() const { return _imp->_leftTangent; }

const Variant& KeyFrame::getRightTangent() const { return _imp->_rightTangent; }

/************************************CURVEPATH************************************/

Curve::Curve()
    : _imp(new CurvePrivate)
{

}

Curve::Curve(AnimatingParam* owner)
    : _imp(new CurvePrivate)
{
    _imp->_owner = owner;
}

Curve::~Curve(){ clearKeyFrames(); }

void Curve::clearKeyFrames(){
    _imp->_keyFrames.clear();
}

void Curve::clone(const Curve& other){
    clearKeyFrames();
    const KeyFrames& otherKeys = other.getKeyFrames();
    for(KeyFrames::const_iterator it = otherKeys.begin();it!=otherKeys.end();++it){
        boost::shared_ptr<KeyFrame> key(new KeyFrame(0,Variant(0),this));
        key->clone(*(*it));
        _imp->_keyFrames.push_back(key);
    }
}


double Curve::getMinimumTimeCovered() const{
    assert(!_imp->_keyFrames.empty());
    return _imp->_keyFrames.front()->getTime();
}

double Curve::getMaximumTimeCovered() const{
    assert(!_imp->_keyFrames.empty());
    return _imp->_keyFrames.back()->getTime();
}

void Curve::addKeyFrame(boost::shared_ptr<KeyFrame> cp)
{
    KeyFrames::iterator newKeyIt = _imp->_keyFrames.end();
    if(_imp->_keyFrames.empty()){
        newKeyIt = _imp->_keyFrames.insert(_imp->_keyFrames.end(),cp);
    }else{
        //finding a matching or the first greater key
        KeyFrames::iterator upper = _imp->_keyFrames.end();
        for(KeyFrames::iterator it = _imp->_keyFrames.begin();it!=_imp->_keyFrames.end();++it){
            if((*it)->getTime() > cp->getTime()){
                upper = it;
                break;
            }else if((*it)->getTime() == cp->getTime()){
                //if the key already exists at this time, just modify it.
                (*it)->setValue(cp->getValue());
                return;
            }
        }
        if(upper == _imp->_keyFrames.end()){
            //if we found no key that has a greater time, just append the key
            newKeyIt = _imp->_keyFrames.insert(_imp->_keyFrames.end(),cp);
        }else if(upper == _imp->_keyFrames.begin()){
            //if all the keys have a greater time, just insert this key at the begining
            newKeyIt = _imp->_keyFrames.insert(_imp->_keyFrames.begin(),cp);
        }else{
            newKeyIt = _imp->_keyFrames.insert(upper,cp);
        }

    }
    refreshTangents(KEYFRAME_CHANGED,newKeyIt);
}

void Curve::removeKeyFrame(boost::shared_ptr<KeyFrame> cp){

    KeyFrames::iterator it = std::find(_imp->_keyFrames.begin(),_imp->_keyFrames.end(),cp);
    KeyFrames::iterator prev = it;
    boost::shared_ptr<KeyFrame> prevCp;
    boost::shared_ptr<KeyFrame> nextCp;
    if(it != _imp->_keyFrames.begin()){
        --prev;
        prevCp = (*prev);
    }
    KeyFrames::iterator next = it;
    ++next;
    if(next != _imp->_keyFrames.end()){
        nextCp = (*next);
    }
    _imp->_keyFrames.erase(it);
    if(prevCp){
        refreshTangents(TANGENT_CHANGED,prevCp);
    }
    if(nextCp){
        refreshTangents(TANGENT_CHANGED,nextCp);
    }

}

void Curve::refreshTangents(Curve::CurveChangedReason reason, KeyFrames::iterator key){
    double tcur = (*key)->getTime();
    double vcur = (*key)->getValue().value<double>();

    double tprev, vprev, tnext, vnext, vprevDerivRight, vnextDerivLeft;
    Natron::KeyframeType prevType, nextType;
    if (key == _imp->_keyFrames.begin()) {
        tprev = tcur;
        vprev = vcur;
        vprevDerivRight = 0.;
        prevType = Natron::KEYFRAME_NONE;
    } else {
        KeyFrames::const_iterator prev = key;
        --prev;
        tprev = (*prev)->getTime();
        vprev = (*prev)->getValue().value<double>();
        vprevDerivRight = (*prev)->getRightTangent().value<double>();
        prevType = (*prev)->getInterpolation();

        //if prev is the first keyframe, and not edited by the user then interpolate linearly
        if(prev == _imp->_keyFrames.begin() && prevType != Natron::KEYFRAME_FREE &&
                prevType != Natron::KEYFRAME_BROKEN){
            prevType = Natron::KEYFRAME_LINEAR;
        }
    }

    KeyFrames::const_iterator next = key;
    ++next;
    if (next == _imp->_keyFrames.end()) {
        tnext = tcur;
        vnext = vcur;
        vnextDerivLeft = 0.;
        nextType = Natron::KEYFRAME_NONE;
    } else {
        tnext = (*next)->getTime();
        vnext = (*next)->getValue().value<double>();
        vnextDerivLeft = (*next)->getLeftTangent().value<double>();
        nextType = (*next)->getInterpolation();

        KeyFrames::const_iterator nextnext = next;
        ++nextnext;
        //if next is thelast keyframe, and not edited by the user then interpolate linearly
        if(nextnext == _imp->_keyFrames.end() && nextType != Natron::KEYFRAME_FREE &&
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
    bool evaluateNeighboor = true;
    if(reason == TANGENT_CHANGED){
        evaluateNeighboor = false;
    }
    (*key)->setLeftTangent(Variant(vcurDerivLeft),evaluateNeighboor);
    (*key)->setRightTangent(Variant(vcurDerivRight),evaluateNeighboor);
}



Variant Curve::getValueAt(double t) const {
    assert(!_imp->_keyFrames.empty());
    if (_imp->_keyFrames.size() == 1) {
        //if there's only 1 keyframe, don't bother interpolating
        return (*_imp->_keyFrames.begin())->getValue();
    }
    double tcur,tnext;
    double vcurDerivRight ,vnextDerivLeft ,vcur ,vnext ;
    Natron::KeyframeType interp ,interpNext;
    KeyFrames::const_iterator upper = _imp->_keyFrames.end();
    for(KeyFrames::const_iterator it = _imp->_keyFrames.begin();it!=_imp->_keyFrames.end();++it){
        if((*it)->getTime() > t){
            upper = it;
            break;
        }else if((*it)->getTime() == t){
            //if the time is exactly the time of a keyframe, return its value
            return (*it)->getValue();
        }
    }


    //if all keys have a greater time (i.e: we search after the last keyframe)
    KeyFrames::const_iterator prev = upper;
    --prev;


    //if we found no key that has a greater time (i.e: we search before the 1st keyframe)
    if (upper == _imp->_keyFrames.begin()) {
        tnext = (*upper)->getTime();
        vnext = (*upper)->getValue().value<double>();
        vnextDerivLeft = (*upper)->getLeftTangent().value<double>();
        interpNext = (*upper)->getInterpolation();
        tcur = tnext - 1.;
        vcur = vnext;
        vcurDerivRight = 0.;
        interp = Natron::KEYFRAME_NONE;

    } else if (upper == _imp->_keyFrames.end()) {
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

    
    const Variant& firstKeyValue = _imp->_keyFrames.front()->getValue();
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


void Curve::refreshTangents(CurveChangedReason reason, boost::shared_ptr<KeyFrame> k){
    KeyFrames::iterator it = std::find(_imp->_keyFrames.begin(),_imp->_keyFrames.end(),k);
    assert(it!=_imp->_keyFrames.end());
    refreshTangents(reason,it);
}

bool Curve::isAnimated() const { return _imp->_keyFrames.size() > 1; }

int Curve::keyFramesCount() const { return (int)_imp->_keyFrames.size(); }

const Curve::KeyFrames& Curve::getKeyFrames() const { return _imp->_keyFrames; }


void Curve::evaluateCurveChanged(CurveChangedReason reason,KeyFrame *k){
    KeyFrames::iterator found = _imp->_keyFrames.end();
    for(KeyFrames::iterator it =  _imp->_keyFrames.begin();it!=_imp->_keyFrames.end();++it){
        if((*it).get() == k){
            found = it;
            break;
        }
    }
    assert(found!=_imp->_keyFrames.end());


    if(k->getInterpolation()!= Natron::KEYFRAME_BROKEN && k->getInterpolation() != Natron::KEYFRAME_FREE
            && reason != TANGENT_CHANGED ){
        refreshTangents(TANGENT_CHANGED,found);
    }
    KeyFrames::iterator prev = found;
    if(found != _imp->_keyFrames.begin()){
        --prev;
        if((*prev)->getInterpolation()!= Natron::KEYFRAME_BROKEN &&
                (*prev)->getInterpolation()!= Natron::KEYFRAME_FREE){
            refreshTangents(TANGENT_CHANGED,prev);
        }
    }
    KeyFrames::iterator next = found;
    ++next;
    if(next != _imp->_keyFrames.end()){
        if((*next)->getInterpolation()!= Natron::KEYFRAME_BROKEN &&
                (*next)->getInterpolation()!= Natron::KEYFRAME_FREE){
            refreshTangents(TANGENT_CHANGED,next);
        }
    }

    _imp->_owner->evaluateAnimationChange();
}



/***********************************MULTIDIMENSIONAL VALUE*********************/
AnimatingParam::AnimatingParam()
    : _imp(new AnimatingParamPrivate(0))
{
    
}

AnimatingParam::AnimatingParam(int dimension )
    : _imp(new AnimatingParamPrivate(dimension))
{
    //default initialize the values map
    for(int i = 0; i < dimension ; ++i){
        _imp->_value.insert(std::make_pair(i,Variant()));
        boost::shared_ptr<Curve> c (new Curve(this));
        _imp->_curves.insert(std::make_pair(i,c));
    }
}

AnimatingParam::AnimatingParam(const AnimatingParam& other)
    : _imp(new AnimatingParamPrivate(other.getDimension()))
{
    *this = other;
}

void AnimatingParam::operator=(const AnimatingParam& other){
    _imp->_value.clear();
    _imp->_curves.clear();
    _imp->_dimension = other._imp->_dimension;
    for(int i = 0; i < other.getDimension() ; ++i){
        _imp->_value.insert(std::make_pair(i,other.getValue(i)));
        boost::shared_ptr<Curve> c(new Curve(this));
        boost::shared_ptr<Curve> otherCurve  = other.getCurve(i);
        if(otherCurve){
            c->clone(*otherCurve);
        }
        _imp->_curves.insert(std::make_pair(i,c));
    }

}


AnimatingParam::~AnimatingParam() { _imp->_value.clear();_imp->_curves.clear(); }


const Variant& AnimatingParam::getValue(int dimension) const{
    std::map<int,Variant>::const_iterator it = _imp->_value.find(dimension);
    assert(it != _imp->_value.end());
    return it->second;
}

void AnimatingParam::setValue(const Variant& v, int dimension, Natron::ValueChangedReason reason){
    std::map<int,Variant>::iterator it = _imp->_value.find(dimension);
    assert(it != _imp->_value.end());
    it->second = v;
    evaluateValueChange(dimension,reason);
}

boost::shared_ptr<KeyFrame> AnimatingParam::setValueAtTime(double time, const Variant& v, int dimension){
    CurvesMap::iterator foundDimension = _imp->_curves.find(dimension);
    assert(foundDimension != _imp->_curves.end());
    boost::shared_ptr<KeyFrame> k(new KeyFrame(time,v,foundDimension->second.get()));
    foundDimension->second->addKeyFrame(k);
    return k;
}

void AnimatingParam::clone(const AnimatingParam& other) {
    assert(other.getDimension() == _imp->_dimension);
    for(int i = 0; i < _imp->_dimension;++i){
        std::map<int,Variant>::iterator it = _imp->_value.find(i);
        assert(it != _imp->_value.end());
        it->second = other.getValue(i);
        boost::shared_ptr<Curve> curve = other.getCurve(i);
        boost::shared_ptr<Curve> thisCurve = getCurve(i);
        if(curve){
            thisCurve->clone(*curve);
        }
    }
}

boost::shared_ptr<Curve> AnimatingParam::getCurve(int dimension) const {
    CurvesMap::const_iterator foundDimension = _imp->_curves.find(dimension);
    assert(foundDimension != _imp->_curves.end());
    return foundDimension->second;
}

Variant AnimatingParam::getValueAtTime(double time,int dimension) const{
    CurvesMap::const_iterator foundDimension = _imp->_curves.find(dimension);
    boost::shared_ptr<Curve> curve = getCurve(dimension);
    if (curve->isAnimated()) {
        return foundDimension->second->getValueAt(time);
    } else {
        /*if the knob as no keys at this dimension, return the value
        at the requested dimension.*/
        std::map<int,Variant>::const_iterator it = _imp->_value.find(dimension);
        if(it != _imp->_value.end()){
            return it->second;
        }else{
            return Variant();
        }
    }
}




const std::map<int,Variant>& AnimatingParam::getValueForEachDimension() const {return _imp->_value;}

int AnimatingParam::getDimension() const {return _imp->_dimension;}


