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