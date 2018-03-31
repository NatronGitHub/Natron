/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
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

#ifndef ROTOUNDOCOMMAND_H
#define ROTOUNDOCOMMAND_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <list>
#include <map>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#endif

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QCoreApplication>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/EngineFwd.h"
#include "Engine/UndoCommand.h"


NATRON_NAMESPACE_ENTER

struct RotoPaintInteract;


class MoveControlPointsUndoCommand
    : public UndoCommand
{
    Q_DECLARE_TR_FUNCTIONS(MoveControlPointsUndoCommand)

public:

    MoveControlPointsUndoCommand(const boost::shared_ptr<RotoPaintInteract>& roto,
                                 const std::list<std::pair<boost::shared_ptr<BezierCP>, boost::shared_ptr<BezierCP> > > & toDrag
                                 ,
                                 double dx,
                                 double dy,
                                 double time);

    virtual ~MoveControlPointsUndoCommand();

    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;
    virtual bool mergeWith(const UndoCommandPtr& other) OVERRIDE FINAL;

private:

    bool _firstRedoCalled; //< false by default
    RotoPaintInteractWPtr _roto;
    double _dx, _dy;
    bool _featherLinkEnabled;
    bool _rippleEditEnabled;
    KnobButtonWPtr _selectedTool; //< corresponds to the RotoGui::RotoToolEnum enum
    double _time; //< the time at which the change was made
    std::list<boost::shared_ptr<RotoDrawableItem> > _selectedCurves;
    std::list<int> _indexesToMove; //< indexes of the control points
    std::list<std::pair<boost::shared_ptr<BezierCP>, boost::shared_ptr<BezierCP> > > _originalPoints, _selectedPoints, _pointsToDrag;
};


class TransformUndoCommand
    : public UndoCommand
{
    Q_DECLARE_TR_FUNCTIONS(TransformUndoCommand)

public:

    enum TransformPointsSelectionEnum
    {
        eTransformAllPoints = 0,
        eTransformMidRight,
        eTransformMidBottom,
        eTransformMidLeft,
        eTransformMidTop
    };

    TransformUndoCommand(const boost::shared_ptr<RotoPaintInteract>& roto,
                         double centerX,
                         double centerY,
                         double rot,
                         double skewX,
                         double skewY,
                         double tx,
                         double ty,
                         double sx,
                         double sy,
                         double time);

    virtual ~TransformUndoCommand();

    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;
    virtual bool mergeWith(const UndoCommandPtr& other) OVERRIDE FINAL;

private:

    void transformPoint(const boost::shared_ptr<BezierCP> & point);

    bool _firstRedoCalled; //< false by default
    RotoPaintInteractWPtr _roto;
    bool _rippleEditEnabled;
    KnobButtonWPtr _selectedTool; //< corresponds to the RotoGui::RotoToolEnum enum
    boost::shared_ptr<Transform::Matrix3x3> _matrix;
    double _time; //< the time at which the change was made
    std::list<boost::shared_ptr<RotoDrawableItem> > _selectedCurves;
    std::list<std::pair<boost::shared_ptr<BezierCP>, boost::shared_ptr<BezierCP> > > _originalPoints, _selectedPoints;
};

class AddPointUndoCommand
    : public UndoCommand
{
    Q_DECLARE_TR_FUNCTIONS(AddPointUndoCommand)

public:

    AddPointUndoCommand(const boost::shared_ptr<RotoPaintInteract>& roto,
                        const boost::shared_ptr<Bezier> & curve,
                        int index,
                        double t);

    virtual ~AddPointUndoCommand();

    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;

private:

    bool _firstRedoCalled; //< false by default
    RotoPaintInteractWPtr _roto;
    boost::shared_ptr<Bezier> _curve;
    int _index;
    double _t;
};


class RemovePointUndoCommand
    : public UndoCommand
{
    Q_DECLARE_TR_FUNCTIONS(RemovePointUndoCommand)

private:
    struct CurveDesc
    {
        boost::shared_ptr<RotoDrawableItem> oldCurve, curve;
        std::list<int> points;
        boost::shared_ptr<RotoLayer> parentLayer;
        bool curveRemoved;
        int indexInLayer;
    };

public:

    RemovePointUndoCommand(const boost::shared_ptr<RotoPaintInteract>& roto,
                           const boost::shared_ptr<Bezier> & curve,
                           const boost::shared_ptr<BezierCP> & cp);

    RemovePointUndoCommand(const boost::shared_ptr<RotoPaintInteract>& roto,
                           const std::list<std::pair < boost::shared_ptr<BezierCP>, boost::shared_ptr<BezierCP> > > & points);

    virtual ~RemovePointUndoCommand();

    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;

private:
    RotoPaintInteractWPtr _roto;
    struct CurveOrdering
    {
        bool operator() (const CurveDesc & lhs,
                         const CurveDesc & rhs)
        {
            return lhs.indexInLayer < rhs.indexInLayer;
        }
    };

    bool _firstRedoCalled;
    std::list<CurveDesc > _curves;
};


class RemoveCurveUndoCommand
    : public UndoCommand
{
    Q_DECLARE_TR_FUNCTIONS(RemoveCurveUndoCommand)

private:
    struct RemovedCurve
    {
        boost::shared_ptr<RotoDrawableItem> curve;
        boost::shared_ptr<RotoLayer> layer;
        int indexInLayer;
    };

public:


    RemoveCurveUndoCommand(const boost::shared_ptr<RotoPaintInteract>& roto,
                           const std::list<boost::shared_ptr<RotoDrawableItem> > & curves);

    virtual ~RemoveCurveUndoCommand();

    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;

private:
    RotoPaintInteractWPtr _roto;
    bool _firstRedoCalled;
    std::list<RemovedCurve> _curves;
};

class AddStrokeUndoCommand
    : public UndoCommand
{
    Q_DECLARE_TR_FUNCTIONS(AddStrokeUndoCommand)

public:

    AddStrokeUndoCommand(const boost::shared_ptr<RotoPaintInteract>& roto,
                         const boost::shared_ptr<RotoStrokeItem>& item);

    virtual ~AddStrokeUndoCommand();
    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;

private:

    RotoPaintInteractWPtr _roto;
    bool _firstRedoCalled;
    boost::shared_ptr<RotoStrokeItem> _item;
    boost::shared_ptr<RotoLayer> _layer;
    int _indexInLayer;
};

class AddMultiStrokeUndoCommand
    : public UndoCommand
{
    Q_DECLARE_TR_FUNCTIONS(AddMultiStrokeUndoCommand)

public:

    AddMultiStrokeUndoCommand(const boost::shared_ptr<RotoPaintInteract>& roto,
                              const boost::shared_ptr<RotoStrokeItem>& item);

    virtual ~AddMultiStrokeUndoCommand();
    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;

private:

    RotoPaintInteractWPtr _roto;
    bool _firstRedoCalled;
    boost::shared_ptr<RotoStrokeItem> _item;
    boost::shared_ptr<RotoLayer> _layer;
    int _indexInLayer;
    boost::shared_ptr<Curve> _xCurve, _yCurve, _pCurve;
    bool isRemoved;
};


class MoveTangentUndoCommand
    : public UndoCommand
{
    Q_DECLARE_TR_FUNCTIONS(MoveTangentUndoCommand)

public:

    MoveTangentUndoCommand(const boost::shared_ptr<RotoPaintInteract>& roto,
                           double dx,
                           double dy,
                           double time,
                           const boost::shared_ptr<BezierCP> & cp,
                           bool left,
                           bool breakTangents);

    virtual ~MoveTangentUndoCommand();

    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;
    virtual bool mergeWith(const UndoCommandPtr& other) OVERRIDE FINAL;

private:

    bool _firstRedoCalled; //< false by default
    RotoPaintInteractWPtr _roto;
    double _dx, _dy;
    bool _featherLinkEnabled;
    bool _rippleEditEnabled;
    double _time; //< the time at which the change was made
    std::list<boost::shared_ptr<RotoDrawableItem> > _selectedCurves;
    std::list<std::pair<boost::shared_ptr<BezierCP>, boost::shared_ptr<BezierCP> > > _selectedPoints;
    boost::shared_ptr<BezierCP> _tangentBeingDragged, _oldCp, _oldFp;
    bool _left;
    bool _breakTangents;
};


class MoveFeatherBarUndoCommand
    : public UndoCommand
{
    Q_DECLARE_TR_FUNCTIONS(MoveFeatherBarUndoCommand)

public:

    MoveFeatherBarUndoCommand(const boost::shared_ptr<RotoPaintInteract>& roto,
                              double dx,
                              double dy,
                              const std::pair<boost::shared_ptr<BezierCP>, boost::shared_ptr<BezierCP> > & point,
                              double time);

    virtual ~MoveFeatherBarUndoCommand();

    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;
    virtual bool mergeWith(const UndoCommandPtr& other) OVERRIDE FINAL;

private:

    RotoPaintInteractWPtr _roto;
    bool _firstRedoCalled;
    double _dx, _dy;
    bool _rippleEditEnabled;
    double _time; //< the time at which the change was made
    boost::shared_ptr<Bezier> _curve;
    std::pair<boost::shared_ptr<BezierCP>, boost::shared_ptr<BezierCP> > _oldPoint, _newPoint;
};


class RemoveFeatherUndoCommand
    : public UndoCommand
{
    Q_DECLARE_TR_FUNCTIONS(RemoveFeatherUndoCommand)

public:

    struct RemoveFeatherData
    {
        boost::shared_ptr<Bezier> curve;
        std::list<boost::shared_ptr<BezierCP> > oldPoints, newPoints;
    };


    RemoveFeatherUndoCommand(const boost::shared_ptr<RotoPaintInteract>& roto,
                             const std::list<RemoveFeatherData> & datas);

    virtual ~RemoveFeatherUndoCommand();

    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;

private:
    RotoPaintInteractWPtr _roto;
    bool _firstRedocalled;
    std::list<RemoveFeatherData> _datas;
};

class OpenCloseUndoCommand
    : public UndoCommand
{
    Q_DECLARE_TR_FUNCTIONS(OpenCloseUndoCommand)

public:

    OpenCloseUndoCommand(const boost::shared_ptr<RotoPaintInteract>& roto,
                         const boost::shared_ptr<Bezier> & curve);

    virtual ~OpenCloseUndoCommand();

    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;

private:

    RotoPaintInteractWPtr _roto;
    bool _firstRedoCalled;
    KnobButtonWPtr _selectedTool;
    boost::shared_ptr<Bezier> _curve;
};


class SmoothCuspUndoCommand
    : public UndoCommand
{
    Q_DECLARE_TR_FUNCTIONS(SmoothCuspUndoCommand)

public:

    typedef std::list<std::pair<boost::shared_ptr<BezierCP>, boost::shared_ptr<BezierCP> > > SelectedPointList;
    struct SmoothCuspCurveData
    {
        boost::shared_ptr<Bezier> curve;
        SelectedPointList newPoints, oldPoints;
    };

    SmoothCuspUndoCommand(const boost::shared_ptr<RotoPaintInteract>& roto,
                          const std::list<SmoothCuspCurveData> & data,
                          double time,
                          bool cusp,
                          const std::pair<double, double>& pixelScale);

    virtual ~SmoothCuspUndoCommand();

    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;
    virtual bool mergeWith(const UndoCommandPtr& other) OVERRIDE FINAL;

private:
    RotoPaintInteractWPtr _roto;
    bool _firstRedoCalled;
    double _time;
    int _count;
    bool _cusp;
    std::list<SmoothCuspCurveData> curves;
    std::pair<double, double> _pixelScale;
};


class MakeBezierUndoCommand
    : public UndoCommand
{
    Q_DECLARE_TR_FUNCTIONS(MakeBezierUndoCommand)

public:

    MakeBezierUndoCommand(const boost::shared_ptr<RotoPaintInteract>& roto,
                          const boost::shared_ptr<Bezier> & curve,
                          bool isOpenBezier,
                          bool createPoint,
                          double dx,
                          double dy,
                          double time);

    virtual ~MakeBezierUndoCommand();

    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;
    virtual bool mergeWith(const UndoCommandPtr& other) OVERRIDE FINAL;
    boost::shared_ptr<Bezier>  getCurve() const
    {
        return _newCurve;
    }

private:
    bool _firstRedoCalled;
    RotoPaintInteractWPtr _roto;
    boost::shared_ptr<RotoLayer> _parentLayer;
    int _indexInLayer;
    boost::shared_ptr<Bezier> _oldCurve, _newCurve;
    bool _curveNonExistant;
    bool _createdPoint;
    double _x, _y;
    double _dx, _dy;
    double _time;
    int _lastPointAdded;
    bool _isOpenBezier;
};


class MakeEllipseUndoCommand
    : public UndoCommand
{
    Q_DECLARE_TR_FUNCTIONS(MakeEllipseUndoCommand)

public:

    MakeEllipseUndoCommand(const boost::shared_ptr<RotoPaintInteract>& roto,
                           bool create,
                           bool fromCenter,
                           bool constrained,
                           double fromx,
                           double fromy,
                           double tox,
                           double toy,
                           double time);

    virtual ~MakeEllipseUndoCommand();

    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;
    virtual bool mergeWith(const UndoCommandPtr& other) OVERRIDE FINAL;

private:
    bool _firstRedoCalled;
    RotoPaintInteractWPtr _roto;
    boost::shared_ptr<RotoLayer> _parentLayer;
    int _indexInLayer;
    boost::shared_ptr<Bezier> _curve;
    bool _create;
    bool _fromCenter;
    bool _constrained;
    double _fromx, _fromy;
    double _tox, _toy;
    double _time;
};


class MakeRectangleUndoCommand
    : public UndoCommand
{
    Q_DECLARE_TR_FUNCTIONS(MakeRectangleUndoCommand)

public:

    MakeRectangleUndoCommand(const boost::shared_ptr<RotoPaintInteract>& roto,
                             bool create,
                             bool fromCenter,
                             bool constrained,
                             double fromx,
                             double fromy,
                             double tox,
                             double toy,
                             double time);

    virtual ~MakeRectangleUndoCommand();

    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;
    virtual bool mergeWith(const UndoCommandPtr& other) OVERRIDE FINAL;

private:
    bool _firstRedoCalled;
    RotoPaintInteractWPtr _roto;
    boost::shared_ptr<RotoLayer> _parentLayer;
    int _indexInLayer;
    boost::shared_ptr<Bezier> _curve;
    bool _create;
    bool _fromCenter;
    bool _constrained;
    double _fromx, _fromy;
    double _tox, _toy;
    double _time;
};


NATRON_NAMESPACE_EXIT

#endif // ROTOUNDOCOMMAND_H
