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


#include "RotoUndoCommand.h"
#include "Global/GlobalDefines.h"
#include "Engine/RotoContext.h"
#include "Gui/RotoGui.h"

typedef boost::shared_ptr<BezierCP> CpPtr;
typedef std::pair<CpPtr,CpPtr> SelectedCp;
typedef std::list<SelectedCp> SelectedCpList;

typedef boost::shared_ptr<Bezier> BezierPtr;
typedef std::list<BezierPtr> BezierList;

MoveControlPointsUndoCommand::MoveControlPointsUndoCommand(RotoGui* roto,double dx,double dy,int time)
: QUndoCommand()
, _firstRedoCalled(false)
, _roto(roto)
, _dx(dx)
, _dy(dy)
, _featherLinkEnabled(roto->getContext()->isFeatherLinkEnabled())
, _rippleEditEnabled(roto->getContext()->isRippleEditEnabled())
, _selectedTool((int)roto->getSelectedTool())
, _time(time)
{
    assert(roto);
    
    roto->getSelection(&_selectedCurves, &_selectedPoints);
    
    ///we make a copy of the points
    for (SelectedCpList::iterator it = _selectedPoints.begin(); it!= _selectedPoints.end(); ++it) {
        CpPtr first(new BezierCP(*(it->first)));
        CpPtr second(new BezierCP(*(it->second)));
        _originalPoints.push_back(std::make_pair(first, second));
    }
}

MoveControlPointsUndoCommand::~MoveControlPointsUndoCommand()
{
    
}


void MoveControlPointsUndoCommand::undo()
{
    SelectedCpList::iterator cpIt = _originalPoints.begin();
    for (SelectedCpList::iterator it = _selectedPoints.begin(); it!=_selectedPoints.end(); ++it,++cpIt) {
        it->first->getCurve()->clonePoint(*(it->first),*(cpIt->first));
        it->second->getCurve()->clonePoint(*(it->second),*(cpIt->second));
    }
    
    _roto->evaluate();
    _roto->setCurrentTool((RotoGui::Roto_Tool)_selectedTool);
    _roto->setSelection(_selectedCurves, _selectedPoints);
    setText(QString("Move points of %1").arg(_roto->getNodeName()));
}

void MoveControlPointsUndoCommand::redo()
{
    for (SelectedCpList::iterator it = _selectedPoints.begin(); it!=_selectedPoints.end(); ++it) {
        int index;
        if (it->first->isFeatherPoint()) {
            if ((RotoGui::Roto_Tool)_selectedTool == RotoGui::SELECT_FEATHER_POINTS ||
                (RotoGui::Roto_Tool)_selectedTool == RotoGui::SELECT_ALL) {
                index = it->second->getCurve()->getControlPointIndex(it->second);
                assert(index != -1);
                it->first->getCurve()->moveFeatherByIndex(index,_time, _dx, _dy);
            }
        } else {
            if ((RotoGui::Roto_Tool)_selectedTool == RotoGui::SELECT_POINTS ||
                (RotoGui::Roto_Tool)_selectedTool == RotoGui::SELECT_ALL) {
                index = it->first->getCurve()->getControlPointIndex(it->first);
                assert(index != -1);
                it->first->getCurve()->movePointByIndex(index,_time, _dx, _dy);
            }
        }
    }
    
    if (_firstRedoCalled) {
        _roto->setSelection(_selectedCurves, _selectedPoints);
        _roto->evaluate();
    }
    
    _firstRedoCalled = true;
    setText(QString("Move points of %1").arg(_roto->getNodeName()));
}

int MoveControlPointsUndoCommand::id() const
{
    return kRotoMoveControlPointsCompressionID;
}

bool MoveControlPointsUndoCommand::mergeWith(const QUndoCommand *other)
{
    const MoveControlPointsUndoCommand* mvCmd = dynamic_cast<const MoveControlPointsUndoCommand*>(other);
    if (!mvCmd) {
        return false;
    }
    
    if (mvCmd->_selectedPoints.size() != _selectedPoints.size() || mvCmd->_time != _time || mvCmd->_selectedTool != _selectedTool
        || mvCmd->_rippleEditEnabled != _rippleEditEnabled || mvCmd->_featherLinkEnabled != _featherLinkEnabled) {
        return false;
    }
    
    SelectedCpList::const_iterator it = _selectedPoints.begin();
    SelectedCpList::const_iterator oIt = mvCmd->_selectedPoints.begin();
    for (; it != _selectedPoints.end(); ++it,++oIt) {
        if (it->first != oIt->first || it->second != oIt->second ) {
            return false;
        }
    }
    
    _dx += mvCmd->_dx;
    _dy += mvCmd->_dy;
    return true;
    
}


////////////////////////


AddPointUndoCommand::AddPointUndoCommand(RotoGui* roto,const boost::shared_ptr<Bezier>& curve,int index,double t)
: QUndoCommand()
, _firstRedoCalled(false)
, _roto(roto)
, _oldCurve()
, _curve(curve)
, _index(index)
, _t(t)
{
    _oldCurve.reset(new Bezier(*curve));
}

AddPointUndoCommand::~AddPointUndoCommand()
{
    
}

void AddPointUndoCommand::undo()
{
    _curve->clone(*_oldCurve);
    _roto->setSelection(_curve, std::make_pair(CpPtr(),CpPtr()));
     _roto->evaluate();
    setText(QString("Add point to %1 of %2").arg(_curve->getName_mt_safe().c_str()).arg(_roto->getNodeName()));
}

void AddPointUndoCommand::redo()
{
    _oldCurve->clone(*_curve);
    boost::shared_ptr<BezierCP> cp = _curve->addControlPointAfterIndex(_index,_t);
    boost::shared_ptr<BezierCP> newFp = _curve->getFeatherPointAtIndex(_index + 1);

    _roto->setSelection(_curve, std::make_pair(cp, newFp));
    if (_firstRedoCalled) {
        _roto->evaluate();
    }

    _firstRedoCalled = true;
    setText(QString("Add point to %1 of %2").arg(_curve->getName_mt_safe().c_str()).arg(_roto->getNodeName()));
}

////////////////////////

RemovePointUndoCommand::RemovePointUndoCommand(RotoGui* roto,const boost::shared_ptr<Bezier>& curve,
                                               const boost::shared_ptr<BezierCP>& cp)
: QUndoCommand()
, _roto(roto)
, _curves()
{
    CurveDesc desc;
    int indexToRemove = curve->getControlPointIndex(cp);
    desc.curveRemoved = false; //set in the redo()
    desc.parentLayer =
    boost::dynamic_pointer_cast<RotoLayer>(_roto->getContext()->getItemByName(curve->getParentLayer()->getName_mt_safe()));
    assert(desc.parentLayer);
    desc.curve = curve; 
    desc.points.push_back(indexToRemove);
    desc.oldCurve.reset(new Bezier(*curve));
    _curves.push_back(desc);
}

RemovePointUndoCommand::RemovePointUndoCommand(RotoGui* roto,const SelectedCpList& points)
: QUndoCommand()
, _roto(roto)
, _firstRedoCalled(false)
, _curves()
{
    for (SelectedCpList::const_iterator it = points.begin(); it!=points.end(); ++it) {
        boost::shared_ptr<BezierCP> cp,fp;
        if (it->first->isFeatherPoint()) {
            cp = it->second;
            fp = it->first;
        } else {
            cp = it->first;
            fp = it->second;
        }
        BezierPtr curve = boost::dynamic_pointer_cast<Bezier>(_roto->getContext()->getItemByName(cp->getCurve()->getName_mt_safe()));
        
        std::list< CurveDesc >::iterator foundCurve = _curves.end();
        for (std::list< CurveDesc >::iterator it2 = _curves.begin(); it2!= _curves.end(); ++it2)
        {
            if (it2->curve == curve) {
                foundCurve = it2;
                break;
            }
        }
        int indexToRemove = curve->getControlPointIndex(cp);
        if (foundCurve == _curves.end()) {
            CurveDesc curveDesc;
            curveDesc.curveRemoved = false; //set in the redo()
            curveDesc.parentLayer =
            boost::dynamic_pointer_cast<RotoLayer>(_roto->getContext()->getItemByName(cp->getCurve()->getParentLayer()->getName_mt_safe()));
            assert(curveDesc.parentLayer);
            curveDesc.points.push_back(indexToRemove);
            curveDesc.curve = curve;
            curveDesc.oldCurve.reset(new Bezier(*curve));
            _curves.push_back(curveDesc);
        } else {
            foundCurve->points.push_back(indexToRemove);
        }
    }
    for (std::list<CurveDesc>::iterator it = _curves.begin(); it!=_curves.end(); ++it) {
        it->points.sort();
    }
}

RemovePointUndoCommand::~RemovePointUndoCommand()
{
    
}

void RemovePointUndoCommand::undo()
{
    
    BezierList selection;
    SelectedCpList cpSelection;
    for (std::list< CurveDesc >::iterator it = _curves.begin(); it!=_curves.end(); ++it) {
        ///clone the curve
        it->curve->clone(*(it->oldCurve));
        if (it->curveRemoved) {
            _roto->getContext()->addItem(it->parentLayer.get(), it->indexInLayer, it->curve);
        }
        selection.push_back(it->curve);
    }
    
    _roto->setSelection(selection,cpSelection);
    _roto->evaluate();
    
    setText(QString("Remove points to %1").arg(_roto->getNodeName()));
}

void RemovePointUndoCommand::redo()
{
    
    ///clone the curve
    for (std::list< CurveDesc >::iterator it = _curves.begin(); it!=_curves.end(); ++it) {
        it->oldCurve->clone(*(it->curve));
    }
    
    std::list<Bezier*> toRemove;
    std::list<BezierPtr> toSelect;
    for (std::list< CurveDesc >::iterator it = _curves.begin(); it!=_curves.end(); ++it) {
        
        ///Remove in decreasing order so indexes don't get messed up
        for (std::list<int>::reverse_iterator it2 = it->points.rbegin(); it2!=it->points.rend(); ++it2) {
            it->curve->removeControlPointByIndex(*it2);
            int cpCount = it->curve->getControlPointsCount();
            if (cpCount == 1) {
                it->curve->setCurveFinished(false);
            } else if (cpCount == 0) {
                it->curveRemoved = true;
                std::list<Bezier*>::iterator found = std::find(toRemove.begin(), toRemove.end(), it->curve.get());
                if (found == toRemove.end()) {
                    toRemove.push_back(it->curve.get());
                }
            }
            if (cpCount > 0) {
                std::list<BezierPtr>::iterator foundSelect = std::find(toSelect.begin(), toSelect.end(), it->curve);
                if (foundSelect == toSelect.end()) {
                    toSelect.push_back(it->curve);
                }
            }
            
        }
    }
   
    for (std::list<Bezier*>::iterator it = toRemove.begin(); it!=toRemove.end(); ++it) {
        _roto->removeCurve(*it);
    }



    _roto->setSelection(toSelect,SelectedCpList());
    _roto->evaluate();
    _firstRedoCalled = true;

    setText(QString("Remove points to %1").arg(_roto->getNodeName()));
}

//////////////////////////

RemoveCurveUndoCommand::RemoveCurveUndoCommand(RotoGui* roto,const std::list<boost::shared_ptr<Bezier> >& curves)
: QUndoCommand()
, _roto(roto)
, _curves()
{
    for (BezierList::const_iterator it = curves.begin(); it!=curves.end(); ++it) {
        RemovedCurve r;
        r.curve = *it;
        r.layer = boost::dynamic_pointer_cast<RotoLayer>(_roto->getContext()->getItemByName((*it)->getParentLayer()->getName_mt_safe()));
        assert(r.layer);
        r.indexInLayer = r.layer->getChildIndex(*it);
        assert(r.indexInLayer != -1);
        _curves.push_back(r);
    }
}

RemoveCurveUndoCommand::~RemoveCurveUndoCommand()
{
    
}

void RemoveCurveUndoCommand::undo()
{
    BezierList selection;
    for (std::list<RemovedCurve>::iterator it = _curves.begin(); it!= _curves.end(); ++it) {
        _roto->getContext()->addItem(it->layer.get(),it->indexInLayer, it->curve);
        selection.push_back(it->curve);
    }
    
    SelectedCpList cpList;
    _roto->setSelection(selection, cpList);
    _roto->evaluate();
    
    setText(QString("Remove curves to %1").arg(_roto->getNodeName()));
}

void RemoveCurveUndoCommand::redo()
{
    for (std::list<RemovedCurve>::iterator it = _curves.begin(); it!=_curves.end(); ++it) {
        _roto->removeCurve(it->curve.get());
    }
    _roto->evaluate();
    
    setText(QString("Remove curves to %1").arg(_roto->getNodeName()));
}

////////////////////////////////

MoveTangentUndoCommand::MoveTangentUndoCommand(RotoGui* roto,double dx,double dy,int time,const boost::shared_ptr<BezierCP>& cp,bool left)
: QUndoCommand()
, _firstRedoCalled(false)
, _roto(roto)
, _dx(dx)
, _dy(dy)
, _featherLinkEnabled(roto->getContext()->isFeatherLinkEnabled())
, _rippleEditEnabled(roto->getContext()->isRippleEditEnabled())
, _time(time)
, _tangentBeingDragged(cp)
, _oldCp()
, _oldFp()
, _left(left)
{
    roto->getSelection(&_selectedCurves, &_selectedPoints);
    boost::shared_ptr<BezierCP> counterPart;
    if (cp->isFeatherPoint()) {
        counterPart = _tangentBeingDragged->getCurve()->getControlPointForFeatherPoint(_tangentBeingDragged);
        _oldCp.reset(new BezierCP(*counterPart));
        _oldFp.reset(new BezierCP(*_tangentBeingDragged));
    } else {
        counterPart = _tangentBeingDragged->getCurve()->getFeatherPointForControlPoint(_tangentBeingDragged);
        _oldCp.reset(new BezierCP(*_tangentBeingDragged));
        _oldFp.reset(new BezierCP(*counterPart));
    }
    
}

MoveTangentUndoCommand::~MoveTangentUndoCommand()
{
    
}



namespace {
    
    static void dragTangent(int time,BezierCP& p,double dx,double dy,bool left,bool autoKeying,bool rippleEdit)
    {
        double leftX,leftY,rightX,rightY,x,y;
        bool isOnKeyframe = p.getLeftBezierPointAtTime(time, &leftX, &leftY);
        p.getRightBezierPointAtTime(time, &rightX, &rightY);
        p.getPositionAtTime(time, &x, &y);
        double dist = left ?  sqrt((rightX - x) * (rightX - x) + (rightY - y) * (rightY - y))
        : sqrt((leftX - x) * (leftX - x) + (leftY - y) * (leftY - y));
        if (left) {
            leftX += dx;
            leftY += dy;
        } else {
            rightX += dx;
            rightY += dy;
        }
        double alpha = left ? std::atan2(y - leftY,x - leftX) : std::atan2(y - rightY,x - rightX);
        std::set<int> times;
        p.getKeyframeTimes(&times);
        
        if (left) {
            rightX = std::cos(alpha) * dist;
            rightY = std::sin(alpha) * dist;
            if (autoKeying || isOnKeyframe) {
                p.getCurve()->setPointLeftAndRightIndex(p, time, leftX, leftY, x + rightX, y + rightY);
            }
            if (rippleEdit) {
                for (std::set<int>::iterator it = times.begin(); it!=times.end() ; ++it) {
                    p.getCurve()->setPointLeftAndRightIndex(p, *it, leftX, leftY, x + rightX, y + rightY);
                }
            }
        } else {
            leftX = std::cos(alpha) * dist;
            leftY = std::sin(alpha) * dist;
            if (autoKeying || isOnKeyframe) {
                p.getCurve()->setPointLeftAndRightIndex(p, time, x + leftX , y + leftY , rightX , rightY);
            }
            if (rippleEdit) {
                for (std::set<int>::iterator it = times.begin(); it!=times.end() ; ++it) {
                    p.getCurve()->setPointLeftAndRightIndex(p, time, x + leftX , y + leftY , rightX , rightY);
                }
            }
        }
        
    }
    
}

void MoveTangentUndoCommand::undo()
{
    boost::shared_ptr<BezierCP> counterPart;
    if (_tangentBeingDragged->isFeatherPoint()) {
        counterPart = _tangentBeingDragged->getCurve()->getControlPointForFeatherPoint(_tangentBeingDragged);
        _oldCp->getCurve()->clonePoint(*counterPart,*_oldCp);
        _oldFp->getCurve()->clonePoint(*_tangentBeingDragged,*_oldFp);
    } else {
        counterPart = _tangentBeingDragged->getCurve()->getFeatherPointForControlPoint(_tangentBeingDragged);
        _oldCp->getCurve()->clonePoint(*_tangentBeingDragged,*_oldCp);
        _oldFp->getCurve()->clonePoint(*counterPart,*_oldFp);
    }
    
    if (_firstRedoCalled) {
        _roto->setSelection(_selectedCurves, _selectedPoints);
    }

    _roto->evaluate();
    
    setText(QString("Move tangent of %1 of %2").arg(_tangentBeingDragged->getCurve()->getName_mt_safe().c_str()).arg(_roto->getNodeName()));


}

void MoveTangentUndoCommand::redo()
{
    
    boost::shared_ptr<BezierCP> counterPart;
    if (_tangentBeingDragged->isFeatherPoint()) {
        counterPart = _tangentBeingDragged->getCurve()->getControlPointForFeatherPoint(_tangentBeingDragged);
        _oldCp->getCurve()->clonePoint(*_oldCp, *counterPart);
        _oldFp->getCurve()->clonePoint(*_oldFp, *_tangentBeingDragged);
    } else {
        counterPart = _tangentBeingDragged->getCurve()->getFeatherPointForControlPoint(_tangentBeingDragged);
        _oldCp->getCurve()->clonePoint(*_oldCp, *_tangentBeingDragged);
        _oldFp->getCurve()->clonePoint(*_oldFp, *counterPart);
    }
    
    bool autoKeying = _roto->getContext()->isAutoKeyingEnabled();
    dragTangent(_time, *_tangentBeingDragged, _dx, _dy, _left,autoKeying,_rippleEditEnabled);
    if (_featherLinkEnabled) {
        dragTangent(_time, *counterPart, _dx, _dy, _left,autoKeying,_rippleEditEnabled);
    }
    
    if (_firstRedoCalled) {
        _roto->setSelection(_selectedCurves, _selectedPoints);
    }
    
    _roto->evaluate();
    
    setText(QString("Move tangent of %1 of %2").arg(_tangentBeingDragged->getCurve()->getName_mt_safe().c_str()).arg(_roto->getNodeName()));
    
}

int MoveTangentUndoCommand::id() const
{
    return kRotoMoveTangentCompressionID;
}

bool MoveTangentUndoCommand::mergeWith(const QUndoCommand *other)
{
    const MoveTangentUndoCommand* mvCmd = dynamic_cast<const MoveTangentUndoCommand*>(other);
    if (!mvCmd) {
        return false;
    }
    if (mvCmd->_tangentBeingDragged != _tangentBeingDragged || mvCmd->_left != _left || mvCmd->_featherLinkEnabled != _featherLinkEnabled
        || mvCmd->_rippleEditEnabled != _rippleEditEnabled) {
        return false;
    }
    return true;
}


