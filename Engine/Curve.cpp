/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

NATRON_NAMESPACE_ENTER


NATRON_NAMESPACE_ANONYMOUS_ENTER

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
    : PropertiesHolder()
    , _time(0.)
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
    : PropertiesHolder()
    , _time(time)
    , _value(initialValue)
    , _leftDerivative(leftDerivative)
    , _rightDerivative(rightDerivative)
    , _interpolation(interpolation)
{
    assert( !(boost::math::isnan)(_time) && !(boost::math::isinf)(_time) );
    assert( !(boost::math::isnan)(_value) && !(boost::math::isinf)(_value) );
    assert( !(boost::math::isnan)(_leftDerivative) && !(boost::math::isinf)(_leftDerivative) );
    assert( !(boost::math::isnan)(_rightDerivative) && !(boost::math::isinf)(_rightDerivative) );
}

KeyFrame::KeyFrame(const KeyFrame & other)
    : PropertiesHolder(other)
    , _time(other._time)
    , _value(other._value)
    , _leftDerivative(other._leftDerivative)
    , _rightDerivative(other._rightDerivative)
    , _interpolation(other._interpolation)
{
    assert( !(boost::math::isnan)(_time) && !(boost::math::isinf)(_time) );
    assert( !(boost::math::isnan)(_value) && !(boost::math::isinf)(_value) );
    assert( !(boost::math::isnan)(_leftDerivative) && !(boost::math::isinf)(_leftDerivative) );
    assert( !(boost::math::isnan)(_rightDerivative) && !(boost::math::isinf)(_rightDerivative) );
}

void
KeyFrame::operator=(const KeyFrame & o)
{
    cloneProperties(o);
    _time = o._time;
    _value = o._value;
    _leftDerivative = o._leftDerivative;
    _rightDerivative = o._rightDerivative;
    _interpolation = o._interpolation;
    assert( !(boost::math::isnan)(_time) && !(boost::math::isinf)(_time) );
    assert( !(boost::math::isnan)(_value) && !(boost::math::isinf)(_value) );
    assert( !(boost::math::isnan)(_leftDerivative) && !(boost::math::isinf)(_leftDerivative) );
    assert( !(boost::math::isnan)(_rightDerivative) && !(boost::math::isinf)(_rightDerivative) );
}

KeyFrame::~KeyFrame()
{
}

void
KeyFrame::setLeftDerivative(double leftDerivative)
{
    _leftDerivative = leftDerivative;
    assert( !(boost::math::isnan)(_leftDerivative) && !(boost::math::isinf)(_leftDerivative) );
}

void
KeyFrame::setRightDerivative(double rightDerivative)
{
    _rightDerivative = rightDerivative;
    assert( !(boost::math::isnan)(_rightDerivative) && !(boost::math::isinf)(_rightDerivative) );
}

void
KeyFrame::setValue(double value)
{
    _value = value;
    assert( !(boost::math::isnan)(_value) && !(boost::math::isinf)(_value) );
}

void
KeyFrame::setTime(TimeValue time)
{
    _time = time;
    assert( !(boost::math::isnan)(_time) && !(boost::math::isinf)(_time) );
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
}


Curve::Curve(const Curve & other)
    : _imp( new CurvePrivate(*other._imp) )
{
}

Curve::~Curve()
{
    clearKeyFrames();
}

CurveTypeEnum
Curve::getType() const
{
    return _imp->type;
}

void
Curve::registerListener(const CurveChangesListenerPtr& listener)
{
    QMutexLocker l(&_imp->_lock);
    for (std::list<CurveChangesListenerWPtr>::const_iterator it = _imp->listeners.begin(); it != _imp->listeners.end(); ++it) {
        CurveChangesListenerPtr l = it->lock();
        if (l == listener) {
            return;
        }
    }
    _imp->listeners.push_back(listener);

}

void
Curve::unregisterListener(const CurveChangesListenerPtr& listener)
{
    QMutexLocker l(&_imp->_lock);
    for (std::list<CurveChangesListenerWPtr>::iterator it = _imp->listeners.begin(); it != _imp->listeners.end(); ++it) {
        CurveChangesListenerPtr l = it->lock();
        if (l == listener) {
            _imp->listeners.erase(it);
        }
    }
}

void
Curve::setInterpolator(const KeyFrameInterpolatorPtr& interpolator)
{
    assert(interpolator);
    if (!interpolator) {
        return;
    }
    QMutexLocker l(&_imp->_lock);
    _imp->interpolator = interpolator;
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

    return _imp->clampKeyFramesTimeToIntegers;
}

void
Curve::setKeyFramesTimeClampedToIntegers(bool enabled)
{
    _imp->clampKeyFramesTimeToIntegers = enabled;
}

std::list<CurveChangesListenerPtr>
Curve::getListeners() const
{
    std::list<CurveChangesListenerPtr> ret;
    for (std::list<CurveChangesListenerWPtr>::const_iterator it = _imp->listeners.begin(); it != _imp->listeners.end(); ++it) {
        CurveChangesListenerPtr listener = it->lock();
        if (!listener) {
            continue;
        }
        ret.push_back(listener);
    }
    return ret;
}

void
Curve::notifyKeyFramesChanged(const std::list<CurveChangesListenerPtr>& listeners, const KeyFrameSet& oldKeyframes,const KeyFrameSet& newKeyframes)
{
    assert(!listeners.empty());

    KeyFrameSet keysAdded,keysRemoved;
    computeKeyFramesDiff(oldKeyframes, newKeyframes, &keysAdded, &keysRemoved);

    for (std::list<CurveChangesListenerPtr>::const_iterator it = listeners.begin(); it != listeners.end(); ++it) {
        const CurveChangesListenerPtr& listener = *it;

        for (KeyFrameSet::const_iterator it2 = keysAdded.begin(); it2 != keysAdded.end(); ++it2) {
            listener->onKeyFrameSet(this, *it2, true);
        }
        for (KeyFrameSet::const_iterator it2 = keysRemoved.begin(); it2 != keysRemoved.end(); ++it2) {
            listener->onKeyFrameRemoved(this, *it2);
        }
    }

}

void
Curve::notifyKeyFramesSet(const std::list<CurveChangesListenerPtr>& listeners, const KeyFrame& k, bool added)
{

    for (std::list<CurveChangesListenerPtr>::const_iterator it = listeners.begin(); it != listeners.end(); ++it) {
        const CurveChangesListenerPtr& listener = *it;
        listener->onKeyFrameSet(this, k, added);
    }
}

void
Curve::clone(const Curve & other)
{
    KeyFrameSet otherKeys = other.getKeyFrames_mt_safe();

    std::list<CurveChangesListenerPtr> listeners;
    boost::scoped_ptr<KeyFrameSet> oldKeys;
    boost::scoped_ptr<KeyFrameSet> newKeys;

    {
        QMutexLocker l(&_imp->_lock);

        listeners = getListeners();
        if (!listeners.empty()) {
            oldKeys.reset(new KeyFrameSet);
            *oldKeys = _imp->keyFrames;
        }


        _imp->keyFrames.clear();
        _imp->keyFrames.insert( otherKeys.begin(), otherKeys.end() );
        if (!listeners.empty()) {
            newKeys.reset(new KeyFrameSet);
            *newKeys = _imp->keyFrames;
        }
        onCurveChanged();
    }
    if (!listeners.empty()) {
        notifyKeyFramesChanged(listeners, *oldKeys, *newKeys);
    }
}

void
Curve::cloneIndexRange(const Curve& other, int firstKeyIdx, int nKeys)
{
    KeyFrameSet otherKeys = other.getKeyFrames_mt_safe();
    std::list<CurveChangesListenerPtr> listeners;
    boost::scoped_ptr<KeyFrameSet> oldKeys, newKeys;
    {
        QMutexLocker l(&_imp->_lock);


        listeners = getListeners();
        if (!listeners.empty()) {
            oldKeys.reset(new KeyFrameSet);
            *oldKeys = _imp->keyFrames;
        }

        _imp->keyFrames.clear();
        if (firstKeyIdx >= (int)otherKeys.size()) {
            if (!listeners.empty()) {
                l.unlock();
                notifyKeyFramesChanged(listeners, *oldKeys, KeyFrameSet());
            }
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
        _imp->keyFrames.insert(start, end);
        if (!listeners.empty()) {
            newKeys.reset(new KeyFrameSet);
            *newKeys = _imp->keyFrames;
        }
        onCurveChanged();
    }

    if (!listeners.empty()) {
        notifyKeyFramesChanged(listeners, *oldKeys, *newKeys);
    }
} // cloneIndexRange

bool
Curve::cloneAndCheckIfChanged(const Curve& other, double offset, const RangeD* range)
{
    KeyFrameSet otherKeys = other.getKeyFrames_mt_safe();
    std::list<CurveChangesListenerPtr> listeners;
    boost::scoped_ptr<KeyFrameSet> oldKeys;

    bool hasChanged = false;

    {
        QMutexLocker l(&_imp->_lock);

        listeners = getListeners();
        if (!listeners.empty()) {
            oldKeys.reset(new KeyFrameSet);
            *oldKeys = _imp->keyFrames;
        }



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
    }
    if (!listeners.empty()) {
        notifyKeyFramesChanged(listeners, *oldKeys, otherKeys);
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

    std::list<CurveChangesListenerPtr> listeners;
    boost::scoped_ptr<KeyFrameSet> oldKeys, newKeys;

    {
        QMutexLocker l(&_imp->_lock);

        listeners = getListeners();
        if (!listeners.empty()) {
            oldKeys.reset(new KeyFrameSet);
            *oldKeys = _imp->keyFrames;
        }
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

        if (!listeners.empty()) {
            newKeys.reset(new KeyFrameSet);
            *newKeys = _imp->keyFrames;
        }


        onCurveChanged();
    }
    if (!listeners.empty()) {
        notifyKeyFramesChanged(listeners, *oldKeys, *newKeys);
    }
} // clone

void
Curve::computeKeyFramesDiff(const KeyFrameSet& keysA,
                            const KeyFrameSet& keysB,
                            KeyFrameSet* keysAdded,
                            KeyFrameSet* keysRemoved)
{
    KeyFrameSet keysACopy = keysA;
    if (keysAdded) {
        for (KeyFrameSet::iterator it = keysB.begin(); it != keysB.end(); ++it) {
            KeyFrameSet::iterator foundInOldKeys = Curve::findWithTime(keysACopy, keysACopy.end(), it->getTime());
            if (foundInOldKeys == keysACopy.end()) {
                keysAdded->insert(*it);
            } else {
                keysACopy.erase(foundInOldKeys);
            }
        }
    }
    if (keysRemoved) {
        for (KeyFrameSet::iterator it = keysACopy.begin(); it != keysACopy.end(); ++it) {
            KeyFrameSet::iterator foundInNextKeys = Curve::findWithTime(keysB, keysB.end(), it->getTime());
            if (foundInNextKeys == keysB.end()) {
                keysRemoved->insert(*it);
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



ValueChangedReturnCodeEnum
Curve::setOrAddKeyframe(const KeyFrame& key, SetKeyFrameFlags flags, int *keyframeIndex)
{

    // check for NaN or infinity
    double val = key.getValue();
    if ( (boost::math::isnan)(val) || (boost::math::isinf)(val) ) {
        KeyFrame tmp = key;
        tmp.setValue(val);
        return setOrAddKeyframe(tmp, flags, keyframeIndex);
    }

    ValueChangedReturnCodeEnum retCode = eValueChangedReturnCodeNothingChanged;
    std::list<CurveChangesListenerPtr> listeners;
    {
        QMutexLocker l(&_imp->_lock);
        listeners = getListeners();
        std::pair<KeyFrameSet::iterator, ValueChangedReturnCodeEnum> it;

        if (isInterpolationConstantOnly()) {
            KeyFrame copy = key;
            copy.setInterpolation(eKeyframeTypeConstant);
            it = setOrUpdateKeyframeInternal(copy, flags);
        } else {
            it = setOrUpdateKeyframeInternal(key, flags);
        }
        retCode = it.second;

        assert(it.first != _imp->keyFrames.end());

        it.first = evaluateCurveChanged(eCurveChangedReasonKeyframeChanged, it.first);

        if (keyframeIndex) {
            *keyframeIndex = std::distance(_imp->keyFrames.begin(), it.first);
        }
    }
    if (retCode == eValueChangedReturnCodeKeyframeAdded) {
        notifyKeyFramesSet(listeners, key, true);
    } else if (retCode == eValueChangedReturnCodeKeyframeModified) {
        notifyKeyFramesSet(listeners, key, false);
    }
    return retCode;
    
}

bool applyKeyFrameConflictsFlags(KeyFrame& tmp, const KeyFrame & cp, SetKeyFrameFlags flags)
{
    bool changed = false;
    if (flags & eSetKeyFrameFlagSetValue) {
        changed |= cp.getValue() != tmp.getValue();
        tmp.setValue(cp.getValue());
        changed |= cp.getInterpolation() != tmp.getInterpolation();
        tmp.setInterpolation(cp.getInterpolation());
        changed |= cp.getLeftDerivative() != tmp.getLeftDerivative();
        tmp.setLeftDerivative(cp.getLeftDerivative());
        changed |= cp.getRightDerivative() != tmp.getRightDerivative();
        tmp.setRightDerivative(cp.getRightDerivative());
    }
    if ((flags & eSetKeyFrameFlagReplaceProperties) || (flags & eSetKeyFrameFlagMergeProperties)) {
        if (flags & eSetKeyFrameFlagReplaceProperties) {
            changed |= tmp.cloneProperties(cp);
        } else if (flags & eSetKeyFrameFlagMergeProperties) {
            changed |= tmp.mergeProperties(cp);
        }
    }
    return changed;
}

std::pair<KeyFrameSet::iterator, ValueChangedReturnCodeEnum> Curve::setOrUpdateKeyframeInternal(const KeyFrame & cp, SetKeyFrameFlags flags)
{
    // PRIVATE - should not lock
    if (_imp->clampKeyFramesTimeToIntegers) {
        std::pair<KeyFrameSet::iterator, bool> newKey = _imp->keyFrames.insert(cp);
        // keyframe at this time exists, erase and insert again
        if (newKey.second) {
            return std::make_pair(newKey.first, eValueChangedReturnCodeKeyframeAdded);
        } else {

            KeyFrame tmp = *newKey.first;
            bool changed = applyKeyFrameConflictsFlags(tmp, cp, flags);
            if (!changed) {
                return std::make_pair(newKey.first, eValueChangedReturnCodeNothingChanged);
            }
            _imp->keyFrames.erase(newKey.first);
            newKey = _imp->keyFrames.insert(tmp);
            assert(newKey.second);
            return std::make_pair(newKey.first,  eValueChangedReturnCodeKeyframeModified);

        }
    } else {
        double paramEps = NATRON_CURVE_X_SPACING_EPSILON;
        ValueChangedReturnCodeEnum retCode = eValueChangedReturnCodeKeyframeAdded;
        KeyFrame tmp = cp;
        for (KeyFrameSet::iterator it = _imp->keyFrames.begin(); it != _imp->keyFrames.end(); ++it) {
            if (std::abs( it->getTime() - cp.getTime() ) < paramEps) {

                tmp = *it;
                bool changed = applyKeyFrameConflictsFlags(tmp, cp, flags);
                if (!changed) {
                    return std::make_pair(it, eValueChangedReturnCodeNothingChanged);
                }

                _imp->keyFrames.erase(it);
                retCode = eValueChangedReturnCodeKeyframeModified;
                break;
            }
        }
        std::pair<KeyFrameSet::iterator, bool> newKey = _imp->keyFrames.insert(tmp);
        return std::make_pair(newKey.first, retCode);
    }
} // setOrUpdateKeyframeInternal

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

    KeyFrame removedKey = *it;
    _imp->keyFrames.erase(it);

    if (mustRefreshPrev) {
        refreshDerivatives( eCurveChangedReasonDerivativesChanged, find( prevKey.getTime(), _imp->keyFrames.end()) );
    }
    if (mustRefreshNext) {
        refreshDerivatives( eCurveChangedReasonDerivativesChanged, find( nextKey.getTime(), _imp->keyFrames.end() ) );
    }

    for (std::list<CurveChangesListenerWPtr>::const_iterator it = _imp->listeners.begin(); it != _imp->listeners.end(); ++it) {
        CurveChangesListenerPtr listener = it->lock();
        if (!listener) {
            continue;
        }
        listener->onKeyFrameRemoved(this, removedKey);
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

    std::list<CurveChangesListenerPtr> listeners;
    boost::scoped_ptr<KeyFrameSet> oldKeys, newKeys;

    {
        QMutexLocker l(&_imp->_lock);

        listeners = getListeners();
        if (!listeners.empty()) {
            oldKeys.reset(new KeyFrameSet);
            *oldKeys = _imp->keyFrames;
        }

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
    if (!listeners.empty()) {
        notifyKeyFramesChanged(listeners, *oldKeys, newSet);
    }
}

void
Curve::removeKeyFramesAfterTime(TimeValue time,
                                std::list<double>* keyframeRemoved)
{
    KeyFrameSet newSet;

    std::list<CurveChangesListenerPtr> listeners;
    boost::scoped_ptr<KeyFrameSet> oldKeys, newKeys;

    {
        QMutexLocker l(&_imp->_lock);

        listeners = getListeners();
        if (!listeners.empty()) {
            oldKeys.reset(new KeyFrameSet);
            *oldKeys = _imp->keyFrames;
        }

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
    if (!listeners.empty()) {
        notifyKeyFramesChanged(listeners, *oldKeys, newSet);
    }
    
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
    if (lowerBound == _imp->keyFrames.begin()) {
        return false;
    } else if (lowerBound == _imp->keyFrames.end()) {
        *k = *_imp->keyFrames.rbegin();
        return true;
    } else {
        --lowerBound;
        *k = *lowerBound;
        return true;
    }
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



KeyFrame
Curve::getValueAt(TimeValue t,
                  bool doClamp) const
{
    QMutexLocker l(&_imp->_lock);

    KeyFrame value(t, 0.);

    if ( _imp->keyFrames.empty() ) {
        //throw std::runtime_error("Curve has no control points!");

        // A curve with no control points is considered to be 0
        // this is to avoid returning StatFailed when KnobParametric::getValue() is called on a parametric curve without control point.
        return value;

        // There is no special case for a curve with one (1) keyframe: the result is a linear curve before and after the keyframe.
    //} else if (_imp->keyFrames.size() == 1) {
    //    return _imp->keyFrames.begin()->getValue();
    }


    // even when there is only one keyframe, there may be tangents!
    //if (_imp->keyFrames.size() == 1) {
    //    //if there's only 1 keyframe, don't bother interpolating
    //    return (*_imp->keyFrames.begin()).getValue();
    //}

    // find the first keyframe with time greater than t
    KeyFrameSet::const_iterator itup = _imp->keyFrames.upper_bound(value);
    value = _imp->interpolator->interpolate(t, itup, _imp->keyFrames, _imp->isPeriodic, TimeValue(_imp->xMin), TimeValue(_imp->xMax));

    double v = value.getValue();


    bool mustUpdateKeyFrame = false;
    if ( doClamp ) {
        v = clampValueToCurveYRange(v);
        mustUpdateKeyFrame = true;
    }

    switch (_imp->type) {
        case eCurveTypeString:
        case eCurveTypeInt:
            v = std::floor(v + 0.5);
            mustUpdateKeyFrame = true;
            break;
        case eCurveTypeBool:
            v = v >= 0.5 ? 1. : 0.;
            mustUpdateKeyFrame = true;
            break;
        default:
            break;
    }
    if (mustUpdateKeyFrame) {
        value.setValue(v);
    }
    return value;
} // getValueAt

double
Curve::getDerivativeAt(TimeValue t) const
{
    QMutexLocker l(&_imp->_lock);

    if ( _imp->keyFrames.empty() ) {
        throw std::runtime_error("Curve has no control points!");
    }
    assert(_imp->type == eCurveTypeDouble); // only real-valued curves can be derived

    // even when there is only one keyframe, there may be tangents!
    //if (_imp->keyFrames.size() == 1) {
    //    //if there's only 1 keyframe, don't bother interpolating
    //    return (*_imp->keyFrames.begin()).getValue();
    //}


    KeyFrame k(t, 0.);
    // find the first keyframe with time greater than t
    KeyFrameSet::const_iterator itup = _imp->keyFrames.upper_bound(k);

    KeyFrame keyCur, keyNext;

    KeyFrameInterpolator::interParams(_imp->keyFrames,
                                      _imp->isPeriodic,
                                      _imp->xMin,
                                      _imp->xMax,
                                      &t,
                                      itup,
                                      &keyCur, &keyNext);
    


    double d;

    if ( _imp->yMin != -std::numeric_limits<double>::infinity() || _imp->yMax != std::numeric_limits<double>::infinity()) {
        d = Interpolation::derive_clamp(keyCur.getTime(), keyCur.getValue(),
                                        keyCur.getRightDerivative(),
                                        keyNext.getLeftDerivative(),
                                        keyNext.getTime(), keyNext.getValue(),
                                        t,
                                        _imp->yMin, _imp->yMax,
                                        keyCur.getInterpolation(),
                                        keyNext.getInterpolation());
    } else {
        d = Interpolation::derive(keyCur.getTime(), keyCur.getValue(),
                                  keyCur.getRightDerivative(),
                                  keyNext.getLeftDerivative(),
                                  keyNext.getTime(), keyNext.getValue(),
                                  t,
                                  keyCur.getInterpolation(),
                                  keyNext.getInterpolation());
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
    assert(_imp->type == eCurveTypeDouble); // only real-valued curves can be derived

    // even when there is only one keyframe, there may be tangents!
    //if (_imp->keyFrames.size() == 1) {
    //    //if there's only 1 keyframe, don't bother interpolating
    //    return (*_imp->keyFrames.begin()).getValue();
    //}

    KeyFrame k(t1, 0.);
    // find the first keyframe with time strictly greater than t1

    KeyFrameSet::const_iterator itup = _imp->keyFrames.upper_bound(k);

    KeyFrame keyCur, keyNext;

    KeyFrameInterpolator::interParams(_imp->keyFrames,
                                      _imp->isPeriodic,
                                      _imp->xMin,
                                      _imp->xMax,
                                      &t1,
                                      itup,
                                      &keyCur, &keyNext);


    
    double sum = 0.;

    // while there are still keyframes after the current time, add to the total sum and advance
    while (itup != _imp->keyFrames.end() && itup->getTime() < t2) {
        // add integral from t1 to itup->getTime() to sum
        if ( _imp->yMin != -std::numeric_limits<double>::infinity() || _imp->yMax != std::numeric_limits<double>::infinity()) {

            sum += Interpolation::integrate_clamp(keyCur.getTime(), keyCur.getValue(),
                                                  keyCur.getRightDerivative(),
                                                  keyNext.getLeftDerivative(),
                                                  keyNext.getTime(), keyNext.getValue(),
                                                  TimeValue(t1), itup->getTime(),
                                                  _imp->yMin, _imp->yMax,
                                                  keyCur.getInterpolation(),
                                                  keyNext.getInterpolation());
        } else {
            sum += Interpolation::integrate(keyCur.getTime(), keyCur.getValue(),
                                            keyCur.getRightDerivative(),
                                            keyNext.getLeftDerivative(),
                                            keyNext.getTime(), keyNext.getValue(),
                                            TimeValue(t1), itup->getTime(),
                                            keyCur.getInterpolation(),
                                            keyNext.getInterpolation());
        }
        // advance
        t1 = itup->getTime();
        ++itup;

        KeyFrameInterpolator::interParams(_imp->keyFrames,
                                          _imp->isPeriodic,
                                          _imp->xMin,
                                          _imp->xMax,
                                          &t1,
                                          itup,
                                          &keyCur, &keyNext);

    }
    
    assert( itup == _imp->keyFrames.end() || t2 <= itup->getTime() );
    // add integral from t1 to t2 to sum
    if ( _imp->yMin != -std::numeric_limits<double>::infinity() || _imp->yMax != std::numeric_limits<double>::infinity()) {
        sum += Interpolation::integrate_clamp(keyCur.getTime(), keyCur.getValue(),
                                              keyCur.getRightDerivative(),
                                              keyNext.getLeftDerivative(),
                                              keyNext.getTime(), keyNext.getValue(),
                                              t1, t2,
                                              _imp->yMin, _imp->yMax,
                                              keyCur.getInterpolation(),
                                              keyNext.getInterpolation());
    } else {
        sum += Interpolation::integrate(keyCur.getTime(), keyCur.getValue(),
                                        keyCur.getRightDerivative(),
                                        keyNext.getLeftDerivative(),
                                        keyNext.getTime(), keyNext.getValue(),
                                        t1, t2,
                                        keyCur.getInterpolation(),
                                        keyNext.getInterpolation());
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

    return setOrUpdateKeyframeInternal(newKey).first;
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

    const bool xClampedToIntegers = areKeyFramesTimeClampedToIntegers();

    // The map containing <warped keyframes, original keyframe>
    std::map<KeyFrame, KeyFrame> warpedKeyFrames;
    std::list<CurveChangesListenerPtr> listeners;
    {
        QMutexLocker l(&_imp->_lock);

        listeners = getListeners();

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
                if (xClampedToIntegers) {
                    warpedKey.setTime(TimeValue(std::floor(warpedKey.getTime() + 0.5)));
                }

                {
                    std::pair<std::map<KeyFrame, KeyFrame>::iterator,bool> insertOk = warpedKeyFrames.insert(std::make_pair(warpedKey, *found));
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
            for (std::map<KeyFrame, KeyFrame>::const_iterator it = warpedKeyFrames.begin();
                 it != warpedKeyFrames.end();
                 ++it, ++i) {
                std::pair<KeyFrameSet::iterator,bool> insertOk = finalSet.insert(it->first);
                // A warped keyframe overlap an original keyframe left in the set, fail operation
                if (!insertOk.second) {
                    return false;
                }
                // If user want output keys, provide them
                if (newKeys) {
                    (*newKeys)[i] = (it->first);
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
            
            std::pair<KeyFrameSet::iterator, ValueChangedReturnCodeEnum> ret = setOrUpdateKeyframeInternal(*it);
            
            // huh, we are just moving one set to another, there cannot be a failure!
            assert(ret.second != eValueChangedReturnCodeNothingChanged);
            
            // refresh derivative
            ret.first = evaluateCurveChanged(eCurveChangedReasonKeyframeChanged, ret.first);
            
        }
    } // QMutexLocker

    if (!listeners.empty()) {
        for (std::map<KeyFrame, KeyFrame>::const_iterator it = warpedKeyFrames.begin();
             it != warpedKeyFrames.end();
             ++it) {
            for (std::list<CurveChangesListenerPtr>::const_iterator it2 = listeners.begin(); it2 != listeners.end(); ++it2) {
                (*it2)->onKeyFrameMoved(this, it->second, it->first);
            }
        }
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
            KeyFrame newKey = *it;
            newKey.setLeftDerivative(value);
            std::pair<KeyFrameSet::iterator, ValueChangedReturnCodeEnum> ok = setOrUpdateKeyframeInternal(newKey);
            it = ok.first;
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
            std::pair<KeyFrameSet::iterator, ValueChangedReturnCodeEnum> ok = setOrUpdateKeyframeInternal(newKey);
            it = ok.first;
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
            std::pair<KeyFrameSet::iterator, ValueChangedReturnCodeEnum> ok = setOrUpdateKeyframeInternal(newKey);
            it = ok.first;
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
        if (isInterpolationConstantOnly() && ( interp != eKeyframeTypeConstant) ) {
            return *it;
        }


        if ( interp != it->getInterpolation() ) {
            std::pair<KeyFrameSet::iterator, ValueChangedReturnCodeEnum> ok = setKeyframeInterpolation_internal(it, interp);
            it = ok.first;
        }
        if (newIndex) {
            *newIndex = std::distance(_imp->keyFrames.begin(), it);
        }
        ret = *it;
    }

    return ret;

}

bool
Curve::canBeVisibleInCurveEditor(CurveTypeEnum type)
{
    return (type != eCurveTypeString) && (type != eCurveTypeChoice) && (type != eCurveTypeProperties);
}

bool
Curve::canBeVisibleInCurveEditor() const
{
    return canBeVisibleInCurveEditor(_imp->type);
}

bool
Curve::isInterpolationConstantOnly() const
{
    return (_imp->type == eCurveTypeString) || (_imp->type == eCurveTypeBool) || (_imp->type == eCurveTypeChoice) || (_imp->type == eCurveTypeProperties);
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
        if (isInterpolationConstantOnly() && ( interp != eKeyframeTypeConstant) ) {
            return;
        }
        for (int i = 0; i < (int)_imp->keyFrames.size(); ++i) {
            KeyFrameSet::iterator it = _imp->keyFrames.begin();
            std::advance(it, i);
            if ( interp != it->getInterpolation() ) {
                std::pair<KeyFrameSet::iterator, ValueChangedReturnCodeEnum> ok = setKeyframeInterpolation_internal(it, interp);
                it = ok.first;
            }
        }
    }

}

std::pair<KeyFrameSet::iterator, ValueChangedReturnCodeEnum>
Curve::setKeyframeInterpolation_internal(KeyFrameSet::iterator it,
                                         KeyframeTypeEnum interp)
{
    ///private should not be locked
    assert( it != _imp->keyFrames.end() );
    KeyFrame newKey(*it);
    newKey.setInterpolation(interp);
    std::pair<KeyFrameSet::iterator, ValueChangedReturnCodeEnum> ret = setOrUpdateKeyframeInternal(newKey);
    ret.first = evaluateCurveChanged(eCurveChangedReasonKeyframeChanged, ret.first);

    return ret;
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
    return canBeVisibleInCurveEditor();
}


bool
Curve::areKeyFramesValuesClampedToIntegers() const
{
    QMutexLocker l(&_imp->_lock);

    return _imp->type == eCurveTypeInt || isInterpolationConstantOnly();
}

bool
Curve::areKeyFramesValuesClampedToBooleans() const
{
    QMutexLocker l(&_imp->_lock);

    return _imp->type == eCurveTypeBool;
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

}

void
Curve::fromSerialization(const SERIALIZATION_NAMESPACE::SerializationObjectBase& obj)
{
    const SERIALIZATION_NAMESPACE::CurveSerialization* s = dynamic_cast<const SERIALIZATION_NAMESPACE::CurveSerialization*>(&obj);
    if (!s) {
        return;
    }


    {
        QMutexLocker l(&_imp->_lock);


        _imp->keyFrames.clear();
        for (std::list<SERIALIZATION_NAMESPACE::KeyFrameSerialization>::const_iterator it = s->keys.begin(); it != s->keys.end(); ++it) {
            KeyFrame k;
            k.setTime(TimeValue(it->time));

            switch (s->curveType) {
                case SERIALIZATION_NAMESPACE::eCurveSerializationTypeString: {
                    if (it->properties.size() == 1) {
                        const SERIALIZATION_NAMESPACE::KeyFrameProperty& prop = it->properties.front();
                        if (prop.values.size() == 1 && prop.type == kKeyFramePropertyVariantTypeString) {
                            k.setProperty<std::string>(kKeyFramePropString, prop.values[0].stringValue,0, false /*failIfnotExist*/);
                        }
                    }
                }   break;
                case SERIALIZATION_NAMESPACE::eCurveSerializationTypePropertiesOnly: {
                    for (std::list<SERIALIZATION_NAMESPACE::KeyFrameProperty>::const_iterator it2 = it->properties.begin(); it2 != it->properties.end(); ++it2) {
                        if (it2->type == kKeyFramePropertyVariantTypeString) {
                            std::vector<std::string> values(it2->values.size());
                            for (std::size_t i = 0; i < it2->values.size(); ++i) {
                                values[i] = it2->values[i].stringValue;
                            }
                            k.setPropertyN(it2->name, values, false /*failIfnotExist*/);

                        } else if (it2->type == kKeyFramePropertyVariantTypeDouble) {
                            std::vector<double> values(it2->values.size());
                            for (std::size_t i = 0; i < it2->values.size(); ++i) {
                                values[i] = it2->values[i].scalarValue;
                            }
                            k.setPropertyN(it2->name, values, false /*failIfnotExist*/);
                        } else if (it2->type == kKeyFramePropertyVariantTypeInt) {
                            std::vector<int> values(it2->values.size());
                            for (std::size_t i = 0; i < it2->values.size(); ++i) {
                                values[i] = it2->values[i].scalarValue;
                            }
                            k.setPropertyN(it2->name, values);
                        } else if (it2->type == kKeyFramePropertyVariantTypeBool) {
                            std::vector<bool> values(it2->values.size());
                            for (std::size_t i = 0; i < it2->values.size(); ++i) {
                                values[i] = it2->values[i].scalarValue;
                            }
                            k.setPropertyN(it2->name, values, false /*failIfnotExist*/);
                        }
                    }
                }   break;
                case SERIALIZATION_NAMESPACE::eCurveSerializationTypeScalar: {
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
                            k.setLeftDerivative(it->rightDerivative);
                        }
                    }
                    
                }   break;
            }

            std::pair<KeyFrameSet::iterator, ValueChangedReturnCodeEnum> ret = setOrUpdateKeyframeInternal(k);
            (void)ret;
#if 0
            if (ret.second != eValueChangedReturnCodeNothingChanged) {
                (void)evaluateCurveChanged(eCurveChangedReasonKeyframeChanged, ret.first);
            }
#endif
        }

        // Now recompute auto tangents
        for (std::size_t i = 0; i < _imp->keyFrames.size(); ++i) {
            KeyFrameSet::iterator it = _imp->keyFrames.begin();
            std::advance(it, i);
            (void)evaluateCurveChanged(Curve::eCurveChangedReasonKeyframeChanged, it);

        }
   
        onCurveChanged();
    }


}

void
Curve::toSerialization(SERIALIZATION_NAMESPACE::SerializationObjectBase* obj)
{
    SERIALIZATION_NAMESPACE::CurveSerialization* s = dynamic_cast<SERIALIZATION_NAMESPACE::CurveSerialization*>(obj);
    if (!s) {
        return;
    }
    switch (_imp->type) {
        case eCurveTypeString:
        case eCurveTypeChoice:
            s->curveType = SERIALIZATION_NAMESPACE::eCurveSerializationTypeString;
            break;
        case eCurveTypeProperties:
            s->curveType = SERIALIZATION_NAMESPACE::eCurveSerializationTypePropertiesOnly;
            break;

        default:
            s->curveType = SERIALIZATION_NAMESPACE::eCurveSerializationTypeScalar;
            break;
    }
    KeyFrameSet keys = getKeyFrames_mt_safe();
    for (KeyFrameSet::iterator it = keys.begin(); it!=keys.end(); ++it) {

        SERIALIZATION_NAMESPACE::KeyFrameSerialization k;
        k.time = it->getTime();

        switch (s->curveType) {
            case SERIALIZATION_NAMESPACE::eCurveSerializationTypeString: {
                SERIALIZATION_NAMESPACE::KeyFrameProperty prop;
                SERIALIZATION_NAMESPACE::KeyFramePropertyVariant v;
                it->getPropertySafe(kKeyFramePropString, 0, &v.stringValue);
                prop.type = kKeyFramePropertyVariantTypeString;
                prop.values.push_back(v);
                k.properties.push_back(prop);
            }   break;
            case SERIALIZATION_NAMESPACE::eCurveSerializationTypePropertiesOnly: {
                if (it->hasProperties()) {
                    const std::map<std::string, boost::shared_ptr<PropertiesHolder::PropertyBase> >& properties = it->getProperties();
                    for (std::map<std::string, boost::shared_ptr<PropertiesHolder::PropertyBase> >::const_iterator it = properties.begin(); it != properties.end(); ++it) {
                        SERIALIZATION_NAMESPACE::KeyFrameProperty prop;
                        prop.name = it->first;
                        PropertiesHolder::Property<std::string>* isStringProp = dynamic_cast<PropertiesHolder::Property<std::string>*>(it->second.get());
                        PropertiesHolder::Property<double>* isDoubleProp = dynamic_cast<PropertiesHolder::Property<double>*>(it->second.get());
                        PropertiesHolder::Property<int>* isIntProp = dynamic_cast<PropertiesHolder::Property<int>*>(it->second.get());
                        PropertiesHolder::Property<bool>* isBoolProp = dynamic_cast<PropertiesHolder::Property<bool>*>(it->second.get());
                        if (isStringProp) {
                            prop.type = kKeyFramePropertyVariantTypeString;
                            prop.values.resize(isStringProp->value.size());
                            for (std::size_t i = 0; i < prop.values.size(); ++i) {
                                prop.values[i].stringValue = isStringProp->value[i];
                            }
                        } else if (isDoubleProp) {
                            prop.type = kKeyFramePropertyVariantTypeDouble;
                            prop.values.resize(isDoubleProp->value.size());
                            for (std::size_t i = 0; i < prop.values.size(); ++i) {
                                prop.values[i].scalarValue = isDoubleProp->value[i];
                            }
                        } else if (isIntProp) {
                            prop.type = kKeyFramePropertyVariantTypeInt;
                            prop.values.resize(isIntProp->value.size());
                            for (std::size_t i = 0; i < prop.values.size(); ++i) {
                                prop.values[i].scalarValue = isIntProp->value[i];
                            }
                        } else if (isBoolProp) {
                            prop.type = kKeyFramePropertyVariantTypeBool;
                            prop.values.resize(isBoolProp->value.size());
                            for (std::size_t i = 0; i < prop.values.size(); ++i) {
                                prop.values[i].scalarValue = isBoolProp->value[i];
                            }
                        } else {
                            continue;
                        }
                        k.properties.push_back(prop);
                    }
                }
            }   break;
            case SERIALIZATION_NAMESPACE::eCurveSerializationTypeScalar: {
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
                

            }   break;

        }

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
            std::pair<KeyFrameSet::iterator, ValueChangedReturnCodeEnum> ret = setOrUpdateKeyframeInternal(*it);
            ret.first = evaluateCurveChanged(Curve::eCurveChangedReasonKeyframeChanged, ret.first);
        }


    }

    onCurveChanged();

}

void
Curve::setKeyframes(const KeyFrameSet& keys, bool refreshDerivatives)
{
    boost::scoped_ptr<KeyFrameSet> oldKeys;
    std::list<CurveChangesListenerPtr> listeners;
    {
        QMutexLocker k(&_imp->_lock);

        listeners = getListeners();
        if (!listeners.empty()) {
            oldKeys.reset(new KeyFrameSet);
            *oldKeys = _imp->keyFrames;
        }

        setKeyframesInternal(keys, refreshDerivatives);
    }
    if (!listeners.empty()) {
        notifyKeyFramesChanged(listeners, *oldKeys, keys);
    }
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

NATRON_NAMESPACE_EXIT
