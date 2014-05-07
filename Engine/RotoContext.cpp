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

#include "RotoContext.h"

#include <algorithm>


#include <boost/bind.hpp>

#include "Engine/RotoContextPrivate.h"

#include "Engine/Interpolation.h"
#include "Engine/AppInstance.h"
#include "Engine/Node.h"
#include "Engine/TimeLine.h"
#include "Engine/Image.h"
#include "Engine/ImageParams.h"
#include "Engine/Hash64.h"
#include "Engine/AppManager.h"
#include "Engine/EffectInstance.h"
#include "Engine/Settings.h"
#include "Engine/RotoSerialization.h"

////////////////////////////////////ControlPoint////////////////////////////////////

BezierCP::BezierCP()
: _imp(new BezierCPPrivate(NULL))
{
    
}

BezierCP::BezierCP(const BezierCP& other)
: _imp(new BezierCPPrivate(other._imp->holder))
{
    clone(other);
}

BezierCP::BezierCP(Bezier* curve)
: _imp(new BezierCPPrivate(curve))
{
    
}

BezierCP::~BezierCP()
{
    
}

bool BezierCP::getPositionAtTime(int time,double* x,double* y) const
{
    KeyFrame k;
    if (_imp->curveX.getKeyFrameWithTime(time, &k)) {
        bool ok;
        *x = k.getValue();
        ok = _imp->curveY.getKeyFrameWithTime(time, &k);
        assert(ok);
        *y = k.getValue();
        return true;
    } else {
        try {
            *x = _imp->curveX.getValueAt(time);
            *y = _imp->curveY.getValueAt(time);
        } catch (const std::exception& e) {
            *x = _imp->x;
            *y = _imp->y;
        }
        return false;
    }
}

void BezierCP::setPositionAtTime(int time,double x,double y)
{
    ///only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    {
        KeyFrame k(time,x);
        _imp->curveX.addKeyFrame(k);
    }
    {
        KeyFrame k(time,y);
        _imp->curveY.addKeyFrame(k);
    }
    
}

void BezierCP::setStaticPosition(double x,double y)
{
    ///only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    _imp->x = x;
    _imp->y = y;
}

void BezierCP::setLeftBezierStaticPosition(double x,double y)
{
    ///only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    _imp->leftX = x;
    _imp->leftY = y;
}

void BezierCP::setRightBezierStaticPosition(double x,double y)
{
    ///only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    _imp->rightX = x;
    _imp->rightY = y;
}

bool BezierCP::getLeftBezierPointAtTime(int time,double* x,double* y) const
{
    KeyFrame k;
    if (_imp->curveLeftBezierX.getKeyFrameWithTime(time, &k)) {
        bool ok;
        *x = k.getValue();
        ok = _imp->curveLeftBezierY.getKeyFrameWithTime(time, &k);
        assert(ok);
        *y = k.getValue();
        return true;
    } else {
        try {
            *x = _imp->curveLeftBezierX.getValueAt(time);
            *y = _imp->curveLeftBezierY.getValueAt(time);
        } catch (const std::exception& e) {
            *x = _imp->leftX;
            *y = _imp->leftY;
        }
        return false;
    }
}

bool BezierCP::getRightBezierPointAtTime(int time,double *x,double *y) const
{
    
    KeyFrame k;
    if (_imp->curveRightBezierX.getKeyFrameWithTime(time, &k)) {
        bool ok;
        *x = k.getValue();
        ok = _imp->curveRightBezierY.getKeyFrameWithTime(time, &k);
        assert(ok);
        *y = k.getValue();
        return true;
    } else {
        try {
            *x = _imp->curveRightBezierX.getValueAt(time);
            *y = _imp->curveRightBezierY.getValueAt(time);
        } catch (const std::exception& e) {
            *x = _imp->rightX;
            *y = _imp->rightY;
        }
        return false;
    }
}

void BezierCP::setLeftBezierPointAtTime(int time,double x,double y)
{
    ///only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    {
        KeyFrame k(time,x);
        _imp->curveLeftBezierX.addKeyFrame(k);
    }
    {
        KeyFrame k(time,y);
        _imp->curveLeftBezierY.addKeyFrame(k);
    }
}

void BezierCP::setRightBezierPointAtTime(int time,double x,double y)
{
    ///only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    {
        KeyFrame k(time,x);
        _imp->curveRightBezierX.addKeyFrame(k);
    }
    {
        KeyFrame k(time,y);
        _imp->curveRightBezierY.addKeyFrame(k);
    }
}

void BezierCP::removeKeyframe(int time)
{
    ///only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    
    ///if the keyframe count reaches 0 update the "static" values which may be fetched
    if (_imp->curveX.getKeyFramesCount() == 1) {
        _imp->x = _imp->curveX.getValueAt(time);
        _imp->y = _imp->curveY.getValueAt(time);
        _imp->leftX = _imp->curveLeftBezierX.getValueAt(time);
        _imp->leftY = _imp->curveLeftBezierY.getValueAt(time);
        _imp->rightX = _imp->curveRightBezierX.getValueAt(time);
        _imp->rightY = _imp->curveRightBezierY.getValueAt(time);
    }
    
    _imp->curveX.removeKeyFrameWithTime(time);
    _imp->curveY.removeKeyFrameWithTime(time);
    _imp->curveLeftBezierX.removeKeyFrameWithTime(time);
    _imp->curveRightBezierX.removeKeyFrameWithTime(time);
    _imp->curveLeftBezierY.removeKeyFrameWithTime(time);
    _imp->curveRightBezierY.removeKeyFrameWithTime(time);
    
    
}


bool BezierCP::hasKeyFrameAtTime(int time) const
{
    KeyFrame k;
    return _imp->curveX.getKeyFrameWithTime(time, &k);
}

void BezierCP::getKeyframeTimes(std::set<int>* times) const
{
    KeyFrameSet set = _imp->curveX.getKeyFrames_mt_safe();
    for (KeyFrameSet::iterator it = set.begin(); it!=set.end(); ++it) {
        times->insert((int)it->getTime());
    }
}

int BezierCP::getKeyframeTime(int index) const
{
    KeyFrame k;
    bool ok = _imp->curveX.getKeyFrameWithIndex(index, &k);
    if (ok) {
        return k.getTime();
    } else {
        return INT_MAX;
    }
}

int BezierCP::getKeyframesCount() const
{
    return _imp->curveX.getKeyFramesCount();
}

Bezier* BezierCP::getCurve() const
{
    return _imp->holder;
}

int BezierCP::isNearbyTangent(int time,double x,double y,double acceptance) const
{
    
        
    double leftX,leftY,rightX,rightY;
    getLeftBezierPointAtTime(time, &leftX, &leftY);
    getRightBezierPointAtTime(time, &rightX, &rightY);
    if (leftX >= (x - acceptance) && leftX <= (x + acceptance) && leftY >= (y - acceptance) && leftY <= (y + acceptance)) {
        return 0;
    }
    if (rightX >= (x - acceptance) && rightX <= (x + acceptance) && rightY >= (y - acceptance) && rightY <= (y + acceptance)) {
        return 1;
    }
    
    return -1;
}

#define TANGENTS_CUSP_LIMIT 50
namespace {
    

static void cuspTangent(double x,double y,double *tx,double *ty)
{
    ///decrease the tangents distance by 1 fourth
    ///if the tangents are equal to the control point, make them 10 pixels long
    double dx = *tx - x;
    double dy = *ty - y;
    double distSquare = dx * dx + dy * dy;
    if (distSquare <= TANGENTS_CUSP_LIMIT * TANGENTS_CUSP_LIMIT) {
        *tx = x;
        *ty = y;
    } else {
        double newDx = 0.75 * dx;
        double newDy = 0.75 * dy;
        *tx = x + newDx;
        *ty = y + newDy;
    }
}
    
static const double pi=3.14159265358979323846264338327950288419717;
    

static void smoothTangent(int time,bool left,const BezierCP* p,double x,double y,double *tx,double *ty)
{
    
    if (x == *tx && y == *ty) {
        const std::list < boost::shared_ptr<BezierCP> >& cps =
        p->isFeatherPoint() ? p->getCurve()->getFeatherPoints() : p->getCurve()->getControlPoints();
        
        if (cps.size() == 1) {
            return;
        }
        
        std::list < boost::shared_ptr<BezierCP> >::const_iterator prev = cps.begin();
        --prev;
        std::list < boost::shared_ptr<BezierCP> >::const_iterator next = cps.begin();
        ++next;
        
        int index = 0;
        int cpCount = (int)cps.size();
        for (std::list < boost::shared_ptr<BezierCP> >::const_iterator it = cps.begin(); it!=cps.end(); ++it,++prev,++next,++index) {
            if (it->get() == p) {
                break;
            }
        }
        
        assert(index < cpCount);
        
        if (index == 0) {
            prev = cps.end();
            --prev;
        } else if (index == ((int)cps.size() -1)) {
            next = cps.begin();
        }
        
        double leftDx,leftDy,rightDx,rightDy;

        Bezier::leftDerivativeAtPoint(time, *p, **prev, &leftDx, &leftDy);
        Bezier::rightDerivativeAtPoint(time, *p, **next, &rightDx, &rightDy);
        
        double leftAlpha,rightAlpha;
        if (leftDx == 0) {
            leftAlpha = leftDy < 0 ? - pi / 2. : pi / 2.;
        } else {
            leftAlpha = std::atan2(leftDy , leftDx);
        }
        if (rightDx == 0) {
            rightAlpha = rightDy < 0 ? - pi / 2. : pi / 2.;
        } else {
            rightAlpha = std::atan2(rightDy , rightDx);
        }
        double alpha = (leftAlpha + rightAlpha) / 2.;
        if (std::abs(alpha) > pi / 2.) {
            if (alpha < 0.) {
                alpha = pi + alpha;
            } else {
                alpha = alpha - pi;
            }
        }
        
        if (!left) {
            *tx = x + std::cos(alpha + pi) * TANGENTS_CUSP_LIMIT;
            *ty = y + std::sin(alpha + pi) * TANGENTS_CUSP_LIMIT;
        } else {
            *tx = x + std::cos(alpha) * TANGENTS_CUSP_LIMIT;
            *ty = y + std::sin(alpha) * TANGENTS_CUSP_LIMIT;
        }
   
    } else {
        ///increase the tangents distance by 1 fourth
        ///if the tangents are equal to the control point, make them 10 pixels long
        double dx = *tx - x;
        double dy = *ty - y;
        double newDx,newDy;
        if (dx == 0 && dy == 0) {
            dx = dx < 0 ? -TANGENTS_CUSP_LIMIT : TANGENTS_CUSP_LIMIT;
            dy = dy < 0 ? -TANGENTS_CUSP_LIMIT : TANGENTS_CUSP_LIMIT;
        }
        newDx = 1.25 * dx;
        newDy = 1.25 * dy;
        
        *tx = x + newDx;
        *ty = y + newDy;
    }
}
}

void BezierCP::cuspPoint(int time,bool autoKeying,bool rippleEdit)
{
    ///only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    
    double x,y,leftX,leftY,rightX,rightY;
    getPositionAtTime(time, &x, &y);
    getLeftBezierPointAtTime(time, &leftX, &leftY);
    bool isOnKeyframe = getRightBezierPointAtTime(time, &rightX, &rightY);
    double newLeftX = leftX,newLeftY = leftY,newRightX = rightX,newRightY = rightY;
    cuspTangent(x, y, &newLeftX, &newLeftY);
    cuspTangent(x, y, &newRightX, &newRightY);
    
    if (autoKeying || isOnKeyframe) {
        setLeftBezierPointAtTime(time, newLeftX, newLeftY);
        setRightBezierPointAtTime(time, newRightX, newRightY);
    }
    
    if (rippleEdit) {
        std::set<int> times;
        getKeyframeTimes(&times);
        for (std::set<int>::iterator it = times.begin(); it!=times.end(); ++it) {
            setLeftBezierPointAtTime(*it, newLeftX, newLeftY);
            setRightBezierPointAtTime(*it, newRightX, newRightY);
        }
    }
}

void BezierCP::smoothPoint(int time,bool autoKeying,bool rippleEdit)
{
    ///only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    
    double x,y,leftX,leftY,rightX,rightY;
    getPositionAtTime(time, &x, &y);
    getLeftBezierPointAtTime(time, &leftX, &leftY);
    bool isOnKeyframe = getRightBezierPointAtTime(time, &rightX, &rightY);
    
    smoothTangent(time,true,this,x, y, &leftX, &leftY);
    smoothTangent(time,false,this,x, y, &rightX, &rightY);
    
    if (autoKeying || isOnKeyframe) {
        setLeftBezierPointAtTime(time, leftX, leftY);
        setRightBezierPointAtTime(time, rightX, rightY);
    }
    
    if (rippleEdit) {
        std::set<int> times;
        getKeyframeTimes(&times);
        for (std::set<int>::iterator it = times.begin(); it!=times.end(); ++it) {
            setLeftBezierPointAtTime(*it, leftX, leftY);
            setRightBezierPointAtTime(*it, rightX, rightY);
        }
    }
}

void BezierCP::clone(const BezierCP& other)
{    
    _imp->curveX.clone(other._imp->curveX);
    _imp->curveY.clone(other._imp->curveY);
    _imp->curveLeftBezierX.clone(other._imp->curveLeftBezierX);
    _imp->curveLeftBezierY.clone(other._imp->curveLeftBezierY);
    _imp->curveRightBezierX.clone(other._imp->curveRightBezierX);
    _imp->curveRightBezierY.clone(other._imp->curveRightBezierY);
    
    _imp->x = other._imp->x;
    _imp->y = other._imp->y;
    _imp->leftX = other._imp->leftX;
    _imp->leftY = other._imp->leftY;
    _imp->rightX = other._imp->rightX;
    _imp->rightY = other._imp->rightY;
}

bool BezierCP::equalsAtTime(int time,const BezierCP& other) const
{
    double x,y,leftX,leftY,rightX,rightY;
    getPositionAtTime(time, &x, &y);
    getLeftBezierPointAtTime(time, &leftX, &leftY);
    getRightBezierPointAtTime(time, &rightX, &rightY);
    
    double ox,oy,oLeftX,oLeftY,oRightX,oRightY;
    other.getPositionAtTime(time, &ox, &oy);
    other.getLeftBezierPointAtTime(time, &oLeftX, &oLeftY);
    other.getRightBezierPointAtTime(time, &oRightX, &oRightY);
    
    if (x == ox && y == oy && leftX == oLeftX && leftY == oLeftY && rightX == oRightX && rightY == oRightY) {
        return true;
    }
    return false;
}

////////////////////////////////////Bezier////////////////////////////////////

namespace  {
    
    enum SplineChangedReason {
        DERIVATIVES_CHANGED = 0,
        CONTROL_POINT_CHANGED = 1
    };
    
    typedef std::pair<double,double> Point;
    typedef std::pair<int,int> PointI;
    
    void
    lerp(Point& dest, const Point& a, const Point& b, const float t)
    {
        dest.first = a.first + (b.first - a.first) * t;
        dest.second = a.second + (b.second - a.second) * t;
    }
    
    static void
    bezier(Point& dest,const Point& p0,const Point& p1,const Point& p2,const Point& p3,double t)
    {
        Point p0p1,p1p2,p2p3,p0p1_p1p2,p1p2_p2p3;
        lerp(p0p1, p0,p1,t);
        lerp(p1p2, p1,p2,t);
        lerp(p2p3, p2,p3,t);
        lerp(p0p1_p1p2, p0p1,p1p2,t);
        lerp(p1p2_p2p3, p1p2,p2p3,t);
        lerp(dest,p0p1_p1p2,p1p2_p2p3,t);
    }
    
    
    static void
    evalBezierSegment(const BezierCP& first,const BezierCP& last,int time,unsigned int mipMapLevel,int nbPointsPerSegment,
                      std::list< Point >* points,double *xmin,double *xmax,double *ymin,double *ymax)
    {
        Point p0,p1,p2,p3;
        
        try {
            first.getPositionAtTime(time, &p0.first, &p0.second);
            first.getRightBezierPointAtTime(time, &p1.first, &p1.second);
            last.getPositionAtTime(time, &p3.first, &p3.second);
            last.getLeftBezierPointAtTime(time, &p2.first, &p2.second);
        } catch (const std::exception& e) {
            assert(false);
        }
        
        if (mipMapLevel > 0) {
            int pot = 1 << mipMapLevel;
            p0.first /= pot;
            p0.second /= pot;
            
            p1.first /= pot;
            p1.second /= pot;
            
            p2.first /= pot;
            p2.second /= pot;
            
            p3.first /= pot;
            p3.second /= pot;
        }
        
        double incr = 1. / (double)(nbPointsPerSegment - 1);
        
        Point cur;
        for (double t = 0.; t <= 1.; t += incr) {
            
            bezier(cur,p0,p1,p2,p3,t);
            points->push_back(cur);
            
            if (cur.first < *xmin) {
                *xmin = cur.first;
            }
            if (cur.first > *xmax) {
                *xmax = cur.first;
            }
            if (cur.second < *ymin) {
                *ymin = cur.second;
            }
            if (cur.second > *ymax) {
                *ymax = cur.second;
            }
        }
        
    }

    
    /**
     * @brief Determines if the point (x,y) lies on the bezier curve segment defined by first and last.
     * @returns True if the point is close (according to the acceptance) to the curve, false otherwise.
     * @param param[out] It is set to the parametric value at which the subdivision of the bezier segment
     * yields the closest point to (x,y) on the curve.
     **/
    static bool
    isPointOnBezierSegment(const BezierCP& first,const BezierCP& last,int time,
                           double x,double y,double acceptance,double *param)
    {
        Point p0,p1,p2,p3;
        first.getPositionAtTime(time, &p0.first, &p0.second);
        first.getRightBezierPointAtTime(time, &p1.first, &p1.second);
        last.getPositionAtTime(time, &p3.first, &p3.second);
        last.getLeftBezierPointAtTime(time, &p2.first, &p2.second);
        
        ///Use 100 points to make the segment
        double incr = 1. / (double)(100 - 1);
        
        ///the minimum square distance between a decasteljau point an the given (x,y) point
        ///we save a sqrt call
        
        double minDistance = INT_MAX;
        double tForMin = -1.;
        for (double t = 0.; t <= 1.; t += incr) {
            Point p;
            bezier(p,p0,p1,p2,p3,t);
            double dist = (p.first - x) * (p.first - x) + (p.second - y) * (p.second - y);
            if (dist < minDistance) {
                minDistance = dist;
                tForMin = t;
            }
        }
        
        if (minDistance <= acceptance) {
            *param = tForMin;
            return true;
        }
        return false;
    }
    
    static bool
    isPointCloseTo(int time,const BezierCP& p,double x,double y,double acceptance)
    {
        double px,py;
        p.getPositionAtTime(time, &px, &py);
        if (px >= (x - acceptance) && px <= (x + acceptance) && py >= (y - acceptance) && py <= (y + acceptance)) {
            return true;
        }
        return false;
    }

    static bool
    areSegmentDifferents(int time,const BezierCP& p0,const BezierCP& p1,const BezierCP& s0,const BezierCP& s1)
    {
        double prevX,prevY,prevXF,prevYF;
        double nextX,nextY,nextXF,nextYF;
        p0.getPositionAtTime(time, &prevX, &prevY);
        p1.getPositionAtTime(time, &nextX, &nextY);
        s0.getPositionAtTime(time, &prevXF, &prevYF);
        s1.getPositionAtTime(time, &nextXF, &nextYF);
        if (prevX != prevXF || prevY != prevYF || nextX != nextXF || nextY != nextYF) {
            return true;
        } else {
            ///check derivatives
            double prevRightX,prevRightY,nextLeftX,nextLeftY;
            double prevRightXF,prevRightYF,nextLeftXF,nextLeftYF;
            p0.getRightBezierPointAtTime(time, &prevRightX, &prevRightY);
            p1.getLeftBezierPointAtTime(time, &nextLeftX, &nextLeftY);
            s0.getRightBezierPointAtTime(time,&prevRightXF, &prevRightYF);
            s1.getLeftBezierPointAtTime(time, &nextLeftXF, &nextLeftYF);
            if (prevRightX != prevRightXF || prevRightY != prevRightYF || nextLeftX != nextLeftXF || nextLeftY != nextLeftYF) {
                return true;
            } else {
                return false;
            }
        }
    }
    
} //anonymous namespace



Bezier::Bezier(RotoContext* ctx)
: _imp(new BezierPrivate(ctx))
{
    
}

Bezier::~Bezier()
{
    
}


boost::shared_ptr<BezierCP> Bezier::addControlPoint(double x,double y)
{
    ///only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    boost::shared_ptr<BezierCP> p;
    {
        QMutexLocker l(&_imp->splineMutex);
        assert(!_imp->finished);
        
        bool autoKeying = _imp->context->isAutoKeyingEnabled();
        int keyframeTime;
        ///if the curve is empty make a new keyframe at the current timeline's time
        ///otherwise re-use the time at which the keyframe was set on the first control point
        if (_imp->points.empty()) {
            keyframeTime = _imp->context->getTimelineCurrentTime();
        } else {
            ///there must be at least 1 keyframe!
            keyframeTime = _imp->points.front()->getKeyframeTime(0);
            assert((keyframeTime != INT_MAX && autoKeying) || !autoKeying);
        }
        p.reset(new BezierCP(this));
        if (autoKeying) {
            p->setPositionAtTime(keyframeTime, x, y);
            p->setLeftBezierPointAtTime(keyframeTime, x,y);
            p->setRightBezierPointAtTime(keyframeTime, x, y);
        } else {
            p->setStaticPosition(x, y);
            p->setLeftBezierStaticPosition(x, y);
            p->setRightBezierStaticPosition(x, y);
        }
        _imp->points.insert(_imp->points.end(),p);
        
        boost::shared_ptr<BezierCP> fp(new FeatherPoint(this));
        if (autoKeying) {
            fp->setPositionAtTime(keyframeTime, x, y);
            fp->setLeftBezierPointAtTime(keyframeTime, x, y);
            fp->setRightBezierPointAtTime(keyframeTime, x, y);
        } else {
            fp->setStaticPosition(x, y);
            fp->setLeftBezierStaticPosition(x, y);
            fp->setRightBezierStaticPosition(x, y);
        }
        _imp->featherPoints.insert(_imp->featherPoints.end(),fp);
    }
    return p;
}

boost::shared_ptr<BezierCP> Bezier::addControlPointAfterIndex(int index,double t)
{
    ///only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    
    QMutexLocker l(&_imp->splineMutex);
        
    if (index >= (int)_imp->points.size()) {
        throw std::invalid_argument("Spline control point index out of range.");
    }
    
    boost::shared_ptr<BezierCP> p(new BezierCP(this));
    boost::shared_ptr<BezierCP> fp(new FeatherPoint(this));
    ///we set the new control point position to be in the exact position the curve would have at each keyframe
    std::set<int> existingKeyframes;
    _imp->getKeyframeTimes(&existingKeyframes);
    
    BezierCPs::const_iterator prev = _imp->atIndex(index);
    BezierCPs::const_iterator next = prev;
    ++next;
    if (_imp->finished && next == _imp->points.end()) {
        next = _imp->points.begin();
    }
    
    BezierCPs::const_iterator prevF = _imp->featherPoints.begin();
    std::advance(prevF, index);
    BezierCPs::const_iterator nextF = prevF;
    ++nextF;
    if (_imp->finished && nextF == _imp->featherPoints.end()) {
        nextF = _imp->featherPoints.begin();
    }
    
    
    assert(next != _imp->points.end());
    
    for (std::set<int>::iterator it = existingKeyframes.begin(); it!=existingKeyframes.end(); ++it) {
        
        Point p0,p1,p2,p3;
        (*prev)->getPositionAtTime(*it, &p0.first, &p0.second);
        (*prev)->getRightBezierPointAtTime(*it, &p1.first, &p1.second);
        (*next)->getPositionAtTime(*it, &p3.first, &p3.second);
        (*next)->getLeftBezierPointAtTime(*it, &p2.first, &p2.second);
        
        
        Point dst;
        Point p0p1,p1p2,p2p3,p0p1_p1p2,p1p2_p2p3;
        lerp(p0p1, p0,p1,t);
        lerp(p1p2, p1,p2,t);
        lerp(p2p3, p2,p3,t);
        lerp(p0p1_p1p2, p0p1,p1p2,t);
        lerp(p1p2_p2p3, p1p2,p2p3,t);
        lerp(dst,p0p1_p1p2,p1p2_p2p3,t);

        //update prev and next inner control points
        (*prev)->setRightBezierPointAtTime(*it, p0p1.first, p0p1.second);
        (*prevF)->setRightBezierPointAtTime(*it, p0p1.first, p0p1.second);
        
        (*next)->setLeftBezierPointAtTime(*it, p2p3.first, p2p3.second);
        (*nextF)->setLeftBezierPointAtTime(*it, p2p3.first, p2p3.second);
        
        p->setPositionAtTime(*it,dst.first,dst.second);
        ///The left control point of p is p0p1_p1p2 and the right control point is p1p2_p2p3
        p->setLeftBezierPointAtTime(*it, p0p1_p1p2.first,p0p1_p1p2.second);
        p->setRightBezierPointAtTime(*it, p1p2_p2p3.first, p1p2_p2p3.second);

        fp->setPositionAtTime(*it, dst.first, dst.second);
        fp->setLeftBezierPointAtTime(*it,p0p1_p1p2.first,p0p1_p1p2.second);
        fp->setRightBezierPointAtTime(*it, p1p2_p2p3.first, p1p2_p2p3.second);
    }
    
    ///if there's no keyframes
    if (existingKeyframes.empty()) {
        Point p0,p1,p2,p3;
        
        (*prev)->getPositionAtTime(0, &p0.first, &p0.second);
        (*prev)->getRightBezierPointAtTime(0, &p1.first, &p1.second);
        (*next)->getPositionAtTime(0, &p3.first, &p3.second);
        (*next)->getLeftBezierPointAtTime(0, &p2.first, &p2.second);

        
        Point dst;
        Point p0p1,p1p2,p2p3,p0p1_p1p2,p1p2_p2p3;
        lerp(p0p1, p0,p1,t);
        lerp(p1p2, p1,p2,t);
        lerp(p2p3, p2,p3,t);
        lerp(p0p1_p1p2, p0p1,p1p2,t);
        lerp(p1p2_p2p3, p1p2,p2p3,t);
        lerp(dst,p0p1_p1p2,p1p2_p2p3,t);

        //update prev and next inner control points
        (*prev)->setRightBezierStaticPosition(p0p1.first, p0p1.second);
        (*prevF)->setRightBezierStaticPosition(p0p1.first, p0p1.second);
        
        (*next)->setLeftBezierStaticPosition(p2p3.first, p2p3.second);
        (*nextF)->setLeftBezierStaticPosition(p2p3.first, p2p3.second);

        p->setStaticPosition(dst.first,dst.second);
        ///The left control point of p is p0p1_p1p2 and the right control point is p1p2_p2p3
        p->setLeftBezierStaticPosition(p0p1_p1p2.first,p0p1_p1p2.second);
        p->setRightBezierStaticPosition(p1p2_p2p3.first, p1p2_p2p3.second);
        
        fp->setStaticPosition(dst.first, dst.second);
        fp->setLeftBezierStaticPosition(p0p1_p1p2.first,p0p1_p1p2.second);
        fp->setRightBezierStaticPosition(p1p2_p2p3.first, p1p2_p2p3.second);
    }
    
    
    ////Insert the point into the container
    BezierCPs::iterator it = _imp->points.begin();
    ///it will point at the element right after index
    std::advance(it, index + 1);
    _imp->points.insert(it,p);
    
    
    ///insert the feather point
    BezierCPs::iterator itF = _imp->featherPoints.begin();
    std::advance(itF, index + 1);
    _imp->featherPoints.insert(itF, fp);

    
    ///If auto-keying is enabled, set a new keyframe
    int currentTime = _imp->context->getTimelineCurrentTime();
    if (!_imp->hasKeyframeAtTime(currentTime) && _imp->context->isAutoKeyingEnabled()) {
        l.unlock();
        setKeyframe(currentTime);
    }
    return p;
}

int Bezier::getControlPointsCount() const
{
    QMutexLocker l(&_imp->splineMutex);
    return (int)_imp->points.size();
}


int Bezier::isPointOnCurve(double x,double y,double acceptance,double *t,bool* feather) const
{
    ///only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    
    int time = _imp->context->getTimelineCurrentTime();
    
    QMutexLocker l(&_imp->splineMutex);
    
    ///special case: if the curve has only 1 control point, just check if the point
    ///is nearby that sole control point
    if (_imp->points.size() == 1) {
        const boost::shared_ptr<BezierCP>& cp = _imp->points.front();
        if (isPointCloseTo(time, *cp, x, y, acceptance)) {
            *feather = false;
            return 0;
        } else {
            ///do the same with the feather points
            const boost::shared_ptr<BezierCP>& fp = _imp->featherPoints.front();
            if (isPointCloseTo(time, *fp, x, y, acceptance)) {
                *feather = true;
                return 0;
            }
        }
        return -1;
    }
    
    ///For each segment find out if the point lies on the bezier
    int index = 0;
    
    ///acceptance square is used by isPointOnBezierSegment because we compare
    ///square distances to avoid sqrt calls
    double a2 = acceptance * acceptance;
    
    assert(_imp->featherPoints.size() == _imp->points.size());
    
    BezierCPs::const_iterator fp = _imp->featherPoints.begin();
    for (BezierCPs::const_iterator it = _imp->points.begin(); it != _imp->points.end(); ++it, ++index,++fp) {
        BezierCPs::const_iterator next = it;
        BezierCPs::const_iterator nextFp = fp;
        ++nextFp;
        ++next;
        if (next == _imp->points.end()) {
            if (!_imp->finished) {
                return -1;
            } else {
                next = _imp->points.begin();
                nextFp = _imp->featherPoints.begin();
            }
        }
        if (isPointOnBezierSegment(*(*it), *(*next), time, x, y, a2,t)) {
            *feather = false;
            return index;
        }
        if (isPointOnBezierSegment(**fp, **nextFp, time, x, y, a2, t)) {
            *feather = true;
            return index;
        }
    }
    
    ///now check the feather points segments only if they are different from the base bezier
    
    
    return -1;
}

void Bezier::setCurveFinished(bool finished)
{
    ///only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    QMutexLocker l(&_imp->splineMutex);
    _imp->finished = finished;
}

bool Bezier::isCurveFinished() const
{
    QMutexLocker l(&_imp->splineMutex);
    return _imp->finished;
}

void Bezier::removeControlPointByIndex(int index)
{
    ///only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    
    QMutexLocker l(&_imp->splineMutex);
    BezierCPs::iterator it = _imp->atIndex(index);
    _imp->points.erase(it);
    
    BezierCPs::iterator itF = _imp->featherPoints.begin();
    std::advance(itF, index);
    _imp->featherPoints.erase(itF);
}


void Bezier::movePointByIndex(int index,int time,double dx,double dy)
{
    ///only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    
    bool autoKeying = _imp->context->isAutoKeyingEnabled();
    {
        QMutexLocker l(&_imp->splineMutex);
        BezierCPs::iterator it = _imp->atIndex(index);
        double x,y,leftX,leftY,rightX,rightY;
        bool isOnKeyframe = (*it)->getPositionAtTime(time, &x, &y);
        (*it)->getLeftBezierPointAtTime(time, &leftX, &leftY);
        (*it)->getRightBezierPointAtTime(time, &rightX, &rightY);
        
        
        BezierCPs::iterator itF = _imp->featherPoints.begin();
        std::advance(itF, index);
        double xF,yF,leftXF,leftYF,rightXF,rightYF;
        (*itF)->getPositionAtTime(time, &xF, &yF);
        (*itF)->getLeftBezierPointAtTime(time, &leftXF, &leftYF);
        (*itF)->getRightBezierPointAtTime(time, &rightXF, &rightYF);
       
        bool fLinkEnabled = _imp->context->isFeatherLinkEnabled();
        bool moveFeather = (fLinkEnabled || (!fLinkEnabled && (*it)->equalsAtTime(time, **itF)));
    
        
        
        if (autoKeying || isOnKeyframe) {
            (*it)->setPositionAtTime(time, x + dx, y + dy);
            (*it)->setLeftBezierPointAtTime(time, leftX + dx, leftY + dy);
            (*it)->setRightBezierPointAtTime(time, rightX + dx, rightY + dy);
        }
        
        if (moveFeather) {
            if (autoKeying || isOnKeyframe) {
                (*itF)->setPositionAtTime(time, xF + dx, yF + dy);
                (*itF)->setLeftBezierPointAtTime(time, leftXF + dx, leftYF + dy);
                (*itF)->setRightBezierPointAtTime(time, rightXF + dx, rightYF + dy);
            }
        }
        
        if (_imp->context->isRippleEditEnabled()) {
            std::set<int> keyframes;
            _imp->getKeyframeTimes(&keyframes);
            for (std::set<int>::iterator it2 = keyframes.begin(); it2!=keyframes.end(); ++it2) {
                (*it)->setPositionAtTime(*it2, x + dx, y + dy);
                (*it)->setLeftBezierPointAtTime(*it2, leftX + dx, leftY + dy);
                (*it)->setRightBezierPointAtTime(*it2, rightX + dx, rightY + dy);
                if (moveFeather) {
                    (*itF)->setPositionAtTime(*it2, xF + dx, yF + dy);
                    (*itF)->setLeftBezierPointAtTime(*it2, leftXF + dx, leftYF + dy);
                    (*itF)->setRightBezierPointAtTime(*it2, rightXF + dx, rightYF + dy);

                }
            }
        }
    }
    if (autoKeying) {
        setKeyframe(time);
    }
}


void Bezier::moveFeatherByIndex(int index,int time,double dx,double dy)
{
    ///only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    
    bool autoKeying = _imp->context->isAutoKeyingEnabled();
    
    {
        QMutexLocker l(&_imp->splineMutex);
        
        BezierCPs::iterator itF = _imp->featherPoints.begin();
        std::advance(itF, index);
        double xF,yF,leftXF,leftYF,rightXF,rightYF;
        bool isOnkeyframe = (*itF)->getPositionAtTime(time, &xF, &yF);
        
        if (isOnkeyframe || autoKeying) {
            (*itF)->setPositionAtTime(time, xF + dx, yF + dy);
        }
        
        (*itF)->getLeftBezierPointAtTime(time, &leftXF, &leftYF);
        (*itF)->getRightBezierPointAtTime(time, &rightXF, &rightYF);
        
        if (isOnkeyframe || autoKeying) {
            (*itF)->setLeftBezierPointAtTime(time, leftXF + dx, leftYF + dy);
            (*itF)->setRightBezierPointAtTime(time, rightXF + dx, rightYF + dy);
        }
        
        if (_imp->context->isRippleEditEnabled()) {
            std::set<int> keyframes;
            _imp->getKeyframeTimes(&keyframes);
            for (std::set<int>::iterator it2 = keyframes.begin(); it2!=keyframes.end(); ++it2) {
                (*itF)->setPositionAtTime(*it2, xF + dx, yF + dy);
                (*itF)->setLeftBezierPointAtTime(*it2, leftXF + dx, leftYF + dy);
                (*itF)->setRightBezierPointAtTime(*it2, rightXF + dx, rightYF + dy);
            }
        }
    }
    if (autoKeying) {
        setKeyframe(time);
    }
}

void Bezier::moveLeftBezierPoint(int index,int time,double dx,double dy)
{
    ///only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    
    bool autoKeying = _imp->context->isAutoKeyingEnabled();
    bool featherLink = _imp->context->isFeatherLinkEnabled();
    {
    QMutexLocker l(&_imp->splineMutex);
    
        BezierCPs::iterator cp = _imp->atIndex(index);
        assert(cp != _imp->points.end());
        
        BezierCPs::iterator fp = _imp->featherPoints.begin();
        std::advance(fp, index);
        
        double x,y,xF,yF;
        (*cp)->getLeftBezierPointAtTime(time, &x, &y);
        bool isOnKeyframe = (*fp)->getLeftBezierPointAtTime(time, &xF, &yF);
        
        bool moveFeather = featherLink || (x == xF && y == yF);
        
        if (autoKeying || isOnKeyframe) {
            (*cp)->setLeftBezierPointAtTime(time,x + dx, y + dy);
            if (moveFeather) {
                (*fp)->setLeftBezierPointAtTime(time, xF + dx, yF + dy);
            }
        } else {
            ///this function is called when building a new bezier we must
            ///move the static position if there is no keyframe, otherwise the
            ///curve would never be built
            (*cp)->setLeftBezierStaticPosition(x + dx, y + dy);
            if (moveFeather) {
                (*fp)->setLeftBezierStaticPosition(xF + dx, yF + dy);
            }
        }
        
        if (_imp->context->isRippleEditEnabled()) {
            std::set<int> keyframes;
            _imp->getKeyframeTimes(&keyframes);
            for (std::set<int>::iterator it2 = keyframes.begin(); it2!=keyframes.end(); ++it2) {
                (*cp)->setLeftBezierPointAtTime(*it2, x + dx, y + dy);
                if (moveFeather) {
                    (*fp)->setLeftBezierPointAtTime(*it2, xF + dx, yF + dy);
                }
            }
        }
    }
    if (autoKeying) {
        setKeyframe(time);
    }
}

void Bezier::moveRightBezierPoint(int index,int time,double dx,double dy)
{
    ///only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    
    bool autoKeying = _imp->context->isAutoKeyingEnabled();
    bool featherLink = _imp->context->isFeatherLinkEnabled();
    {
    QMutexLocker l(&_imp->splineMutex);
    
    
        BezierCPs::iterator cp = _imp->atIndex(index);
        assert(cp != _imp->points.end());
        
        BezierCPs::iterator fp = _imp->featherPoints.begin();
        std::advance(fp, index);
        
        double x,y,xF,yF;
        (*cp)->getRightBezierPointAtTime(time, &x, &y);
        bool isOnKeyframe = (*fp)->getRightBezierPointAtTime(time, &xF, &yF);
        
        bool moveFeather = featherLink || (x == xF && y == yF);
        
        if (autoKeying || isOnKeyframe) {
            (*cp)->setRightBezierPointAtTime(time,x + dx, y + dy);
            if (moveFeather) {
                (*fp)->setRightBezierPointAtTime(time, xF + dx, yF + dy);
            }
        } else {
            ///this function is called when building a new bezier we must
            ///move the static position if there is no keyframe, otherwise the
            ///curve would never be built
            (*cp)->setRightBezierStaticPosition(x + dx, y + dy);
            if (moveFeather) {
                (*fp)->setRightBezierStaticPosition(xF + dx, yF + dy);
            }
        }
        
        if (_imp->context->isRippleEditEnabled()) {
            std::set<int> keyframes;
            _imp->getKeyframeTimes(&keyframes);
            for (std::set<int>::iterator it2 = keyframes.begin(); it2!=keyframes.end(); ++it2) {
                (*cp)->setRightBezierPointAtTime(*it2, x + dx, y + dy);
                if (moveFeather) {
                    (*fp)->setRightBezierPointAtTime(*it2, xF + dx, yF + dy);
                }
            }
        }
        
    }
    if (autoKeying) {
        setKeyframe(time);
    }
}

void Bezier::setLeftBezierPoint(int index,int time,double x,double y)
{
    ///only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    
    bool autoKeying = _imp->context->isAutoKeyingEnabled();
    {
    QMutexLocker l(&_imp->splineMutex);
    
        BezierCPs::iterator cp = _imp->atIndex(index);
        assert(cp != _imp->points.end());
        
        BezierCPs::iterator fp = _imp->featherPoints.begin();
        std::advance(fp, index);
        
        bool isOnKeyframe = _imp->hasKeyframeAtTime(time);
        
        if (autoKeying || isOnKeyframe) {
            (*cp)->setLeftBezierPointAtTime(time, x, y);
            (*fp)->setLeftBezierPointAtTime(time, x, y);
            
        } else {
            (*cp)->setLeftBezierStaticPosition(x, y);
            (*fp)->setLeftBezierStaticPosition(x, y);
        }
        
        if (_imp->context->isRippleEditEnabled()) {
            std::set<int> keyframes;
            _imp->getKeyframeTimes(&keyframes);
            for (std::set<int>::iterator it2 = keyframes.begin(); it2!=keyframes.end(); ++it2) {
                (*cp)->setLeftBezierPointAtTime(*it2, x,y);
                (*fp)->setLeftBezierPointAtTime(*it2,x,y);
            }
        }
    }
    if (autoKeying) {
        setKeyframe(time);
    }
}

void Bezier::setRightBezierPoint(int index,int time,double x,double y)
{
    ///only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    
    bool autoKeying = _imp->context->isAutoKeyingEnabled();
    {
        QMutexLocker l(&_imp->splineMutex);
        
        BezierCPs::iterator cp = _imp->atIndex(index);
        assert(cp != _imp->points.end());
        
        BezierCPs::iterator fp = _imp->featherPoints.begin();
        std::advance(fp, index);
        
        bool isOnKeyframe = _imp->hasKeyframeAtTime(time);
        
        if (autoKeying || isOnKeyframe) {
            (*cp)->setRightBezierPointAtTime(time, x, y);
            (*fp)->setRightBezierPointAtTime(time, x, y);
            
        } else {
            (*cp)->setRightBezierStaticPosition(x, y);
            (*fp)->setRightBezierStaticPosition(x, y);
        }
        
        if (_imp->context->isRippleEditEnabled()) {
            std::set<int> keyframes;
            _imp->getKeyframeTimes(&keyframes);
            for (std::set<int>::iterator it2 = keyframes.begin(); it2!=keyframes.end(); ++it2) {
                (*cp)->setRightBezierPointAtTime(*it2, x,y);
                (*fp)->setRightBezierPointAtTime(*it2,x,y);
            }
        }
    }
    if (autoKeying) {
        setKeyframe(time);
    }
    
}

void Bezier::setPointAtIndex(bool feather,int index,int time,double x,double y,double lx,double ly,double rx,double ry)
{
    ///only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    
    bool autoKeying = _imp->context->isAutoKeyingEnabled();
    bool rippleEdit = _imp->context->isRippleEditEnabled();
    
    {
        QMutexLocker l(&_imp->splineMutex);
        
        bool isOnKeyframe = _imp->hasKeyframeAtTime(time);
        
        if (index >= (int)_imp->featherPoints.size()) {
            throw std::invalid_argument("Bezier::setPointAtIndex: Index out of range.");
        }
        
        BezierCPs::iterator fp = feather ?  _imp->featherPoints.begin() : _imp->points.begin();
        std::advance(fp, index);
        
        if (autoKeying || isOnKeyframe) {
            (*fp)->setPositionAtTime(time, x, y);
            (*fp)->setLeftBezierPointAtTime(time, lx, ly);
            (*fp)->setRightBezierPointAtTime(time, rx, ry);
        }
        
        if (rippleEdit) {
            std::set<int> keyframes;
            _imp->getKeyframeTimes(&keyframes);
            for (std::set<int>::iterator it2 = keyframes.begin(); it2!=keyframes.end(); ++it2) {
                (*fp)->setPositionAtTime(*it2, x, y);
                (*fp)->setLeftBezierPointAtTime(*it2, lx, ly);
                (*fp)->setRightBezierPointAtTime(*it2, rx, ry);
                
            }
        }
    }
    if (autoKeying) {
        setKeyframe(time);
    }
}

void Bezier::setPointLeftAndRightIndex(BezierCP& p,int time,double lx,double ly,double rx,double ry)
{
    ///only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    
    bool autoKeying = _imp->context->isAutoKeyingEnabled();
    bool rippleEdit = _imp->context->isRippleEditEnabled();
    
    {
        QMutexLocker l(&_imp->splineMutex);
        
        bool isOnKeyframe = _imp->hasKeyframeAtTime(time);
        
        if (autoKeying || isOnKeyframe) {
            p.setLeftBezierPointAtTime(time, lx, ly);
            p.setRightBezierPointAtTime(time, rx, ry);
        }
        
        if (rippleEdit) {
            std::set<int> keyframes;
            _imp->getKeyframeTimes(&keyframes);
            for (std::set<int>::iterator it2 = keyframes.begin(); it2!=keyframes.end(); ++it2) {
                p.setLeftBezierPointAtTime(*it2, lx, ly);
                p.setRightBezierPointAtTime(*it2, rx, ry);
                
            }
        }
    }
    if (autoKeying) {
        setKeyframe(time);
    }

}

void Bezier::removeFeatherAtIndex(int index)
{
    ///only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    QMutexLocker l(&_imp->splineMutex);
    
    if (index >= (int)_imp->points.size()) {
        throw std::invalid_argument("Bezier::removeFeatherAtIndex: Index out of range.");
    }
    
    BezierCPs::iterator cp = _imp->atIndex(index);
    BezierCPs::iterator fp = _imp->featherPoints.begin();
    std::advance(fp, index);
    
    assert(cp != _imp->points.end() && fp != _imp->featherPoints.end());
    
    (*fp)->clone(**cp);
}


void Bezier::smoothPointAtIndex(int index,int time)
{
    ///only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    
    bool autoKeying = _imp->context->isAutoKeyingEnabled();
    bool rippleEdit = _imp->context->isRippleEditEnabled();
    
    {
        QMutexLocker l(&_imp->splineMutex);
        if (index >= (int)_imp->points.size()) {
            throw std::invalid_argument("Bezier::smoothPointAtIndex: Index out of range.");
        }
        
        BezierCPs::iterator cp = _imp->atIndex(index);
        BezierCPs::iterator fp = _imp->featherPoints.begin();
        std::advance(fp, index);
        
        assert(cp != _imp->points.end() && fp != _imp->featherPoints.end());
        (*cp)->smoothPoint(time,autoKeying,rippleEdit);
        (*fp)->smoothPoint(time,autoKeying,rippleEdit);
    }
    if (autoKeying) {
        setKeyframe(time);
    }
}


void Bezier::cuspPointAtIndex(int index,int time)
{
    ///only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    
    bool autoKeying = _imp->context->isAutoKeyingEnabled();
    bool rippleEdit = _imp->context->isRippleEditEnabled();
    
    {
        QMutexLocker l(&_imp->splineMutex);
        if (index >= (int)_imp->points.size()) {
            throw std::invalid_argument("Bezier::cuspPointAtIndex: Index out of range.");
        }
        
        BezierCPs::iterator cp = _imp->atIndex(index);
        BezierCPs::iterator fp = _imp->featherPoints.begin();
        std::advance(fp, index);
        
        assert(cp != _imp->points.end() && fp != _imp->featherPoints.end());
        (*cp)->cuspPoint(time,autoKeying,rippleEdit);
        (*fp)->cuspPoint(time,autoKeying,rippleEdit);
        
    }
    if (autoKeying) {
        setKeyframe(time);
    }
}

void Bezier::setKeyframe(int time)
{
    ///only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    
    QMutexLocker l(&_imp->splineMutex);
    
    if (_imp->hasKeyframeAtTime(time)) {
        return;
    }
    
    assert(_imp->points.size() == _imp->featherPoints.size());
    
    BezierCPs::iterator itF = _imp->featherPoints.begin();
    for (BezierCPs::iterator it = _imp->points.begin(); it != _imp->points.end(); ++it,++itF) {
        double x,y;
        double leftDerivX,rightDerivX,leftDerivY,rightDerivY;
        
        {
            (*it)->getPositionAtTime(time, &x, &y);
            (*it)->setPositionAtTime(time, x, y);
            
            (*it)->getLeftBezierPointAtTime(time, &leftDerivX, &leftDerivY);
            (*it)->getRightBezierPointAtTime(time, &rightDerivX, &rightDerivY);
            (*it)->setLeftBezierPointAtTime(time, leftDerivX, leftDerivY);
            (*it)->setRightBezierPointAtTime(time, rightDerivX, rightDerivY);
        }
        
        {
            (*itF)->getPositionAtTime(time, &x, &y);
            (*itF)->setPositionAtTime(time, x, y);
            
            (*itF)->getLeftBezierPointAtTime(time, &leftDerivX, &leftDerivY);
            (*itF)->getRightBezierPointAtTime(time, &rightDerivX, &rightDerivY);
            (*itF)->setLeftBezierPointAtTime(time, leftDerivX, leftDerivY);
            (*itF)->setRightBezierPointAtTime(time, rightDerivX, rightDerivY);


        }
    }
}


void Bezier::removeKeyframe(int time)
{
    ///only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    
    QMutexLocker l(&_imp->splineMutex);
    
    if (!_imp->hasKeyframeAtTime(time)) {
        return;
    }
    
    assert(_imp->featherPoints.size() == _imp->points.size());
    
    BezierCPs::iterator fp = _imp->featherPoints.begin();
    for (BezierCPs::iterator it = _imp->points.begin(); it != _imp->points.end(); ++it,++fp) {
        (*it)->removeKeyframe(time);
        (*fp)->removeKeyframe(time);
    }

}

void Bezier::setActivated(bool activated)
{
    ///only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    
    QMutexLocker l(&_imp->splineMutex);
    _imp->activated = activated;
}

bool Bezier::isActivated() const
{
    QMutexLocker l(&_imp->splineMutex);
    return _imp->activated;
}

int Bezier::getKeyframesCount() const
{
    QMutexLocker l(&_imp->splineMutex);
    if (_imp->points.empty()) {
        return 0;
    } else {
        return _imp->points.front()->getKeyframesCount();
    }
}



void Bezier::evaluateAtTime_DeCastelJau(int time,unsigned int mipMapLevel,
                                        int nbPointsPerSegment,std::list<std::pair<double,double> >* points) const
{
    QMutexLocker l(&_imp->splineMutex);
    _imp->boundingBox.x1 = INT_MAX;
    _imp->boundingBox.x2 = INT_MIN;
    _imp->boundingBox.y1 = INT_MAX;
    _imp->boundingBox.y2 = INT_MIN;
    for (BezierCPs::const_iterator it = _imp->points.begin(); it != _imp->points.end(); ++it) {
        BezierCPs::const_iterator next = it;
        ++next;
        if (next == _imp->points.end()) {
            if (_imp->finished) {
                next = _imp->points.begin();
            }  else {
                break;
            }
        }
        evalBezierSegment(*(*it),*(*next), time,mipMapLevel, nbPointsPerSegment, points,
                          &_imp->boundingBox.x1,&_imp->boundingBox.x2,&_imp->boundingBox.y1,&_imp->boundingBox.y2);
    }
}

void Bezier::evaluateFeatherPointsAtTime_DeCastelJau(int time,int nbPointsPerSegment,std::list<std::pair<double,double> >* points) const
{
    QMutexLocker l(&_imp->splineMutex);
    BezierCPs::const_iterator itCp = _imp->points.begin();
    for (BezierCPs::const_iterator it = _imp->featherPoints.begin(); it != _imp->featherPoints.end(); ++it,++itCp) {
        BezierCPs::const_iterator next = it;
        BezierCPs::const_iterator nextCp = itCp;
        ++nextCp;
        ++next;
        if (nextCp == _imp->points.end()) {
            if (_imp->finished) {
                nextCp = _imp->points.begin();
                next = _imp->featherPoints.begin();
            }  else {
                break;
            }
        }
        if (areSegmentDifferents(time, **itCp, **nextCp, **it, **next)) {
            evalBezierSegment(*(*it),*(*next), time,0, nbPointsPerSegment, points,
                              &_imp->boundingBox.x1,&_imp->boundingBox.x2,&_imp->boundingBox.y1,&_imp->boundingBox.y2);
        }
    }
    if (_imp->boundingBox.isNull()) {
        _imp->boundingBox.x2 = _imp->boundingBox.x1 + 1;
        _imp->boundingBox.y2 = _imp->boundingBox.y1 + 1;
    }
    _imp->isBoundingBoxValid = true;

}

const RectD& Bezier::getBoundingBox(int time) const
{
    QMutexLocker l(&_imp->splineMutex);
    if (!_imp->isBoundingBoxValid) {
        std::list<Point> pts;
        l.unlock();
        evaluateAtTime_DeCastelJau(time,0, 100,&pts);
        evaluateFeatherPointsAtTime_DeCastelJau(time, 100, &pts);
        l.relock();
        assert(_imp->isBoundingBoxValid);
    }
    return _imp->boundingBox;

}

const std::list< boost::shared_ptr<BezierCP> >& Bezier::getControlPoints() const
{
    ///only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    return _imp->points;
}

const std::list< boost::shared_ptr<BezierCP> >& Bezier::getFeatherPoints() const
{
    ///only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    return _imp->featherPoints;
}

std::pair<boost::shared_ptr<BezierCP>,boost::shared_ptr<BezierCP> >
Bezier::isNearbyControlPoint(double x,double y,double acceptance,int* index) const
{
    ///only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    int time = _imp->context->getTimelineCurrentTime();
    
    QMutexLocker l(&_imp->splineMutex);
    boost::shared_ptr<BezierCP> cp,fp;
    int i = 0;
    for (BezierCPs::const_iterator it = _imp->points.begin(); it != _imp->points.end(); ++it,++i) {
        double pX,pY;
        (*it)->getPositionAtTime(time, &pX, &pY);
        if (pX >= (x - acceptance) && pX <= (x + acceptance) && pY >= (y - acceptance) && pY <= (y + acceptance)) {
            cp =  *it;
            
            ///find the equivalent feather point
            BezierCPs::const_iterator itF = _imp->featherPoints.begin();
            std::advance(itF, i);
            fp = *itF;
            *index = i;
            return std::make_pair(cp,fp);
        }
    }
    
    if (!cp && !fp) {
        i = 0;
        for (BezierCPs::const_iterator it = _imp->featherPoints.begin(); it != _imp->featherPoints.end(); ++it,++i) {
            double pX,pY;
            (*it)->getPositionAtTime(time, &pX, &pY);
            if (pX >= (x - acceptance) && pX <= (x + acceptance) && pY >= (y - acceptance) && pY <= (y + acceptance)) {
                fp =  *it;
                
                ///find the equivalent control point
                BezierCPs::const_iterator it2 = _imp->points.begin();
                std::advance(it2, i);
                cp = *it2;
                *index = i;
                return std::make_pair(fp,cp);
            }
        }
    }
    ///empty pair
    *index = -1;
    return std::make_pair(cp,fp);
}

int Bezier::getControlPointIndex(const boost::shared_ptr<BezierCP>& cp) const
{
    ///only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    QMutexLocker l(&_imp->splineMutex);
    
    int i = 0;
    for (BezierCPs::const_iterator it = _imp->points.begin();it!=_imp->points.end();++it,++i)
    {
        if (*it == cp) {
            return i;
        }
    }
    return -1;
}

int Bezier::getFeatherPointIndex(const boost::shared_ptr<BezierCP>& fp) const
{
    ///only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    QMutexLocker l(&_imp->splineMutex);
    
    int i = 0;
    for (BezierCPs::const_iterator it = _imp->featherPoints.begin();it!=_imp->featherPoints.end();++it,++i)
    {
        if (*it == fp) {
            return i;
        }
    }
    return -1;

}

boost::shared_ptr<BezierCP> Bezier::getControlPointAtIndex(int index) const
{
    QMutexLocker l(&_imp->splineMutex);
    if (index >= (int)_imp->points.size()) {
        return boost::shared_ptr<BezierCP>();
    }
    
    BezierCPs::const_iterator it = _imp->points.begin();
    std::advance(it, index);
    return *it;
}

boost::shared_ptr<BezierCP> Bezier::getFeatherPointAtIndex(int index) const
{
    QMutexLocker l(&_imp->splineMutex);
    if (index >= (int)_imp->featherPoints.size()) {
        return boost::shared_ptr<BezierCP>();
    }
    
    BezierCPs::const_iterator it = _imp->featherPoints.begin();
    std::advance(it, index);
    return *it;
}

std::list< std::pair<boost::shared_ptr<BezierCP>,boost::shared_ptr<BezierCP> > >
Bezier::controlPointsWithinRect(double l,double r,double b,double t,double acceptance,int mode) const
{
    
    std::list< std::pair<boost::shared_ptr<BezierCP>,boost::shared_ptr<BezierCP> > > ret;
    
    ///only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    QMutexLocker locker(&_imp->splineMutex);
    
    int time = _imp->context->getTimelineCurrentTime();
    
    int i = 0;
    if (mode == 0 || mode == 1) {
        for (BezierCPs::const_iterator it = _imp->points.begin(); it!=_imp->points.end(); ++it,++i) {
            double x,y;
            (*it)->getPositionAtTime(time, &x, &y);
            if (x >= (l - acceptance) && x <= (r + acceptance) && y >= (b - acceptance) && y <= (t - acceptance)) {
                std::pair<boost::shared_ptr<BezierCP>,boost::shared_ptr<BezierCP> > p;
                p.first = *it;
                BezierCPs::const_iterator itF = _imp->featherPoints.begin();
                std::advance(itF, i);
                p.second = *itF;
                ret.push_back(p);
            }
        }
    }
    i = 0;
    if (mode == 0 || mode == 2) {
        for (BezierCPs::const_iterator it = _imp->featherPoints.begin(); it!=_imp->featherPoints.end(); ++it,++i) {
            double x,y;
            (*it)->getPositionAtTime(time, &x, &y);
            if (x >= (l - acceptance) && x <= (r + acceptance) && y >= (b - acceptance) && y <= (t - acceptance)) {
                std::pair<boost::shared_ptr<BezierCP>,boost::shared_ptr<BezierCP> > p;
                p.first = *it;
                BezierCPs::const_iterator itF = _imp->points.begin();
                std::advance(itF, i);
                p.second = *itF;
                
                ///avoid duplicates
                bool found = false;
                for (std::list< std::pair<boost::shared_ptr<BezierCP>,boost::shared_ptr<BezierCP> > >::iterator it2 = ret.begin();
                     it2 != ret.end();++it2) {
                    if (it2->first == *itF) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    ret.push_back(p);
                }
            }
        }
    }
    
    return ret;
}

boost::shared_ptr<BezierCP> Bezier::getFeatherPointForControlPoint(const boost::shared_ptr<BezierCP>& cp) const
{
    assert(!cp->isFeatherPoint());
    int index = getControlPointIndex(cp);
    assert(index != -1);
    return getFeatherPointAtIndex(index);
}

boost::shared_ptr<BezierCP> Bezier::getControlPointForFeatherPoint(const boost::shared_ptr<BezierCP>& fp) const
{
    assert(fp->isFeatherPoint());
    int index = getFeatherPointIndex(fp);
    assert(index != -1);
    return getControlPointAtIndex(index);
}

void Bezier::leftDerivativeAtPoint(int time,const BezierCP& p,const BezierCP& prev,double *dx,double *dy)
{
    ///First-off, determine if the segment is a linear/quadratic/cubic bezier segment.
    assert(!p.equalsAtTime(time, prev));
    bool p0equalsP1,p1equalsP2,p2equalsP3;
    Point p0,p1,p2,p3;
    prev.getPositionAtTime(time, &p0.first, &p0.second);
    prev.getRightBezierPointAtTime(time, &p1.first, &p1.second);
    p.getLeftBezierPointAtTime(time, &p2.first, &p2.second);
    p.getPositionAtTime(time, &p3.first, &p3.second);
    p0equalsP1 = p0.first == p1.first && p0.second == p1.second;
    p1equalsP2 = p1.first == p2.first && p1.second == p2.second;
    p2equalsP3 = p2.first == p3.first && p2.second == p3.second;
    int degree = 3;
    if (p0equalsP1) {
        --degree;
    }
    if (p1equalsP2) {
        --degree;
    }
    if (p2equalsP3) {
        --degree;
    }
    assert(degree >= 1 && degree <= 3);
    
    ///derivatives for t == 1.
    if (degree == 1) {
        *dx = p3.first - p0.first;
        *dy = p3.second - p0.second;
    } else if (degree == 2) {
        if (p0equalsP1) {
            p1 = p2;
        }
        *dx = 2. * (p3.first - p1.first);
        *dy = 2. * (p3.second - p1.second);
    } else {
        *dx = 3. * (p3.first - p2.first);
        *dy = 3. * (p3.second - p2.second);
    }
}

void Bezier::rightDerivativeAtPoint(int time,const BezierCP& p,const BezierCP& next,double *dx,double *dy)
{
    ///First-off, determine if the segment is a linear/quadratic/cubic bezier segment.
    assert(!p.equalsAtTime(time, next));
    bool p0equalsP1,p1equalsP2,p2equalsP3;
    Point p0,p1,p2,p3;
    p.getPositionAtTime(time, &p0.first, &p0.second);
    p.getRightBezierPointAtTime(time, &p1.first, &p1.second);
    next.getLeftBezierPointAtTime(time, &p2.first, &p2.second);
    next.getPositionAtTime(time, &p3.first, &p3.second);
    p0equalsP1 = p0.first == p1.first && p0.second == p1.second;
    p1equalsP2 = p1.first == p2.first && p1.second == p2.second;
    p2equalsP3 = p2.first == p3.first && p2.second == p3.second;
    int degree = 3;
    if (p0equalsP1) {
        --degree;
    }
    if (p1equalsP2) {
        --degree;
    }
    if (p2equalsP3) {
        --degree;
    }
    assert(degree >= 1 && degree <= 3);
    
    ///derivatives for t == 0.
    if (degree == 1) {
        *dx = p3.first - p0.first;
        *dy = p3.second - p0.second;
    } else if (degree == 2) {
        if (p0equalsP1) {
            p1 = p2;
        }
        *dx = 2. * (p1.first - p0.first);
        *dy = 2. * (p1.second - p0.second);
    } else {
        *dx = 3. * (p1.first - p0.first);
        *dy = 3. * (p1.second - p0.second);
    }

}

void Bezier::save(BezierSerialization* obj) const
{
    QMutexLocker l(&_imp->splineMutex);
    
    obj->_closed = _imp->finished;
    obj->_activated = _imp->activated;
    
    assert(_imp->featherPoints.size() == _imp->points.size());
    
    
    BezierCPs::const_iterator fp = _imp->featherPoints.begin();
    for (BezierCPs::const_iterator it = _imp->points.begin(); it != _imp->points.end(); ++it,++fp) {
        BezierCP c,f;
        c.clone(**it);
        f.clone(**fp);
        obj->_controlPoints.push_back(c);
        obj->_featherPoints.push_back(f);
    }
}

void Bezier::load(const BezierSerialization& obj)
{
    QMutexLocker l(&_imp->splineMutex);
    _imp->finished = obj._closed;
    _imp->activated = obj._activated;
    
    ///do not load broken serializations
    if (obj._controlPoints.size() != obj._featherPoints.size()) {
        return;
    }
    
    std::list<BezierCP>::const_iterator itF = obj._featherPoints.begin();
    for (std::list<BezierCP>::const_iterator it = obj._controlPoints.begin();it!=obj._controlPoints.end();++it,++itF)
    {
        boost::shared_ptr<BezierCP> cp(new BezierCP(this));
        cp->clone(*it);
        _imp->points.push_back(cp);
        
        boost::shared_ptr<BezierCP> fp(new BezierCP(this));
        fp->clone(*itF);
        _imp->featherPoints.push_back(fp);
    }
}

////////////////////////////////////RotoContext////////////////////////////////////


RotoContext::RotoContext(Natron::Node* node)
: _imp(new RotoContextPrivate(node))
{
}

RotoContext::~RotoContext()
{
    
}


void RotoContext::setAutoKeyingEnabled(bool enabled)
{
    ///MT-safe: only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    
    QMutexLocker l(&_imp->rotoContextMutex);
    _imp->autoKeying = enabled;
}

bool RotoContext::isAutoKeyingEnabled() const
{
    ///MT-safe: only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    
    QMutexLocker l(&_imp->rotoContextMutex);
    return _imp->autoKeying;
}


void RotoContext::setFeatherLinkEnabled(bool enabled)
{
    ///MT-safe: only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    
    QMutexLocker l(&_imp->rotoContextMutex);
    _imp->featherLink = enabled;
}

bool RotoContext::isFeatherLinkEnabled() const
{
    ///MT-safe: only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    
    QMutexLocker l(&_imp->rotoContextMutex);
    return _imp->featherLink;
}

void RotoContext::setRippleEditEnabled(bool enabled)
{
    ///MT-safe: only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    
    QMutexLocker l(&_imp->rotoContextMutex);
    _imp->rippleEdit = enabled;
}

bool RotoContext::isRippleEditEnabled() const
{
    ///MT-safe: only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    
    QMutexLocker l(&_imp->rotoContextMutex);
    return _imp->rippleEdit;
}

int RotoContext::getTimelineCurrentTime() const
{
    return _imp->node->getApp()->getTimeLine()->currentFrame();
}

boost::shared_ptr<Bezier> RotoContext::makeBezier(double x,double y)
{
    boost::shared_ptr<Bezier> curve(new Bezier(this));
    curve->addControlPoint(x, y);
    
    QMutexLocker l(&_imp->rotoContextMutex);
    _imp->splines.push_back(curve);
    return curve;
}

void RotoContext::removeBezier(const Bezier* c)
{
    ///MT-safe: only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    
    QMutexLocker l(&_imp->rotoContextMutex);
    for (std::list< boost::shared_ptr<Bezier> >::iterator found = _imp->splines.begin(); found != _imp->splines.end();++found) {
        if (found->get() == c) {
            _imp->splines.erase(found);
            return;
        }
    }
    assert(false);
}

const std::list< boost::shared_ptr<Bezier> >& RotoContext::getBeziers() const
{
    ///MT-safe: only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    return _imp->splines;
}

boost::shared_ptr<Bezier>  RotoContext::isNearbyBezier(double x,double y,double acceptance,int* index,double* t,bool* feather) const
{
    ///MT-safe: only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());

    QMutexLocker l(&_imp->rotoContextMutex);
    for (std::list< boost::shared_ptr<Bezier> >::const_iterator it = _imp->splines.begin();it!=_imp->splines.end();++it) {
        double param;
        int i = (*it)->isPointOnCurve(x, y, acceptance, &param,feather);
        if (i != -1) {
            *index = i;
            *t = param;
            return *it;
        }
    }
    
    return boost::shared_ptr<Bezier>();
}

void RotoContext::onAutoKeyingChanged(bool enabled)
{
    ///MT-safe: only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    QMutexLocker l(&_imp->rotoContextMutex);
    _imp->autoKeying = enabled;
}

void RotoContext::onFeatherLinkChanged(bool enabled)
{
    ///MT-safe: only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    QMutexLocker l(&_imp->rotoContextMutex);
    _imp->featherLink = enabled;
}

void RotoContext::onRippleEditChanged(bool enabled)
{
    ///MT-safe: only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    QMutexLocker l(&_imp->rotoContextMutex);
    _imp->rippleEdit = enabled;
}


void RotoContext::getMaskRegionOfDefinition(int time,int /*view*/,RectI* rod) const
{
    QMutexLocker l(&_imp->rotoContextMutex);
    for (std::list<boost::shared_ptr<Bezier> >::const_iterator it = _imp->splines.begin(); it!=_imp->splines.end(); ++it) {
        if ((*it)->isActivated() && (*it)->isCurveFinished()) {
            RectD splineRoD = (*it)->getBoundingBox(time);
            RectI splineRoDI;
            splineRoDI.x1 = std::floor(splineRoD.x1);
            splineRoDI.y1 = std::floor(splineRoD.y1);
            splineRoDI.x2 = std::ceil(splineRoD.x2);
            splineRoDI.y2 = std::ceil(splineRoD.y2);
            rod->merge(splineRoDI);
        }
    }
}

void RotoContext::save(RotoContextSerialization* obj) const
{
    QMutexLocker l(&_imp->rotoContextMutex);
    obj->_autoKeying = _imp->autoKeying;
    obj->_featherLink = _imp->featherLink;
    obj->_rippleEdit = _imp->rippleEdit;
    
    ///the age of the context is not serialized as the images are wiped from the cache anyway
    for (std::list<boost::shared_ptr<Bezier> >::const_iterator it = _imp->splines.begin(); it!= _imp->splines.end(); ++it) {
        BezierSerialization b;
        (*it)->save(&b);
        obj->_beziers.push_back(b);
    }
}

void RotoContext::load(const RotoContextSerialization& obj)
{
    QMutexLocker l(&_imp->rotoContextMutex);
    _imp->autoKeying = obj._autoKeying;
    _imp->featherLink = obj._featherLink;
    _imp->rippleEdit = obj._rippleEdit;
    
    for (std::list<BezierSerialization>::const_iterator it = obj._beziers.begin(); it != obj._beziers.end(); ++it) {
        boost::shared_ptr<Bezier> b(new Bezier(this));
        b->load(*it);
        _imp->splines.push_back(b);
    }
}

void RotoContext::evaluateChange()
{
    _imp->incrementRotoAge();
    _imp->node->getLiveInstance()->evaluate_public(NULL, true);
}


boost::shared_ptr<Natron::Image> RotoContext::renderMask(const RectI& roi,U64 nodeHash,SequenceTime time,
                                            int view,unsigned int mipmapLevel,bool byPassCache)
{
    

    
    
    ///compute an enhanced hash different from the one of the node in order to differentiate within the cache
    ///the output image of the roto node and the mask image.
    Hash64 hash;
    hash.append(nodeHash);
    {
        QMutexLocker l(&_imp->rotoContextMutex);
        hash.append(_imp->age);
    }
    hash.computeHash();

    Natron::ImageKey key = Natron::Image::makeKey(hash.value(), time, mipmapLevel, view);
    boost::shared_ptr<const Natron::ImageParams> params;
    boost::shared_ptr<Natron::Image> image;
    
    bool cached = Natron::getImageFromCache(key, &params, &image);
   
    if (cached) {
        if (cached && byPassCache) {
            ///If we want to by-pass the cache, we will just zero-out the bitmap of the image, so
            ///we're sure renderRoIInternal will compute the whole image again.
            ///We must use the cache facility anyway because we rely on it for caching the results
            ///of actions which is necessary to avoid recursive actions.
            image->clearBitmap();
        }
        
    } else {
        RectI rod;
        getMaskRegionOfDefinition(time, view, &rod);
        
        Natron::ImageComponents maskComps = Natron::ImageComponentAlpha;
        
        params = Natron::Image::makeParams(0, rod,mipmapLevel,FALSE,
                                                    maskComps,
                                                    -1, time,
                                                    std::map<int, std::vector<RangeD> >());
        
        cached = appPTR->getImageOrCreate(key, params, &image);
        assert(image);

        if (cached && byPassCache) {
            image->clearBitmap();
        } else if (!cached) {
            ///fill the image of 0s
            image->defaultInitialize(0,0);
        }
    }
    
    ///////////////////////////////////Render internal
    RectI pixelRod = params->getPixelRoD();
    RectI clippedRoI;
    roi.intersect(pixelRod, &clippedRoI);
 
    QMutexLocker l(&_imp->rotoContextMutex);
    int i = 0;
    for (std::list<boost::shared_ptr<Bezier> >::iterator it2 = _imp->splines.begin(); it2!=_imp->splines.end(); ++it2,++i) {
        
        ///render the bezier ONLY if the image is not cached OR if the image is cached but the bezier has changed.
        ///Also render only finished bezier
        if ((*it2)->isCurveFinished() && (*it2)->isActivated()) {
            std::list< Point > points;
            (*it2)->evaluateAtTime_DeCastelJau(time,mipmapLevel, 100, &points);
            fillPolygon_evenOdd(clippedRoI,points, image.get());
        }
    }
    


    ////////////////////////////////////
    if(_imp->node->aborted()){
        //if render was aborted, remove the frame from the cache as it contains only garbage
        appPTR->removeFromNodeCache(image);
    } else {
         image->markForRendered(clippedRoI);
    }
    return image;
}

//////////////////////////////////////////RENDERING///////////////////////////////

namespace  {
 
    /**
     *     The object is assumed to be filled in scanline order, and thus the
     *     algorithm used is an extension of Bresenham's line
     *     drawing algorithm which assumes that y is always the
     *     major axis.
     **/
    struct BresenhamData {
        
        ////These are public since they are local to the same function.
        int minorAxis;                  /// minor axis
        int d;                      /// decision variable
        int m, m1;                  /// slope and slope+1
        int incr1, incr2;           /// error increments
        
        BresenhamData()
        : minorAxis(INT_MIN) , d(0) , m(0) , m1(0) , incr1(0) , incr2(0)
        {
            
        }
        
        
        /*
         *  In scan converting polygons, we want to choose those pixels
         *  which are inside the polygon.  Thus, we add .5 to the starting
         *  x coordinate for both left and right edges.  Now we choose the
         *  first pixel which is inside the pgon for the left edge and the
         *  first pixel which is outside the pgon for the right edge.
         *  Draw the left pixel, but not the right.
         *
         *  How to add .5 to the starting x coordinate:
         *      If the edge is moving to the right, then subtract dy from the
         *  error term from the general form of the algorithm.
         *      If the edge is moving to the left, then add dy to the error term.
         *
         *  The reason for the difference between edges moving to the left
         *  and edges moving to the right is simple:  If an edge is moving
         *  to the right, then we want the algorithm to flip immediately.
         *  If it is moving to the left, then we don't want it to flip until
         *  we traverse an entire pixel.
         */
        BresenhamData(int dy,int x1,int x2)
        {
            int dx;
            ///ignore horizontal edges
            if (dy != 0.) {
                minorAxis = x1;
                dx = x2 - minorAxis;
                if (dx < 0) {
                    m = dx / dy;
                    m1 = m - 1;
                    incr1 = -2 * dx + 2 * dy * m1;
                    incr2 = -2 * dx + 2 * dy * m;
                    d = 2 * m * dy - 2 * dx - 2 * dy;
                } else {
                    m = dx / dy;
                    m1 = m + 1;
                    incr1 = 2 * dx - 2 * dy * m1;
                    incr2 = 2 * dx - 2 * dy * m;
                    d = -2 * m * dy + 2 * dx;
                }
            }
        }
        
        void bresIncr()
        {
            if (m1 > 0) {
                if (d > 0) {
                    minorAxis += m1;
                    d += incr1;
                }
                else {
                    minorAxis += m;
                    d += incr2;
                }
            } else {
                if (d >= 0) {
                    minorAxis += m1;
                    d += incr1;
                }
                else {
                    minorAxis += m;
                    d += incr2;
                }
            }
        }
    };
    
    struct EdgeTableEntry {
        int ymax;                   /// ycoord at which we exit this edge
        BresenhamData bres;              /// Bresenham info to run the edge
    } ;
    
    typedef boost::shared_ptr<EdgeTableEntry> EdgePtr;
    
    struct ScanLine {
        int scanline;               /// the scanline represented
        std::list<EdgePtr> edgelist;   /// edge list
    } ;
    
    struct EdgeTable {
        int ymax;                   /// ymax for the polygon
        int ymin;                   /// ymin for the polygon
        std::list<ScanLine> scanlines;     /// list of scanlines
        
        EdgeTable()
        : ymax(INT_MIN)
        , ymin(INT_MAX)
        {
        }
    } ;

    static void insertEdgeInET(EdgeTable* ET,const EdgePtr& ETE,int scanline)
    {
        std::list<ScanLine>::iterator itSLL = ET->scanlines.begin();
        while (itSLL != ET->scanlines.end() && itSLL->scanline < scanline)
        {
            ++itSLL;
        }
        
        ScanLine* sl;
        if (itSLL == ET->scanlines.end() || itSLL->scanline > scanline) {
            ScanLine sTmp;
            std::list<ScanLine>::iterator inserted = ET->scanlines.insert(itSLL,sTmp);
            sl = &(*inserted);
        } else {
            sl = &(*itSLL);
        }
        sl->scanline = scanline;
        
        std::list<EdgePtr>::iterator itEdge = sl->edgelist.begin();
        while (itEdge != sl->edgelist.end() && ((*itEdge)->bres.minorAxis < ETE->bres.minorAxis)) {
            ++itEdge;
        }

        sl->edgelist.insert(itEdge, ETE);
        
    }
    
    static void createETandAET(const std::list< Point >& points, EdgeTable * ET,std::vector<EdgePtr>& pETEs)
    {
        if (points.size() < 2)
            return;
        
        std::list< Point >::const_iterator next = points.begin();
        ++next;
        
        
        int i = 0;
        for (std::list< Point >::const_iterator it = points.begin(); next!=points.end(); ++it,++next,++i) {
            
            PointI bottom,top;
            bool curIsTop,curIsLeft;
            if (next->second < it->second) {
                curIsTop = true;
                bottom.second = std::floor(next->second + 0.5);
                top.second = std::floor(it->second + 0.5);
                bottom.first = std::floor(next->first + 0.5);
                top.first = std::floor(it->first + 0.5);

            } else {
                curIsTop = false;
                bottom.second = std::floor(it->second + 0.5);
                top.second = std::floor(next->second + 0.5);
                bottom.first = std::floor(it->first + 0.5);
                top.first = std::floor(next->first + 0.5);
            }
            if (next->first < it->second) {
                curIsLeft = false;
            } else {
                curIsLeft = true;
            }
            
            ///horizontal edges are discarded
            if (bottom.second != top.second) {
                pETEs[i].reset(new EdgeTableEntry);
                pETEs[i]->ymax = /*curIsTop ? std::ceil(it->second) -1 : std::ceil(next->second) - 1;*/top.second - 1;  /// -1 so we don't get last scanline
                int dy = std::ceil(std::abs(next->second - it->second));
                pETEs[i]->bres = BresenhamData(dy,curIsLeft ? std::floor(it->first) : std::ceil(it->first),
                                               curIsLeft ? std::ceil(next->first) : std::floor(next->first));// bottom.first, top.first);
                insertEdgeInET(ET, pETEs[i], curIsTop ? std::floor(next->second + 0.5) : std::floor(it->second + 0.5));
                
                ET->ymax = std::max<int>(ET->ymax, curIsTop ? std::ceil(it->second) : std::ceil(next->second));
                ET->ymin = std::min<int>(ET->ymin, curIsTop ? std::floor(next->second) : std::floor(it->second));
            }
        }
        
        
    }
    
    static bool edgeX_SortFunctor(const EdgePtr& e1, const EdgePtr& e2)
    {
        return e1->bres.minorAxis < e2->bres.minorAxis;
    }
    
#if 0
    /**
     * @brief Returns whether the segments S1 (p0,p1) and the segment S2 (p2,p3) intersect.
     * If so, then the parameter intersection is set to the intersection point.
     * This is used to remove from the polygon to fill the unused segments.
     *
     * EDIT: This is not used anymore since it is way too complicated to remove from the polygons the parts
     * that do not intersect the roi. Instead we just don't render the pixels. We keep this function here
     * in case we would need it.
     **/
    static bool segmentsIntersection(const Point& p0,const Point& p1,const Point& p2,const Point& p3,Point* intersection)
    {
        double d = (p0.first - p1.first) * (p2.second - p3.second) - (p0.second - p1.second) * (p2.first - p3.first);
        if (d == 0)
            return false;
        intersection->first = ((p2.first - p3.first) * (p0.first * p1.second - p0.second * p1.first) - (p0.first - p1.first) *
                     (p2.first * p3.second - p2.second * p3.first)) / d;
        intersection->second = ((p2.second - p3.second) * (p0.first * p1.second - p0.second * p1.first) - (p0.second - p1.second) *
                     (p2.first * p3.second - p2.second * p3.first)) / d;
        if(p2.first == p3.first) {
            if ( intersection->second < std::min(p0.second,p1.second) || intersection->second > std::max(p0.second,p1.second) )
                return false;
        }
        if (intersection->first < std::min(p0.first,p1.first) || intersection->first > std::max(p0.first,p1.first))
            return false;
        if (intersection->first < std::min(p2.first,p3.first) || intersection->first > std::max(p2.first,p3.first))
            return false;
        return true;
    }
#endif
}




void RotoContext::fillPolygon_evenOdd(const RectI& roi,const std::list< Point >& points,Natron::Image* output)
{
    ///An optimization could be made to actually clip the points of the polygon the the RoI first.
    ///That would make a lot less points to process. For now we just discard the pixels outside the roi.
    ///see http://coitweb.uncc.edu/~krs/courses/4120-5120/lectures/raster2.pdf , the algorithm of
    ///Liang-Barsky for an explanation on how to clip the polygon.
    
    ///A polygon is at least a triangle
    if (points.size() < 3) {
        return;
    }
    
    EdgeTable edgeTable;
    std::list<EdgePtr> activeEdgeTable;
    std::vector<EdgePtr> edges(points.size());
    
    createETandAET(points, &edgeTable, edges);
    
    std::list<ScanLine>::iterator scanLineIT = edgeTable.scanlines.begin();  /// Current ScanLine
    
    const RectI& dstRoD = output->getPixelRoD();
    
    for (int y = edgeTable.ymin; y <= edgeTable.ymax; ++y) {
        
        if (_imp->node->aborted()) {
            return;
        }
        
        ///load into the active edge table the scan-line edges sorted by increasing x
        if (scanLineIT != edgeTable.scanlines.end() && y == scanLineIT->scanline) {
            for (std::list<EdgePtr>::iterator entries = scanLineIT->edgelist.begin();entries != scanLineIT->edgelist.end();++entries) {
                
                std::list<EdgePtr>::iterator itAET = activeEdgeTable.begin();
                while (itAET != activeEdgeTable.end() && ((*itAET)->bres.minorAxis < (*entries)->bres.minorAxis)) {
                    ++itAET;
                }
                activeEdgeTable.insert(itAET, *entries);
            }
            ++scanLineIT;
        }

        
        float* dstPixels = output->pixelAt(dstRoD.x1, y);
        
        int comps = (int)output->getComponentsCount();
        
        ///only RGBA or Alpha supported
        assert(comps == 4 || comps == 1);
        
        ///move to x = 0
        dstPixels -= (dstRoD.x1 * comps);
        
        std::list<EdgePtr>::iterator itPrevAET = activeEdgeTable.begin();
        --itPrevAET;
        std::list<EdgePtr>::iterator itNextAET = activeEdgeTable.begin();
        ++itNextAET;
        
        std::list<EdgePtr> toDelete;
        std::list<EdgePtr>::iterator itAET = activeEdgeTable.begin();
        while (itAET != activeEdgeTable.end() && itNextAET != activeEdgeTable.end()) {
            
            int x = (*itAET)->bres.minorAxis;
            int end =  x + (*itNextAET)->bres.minorAxis - (*itAET)->bres.minorAxis;
            
            ///fill pixels
            for (int xx = x; xx < end; ++xx) {
                
                double val = 1.;
                
                if (roi.contains(xx, y)) {
                    if (comps == 4) {
                        dstPixels[xx * comps + 3] = val;
                    } else {
                        dstPixels[xx] = val;
                    }
                }
            }
            
            ///do this twice, for even and odd edges
            for (int i = 0; i < 2 ; ++i) {
                assert(itAET != activeEdgeTable.end());
                if ((*itAET)->ymax == y) {
                    //Mark to remove any edge from the active edge table for which the maximum y value is equal to the scanline.
                    toDelete.push_back(*itAET);
                    
                } else {
                    (*itAET)->bres.bresIncr();
                }
                ++itPrevAET;
                ++itAET;
                ++itNextAET;
            }
        }
        
        ///Actually remove
        for (std::list<EdgePtr>::iterator itDel = toDelete.begin(); itDel != toDelete.end(); ++itDel) {
            std::list<EdgePtr>::iterator found = std::find(activeEdgeTable.begin(),activeEdgeTable.end(),*itDel);
            assert(found != activeEdgeTable.end());
            activeEdgeTable.erase(found);
        }
        
        activeEdgeTable.sort(edgeX_SortFunctor);
    }
   }
