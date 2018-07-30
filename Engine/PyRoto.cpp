/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
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

#include "PyRoto.h"

#include <cassert>
#include <stdexcept>

#include "Engine/Bezier.h"
#include "Engine/BezierCP.h"
#include "Engine/EffectInstance.h"
#include "Engine/KnobTypes.h"
#include "Engine/PyNode.h"
#include "Engine/Node.h"
#include "Engine/RotoLayer.h"
#include "Engine/RotoPaint.h"
#include "Engine/RotoPaintPrivate.h"
#include "Engine/AppInstance.h"
#include "Engine/TimeLine.h"
#include "Engine/Project.h"
#include "Engine/RotoPoint.h"
#include "Engine/RotoStrokeItem.h"

NATRON_NAMESPACE_ENTER
NATRON_PYTHON_NAMESPACE_ENTER

BezierCurve::BezierCurve(const BezierPtr& item)
    : ItemBase(item)
    , _bezier(item)
{
}

BezierCurve::~BezierCurve()
{
}

void
BezierCurve::splitView(const QString& viewName)
{
    BezierPtr item = _bezier.lock();
    if (!item) {
        PythonSetNullError();
        return;
    }
    ViewIdx thisViewSpec;
    if (!getViewIdxFromViewName(viewName, &thisViewSpec)) {
        PythonSetInvalidViewName(viewName);
        return;
    }
    if (!item->canSplitViews()) {
        PyErr_SetString(PyExc_ValueError, ItemBase::tr("splitView: Cannot split view for an item that cannot animate").toStdString().c_str());
        return;
    }
    ignore_result(item->splitView(ViewIdx(thisViewSpec.value())));
}

void
BezierCurve::unSplitView(const QString& viewName)
{
    BezierPtr item = _bezier.lock();
    if (!item) {
        PythonSetNullError();
        return;
    }
    ViewIdx thisViewSpec;
    if (!getViewIdxFromViewName(viewName, &thisViewSpec)) {
        PythonSetInvalidViewName(viewName);
        return;
    }
    if (!item->canSplitViews()) {
        PyErr_SetString(PyExc_ValueError, ItemBase::tr("splitView: Cannot split view for an item that cannot animate").toStdString().c_str());
        return;
    }
    ignore_result(item->unSplitView(ViewIdx(thisViewSpec.value())));
}


QStringList
BezierCurve::getViewsList() const
{
    QStringList ret;
    BezierPtr item = _bezier.lock();
    if (!item) {
        PythonSetNullError();
        return ret;
    }

    AppInstancePtr app = item->getApp();
    if (!app) {
        return ret;
    }
    const std::vector<std::string>& projectViews = app->getProject()->getProjectViewNames();
    std::list<ViewIdx> views = item->getViewsList();
    for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
        if (*it < 0 && *it >= (int)projectViews.size()) {
            continue;
        }
        ret.push_back(QString::fromUtf8(projectViews[*it].c_str()));
    }
    return ret;
}

bool
BezierCurve::isActivated(double frame, const QString& view) const
{
    BezierPtr item = _bezier.lock();
    if (!item) {
        PythonSetNullError();
        return false;
    }

    ViewIdx viewSpec;
    if (!getViewIdxFromViewName(view, &viewSpec)) {
        PythonSetInvalidViewName(view);
        return false;
    }
    return item->isActivated(TimeValue(frame), viewSpec);
}

void
BezierCurve::setCurveFinished(bool finished, const QString& view)
{
    BezierPtr item = _bezier.lock();
    if (!item) {
        PythonSetNullError();
        return;
    }

    ViewSetSpec viewSpec;
    if (!getViewSetSpecFromViewName(view, &viewSpec)) {
        PythonSetInvalidViewName(view);
        return;
    }
    item->setCurveFinished(finished, viewSpec);
}

bool
BezierCurve::isCurveFinished(const QString& view) const
{
    BezierPtr item = _bezier.lock();
    if (!item) {
        PythonSetNullError();
        return false;
    }

    ViewIdx viewSpec;
    if (!getViewIdxFromViewName(view, &viewSpec)) {
        PythonSetInvalidViewName(view);
        return false;
    }

    return item->isCurveFinished(viewSpec);
}

void
BezierCurve::addControlPoint(double x,
                             double y, const QString& view)
{
    BezierPtr item = _bezier.lock();
    if (!item) {
        PythonSetNullError();
        return;
    }

    ViewSetSpec viewSpec;
    if (!getViewSetSpecFromViewName(view, &viewSpec)) {
        PythonSetInvalidViewName(view);
        return;
    }

    KeyFrameSet keys = item->getKeyFrames(ViewIdx(0));
    KeyFrame k;
    if (!keys.empty()) {
        k = *keys.begin();
    }
    item->addControlPoint(x, y, k.getTime(), viewSpec);
}

void
BezierCurve::addControlPointOnSegment(int index,
                                      double t, const QString& view)
{
    BezierPtr item = _bezier.lock();
    if (!item) {
        PythonSetNullError();
        return;
    }

    ViewSetSpec viewSpec;
    if (!getViewSetSpecFromViewName(view, &viewSpec)) {
        PythonSetInvalidViewName(view);
        return;
    }

    item->addControlPointAfterIndex(index, t, viewSpec);
}

void
BezierCurve::removeControlPointByIndex(int index, const QString& view)
{
    BezierPtr item = _bezier.lock();
    if (!item) {
        PythonSetNullError();
        return;
    }

    ViewSetSpec viewSpec;
    if (!getViewSetSpecFromViewName(view, &viewSpec)) {
        PythonSetInvalidViewName(view);
        return;
    }
    item->removeControlPointByIndex(index, viewSpec);
}

void
BezierCurve::movePointByIndex(int index,
                              double time,
                              double dx,
                              double dy, const QString& view)
{
    BezierPtr item = _bezier.lock();
    if (!item) {
        PythonSetNullError();
        return;
    }

    ViewSetSpec viewSpec;
    if (!getViewSetSpecFromViewName(view, &viewSpec)) {
        PythonSetInvalidViewName(view);
        return;
    }
    item->movePointByIndex(index, TimeValue(time), viewSpec, dx, dy);
}

void
BezierCurve::moveFeatherByIndex(int index,
                                double time,
                                double dx,
                                double dy, const QString& view)
{
    BezierPtr item = _bezier.lock();
    if (!item) {
        PythonSetNullError();
        return;
    }

    ViewSetSpec viewSpec;
    if (!getViewSetSpecFromViewName(view, &viewSpec)) {
        PythonSetInvalidViewName(view);
        return;
    }
    item->moveFeatherByIndex(index, TimeValue(time), viewSpec, dx, dy);
}

void
BezierCurve::moveLeftBezierPoint(int index,
                                 double time,
                                 double dx,
                                 double dy, const QString& view)
{
    BezierPtr item = _bezier.lock();
    if (!item) {
        PythonSetNullError();
        return;
    }

    ViewSetSpec viewSpec;
    if (!getViewSetSpecFromViewName(view, &viewSpec)) {
        PythonSetInvalidViewName(view);
        return;
    }
    item->moveLeftBezierPoint(index, TimeValue(time), viewSpec, dx, dy);
}

void
BezierCurve::moveRightBezierPoint(int index,
                                  double time,
                                  double dx,
                                  double dy, const QString& view)
{
    BezierPtr item = _bezier.lock();
    if (!item) {
        PythonSetNullError();
        return;
    }

    ViewSetSpec viewSpec;
    if (!getViewSetSpecFromViewName(view, &viewSpec)) {
        PythonSetInvalidViewName(view);
        return;
    }
    item->moveRightBezierPoint(index, TimeValue(time), viewSpec, dx, dy);
}

void
BezierCurve::setPointAtIndex(int index,
                             double time,
                             double x,
                             double y,
                             double lx,
                             double ly,
                             double rx,
                             double ry, const QString& view)
{
    BezierPtr item = _bezier.lock();
    if (!item) {
        PythonSetNullError();
        return;
    }

    ViewSetSpec viewSpec;
    if (!getViewSetSpecFromViewName(view, &viewSpec)) {
        PythonSetInvalidViewName(view);
        return;
    }
    item->setPointAtIndex(false, index, TimeValue(time), viewSpec, x, y, lx, ly, rx, ry);
}

void
BezierCurve::setFeatherPointAtIndex(int index,
                                    double time,
                                    double x,
                                    double y,
                                    double lx,
                                    double ly,
                                    double rx,
                                    double ry, const QString& view)
{
    BezierPtr item = _bezier.lock();
    if (!item) {
        PythonSetNullError();
        return;
    }

    ViewSetSpec viewSpec;
    if (!getViewSetSpecFromViewName(view, &viewSpec)) {
        PythonSetInvalidViewName(view);
        return;
    }
    item->setPointAtIndex(true, index, TimeValue(time), viewSpec, x, y, lx, ly, rx, ry);
}

int
BezierCurve::getNumControlPoints(const QString& view) const
{
    BezierPtr item = _bezier.lock();
    if (!item) {
        PythonSetNullError();
        return 0;
    }

    ViewIdx viewSpec;
    if (!getViewIdxFromViewName(view, &viewSpec)) {
        PythonSetInvalidViewName(view);
        return 0;
    }
    return item->getControlPointsCount(viewSpec);
}


void
BezierCurve::getControlPointPosition(int index,
                                     double time,
                                     double* x,
                                     double *y,
                                     double *lx,
                                     double *ly,
                                     double *rx,
                                     double *ry, const QString& view) const
{
    BezierPtr item = _bezier.lock();
    if (!item) {
        PythonSetNullError();
        return;
    }

    ViewIdx viewSpec;
    if (!getViewIdxFromViewName(view, &viewSpec)) {
        PythonSetInvalidViewName(view);
        return;
    }

    BezierCPPtr cp = item->getControlPointAtIndex(index, viewSpec);
    if (!cp) {
        PyErr_SetString(PyExc_ValueError, ItemBase::tr("Invalid control point index").toStdString().c_str());
        return;
    }
    cp->getPositionAtTime(TimeValue(time), x, y);
    cp->getLeftBezierPointAtTime(TimeValue(time), lx, ly);
    cp->getRightBezierPointAtTime(TimeValue(time), rx, ry);
}

void
BezierCurve::getFeatherPointPosition(int index,
                                     double time,
                                     double* x,
                                     double *y,
                                     double *lx,
                                     double *ly,
                                     double *rx,
                                     double *ry, const QString& view) const
{
    BezierPtr item = _bezier.lock();
    if (!item) {
        PythonSetNullError();
        return;
    }

    ViewIdx viewSpec;
    if (!getViewIdxFromViewName(view, &viewSpec)) {
        PythonSetInvalidViewName(view);
        return;
    }

    BezierCPPtr cp = item->getFeatherPointAtIndex(index, viewSpec);
    if (!cp) {
        PyErr_SetString(PyExc_ValueError, ItemBase::tr("Invalid control point index").toStdString().c_str());
        return;
    }
    cp->getPositionAtTime(TimeValue(time), x, y);
    cp->getLeftBezierPointAtTime(TimeValue(time), lx, ly);
    cp->getRightBezierPointAtTime(TimeValue(time), rx, ry);

}

RectD
BezierCurve::getBoundingBox(double time, const QString& view) const
{
    BezierPtr item = _bezier.lock();
    if (!item) {
        PythonSetNullError();
        return RectD();
    }

    ViewIdx viewSpec;
    if (!getViewIdxFromViewName(view, &viewSpec)) {
        PythonSetInvalidViewName(view);
        return RectD();
    }
    return item->getBoundingBox(TimeValue(time), viewSpec);

}

StrokeItem::StrokeItem(const RotoStrokeItemPtr& item)
: ItemBase(item)
, _stroke(item)
{

}

StrokeItem::~StrokeItem()
{

}

RectD
StrokeItem::getBoundingBox(double time, const QString& view) const
{
    RotoStrokeItemPtr item = _stroke.lock();
    if (!item) {
        PythonSetNullError();
        return RectD();
    }

    ViewIdx viewSpec;
    if (!getViewIdxFromViewName(view, &viewSpec)) {
        PythonSetInvalidViewName(view);
        return RectD();
    }
    return item->getBoundingBox(TimeValue(time), viewSpec);
}

NATRON_NAMESPACE::RotoStrokeType
StrokeItem::getBrushType() const
{
    RotoStrokeItemPtr item = _stroke.lock();
    if (!item) {
        PythonSetNullError();
        return eRotoStrokeTypeSolid;
    }
    return item->getBrushType();
}

std::list<std::list<StrokePoint> >
StrokeItem::getPoints() const
{
    std::list<std::list<StrokePoint> > ret;
    RotoStrokeItemPtr item = _stroke.lock();
    if (!item) {
        PythonSetNullError();
        return ret;
    }
    std::list<std::list<std::pair<Point, double> > > strokes;
    item->evaluateStroke(0, TimeValue(0), ViewIdx(0), &strokes);
    for (std::list<std::list<std::pair<Point, double> > >::const_iterator it = strokes.begin(); it != strokes.end(); ++it) {
        int i = 0;
        std::list<StrokePoint> subStroke;
        for (std::list<std::pair<Point, double> >::const_iterator it2 = it->begin(); it2 != it->end(); ++it2, ++i) {
            StrokePoint p;
            p.x = it2->first.x;
            p.y = it2->first.y;
            p.pressure = it2->second;
            p.timestamp = i;
            subStroke.push_back(p);
        }
        ret.push_back(subStroke);
    }
    return ret;
}

void
StrokeItem::setPoints(const std::list<std::list<StrokePoint> >& strokes)
{
    RotoStrokeItemPtr item = _stroke.lock();
    if (!item) {
        PythonSetNullError();
        return;
    }
    std::list<std::list<RotoPoint> > ret;
    for (std::list<std::list<StrokePoint> >::const_iterator it = strokes.begin(); it!=strokes.end(); ++it) {
        std::list<RotoPoint> subStroke;
        for (std::list<StrokePoint>::const_iterator it2 = it->begin(); it2 != it->end(); ++it2) {
            RotoPoint p(it2->x, it2->y, it2->pressure, TimeValue(it2->timestamp));
            subStroke.push_back(p);
        }
        ret.push_back(subStroke);
    }
    item->setStrokes(ret);
}

Roto::Roto(const KnobItemsTablePtr& model)
: ItemsTable(model)
{
}

Roto::~Roto()
{
}


ItemBase*
Roto::createLayer()
{
    KnobItemsTablePtr model = getInternalModel();
    if (!model) {
        PythonSetNullError();
        return 0;
    }
    NodePtr node = model->getNode();
    if (!node) {
        PythonSetNullError();
        return 0;
    }
    RotoPaintPtr isRotoPaint = toRotoPaint(node->getEffectInstance());
    if (!isRotoPaint) {
        PythonSetNullError();
        return 0;
    }
    RotoLayerPtr  layer = isRotoPaint->addLayer();

    if (layer) {
        return createPyItemWrapper(layer);
    }

    return 0;
}

BezierCurve*
Roto::createBezier(double x,
                   double y,
                   double time)
{
    KnobItemsTablePtr model = getInternalModel();
    if (!model) {
        PythonSetNullError();
        return 0;
    }
    NodePtr node = model->getNode();
    if (!node) {
        PythonSetNullError();
        return 0;
    }
    RotoPaintPtr isRotoPaint = toRotoPaint(node->getEffectInstance());
    if (!isRotoPaint) {
        PythonSetNullError();
        return 0;
    }
    BezierPtr  bezier = isRotoPaint->makeBezier(x, y, kRotoBezierBaseName, TimeValue(time), false);

    if (bezier) {
        return (BezierCurve*)createPyItemWrapper(bezier);
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
    KnobItemsTablePtr model = getInternalModel();
    if (!model) {
        PythonSetNullError();
        return 0;
    }
    NodePtr node = model->getNode();
    if (!node) {
        PythonSetNullError();
        return 0;
    }
    RotoPaintPtr isRotoPaint = toRotoPaint(node->getEffectInstance());
    if (!isRotoPaint) {
        PythonSetNullError();
        return 0;
    }
    BezierPtr  bezier = isRotoPaint->makeEllipse(x, y, diameter, fromCenter, TimeValue(time));

    if (bezier) {
        return (BezierCurve*)createPyItemWrapper(bezier);
    }

    return 0;


}

BezierCurve*
Roto::createRectangle(double x,
                      double y,
                      double size,
                      double time)
{
    KnobItemsTablePtr model = getInternalModel();
    if (!model) {
        PythonSetNullError();
        return 0;
    }
    NodePtr node = model->getNode();
    if (!node) {
        PythonSetNullError();
        return 0;
    }
    RotoPaintPtr isRotoPaint = toRotoPaint(node->getEffectInstance());
    if (!isRotoPaint) {
        PythonSetNullError();
        return 0;
    }
    BezierPtr  bezier = isRotoPaint->makeSquare(x, y, size, TimeValue(time));

    if (bezier) {
        return (BezierCurve*)createPyItemWrapper(bezier);
    }

    return 0;
}


ItemBase*
Roto::createStroke(NATRON_NAMESPACE::RotoStrokeType type)
{
    KnobItemsTablePtr model = getInternalModel();
    if (!model) {
        PythonSetNullError();
        return 0;
    }
    NodePtr node = model->getNode();
    if (!node) {
        PythonSetNullError();
        return 0;
    }
    RotoPaintPtr isRotoPaint = toRotoPaint(node->getEffectInstance());
    if (!isRotoPaint) {
        PythonSetNullError();
        return 0;
    }
    RotoLayerPtr baseLayer = isRotoPaint->getOrCreateBaseLayer();
    RotoStrokeItemPtr stroke(new RotoStrokeItem(type, model));
    model->insertItem(0, stroke, baseLayer, eTableChangeReasonInternal);
    
    return createPyItemWrapper(stroke);
}


NATRON_PYTHON_NAMESPACE_EXIT


NATRON_NAMESPACE_EXIT
