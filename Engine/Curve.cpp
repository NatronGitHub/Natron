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
#include <stdexcept>
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


KeyFrame::KeyFrame(double time, double initialValue, double leftDerivative, double rightDerivative, Natron::KeyframeType interpolation)
    : _time(time)
    , _value(initialValue)
    , _leftDerivative(leftDerivative)
    , _rightDerivative(rightDerivative)
    , _interpolation(interpolation)
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

KeyFrame::~KeyFrame()
{
}


void KeyFrame::setLeftDerivative(double v)
{
    assert(!boost::math::isnan(v));
    _leftDerivative = v;
}


void KeyFrame::setRightDerivative(double v)
{
    assert(!boost::math::isnan(v));
    _rightDerivative = v;
}


void KeyFrame::setValue(double v)
{
    assert(!boost::math::isnan(v));
    _value = v;
}

void KeyFrame::setTime(double time)
{
    assert(!boost::math::isnan(time));
     _time = time;
}


void KeyFrame::setInterpolation(Natron::KeyframeType interp)
{
    _interpolation = interp;
}

Natron::KeyframeType KeyFrame::getInterpolation() const
{
    return _interpolation;
}


double KeyFrame::getValue() const
{
    return _value;
}

double KeyFrame::getTime() const
{
    return _time;
}

double KeyFrame::getLeftDerivative() const
{
    return _leftDerivative;
}

double KeyFrame::getRightDerivative() const
{
    return _rightDerivative;
}

/************************************CURVEPATH************************************/

Curve::Curve()
    : _imp(new CurvePrivate)
{
}

Curve::Curve(KnobI *owner)
: _imp(new CurvePrivate)
{
    assert(owner);
    _imp->owner = owner;
    //std::string typeName = _imp->owner->typeName(); // crashes because the Knob constructor is not finished at this point
    _imp->type = CurvePrivate::DOUBLE_CURVE;
    // use RTTI to guess curve type
    if (_imp->type == CurvePrivate::DOUBLE_CURVE) {
        try {
            Int_Knob* k = dynamic_cast<Int_Knob*>(owner);
            if (k) {
                _imp->type = CurvePrivate::INT_CURVE;
            }
        } catch (const std::bad_cast& e) {
        }
    }
    if (_imp->type == CurvePrivate::DOUBLE_CURVE) {
        try {
            Choice_Knob* k = dynamic_cast<Choice_Knob*>(owner);
            if (k) {
                _imp->type = CurvePrivate::INT_CURVE_CONSTANT_INTERP;
            }
        } catch (const std::bad_cast& e) {
        }
    }
    if (_imp->type == CurvePrivate::DOUBLE_CURVE) {
        try {
            String_Knob* k = dynamic_cast<String_Knob*>(owner);
            if (k) {
                _imp->type = CurvePrivate::STRING_CURVE;
            }
        } catch (const std::bad_cast& e) {
        }
    }
    if (_imp->type == CurvePrivate::DOUBLE_CURVE) {
        try {
            File_Knob* k = dynamic_cast<File_Knob*>(owner);
            if (k) {
                _imp->type = CurvePrivate::STRING_CURVE;
            }
        } catch (const std::bad_cast& e) {
        }
    }
    if (_imp->type == CurvePrivate::DOUBLE_CURVE) {
        try {
            Bool_Knob* k = dynamic_cast<Bool_Knob*>(owner);
            if (k) {
                _imp->type = CurvePrivate::BOOL_CURVE;
            }
        } catch (const std::bad_cast& e) {
        }
    }
    
    Parametric_Knob* parametric = dynamic_cast<Parametric_Knob*>(owner);
    if (parametric) {
        _imp->isParametric = true;
    }
}

Curve::Curve(const Curve& other)
: _imp(new CurvePrivate(*other._imp))
{
    
}
Curve::~Curve()
{
    clearKeyFrames();
}

void Curve::operator=(const Curve& other) {
    *_imp = *other._imp;
}

void Curve::clearKeyFrames()
{
    QWriteLocker l(&_imp->_lock);
    _imp->keyFrames.clear();
}


bool Curve::areKeyFramesTimeClampedToIntegers() const
{
    QReadLocker l(&_imp->_lock);
    return !_imp->isParametric;
}

void Curve::clone(const Curve& other)
{
    KeyFrameSet otherKeys = other.getKeyFrames_mt_safe();
    QWriteLocker l(&_imp->_lock);
    _imp->keyFrames.clear();
    std::transform(otherKeys.begin(), otherKeys.end(), std::inserter(_imp->keyFrames, _imp->keyFrames.begin()), KeyFrameCloner());
}

void Curve::clone(const Curve& other, SequenceTime offset, const RangeD* range)
{
    KeyFrameSet otherKeys = other.getKeyFrames_mt_safe();
    // The range=[0,0] case is obviously a bug in the spec of paramCopy() from the parameter suite:
    // it prevents copying the value of frame 0.
    bool copyRange = range != NULL /*&& (range->min != 0 || range->max != 0)*/;
    QWriteLocker l(&_imp->_lock);
    _imp->keyFrames.clear();
    for (KeyFrameSet::iterator it = otherKeys.begin(); it!=otherKeys.end(); ++it) {
        double time = it->getTime();
        if (copyRange && (time < range->min || time > range->max)) {
            continue;
        }
        KeyFrame k(*it);
        if (offset != 0) {
            k.setTime(time + offset);
        }
        _imp->keyFrames.insert(k);
    }
}


double Curve::getMinimumTimeCovered() const
{
    QReadLocker l(&_imp->_lock);
    assert(!_imp->keyFrames.empty());
    return (*_imp->keyFrames.begin()).getTime();
}

double Curve::getMaximumTimeCovered() const
{
    QReadLocker l(&_imp->_lock);
    assert(!_imp->keyFrames.empty());
    return (*_imp->keyFrames.rbegin()).getTime();
}

bool Curve::addKeyFrame(KeyFrame key)
{
    
    QWriteLocker l(&_imp->_lock);
    
    if (_imp->type == CurvePrivate::BOOL_CURVE || _imp->type == CurvePrivate::STRING_CURVE ||
        _imp->type == CurvePrivate::INT_CURVE_CONSTANT_INTERP) {
        key.setInterpolation(Natron::KEYFRAME_CONSTANT);
    }
    
    
    std::pair<KeyFrameSet::iterator,bool> it = addKeyFrameNoUpdate(key);
    
    it.first = evaluateCurveChanged(KEYFRAME_CHANGED,it.first);
    l.unlock();
    ///This call must not be locked!
    if (_imp->owner) {
        _imp->owner->evaluateAnimationChange();
    }
    return it.second;
}


std::pair<KeyFrameSet::iterator,bool> Curve::addKeyFrameNoUpdate(const KeyFrame& cp)
{
    // PRIVATE - should not lock
    if (!_imp->isParametric) { //< if keyframes are clamped to integers
        std::pair<KeyFrameSet::iterator,bool> newKey = _imp->keyFrames.insert(cp);
        // keyframe at this time exists, erase and insert again
        bool addedKey = true;
        if (!newKey.second) {
            _imp->keyFrames.erase(newKey.first);
            newKey = _imp->keyFrames.insert(cp);
            assert(newKey.second);
            addedKey = false;
        }
        return std::make_pair(newKey.first,addedKey);
    } else {
        bool addedKey = true;
        double paramEps = NATRON_CURVE_X_SPACING_EPSILON * std::abs(_imp->xMax - _imp->xMin);
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

void Curve::removeKeyFrame(KeyFrameSet::const_iterator it)
{
    // PRIVATE - should not lock
    KeyFrame prevKey;
    bool mustRefreshPrev = false;
    KeyFrame nextKey;
    bool mustRefreshNext = false;

    if (it != _imp->keyFrames.begin()) {
        KeyFrameSet::iterator prev = it;
        --prev;
        prevKey = *prev;
        mustRefreshPrev = prevKey.getInterpolation() != Natron::KEYFRAME_BROKEN &&
        prevKey.getInterpolation() != Natron::KEYFRAME_FREE &&
        prevKey.getInterpolation() != Natron::KEYFRAME_NONE;
    }
    KeyFrameSet::iterator next = it;
    ++next;
    if (next != _imp->keyFrames.end()) {
        nextKey = *next;
        mustRefreshNext = nextKey.getInterpolation() != Natron::KEYFRAME_BROKEN &&
        nextKey.getInterpolation() != Natron::KEYFRAME_FREE &&
        nextKey.getInterpolation() != Natron::KEYFRAME_NONE;
    }

    _imp->keyFrames.erase(it);

    if (mustRefreshPrev) {
        refreshDerivatives(DERIVATIVES_CHANGED,find(prevKey.getTime()));
    }
    if (mustRefreshNext) {
        refreshDerivatives(DERIVATIVES_CHANGED,find(nextKey.getTime()));
    }
}

void Curve::removeKeyFrameWithIndex(int index)
{
    QWriteLocker l(&_imp->_lock);
    removeKeyFrame(atIndex(index));
}

void Curve::removeKeyFrameWithTime(double time)
{
    QWriteLocker l(&_imp->_lock);
    KeyFrameSet::iterator it = find(time);

    if (it == _imp->keyFrames.end()) {
        throw std::out_of_range("Curve::removeKeyFrameWithTime: non-existent keyframe");
    }

    removeKeyFrame(it);
}

bool Curve::getKeyFrameWithIndex(int index, KeyFrame* k) const
{
    assert(k);
    QReadLocker l(&_imp->_lock);
    if (index >= (int)_imp->keyFrames.size()) {
        return false;
    }
    *k = *atIndex(index);
    return true;
}

bool Curve::getNearestKeyFrameWithTime(double time,KeyFrame* k) const
{
    assert(k);
    QReadLocker l(&_imp->_lock);
    if (_imp->keyFrames.empty()) {
        return false;
    }
    KeyFrameSet::const_iterator upper = _imp->keyFrames.end();
    for (KeyFrameSet::const_iterator it = _imp->keyFrames.begin(); it!=_imp->keyFrames.end(); ++it) {
        if (it->getTime() > time) {
            upper = it;
            break;
        } else if (it->getTime() == time) {
            *k = *it;
            return true;
        }
    }
    
    if (upper == _imp->keyFrames.begin()) {
        *k = *upper;
        return true;
    }
    
    KeyFrameSet::const_iterator lower = upper;
    --lower;
    if (upper == _imp->keyFrames.end()) {
        *k = *lower;
        return true;
    }
    
    assert(time - lower->getTime() > 0);
    assert(upper->getTime() - time > 0);
    
    if ((upper->getTime() - time) < (time - lower->getTime())) {
        *k = *upper;
    } else {
        *k = *lower;
    }
    return true;
}

bool Curve::getKeyFrameWithTime(double time, KeyFrame* k) const
{
    assert(k);
    QReadLocker l(&_imp->_lock);
    KeyFrameSet::const_iterator it = find(time);

    if (it == _imp->keyFrames.end()) {
        return false;
    }

    *k = *it;
    return true;
}

/// compute interpolation parameters from keyframes and an iterator
/// to the next keyframe (the first with time > t)
static void
interParams(const KeyFrameSet &keyFrames,
            double t,
            const KeyFrameSet::const_iterator &itup,
            double *tcur,
            double *vcur,
            double *vcurDerivRight,
            Natron::KeyframeType *interp,
            double *tnext,
            double *vnext,
            double *vnextDerivLeft,
            Natron::KeyframeType *interpNext)
{
    assert(itup == keyFrames.end() || t < itup->getTime());
    if (itup == keyFrames.begin()) {
        //if all keys have a greater time
        // get the first keyframe
        *tnext = itup->getTime();
        *vnext = itup->getValue();
        *vnextDerivLeft = itup->getLeftDerivative();
        *interpNext = itup->getInterpolation();
        *tcur = *tnext - 1.;
        *vcur = *vnext;
        *vcurDerivRight = 0.;
        *interp = Natron::KEYFRAME_NONE;
    } else if (itup == keyFrames.end()) {
        //if we found no key that has a greater time
        // get the last keyframe
        KeyFrameSet::const_reverse_iterator itlast = keyFrames.rbegin();
        *tcur = itlast->getTime();
        *vcur = itlast->getValue();
        *vcurDerivRight = itlast->getRightDerivative();
        *interp = itlast->getInterpolation();
        *tnext = *tcur + 1.;
        *vnext = *vcur;
        *vnextDerivLeft = 0.;
        *interpNext = Natron::KEYFRAME_NONE;
    } else {
        // between two keyframes
        // get the last keyframe with time <= t
        KeyFrameSet::const_iterator itcur = itup;
        --itcur;
        assert(itcur->getTime() <= t);
        *tcur = itcur->getTime();
        *vcur = itcur->getValue();
        *vcurDerivRight = itcur->getRightDerivative();
        *interp = itcur->getInterpolation();
        *tnext = itup->getTime();
        *vnext = itup->getValue();
        *vnextDerivLeft = itup->getLeftDerivative();
        *interpNext = itup->getInterpolation();
    }
}

double Curve::getValueAt(double t) const
{
    QReadLocker l(&_imp->_lock);
    
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
    interParams(_imp->keyFrames,
                t,
                itup,
                &tcur,
                &vcur,
                &vcurDerivRight,
                &interp,
                &tnext,
                &vnext,
                &vnextDerivLeft,
                &interpNext);

    double v = Natron::interpolate(tcur,vcur,
                                   vcurDerivRight,
                                   vnextDerivLeft,
                                   tnext,vnext,
                                   t,
                                   interp,
                                   interpNext);

    if (mustClamp()) {
        v = clampValueToCurveYRange(v);
    }

    switch (_imp->type) {
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
    QReadLocker l(&_imp->_lock);

    if (_imp->keyFrames.empty()) {
        throw std::runtime_error("Curve has no control points!");
    }
    assert(_imp->type == CurvePrivate::DOUBLE_CURVE); // only real-valued curves can be derived
    
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
    interParams(_imp->keyFrames,
                t,
                itup,
                &tcur,
                &vcur,
                &vcurDerivRight,
                &interp,
                &tnext,
                &vnext,
                &vnextDerivLeft,
                &interpNext);

    double d;

    if (mustClamp()) {
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

double Curve::getIntegrateFromTo(double t1, double t2) const
{
    QReadLocker l(&_imp->_lock);
    bool opposite = false;

    // the following assumes that t2 > t1. If it's not the case, swap them and return the opposite.
    if (t1 > t2) {
        opposite = true;
        std::swap(t1,t2);
    }

    if (_imp->keyFrames.empty()) {
        throw std::runtime_error("Curve has no control points!");
    }
    assert(_imp->type == CurvePrivate::DOUBLE_CURVE); // only real-valued curves can be derived

    // even when there is only one keyframe, there may be tangents!
    //if (_imp->keyFrames.size() == 1) {
    //    //if there's only 1 keyframe, don't bother interpolating
    //    return (*_imp->keyFrames.begin()).getValue();
    //}
    double tcur,tnext;
    double vcurDerivRight ,vnextDerivLeft ,vcur ,vnext ;
    Natron::KeyframeType interp ,interpNext;

    KeyFrame k(t1,0.);
    // find the first keyframe with time strictly greater than t1
    KeyFrameSet::const_iterator itup;
    itup = _imp->keyFrames.upper_bound(k);
    interParams(_imp->keyFrames,
                t1,
                itup,
                &tcur,
                &vcur,
                &vcurDerivRight,
                &interp,
                &tnext,
                &vnext,
                &vnextDerivLeft,
                &interpNext);

    double sum = 0.;

    // while there are still keyframes after the current time, add to the total sum and advance
    while (itup != _imp->keyFrames.end() && itup->getTime() < t2) {
        // add integral from t1 to itup->getTime() to sum
        if (mustClamp()) {
            std::pair<double,double> minmax = getCurveYRange();
            sum += Natron::integrate_clamp(tcur,vcur,
                                           vcurDerivRight,
                                           vnextDerivLeft,
                                           tnext,vnext,
                                           t1, itup->getTime(),
                                           minmax.first, minmax.second,
                                           interp,
                                           interpNext);
        } else {
            sum += Natron::integrate(tcur,vcur,
                                     vcurDerivRight,
                                     vnextDerivLeft,
                                     tnext,vnext,
                                     t1, itup->getTime(),
                                     interp,
                                     interpNext);
        }
        // advance
        t1 = itup->getTime();
        ++itup;
        interParams(_imp->keyFrames,
                    t1,
                    itup,
                    &tcur,
                    &vcur,
                    &vcurDerivRight,
                    &interp,
                    &tnext,
                    &vnext,
                    &vnextDerivLeft,
                    &interpNext);
    }

    assert(itup == _imp->keyFrames.end() || t2 <= itup->getTime());
    // add integral from t1 to t2 to sum
    if (mustClamp()) {
        std::pair<double,double> minmax = getCurveYRange();
        sum += Natron::integrate_clamp(tcur,vcur,
                                       vcurDerivRight,
                                       vnextDerivLeft,
                                       tnext,vnext,
                                       t1, t2,
                                       minmax.first, minmax.second,
                                       interp,
                                       interpNext);
    } else {
        sum += Natron::integrate(tcur,vcur,
                                 vcurDerivRight,
                                 vnextDerivLeft,
                                 tnext,vnext,
                                 t1, t2,
                                 interp,
                                 interpNext);
    }

    return opposite ? -sum : sum;
}

std::pair<double,double>  Curve::getCurveYRange() const
{
    QReadLocker l(&_imp->_lock);
    if (!mustClamp()) {
        throw std::logic_error("Curve::getCurveYRange() called for a curve without owner or Y range");
    }
    if (_imp->owner) {
        Double_Knob* isDouble = dynamic_cast<Double_Knob*>(_imp->owner);
        Int_Knob* isInt = dynamic_cast<Int_Knob*>(_imp->owner);
        if (isDouble) {
            return isDouble->getMinMaxForCurve(this);
        } else if(isInt) {
            return isInt->getMinMaxForCurve(this);
        } else {
            return std::make_pair((double)INT_MIN, (double)INT_MAX);
        }
    }
    assert(hasYRange());
    return std::make_pair(_imp->yMin, _imp->yMax);
}

double Curve::clampValueToCurveYRange(double v) const
{
    // PRIVATE - should not lock
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
    QReadLocker l(&_imp->_lock);
    // even when there is only one keyframe, there may be tangents!
    return _imp->keyFrames.size() > 0;
}

void Curve::setXRange(double a,double b)
{
    QWriteLocker l(&_imp->_lock);
    _imp->xMin = a;
    _imp->xMax = b;
}

std::pair<double,double> Curve::getXRange() const
{
    QReadLocker l(&_imp->_lock);
    return std::make_pair(_imp->xMin, _imp->xMax);
}

int Curve::getKeyFramesCount() const
{
    QReadLocker l(&_imp->_lock);
    return (int)_imp->keyFrames.size();
}

KeyFrameSet Curve::getKeyFrames_mt_safe() const {
    QReadLocker l(&_imp->_lock);
    return _imp->keyFrames;
}

KeyFrameSet::iterator Curve::setKeyFrameValueAndTimeNoUpdate(double value,double time, KeyFrameSet::iterator k)
{
    // PRIVATE - should not lock
    KeyFrame newKey(*k);
    
    // nothing special has to be done, since the derivatives are with respect to t
    newKey.setTime(time);
    newKey.setValue(value);
    _imp->keyFrames.erase(k);
    return addKeyFrameNoUpdate(newKey).first;
}

KeyFrame Curve::setKeyFrameValueAndTime(double time,double value,int index,int* newIndex)
{
    bool evaluateAnimation = false;
    KeyFrame ret;
    {
        QWriteLocker l(&_imp->_lock);
        KeyFrameSet::iterator it = atIndex(index);
        if (it == _imp->keyFrames.end()) {
            QString err = QString("No such keyframe at index %1").arg(index);
            throw std::invalid_argument(err.toStdString());
        }
        
        bool setTime = (time != it->getTime());
        bool setValue = (value != it->getValue());
        
        if (setTime || setValue) {
            it = setKeyFrameValueAndTimeNoUpdate(value,time, it);
            it = evaluateCurveChanged(KEYFRAME_CHANGED,it);
            evaluateAnimation = true;
        }
        if (newIndex) {
            *newIndex = std::distance(_imp->keyFrames.begin(),it);
        }
        ret = *it;
        
    }
    if (evaluateAnimation && _imp->owner) {
        _imp->owner->evaluateAnimationChange();
    }
    return ret;

}


KeyFrame Curve::setKeyFrameLeftDerivative(double value,int index,int* newIndex)
{
 
    bool evaluateAnimation = false;
    KeyFrame ret;
    {
        
        QWriteLocker l(&_imp->_lock);
        KeyFrameSet::iterator it = atIndex(index);
        assert(it != _imp->keyFrames.end());
        
        if (value != it->getLeftDerivative()) {
            KeyFrame newKey(*it);
            newKey.setLeftDerivative(value);
            it = addKeyFrameNoUpdate(newKey).first;
            it = evaluateCurveChanged(DERIVATIVES_CHANGED,it);
            evaluateAnimation = true;
        }
        if (newIndex) {
            *newIndex = std::distance(_imp->keyFrames.begin(),it);
        }
        ret = *it;
    }
    if (evaluateAnimation && _imp->owner) {
        _imp->owner->evaluateAnimationChange();
    }
    return ret;
}


KeyFrame Curve::setKeyFrameRightDerivative(double value,int index,int* newIndex)
{
    bool evaluateAnimation = false;
    KeyFrame ret;
    {
        
        
        QWriteLocker l(&_imp->_lock);
        KeyFrameSet::iterator it = atIndex(index);
        assert(it != _imp->keyFrames.end());
        
        if (value != it->getRightDerivative()) {
            KeyFrame newKey(*it);
            newKey.setRightDerivative(value);
            it = addKeyFrameNoUpdate(newKey).first;
            it = evaluateCurveChanged(DERIVATIVES_CHANGED,it);
            evaluateAnimation = true;
        }
        if (newIndex) {
            *newIndex = std::distance(_imp->keyFrames.begin(),it);
        }
        ret = *it;
    }
    if (evaluateAnimation && _imp->owner) {
        _imp->owner->evaluateAnimationChange();
    }
    return ret;
}


KeyFrame Curve::setKeyFrameDerivatives(double left, double right,int index,int* newIndex)
{
    bool evaluateAnimation = false;
    KeyFrame ret;
    {
        
        QWriteLocker l(&_imp->_lock);
        KeyFrameSet::iterator it = atIndex(index);
        assert(it != _imp->keyFrames.end());
        
        if (left != it->getLeftDerivative() || right != it->getRightDerivative()) {
            KeyFrame newKey(*it);
            newKey.setLeftDerivative(left);
            newKey.setRightDerivative(right);
            it = addKeyFrameNoUpdate(newKey).first;
            it = evaluateCurveChanged(DERIVATIVES_CHANGED,it);
            evaluateAnimation = true;
            
        }
        if (newIndex) {
            *newIndex = std::distance(_imp->keyFrames.begin(),it);
        }
        ret = *it;
    }
    if (evaluateAnimation && _imp->owner) {
        _imp->owner->evaluateAnimationChange();
    }
    return ret;
    
}

KeyFrame Curve::setKeyFrameInterpolation(Natron::KeyframeType interp,int index,int* newIndex)
{
    
    bool evaluateAnimation = false;
    KeyFrame ret;
    {
        QWriteLocker l(&_imp->_lock);
        KeyFrameSet::iterator it = atIndex(index);
        assert(it != _imp->keyFrames.end());
        
        ///if the curve is a string_curve or bool_curve the interpolation is bound to be constant.
        if ((_imp->type == CurvePrivate::STRING_CURVE || _imp->type == CurvePrivate::BOOL_CURVE ||
             _imp->type == CurvePrivate::INT_CURVE_CONSTANT_INTERP) && interp != Natron::KEYFRAME_CONSTANT) {
            return *it;
        }
        
        
        if (interp != it->getInterpolation()) {
            it = setKeyframeInterpolation_internal(it, interp);
        }
        if (newIndex) {
            *newIndex = std::distance(_imp->keyFrames.begin(),it);
        }
        ret = *it;
    }
    if (evaluateAnimation && _imp->owner) {
        _imp->owner->evaluateAnimationChange();
    }
    return ret;
}

void Curve::setCurveInterpolation(Natron::KeyframeType interp)
{
    bool evaluateAnimation = false;

    {
        QWriteLocker l(&_imp->_lock);
        ///if the curve is a string_curve or bool_curve the interpolation is bound to be constant.
        if ((_imp->type == CurvePrivate::STRING_CURVE || _imp->type == CurvePrivate::BOOL_CURVE ||
             _imp->type == CurvePrivate::INT_CURVE_CONSTANT_INTERP) && interp != Natron::KEYFRAME_CONSTANT) {
            return;
        }
        for (int i = 0; i < (int)_imp->keyFrames.size(); ++i)
        {
            KeyFrameSet::iterator it = _imp->keyFrames.begin();
            std::advance(it, i);
            if (interp != it->getInterpolation()) {
                it = setKeyframeInterpolation_internal(it, interp);
                evaluateAnimation = true;
            }
        }
    }
    if (evaluateAnimation && _imp->owner) {
        _imp->owner->evaluateAnimationChange();
    }
}

KeyFrameSet::iterator Curve::setKeyframeInterpolation_internal(KeyFrameSet::iterator it,Natron::KeyframeType interp)
{
    ///private should not be locked
    assert(it != _imp->keyFrames.end());
    KeyFrame newKey(*it);
    newKey.setInterpolation(interp);
    it = addKeyFrameNoUpdate(newKey).first;
    it = evaluateCurveChanged(KEYFRAME_CHANGED,it);
    return it;
}

KeyFrameSet::iterator Curve::refreshDerivatives(Curve::CurveChangedReason reason, KeyFrameSet::iterator key)
{
    // PRIVATE - should not lock
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
        if (prev == _imp->keyFrames.begin() && prevType != Natron::KEYFRAME_FREE &&
            prevType != Natron::KEYFRAME_BROKEN) {
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
        if (nextnext == _imp->keyFrames.end() && nextType != Natron::KEYFRAME_FREE &&
            nextType != Natron::KEYFRAME_BROKEN) {
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
    if (!newKeyIt.second) {
        _imp->keyFrames.erase(newKeyIt.first);
        newKeyIt = _imp->keyFrames.insert(newKey);
        assert(newKeyIt.second);
    }
    key = newKeyIt.first;
    
    if (reason != DERIVATIVES_CHANGED) {
        key = evaluateCurveChanged(DERIVATIVES_CHANGED,key);
    }
    return key;
}



KeyFrameSet::iterator Curve::evaluateCurveChanged(CurveChangedReason reason, KeyFrameSet::iterator key)
{
    // PRIVATE - should not lock
    assert(key!=_imp->keyFrames.end());

    if (key->getInterpolation()!= Natron::KEYFRAME_BROKEN && key->getInterpolation() != Natron::KEYFRAME_FREE
        && reason != DERIVATIVES_CHANGED ) {
        key = refreshDerivatives(DERIVATIVES_CHANGED,key);
    }
    KeyFrameSet::iterator prev = key;
    if (key != _imp->keyFrames.begin()) {
        --prev;
        if(prev->getInterpolation()!= Natron::KEYFRAME_BROKEN &&
           prev->getInterpolation()!= Natron::KEYFRAME_FREE &&
           prev->getInterpolation()!= Natron::KEYFRAME_NONE){
            prev = refreshDerivatives(DERIVATIVES_CHANGED,prev);
        }
    }
    KeyFrameSet::iterator next = key;
    ++next;
    if (next != _imp->keyFrames.end()) {
        if(next->getInterpolation()!= Natron::KEYFRAME_BROKEN &&
           next->getInterpolation()!= Natron::KEYFRAME_FREE &&
           next->getInterpolation()!= Natron::KEYFRAME_NONE){
            next = refreshDerivatives(DERIVATIVES_CHANGED,next);
        }
    }
	return key;
}

KeyFrameSet::const_iterator Curve::find(double time) const
{
    // PRIVATE - should not lock
    return std::find_if(_imp->keyFrames.begin(), _imp->keyFrames.end(), KeyFrameTimePredicate(time));
}

KeyFrameSet::const_iterator Curve::atIndex(int index) const
{
    // PRIVATE - should not lock
    if (index >= (int)_imp->keyFrames.size()) {
        throw std::out_of_range("Curve::removeKeyFrameWithIndex: non-existent keyframe");
    }

    KeyFrameSet::iterator it = _imp->keyFrames.begin();
    std::advance(it, index);
    return it;
}

int Curve::keyFrameIndex(double time) const
{
    QReadLocker l(&_imp->_lock);
    int i = 0;
    double paramEps;
    if (_imp->xMax != INT_MAX && _imp->xMin != INT_MIN) {
        paramEps = NATRON_CURVE_X_SPACING_EPSILON * std::abs(_imp->xMax - _imp->xMin);
    } else {
        paramEps = NATRON_CURVE_X_SPACING_EPSILON;
    }
    for (KeyFrameSet::const_iterator it = _imp->keyFrames.begin();
         it!=_imp->keyFrames.end() && (it->getTime() < time+paramEps);
         ++it, ++i) {
        if (std::abs(it->getTime() - time) < paramEps) {
            return i;
        }
    }
    return -1;

}

KeyFrameSet::const_iterator Curve::begin() const
{
    // PRIVATE - should not lock
    return _imp->keyFrames.begin();
}

KeyFrameSet::const_iterator Curve::end() const
{
    // PRIVATE - should not lock
    return _imp->keyFrames.end();
}

bool Curve::isYComponentMovable() const
{
    QReadLocker l(&_imp->_lock);
    return _imp->type != CurvePrivate::STRING_CURVE;
}

bool Curve::areKeyFramesValuesClampedToIntegers() const
{
    QReadLocker l(&_imp->_lock);
    return _imp->type == CurvePrivate::INT_CURVE;
}

bool Curve::areKeyFramesValuesClampedToBooleans() const
{
    QReadLocker l(&_imp->_lock);
    return _imp->type == CurvePrivate::BOOL_CURVE;
}

void Curve::setYRange(double yMin, double yMax)
{
    QReadLocker l(&_imp->_lock);
    _imp->yMin = yMin;
    _imp->yMax = yMax;
    _imp->hasYRange = true;
}

bool Curve::hasYRange() const
{
    // PRIVATE - should not lock
    return _imp->hasYRange;
}

bool Curve::mustClamp() const
{
    // PRIVATE - should not lock
    return _imp->owner || hasYRange();
}

void Curve::getKeyFramesWithinRect(double l,double b,double r,double t,std::vector<KeyFrame>* ret) const
{
    QReadLocker locker(&_imp->_lock);
    for (KeyFrameSet::const_iterator it2 = _imp->keyFrames.begin(); it2 != _imp->keyFrames.end(); ++it2) {
        double y = it2->getValue();
        double x = it2->getTime() ;
        if (x <= r && x >= l && y <= t && y >= b) {
            ret->push_back(*it2);
        }
    }
}