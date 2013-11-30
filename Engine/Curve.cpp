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
    _imp->clone(other._imp->_value,other._imp->_time,other._imp->_rightTangent,other._imp->_leftTangent,other._imp->_interpolation);
}

KeyFrame::~KeyFrame(){}

bool KeyFrame::operator==(const KeyFrame& o) const {
    return _imp->_value == o._imp->_value &&
            _imp->_time == o._imp->_time;
}



void KeyFrame::setLeftTangent(const Variant& v){
    _imp->_leftTangent = v;
    assert(_imp->_curve);
    _imp->_curve->evaluateCurveChanged();
}

void KeyFrame::setRightTangent(const Variant& v){
    _imp->_rightTangent = v;
    assert(_imp->_curve);
    _imp->_curve->evaluateCurveChanged();
}

void KeyFrame::setValue(const Variant& v){
    _imp->_value = v;
    assert(_imp->_curve);
    if(_imp->_interpolation != Natron::KEYFRAME_BROKEN && _imp->_interpolation != Natron::KEYFRAME_FREE){
        _imp->_curve->refreshTangents(this);
    }
    _imp->_curve->evaluateCurveChanged();
}

void KeyFrame::setTime(double time){
    _imp->_time = time;
    assert(_imp->_curve);
    _imp->_curve->evaluateCurveChanged();
}

void KeyFrame::setInterpolation(Natron::KeyframeType interp) { _imp->_interpolation = interp; }

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
    for(KeyFrames::const_iterator it = _imp->_keyFrames.begin();it!=_imp->_keyFrames.end();++it){
        delete (*it);
    }
    _imp->_keyFrames.clear();
}

void Curve::clone(const Curve& other){
    clearKeyFrames();
    const KeyFrames& otherKeys = other.getKeyFrames();
    for(KeyFrames::const_iterator it = otherKeys.begin();it!=otherKeys.end();++it){
        KeyFrame* key = new KeyFrame(0,Variant(0),this);
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

void Curve::addControlPoint(KeyFrame* cp)
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
                refreshTangents(it);
                delete cp;
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
    refreshTangents(newKeyIt);
}

void Curve::refreshTangents(KeyFrames::iterator key){
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
    }

    double vcurDerivLeft,vcurDerivRight;
    Natron::autoComputeTangents<double>(prevType,
                                        (*key)->getInterpolation(),
                                        nextType,
                                        tprev, vprev,
                                        tcur, vcur,
                                        tnext, vnext,
                                        vprevDerivRight,
                                        vnextDerivLeft,
                                        &vcurDerivLeft, &vcurDerivRight);


    (*key)->setLeftTangent(Variant(vcurDerivLeft));
    (*key)->setRightTangent(Variant(vcurDerivRight));
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
        return Variant((int)v);
    case QVariant::Double :
        return Variant(v);
    default:
        std::string exc("The type requested ( ");
        exc.append(firstKeyValue.typeName());
        exc.append(") is not interpolable, it cannot animate!");
        throw std::invalid_argument(exc);
    }



}


void Curve::refreshTangents(KeyFrame* k){
    KeyFrames::iterator it = std::find(_imp->_keyFrames.begin(),_imp->_keyFrames.end(),k);
    assert(it!=_imp->_keyFrames.end());
    refreshTangents(it);
}

bool Curve::isAnimated() const { return _imp->_keyFrames.size() > 1; }

int Curve::getControlPointsCount() const { return (int)_imp->_keyFrames.size(); }

const KeyFrame* Curve::getStart() const { assert(!_imp->_keyFrames.empty()); return _imp->_keyFrames.front(); }

const KeyFrame* Curve::getEnd() const { assert(!_imp->_keyFrames.empty()); return _imp->_keyFrames.back(); }

const Curve::KeyFrames& Curve::getKeyFrames() const { return _imp->_keyFrames; }


void Curve::evaluateCurveChanged(){
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

void AnimatingParam::setValue(const Variant& v, int dimension, ValueChangedReason reason){
    std::map<int,Variant>::iterator it = _imp->_value.find(dimension);
    assert(it != _imp->_value.end());
    it->second = v;
    evaluateValueChange(dimension,reason);
}

void AnimatingParam::setValueAtTime(double time, const Variant& v, int dimension){
    CurvesMap::iterator foundDimension = _imp->_curves.find(dimension);
    assert(foundDimension != _imp->_curves.end());
    foundDimension->second->addControlPoint(new KeyFrame(time,v,foundDimension->second.get()));
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
    if(!curve->isAnimated()){
        /*if the knob as no keys at this dimension, return the value
        at the requested dimension.*/
        std::map<int,Variant>::const_iterator it = _imp->_value.find(dimension);
        if(it != _imp->_value.end()){
            return it->second;
        }else{
            return Variant();
        }
    }else{
        try{
            return foundDimension->second->getValueAt(time);
        }catch(const std::exception& e){
            std::cout << e.what() << std::endl;
            assert(false);
        }

    }
}


RectD AnimatingParam::getCurvesBoundingBox() const{
    RectD ret;
    for(CurvesMap::const_iterator it = _imp->_curves.begin() ; it!=_imp->_curves.end();++it){
        
        
        double xmin = it->second->getMinimumTimeCovered();
        double xmax = it->second->getMaximumTimeCovered();
        double ymin = INT_MAX;
        double ymax = INT_MIN;
        //find out ymin,ymax
        const Curve::KeyFrames& keys = it->second->getKeyFrames();
        for (Curve::KeyFrames::const_iterator it2 = keys.begin(); it2!=keys.end(); ++it2) {
            const Variant& v = (*it2)->getValue();
            double value;
            if(v.type() == QVariant::Int){
                value = (double)v.toInt();
            }else{
                value = v.toDouble();
            }
            if(value < ymin)
                ymin = value;
            if(value > ymax)
                ymax = value;
        }
        ret.merge(xmin,ymin,xmax,ymax);
    }
    return ret;
}

const std::map<int,Variant>& AnimatingParam::getValueForEachDimension() const {return _imp->_value;}

int AnimatingParam::getDimension() const {return _imp->_dimension;}


