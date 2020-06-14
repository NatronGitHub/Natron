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
#include "Engine/PyItemsTable.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER;
NATRON_PYTHON_NAMESPACE_ENTER;


class BezierCurve
    : public ItemBase
{
public:


    BezierCurve(const BezierPtr& item);

    virtual ~BezierCurve();

    bool isActivated(double frame, const QString& view = QLatin1String(kPyParamViewIdxMain)) const;

    void setCurveFinished(bool finished, const QString& view = QLatin1String(kPyParamViewSetSpecAll));
    bool isCurveFinished(const QString& view = QLatin1String(kPyParamViewIdxMain)) const;

    void addControlPoint(double x, double y, const QString& view = QLatin1String(kPyParamViewSetSpecAll));

    void addControlPointOnSegment(int index, double t, const QString& view = QLatin1String(kPyParamViewSetSpecAll));

    void removeControlPointByIndex(int index, const QString& view = QLatin1String(kPyParamViewSetSpecAll));

    void movePointByIndex(int index, double time, double dx, double dy, const QString& view = QLatin1String(kPyParamViewSetSpecAll));

    void moveFeatherByIndex(int index, double time, double dx, double dy, const QString& view = QLatin1String(kPyParamViewSetSpecAll));

    void moveLeftBezierPoint(int index, double time, double dx, double dy, const QString& view = QLatin1String(kPyParamViewSetSpecAll));

    void moveRightBezierPoint(int index, double time, double dx, double dy, const QString& view = QLatin1String(kPyParamViewSetSpecAll));

    void setPointAtIndex(int index, double time, double x, double y, double lx, double ly, double rx, double ry, const QString& view = QLatin1String(kPyParamViewSetSpecAll));

    void setFeatherPointAtIndex(int index, double time, double x, double y, double lx, double ly, double rx, double ry, const QString& view = QLatin1String(kPyParamViewSetSpecAll));

    int getNumControlPoints(const QString& view = QLatin1String(kPyParamViewIdxMain)) const;

    void getControlPointPosition(int index, double time, double* x, double *y, double *lx, double *ly, double *rx, double *ry, const QString& view = QLatin1String(kPyParamViewIdxMain)) const;

    void getFeatherPointPosition(int index, double time, double* x, double *y, double *lx, double *ly, double *rx, double *ry, const QString& view = QLatin1String(kPyParamViewIdxMain)) const;

    RectD getBoundingBox(double time, const QString& view = QLatin1String(kPyParamViewIdxMain)) const;

    void splitView(const QString& viewName);

    void unSplitView(const QString& viewName);

    QStringList getViewsList() const;
    
private:

    BezierWPtr _bezier;
};

struct StrokePoint
{
    double x, y, pressure, timestamp;
};

class StrokeItem
: public ItemBase
{
    RotoStrokeItemWPtr _stroke;
    
public:

    StrokeItem(const RotoStrokeItemPtr& item);

    virtual ~StrokeItem();

    RectD getBoundingBox(double time, const QString& view = QLatin1String(kPyParamViewIdxMain)) const;

    NATRON_NAMESPACE::RotoStrokeType getBrushType() const;

    std::list<std::list<StrokePoint> > getPoints() const;

    void setPoints(const std::list<std::list<StrokePoint> >& strokes);

};

class Roto : public ItemsTable
{
public:

    Roto(const KnobItemsTablePtr& model);

    virtual ~Roto();

    ItemBase* createLayer();
    ItemBase* createStroke(NATRON_NAMESPACE::RotoStrokeType type);

    BezierCurve* createBezier(double x, double y, double time);
    BezierCurve* createEllipse(double x, double y, double diameter, bool fromCenter, double time);
    BezierCurve* createRectangle(double x, double y, double size, double time);


};

NATRON_PYTHON_NAMESPACE_EXIT;
NATRON_NAMESPACE_EXIT;

#endif // ROTOWRAPPER_H
