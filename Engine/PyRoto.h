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

#ifndef ROTOWRAPPER_H
#define ROTOWRAPPER_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <list>
#include <vector>

#include "Engine/MergingEnum.h"
#include "Engine/PyParameter.h"
#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER;
NATRON_PYTHON_NAMESPACE_ENTER;

class Layer; // defined below

class ItemBase
{
public:

    ItemBase(const RotoItemPtr& item);

    virtual ~ItemBase();

    RotoItemPtr getInternalItem() const
    {
        return _item;
    }

    void setLabel(const QString & name);
    QString getLabel() const;

    bool setScriptName(const QString& name);
    QString getScriptName() const;

    void setLocked(bool locked);
    bool getLocked() const;
    bool getLockedRecursive() const;

    void setVisible(bool activated);
    bool getVisible() const;

    Layer* getParentLayer() const;
    Param* getParam(const QString& name) const;

private:

    RotoItemPtr _item;
};

class Layer
    : public ItemBase
{
public:

    Layer(const RotoItemPtr& item);

    virtual ~Layer();

    void addItem(ItemBase* item);

    void insertItem(int pos, ItemBase* item);

    void removeItem(ItemBase* item);

    std::list<ItemBase*> getChildren() const;

private:

    RotoLayerPtr _layer;
};

class BezierCurve
    : public ItemBase
{
public:


    BezierCurve(const RotoItemPtr& item);

    virtual ~BezierCurve();


    void setCurveFinished(bool finished);
    bool isCurveFinished() const;

    void addControlPoint(double x, double y);

    void addControlPointOnSegment(int index, double t);

    void removeControlPointByIndex(int index);

    void movePointByIndex(int index, double time, double dx, double dy);

    void moveFeatherByIndex(int index, double time, double dx, double dy);

    void moveLeftBezierPoint(int index, double time, double dx, double dy);

    void moveRightBezierPoint(int index, double time, double dx, double dy);

    void setPointAtIndex(int index, double time, double x, double y, double lx, double ly, double rx, double ry);

    void setFeatherPointAtIndex(int index, double time, double x, double y, double lx, double ly, double rx, double ry);

    int getNumControlPoints() const;

    void getKeyframes(std::list<double>* keys) const;

    // The interpolation of the shape's keyframe at time, for all its control and feather points: false (and
    // eKeyframeTypeNone for the getter) if there is no keyframe at time.
    bool setKeyframeInterpolation(double time, Natron::KeyframeTypeEnum interpolation);
    Natron::KeyframeTypeEnum getKeyframeInterpolation(double time) const;

    void getControlPointPosition(int index, double time, double* x, double *y, double *lx, double *ly, double *rx, double *ry) const;

    void getFeatherPointPosition(int index, double time, double* x, double *y, double *lx, double *ly, double *rx, double *ry) const;

    void setActivated(double time, bool activated);
    bool getIsActivated(double time);

    void setOpacity(double opacity, double time);
    double getOpacity(double time) const;

    ColorTuple getOverlayColor() const;
    void setOverlayColor(double r, double g, double b);

    double getFeatherDistance(double time) const;
    void setFeatherDistance(double dist, double time);

    double getFeatherFallOff(double time) const;
    void setFeatherFallOff(double falloff, double time);

    ColorTuple getColor(double time);
    void setColor(double time, double r, double g, double b);

    void setCompositingOperator(Natron::MergingFunctionEnum op);
    Natron::MergingFunctionEnum getCompositingOperator() const;
    BooleanParam* getActivatedParam() const;
    DoubleParam* getOpacityParam() const;
    DoubleParam* getFeatherDistanceParam() const;
    DoubleParam* getFeatherFallOffParam() const;
    ColorParam* getColorParam() const;
    ChoiceParam* getCompositingOperatorParam() const;

private:

    BezierPtr _bezier;
};

// A paint stroke of a RotoPaint node: one or more continuous lines (the "strokes" of the item), each made of
// points (x, y, pressure, timestamp), with the parameters of the stroke (brush size, color...).
class StrokeItem
    : public ItemBase
{
public:

    StrokeItem(const RotoItemPtr& item);

    virtual ~StrokeItem();

    Natron::RotoStrokeType getBrushType() const;

    int getNumStrokes() const;

    // The points (x, y, pressure, timestamp) of the line at index: none if there is no such line
    void getPoints(int index, std::list<std::vector<double> >* points) const;

    // Adds a line made of points (x, y, pressure, timestamp), interpolated as a painted line (Catmull-Rom).
    // The timestamps are in seconds from the first point. False, and nothing added, if points is empty or its
    // timestamps don't increase. Resets the center of the stroke (RotoStrokeItem::setStrokeFinished()).
    bool addStroke(const std::list<std::vector<double> >& points);

private:

    RotoStrokeItemPtr _stroke;
};

class Roto
{
public:

    Roto(const RotoContextPtr& ctx);

    ~Roto();

    Layer* getBaseLayer() const;
    ItemBase* getItemByName(const QString& name) const;
    std::list<ItemBase*> getSelectedItems() const;
    Layer* createLayer();
    BezierCurve* createBezier(double x, double y, double time);
    BezierCurve* createEllipse(double x, double y, double diameter, bool fromCenter, double time);
    BezierCurve* createRectangle(double x, double y, double size, double time);
    // A paint stroke with no line yet (see StrokeItem::addStroke()), at the top of the selected layer (a RotoPaint node)
    StrokeItem* createStroke(Natron::RotoStrokeType type);

private:

    RotoContextPtr _ctx;
};

NATRON_PYTHON_NAMESPACE_EXIT;
NATRON_NAMESPACE_EXIT;

#endif // ROTOWRAPPER_H
