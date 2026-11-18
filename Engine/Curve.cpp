/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2023 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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
#include <iterator>
#include <cassert>
#include <limits>
#include <stdexcept>

#include "Engine/AppManager.h"

#include "Engine/CurvePrivate.h"
#include "Engine/Interpolation.h"
#include "Engine/KnobTypes.h"
#include "Engine/KnobFile.h"
#include "Engine/Smooth1D.h"
#include "Global/MathUtils.h"

NATRON_NAMESPACE_ENTER


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
    assert( !std::isnan(_time) && !std::isinf(_time) );
    assert( !std::isnan(_value) && !std::isinf(_value) );
    assert( !std::isnan(_leftDerivative) && !std::isinf(_leftDerivative) );
    assert( !std::isnan(_rightDerivative) && !std::isinf(_rightDerivative) );
}

KeyFrame::KeyFrame(const KeyFrame & other)
    : _time(other._time)
    , _value(other._value)
    , _leftDerivative(other._leftDerivative)
    , _rightDerivative(other._rightDerivative)
    , _interpolation(other._interpolation)
{
    assert( !std::isnan(_time) && !std::isinf(_time) );
    assert( !std::isnan(_value) && !std::isinf(_value) );
    assert( !std::isnan(_leftDerivative) && !std::isinf(_leftDerivative) );
    assert( !std::isnan(_rightDerivative) && !std::isinf(_rightDerivative) );
}

void
KeyFrame::operator=(const KeyFrame & o)
{
    _time = o._time;
    _value = o._value;
    _leftDerivative = o._leftDerivative;
    _rightDerivative = o._rightDerivative;
    _interpolation = o._interpolation;
    assert( !std::isnan(_time) && !std::isinf(_time) );
    assert( !std::isnan(_value) && !std::isinf(_value) );
    assert( !std::isnan(_leftDerivative) && !std::isinf(_leftDerivative) );
    assert( !std::isnan(_rightDerivative) && !std::isinf(_rightDerivative) );
}

KeyFrame::~KeyFrame()
{
}

void
KeyFrame::setLeftDerivative(double leftDerivative)
{
    _leftDerivative = leftDerivative;
    assert( !std::isnan(_leftDerivative) && !std::isinf(_leftDerivative) );
}

void
KeyFrame::setRightDerivative(double rightDerivative)
{
    _rightDerivative = rightDerivative;
    assert( !std::isnan(_rightDerivative) && !std::isinf(_rightDerivative) );
}

void
KeyFrame::setValue(double value)
{
    _value = value;
    assert( !std::isnan(_value) && !std::isinf(_value) );
}

void
KeyFrame::setTime(double time)
{
    _time = time;
    assert( !std::isnan(_time) && !std::isinf(_time) );
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

Curve::Curve(KnobI *owner,
             int dimensionInOwner)
    : _imp(new CurvePrivate)
{
    assert(owner);
    _imp->owner = owner;
    _imp->dimensionInOwner = dimensionInOwner;
    //std::string typeName = _imp->owner->typeName(); // crashes because the Knob constructor is not finished at this point
    bool found = false;
    // use RTTI to guess curve type
    if (!found) {
        KnobDouble* k = dynamic_cast<KnobDouble*>(owner);
        if (k) {
            _imp->type = CurvePrivate::eCurveTypeDouble;
            found = true;
        }
    }
    if (!found) {
        KnobColor* k = dynamic_cast<KnobColor*>(owner);
        if (k) {
            _imp->type = CurvePrivate::eCurveTypeDouble;
            found = true;
        }
    }
    if (!found) {
        KnobInt* k = dynamic_cast<KnobInt*>(owner);
        if (k) {
            _imp->type = CurvePrivate::eCurveTypeInt;
            found = true;
        }
    }
    if (!found) {
        KnobChoice* k = dynamic_cast<KnobChoice*>(owner);
        if (k) {
            _imp->type = CurvePrivate::eCurveTypeIntConstantInterp;
            found = true;
        }
    }
    if (!found) {
        KnobString* k = dynamic_cast<KnobString*>(owner);
        if (k) {
            _imp->type = CurvePrivate::eCurveTypeString;
            found = true;
        }
    }
    if (!found) {
        KnobFile* k = dynamic_cast<KnobFile*>(owner);
        if (k) {
            _imp->type = CurvePrivate::eCurveTypeString;
            found = true;
        }
    }
    if (!found) {
        KnobOutputFile* k = dynamic_cast<KnobOutputFile*>(owner);
        if (k) {
            _imp->type = CurvePrivate::eCurveTypeString;
            found = true;
        }
    }
    if (!found) {
        KnobPath* k = dynamic_cast<KnobPath*>(owner);
        if (k) {
            _imp->type = CurvePrivate::eCurveTypeString;
            found = true;
        }
    }
    if (!found) {
        KnobBool* k = dynamic_cast<KnobBool*>(owner);
        if (k) {
            _imp->type = CurvePrivate::eCurveTypeBool;
            found = true;
        }
    }

    if (!found) {
        KnobParametric* parametric = dynamic_cast<KnobParametric*>(owner);
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

    return !_imp->isParametric;
}

void
Curve::clone(const Curve & other)
{
    // Read other's keys and flag under other's lock only, before locking this:
    // locking both (A.clone(B) and B.clone(A) at the same time) could deadlock.
    KeyFrameSet otherKeys;
    bool mustRefreshDerivatives;
    {
        QMutexLocker lo(&other._imp->_lock);
        otherKeys = other._imp->keyFrames;
        mustRefreshDerivatives = other._imp->mustRefreshDerivatives;
    }
    QMutexLocker l(&_imp->_lock);

    _imp->keyFrames.clear();
    std::transform( otherKeys.begin(), otherKeys.end(), std::inserter( _imp->keyFrames, _imp->keyFrames.begin() ), KeyFrameCloner() );
    // The flag is not propagated to this curve: it is set explicitly only on the curves of
    // the temporary knobs of KnobSerialization (see setMustRefreshDerivatives()), and the
    // final curve, cloned from them, keeps no flag.
    if (mustRefreshDerivatives) {
        // other was loaded from a file with NaN values: its derivatives must be recomputed
        // with this curve's properties (e.g. periodicity)
        refreshAutomaticDerivatives();
    }
    onCurveChanged();
}

void
Curve::setMustRefreshDerivatives()
{
    QMutexLocker l(&_imp->_lock);

    _imp->mustRefreshDerivatives = true;
}

bool
Curve::getMustRefreshDerivatives() const
{
    QMutexLocker l(&_imp->_lock);

    return _imp->mustRefreshDerivatives;
}

void
Curve::refreshAutomaticDerivatives()
{
    QMutexLocker l(&_imp->_lock);

    for (KeyFrameSet::iterator it = _imp->keyFrames.begin(); it != _imp->keyFrames.end(); ++it) {
        KeyframeTypeEnum interp = it->getInterpolation();
        if ( (interp != eKeyframeTypeFree) && (interp != eKeyframeTypeBroken) && (interp != eKeyframeTypeNone) ) {
            // refreshDerivatives() replaces the keyframe, and returns an iterator on the new one
            it = refreshDerivatives(eCurveChangedReasonDerivativesChanged, it);
        }
    }
}

bool
Curve::cloneAndCheckIfChanged(const Curve& other)
{
    // Read other's keys and flag under other's lock only, before locking this (see clone()).
    KeyFrameSet otherKeys;
    bool mustRefreshDerivatives;
    {
        QMutexLocker lo(&other._imp->_lock);
        otherKeys = other._imp->keyFrames;
        mustRefreshDerivatives = other._imp->mustRefreshDerivatives;
    }
    QMutexLocker l(&_imp->_lock);

    if (mustRefreshDerivatives) {
        // other was loaded from a file with NaN values (#1267): copy the keys and recompute
        // the derivatives with this curve's properties (e.g. periodicity), as clone() does.
        // This is the last step of loading a project (Node::loadKnob() -> cloneAndUpdateGui()).
        _imp->keyFrames.clear();
        std::transform( otherKeys.begin(), otherKeys.end(), std::inserter( _imp->keyFrames, _imp->keyFrames.begin() ), KeyFrameCloner() );
        refreshAutomaticDerivatives();
        onCurveChanged();

        return true;
    }

    bool hasChanged = false;

    if ( otherKeys.size() != _imp->keyFrames.size() ) {
        hasChanged = true;
    }
    if (!hasChanged) {
        assert( otherKeys.size() == _imp->keyFrames.size() );
        KeyFrameSet::iterator oit = otherKeys.begin();
        for (KeyFrameSet::iterator it = _imp->keyFrames.begin(); it != _imp->keyFrames.end(); ++it, ++oit) {
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

    // the default interpolation for bool, string, chaice, int is constant
    if ( (_imp->type == CurvePrivate::eCurveTypeBool) || (_imp->type == CurvePrivate::eCurveTypeString) ||
         ( _imp->type == CurvePrivate::eCurveTypeInt) ||
         ( _imp->type == CurvePrivate::eCurveTypeIntConstantInterp) ) {
        key.setInterpolation(eKeyframeTypeConstant);
    }


    std::pair<KeyFrameSet::iterator, bool> it = addKeyFrameNoUpdate(key);

    it.first = evaluateCurveChanged(eCurveChangedReasonKeyframeChanged, it.first);
    l.unlock();

    return it.second;
}

std::pair<KeyFrameSet::iterator, bool> Curve::addKeyFrameNoUpdate(const KeyFrame & cp)
{
    // PRIVATE - should not lock
    if (!_imp->isParametric) { //< if keyframes are clamped to integers
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
    KeyFrame prevKey;
    bool mustRefreshPrev = false;
    KeyFrame nextKey;
    bool mustRefreshNext = false;

    KeyFrameSet::iterator next = it;
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
        refreshDerivatives( eCurveChangedReasonDerivativesChanged, find( prevKey.getTime() ) );
    }
    if (mustRefreshNext) {
        refreshDerivatives( eCurveChangedReasonDerivativesChanged, find( nextKey.getTime() ) );
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
Curve::removeKeyFramesBeforeTime(double time,
                                 std::list<int>* keyframeRemoved)
{
    KeyFrameSet newSet;
    QMutexLocker l(&_imp->_lock);

    for (KeyFrameSet::iterator it = _imp->keyFrames.begin(); it != _imp->keyFrames.end(); ++it) {
        if (it->getTime() < time) {
            keyframeRemoved->push_back( it->getTime() );
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
Curve::removeKeyFramesAfterTime(double time,
                                std::list<int>* keyframeRemoved)
{
    KeyFrameSet newSet;
    QMutexLocker l(&_imp->_lock);

    for (KeyFrameSet::iterator it = _imp->keyFrames.begin(); it != _imp->keyFrames.end(); ++it) {
        if (it->getTime() > time) {
            keyframeRemoved->push_back( it->getTime() );
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
            bool isPeriodic,
            double xMin,
            double xMax,
            double *t,
            KeyFrameSet::const_iterator itup,
            double *tcur,
            double *vcur,
            double *vcurDerivRight,
            KeyframeTypeEnum *interp,
            double *tnext,
            double *vnext,
            double *vnextDerivLeft,
            KeyframeTypeEnum *interpNext)
{

    assert(keyFrames.size() >= 1);
    assert( itup == keyFrames.end() || *t < itup->getTime() );
    double period = xMax - xMin;
    // the default range is unbounded, and its period (DBL_MAX - lowest()) is infinite:
    // such a curve is not periodic
    if ( !(period > 0.) || !MathUtils::isFinite(period) ) {
        isPeriodic = false;
    }
    if (isPeriodic) {
        // If the curve is periodic, bring t back into the half-open period
        // [minKeyFrameX, minKeyFrameX + period), which starts at the first keyframe.
        // The period must be half-open: a keyframe at minKeyFrameX + period is the same
        // point as the first keyframe, and t = minKeyFrameX + period must not use the
        // zero-width segment between them (see #1267).
        double minKeyFrameX = keyFrames.begin()->getTime();
        assert(xMin < xMax);
        if (*t < minKeyFrameX || *t >= minKeyFrameX + period) {
            // This brings t into minKeyFrameX - period < t < minKeyFrameX + period
            *t = std::fmod(*t - minKeyFrameX, period ) + minKeyFrameX;
            if (*t < minKeyFrameX) {
                *t += period;
            }
            if (*t >= minKeyFrameX + period) {
                // rounding error in the line above
                *t = minKeyFrameX;
            }
            // (with large times, minKeyFrameX + period may round to minKeyFrameX)
            assert( *t >= minKeyFrameX && (*t < minKeyFrameX + period || minKeyFrameX + period == minKeyFrameX) );
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
            *tcur = last->getTime() - period;
            *vcur = last->getValue();
            *vcurDerivRight = last->getRightDerivative();
            *interp = last->getInterpolation();
        } else {
            *tnext = itup->getTime();
            *vnext = itup->getValue();
            *vnextDerivLeft = itup->getLeftDerivative();
            *interpNext = itup->getInterpolation();
            *tcur = *tnext - 1.;
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
            *tnext = next->getTime() + period;
            *vnext = next->getValue();
            *vnextDerivLeft = next->getLeftDerivative();
            *interpNext = next->getInterpolation();
        } else {

            KeyFrameSet::const_reverse_iterator itlast = keyFrames.rbegin();
            *tcur = itlast->getTime();
            *vcur = itlast->getValue();
            *vcurDerivRight = itlast->getRightDerivative();
            *interp = itlast->getInterpolation();
            *tnext = *tcur + 1.;
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

/// index of the last keyframe at or before time, or -1
static int
keyFramesAround(const KeyFrameSet& keyFrames,
                double time,
                KeyFrame* prev,
                KeyFrame* next,
                bool* hasNext)
{
    // the first keyframe after time
    KeyFrameSet::const_iterator upper = keyFrames.upper_bound( KeyFrame(time, 0.) );

    if (next) {
        assert(hasNext);
        *hasNext = ( upper != keyFrames.end() );
        if (*hasNext) {
            *next = *upper;
        }
    }
    if ( upper == keyFrames.begin() ) {
        return -1;
    }
    KeyFrameSet::const_iterator lower = upper;
    --lower;
    if (prev) {
        *prev = *lower;
    }

    return (int)std::distance(keyFrames.begin(), lower);
}

double
Curve::getValueAt(double t,
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

    if (_imp->type == CurvePrivate::eCurveTypeString) {
        // the keyframe values reference strings and cannot be interpolated: see getKeyFramesForDisplay_mt_safe()
        int index = keyFramesAround(_imp->keyFrames, t, NULL, NULL, NULL);

        return index < 0 ? 0. : (double)index;
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
        double tcur, tnext;
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
    double tcur, tnext;
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

    if ( mustClamp() ) {
        Curve::YRange minmax = getCurveYRange();
        d = Interpolation::derive_clamp(tcur, vcur,
                                        vcurDerivRight,
                                        vnextDerivLeft,
                                        tnext, vnext,
                                        t,
                                        minmax.min, minmax.max,
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
Curve::getIntegrateFromTo(double t1,
                          double t2) const
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
    assert(_imp->type == CurvePrivate::eCurveTypeDouble); // only real-valued curves can be derived

    // even when there is only one keyframe, there may be tangents!
    //if (_imp->keyFrames.size() == 1) {
    //    //if there's only 1 keyframe, don't bother interpolating
    //    return (*_imp->keyFrames.begin()).getValue();
    //}
    double tcur, tnext;
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
        if ( mustClamp() ) {
            Curve::YRange minmax = getCurveYRange();
            sum += Interpolation::integrate_clamp(tcur, vcur,
                                                  vcurDerivRight,
                                                  vnextDerivLeft,
                                                  tnext, vnext,
                                                  t1, itup->getTime(),
                                                  minmax.min, minmax.max,
                                                  interp,
                                                  interpNext);
        } else {
            sum += Interpolation::integrate(tcur, vcur,
                                            vcurDerivRight,
                                            vnextDerivLeft,
                                            tnext, vnext,
                                            t1, itup->getTime(),
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
    if ( mustClamp() ) {
        Curve::YRange minmax = getCurveYRange();
        sum += Interpolation::integrate_clamp(tcur, vcur,
                                              vcurDerivRight,
                                              vnextDerivLeft,
                                              tnext, vnext,
                                              t1, t2,
                                              minmax.min, minmax.max,
                                              interp,
                                              interpNext);
    } else {
        sum += Interpolation::integrate(tcur, vcur,
                                        vcurDerivRight,
                                        vnextDerivLeft,
                                        tnext, vnext,
                                        t1, t2,
                                        interp,
                                        interpNext);
    }

    return opposite ? -sum : sum;
} // getIntegrateFromTo

Curve::YRange
Curve::getCurveDisplayYRange() const
{
    QMutexLocker l(&_imp->_lock);

    if ( !mustClamp() ) {
        return YRange( std::numeric_limits<double>::lowest(), std::numeric_limits<double>::max() );
    }
    if (!_imp->owner) {
        return YRange(_imp->yMin, _imp->yMax);
    }

    KnobDoubleBase* isDouble = dynamic_cast<KnobDoubleBase*>(_imp->owner);
    KnobIntBase* isInt = dynamic_cast<KnobIntBase*>(_imp->owner);
    if (isDouble) {
        double min = isDouble->getDisplayMinimum(_imp->dimensionInOwner);
        if (min <= std::numeric_limits<double>::lowest()) {
            min = std::numeric_limits<double>::lowest();
        }
        double max = isDouble->getDisplayMaximum(_imp->dimensionInOwner);
        if (max >= std::numeric_limits<double>::max()) {
            max = std::numeric_limits<double>::max();
        }

        return YRange(min, max);
    } else if (isInt) {
        double min = isInt->getDisplayMinimum(_imp->dimensionInOwner);
        double max = isInt->getDisplayMaximum(_imp->dimensionInOwner);

        return YRange(min, max);
    } else {
        return YRange( std::numeric_limits<double>::lowest(), std::numeric_limits<double>::max() );
    }
}

Curve::YRange Curve::getCurveYRange() const
{
    QMutexLocker l(&_imp->_lock);

    if ( !mustClamp() ) {
        return YRange( std::numeric_limits<double>::lowest(), std::numeric_limits<double>::max() );
    }
    if (!_imp->owner) {
        return YRange(_imp->yMin, _imp->yMax);
    }

    KnobDoubleBase* isDouble = dynamic_cast<KnobDoubleBase*>(_imp->owner);
    KnobIntBase* isInt = dynamic_cast<KnobIntBase*>(_imp->owner);
    if (isDouble) {
        double min = isDouble->getMinimum(_imp->dimensionInOwner);
        if (min <= std::numeric_limits<double>::lowest()) {
            min = std::numeric_limits<double>::lowest();
        }
        double max = isDouble->getMaximum(_imp->dimensionInOwner);
        if (max >= std::numeric_limits<double>::max()) {
            max = std::numeric_limits<double>::max();
        }

        return YRange(min, max);
    } else if (isInt) {
        double min = isInt->getMinimum(_imp->dimensionInOwner);
        double max = isInt->getMaximum(_imp->dimensionInOwner);

        return YRange(min, max);
    } else {
        return YRange( std::numeric_limits<double>::lowest(), std::numeric_limits<double>::max() );
    }

}

double
Curve::clampValueToCurveYRange(double v) const
{
    // PRIVATE - should not lock
    ////clamp to min/max if the owner of the curve is a Double or Int knob.
    YRange minmax = getCurveYRange();

    if (v > minmax.max) {
        return minmax.max;
    } else if (v < minmax.min) {
        return minmax.min;
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

KeyFrameSet
Curve::getKeyFramesForDisplay_mt_safe() const
{
    QMutexLocker l(&_imp->_lock);

    if (_imp->type != CurvePrivate::eCurveTypeString) {
        return _imp->keyFrames;
    }
    KeyFrameSet ret;
    int index = 0;
    for (KeyFrameSet::const_iterator it = _imp->keyFrames.begin(); it != _imp->keyFrames.end(); ++it, ++index) {
        KeyFrame k(*it);
        k.setValue(index);
        ret.insert(ret.end(), k);
    }

    return ret;
}

int
Curve::getKeyFramesAround(double time,
                          KeyFrame* prev,
                          KeyFrame* next,
                          bool* hasNext) const
{
    QMutexLocker l(&_imp->_lock);

    return keyFramesAround(_imp->keyFrames, time, prev, next, hasNext);
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
Curve::setKeyFrameValueAndTimeInternal(double time, double value, int index, int* newIndex)
{
    KeyFrame ret;
    {
        QMutexLocker l(&_imp->_lock);
        KeyFrameSet::iterator it = atIndex(index);
        if ( it == _imp->keyFrames.end() ) {
            QString err = QString( QString::fromUtf8("No such keyframe at index %1") ).arg(index);
            throw std::invalid_argument( err.toStdString() );
        }

        if (_imp->type == CurvePrivate::eCurveTypeString) {
            // the value references a string (see StringAnimationManager): only the time can change
            value = it->getValue();
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
Curve::setKeyFrameValueAndTime(double time,
                               double value,
                               int index,
                               int* newIndex)
{
    return setKeyFrameValueAndTimeInternal(time, value, index, newIndex);
}

bool
Curve::moveKeyFrameValueAndTimeInternal(const double time, const double dt, const double dv, KeyFrame* newKey)
{
    bool isFirst = false;
    bool isLast = false;
    {
        QMutexLocker l(&_imp->_lock);
        KeyFrameSet::iterator it = find(time);

        if (_imp->isPeriodic) {
            if (it == _imp->keyFrames.begin()) {
                isFirst = true;
            } else if (it != _imp->keyFrames.end()) {
                KeyFrameSet::iterator next = it;
                ++next;
                if (next == _imp->keyFrames.end()) {
                    isLast = true;
                }
            }
        }

        if ( it == _imp->keyFrames.end() ) {
            return false;
        }
        if ( (dt == 0) && (dv == 0) ) {
            return true;
        }

        double newX = it->getTime() + dt;
        double newY = it->getValue() + dv;
        if (_imp->type == CurvePrivate::eCurveTypeString) {
            // the value references a string (see StringAnimationManager): only the time can move
            newY = it->getValue();
        } else if (_imp->type == CurvePrivate::eCurveTypeInt) {
            newY = std::floor(newY + 0.5);
        } else if (_imp->type == CurvePrivate::eCurveTypeBool) {
            newY = newY < 0.5 ? 0 : 1;
        }

        it = setKeyFrameValueAndTimeNoUpdate(newY, newX, it);
        it = evaluateCurveChanged(eCurveChangedReasonKeyframeChanged, it);
        if (newKey) {
            *newKey = *it;
        }
    }
    Q_UNUSED(isFirst);
    Q_UNUSED(isLast);
    return true;
}

bool
Curve::moveKeyFrameValueAndTime(const double time,
                                const double dt,
                                const double dv,
                                KeyFrame* newKey)
{
    return moveKeyFrameValueAndTimeInternal(time, dt, dv, newKey);
}

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
        if ( ( (_imp->type == CurvePrivate::eCurveTypeString) || (_imp->type == CurvePrivate::eCurveTypeBool) ||
              ( _imp->type == CurvePrivate::eCurveTypeIntConstantInterp) ) && ( interp != eKeyframeTypeConstant) ) {
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

void
Curve::setCurveInterpolation(KeyframeTypeEnum interp)
{
    {
        QMutexLocker l(&_imp->_lock);
        ///if the curve is a string_curve or bool_curve the interpolation is bound to be constant.
        if ( ( (_imp->type == CurvePrivate::eCurveTypeString) || (_imp->type == CurvePrivate::eCurveTypeBool) ||
               ( _imp->type == CurvePrivate::eCurveTypeIntConstantInterp) ) && ( interp != eKeyframeTypeConstant) ) {
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
    // periodic, with a positive finite period (the default range is unbounded, and its period is infinite)
    const bool effectivelyPeriodic = _imp->isPeriodic && (period > 0.) && MathUtils::isFinite(period);
    assert( key != _imp->keyFrames.end() );
    if ( key == _imp->keyFrames.begin() ) {
        if (effectivelyPeriodic && _imp->keyFrames.size() > 1) {
            // The previous keyframe is the last one, shifted by one period.
            // Skip the keyframes that would not be strictly before tcur once shifted, e.g.
            // a keyframe at tcur + period, which is the same point as this one (see #1267).
            // At most size() - 1 keys are skipped: the loop normally ends on key itself, since
            // period > 0, but with large times tcur - period may round to tcur. The degenerate
            // neighbour that remains is then ignored by Interpolation::autoComputeDerivatives().
            KeyFrameSet::const_reverse_iterator prev = _imp->keyFrames.rbegin();
            for (std::size_t skipped = 1; skipped < _imp->keyFrames.size() && prev->getTime() - period >= tcur; ++skipped) {
                ++prev;
            }
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
        if ( !effectivelyPeriodic && ( prev == _imp->keyFrames.begin() ) && (prevType != eKeyframeTypeFree) &&
             ( prevType != eKeyframeTypeBroken) ) {
            prevType = eKeyframeTypeLinear;
        }
    }

    KeyFrameSet::const_iterator next = key;
    if ( next != _imp->keyFrames.end() ) {
        ++next;
    }
    if ( next == _imp->keyFrames.end() ) {
        if (effectivelyPeriodic && _imp->keyFrames.size() > 1) {
            // The next keyframe is the first one, shifted by one period.
            // Skip the keyframes that would not be strictly after tcur once shifted, e.g.
            // a keyframe at tcur - period, which is the same point as this one (see #1267).
            // At most size() - 1 keys are skipped (see the previous keyframe above).
            KeyFrameSet::const_iterator start = _imp->keyFrames.begin();
            for (std::size_t skipped = 1; skipped < _imp->keyFrames.size() && start->getTime() + period <= tcur; ++skipped) {
                ++start;
            }
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
        if ( !effectivelyPeriodic && ( nextnext == _imp->keyFrames.end() ) && (nextType != eKeyframeTypeFree) &&
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
                    double time)
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

    return _imp->type != CurvePrivate::eCurveTypeString;
}

bool
Curve::isStringCurve() const
{
    // the type does not change after construction
    return _imp->type == CurvePrivate::eCurveTypeString;
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
}

bool
Curve::mustClamp() const
{
    // PRIVATE - should not lock
    // unbounded is +/-DBL_MAX (an infinite bound would also be unbounded)
    return _imp->owner || _imp->yMin > std::numeric_limits<double>::lowest() || _imp->yMax < std::numeric_limits<double>::max();
}

void
Curve::onCurveChanged()
{
    // PRIVATE - should not lock
    if (_imp->owner) {
        _imp->owner->clearExpressionsResults(_imp->dimensionInOwner);
    }
#ifdef NATRON_CURVE_USE_CACHE
    _imp->resultCache.clear();
#endif
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
        double time = it->getTime();
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
