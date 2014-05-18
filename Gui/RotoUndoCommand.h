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
#include <boost/shared_ptr.hpp>

#include "Global/Macros.h"
class Bezier;
class BezierCP;
class RotoGui;
class RotoLayer;
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
    std::list<RemovedCurve> _curves;
};

#endif // ROTOUNDOCOMMAND_H
