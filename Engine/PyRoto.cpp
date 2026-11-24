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

#include "PyRoto.h"

#include <cassert>
#include <stdexcept>

#include "Engine/Bezier.h"
#include "Engine/BezierCP.h"
#include "Engine/EffectInstance.h"
#include "Engine/KnobTypes.h"
#include "Engine/PyNode.h"
#include "Engine/Node.h"
#include "Engine/RotoContext.h"
#include "Engine/RotoLayer.h"
#include "Engine/RotoStrokeItem.h"

NATRON_NAMESPACE_ENTER
NATRON_PYTHON_NAMESPACE_ENTER

ItemBase::ItemBase(const RotoItemPtr& item)
    : _item(item)
{
}

ItemBase::~ItemBase()
{
}

void
ItemBase::setLabel(const QString & name)
{
    _item->setLabel( name.toStdString() );
}

QString
ItemBase::getLabel() const
{
    return QString::fromUtf8( _item->getLabel().c_str() );
}

bool
ItemBase::setScriptName(const QString& name)
{
    return _item->setScriptName( name.toStdString() );
}

QString
ItemBase::getScriptName() const
{
    return QString::fromUtf8( _item->getScriptName().c_str() );
}

void
ItemBase::setLocked(bool locked)
{
    _item->setLocked(locked, true, RotoItem::eSelectionReasonOther);
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
    RotoLayerPtr layer =  _item->getParentLayer();

    if (layer) {
        return new Layer(layer);
    } else {
        return 0;
    }
}

Param*
ItemBase::getParam(const QString& name) const
{
    RotoDrawableItem* drawable = dynamic_cast<RotoDrawableItem*>( _item.get() );

    if (!drawable) {
        return 0;
    }
    KnobIPtr knob = drawable->getKnobByName( name.toStdString() );
    if (!knob) {
        return 0;
    }
    Param* ret = Effect::createParamWrapperForKnob(knob);

    return ret;
}

Layer::Layer(const RotoItemPtr& item)
    : ItemBase(item)
    , _layer( std::dynamic_pointer_cast<RotoLayer>(item) )
{
}

Layer::~Layer()
{
}

// Adds item to layer at index (at the end if index < 0).
// An item that is in no layer, e.g. after Layer.removeItem(), is added back as the Roto panel's undo does, with
// RotoContext::addItem(): it registers a layer in the context again, and tells the Roto panel (itemInserted).
// An item that is in a layer is moved, as before.
static void
addItemToLayer(const RotoLayerPtr& layer,
               ItemBase* item,
               int index)
{
    // item is NULL if Python passed None
    RotoItemPtr internalItem = item ? item->getInternalItem() : RotoItemPtr();

    if (!internalItem) {
        return;
    }
    if ( internalItem->getParentLayer() ) {
        if (index < 0) {
            layer->addItem(internalItem);
        } else {
            layer->insertItem(internalItem, index);
        }

        return;
    }
    RotoContextPtr ctx = layer->getContext();
    ctx->addItem(layer, index < 0 ? (int)layer->getItems().size() : index, internalItem, RotoItem::eSelectionReasonOther);
    ctx->evaluateChange();
}

void
Layer::addItem(ItemBase* item)
{
    addItemToLayer(_layer, item, -1);
}

void
Layer::insertItem(int pos,
                  ItemBase* item)
{
    addItemToLayer(_layer, item, pos);
}

void
Layer::removeItem(ItemBase* item)
{
    // item is NULL if Python passed None
    RotoItemPtr internalItem = item ? item->getInternalItem() : RotoItemPtr();

    // Only the children of this layer are removed, as before
    if ( !internalItem || (internalItem->getParentLayer() != _layer) ) {
        return;
    }
    // Remove it as the Roto panel does: RotoContext::removeItem() also deselects the item and its children,
    // and tells the Roto panel and the curve editor, which otherwise kept showing an empty item (#405).
    RotoContextPtr ctx = internalItem->getContext();
    ctx->removeItem(internalItem, RotoItem::eSelectionReasonOther);
    ctx->evaluateChange();
}

std::list<ItemBase*>
Layer::getChildren() const
{
    std::list<ItemBase*> ret;
    std::list<RotoItemPtr> items = _layer->getItems_mt_safe();

    for (std::list<RotoItemPtr>::iterator it = items.begin(); it != items.end(); ++it) {
        ret.push_back( new ItemBase(*it) );
    }

    return ret;
}

BezierCurve::BezierCurve(const RotoItemPtr& item)
    : ItemBase(item)
    , _bezier( std::dynamic_pointer_cast<Bezier>(item) )
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
BezierCurve::addControlPoint(double x,
                             double y)
{
    const std::list<BezierCPPtr>& cps = _bezier->getControlPoints();
    double keyframeTime;

    if ( !cps.empty() ) {
        keyframeTime = cps.front()->getKeyframeTime(false, 0);
    } else {
        keyframeTime = _bezier->getContext()->getTimelineCurrentTime();
    }
    _bezier->addControlPoint(x, y, keyframeTime);
}

void
BezierCurve::addControlPointOnSegment(int index,
                                      double t)
{
    _bezier->addControlPointAfterIndex(index, t);
}

void
BezierCurve::removeControlPointByIndex(int index)
{
    _bezier->removeControlPointByIndex(index);
}

void
BezierCurve::movePointByIndex(int index,
                              double time,
                              double dx,
                              double dy)
{
    _bezier->movePointByIndex(index, time, dx, dy);
}

void
BezierCurve::moveFeatherByIndex(int index,
                                double time,
                                double dx,
                                double dy)
{
    _bezier->moveFeatherByIndex(index, time, dx, dy);
}

void
BezierCurve::moveLeftBezierPoint(int index,
                                 double time,
                                 double dx,
                                 double dy)
{
    _bezier->moveLeftBezierPoint(index, time, dx, dy);
}

void
BezierCurve::moveRightBezierPoint(int index,
                                  double time,
                                  double dx,
                                  double dy)
{
    _bezier->moveRightBezierPoint(index, time, dx, dy);
}

void
BezierCurve::setPointAtIndex(int index,
                             double time,
                             double x,
                             double y,
                             double lx,
                             double ly,
                             double rx,
                             double ry)
{
    _bezier->setPointAtIndex(false, index, time, x, y, lx, ly, rx, ry);
}

void
BezierCurve::setFeatherPointAtIndex(int index,
                                    double time,
                                    double x,
                                    double y,
                                    double lx,
                                    double ly,
                                    double rx,
                                    double ry)
{
    _bezier->setPointAtIndex(true, index, time, x, y, lx, ly, rx, ry);
}

int
BezierCurve::getNumControlPoints() const
{
    return _bezier->getControlPointsCount();
}

void
BezierCurve::getKeyframes(std::list<double>* keys) const
{
    std::set<double> keyframes;

    _bezier->getKeyframeTimes(&keyframes);
    for (std::set<double>::iterator it = keyframes.begin(); it != keyframes.end(); ++it) {
        keys->push_back(*it);
    }
}

void
BezierCurve::getControlPointPosition(int index,
                                     double time,
                                     double* x,
                                     double *y,
                                     double *lx,
                                     double *ly,
                                     double *rx,
                                     double *ry) const
{
    BezierCPPtr cp = _bezier->getControlPointAtIndex(index);

    cp->getPositionAtTime(true, time, ViewIdx(0), x, y);
    cp->getLeftBezierPointAtTime(true, time, ViewIdx(0), lx, ly);
    cp->getRightBezierPointAtTime(true, time, ViewIdx(0), rx, ry);
}

void
BezierCurve::getFeatherPointPosition(int index,
                                     double time,
                                     double* x,
                                     double *y,
                                     double *lx,
                                     double *ly,
                                     double *rx,
                                     double *ry) const
{
    BezierCPPtr cp = _bezier->getFeatherPointAtIndex(index);

    cp->getPositionAtTime(true, time, ViewIdx(0), x, y);
    cp->getLeftBezierPointAtTime(true, time, ViewIdx(0), lx, ly);
    cp->getRightBezierPointAtTime(true, time, ViewIdx(0), rx, ry);
}

void
BezierCurve::setActivated(double time,
                          bool activated)
{
    _bezier->setActivated(activated, time);
}

bool
BezierCurve::getIsActivated(double time)
{
    return _bezier->isActivated(time);
}

void
BezierCurve::setOpacity(double opacity,
                        double time)
{
    _bezier->setOpacity(opacity, time);
}

double
BezierCurve::getOpacity(double time) const
{
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
BezierCurve::setOverlayColor(double r,
                             double g,
                             double b)
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
BezierCurve::setFeatherDistance(double dist,
                                double time)
{
    _bezier->setFeatherDistance(dist, time);
}

double
BezierCurve::getFeatherFallOff(double time) const
{
    return _bezier->getFeatherFallOff(time);
}

void
BezierCurve::setFeatherFallOff(double falloff,
                               double time)
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
BezierCurve::setColor(double time,
                      double r,
                      double g,
                      double b)
{
    _bezier->setColor(time, r, g, b);
}

void
BezierCurve::setCompositingOperator(MergingFunctionEnum op)
{
    _bezier->setCompositingOperator( (int)op );
}

MergingFunctionEnum
BezierCurve::getCompositingOperator() const
{
    return (MergingFunctionEnum)_bezier->getCompositingOperator();
}

BooleanParam*
BezierCurve::getActivatedParam() const
{
    KnobBoolPtr ret = _bezier->getActivatedKnob();

    if (ret) {
        return new BooleanParam(ret);
    }

    return 0;
}

DoubleParam*
BezierCurve::getOpacityParam() const
{
    KnobDoublePtr ret = _bezier->getOpacityKnob();

    if (ret) {
        return new DoubleParam(ret);
    }

    return 0;
}

DoubleParam*
BezierCurve::getFeatherDistanceParam() const
{
    KnobDoublePtr ret = _bezier->getFeatherKnob();

    if (ret) {
        return new DoubleParam(ret);
    }

    return 0;
}

DoubleParam*
BezierCurve::getFeatherFallOffParam() const
{
    KnobDoublePtr ret = _bezier->getFeatherFallOffKnob();

    if (ret) {
        return new DoubleParam(ret);
    }

    return 0;
}

ColorParam*
BezierCurve::getColorParam() const
{
    KnobColorPtr ret = _bezier->getColorKnob();

    if (ret) {
        return new ColorParam(ret);
    }

    return 0;
}

ChoiceParam*
BezierCurve::getCompositingOperatorParam() const
{
    KnobChoicePtr ret = _bezier->getOperatorKnob();

    if (ret) {
        return new ChoiceParam(ret);
    }

    return 0;
}

Roto::Roto(const RotoContextPtr& ctx)
    : _ctx(ctx)
{
}

Roto::~Roto()
{
}

Layer*
Roto::getBaseLayer() const
{
    const std::list<RotoLayerPtr>& layers = _ctx->getLayers();

    if ( !layers.empty() ) {
        return new Layer( layers.front() );
    }

    return 0;
}

// Returns a new wrapper of the type of item: Layer, BezierCurve, or ItemBase for strokes
static ItemBase*
createItemWrapper(const RotoItemPtr& item)
{
    if (!item) {
        return 0;
    }
    RotoLayer* isLayer = dynamic_cast<RotoLayer*>( item.get() );
    if (isLayer) {
        return new Layer(item);
    }
    Bezier* isBezier = dynamic_cast<Bezier*>( item.get() );
    if (isBezier) {
        return new BezierCurve(item);
    }
    RotoStrokeItem* isStroke = dynamic_cast<RotoStrokeItem*>( item.get() );
    if (isStroke) {
        return new ItemBase(item);
    }

    return 0;
}

ItemBase*
Roto::getItemByName(const QString& name) const
{
    return createItemWrapper( _ctx->getItemByName( name.toStdString() ) );
}

std::list<ItemBase*>
Roto::getSelectedItems() const
{
    std::list<ItemBase*> ret;
    // A copy of the selection, which getSelectedItems() returns by reference
    const std::list<RotoItemPtr> items = _ctx->getSelectedItems();

    for (std::list<RotoItemPtr>::const_iterator it = items.begin(); it != items.end(); ++it) {
        ItemBase* item = createItemWrapper(*it);
        if (item) {
            ret.push_back(item);
        }
    }

    return ret;
}

Layer*
Roto::createLayer()
{
    RotoLayerPtr  layer = _ctx->addLayer();

    if (layer) {
        return new Layer(layer);
    }

    return 0;
}

BezierCurve*
Roto::createBezier(double x,
                   double y,
                   double time)
{
    BezierPtr  ret = _ctx->makeBezier(x, y, kRotoBezierBaseName, time, false);

    if (ret) {
        return new BezierCurve(ret);
    }

    return 0;
}

BezierCurve*
Roto::createEllipse(double x,
                    double y,
                    double diameter,
                    bool fromCenter,
                    double time)
{
    BezierPtr  ret = _ctx->makeEllipse(x, y, diameter, fromCenter, time);

    if (ret) {
        return new BezierCurve(ret);
    }

    return 0;
}

BezierCurve*
Roto::createRectangle(double x,
                      double y,
                      double size,
                      double time)
{
    BezierPtr  ret = _ctx->makeSquare(x, y, size, time);

    if (ret) {
        return new BezierCurve(ret);
    }

    return 0;
}

NATRON_PYTHON_NAMESPACE_EXIT
NATRON_NAMESPACE_EXIT
