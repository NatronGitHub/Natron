/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
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
#include <stdexcept>

#include <QtCore/QThread>
#include <QtCore/QCoreApplication>

#include "Engine/Bezier.h"
#include "Engine/KnobTypes.h"
#include "Engine/Transform.h" // Point3D

NATRON_NAMESPACE_USING


////////////////////////////////////ControlPoint////////////////////////////////////

BezierCP::BezierCP()
    : _imp( new BezierCPPrivate(boost::shared_ptr<Bezier>()) )
{
}

BezierCP::BezierCP(const BezierCP & other)
    : _imp( new BezierCPPrivate(other._imp->holder.lock()) )
{
    clone(other);
}

BezierCP::BezierCP(const boost::shared_ptr<Bezier>& curve)
    : _imp( new BezierCPPrivate(curve) )
{
}

BezierCP::~BezierCP()
{
}

bool
BezierCP::getPositionAtTime(bool useGuiCurves,
                            double time,
                            double* x,
                            double* y,
                            bool skipMasterOrRelative) const
{
    bool ret = false;
    KeyFrame k;
    Curve* xCurve = useGuiCurves ? _imp->guiCurveX.get() : _imp->curveX.get();
    Curve* yCurve = useGuiCurves ? _imp->guiCurveY.get() : _imp->curveY.get();
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
        try {
            *x = xCurve->getValueAt(time);
            *y = yCurve->getValueAt(time);
        } catch (const std::exception & e) {
            QMutexLocker l(&_imp->staticPositionMutex);
            *x = useGuiCurves ? _imp->guiX : _imp->x;
            *y = useGuiCurves ? _imp->guiY : _imp->y;
        }

        ret = false;
    }

    if (!skipMasterOrRelative) {
        SequenceTime offsetTime;
        KnobDouble* masterTrack;
        {
            QReadLocker l(&_imp->masterMutex);
            offsetTime = _imp->offsetTime;
            masterTrack = _imp->masterTrack ? _imp->masterTrack.get() : NULL;
        }
        if (masterTrack) {
            double masterX = masterTrack->getValueAtTime(time,0);
            double masterY = masterTrack->getValueAtTime(time,1);
            double masterOffsetTimeX = masterTrack->getValueAtTime(offsetTime,0);
            double masterOffsetTimeY = masterTrack->getValueAtTime(offsetTime,1);
            *x += (masterX - masterOffsetTimeX);
            *y += (masterY - masterOffsetTimeY);
        }
    }

    return ret;
}

void
BezierCP::setPositionAtTime(bool useGuiCurves,
                            double time,
                            double x,
                            double y)
{
    {
        KeyFrame k(time,x);
        k.setInterpolation(eKeyframeTypeLinear);
        if (!useGuiCurves) {
            _imp->curveX->addKeyFrame(k);
        } else {
            _imp->guiCurveX->addKeyFrame(k);
        }
    }
    {
        KeyFrame k(time,y);
        k.setInterpolation(eKeyframeTypeLinear);
        if (!useGuiCurves) {
            _imp->curveY->addKeyFrame(k);
        } else {
            _imp->guiCurveY->addKeyFrame(k);
        }
    }
}

void
BezierCP::setStaticPosition(bool useGuiCurves,
                            double x,
                            double y)
{
    QMutexLocker l(&_imp->staticPositionMutex);
    if (!useGuiCurves) {
        _imp->x = x;
        _imp->y = y;
    } else {
        _imp->guiX = x;
        _imp->guiY = y;
    }
}

void
BezierCP::setLeftBezierStaticPosition(bool useGuiCurves,
                                      double x,
                                      double y)
{
    QMutexLocker l(&_imp->staticPositionMutex);
    if (!useGuiCurves) {
        _imp->leftX = x;
        _imp->leftY = y;
    } else {
        _imp->guiLeftX = x;
        _imp->guiLeftY = y;
    }
}

void
BezierCP::setRightBezierStaticPosition(bool useGuiCurves,
                                       double x,
                                       double y)
{
    QMutexLocker l(&_imp->staticPositionMutex);
    if (!useGuiCurves) {
        _imp->rightX = x;
        _imp->rightY = y;
    } else {
        _imp->guiRightX = x;
        _imp->guiRightY = y;
    }
}

bool
BezierCP::getLeftBezierPointAtTime(bool useGuiCurves,
                                   double time,
                                   double* x,
                                   double* y,
                                   bool skipMasterOrRelative) const
{
    KeyFrame k;
    bool ret = false;

    Curve* xCurve = useGuiCurves ? _imp->guiCurveLeftBezierX.get() : _imp->curveLeftBezierX.get();
    Curve* yCurve = useGuiCurves ? _imp->guiCurveLeftBezierY.get() : _imp->curveLeftBezierY.get();
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
        try {
            *x = xCurve->getValueAt(time);
            *y = yCurve->getValueAt(time);
        } catch (const std::exception & e) {
            QMutexLocker l(&_imp->staticPositionMutex);
            if (!useGuiCurves) {
                *x = _imp->leftX;
                *y = _imp->leftY;
            } else {
                *x = _imp->guiLeftX;
                *y = _imp->guiLeftY;
            }
        }

        ret = false;
    }

    if (!skipMasterOrRelative) {
        KnobDouble* masterTrack;
        SequenceTime offsetTime;
        {
            QReadLocker l(&_imp->masterMutex);
            masterTrack = _imp->masterTrack ? _imp->masterTrack.get() : NULL;
            offsetTime = _imp->offsetTime;
        }
        if (masterTrack) {
            double masterX = masterTrack->getValueAtTime(time,0);
            double masterY = masterTrack->getValueAtTime(time,1);
            double masterOffsetTimeX = masterTrack->getValueAtTime(offsetTime,0);
            double masterOffsetTimeY = masterTrack->getValueAtTime(offsetTime,1);
            *x += (masterX - masterOffsetTimeX);
            *y += (masterY - masterOffsetTimeY);
        }
    }

    return ret;
}

bool
BezierCP::getRightBezierPointAtTime(bool useGuiCurves,
                                    double time,
                                    double *x,
                                    double *y,
                                    bool skipMasterOrRelative) const
{
    KeyFrame k;
    bool ret = false;
    Curve* xCurve = useGuiCurves ? _imp->guiCurveRightBezierX.get() : _imp->curveRightBezierX.get();
    Curve* yCurve = useGuiCurves ? _imp->guiCurveRightBezierY.get() : _imp->curveRightBezierY.get();
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
        try {
            *x = xCurve->getValueAt(time);
            *y = yCurve->getValueAt(time);
        } catch (const std::exception & e) {
            QMutexLocker l(&_imp->staticPositionMutex);
            if (!useGuiCurves) {
                *x = _imp->rightX;
                *y = _imp->rightY;
            } else {
                *x = _imp->guiRightX;
                *y = _imp->guiRightY;
            }
        }

        ret =  false;
    }


    if (!skipMasterOrRelative) {
        KnobDouble* masterTrack;
        SequenceTime offsetTime;
        {
            QReadLocker l(&_imp->masterMutex);
            masterTrack = _imp->masterTrack ? _imp->masterTrack.get() : NULL;
            offsetTime = _imp->offsetTime;
        }
        if (masterTrack) {
            double masterX = masterTrack->getValueAtTime(time,0);
            double masterY = masterTrack->getValueAtTime(time,1);
            double masterOffsetTimeX = masterTrack->getValueAtTime(offsetTime,0);
            double masterOffsetTimeY = masterTrack->getValueAtTime(offsetTime,1);
            *x += (masterX - masterOffsetTimeX);
            *y += (masterY - masterOffsetTimeY);
        }
    }

    return ret;
}

void
BezierCP::setLeftBezierPointAtTime(bool useGuiCurves,
                                   double time,
                                   double x,
                                   double y)
{
    {
        KeyFrame k(time,x);
        k.setInterpolation(eKeyframeTypeLinear);
        if (!useGuiCurves) {
            _imp->curveLeftBezierX->addKeyFrame(k);
        } else {
            _imp->guiCurveLeftBezierX->addKeyFrame(k);
        }
    }
    {
        KeyFrame k(time,y);
        k.setInterpolation(eKeyframeTypeLinear);
        if (!useGuiCurves) {
            _imp->curveLeftBezierY->addKeyFrame(k);
        } else {
            _imp->guiCurveLeftBezierY->addKeyFrame(k);
        }
    }
}

void
BezierCP::setRightBezierPointAtTime(bool useGuiCurves,
                                    double time,
                                    double x,
                                    double y)
{
    {
        KeyFrame k(time,x);
        k.setInterpolation(eKeyframeTypeLinear);
        if (!useGuiCurves) {
            _imp->curveRightBezierX->addKeyFrame(k);
        } else {
            _imp->guiCurveRightBezierX->addKeyFrame(k);
        }
    }
    {
        KeyFrame k(time,y);
        k.setInterpolation(eKeyframeTypeLinear);
        if (!useGuiCurves) {
            _imp->curveRightBezierY->addKeyFrame(k);
        } else {
            _imp->guiCurveRightBezierY->addKeyFrame(k);
        }
    }
}


void
BezierCP::removeAnimation(bool useGuiCurves,double currentTime)
{
    {
        QMutexLocker k(&_imp->staticPositionMutex);
        try {
            if (!useGuiCurves) {
                _imp->x = _imp->curveX->getValueAt(currentTime);
                _imp->y = _imp->curveY->getValueAt(currentTime);
                _imp->leftX = _imp->curveLeftBezierX->getValueAt(currentTime);
                _imp->leftY = _imp->curveLeftBezierY->getValueAt(currentTime);
                _imp->rightX = _imp->curveRightBezierX->getValueAt(currentTime);
                _imp->rightY = _imp->curveRightBezierY->getValueAt(currentTime);
            } else {
                _imp->guiX = _imp->guiCurveX->getValueAt(currentTime);
                _imp->guiY = _imp->guiCurveY->getValueAt(currentTime);
                _imp->guiLeftX = _imp->guiCurveLeftBezierX->getValueAt(currentTime);
                _imp->guiLeftY = _imp->guiCurveLeftBezierY->getValueAt(currentTime);
                _imp->guiRightX = _imp->guiCurveRightBezierX->getValueAt(currentTime);
                _imp->guiRightY = _imp->guiCurveRightBezierY->getValueAt(currentTime);

            }
        } catch (const std::exception & e) {
            //
        }
    }
    if (!useGuiCurves) {
        _imp->curveX->clearKeyFrames();
        _imp->curveY->clearKeyFrames();
        _imp->curveLeftBezierX->clearKeyFrames();
        _imp->curveRightBezierX->clearKeyFrames();
        _imp->curveLeftBezierY->clearKeyFrames();
        _imp->curveRightBezierY->clearKeyFrames();
    } else {
        _imp->guiCurveX->clearKeyFrames();
        _imp->guiCurveY->clearKeyFrames();
        _imp->guiCurveLeftBezierX->clearKeyFrames();
        _imp->guiCurveRightBezierX->clearKeyFrames();
        _imp->guiCurveLeftBezierY->clearKeyFrames();
        _imp->guiCurveRightBezierY->clearKeyFrames();
    }
}

void
BezierCP::removeKeyframe(bool useGuiCurves,double time)
{
    ///only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );

    ///if the keyframe count reaches 0 update the "static" values which may be fetched
    if (_imp->curveX->getKeyFramesCount() == 1) {
        QMutexLocker l(&_imp->staticPositionMutex);
        if (!useGuiCurves) {
            _imp->x = _imp->curveX->getValueAt(time);
            _imp->y = _imp->curveY->getValueAt(time);
            _imp->leftX = _imp->curveLeftBezierX->getValueAt(time);
            _imp->leftY = _imp->curveLeftBezierY->getValueAt(time);
            _imp->rightX = _imp->curveRightBezierX->getValueAt(time);
            _imp->rightY = _imp->curveRightBezierY->getValueAt(time);
        } else {
            _imp->guiX = _imp->guiCurveX->getValueAt(time);
            _imp->guiY = _imp->guiCurveY->getValueAt(time);
            _imp->guiLeftX = _imp->guiCurveLeftBezierX->getValueAt(time);
            _imp->guiLeftY = _imp->guiCurveLeftBezierY->getValueAt(time);
            _imp->guiRightX = _imp->guiCurveRightBezierX->getValueAt(time);
            _imp->guiRightY = _imp->guiCurveRightBezierY->getValueAt(time);
        }
    }

    try {
        if (!useGuiCurves) {
            _imp->curveX->removeKeyFrameWithTime(time);
            _imp->curveY->removeKeyFrameWithTime(time);
            _imp->curveLeftBezierX->removeKeyFrameWithTime(time);
            _imp->curveRightBezierX->removeKeyFrameWithTime(time);
            _imp->curveLeftBezierY->removeKeyFrameWithTime(time);
            _imp->curveRightBezierY->removeKeyFrameWithTime(time);
        } else {
            _imp->guiCurveX->removeKeyFrameWithTime(time);
            _imp->guiCurveY->removeKeyFrameWithTime(time);
            _imp->guiCurveLeftBezierX->removeKeyFrameWithTime(time);
            _imp->guiCurveRightBezierX->removeKeyFrameWithTime(time);
            _imp->guiCurveLeftBezierY->removeKeyFrameWithTime(time);
            _imp->guiCurveRightBezierY->removeKeyFrameWithTime(time);
        }
    } catch (...) {
    }
}



bool
BezierCP::hasKeyFrameAtTime(bool useGuiCurves,double time) const
{
    KeyFrame k;
    if (!useGuiCurves) {
        return _imp->curveX->getKeyFrameWithTime(time, &k);
    } else {
        return _imp->guiCurveX->getKeyFrameWithTime(time, &k);
    }
}

void
BezierCP::getKeyframeTimes(bool useGuiCurves,std::set<double>* times) const
{
    KeyFrameSet set;
    if (!useGuiCurves) {
        set = _imp->curveX->getKeyFrames_mt_safe();
    } else {
        set = _imp->guiCurveX->getKeyFrames_mt_safe();
    }

    for (KeyFrameSet::iterator it = set.begin(); it != set.end(); ++it) {
        times->insert(it->getTime());
    }
}

void
BezierCP::getKeyFrames(bool useGuiCurves,std::list<std::pair<double,KeyframeTypeEnum> >* keys) const
{
    KeyFrameSet set;
    if (!useGuiCurves) {
        set = _imp->curveX->getKeyFrames_mt_safe();
    } else {
        set = _imp->guiCurveX->getKeyFrames_mt_safe();
    }
    for (KeyFrameSet::iterator it = set.begin(); it != set.end(); ++it) {
        keys->push_back(std::make_pair(it->getTime(), it->getInterpolation()));
    }
}

int
BezierCP::getKeyFrameIndex(bool useGuiCurves,double time) const
{
    if (!useGuiCurves) {
        return _imp->curveX->keyFrameIndex(time);
    } else {
        return _imp->guiCurveX->keyFrameIndex(time);
    }
}

void
BezierCP::setKeyFrameInterpolation(bool useGuiCurves,KeyframeTypeEnum interp,int index)
{
    if (!useGuiCurves) {
        _imp->curveX->setKeyFrameInterpolation(interp, index);
        _imp->curveY->setKeyFrameInterpolation(interp, index);
        _imp->curveLeftBezierX->setKeyFrameInterpolation(interp, index);
        _imp->curveLeftBezierY->setKeyFrameInterpolation(interp, index);
        _imp->curveRightBezierX->setKeyFrameInterpolation(interp, index);
        _imp->curveRightBezierY->setKeyFrameInterpolation(interp, index);
    } else {
        _imp->guiCurveX->setKeyFrameInterpolation(interp, index);
        _imp->guiCurveY->setKeyFrameInterpolation(interp, index);
        _imp->guiCurveLeftBezierX->setKeyFrameInterpolation(interp, index);
        _imp->guiCurveLeftBezierY->setKeyFrameInterpolation(interp, index);
        _imp->guiCurveRightBezierX->setKeyFrameInterpolation(interp, index);
        _imp->guiCurveRightBezierY->setKeyFrameInterpolation(interp, index);
    }
}

double
BezierCP::getKeyframeTime(bool useGuiCurves,int index) const
{
    KeyFrame k;
    bool ok ;
    if (!useGuiCurves) {
        ok = _imp->curveX->getKeyFrameWithIndex(index, &k);
    } else {
        ok = _imp->guiCurveX->getKeyFrameWithIndex(index, &k);
    }

    if (ok) {
        return k.getTime();
    } else {
        return INT_MAX;
    }
}

int
BezierCP::getKeyframesCount(bool useGuiCurves) const
{
    return !useGuiCurves ? _imp->curveX->getKeyFramesCount() : _imp->guiCurveX->getKeyFramesCount();
}

int
BezierCP::getControlPointsCount() const
{
    boost::shared_ptr<Bezier> b = _imp->holder.lock();
    assert(b);
    return b->getControlPointsCount();
}

boost::shared_ptr<Bezier>
BezierCP::getBezier() const
{
    boost::shared_ptr<Bezier> b = _imp->holder.lock();
    assert(b);
    return b;
}

int
BezierCP::isNearbyTangent(bool useGuiCurves,
                          double time,
                          double x,
                          double y,
                          double acceptance) const
{

    Transform::Point3D p,left,right;
    p.z = left.z = right.z = 1;
    
    Transform::Matrix3x3 transform;
    getBezier()->getTransformAtTime(time, &transform);
    
    getPositionAtTime(useGuiCurves,time, &p.x, &p.y);
    getLeftBezierPointAtTime(useGuiCurves,time, &left.x, &left.y);
    getRightBezierPointAtTime(useGuiCurves,time, &right.x, &right.y);
    
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

#define TANGENTS_CUSP_LIMIT 25
namespace {
static void
cuspTangent(double x,
            double y,
            double *tx,
            double *ty,
            const std::pair<double,double>& pixelScale)
{
    ///decrease the tangents distance by 1 fourth
    ///if the tangents are equal to the control point, make them 10 pixels long
    double dx = *tx - x;
    double dy = *ty - y;
    double distSquare = dx * dx + dy * dy;

    if (distSquare <= pixelScale.first * pixelScale.second * TANGENTS_CUSP_LIMIT * TANGENTS_CUSP_LIMIT) {
        *tx = x;
        *ty = y;
    } else {
        double newDx = 0.9 * dx ;
        double newDy = 0.9 * dy;
        *tx = x + newDx;
        *ty = y + newDy;
    }
}

static void
smoothTangent(bool useGuiCurves,
              double time,
              bool left,
              const BezierCP* p,
              const Transform::Matrix3x3& transform,
              double x,
              double y,
              double *tx,
              double *ty,
              const std::pair<double,double>& pixelScale)
{
    if ( (x == *tx) && (y == *ty) ) {
        
        const std::list < boost::shared_ptr<BezierCP> > & cps = ( p->isFeatherPoint() ?
                                                                  p->getBezier()->getFeatherPoints() :
                                                                  p->getBezier()->getControlPoints() );

        if (cps.size() == 1) {
            return;
        }

        std::list < boost::shared_ptr<BezierCP> >::const_iterator prev = cps.end();
        if (prev != cps.begin()) {
            --prev;
        }
        std::list < boost::shared_ptr<BezierCP> >::const_iterator next = cps.begin();
        if (next != cps.end()) {
            ++next;
        }

        int index = 0;
        int cpCount = (int)cps.size();
        for (std::list < boost::shared_ptr<BezierCP> >::const_iterator it = cps.begin();
             it != cps.end();
             ++it) {
            if ( prev == cps.end() ) {
                prev = cps.begin();
            }
            if ( next == cps.end() ) {
                next = cps.begin();
            }
            if (it->get() == p) {
                break;
            }

            // increment for next iteration
            if (prev != cps.end()) {
                ++prev;
            }
            if (next != cps.end()) {
                ++next;
            }
            ++index;
        } // for(it)
        if ( prev == cps.end() ) {
            prev = cps.begin();
        }
        if ( next == cps.end() ) {
            next = cps.begin();
        }

        assert(index < cpCount);
        Q_UNUSED(cpCount);

        double leftDx,leftDy,rightDx,rightDy;
        Bezier::leftDerivativeAtPoint(useGuiCurves,time, *p, **prev, transform, &leftDx, &leftDy);
        Bezier::rightDerivativeAtPoint(useGuiCurves,time, *p, **next, transform, &rightDx, &rightDy);
        double norm = sqrt( (rightDx - leftDx) * (rightDx - leftDx) + (rightDy - leftDy) * (rightDy - leftDy) );
        Point delta;
        ///normalize derivatives by their norm
        if (norm != 0) {
            delta.x = ( (rightDx - leftDx) / norm ) * TANGENTS_CUSP_LIMIT * pixelScale.first;
            delta.y = ( (rightDy - leftDy) / norm ) * TANGENTS_CUSP_LIMIT * pixelScale.second;
        } else {
            ///both derivatives are the same, use the direction of the left one
            norm = sqrt( (leftDx - x) * (leftDx - x) + (leftDy - y) * (leftDy - y) );
            if (norm != 0) {
                delta.x = ( (rightDx - x) / norm ) * TANGENTS_CUSP_LIMIT * pixelScale.first;
                delta.y = ( (leftDy - y) / norm ) * TANGENTS_CUSP_LIMIT * pixelScale.second;
            } else {
                ///both derivatives and control point are equal, just use 0
                delta.x = delta.y = 0;
            }
        }

        if (!left) {
            *tx = x + delta.x;
            *ty = y + delta.y;
        } else {
            *tx = x - delta.x;
            *ty = y - delta.y;
        }
    } else {
        ///increase the tangents distance by 1 fourth
        ///if the tangents are equal to the control point, make them 10 pixels long
        double dx = *tx - x;
        double dy = *ty - y;
        double newDx,newDy;
        if ( (dx == 0) && (dy == 0) ) {
            dx = (dx < 0 ? -TANGENTS_CUSP_LIMIT : TANGENTS_CUSP_LIMIT) * pixelScale.first;
            dy = (dy < 0 ? -TANGENTS_CUSP_LIMIT : TANGENTS_CUSP_LIMIT) * pixelScale.second;
        }
        newDx = dx * 1.1;
        newDy = dy * 1.1;

        *tx = x + newDx;
        *ty = y + newDy;
    }
} // smoothTangent
}

bool
BezierCP::cuspPoint(bool useGuiCurves,
                    double time,
                    bool autoKeying,
                    bool rippleEdit,
                    const std::pair<double,double>& pixelScale)
{
    ///only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    {
        ///update the offset time
        QWriteLocker l(&_imp->masterMutex);
        if (_imp->masterTrack) {
            _imp->offsetTime = time;
        }
    }

    double x,y,leftX,leftY,rightX,rightY;
    getPositionAtTime(useGuiCurves,time, &x, &y,true);
    getLeftBezierPointAtTime(useGuiCurves,time, &leftX, &leftY,true);
    bool isOnKeyframe = getRightBezierPointAtTime(useGuiCurves,time, &rightX, &rightY,true);
    double newLeftX = leftX,newLeftY = leftY,newRightX = rightX,newRightY = rightY;
    cuspTangent(x, y, &newLeftX, &newLeftY, pixelScale);
    cuspTangent(x, y, &newRightX, &newRightY, pixelScale);

    bool keyframeSet = false;

    if (autoKeying || isOnKeyframe) {
        setLeftBezierPointAtTime(useGuiCurves,time, newLeftX, newLeftY);
        setRightBezierPointAtTime(useGuiCurves,time, newRightX, newRightY);
        if (!isOnKeyframe) {
            keyframeSet = true;
        }
    }

    if (rippleEdit) {
        std::set<double> times;
        getKeyframeTimes(useGuiCurves,&times);
        for (std::set<double>::iterator it = times.begin(); it != times.end(); ++it) {
            setLeftBezierPointAtTime(useGuiCurves,*it, newLeftX, newLeftY);
            setRightBezierPointAtTime(useGuiCurves,*it, newRightX, newRightY);
        }
    }

    return keyframeSet;
}

bool
BezierCP::smoothPoint(bool useGuiCurves,
                      double time,
                      bool autoKeying,
                      bool rippleEdit,
                      const std::pair<double,double>& pixelScale)
{
    ///only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    {
        ///update the offset time
        QWriteLocker l(&_imp->masterMutex);
        if (_imp->masterTrack) {
            _imp->offsetTime = time;
        }
    }
    Transform::Matrix3x3 transform;
    getBezier()->getTransformAtTime(time, &transform);
    
    Transform::Point3D pos,left,right;
    pos.z = left.z = right.z = 1.;
    
    getPositionAtTime(useGuiCurves,time, &pos.x, &pos.y,true);
    
    getLeftBezierPointAtTime(useGuiCurves, time, &left.x, &left.y,true);
    bool isOnKeyframe = getRightBezierPointAtTime(useGuiCurves, time, &right.x, &right.y,true);

    pos = Transform::matApply(transform, pos);
    left = Transform::matApply(transform, left);
    right = Transform::matApply(transform, right);
    
    smoothTangent(useGuiCurves,time,true,this , transform, pos.x, pos.y, &left.x, &left.y, pixelScale);
    smoothTangent(useGuiCurves,time,false,this, transform, pos.x, pos.y, &right.x, &right.y, pixelScale);

    bool keyframeSet = false;

    transform = Transform::matInverse(transform);
    
    if (autoKeying || isOnKeyframe) {
        setLeftBezierPointAtTime(useGuiCurves, time, left.x, left.y);
        setRightBezierPointAtTime(useGuiCurves, time, right.x, right.y);
        if (!isOnKeyframe) {
            keyframeSet = true;
        }
    }

    if (rippleEdit) {
        std::set<double> times;
        getKeyframeTimes(useGuiCurves, &times);
        for (std::set<double>::iterator it = times.begin(); it != times.end(); ++it) {
            setLeftBezierPointAtTime(useGuiCurves, *it, left.x, left.y);
            setRightBezierPointAtTime(useGuiCurves, *it, right.x, right.y);
        }
    }

    return keyframeSet;
}

boost::shared_ptr<Curve>
BezierCP::getXCurve() const
{
    return _imp->curveX;
}

boost::shared_ptr<Curve>
BezierCP::getYCurve() const
{
    return _imp->curveY;
}

boost::shared_ptr<Curve>
BezierCP::getLeftXCurve() const
{
    return _imp->curveLeftBezierX;
}

boost::shared_ptr<Curve>
BezierCP::getLeftYCurve() const
{
    return _imp->curveLeftBezierY;
}

boost::shared_ptr<Curve>
BezierCP::getRightXCurve() const
{
    return _imp->curveRightBezierX;
}

boost::shared_ptr<Curve>
BezierCP::getRightYCurve() const
{
    return _imp->curveRightBezierY;
}

void
BezierCP::clone(const BezierCP & other)
{
    _imp->curveX->clone(*other._imp->curveX);
    _imp->curveY->clone(*other._imp->curveY);
    _imp->curveLeftBezierX->clone(*other._imp->curveLeftBezierX);
    _imp->curveLeftBezierY->clone(*other._imp->curveLeftBezierY);
    _imp->curveRightBezierX->clone(*other._imp->curveRightBezierX);
    _imp->curveRightBezierY->clone(*other._imp->curveRightBezierY);

    _imp->guiCurveX->clone(*other._imp->curveX);
    _imp->guiCurveY->clone(*other._imp->curveY);
    _imp->guiCurveLeftBezierX->clone(*other._imp->curveLeftBezierX);
    _imp->guiCurveLeftBezierY->clone(*other._imp->curveLeftBezierY);
    _imp->guiCurveRightBezierX->clone(*other._imp->curveRightBezierX);
    _imp->guiCurveRightBezierY->clone(*other._imp->curveRightBezierY);
    
    {
        QMutexLocker l(&_imp->staticPositionMutex);
        _imp->x = other._imp->x;
        _imp->y = other._imp->y;
        _imp->leftX = other._imp->leftX;
        _imp->leftY = other._imp->leftY;
        _imp->rightX = other._imp->rightX;
        _imp->rightY = other._imp->rightY;
        
        _imp->guiX = other._imp->x;
        _imp->guiY = other._imp->y;
        _imp->guiLeftX = other._imp->leftX;
        _imp->guiLeftY = other._imp->leftY;
        _imp->guiRightX = other._imp->rightX;
        _imp->guiRightY = other._imp->rightY;
    }

    {
        QWriteLocker l(&_imp->masterMutex);
        _imp->masterTrack = other._imp->masterTrack;
        _imp->offsetTime = other._imp->offsetTime;
    }
}

bool
BezierCP::equalsAtTime(bool useGuiCurves,
                       double time,
                       const BezierCP & other) const
{
    double x,y,leftX,leftY,rightX,rightY;

    getPositionAtTime(useGuiCurves, time, &x, &y,true);
    getLeftBezierPointAtTime(useGuiCurves, time, &leftX, &leftY,true);
    getRightBezierPointAtTime(useGuiCurves, time, &rightX, &rightY,true);

    double ox,oy,oLeftX,oLeftY,oRightX,oRightY;
    other.getPositionAtTime(useGuiCurves, time, &ox, &oy,true);
    other.getLeftBezierPointAtTime(useGuiCurves, time, &oLeftX, &oLeftY,true);
    other.getRightBezierPointAtTime(useGuiCurves, time, &oRightX, &oRightY,true);

    if ( (x == ox) && (y == oy) && (leftX == oLeftX) && (leftY == oLeftY) && (rightX == oRightX) && (rightY == oRightY) ) {
        return true;
    }

    return false;
}

SequenceTime
BezierCP::getOffsetTime() const
{
    QReadLocker l(&_imp->masterMutex);

    return _imp->offsetTime;
}

void
BezierCP::cloneInternalCurvesToGuiCurves()
{
    _imp->guiCurveX->clone(*_imp->curveX);
    _imp->guiCurveY->clone(*_imp->curveY);
    
    
    _imp->guiCurveLeftBezierX->clone(*_imp->curveLeftBezierX);
    _imp->guiCurveLeftBezierY->clone(*_imp->curveLeftBezierY);
    _imp->guiCurveRightBezierX->clone(*_imp->curveRightBezierX);
    _imp->guiCurveRightBezierY->clone(*_imp->curveRightBezierY);
    
    QMutexLocker k(&_imp->staticPositionMutex);
    _imp->guiX = _imp->x;
    _imp->guiY = _imp->y;
    _imp->guiLeftX = _imp->leftX;
    _imp->guiLeftY = _imp->leftY;
    _imp->guiRightX = _imp->rightX;
    _imp->guiRightY = _imp->rightY;
}

void
BezierCP::cloneGuiCurvesToInternalCurves()
{
    _imp->curveX->clone(*_imp->guiCurveX);
    _imp->curveY->clone(*_imp->guiCurveY);
    
    
    _imp->curveLeftBezierX->clone(*_imp->guiCurveLeftBezierX);
    _imp->curveLeftBezierY->clone(*_imp->guiCurveLeftBezierY);
    _imp->curveRightBezierX->clone(*_imp->guiCurveRightBezierX);
    _imp->curveRightBezierY->clone(*_imp->guiCurveRightBezierY);
    
    QMutexLocker k(&_imp->staticPositionMutex);
    _imp->x = _imp->guiX;
    _imp->y = _imp->guiY;
    _imp->leftX = _imp->guiLeftX;
    _imp->leftY = _imp->guiLeftY;
    _imp->rightX = _imp->guiRightX;
    _imp->rightY = _imp->guiRightY;
}

void
BezierCP::slaveTo(SequenceTime offsetTime,
                  const boost::shared_ptr<KnobDouble> & track)
{
    assert( QThread::currentThread() == qApp->thread() );
    assert(!_imp->masterTrack);
    QWriteLocker l(&_imp->masterMutex);
    _imp->masterTrack = track;
    _imp->offsetTime = offsetTime;
}

void
BezierCP::unslave()
{
    assert( QThread::currentThread() == qApp->thread() );
    assert(_imp->masterTrack);
    QWriteLocker l(&_imp->masterMutex);
    _imp->masterTrack.reset();
}

boost::shared_ptr<KnobDouble>
BezierCP::isSlaved() const
{
    QReadLocker l(&_imp->masterMutex);

    return _imp->masterTrack;
}

