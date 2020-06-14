/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2020 The Natron developers
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

#include "BezierCPPrivate.h"
#include "BezierCP.h"

#include <algorithm> // min, max
#include <limits> // INT_MAX
#include <set>
#include <list>
#include <utility>
#include <cassert>
#include <stdexcept>

#include <QtCore/QThread>
#include <QtCore/QCoreApplication>

#include "Engine/Bezier.h"
#include "Engine/KnobTypes.h"
#include "Engine/Transform.h" // Point3D
#include "Engine/ViewIdx.h"

#include "Serialization/BezierCPSerialization.h"


NATRON_NAMESPACE_ENTER


////////////////////////////////////ControlPoint////////////////////////////////////

BezierCP::BezierCP()
    : _imp( new BezierCPPrivate( BezierPtr() ) )
{
}

BezierCP::BezierCP(const BezierCP & other)
    : _imp( new BezierCPPrivate( other._imp->holder.lock() ) )
{
    copyControlPoint(other, 0);
}

BezierCP::BezierCP(const BezierPtr& curve)
    : _imp( new BezierCPPrivate(curve) )
{
}

BezierCP::~BezierCP()
{
}

void
BezierCP::getKeyframeTimes(std::set<double>* keys) const
{
    QMutexLocker l(&_imp->lock);
    KeyFrameSet set;
    set = _imp->curveX->getKeyFrames_mt_safe();

    for (KeyFrameSet::iterator it = set.begin(); it != set.end(); ++it) {
        keys->insert( it->getTime() );
    }
}

bool
BezierCP::getPositionAtTime(TimeValue time,
                            double* x,
                            double* y) const
{
    QMutexLocker l(&_imp->lock);
    bool ret = false;
    KeyFrame k;
    Curve* xCurve = _imp->curveX.get();
    Curve* yCurve = _imp->curveY.get();

    if ( xCurve->getKeyFrameWithTime(time, &k) ) {
        bool ok;
        *x = k.getValue();
        ok = yCurve->getKeyFrameWithTime(time, &k);
        assert(ok);
        if (ok) {
            *y = k.getValue();
            ret = true;
        }
    } else {
        if (xCurve->isAnimated()) {
            *x = xCurve->getValueAt(time).getValue();
            *y = yCurve->getValueAt(time).getValue();
        } else {
            *x = _imp->x;
            *y = _imp->y;
        }

        ret = false;
    }

    return ret;
}

void
BezierCP::setPositionAtTime(TimeValue time,
                            double x,
                            double y)
{
    QMutexLocker l(&_imp->lock);
    {
        KeyFrame k(time, x);
        k.setInterpolation(eKeyframeTypeLinear);
        _imp->curveX->setOrAddKeyframe(k);
    }
    {
        KeyFrame k(time, y);
        k.setInterpolation(eKeyframeTypeLinear); 
        _imp->curveY->setOrAddKeyframe(k);
    }
}

void
BezierCP::setStaticPosition(double x,
                            double y)
{
    QMutexLocker l(&_imp->lock);
    _imp->x = x;
    _imp->y = y;
}

void
BezierCP::setLeftBezierStaticPosition(double x,
                                      double y)
{
    QMutexLocker l(&_imp->lock);
    _imp->leftX = x;
    _imp->leftY = y;
}

void
BezierCP::setRightBezierStaticPosition(double x,
                                       double y)
{
    QMutexLocker l(&_imp->lock);
    _imp->rightX = x;
    _imp->rightY = y;
}

bool
BezierCP::getLeftBezierPointAtTime(TimeValue time,
                                   double* x,
                                   double* y) const
{
    QMutexLocker l(&_imp->lock);
    KeyFrame k;
    bool ret = false;
    Curve* xCurve = _imp->curveLeftBezierX.get();
    Curve* yCurve = _imp->curveLeftBezierY.get();

    if ( xCurve && xCurve->getKeyFrameWithTime(time, &k) ) {
        bool ok;
        *x = k.getValue();
        ok = yCurve && yCurve->getKeyFrameWithTime(time, &k);
        assert(ok);
        if (ok) {
            *y = k.getValue();
            ret = true;
        }
    } else {
        if (xCurve && xCurve->isAnimated()) {
            *x = xCurve->getValueAt(time).getValue();
            *y = yCurve->getValueAt(time).getValue();
        } else {
            *x = _imp->leftX;
            *y = _imp->leftY;
        }

        ret = false;
    }


    return ret;
} // BezierCP::getLeftBezierPointAtTime

bool
BezierCP::getRightBezierPointAtTime(TimeValue time,
                                    double *x,
                                    double *y) const
{
    QMutexLocker l(&_imp->lock);
    KeyFrame k;
    bool ret = false;
    Curve* xCurve = _imp->curveRightBezierX.get();
    Curve* yCurve = _imp->curveRightBezierY.get();

    if ( xCurve->getKeyFrameWithTime(time, &k) ) {
        bool ok;
        *x = k.getValue();
        ok = yCurve->getKeyFrameWithTime(time, &k);
        assert(ok);
        if (ok) {
            *y = k.getValue();
            ret = true;
        }
    } else {
        if (xCurve->isAnimated()) {
            *x = xCurve->getValueAt(time).getValue();
            *y = yCurve->getValueAt(time).getValue();
        } else {
            *x = _imp->rightX;
            *y = _imp->rightY;
        }

        ret =  false;
    }


    return ret;
} // BezierCP::getRightBezierPointAtTime

void
BezierCP::setLeftBezierPointAtTime(TimeValue time,
                                   double x,
                                   double y)
{
    QMutexLocker l(&_imp->lock);
    {
        KeyFrame k(time, x);
        k.setInterpolation(eKeyframeTypeLinear);
        _imp->curveLeftBezierX->setOrAddKeyframe(k);

    }
    {
        KeyFrame k(time, y);
        k.setInterpolation(eKeyframeTypeLinear);
        _imp->curveLeftBezierY->setOrAddKeyframe(k);

    }
}

void
BezierCP::setRightBezierPointAtTime(TimeValue time,
                                    double x,
                                    double y)
{
    QMutexLocker l(&_imp->lock);
    {
        KeyFrame k(time, x);
        k.setInterpolation(eKeyframeTypeLinear);
        _imp->curveRightBezierX->setOrAddKeyframe(k);
    }
    {
        KeyFrame k(time, y);
        k.setInterpolation(eKeyframeTypeLinear);
        _imp->curveRightBezierY->setOrAddKeyframe(k);
    }
}

void
BezierCP::removeKeyframe(TimeValue time)
{

    QMutexLocker l(&_imp->lock);
    ///if the keyframe count reaches 0 update the "static" values which may be fetched
    if (_imp->curveX->getKeyFramesCount() == 1) {
        _imp->x = _imp->curveX->getValueAt(time).getValue();
        _imp->y = _imp->curveY->getValueAt(time).getValue();
        _imp->leftX = _imp->curveLeftBezierX->getValueAt(time).getValue();
        _imp->leftY = _imp->curveLeftBezierY->getValueAt(time).getValue();
        _imp->rightX = _imp->curveRightBezierX->getValueAt(time).getValue();
        _imp->rightY = _imp->curveRightBezierY->getValueAt(time).getValue();
    }

    try {
        _imp->curveX->removeKeyFrameWithTime(time);
        _imp->curveY->removeKeyFrameWithTime(time);
        _imp->curveLeftBezierX->removeKeyFrameWithTime(time);
        _imp->curveLeftBezierY->removeKeyFrameWithTime(time);
        _imp->curveRightBezierX->removeKeyFrameWithTime(time);
        _imp->curveRightBezierY->removeKeyFrameWithTime(time);
    } catch (...) {
    }
}

void
BezierCP::setKeyFrameInterpolation(KeyframeTypeEnum interp,
                                   int index)
{
    QMutexLocker l(&_imp->lock);

    _imp->curveX->setKeyFrameInterpolation(interp, index);
    _imp->curveY->setKeyFrameInterpolation(interp, index);
    _imp->curveLeftBezierX->setKeyFrameInterpolation(interp, index);
    _imp->curveLeftBezierY->setKeyFrameInterpolation(interp, index);
    _imp->curveRightBezierX->setKeyFrameInterpolation(interp, index);
    _imp->curveRightBezierY->setKeyFrameInterpolation(interp, index);
}


int
BezierCP::getControlPointsCount(ViewIdx view) const
{
    BezierPtr b = _imp->holder.lock();

    assert(b);

    return b->getControlPointsCount(view);
}

BezierPtr
BezierCP::getBezier() const
{
    BezierPtr b = _imp->holder.lock();

    assert(b);

    return b;
}

int
BezierCP::isNearbyTangent(TimeValue time,
                          ViewIdx view,
                          double x,
                          double y,
                          double acceptance) const
{
    Transform::Point3D p, left, right;

    p.z = left.z = right.z = 1;

    Transform::Matrix3x3 transform;
    getBezier()->getTransformAtTime(time, view, &transform);

    getPositionAtTime(time, &p.x, &p.y);
    getLeftBezierPointAtTime(time, &left.x, &left.y);
    getRightBezierPointAtTime(time, &right.x, &right.y);

    p = Transform::matApply(transform, p);
    left = Transform::matApply(transform, left);
    right = Transform::matApply(transform, right);

    if ( (p.x != left.x) || (p.y != left.y) ) {
        if ( ( left.x >= (x - acceptance) ) && ( left.x <= (x + acceptance) ) && ( left.y >= (y - acceptance) ) && ( left.y <= (y + acceptance) ) ) {
            return 0;
        }
    }
    if ( (p.x != right.x) || (p.y != right.y) ) {
        if ( ( right.x >= (x - acceptance) ) && ( right.x <= (x + acceptance) ) && ( right.y >= (y - acceptance) ) && ( right.y <= (y + acceptance) ) ) {
            return 1;
        }
    }

    return -1;
}


CurvePtr
BezierCP::getXCurve() const
{
    return _imp->curveX;
}

CurvePtr
BezierCP::getYCurve() const
{
    return _imp->curveY;
}

CurvePtr
BezierCP::getLeftXCurve() const
{
    return _imp->curveLeftBezierX;
}

CurvePtr
BezierCP::getLeftYCurve() const
{
    return _imp->curveLeftBezierY;
}

CurvePtr
BezierCP::getRightXCurve() const
{
    return _imp->curveRightBezierX;
}

CurvePtr
BezierCP::getRightYCurve() const
{
    return _imp->curveRightBezierY;
}

void
BezierCP::copyControlPoint(const BezierCP & other, const RangeD* range)
{


    QMutexLocker l(&other._imp->lock);
    QMutexLocker l2(&_imp->lock);
    const double offset = 0;

    // If a range to copy is passed, round it to the nearest enclosing keyframes
    RangeD roundedRange;
    if (range) {
        roundedRange = *range;
        KeyFrame prevKey;
        if (other._imp->curveX->getPreviousKeyframeTime(TimeValue(range->min), &prevKey)) {
            roundedRange.min = prevKey.getTime();
        }
        KeyFrame nextKey;
        if (other._imp->curveX->getNextKeyframeTime(TimeValue(range->max), &nextKey)) {
            roundedRange.max = nextKey.getTime();
        }
    }

    _imp->curveX->clone(*other._imp->curveX, offset, range ? &roundedRange : 0);
    _imp->curveY->clone(*other._imp->curveY, offset, range ? &roundedRange : 0);
    _imp->curveLeftBezierX->clone(*other._imp->curveLeftBezierX, offset, range ? &roundedRange : 0);
    _imp->curveLeftBezierY->clone(*other._imp->curveLeftBezierY, offset, range ? &roundedRange : 0);
    _imp->curveRightBezierX->clone(*other._imp->curveRightBezierX, offset, range ? &roundedRange : 0);
    _imp->curveRightBezierY->clone(*other._imp->curveRightBezierY, offset, range ? &roundedRange : 0);

    _imp->x = other._imp->x;
    _imp->y = other._imp->y;
    _imp->leftX = other._imp->leftX;
    _imp->leftY = other._imp->leftY;
    _imp->rightX = other._imp->rightX;
    _imp->rightY = other._imp->rightY;

}

bool
BezierCP::equalsAtTime(TimeValue time,
                       const BezierCP & other) const
{
    double x, y, leftX, leftY, rightX, rightY;

    getPositionAtTime(time,  &x, &y);
    getLeftBezierPointAtTime(time,  &leftX, &leftY);
    getRightBezierPointAtTime(time,  &rightX, &rightY);

    double ox, oy, oLeftX, oLeftY, oRightX, oRightY;
    other.getPositionAtTime(time,  &ox, &oy);
    other.getLeftBezierPointAtTime(time,  &oLeftX, &oLeftY);
    other.getRightBezierPointAtTime(time,  &oRightX, &oRightY);

    if ( (x == ox) && (y == oy) && (leftX == oLeftX) && (leftY == oLeftY) && (rightX == oRightX) && (rightY == oRightY) ) {
        return true;
    }

    return false;
}

bool
BezierCP::operator==(const BezierCP& other) const
{
    QMutexLocker l(&_imp->lock);
    QMutexLocker l2(&other._imp->lock);
    if (*_imp->curveX != *other._imp->curveX) {
        return false;
    }
    if (*_imp->curveY != *other._imp->curveY) {
        return false;
    }
    if (*_imp->curveLeftBezierX != *other._imp->curveLeftBezierX) {
        return false;
    }
    if (*_imp->curveLeftBezierY != *other._imp->curveLeftBezierY) {
        return false;
    }
    if (*_imp->curveRightBezierX != *other._imp->curveRightBezierX) {
        return false;
    }
    if (*_imp->curveRightBezierY != *other._imp->curveRightBezierY) {
        return false;
    }

    if (_imp->x != other._imp->x) {
        return false;
    }
    if (_imp->y != other._imp->y) {
        return false;
    }
    if (_imp->leftX != other._imp->leftX) {
        return false;
    }
    if (_imp->leftY != other._imp->leftY) {
        return false;
    }
    if (_imp->rightX != other._imp->rightX) {
        return false;
    }
    if (_imp->rightY != other._imp->rightY) {
        return false;
    }
    return true;
    
}

void
BezierCP::toSerialization(SERIALIZATION_NAMESPACE::SerializationObjectBase* obj)
{
    SERIALIZATION_NAMESPACE::BezierCPSerialization* s = dynamic_cast<SERIALIZATION_NAMESPACE::BezierCPSerialization*>(obj);
    if (!s) {
        return;
    }
    QMutexLocker l(&_imp->lock);
    _imp->curveX->toSerialization(&s->xCurve);
    _imp->curveY->toSerialization(&s->yCurve);
    _imp->curveLeftBezierX->toSerialization(&s->leftCurveX);
    _imp->curveLeftBezierY->toSerialization(&s->leftCurveY);
    _imp->curveRightBezierX->toSerialization(&s->rightCurveX);
    _imp->curveRightBezierY->toSerialization(&s->rightCurveY);

    s->x = _imp->x;
    s->y = _imp->y;
    s->leftX = _imp->leftX;
    s->leftY = _imp->leftY;
    s->rightX = _imp->rightX;
    s->rightY = _imp->rightY;
}

void
BezierCP::fromSerialization(const SERIALIZATION_NAMESPACE::SerializationObjectBase & obj)
{
    const SERIALIZATION_NAMESPACE::BezierCPSerialization* s = dynamic_cast<const SERIALIZATION_NAMESPACE::BezierCPSerialization*>(&obj);
    if (!s) {
        return;
    }
    QMutexLocker l(&_imp->lock);

    _imp->curveX->fromSerialization(s->xCurve);
    _imp->curveY->fromSerialization(s->yCurve);
    _imp->curveLeftBezierX->fromSerialization(s->leftCurveX);
    _imp->curveLeftBezierY->fromSerialization(s->leftCurveY);
    _imp->curveRightBezierX->fromSerialization(s->rightCurveX);
    _imp->curveRightBezierY->fromSerialization(s->rightCurveY);

    _imp->x = s->x;
    _imp->y = s->y;
    _imp->leftX = s->leftX;
    _imp->leftY = s->leftY;
    _imp->rightX = s->rightX;
    _imp->rightY = s->rightY;

}

NATRON_NAMESPACE_EXIT

