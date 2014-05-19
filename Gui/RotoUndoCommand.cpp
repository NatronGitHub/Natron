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
#include <QTreeWidgetItem>

#include "Global/GlobalDefines.h"
#include "Engine/RotoContext.h"
#include "Gui/RotoGui.h"
#include "Gui/RotoPanel.h"

using namespace Natron;

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
    
    _roto->evaluate(true);
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
                (RotoGui::Roto_Tool)_selectedTool == RotoGui::SELECT_ALL ||
                (RotoGui::Roto_Tool)_selectedTool == RotoGui::DRAW_BEZIER) {
                index = it->second->getCurve()->getControlPointIndex(it->second);
                assert(index != -1);
                it->first->getCurve()->moveFeatherByIndex(index,_time, _dx, _dy);
            }
        } else {
            if ((RotoGui::Roto_Tool)_selectedTool == RotoGui::SELECT_POINTS ||
                (RotoGui::Roto_Tool)_selectedTool == RotoGui::SELECT_ALL ||
                (RotoGui::Roto_Tool)_selectedTool == RotoGui::DRAW_BEZIER) {
                index = it->first->getCurve()->getControlPointIndex(it->first);
                assert(index != -1);
                it->first->getCurve()->movePointByIndex(index,_time, _dx, _dy);
            }
        }
    }
    
    if (_firstRedoCalled) {
        _roto->setSelection(_selectedCurves, _selectedPoints);
        _roto->evaluate(true);
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
     _roto->evaluate(true);
    setText(QString("Add point to %1 of %2").arg(_curve->getName_mt_safe().c_str()).arg(_roto->getNodeName()));
}

void AddPointUndoCommand::redo()
{
    _oldCurve->clone(*_curve);
    boost::shared_ptr<BezierCP> cp = _curve->addControlPointAfterIndex(_index,_t);
    boost::shared_ptr<BezierCP> newFp = _curve->getFeatherPointAtIndex(_index + 1);

    _roto->setSelection(_curve, std::make_pair(cp, newFp));
    if (_firstRedoCalled) {
        _roto->evaluate(true);
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
            _roto->getContext()->addItem(it->parentLayer.get(), it->indexInLayer, it->curve,RotoContext::OVERLAY_INTERACT);
        }
        selection.push_back(it->curve);
    }
    
    _roto->setSelection(selection,cpSelection);
    _roto->evaluate(true);
    
    setText(QString("Remove points to %1").arg(_roto->getNodeName()));
}

void RemovePointUndoCommand::redo()
{
    
    ///clone the curve
    for (std::list< CurveDesc >::iterator it = _curves.begin(); it!=_curves.end(); ++it) {
        it->oldCurve->clone(*(it->curve));
    }
    
    std::list<Bezier*> toRemove;
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
        }
    }
   
    for (std::list<Bezier*>::iterator it = toRemove.begin(); it!=toRemove.end(); ++it) {
        _roto->removeCurve(*it);
    }



    _roto->setSelection(BezierPtr(),std::make_pair(CpPtr(), CpPtr()));
    _roto->evaluate(_firstRedoCalled);
    _firstRedoCalled = true;

    setText(QString("Remove points to %1").arg(_roto->getNodeName()));
}

//////////////////////////

RemoveCurveUndoCommand::RemoveCurveUndoCommand(RotoGui* roto,const std::list<boost::shared_ptr<Bezier> >& curves)
: QUndoCommand()
, _roto(roto)
, _firstRedoCalled(false)
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
        _roto->getContext()->addItem(it->layer.get(),it->indexInLayer, it->curve,RotoContext::OVERLAY_INTERACT);
        selection.push_back(it->curve);
    }
    
    SelectedCpList cpList;
    _roto->setSelection(selection, cpList);
    _roto->evaluate(true);
    
    setText(QString("Remove curves to %1").arg(_roto->getNodeName()));
}

void RemoveCurveUndoCommand::redo()
{
    for (std::list<RemovedCurve>::iterator it = _curves.begin(); it!=_curves.end(); ++it) {
        _roto->removeCurve(it->curve.get());
    }
    _roto->evaluate(_firstRedoCalled);
    _roto->setSelection(BezierPtr(), std::make_pair(CpPtr(), CpPtr()));
    _firstRedoCalled = true;
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

    _roto->evaluate(true);
    
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
        _roto->evaluate(true);
    }
    
    
    _firstRedoCalled = true;
    
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

//////////////////////////

    
MoveFeatherBarUndoCommand::MoveFeatherBarUndoCommand(RotoGui* roto,double dx,double dy,
                              const std::pair<boost::shared_ptr<BezierCP>,boost::shared_ptr<BezierCP> >& point,
                              int time)
: QUndoCommand()
, _roto(roto)
, _firstRedoCalled(false)
, _dx(dx)
, _dy(dy)
, _rippleEditEnabled(roto->getContext()->isRippleEditEnabled())
, _time(time)
, _curve()
, _oldPoint()
, _newPoint(point)
{
    _curve = boost::dynamic_pointer_cast<Bezier>(_roto->getContext()->getItemByName(point.first->getCurve()->getName_mt_safe()));
    assert(_curve);
    _oldPoint.first.reset(new BezierCP(*_newPoint.first));
    _oldPoint.second.reset(new BezierCP(*_newPoint.second));
}
    
MoveFeatherBarUndoCommand::~MoveFeatherBarUndoCommand()
{
    
}
    
void MoveFeatherBarUndoCommand::undo()
{
    
    _curve->clonePoint(*_newPoint.first, *_oldPoint.first);
    _curve->clonePoint(*_newPoint.second, *_oldPoint.second);

    _roto->evaluate(true);
    _roto->setSelection(_curve, _newPoint);
    setText(QString("Move feather bar of %1 of %2").arg(_curve->getName_mt_safe().c_str()).arg(_roto->getNodeName()));
    
}
    
void MoveFeatherBarUndoCommand::redo()
{
    
    _curve->clonePoint(*_oldPoint.first, *_newPoint.first);
    _curve->clonePoint(*_oldPoint.second, *_newPoint.second);
    
    boost::shared_ptr<BezierCP> p = _newPoint.first->isFeatherPoint() ?
    _newPoint.second : _newPoint.first;
    boost::shared_ptr<BezierCP> fp = _newPoint.first->isFeatherPoint() ?
    _newPoint.first : _newPoint.second;
    
    Point delta;
    Point featherPoint,controlPoint;
    p->getPositionAtTime(_time, &controlPoint.x, &controlPoint.y);
    bool isOnKeyframe = fp->getPositionAtTime(_time, &featherPoint.x, &featherPoint.y);
    
    if (controlPoint.x != featherPoint.x || controlPoint.y != featherPoint.y) {
        Point featherVec;
        featherVec.x = featherPoint.x - controlPoint.x;
        featherVec.y = featherPoint.y - controlPoint.y;
        double norm = sqrt((featherPoint.x - controlPoint.x) * (featherPoint.x - controlPoint.x)
                           + (featherPoint.y - controlPoint.y) * (featherPoint.y - controlPoint.y));
        assert(norm != 0);
        delta.x = featherVec.x / norm;
        delta.y = featherVec.y / norm;
        
        double dotProduct = delta.x * _dx + delta.y * _dy;
        delta.x = delta.x * dotProduct;
        delta.y = delta.y * dotProduct;
        
    } else {
        
        ///the feather point equals the control point, use derivatives
        const std::list<boost::shared_ptr<BezierCP> >& cps = p->getCurve()->getFeatherPoints();
        assert(cps.size() > 1);
        
        std::list<boost::shared_ptr<BezierCP> >::const_iterator prev = cps.end();
        --prev;
        std::list<boost::shared_ptr<BezierCP> >::const_iterator next = cps.begin();
        ++next;
        std::list<boost::shared_ptr<BezierCP> >::const_iterator cur = cps.begin();
        for (; cur!=cps.end(); ++cur,++prev,++next) {
            if (prev == cps.end()) {
                prev = cps.begin();
            }
            if (next == cps.end()) {
                next = cps.begin();
            }
            
            if (*cur == fp) {
                break;
            }
        }
        assert( cur != cps.end() );
        
        double leftX,leftY,rightX,rightY,norm;
        Bezier::leftDerivativeAtPoint(_time, **cur, **prev, &leftX, &leftY);
        Bezier::rightDerivativeAtPoint(_time, **cur, **next, &rightX, &rightY);
        norm = sqrt((rightX - leftX) * (rightX - leftX) + (rightY - leftY) * (rightY - leftY));
        
        ///normalize derivatives by their norm
        if (norm != 0) {
            delta.x = -((rightY - leftY) / norm);
            delta.y = ((rightX - leftX) / norm) ;
        } else {
            ///both derivatives are the same, use the direction of the left one
            norm = sqrt((leftX - featherPoint.x) * (leftX - featherPoint.x) + (leftY - featherPoint.y) * (leftY - featherPoint.y));
            if (norm != 0) {
                delta.x = -((leftY - featherPoint.y) / norm);
                delta.y = ((leftX - featherPoint.x) / norm);
            } else {
                ///both derivatives and control point are equal, just use 0
                delta.x = delta.y = 0;
            }
        }
        
        double dotProduct = delta.x * _dx + delta.y * _dy;
        delta.x = delta.x * dotProduct;
        delta.y = delta.y * dotProduct;
    }
    
    Point extent;
    
    extent.x = featherPoint.x + delta.x;
    extent.y = featherPoint.y + delta.y;
    
    if (_roto->getContext()->isAutoKeyingEnabled() || isOnKeyframe) {
        int index = fp->getCurve()->getFeatherPointIndex(fp);
        double leftX,leftY,rightX,rightY;
        
        fp->getLeftBezierPointAtTime(_time, &leftX, &leftY);
        fp->getRightBezierPointAtTime(_time, &rightX, &rightY);
        
        fp->getCurve()->setPointAtIndex(true, index, _time, extent.x,extent.y,
                                        leftX + delta.x, leftY + delta.y,
                                        rightX + delta.x, rightY + delta.y);
        
    }
    if (_firstRedoCalled) {
        _roto->evaluate(true);
    }
    
    _roto->setSelection(_curve, _newPoint);
    
    _firstRedoCalled = true;
    setText(QString("Move feather bar of %1 of %2").arg(_curve->getName_mt_safe().c_str()).arg(_roto->getNodeName()));
    
    
}

int MoveFeatherBarUndoCommand::id() const
{
    return kRotoMoveFeatherBarCompressionID;
}

bool MoveFeatherBarUndoCommand::mergeWith(const QUndoCommand *other)
{
    const MoveFeatherBarUndoCommand* mvCmd = dynamic_cast<const MoveFeatherBarUndoCommand*>(other);
    if (!mvCmd) {
        return false;
    }
    if (mvCmd->_newPoint.first != _newPoint.first || mvCmd->_newPoint.second!=_newPoint.second ||
        mvCmd->_rippleEditEnabled != _rippleEditEnabled || mvCmd->_time != _time) {
        return false;
    }
    return true;
}
    


/////////////////////////

RemoveFeatherUndoCommand::RemoveFeatherUndoCommand(RotoGui* roto,const boost::shared_ptr<Bezier>& curve,
                         const boost::shared_ptr<BezierCP>& fp)
: QUndoCommand()
, _roto(roto)
, _firstRedocalled(false)
, _curve(curve)
, _oldFp(new BezierCP(*fp))
, _newFp(fp)
{
    
}

RemoveFeatherUndoCommand::~RemoveFeatherUndoCommand()
{
    
}

void RemoveFeatherUndoCommand::undo()
{
    _curve->clonePoint(*_newFp, *_oldFp);
    _roto->evaluate(true);

    boost::shared_ptr<BezierCP> cp = _curve->getControlPointForFeatherPoint(_newFp);
    _roto->setSelection(_curve, std::make_pair(cp, _newFp));
    setText(QString("Remove feather of %1 of %2").arg(_curve->getName_mt_safe().c_str()).arg(_roto->getNodeName()));
}

void RemoveFeatherUndoCommand::redo()
{
    _curve->clonePoint(*_oldFp, *_newFp);
    try {
        _curve->removeFeatherAtIndex(_curve->getFeatherPointIndex(_newFp));
    } catch (...) {
        ///the point doesn't exist anymore, just do nothing
        return;
    }
    
    _roto->evaluate(_firstRedocalled);
    
    boost::shared_ptr<BezierCP> cp = _curve->getControlPointForFeatherPoint(_newFp);
    _roto->setSelection(_curve, std::make_pair(cp, _newFp));
    
    _firstRedocalled = true;
    
    setText(QString("Remove feather of %1 of %2").arg(_curve->getName_mt_safe().c_str()).arg(_roto->getNodeName()));
}

////////////////////////////


OpenCloseUndoCommand::OpenCloseUndoCommand(RotoGui* roto,const boost::shared_ptr<Bezier>& curve)
: QUndoCommand()
, _roto(roto)
, _firstRedoCalled(false)
, _selectedTool((int)roto->getSelectedTool())
, _curve(curve)
{
    
}

OpenCloseUndoCommand::~OpenCloseUndoCommand()
{
    
}

void OpenCloseUndoCommand::undo()
{
    if (_firstRedoCalled) {
        _roto->setCurrentTool((RotoGui::Roto_Tool)_selectedTool);
        if ((RotoGui::Roto_Tool)_selectedTool == RotoGui::DRAW_BEZIER) {
            _roto->setBuiltBezier(_curve);
        }
    }
    _curve->setCurveFinished(!_curve->isCurveFinished());
    _roto->evaluate(true);
    _roto->setSelection(_curve, std::make_pair(CpPtr(), CpPtr()));
    setText(QString("Open/Close %1 of %2").arg(_curve->getName_mt_safe().c_str()).arg(_roto->getNodeName()));
}

void OpenCloseUndoCommand::redo()
{
    if (_firstRedoCalled) {
        _roto->setCurrentTool((RotoGui::Roto_Tool)_selectedTool);
    }
    _curve->setCurveFinished(!_curve->isCurveFinished());
    _roto->evaluate(_firstRedoCalled);
    _roto->setSelection(_curve, std::make_pair(CpPtr(), CpPtr()));
    _firstRedoCalled = true;
    setText(QString("Open/Close %1 of %2").arg(_curve->getName_mt_safe().c_str()).arg(_roto->getNodeName()));
}

////////////////////////////

SmoothCuspUndoCommand::SmoothCuspUndoCommand(RotoGui* roto,const boost::shared_ptr<Bezier>& curve,
                      const std::pair<boost::shared_ptr<BezierCP>,boost::shared_ptr<BezierCP> >& point,
                      int time,bool cusp)
: QUndoCommand()
, _roto(roto)
, _firstRedoCalled(false)
, _curve(curve)
, _time(time)
, _count(1)
, _cusp(cusp)
, _oldPoint()
, _newPoint(point)
{
    _oldPoint.first.reset(new BezierCP(*point.first));
    _oldPoint.second.reset(new BezierCP(*point.second));
}

SmoothCuspUndoCommand::~SmoothCuspUndoCommand()
{

}
void SmoothCuspUndoCommand::undo()
{
    _curve->clonePoint(*_newPoint.first, *_oldPoint.first);
    _curve->clonePoint(*_newPoint.second, *_oldPoint.second);
    
    _roto->evaluate(true);
    _roto->setSelection(_curve, _newPoint);
    if (_cusp) {
        setText(QString("Cusp %1 of %2").arg(_curve->getName_mt_safe().c_str()).arg(_roto->getNodeName()));
    } else {
        setText(QString("Smooth %1 of %2").arg(_curve->getName_mt_safe().c_str()).arg(_roto->getNodeName()));
    }
}

void SmoothCuspUndoCommand::redo()
{
    
    _curve->clonePoint(*_oldPoint.first, *_newPoint.first);
    _curve->clonePoint(*_oldPoint.second, *_newPoint.second);
    
    for (int i = 0; i < _count; ++i) {
        int index = _curve->getControlPointIndex(_newPoint.first->isFeatherPoint() ? _newPoint.second : _newPoint.first);
        assert(index != -1);
        
        if (_cusp) {
            _curve->cuspPointAtIndex(index, _time);
        } else {
            _curve->smoothPointAtIndex(index, _time);
        }
    }
    
    
    _roto->evaluate(_firstRedoCalled);
    _roto->setSelection(_curve, _newPoint);
    
    _firstRedoCalled = true;
    if (_cusp) {
        setText(QString("Cusp %1 of %2").arg(_curve->getName_mt_safe().c_str()).arg(_roto->getNodeName()));
    } else {
        setText(QString("Smooth %1 of %2").arg(_curve->getName_mt_safe().c_str()).arg(_roto->getNodeName()));
    }
    
}

int SmoothCuspUndoCommand::id() const
{
    return kRotoCuspSmoothCompressionID;
}

bool SmoothCuspUndoCommand::mergeWith(const QUndoCommand *other)
{
    const SmoothCuspUndoCommand* sCmd = dynamic_cast<const SmoothCuspUndoCommand*>(other);
    if (!sCmd) {
        return false;
    }
    if (sCmd->_newPoint.first != _newPoint.first || sCmd->_newPoint.second != _newPoint.second ||
        sCmd->_cusp != _cusp || sCmd->_time != _time) {
        return false;
    }
    ++_count;
    return true;
}

/////////////////////////

MakeBezierUndoCommand::MakeBezierUndoCommand(RotoGui* roto,const boost::shared_ptr<Bezier>& curve,bool createPoint,double dx,double dy,int time)
: QUndoCommand()
, _firstRedoCalled(false)
, _roto(roto)
, _parentLayer()
, _indexInLayer(0)
, _oldCurve()
, _newCurve(curve)
, _curveNonExistant(false)
, _createdPoint(createPoint)
, _x(dx)
, _y(dy)
, _dx(createPoint ? 0. : dx)
, _dy(createPoint ? 0. : dy)
, _time(time)
, _lastPointAdded(-1)
{
    if (!_newCurve) {
        _curveNonExistant = true;
    } else {
        _oldCurve.reset(new Bezier(*_newCurve));
    }
}

MakeBezierUndoCommand::~MakeBezierUndoCommand()
{
    
}

void MakeBezierUndoCommand::undo()
{
    assert(_createdPoint);
    _roto->setCurrentTool(RotoGui::DRAW_BEZIER);
    assert(_lastPointAdded != -1);
    _oldCurve->clone(*_newCurve);
    _newCurve->removeControlPointByIndex(_lastPointAdded);
    if (_newCurve->getControlPointsCount() == 0) {
         _curveNonExistant = true;
        _roto->removeCurve(_newCurve.get());
    }
    if (!_curveNonExistant) {
        _roto->setSelection(_newCurve, std::make_pair(CpPtr(), CpPtr()));
        _roto->setBuiltBezier(_newCurve);
    } else {
        _roto->setSelection(BezierPtr(), std::make_pair(CpPtr(), CpPtr()));
    }
    _roto->evaluate(true);
    setText(QString("Build bezier %1 of %2").arg(_newCurve->getName_mt_safe().c_str()).arg(_roto->getNodeName()));
    
}

void MakeBezierUndoCommand::redo()
{
    if (_firstRedoCalled) {
        _roto->setCurrentTool(RotoGui::DRAW_BEZIER);
    }
    
    if (!_firstRedoCalled) {
        if (_createdPoint) {
            if (!_newCurve) {
                _newCurve = _roto->getContext()->makeBezier(_x, _y, kRotoBezierBaseName);
                _oldCurve.reset(new Bezier(*_newCurve));
                _lastPointAdded = 0;
                 _curveNonExistant = false;
            } else {
                _oldCurve->clone(*_newCurve);
                _newCurve->addControlPoint(_x, _y);
                int lastIndex = _newCurve->getControlPointsCount() - 1;
                assert(lastIndex > 0);
                _lastPointAdded = lastIndex;
            }
        } else {
            _oldCurve->clone(*_newCurve);
            int lastIndex = _newCurve->getControlPointsCount() - 1;
            assert(lastIndex >= 0);
            _lastPointAdded = lastIndex;
            _newCurve->moveLeftBezierPoint(lastIndex ,_time, -_dx, -_dy);
            _newCurve->moveRightBezierPoint(lastIndex, _time, _dx, _dy);
        }
        _parentLayer = boost::dynamic_pointer_cast<RotoLayer>(_roto->getContext()->
                                                              getItemByName(_newCurve->getParentLayer()->getName_mt_safe()));
        _indexInLayer = _parentLayer->getChildIndex(_newCurve);

    } else {
        _newCurve->clone(*_oldCurve);
        if (_curveNonExistant) {
            _roto->getContext()->addItem(_parentLayer.get(), _indexInLayer, _newCurve,RotoContext::OVERLAY_INTERACT);
        }
    }

    
    boost::shared_ptr<BezierCP> cp = _newCurve->getControlPointAtIndex(_lastPointAdded);
    boost::shared_ptr<BezierCP> fp = _newCurve->getFeatherPointAtIndex(_lastPointAdded);
    _roto->setSelection(_newCurve, std::make_pair(cp, fp));
    _roto->evaluate(_firstRedoCalled);
    
    _firstRedoCalled = true;
    
    
    
    setText(QString("Build bezier %1 of %2").arg(_newCurve->getName_mt_safe().c_str()).arg(_roto->getNodeName()));
}

int MakeBezierUndoCommand::id() const
{
    return kRotoMakeBezierCompressionID;
}

bool MakeBezierUndoCommand::mergeWith(const QUndoCommand *other)
{
    const MakeBezierUndoCommand* sCmd = dynamic_cast<const MakeBezierUndoCommand*>(other);
    if (!sCmd) {
        return false;
    }
    
    if (sCmd->_createdPoint || sCmd->_newCurve != _newCurve) {
        return false;
    }
    
    if (!sCmd->_createdPoint) {
        _dx += sCmd->_dx;
        _dy += sCmd->_dy;
    }
    
    return true;
}

//////////////////////////////


MakeEllipseUndoCommand::MakeEllipseUndoCommand(RotoGui* roto,bool create,bool fromCenter,double dx,double dy,int time)
: QUndoCommand()
, _firstRedoCalled(false)
, _roto(roto)
, _newCurve()
, _curve()
, _create(create)
, _fromCenter(fromCenter)
, _x(dx)
, _y(dy)
, _dx(create ? 0 : dx)
, _dy(create ? 0 : dy)
, _time(time)
{
    if (!_create) {
        _curve = _roto->getBezierBeingBuild();
    }
    
}

MakeEllipseUndoCommand::~MakeEllipseUndoCommand()
{
    
}

void MakeEllipseUndoCommand::undo() {
    
    _roto->removeCurve(_curve.get());
    _roto->evaluate(true);
    _roto->setSelection(BezierPtr(), std::make_pair(CpPtr(), CpPtr()));
    setText(QString("Build Ellipse %1 of %2").arg(_curve->getName_mt_safe().c_str()).arg(_roto->getNodeName()));
}

void MakeEllipseUndoCommand::redo()
{
    if (_firstRedoCalled) {
        _curve->clone(*_newCurve);
        _roto->getContext()->addItem(_parentLayer.get(), _indexInLayer, _curve,RotoContext::OVERLAY_INTERACT);
        _roto->evaluate(true);
    } else {
        if (_create) {
            _curve = _roto->getContext()->makeBezier(_x,_y,kRotoEllipseBaseName);
            _curve->addControlPoint(_x,_y);
            _curve->addControlPoint(_x,_y);
            _curve->addControlPoint(_x,_y);
            _curve->setCurveFinished(true);
            _newCurve.reset(new Bezier(*_curve));
        } else {
            boost::shared_ptr<BezierCP> top = _curve->getControlPointAtIndex(0);
            boost::shared_ptr<BezierCP> right = _curve->getControlPointAtIndex(1);
            boost::shared_ptr<BezierCP> bottom = _curve->getControlPointAtIndex(2);
            boost::shared_ptr<BezierCP> left = _curve->getControlPointAtIndex(3);
            if (_fromCenter) {
                
                
                //top only moves by x
                _curve->movePointByIndex(0,_time, 0, _dy);
                
                //right
                _curve->movePointByIndex(1,_time, _dx , 0);
                
                //bottom
                _curve->movePointByIndex(2,_time, 0., -_dy );
                
                //left only moves by y
                _curve->movePointByIndex(3,_time, -_dx, 0);
                double topX,topY,rightX,rightY,btmX,btmY,leftX,leftY;
                top->getPositionAtTime(_time, &topX, &topY);
                right->getPositionAtTime(_time, &rightX, &rightY);
                bottom->getPositionAtTime(_time, &btmX, &btmY);
                left->getPositionAtTime(_time, &leftX, &leftY);
                
                _curve->setLeftBezierPoint(0, _time,  (leftX + topX) / 2., topY);
                _curve->setRightBezierPoint(0, _time, (rightX + topX) / 2., topY);
                
                _curve->setLeftBezierPoint(1, _time,  rightX, (rightY + topY) / 2.);
                _curve->setRightBezierPoint(1, _time, rightX, (rightY + btmY) / 2.);
                
                _curve->setLeftBezierPoint(2, _time,  (rightX + btmX) / 2., btmY);
                _curve->setRightBezierPoint(2, _time, (leftX + btmX) / 2., btmY);
                
                _curve->setLeftBezierPoint(3, _time,   leftX, (btmY + leftY) / 2.);
                _curve->setRightBezierPoint(3, _time, leftX, (topY + leftY) / 2.);

            } else {

                
                //top only moves by x
                _curve->movePointByIndex(0,_time, _dx / 2., 0);
                
                //right
                _curve->movePointByIndex(1,_time, _dx, _dy / 2.);
                
                //bottom
                _curve->movePointByIndex(2,_time, _dx / 2., _dy );
                
                //left only moves by y
                _curve->movePointByIndex(3,_time, 0, _dy / 2.);
                
                double topX,topY,rightX,rightY,btmX,btmY,leftX,leftY;
                top->getPositionAtTime(_time, &topX, &topY);
                right->getPositionAtTime(_time, &rightX, &rightY);
                bottom->getPositionAtTime(_time, &btmX, &btmY);
                left->getPositionAtTime(_time, &leftX, &leftY);
                
                _curve->setLeftBezierPoint(0, _time,  (leftX + topX) / 2., topY);
                _curve->setRightBezierPoint(0, _time, (rightX + topX) / 2., topY);
                
                _curve->setLeftBezierPoint(1, _time,  rightX, (rightY + topY) / 2.);
                _curve->setRightBezierPoint(1, _time, rightX, (rightY + btmY) / 2.);
                
                _curve->setLeftBezierPoint(2, _time,  (rightX + btmX) / 2., btmY);
                _curve->setRightBezierPoint(2, _time, (leftX + btmX) / 2., btmY);
                
                _curve->setLeftBezierPoint(3, _time,   leftX, (btmY + leftY) / 2.);
                _curve->setRightBezierPoint(3, _time, leftX, (topY + leftY) / 2.);
                

            }
        }
        _parentLayer = boost::dynamic_pointer_cast<RotoLayer>(_roto->getContext()->
                                                              getItemByName(_curve->getParentLayer()->getName_mt_safe()));
        _indexInLayer = _parentLayer->getChildIndex(_curve);
    }
    _roto->setBuiltBezier(_curve);
    _firstRedoCalled = true;
    _roto->setSelection(_curve, std::make_pair(CpPtr(), CpPtr()));
    setText(QString("Build Ellipse %1 of %2").arg(_curve->getName_mt_safe().c_str()).arg(_roto->getNodeName()));
}

int MakeEllipseUndoCommand::id() const
{
    return kRotoMakeEllipseCompressionID;
}

bool MakeEllipseUndoCommand::mergeWith(const QUndoCommand *other)
{
    const MakeEllipseUndoCommand* sCmd = dynamic_cast<const MakeEllipseUndoCommand*>(other);
    if (!sCmd) {
        return false;
    }
    if (sCmd->_curve != _curve || sCmd->_create) {
        return false;
    }
    _newCurve->clone(*sCmd->_curve);
    _dx += sCmd->_dx;
    _dy += sCmd->_dy;
    return true;
}

////////////////////////////////////


MakeRectangleUndoCommand::MakeRectangleUndoCommand(RotoGui* roto,bool create,double dx,double dy,int time)
: QUndoCommand()
, _firstRedoCalled(false)
, _roto(roto)
, _parentLayer()
, _indexInLayer(-1)
, _newCurve()
, _curve()
, _create(create)
, _x(dx)
, _y(dy)
, _dx(create ? 0 : dx)
, _dy(create ? 0 : dy)
, _time(time)
{
    if (!_create) {
        _curve = _roto->getBezierBeingBuild();
    }
}

MakeRectangleUndoCommand::~MakeRectangleUndoCommand()
{
    
}

void MakeRectangleUndoCommand::undo()
{
    _roto->removeCurve(_curve.get());
    _roto->evaluate(true);
    _roto->setSelection(BezierPtr(), std::make_pair(CpPtr(), CpPtr()));
    setText(QString("Build Ellipse %1 of %2").arg(_curve->getName_mt_safe().c_str()).arg(_roto->getNodeName()));
}

void MakeRectangleUndoCommand::redo()
{
    
    if (_firstRedoCalled) {
        _curve->clone(*_newCurve);
        _roto->getContext()->addItem(_parentLayer.get(), _indexInLayer, _curve,RotoContext::OVERLAY_INTERACT);
        _roto->evaluate(true);
    } else {
        if (_create) {
            _curve = _roto->getContext()->makeBezier(_x,_y,kRotoRectangleBaseName);
            _curve->addControlPoint(_x,_y);
            _curve->addControlPoint(_x,_y);
            _curve->addControlPoint(_x,_y);
            _curve->setCurveFinished(true);
            _newCurve.reset(new Bezier(*_curve));
        } else {
            _curve->movePointByIndex(1,_time, _dx, 0);
            _curve->movePointByIndex(2,_time, _dx, _dy);
            _curve->movePointByIndex(3,_time, 0, _dy);
        }
        _parentLayer = boost::dynamic_pointer_cast<RotoLayer>(_roto->getContext()->
                                                              getItemByName(_curve->getParentLayer()->getName_mt_safe()));
        _indexInLayer = _parentLayer->getChildIndex(_curve);
        
    }
    _roto->setBuiltBezier(_curve);
    _firstRedoCalled = true;
    _roto->setSelection(_curve, std::make_pair(CpPtr(), CpPtr()));
    setText(QString("Build Rectangle %1 of %2").arg(_curve->getName_mt_safe().c_str()).arg(_roto->getNodeName()));
}

int MakeRectangleUndoCommand::id() const
{
    return kRotoMakeRectangleCompressionID;
}

bool MakeRectangleUndoCommand::mergeWith(const QUndoCommand *other)
{
    const MakeRectangleUndoCommand* sCmd = dynamic_cast<const MakeRectangleUndoCommand*>(other);
    if (!sCmd) {
        return false;
    }
    if (sCmd->_curve != _curve || sCmd->_create) {
        return false;
    }
    _newCurve->clone(*sCmd->_curve);
    _dx += sCmd->_dx;
    _dy += sCmd->_dy;
    return true;
}

//////////////////////////////


RemoveItemsUndoCommand::RemoveItemsUndoCommand(RotoPanel* roto,const QList<QTreeWidgetItem*>& items)
: QUndoCommand()
, _roto(roto)
, _items()
{
    
    for (QList<QTreeWidgetItem*>::const_iterator it = items.begin(); it!=items.end(); ++it) {
        RemovedItem r;
        r.treeItem = *it;
        r.parentTreeItem = r.treeItem->parent();
        r.item = _roto->getRotoItemForTreeItem(r.treeItem);
        assert(r.item);
        if (r.parentTreeItem) {
            r.parentLayer = boost::dynamic_pointer_cast<RotoLayer>(_roto->getRotoItemForTreeItem(r.parentTreeItem));
            assert(r.parentLayer);
            r.indexInLayer = r.parentLayer->getChildIndex(r.item);
        }
        _items.push_back(r);
    }
}

RemoveItemsUndoCommand::~RemoveItemsUndoCommand()
{
    
}

void RemoveItemsUndoCommand::undo()
{
    for (std::list<RemovedItem>::iterator it = _items.begin(); it!= _items.end(); ++it) {
        _roto->getContext()->addItem(it->parentLayer.get(), it->indexInLayer, it->item,RotoContext::SETTINGS_PANEL);
        if (it->parentTreeItem) {
            it->parentTreeItem->addChild(it->treeItem);
        }
        it->treeItem->setHidden(false);
    }
    _roto->getContext()->evaluateChange();
    setText(QString("Remove items of %2").arg(_roto->getNodeName().c_str()));
}

void RemoveItemsUndoCommand::redo()
{
    for (std::list<RemovedItem>::iterator it = _items.begin(); it!= _items.end(); ++it) {
        _roto->getContext()->removeItem(it->item.get(),RotoContext::SETTINGS_PANEL);
        it->treeItem->setHidden(true);
        if (it->treeItem->isSelected()) {
            it->treeItem->setSelected(false);
        }
        if (it->parentTreeItem) {
            it->parentTreeItem->removeChild(it->treeItem);
        }
    }
    _roto->clearSelection();
    _roto->getContext()->evaluateChange();
    setText(QString("Remove items of %2").arg(_roto->getNodeName().c_str()));
}


/////////////////////////////


AddLayerUndoCommand::AddLayerUndoCommand(RotoPanel* roto)
: QUndoCommand()
, _roto(roto)
, _layer()
{
    
}

AddLayerUndoCommand::~AddLayerUndoCommand()
{
    
}

void AddLayerUndoCommand::undo()
{
    _roto->getContext()->removeItem(_layer.get());
    _roto->clearSelection();
    _roto->getContext()->evaluateChange();
    _layer.reset();
    setText(QString("Add layer to %2").arg(_roto->getNodeName().c_str()));
}

void AddLayerUndoCommand::redo()
{
    _layer = _roto->getContext()->addLayer();
    _roto->clearSelection();
    _roto->getContext()->select(_layer, RotoContext::OTHER);
    _roto->getContext()->evaluateChange();
    setText(QString("Add layer to %2").arg(_roto->getNodeName().c_str()));
}