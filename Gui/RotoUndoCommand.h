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


#ifndef ROTOUNDOCOMMAND_H
#define ROTOUNDOCOMMAND_H

#include <list>
#include <map>
#include <QUndoCommand>
#include <QList>
#include <boost/shared_ptr.hpp>

#include "Global/Macros.h"
class Bezier;
class BezierCP;
class RotoGui;
class RotoLayer;
class RotoPanel;
class QTreeWidgetItem;
class RotoItem;
class DroppedTreeItem;

class MoveControlPointsUndoCommand : public QUndoCommand
{
public:
    
    MoveControlPointsUndoCommand(RotoGui* roto,double dx,double dy,int time);
    
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
    int _selectedTool; //< corresponds to the RotoGui::Roto_Tool enum
    int _time; //< the time at which the change was made
    std::list<boost::shared_ptr<Bezier> > _selectedCurves;
    
    std::list< std::pair<boost::shared_ptr<BezierCP> ,boost::shared_ptr<BezierCP> > > _originalPoints,_selectedPoints;
};

class AddPointUndoCommand: public QUndoCommand
{
public:
    
    AddPointUndoCommand(RotoGui* roto,const boost::shared_ptr<Bezier>& curve,int index,double t);
    
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




class RemovePointUndoCommand : public QUndoCommand
{

    

    struct CurveDesc
    {
        boost::shared_ptr<Bezier> oldCurve,curve;
        std::list<int> points;
        boost::shared_ptr<RotoLayer> parentLayer;
        bool curveRemoved;
        int indexInLayer;
    };
    
    
    
public:
    
    RemovePointUndoCommand(RotoGui* roto,const boost::shared_ptr<Bezier>& curve,
                           const boost::shared_ptr<BezierCP>& cp);
    
    RemovePointUndoCommand(RotoGui* roto,const std::list< std::pair < boost::shared_ptr<BezierCP>,boost::shared_ptr<BezierCP> > >& points);
    
    virtual ~RemovePointUndoCommand();
    
    virtual void undo() OVERRIDE FINAL;
    
    virtual void redo() OVERRIDE FINAL;

private:
    RotoGui* _roto;
    
    struct CurveOrdering
    {
        bool operator() (const CurveDesc& lhs,const CurveDesc& rhs)
        {
            return lhs.indexInLayer < rhs.indexInLayer;
        }
    };
    
    bool _firstRedoCalled;
    
    std::list< CurveDesc > _curves;

};


class RemoveCurveUndoCommand: public QUndoCommand
{
    
    struct RemovedCurve
    {
        boost::shared_ptr<Bezier> curve;
        boost::shared_ptr<RotoLayer> layer;
        int indexInLayer;
    };
    
public:
    
    
    
    RemoveCurveUndoCommand(RotoGui* roto,const std::list<boost::shared_ptr<Bezier> >& curves);
    
    virtual ~RemoveCurveUndoCommand();
    
    virtual void undo() OVERRIDE FINAL;
    
    virtual void redo() OVERRIDE FINAL;
    
private:
    RotoGui* _roto;
    bool _firstRedoCalled;
    std::list<RemovedCurve> _curves;
};


class MoveTangentUndoCommand : public QUndoCommand
{
public:
    
    MoveTangentUndoCommand(RotoGui* roto,double dx,double dy,int time,const boost::shared_ptr<BezierCP>& cp,bool left);
    
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
    int _time; //< the time at which the change was made
    std::list<boost::shared_ptr<Bezier> > _selectedCurves;
    std::list< std::pair<boost::shared_ptr<BezierCP> ,boost::shared_ptr<BezierCP> > > _selectedPoints;
    boost::shared_ptr<BezierCP> _tangentBeingDragged,_oldCp,_oldFp;
    
    bool _left;
};


class MoveFeatherBarUndoCommand : public QUndoCommand
{
public:
    
    MoveFeatherBarUndoCommand(RotoGui* roto,double dx,double dy,
                              const std::pair<boost::shared_ptr<BezierCP>,boost::shared_ptr<BezierCP> >& point,
                              int time);
    
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
    int _time; //< the time at which the change was made
    boost::shared_ptr<Bezier> _curve;
    std::pair<boost::shared_ptr<BezierCP>,boost::shared_ptr<BezierCP> > _oldPoint,_newPoint;
};



class RemoveFeatherUndoCommand: public QUndoCommand
{
    

public:
    
    
    
    RemoveFeatherUndoCommand(RotoGui* roto,const boost::shared_ptr<Bezier>& curve,
                             const boost::shared_ptr<BezierCP>& fp);
    
    virtual ~RemoveFeatherUndoCommand();
    
    virtual void undo() OVERRIDE FINAL;
    
    virtual void redo() OVERRIDE FINAL;
    
private:
    RotoGui* _roto;
    bool _firstRedocalled;
    boost::shared_ptr<Bezier> _curve;
    boost::shared_ptr<BezierCP> _oldFp,_newFp;
};

class OpenCloseUndoCommand: public QUndoCommand
{
    
    
public:
    
    
    
    OpenCloseUndoCommand(RotoGui* roto,const boost::shared_ptr<Bezier>& curve);
    
    virtual ~OpenCloseUndoCommand();
    
    virtual void undo() OVERRIDE FINAL;
    
    virtual void redo() OVERRIDE FINAL;
    
private:
    
    RotoGui* _roto;
    bool _firstRedoCalled;
    int _selectedTool;
    boost::shared_ptr<Bezier> _curve;
};



class SmoothCuspUndoCommand: public QUndoCommand
{
    
    
public:
    
    
    
    SmoothCuspUndoCommand(RotoGui* roto,const boost::shared_ptr<Bezier>& curve,
                          const std::pair<boost::shared_ptr<BezierCP>,boost::shared_ptr<BezierCP> >& point,
                          int time,bool cusp);
    
    virtual ~SmoothCuspUndoCommand();
    
    virtual void undo() OVERRIDE FINAL;
    
    virtual void redo() OVERRIDE FINAL;
    
    virtual int id() const OVERRIDE FINAL;
    
    virtual bool mergeWith(const QUndoCommand *other) OVERRIDE FINAL;

    
private:
    RotoGui* _roto;
    bool _firstRedoCalled;
    boost::shared_ptr<Bezier> _curve;
    int _time;
    int _count;
    bool _cusp;
    std::pair<boost::shared_ptr<BezierCP>,boost::shared_ptr<BezierCP> > _oldPoint,_newPoint;
};



class MakeBezierUndoCommand: public QUndoCommand
{
    
    
public:
    
    MakeBezierUndoCommand(RotoGui* roto,const boost::shared_ptr<Bezier>& curve,bool createPoint,double dx,double dy,int time);
    
    virtual ~MakeBezierUndoCommand();
    
    virtual void undo() OVERRIDE FINAL;
    
    virtual void redo() OVERRIDE FINAL;
    
    virtual int id() const OVERRIDE FINAL;
    
    virtual bool mergeWith(const QUndoCommand *other) OVERRIDE FINAL;
    
    boost::shared_ptr<Bezier>  getCurve() const { return _newCurve; }
    
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
    int _time;
    int _lastPointAdded;
};


class MakeEllipseUndoCommand: public QUndoCommand
{
    
    
public:
    
    MakeEllipseUndoCommand(RotoGui* roto,bool create,bool fromCenter,double dx,double dy,int time);
    
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
    boost::shared_ptr<Bezier> _newCurve,_curve;
    bool _create;
    bool _fromCenter;
    double _x,_y;
    double _dx,_dy;
    int _time;
};


class MakeRectangleUndoCommand: public QUndoCommand
{
    
    
public:
    
    MakeRectangleUndoCommand(RotoGui* roto,bool create,double dx,double dy,int time);
    
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
    boost::shared_ptr<Bezier> _newCurve,_curve;
    bool _create;
    double _x,_y;
    double _dx,_dy;
    int _time;
};




class RemoveItemsUndoCommand: public QUndoCommand
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
    
    
    
    RemoveItemsUndoCommand(RotoPanel* roto,const QList<QTreeWidgetItem*>& items);
    
    virtual ~RemoveItemsUndoCommand();
    
    virtual void undo() OVERRIDE FINAL;
    
    virtual void redo() OVERRIDE FINAL;
    
private:
    
    RotoPanel* _roto;
    std::list<RemovedItem> _items;
};

class AddLayerUndoCommand: public QUndoCommand
{

    
public:
    
    
    
    AddLayerUndoCommand(RotoPanel* roto);
    
    virtual ~AddLayerUndoCommand();
    
    virtual void undo() OVERRIDE FINAL;
    
    virtual void redo() OVERRIDE FINAL;
    
private:
    
    RotoPanel* _roto;
    bool _firstRedoCalled;
    RotoLayer* _parentLayer;
    QTreeWidgetItem* _parentTreeItem;
    QTreeWidgetItem* _treeItem;
    boost::shared_ptr<RotoLayer> _layer;
    int _indexInParentLayer;
};


class DragItemsUndoCommand: public QUndoCommand
{
    
    
public:
    
    struct Item
    {
        boost::shared_ptr<DroppedTreeItem> dropped;
        RotoLayer* oldParentLayer;
        int indexInOldLayer;
        QTreeWidgetItem* oldParentItem;
    };
    
    
    
    DragItemsUndoCommand(RotoPanel* roto,const std::list< boost::shared_ptr<DroppedTreeItem> >& items);
    
    virtual ~DragItemsUndoCommand();
    
    virtual void undo() OVERRIDE FINAL;
    
    virtual void redo() OVERRIDE FINAL;
    
private:
    
    RotoPanel* _roto;
    std::list < Item > _items;
};



#endif // ROTOUNDOCOMMAND_H
