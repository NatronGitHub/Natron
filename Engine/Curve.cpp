/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier
 *
 * Natron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Natron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Curve.h"

#include <algorithm>
#include <stdexcept>

#if !defined(SBK_RUN) && !defined(Q_MOC_RUN)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
#include <boost/math/special_functions/fpclassify.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
#endif
#include "Engine/AppManager.h"

#include "Engine/CurvePrivate.h"
#include "Engine/Interpolation.h"
#include "Engine/KnobTypes.h"
#include "Engine/KnobFile.h"

namespace {
struct KeyFrameCloner
{
    KeyFrame operator()(const KeyFrame & kf) const
    {
        return KeyFrame(kf);
    }
};

class KeyFrameTimePredicate
{
public:
    KeyFrameTimePredicate(double t)
        : _t(t)
    {
    };

    bool operator()(const KeyFrame & f)
    {
        return f.getTime() == _t;
    }

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
      , _interpolation(Natron::eKeyframeTypeSmooth)
{
}

KeyFrame::KeyFrame(double time,
                   double initialValue,
                   double leftDerivative,
                   double rightDerivative,
                   Natron::KeyframeTypeEnum interpolation)
    : _time(time)
      , _value(initialValue)
      , _leftDerivative(leftDerivative)
      , _rightDerivative(rightDerivative)
      , _interpolation(interpolation)
{
    assert(!boost::math::isnan(_time) && !boost::math::isinf(_time));
    assert(!boost::math::isnan(_value) && !boost::math::isinf(_value));
    assert(!boost::math::isnan(_leftDerivative) && !boost::math::isinf(_leftDerivative));
    assert(!boost::math::isnan(_rightDerivative) && !boost::math::isinf(_rightDerivative));
}

KeyFrame::KeyFrame(const KeyFrame & other)
    : _time(other._time)
      , _value(other._value)
      , _leftDerivative(other._leftDerivative)
      , _rightDerivative(other._rightDerivative)
      , _interpolation(other._interpolation)
{
    assert(!boost::math::isnan(_time) && !boost::math::isinf(_time));
    assert(!boost::math::isnan(_value) && !boost::math::isinf(_value));
    assert(!boost::math::isnan(_leftDerivative) && !boost::math::isinf(_leftDerivative));
    assert(!boost::math::isnan(_rightDerivative) && !boost::math::isinf(_rightDerivative));
}

void
KeyFrame::operator=(const KeyFrame & o)
{
    _time = o._time;
    _value = o._value;
    _leftDerivative = o._leftDerivative;
    _rightDerivative = o._rightDerivative;
    _interpolation = o._interpolation;
    assert(!boost::math::isnan(_time) && !boost::math::isinf(_time));
    assert(!boost::math::isnan(_value) && !boost::math::isinf(_value));
    assert(!boost::math::isnan(_leftDerivative) && !boost::math::isinf(_leftDerivative));
    assert(!boost::math::isnan(_rightDerivative) && !boost::math::isinf(_rightDerivative));
}

KeyFrame::~KeyFrame()
{
}

void
KeyFrame::setLeftDerivative(double leftDerivative)
{
    _leftDerivative = leftDerivative;
    assert(!boost::math::isnan(_leftDerivative) && !boost::math::isinf(_leftDerivative));
}

void
KeyFrame::setRightDerivative(double rightDerivative)
{
    _rightDerivative = rightDerivative;
    assert(!boost::math::isnan(_rightDerivative) && !boost::math::isinf(_rightDerivative));
}

void
KeyFrame::setValue(double value)
{
    _value = value;
    assert(!boost::math::isnan(_value) && !boost::math::isinf(_value));
}

void
KeyFrame::setTime(double time)
{
    _time = time;
    assert(!boost::math::isnan(_time) && !boost::math::isinf(_time));
}

void
KeyFrame::setInterpolation(Natron::KeyframeTypeEnum interp)
{
    _interpolation = interp;
}

Natron::KeyframeTypeEnum
KeyFrame::getInterpolation() const
{
    return _interpolation;
}

double
KeyFrame::getValue() const
{
    return _value;
}

double
KeyFrame::getTime() const
{
    return _time;
}

double
KeyFrame::getLeftDerivative() const
{
    return _leftDerivative;
}

double
KeyFrame::getRightDerivative() const
{
    return _rightDerivative;
}

/************************************CURVEPATH************************************/

Curve::Curve()
    : _imp(new CurvePrivate)
{
}

Curve::Curve(KnobI *owner,int dimensionInOwner)
    : _imp(new CurvePrivate)
{
    assert(owner);
    _imp->owner = owner;
    _imp->dimensionInOwner = dimensionInOwner;
    //std::string typeName = _imp->owner->typeName(); // crashes because the Knob constructor is not finished at this point
    bool found = false;
    // use RTTI to guess curve type
    if (!found) {
        Double_Knob* k = dynamic_cast<Double_Knob*>(owner);
        if (k) {
            _imp->type = CurvePrivate::eCurveTypeDouble;
            found = true;
        }
    }
    if (!found) {
        Color_Knob* k = dynamic_cast<Color_Knob*>(owner);
        if (k) {
            _imp->type = CurvePrivate::eCurveTypeDouble;
            found = true;
        }
    }
    if (!found) {
        Int_Knob* k = dynamic_cast<Int_Knob*>(owner);
        if (k) {
            _imp->type = CurvePrivate::eCurveTypeInt;
            found = true;
        }
    }
    if (!found) {
        Choice_Knob* k = dynamic_cast<Choice_Knob*>(owner);
        if (k) {
            _imp->type = CurvePrivate::eCurveTypeIntConstantInterp;
            found = true;
        }
    }
    if (!found) {
        String_Knob* k = dynamic_cast<String_Knob*>(owner);
        if (k) {
            _imp->type = CurvePrivate::eCurveTypeString;
            found = true;
        }
    }
    if (!found) {
        File_Knob* k = dynamic_cast<File_Knob*>(owner);
        if (k) {
            _imp->type = CurvePrivate::eCurveTypeString;
            found = true;
        }
    }
    if (!found) {
        OutputFile_Knob* k = dynamic_cast<OutputFile_Knob*>(owner);
        if (k) {
            _imp->type = CurvePrivate::eCurveTypeString;
            found = true;
        }
    }
    if (!found) {
        Path_Knob* k = dynamic_cast<Path_Knob*>(owner);
        if (k) {
            _imp->type = CurvePrivate::eCurveTypeString;
            found = true;
        }
    }
    if (!found) {
        Bool_Knob* k = dynamic_cast<Bool_Knob*>(owner);
        if (k) {
            _imp->type = CurvePrivate::eCurveTypeBool;
            found = true;
        }
    }
    
    if (!found) {
        Parametric_Knob* parametric = dynamic_cast<Parametric_Knob*>(owner);
        if (parametric) {
            _imp->isParametric = true;
            found = true;
        }
    }
    assert(found);
}

Curve::Curve(const Curve & other)
    : _imp( new CurvePrivate(*other._imp) )
{
}

Curve::~Curve()
{
    clearKeyFrames();
}

void
Curve::operator=(const Curve & other)
{
    *_imp = *other._imp;
}

void
Curve::clearKeyFrames()
{
    QMutexLocker l(&_imp->_lock);

    _imp->keyFrames.clear();
}

bool
Curve::areKeyFramesTimeClampedToIntegers() const
{
    QMutexLocker l(&_imp->_lock);

    return !_imp->isParametric;
}

void
Curve::clone(const Curve & other)
{
    KeyFrameSet otherKeys = other.getKeyFrames_mt_safe();
    QMutexLocker l(&_imp->_lock);

    _imp->keyFrames.clear();
    std::transform( otherKeys.begin(), otherKeys.end(), std::inserter( _imp->keyFrames, _imp->keyFrames.begin() ), KeyFrameCloner() );
    onCurveChanged();
}

bool
Curve::cloneAndCheckIfChanged(const Curve& other)
{
    KeyFrameSet otherKeys = other.getKeyFrames_mt_safe();
    QMutexLocker l(&_imp->_lock);
    bool hasChanged = false;
    if (otherKeys.size() != _imp->keyFrames.size()) {
        hasChanged = true;
    }
    if (!hasChanged) {
        assert(otherKeys.size() == _imp->keyFrames.size());
        KeyFrameSet::iterator oit = otherKeys.begin();
        for (KeyFrameSet::iterator it = _imp->keyFrames.begin(); it!=_imp->keyFrames.end(); ++it,++oit) {
            if (*it != *oit) {
                hasChanged = true;
                break;
            }
        }
    }
    if (hasChanged) {
        _imp->keyFrames.clear();
        std::transform( otherKeys.begin(), otherKeys.end(), std::inserter( _imp->keyFrames, _imp->keyFrames.begin() ), KeyFrameCloner() );
        onCurveChanged();
    }
    return hasChanged;
}

void
Curve::clone(const Curve & other,
             SequenceTime offset,
             const RangeD* range)
{
    KeyFrameSet otherKeys = other.getKeyFrames_mt_safe();
    // The range=[0,0] case is obviously a bug in the spec of paramCopy() from the parameter suite:
    // it prevents copying the value of frame 0.
    bool copyRange = range != NULL /*&& (range->min != 0 || range->max != 0)*/;
    QMutexLocker l(&_imp->_lock);

    _imp->keyFrames.clear();
    for (KeyFrameSet::iterator it = otherKeys.begin(); it != otherKeys.end(); ++it) {
        double time = it->getTime();
        if ( copyRange && ( (time < range->min) || (time > range->max) ) ) {
            continue;
        }
        KeyFrame k(*it);
        if (offset != 0) {
            k.setTime(time + offset);
        }
        _imp->keyFrames.insert(k);
    }
    onCurveChanged();
}

double
Curve::getMinimumTimeCovered() const
{
    QMutexLocker l(&_imp->_lock);

    assert( !_imp->keyFrames.empty() );

    return ( *_imp->keyFrames.begin() ).getTime();
}

double
Curve::getMaximumTimeCovered() const
{
    QMutexLocker l(&_imp->_lock);

    assert( !_imp->keyFrames.empty() );

    return ( *_imp->keyFrames.rbegin() ).getTime();
}

bool
Curve::addKeyFrame(KeyFrame key)
{
    QMutexLocker l(&_imp->_lock);

    if ( (_imp->type == CurvePrivate::eCurveTypeBool) || (_imp->type == CurvePrivate::eCurveTypeString) ||
         ( _imp->type == CurvePrivate::eCurveTypeIntConstantInterp) ) {
        key.setInterpolation(Natron::eKeyframeTypeConstant);
    }


    std::pair<KeyFrameSet::iterator,bool> it = addKeyFrameNoUpdate(key);

    it.first = evaluateCurveChanged(eCurveChangedReasonKeyframeChanged, it.first);
    l.unlock();

    return it.second;
}

std::pair<KeyFrameSet::iterator,bool> Curve::addKeyFrameNoUpdate(const KeyFrame & cp)
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
        for (KeyFrameSet::iterator it = _imp->keyFrames.begin(); it != _imp->keyFrames.end(); ++it) {
            if (std::abs( it->getTime() - cp.getTime() ) < paramEps) {
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

void
Curve::removeKeyFrame(KeyFrameSet::const_iterator it)
{
    // PRIVATE - should not lock
    KeyFrame prevKey;
    bool mustRefreshPrev = false;
    KeyFrame nextKey;
    bool mustRefreshNext = false;

    if ( it != _imp->keyFrames.begin() ) {
        KeyFrameSet::iterator prev = it;
        if (prev != _imp->keyFrames.begin()) {
            --prev;
        }
        prevKey = *prev;
        mustRefreshPrev = prevKey.getInterpolation() != Natron::eKeyframeTypeBroken &&
                          prevKey.getInterpolation() != Natron::eKeyframeTypeFree &&
                          prevKey.getInterpolation() != Natron::eKeyframeTypeNone;
    }
    KeyFrameSet::iterator next = it;
    if (next != _imp->keyFrames.end()) {
        ++next;
    }
    if ( next != _imp->keyFrames.end() ) {
        nextKey = *next;
        mustRefreshNext = nextKey.getInterpolation() != Natron::eKeyframeTypeBroken &&
                          nextKey.getInterpolation() != Natron::eKeyframeTypeFree &&
                          nextKey.getInterpolation() != Natron::eKeyframeTypeNone;
    }

    _imp->keyFrames.erase(it);

    if (mustRefreshPrev) {
        refreshDerivatives( eCurveChangedReasonDerivativesChanged,find( prevKey.getTime() ) );
    }
    if (mustRefreshNext) {
        refreshDerivatives( eCurveChangedReasonDerivativesChanged,find( nextKey.getTime() ) );
    }
    onCurveChanged();
}

void
Curve::removeKeyFrameWithIndex(int index)
{
    if (index == -1) {
        return;
    }
    QMutexLocker l(&_imp->_lock);

    removeKeyFrame( atIndex(index) );
}


void
Curve::removeKeyFrameWithTime(double time)
{
    QMutexLocker l(&_imp->_lock);
    KeyFrameSet::iterator it = find(time);

    if ( it == _imp->keyFrames.end() ) {
        throw std::out_of_range("Curve::removeKeyFrameWithTime: non-existent keyframe");
    }

    removeKeyFrame(it);
}

void
Curve::removeKeyFramesBeforeTime(double time,std::list<int>* keyframeRemoved)
{
    KeyFrameSet newSet;
    QMutexLocker l(&_imp->_lock);
    for (KeyFrameSet::iterator it = _imp->keyFrames.begin(); it != _imp->keyFrames.end(); ++it) {
        if (it->getTime() < time) {
            keyframeRemoved->push_back(it->getTime());
            continue;
        }
        newSet.insert(*it);
    }
    _imp->keyFrames = newSet;
    if (!_imp->keyFrames.empty()) {
        refreshDerivatives(Curve::eCurveChangedReasonKeyframeChanged, _imp->keyFrames.begin());
    }
    onCurveChanged();
}

void
Curve::removeKeyFramesAfterTime(double time,std::list<int>* keyframeRemoved)
{
    KeyFrameSet newSet;
    QMutexLocker l(&_imp->_lock);
    for (KeyFrameSet::iterator it = _imp->keyFrames.begin(); it != _imp->keyFrames.end(); ++it) {
        if (it->getTime() > time) {
            keyframeRemoved->push_back(it->getTime());
            continue;
        }
        newSet.insert(*it);
    }
    _imp->keyFrames = newSet;
    if (!_imp->keyFrames.empty()) {
        KeyFrameSet::iterator last = _imp->keyFrames.end();
        --last;
        refreshDerivatives(Curve::eCurveChangedReasonKeyframeChanged, last);
    }
    onCurveChanged();
}

bool
Curve::getKeyFrameWithIndex(int index,
                            KeyFrame* k) const
{
    assert(k);
    QMutexLocker l(&_imp->_lock);
    if (index < 0 || (int)_imp->keyFrames.size() <= index ) {
        return false;
    }
    *k = *atIndex(index);

    return true;
}

bool
Curve::getNearestKeyFrameWithTime(double time,
                                  KeyFrame* k) const
{
    assert(k);
    QMutexLocker l(&_imp->_lock);
    if ( _imp->keyFrames.empty() ) {
        return false;
    }
    if (_imp->keyFrames.size() == 1) {
        *k = *_imp->keyFrames.begin();

        return true;
    }

    KeyFrame kt(time, 0.); // virtual keyframe at t for comparison

    KeyFrameSet::const_iterator lower = _imp->keyFrames.lower_bound(kt);
    if (lower == _imp->keyFrames.end()) {
        // all elements are before, take the last one
        *k = *_imp->keyFrames.rbegin();

        return true;
    }
    if (lower->getTime() >= time) {
        // we are before the first element, return it
        *k = *lower;

        return true;
    }
    KeyFrameSet::const_iterator upper = _imp->keyFrames.upper_bound(kt);
    if (upper == _imp->keyFrames.end()) {
        // no element after this one, return the lower bound
        *k = *lower;

        return true;
    }

    // upper and lower are both valid iterators, take the closest one
    assert(time - lower->getTime() > 0);
    assert(upper->getTime() - time > 0);

    if ( (upper->getTime() - time) < ( time - lower->getTime() ) ) {
        *k = *upper;
    } else {
        *k = *lower;
    }

    return true;
}

bool
Curve::getPreviousKeyframeTime(double time,
                               KeyFrame* k) const
{
    assert(k);
    QMutexLocker l(&_imp->_lock);
    if ( _imp->keyFrames.empty() ) {
        return false;
    }
    KeyFrameSet::const_iterator upper = _imp->keyFrames.end();
    for (KeyFrameSet::const_iterator it = _imp->keyFrames.begin(); it != _imp->keyFrames.end(); ++it) {
        if (it->getTime() > time) {
            upper = it;
            break;
        } else if (it->getTime() == time) {
            if ( it == _imp->keyFrames.begin() ) {
                return false;
            } else {
                --it;
                *k = *it;

                return true;
            }
        }
    }
    if ( upper == _imp->keyFrames.end() ) {
        *k = *_imp->keyFrames.rbegin();

        return true;
    } else if ( upper == _imp->keyFrames.begin() ) {
        return false;
    } else {
        ///If we reach here the previous keyframe is exactly the previous to upper because we already checked
        ///in the for loop that the previous key wasn't equal to the given time
        --upper;
        assert(upper->getTime() < time);
        *k = *upper;

        return true;
    }
}

bool
Curve::getNextKeyframeTime(double time,
                           KeyFrame* k) const
{
    assert(k);
    QMutexLocker l(&_imp->_lock);
    if ( _imp->keyFrames.empty() ) {
        return false;
    }
    KeyFrameSet::const_iterator upper = _imp->keyFrames.end();
    for (KeyFrameSet::const_iterator it = _imp->keyFrames.begin(); it != _imp->keyFrames.end(); ++it) {
        if (it->getTime() > time) {
            upper = it;
            break;
        }
    }
    if ( upper == _imp->keyFrames.end() ) {
        return false;
    } else {
        *k = *upper;

        return true;
    }
}

bool
Curve::getKeyFrameWithTime(double time,
                           KeyFrame* k) const
{
    assert(k);
    QMutexLocker l(&_imp->_lock);
    KeyFrameSet::const_iterator it = find(time);

    if ( it == _imp->keyFrames.end() ) {
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
            Natron::KeyframeTypeEnum *interp,
            double *tnext,
            double *vnext,
            double *vnextDerivLeft,
            Natron::KeyframeTypeEnum *interpNext)
{
//#pragma message WARN("Unused parameter 't'") //the parameter is good enough for asserts
    Q_UNUSED(t);
    assert( itup == keyFrames.end() || t < itup->getTime() );
    if ( itup == keyFrames.begin() ) {
        //if all keys have a greater time
        // get the first keyframe
        *tnext = itup->getTime();
        *vnext = itup->getValue();
        *vnextDerivLeft = itup->getLeftDerivative();
        *interpNext = itup->getInterpolation();
        *tcur = *tnext - 1.;
        *vcur = *vnext;
        *vcurDerivRight = 0.;
        *interp = Natron::eKeyframeTypeNone;
    } else if ( itup == keyFrames.end() ) {
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
        *interpNext = Natron::eKeyframeTypeNone;
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

double
Curve::getValueAt(double t,bool doClamp) const
{
    QMutexLocker l(&_imp->_lock);

    if ( _imp->keyFrames.empty() ) {
        throw std::runtime_error("Curve has no control points!");
    }
    
    std::map<double,double>::const_iterator foundCached = _imp->resultCache.find(t);
    double v;
    if (foundCached != _imp->resultCache.end()) {
        v = foundCached->second;
    } else {
        
        // even when there is only one keyframe, there may be tangents!
        //if (_imp->keyFrames.size() == 1) {
        //    //if there's only 1 keyframe, don't bother interpolating
        //    return (*_imp->keyFrames.begin()).getValue();
        //}
        double tcur,tnext;
        double vcurDerivRight,vnextDerivLeft,vcur,vnext;
        Natron::KeyframeTypeEnum interp,interpNext;
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
        
        v = Natron::interpolate(tcur,vcur,
                                vcurDerivRight,
                                vnextDerivLeft,
                                tnext,vnext,
                                t,
                                interp,
                                interpNext);
        _imp->resultCache[t] = v;
    }

    if ( doClamp && mustClamp() ) {
        v = clampValueToCurveYRange(v);
    }

    switch (_imp->type) {
    case CurvePrivate::eCurveTypeString:
    case CurvePrivate::eCurveTypeInt:

        return std::floor(v + 0.5);
    case CurvePrivate::eCurveTypeDouble:

        return v;
    case CurvePrivate::eCurveTypeBool:

        return v >= 0.5 ? 1. : 0.;
    default:

        return v;
    }
} // getValueAt

double
Curve::getDerivativeAt(double t) const
{
    QMutexLocker l(&_imp->_lock);

    if ( _imp->keyFrames.empty() ) {
        throw std::runtime_error("Curve has no control points!");
    }
    assert(_imp->type == CurvePrivate::eCurveTypeDouble); // only real-valued curves can be derived

    // even when there is only one keyframe, there may be tangents!
    //if (_imp->keyFrames.size() == 1) {
    //    //if there's only 1 keyframe, don't bother interpolating
    //    return (*_imp->keyFrames.begin()).getValue();
    //}
    double tcur,tnext;
    double vcurDerivRight,vnextDerivLeft,vcur,vnext;
    Natron::KeyframeTypeEnum interp,interpNext;
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

    if ( mustClamp() ) {
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
} // getDerivativeAt

double
Curve::getIntegrateFromTo(double t1,
                          double t2) const
{
    QMutexLocker l(&_imp->_lock);
    bool opposite = false;

    // the following assumes that t2 > t1. If it's not the case, swap them and return the opposite.
    if (t1 > t2) {
        opposite = true;
        std::swap(t1,t2);
    }

    if ( _imp->keyFrames.empty() ) {
        throw std::runtime_error("Curve has no control points!");
    }
    assert(_imp->type == CurvePrivate::eCurveTypeDouble); // only real-valued curves can be derived

    // even when there is only one keyframe, there may be tangents!
    //if (_imp->keyFrames.size() == 1) {
    //    //if there's only 1 keyframe, don't bother interpolating
    //    return (*_imp->keyFrames.begin()).getValue();
    //}
    double tcur,tnext;
    double vcurDerivRight,vnextDerivLeft,vcur,vnext;
    Natron::KeyframeTypeEnum interp,interpNext;
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
        if ( mustClamp() ) {
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

    assert( itup == _imp->keyFrames.end() || t2 <= itup->getTime() );
    // add integral from t1 to t2 to sum
    if ( mustClamp() ) {
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
} // getIntegrateFromTo

std::pair<double,double>  Curve::getCurveYRange() const
{
    QMutexLocker l(&_imp->_lock);

    if ( !mustClamp() ) {
        throw std::logic_error("Curve::getCurveYRange() called for a curve without owner or Y range");
    }
    if (_imp->owner) {
        Knob<double>* isDouble = dynamic_cast<Knob<double>*>(_imp->owner);
        Knob<int>* isInt = dynamic_cast<Knob<int>*>(_imp->owner);
        if (isDouble) {
            std::pair<double, double> ret;
            ret.first = isDouble->getMinimum(_imp->dimensionInOwner);
            ret.second = isDouble->getMaximum(_imp->dimensionInOwner);
            return ret;
        } else if (isInt) {
            std::pair<double, double> ret;
            ret.first = isInt->getMinimum(_imp->dimensionInOwner);
            ret.second = isInt->getMaximum(_imp->dimensionInOwner);
            return ret;
        } else {
            return std::make_pair( (double)INT_MIN, (double)INT_MAX );
        }
    }
    assert( hasYRange() );

    return std::make_pair(_imp->yMin, _imp->yMax);
}

double
Curve::clampValueToCurveYRange(double v) const
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

bool
Curve::isAnimated() const
{
    QMutexLocker l(&_imp->_lock);

    // even when there is only one keyframe, there may be tangents!
    return _imp->keyFrames.size() > 0;
}

void
Curve::setXRange(double a,
                 double b)
{
    QMutexLocker l(&_imp->_lock);

    _imp->xMin = a;
    _imp->xMax = b;
}

std::pair<double,double> Curve::getXRange() const
{
    QMutexLocker l(&_imp->_lock);

    return std::make_pair(_imp->xMin, _imp->xMax);
}

int
Curve::getKeyFramesCount() const
{
    QMutexLocker l(&_imp->_lock);
    
    return (int)_imp->keyFrames.size();
}

KeyFrameSet
Curve::getKeyFrames_mt_safe() const
{
    QMutexLocker l(&_imp->_lock);

    return _imp->keyFrames;
}

KeyFrameSet::iterator
Curve::setKeyFrameValueAndTimeNoUpdate(double value,
                                       double time,
                                       KeyFrameSet::iterator k)
{
    // PRIVATE - should not lock
    KeyFrame newKey(*k);

    // nothing special has to be done, since the derivatives are with respect to t
    newKey.setTime(time);
    newKey.setValue(value);
    _imp->keyFrames.erase(k);

    return addKeyFrameNoUpdate(newKey).first;
}

KeyFrame
Curve::setKeyFrameValueAndTime(double time,
                               double value,
                               int index,
                               int* newIndex)
{
    KeyFrame ret;
    {
        QMutexLocker l(&_imp->_lock);
        KeyFrameSet::iterator it = atIndex(index);
        if ( it == _imp->keyFrames.end() ) {
            QString err = QString("No such keyframe at index %1").arg(index);
            throw std::invalid_argument( err.toStdString() );
        }

        bool setTime = ( time != it->getTime() );
        bool setValue = ( value != it->getValue() );

        if (setTime || setValue) {
            it = setKeyFrameValueAndTimeNoUpdate(value,time, it);
            it = evaluateCurveChanged(eCurveChangedReasonKeyframeChanged, it);
        }
        if (newIndex) {
            *newIndex = std::distance(_imp->keyFrames.begin(),it);
        }
        ret = *it;
    }
    return ret;
}

KeyFrame
Curve::setKeyFrameLeftDerivative(double value,
                                 int index,
                                 int* newIndex)
{
    KeyFrame ret;
    {
        QMutexLocker l(&_imp->_lock);
        KeyFrameSet::iterator it = atIndex(index);
        assert( it != _imp->keyFrames.end() );

        if ( value != it->getLeftDerivative() ) {
            KeyFrame newKey(*it);
            newKey.setLeftDerivative(value);
            it = addKeyFrameNoUpdate(newKey).first;
            it = evaluateCurveChanged(eCurveChangedReasonDerivativesChanged,it);
        }
        if (newIndex) {
            *newIndex = std::distance(_imp->keyFrames.begin(),it);
        }
        ret = *it;
    }
    return ret;
}

KeyFrame
Curve::setKeyFrameRightDerivative(double value,
                                  int index,
                                  int* newIndex)
{
    KeyFrame ret;
    {
        QMutexLocker l(&_imp->_lock);
        KeyFrameSet::iterator it = atIndex(index);
        assert( it != _imp->keyFrames.end() );

        if ( value != it->getRightDerivative() ) {
            KeyFrame newKey(*it);
            newKey.setRightDerivative(value);
            it = addKeyFrameNoUpdate(newKey).first;
            it = evaluateCurveChanged(eCurveChangedReasonDerivativesChanged,it);
        }
        if (newIndex) {
            *newIndex = std::distance(_imp->keyFrames.begin(),it);
        }
        ret = *it;
    }
    return ret;
}

KeyFrame
Curve::setKeyFrameDerivatives(double left,
                              double right,
                              int index,
                              int* newIndex)
{
    KeyFrame ret;
    {
        QMutexLocker l(&_imp->_lock);
        KeyFrameSet::iterator it = atIndex(index);
        assert( it != _imp->keyFrames.end() );

        if ( ( left != it->getLeftDerivative() ) || ( right != it->getRightDerivative() ) ) {
            KeyFrame newKey(*it);
            newKey.setLeftDerivative(left);
            newKey.setRightDerivative(right);
            it = addKeyFrameNoUpdate(newKey).first;
            it = evaluateCurveChanged(eCurveChangedReasonDerivativesChanged,it);
        }
        if (newIndex) {
            *newIndex = std::distance(_imp->keyFrames.begin(),it);
        }
        ret = *it;
    }

    return ret;
}

KeyFrame
Curve::setKeyFrameInterpolation(Natron::KeyframeTypeEnum interp,
                                int index,
                                int* newIndex)
{
    KeyFrame ret;
    {
        QMutexLocker l(&_imp->_lock);
        KeyFrameSet::iterator it = atIndex(index);
        assert( it != _imp->keyFrames.end() );

        ///if the curve is a string_curve or bool_curve the interpolation is bound to be constant.
        if ( ( (_imp->type == CurvePrivate::eCurveTypeString) || (_imp->type == CurvePrivate::eCurveTypeBool) ||
               ( _imp->type == CurvePrivate::eCurveTypeIntConstantInterp) ) && ( interp != Natron::eKeyframeTypeConstant) ) {
            return *it;
        }


        if ( interp != it->getInterpolation() ) {
            it = setKeyframeInterpolation_internal(it, interp);
        }
        if (newIndex) {
            *newIndex = std::distance(_imp->keyFrames.begin(),it);
        }
        ret = *it;
    }


    return ret;
}

void
Curve::setCurveInterpolation(Natron::KeyframeTypeEnum interp)
{

    {
        QMutexLocker l(&_imp->_lock);
        ///if the curve is a string_curve or bool_curve the interpolation is bound to be constant.
        if ( ( (_imp->type == CurvePrivate::eCurveTypeString) || (_imp->type == CurvePrivate::eCurveTypeBool) ||
               ( _imp->type == CurvePrivate::eCurveTypeIntConstantInterp) ) && ( interp != Natron::eKeyframeTypeConstant) ) {
            return;
        }
        for (int i = 0; i < (int)_imp->keyFrames.size(); ++i) {
            KeyFrameSet::iterator it = _imp->keyFrames.begin();
            std::advance(it, i);
            if ( interp != it->getInterpolation() ) {
                it = setKeyframeInterpolation_internal(it, interp);
            }
        }
    }

}

KeyFrameSet::iterator
Curve::setKeyframeInterpolation_internal(KeyFrameSet::iterator it,
                                         Natron::KeyframeTypeEnum interp)
{
    ///private should not be locked
    assert( it != _imp->keyFrames.end() );
    KeyFrame newKey(*it);
    newKey.setInterpolation(interp);
    it = addKeyFrameNoUpdate(newKey).first;
    it = evaluateCurveChanged(eCurveChangedReasonKeyframeChanged, it);

    return it;
}

KeyFrameSet::iterator
Curve::refreshDerivatives(Curve::CurveChangedReasonEnum reason,
                          KeyFrameSet::iterator key)
{
    // PRIVATE - should not lock
    double tcur = key->getTime();
    double vcur = key->getValue();
    double tprev, vprev, tnext, vnext, vprevDerivRight, vnextDerivLeft;
    Natron::KeyframeTypeEnum prevType, nextType;

    assert(key != _imp->keyFrames.end());
    if ( key == _imp->keyFrames.begin() ) {
        tprev = tcur;
        vprev = vcur;
        vprevDerivRight = 0.;
        prevType = Natron::eKeyframeTypeNone;
    } else {
        KeyFrameSet::const_iterator prev = key;
        if (prev != _imp->keyFrames.begin()) {
            --prev;
        }
        tprev = prev->getTime();
        vprev = prev->getValue();
        vprevDerivRight = prev->getRightDerivative();
        prevType = prev->getInterpolation();

        //if prev is the first keyframe, and not edited by the user then interpolate linearly
        if ( ( prev == _imp->keyFrames.begin() ) && (prevType != Natron::eKeyframeTypeFree) &&
             ( prevType != Natron::eKeyframeTypeBroken) ) {
            prevType = Natron::eKeyframeTypeLinear;
        }
    }

    KeyFrameSet::const_iterator next = key;
    if (next != _imp->keyFrames.end()) {
        ++next;
    }
    if ( next == _imp->keyFrames.end() ) {
        tnext = tcur;
        vnext = vcur;
        vnextDerivLeft = 0.;
        nextType = Natron::eKeyframeTypeNone;
    } else {
        tnext = next->getTime();
        vnext = next->getValue();
        vnextDerivLeft = next->getLeftDerivative();
        nextType = next->getInterpolation();

        KeyFrameSet::const_iterator nextnext = next;
        if (nextnext != _imp->keyFrames.end()) {
            ++nextnext;
        }
        //if next is thelast keyframe, and not edited by the user then interpolate linearly
        if ( ( nextnext == _imp->keyFrames.end() ) && (nextType != Natron::eKeyframeTypeFree) &&
             ( nextType != Natron::eKeyframeTypeBroken) ) {
            nextType = Natron::eKeyframeTypeLinear;
        }
    }

    double vcurDerivLeft,vcurDerivRight;

    assert(key != _imp->keyFrames.end() &&
           key->getInterpolation() != Natron::eKeyframeTypeNone &&
           key->getInterpolation() != Natron::eKeyframeTypeBroken &&
           key->getInterpolation() != Natron::eKeyframeTypeFree);
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

    if (reason != eCurveChangedReasonDerivativesChanged) {
        key = evaluateCurveChanged(eCurveChangedReasonDerivativesChanged,key);
    }

    return key;
} // refreshDerivatives

KeyFrameSet::iterator
Curve::evaluateCurveChanged(CurveChangedReasonEnum reason,
                            KeyFrameSet::iterator key)
{
    // PRIVATE - should not lock
    assert( key != _imp->keyFrames.end() );

    if ( (key->getInterpolation() != Natron::eKeyframeTypeBroken) && (key->getInterpolation() != Natron::eKeyframeTypeFree)
         && ( reason != eCurveChangedReasonDerivativesChanged) ) {
        key = refreshDerivatives(eCurveChangedReasonDerivativesChanged,key);
    }
    KeyFrameSet::iterator prev = key;
    if (prev != _imp->keyFrames.begin()) {
        --prev;
    }
    if ( key != _imp->keyFrames.begin() ) {
        if ( (prev->getInterpolation() != Natron::eKeyframeTypeBroken) &&
             ( prev->getInterpolation() != Natron::eKeyframeTypeFree) &&
             ( prev->getInterpolation() != Natron::eKeyframeTypeNone) ) {
            prev = refreshDerivatives(eCurveChangedReasonDerivativesChanged,prev);
        }
    }
    KeyFrameSet::iterator next = key;
    if (next != _imp->keyFrames.end()) {
        ++next;
    }
    if ( next != _imp->keyFrames.end() ) {
        if ( (next->getInterpolation() != Natron::eKeyframeTypeBroken) &&
             ( next->getInterpolation() != Natron::eKeyframeTypeFree) &&
             ( next->getInterpolation() != Natron::eKeyframeTypeNone) ) {
            next = refreshDerivatives(eCurveChangedReasonDerivativesChanged,next);
        }
    }
    onCurveChanged();
    return key;
}

KeyFrameSet::const_iterator
Curve::findWithTime(const KeyFrameSet& keys,double time)
{
    return std::find_if( keys.begin(), keys.end(), KeyFrameTimePredicate(time) );
}

KeyFrameSet::const_iterator
Curve::find(double time) const
{
    // PRIVATE - should not lock
    return findWithTime(_imp->keyFrames, time);
}

KeyFrameSet::const_iterator
Curve::atIndex(int index) const
{
    // PRIVATE - should not lock
    if ( index >= (int)_imp->keyFrames.size() ) {
        throw std::out_of_range("Curve::removeKeyFrameWithIndex: non-existent keyframe");
    }

    KeyFrameSet::iterator it = _imp->keyFrames.begin();
    std::advance(it, index);

    return it;
}

int
Curve::keyFrameIndex(double time) const
{
    QMutexLocker l(&_imp->_lock);
    int i = 0;
    double paramEps;

    if ( (_imp->xMax != INT_MAX) && (_imp->xMin != INT_MIN) ) {
        paramEps = NATRON_CURVE_X_SPACING_EPSILON * std::abs(_imp->xMax - _imp->xMin);
    } else {
        paramEps = NATRON_CURVE_X_SPACING_EPSILON;
    }
    for (KeyFrameSet::const_iterator it = _imp->keyFrames.begin();
         it != _imp->keyFrames.end() && (it->getTime() < time + paramEps);
         ++it, ++i) {
        if (std::abs(it->getTime() - time) < paramEps) {
            return i;
        }
    }

    return -1;
}

KeyFrameSet::const_iterator
Curve::begin() const
{
    // PRIVATE - should not lock
    return _imp->keyFrames.begin();
}

KeyFrameSet::const_iterator
Curve::end() const
{
    // PRIVATE - should not lock
    return _imp->keyFrames.end();
}

bool
Curve::isYComponentMovable() const
{
    QMutexLocker l(&_imp->_lock);

    return _imp->type != CurvePrivate::eCurveTypeString;
}

bool
Curve::areKeyFramesValuesClampedToIntegers() const
{
    QMutexLocker l(&_imp->_lock);

    return _imp->type == CurvePrivate::eCurveTypeInt;
}

bool
Curve::areKeyFramesValuesClampedToBooleans() const
{
    QMutexLocker l(&_imp->_lock);

    return _imp->type == CurvePrivate::eCurveTypeBool;
}

void
Curve::setYRange(double yMin,
                 double yMax)
{
    QMutexLocker l(&_imp->_lock);

    _imp->yMin = yMin;
    _imp->yMax = yMax;
    _imp->hasYRange = true;
}

bool
Curve::hasYRange() const
{
    // PRIVATE - should not lock
    return _imp->hasYRange;
}

bool
Curve::mustClamp() const
{
    // PRIVATE - should not lock
    return _imp->owner || hasYRange();
}

void
Curve::onCurveChanged()
{
    // PRIVATE - should not lock
    if (_imp->owner) {
        _imp->owner->clearExpressionsResults(_imp->dimensionInOwner);
    }
    _imp->resultCache.clear();
}
