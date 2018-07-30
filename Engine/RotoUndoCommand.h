/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://natrongithub.github.io/>,
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

#include "Engine/UndoCommand.h"
#include "Engine/ViewIdx.h"
#include "Engine/TimeValue.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

class MoveControlPointsUndoCommand
    : public UndoCommand
{
    Q_DECLARE_TR_FUNCTIONS(MoveControlPointsUndoCommand)

public:

    MoveControlPointsUndoCommand(const RotoPaintInteractPtr& roto,
                                 const std::list<std::pair<BezierCPPtr, BezierCPPtr> > & toDrag
                                 ,
                                 double dx,
                                 double dy,
                                 TimeValue time, ViewIdx view);

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
    TimeValue _time; //< the time at which the change was made
    ViewIdx _view;
    std::list<RotoDrawableItemPtr> _selectedCurves;
    std::list<int> _indexesToMove; //< indexes of the control points
    std::list<std::pair<BezierCPPtr, BezierCPPtr> > _originalPoints, _selectedPoints, _pointsToDrag;
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

    TransformUndoCommand(const RotoPaintInteractPtr& roto,
                         double centerX,
                         double centerY,
                         double rot,
                         double skewX,
                         double skewY,
                         double tx,
                         double ty,
                         double sx,
                         double sy,
                         TimeValue time,
                         ViewIdx view);

    virtual ~TransformUndoCommand();

    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;
    virtual bool mergeWith(const UndoCommandPtr& other) OVERRIDE FINAL;

private:

    void transformPoint(const BezierCPPtr & point);

    bool _firstRedoCalled; //< false by default
    RotoPaintInteractWPtr _roto;
    bool _rippleEditEnabled;
    KnobButtonWPtr _selectedTool; //< corresponds to the RotoGui::RotoToolEnum enum
    boost::shared_ptr<Transform::Matrix3x3> _matrix;
    TimeValue _time; //< the time at which the change was made
    ViewIdx _view;
    std::list<RotoDrawableItemPtr> _selectedCurves;
    std::list<std::pair<BezierCPPtr, BezierCPPtr> > _originalPoints, _selectedPoints;
};

class AddPointUndoCommand
    : public UndoCommand
{
    Q_DECLARE_TR_FUNCTIONS(AddPointUndoCommand)

public:

    AddPointUndoCommand(const RotoPaintInteractPtr& roto,
                        const BezierPtr & curve,
                        int index,
                        double t,
                        ViewIdx view);

    virtual ~AddPointUndoCommand();

    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;

private:

    bool _firstRedoCalled; //< false by default
    RotoPaintInteractWPtr _roto;
    BezierPtr _curve;
    int _index;
    double _t;
    ViewIdx _view;
};


class RemovePointUndoCommand
    : public UndoCommand
{
    Q_DECLARE_TR_FUNCTIONS(RemovePointUndoCommand)

private:
    struct CurveDesc
    {
        BezierPtr oldCurve, curve;
        std::list<int> points;
        RotoLayerPtr parentLayer;
        bool curveRemoved;
        int indexInLayer;
    };

public:

    RemovePointUndoCommand(const RotoPaintInteractPtr& roto,
                           const BezierPtr & curve,
                           const BezierCPPtr & cp,
                           ViewIdx view);

    RemovePointUndoCommand(const RotoPaintInteractPtr& roto,
                           const std::list<std::pair < BezierCPPtr, BezierCPPtr> > & points,
                           ViewIdx view);

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
    ViewIdx _view;
    std::list<CurveDesc > _curves;
};


class AddStrokeUndoCommand
    : public UndoCommand
{
    Q_DECLARE_TR_FUNCTIONS(AddStrokeUndoCommand)

public:

    AddStrokeUndoCommand(const RotoPaintInteractPtr& roto,
                         const RotoStrokeItemPtr& item);

    virtual ~AddStrokeUndoCommand();
    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;

private:

    RotoPaintInteractWPtr _roto;
    bool _firstRedoCalled;
    RotoStrokeItemPtr _item;
    RotoLayerPtr _layer;
    int _indexInLayer;
};

class AddMultiStrokeUndoCommand
    : public UndoCommand
{
    Q_DECLARE_TR_FUNCTIONS(AddMultiStrokeUndoCommand)

public:

    AddMultiStrokeUndoCommand(const RotoPaintInteractPtr& roto,
                              const RotoStrokeItemPtr& item);

    virtual ~AddMultiStrokeUndoCommand();
    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;

private:

    RotoPaintInteractWPtr _roto;
    bool _firstRedoCalled;
    RotoStrokeItemPtr _item;
    RotoLayerPtr _layer;
    int _indexInLayer;
    CurvePtr _xCurve, _yCurve, _pCurve;
    bool isRemoved;
};


class MoveTangentUndoCommand
    : public UndoCommand
{
    Q_DECLARE_TR_FUNCTIONS(MoveTangentUndoCommand)

public:

    MoveTangentUndoCommand(const RotoPaintInteractPtr& roto,
                           double dx,
                           double dy,
                           TimeValue time,
                           ViewIdx view,
                           const BezierCPPtr & cp,
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
    TimeValue _time; //< the time at which the change was made
    ViewIdx _view;
    std::list<RotoDrawableItemPtr> _selectedCurves;
    std::list<std::pair<BezierCPPtr, BezierCPPtr> > _selectedPoints;
    BezierCPPtr _tangentBeingDragged, _oldCp, _oldFp;
    bool _left;
    bool _breakTangents;
};


class MoveFeatherBarUndoCommand
    : public UndoCommand
{
    Q_DECLARE_TR_FUNCTIONS(MoveFeatherBarUndoCommand)

public:

    MoveFeatherBarUndoCommand(const RotoPaintInteractPtr& roto,
                              double dx,
                              double dy,
                              const std::pair<BezierCPPtr, BezierCPPtr> & point,
                              TimeValue time,
                              ViewIdx view);

    virtual ~MoveFeatherBarUndoCommand();

    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;
    virtual bool mergeWith(const UndoCommandPtr& other) OVERRIDE FINAL;

private:

    RotoPaintInteractWPtr _roto;
    bool _firstRedoCalled;
    double _dx, _dy;
    bool _rippleEditEnabled;
    TimeValue _time; //< the time at which the change was made
    ViewIdx _view;
    BezierPtr _curve;
    std::pair<BezierCPPtr, BezierCPPtr> _oldPoint, _newPoint;
};


class RemoveFeatherUndoCommand
    : public UndoCommand
{
    Q_DECLARE_TR_FUNCTIONS(RemoveFeatherUndoCommand)

public:

    struct RemoveFeatherData
    {
        BezierPtr curve;
        std::list<BezierCPPtr> oldPoints, newPoints;
    };


    RemoveFeatherUndoCommand(const RotoPaintInteractPtr& roto,
                             const std::list<RemoveFeatherData> & datas,
                             ViewIdx view);

    virtual ~RemoveFeatherUndoCommand();

    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;

private:
    RotoPaintInteractWPtr _roto;
    bool _firstRedocalled;
    std::list<RemoveFeatherData> _datas;
    ViewIdx _view;
};

class OpenCloseUndoCommand
    : public UndoCommand
{
    Q_DECLARE_TR_FUNCTIONS(OpenCloseUndoCommand)

public:

    OpenCloseUndoCommand(const RotoPaintInteractPtr& roto,
                         const BezierPtr & curve,
                         ViewIdx view);

    virtual ~OpenCloseUndoCommand();

    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;

private:

    RotoPaintInteractWPtr _roto;
    bool _firstRedoCalled;
    KnobButtonWPtr _selectedTool;
    BezierPtr _curve;
    ViewIdx _view;
};


class SmoothCuspUndoCommand
    : public UndoCommand
{
    Q_DECLARE_TR_FUNCTIONS(SmoothCuspUndoCommand)

public:

    typedef std::list<std::pair<BezierCPPtr, BezierCPPtr> > SelectedPointList;
    struct SmoothCuspCurveData
    {
        BezierPtr curve;
        SelectedPointList newPoints, oldPoints;
    };

    SmoothCuspUndoCommand(const RotoPaintInteractPtr& roto,
                          const std::list<SmoothCuspCurveData> & data,
                          TimeValue time,
                          ViewIdx view,
                          bool cusp,
                          const std::pair<double, double>& pixelScale);

    virtual ~SmoothCuspUndoCommand();

    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;
    virtual bool mergeWith(const UndoCommandPtr& other) OVERRIDE FINAL;

private:
    RotoPaintInteractWPtr _roto;
    bool _firstRedoCalled;
    TimeValue _time;
    ViewIdx _view;
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

    MakeBezierUndoCommand(const RotoPaintInteractPtr& roto,
                          const BezierPtr & curve,
                          bool isOpenBezier,
                          bool createPoint,
                          double dx,
                          double dy,
                          TimeValue time);

    virtual ~MakeBezierUndoCommand();

    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;
    virtual bool mergeWith(const UndoCommandPtr& other) OVERRIDE FINAL;
    BezierPtr  getCurve() const
    {
        return _newCurve;
    }

private:
    bool _firstRedoCalled;
    RotoPaintInteractWPtr _roto;
    RotoLayerPtr _parentLayer;
    int _indexInLayer;
    BezierPtr _oldCurve, _newCurve;
    bool _curveNonExistant;
    bool _createdPoint;
    double _x, _y;
    double _dx, _dy;
    TimeValue _time;
    int _lastPointAdded;
    bool _isOpenBezier;
};


class MakeEllipseUndoCommand
    : public UndoCommand
{
    Q_DECLARE_TR_FUNCTIONS(MakeEllipseUndoCommand)

public:

    MakeEllipseUndoCommand(const RotoPaintInteractPtr& roto,
                           bool create,
                           bool fromCenter,
                           bool constrained,
                           double fromx,
                           double fromy,
                           double tox,
                           double toy,
                           TimeValue time);

    virtual ~MakeEllipseUndoCommand();

    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;
    virtual bool mergeWith(const UndoCommandPtr& other) OVERRIDE FINAL;

private:
    bool _firstRedoCalled;
    RotoPaintInteractWPtr _roto;
    RotoLayerPtr _parentLayer;
    int _indexInLayer;
    BezierPtr _curve;
    bool _create;
    bool _fromCenter;
    bool _constrained;
    double _fromx, _fromy;
    double _tox, _toy;
    TimeValue _time;
};


class MakeRectangleUndoCommand
    : public UndoCommand
{
    Q_DECLARE_TR_FUNCTIONS(MakeRectangleUndoCommand)

public:

    MakeRectangleUndoCommand(const RotoPaintInteractPtr& roto,
                             bool create,
                             bool fromCenter,
                             bool constrained,
                             double fromx,
                             double fromy,
                             double tox,
                             double toy,
                             TimeValue time);

    virtual ~MakeRectangleUndoCommand();

    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;
    virtual bool mergeWith(const UndoCommandPtr& other) OVERRIDE FINAL;

private:
    bool _firstRedoCalled;
    RotoPaintInteractWPtr _roto;
    RotoLayerPtr _parentLayer;
    int _indexInLayer;
    BezierPtr _curve;
    bool _create;
    bool _fromCenter;
    bool _constrained;
    double _fromx, _fromy;
    double _tox, _toy;
    TimeValue _time;
};


NATRON_NAMESPACE_EXIT

#endif // ROTOUNDOCOMMAND_H
