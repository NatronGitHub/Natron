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
#include <sstream>

#include <boost/bind.hpp>

#include "Global/MemoryInfo.h"
#include "Engine/RotoContextPrivate.h"

#include "Engine/Interpolation.h"
#include "Engine/AppInstance.h"
#include "Engine/TimeLine.h"
#include "Engine/Image.h"
#include "Engine/ImageParams.h"
#include "Engine/Hash64.h"
#include "Engine/Settings.h"
#include "Engine/Format.h"
#include "Engine/RotoSerialization.h"
#include "Engine/Transform.h"

using namespace Natron;

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

bool BezierCP::getPositionAtTime(int time,double* x,double* y,bool skipMasterOrRelative) const
{
    if (!skipMasterOrRelative) {
        Double_Knob* masterTrack ;
        BezierCP* relative;
        {
            QReadLocker l(&_imp->masterMutex);
            masterTrack = _imp->masterTrack;
            relative = _imp->relativePoint;
        }
        if (masterTrack) {
            KeyFrame k;
            if (masterTrack->getCurve(0)->getKeyFrameWithTime(time, &k)) {
                *x = k.getValue();
                (void)masterTrack->getCurve(1)->getKeyFrameWithTime(time, &k);
                *y = k.getValue();
                return true;
            } else {
                *x = masterTrack->getValueAtTime(time,0);
                *y = masterTrack->getValueAtTime(time,1);
                return false;
            }
            
        } else if (relative) {
            assert(!masterTrack);
            
            ///get the position of the relative point and the position of this point absolute
            ///then compute the distance between the 2 positions then apply it as an offset to the positino
            ///of the relative with the master track
            double xRelative,yRelative;
            _imp->relativePoint->getPositionAtTime(time, &xRelative, &yRelative,true);
            double thisX,thisY;
            bool onKey = getPositionAtTime(time, &thisX, &thisY,true);
            double xOffset = thisX - xRelative;
            double yOffset = thisY - yRelative;
            
            //get the position of the relative with track applied
            _imp->relativePoint->getPositionAtTime(time, &xRelative, &yRelative,false);
            *x = xRelative + xOffset;
            *y = yRelative + yOffset;
            return onKey;
        }
    }
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
    
    ///If linked to a track or a feather of a point that is linked to a point, don't do anything
    Double_Knob* masterTrack;
    BezierCP* relativePoint;
    {
        QReadLocker l(&_imp->masterMutex);
        masterTrack = _imp->masterTrack;
        relativePoint = _imp->relativePoint;
    }
    if (masterTrack || relativePoint)
    {
        return;
    }
    
    {
        KeyFrame k(time,x);
        k.setInterpolation(Natron::KEYFRAME_LINEAR);
        _imp->curveX.addKeyFrame(k);
    }
    {
        KeyFrame k(time,y);
        k.setInterpolation(Natron::KEYFRAME_LINEAR);
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
    ///If linked to a track or a feather of a point that is linked to a point, don't do anything
    Double_Knob* masterTrack;
    BezierCP* relativePoint;
    {
        QReadLocker l(&_imp->masterMutex);
        masterTrack = _imp->masterTrack;
        relativePoint = _imp->relativePoint;
    }
    if (masterTrack || relativePoint)
    {
        return;
    }
    _imp->leftX = x;
    _imp->leftY = y;
}

void BezierCP::setRightBezierStaticPosition(double x,double y)
{
    ///only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    ///If linked to a track or a feather of a point that is linked to a point, don't do anything
    Double_Knob* masterTrack;
    BezierCP* relativePoint;
    {
        QReadLocker l(&_imp->masterMutex);
        masterTrack = _imp->masterTrack;
        relativePoint = _imp->relativePoint;
    }
    if (masterTrack || relativePoint)
    {
        return;
    }
    _imp->rightX = x;
    _imp->rightY = y;
}

bool BezierCP::getLeftBezierPointAtTime(int time,double* x,double* y) const
{
    
    Double_Knob* masterTrack;
    BezierCP* relativePoint;
    {
        QReadLocker l(&_imp->masterMutex);
        masterTrack = _imp->masterTrack;
        relativePoint = _imp->relativePoint;
    }
    KeyFrame k;
    bool ret;
    if (_imp->curveLeftBezierX.getKeyFrameWithTime(time, &k)) {
        bool ok;
        *x = k.getValue();
        ok = _imp->curveLeftBezierY.getKeyFrameWithTime(time, &k);
        assert(ok);
        *y = k.getValue();
        ret =  true;
    } else {
        try {
            *x = _imp->curveLeftBezierX.getValueAt(time);
            *y = _imp->curveLeftBezierY.getValueAt(time);
        } catch (const std::exception& e) {
            *x = _imp->leftX;
            *y = _imp->leftY;
        }
        ret =  false;
    }
    
    
    if (masterTrack) {
        double xCenter,yCenter;
        ret = getPositionAtTime(time, &xCenter, &yCenter,true);
        
        ///make it an offset relative to the center
        *x -= xCenter;
        *y -= yCenter;
        
        ///apply the offset to the track center
        *x = masterTrack->getValueAtTime(time,0) + *x;
        *y = masterTrack->getValueAtTime(time,1) + *y;
    } else if (relativePoint) {
        assert(!masterTrack);
        double xCenter,yCenter;
        getPositionAtTime(time, &xCenter, &yCenter,true);
        *x -= xCenter;
        *y -= yCenter;
        
        double xRelative,yRelative;
        ret = getPositionAtTime(time, &xRelative, &yRelative);
        *x += xRelative;
        *y += yRelative;
    }
    
    return ret;
}

bool BezierCP::getRightBezierPointAtTime(int time,double *x,double *y) const
{
    Double_Knob* masterTrack;
    BezierCP* relativePoint;
    {
        QReadLocker l(&_imp->masterMutex);
        masterTrack = _imp->masterTrack;
        relativePoint = _imp->relativePoint;
    }
    KeyFrame k;
    bool ret;
    if (_imp->curveRightBezierX.getKeyFrameWithTime(time, &k)) {
        bool ok;
        *x = k.getValue();
        ok = _imp->curveRightBezierY.getKeyFrameWithTime(time, &k);
        assert(ok);
        *y = k.getValue();
        ret = true;
    } else {
        try {
            *x = _imp->curveRightBezierX.getValueAt(time);
            *y = _imp->curveRightBezierY.getValueAt(time);
        } catch (const std::exception& e) {
            *x = _imp->rightX;
            *y = _imp->rightY;
        }
        ret =  false;
    }
    
    
    if (masterTrack) {
        double xCenter,yCenter;
        ret = getPositionAtTime(time, &xCenter, &yCenter,true);
        
        ///make it an offset relative to the center
        *x -= xCenter;
        *y -= yCenter;
        
        ///apply the offset to the track center
        *x = _imp->masterTrack->getValueAtTime(time,0) + *x;
        *y = _imp->masterTrack->getValueAtTime(time,1) + *y;
    } else if (relativePoint) {
        assert(!masterTrack);
        double xCenter,yCenter;
        getPositionAtTime(time, &xCenter, &yCenter,true);
        *x -= xCenter;
        *y -= yCenter;
        
        double xRelative,yRelative;
        ret = getPositionAtTime(time, &xRelative, &yRelative);
        *x += xRelative;
        *y += yRelative;
    }
    
    return ret;
}

void BezierCP::setLeftBezierPointAtTime(int time,double x,double y)
{
    ///only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    ///If linked to a track or a feather of a point that is linked to a point, don't do anything
    Double_Knob* masterTrack;
    BezierCP* relativePoint;
    {
        QReadLocker l(&_imp->masterMutex);
        masterTrack = _imp->masterTrack;
        relativePoint = _imp->relativePoint;
    }
    if (masterTrack || relativePoint)
    {
        return;
    }
    {
        KeyFrame k(time,x);
        k.setInterpolation(Natron::KEYFRAME_LINEAR);
        _imp->curveLeftBezierX.addKeyFrame(k);
    }
    {
        KeyFrame k(time,y);
        k.setInterpolation(Natron::KEYFRAME_LINEAR);
        _imp->curveLeftBezierY.addKeyFrame(k);
    }
}

void BezierCP::setRightBezierPointAtTime(int time,double x,double y)
{
    ///only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    ///If linked to a track or a feather of a point that is linked to a point, don't do anything
    Double_Knob* masterTrack;
    BezierCP* relativePoint;
    {
        QReadLocker l(&_imp->masterMutex);
        masterTrack = _imp->masterTrack;
        relativePoint = _imp->relativePoint;
    }
    if (masterTrack || relativePoint)
    {
        return;
    }
    {
        KeyFrame k(time,x);
        k.setInterpolation(Natron::KEYFRAME_LINEAR);
        _imp->curveRightBezierX.addKeyFrame(k);
    }
    {
        KeyFrame k(time,y);
        k.setInterpolation(Natron::KEYFRAME_LINEAR);
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
    
        
    double xp,yp,leftX,leftY,rightX,rightY;
    getPositionAtTime(time, &xp, &yp);
    getLeftBezierPointAtTime(time, &leftX, &leftY);
    getRightBezierPointAtTime(time, &rightX, &rightY);
    
    if (xp != leftX || yp != leftY) {
        if (leftX >= (x - acceptance) && leftX <= (x + acceptance) && leftY >= (y - acceptance) && leftY <= (y + acceptance)) {
            return 0;
        }
    }
    if (xp != rightX || yp != rightY) {
        if (rightX >= (x - acceptance) && rightX <= (x + acceptance) && rightY >= (y - acceptance) && rightY <= (y + acceptance)) {
            return 1;
        }
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
    
static void smoothTangent(int time,bool left,const BezierCP* p,double x,double y,double *tx,double *ty)
{
    
    if (x == *tx && y == *ty) {
        const std::list < boost::shared_ptr<BezierCP> >& cps =
        p->isFeatherPoint() ? p->getCurve()->getFeatherPoints() : p->getCurve()->getControlPoints();
        
        if (cps.size() == 1) {
            return;
        }
        
        std::list < boost::shared_ptr<BezierCP> >::const_iterator prev = cps.end();
        --prev;
        std::list < boost::shared_ptr<BezierCP> >::const_iterator next = cps.begin();
        ++next;
        
        int index = 0;
        int cpCount = (int)cps.size();
        for (std::list < boost::shared_ptr<BezierCP> >::const_iterator it = cps.begin(); it!=cps.end(); ++it,++prev,++next,++index) {
            if (prev == cps.end()) {
                prev = cps.begin();
            }
            if (next == cps.end()) {
                next = cps.begin();
            }
            if (it->get() == p) {
                break;
            }
        }
        
        assert(index < cpCount);
        
        double leftDx,leftDy,rightDx,rightDy;

        Bezier::leftDerivativeAtPoint(time, *p, **prev, &leftDx, &leftDy);
        Bezier::rightDerivativeAtPoint(time, *p, **next, &rightDx, &rightDy);
        
        double norm = sqrt((rightDx - leftDx) * (rightDx - leftDx) + (rightDy - leftDy) * (rightDy - leftDy));
        
        Point delta;
        ///normalize derivatives by their norm
        if (norm != 0) {
            delta.x = ((rightDx - leftDx) / norm) * TANGENTS_CUSP_LIMIT;
            delta.y = ((rightDy - leftDy) / norm) * TANGENTS_CUSP_LIMIT;
        } else {
            ///both derivatives are the same, use the direction of the left one
            norm = sqrt((leftDx - x) * (leftDx - x) + (leftDy - y) * (leftDy - y));
            if (norm != 0) {
                delta.x = ((rightDx - x) / norm) * TANGENTS_CUSP_LIMIT;
                delta.y = ((leftDy - y) / norm) * TANGENTS_CUSP_LIMIT;
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

bool BezierCP::cuspPoint(int time,bool autoKeying,bool rippleEdit)
{
    ///only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    ///If linked to a track or a feather of a point that is linked to a point, don't do anything
    Double_Knob* masterTrack;
    BezierCP* relativePoint;
    {
        QReadLocker l(&_imp->masterMutex);
        masterTrack = _imp->masterTrack;
        relativePoint = _imp->relativePoint;
    }
    if (masterTrack || relativePoint)
    {
        return false;
    }
    
    double x,y,leftX,leftY,rightX,rightY;
    getPositionAtTime(time, &x, &y);
    getLeftBezierPointAtTime(time, &leftX, &leftY);
    bool isOnKeyframe = getRightBezierPointAtTime(time, &rightX, &rightY);
    double newLeftX = leftX,newLeftY = leftY,newRightX = rightX,newRightY = rightY;
    cuspTangent(x, y, &newLeftX, &newLeftY);
    cuspTangent(x, y, &newRightX, &newRightY);
    
    bool keyframeSet = false;
    
    if (autoKeying || isOnKeyframe) {
        setLeftBezierPointAtTime(time, newLeftX, newLeftY);
        setRightBezierPointAtTime(time, newRightX, newRightY);
        if (!isOnKeyframe) {
            keyframeSet = true;
        }
    }
    
    if (rippleEdit) {
        std::set<int> times;
        getKeyframeTimes(&times);
        for (std::set<int>::iterator it = times.begin(); it!=times.end(); ++it) {
            setLeftBezierPointAtTime(*it, newLeftX, newLeftY);
            setRightBezierPointAtTime(*it, newRightX, newRightY);
        }
    }
    return keyframeSet;
}

bool BezierCP::smoothPoint(int time,bool autoKeying,bool rippleEdit)
{
    ///only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    ///If linked to a track or a feather of a point that is linked to a point, don't do anything
    Double_Knob* masterTrack;
    BezierCP* relativePoint;
    {
        QReadLocker l(&_imp->masterMutex);
        masterTrack = _imp->masterTrack;
        relativePoint = _imp->relativePoint;
    }
    if (masterTrack || relativePoint)
    {
        return false;
    }
    
    double x,y,leftX,leftY,rightX,rightY;
    getPositionAtTime(time, &x, &y);
    getLeftBezierPointAtTime(time, &leftX, &leftY);
    bool isOnKeyframe = getRightBezierPointAtTime(time, &rightX, &rightY);
    
    smoothTangent(time,true,this,x, y, &leftX, &leftY);
    smoothTangent(time,false,this,x, y, &rightX, &rightY);
    
    bool keyframeSet = false;
    
    if (autoKeying || isOnKeyframe) {
        setLeftBezierPointAtTime(time, leftX, leftY);
        setRightBezierPointAtTime(time, rightX, rightY);
        if (!isOnKeyframe) {
            keyframeSet = true;
        }
    }
    
    if (rippleEdit) {
        std::set<int> times;
        getKeyframeTimes(&times);
        for (std::set<int>::iterator it = times.begin(); it!=times.end(); ++it) {
            setLeftBezierPointAtTime(*it, leftX, leftY);
            setRightBezierPointAtTime(*it, rightX, rightY);
        }
    }
    return keyframeSet;
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
    
    _imp->masterTrack = other._imp->masterTrack;
    _imp->relativePoint = other._imp->relativePoint;
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

void BezierCP::slaveTo(Double_Knob* track)
{
    assert(QThread::currentThread() == qApp->thread());
    assert(!_imp->masterTrack);
    QWriteLocker l(&_imp->masterMutex);
    assert(!_imp->relativePoint);
    _imp->masterTrack = track;
    
}

void BezierCP::unslave()
{
    assert(QThread::currentThread() == qApp->thread());
    assert(_imp->masterTrack);
    QWriteLocker l(&_imp->masterMutex);
    _imp->masterTrack = NULL;
}


Double_Knob* BezierCP::isSlaved() const
{
    QReadLocker l(&_imp->masterMutex);
    return _imp->masterTrack;
}

void BezierCP::setRelativeTo(BezierCP* other)
{
    assert(QThread::currentThread() == qApp->thread());
    QWriteLocker l(&_imp->masterMutex);
    assert(!_imp->masterTrack);
    _imp->relativePoint = other;
}

BezierCP* BezierCP::hasRelative() const
{
    QReadLocker l(&_imp->masterMutex);
    return _imp->relativePoint;
}

void BezierCP::removeRelative()
{
    assert(QThread::currentThread() == qApp->thread());
    QWriteLocker l(&_imp->masterMutex);
    _imp->relativePoint = NULL;
}

////////////////////////////////////RotoItem////////////////////////////////////
namespace {
    class RotoMetaTypesRegistration
    {
    public:
        inline RotoMetaTypesRegistration()
        {
            qRegisterMetaType<RotoItem*>("RotoItem");
        }
    };
}

static RotoMetaTypesRegistration registration;

RotoItem::RotoItem(RotoContext* context,const std::string& name,RotoLayer* parent)
: itemMutex()
, _imp(new RotoItemPrivate(context,name,parent))
{
}

RotoItem::~RotoItem()
{
    
}

void RotoItem::clone(const RotoItem& other)
{
    QMutexLocker l(&itemMutex);
    _imp->parentLayer = other._imp->parentLayer;
    _imp->name = other._imp->name;
    _imp->globallyActivated = other._imp->globallyActivated;
    _imp->locked = other._imp->locked;
}

void RotoItem::setParentLayer(RotoLayer* layer)
{
    ///called on the main-thread only
    assert(QThread::currentThread() == qApp->thread());
    
    QMutexLocker l(&itemMutex);
    _imp->parentLayer = layer;
}

RotoLayer* RotoItem::getParentLayer() const
{
    QMutexLocker l(&itemMutex);
    return _imp->parentLayer;
}

void RotoItem::setGloballyActivated_recursive(bool a)
{
    {
        QMutexLocker l(&itemMutex);
        _imp->globallyActivated = a;
        RotoLayer* layer = dynamic_cast<RotoLayer*>(this);
        if (layer) {
            const RotoItems& children = layer->getItems();
            for (RotoItems::const_iterator it = children.begin(); it!= children.end(); ++it) {
                (*it)->setGloballyActivated_recursive(a);
            }
        }
    }
}

void RotoItem::setGloballyActivated(bool a,bool setChildren)
{
    ///called on the main-thread only
    assert(QThread::currentThread() == qApp->thread());
    if (setChildren) {
        setGloballyActivated_recursive(a);
    } else {
        QMutexLocker l(&itemMutex);
        _imp->globallyActivated = a;
    }
    _imp->context->evaluateChange();
    
}

bool RotoItem::isGloballyActivated() const
{
    QMutexLocker l(&itemMutex);
    return _imp->globallyActivated;
}

static void isDeactivated_imp(RotoLayer* item,bool* ret)
{
    if (!item->isGloballyActivated()) {
        *ret = true;
    } else {
        RotoLayer* parent = item->getParentLayer();
        if (parent) {
            isDeactivated_imp(parent, ret);
        }
    }
}

void RotoItem::isDeactivatedRecursive(bool* ret) const
{
    RotoLayer* parent = 0;
    {
        QMutexLocker l(&itemMutex);
        if (!_imp->globallyActivated) {
            *ret = true;
            return;
        }
        parent = _imp->parentLayer;
    }
    if (parent) {
        isDeactivated_imp(parent, ret);
    }
}

void RotoItem::setLocked_recursive(bool locked)
{
    {
        {
            QMutexLocker m(&itemMutex);
            _imp->locked = locked;
        }
        getContext()->onItemLockedChanged(this);
        RotoLayer* layer = dynamic_cast<RotoLayer*>(this);
        if (layer) {
            const RotoItems& children = layer->getItems();
            for (RotoItems::const_iterator it = children.begin(); it!= children.end(); ++it) {
                (*it)->setLocked_recursive(locked);
            }
        }
    }
}

void RotoItem::setLocked(bool l,bool lockChildren){
    ///called on the main-thread only
    assert(QThread::currentThread() == qApp->thread());
    if (!lockChildren) {
        {
            QMutexLocker m(&itemMutex);
            _imp->locked = l;
        }
        getContext()->onItemLockedChanged(this);
    } else {
        setLocked_recursive(l);
    }
}

bool RotoItem::getLocked() const
{
    QMutexLocker l(&itemMutex);
    return _imp->locked;
}

static void isLocked_imp(RotoLayer* item,bool* ret)
{
    if (item->getLocked()) {
        *ret =  true;
    } else {
        RotoLayer* parent = item->getParentLayer();
        if (parent) {
            isLocked_imp(parent, ret);
        }
    }
}

bool RotoItem::isLockedRecursive() const
{
    RotoLayer* parent = 0;
    {
        QMutexLocker l(&itemMutex);
        if (_imp->locked) {
            return true;
        }
        parent = _imp->parentLayer;
    }
    if (parent) {
        bool ret = false;
        isLocked_imp(parent, &ret);
        return ret;
    } else {
        return false;
    }
}

int RotoItem::getHierarchyLevel() const
{
    int ret = 0;
    
    RotoLayer* parent;
    
    {
        QMutexLocker l(&itemMutex);
        parent = _imp->parentLayer;
    }
    while (parent) {
        parent = parent->getParentLayer();
        ++ret;
    }
    return ret;
}

RotoContext* RotoItem::getContext() const
{
    return _imp->context;
}

void RotoItem::setName(const std::string& name)
{
    ///called on the main-thread only
    assert(QThread::currentThread() == qApp->thread());
    
    QMutexLocker l(&itemMutex);
    _imp->name = name;
}

std::string RotoItem::getName_mt_safe() const
{
    QMutexLocker l(&itemMutex);
    return _imp->name;
}

void RotoItem::save(RotoItemSerialization *obj) const
{
    
    RotoLayer* parent = 0;
    {
        QMutexLocker l(&itemMutex);
        obj->activated = _imp->globallyActivated;
        obj->name = _imp->name;
        obj->locked = _imp->locked;
        parent = _imp->parentLayer;
    }
    if (parent) {
        obj->parentLayerName = parent->getName_mt_safe();
    }
}

void RotoItem::load(const RotoItemSerialization &obj)
{
    {
        QMutexLocker l(&itemMutex);
        _imp->globallyActivated = obj.activated;
        _imp->locked = obj.locked;
        _imp->name = obj.name;
    }
    boost::shared_ptr<RotoLayer> parent = getContext()->getLayerByName(obj.parentLayerName);
    
    {
        QMutexLocker l(&itemMutex);
        _imp->parentLayer = parent.get();
    }
    
}

std::string RotoItem::getRotoNodeName() const
{
    return getContext()->getRotoNodeName();
}

////////////////////////////////////RotoDrawableItem////////////////////////////////////

RotoDrawableItem::RotoDrawableItem(RotoContext* context,const std::string& name,RotoLayer* parent)
: RotoItem(context,name,parent)
, _imp(new RotoDrawableItemPrivate())
{
#ifdef NATRON_ROTO_INVERTIBLE
    QObject::connect(_imp->inverted->getSignalSlotHandler().get(), SIGNAL(valueChanged(int,int)), this, SIGNAL(invertedStateChanged()));
#endif
    QObject::connect(this, SIGNAL(overlayColorChanged()), context, SIGNAL(refreshViewerOverlays()));
    QObject::connect(_imp->color->getSignalSlotHandler().get(), SIGNAL(valueChanged(int,int)), this, SIGNAL(shapeColorChanged()));
    QObject::connect(_imp->compOperator->getSignalSlotHandler().get(), SIGNAL(valueChanged(int,int)), this,
                     SIGNAL(compositingOperatorChanged()));
}

RotoDrawableItem::~RotoDrawableItem()
{
    
}

void RotoDrawableItem::clone(const RotoDrawableItem& other)
{
    {
        QMutexLocker l(&itemMutex);
        _imp->activated->clone(other._imp->activated.get());
        _imp->feather->clone(other._imp->feather.get());
        _imp->featherFallOff->clone(other._imp->featherFallOff.get());
        _imp->opacity->clone(other._imp->opacity.get());
#ifdef NATRON_ROTO_INVERTIBLE
        _imp->inverted->clone(other._imp->inverted.get());
#endif
        memcpy(_imp->overlayColor, other._imp->overlayColor, sizeof(double)*4);
    }
    RotoItem::clone(other);
}

static void serializeRotoKnob(const boost::shared_ptr<KnobI>& knob,KnobSerialization* serialization)
{
    std::pair<int, boost::shared_ptr<KnobI> > master = knob->getMaster(0);
    bool wasSlaved = false;
    if (master.second) {
        wasSlaved = true;
        knob->unSlave(0,false);
    }
    
    serialization->initialize(knob);
    
    if (wasSlaved) {
        knob->slaveTo(0, master.second, master.first);
    }
}

void RotoDrawableItem::save(RotoItemSerialization *obj) const
{
    RotoDrawableItemSerialization* s = dynamic_cast<RotoDrawableItemSerialization*>(obj);
    assert(s);
    
    {
        QMutexLocker l(&itemMutex);
        serializeRotoKnob(_imp->activated, &s->_activated);
        serializeRotoKnob(_imp->feather, &s->_feather);
        serializeRotoKnob(_imp->opacity, &s->_opacity);
        serializeRotoKnob(_imp->featherFallOff, &s->_featherFallOff);
#ifdef NATRON_ROTO_INVERTIBLE
        serializeRotoKnob(_imp->inverted, &s->_inverted);
#endif
        serializeRotoKnob(_imp->color, &s->_color);
        serializeRotoKnob(_imp->compOperator, &s->_compOp);
        memcpy(s->_overlayColor, _imp->overlayColor, sizeof(double) * 4);
        
        
    }
    RotoItem::save(obj);
}

void RotoDrawableItem::load(const RotoItemSerialization &obj)
{
    const RotoDrawableItemSerialization& s = dynamic_cast<const RotoDrawableItemSerialization&>(obj);
    
    
    {
        _imp->activated->clone(s._activated.getKnob().get());
        _imp->opacity->clone(s._opacity.getKnob().get());
        _imp->feather->clone(s._feather.getKnob().get());
        _imp->featherFallOff->clone(s._featherFallOff.getKnob().get());
#ifdef NATRON_ROTO_INVERTIBLE
        _imp->inverted->clone(s._inverted.getKnob().get());
#endif
        if (s._hasColorAndCompOp) {
            _imp->color->clone(s._color.getKnob().get());
            _imp->compOperator->clone(s._compOp.getKnob().get());
        }
        QMutexLocker l(&itemMutex);
        memcpy(_imp->overlayColor, s._overlayColor, sizeof(double) * 4);
    }
    RotoItem::load(obj);
}


bool RotoDrawableItem::isActivated(int time) const
{
    bool deactivated = false;
    isDeactivatedRecursive(&deactivated);
    if (deactivated) {
        return false;
    } else {
        return _imp->activated->getValueAtTime(time);
    }
}

double RotoDrawableItem::getOpacity(int time) const
{
    ///MT-safe thanks to Knob
    return _imp->opacity->getValueAtTime(time);
}


double RotoDrawableItem::getFeatherDistance(int time) const
{
    ///MT-safe thanks to Knob
    return _imp->feather->getValueAtTime(time);
}

double RotoDrawableItem::getFeatherFallOff(int time) const
{
    ///MT-safe thanks to Knob
    return _imp->featherFallOff->getValueAtTime(time);
}

#ifdef NATRON_ROTO_INVERTIBLE
bool RotoDrawableItem::getInverted(int time) const
{
    ///MT-safe thanks to Knob
    return _imp->inverted->getValueAtTime(time);
}
#endif

void RotoDrawableItem::getColor(int time,double* color) const
{
    color[0] = _imp->color->getValueAtTime(time,0);
    color[1] = _imp->color->getValueAtTime(time,1);
    color[2] = _imp->color->getValueAtTime(time,2);
}

int RotoDrawableItem::getCompositingOperator(int time) const
{
    return _imp->compOperator->getValueAtTime(time);
}

void RotoDrawableItem::getOverlayColor(double* color) const
{
    QMutexLocker l(&itemMutex);
    memcpy(color, _imp->overlayColor, sizeof(double) * 4);
}

void RotoDrawableItem::setOverlayColor(const double *color)
{
    ///MT-safe: only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    {
        QMutexLocker l(&itemMutex);
        memcpy(_imp->overlayColor, color, sizeof(double) * 4);
    }
    emit overlayColorChanged();
}



boost::shared_ptr<Bool_Knob> RotoDrawableItem::getActivatedKnob() const { return _imp->activated; }
boost::shared_ptr<Double_Knob> RotoDrawableItem::getFeatherKnob() const { return _imp->feather; }
boost::shared_ptr<Double_Knob> RotoDrawableItem::getFeatherFallOffKnob() const { return _imp->featherFallOff; }
boost::shared_ptr<Double_Knob> RotoDrawableItem::getOpacityKnob() const { return _imp->opacity; }
#ifdef NATRON_ROTO_INVERTIBLE
boost::shared_ptr<Bool_Knob> RotoDrawableItem::getInvertedKnob() const { return _imp->inverted; }
#endif
boost::shared_ptr<Choice_Knob> RotoDrawableItem::getOperatorKnob() const { return _imp->compOperator; }
boost::shared_ptr<Color_Knob> RotoDrawableItem::getColorKnob() const { return _imp->color; }
////////////////////////////////////Layer////////////////////////////////////

RotoLayer::RotoLayer(RotoContext* context,const std::string& n,RotoLayer* parent)
: RotoItem(context,n,parent)
, _imp(new RotoLayerPrivate())
{
    
}

RotoLayer::RotoLayer(const RotoLayer& other)
: RotoItem(other.getContext(),other.getName_mt_safe(),other.getParentLayer())
,_imp(new RotoLayerPrivate())
{
    clone(other);
}

RotoLayer::~RotoLayer()
{
    
}

void RotoLayer::clone(const RotoLayer& other)
{
    RotoItem::clone(other);
    
    QMutexLocker l(&itemMutex);
    _imp->items.clear();
    for (std::list<boost::shared_ptr<RotoItem> >::const_iterator it = other._imp->items.begin() ; it!= other._imp->items.end(); ++it) {
        RotoLayer* isLayer = dynamic_cast<RotoLayer*>(it->get());
        Bezier* isBezier = dynamic_cast<Bezier*>(it->get());
        if (isBezier) {
            boost::shared_ptr<Bezier> copy(new Bezier(*isBezier));
            copy->setParentLayer(this);
            _imp->items.push_back(copy);
        } else {
            assert(isLayer);
            boost::shared_ptr<RotoLayer> copy(new RotoLayer(*isLayer));
            copy->setParentLayer(this);
            _imp->items.push_back(copy);
            getContext()->addLayer(copy);
        }
    }
}

void RotoLayer::save(RotoItemSerialization *obj) const
{
    RotoLayerSerialization* s = dynamic_cast<RotoLayerSerialization*>(obj);
    assert(s);
    RotoItems items;
    {
        QMutexLocker l(&itemMutex);
        items = _imp->items;
    }
    
    for (RotoItems::const_iterator it = items.begin(); it!=items.end(); ++it) {
        Bezier* b = dynamic_cast<Bezier*>(it->get());
        RotoLayer* layer = dynamic_cast<RotoLayer*>(it->get());
        boost::shared_ptr<RotoItemSerialization> childSerialization;
        if (b) {
            childSerialization.reset(new BezierSerialization);
            b->save(childSerialization.get());
        } else
        {
            assert(layer);
            childSerialization.reset(new RotoLayerSerialization);
            layer->save(childSerialization.get());
        }
        assert(childSerialization);
        s->children.push_back(childSerialization);
    }

    
    RotoItem::save(obj);
}

void RotoLayer::load(const RotoItemSerialization &obj)
{
    
    const RotoLayerSerialization& s = dynamic_cast<const RotoLayerSerialization&>(obj);
    RotoItem::load(obj);
    {
        for (std::list<boost::shared_ptr<RotoItemSerialization> >::const_iterator it = s.children.begin(); it!=s.children.end(); ++it) {
            BezierSerialization* b = dynamic_cast<BezierSerialization*>(it->get());
            RotoLayerSerialization* l = dynamic_cast<RotoLayerSerialization*>(it->get());
            if (b) {
                boost::shared_ptr<Bezier> bezier(new Bezier(getContext(),kRotoBezierBaseName,this));
                bezier->load(*b);
                
                QMutexLocker l(&itemMutex);
                _imp->items.push_back(bezier);
            } else if (l) {
                boost::shared_ptr<RotoLayer> layer(new RotoLayer(getContext(),kRotoLayerBaseName,this));
                _imp->items.push_back(layer);
                getContext()->addLayer(layer);
                layer->load(*l);
            }
        }
        
    }
}

void RotoLayer::addItem(const boost::shared_ptr<RotoItem>& item)
{
    ///only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    QMutexLocker l(&itemMutex);
    _imp->items.push_back(item);
    
}

void RotoLayer::insertItem(const boost::shared_ptr<RotoItem>& item,int index)
{
    ///only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    assert(index >= 0);
    QMutexLocker l(&itemMutex);
    RotoItems::iterator it = _imp->items.begin();
    if (index >= (int)_imp->items.size()) {
        it = _imp->items.end();
    } else {
        std::advance(it, index);
    }
    ///insert before the iterator
    _imp->items.insert(it, item);
}

void RotoLayer::removeItem(const RotoItem* item)
{
    ///only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    QMutexLocker l(&itemMutex);
    for (RotoItems::iterator it = _imp->items.begin(); it!=_imp->items.end();++it)
    {
        if (it->get() == item) {
            _imp->items.erase(it);
            break;
        }
    }
}

int RotoLayer::getChildIndex(const boost::shared_ptr<RotoItem>& item) const
{
    QMutexLocker l(&itemMutex);
    int i = 0;
    for (RotoItems::iterator it = _imp->items.begin(); it!=_imp->items.end();++it,++i)
    {
        if (*it == item) {
            return i;
        }
    }
    return -1;
}

const RotoItems& RotoLayer::getItems() const
{
    ///only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    return _imp->items;
}


RotoItems RotoLayer::getItems_mt_safe() const
{
    QMutexLocker l(&itemMutex);
    return _imp->items;
}

////////////////////////////////////Bezier////////////////////////////////////

namespace  {
    
    enum SplineChangedReason {
        DERIVATIVES_CHANGED = 0,
        CONTROL_POINT_CHANGED = 1
    };
    
    void
    lerp(Point& dest, const Point& a, const Point& b, const float t)
    {
        dest.x = a.x + (b.x - a.x) * t;
        dest.y = a.y + (b.y - a.y) * t;
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
                      std::list< Point >* points,RectD* bbox = NULL)
    {
        Point p0,p1,p2,p3;
        
        try {
            first.getPositionAtTime(time, &p0.x, &p0.y);
            first.getRightBezierPointAtTime(time, &p1.x, &p1.y);
            last.getPositionAtTime(time, &p3.x, &p3.y);
            last.getLeftBezierPointAtTime(time, &p2.x, &p2.y);
        } catch (const std::exception& e) {
            assert(false);
        }
        
        if (mipMapLevel > 0) {
            int pot = 1 << mipMapLevel;
            p0.x /= pot;
            p0.y /= pot;
            
            p1.x /= pot;
            p1.y /= pot;
            
            p2.x /= pot;
            p2.y /= pot;
            
            p3.x /= pot;
            p3.y /= pot;
        }
        
        double incr = 1. / (double)(nbPointsPerSegment - 1);
        
        Point cur;
        for (double t = 0.; t <= 1.; t += incr) {
            
            bezier(cur,p0,p1,p2,p3,t);
            points->push_back(cur);
            
            if (bbox) {
                if (cur.x < bbox->x1) {
                    bbox->x1 = cur.x;
                }
                if (cur.x >= bbox->x2) {
                    bbox->x2 = cur.x + 1;
                }
                if (cur.y < bbox->y1) {
                    bbox->y1 = cur.y;
                }
                if (cur.y >= bbox->y2) {
                    bbox->y2 = cur.y;
                }
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
        first.getPositionAtTime(time, &p0.x, &p0.y);
        first.getRightBezierPointAtTime(time, &p1.x, &p1.y);
        last.getPositionAtTime(time, &p3.x, &p3.y);
        last.getLeftBezierPointAtTime(time, &p2.x, &p2.y);
        
        ///Use 100 points to make the segment
        double incr = 1. / (double)(100 - 1);
        
        ///the minimum square distance between a decasteljau point an the given (x,y) point
        ///we save a sqrt call
        
        double minDistance = INT_MAX;
        double tForMin = -1.;
        for (double t = 0.; t <= 1.; t += incr) {
            Point p;
            bezier(p,p0,p1,p2,p3,t);
            double dist = (p.x - x) * (p.x - x) + (p.y - y) * (p.y - y);
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



Bezier::Bezier(RotoContext* ctx,const std::string& name,RotoLayer* parent)
: RotoDrawableItem(ctx,name,parent)
, _imp(new BezierPrivate())
{
}

Bezier::Bezier(const Bezier& other)
: RotoDrawableItem(other.getContext(),other.getName_mt_safe(),other.getParentLayer())
, _imp(new BezierPrivate())
{
    clone(other);
}

void Bezier::clone(const Bezier& other)
{
    emit aboutToClone();
    {
        QMutexLocker l(&itemMutex);
        assert(other._imp->featherPoints.size() == other._imp->points.size());
        
        _imp->featherPoints.clear();
        _imp->points.clear();
        BezierCPs::const_iterator itF = other._imp->featherPoints.begin();
        for (BezierCPs::const_iterator it = other._imp->points.begin(); it!=other._imp->points.end(); ++it,++itF) {
            boost::shared_ptr<BezierCP> cp(new BezierCP(this));
            boost::shared_ptr<BezierCP> fp(new BezierCP(this));
            cp->clone(**it);
            fp->clone(**itF);
            _imp->featherPoints.push_back(fp);
            _imp->points.push_back(cp);
        }
        _imp->finished = other._imp->finished;
    }
    RotoDrawableItem::clone(other);
    emit cloned();
}


Bezier::~Bezier()
{
    BezierCPs::iterator itFp = _imp->featherPoints.begin();
    for (BezierCPs::iterator itCp = _imp->points.begin(); itCp != _imp->points.end(); ++itCp,++itFp) {
        Double_Knob* masterCp = (*itCp)->isSlaved();
        Double_Knob* masterFp = (*itFp)->isSlaved();
        if (masterCp) {
            masterCp->removeSlavedTrack(*itCp);
        }
        if (masterFp) {
            masterFp->removeSlavedTrack(*itFp);
        }
    }
}


boost::shared_ptr<BezierCP> Bezier::addControlPoint(double x,double y)
{
    ///only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    boost::shared_ptr<BezierCP> p;
    bool autoKeying = getContext()->isAutoKeyingEnabled();
    {
        QMutexLocker l(&itemMutex);
        assert(!_imp->finished);
        
        int keyframeTime;
        ///if the curve is empty make a new keyframe at the current timeline's time
        ///otherwise re-use the time at which the keyframe was set on the first control point
        if (_imp->points.empty()) {
            keyframeTime = getContext()->getTimelineCurrentTime();
        } else {
            
            keyframeTime = _imp->points.front()->getKeyframeTime(0);
            if (keyframeTime == INT_MAX) {
                keyframeTime = getContext()->getTimelineCurrentTime();
            }
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
    
    QMutexLocker l(&itemMutex);
        
    if (index >= (int)_imp->points.size() || index < -1) {
        throw std::invalid_argument("Spline control point index out of range.");
    }
    
    boost::shared_ptr<BezierCP> p(new BezierCP(this));
    boost::shared_ptr<BezierCP> fp(new FeatherPoint(this));
    ///we set the new control point position to be in the exact position the curve would have at each keyframe
    std::set<int> existingKeyframes;
    _imp->getKeyframeTimes(&existingKeyframes);
    
    BezierCPs::const_iterator prev,next,prevF,nextF;
    if (index == -1) {
        prev = _imp->points.end();
        --prev;
        next = _imp->points.begin();
        
        prevF = _imp->featherPoints.end();
        --prevF;
        nextF = _imp->featherPoints.begin();
    } else {
        prev = _imp->atIndex(index);
        next = prev;
        ++next;
        if (_imp->finished && next == _imp->points.end()) {
            next = _imp->points.begin();
        }
        assert(next != _imp->points.end());
        prevF = _imp->featherPoints.begin();
        std::advance(prevF, index);
        nextF = prevF;
        ++nextF;
        if (_imp->finished && nextF == _imp->featherPoints.end()) {
            nextF = _imp->featherPoints.begin();
        }
    }
  
    

    
    for (std::set<int>::iterator it = existingKeyframes.begin(); it!=existingKeyframes.end(); ++it) {
        
        Point p0,p1,p2,p3;
        (*prev)->getPositionAtTime(*it, &p0.x, &p0.y);
        (*prev)->getRightBezierPointAtTime(*it, &p1.x, &p1.y);
        (*next)->getPositionAtTime(*it, &p3.x, &p3.y);
        (*next)->getLeftBezierPointAtTime(*it, &p2.x, &p2.y);
        
        
        Point dst;
        Point p0p1,p1p2,p2p3,p0p1_p1p2,p1p2_p2p3;
        lerp(p0p1, p0,p1,t);
        lerp(p1p2, p1,p2,t);
        lerp(p2p3, p2,p3,t);
        lerp(p0p1_p1p2, p0p1,p1p2,t);
        lerp(p1p2_p2p3, p1p2,p2p3,t);
        lerp(dst,p0p1_p1p2,p1p2_p2p3,t);

        //update prev and next inner control points
        (*prev)->setRightBezierPointAtTime(*it, p0p1.x, p0p1.y);
        (*prevF)->setRightBezierPointAtTime(*it, p0p1.x, p0p1.y);
        
        (*next)->setLeftBezierPointAtTime(*it, p2p3.x, p2p3.y);
        (*nextF)->setLeftBezierPointAtTime(*it, p2p3.x, p2p3.y);
        
        p->setPositionAtTime(*it,dst.x,dst.y);
        ///The left control point of p is p0p1_p1p2 and the right control point is p1p2_p2p3
        p->setLeftBezierPointAtTime(*it, p0p1_p1p2.x,p0p1_p1p2.y);
        p->setRightBezierPointAtTime(*it, p1p2_p2p3.x, p1p2_p2p3.y);

        fp->setPositionAtTime(*it, dst.x, dst.y);
        fp->setLeftBezierPointAtTime(*it,p0p1_p1p2.x,p0p1_p1p2.y);
        fp->setRightBezierPointAtTime(*it, p1p2_p2p3.x, p1p2_p2p3.y);
    }
    
    ///if there's no keyframes
    if (existingKeyframes.empty()) {
        Point p0,p1,p2,p3;
        
        (*prev)->getPositionAtTime(0, &p0.x, &p0.y);
        (*prev)->getRightBezierPointAtTime(0, &p1.x, &p1.y);
        (*next)->getPositionAtTime(0, &p3.x, &p3.y);
        (*next)->getLeftBezierPointAtTime(0, &p2.x, &p2.y);

        
        Point dst;
        Point p0p1,p1p2,p2p3,p0p1_p1p2,p1p2_p2p3;
        lerp(p0p1, p0,p1,t);
        lerp(p1p2, p1,p2,t);
        lerp(p2p3, p2,p3,t);
        lerp(p0p1_p1p2, p0p1,p1p2,t);
        lerp(p1p2_p2p3, p1p2,p2p3,t);
        lerp(dst,p0p1_p1p2,p1p2_p2p3,t);

        //update prev and next inner control points
        (*prev)->setRightBezierStaticPosition(p0p1.x, p0p1.y);
        (*prevF)->setRightBezierStaticPosition(p0p1.x, p0p1.y);
        
        (*next)->setLeftBezierStaticPosition(p2p3.x, p2p3.y);
        (*nextF)->setLeftBezierStaticPosition(p2p3.x, p2p3.y);

        p->setStaticPosition(dst.x,dst.y);
        ///The left control point of p is p0p1_p1p2 and the right control point is p1p2_p2p3
        p->setLeftBezierStaticPosition(p0p1_p1p2.x,p0p1_p1p2.y);
        p->setRightBezierStaticPosition(p1p2_p2p3.x, p1p2_p2p3.y);
        
        fp->setStaticPosition(dst.x, dst.y);
        fp->setLeftBezierStaticPosition(p0p1_p1p2.x,p0p1_p1p2.y);
        fp->setRightBezierStaticPosition(p1p2_p2p3.x, p1p2_p2p3.y);
    }
    
    
    ////Insert the point into the container
    if (index != -1) {
        BezierCPs::iterator it = _imp->points.begin();
        ///it will point at the element right after index
        std::advance(it, index + 1);
        _imp->points.insert(it,p);
        
        ///insert the feather point
        BezierCPs::iterator itF = _imp->featherPoints.begin();
        std::advance(itF, index + 1);
        _imp->featherPoints.insert(itF, fp);
    } else {
        _imp->points.push_front(p);
        _imp->featherPoints.push_front(fp);
    }
    
    
    
    
    ///If auto-keying is enabled, set a new keyframe
    int currentTime = getContext()->getTimelineCurrentTime();
    if (!_imp->hasKeyframeAtTime(currentTime) && getContext()->isAutoKeyingEnabled()) {
        l.unlock();
        setKeyframe(currentTime);
    }
    return p;
}

int Bezier::getControlPointsCount() const
{
    QMutexLocker l(&itemMutex);
    return (int)_imp->points.size();
}


int Bezier::isPointOnCurve(double x,double y,double acceptance,double *t,bool* feather) const
{
    ///only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    
    int time = getContext()->getTimelineCurrentTime();
    
    QMutexLocker l(&itemMutex);
    
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
    QMutexLocker l(&itemMutex);
    _imp->finished = finished;
}

bool Bezier::isCurveFinished() const
{
    QMutexLocker l(&itemMutex);
    return _imp->finished;
}

void Bezier::removeControlPointByIndex(int index)
{
    ///only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    
    QMutexLocker l(&itemMutex);
    
    BezierCPs::iterator it;
    try {
        it = _imp->atIndex(index);
    } catch (...) {
        ///attempt to remove an unexsiting point
        return;
    }
    Double_Knob* isSlaved = (*it)->isSlaved();
    if (isSlaved) {
        isSlaved->removeSlavedTrack(*it);
    }
    _imp->points.erase(it);
    
    BezierCPs::iterator itF = _imp->featherPoints.begin();
    std::advance(itF, index);
    _imp->featherPoints.erase(itF);
}


void Bezier::movePointByIndex(int index,int time,double dx,double dy)
{
    ///only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    
    bool autoKeying = getContext()->isAutoKeyingEnabled();
    bool keySet = false;
    {
        QMutexLocker l(&itemMutex);
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
       
        bool fLinkEnabled = getContext()->isFeatherLinkEnabled();
        bool moveFeather = (fLinkEnabled || (!fLinkEnabled && (*it)->equalsAtTime(time, **itF)));
    
        
        
        if (autoKeying || isOnKeyframe) {
            (*it)->setPositionAtTime(time, x + dx, y + dy);
            (*it)->setLeftBezierPointAtTime(time, leftX + dx, leftY + dy);
            (*it)->setRightBezierPointAtTime(time, rightX + dx, rightY + dy);
            if (!isOnKeyframe) {
                keySet = true;
            }
        }
        
        if (moveFeather) {
            if (autoKeying || isOnKeyframe) {
                (*itF)->setPositionAtTime(time, xF + dx, yF + dy);
                (*itF)->setLeftBezierPointAtTime(time, leftXF + dx, leftYF + dy);
                (*itF)->setRightBezierPointAtTime(time, rightXF + dx, rightYF + dy);
            }
        }
        
        if (getContext()->isRippleEditEnabled()) {
            std::set<int> keyframes;
            _imp->getKeyframeTimes(&keyframes);
            for (std::set<int>::iterator it2 = keyframes.begin(); it2!=keyframes.end(); ++it2) {
                if (*it2 == time) {
                    continue;
                }
                (*it)->getPositionAtTime(*it2, &x, &y);
                (*it)->getLeftBezierPointAtTime(*it2, &leftX, &leftY);
                (*it)->getRightBezierPointAtTime(*it2, &rightX, &rightY);

                (*it)->setPositionAtTime(*it2, x + dx, y + dy);
                (*it)->setLeftBezierPointAtTime(*it2, leftX + dx, leftY + dy);
                (*it)->setRightBezierPointAtTime(*it2, rightX + dx, rightY + dy);
                if (moveFeather) {
                    
                    (*itF)->getPositionAtTime(*it2, &xF, &yF);
                    (*itF)->getLeftBezierPointAtTime(*it2, &leftXF, &leftYF);
                    (*itF)->getRightBezierPointAtTime(*it2, &rightXF, &rightYF);
                    
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
    if (keySet) {
        emit keyframeSet(time);
    }
}


void Bezier::moveFeatherByIndex(int index,int time,double dx,double dy)
{
    ///only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    
    bool autoKeying = getContext()->isAutoKeyingEnabled();
    bool keySet = false;
    {
        QMutexLocker l(&itemMutex);
        
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
            if (!isOnkeyframe) {
                keySet = true;
            }
        }
        
        if (getContext()->isRippleEditEnabled()) {
            std::set<int> keyframes;
            _imp->getKeyframeTimes(&keyframes);
            for (std::set<int>::iterator it2 = keyframes.begin(); it2!=keyframes.end(); ++it2) {
                if (*it2 == time) {
                    continue;
                }

                (*itF)->getPositionAtTime(*it2, &xF, &yF);
                (*itF)->getLeftBezierPointAtTime(*it2, &leftXF, &leftYF);
                (*itF)->getRightBezierPointAtTime(*it2, &rightXF, &rightYF);
                
                (*itF)->setPositionAtTime(*it2, xF + dx, yF + dy);
                (*itF)->setLeftBezierPointAtTime(*it2, leftXF + dx, leftYF + dy);
                (*itF)->setRightBezierPointAtTime(*it2, rightXF + dx, rightYF + dy);
            }
        }
    }
    if (autoKeying) {
        setKeyframe(time);
    }
    if (keySet) {
        emit keyframeSet(time);
    }
}

void Bezier::transformPoint(const boost::shared_ptr<BezierCP>& point,int time,Transform::Matrix3x3* matrix)
{
    
    bool autoKeying = getContext()->isAutoKeyingEnabled();
    bool keySet = false;
    {
        QMutexLocker l(&itemMutex);
        Transform::Point3D cp,leftCp,rightCp;
        point->getPositionAtTime(time, &cp.x, &cp.y);
        point->getLeftBezierPointAtTime(time, &leftCp.x, &leftCp.y);
        bool isonKeyframe = point->getRightBezierPointAtTime(time, &rightCp.x, &rightCp.y);
        
        
        cp.z = 1.;
        leftCp.z = 1.;
        rightCp.z = 1.;
        
        cp = matApply(*matrix, cp);
        leftCp = matApply(*matrix, leftCp);
        rightCp = matApply(*matrix, rightCp);
        
        cp.x /= cp.z; cp.y /= cp.z;
        leftCp.x /= leftCp.z; leftCp.y /= leftCp.z;
        rightCp.x /= rightCp.z; rightCp.y /= rightCp.z;
        
        if (autoKeying || isonKeyframe) {
            point->setPositionAtTime(time, cp.x, cp.y);
            point->setLeftBezierPointAtTime(time, leftCp.x, leftCp.y);
            point->setRightBezierPointAtTime(time, rightCp.x, rightCp.y);
            if (!isonKeyframe) {
                keySet = true;
            }
        }
    }
    if (keySet) {
        emit keyframeSet(time);
    }
}

void Bezier::moveLeftBezierPoint(int index,int time,double dx,double dy)
{
    ///only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    
    bool autoKeying = getContext()->isAutoKeyingEnabled();
    bool featherLink = getContext()->isFeatherLinkEnabled();
    bool rippleEdit = getContext()->isRippleEditEnabled();
    bool keySet = false;
    {
    QMutexLocker l(&itemMutex);
    
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
            if (!isOnKeyframe) {
                keySet = true;
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
        
        if (rippleEdit) {
            std::set<int> keyframes;
            _imp->getKeyframeTimes(&keyframes);
            for (std::set<int>::iterator it2 = keyframes.begin(); it2!=keyframes.end(); ++it2) {
                if (*it2 == time) {
                    continue;
                }

                (*cp)->getLeftBezierPointAtTime(*it2, &x, &y);
                (*cp)->setLeftBezierPointAtTime(*it2, x + dx, y + dy);
                if (moveFeather) {
                    (*fp)->getLeftBezierPointAtTime(*it2, &xF, &yF);
                    (*fp)->setLeftBezierPointAtTime(*it2, xF + dx, yF + dy);
                }
            }
        }
    }
    if (autoKeying) {
        setKeyframe(time);
    }
    if (keySet) {
        emit keyframeSet(time);
    }
}

void Bezier::moveRightBezierPoint(int index,int time,double dx,double dy)
{
    ///only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    
    bool autoKeying = getContext()->isAutoKeyingEnabled();
    bool featherLink = getContext()->isFeatherLinkEnabled();
    bool rippleEdit = getContext()->isRippleEditEnabled();
    bool keySet = false;
    {
    QMutexLocker l(&itemMutex);
    
    
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
            if (!isOnKeyframe) {
                keySet = true;
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
        
        if (rippleEdit) {
            std::set<int> keyframes;
            _imp->getKeyframeTimes(&keyframes);
            for (std::set<int>::iterator it2 = keyframes.begin(); it2!=keyframes.end(); ++it2) {
                if (*it2 == time) {
                    continue;
                }

                
                (*cp)->getRightBezierPointAtTime(*it2, &x, &y);
                (*cp)->setRightBezierPointAtTime(*it2, x + dx, y + dy);
                if (moveFeather) {
                    (*fp)->getRightBezierPointAtTime(*it2, &xF, &yF);
                    (*fp)->setRightBezierPointAtTime(*it2, xF + dx, yF + dy);
                }
            }
        }
        
    }
    if (autoKeying) {
        setKeyframe(time);
    }
    if (keySet) {
        emit keyframeSet(time);
    }
}

void Bezier::setLeftBezierPoint(int index,int time,double x,double y)
{
    ///only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    
    bool autoKeying = getContext()->isAutoKeyingEnabled();
    bool keySet = false;
    {
    QMutexLocker l(&itemMutex);
    
        BezierCPs::iterator cp = _imp->atIndex(index);
        assert(cp != _imp->points.end());
        
        BezierCPs::iterator fp = _imp->featherPoints.begin();
        std::advance(fp, index);
        
        bool isOnKeyframe = _imp->hasKeyframeAtTime(time);
        
        if (autoKeying || isOnKeyframe) {
            (*cp)->setLeftBezierPointAtTime(time, x, y);
            (*fp)->setLeftBezierPointAtTime(time, x, y);
            if (!isOnKeyframe) {
                keySet = true;
            }
        } else {
            (*cp)->setLeftBezierStaticPosition(x, y);
            (*fp)->setLeftBezierStaticPosition(x, y);
        }
        
        if (getContext()->isRippleEditEnabled()) {
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
    if (keySet) {
        emit keyframeSet(time);
    }
}

void Bezier::setRightBezierPoint(int index,int time,double x,double y)
{
    ///only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    
    bool autoKeying = getContext()->isAutoKeyingEnabled();
    bool keySet = false;
    {
        QMutexLocker l(&itemMutex);
        
        BezierCPs::iterator cp = _imp->atIndex(index);
        assert(cp != _imp->points.end());
        
        BezierCPs::iterator fp = _imp->featherPoints.begin();
        std::advance(fp, index);
        
        bool isOnKeyframe = _imp->hasKeyframeAtTime(time);
        
        if (autoKeying || isOnKeyframe) {
            (*cp)->setRightBezierPointAtTime(time, x, y);
            (*fp)->setRightBezierPointAtTime(time, x, y);
            if (!isOnKeyframe) {
                keySet = true;
            }
        } else {
            (*cp)->setRightBezierStaticPosition(x, y);
            (*fp)->setRightBezierStaticPosition(x, y);
        }
        
        if (getContext()->isRippleEditEnabled()) {
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
    if (keySet) {
        emit keyframeSet(time);
    }
    
}

void Bezier::setPointAtIndex(bool feather,int index,int time,double x,double y,double lx,double ly,double rx,double ry)
{
    ///only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    
    bool autoKeying = getContext()->isAutoKeyingEnabled();
    bool rippleEdit = getContext()->isRippleEditEnabled();
    bool keySet = false;
    
    {
        QMutexLocker l(&itemMutex);
        
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
            if (!isOnKeyframe) {
                keySet = true;
            }
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
    if (keySet) {
        emit keyframeSet(time);
    }
}

void Bezier::movePointLeftAndRightIndex(BezierCP& p,int time,double lx,double ly,double rx,double ry)
{
    ///only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    
    bool autoKeying = getContext()->isAutoKeyingEnabled();
    bool rippleEdit = getContext()->isRippleEditEnabled();
    bool keySet = false;
    {
        QMutexLocker l(&itemMutex);
        
        bool isOnKeyframe = _imp->hasKeyframeAtTime(time);
        double leftX,leftY,rightX,rightY;
        p.getLeftBezierPointAtTime(time, &leftX, &leftY);
        p.getRightBezierPointAtTime(time, &rightX, &rightY);
        
        if (autoKeying || isOnKeyframe) {
            p.setLeftBezierPointAtTime(time, leftX + lx,leftY +  ly);
            p.setRightBezierPointAtTime(time, rightX + rx, rightY + ry);
            if (!isOnKeyframe) {
                keySet = true;
            }
        }
        
        if (rippleEdit) {
            std::set<int> keyframes;
            _imp->getKeyframeTimes(&keyframes);
            for (std::set<int>::iterator it2 = keyframes.begin(); it2!=keyframes.end(); ++it2) {
                if (*it2 == time) {
                    continue;
                }
                p.getLeftBezierPointAtTime(*it2, &leftX, &leftY);
                p.getRightBezierPointAtTime(*it2, &rightX, &rightY);
                p.setLeftBezierPointAtTime(*it2, leftX + lx,leftY +  ly);
                p.setRightBezierPointAtTime(*it2, rightX + rx, rightY + ry);
                
            }
        }
    }
    if (autoKeying) {
        setKeyframe(time);
    }
    if (keySet) {
        emit keyframeSet(time);
    }
}

void Bezier::clonePoint(BezierCP& p,const BezierCP& to) const
{
    QMutexLocker l(&itemMutex);
    p.clone(to);
}

void Bezier::removeFeatherAtIndex(int index)
{
    ///only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    QMutexLocker l(&itemMutex);
    
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
    bool keySet = false;
    bool autoKeying = getContext()->isAutoKeyingEnabled();
    bool rippleEdit = getContext()->isRippleEditEnabled();
    
    {
        QMutexLocker l(&itemMutex);
        if (index >= (int)_imp->points.size()) {
            throw std::invalid_argument("Bezier::smoothPointAtIndex: Index out of range.");
        }
        
        BezierCPs::iterator cp = _imp->atIndex(index);
        BezierCPs::iterator fp = _imp->featherPoints.begin();
        std::advance(fp, index);
        
        assert(cp != _imp->points.end() && fp != _imp->featherPoints.end());
        (*cp)->smoothPoint(time,autoKeying,rippleEdit);
        keySet = (*fp)->smoothPoint(time,autoKeying,rippleEdit);
        
    }
    if (autoKeying) {
        setKeyframe(time);
    }
    if (keySet) {
        emit keyframeSet(time);
    }
}


void Bezier::cuspPointAtIndex(int index,int time)
{
    ///only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    
    bool autoKeying = getContext()->isAutoKeyingEnabled();
    bool rippleEdit = getContext()->isRippleEditEnabled();
    bool keySet = false;
    {
        QMutexLocker l(&itemMutex);
        if (index >= (int)_imp->points.size()) {
            throw std::invalid_argument("Bezier::cuspPointAtIndex: Index out of range.");
        }
        
        BezierCPs::iterator cp = _imp->atIndex(index);
        BezierCPs::iterator fp = _imp->featherPoints.begin();
        std::advance(fp, index);
        
        assert(cp != _imp->points.end() && fp != _imp->featherPoints.end());
        (*cp)->cuspPoint(time,autoKeying,rippleEdit);
        keySet = (*fp)->cuspPoint(time,autoKeying,rippleEdit);
       
    }
    if (autoKeying) {
        setKeyframe(time);
    }
    if (keySet) {
        emit keyframeSet(time);
    }
    
}

void Bezier::setKeyframe(int time)
{
    ///only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());

    {
        

        QMutexLocker l(&itemMutex);
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
    emit keyframeSet(time);

}


void Bezier::removeKeyframe(int time)
{
    ///only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    
    {
        QMutexLocker l(&itemMutex);
        
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
    emit keyframeRemoved(time);
}


int Bezier::getKeyframesCount() const
{
    QMutexLocker l(&itemMutex);
    if (_imp->points.empty()) {
        return 0;
    } else {
        return _imp->points.front()->getKeyframesCount();
    }
}



void Bezier::evaluateAtTime_DeCasteljau(int time,unsigned int mipMapLevel,
                                        int nbPointsPerSegment,std::list< Natron::Point >* points,RectD* bbox) const
{
    QMutexLocker l(&itemMutex);
    BezierCPs::const_iterator next = _imp->points.begin();
	if (_imp->points.empty()) {
			return;
	}
    ++next;
    for (BezierCPs::const_iterator it = _imp->points.begin(); it != _imp->points.end(); ++it,++next) {
        if (next == _imp->points.end()) {
            if (!_imp->finished) {
                break;
            }
            next = _imp->points.begin();
        }
        evalBezierSegment(*(*it),*(*next), time,mipMapLevel, nbPointsPerSegment, points,bbox);
    }
}

void Bezier::evaluateFeatherPointsAtTime_DeCasteljau(int time,unsigned int mipMapLevel,int nbPointsPerSegment,
                                                     std::list< Natron::Point >* points,bool evaluateIfEqual,RectD* bbox) const
{
    QMutexLocker l(&itemMutex);
	if (_imp->points.empty()) {
		return;
	}
    BezierCPs::const_iterator itCp = _imp->points.begin();
    BezierCPs::const_iterator next = _imp->featherPoints.begin();
    ++next;
    BezierCPs::const_iterator nextCp = itCp;
    ++nextCp;
    for (BezierCPs::const_iterator it = _imp->featherPoints.begin(); it != _imp->featherPoints.end(); ++it,++itCp,++next,++nextCp) {
        
        if (next == _imp->featherPoints.end()) {
            next = _imp->featherPoints.begin();
        }
        if (nextCp == _imp->points.end()) {
            if (!_imp->finished) {
                break;
            }
            nextCp = _imp->points.begin();
        }
        if (!evaluateIfEqual && !areSegmentDifferents(time, **itCp, **nextCp, **it, **next))
        {
            continue;
        }

        evalBezierSegment(*(*it),*(*next), time,mipMapLevel, nbPointsPerSegment, points,bbox);
        
    }
}

RectD Bezier::getBoundingBox(int time) const
{
    std::list<Point> pts;
    RectD bbox;
    bbox.x1 = INT_MAX;
    bbox.x2 = INT_MIN;
    bbox.y1 = INT_MAX;
    bbox.y2 = INT_MIN;

    evaluateAtTime_DeCasteljau(time,0, 50,&pts,&bbox);
    evaluateFeatherPointsAtTime_DeCasteljau(time, 0, 50, &pts, false, &bbox);
    
    if (bbox.x1 == INT_MAX) {
        bbox.x1 = 0;
    }
    if (bbox.x2 == INT_MIN) {
        bbox.x2 = 1;
    }
    if (bbox.y1 == INT_MAX) {
        bbox.y1 = 0;
    }
    if (bbox.y2 == INT_MIN) {
        bbox.y2 = 1;
    }
    if (bbox.isNull()) {
        bbox.x2 = bbox.x1 + 1;
        bbox.y2 = bbox.y1 + 1;
    }
    return bbox;
}

const std::list< boost::shared_ptr<BezierCP> >& Bezier::getControlPoints() const
{
    ///only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    return _imp->points;
}

std::list< boost::shared_ptr<BezierCP> > Bezier::getControlPoints_mt_safe() const
{
    QMutexLocker l(&itemMutex);
    return _imp->points;
}

const std::list< boost::shared_ptr<BezierCP> >& Bezier::getFeatherPoints() const
{
    ///only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    return _imp->featherPoints;
}

std::list< boost::shared_ptr<BezierCP> > Bezier::getFeatherPoints_mt_safe() const
{
    QMutexLocker l(&itemMutex);
    return _imp->featherPoints;
}

std::pair<boost::shared_ptr<BezierCP>,boost::shared_ptr<BezierCP> >
Bezier::isNearbyControlPoint(double x,double y,double acceptance,ControlPointSelectionPref pref,int* index) const
{
    ///only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    int time = getContext()->getTimelineCurrentTime();
    
    QMutexLocker l(&itemMutex);
    boost::shared_ptr<BezierCP> cp,fp;
    
    switch (pref) {
        case FEATHER_FIRST:
        {
            BezierCPs::const_iterator itF = _imp->findFeatherPointNearby(x, y, acceptance, time, index);
            if (itF != _imp->featherPoints.end()) {
                fp = *itF;
                BezierCPs::const_iterator it = _imp->points.begin();
                std::advance(it, *index);
                cp = *it;
                return std::make_pair(fp, cp);
            } else {
                BezierCPs::const_iterator it = _imp->findControlPointNearby(x, y, acceptance, time, index);
                if (it != _imp->points.end()) {
                    cp = *it;
                    itF = _imp->featherPoints.begin();
                    std::advance(itF, *index);
                    fp = *itF;
                    return std::make_pair(cp, fp);
                }
            }
        }  break;
        case CONTROL_POINT_FIRST:
        case WHATEVER_FIRST:
        default:
        {
            BezierCPs::const_iterator it = _imp->findControlPointNearby(x, y, acceptance, time, index);
            if (it != _imp->points.end()) {
                cp = *it;
                BezierCPs::const_iterator itF = _imp->featherPoints.begin();
                std::advance(itF, *index);
                fp = *itF;
                return std::make_pair(cp, fp);
            } else {
                BezierCPs::const_iterator itF = _imp->findFeatherPointNearby(x, y, acceptance, time, index);
                if (itF != _imp->featherPoints.end()) {
                    fp = *itF;
                    it = _imp->points.begin();
                    std::advance(it, *index);
                    cp = *it;
                    return std::make_pair(fp, cp);
                }
            }

        }   break;

        
    }
    
    ///empty pair
    *index = -1;
    return std::make_pair(cp,fp);
}

int Bezier::getControlPointIndex(const boost::shared_ptr<BezierCP>& cp) const
{
    return getControlPointIndex(cp.get());
}

int Bezier::getControlPointIndex(const BezierCP* cp) const
{
    ///only called on the main-thread
    assert(cp);
    QMutexLocker l(&itemMutex);
    
    int i = 0;
    for (BezierCPs::const_iterator it = _imp->points.begin();it!=_imp->points.end();++it,++i)
    {
        if (it->get() == cp) {
            return i;
        }
    }
    return -1;
}

int Bezier::getFeatherPointIndex(const boost::shared_ptr<BezierCP>& fp) const
{
    ///only called on the main-thread
    QMutexLocker l(&itemMutex);
    
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
    QMutexLocker l(&itemMutex);
    if (index >= (int)_imp->points.size()) {
        return boost::shared_ptr<BezierCP>();
    }
    
    BezierCPs::const_iterator it = _imp->points.begin();
    std::advance(it, index);
    return *it;
}

boost::shared_ptr<BezierCP> Bezier::getFeatherPointAtIndex(int index) const
{
    QMutexLocker l(&itemMutex);
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
    QMutexLocker locker(&itemMutex);
    
    int time = getContext()->getTimelineCurrentTime();
    
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
    prev.getPositionAtTime(time, &p0.x, &p0.y);
    prev.getRightBezierPointAtTime(time, &p1.x, &p1.y);
    p.getLeftBezierPointAtTime(time, &p2.x, &p2.y);
    p.getPositionAtTime(time, &p3.x, &p3.y);
    p0equalsP1 = p0.x == p1.x && p0.y == p1.y;
    p1equalsP2 = p1.x == p2.x && p1.y == p2.y;
    p2equalsP3 = p2.x == p3.x && p2.y == p3.y;
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
        *dx = p0.x - p3.x;
        *dy = p0.y - p3.y;
    } else if (degree == 2) {
        if (p0equalsP1) {
            p1 = p2;
        }
        *dx = 2. * (p1.x - p3.x);
        *dy = 2. * (p1.y - p3.y);
    } else {
        *dx = 3. * (p2.x - p3.x);
        *dy = 3. * (p2.y - p3.y);
    }
}

void Bezier::rightDerivativeAtPoint(int time,const BezierCP& p,const BezierCP& next,double *dx,double *dy)
{
    ///First-off, determine if the segment is a linear/quadratic/cubic bezier segment.
    assert(!p.equalsAtTime(time, next));
    bool p0equalsP1,p1equalsP2,p2equalsP3;
    Point p0,p1,p2,p3;
    p.getPositionAtTime(time, &p0.x, &p0.y);
    p.getRightBezierPointAtTime(time, &p1.x, &p1.y);
    next.getLeftBezierPointAtTime(time, &p2.x, &p2.y);
    next.getPositionAtTime(time, &p3.x, &p3.y);
    p0equalsP1 = p0.x == p1.x && p0.y == p1.y;
    p1equalsP2 = p1.x == p2.x && p1.y == p2.y;
    p2equalsP3 = p2.x == p3.x && p2.y == p3.y;
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
        *dx = p3.x - p0.x;
        *dy = p3.y - p0.y;
    } else if (degree == 2) {
        if (p0equalsP1) {
            p1 = p2;
        }
        *dx = 2. * (p1.x - p0.x);
        *dy = 2. * (p1.y - p0.y);
    } else {
        *dx = 3. * (p1.x - p0.x);
        *dy = 3. * (p1.y - p0.y);
    }

}

void Bezier::save(RotoItemSerialization* obj) const
{
    BezierSerialization* s = dynamic_cast<BezierSerialization*>(obj);
    {
        QMutexLocker l(&itemMutex);
        
        s->_closed = _imp->finished;
        
        assert(_imp->featherPoints.size() == _imp->points.size());
        
        
        BezierCPs::const_iterator fp = _imp->featherPoints.begin();
        for (BezierCPs::const_iterator it = _imp->points.begin(); it != _imp->points.end(); ++it,++fp) {
            BezierCP c,f;
            c.clone(**it);
            f.clone(**fp);
            s->_controlPoints.push_back(c);
            s->_featherPoints.push_back(f);
        }
    }
    RotoDrawableItem::save(obj);
}

void Bezier::load(const RotoItemSerialization& obj)
{
    const BezierSerialization& s = dynamic_cast<const BezierSerialization&>(obj);
    {
        QMutexLocker l(&itemMutex);
        _imp->finished = s._closed;
        
        
        if (s._controlPoints.size() != s._featherPoints.size()) {
            ///do not load broken serialization objects
            return;
        }
        
        std::list<BezierCP>::const_iterator itF = s._featherPoints.begin();
        for (std::list<BezierCP>::const_iterator it = s._controlPoints.begin();it!=s._controlPoints.end();++it,++itF)
        {
            boost::shared_ptr<BezierCP> cp(new BezierCP(this));
            cp->clone(*it);
            _imp->points.push_back(cp);
            
            boost::shared_ptr<BezierCP> fp(new FeatherPoint(this));
            fp->clone(*itF);
            _imp->featherPoints.push_back(fp);
        }
    }
    RotoDrawableItem::load(obj);
}

void Bezier::getKeyframeTimes(std::set<int> *times) const
{
    QMutexLocker l(&itemMutex);
    _imp->getKeyframeTimes(times);
}

int Bezier::getPreviousKeyframeTime(int time) const
{
    std::set<int> times;
    QMutexLocker l(&itemMutex);
    _imp->getKeyframeTimes(&times);
    for (std::set<int>::reverse_iterator it = times.rbegin(); it!=times.rend(); ++it) {
        if (*it < time) {
            return *it;
        }
    }
    return INT_MIN;
}

int Bezier::getNextKeyframeTime(int time) const
{
    std::set<int> times;
    QMutexLocker l(&itemMutex);
    _imp->getKeyframeTimes(&times);
    for (std::set<int>::iterator it = times.begin(); it!=times.end(); ++it) {
        if (*it > time) {
            return *it;
        }
    }
    return INT_MAX;
}

void Bezier::precomputePointInPolygonTables(const std::list<Point>& polygon,
                                           std::vector<double>* constants,std::vector<double>* multiples)
{
    assert(constants->size() == multiples->size() && constants->size() == polygon.size());
    
    std::list<Point>::const_iterator i = polygon.begin();
	if (polygon.empty()) {
		return;
	}
    ++i;
    int index = 0;
    for (std::list<Point>::const_iterator j = polygon.begin(); j!=polygon.end(); ++j,++i,++index) {
        if (i == polygon.end()) {
            i = polygon.begin();
        }
        if (i->y == j->y) {
            constants->at(index) = j->x;
            multiples->at(index) = 0;
        } else {
            double multiplier = (j->x - i->x) / (j->y - i->y);
            constants->at(index) = i->x - i->y * multiplier;
            multiples->at(index) = multiplier;
        }
    }
}

bool Bezier::pointInPolygon(const Point& p,const std::list<Point>& polygon,
                            const std::vector<double>& constants,
                            const std::vector<double>& multiples,
                            const RectD& featherPolyBBox) {
    
    assert(constants.size() == multiples.size() && constants.size() == polygon.size());
    
    ///first check if the point lies inside the bounding box
    if (p.x < featherPolyBBox.x1 || p.x >= featherPolyBBox.x2 || p.y < featherPolyBBox.y1 || p.y >= featherPolyBBox.y2) {
        return false;
    }
    
    bool odd = false;
    std::list<Point>::const_iterator i = polygon.begin();
    ++i;
    std::vector<double>::const_iterator cIt = constants.begin();
    std::vector<double>::const_iterator mIt = multiples.begin();
    
    for (std::list<Point>::const_iterator j = polygon.begin(); j!=polygon.end(); ++j,++i,++cIt,++mIt) {
        if (i == polygon.end()) {
            i = polygon.begin();
        }
        if (((i->y > p.y) != (j->y > p.y)) &&
            (p.x < (p.y * *mIt + *cIt))) {
            odd = !odd;
        }
    }
    return odd;
}

/**
 * @brief Computes the location of the feather extent relative to the current feather point position and
 * the given feather distance.
 * In the case the control point and the feather point of the bezier are distinct, this function just makes use
 * of Thales theorem.
 * If the feather point and the control point are equal then this function computes the left and right derivative
 * of the bezier at that point to determine the direction in which the extent is.
 * @returns The delta from the given feather point to apply to find out the extent position.
 *
 * Note that the delta will be applied to fp.
 **/
Point Bezier::expandToFeatherDistance(const Point& cp, //< the point
                                      Point* fp, //< the feather point
                                      double featherDistance, //< feather distance
                                      const std::list<Point>& featherPolygon, //< the polygon of the bezier
                                      const std::vector<double>& constants, //< helper to speed-up pointInPolygon computations
                                      const std::vector<double>& multiples, //< helper to speed-up pointInPolygon computations
                                      const RectD& featherPolyBBox, //< helper to speed-up pointInPolygon computations
                                      int time, //< time
                                      BezierCPs::const_iterator prevFp, //< iterator pointing to the feather before curFp
                                      BezierCPs::const_iterator curFp, //< iterator pointing to fp
                                      BezierCPs::const_iterator nextFp) //< iterator pointing after curFp
{
    Point ret;
    if (featherDistance != 0) {
        
        ///shortcut when the feather point is different than the control point
        if (cp.x != fp->x && cp.y != fp->y) {
            double dx = (fp->x - cp.x);
            double dy = (fp->y - cp.y);
            double dist = sqrt(dx * dx + dy * dy);
            ret.x = (dx * (dist + featherDistance)) / dist;
            ret.y = (dy * (dist + featherDistance)) / dist;
            fp->x =  ret.x + cp.x;
            fp->y =  ret.y + cp.y;
        } else {
            //compute derivatives to determine the feather extent
            double leftX,leftY,rightX,rightY,norm;
            Bezier::leftDerivativeAtPoint(time, **curFp, **prevFp, &leftX, &leftY);
            Bezier::rightDerivativeAtPoint(time, **curFp, **nextFp, &rightX, &rightY);
            norm = sqrt((rightX - leftX) * (rightX - leftX) + (rightY - leftY) * (rightY - leftY));
  
            ///normalize derivatives by their norm
            if (norm != 0) {
                ret.x = -((rightY - leftY) / norm);
                ret.y = ((rightX - leftX) / norm);
            } else {
                ///both derivatives are the same, use the direction of the left one
                norm = sqrt((leftX - cp.x) * (leftX - cp.x) + (leftY - cp.y) * (leftY - cp.y));
                if (norm != 0) {
                    ret.x = -((leftY - cp.y) / norm);
                    ret.y = ((leftX - cp.x) / norm) ;
                } else {
                    ///both derivatives and control point are equal, just use 0
                    ret.x = ret.y = 0;
                }
            }
            
            
            ///retrieve the position of the extent of the feather
            ///To retrieve the extent which is in not inside the polygon we test the 2 points
            
            ///Note that we're testing with a normalized vector because the polygon could be 1 pixel thin
            ///and the test would yield wrong results. We multiply by the distance once we've done the test.
            Point extent;
            extent.x = cp.x + ret.x;
            extent.y = cp.y + ret.y;
            
            bool inside = pointInPolygon(extent, featherPolygon, constants, multiples,featherPolyBBox);
            if ((!inside && featherDistance > 0) || (inside && featherDistance < 0)) {
                //*fp = extent;
                fp->x = cp.x + ret.x * featherDistance;
                fp->y = cp.y + ret.y * featherDistance;
            } else {
                fp->x = cp.x - ret.x * featherDistance;
                fp->y = cp.y - ret.y * featherDistance;
            }
        }
    } else {
        ret.x = ret.y = 0;
    }
    return ret;
    
}

////////////////////////////////////RotoContext////////////////////////////////////


RotoContext::RotoContext(Natron::Node* node)
: _imp(new RotoContextPrivate(node))
{
    ////Add the base layer
    boost::shared_ptr<RotoLayer> base = addLayer();
    deselect(base,RotoContext::OTHER);
}

RotoContext::~RotoContext()
{
    
}

boost::shared_ptr<RotoLayer> RotoContext::addLayer()
{
    int no;
    
    ///MT-safe: only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    boost::shared_ptr<RotoLayer> item;
    {
        QMutexLocker l(&_imp->rotoContextMutex);
        std::map<std::string, int>::iterator it = _imp->itemCounters.find(kRotoLayerBaseName);
        if (it != _imp->itemCounters.end()) {
            ++it->second;
            no = it->second;
        } else {
            _imp->itemCounters.insert(std::make_pair(kRotoLayerBaseName, 1));
            no = 1;
        }
        std::stringstream ss;
        ss << kRotoLayerBaseName << ' ' << no;
        
        RotoLayer* deepestLayer = findDeepestSelectedLayer();
        
        RotoLayer* parentLayer = 0;
        if (!deepestLayer) {
            ///find out if there's a base layer, if so add to the base layer,
            ///otherwise create the base layer
            for (std::list<boost::shared_ptr<RotoLayer> >::iterator it = _imp->layers.begin(); it!=_imp->layers.end(); ++it) {
                int hierarchy = (*it)->getHierarchyLevel();
                if (hierarchy == 0) {
                    parentLayer = it->get();
                    break;
                }
            }
            
            
        } else {
            parentLayer = deepestLayer;
        }
        
        item.reset(new RotoLayer(this,ss.str(),parentLayer));
        if (parentLayer) {
            parentLayer->addItem(item);
        }
        _imp->layers.push_back(item);
        
        _imp->lastInsertedItem = item;
    }
    emit itemInserted(RotoContext::OTHER);
    
    clearSelection(RotoContext::OTHER);
    select(item, RotoContext::OTHER);
    return item;
}

void RotoContext::addLayer(const boost::shared_ptr<RotoLayer>& layer)
{
    std::list<boost::shared_ptr<RotoLayer> >::iterator it = std::find(_imp->layers.begin(), _imp->layers.end(), layer);
    if (it == _imp->layers.end()) {
        _imp->layers.push_back(layer);
    }
    
}

boost::shared_ptr<RotoItem> RotoContext::getLastInsertedItem() const
{
    ///MT-safe: only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    return _imp->lastInsertedItem;
}

#ifdef NATRON_ROTO_INVERTIBLE
boost::shared_ptr<Bool_Knob> RotoContext::getInvertedKnob() const
{
    return _imp->inverted;
}
#endif

boost::shared_ptr<Color_Knob> RotoContext::getColorKnob() const
{
    return _imp->colorKnob;
}

boost::shared_ptr<Choice_Knob> RotoContext::getOperatorKnob() const
{
    return _imp->compOperator;
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

boost::shared_ptr<Bezier> RotoContext::makeBezier(double x,double y,const std::string& baseName)
{
    ///MT-safe: only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    RotoLayer* parentLayer = 0;
    std::stringstream ss;
    {
        QMutexLocker l(&_imp->rotoContextMutex);
        int no;
        std::map<std::string, int>::iterator it = _imp->itemCounters.find(baseName);
        if (it != _imp->itemCounters.end()) {
            ++it->second;
            no = it->second;
        } else {
            _imp->itemCounters.insert(std::make_pair(baseName, 1));
            no = 1;
        }
        
        ss << baseName << ' ' << no;
        
        RotoLayer* deepestLayer = findDeepestSelectedLayer();
        
        
        if (!deepestLayer) {
            ///if there is no base layer, create one
            if (_imp->layers.empty()) {
                l.unlock();
                addLayer();
                l.relock();
            }
            parentLayer = _imp->layers.front().get();
            
        } else {
            parentLayer = deepestLayer;
        }
        
    }
    assert(parentLayer);
    boost::shared_ptr<Bezier> curve(new Bezier(this,ss.str(),parentLayer));
    if (parentLayer) {
        parentLayer->addItem(curve);
    }
    _imp->lastInsertedItem = curve;
    emit itemInserted(RotoContext::OTHER);

    clearSelection(RotoContext::OTHER);
    select(curve, RotoContext::OTHER);
    
    if (isAutoKeyingEnabled()) {
        curve->setKeyframe(getTimelineCurrentTime());
    }
    curve->addControlPoint(x, y);
   
    return curve;
}

void RotoContext::removeItemRecursively(RotoItem* item,SelectionReason reason)
{
    RotoLayer* isLayer = dynamic_cast<RotoLayer*>(item);
    boost::shared_ptr<RotoItem> foundSelected;
    for (std::list< boost::shared_ptr<RotoItem> >::iterator it = _imp->selectedItems.begin(); it!=_imp->selectedItems.end(); ++it) {
        if (it->get() == item) {
            foundSelected = *it;
            break;
        }
    }
    if (foundSelected) {
        deselectInternal(foundSelected);
    }
    
    if (isLayer) {
        const RotoItems& items = isLayer->getItems();
        for (RotoItems::const_iterator it = items.begin(); it!=items.end(); ++it) {
            removeItemRecursively(it->get(),reason);
        }
        for (std::list<boost::shared_ptr<RotoLayer> >::iterator it = _imp->layers.begin(); it!=_imp->layers.end(); ++it) {
            if (it->get() == isLayer) {
                _imp->layers.erase(it);
                break;
            }
        }
    }
    emit itemRemoved(item,(int)reason);

}

void RotoContext::removeItem(RotoItem* item,SelectionReason reason)
{
    ///MT-safe: only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    {
        QMutexLocker l(&_imp->rotoContextMutex);
        RotoLayer* layer = item->getParentLayer();
        if (layer) {
            layer->removeItem(item);
        }
        removeItemRecursively(item,reason);
    }
    emit selectionChanged((int)reason);
}

void RotoContext::addItem(RotoLayer* layer,int indexInLayer,const boost::shared_ptr<RotoItem>& item,SelectionReason reason)
{
    ///MT-safe: only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    {
        QMutexLocker l(&_imp->rotoContextMutex);
        if (layer) {
            layer->insertItem(item,indexInLayer);
        }
        boost::shared_ptr<RotoLayer> isLayer = boost::dynamic_pointer_cast<RotoLayer>(item);
        if (isLayer) {
            std::list<boost::shared_ptr<RotoLayer> >::iterator foundLayer = std::find(_imp->layers.begin(), _imp->layers.end(), isLayer);
            if (foundLayer == _imp->layers.end()) {
                _imp->layers.push_back(isLayer);
            }
        }
        _imp->lastInsertedItem = item;
    }
    emit itemInserted(reason);

}

const std::list< boost::shared_ptr<RotoLayer> >& RotoContext::getLayers() const
{
    ///MT-safe: only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    return _imp->layers;
}

boost::shared_ptr<Bezier>  RotoContext::isNearbyBezier(double x,double y,double acceptance,int* index,double* t,bool* feather) const
{
    ///MT-safe: only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());

    QMutexLocker l(&_imp->rotoContextMutex);
    for (std::list< boost::shared_ptr<RotoLayer> >::const_iterator it = _imp->layers.begin();it!=_imp->layers.end();++it) {
        const RotoItems& items = (*it)->getItems();
        for (RotoItems::const_iterator it2 = items.begin(); it2 != items.end(); ++it2) {
            boost::shared_ptr<Bezier> b = boost::dynamic_pointer_cast<Bezier>(*it2);
            if (b) {
                double param;
                int i = b->isPointOnCurve(x, y, acceptance, &param,feather);
                if (i != -1) {
                    *index = i;
                    *t = param;
                    return b;
                }
            }
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
    
    bool first = true;
    for (std::list<boost::shared_ptr<RotoLayer> >::const_iterator it = _imp->layers.begin(); it!=_imp->layers.end(); ++it) {
        RotoItems items = (*it)->getItems_mt_safe();
        for (RotoItems::iterator it2 = items.begin(); it2 != items.end(); ++it2) {
            Bezier* b = dynamic_cast<Bezier*>(it2->get());
            if (b && b->isActivated(time) && b->isCurveFinished()) {
                
                RectD splineRoD = b->getBoundingBox(time);
                RectI splineRoDI;
                splineRoDI.x1 = std::floor(splineRoD.x1);
                splineRoDI.y1 = std::floor(splineRoD.y1);
                splineRoDI.x2 = std::ceil(splineRoD.x2);
                splineRoDI.y2 = std::ceil(splineRoD.y2);
                
                if (first) {
                    first = false;
                    *rod = splineRoDI;
                } else {
                    rod->merge(splineRoDI);
                }
            }
        }
    }
}

void RotoContext::save(RotoContextSerialization* obj) const
{
    QMutexLocker l(&_imp->rotoContextMutex);
    obj->_autoKeying = _imp->autoKeying;
    obj->_featherLink = _imp->featherLink;
    obj->_rippleEdit = _imp->rippleEdit;
    
    ///There must always be the base layer
    assert(!_imp->layers.empty());
    
    ///Serializing this layer will recursively serialize everything
    _imp->layers.front()->save(dynamic_cast<RotoItemSerialization*>(&obj->_baseLayer));
    
    ///the age of the context is not serialized as the images are wiped from the cache anyway

    ///Serialize the selection
    for (std::list<boost::shared_ptr<RotoItem> >::const_iterator it = _imp->selectedItems.begin(); it!=_imp->selectedItems.end(); ++it) {
        obj->_selectedItems.push_back((*it)->getName_mt_safe());
    }
    
    obj->_itemCounters = _imp->itemCounters;
}

static void linkItemsKnobsRecursively(RotoContext* ctx,const boost::shared_ptr<RotoLayer>& layer)
{
    const RotoItems& items = layer->getItems();
    for (RotoItems::const_iterator it = items.begin(); it!=items.end(); ++it) {
        boost::shared_ptr<Bezier> isBezier = boost::dynamic_pointer_cast<Bezier>(*it);
        boost::shared_ptr<RotoLayer> isLayer = boost::dynamic_pointer_cast<RotoLayer>(*it);
        
        if (isBezier) {
            ctx->select(isBezier,RotoContext::OTHER);
        } else if (isLayer) {
            linkItemsKnobsRecursively(ctx, isLayer);
        }
    }
}

void RotoContext::load(const RotoContextSerialization& obj)
{
    
    assert(QThread::currentThread() == qApp->thread());
    ///no need to lock here, when this is called the main-thread is the only active thread
    
    _imp->autoKeying = obj._autoKeying;
    _imp->featherLink = obj._featherLink;
    _imp->rippleEdit = obj._rippleEdit;
    
    _imp->activated->setAllDimensionsEnabled(false);
    _imp->opacity->setAllDimensionsEnabled(false);
    _imp->feather->setAllDimensionsEnabled(false);
    _imp->featherFallOff->setAllDimensionsEnabled(false);
#ifdef NATRON_ROTO_INVERTIBLE
    _imp->inverted->setAllDimensionsEnabled(false);
#endif

    assert(_imp->layers.size() == 1);
    
    boost::shared_ptr<RotoLayer> baseLayer = _imp->layers.front();
    
    baseLayer->load(obj._baseLayer);
    
    _imp->itemCounters = obj._itemCounters;
    
    for (std::list<std::string>::const_iterator it = obj._selectedItems.begin(); it!=obj._selectedItems.end(); ++it) {
        
        boost::shared_ptr<RotoItem> item = getItemByName(*it);
        
        boost::shared_ptr<Bezier> isBezier = boost::dynamic_pointer_cast<Bezier>(item);
        boost::shared_ptr<RotoLayer> isLayer = boost::dynamic_pointer_cast<RotoLayer>(item);
        if (isBezier) {
            select(isBezier,RotoContext::OTHER);
        } else if (isLayer) {
            linkItemsKnobsRecursively(this, isLayer);
        }
        
    }
    
}

void RotoContext::select(const boost::shared_ptr<RotoItem>& b,RotoContext::SelectionReason reason)
{
    {
        QMutexLocker l(&_imp->rotoContextMutex);
        selectInternal(b);
    }
    emit selectionChanged((int)reason);
}

void RotoContext::select(const std::list<boost::shared_ptr<Bezier> > & beziers,RotoContext::SelectionReason reason)
{
    {
        QMutexLocker l(&_imp->rotoContextMutex);
        for (std::list<boost::shared_ptr<Bezier> >::const_iterator it = beziers.begin(); it!=beziers.end(); ++it) {
            selectInternal(*it);
        }
    }
    emit selectionChanged((int)reason);
}

void RotoContext::select(const std::list<boost::shared_ptr<RotoItem> > & items,RotoContext::SelectionReason reason)
{
    {
        QMutexLocker l(&_imp->rotoContextMutex);
        for (std::list<boost::shared_ptr<RotoItem> >::const_iterator it = items.begin(); it!=items.end(); ++it) {
            selectInternal(*it);
        }
    }
    emit selectionChanged((int)reason);
}

void RotoContext::deselect(const boost::shared_ptr<RotoItem>& b,RotoContext::SelectionReason reason)
{
    {
        QMutexLocker l(&_imp->rotoContextMutex);
        deselectInternal(b);
    }
    emit selectionChanged((int)reason);
    
}

void RotoContext::deselect(const std::list<boost::shared_ptr<Bezier> >& beziers,RotoContext::SelectionReason reason)
{
    {
        QMutexLocker l(&_imp->rotoContextMutex);
        for (std::list<boost::shared_ptr<Bezier> >::const_iterator it = beziers.begin(); it!=beziers.end(); ++it) {
            deselectInternal(*it);
        }
    }
    emit selectionChanged((int)reason);
}

void RotoContext::deselect(const std::list<boost::shared_ptr<RotoItem> >& items,RotoContext::SelectionReason reason)
{
    {
        QMutexLocker l(&_imp->rotoContextMutex);
        for (std::list<boost::shared_ptr<RotoItem> >::const_iterator it = items.begin(); it!=items.end(); ++it) {
            deselectInternal(*it);
        }
    }
    emit selectionChanged((int)reason);
}

void RotoContext::clearSelection(RotoContext::SelectionReason reason)
{
    {
        QMutexLocker l(&_imp->rotoContextMutex);
        while (!_imp->selectedItems.empty()) {
            deselectInternal(_imp->selectedItems.front());
        }
    }
    emit selectionChanged((int)reason);
}

void RotoContext::selectInternal(const boost::shared_ptr<RotoItem>& item)
{
    
    ///only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    
    assert(!_imp->rotoContextMutex.tryLock());
    
    int nbUnlockedBeziers = 0;
    bool foundItem = false;
    for (std::list<boost::shared_ptr<RotoItem> >::iterator it = _imp->selectedItems.begin(); it != _imp->selectedItems.end(); ++it) {
        Bezier* isBezier = dynamic_cast<Bezier*>(it->get());
        if (isBezier && !isBezier->isLockedRecursive()) {
            ++nbUnlockedBeziers;
        }
        if (it->get() == item.get()) {
            foundItem = true;
        }
    }
    
    ///the item is already selected, exit
    if (foundItem) {
        return;
    }
    
    
    
    Bezier* isBezier = dynamic_cast<Bezier*>(item.get());
    RotoLayer* isLayer = dynamic_cast<RotoLayer*>(item.get());
    
    if (isBezier) {
        
        if (!isBezier->isLockedRecursive()) {
            ++nbUnlockedBeziers;
        }
        ///first-off set the context knobs to the value of this bezier
        boost::shared_ptr<KnobI> activated = isBezier->getActivatedKnob();
        boost::shared_ptr<KnobI> feather = isBezier->getFeatherKnob();
        boost::shared_ptr<KnobI> featherFallOff = isBezier->getFeatherFallOffKnob();
        boost::shared_ptr<KnobI> opacity = isBezier->getOpacityKnob();
#ifdef NATRON_ROTO_INVERTIBLE
        boost::shared_ptr<KnobI> inverted = isBezier->getInvertedKnob();
#endif
        boost::shared_ptr<KnobI> colorknob = isBezier->getColorKnob();
        boost::shared_ptr<KnobI> compOp = isBezier->getOperatorKnob();
        
        _imp->activated->clone(activated.get());
        _imp->feather->clone(feather.get());
        _imp->featherFallOff->clone(featherFallOff.get());
        _imp->opacity->clone(opacity.get());
#ifdef NATRON_ROTO_INVERTIBLE
        _imp->inverted->clone(inverted.get());
#endif
        _imp->colorKnob->clone(colorknob.get());
        _imp->compOperator->clone(compOp.get());
        
        ///link this bezier knobs to the context
        activated->slaveTo(0, _imp->activated, 0);
        feather->slaveTo(0, _imp->feather, 0);
        featherFallOff->slaveTo(0, _imp->featherFallOff, 0);
        opacity->slaveTo(0, _imp->opacity, 0);
#ifdef NATRON_ROTO_INVERTIBLE
        inverted->slaveTo(0, _imp->inverted, 0);
#endif
        for (int i = 0; i < colorknob->getDimension();++i) {
            colorknob->slaveTo(i, _imp->colorKnob, i);
        }
        compOp->slaveTo(0, _imp->compOperator, 0);
        
    } else if (isLayer) {
        const RotoItems& children = isLayer->getItems();
        for (RotoItems::const_iterator it = children.begin(); it!=children.end(); ++it) {
            selectInternal(*it);
        }
    }
    
    if (nbUnlockedBeziers > 0) {
        ///enable the knobs
        _imp->activated->setAllDimensionsEnabled(true);
        _imp->opacity->setAllDimensionsEnabled(true);
        _imp->featherFallOff->setAllDimensionsEnabled(true);
        _imp->feather->setAllDimensionsEnabled(true);
#ifdef NATRON_ROTO_INVERTIBLE
        _imp->inverted->setAllDimensionsEnabled(true);
#endif
        _imp->colorKnob->setAllDimensionsEnabled(true);
        _imp->compOperator->setAllDimensionsEnabled(true);
    }
    
    ///if there are multiple selected beziers, notify the gui knobs so they appear like not displaying an accurate value
    ///(maybe black or something)
    if (nbUnlockedBeziers >= 2) {
        _imp->activated->setDirty(true);
        _imp->opacity->setDirty(true);
        _imp->feather->setDirty(true);
        _imp->featherFallOff->setDirty(true);
#ifdef NATRON_ROTO_INVERTIBLE
        _imp->inverted->setDirty(true);
#endif
        _imp->colorKnob->setDirty(true);
        _imp->compOperator->setDirty(true);
    }
    
    
    
    _imp->selectedItems.push_back(item);
    
    
}

void RotoContext::deselectInternal(boost::shared_ptr<RotoItem> b)
{
    ///only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    
    assert(!_imp->rotoContextMutex.tryLock());
    
    std::list<boost::shared_ptr<RotoItem> >::iterator foundSelected = std::find(_imp->selectedItems.begin(),_imp->selectedItems.end(),b);
    
    ///if the item is not selected, exit
    if (foundSelected == _imp->selectedItems.end()) {
        return;
    }
    
    _imp->selectedItems.erase(foundSelected);
    
    int nbBeziersUnLockedBezier = 0;
    for (std::list<boost::shared_ptr<RotoItem> >::iterator it = _imp->selectedItems.begin(); it != _imp->selectedItems.end(); ++it) {
        Bezier* isBezier = dynamic_cast<Bezier*>(it->get());
        if (isBezier && !isBezier->isLockedRecursive()) {
            ++nbBeziersUnLockedBezier;
        }
    }
    bool notDirty = nbBeziersUnLockedBezier <= 1;
    
    Bezier* isBezier = dynamic_cast<Bezier*>(b.get());
    RotoLayer* isLayer = dynamic_cast<RotoLayer*>(b.get());
    if (isBezier) {
  
        ///first-off set the context knobs to the value of this bezier
        boost::shared_ptr<KnobI> activated = isBezier->getActivatedKnob();
        boost::shared_ptr<KnobI> feather = isBezier->getFeatherKnob();
        boost::shared_ptr<KnobI> featherFallOff = isBezier->getFeatherFallOffKnob();
        boost::shared_ptr<KnobI> opacity = isBezier->getOpacityKnob();
#ifdef NATRON_ROTO_INVERTIBLE
        boost::shared_ptr<KnobI> inverted = isBezier->getInvertedKnob();
#endif
        boost::shared_ptr<KnobI> colorknob = isBezier->getColorKnob();
        boost::shared_ptr<KnobI> compOp = isBezier->getOperatorKnob();
        
        activated->unSlave(0,notDirty);
        feather->unSlave(0,notDirty);
        featherFallOff->unSlave(0,notDirty);
        opacity->unSlave(0,notDirty);
#ifdef NATRON_ROTO_INVERTIBLE
        inverted->unSlave(0,notDirty);
#endif
        for (int i = 0;i < colorknob->getDimension();++i) {
            colorknob->unSlave(i, notDirty);
        }
        compOp->unSlave(0, notDirty);
        
    } else if (isLayer) {
        const RotoItems& children = isLayer->getItems();
        for (RotoItems::const_iterator it = children.begin(); it!=children.end(); ++it) {
            deselectInternal(*it);
        }
    }
    
    if (notDirty) {
        _imp->activated->setDirty(false);
        _imp->opacity->setDirty(false);
        _imp->feather->setDirty(false);
        _imp->featherFallOff->setDirty(false);
#ifdef NATRON_ROTO_INVERTIBLE
        _imp->inverted->setDirty(false);
#endif
        _imp->compOperator->setDirty(false);
        _imp->colorKnob->setDirty(false);
    }
    
    ///if the selected beziers count reaches 0 notify the gui knobs so they appear not enabled
    if (nbBeziersUnLockedBezier == 0) {
        _imp->activated->setAllDimensionsEnabled(false);
        _imp->opacity->setAllDimensionsEnabled(false);
        _imp->featherFallOff->setAllDimensionsEnabled(false);
        _imp->feather->setAllDimensionsEnabled(false);
#ifdef NATRON_ROTO_INVERTIBLE
        _imp->inverted->setAllDimensionsEnabled(false);
#endif
        _imp->compOperator->setAllDimensionsEnabled(false);
        _imp->colorKnob->setAllDimensionsEnabled(false);
    }
    
    
}

void RotoContext::setLastItemLocked(const boost::shared_ptr<RotoItem> &item)
{
    {
        QMutexLocker l(&_imp->rotoContextMutex);
        _imp->lastLockedItem = item;
    }
    emit itemLockedChanged();
}

boost::shared_ptr<RotoItem> RotoContext::getLastItemLocked() const
{
    QMutexLocker l(&_imp->rotoContextMutex);
    return _imp->lastLockedItem;
}

static void addOrRemoveKeyRecursively(RotoLayer* isLayer,int time,bool add)
{
    const RotoItems& items= isLayer->getItems();
    for (RotoItems::const_iterator it2 = items.begin(); it2 != items.end(); ++it2) {
        RotoLayer* layer = dynamic_cast<RotoLayer*>(it2->get());
        Bezier* isBezier = dynamic_cast<Bezier*>(it2->get());
        if (isBezier) {
            if (add) {
                isBezier->setKeyframe(time);
            } else if (layer) {
                isBezier->removeKeyframe(time);
            }
        } else if (isLayer) {
            addOrRemoveKeyRecursively(layer,time, add);
        }
    }
}

void RotoContext::setKeyframeOnSelectedCurves()
{
    ///only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    
    int time = getTimelineCurrentTime();
    
    QMutexLocker l(&_imp->rotoContextMutex);
    for (std::list<boost::shared_ptr<RotoItem> >::iterator it = _imp->selectedItems.begin(); it!=_imp->selectedItems.end(); ++it) {
        RotoLayer* isLayer = dynamic_cast<RotoLayer*>(it->get());
        Bezier* isBezier = dynamic_cast<Bezier*>(it->get());
        if (isBezier) {
            isBezier->setKeyframe(time);
        } else if (isLayer) {
            addOrRemoveKeyRecursively(isLayer,time, true);
        }
    }
}

void RotoContext::removeKeyframeOnSelectedCurves()
{
    ///only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    
    int time = getTimelineCurrentTime();
    QMutexLocker l(&_imp->rotoContextMutex);
    for (std::list<boost::shared_ptr<RotoItem> >::iterator it = _imp->selectedItems.begin(); it!=_imp->selectedItems.end(); ++it) {
        RotoLayer* isLayer = dynamic_cast<RotoLayer*>(it->get());
        Bezier* isBezier = dynamic_cast<Bezier*>(it->get());
        if (isBezier) {
            isBezier->removeKeyframe(time);
        } else if (isLayer) {
            addOrRemoveKeyRecursively(isLayer,time, false);
        }
    }
}

static void findOutNearestKeyframeRecursively(RotoLayer* layer,bool previous,int time,int* nearest)
{
    const RotoItems& items = layer->getItems();
    for (RotoItems::const_iterator it = items.begin(); it!=items.end(); ++it) {
        RotoLayer* layer = dynamic_cast<RotoLayer*>(it->get());
        Bezier* isBezier = dynamic_cast<Bezier*>(it->get());
        if (isBezier) {
            if (previous) {
                int t = isBezier->getPreviousKeyframeTime(time);
                if (t != INT_MIN && t > *nearest) {
                    *nearest = t;
                }
            } else if (layer) {
                int t = isBezier->getNextKeyframeTime(time);
                if (t != INT_MAX && t < *nearest) {
                    *nearest = t;
                }
            }
        } else {
            findOutNearestKeyframeRecursively(layer, previous,time,nearest);
        }
    }
}

void RotoContext::goToPreviousKeyframe()
{
    
    ///only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    
    int time = getTimelineCurrentTime();
    int minimum = INT_MIN;
    
    {
        QMutexLocker l(&_imp->rotoContextMutex);
        for (std::list<boost::shared_ptr<RotoItem> >::iterator it = _imp->selectedItems.begin(); it!=_imp->selectedItems.end(); ++it) {
            RotoLayer* layer = dynamic_cast<RotoLayer*>(it->get());
            Bezier* isBezier = dynamic_cast<Bezier*>(it->get());
            if (isBezier) {
                int t = isBezier->getPreviousKeyframeTime(time);
                if (t != INT_MIN && t > minimum) {
                    minimum = t;
                }
            } else {
                findOutNearestKeyframeRecursively(layer, true,time,&minimum);
            }
        }
    }
    
    if (minimum != INT_MIN) {
        _imp->node->getApp()->getTimeLine()->seekFrame(minimum, NULL);
    }
}

void RotoContext::goToNextKeyframe()
{
    ///only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    
    int time = getTimelineCurrentTime();
    
    int maximum = INT_MAX;
    
    {
        QMutexLocker l(&_imp->rotoContextMutex);
        for (std::list<boost::shared_ptr<RotoItem> >::iterator it = _imp->selectedItems.begin(); it!=_imp->selectedItems.end(); ++it) {
            RotoLayer* isLayer = dynamic_cast<RotoLayer*>(it->get());
            Bezier* isBezier = dynamic_cast<Bezier*>(it->get());
            if (isBezier) {
                int t = isBezier->getNextKeyframeTime(time);
                if (t != INT_MAX && t < maximum) {
                    maximum = t;
                }
            } else {
                findOutNearestKeyframeRecursively(isLayer, false,time,&maximum);
            }
            
        }
    
    }
    if (maximum != INT_MAX) {
        _imp->node->getApp()->getTimeLine()->seekFrame(maximum, NULL);
    }
}

static void appendToSelectedCurvesRecursively(std::list< boost::shared_ptr<Bezier> > * curves,RotoLayer* isLayer,int time,bool onlyActives)
{
    RotoItems items = isLayer->getItems_mt_safe();
    for (RotoItems::const_iterator it = items.begin(); it!=items.end(); ++it) {
        RotoLayer* layer = dynamic_cast<RotoLayer*>(it->get());
        boost::shared_ptr<Bezier> isBezier = boost::dynamic_pointer_cast<Bezier>(*it);
        if (isBezier) {
            if (!onlyActives || isBezier->isActivated(time))
            {
                curves->push_back(isBezier);
            }
            
        } else if (layer && layer->isGloballyActivated()) {
            appendToSelectedCurvesRecursively(curves, layer,time,onlyActives);
        }
    }
}

const std::list< boost::shared_ptr<RotoItem> >& RotoContext::getSelectedItems() const
{
    ///only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    QMutexLocker l(&_imp->rotoContextMutex);
    return _imp->selectedItems;
}

std::list< boost::shared_ptr<Bezier> > RotoContext::getSelectedCurves() const
{
    ///only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    std::list< boost::shared_ptr<Bezier> >   ret;
    int time = getTimelineCurrentTime();
    {
        QMutexLocker l(&_imp->rotoContextMutex);
        for (std::list<boost::shared_ptr<RotoItem> >::iterator it = _imp->selectedItems.begin(); it!=_imp->selectedItems.end(); ++it) {
            RotoLayer* isLayer = dynamic_cast<RotoLayer*>(it->get());
            boost::shared_ptr<Bezier> isBezier = boost::dynamic_pointer_cast<Bezier>(*it);
            if (isBezier) {
                ret.push_back(isBezier);
            } else {
                appendToSelectedCurvesRecursively(&ret, isLayer,time,false);
            }
            
        }

    }
    
    return ret;
}


std::list< boost::shared_ptr<Bezier> > RotoContext::getCurvesByRenderOrder() const
{
    std::list< boost::shared_ptr<Bezier> > ret;
    ///Note this might not be the timeline's current frame if this is a writer.
    int time = _imp->node->getLiveInstance()->getCurrentFrameRecursive();
    {
        QMutexLocker l(&_imp->rotoContextMutex);
        if (!_imp->layers.empty()) {
            appendToSelectedCurvesRecursively(&ret, _imp->layers.front().get(),time,true);
        }
    }
    return ret;
}

boost::shared_ptr<RotoLayer> RotoContext::getLayerByName(const std::string& n) const
{
    QMutexLocker l(&_imp->rotoContextMutex);
    for (std::list<boost::shared_ptr<RotoLayer> >::const_iterator it = _imp->layers.begin(); it!=_imp->layers.end(); ++it) {
        if ((*it)->getName_mt_safe() == n) {
            return *it;
        }
    }
    return boost::shared_ptr<RotoLayer>();
}

static void findItemRecursively(const std::string& n,const boost::shared_ptr<RotoLayer>& layer,boost::shared_ptr<RotoItem>* ret)
{
    if (layer->getName_mt_safe() == n) {
        *ret = boost::dynamic_pointer_cast<RotoItem>(layer);
    } else {
        const RotoItems& items = layer->getItems();
        for (RotoItems::const_iterator it2 = items.begin(); it2!=items.end(); ++it2) {
            boost::shared_ptr<RotoLayer> isLayer = boost::dynamic_pointer_cast<RotoLayer>(*it2);
            if ((*it2)->getName_mt_safe() == n) {
                *ret = *it2;
                return;
            } else if (isLayer) {
                findItemRecursively(n, isLayer, ret);
            }

        }
    }
}

boost::shared_ptr<RotoItem> RotoContext::getItemByName(const std::string& n) const
{
    boost::shared_ptr<RotoItem> ret;
    QMutexLocker l(&_imp->rotoContextMutex);
    for (std::list<boost::shared_ptr<RotoLayer> >::const_iterator it = _imp->layers.begin(); it!=_imp->layers.end(); ++it) {
        findItemRecursively(n, *it, &ret);
    }
    return ret;
}

RotoLayer* RotoContext::getDeepestSelectedLayer() const
{
    QMutexLocker l(&_imp->rotoContextMutex);
    return findDeepestSelectedLayer();
}


RotoLayer* RotoContext::findDeepestSelectedLayer() const
{
    assert(!_imp->rotoContextMutex.tryLock());
    
    int minLevel = -1;
    RotoLayer* minLayer = 0;
    for (std::list< boost::shared_ptr<RotoItem> >::const_iterator it = _imp->selectedItems.begin();
         it != _imp->selectedItems.end(); ++it) {
        int lvl = (*it)->getHierarchyLevel();
        if (lvl > minLevel) {
            RotoLayer* isLayer = dynamic_cast<RotoLayer*>(it->get());
            if (isLayer) {
                minLayer = isLayer;
            } else {
                minLayer = (*it)->getParentLayer();
            }
            minLevel = lvl;
        }
    }
    
    return minLayer;
}

void RotoContext::evaluateChange()
{
    _imp->incrementRotoAge();
    _imp->node->getLiveInstance()->evaluate_public(NULL, true,Natron::USER_EDITED);
}

U64 RotoContext::getAge()
{
    QMutexLocker l(&_imp->rotoContextMutex);
    return _imp->age;
}

void RotoContext::onItemLockedChanged(RotoItem* item)
{
    assert(item);
    ///refresh knobs
    int nbBeziersUnLockedBezier = 0;
    
    {
        QMutexLocker l(&_imp->rotoContextMutex);
        
        for (std::list<boost::shared_ptr<RotoItem> >::iterator it = _imp->selectedItems.begin(); it != _imp->selectedItems.end(); ++it) {
            Bezier* isBezier = dynamic_cast<Bezier*>(it->get());
            if (isBezier && !isBezier->isLockedRecursive()) {
                ++nbBeziersUnLockedBezier;
            }
        }
        
    }
    bool dirty = nbBeziersUnLockedBezier > 1;
    bool enabled = nbBeziersUnLockedBezier > 0;
    
    _imp->activated->setDirty(dirty);
    _imp->opacity->setDirty(dirty);
    _imp->feather->setDirty(dirty);
    _imp->featherFallOff->setDirty(dirty);
#ifdef NATRON_ROTO_INVERTIBLE
    _imp->inverted->setDirty(dirty);
#endif

    _imp->activated->setAllDimensionsEnabled(enabled);
    _imp->opacity->setAllDimensionsEnabled(enabled);
    _imp->featherFallOff->setAllDimensionsEnabled(enabled);
    _imp->feather->setAllDimensionsEnabled(enabled);
#ifdef NATRON_ROTO_INVERTIBLE
    _imp->inverted->setAllDimensionsEnabled(enabled);
#endif

    emit itemLockedChanged();
}

std::string RotoContext::getRotoNodeName() const
{
    return _imp->node->getName_mt_safe();
}

void RotoContext::emitRefreshViewerOverlays()
{
    emit refreshViewerOverlays();
}

void RotoContext::getBeziersKeyframeTimes(std::list<int> *times) const
{
    std::list< boost::shared_ptr<Bezier> > splines = getCurvesByRenderOrder();
    for (std::list< boost::shared_ptr<Bezier> > ::iterator it = splines.begin(); it!=splines.end(); ++it) {
        std::set<int> splineKeys;
        (*it)->getKeyframeTimes(&splineKeys);
        for (std::set<int>::iterator it2 = splineKeys.begin(); it2!=splineKeys.end(); ++it2) {
            times->push_back(*it2);
        }
    }
}

static void adjustToPointToScale(unsigned int mipmapLevel,double &x,double &y)
{
    if (mipmapLevel != 0) {
        int pot = (1 << mipmapLevel);
        x /= pot;
        y /= pot;
    }
}

template <typename PIX,int maxValue>
void convertCairoImageToNatronImage(cairo_surface_t* cairoImg,Natron::Image* image,const RectI& pixelRod)
{
    unsigned char* cdata = cairo_image_surface_get_data(cairoImg);
    unsigned char* srcPix = cdata;
    int stride = cairo_image_surface_get_stride(cairoImg);
    
    int comps = (int)image->getComponentsCount();
    for (int y = 0; y < pixelRod.height(); ++y, srcPix += stride) {
        
        PIX* dstPix = (PIX*)image->pixelAt(pixelRod.x1, pixelRod.y1 + y);
        assert(dstPix);
        
        for (int x = 0; x < pixelRod.width(); ++x) {
            if (comps == 1) {
                dstPix[x] = PIX((float)srcPix[x] / 255.f) * maxValue;;
            } else {
                int offset = 0;
                if (comps == 4) {
                    offset = 1; //< cairo's format is ARGB (that is BGRA when interpreted as bytes)
                    dstPix[x * 4 + 3] = PIX((float)srcPix[x * 4 + 3] / 255.f) * maxValue;
                }
                dstPix[x * comps + 0] = PIX((float)srcPix[x * comps + 2] / 255.f) * maxValue;
                dstPix[x * comps + 1] = PIX((float)srcPix[x * comps + 1] / 255.f) * maxValue;
                dstPix[x * comps + 2] = PIX((float)srcPix[x * comps + 0] / 255.f) * maxValue;
            }
        }
    }
}

boost::shared_ptr<Natron::Image> RotoContext::renderMask(const RectI& roi,Natron::ImageComponents components,
                                                         U64 nodeHash,U64 ageToRender,const RectI& nodeRoD,SequenceTime time,
                                            Natron::ImageBitDepth depth,int view,unsigned int mipmapLevel,bool byPassCache)
{
    

    std::list< boost::shared_ptr<Bezier> > splines = getCurvesByRenderOrder();
    
    ///compute an enhanced hash different from the one of the node in order to differentiate within the cache
    ///the output image of the roto node and the mask image.
    Hash64 hash;
    hash.append(nodeHash);
    hash.append(ageToRender);
    hash.computeHash();

    Natron::ImageKey key = Natron::Image::makeKey(hash.value(), time, mipmapLevel, view);
    
    ///If the last rendered image  was with a different hash key (i.e a parameter changed or an input changed)
    ///just remove the old image from the cache to recycle memory.
    boost::shared_ptr<Image> lastRenderedImage;
    U64 lastRenderHash;
    {
        QMutexLocker l(&_imp->lastRenderArgsMutex);
        lastRenderHash = _imp->lastRenderHash;
        lastRenderedImage = _imp->lastRenderedImage;
        
    }
    
    if (lastRenderedImage &&
        (lastRenderHash != hash.value())) {
        ///try to obtain the lock for the last rendered image as another thread might still rely on it in the cache
        Natron::OutputImageLocker imgLocker(_imp->node,lastRenderedImage);
        ///once we got it remove it from the cache
        appPTR->removeAllImagesFromCacheWithMatchingKey(lastRenderHash);
        {
            QMutexLocker l(&_imp->lastRenderArgsMutex);
            _imp->lastRenderedImage.reset();
        }
        
    }
    
    
    boost::shared_ptr<const Natron::ImageParams> params;
    boost::shared_ptr<Natron::Image> image;
    
    bool cached = Natron::getImageFromCache(key, &params, &image);
   
    if (cached && (image->getBitDepth() != depth || image->getComponents() != components)) {
        cached = false;
        image.reset();
        params.reset();
    }
    ///If there's only 1 shape to render and this shape is inverted, initialize the image
    ///with the invert instead of the default fill value to speed up rendering
    if (cached) {
        if (cached && byPassCache) {
            ///If we want to by-pass the cache, we will just zero-out the bitmap of the image, so
            ///we're sure renderRoIInternal will compute the whole image again.
            ///We must use the cache facility anyway because we rely on it for caching the results
            ///of actions which is necessary to avoid recursive actions.
            image->clearBitmap();
        }
        
    } else {
        
        params = Natron::Image::makeParams(0, nodeRoD,mipmapLevel,false,
                                           components,
                                           depth,
                                           -1, time,
                                           std::map<int, std::vector<RangeD> >());
        
        cached = appPTR->getImageOrCreate(key, params, &image);
        if (!image) {
            std::stringstream ss;
            ss << "Failed to allocate an image of ";
            ss << printAsRAM(params->getElementsCount() * sizeof(Natron::Image::data_t)).toStdString();
            Natron::errorDialog(QObject::tr("Out of memory").toStdString(),ss.str());
            return image;
        }

        if (cached && byPassCache) {
            image->clearBitmap();
        }
    }
    
    ///////////////////////////////////Render internal
    RectI pixelRod = params->getPixelRoD();
    RectI clippedRoI;
    roi.intersect(pixelRod, &clippedRoI);
 
    cairo_format_t cairoImgFormat;
    switch (components) {
        case Natron::ImageComponentAlpha:
            cairoImgFormat = CAIRO_FORMAT_A8;
            break;
        case Natron::ImageComponentRGB:
            cairoImgFormat = CAIRO_FORMAT_RGB24;
            break;
        case Natron::ImageComponentRGBA:
            cairoImgFormat = CAIRO_FORMAT_ARGB32;
            break;
        default:
            cairoImgFormat = CAIRO_FORMAT_A8;
            break;
    }
    
    ////Allocate the cairo temporary buffer
    cairo_surface_t* cairoImg = cairo_image_surface_create(cairoImgFormat, pixelRod.width(), pixelRod.height());
    cairo_surface_set_device_offset(cairoImg, -pixelRod.x1, -pixelRod.y1);
    if (cairo_surface_status(cairoImg) != CAIRO_STATUS_SUCCESS) {
        appPTR->removeFromNodeCache(image);
        return image;
    }
    cairo_t* cr = cairo_create(cairoImg);
    cairo_set_fill_rule(cr, CAIRO_FILL_RULE_EVEN_ODD);
    
    ///We could also propose the user to render a mask to SVG
    _imp->renderInternal(cr, cairoImg, splines,mipmapLevel,time);
   
    switch (depth) {
        case Natron::IMAGE_FLOAT:
            convertCairoImageToNatronImage<float, 1>(cairoImg, image.get(), pixelRod);
            break;
        case Natron::IMAGE_BYTE:
            convertCairoImageToNatronImage<unsigned char, 255>(cairoImg, image.get(), pixelRod);
            break;
        case Natron::IMAGE_SHORT:
            convertCairoImageToNatronImage<unsigned short, 65535>(cairoImg, image.get(), pixelRod);
            break;
        default:
            assert(false);
            break;
    }
    
    cairo_destroy(cr);
    ////Free the buffer used by Cairo
    cairo_surface_destroy(cairoImg);


    ////////////////////////////////////
    if(_imp->node->aborted()){
        //if render was aborted, remove the frame from the cache as it contains only garbage
        appPTR->removeFromNodeCache(image);
    } else {
         image->markForRendered(clippedRoI);
    }
    
    {
        QMutexLocker l(&_imp->lastRenderArgsMutex);
        _imp->lastRenderHash = hash.value();
        _imp->lastRenderedImage = image;
    }
    
    return image;
}

void
RotoContextPrivate::renderInternal(cairo_t* cr,
                                   cairo_surface_t* cairoImg,
                                   const std::list< boost::shared_ptr<Bezier> >& splines,
                                   unsigned int mipmapLevel,
                                   int time)
{
    // these Roto shapes must be rendered WITHOUT antialias, or the junction between the inner
    // polygon and the feather zone will have artifacts. This is partly due to the fact that cairo
    // meshes are not antialiased.
    // Use a default feather distance of 1 pixel instead!
    // UPDATE: unfortunately, this produces less artifacts, but there are still some remaining (use opacity=0.5 to test)
    // maybe the inner polygon should be made of mesh patterns too?
    cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
    for (std::list<boost::shared_ptr<Bezier> >::const_iterator it2 = splines.begin(); it2!=splines.end(); ++it2) {
        ///render the bezier only if finished (closed) and activated
        if (!(*it2)->isCurveFinished() || !(*it2)->isActivated(time)) {
            continue;
        }


        double fallOff = (*it2)->getFeatherFallOff(time);
        double fallOffInverse = 1. / fallOff;
        double featherDist = (*it2)->getFeatherDistance(time);
        double opacity = (*it2)->getOpacity(time);
#ifdef NATRON_ROTO_INVERTIBLE
        bool inverted = (*it2)->getInverted(time);
#else
        const bool inverted = false;
#endif
        int operatorIndex = (*it2)->getCompositingOperator(time);
        double shapeColor[3];
        (*it2)->getColor(time, shapeColor);

        cairo_set_operator(cr, (cairo_operator_t)operatorIndex);

        BezierCPs cps = inverted ? (*it2)->getFeatherPoints_mt_safe() : (*it2)->getControlPoints_mt_safe();
        BezierCPs fps = inverted ? (*it2)->getControlPoints_mt_safe() : (*it2)->getFeatherPoints_mt_safe();

        assert(cps.size() == fps.size());

        if (cps.empty()) {
            continue;
        }

        cairo_new_path(cr);
#ifdef NATRON_ROTO_INVERTIBLE
#pragma message WARN("doesn't work! the image should be infinite for this to work!")
        // Doesn't work! the image should be infinite for this to work!
        // Or at least it should contain the Union of the source RoDs.
        // Here, it only contains the boinding box of the Bezier.
        // If there's a transform after the roto node, a black border will appear.
        // The only solution would be to have a color parameter which specifies how on image is outside of its RoD.
        // Unfortunately, the OFX definition is: "it is black and transparent"

        ///If inverted, draw an inverted rectangle on all the image first
        if (inverted) {
            double xOffset, yOffset;
            cairo_surface_get_device_offset(cairoImg, &xOffset, &yOffset);
            int width = cairo_image_surface_get_width(cairoImg);
            int height = cairo_image_surface_get_height(cairoImg);

            cairo_move_to(cr, -xOffset, -yOffset);
            cairo_line_to(cr, -xOffset + width, -yOffset);
            cairo_line_to(cr, -xOffset + width, -yOffset + height);
            cairo_line_to(cr, -xOffset, -yOffset + height);
            cairo_line_to(cr, -xOffset, -yOffset);

        }
#endif

        ////Define the feather edge pattern
        cairo_pattern_t* mesh = cairo_pattern_create_mesh();
        if (cairo_pattern_status(mesh) != CAIRO_STATUS_SUCCESS) {
            cairo_pattern_destroy(mesh);
            continue;
        }

#ifdef NATRON_WHY_USE_TWICE_THE_SAME_CODE_COPY_PASTE_IS_UGLY
#error "We don't need twice the code here, with twice the bugs..."
        if (featherDist != 0) {
#endif
            ///Adjust the feather distance so it takes the mipmap level into account
            if (mipmapLevel != 0) {
                featherDist /= (1 << mipmapLevel);
            }

            ///here is the polygon of the feather bezier
            ///This is used only if the feather distance is different of 0 and the feather points equal
            ///the control points in order to still be able to apply the feather distance.
            std::list<Point> featherPolygon;
            std::list<Point> bezierPolygon;
            std::vector<double> multiples,constants;
            RectD featherPolyBBox(INT_MAX,INT_MAX,INT_MIN,INT_MIN);

            (*it2)->evaluateFeatherPointsAtTime_DeCasteljau(time,mipmapLevel, 50, &featherPolygon,true,&featherPolyBBox);
            (*it2)->evaluateAtTime_DeCasteljau(time, mipmapLevel, 50, &bezierPolygon);


            assert(!featherPolygon.empty());

            multiples.resize(featherPolygon.size());
            constants.resize(featherPolygon.size());
            Bezier::precomputePointInPolygonTables(featherPolygon, &constants, &multiples);


            std::list<Point> featherContour;

            std::list<Point>::iterator cur = featherPolygon.begin();
            std::list<Point>::iterator next = cur;
            ++next;
            std::list<Point>::iterator prev = featherPolygon.end();
            --prev;
            std::list<Point>::iterator bezIT = bezierPolygon.begin();
            std::list<Point>::iterator prevBez = bezierPolygon.end();
            --prevBez;
            double absFeatherDist = std::abs(featherDist);

            Point p1 = *cur;
            double norm = sqrt((next->x - prev->x) * (next->x - prev->x) + (next->y - prev->y) * (next->y - prev->y));
            assert(norm != 0);
            double dx = - ((next->y - prev->y) / norm);
            double dy = ((next->x - prev->x) / norm);
            p1.x = cur->x + dx;
            p1.y = cur->y + dy;


            bool inside = Bezier::pointInPolygon(p1, featherPolygon, constants, multiples, featherPolyBBox);
            if ((!inside && featherDist < 0) || (inside && featherDist > 0)) {
                p1.x = cur->x - dx * absFeatherDist;
                p1.y = cur->y - dy * absFeatherDist;
            } else {
                p1.x = cur->x + dx * absFeatherDist;
                p1.y = cur->y + dy * absFeatherDist;
            }

            Point origin = p1;
            featherContour.push_back(p1);

            ++prev; ++next; ++cur; ++bezIT; ++prevBez;

            for (;;++prev,++cur,++next,++bezIT,++prevBez) { // for each point in polygon
                if (next == featherPolygon.end()) {
                    next = featherPolygon.begin();
                }
                if (prev == featherPolygon.end()) {
                    prev = featherPolygon.begin();
                }
                if (bezIT == bezierPolygon.end()) {
                    bezIT = bezierPolygon.begin();
                }
                if (prevBez == bezierPolygon.end()) {
                    prevBez = bezierPolygon.begin();
                }
                bool mustStop = false;
                if (cur == featherPolygon.end()) {
                    mustStop = true;
                    cur = featherPolygon.begin();
                }

                ///skip it
                if (cur->x == prev->x && cur->y == prev->y) {
                    continue;
                }

                Point p0, p0p1, p1p0, p2, p2p3, p3p2, p3;
                p0.x = prevBez->x;
                p0.y = prevBez->y;
                p3.x = bezIT->x;
                p3.y = bezIT->y;

                if (!mustStop) {
                    norm = sqrt((next->x - prev->x) * (next->x - prev->x) + (next->y - prev->y) * (next->y - prev->y));
                    assert(norm != 0);
                    dx = - ((next->y - prev->y) / norm);
                    dy = ((next->x - prev->x) / norm);
                    p2.x = cur->x + dx;
                    p2.y = cur->y + dy;

                    inside = Bezier::pointInPolygon(p2, featherPolygon, constants, multiples, featherPolyBBox);
                    if ((!inside && featherDist < 0) || (inside && featherDist > 0)) {
                        p2.x = cur->x - dx * absFeatherDist;
                        p2.y = cur->y - dy * absFeatherDist;
                    } else {
                        p2.x = cur->x + dx * absFeatherDist;
                        p2.y = cur->y + dy * absFeatherDist;
                    }
                } else {
                    p2 = origin;
                }
                featherContour.push_back(p2);

                ///linear interpolation
                p0p1.x = (p0.x * fallOff * 2. + fallOffInverse * p1.x) / (fallOff * 2. + fallOffInverse);
                p0p1.y = (p0.y * fallOff * 2. + fallOffInverse * p1.y) / (fallOff * 2. + fallOffInverse);
                p1p0.x = (p0.x * fallOff + 2. * fallOffInverse * p1.x) / (fallOff + 2. * fallOffInverse);
                p1p0.y = (p0.y * fallOff + 2. * fallOffInverse * p1.y) / (fallOff + 2. * fallOffInverse);

                p2p3.x = (p3.x * fallOff + 2. * fallOffInverse * p2.x) / (fallOff + 2. * fallOffInverse);
                p2p3.y = (p3.y * fallOff + 2. * fallOffInverse * p2.y) / (fallOff + 2. * fallOffInverse);
                p3p2.x = (p3.x * fallOff * 2. + fallOffInverse * p2.x) / (fallOff * 2. + fallOffInverse);
                p3p2.y = (p3.y * fallOff * 2. + fallOffInverse * p2.y) / (fallOff * 2. + fallOffInverse);


                ///move to the initial point
                cairo_mesh_pattern_begin_patch(mesh);
                cairo_mesh_pattern_move_to(mesh, p0.x, p0.y);
                cairo_mesh_pattern_curve_to(mesh, p0p1.x, p0p1.y, p1p0.x, p1p0.y, p1.x, p1.y);
                cairo_mesh_pattern_line_to(mesh, p2.x, p2.y);
                cairo_mesh_pattern_curve_to(mesh, p2p3.x, p2p3.y, p3p2.x, p3p2.y, p3.x, p3.y);
                cairo_mesh_pattern_line_to(mesh, p0.x, p0.y);
                ///Set the 4 corners color
                ///inner is full color
#pragma message WARN("the two sqrt below are probably due to a cairo bug, please read comments")
                //the two sqrt below are probably due to a cairo bug.
                // to check wether the bug is present, make any shape with a very large feather and set
                // opacity to 0.5. Then, zoom on the polygon border to check if the intensity is continuous
                // and approximately equal to 0.5
                cairo_mesh_pattern_set_corner_color_rgba(mesh, 0, shapeColor[0], shapeColor[1], shapeColor[2],
                                                         std::sqrt(inverted ? 1. - opacity : opacity));
                ///outter is faded
                cairo_mesh_pattern_set_corner_color_rgba(mesh, 1, shapeColor[0], shapeColor[1], shapeColor[2],
                                                         inverted ? 1. : 0.);
                cairo_mesh_pattern_set_corner_color_rgba(mesh, 2, shapeColor[0], shapeColor[1], shapeColor[2],
                                                         inverted ? 1. : 0.);
                ///inner is full color
                cairo_mesh_pattern_set_corner_color_rgba(mesh, 3, shapeColor[0], shapeColor[1], shapeColor[2],
                                                         std::sqrt(inverted ? 1. - opacity : opacity));
                assert(cairo_pattern_status(mesh) == CAIRO_STATUS_SUCCESS);

                cairo_mesh_pattern_end_patch(mesh);

                if (mustStop) {
                    break;
                }

                p1 = p2;
            }  // for each point in polygon

            int loopCount;
            if (!inverted) {
                // strangely, the above-mentioned cairo bug doesn't affect this function
                cairo_set_source_rgba(cr, shapeColor[0], shapeColor[1], shapeColor[2], opacity);
                renderInternalShape(time,mipmapLevel,cr,cps);
                loopCount = 1;
            } else {
                loopCount = 2;
            }
            for (int i = 0; i < loopCount ;++i) {
                std::list<Point>::iterator featherContourIT = featherContour.begin();
                cairo_move_to(cr, featherContourIT->x, featherContourIT->y);
                ++featherContourIT;
                while (featherContourIT != featherContour.end()) {
                    cairo_line_to(cr, featherContourIT->x, featherContourIT->y);
                    ++featherContourIT;
                }
                if (i == 0 && inverted) {
//                    if (cairo_get_antialias(cr) != CAIRO_ANTIALIAS_NONE ) {
//                        cairo_fill_preserve(cr);
//                        // These line properties make for a nicer looking polygon mesh
//                        cairo_set_line_join(cr, CAIRO_LINE_JOIN_BEVEL);
//                        cairo_set_line_cap(cr, CAIRO_LINE_CAP_BUTT);
//                        // Comment out the following call to cairo_set_line width
//                        // since the hard-coded width value of 1.0 is not appropriate
//                        // for fills of small areas. Instead, use the line width that
//                        // has already been set by the user via the above call of
//                        // poly_line which in turn calls set_current_context which in
//                        // turn calls cairo_set_line_width for the user-specified
//                        // width.
//                        // cairo_set_line_width(cr, 1.0);
//                        cairo_stroke(cr);
//                    } else {
                    cairo_fill(cr);
//                    }
                }
            }
            applyAndDestroyMask(cr, mesh);
#ifdef NATRON_WHY_USE_TWICE_THE_SAME_CODE_COPY_PASTE_IS_UGLY
        } else {
            ////1st pass, fill the internal bezier
            ////When inverted it will be drawn using the invert of the opacity because we're in EVEN/ODD polygon fill mode
            cairo_set_source_rgba(cr, shapeColor[0], shapeColor[1],shapeColor[2],opacity);
            renderInternalShape(time,mipmapLevel,cr,cps);
            ///This is a vector of feather points that we compute during
            ///the first pass to avoid recompute them when we do the actual
            ///path rendering.
            ///The points are interpreted as a triplet <prev right point,cur left point,cur>
            std::vector<Point> preComputedFeatherPoints(cps.size() * 3);
            std::vector<Point>::iterator preFillIt = preComputedFeatherPoints.begin();

            BezierCPs::iterator point = cps.begin();
            BezierCPs::iterator nextPoint = point;
            ++nextPoint;

            BezierCPs::iterator fpoint = fps.begin();
            BezierCPs::iterator nextFPoint = fpoint;
            ++nextFPoint;


            while (point != cps.end()) {

                if (nextPoint == cps.end()) {
                    nextPoint = cps.begin();
                    nextFPoint = fps.begin();
                }

                Point p0,p1,p2,p3,p0p1,p1p0,p0Right,p1Right,p2p3,p3p2,p2Left,p3Left;
                (*point)->getPositionAtTime(time, &p0.x, &p0.y);
                adjustToPointToScale(mipmapLevel,p0.x,p0.y);

                (*point)->getRightBezierPointAtTime(time, &p0Right.x, &p0Right.y);
                adjustToPointToScale(mipmapLevel, p0Right.x, p0Right.y);

                (*fpoint)->getRightBezierPointAtTime(time, &p1Right.x, &p1Right.y);
                adjustToPointToScale(mipmapLevel, p1Right.x, p1Right.y);

                (*fpoint)->getPositionAtTime(time, &p1.x, &p1.y);
                adjustToPointToScale(mipmapLevel,p1.x,p1.y);

                (*nextPoint)->getPositionAtTime(time, &p3.x, &p3.y);
                adjustToPointToScale(mipmapLevel, p3.x, p3.y);

                (*nextPoint)->getLeftBezierPointAtTime(time, &p3Left.x, &p3Left.y);
                adjustToPointToScale(mipmapLevel, p3Left.x,p3Left.y);

                (*nextFPoint)->getPositionAtTime(time, &p2.x, &p2.y);
                adjustToPointToScale(mipmapLevel, p2.x, p2.y);

                (*nextFPoint)->getLeftBezierPointAtTime(time, &p2Left.x, &p2Left.y);
                adjustToPointToScale(mipmapLevel, p2Left.x, p2Left.y);


                ///This completes the previous iteration's triplet
                *preFillIt++ = p1Right;

                ///This makes up the new triplet
                *preFillIt++ = p2Left;
                *preFillIt++ = p2;

                ///linear interpolation
                p0p1.x = (2. * fallOff * p0.x + fallOffInverse * p1.x) / (2. * fallOff + fallOffInverse);
                p0p1.y = (2. * fallOff * p0.y + fallOffInverse * p1.y) / (2. * fallOff + fallOffInverse);
                p1p0.x = (p0.x * fallOff + 2. * fallOffInverse * p1.x) / (fallOff + 2. * fallOffInverse);
                p1p0.y = (p0.y * fallOff + 2. * fallOffInverse * p1.y) /(fallOff + 2. * fallOffInverse);

                p2p3.x = (p3.x * fallOff + 2. * fallOffInverse * p2.x) / (fallOff + 2. * fallOffInverse);
                p2p3.y = (p3.y * fallOff + 2. * fallOffInverse * p2.y) / (fallOff + 2. * fallOffInverse);
                p3p2.x = (2. * fallOff * p3.x + fallOffInverse * p2.x) / (2. * fallOff + fallOffInverse);
                p3p2.y = (2. * fallOff * p3.y + fallOffInverse * p2.y) / (2. * fallOff + fallOffInverse);

                ///move to the initial point
                cairo_mesh_pattern_begin_patch(mesh);
                cairo_mesh_pattern_move_to(mesh, p0.x, p0.y);

                ///make the 1st bezier segment
                cairo_mesh_pattern_curve_to(mesh, p0p1.x,p0p1.y,p1p0.x,p1p0.y,p1.x,p1.y);

                ///make the 2nd bezier segment
                cairo_mesh_pattern_curve_to(mesh, p1Right.x,p1Right.y,p2Left.x,p2Left.y,p2.x,p2.y);

                ///make the 3rd bezier segment
                cairo_mesh_pattern_curve_to(mesh, p2p3.x,p2p3.y,p3p2.x,p3p2.y,p3.x,p3.y);

                ///make the last bezier segment to close the pattern
                cairo_mesh_pattern_curve_to(mesh, p3Left.x,p3Left.y,p0Right.x,p0Right.y,p0.x,p0.y);

                ///Set the 4 corners color

                ///inner is full color
                cairo_mesh_pattern_set_corner_color_rgba(mesh, 0, shapeColor[0], shapeColor[1],shapeColor[2],opacity);

                ///outter is faded
                cairo_mesh_pattern_set_corner_color_rgba(mesh, 1, shapeColor[0], shapeColor[1],shapeColor[2], 0.);
                cairo_mesh_pattern_set_corner_color_rgba(mesh, 2, shapeColor[0], shapeColor[1],shapeColor[2], 0.);

                ///inner is full color
                cairo_mesh_pattern_set_corner_color_rgba(mesh, 3, shapeColor[0], shapeColor[1],shapeColor[2], opacity);
                assert(cairo_pattern_status(mesh) == CAIRO_STATUS_SUCCESS);

                cairo_mesh_pattern_end_patch(mesh);

                ++point;
                ++nextPoint;
                ++fpoint;
                ++nextFPoint;

            }

            ///2nd pass, draw the feather using the pattern previously defined
            preFillIt = preComputedFeatherPoints.begin();


            Point initFp = *preComputedFeatherPoints.rbegin();


            cairo_new_path(cr);
            cairo_move_to(cr, initFp.x, initFp.y);

            while (preFillIt != preComputedFeatherPoints.end()) {

                Point right,nextLeft,next;
                right = *preFillIt++;
                assert(preFillIt != preComputedFeatherPoints.end());
                nextLeft = *preFillIt++;
                assert(preFillIt != preComputedFeatherPoints.end());
                next = *preFillIt++;

                cairo_curve_to(cr,right.x,right.y,nextLeft.x,nextLeft.y,next.x,next.y);

            }
            applyAndDestroyMask(cr,mesh);
        }
#endif // NATRON_WHY_USE_TWICE_THE_SAME_CODE_COPY_PASTE_IS_UGLY
    } // foreach(splines)
    assert(cairo_surface_status(cairoImg) == CAIRO_STATUS_SUCCESS);

    ///A call to cairo_surface_flush() is required before accessing the pixel data
    ///to ensure that all pending drawing operations are finished.
    cairo_surface_flush(cairoImg);
}

void RotoContextPrivate::renderInternalShape(int time,unsigned int mipmapLevel,cairo_t* cr,const BezierCPs& cps)
{

    BezierCPs::const_iterator point = cps.begin();
    BezierCPs::const_iterator nextPoint = point;
    ++nextPoint;

    Point initCp;
    (*point)->getPositionAtTime(time, &initCp.x,&initCp.y);
    adjustToPointToScale(mipmapLevel,initCp.x,initCp.y);

    cairo_move_to(cr, initCp.x,initCp.y);

    while (point != cps.end()) {
        if (nextPoint == cps.end()) {
            nextPoint = cps.begin();
        }

        double rightX,rightY,nextX,nextY,nextLeftX,nextLeftY;
        (*point)->getRightBezierPointAtTime(time, &rightX, &rightY);
        (*nextPoint)->getLeftBezierPointAtTime(time, &nextLeftX, &nextLeftY);
        (*nextPoint)->getPositionAtTime(time, &nextX, &nextY);

        adjustToPointToScale(mipmapLevel,rightX,rightY);
        adjustToPointToScale(mipmapLevel,nextX,nextY);
        adjustToPointToScale(mipmapLevel,nextLeftX,nextLeftY);
        cairo_curve_to(cr, rightX, rightY, nextLeftX, nextLeftY, nextX, nextY);

        ++point;
        ++nextPoint;
    }
//    if (cairo_get_antialias(cr) != CAIRO_ANTIALIAS_NONE ) {
//        cairo_fill_preserve(cr);
//        // These line properties make for a nicer looking polygon mesh
//        cairo_set_line_join(cr, CAIRO_LINE_JOIN_BEVEL);
//        cairo_set_line_cap(cr, CAIRO_LINE_CAP_BUTT);
//        // Comment out the following call to cairo_set_line width
//        // since the hard-coded width value of 1.0 is not appropriate
//        // for fills of small areas. Instead, use the line width that
//        // has already been set by the user via the above call of
//        // poly_line which in turn calls set_current_context which in
//        // turn calls cairo_set_line_width for the user-specified
//        // width.
//        cairo_set_line_width(cr, 1.0);
//        cairo_stroke(cr);
//    } else {
    cairo_fill(cr);
//    }
}

void RotoContextPrivate::applyAndDestroyMask(cairo_t* cr,cairo_pattern_t* mesh)
{
    assert(cairo_pattern_status(mesh) == CAIRO_STATUS_SUCCESS);
    cairo_set_source(cr, mesh);
    
    ///paint with the feather with the pattern as a mask
    cairo_mask(cr, mesh);
    
    cairo_pattern_destroy(mesh);
    

}
