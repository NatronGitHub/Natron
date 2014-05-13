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
#include <cairo/cairo.h>

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
        QMutexLocker m(&itemMutex);
        _imp->locked = locked;
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
        QMutexLocker m(&itemMutex);
        _imp->locked = l;
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
        bool ret;
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

////////////////////////////////////RotoDrawableItem////////////////////////////////////

RotoDrawableItem::RotoDrawableItem(RotoContext* context,const std::string& name,RotoLayer* parent)
: RotoItem(context,name,parent)
, _imp(new RotoDrawableItemPrivate())
{
    QObject::connect(_imp->inverted->getSignalSlotHandler().get(), SIGNAL(valueChanged(int)), this, SIGNAL(inversionChanged()));
    QObject::connect(this, SIGNAL(overlayColorChanged()), context, SIGNAL(refreshViewerOverlays()));
}

RotoDrawableItem::~RotoDrawableItem()
{
    
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
        serializeRotoKnob(_imp->inverted, &s->_inverted);
        memcpy(s->_overlayColor, _imp->overlayColor, sizeof(double) * 4);
        
        
    }
    RotoItem::save(obj);
}

void RotoDrawableItem::load(const RotoItemSerialization &obj)
{
    const RotoDrawableItemSerialization& s = dynamic_cast<const RotoDrawableItemSerialization&>(obj);
    
    
    {
        _imp->activated->clone(s._activated.getKnob());
        _imp->opacity->clone(s._opacity.getKnob());
        _imp->feather->clone(s._feather.getKnob());
        _imp->featherFallOff->clone(s._featherFallOff.getKnob());
        _imp->inverted->clone(s._inverted.getKnob());
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


int RotoDrawableItem::getFeatherDistance(int time) const
{
    ///MT-safe thanks to Knob
    return _imp->feather->getValueAtTime(time);
}

double RotoDrawableItem::getFeatherFallOff(int time) const
{
    ///MT-safe thanks to Knob
    return _imp->featherFallOff->getValueAtTime(time);
}

bool RotoDrawableItem::getInverted(int time) const
{
    ///MT-safe thanks to Knob
    return _imp->inverted->getValueAtTime(time);
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
boost::shared_ptr<Int_Knob> RotoDrawableItem::getFeatherKnob() const { return _imp->feather; }
boost::shared_ptr<Double_Knob> RotoDrawableItem::getFeatherFallOffKnob() const { return _imp->featherFallOff; }
boost::shared_ptr<Double_Knob> RotoDrawableItem::getOpacityKnob() const { return _imp->opacity; }
boost::shared_ptr<Bool_Knob> RotoDrawableItem::getInvertedKnob() const { return _imp->inverted; }

////////////////////////////////////Layer////////////////////////////////////

RotoLayer::RotoLayer(RotoContext* context,const std::string& n,RotoLayer* parent)
: RotoItem(context,n,parent)
, _imp(new RotoLayerPrivate())
{
    
}

RotoLayer::~RotoLayer()
{
    
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
                boost::shared_ptr<RotoLayer> layer = getContext()->addLayer();
                layer->load(*l);
            }
        }
        
    }
    RotoItem::load(obj);
}

void RotoLayer::addItem(const boost::shared_ptr<RotoItem>& item)
{
    ///only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    _imp->items.push_back(item);
    
}

void RotoLayer::removeItem(const RotoItem* item)
{
    ///only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    for (RotoItems::iterator it = _imp->items.begin(); it!=_imp->items.end();++it)
    {
        if (it->get() == item) {
            _imp->items.erase(it);
        }
    }
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
                      std::list< Point >* points,RectD* bbox = NULL)
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
            
            if (bbox) {
                if (cur.first < bbox->x1) {
                    bbox->x1 = cur.first;
                }
                if (cur.first >= bbox->x2) {
                    bbox->x2 = cur.first + 1;
                }
                if (cur.second < bbox->y1) {
                    bbox->y1 = cur.second;
                }
                if (cur.second >= bbox->y2) {
                    bbox->y2 = cur.second;
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



Bezier::Bezier(RotoContext* ctx,const std::string& name,RotoLayer* parent)
: RotoDrawableItem(ctx,name,parent)
, _imp(new BezierPrivate())
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
        QMutexLocker l(&itemMutex);
        assert(!_imp->finished);
        
        bool autoKeying = getContext()->isAutoKeyingEnabled();
        int keyframeTime;
        ///if the curve is empty make a new keyframe at the current timeline's time
        ///otherwise re-use the time at which the keyframe was set on the first control point
        if (_imp->points.empty()) {
            keyframeTime = getContext()->getTimelineCurrentTime();
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
    
    QMutexLocker l(&itemMutex);
        
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
    
    bool autoKeying = getContext()->isAutoKeyingEnabled();
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
    
    bool autoKeying = getContext()->isAutoKeyingEnabled();
    
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
        }
        
        if (getContext()->isRippleEditEnabled()) {
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
    
    bool autoKeying = getContext()->isAutoKeyingEnabled();
    bool featherLink = getContext()->isFeatherLinkEnabled();
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
        } else {
            ///this function is called when building a new bezier we must
            ///move the static position if there is no keyframe, otherwise the
            ///curve would never be built
            (*cp)->setLeftBezierStaticPosition(x + dx, y + dy);
            if (moveFeather) {
                (*fp)->setLeftBezierStaticPosition(xF + dx, yF + dy);
            }
        }
        
        if (getContext()->isRippleEditEnabled()) {
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
    
    bool autoKeying = getContext()->isAutoKeyingEnabled();
    bool featherLink = getContext()->isFeatherLinkEnabled();
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
        } else {
            ///this function is called when building a new bezier we must
            ///move the static position if there is no keyframe, otherwise the
            ///curve would never be built
            (*cp)->setRightBezierStaticPosition(x + dx, y + dy);
            if (moveFeather) {
                (*fp)->setRightBezierStaticPosition(xF + dx, yF + dy);
            }
        }
        
        if (getContext()->isRippleEditEnabled()) {
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
    
    bool autoKeying = getContext()->isAutoKeyingEnabled();
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
}

void Bezier::setRightBezierPoint(int index,int time,double x,double y)
{
    ///only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    
    bool autoKeying = getContext()->isAutoKeyingEnabled();
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
    
}

void Bezier::setPointAtIndex(bool feather,int index,int time,double x,double y,double lx,double ly,double rx,double ry)
{
    ///only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    
    bool autoKeying = getContext()->isAutoKeyingEnabled();
    bool rippleEdit = getContext()->isRippleEditEnabled();
    
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
    
    bool autoKeying = getContext()->isAutoKeyingEnabled();
    bool rippleEdit = getContext()->isRippleEditEnabled();
    
    {
        QMutexLocker l(&itemMutex);
        
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
    
    bool autoKeying = getContext()->isAutoKeyingEnabled();
    bool rippleEdit = getContext()->isRippleEditEnabled();
    
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

    {
        

        QMutexLocker l(&itemMutex);
        if (_imp->hasKeyframeAtTime(time)) {
            l.unlock();
            emit keyframeSet(time);
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



void Bezier::evaluateAtTime_DeCastelJau(int time,unsigned int mipMapLevel,
                                        int nbPointsPerSegment,std::list<std::pair<double,double> >* points,RectD* bbox) const
{
    QMutexLocker l(&itemMutex);
 
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
        evalBezierSegment(*(*it),*(*next), time,mipMapLevel, nbPointsPerSegment, points,bbox);
    }
}

void Bezier::evaluateFeatherPointsAtTime_DeCastelJau(int time,int nbPointsPerSegment,std::list<std::pair<double,double> >* points,
                                                     RectD* bbox) const
{
    QMutexLocker l(&itemMutex);
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
            evalBezierSegment(*(*it),*(*next), time,0, nbPointsPerSegment, points,bbox);
        }
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
    evaluateAtTime_DeCastelJau(time,0, 50,&pts,&bbox);
    evaluateFeatherPointsAtTime_DeCastelJau(time, 50, &pts,&bbox);
    
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
Bezier::isNearbyControlPoint(double x,double y,double acceptance,int* index) const
{
    ///only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    int time = getContext()->getTimelineCurrentTime();
    
    QMutexLocker l(&itemMutex);
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
    QMutexLocker l(&itemMutex);
    
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

////////////////////////////////////RotoContext////////////////////////////////////


RotoContext::RotoContext(Natron::Node* node)
: _imp(new RotoContextPrivate(node))
{
    ////Add the base layer
    addLayer();
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
            if (!_imp->layers.empty()) {
                parentLayer = _imp->layers.front().get();
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
    emit itemInserted();
    
    clearSelection(RotoContext::OTHER);
    select(item, RotoContext::OTHER);
    return item;
}

boost::shared_ptr<RotoItem> RotoContext::getLastInsertedItem() const
{
    ///MT-safe: only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    return _imp->lastInsertedItem;
}

boost::shared_ptr<Bool_Knob> RotoContext::getInvertedKnob() const
{
    return _imp->inverted;
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
                addLayer();
            }
            parentLayer = _imp->layers.front().get();
            
        } else {
            parentLayer = deepestLayer;
        }
        
    }
    assert(parentLayer);
    boost::shared_ptr<Bezier> curve(new Bezier(this,ss.str(),parentLayer));
    curve->addControlPoint(x, y);
    if (parentLayer) {
        parentLayer->addItem(curve);
    }
    _imp->lastInsertedItem = curve;
    emit itemInserted();
    
    clearSelection(RotoContext::OTHER);
    select(curve, RotoContext::OTHER);
    
    return curve;
}

void RotoContext::removeItemRecursively(RotoItem* item)
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
            removeItemRecursively(it->get());
        }
        for (std::list<boost::shared_ptr<RotoLayer> >::iterator it = _imp->layers.begin(); it!=_imp->layers.end(); ++it) {
            if (it->get() == isLayer) {
                _imp->layers.erase(it);
                break;
            }
        }
    }
    emit itemRemoved(item);

}

void RotoContext::removeItem(RotoItem* item)
{
    ///MT-safe: only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    {
        QMutexLocker l(&_imp->rotoContextMutex);
        RotoLayer* layer = item->getParentLayer();
        if (layer) {
            layer->removeItem(item);
        }
        removeItemRecursively(item);
    }
    emit selectionChanged((int)RotoContext::OTHER);
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
                rod->merge(splineRoDI);
                
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
    _imp->inverted->setAllDimensionsEnabled(false);
    
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
    
    int nbBeziers = 0;
    bool foundItem = false;
    for (std::list<boost::shared_ptr<RotoItem> >::iterator it = _imp->selectedItems.begin(); it != _imp->selectedItems.end(); ++it) {
        if (dynamic_cast<Bezier*>(it->get())) {
            ++nbBeziers;
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
        
        ++nbBeziers;
        ///first-off set the context knobs to the value of this bezier
        boost::shared_ptr<KnobI> activated = isBezier->getActivatedKnob();
        boost::shared_ptr<KnobI> feather = isBezier->getFeatherKnob();
        boost::shared_ptr<KnobI> featherFallOff = isBezier->getFeatherFallOffKnob();
        boost::shared_ptr<KnobI> opacity = isBezier->getOpacityKnob();
        boost::shared_ptr<KnobI> inverted = isBezier->getInvertedKnob();
        
        _imp->activated->clone(activated);
        _imp->feather->clone(feather);
        _imp->featherFallOff->clone(featherFallOff);
        _imp->opacity->clone(opacity);
        _imp->inverted->clone(inverted);
        
        ///link this bezier knobs to the context
        activated->slaveTo(0, _imp->activated, 0);
        feather->slaveTo(0, _imp->feather, 0);
        featherFallOff->slaveTo(0, _imp->featherFallOff, 0);
        opacity->slaveTo(0, _imp->opacity, 0);
        inverted->slaveTo(0, _imp->inverted, 0);
        
    } else if (isLayer) {
        const RotoItems& children = isLayer->getItems();
        for (RotoItems::const_iterator it = children.begin(); it!=children.end(); ++it) {
            selectInternal(*it);
        }
    }
    
    if (nbBeziers > 0) {
        ///enable the knobs
        _imp->activated->setAllDimensionsEnabled(true);
        _imp->opacity->setAllDimensionsEnabled(true);
        _imp->featherFallOff->setAllDimensionsEnabled(true);
        _imp->feather->setAllDimensionsEnabled(true);
        _imp->inverted->setAllDimensionsEnabled(true);
    }
    
    ///if there are multiple selected beziers, notify the gui knobs so they appear like not displaying an accurate value
    ///(maybe black or something)
    if (nbBeziers >= 2) {
        _imp->activated->setDirty(true);
        _imp->opacity->setDirty(true);
        _imp->feather->setDirty(true);
        _imp->featherFallOff->setDirty(true);
        _imp->inverted->setDirty(true);
    }
    
    
    
    _imp->selectedItems.push_back(item);
    
    
}

void RotoContext::deselectInternal(const boost::shared_ptr<RotoItem>& b)
{
    ///only called on the main-thread
    assert(QThread::currentThread() == qApp->thread());
    
    assert(!_imp->rotoContextMutex.tryLock());
    
    std::list<boost::shared_ptr<RotoItem> >::iterator it = std::find(_imp->selectedItems.begin(),_imp->selectedItems.end(),b);
    
    ///if the item is not selected, exit
    if (it == _imp->selectedItems.end()) {
        return;
    }
    
    _imp->selectedItems.erase(it);
    
    Bezier* isBezier = dynamic_cast<Bezier*>(b.get());
    RotoLayer* isLayer = dynamic_cast<RotoLayer*>(b.get());
    if (isBezier) {
  
        ///first-off set the context knobs to the value of this bezier
        boost::shared_ptr<KnobI> activated = isBezier->getActivatedKnob();
        boost::shared_ptr<KnobI> feather = isBezier->getFeatherKnob();
        boost::shared_ptr<KnobI> featherFallOff = isBezier->getFeatherFallOffKnob();
        boost::shared_ptr<KnobI> opacity = isBezier->getOpacityKnob();
        boost::shared_ptr<KnobI> inverted = isBezier->getInvertedKnob();
        
        activated->unSlave(0,false);
        feather->unSlave(0,false);
        featherFallOff->unSlave(0,false);
        opacity->unSlave(0,false);
        inverted->unSlave(0,false);
        
    } else if (isLayer) {
        const RotoItems& children = isLayer->getItems();
        for (RotoItems::const_iterator it = children.begin(); it!=children.end(); ++it) {
            deselectInternal(*it);
        }
    }
    int nbBeziers = 0;
    for (std::list<boost::shared_ptr<RotoItem> >::iterator it = _imp->selectedItems.begin(); it != _imp->selectedItems.end(); ++it) {
        if (dynamic_cast<Bezier*>(it->get())) {
            ++nbBeziers;
        }
    }
    
    if (nbBeziers <= 1) {
        _imp->activated->setDirty(false);
        _imp->opacity->setDirty(false);
        _imp->feather->setDirty(false);
        _imp->featherFallOff->setDirty(false);
        _imp->inverted->setDirty(false);
    }
    
    ///if the selected beziers count reaches 0 notify the gui knobs so they appear not enabled
    if (nbBeziers == 0) {
        _imp->activated->setAllDimensionsEnabled(false);
        _imp->opacity->setAllDimensionsEnabled(false);
        _imp->featherFallOff->setAllDimensionsEnabled(false);
        _imp->feather->setAllDimensionsEnabled(false);
        _imp->inverted->setAllDimensionsEnabled(false);
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
    int time = getTimelineCurrentTime();
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
    _imp->node->getLiveInstance()->evaluate_public(NULL, true);
}

U64 RotoContext::getAge()
{
    QMutexLocker l(&_imp->rotoContextMutex);
    return _imp->age;
}

void RotoContext::onItemLockedChanged()
{
    RotoItem* item = qobject_cast<RotoItem*>(sender());
    if (item) {
        emit itemLockedChanged();
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

#pragma message WARN("This is broken, we should instead use left and right derivatives directions because if feather == cp it doesn't work")
static void expandToFeatherDistance(const Point& point,Point* featherPoint,int featherDistance)
{
    if (featherDistance != 0) {
        double dx = (featherPoint->first - point.first);
        double dy = (featherPoint->second - point.second);
        double dist = sqrt(dx * dx + dy * dy);
        featherPoint->first = (dx * (dist + featherDistance)) / dist + point.first;
        featherPoint->second = (dy * (dist + featherDistance)) / dist + point.second;
    }
}

boost::shared_ptr<Natron::Image> RotoContext::renderMask(const RectI& roi,U64 nodeHash,U64 ageToRender,const RectI& nodeRoD,SequenceTime time,
                                            int view,unsigned int mipmapLevel,bool byPassCache)
{
    

    std::list< boost::shared_ptr<Bezier> > splines = getCurvesByRenderOrder();
    
    ///compute an enhanced hash different from the one of the node in order to differentiate within the cache
    ///the output image of the roto node and the mask image.
    Hash64 hash;
    hash.append(nodeHash);
    hash.append(ageToRender);
    hash.computeHash();

    Natron::ImageKey key = Natron::Image::makeKey(hash.value(), time, mipmapLevel, view);
    boost::shared_ptr<const Natron::ImageParams> params;
    boost::shared_ptr<Natron::Image> image;
    
    bool cached = Natron::getImageFromCache(key, &params, &image);
   
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
        Natron::ImageComponents maskComps = Natron::ImageComponentAlpha;
        
        params = Natron::Image::makeParams(0, nodeRoD,mipmapLevel,false,
                                                    maskComps,
                                                    -1, time,
                                                    std::map<int, std::vector<RangeD> >());
        
        cached = appPTR->getImageOrCreate(key, params, &image);
        assert(image);

        if (cached && byPassCache) {
            image->clearBitmap();
        }
    }
    
    ///////////////////////////////////Render internal
    RectI pixelRod = params->getPixelRoD();
    RectI clippedRoI;
    roi.intersect(pixelRod, &clippedRoI);
 
    QMutexLocker l(&_imp->rotoContextMutex);
    int i = 0;
    
    ////Allocate the cairo temporary buffer
    cairo_surface_t* cairoImg = cairo_image_surface_create(CAIRO_FORMAT_A8, pixelRod.width(), pixelRod.height());
    if (cairo_surface_status(cairoImg) != CAIRO_STATUS_SUCCESS) {
        appPTR->removeFromNodeCache(image);
        return image;
    }
    cairo_t* cr = cairo_create(cairoImg);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    
    for (std::list<boost::shared_ptr<Bezier> >::iterator it2 = splines.begin(); it2!=splines.end(); ++it2,++i) {
        
        ///render the bezier ONLY if the image is not cached OR if the image is cached but the bezier has changed.
        ///Also render only finished bezier
        if ((*it2)->isCurveFinished() && (*it2)->isActivated(time)) {
            
            
            BezierCPs cps = (*it2)->getControlPoints_mt_safe();
            BezierCPs fps = (*it2)->getFeatherPoints_mt_safe();
            
            assert(cps.size() == fps.size());
            
            if (cps.empty()) {
                continue;
            }
            
            double opacity = (*it2)->getOpacity(time);
            bool inverted = (*it2)->getInverted(time);
            if (inverted) {
                cairo_set_source_rgba(cr, 1.,1.,1., opacity);
                cairo_paint(cr);
            }
            
            
            BezierCPs::iterator point = cps.begin();
            BezierCPs::iterator fpoint = fps.begin();
            
//            BezierCPs::iterator prevPoint = cps.end();
//            --prevPoint;
//            BezierCPs::iterator prevFPoint = fps.end();
//            --prevFPoint;
            
            BezierCPs::iterator nextPoint = point;
            ++nextPoint;
            BezierCPs::iterator nextFPoint = fpoint;
            ++nextFPoint;

            
            ////1st pass, define the feather edge pattern
            cairo_pattern_t* mesh = cairo_pattern_create_mesh();
            if (cairo_pattern_status(mesh) != CAIRO_STATUS_SUCCESS) {
                cairo_pattern_destroy(mesh);
                continue;
            }
            
            double fallOff = (*it2)->getFeatherFallOff(time);
            double fallOffInverse = 1. / fallOff;
            
            int featherDist = (*it2)->getFeatherDistance(time);
            if (mipmapLevel != 0) {
                featherDist /= (1 << mipmapLevel);
            }
            while (point != cps.end()) {
                if (nextPoint == cps.end()) {
                    nextPoint = cps.begin();
                    nextFPoint = fps.begin();
                }
                
                Point p0,p1,p2,p3,p0p1,p1p0,p0Right,p1Left,p1Right,p2p3,p3p2,p2Left,p2Right,p3Left;
                (*point)->getPositionAtTime(time, &p0.first, &p0.second);
                adjustToPointToScale(mipmapLevel,p0.first,p0.second);
                
                (*fpoint)->getPositionAtTime(time, &p1.first, &p1.second);
                adjustToPointToScale(mipmapLevel,p1.first,p1.second);
                
                (*nextFPoint)->getPositionAtTime(time, &p2.first, &p2.second);
                adjustToPointToScale(mipmapLevel, p2.first, p2.second);
                
                (*nextPoint)->getPositionAtTime(time, &p3.first, &p3.second);
                adjustToPointToScale(mipmapLevel, p3.first, p3.second);
                
                (*point)->getRightBezierPointAtTime(time, &p0Right.first, &p0Right.second);
                adjustToPointToScale(mipmapLevel, p0Right.first, p0Right.second);
                
                (*fpoint)->getRightBezierPointAtTime(time, &p1Right.first, &p1Right.second);
                adjustToPointToScale(mipmapLevel, p1Right.first, p1Right.second);
                
                (*nextFPoint)->getLeftBezierPointAtTime(time, &p2Left.first, &p2Left.second);
                adjustToPointToScale(mipmapLevel, p2Left.first, p2Left.second);
                
                (*nextPoint)->getLeftBezierPointAtTime(time, &p3Left.first, &p3Left.second);
                adjustToPointToScale(mipmapLevel, p3Left.first,p3Left.second);
                
                expandToFeatherDistance(p0, &p1, featherDist);
                expandToFeatherDistance(p3, &p2, featherDist);
                expandToFeatherDistance(p0Right, &p1Right, featherDist);
                expandToFeatherDistance(p3Left, &p2Left, featherDist);
                
                ///linear interpolation
                p0p1.first = (2. * fallOff * p0.first + fallOffInverse * p1.first) / (2. * fallOff + fallOffInverse);
                p0p1.second = (2. * fallOff * p0.second + fallOffInverse * p1.second) / (2. * fallOff + fallOffInverse);
                p1p0.first = (p0.first * fallOff + 2. * fallOffInverse * p1.first) / (fallOff + 2. * fallOffInverse);
                p1p0.second = (p0.second * fallOff + 2. * fallOffInverse * p1.second) /(fallOff + 2. * fallOffInverse);
                
                p2p3.first = (p3.first * fallOff + 2. * fallOffInverse * p2.first) / (fallOff + 2. * fallOffInverse);
                p2p3.second = (p3.second * fallOff + 2. * fallOffInverse * p2.second) / (fallOff + 2. * fallOffInverse);
                p3p2.first = (2. * fallOff * p3.first + fallOffInverse * p2.first) / (2. * fallOff + fallOffInverse);
                p3p2.second = (2. * fallOff * p3.second + fallOffInverse * p2.second) / (2. * fallOff + fallOffInverse);
                
                ///move to the initial point
                cairo_mesh_pattern_begin_patch(mesh);
                cairo_mesh_pattern_move_to(mesh, p0.first, p0.second);
                
                ///make the 1st bezier segment
                cairo_mesh_pattern_curve_to(mesh, p0p1.first,p0p1.second,p1p0.first,p1p0.second,p1.first,p1.second);
                
                ///make the 2nd bezier segment
                cairo_mesh_pattern_curve_to(mesh, p1Right.first,p1Right.second,p2Left.first,p2Left.second,p2.first,p2.second);
                
                ///make the 3rd bezier segment
                cairo_mesh_pattern_curve_to(mesh, p2p3.first,p2p3.second,p3p2.first,p3p2.second,p3.first,p3.second);
                
                ///make the last bezier segment to close the pattern
                cairo_mesh_pattern_curve_to(mesh, p3Left.first,p3Left.second,p0Right.first,p0Right.second,p0.first,p0.second);
                
                ///Set the 4 corners color
                
                ///inner is full color
                cairo_mesh_pattern_set_corner_color_rgba(mesh, 0, 1., 1., 1., inverted ? 1. - opacity : opacity);
                
                ///outter is faded
                cairo_mesh_pattern_set_corner_color_rgba(mesh, 1, 1., 1., 1., inverted ? 1. :  0.);
                cairo_mesh_pattern_set_corner_color_rgba(mesh, 2, 1., 1., 1., inverted ? 1. :  0.);
                
                ///inner is full color
                cairo_mesh_pattern_set_corner_color_rgba(mesh, 3, 1., 1., 1., inverted ? 1. - opacity : opacity);
                assert(cairo_pattern_status(mesh) == CAIRO_STATUS_SUCCESS);
                
                cairo_mesh_pattern_end_patch(mesh);
                
                ++point;
                ++nextPoint;
                ++fpoint;
                ++nextFPoint;
                
            }
            assert(cairo_pattern_status(mesh) == CAIRO_STATUS_SUCCESS);
            cairo_set_source(cr, mesh);
            
        
            
            Point initFp,initCp;
        
            ///2nd pass, draw the feather using the pattern previously defined
            fpoint = fps.begin();
            nextFPoint = fpoint;
            ++nextFPoint;
            point = cps.begin();
            nextPoint = point;
            ++nextPoint;

            (*fpoint)->getPositionAtTime(time, &initFp.first, &initFp.second);
            adjustToPointToScale(mipmapLevel, initFp.first, initFp.second);
            (*point)->getPositionAtTime(time,&initCp.first,&initCp.second);
            adjustToPointToScale(mipmapLevel, initCp.first, initCp.second);
            expandToFeatherDistance(initCp, &initFp, featherDist);
            
            cairo_new_path(cr);
            cairo_move_to(cr, initFp.first, initFp.second);
            
            while (fpoint != fps.end()) {
                if (nextFPoint == fps.end()) {
                    nextPoint = cps.begin();
                    nextFPoint = fps.begin();
                }
                
                Point rightF,nextLeftF,nextF,rightP,nextLeftP,nextP;

                (*fpoint)->getRightBezierPointAtTime(time, &rightF.first, &rightF.second);
                adjustToPointToScale(mipmapLevel, rightF.first, rightF.second);
                (*point)->getRightBezierPointAtTime(time, &rightP.first, &rightP.second);
                adjustToPointToScale(mipmapLevel, rightP.first, rightP.second);
                
                
                (*nextFPoint)->getLeftBezierPointAtTime(time, &nextLeftF.first, &nextLeftF.second);
                adjustToPointToScale(mipmapLevel, nextLeftF.first, nextLeftF.second);
                (*nextPoint)->getLeftBezierPointAtTime(time, &nextLeftP.first, &nextLeftP.second);
                adjustToPointToScale(mipmapLevel, nextLeftP.first, nextLeftP.second);
                
                (*nextFPoint)->getPositionAtTime(time, &nextF.first, &nextF.second);
                adjustToPointToScale(mipmapLevel, nextF.first, nextF.second);
                (*nextPoint)->getPositionAtTime(time, &nextP.first, &nextP.second);
                adjustToPointToScale(mipmapLevel, nextP.first, nextP.second);
                
                
                expandToFeatherDistance(rightP, &rightF, featherDist);
                expandToFeatherDistance(nextLeftP, &nextLeftF, featherDist);
                expandToFeatherDistance(nextP, &nextF, featherDist);
                
                cairo_curve_to(cr,rightF.first,rightF.second,nextLeftF.first,nextLeftF.second,nextF.first,nextF.second);
                
                ++fpoint;
                ++point;
                ++nextFPoint;
                ++nextPoint;
            }

            
            cairo_fill(cr);
            cairo_pattern_destroy(mesh);
            
            
            ////3rd pass, fill the internal bezier

            point = cps.begin();
            nextPoint = point;
            ++nextPoint;
            
            (*point)->getPositionAtTime(time, &initCp.first,&initCp.second);
            adjustToPointToScale(mipmapLevel,initCp.first,initCp.second);
            
            
            cairo_set_source_rgba(cr, 1.,1.,1., inverted ? 1. - opacity : opacity);
            cairo_new_path(cr);
            cairo_move_to(cr, initCp.first,initCp.second);
        
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
            
            cairo_fill(cr);
            
            
           
        }
        
    }
    assert(cairo_surface_status(cairoImg) == CAIRO_STATUS_SUCCESS);
    
    ///A call to cairo_surface_flush() is required before accessing the pixel data
    ///to ensure that all pending drawing operations are finished.
    cairo_surface_flush(cairoImg);
    unsigned char* cdata = cairo_image_surface_get_data(cairoImg);
    unsigned char* srcPix = cdata;
    int stride = cairo_image_surface_get_stride(cairoImg);
    
    int comps = (int)image->getComponentsCount();
    for (int y = 0; y < pixelRod.height(); ++y, srcPix += stride) {
        
        float* dstPix = image->pixelAt(pixelRod.x1, pixelRod.y1 + y);
        assert(dstPix);
        
        for (int x = 0; x < pixelRod.width(); ++x) {
            if (comps == 1) {
                dstPix[x] = srcPix[x] / 255.f;
            } else {
                assert(comps == 4);
                dstPix[x * 4 + 3] = srcPix[x] / 255.f;
            }
        }
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
    return image;
}
