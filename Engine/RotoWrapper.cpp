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

#include "RotoWrapper.h"

#include "Engine/Bezier.h"
#include "Engine/BezierCP.h"
#include "Engine/EffectInstance.h"
#include "Engine/KnobTypes.h"
#include "Engine/NodeWrapper.h"
#include "Engine/Node.h"
#include "Engine/RotoContext.h"
#include "Engine/RotoLayer.h"
#include "Engine/RotoStrokeItem.h"

NATRON_NAMESPACE_USING

ItemBase::ItemBase(const boost::shared_ptr<RotoItem>& item)
: _item(item)
{
    
}

ItemBase::~ItemBase()
{
    
}

void
ItemBase::setLabel(const std::string & name)
{
    _item->setLabel(name);
}

std::string
ItemBase::getLabel() const
{
    return _item->getLabel();
}

bool
ItemBase::setScriptName(const std::string& name)
{
    return _item->setScriptName(name);
}

std::string
ItemBase::getScriptName() const
{
    return _item->getScriptName();
}

void
ItemBase::setLocked(bool locked)
{
    _item->setLocked(locked, true,RotoItem::eSelectionReasonOther);
}

bool
ItemBase::getLocked() const
{
    return _item->getLocked();
}

bool
ItemBase::getLockedRecursive() const
{
    return _item->isLockedRecursive();
}

void
ItemBase::setVisible(bool activated)
{
    _item->setGloballyActivated(activated, true);
}

bool
ItemBase::getVisible() const
{
    return _item->isGloballyActivated();
}

Layer*
ItemBase::getParentLayer() const
{
    boost::shared_ptr<RotoLayer> layer =  _item->getParentLayer();
    if (layer) {
        return new Layer(layer);
    } else {
        return 0;
    }
}

Param*
ItemBase::getParam(const std::string& name) const
{
    RotoDrawableItem* drawable = dynamic_cast<RotoDrawableItem*>(_item.get());
    if (!drawable) {
        return 0;
    }
    boost::shared_ptr<KnobI> knob = drawable->getKnobByName(name);
    if (!knob) {
        return 0;
    }
    Param* ret = Effect::createParamWrapperForKnob(knob);
    return ret;
}


Layer::Layer(const boost::shared_ptr<RotoItem>& item)
: ItemBase(item)
, _layer(boost::dynamic_pointer_cast<RotoLayer>(item))
{
    
}

Layer::~Layer()
{
    
}

void
Layer::addItem(ItemBase* item)
{
    _layer->addItem(item->getInternalItem());
}

void
Layer::insertItem(int pos, ItemBase* item)
{
    _layer->insertItem(item->getInternalItem(), pos);
}

void
Layer::removeItem(ItemBase* item)
{
    _layer->removeItem(item->getInternalItem());
}

std::list<ItemBase*>
Layer::getChildren() const
{
    std::list<ItemBase*> ret;
    std::list<boost::shared_ptr<RotoItem> > items = _layer->getItems_mt_safe();
    for (std::list<boost::shared_ptr<RotoItem> >::iterator it = items.begin(); it != items.end(); ++it) {
        ret.push_back(new ItemBase(*it));
    }
    return ret;
}

BezierCurve::BezierCurve(const boost::shared_ptr<RotoItem>& item)
: ItemBase(item)
, _bezier(boost::dynamic_pointer_cast<Bezier>(item))
{
    
}

BezierCurve::~BezierCurve()
{
    
}


void
BezierCurve::setCurveFinished(bool finished)
{
    _bezier->setCurveFinished(finished);
    _bezier->getContext()->emitRefreshViewerOverlays();
    _bezier->getContext()->evaluateChange();
}

bool
BezierCurve::isCurveFinished() const
{
    return _bezier->isCurveFinished();
}

void
BezierCurve::addControlPoint(double x, double y)
{
    const std::list<boost::shared_ptr<BezierCP> >& cps = _bezier->getControlPoints();
    double keyframeTime;
    if (!cps.empty()) {
        keyframeTime = cps.front()->getKeyframeTime(false,0);
    } else {
        keyframeTime = _bezier->getContext()->getTimelineCurrentTime();
    }
    _bezier->addControlPoint(x, y, keyframeTime);
}

void
BezierCurve::addControlPointOnSegment(int index,double t)
{
    _bezier->addControlPointAfterIndex(index, t);
}

void
BezierCurve::removeControlPointByIndex(int index)
{
    _bezier->removeControlPointByIndex(index);
}

void
BezierCurve::movePointByIndex(int index,double time,double dx,double dy)
{
    _bezier->movePointByIndex(index, time, dx, dy);
}

void
BezierCurve::moveFeatherByIndex(int index,double time,double dx,double dy)
{
    _bezier->moveFeatherByIndex(index, time, dx, dy);
}

void
BezierCurve::moveLeftBezierPoint(int index,double time,double dx,double dy)
{
    _bezier->moveLeftBezierPoint(index, time, dx, dy);
}

void
BezierCurve::moveRightBezierPoint(int index,double time,double dx,double dy)
{
    _bezier->moveRightBezierPoint(index, time, dx, dy);
}

void
BezierCurve::setPointAtIndex(int index,double time,double x,double y,double lx,double ly,double rx,double ry)
{
    _bezier->setPointAtIndex(false, index, time, x, y, lx, ly, rx, ry);
}

void
BezierCurve::setFeatherPointAtIndex(int index,double time,double x,double y,double lx,double ly,double rx,double ry)
{
    _bezier->setPointAtIndex(true, index, time, x, y, lx, ly, rx, ry);
}

void
BezierCurve::slavePointToTrack(int index, double trackTime, DoubleParam* trackCenter)
{
    if (!trackCenter) {
        return;
    }
    boost::shared_ptr<KnobI> internalKnob = trackCenter->getInternalKnob();
    if (!internalKnob) {
        return;
    }
    
    boost::shared_ptr<KnobDouble> isDouble = boost::dynamic_pointer_cast<KnobDouble>(internalKnob);
    if (!isDouble) {
        return;
    }
    
    Natron::EffectInstance* parent = dynamic_cast<Natron::EffectInstance*>(isDouble->getHolder());
    if (!parent) {
        return;
    }
    if (!parent->getNode()->isPointTrackerNode()) {
        return;
    }
    
    if (isDouble->getName() != "center" || isDouble->getDimension() != 2) {
        return;
    }
    
    boost::shared_ptr<BezierCP> cp = _bezier->getControlPointAtIndex(index);
    if (!cp) {
        return;
    }
    
    cp->slaveTo(trackTime, isDouble);
    
    boost::shared_ptr<BezierCP> fp = _bezier->getFeatherPointAtIndex(index);
    if (!fp) {
        return;
    }
    
    fp->slaveTo(trackTime, isDouble);
}

DoubleParam*
BezierCurve::getPointMasterTrack(int index) const
{
    boost::shared_ptr<BezierCP> cp = _bezier->getControlPointAtIndex(index);
    if (!cp) {
        return 0;
    }
    
    boost::shared_ptr<KnobDouble>  knob = cp->isSlaved();
    if (!knob) {
        return 0;
    }
    return new DoubleParam(knob);
}


int
BezierCurve::getNumControlPoints() const
{
    return _bezier->getControlPointsCount();
}

void
BezierCurve::setActivated(double time, bool activated)
{
    _bezier->setActivated(activated, time);
}

bool
BezierCurve::getIsActivated(double time)
{
    return _bezier->isActivated(time);
}

void
BezierCurve::setOpacity(double opacity, double time)
{
    _bezier->setOpacity(opacity, time);
}

double
BezierCurve::getOpacity(double time) const{
    return _bezier->getOpacity(time);
}

ColorTuple
BezierCurve::getOverlayColor() const
{
    ColorTuple c;
    double color[4];
    _bezier->getOverlayColor(color);
    c.r = color[0];
    c.g = color[1];
    c.b = color[2];
    c.a = 1.;
    return c;
}

void
BezierCurve::setOverlayColor(double r,double g,double b)
{
    double color[4];
    color[0] = r;
    color[1] = g;
    color[2] = b;
    color[3] = 1.;
    _bezier->setOverlayColor(color);
}

double
BezierCurve::getFeatherDistance(double time) const
{
    return _bezier->getFeatherDistance(time);
}

void
BezierCurve::setFeatherDistance(double dist,double time)
{
    _bezier->setFeatherDistance(dist, time);
}

double
BezierCurve::getFeatherFallOff(double time) const
{
    return _bezier->getFeatherFallOff(time);
}

void
BezierCurve::setFeatherFallOff(double falloff,double time)
{
    _bezier->setFeatherFallOff(falloff, time);
}

ColorTuple
BezierCurve::getColor(double time)
{
    ColorTuple c;
    double color[3];
    _bezier->getColor(time, color);
    c.r = color[0];
    c.g = color[1];
    c.b = color[2];
    c.a = 1.;
    return c;
}

void
BezierCurve::setColor(double time,double r, double g, double b)
{
    _bezier->setColor(time, r, g, b);
}

void
BezierCurve::setCompositingOperator(BezierCurve::CairoOperatorEnum op)
{

    _bezier->setCompositingOperator((int)op);
}

BezierCurve::CairoOperatorEnum
BezierCurve::getCompositingOperator() const
{
    return (BezierCurve::CairoOperatorEnum)_bezier->getCompositingOperator();
}

BooleanParam*
BezierCurve::getActivatedParam() const
{
    
    boost::shared_ptr<KnobBool> ret = _bezier->getActivatedKnob();
    if (ret) {
        return new BooleanParam(ret);
    }
    return 0;
}

DoubleParam*
BezierCurve::getOpacityParam() const
{
    boost::shared_ptr<KnobDouble> ret = _bezier->getOpacityKnob();
    if (ret) {
        return new DoubleParam(ret);
    }
    return 0;
}
DoubleParam*
BezierCurve::getFeatherDistanceParam() const
{
    boost::shared_ptr<KnobDouble> ret = _bezier->getFeatherKnob();
    if (ret) {
        return new DoubleParam(ret);
    }
    return 0;
}

DoubleParam*
BezierCurve::getFeatherFallOffParam() const
{
    boost::shared_ptr<KnobDouble> ret = _bezier->getFeatherFallOffKnob();
    if (ret) {
        return new DoubleParam(ret);
    }
    return 0;
}

ColorParam*
BezierCurve::getColorParam() const
{
    boost::shared_ptr<KnobColor> ret = _bezier->getColorKnob();
    if (ret) {
        return new ColorParam(ret);
    }
    return 0;
}

ChoiceParam*
BezierCurve::getCompositingOperatorParam() const
{
    boost::shared_ptr<KnobChoice> ret = _bezier->getOperatorKnob();
    if (ret) {
        return new ChoiceParam(ret);
    }
    return 0;

}

Roto::Roto(const boost::shared_ptr<RotoContext>& ctx)
: _ctx(ctx)
{
    
}

Roto::~Roto()
{
    
}


Layer*
Roto::getBaseLayer() const
{
    const std::list<boost::shared_ptr<RotoLayer> >& layers = _ctx->getLayers();
    if (!layers.empty()) {
        return new Layer(layers.front());
    }
    return 0;
}

ItemBase*
Roto::getItemByName(const std::string& name) const
{
    boost::shared_ptr<RotoItem> item =  _ctx->getItemByName(name);
    if (!item) {
        return 0;
    }
    RotoLayer* isLayer = dynamic_cast<RotoLayer*>(item.get());
    if (isLayer) {
        return new Layer(item);
    }
    Bezier* isBezier = dynamic_cast<Bezier*>(item.get());
    if (isBezier) {
        return new BezierCurve(item);
    }
    RotoStrokeItem* isStroke = dynamic_cast<RotoStrokeItem*>(item.get());
    if (isStroke) {
        return new ItemBase(item);
    }
    return 0;
}

Layer*
Roto::createLayer()
{
    boost::shared_ptr<RotoLayer>  layer = _ctx->addLayer();
    if (layer) {
        return new Layer(layer);
    }
    return 0;
}

BezierCurve*
Roto::createBezier(double x,double y,double time)
{
    boost::shared_ptr<Bezier>  ret = _ctx->makeBezier(x, y, kRotoBezierBaseName, time,false);
    if (ret) {
        return new BezierCurve(ret);
    }
    return 0;
}

BezierCurve*
Roto::createEllipse(double x,double y,double diameter,bool fromCenter,double time)
{
    boost::shared_ptr<Bezier>  ret = _ctx->makeEllipse(x, y, diameter, fromCenter, time);
    if (ret) {
        return new BezierCurve(ret);
    }
    return 0;
}

BezierCurve*
Roto::createRectangle(double x,double y,double size,double time)
{
    boost::shared_ptr<Bezier>  ret = _ctx->makeSquare(x, y, size, time);
    if (ret) {
        return new BezierCurve(ret);
    }
    return 0;
}