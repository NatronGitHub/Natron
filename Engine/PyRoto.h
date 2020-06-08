/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 * Copyright (C) 2018-2020 The Natron developers
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

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#endif

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

    void setCompositingOperator(NATRON_NAMESPACE::MergingFunctionEnum op);
    NATRON_NAMESPACE::MergingFunctionEnum getCompositingOperator() const;
    BooleanParam* getActivatedParam() const;
    DoubleParam* getOpacityParam() const;
    DoubleParam* getFeatherDistanceParam() const;
    DoubleParam* getFeatherFallOffParam() const;
    ColorParam* getColorParam() const;
    ChoiceParam* getCompositingOperatorParam() const;

private:

    BezierPtr _bezier;
};

class Roto
{
public:

    Roto(const RotoContextPtr& ctx);

    ~Roto();

    Layer* getBaseLayer() const;
    ItemBase* getItemByName(const QString& name) const;
    Layer* createLayer();
    BezierCurve* createBezier(double x, double y, double time);
    BezierCurve* createEllipse(double x, double y, double diameter, bool fromCenter, double time);
    BezierCurve* createRectangle(double x, double y, double size, double time);

private:

    RotoContextPtr _ctx;
};

NATRON_PYTHON_NAMESPACE_EXIT;
NATRON_NAMESPACE_EXIT;

#endif // ROTOWRAPPER_H
