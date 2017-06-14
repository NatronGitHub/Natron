/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2017 INRIA and Alexandre Gauthier-Foichat
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
#include <cassert>
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
#include "Serialization/CurveSerialization.h"
#include "Engine/Smooth1D.h"

NATRON_NAMESPACE_ENTER;


NATRON_NAMESPACE_ANONYMOUS_ENTER

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

NATRON_NAMESPACE_ANONYMOUS_EXIT


/************************************KEYFRAME************************************/

KeyFrame::KeyFrame()
    : _time(0.)
    , _value(0.)
    , _leftDerivative(0.)
    , _rightDerivative(0.)
    , _interpolation(eKeyframeTypeSmooth)
{
}

KeyFrame::KeyFrame(double time,
                   double initialValue,
                   double leftDerivative,
                   double rightDerivative,
                   KeyframeTypeEnum interpolation)
    : _time(time)
    , _value(initialValue)
    , _leftDerivative(leftDerivative)
    , _rightDerivative(rightDerivative)
    , _interpolation(interpolation)
{
    assert( !boost::math::isnan(_time) && !boost::math::isinf(_time) );
    assert( !boost::math::isnan(_value) && !boost::math::isinf(_value) );
    assert( !boost::math::isnan(_leftDerivative) && !boost::math::isinf(_leftDerivative) );
    assert( !boost::math::isnan(_rightDerivative) && !boost::math::isinf(_rightDerivative) );
}

KeyFrame::KeyFrame(const KeyFrame & other)
    : _time(other._time)
    , _value(other._value)
    , _leftDerivative(other._leftDerivative)
    , _rightDerivative(other._rightDerivative)
    , _interpolation(other._interpolation)
{
    assert( !boost::math::isnan(_time) && !boost::math::isinf(_time) );
    assert( !boost::math::isnan(_value) && !boost::math::isinf(_value) );
    assert( !boost::math::isnan(_leftDerivative) && !boost::math::isinf(_leftDerivative) );
    assert( !boost::math::isnan(_rightDerivative) && !boost::math::isinf(_rightDerivative) );
}

void
KeyFrame::operator=(const KeyFrame & o)
{
    _time = o._time;
    _value = o._value;
    _leftDerivative = o._leftDerivative;
    _rightDerivative = o._rightDerivative;
    _interpolation = o._interpolation;
    assert( !boost::math::isnan(_time) && !boost::math::isinf(_time) );
    assert( !boost::math::isnan(_value) && !boost::math::isinf(_value) );
    assert( !boost::math::isnan(_leftDerivative) && !boost::math::isinf(_leftDerivative) );
    assert( !boost::math::isnan(_rightDerivative) && !boost::math::isinf(_rightDerivative) );
}

KeyFrame::~KeyFrame()
{
}

void
KeyFrame::setLeftDerivative(double leftDerivative)
{
    _leftDerivative = leftDerivative;
    assert( !boost::math::isnan(_leftDerivative) && !boost::math::isinf(_leftDerivative) );
}

void
KeyFrame::setRightDerivative(double rightDerivative)
{
    _rightDerivative = rightDerivative;
    assert( !boost::math::isnan(_rightDerivative) && !boost::math::isinf(_rightDerivative) );
}

void
KeyFrame::setValue(double value)
{
    _value = value;
    assert( !boost::math::isnan(_value) && !boost::math::isinf(_value) );
}

void
KeyFrame::setTime(TimeValue time)
{
    _time = time;
    assert( !boost::math::isnan(_time) && !boost::math::isinf(_time) );
}

void
KeyFrame::setInterpolation(KeyframeTypeEnum interp)
{
    _interpolation = interp;
}

KeyframeTypeEnum
KeyFrame::getInterpolation() const
{
    return _interpolation;
}

double
KeyFrame::getValue() const
{
    return _value;
}

TimeValue
KeyFrame::getTime() const
{
    return TimeValue(_time);
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

Curve::Curve(CurveTypeEnum type)
    : SERIALIZATION_NAMESPACE::SerializableObjectBase()
    , _imp(new CurvePrivate())
{
    _imp->type = type;
    if (type == eCurveTypeString) {
        _imp->canMoveY = false;
    }
}


Curve::Curve(const Curve & other)
    : _imp( new CurvePrivate(*other._imp) )
{
}

Curve::~Curve()
{
    clearKeyFrames();
}

Curve::CurveTypeEnum
Curve::getType() const
{
    return _imp->type;
}

void
Curve::setPeriodic(bool periodic)
{
    QMutexLocker k(&_imp->_lock);
    _imp->isPeriodic = periodic;
    _imp->keyFrames.clear();
}

bool
Curve::isCurvePeriodic() const
{
    return _imp->isPeriodic;
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

    return _imp->type != eCurveTypeParametric;
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

void
Curve::cloneIndexRange(const Curve& other, int firstKeyIdx, int nKeys)
{
    KeyFrameSet otherKeys = other.getKeyFrames_mt_safe();
    QMutexLocker l(&_imp->_lock);
    _imp->keyFrames.clear();
    if (firstKeyIdx >= (int)otherKeys.size()) {
        return;
    }
    KeyFrameSet::iterator start = otherKeys.begin();
    std::advance(start, firstKeyIdx);

    KeyFrameSet::iterator end;
    if (nKeys == -1 || (firstKeyIdx + nKeys >= (int)otherKeys.size())) {
        end = otherKeys.end();
    } else {
        end = start;
        std::advance(end, nKeys);
    }
    std::transform( start, end, std::inserter( _imp->keyFrames, _imp->keyFrames.begin() ), KeyFrameCloner() );
    onCurveChanged();

}

bool
Curve::cloneAndCheckIfChanged(const Curve& other, double offset, const RangeD* range)
{
    KeyFrameSet otherKeys = other.getKeyFrames_mt_safe();
    QMutexLocker l(&_imp->_lock);
    bool hasChanged = false;

    if ( otherKeys.size() != _imp->keyFrames.size() ) {
        hasChanged = true;
    }
    if (offset != 0) {
        hasChanged = true;
    }

    KeyFrameSet tmpSet = _imp->keyFrames;
    KeyFrameSet::iterator oit = tmpSet.begin();
    _imp->keyFrames.clear();
    for (KeyFrameSet::iterator it = otherKeys.begin(); it != otherKeys.end(); ++it) {
        TimeValue time = it->getTime();
        if ( range && ( (time < range->min) || (time > range->max) ) ) {
            // We ignore a keyframe, then consider the curve has changed
            hasChanged = true;
            continue;
        }
        KeyFrame k(*it);
        if (offset != 0) {
            k.setTime(TimeValue(time + offset));
        }
        if (!hasChanged && oit != tmpSet.end() && *oit != *it) {
            hasChanged = true;
        }
        _imp->keyFrames.insert(k);

        if (oit != tmpSet.end()) {
            ++oit;
        }
    }
    return hasChanged;
}

void
Curve::clone(const Curve & other,
             double offset,
             const RangeD* range)
{
    KeyFrameSet otherKeys = other.getKeyFrames_mt_safe();
    // The range=[0,0] case is obviously a bug in the spec of paramCopy() from the parameter suite:
    // it prevents copying the value of frame 0.
    bool copyRange = range != NULL /*&& (range->min != 0 || range->max != 0)*/;
    QMutexLocker l(&_imp->_lock);

    _imp->keyFrames.clear();
    for (KeyFrameSet::iterator it = otherKeys.begin(); it != otherKeys.end(); ++it) {
        TimeValue time = it->getTime();
        if ( copyRange && ( (time < range->min) || (time > range->max) ) ) {
            continue;
        }
        KeyFrame k(*it);
        if (offset != 0) {
            k.setTime(TimeValue(time + offset));
        }
        _imp->keyFrames.insert(k);
    }
    onCurveChanged();
}

void
Curve::computeKeyFramesDiff(const KeyFrameSet& keysA,
                            const KeyFrameSet& keysB,
                            std::list<double>* keysAdded,
                            std::list<double>* keysRemoved)
{
    KeyFrameSet keysACopy = keysA;
    if (keysAdded) {
        for (KeyFrameSet::iterator it = keysB.begin(); it != keysB.end(); ++it) {
            KeyFrameSet::iterator foundInOldKeys = Curve::findWithTime(keysACopy, keysACopy.end(), it->getTime());
            if (foundInOldKeys == keysA.end()) {
                keysAdded->push_back(it->getTime());
            } else {
                keysACopy.erase(foundInOldKeys);
            }
        }
    }
    if (keysRemoved) {
        for (KeyFrameSet::iterator it = keysACopy.begin(); it != keysACopy.end(); ++it) {
            KeyFrameSet::iterator foundInNextKeys = Curve::findWithTime(keysB, keysB.end(), it->getTime());
            if (foundInNextKeys == keysB.end()) {
                keysRemoved->push_back(it->getTime());
            }
        }
    }
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

    // the default interpolation for bool, string, choice, int is constant
    if ( (_imp->type == Curve::eCurveTypeBool) || (_imp->type == Curve::eCurveTypeString) ||
         (_imp->type == Curve::eCurveTypeInt) ||
         (_imp->type == Curve::eCurveTypeIntConstantInterp) ) {
        key.setInterpolation(eKeyframeTypeConstant);
    }


    std::pair<KeyFrameSet::iterator, bool> it = addKeyFrameNoUpdate(key);

    it.first = evaluateCurveChanged(eCurveChangedReasonKeyframeChanged, it.first);
    l.unlock();

    return it.second;
}

ValueChangedReturnCodeEnum
Curve::setOrAddKeyframe(KeyFrame key)
{
    if ( (_imp->type == Curve::eCurveTypeBool) || (_imp->type == Curve::eCurveTypeString) ||
        ( _imp->type == Curve::eCurveTypeIntConstantInterp) ) {
        key.setInterpolation(eKeyframeTypeConstant);
    }

    QMutexLocker l(&_imp->_lock);
    std::pair<KeyFrameSet::iterator, bool> it = addKeyFrameNoUpdate(key);
    ValueChangedReturnCodeEnum ret = eValueChangedReturnCodeNothingChanged;
    if (!it.second) {
        if (it.first->getValue() != key.getValue() || it.first->getTime() == key.getTime()) {
            ret = eValueChangedReturnCodeKeyframeModified;
        }
        _imp->keyFrames.erase(it.first);
        it = addKeyFrameNoUpdate(key);
    } else {
        ret = eValueChangedReturnCodeKeyframeAdded;
    }
    assert(it.second);
    it.first = evaluateCurveChanged(eCurveChangedReasonKeyframeChanged, it.first);

    return ret;
    
}

std::pair<KeyFrameSet::iterator, bool> Curve::addKeyFrameNoUpdate(const KeyFrame & cp)
{
    // PRIVATE - should not lock
    if (_imp->type != eCurveTypeParametric) { //< if keyframes are clamped to integers
        std::pair<KeyFrameSet::iterator, bool> newKey = _imp->keyFrames.insert(cp);
        // keyframe at this time exists, erase and insert again
        bool addedKey = true;
        if (!newKey.second) {
            _imp->keyFrames.erase(newKey.first);
            newKey = _imp->keyFrames.insert(cp);
            assert(newKey.second);
            addedKey = false;
        }

        return std::make_pair(newKey.first, addedKey);
    } else {
        bool addedKey = true;
        double paramEps = NATRON_CURVE_X_SPACING_EPSILON;
        for (KeyFrameSet::iterator it = _imp->keyFrames.begin(); it != _imp->keyFrames.end(); ++it) {
            if (std::abs( it->getTime() - cp.getTime() ) < paramEps) {
                _imp->keyFrames.erase(it);
                addedKey = false;
                break;
            }
        }
        std::pair<KeyFrameSet::iterator, bool> newKey = _imp->keyFrames.insert(cp);
        newKey.second = addedKey;

        return newKey;
    }
}

void
Curve::removeKeyFrame(KeyFrameSet::const_iterator it)
{
    // PRIVATE - should not lock
    assert(it != _imp->keyFrames.end());
    if (it == _imp->keyFrames.end()) {
        return;
    }
    KeyFrame prevKey;
    bool mustRefreshPrev = false;
    KeyFrame nextKey;
    bool mustRefreshNext = false;


    KeyFrameSet::iterator next = it;
    ++next;
    if ( next != _imp->keyFrames.end() ) {
        ++next;
    }

    // If the curve is periodic, it has a keyframe before if it is no
    if ( it != _imp->keyFrames.begin() || (_imp->isPeriodic && _imp->keyFrames.size() > 2)) {
        bool prevKeySet = false;
        if (it != _imp->keyFrames.begin()) {
            KeyFrameSet::iterator prev = it;
            if ( prev != _imp->keyFrames.begin() ) {
                --prev;
            }
            if ( prev != _imp->keyFrames.end() ) {
                prevKey = *prev;
                prevKeySet = true;
            }
        } else {
            KeyFrameSet::reverse_iterator end = _imp->keyFrames.rbegin();
            ++end;
            if ( end != _imp->keyFrames.rend() ) {
                prevKey = *end;
                prevKeySet = true;
            }
        }
        mustRefreshPrev = (prevKeySet &&
                           prevKey.getInterpolation() != eKeyframeTypeBroken &&
                           prevKey.getInterpolation() != eKeyframeTypeFree &&
                           prevKey.getInterpolation() != eKeyframeTypeNone);
    }

    if ( next != _imp->keyFrames.end() || (_imp->isPeriodic && _imp->keyFrames.size() > 2)) {
        bool nextKeySet = false;
        if (next != _imp->keyFrames.end()) {
            nextKey = *next;
            nextKeySet = true;
        } else {
            KeyFrameSet::iterator start = _imp->keyFrames.begin();
            ++start;
            if ( start != _imp->keyFrames.end() ) {
                nextKey = *start;
                nextKeySet = true;
            }
        }
        mustRefreshNext = (nextKeySet &&
                           nextKey.getInterpolation() != eKeyframeTypeBroken &&
                           nextKey.getInterpolation() != eKeyframeTypeFree &&
                           nextKey.getInterpolation() != eKeyframeTypeNone);
    }

    _imp->keyFrames.erase(it);

    if (mustRefreshPrev) {
        refreshDerivatives( eCurveChangedReasonDerivativesChanged, find( prevKey.getTime(), _imp->keyFrames.end()) );
    }
    if (mustRefreshNext) {
        refreshDerivatives( eCurveChangedReasonDerivativesChanged, find( nextKey.getTime(), _imp->keyFrames.end() ) );
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
Curve::removeKeyFrameWithTime(TimeValue time)
{
    QMutexLocker l(&_imp->_lock);
    KeyFrameSet::iterator it = find(time, _imp->keyFrames.end());

    if ( it == _imp->keyFrames.end() ) {
        throw std::out_of_range("Curve::removeKeyFrameWithTime: non-existent keyframe");
    }

    removeKeyFrame(it);
}

void
Curve::removeKeyFramesBeforeTime(TimeValue time,
                                 std::list<double>* keyframeRemoved)
{
    KeyFrameSet newSet;
    QMutexLocker l(&_imp->_lock);

    for (KeyFrameSet::iterator it = _imp->keyFrames.begin(); it != _imp->keyFrames.end(); ++it) {
        if (it->getTime() < time) {
            if (keyframeRemoved) {
                keyframeRemoved->push_back( it->getTime() );
            }
            continue;
        }
        newSet.insert(*it);
    }
    _imp->keyFrames = newSet;
    if ( !_imp->keyFrames.empty() ) {
        refreshDerivatives( Curve::eCurveChangedReasonKeyframeChanged, _imp->keyFrames.begin() );
    }
    onCurveChanged();
}

void
Curve::removeKeyFramesAfterTime(TimeValue time,
                                std::list<double>* keyframeRemoved)
{
    KeyFrameSet newSet;
    QMutexLocker l(&_imp->_lock);

    for (KeyFrameSet::iterator it = _imp->keyFrames.begin(); it != _imp->keyFrames.end(); ++it) {
        if (it->getTime() > time) {
            if (keyframeRemoved) {
                keyframeRemoved->push_back( it->getTime() );
            }
            continue;
        }
        newSet.insert(*it);
    }
    _imp->keyFrames = newSet;
    if ( !_imp->keyFrames.empty() ) {
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
    if ( (index < 0) || ( (int)_imp->keyFrames.size() <= index ) ) {
        return false;
    }
    *k = *atIndex(index);

    return true;
}

bool
Curve::getNearestKeyFrameWithTime(TimeValue time,
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
    if ( lower == _imp->keyFrames.end() ) {
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
    if ( upper == _imp->keyFrames.end() ) {
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
Curve::getPreviousKeyframeTime(TimeValue time,
                               KeyFrame* k) const
{
    assert(k);
    QMutexLocker l(&_imp->_lock);
    if ( _imp->keyFrames.empty() ) {
        return false;
    }
    KeyFrame tmp;
    tmp.setTime(time);
    KeyFrameSet::const_iterator lowerBound = _imp->keyFrames.lower_bound(tmp);
    if (lowerBound == _imp->keyFrames.end() || lowerBound == _imp->keyFrames.begin()) {
        return false;
    }
    --lowerBound;
    *k = *lowerBound;
    return true;
}

bool
Curve::getNextKeyframeTime(TimeValue time,
                           KeyFrame* k) const
{
    assert(k);
    QMutexLocker l(&_imp->_lock);
    if ( _imp->keyFrames.empty() ) {
        return false;
    }
    KeyFrame tmp;
    tmp.setTime(time);
    KeyFrameSet::const_iterator upper = _imp->keyFrames.upper_bound(tmp);
    if ( upper == _imp->keyFrames.end() ) {
        return false;
    }
    *k = *upper;
    return true;
}

int
Curve::getNKeyFramesInRange(double first,
                            double last) const
{
    int ret = 0;
    QMutexLocker k(&_imp->_lock);
    KeyFrameSet::const_iterator upper = _imp->keyFrames.end();

    for (KeyFrameSet::const_iterator it = _imp->keyFrames.begin(); it != _imp->keyFrames.end(); ++it) {
        if (it->getTime() >= first) {
            upper = it;
            break;
        }
    }
    for (; upper != _imp->keyFrames.end(); ++upper) {
        if (upper->getTime() >= last) {
            break;
        }
        ++ret;
    }

    return ret;
}

bool
Curve::getKeyFrameWithTime(TimeValue time,
                           KeyFrame* k) const
{
    assert(k);
    QMutexLocker l(&_imp->_lock);
    KeyFrameSet::const_iterator it = find(time, _imp->keyFrames.end());

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
            bool isPeriodic,
            double xMin,
            double xMax,
            TimeValue *t,
            KeyFrameSet::const_iterator itup,
            TimeValue *tcur,
            double *vcur,
            double *vcurDerivRight,
            KeyframeTypeEnum *interp,
            TimeValue *tnext,
            double *vnext,
            double *vnextDerivLeft,
            KeyframeTypeEnum *interpNext)
{

    assert(keyFrames.size() >= 1);
    assert( itup == keyFrames.end() || *t < itup->getTime() );
    double period = xMax - xMin;
    if (isPeriodic) {
        // if the curve is periodic, bring back t in the curve keyframes range
        double minKeyFrameX = keyFrames.begin()->getTime() + xMin;
        assert(xMin < xMax);
        if (*t < minKeyFrameX || *t > minKeyFrameX + period) {
            // This will bring t either in minTime <= t <= maxTime or t in the range minTime - (maxTime - minTime) < t < minTime
            *t = TimeValue(std::fmod(*t - minKeyFrameX, period ) + minKeyFrameX);
            if (*t < minKeyFrameX) {
                *t = TimeValue(*t + period);
            }
            assert(*t >= minKeyFrameX && *t <= minKeyFrameX + period);
        }
        itup = keyFrames.upper_bound(KeyFrame(*t, 0.));
    }
    if ( itup == keyFrames.begin() ) {
        // We are in the case where all keys have a greater time
        // If periodic, we are in between xMin and the first keyframe
        if (isPeriodic) {
            *tnext = itup->getTime();
            *vnext = itup->getValue();
            *vnextDerivLeft = itup->getLeftDerivative();
            *interpNext = itup->getInterpolation();
            KeyFrameSet::const_reverse_iterator last =  keyFrames.rbegin();
            *tcur = TimeValue(last->getTime() - period);
            *vcur = last->getValue();
            *vcurDerivRight = last->getRightDerivative();
            *interp = last->getInterpolation();
        } else {
            *tnext = itup->getTime();
            *vnext = itup->getValue();
            *vnextDerivLeft = itup->getLeftDerivative();
            *interpNext = itup->getInterpolation();
            *tcur = TimeValue(*tnext - 1.);
            *vcur = *vnext;
            *vcurDerivRight = 0.;
            *interp = eKeyframeTypeNone;
        }

    } else if ( itup == keyFrames.end() ) {

        // We are in the case where no key has a greater time
        // If periodic, we are in-between the last keyframe and xMax
        if (isPeriodic) {
            KeyFrameSet::const_iterator next = keyFrames.begin();
            KeyFrameSet::const_reverse_iterator prev = keyFrames.rbegin();
            *tcur = prev->getTime();
            *vcur = prev->getValue();
            *vcurDerivRight = prev->getRightDerivative();
            *interp = prev->getInterpolation();
            *tnext = TimeValue(next->getTime() + period);
            *vnext = next->getValue();
            *vnextDerivLeft = next->getLeftDerivative();
            *interpNext = next->getInterpolation();
        } else {

            KeyFrameSet::const_reverse_iterator itlast = keyFrames.rbegin();
            *tcur = itlast->getTime();
            *vcur = itlast->getValue();
            *vcurDerivRight = itlast->getRightDerivative();
            *interp = itlast->getInterpolation();
            *tnext = TimeValue(*tcur + 1.);
            *vnext = *vcur;
            *vnextDerivLeft = 0.;
            *interpNext = eKeyframeTypeNone;
        }
        
    } else {
        // between two keyframes
        // get the last keyframe with time <= t
        KeyFrameSet::const_iterator itcur = itup;
        --itcur;
        assert(itcur->getTime() <= *t);
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
Curve::getValueAt(TimeValue t,
                  bool doClamp) const
{
    QMutexLocker l(&_imp->_lock);

    if ( _imp->keyFrames.empty() ) {
        //throw std::runtime_error("Curve has no control points!");

        // A curve with no control points is considered to be 0
        // this is to avoid returning StatFailed when KnobParametric::getValue() is called on a parametric curve without control point.
        return 0.;

        // There is no special case for a curve with one (1) keyframe: the result is a linear curve before and after the keyframe.
    //} else if (_imp->keyFrames.size() == 1) {
    //    return _imp->keyFrames.begin()->getValue();
    }

    double v;
#ifdef NATRON_CURVE_USE_CACHE
    std::map<double, double>::const_iterator foundCached = _imp->resultCache.find(t);
    if ( foundCached != _imp->resultCache.end() ) {
        v = foundCached->second;
    } else
#endif
    {
        // even when there is only one keyframe, there may be tangents!
        //if (_imp->keyFrames.size() == 1) {
        //    //if there's only 1 keyframe, don't bother interpolating
        //    return (*_imp->keyFrames.begin()).getValue();
        //}
        TimeValue tcur, tnext;
        double vcurDerivRight, vnextDerivLeft, vcur, vnext;
        KeyframeTypeEnum interp, interpNext;
        KeyFrame k(t, 0.);
        // find the first keyframe with time greater than t
        KeyFrameSet::const_iterator itup;
        itup = _imp->keyFrames.upper_bound(k);
        interParams(_imp->keyFrames,
                    _imp->isPeriodic,
                    _imp->xMin,
                    _imp->xMax,
                    &t,
                    itup,
                    &tcur,
                    &vcur,
                    &vcurDerivRight,
                    &interp,
                    &tnext,
                    &vnext,
                    &vnextDerivLeft,
                    &interpNext);

        v = Interpolation::interpolate(tcur, vcur,
                                       vcurDerivRight,
                                       vnextDerivLeft,
                                       tnext, vnext,
                                       t,
                                       interp,
                                       interpNext);
#ifdef NATRON_CURVE_USE_CACHE
        _imp->resultCache[t] = v;
#endif
    }

    if ( doClamp ) {
        v = clampValueToCurveYRange(v);
    }

    switch (_imp->type) {
    case Curve::eCurveTypeString:
    case Curve::eCurveTypeInt:

        return std::floor(v + 0.5);
    case Curve::eCurveTypeBool:

        return v >= 0.5 ? 1. : 0.;
    default:

        return v;
    }
} // getValueAt

double
Curve::getDerivativeAt(TimeValue t) const
{
    QMutexLocker l(&_imp->_lock);

    if ( _imp->keyFrames.empty() ) {
        throw std::runtime_error("Curve has no control points!");
    }
    assert(_imp->type == Curve::eCurveTypeDouble); // only real-valued curves can be derived

    // even when there is only one keyframe, there may be tangents!
    //if (_imp->keyFrames.size() == 1) {
    //    //if there's only 1 keyframe, don't bother interpolating
    //    return (*_imp->keyFrames.begin()).getValue();
    //}
    TimeValue tcur, tnext;
    double vcurDerivRight, vnextDerivLeft, vcur, vnext;
    KeyframeTypeEnum interp, interpNext;
    KeyFrame k(t, 0.);
    // find the first keyframe with time greater than t
    KeyFrameSet::const_iterator itup;
    itup = _imp->keyFrames.upper_bound(k);
    interParams(_imp->keyFrames,
                _imp->isPeriodic,
                _imp->xMin,
                _imp->xMax,
                &t,
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

    if ( _imp->yMin != -std::numeric_limits<double>::infinity() || _imp->yMax != std::numeric_limits<double>::infinity()) {
        d = Interpolation::derive_clamp(tcur, vcur,
                                        vcurDerivRight,
                                        vnextDerivLeft,
                                        tnext, vnext,
                                        t,
                                        _imp->yMin, _imp->yMax,
                                        interp,
                                        interpNext);
    } else {
        d = Interpolation::derive(tcur, vcur,
                                  vcurDerivRight,
                                  vnextDerivLeft,
                                  tnext, vnext,
                                  t,
                                  interp,
                                  interpNext);
    }

    return d;
} // getDerivativeAt

double
Curve::getIntegrateFromTo(TimeValue t1,
                          TimeValue t2) const
{
    QMutexLocker l(&_imp->_lock);
    bool opposite = false;

    // the following assumes that t2 > t1. If it's not the case, swap them and return the opposite.
    if (t1 > t2) {
        opposite = true;
        std::swap(t1, t2);
    }

    if ( _imp->keyFrames.empty() ) {
        throw std::runtime_error("Curve has no control points!");
    }
    assert(_imp->type == Curve::eCurveTypeDouble); // only real-valued curves can be derived

    // even when there is only one keyframe, there may be tangents!
    //if (_imp->keyFrames.size() == 1) {
    //    //if there's only 1 keyframe, don't bother interpolating
    //    return (*_imp->keyFrames.begin()).getValue();
    //}
    TimeValue tcur, tnext;
    double vcurDerivRight, vnextDerivLeft, vcur, vnext;
    KeyframeTypeEnum interp, interpNext;
    KeyFrame k(t1, 0.);
    // find the first keyframe with time strictly greater than t1
    KeyFrameSet::const_iterator itup;
    itup = _imp->keyFrames.upper_bound(k);
    interParams(_imp->keyFrames,
                _imp->isPeriodic,
                _imp->xMin,
                _imp->xMax,
                &t1,
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
        if ( _imp->yMin != -std::numeric_limits<double>::infinity() || _imp->yMax != std::numeric_limits<double>::infinity()) {
            sum += Interpolation::integrate_clamp(tcur, vcur,
                                                  vcurDerivRight,
                                                  vnextDerivLeft,
                                                  tnext, vnext,
                                                  TimeValue(t1), itup->getTime(),
                                                  _imp->yMin, _imp->yMax,
                                                  interp,
                                                  interpNext);
        } else {
            sum += Interpolation::integrate(tcur, vcur,
                                            vcurDerivRight,
                                            vnextDerivLeft,
                                            tnext, vnext,
                                            TimeValue(t1), itup->getTime(),
                                            interp,
                                            interpNext);
        }
        // advance
        t1 = itup->getTime();
        ++itup;
        interParams(_imp->keyFrames,
                    _imp->isPeriodic,
                    _imp->xMin,
                    _imp->xMax,
                    &t1,
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
    if ( _imp->yMin != -std::numeric_limits<double>::infinity() || _imp->yMax != std::numeric_limits<double>::infinity()) {
        sum += Interpolation::integrate_clamp(tcur, vcur,
                                              vcurDerivRight,
                                              vnextDerivLeft,
                                              tnext, vnext,
                                              TimeValue(t1), TimeValue(t2),
                                              _imp->yMin, _imp->yMax,
                                              interp,
                                              interpNext);
    } else {
        sum += Interpolation::integrate(tcur, vcur,
                                        vcurDerivRight,
                                        vnextDerivLeft,
                                        tnext, vnext,
                                        TimeValue(t1), TimeValue(t2),
                                        interp,
                                        interpNext);
    }

    return opposite ? -sum : sum;
} // getIntegrateFromTo

Curve::YRange
Curve::getCurveDisplayYRange() const
{
    QMutexLocker l(&_imp->_lock);
    return YRange(_imp->displayMin, _imp->displayMax);
}

Curve::YRange Curve::getCurveYRange() const
{
    QMutexLocker l(&_imp->_lock);
    return YRange(_imp->yMin, _imp->yMax);
}

double
Curve::clampValueToCurveYRange(double v) const
{
    // PRIVATE - should not lock
    ////clamp to min/max if the owner of the curve is a Double or Int knob.
    if (v > _imp->yMax) {
        return _imp->yMax;
    } else if (v < _imp->yMin) {
        return _imp->yMin;
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

std::pair<double, double> Curve::getXRange() const
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
                                       TimeValue time,
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
Curve::setKeyFrameValueAndTimeInternal(TimeValue time, double value, int index, int* newIndex)
{
    KeyFrame ret;
    {
        QMutexLocker l(&_imp->_lock);
        KeyFrameSet::iterator it = atIndex(index);
        if ( it == _imp->keyFrames.end() ) {
            QString err = QString( QString::fromUtf8("No such keyframe at index %1") ).arg(index);
            throw std::invalid_argument( err.toStdString() );
        }

        bool setTime = ( time != it->getTime() );
        bool setValue = ( value != it->getValue() );

        if (setTime || setValue) {
            it = setKeyFrameValueAndTimeNoUpdate(value, time, it);
            it = evaluateCurveChanged(eCurveChangedReasonKeyframeChanged, it);
        }
        if (newIndex) {
            *newIndex = std::distance(_imp->keyFrames.begin(), it);
        }
        ret = *it;
    }
    return ret;
}


KeyFrame
Curve::setKeyFrameValueAndTime(TimeValue time,
                               double value,
                               int index,
                               int* newIndex)
{
    return setKeyFrameValueAndTimeInternal(time, value, index, newIndex);
}


bool
Curve::transformKeyframesValueAndTime(const std::list<double>& times,
                                      const KeyFrameWarp& warp,
                                      std::vector<KeyFrame>* newKeys,
                                      std::list<double>* keysAddedOut,
                                      std::list<double>* keysRemovedOut)
{
    if (times.empty()) {
        return true;
    }
    if (warp.isIdentity()) {
        if (newKeys) {
            for (std::list<double>::const_iterator it = times.begin(); it != times.end(); ++it) {
                KeyFrame k;
                if (getKeyFrameWithTime(TimeValue(*it), &k)) {
                    newKeys->push_back(k);
                }
            }
        }
        return true;
    }


    QMutexLocker l(&_imp->_lock);

    // First compute all transformed keyframes
    std::list<double>::const_iterator next = times.begin();
    ++next;
    
    // We remove progressively for each keyframe warped the corresponding original keyframe
    // to get in this set only the keyframes that were not modified by this function call
    // We use a list here because the erase() function in C++98 does not return an iterator
    // and we need it to be efficient
    std::list<KeyFrame> originalKeyFramesMinusTransformedKeyFrames;
    for (KeyFrameSet::const_iterator it = _imp->keyFrames.begin();
         it != _imp->keyFrames.end();
         ++it) {
        originalKeyFramesMinusTransformedKeyFrames.push_back(*it);
    }
    
    // The set containing warped keyframes
    KeyFrameSet warpedKeyFrames;
    
    // To speed up findWithTime search, taking advantage of the fact that input times are ordered
    {
        std::list<KeyFrame>::iterator findHint = originalKeyFramesMinusTransformedKeyFrames.end();
        for (std::list<double>::const_iterator it = times.begin();
             it!=times.end();
             ++it) {
            // Ensure the user passed increasing sorted keyframes
            assert(next == times.end() || *next > *it);
            if (next != times.end() && *next <= *it) {
                throw std::invalid_argument("Curve::transformKeyframesValueAndTime: input keyframe times to transform were not sorted by increasing order by the caller");
            }

            // Find in the original keyframes
            std::list<KeyFrame>::iterator found = originalKeyFramesMinusTransformedKeyFrames.end();
            {
                // Since input keyframe times are sorted, we don't have to find before the time that was found
                // at the previous iteration
                std::list<KeyFrame>::iterator findStart = ( ( findHint == originalKeyFramesMinusTransformedKeyFrames.end() ) ?
                                                           originalKeyFramesMinusTransformedKeyFrames.begin() :
                                                           findHint );
                found = std::find_if(findStart, originalKeyFramesMinusTransformedKeyFrames.end(), KeyFrameTimePredicate(*it));
            }

            if (found == originalKeyFramesMinusTransformedKeyFrames.end()) {
                // It can fail here because the time provided by the user does not exist in the original keyframes
                // or if the same keyframe time was passed multiple times in the input times list.
                return false;
            }

            // Apply warp
            KeyFrame warpedKey = warp.applyForwardWarp(*found);

            {
                std::pair<KeyFrameSet::iterator,bool> insertOk = warpedKeyFrames.insert(warpedKey);
                if (!insertOk.second) {
                    // 2 input keyframes were warped to the same point
                    return false;
                }
            }

            // Remove from the set to get a diff of keyframes untouched by the warp operation
            findHint = originalKeyFramesMinusTransformedKeyFrames.erase(found);
            
            if (next != times.end()) {
                ++next;
            }
        } // for all times
    }
    
    KeyFrameSet finalSet;
    
    // Insert in the final set the original keyframes we did not transform
    for (std::list<KeyFrame>::const_iterator it = originalKeyFramesMinusTransformedKeyFrames.begin();
         it != originalKeyFramesMinusTransformedKeyFrames.end();
         ++it) {
        finalSet.insert(*it);
    }
    
    if (newKeys) {
        newKeys->resize(warpedKeyFrames.size());
    }
    
    // Now make up the final keyframe set by merging originalKeyFramesMinusTransformedKeyFrames and warpedKeyFrames
    {
        int i = 0;
        for (KeyFrameSet::const_iterator it = warpedKeyFrames.begin();
             it != warpedKeyFrames.end();
             ++it, ++i) {
            std::pair<KeyFrameSet::iterator,bool> insertOk = finalSet.insert(*it);
            // A warped keyframe overlap an original keyframe left in the set, fail operation
            if (!insertOk.second) {
                return false;
            }
            // If user want output keys, provide them
            if (newKeys) {
                (*newKeys)[i] = (*it);
            }
        }
    }
    
    
    // Make sure we did not overlap any keyframe.
    assert(finalSet.size() == _imp->keyFrames.size());
    
    // Compute keyframes added if needed
    if (keysAddedOut) {
        // Keyframes added are those in the final set that are not in the original set
        KeyFrameSet::const_iterator findHint = _imp->keyFrames.end();
        for (KeyFrameSet::const_iterator it = finalSet.begin();
             it != finalSet.end();
             ++it) {
            // Find in the original keyframes
            findHint = findWithTime(_imp->keyFrames, findHint, it->getTime());
            if (findHint == _imp->keyFrames.end()) {
                keysAddedOut->push_back(it->getTime());
            }
        }
    } // keysAddedOut
    
    // Compute keyframes removed if needed
    if (keysRemovedOut) {
        // Keyframes removed are those in the original set that are not in the final set
        KeyFrameSet::const_iterator findHint = finalSet.end();
        for (KeyFrameSet::const_iterator it = _imp->keyFrames.begin();
             it != _imp->keyFrames.end();
             ++it) {
            // Find in the original keyframes
            findHint = findWithTime(finalSet, findHint, it->getTime());
            if (findHint == finalSet.end()) {
                keysRemovedOut->push_back(it->getTime());
            }
        }
    } // keysRemovedOut
    
    // Now move finalSet to the member keyframes
    _imp->keyFrames.clear();
    for (KeyFrameSet::const_iterator it = finalSet.begin();
         it != finalSet.end();
         ++it) {
        
        std::pair<KeyFrameSet::iterator, bool> ret = addKeyFrameNoUpdate(*it);
        
        // huh, we are just moving one set to another, there cannot be a failure!
        assert(ret.second);
        
        // refresh derivative
        ret.first = evaluateCurveChanged(eCurveChangedReasonKeyframeChanged, ret.first);
        
    }
    return true;
} // transformKeyframesValueAndTime


KeyFrame
Curve::setKeyFrameLeftDerivativeInternal(double value, int index, int* newIndex)
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
            it = evaluateCurveChanged(eCurveChangedReasonDerivativesChanged, it);
        }
        if (newIndex) {
            *newIndex = std::distance(_imp->keyFrames.begin(), it);
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
    return setKeyFrameLeftDerivativeInternal(value, index, newIndex);
}

KeyFrame
Curve::setKeyFrameRightDerivativeInternal(double value, int index, int* newIndex)
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
            it = evaluateCurveChanged(eCurveChangedReasonDerivativesChanged, it);
        }
        if (newIndex) {
            *newIndex = std::distance(_imp->keyFrames.begin(), it);
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
    return setKeyFrameRightDerivativeInternal(value, index, newIndex);
}

KeyFrame
Curve::setKeyFrameDerivativesInternal(double left, double right, int index, int* newIndex)
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
            it = evaluateCurveChanged(eCurveChangedReasonDerivativesChanged, it);
        }
        if (newIndex) {
            *newIndex = std::distance(_imp->keyFrames.begin(), it);
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
    return setKeyFrameDerivativesInternal(left, right, index, newIndex);
}

KeyFrame
Curve::setKeyFrameInterpolationInternal(KeyframeTypeEnum interp, int index, int* newIndex)
{
    KeyFrame ret;
    {
        QMutexLocker l(&_imp->_lock);
        KeyFrameSet::iterator it = atIndex(index);
        assert( it != _imp->keyFrames.end() );

        ///if the curve is a string_curve or bool_curve the interpolation is bound to be constant.
        if ( ( (_imp->type == Curve::eCurveTypeString) || (_imp->type == Curve::eCurveTypeBool) ||
              ( _imp->type == Curve::eCurveTypeIntConstantInterp) ) && ( interp != eKeyframeTypeConstant) ) {
            return *it;
        }


        if ( interp != it->getInterpolation() ) {
            it = setKeyframeInterpolation_internal(it, interp);
        }
        if (newIndex) {
            *newIndex = std::distance(_imp->keyFrames.begin(), it);
        }
        ret = *it;
    }
    
    return ret;

}

KeyFrame
Curve::setKeyFrameInterpolation(KeyframeTypeEnum interp,
                                int index,
                                int* newIndex)
{
    return setKeyFrameInterpolationInternal(interp, index, newIndex);
}

bool
Curve::setKeyFrameInterpolation(KeyframeTypeEnum interp, TimeValue time, KeyFrame* ret)
{
    int index = keyFrameIndex(time);
    if (index == -1) {
        return false;
    }
    KeyFrame r = setKeyFrameInterpolationInternal(interp, index, &index);
    if (ret) {
        *ret = r;
    }
    return true;
}

void
Curve::setCurveInterpolation(KeyframeTypeEnum interp)
{
    {
        QMutexLocker l(&_imp->_lock);
        ///if the curve is a string_curve or bool_curve the interpolation is bound to be constant.
        if ( ( (_imp->type == Curve::eCurveTypeString) || (_imp->type == Curve::eCurveTypeBool) ||
               ( _imp->type == Curve::eCurveTypeIntConstantInterp) ) && ( interp != eKeyframeTypeConstant) ) {
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
                                         KeyframeTypeEnum interp)
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
    KeyframeTypeEnum prevType, nextType;

    double period = _imp->xMax - _imp->xMin;
    assert( key != _imp->keyFrames.end() );
    if ( key == _imp->keyFrames.begin() ) {
        if (_imp->isPeriodic && _imp->keyFrames.size() > 1) {
            KeyFrameSet::const_reverse_iterator prev = _imp->keyFrames.rbegin();
            tprev = prev->getTime() - period;
            vprev = prev->getValue();
            vprevDerivRight = prev->getRightDerivative();
            prevType = prev->getInterpolation();
        } else {
            tprev = tcur;
            vprev = vcur;
            vprevDerivRight = 0.;
            prevType = eKeyframeTypeNone;
        }
    } else {
        KeyFrameSet::const_iterator prev = key;
        if ( prev != _imp->keyFrames.begin() ) {
            --prev;
        }
        tprev = prev->getTime();
        vprev = prev->getValue();
        vprevDerivRight = prev->getRightDerivative();
        prevType = prev->getInterpolation();

        //if prev is the first keyframe, and not edited by the user then interpolate linearly
        if ( !_imp->isPeriodic && ( prev == _imp->keyFrames.begin() ) && (prevType != eKeyframeTypeFree) &&
             ( prevType != eKeyframeTypeBroken) ) {
            prevType = eKeyframeTypeLinear;
        }
    }

    KeyFrameSet::const_iterator next = key;
    if ( next != _imp->keyFrames.end() ) {
        ++next;
    }
    if ( next == _imp->keyFrames.end() ) {
        if (_imp->isPeriodic && _imp->keyFrames.size() > 1) {
            KeyFrameSet::const_iterator start = _imp->keyFrames.begin();
            tnext = start->getTime() + period;
            vnext = start->getValue();
            vnextDerivLeft = start->getLeftDerivative();
            nextType = start->getInterpolation();
        } else {
            tnext = tcur;
            vnext = vcur;
            vnextDerivLeft = 0.;
            nextType = eKeyframeTypeNone;
        }
    } else {
        tnext = next->getTime();
        vnext = next->getValue();
        vnextDerivLeft = next->getLeftDerivative();
        nextType = next->getInterpolation();

        KeyFrameSet::const_iterator nextnext = next;
        if ( nextnext != _imp->keyFrames.end() ) {
            ++nextnext;
        }
        //if next is thelast keyframe, and not edited by the user then interpolate linearly
        if ( !_imp->isPeriodic && ( nextnext == _imp->keyFrames.end() ) && (nextType != eKeyframeTypeFree) &&
             ( nextType != eKeyframeTypeBroken) ) {
            nextType = eKeyframeTypeLinear;
        }
    }

    double vcurDerivLeft, vcurDerivRight;

    assert(key != _imp->keyFrames.end() &&
           key->getInterpolation() != eKeyframeTypeNone &&
           key->getInterpolation() != eKeyframeTypeBroken &&
           key->getInterpolation() != eKeyframeTypeFree);
    if ( key == _imp->keyFrames.end() ) {
        throw std::logic_error("Curve::refreshDerivatives");
    }
    Interpolation::autoComputeDerivatives(prevType,
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

    std::pair<KeyFrameSet::iterator, bool> newKeyIt = _imp->keyFrames.insert(newKey);

    // keyframe at this time exists, erase and insert again
    if (!newKeyIt.second) {
        _imp->keyFrames.erase(newKeyIt.first);
        newKeyIt = _imp->keyFrames.insert(newKey);
        assert(newKeyIt.second);
    }
    key = newKeyIt.first;

    if (reason != eCurveChangedReasonDerivativesChanged) {
        key = evaluateCurveChanged(eCurveChangedReasonDerivativesChanged, key);
    }

    return key;
} // refreshDerivatives

KeyFrameSet::iterator
Curve::evaluateCurveChanged(CurveChangedReasonEnum reason,
                            KeyFrameSet::iterator key)
{
    // PRIVATE - should not lock
    assert( key != _imp->keyFrames.end() );

    if ( (key->getInterpolation() != eKeyframeTypeBroken) && (key->getInterpolation() != eKeyframeTypeFree)
         && ( reason != eCurveChangedReasonDerivativesChanged) ) {
        key = refreshDerivatives(eCurveChangedReasonDerivativesChanged, key);
    }

    if ( key != _imp->keyFrames.begin() ) {
        KeyFrameSet::iterator prev = key;
        if ( prev != _imp->keyFrames.begin() ) {
            --prev;
        }
        if ( (prev->getInterpolation() != eKeyframeTypeBroken) &&
             ( prev->getInterpolation() != eKeyframeTypeFree) &&
             ( prev->getInterpolation() != eKeyframeTypeNone) ) {
            prev = refreshDerivatives(eCurveChangedReasonDerivativesChanged, prev);
        }
    } else if (_imp->isPeriodic && _imp->keyFrames.size() > 1) {
        KeyFrameSet::iterator prev = _imp->keyFrames.end();
        --prev;
        if (prev != key && (prev->getInterpolation() != eKeyframeTypeBroken) &&
            ( prev->getInterpolation() != eKeyframeTypeFree) &&
            ( prev->getInterpolation() != eKeyframeTypeNone) ) {
            prev = refreshDerivatives(eCurveChangedReasonDerivativesChanged, prev);
        }
    }

    KeyFrameSet::iterator next = key;
    if ( next != _imp->keyFrames.end() ) {
        ++next;
    }
    if ( next != _imp->keyFrames.end() ) {
        if ( (next->getInterpolation() != eKeyframeTypeBroken) &&
             ( next->getInterpolation() != eKeyframeTypeFree) &&
             ( next->getInterpolation() != eKeyframeTypeNone) ) {
            next = refreshDerivatives(eCurveChangedReasonDerivativesChanged, next);
        }
    } else if (_imp->isPeriodic && _imp->keyFrames.size() > 1) {
        next = _imp->keyFrames.begin();
        if (next != key && (next->getInterpolation() != eKeyframeTypeBroken) &&
            ( next->getInterpolation() != eKeyframeTypeFree) &&
            ( next->getInterpolation() != eKeyframeTypeNone) ) {
            next = refreshDerivatives(eCurveChangedReasonDerivativesChanged, next);
        }
    }
    onCurveChanged();

    return key;
}

KeyFrameSet::const_iterator
Curve::findWithTime(const KeyFrameSet& keys,
                    KeyFrameSet::const_iterator fromIt,
                    TimeValue time)
{
    if (fromIt == keys.end()) {
        fromIt = keys.begin();
    }
    return std::find_if( fromIt, keys.end(), KeyFrameTimePredicate(time) );
}

KeyFrameSet::const_iterator
Curve::find(TimeValue time, KeyFrameSet::const_iterator fromIt) const
{
    // PRIVATE - should not lock
    return findWithTime(_imp->keyFrames, fromIt, time);
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
Curve::keyFrameIndex(TimeValue time) const
{
    QMutexLocker l(&_imp->_lock);
    int i = 0;
    double paramEps;

    paramEps = NATRON_CURVE_X_SPACING_EPSILON;

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

    return _imp->canMoveY;
}

void
Curve::setYComponentMovable(bool canEdit)
{
    QMutexLocker l(&_imp->_lock);
    _imp->canMoveY = canEdit;
}

bool
Curve::areKeyFramesValuesClampedToIntegers() const
{
    QMutexLocker l(&_imp->_lock);

    return _imp->type == Curve::eCurveTypeInt || _imp->type == Curve::eCurveTypeIntConstantInterp;
}

bool
Curve::areKeyFramesValuesClampedToBooleans() const
{
    QMutexLocker l(&_imp->_lock);

    return _imp->type == Curve::eCurveTypeBool;
}


void
Curve::setDisplayYRange(double displayMin, double displayMax)
{
    QMutexLocker l(&_imp->_lock);

    _imp->displayMin = displayMin;
    _imp->displayMax = displayMax;
}

void
Curve::setYRange(double yMin,
                 double yMax)
{
    QMutexLocker l(&_imp->_lock);

    _imp->yMin = yMin;
    _imp->yMax = yMax;
}

void
Curve::onCurveChanged()
{
#ifdef NATRON_CURVE_USE_CACHE
    _imp->resultCache.clear();
#endif
}

void
Curve::fromSerialization(const SERIALIZATION_NAMESPACE::SerializationObjectBase& obj)
{
    const SERIALIZATION_NAMESPACE::CurveSerialization* s = dynamic_cast<const SERIALIZATION_NAMESPACE::CurveSerialization*>(&obj);
    if (!s) {
        return;
    }
    QMutexLocker l(&_imp->_lock);
    _imp->keyFrames.clear();
    for (std::list<SERIALIZATION_NAMESPACE::KeyFrameSerialization>::const_iterator it = s->keys.begin(); it != s->keys.end(); ++it) {
        KeyFrame k;
        k.setTime(TimeValue(it->time));
        k.setValue(it->value);
        KeyframeTypeEnum t = eKeyframeTypeSmooth;
        if (it->interpolation == kKeyframeSerializationTypeBroken) {
            t = eKeyframeTypeBroken;
        } else if (it->interpolation == kKeyframeSerializationTypeCatmullRom) {
            t = eKeyframeTypeCatmullRom;
        } else if (it->interpolation == kKeyframeSerializationTypeConstant) {
            t = eKeyframeTypeConstant;
        } else if (it->interpolation == kKeyframeSerializationTypeCubic) {
            t = eKeyframeTypeCubic;
        } else if (it->interpolation == kKeyframeSerializationTypeFree) {
            t = eKeyframeTypeFree;
        } else if (it->interpolation == kKeyframeSerializationTypeHorizontal) {
            t = eKeyframeTypeHorizontal;
        } else if (it->interpolation == kKeyframeSerializationTypeLinear) {
            t = eKeyframeTypeLinear;
        } else if (it->interpolation == kKeyframeSerializationTypeSmooth) {
            t = eKeyframeTypeSmooth;
        }
        k.setInterpolation(t);
        if (t == eKeyframeTypeBroken || t == eKeyframeTypeFree) {
            k.setRightDerivative(it->rightDerivative);
            if (t == eKeyframeTypeBroken) {
                k.setLeftDerivative(it->leftDerivative);
            } else {
                k.setLeftDerivative(-it->rightDerivative);
            }
        }
        std::pair<KeyFrameSet::iterator, bool> ret = addKeyFrameNoUpdate(k);
        if (ret.second) {
            (void)evaluateCurveChanged(eCurveChangedReasonKeyframeChanged, ret.first);
        }
    }
    onCurveChanged();
}

void
Curve::toSerialization(SERIALIZATION_NAMESPACE::SerializationObjectBase* obj)
{
    SERIALIZATION_NAMESPACE::CurveSerialization* s = dynamic_cast<SERIALIZATION_NAMESPACE::CurveSerialization*>(obj);
    if (!s) {
        return;
    }
    KeyFrameSet keys = getKeyFrames_mt_safe();
    for (KeyFrameSet::iterator it = keys.begin(); it!=keys.end(); ++it) {
        QMutexLocker l(&_imp->_lock);
        SERIALIZATION_NAMESPACE::KeyFrameSerialization k;
        k.time = it->getTime();
        k.value = it->getValue();
        KeyframeTypeEnum t = it->getInterpolation();
        switch (t) {
            case eKeyframeTypeSmooth:
                k.interpolation = kKeyframeSerializationTypeSmooth;
                break;
            case eKeyframeTypeLinear:
                k.interpolation = kKeyframeSerializationTypeLinear;
                break;
            case eKeyframeTypeBroken:
                k.interpolation = kKeyframeSerializationTypeBroken;
                break;
            case eKeyframeTypeFree:
                k.interpolation = kKeyframeSerializationTypeFree;
                break;
            case eKeyframeTypeCatmullRom:
                k.interpolation = kKeyframeSerializationTypeCatmullRom;
                break;
            case eKeyframeTypeCubic:
                k.interpolation = kKeyframeSerializationTypeCubic;
                break;
            case eKeyframeTypeHorizontal:
                k.interpolation = kKeyframeSerializationTypeHorizontal;
                break;
            case eKeyframeTypeConstant:
                k.interpolation = kKeyframeSerializationTypeConstant;
                break;
            default:
                break;
        }
        k.leftDerivative = it->getLeftDerivative();
        k.rightDerivative = it->getRightDerivative();
        s->keys.push_back(k);
    }
}

bool
Curve::operator==(const Curve & other) const
{
    KeyFrameSet keys = getKeyFrames_mt_safe();
    KeyFrameSet otherKeys = other.getKeyFrames_mt_safe();
    if (keys.size() != otherKeys.size()) {
        return false;
    }

    KeyFrameSet::const_iterator itOther = otherKeys.begin();
    for (KeyFrameSet::const_iterator it = keys.begin(); it!=keys.end(); ++it, ++itOther) {
        if (*it != *itOther) {
            return false;
        }
    }
    return true;

}

void
Curve::setKeyframesInternal(const KeyFrameSet& keys, bool refreshDerivatives)
{
    if (!refreshDerivatives) {
        _imp->keyFrames = keys;
    } else {
        _imp->keyFrames.clear();

        // Now recompute auto tangents
        for (KeyFrameSet::iterator it = keys.begin(); it != keys.end(); ++it) {
            std::pair<KeyFrameSet::iterator, bool> ret = addKeyFrameNoUpdate(*it);
            ret.first = evaluateCurveChanged(Curve::eCurveChangedReasonKeyframeChanged, ret.first);
        }


    }
    onCurveChanged();

}

void
Curve::setKeyframes(const KeyFrameSet& keys, bool refreshDerivatives)
{
    QMutexLocker k(&_imp->_lock);
    setKeyframesInternal(keys, refreshDerivatives);
}

void
Curve::smooth(const RangeD* range)
{
    std::vector<float> smoothedCurve;

    QMutexLocker l(&_imp->_lock);

    KeyFrameSet::iterator start = _imp->keyFrames.end();

    for (KeyFrameSet::iterator it = _imp->keyFrames.begin(); it != _imp->keyFrames.end(); ++it) {
        TimeValue time = it->getTime();
        if ( range != 0 ) {
            if (time < range->min) {
                continue;
            }
            if (time > range->max) {
                break;
            }
        }
        if (start == _imp->keyFrames.end()) {
            start = it;
        }
        smoothedCurve.push_back(it->getValue());
    }

    Smooth1D::laplacian_1D(smoothedCurve);

    KeyFrameSet newSet;
    if (start != _imp->keyFrames.end()) {

        // Add keyframes before range first
        for (KeyFrameSet::iterator it = _imp->keyFrames.begin(); it != start; ++it) {
            newSet.insert(*it);
        }

        // Now insert modified keyframes
        for (std::size_t i = 0; i < smoothedCurve.size(); ++i, ++start) {
            KeyFrame k(*start);
            k.setValue(smoothedCurve[i]);
            newSet.insert(k);
        }

        // Now insert original keys after range
        for (KeyFrameSet::iterator it = start; it != _imp->keyFrames.end(); ++it) {
            newSet.insert(*it);
        }

        setKeyframesInternal(newSet, true);

    }
}

NATRON_NAMESPACE_EXIT;
