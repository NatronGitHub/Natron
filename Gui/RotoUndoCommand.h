/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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

#include <list>
#include <map>
#include <QUndoCommand>
#include <QList>
#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#endif
#include "Global/Macros.h"
class Bezier;
class BezierCP;
class RotoGui;
class RotoLayer;
class RotoPanel;
class QRectF;
class RotoDrawableItem;
class RotoStrokeItem;
class QTreeWidgetItem;
class RotoItem;
class Double_Knob;
namespace Transform {
struct Matrix3x3;
}
struct DroppedTreeItem;

class MoveControlPointsUndoCommand
    : public QUndoCommand
{
public:

    MoveControlPointsUndoCommand(RotoGui* roto,
                                 const std::list< std::pair<boost::shared_ptr<BezierCP>,boost::shared_ptr<BezierCP> > > & toDrag
                                 ,
                                 double dx,
                                 double dy,
                                 double time);

    virtual ~MoveControlPointsUndoCommand();

    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;
    virtual int id() const OVERRIDE FINAL;
    virtual bool mergeWith(const QUndoCommand *other) OVERRIDE FINAL;

private:

    bool _firstRedoCalled; //< false by default
    RotoGui* _roto;
    double _dx,_dy;
    bool _featherLinkEnabled;
    bool _rippleEditEnabled;
    int _selectedTool; //< corresponds to the RotoGui::RotoToolEnum enum
    double _time; //< the time at which the change was made
    std::list<boost::shared_ptr<RotoDrawableItem> > _selectedCurves;
    std::list<int> _indexesToMove; //< indexes of the control points
    std::list< std::pair<boost::shared_ptr<BezierCP>,boost::shared_ptr<BezierCP> > > _originalPoints,_selectedPoints,_pointsToDrag;
};


class TransformUndoCommand
    : public QUndoCommand
{
public:
    
    enum TransformPointsSelectionEnum
    {
        eTransformAllPoints = 0,
        eTransformMidRight,
        eTransformMidBottom,
        eTransformMidLeft,
        eTransformMidTop
    };

    TransformUndoCommand(RotoGui* roto,
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
    virtual int id() const OVERRIDE FINAL;
    virtual bool mergeWith(const QUndoCommand *other) OVERRIDE FINAL;

private:

    void transformPoint(const boost::shared_ptr<BezierCP> & point);

    bool _firstRedoCalled; //< false by default
    RotoGui* _roto;
    bool _rippleEditEnabled;
    int _selectedTool; //< corresponds to the RotoGui::RotoToolEnum enum
    boost::shared_ptr<Transform::Matrix3x3> _matrix;
    double _time; //< the time at which the change was made
    std::list<boost::shared_ptr<RotoDrawableItem> > _selectedCurves;
    std::list< std::pair<boost::shared_ptr<BezierCP>,boost::shared_ptr<BezierCP> > > _originalPoints,_selectedPoints;
};

class AddPointUndoCommand
    : public QUndoCommand
{
public:

    AddPointUndoCommand(RotoGui* roto,
                        const boost::shared_ptr<Bezier> & curve,
                        int index,
                        double t);

    virtual ~AddPointUndoCommand();

    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;

private:

    bool _firstRedoCalled; //< false by default
    RotoGui* _roto;
    boost::shared_ptr<Bezier> _oldCurve,_curve;
    int _index;
    double _t;
};


class RemovePointUndoCommand
    : public QUndoCommand
{
    struct CurveDesc
    {
        boost::shared_ptr<RotoDrawableItem> oldCurve,curve;
        std::list<int> points;
        boost::shared_ptr<RotoLayer> parentLayer;
        bool curveRemoved;
        int indexInLayer;
    };

public:

    RemovePointUndoCommand(RotoGui* roto,
                           const boost::shared_ptr<Bezier> & curve,
                           const boost::shared_ptr<BezierCP> & cp);

    RemovePointUndoCommand(RotoGui* roto,
                           const std::list< std::pair < boost::shared_ptr<BezierCP>,boost::shared_ptr<BezierCP> > > & points);

    virtual ~RemovePointUndoCommand();

    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;

private:
    RotoGui* _roto;
    struct CurveOrdering
    {
        bool operator() (const CurveDesc & lhs,
                         const CurveDesc & rhs)
        {
            return lhs.indexInLayer < rhs.indexInLayer;
        }
    };

    bool _firstRedoCalled;
    std::list< CurveDesc > _curves;
};


class RemoveCurveUndoCommand
    : public QUndoCommand
{
    struct RemovedCurve
    {
        boost::shared_ptr<RotoDrawableItem> curve;
        boost::shared_ptr<RotoLayer> layer;
        int indexInLayer;
    };

public:


    RemoveCurveUndoCommand(RotoGui* roto,
                           const std::list<boost::shared_ptr<RotoDrawableItem> > & curves);

    virtual ~RemoveCurveUndoCommand();

    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;

private:
    RotoGui* _roto;
    bool _firstRedoCalled;
    std::list<RemovedCurve> _curves;
};

class AddStrokeUndoCommand : public QUndoCommand
{
public:
    
    AddStrokeUndoCommand(RotoGui* roto,const boost::shared_ptr<RotoStrokeItem>& item);
    
    virtual ~AddStrokeUndoCommand();
    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;
    
private:
    
    RotoGui* _roto;
    bool _firstRedoCalled;
    boost::shared_ptr<RotoStrokeItem> _item;
    boost::shared_ptr<RotoLayer> _layer;
    int _indexInLayer;
};


class MoveTangentUndoCommand
    : public QUndoCommand
{
public:

    MoveTangentUndoCommand(RotoGui* roto,
                           double dx,
                           double dy,
                           double time,
                           const boost::shared_ptr<BezierCP> & cp,
                           bool left,
                           bool breakTangents);

    virtual ~MoveTangentUndoCommand();

    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;
    virtual int id() const OVERRIDE FINAL;
    virtual bool mergeWith(const QUndoCommand *other) OVERRIDE FINAL;

private:

    bool _firstRedoCalled; //< false by default
    RotoGui* _roto;
    double _dx,_dy;
    bool _featherLinkEnabled;
    bool _rippleEditEnabled;
    double _time; //< the time at which the change was made
    std::list<boost::shared_ptr<RotoDrawableItem> > _selectedCurves;
    std::list< std::pair<boost::shared_ptr<BezierCP>,boost::shared_ptr<BezierCP> > > _selectedPoints;
    boost::shared_ptr<BezierCP> _tangentBeingDragged,_oldCp,_oldFp;
    bool _left;
    bool _breakTangents;
};


class MoveFeatherBarUndoCommand
    : public QUndoCommand
{
public:

    MoveFeatherBarUndoCommand(RotoGui* roto,
                              double dx,
                              double dy,
                              const std::pair<boost::shared_ptr<BezierCP>,boost::shared_ptr<BezierCP> > & point,
                              double time);

    virtual ~MoveFeatherBarUndoCommand();

    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;
    virtual int id() const OVERRIDE FINAL;
    virtual bool mergeWith(const QUndoCommand *other) OVERRIDE FINAL;

private:

    RotoGui* _roto;
    bool _firstRedoCalled;
    double _dx,_dy;
    bool _rippleEditEnabled;
    double _time; //< the time at which the change was made
    boost::shared_ptr<Bezier> _curve;
    std::pair<boost::shared_ptr<BezierCP>,boost::shared_ptr<BezierCP> > _oldPoint,_newPoint;
};


class RemoveFeatherUndoCommand
    : public QUndoCommand
{
public:

    struct RemoveFeatherData
    {
        boost::shared_ptr<Bezier> curve;
        std::list<boost::shared_ptr<BezierCP> > oldPoints,newPoints;
    };


    RemoveFeatherUndoCommand(RotoGui* roto,
                             const std::list<RemoveFeatherData> & datas);

    virtual ~RemoveFeatherUndoCommand();

    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;

private:
    RotoGui* _roto;
    bool _firstRedocalled;
    std::list<RemoveFeatherData> _datas;
};

class OpenCloseUndoCommand
    : public QUndoCommand
{
public:


    OpenCloseUndoCommand(RotoGui* roto,
                         const boost::shared_ptr<Bezier> & curve);

    virtual ~OpenCloseUndoCommand();

    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;

private:

    RotoGui* _roto;
    bool _firstRedoCalled;
    int _selectedTool;
    boost::shared_ptr<Bezier> _curve;
};


class SmoothCuspUndoCommand
    : public QUndoCommand
{
public:

    typedef std::list<std::pair<boost::shared_ptr<BezierCP>,boost::shared_ptr<BezierCP> > > SelectedPointList;
    struct SmoothCuspCurveData
    {
        boost::shared_ptr<Bezier> curve;
        SelectedPointList newPoints,oldPoints;
    };

    SmoothCuspUndoCommand(RotoGui* roto,
                          const std::list<SmoothCuspCurveData> & data,
                          double time,
                          bool cusp,
                          const std::pair<double, double>& pixelScale);

    virtual ~SmoothCuspUndoCommand();

    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;
    virtual int id() const OVERRIDE FINAL;
    virtual bool mergeWith(const QUndoCommand *other) OVERRIDE FINAL;

private:
    RotoGui* _roto;
    bool _firstRedoCalled;
    double _time;
    int _count;
    bool _cusp;
    std::list<SmoothCuspCurveData> curves;
    std::pair<double, double> _pixelScale;
};


class MakeBezierUndoCommand
    : public QUndoCommand
{
public:

    MakeBezierUndoCommand(RotoGui* roto,
                          const boost::shared_ptr<Bezier> & curve,
                          bool isOpenBezier,
                          bool createPoint,
                          double dx,
                          double dy,
                          double time);

    virtual ~MakeBezierUndoCommand();

    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;
    virtual int id() const OVERRIDE FINAL;
    virtual bool mergeWith(const QUndoCommand *other) OVERRIDE FINAL;
    boost::shared_ptr<Bezier>  getCurve() const
    {
        return _newCurve;
    }

private:
    bool _firstRedoCalled;
    RotoGui* _roto;
    boost::shared_ptr<RotoLayer> _parentLayer;
    int _indexInLayer;
    boost::shared_ptr<Bezier> _oldCurve,_newCurve;
    bool _curveNonExistant;
    bool _createdPoint;
    double _x,_y;
    double _dx,_dy;
    double _time;
    int _lastPointAdded;
    bool _isOpenBezier;
};


class MakeEllipseUndoCommand
    : public QUndoCommand
{
public:

    MakeEllipseUndoCommand(RotoGui* roto,
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
    virtual int id() const OVERRIDE FINAL;
    virtual bool mergeWith(const QUndoCommand *other) OVERRIDE FINAL;

private:
    bool _firstRedoCalled;
    RotoGui* _roto;
    boost::shared_ptr<RotoLayer> _parentLayer;
    int _indexInLayer;
    boost::shared_ptr<Bezier> _curve;
    bool _create;
    bool _fromCenter;
    bool _constrained;
    double _fromx,_fromy;
    double _tox, _toy;
    double _time;
};


class MakeRectangleUndoCommand
    : public QUndoCommand
{
public:

    MakeRectangleUndoCommand(RotoGui* roto,
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
    virtual int id() const OVERRIDE FINAL;
    virtual bool mergeWith(const QUndoCommand *other) OVERRIDE FINAL;

private:
    bool _firstRedoCalled;
    RotoGui* _roto;
    boost::shared_ptr<RotoLayer> _parentLayer;
    int _indexInLayer;
    boost::shared_ptr<Bezier> _curve;
    bool _create;
    bool _fromCenter;
    bool _constrained;
    double _fromx,_fromy;
    double _tox, _toy;
    double _time;
};


class RemoveItemsUndoCommand
    : public QUndoCommand
{
    struct RemovedItem
    {
        QTreeWidgetItem* treeItem;
        QTreeWidgetItem* parentTreeItem;
        boost::shared_ptr<RotoLayer> parentLayer;
        int indexInLayer;
        boost::shared_ptr<RotoItem> item;

        RemovedItem()
            : treeItem(0)
              , parentTreeItem(0)
              , parentLayer()
              , indexInLayer(-1)
              , item()
        {
        }
    };

public:


    RemoveItemsUndoCommand(RotoPanel* roto,
                           const QList<QTreeWidgetItem*> & items);

    virtual ~RemoveItemsUndoCommand();

    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;

private:

    RotoPanel* _roto;
    std::list<RemovedItem> _items;
};

class AddLayerUndoCommand
    : public QUndoCommand
{
public:


    AddLayerUndoCommand(RotoPanel* roto);

    virtual ~AddLayerUndoCommand();

    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;

private:

    RotoPanel* _roto;
    bool _firstRedoCalled;
    boost::shared_ptr<RotoLayer> _parentLayer;
    QTreeWidgetItem* _parentTreeItem;
    QTreeWidgetItem* _treeItem;
    boost::shared_ptr<RotoLayer> _layer;
    int _indexInParentLayer;
};


class DragItemsUndoCommand
    : public QUndoCommand
{
public:

    struct Item
    {
        boost::shared_ptr<DroppedTreeItem> dropped;
        boost::shared_ptr<RotoLayer> oldParentLayer;
        int indexInOldLayer;
        QTreeWidgetItem* oldParentItem;
    };


    DragItemsUndoCommand(RotoPanel* roto,
                         const std::list< boost::shared_ptr<DroppedTreeItem> > & items);

    virtual ~DragItemsUndoCommand();

    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;

private:

    RotoPanel* _roto;
    std::list < Item > _items;
};


/**
 * @class This class supports 2 behaviours:
 * 1) The user pastes one item upon another. The target's shape and attributes are copied and the
 * name is the source's name plus "- copy" at the end.
 * 2) The user pastes several items upon a layer in which case the items are copied into that layer and
 * the new items name is the same than the original appeneded with "- copy".
 *
 * Anything else will not do anything and you should not issue a command which will yield an unsupported behaviour
 * otherwise you'll create an empty action in the undo/redo stack.
 **/
class PasteItemUndoCommand
    : public QUndoCommand
{
public:

    enum PasteModeEnum
    {
        ePasteModeCopyToLayer = 0,
        ePasteModeCopyToItem
    };

    struct PastedItem
    {
        QTreeWidgetItem* treeItem;
        boost::shared_ptr<RotoItem> rotoItem;
        boost::shared_ptr<RotoItem> itemCopy;
    };


    PasteItemUndoCommand(RotoPanel* roto,
                         QTreeWidgetItem* target,
                         QList<QTreeWidgetItem*> source);

    virtual ~PasteItemUndoCommand();

    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;

private:

    RotoPanel* _roto;
    PasteModeEnum _mode;
    QTreeWidgetItem* _targetTreeItem;
    boost::shared_ptr<RotoItem> _targetItem;
    boost::shared_ptr<RotoItem> _oldTargetItem;
    std::list < PastedItem > _pastedItems;
};


class DuplicateItemUndoCommand
    : public QUndoCommand
{
public:

    struct DuplicatedItem
    {
        QTreeWidgetItem* treeItem;
        boost::shared_ptr<RotoItem> item;
        boost::shared_ptr<RotoItem> duplicatedItem;
    };

    DuplicateItemUndoCommand(RotoPanel* roto,
                             QTreeWidgetItem* items);

    virtual ~DuplicateItemUndoCommand();

    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;

private:

    RotoPanel* _roto;
    DuplicatedItem _item;
};

class LinkToTrackUndoCommand
    : public QUndoCommand
{
public:


    LinkToTrackUndoCommand(RotoGui* roto,
                           const std::list<std::pair<boost::shared_ptr<BezierCP>,boost::shared_ptr<BezierCP> > > & points,
                           const boost::shared_ptr<Double_Knob> & track);

    virtual ~LinkToTrackUndoCommand();

    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;

private:

    RotoGui* _roto;
    std::list<std::pair<boost::shared_ptr<BezierCP>,boost::shared_ptr<BezierCP> > > _points;
    boost::shared_ptr<Double_Knob> _track;
};

class UnLinkFromTrackUndoCommand
    : public QUndoCommand
{
    struct PointToUnlink
    {
        boost::shared_ptr<BezierCP> cp;
        boost::shared_ptr<BezierCP> fp;
        boost::shared_ptr<Double_Knob> track;
    };

public:


    UnLinkFromTrackUndoCommand(RotoGui* roto,
                               const std::list<std::pair<boost::shared_ptr<BezierCP>,boost::shared_ptr<BezierCP> > > & points);

    virtual ~UnLinkFromTrackUndoCommand();

    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;

private:

    RotoGui* _roto;
    std::list<PointToUnlink> _points;
};


#endif // ROTOUNDOCOMMAND_H
