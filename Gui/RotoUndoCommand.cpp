/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "RotoUndoCommand.h"

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QTreeWidgetItem>
#include <QDebug>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/GlobalDefines.h"

#include "Engine/Bezier.h"
#include "Engine/BezierCP.h"
#include "Engine/FeatherPoint.h"
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/Project.h"
#include "Engine/RotoContext.h"
#include "Engine/RotoLayer.h"
#include "Engine/RotoStrokeItem.h"
#include "Engine/Transform.h"

#include "Gui/GuiAppInstance.h"
#include "Gui/RotoGui.h"
#include "Gui/RotoPanel.h"



NATRON_NAMESPACE_USING

typedef boost::shared_ptr<BezierCP> CpPtr;
typedef std::pair<CpPtr,CpPtr> SelectedCp;
typedef std::list<SelectedCp> SelectedCpList;
typedef boost::shared_ptr<Bezier> BezierPtr;
typedef std::list<BezierPtr> BezierList;

MoveControlPointsUndoCommand::MoveControlPointsUndoCommand(RotoGui* roto,
                                                           const std::list< std::pair<boost::shared_ptr<BezierCP>,boost::shared_ptr<BezierCP> > > & toDrag
                                                           ,
                                                           double dx,
                                                           double dy,
                                                           double time)
    : QUndoCommand()
      , _firstRedoCalled(false)
      , _roto(roto)
      , _dx(dx)
      , _dy(dy)
      , _featherLinkEnabled( roto->getContext()->isFeatherLinkEnabled() )
      , _rippleEditEnabled( roto->getContext()->isRippleEditEnabled() )
      , _selectedTool( (int)roto->getSelectedTool() )
      , _time(time)
      , _pointsToDrag(toDrag)
{
    assert(roto);

    roto->getSelection(&_selectedCurves, &_selectedPoints);

    ///we make a copy of the points
    for (SelectedCpList::iterator it = _pointsToDrag.begin(); it != _pointsToDrag.end(); ++it) {
        CpPtr first,second;
        if (it->first->isFeatherPoint()) {
            first.reset( new FeatherPoint(it->first->getBezier()));
            first->clone(*(it->first));
        } else {
            first.reset( new BezierCP( *(it->first) ) );
        }
        
        if (it->second) {
            if (it->second->isFeatherPoint()) {
                second.reset( new FeatherPoint(it->second->getBezier()));
                second->clone(*(it->second));
            } else {
                second.reset( new BezierCP( *(it->second) ) );
            }
        }
        _originalPoints.push_back( std::make_pair(first, second) );
    }

    for (SelectedCpList::iterator it = _pointsToDrag.begin(); it != _pointsToDrag.end(); ++it) {
        if ( !it->first->isFeatherPoint() ) {
            _indexesToMove.push_back( it->first->getBezier()->getControlPointIndex(it->first) );
        } else {
            _indexesToMove.push_back( it->second->getBezier()->getControlPointIndex(it->second) );
        }
    }
}

MoveControlPointsUndoCommand::~MoveControlPointsUndoCommand()
{
}

void
MoveControlPointsUndoCommand::undo()
{
    
    
    SelectedCpList::iterator cpIt = _originalPoints.begin();

    std::set<Bezier*> beziers;
    for (SelectedCpList::iterator it = _pointsToDrag.begin(); it != _pointsToDrag.end(); ++it) {
        beziers.insert(it->first->getBezier().get());
    }
    
    for (SelectedCpList::iterator it = _pointsToDrag.begin(); it != _pointsToDrag.end(); ++it, ++cpIt) {
        it->first->clone(*cpIt->first);
        if (it->second) {
            it->second->clone(*cpIt->second);
        }
    }
    
    for (std::set<Bezier*>::iterator it = beziers.begin(); it!=beziers.end(); ++it) {
        (*it)->incrementNodesAge();
    }

    _roto->evaluate(true);
    _roto->setCurrentTool( (RotoGui::RotoToolEnum)_selectedTool,true );
    _roto->setSelection(_selectedCurves, _selectedPoints);
    setText( QObject::tr("Move points of %1").arg( _roto->getNodeName() ) );
}

void
MoveControlPointsUndoCommand::redo()
{
    SelectedCpList::iterator itPoints = _pointsToDrag.begin();

    assert( _pointsToDrag.size() == _indexesToMove.size() );

    try {
        for (std::list<int>::iterator it = _indexesToMove.begin(); it != _indexesToMove.end(); ++it, ++itPoints) {
            if ( itPoints->first->isFeatherPoint() ) {
                if ( ( (RotoGui::RotoToolEnum)_selectedTool == RotoGui::eRotoToolSelectFeatherPoints ) ||
                     ( (RotoGui::RotoToolEnum)_selectedTool == RotoGui::eRotoToolSelectAll ) ||
                     ( (RotoGui::RotoToolEnum)_selectedTool == RotoGui::eRotoToolDrawBezier ) ) {
                    itPoints->first->getBezier()->moveFeatherByIndex(*it,_time, _dx, _dy);
                }
            } else {
                if ( ( (RotoGui::RotoToolEnum)_selectedTool == RotoGui::eRotoToolSelectPoints ) ||
                     ( (RotoGui::RotoToolEnum)_selectedTool == RotoGui::eRotoToolSelectAll ) ||
                     ( (RotoGui::RotoToolEnum)_selectedTool == RotoGui::eRotoToolDrawBezier ) ) {
                    itPoints->first->getBezier()->movePointByIndex(*it,_time, _dx, _dy);
                }
            }
        }
    } catch (const std::exception & e) {
        qDebug() << "Exception while operating MoveControlPointsUndoCommand::redo(): " << e.what();
    }

    if (_firstRedoCalled) {
        _roto->setSelection(_selectedCurves, _selectedPoints);
        _roto->evaluate(true);
    }

    _firstRedoCalled = true;
    setText( QObject::tr("Move points of %1").arg( _roto->getNodeName() ) );
}

int
MoveControlPointsUndoCommand::id() const
{
    return kRotoMoveControlPointsCompressionID;
}

bool
MoveControlPointsUndoCommand::mergeWith(const QUndoCommand *other)
{
    const MoveControlPointsUndoCommand* mvCmd = dynamic_cast<const MoveControlPointsUndoCommand*>(other);

    if (!mvCmd) {
        return false;
    }

    if ( ( mvCmd->_pointsToDrag.size() != _pointsToDrag.size() ) || (mvCmd->_time != _time) || (mvCmd->_selectedTool != _selectedTool)
         || ( mvCmd->_rippleEditEnabled != _rippleEditEnabled) || ( mvCmd->_featherLinkEnabled != _featherLinkEnabled) ) {
        return false;
    }

    std::list< std::pair<boost::shared_ptr<BezierCP>,boost::shared_ptr<BezierCP> > >::const_iterator it = _pointsToDrag.begin();
    std::list< std::pair<boost::shared_ptr<BezierCP>,boost::shared_ptr<BezierCP> > >::const_iterator oIt = mvCmd->_pointsToDrag.begin();
    for (; it != _pointsToDrag.end(); ++it, ++oIt) {
        if ( (it->first != oIt->first) || (it->second != oIt->second) ) {
            return false;
        }
    }

    _dx += mvCmd->_dx;
    _dy += mvCmd->_dy;

    return true;
}

////////////////////////

TransformUndoCommand::TransformUndoCommand(RotoGui* roto,
                                           double centerX,
                                           double centerY,
                                           double rot,
                                           double skewX,
                                           double skewY,
                                           double tx,
                                           double ty,
                                           double sx,
                                           double sy,
                                           double time)
    : QUndoCommand()
      , _firstRedoCalled(false)
      , _roto(roto)
      , _rippleEditEnabled( roto->getContext()->isRippleEditEnabled() )
      , _selectedTool( (int)roto->getSelectedTool() )
      , _matrix(new Transform::Matrix3x3)
      , _time(time)
      , _selectedCurves()
      , _originalPoints()
      , _selectedPoints()
{
    
    std::list< std::pair<boost::shared_ptr<BezierCP>,boost::shared_ptr<BezierCP> > > selected;
    roto->getSelection(&_selectedCurves, &selected);
    
    _selectedPoints = selected;

  
    

    *_matrix = Transform::matTransformCanonical(tx, ty, sx, sy, skewX, skewY, true, (rot), centerX, centerY);
    ///we make a copy of the points
    for (SelectedCpList::iterator it = _selectedPoints.begin(); it != _selectedPoints.end(); ++it) {
        CpPtr first( new BezierCP( *(it->first) ) );
        
        CpPtr second;
        if (it->second) {
            second.reset( new BezierCP( *(it->second) ) );
        }
        _originalPoints.push_back( std::make_pair(first, second) );
    }
}

TransformUndoCommand::~TransformUndoCommand()
{
}

void
TransformUndoCommand::undo()
{
    SelectedCpList::iterator cpIt = _originalPoints.begin();

    std::set<Bezier*> beziers;
    for (SelectedCpList::iterator it = _selectedPoints.begin(); it != _selectedPoints.end(); ++it) {
        beziers.insert(it->first->getBezier().get());
    }
    
    for (std::set<Bezier*>::iterator it = beziers.begin(); it!=beziers.end(); ++it) {
        (*it)->incrementNodesAge();
    }

    
    for (SelectedCpList::iterator it = _selectedPoints.begin(); it != _selectedPoints.end(); ++it, ++cpIt) {
        it->first->clone(*cpIt->first);
        if (it->second) {
            it->second->clone(*cpIt->second);
        }
    }

    _roto->evaluate(true);
    _roto->setCurrentTool( (RotoGui::RotoToolEnum)_selectedTool,true );
    _roto->setSelection(_selectedCurves, _selectedPoints);
    setText( QObject::tr("Transform points of %1").arg( _roto->getNodeName() ) );
}

void
TransformUndoCommand::transformPoint(const boost::shared_ptr<BezierCP> & point)
{
    point->getBezier()->transformPoint( point, _time, _matrix.get() );
}

void
TransformUndoCommand::redo()
{
    for (SelectedCpList::iterator it = _selectedPoints.begin(); it != _selectedPoints.end(); ++it) {
        transformPoint(it->first);
        if (it->second) {
            transformPoint(it->second);
        }
    }

    if (_firstRedoCalled) {
        _roto->setSelection(_selectedCurves, _selectedPoints);
        _roto->evaluate(true);
    } else {
        _roto->refreshSelectionBBox();
        _roto->onRefreshAsked();
    }
    
    _firstRedoCalled = true;
    setText( QObject::tr("Transform points of %1").arg( _roto->getNodeName() ) );
}

int
TransformUndoCommand::id() const
{
    return kRotoTransformCompressionID;
}

bool
TransformUndoCommand::mergeWith(const QUndoCommand *other)
{
    const TransformUndoCommand* cmd = dynamic_cast<const TransformUndoCommand*>(other);

    if (!cmd) {
        return false;
    }

    if ( ( cmd->_selectedPoints.size() != _selectedPoints.size() ) || (cmd->_time != _time) || (cmd->_selectedTool != _selectedTool)
         || ( cmd->_rippleEditEnabled != _rippleEditEnabled) ) {
        return false;
    }

    SelectedCpList::const_iterator it = _selectedPoints.begin();
    SelectedCpList::const_iterator oIt = cmd->_selectedPoints.begin();
    for (; it != _selectedPoints.end(); ++it, ++oIt) {
        if ( (it->first != oIt->first) || (it->second != oIt->second) ) {
            return false;
        }
    }

    *_matrix = matMul(*_matrix, *cmd->_matrix);

    return true;
}

////////////////////////


AddPointUndoCommand::AddPointUndoCommand(RotoGui* roto,
                                         const boost::shared_ptr<Bezier> & curve,
                                         int index,
                                         double t)
    : QUndoCommand()
      , _firstRedoCalled(false)
      , _roto(roto)
      , _oldCurve()
      , _curve(curve)
      , _index(index)
      , _t(t)
{
    _oldCurve.reset( new Bezier(curve->getContext(),curve->getScriptName(),curve->getParentLayer(), false) );
    _oldCurve->clone(curve.get());
    
}

AddPointUndoCommand::~AddPointUndoCommand()
{
}

void
AddPointUndoCommand::undo()
{
    _curve->clone(_oldCurve.get());
    _roto->setSelection( _curve, std::make_pair( CpPtr(),CpPtr() ) );
    _roto->evaluate(true);
    setText( QObject::tr("Add point to %1 of %2").arg( _curve->getLabel().c_str() ).arg( _roto->getNodeName() ) );
}

void
AddPointUndoCommand::redo()
{
    _oldCurve->clone(_curve.get());
    boost::shared_ptr<BezierCP> cp = _curve->addControlPointAfterIndex(_index,_t);
    boost::shared_ptr<BezierCP> newFp = _curve->getFeatherPointAtIndex(_index + 1);

    _roto->setSelection( _curve, std::make_pair(cp, newFp) );
    if (_firstRedoCalled) {
        _roto->evaluate(true);
    }

    _firstRedoCalled = true;
    setText( QObject::tr("Add point to %1 of %2").arg( _curve->getLabel().c_str() ).arg( _roto->getNodeName() ) );
}

////////////////////////

RemovePointUndoCommand::RemovePointUndoCommand(RotoGui* roto,
                                               const boost::shared_ptr<Bezier> & curve,
                                               const boost::shared_ptr<BezierCP> & cp)
    : QUndoCommand()
      , _roto(roto)
      , _firstRedoCalled(false)
      , _curves()
{
    assert(curve && cp);
    CurveDesc desc;
    assert(curve && cp);
    int indexToRemove = curve->getControlPointIndex(cp);
    desc.curveRemoved = false; //set in the redo()
    desc.parentLayer =
        boost::dynamic_pointer_cast<RotoLayer>( _roto->getContext()->getItemByName( curve->getParentLayer()->getScriptName() ) );
    assert(desc.parentLayer);
    desc.curve = curve;
    desc.points.push_back(indexToRemove);
    desc.oldCurve.reset( new Bezier(curve->getContext(),curve->getScriptName(),curve->getParentLayer(), false) );
    desc.oldCurve->clone(curve.get());
    _curves.push_back(desc);
}

RemovePointUndoCommand::RemovePointUndoCommand(RotoGui* roto,
                                               const SelectedCpList & points)
    : QUndoCommand()
      , _roto(roto)
      , _firstRedoCalled(false)
      , _curves()
{
    for (SelectedCpList::const_iterator it = points.begin(); it != points.end(); ++it) {
        boost::shared_ptr<BezierCP> cp;
        if ( it->first->isFeatherPoint() ) {
            cp = it->second;
        } else {
            cp = it->first;
        }
        assert(cp && cp->getBezier() && _roto && _roto->getContext() );
        BezierPtr curve = boost::dynamic_pointer_cast<Bezier>( _roto->getContext()->getItemByName( cp->getBezier()->getScriptName() ) );
        assert(curve);
        RotoStrokeItem* isStroke = dynamic_cast<RotoStrokeItem*>(curve.get());

        std::list< CurveDesc >::iterator foundCurve = _curves.end();
        for (std::list< CurveDesc >::iterator it2 = _curves.begin(); it2 != _curves.end(); ++it2) {
            if (it2->curve == curve) {
                foundCurve = it2;
                break;
            }
        }
        assert(curve);
        assert(cp);
        int indexToRemove = curve->getControlPointIndex(cp);
        if ( foundCurve == _curves.end() ) {
            CurveDesc curveDesc;
            curveDesc.curveRemoved = false; //set in the redo()
            curveDesc.parentLayer =
                boost::dynamic_pointer_cast<RotoLayer>( _roto->getContext()->getItemByName( cp->getBezier()->getParentLayer()->getScriptName() ) );
            assert(curveDesc.parentLayer);
            curveDesc.points.push_back(indexToRemove);
            curveDesc.curve = curve;
            if (!isStroke) {
                curveDesc.oldCurve.reset( new Bezier(curve->getContext(),curve->getScriptName(),curve->getParentLayer(), false) );
            } else {
                curveDesc.oldCurve.reset( new RotoStrokeItem(isStroke->getBrushType(),curve->getContext(),curve->getScriptName(),curve->getParentLayer()) );
            }
            curveDesc.oldCurve->clone(curve.get());
            _curves.push_back(curveDesc);
        } else {
            foundCurve->points.push_back(indexToRemove);
        }
    }
    for (std::list<CurveDesc>::iterator it = _curves.begin(); it != _curves.end(); ++it) {
        it->points.sort();
    }
}

RemovePointUndoCommand::~RemovePointUndoCommand()
{
}

void
RemovePointUndoCommand::undo()
{
    std::list<boost::shared_ptr<RotoDrawableItem> > selection;
    SelectedCpList cpSelection;

    for (std::list< CurveDesc >::iterator it = _curves.begin(); it != _curves.end(); ++it) {
        ///clone the curve
        it->curve->clone(it->oldCurve.get());
        if (it->curveRemoved) {
            _roto->getContext()->addItem(it->parentLayer, it->indexInLayer, it->curve, RotoItem::eSelectionReasonOverlayInteract);
        }
        selection.push_back(it->curve);
    }

    _roto->setSelection(selection,cpSelection);
    _roto->evaluate(true);

    setText( QObject::tr("Remove points to %1").arg( _roto->getNodeName() ) );
}

void
RemovePointUndoCommand::redo()
{
    ///clone the curve
    for (std::list< CurveDesc >::iterator it = _curves.begin(); it != _curves.end(); ++it) {
        it->oldCurve->clone(it->curve.get());
    }

    std::list<boost::shared_ptr<Bezier> > toRemove;
    for (std::list< CurveDesc >::iterator it = _curves.begin(); it != _curves.end(); ++it) {
        
        boost::shared_ptr<Bezier> isBezier = boost::dynamic_pointer_cast<Bezier>(it->curve);
        if (!isBezier) {
            continue;
        }
        ///Remove in decreasing order so indexes don't get messed up
        isBezier->setAutoOrientationComputation(false);
        for (std::list<int>::reverse_iterator it2 = it->points.rbegin(); it2 != it->points.rend(); ++it2) {
            isBezier->removeControlPointByIndex(*it2);
            int cpCount = isBezier->getControlPointsCount();
            if (cpCount == 1) {
                isBezier->setCurveFinished(false);
            } else if (cpCount == 0) {
                it->curveRemoved = true;
                std::list<boost::shared_ptr<Bezier> >::iterator found = std::find( toRemove.begin(), toRemove.end(), it->curve );
                if ( found == toRemove.end() ) {
                    toRemove.push_back(isBezier);
                }
            }
        }
        isBezier->setAutoOrientationComputation(true);
        isBezier->refreshPolygonOrientation(true);
    }

    for (std::list<boost::shared_ptr<Bezier> >::iterator it = toRemove.begin(); it != toRemove.end(); ++it) {
        _roto->removeCurve(*it);
    }


    _roto->setSelection( BezierPtr(),std::make_pair( CpPtr(), CpPtr() ) );
    _roto->evaluate(_firstRedoCalled);
    _firstRedoCalled = true;

    setText( QObject::tr("Remove points to %1").arg( _roto->getNodeName() ) );
}

//////////////////////////

RemoveCurveUndoCommand::RemoveCurveUndoCommand(RotoGui* roto,
                                               const std::list<boost::shared_ptr<RotoDrawableItem> > & curves)
    : QUndoCommand()
      , _roto(roto)
      , _firstRedoCalled(false)
      , _curves()
{
    for (std::list<boost::shared_ptr<RotoDrawableItem> >::const_iterator it = curves.begin(); it != curves.end(); ++it) {
        RemovedCurve r;
        r.curve = *it;
        r.layer = boost::dynamic_pointer_cast<RotoLayer>( _roto->getContext()->getItemByName( (*it)->getParentLayer()->getScriptName() ) );
        assert(r.layer);
        r.indexInLayer = r.layer->getChildIndex(*it);
        assert(r.indexInLayer != -1);
        _curves.push_back(r);
    }
}

RemoveCurveUndoCommand::~RemoveCurveUndoCommand()
{
}

void
RemoveCurveUndoCommand::undo()
{
    std::list<boost::shared_ptr<RotoDrawableItem> > selection;

    for (std::list<RemovedCurve>::iterator it = _curves.begin(); it != _curves.end(); ++it) {
        _roto->getContext()->addItem(it->layer, it->indexInLayer, it->curve, RotoItem::eSelectionReasonOverlayInteract);
        boost::shared_ptr<Bezier> isBezier = boost::dynamic_pointer_cast<Bezier>(it->curve);
        if (isBezier) {
            selection.push_back(isBezier);
        }
    }

    SelectedCpList cpList;
    if (!selection.empty()) {
        _roto->setSelection(selection, cpList);
    }
    _roto->evaluate(true);

    setText( QObject::tr("Remove curves to %1").arg( _roto->getNodeName() ) );
}

void
RemoveCurveUndoCommand::redo()
{
    for (std::list<RemovedCurve>::iterator it = _curves.begin(); it != _curves.end(); ++it) {
        _roto->removeCurve(it->curve);
    }
    _roto->evaluate(_firstRedoCalled);
    _roto->setSelection( BezierPtr(), std::make_pair( CpPtr(), CpPtr() ) );
    _firstRedoCalled = true;
    setText( QObject::tr("Remove curves to %1").arg( _roto->getNodeName() ) );
}

////////////////////////////////


AddStrokeUndoCommand::AddStrokeUndoCommand(RotoGui* roto,const boost::shared_ptr<RotoStrokeItem>& item)
: QUndoCommand()
, _roto(roto)
, _firstRedoCalled(false)
, _item(item)
, _layer(item->getParentLayer())
, _indexInLayer(_layer ? _layer->getChildIndex(_item) : -1)
{
    assert(_indexInLayer != -1);
    setText(QObject::tr("Paint Stroke"));
}

AddStrokeUndoCommand::~AddStrokeUndoCommand()
{

}

void
AddStrokeUndoCommand::undo()
{
    _roto->removeCurve(_item);
    _roto->evaluate(true);
}

void
AddStrokeUndoCommand::redo()
{
    if (_firstRedoCalled) {
        _roto->getContext()->addItem(_layer, _indexInLayer, _item, RotoItem::eSelectionReasonOverlayInteract);
    }
    if (_firstRedoCalled) {
        _roto->evaluate(true);
    }
    _firstRedoCalled = true;
}



AddMultiStrokeUndoCommand::AddMultiStrokeUndoCommand(RotoGui* roto,const boost::shared_ptr<RotoStrokeItem>& item)
: QUndoCommand()
, _roto(roto)
, _firstRedoCalled(false)
, _item(item)
, _layer(item->getParentLayer())
, _indexInLayer(_layer ? _layer->getChildIndex(_item) : -1)
, isRemoved(false)
{
    assert(_indexInLayer != -1);
    setText(QObject::tr("Paint Stroke"));
}

AddMultiStrokeUndoCommand::~AddMultiStrokeUndoCommand()
{

}

void
AddMultiStrokeUndoCommand::undo()
{
    if (_item->removeLastStroke(&_xCurve, &_yCurve, &_pCurve)) {
        _roto->removeCurve(_item);
        isRemoved = true;
    }
    
    _roto->evaluate(true);
}

void
AddMultiStrokeUndoCommand::redo()
{
    if (_firstRedoCalled) {
        if (_xCurve) {
            _item->addStroke(_xCurve, _yCurve, _pCurve);
        }
        if (isRemoved) {
            _roto->getContext()->addItem(_layer, _indexInLayer, _item, RotoItem::eSelectionReasonOverlayInteract);
        }
        _roto->evaluate(true);
    }
    
    _firstRedoCalled = true;
}

MoveTangentUndoCommand::MoveTangentUndoCommand(RotoGui* roto,
                                               double dx,
                                               double dy,
                                               double time,
                                               const boost::shared_ptr<BezierCP> & cp,
                                               bool left,
                                               bool breakTangents)
    : QUndoCommand()
      , _firstRedoCalled(false)
      , _roto(roto)
      , _dx(dx)
      , _dy(dy)
      , _featherLinkEnabled( roto->getContext()->isFeatherLinkEnabled() )
      , _rippleEditEnabled( roto->getContext()->isRippleEditEnabled() )
      , _time(time)
      , _tangentBeingDragged(cp)
      , _oldCp()
      , _oldFp()
      , _left(left)
      , _breakTangents(breakTangents)
{
    roto->getSelection(&_selectedCurves, &_selectedPoints);
    boost::shared_ptr<BezierCP> counterPart;
    if ( cp->isFeatherPoint() ) {
        counterPart = _tangentBeingDragged->getBezier()->getControlPointForFeatherPoint(_tangentBeingDragged);
        _oldCp.reset( new BezierCP(*counterPart) );
        _oldFp.reset( new BezierCP(*_tangentBeingDragged) );
    } else {
        counterPart = _tangentBeingDragged->getBezier()->getFeatherPointForControlPoint(_tangentBeingDragged);
        _oldCp.reset( new BezierCP(*_tangentBeingDragged) );
        if (counterPart) {
            _oldFp.reset( new BezierCP(*counterPart) );
        }
    }
}

MoveTangentUndoCommand::~MoveTangentUndoCommand()
{
}

namespace {
static void
dragTangent(double time,
            BezierCP & p,
            const Transform::Matrix3x3& transform,
            double dx,
            double dy,
            bool left,
            bool autoKeying,
            bool breakTangents)
{
    Transform::Point3D ltan,rtan,pos;
    ltan.z = rtan.z = pos.z = 1;
    bool isOnKeyframe = p.getLeftBezierPointAtTime(true,time, &ltan.x, &ltan.y,true);
    p.getRightBezierPointAtTime(true,time, &rtan.x, &rtan.y,true);
    p.getPositionAtTime(true,time, &pos.x, &pos.y,true);
    
    pos = Transform::matApply(transform, pos);
    ltan = Transform::matApply(transform, ltan);
    rtan = Transform::matApply(transform, rtan);
    
    double dist = left ?  sqrt( (rtan.x - pos.x) * (rtan.x - pos.x) + (rtan.y - pos.y) * (rtan.y - pos.y) )
                  : sqrt( (ltan.x - pos.x) * (ltan.x - pos.x) + (ltan.y - pos.y) * (ltan.y - pos.y) );
    if (left) {
        ltan.x += dx;
        ltan.y += dy;
    } else {
        rtan.x += dx;
        rtan.y += dy;
    }
    double alpha = left ? std::atan2(pos.y - ltan.y,pos.x - ltan.x) : std::atan2(pos.y - rtan.y,pos.x - rtan.x);

    if (left) {
        double rightDiffX = breakTangents ? 0 : pos.x + std::cos(alpha) * dist - rtan.x;
        double rightDiffY = breakTangents ? 0 : pos.y + std::sin(alpha) * dist - rtan.y;
        if (autoKeying || isOnKeyframe) {
            p.getBezier()->movePointLeftAndRightIndex(p, time, dx, dy, rightDiffX, rightDiffY);
        }
    } else {
        double leftDiffX = breakTangents ? 0 : pos.x + std::cos(alpha) * dist - ltan.x;
        double leftDiffY = breakTangents ? 0 : pos.y + std::sin(alpha) * dist - ltan.y;
        if (autoKeying || isOnKeyframe) {
            p.getBezier()->movePointLeftAndRightIndex(p, time, leftDiffX, leftDiffY, dx, dy);
        }
    }
}
}

void
MoveTangentUndoCommand::undo()
{
    boost::shared_ptr<BezierCP> counterPart;

    if ( _tangentBeingDragged->isFeatherPoint() ) {
        counterPart = _tangentBeingDragged->getBezier()->getControlPointForFeatherPoint(_tangentBeingDragged);
        counterPart->clone(*_oldCp);
        _tangentBeingDragged->clone(*_oldFp);
    } else {
        counterPart = _tangentBeingDragged->getBezier()->getFeatherPointForControlPoint(_tangentBeingDragged);
        if (counterPart) {
            counterPart->clone(*_oldFp);
        }
        _tangentBeingDragged->clone(*_oldCp);
    }
    _tangentBeingDragged->getBezier()->incrementNodesAge();
    if (_firstRedoCalled) {
        _roto->setSelection(_selectedCurves, _selectedPoints);
    }

    _roto->evaluate(true);

    setText( QObject::tr("Move tangent of %1 of %2").arg( _tangentBeingDragged->getBezier()->getLabel().c_str() ).arg( _roto->getNodeName() ) );
}

void
MoveTangentUndoCommand::redo()
{
    boost::shared_ptr<BezierCP> counterPart;

    if ( _tangentBeingDragged->isFeatherPoint() ) {
        counterPart = _tangentBeingDragged->getBezier()->getControlPointForFeatherPoint(_tangentBeingDragged);
        _oldCp->clone(*counterPart);
        _oldFp->clone(*_tangentBeingDragged);
    } else {
        counterPart = _tangentBeingDragged->getBezier()->getFeatherPointForControlPoint(_tangentBeingDragged);
        if (counterPart) {
            _oldFp->clone(*counterPart);
        }
        _oldCp->clone(*_tangentBeingDragged);
    }
    
    _tangentBeingDragged->getBezier()->incrementNodesAge();

    Transform::Matrix3x3 transform;
    _tangentBeingDragged->getBezier()->getTransformAtTime(_time, &transform);
    
    bool autoKeying = _roto->getContext()->isAutoKeyingEnabled();
    dragTangent(_time, *_tangentBeingDragged, transform, _dx, _dy, _left,autoKeying,_breakTangents);
    if (_featherLinkEnabled && counterPart) {
        dragTangent(_time, *counterPart, transform, _dx, _dy, _left,autoKeying,_breakTangents);
    }

    if (_firstRedoCalled) {
        _roto->setSelection(_selectedCurves, _selectedPoints);
        _roto->evaluate(true);
    } else {
        _roto->refreshSelectionBBox();
    }


    _firstRedoCalled = true;

    setText( QObject::tr("Move tangent of %1 of %2").arg( _tangentBeingDragged->getBezier()->getLabel().c_str() ).arg( _roto->getNodeName() ) );
}

int
MoveTangentUndoCommand::id() const
{
    return kRotoMoveTangentCompressionID;
}

bool
MoveTangentUndoCommand::mergeWith(const QUndoCommand *other)
{
    const MoveTangentUndoCommand* mvCmd = dynamic_cast<const MoveTangentUndoCommand*>(other);

    if (!mvCmd) {
        return false;
    }
    if ( (mvCmd->_tangentBeingDragged != _tangentBeingDragged) || (mvCmd->_left != _left) || (mvCmd->_featherLinkEnabled != _featherLinkEnabled)
         || ( mvCmd->_rippleEditEnabled != _rippleEditEnabled) ) {
        return false;
    }
    _dx += mvCmd->_dx;
    _dy += mvCmd->_dy;
    return true;
}

//////////////////////////


MoveFeatherBarUndoCommand::MoveFeatherBarUndoCommand(RotoGui* roto,
                                                     double dx,
                                                     double dy,
                                                     const std::pair<boost::shared_ptr<BezierCP>,boost::shared_ptr<BezierCP> > & point,
                                                     double time)
    : QUndoCommand()
      , _roto(roto)
      , _firstRedoCalled(false)
      , _dx(dx)
      , _dy(dy)
      , _rippleEditEnabled( roto->getContext()->isRippleEditEnabled() )
      , _time(time)
      , _curve()
      , _oldPoint()
      , _newPoint(point)
{
    _curve = boost::dynamic_pointer_cast<Bezier>( _roto->getContext()->getItemByName( point.first->getBezier()->getScriptName() ) );
    assert(_curve);
    _oldPoint.first.reset( new BezierCP(*_newPoint.first) );
    _oldPoint.second.reset( new BezierCP(*_newPoint.second) );
}

MoveFeatherBarUndoCommand::~MoveFeatherBarUndoCommand()
{
}

void
MoveFeatherBarUndoCommand::undo()
{
    _newPoint.first->clone(*_oldPoint.first);
    _newPoint.second->clone(*_oldPoint.second);
    _newPoint.first->getBezier()->incrementNodesAge();
    _roto->evaluate(true);
    _roto->setSelection(_curve, _newPoint);
    setText( QObject::tr("Move feather bar of %1 of %2").arg( _curve->getLabel().c_str() ).arg( _roto->getNodeName() ) );
}

void
MoveFeatherBarUndoCommand::redo()
{
    
    boost::shared_ptr<BezierCP> p = _newPoint.first->isFeatherPoint() ?
    _newPoint.second : _newPoint.first;
    boost::shared_ptr<BezierCP> fp = _newPoint.first->isFeatherPoint() ?
    _newPoint.first : _newPoint.second;
    Point delta;
    
    Transform::Matrix3x3 transform;
    p->getBezier()->getTransformAtTime(_time, &transform);
    
    Transform::Point3D featherPoint,controlPoint;
    featherPoint.z = controlPoint.z = 1.;
    p->getPositionAtTime(true,_time, &controlPoint.x, &controlPoint.y);
    bool isOnKeyframe = fp->getPositionAtTime(true,_time, &featherPoint.x, &featherPoint.y);
    
    controlPoint = Transform::matApply(transform, controlPoint);
    featherPoint = Transform::matApply(transform, featherPoint);
    
    if ( (controlPoint.x != featherPoint.x) || (controlPoint.y != featherPoint.y) ) {
        Point featherVec;
        featherVec.x = featherPoint.x - controlPoint.x;
        featherVec.y = featherPoint.y - controlPoint.y;
        double norm = sqrt( (featherPoint.x - controlPoint.x) * (featherPoint.x - controlPoint.x)
                           + (featherPoint.y - controlPoint.y) * (featherPoint.y - controlPoint.y) );
        assert(norm != 0);
        delta.x = featherVec.x / norm;
        delta.y = featherVec.y / norm;
        
        double dotProduct = delta.x * _dx + delta.y * _dy;
        delta.x = delta.x * dotProduct;
        delta.y = delta.y * dotProduct;
    } else {
        ///the feather point equals the control point, use derivatives
        const std::list<boost::shared_ptr<BezierCP> > & cps = p->getBezier()->getFeatherPoints();
        assert(cps.size() > 1);
        
        std::list<boost::shared_ptr<BezierCP> >::const_iterator cur = std::find(cps.begin(), cps.end(), fp);
        assert( cur != cps.end() );
        // compute previous and next element in the cyclic list
        std::list<boost::shared_ptr<BezierCP> >::const_iterator prev = cur;
        if (prev == cps.begin()) {
            prev = cps.end();
        }
        --prev; // the list has at least one element
        std::list<boost::shared_ptr<BezierCP> >::const_iterator next = cur;
        ++next; // the list has at least one element
        if (next == cps.end()) {
            next = cps.begin();
        }
        
        double leftX,leftY,rightX,rightY,norm;
        Bezier::leftDerivativeAtPoint(true,_time, **cur, **prev, transform ,&leftX, &leftY);
        Bezier::rightDerivativeAtPoint(true,_time, **cur, **next, transform ,&rightX, &rightY);
        norm = sqrt( (rightX - leftX) * (rightX - leftX) + (rightY - leftY) * (rightY - leftY) );
        
        ///normalize derivatives by their norm
        if (norm != 0) {
            delta.x = -( (rightY - leftY) / norm );
            delta.y = ( (rightX - leftX) / norm );
        } else {
            ///both derivatives are the same, use the direction of the left one
            norm = sqrt( (leftX - featherPoint.x) * (leftX - featherPoint.x) + (leftY - featherPoint.y) * (leftY - featherPoint.y) );
            if (norm != 0) {
                delta.x = -( (leftY - featherPoint.y) / norm );
                delta.y = ( (leftX - featherPoint.x) / norm );
            } else {
                ///both derivatives and control point are equal, just use 0
                delta.x = delta.y = 0;
            }
        }
        
        double dotProduct = delta.x * _dx + delta.y * _dy;
        delta.x = delta.x * dotProduct;
        delta.y = delta.y * dotProduct;
    }
    
    if (_roto->getContext()->isAutoKeyingEnabled() || isOnKeyframe) {
        int index = fp->getBezier()->getFeatherPointIndex(fp);
        fp->getBezier()->moveFeatherByIndex(index, _time, delta.x, delta.y);
    }
    
    if (_firstRedoCalled) {
        _roto->evaluate(true);
    }
    
    _roto->setSelection(_curve, _newPoint);

    _firstRedoCalled = true;
    setText( QObject::tr("Move feather bar of %1 of %2").arg( _curve->getLabel().c_str() ).arg( _roto->getNodeName() ) );
} // redo

int
MoveFeatherBarUndoCommand::id() const
{
    return kRotoMoveFeatherBarCompressionID;
}

bool
MoveFeatherBarUndoCommand::mergeWith(const QUndoCommand *other)
{
    const MoveFeatherBarUndoCommand* mvCmd = dynamic_cast<const MoveFeatherBarUndoCommand*>(other);

    if (!mvCmd) {
        return false;
    }
    if ( (mvCmd->_newPoint.first != _newPoint.first) || (mvCmd->_newPoint.second != _newPoint.second) ||
         ( mvCmd->_rippleEditEnabled != _rippleEditEnabled) || ( mvCmd->_time != _time) ) {
        return false;
    }
    
    _dx += mvCmd->_dx;
    _dy += mvCmd->_dy;

    return true;
}

/////////////////////////

RemoveFeatherUndoCommand::RemoveFeatherUndoCommand(RotoGui* roto,
                                                   const std::list<RemoveFeatherData> & datas)
    : QUndoCommand()
      , _roto(roto)
      , _firstRedocalled(false)
      , _datas(datas)
{
    for (std::list<RemoveFeatherData>::iterator it = _datas.begin(); it != _datas.end(); ++it) {
        for (std::list<boost::shared_ptr<BezierCP> >::const_iterator it2 = it->newPoints.begin(); it2 != it->newPoints.end(); ++it2) {
            it->oldPoints.push_back( boost::shared_ptr<BezierCP>( new BezierCP(**it2) ) );
        }
    }
}

RemoveFeatherUndoCommand::~RemoveFeatherUndoCommand()
{
}

void
RemoveFeatherUndoCommand::undo()
{
    for (std::list<RemoveFeatherData>::iterator it = _datas.begin(); it != _datas.end(); ++it) {
        std::list<boost::shared_ptr<BezierCP> >::const_iterator itOld = it->oldPoints.begin();
        for (std::list<boost::shared_ptr<BezierCP> >::const_iterator itNew = it->newPoints.begin();
             itNew != it->newPoints.end(); ++itNew, ++itOld) {
            (*itNew)->clone(**itOld);
        }
        it->curve->incrementNodesAge();
    }
    _roto->evaluate(true);

    setText( QObject::tr("Remove feather of %1").arg( _roto->getNodeName() ) );
}

void
RemoveFeatherUndoCommand::redo()
{
    for (std::list<RemoveFeatherData>::iterator it = _datas.begin(); it != _datas.end(); ++it) {
        std::list<boost::shared_ptr<BezierCP> >::const_iterator itOld = it->oldPoints.begin();
        for (std::list<boost::shared_ptr<BezierCP> >::const_iterator itNew = it->newPoints.begin();
             itNew != it->newPoints.end(); ++itNew, ++itOld) {
            (*itOld)->clone(**itNew);
            try {
                it->curve->removeFeatherAtIndex( it->curve->getFeatherPointIndex(*itNew) );
            } catch (...) {
                ///the point doesn't exist anymore, just do nothing
                return;
            }
        }
    }


    _roto->evaluate(_firstRedocalled);


    _firstRedocalled = true;

    setText( QObject::tr("Remove feather of %1").arg( _roto->getNodeName() ) );
}

////////////////////////////


OpenCloseUndoCommand::OpenCloseUndoCommand(RotoGui* roto,
                                           const boost::shared_ptr<Bezier> & curve)
    : QUndoCommand()
      , _roto(roto)
      , _firstRedoCalled(false)
      , _selectedTool( (int)roto->getSelectedTool() )
      , _curve(curve)
{
}

OpenCloseUndoCommand::~OpenCloseUndoCommand()
{
}

void
OpenCloseUndoCommand::undo()
{
    if (_firstRedoCalled) {
        _roto->setCurrentTool( (RotoGui::RotoToolEnum)_selectedTool,true );
        if ( (RotoGui::RotoToolEnum)_selectedTool == RotoGui::eRotoToolDrawBezier ) {
            _roto->setBuiltBezier(_curve);
        }
    }
    _curve->setCurveFinished( !_curve->isCurveFinished() );
    _roto->evaluate(true);
    _roto->setSelection( _curve, std::make_pair( CpPtr(), CpPtr() ) );
    setText( QObject::tr("Open/Close %1 of %2").arg( _curve->getLabel().c_str() ).arg( _roto->getNodeName() ) );
}

void
OpenCloseUndoCommand::redo()
{
    if (_firstRedoCalled) {
        _roto->setCurrentTool( (RotoGui::RotoToolEnum)_selectedTool,true );
    }
    _curve->setCurveFinished( !_curve->isCurveFinished() );
    _roto->evaluate(_firstRedoCalled);
    _roto->setSelection( _curve, std::make_pair( CpPtr(), CpPtr() ) );
    _firstRedoCalled = true;
    setText( QObject::tr("Open/Close %1 of %2").arg( _curve->getLabel().c_str() ).arg( _roto->getNodeName() ) );
}

////////////////////////////

SmoothCuspUndoCommand::SmoothCuspUndoCommand(RotoGui* roto,
                                             const std::list<SmoothCuspCurveData> & data,
                                             double time,
                                             bool cusp,
                                             const std::pair<double, double>& pixelScale)
    : QUndoCommand()
      , _roto(roto)
      , _firstRedoCalled(false)
      , _time(time)
      , _count(1)
      , _cusp(cusp)
      , curves(data)
      , _pixelScale(pixelScale)
{
    for (std::list<SmoothCuspCurveData>::iterator it = curves.begin(); it != curves.end(); ++it) {
        for (SelectedPointList::const_iterator it2 = it->newPoints.begin(); it2 != it->newPoints.end(); ++it2) {
            boost::shared_ptr<BezierCP> firstCpy( new BezierCP(*(*it2).first) );
            boost::shared_ptr<BezierCP> secondCpy( new BezierCP(*(*it2).second) );
            it->oldPoints.push_back( std::make_pair(firstCpy, secondCpy) );
        }
    }
}

SmoothCuspUndoCommand::~SmoothCuspUndoCommand()
{
}

void
SmoothCuspUndoCommand::undo()
{
    for (std::list<SmoothCuspCurveData>::iterator it = curves.begin(); it != curves.end(); ++it) {
        SelectedPointList::const_iterator itOld = it->oldPoints.begin();
        for (SelectedPointList::const_iterator itNew = it->newPoints.begin();
             itNew != it->newPoints.end(); ++itNew, ++itOld) {
            itNew->first->clone(*itOld->first);
            itNew->second->clone(*itOld->second);
            it->curve->incrementNodesAge();
        }
    }

    _roto->evaluate(true);
    if (_cusp) {
        setText( QObject::tr("Cusp points of %1").arg( _roto->getNodeName() ) );
    } else {
        setText( QObject::tr("Smooth points of %1").arg( _roto->getNodeName() ) );
    }
}

void
SmoothCuspUndoCommand::redo()
{
    for (std::list<SmoothCuspCurveData>::iterator it = curves.begin(); it != curves.end(); ++it) {
        SelectedPointList::const_iterator itOld = it->oldPoints.begin();
        for (SelectedPointList::const_iterator itNew = it->newPoints.begin();
             itNew != it->newPoints.end(); ++itNew, ++itOld) {
            itOld->first->clone(*itNew->first);
            itOld->second->clone(*itNew->second);

            for (int i = 0; i < _count; ++i) {
                int index = it->curve->getControlPointIndex( (*itNew).first->isFeatherPoint() ? (*itNew).second : (*itNew).first );
                assert(index != -1);

                if (_cusp) {
                    it->curve->cuspPointAtIndex(index, _time, _pixelScale);
                } else {
                    it->curve->smoothPointAtIndex(index, _time, _pixelScale);
                }
            }
        }
    }

    _roto->evaluate(_firstRedoCalled);

    _firstRedoCalled = true;
    if (_cusp) {
        setText( QObject::tr("Cusp points of %1").arg( _roto->getNodeName() ) );
    } else {
        setText( QObject::tr("Smooth points of %1").arg( _roto->getNodeName() ) );
    }
}

int
SmoothCuspUndoCommand::id() const
{
    return kRotoCuspSmoothCompressionID;
}

bool
SmoothCuspUndoCommand::mergeWith(const QUndoCommand *other)
{
    const SmoothCuspUndoCommand* sCmd = dynamic_cast<const SmoothCuspUndoCommand*>(other);

    if (!sCmd) {
        return false;
    }
    if ( ( sCmd->curves.size() != curves.size() ) ||
         ( sCmd->_cusp != _cusp) || ( sCmd->_time != _time) ) {
        return false;
    }
    std::list<SmoothCuspCurveData>::const_iterator itOther = sCmd->curves.begin();
    for (std::list<SmoothCuspCurveData>::const_iterator it = curves.begin(); it != curves.end(); ++it, ++itOther) {
        if (it->curve != itOther->curve) {
            return false;
        }
        SelectedPointList::const_iterator itNewOther = itOther->newPoints.begin();
        for (SelectedPointList::const_iterator itNew = it->newPoints.begin();
             itNew != it->newPoints.end(); ++itNew, ++itNewOther) {
            if ( (itNewOther->first != itNew->first) || (itNewOther->second != itNew->second) ) {
                return false;
            }
        }
    }


    ++_count;

    return true;
}

/////////////////////////

MakeBezierUndoCommand::MakeBezierUndoCommand(RotoGui* roto,
                                             const boost::shared_ptr<Bezier> & curve,
                                             bool isOpenBezier,
                                             bool createPoint,
                                             double dx,
                                             double dy,
                                             double time)
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
      , _isOpenBezier(isOpenBezier)
{
    if (!_newCurve) {
        _curveNonExistant = true;
    } else {
        _oldCurve.reset( new Bezier(_newCurve->getContext(),_newCurve->getScriptName(),_newCurve->getParentLayer(), false) );
        _oldCurve->clone(_newCurve.get());
    }
}

MakeBezierUndoCommand::~MakeBezierUndoCommand()
{
}

void
MakeBezierUndoCommand::undo()
{
    assert(_createdPoint);
    _roto->setCurrentTool(RotoGui::eRotoToolDrawBezier,true);
    assert(_lastPointAdded != -1);
    _oldCurve->clone(_newCurve.get());
    if (_newCurve->getControlPointsCount() == 1) {
        _curveNonExistant = true;
        _roto->removeCurve(_newCurve);
    }
    _newCurve->removeControlPointByIndex(_lastPointAdded);

    if (!_curveNonExistant) {
        _roto->setSelection( _newCurve, std::make_pair( CpPtr(), CpPtr() ) );
        _roto->setBuiltBezier(_newCurve);
    } else {
        _roto->setSelection( BezierPtr(), std::make_pair( CpPtr(), CpPtr() ) );
    }
    _roto->evaluate(true);
    setText( QObject::tr("Build bezier %1 of %2").arg( _newCurve->getLabel().c_str() ).arg( _roto->getNodeName() ) );
}

void
MakeBezierUndoCommand::redo()
{
    if (_firstRedoCalled) {
        _roto->setCurrentTool(RotoGui::eRotoToolDrawBezier,true);
    }
    

    if (!_firstRedoCalled) {
        if (_createdPoint) {
            if (!_newCurve) {
                _newCurve = _roto->getContext()->makeBezier(_x, _y, _isOpenBezier ? kRotoOpenBezierBaseName : kRotoBezierBaseName,_time, _isOpenBezier);
                assert(_newCurve);
                _oldCurve.reset( new Bezier(_newCurve->getContext(), _newCurve->getScriptName(), _newCurve->getParentLayer(), false) );
                _oldCurve->clone(_newCurve.get());
                _lastPointAdded = 0;
                _curveNonExistant = false;
            } else {
                _oldCurve->clone(_newCurve.get());
                _newCurve->addControlPoint(_x, _y,_time);
                int lastIndex = _newCurve->getControlPointsCount() - 1;
                assert(lastIndex > 0);
                _lastPointAdded = lastIndex;
            }
        } else {
            assert(_newCurve);
            _oldCurve->clone(_newCurve.get());
            int lastIndex = _newCurve->getControlPointsCount() - 1;
            assert(lastIndex >= 0);
            _lastPointAdded = lastIndex;
            _newCurve->moveLeftBezierPoint(lastIndex,_time, -_dx, -_dy);
            _newCurve->moveRightBezierPoint(lastIndex, _time, _dx, _dy);
        }
        
        boost::shared_ptr<RotoItem> parentItem;
        if (_newCurve->getParentLayer()) {
            parentItem =  _roto->getContext()->getItemByName(_newCurve->getParentLayer()->getScriptName());
        }
        if (parentItem) {
            _parentLayer = boost::dynamic_pointer_cast<RotoLayer>(parentItem);
            _indexInLayer = _parentLayer->getChildIndex(_newCurve);
        }
    } else {
        _newCurve->clone(_oldCurve.get());
        if (_curveNonExistant) {
            _roto->getContext()->addItem(_parentLayer, _indexInLayer, _newCurve, RotoItem::eSelectionReasonOverlayInteract);
        }
    }


    boost::shared_ptr<BezierCP> cp = _newCurve->getControlPointAtIndex(_lastPointAdded);
    boost::shared_ptr<BezierCP> fp = _newCurve->getFeatherPointAtIndex(_lastPointAdded);
    _roto->setSelection( _newCurve, std::make_pair(cp, fp) );
    _roto->autoSaveAndRedraw();
    _firstRedoCalled = true;


    setText( QObject::tr("Build bezier %1 of %2").arg( _newCurve->getLabel().c_str() ).arg( _roto->getNodeName() ) );
} // redo

int
MakeBezierUndoCommand::id() const
{
    return kRotoMakeBezierCompressionID;
}

bool
MakeBezierUndoCommand::mergeWith(const QUndoCommand *other)
{
    const MakeBezierUndoCommand* sCmd = dynamic_cast<const MakeBezierUndoCommand*>(other);

    if (!sCmd) {
        return false;
    }

    if ( sCmd->_createdPoint || (sCmd->_newCurve != _newCurve) ) {
        return false;
    }

    if (!sCmd->_createdPoint) {
        _dx += sCmd->_dx;
        _dy += sCmd->_dy;
    }

    return true;
}

//////////////////////////////


MakeEllipseUndoCommand::MakeEllipseUndoCommand(RotoGui* roto,
                                               bool create,
                                               bool fromCenter,
                                               bool constrained,
                                               double fromx,
                                               double fromy,
                                               double tox,
                                               double toy,
                                               double time)
    : QUndoCommand()
      , _firstRedoCalled(false)
      , _roto(roto)
      , _indexInLayer(-1)
      , _curve()
      , _create(create)
      , _fromCenter(fromCenter)
      , _constrained(constrained)
      , _fromx(fromx)
      , _fromy(fromy)
      , _tox(tox)
      , _toy(toy)
      , _time(time)
{
    if (!_create) {
        _curve = _roto->getBezierBeingBuild();
    }
}

MakeEllipseUndoCommand::~MakeEllipseUndoCommand()
{
}

void
MakeEllipseUndoCommand::undo()
{
    _roto->removeCurve(_curve);
    _roto->evaluate(true);
    _roto->setSelection( BezierPtr(), std::make_pair( CpPtr(), CpPtr() ) );
    setText( QObject::tr("Build Ellipse %1 of %2").arg( _curve->getLabel().c_str() ).arg( _roto->getNodeName() ) );
}

void
MakeEllipseUndoCommand::redo()
{
    if (_firstRedoCalled) {
        _roto->getContext()->addItem(_parentLayer, _indexInLayer, _curve, RotoItem::eSelectionReasonOverlayInteract);
        _roto->evaluate(true);
    } else {
        double ytop, xright, ybottom, xleft;
        xright = _tox;
        if (_constrained) {
            xright =  _fromx + (_tox > _fromx ? 1 : -1) * std::abs(_toy - _fromy);
        }
        if (_fromCenter) {
            ytop = _fromy - (_toy - _fromy);
            xleft = _fromx - (xright - _fromx);
            if (xleft == xright) {
                xleft -= 1.;
            }
        } else {
            ytop = _fromy;
            xleft = _fromx;
            if (xleft == xright) {
                xleft -= 1.;
            }
        }
        ybottom = _toy;
        if (ybottom == ytop) {
            ybottom -= 1.;
        }
        double xmid = (xleft + xright) / 2.;
        double ymid = (ytop + ybottom) / 2.;
        if (_create) {
            _curve = _roto->getContext()->makeBezier(xmid, ytop, kRotoEllipseBaseName, _time, false); //top
            assert(_curve);
            _curve->addControlPoint(xright, ymid, _time); // right
            _curve->addControlPoint(xmid, ybottom, _time); // bottom
            _curve->addControlPoint(xleft, ymid, _time); //left
            _curve->setCurveFinished(true);
        } else {
            _curve->setPointByIndex(0, _time, xmid, ytop); // top
            _curve->setPointByIndex(1,_time, xright, ymid); // right
            _curve->setPointByIndex(2,_time, xmid, ybottom); // bottom
            _curve->setPointByIndex(3,_time, xleft, ymid); // left

            boost::shared_ptr<BezierCP> top = _curve->getControlPointAtIndex(0);
            boost::shared_ptr<BezierCP> right = _curve->getControlPointAtIndex(1);
            boost::shared_ptr<BezierCP> bottom = _curve->getControlPointAtIndex(2);
            boost::shared_ptr<BezierCP> left = _curve->getControlPointAtIndex(3);
            double topX,topY,rightX,rightY,btmX,btmY,leftX,leftY;
            top->getPositionAtTime(true,_time, &topX, &topY);
            right->getPositionAtTime(true,_time, &rightX, &rightY);
            bottom->getPositionAtTime(true,_time, &btmX, &btmY);
            left->getPositionAtTime(true,_time, &leftX, &leftY);
            
            _curve->setLeftBezierPoint(0, _time,  (leftX + topX) / 2., topY);
            _curve->setRightBezierPoint(0, _time, (rightX + topX) / 2., topY);
            
            _curve->setLeftBezierPoint(1, _time,  rightX, (rightY + topY) / 2.);
            _curve->setRightBezierPoint(1, _time, rightX, (rightY + btmY) / 2.);
            
            _curve->setLeftBezierPoint(2, _time,  (rightX + btmX) / 2., btmY);
            _curve->setRightBezierPoint(2, _time, (leftX + btmX) / 2., btmY);
            
            _curve->setLeftBezierPoint(3, _time,   leftX, (btmY + leftY) / 2.);
            _curve->setRightBezierPoint(3, _time, leftX, (topY + leftY) / 2.);
        }
        boost::shared_ptr<RotoItem> parentItem =  _roto->getContext()->getItemByName( _curve->getParentLayer()->getScriptName() );
        if (parentItem) {
            _parentLayer = boost::dynamic_pointer_cast<RotoLayer>(parentItem);
            _indexInLayer = _parentLayer->getChildIndex(_curve);
        }
    }
    _roto->setBuiltBezier(_curve);
    _firstRedoCalled = true;
    _roto->setSelection( _curve, std::make_pair( CpPtr(), CpPtr() ) );
    setText( QObject::tr("Build Ellipse %1 of %2").arg( _curve->getLabel().c_str() ).arg( _roto->getNodeName() ) );
} // redo

int
MakeEllipseUndoCommand::id() const
{
    return kRotoMakeEllipseCompressionID;
}

bool
MakeEllipseUndoCommand::mergeWith(const QUndoCommand *other)
{
    const MakeEllipseUndoCommand* sCmd = dynamic_cast<const MakeEllipseUndoCommand*>(other);

    if (!sCmd) {
        return false;
    }
    if ( (sCmd->_curve != _curve) || sCmd->_create ) {
        return false;
    }
    if (_curve != sCmd->_curve) {
        _curve->clone(sCmd->_curve.get());
    }
    _fromx = sCmd->_fromx;
    _fromy = sCmd->_fromy;
    _tox = sCmd->_tox;
    _toy = sCmd->_toy;
    _fromCenter = sCmd->_fromCenter;
    _constrained = sCmd->_constrained;

    return true;
}

////////////////////////////////////


MakeRectangleUndoCommand::MakeRectangleUndoCommand(RotoGui* roto,
                                                   bool create,
                                                   bool fromCenter,
                                                   bool constrained,
                                                   double fromx,
                                                   double fromy,
                                                   double tox,
                                                   double toy,
                                                   double time)
    : QUndoCommand()
      , _firstRedoCalled(false)
      , _roto(roto)
      , _parentLayer()
      , _indexInLayer(-1)
      , _curve()
      , _create(create)
      , _fromCenter(fromCenter)
      , _constrained(constrained)
      , _fromx(fromx)
      , _fromy(fromy)
      , _tox(tox)
      , _toy(toy)
      , _time(time)
{
    if (!_create) {
        _curve = _roto->getBezierBeingBuild();
    }
}

MakeRectangleUndoCommand::~MakeRectangleUndoCommand()
{
}

void
MakeRectangleUndoCommand::undo()
{
    _roto->removeCurve(_curve);
    _roto->evaluate(true);
    _roto->setSelection( BezierPtr(), std::make_pair( CpPtr(), CpPtr() ) );
    setText( QObject::tr("Build Ellipse %1 of %2").arg( _curve->getLabel().c_str() ).arg( _roto->getNodeName() ) );
}

void
MakeRectangleUndoCommand::redo()
{
    if (_firstRedoCalled) {
        _roto->getContext()->addItem(_parentLayer, _indexInLayer, _curve, RotoItem::eSelectionReasonOverlayInteract);
        _roto->evaluate(true);
    } else {
        double ytop, xright, ybottom, xleft;
        xright = _tox;
        if (_constrained) {
            xright =  _fromx + (_tox > _fromx ? 1 : -1) * std::abs(_toy - _fromy);
        }
        if (_fromCenter) {
            ytop = _fromy - (_toy - _fromy);
            xleft = _fromx - (xright - _fromx);
            if (xleft == xright) {
                xleft -= 1.;
            }
        } else {
            ytop = _fromy;
            xleft = _fromx;
            if (xleft == xright) {
                xleft -= 1.;
            }
        }
        ybottom = _toy;
        if (ybottom == ytop) {
            ybottom -= 1.;
        }
        if (_create) {
            _curve = _roto->getContext()->makeBezier(xleft, ytop, kRotoRectangleBaseName, _time, false); //topleft
            assert(_curve);
            _curve->addControlPoint(xright, ytop, _time); // topright
            _curve->addControlPoint(xright, ybottom, _time); // bottomright
            _curve->addControlPoint(xleft, ybottom, _time); // bottomleft
            _curve->setCurveFinished(true);
        } else {
            _curve->setPointByIndex(0, _time, xleft, ytop); // topleft
            _curve->setPointByIndex(1,_time, xright, ytop); // topright
            _curve->setPointByIndex(2,_time, xright, ybottom); // bottomright
            _curve->setPointByIndex(3,_time, xleft, ybottom); // bottomleft
        }
        boost::shared_ptr<RotoItem> parentItem =  _roto->getContext()->getItemByName( _curve->getParentLayer()->getScriptName() );
        if (parentItem) {
            _parentLayer = boost::dynamic_pointer_cast<RotoLayer>(parentItem);
            _indexInLayer = _parentLayer->getChildIndex(_curve);
        }
    }
    _roto->setBuiltBezier(_curve);
    _firstRedoCalled = true;
    _roto->setSelection( _curve, std::make_pair( CpPtr(), CpPtr() ) );
    setText( QObject::tr("Build Rectangle %1 of %2").arg( _curve->getScriptName().c_str() ).arg( _roto->getNodeName() ) );
}

int
MakeRectangleUndoCommand::id() const
{
    return kRotoMakeRectangleCompressionID;
}

bool
MakeRectangleUndoCommand::mergeWith(const QUndoCommand *other)
{
    const MakeRectangleUndoCommand* sCmd = dynamic_cast<const MakeRectangleUndoCommand*>(other);

    if (!sCmd) {
        return false;
    }
    if ( (sCmd->_curve != _curve) || sCmd->_create ) {
        return false;
    }
    if (_curve != sCmd->_curve) {
        _curve->clone(sCmd->_curve.get());
    }
    _fromx = sCmd->_fromx;
    _fromy = sCmd->_fromy;
    _tox = sCmd->_tox;
    _toy = sCmd->_toy;
    _fromCenter = sCmd->_fromCenter;
    _constrained = sCmd->_constrained;

    return true;
}

//////////////////////////////


RemoveItemsUndoCommand::RemoveItemsUndoCommand(RotoPanel* roto,
                                               const QList<QTreeWidgetItem*> & items)
    : QUndoCommand()
      , _roto(roto)
      , _items()
{
    for (QList<QTreeWidgetItem*>::const_iterator it = items.begin(); it != items.end(); ++it) {
        
        QTreeWidgetItem* parentItem = (*it)->parent();
        bool foundParent = false;
        for (QList<QTreeWidgetItem*>::const_iterator it2 = items.begin(); it2 != items.end(); ++it2) {
            if ((*it2) == parentItem) {
                foundParent = true;
                break;
            }
        }
        if (foundParent) {
            //Not necessary to add this item to the list since the parent is going to remove it anyway
            continue;
        }
        RemovedItem r;
        r.treeItem = *it;
        r.parentTreeItem = parentItem;
        r.item = _roto->getRotoItemForTreeItem(r.treeItem);
        assert(r.item);
        if (r.parentTreeItem) {
            r.parentLayer = boost::dynamic_pointer_cast<RotoLayer>( _roto->getRotoItemForTreeItem(r.parentTreeItem) );
            assert(r.parentLayer);
            r.indexInLayer = r.parentLayer->getChildIndex(r.item);
        }
        _items.push_back(r);
    }
}

RemoveItemsUndoCommand::~RemoveItemsUndoCommand()
{
}

void
RemoveItemsUndoCommand::undo()
{
    for (std::list<RemovedItem>::iterator it = _items.begin(); it != _items.end(); ++it) {
        if (it->parentTreeItem) {
            it->parentTreeItem->addChild(it->treeItem);
        }
        _roto->getContext()->addItem(it->parentLayer, it->indexInLayer, it->item, RotoItem::eSelectionReasonSettingsPanel);

        it->treeItem->setHidden(false);
    }
    _roto->getContext()->evaluateChange();
    setText( QObject::tr("Remove items of %2").arg( _roto->getNodeName().c_str() ) );
}

void
RemoveItemsUndoCommand::redo()
{
    if (_items.empty()) {
        return;
    }
    _roto->clearAndSelectPreviousItem(_items.back().item);
    for (std::list<RemovedItem>::iterator it = _items.begin(); it != _items.end(); ++it) {
        _roto->getContext()->removeItem(it->item, RotoItem::eSelectionReasonSettingsPanel);
        it->treeItem->setHidden(true);
        if ( it->treeItem->isSelected() ) {
            it->treeItem->setSelected(false);
        }
        if (it->parentTreeItem) {
            it->parentTreeItem->removeChild(it->treeItem);
        }
    }
    _roto->getContext()->evaluateChange();
    setText( QObject::tr("Remove items of %2").arg( _roto->getNodeName().c_str() ) );
}

/////////////////////////////


AddLayerUndoCommand::AddLayerUndoCommand(RotoPanel* roto)
    : QUndoCommand()
      , _roto(roto)
      , _firstRedoCalled(false)
      , _parentLayer()
      , _parentTreeItem(0)
      , _treeItem(0)
      , _layer()
      , _indexInParentLayer(-1)
{
}

AddLayerUndoCommand::~AddLayerUndoCommand()
{
}

void
AddLayerUndoCommand::undo()
{
    _treeItem->setHidden(true);
    if (_parentTreeItem) {
        _parentTreeItem->removeChild(_treeItem);
    }
    _roto->getContext()->removeItem(_layer, RotoItem::eSelectionReasonSettingsPanel);
    _roto->clearSelection();
    _roto->getContext()->evaluateChange();
    setText( QObject::tr("Add layer to %2").arg( _roto->getNodeName().c_str() ) );
}

void
AddLayerUndoCommand::redo()
{
    if (!_firstRedoCalled) {
        _layer = _roto->getContext()->addLayer();
        _parentLayer = _layer->getParentLayer();
        _treeItem = _roto->getTreeItemForRotoItem(_layer);
        _parentTreeItem = _treeItem->parent();
        if (_parentLayer) {
            _indexInParentLayer = _parentLayer->getChildIndex(_layer);
        }
    } else {
        _roto->getContext()->addLayer(_layer);
        _treeItem->setHidden(false);
        if (_parentLayer) {
            _roto->getContext()->addItem(_parentLayer, _indexInParentLayer, _layer, RotoItem::eSelectionReasonSettingsPanel);
            _parentTreeItem->addChild(_treeItem);
        }
    }
    _roto->clearSelection();
    _roto->getContext()->select(_layer, RotoItem::eSelectionReasonOther);
    _roto->getContext()->evaluateChange();
    setText( QObject::tr("Add layer to %2").arg( _roto->getNodeName().c_str() ) );
    _firstRedoCalled = true;
}

/////////////////////////////////

DragItemsUndoCommand::DragItemsUndoCommand(RotoPanel* roto,
                                           const std::list< boost::shared_ptr<DroppedTreeItem> > & items)
    : QUndoCommand()
      , _roto(roto)
      , _items()
{
    for (std::list< boost::shared_ptr<DroppedTreeItem> >::const_iterator it = items.begin(); it != items.end(); ++it) {
        assert( (*it)->newParentLayer && (*it)->newParentItem && (*it)->insertIndex != -1 );
        Item i;
        i.dropped = *it;
        i.oldParentItem = (*it)->dropped->parent();
        if (!i.oldParentItem) {
            continue;
        }
        i.oldParentLayer = (*it)->droppedRotoItem->getParentLayer();
        if (i.oldParentLayer) {
            i.indexInOldLayer = i.oldParentLayer->getChildIndex( (*it)->droppedRotoItem );
        } else {
            i.indexInOldLayer = -1;
        }
        _items.push_back(i);
    }
}

DragItemsUndoCommand::~DragItemsUndoCommand()
{
}

static void
createCustomWidgetRecursively(RotoPanel* panel,
                              const boost::shared_ptr<RotoItem>& item)
{
    const boost::shared_ptr<RotoDrawableItem> isDrawable = boost::dynamic_pointer_cast<RotoDrawableItem>(item);

    if (isDrawable) {
        panel->makeCustomWidgetsForItem(isDrawable);
    }
    const boost::shared_ptr<RotoLayer> isLayer = boost::dynamic_pointer_cast<RotoLayer>(item);
    if (isLayer) {
        const std::list<boost::shared_ptr<RotoItem> > & children = isLayer->getItems();
        for (std::list<boost::shared_ptr<RotoItem> >::const_iterator it = children.begin(); it != children.end(); ++it) {
            createCustomWidgetRecursively( panel, *it );
        }
    }
}

void
DragItemsUndoCommand::undo()
{
    for (std::list<Item>::iterator it = _items.begin(); it != _items.end(); ++it) {
        assert(it->dropped->newParentItem);
        it->dropped->newParentItem->removeChild(it->dropped->dropped);
        it->dropped->newParentLayer->removeItem(it->dropped->droppedRotoItem);
        if (it->oldParentItem) {
            it->oldParentItem->insertChild(it->indexInOldLayer, it->dropped->dropped);

            createCustomWidgetRecursively( _roto,it->dropped->droppedRotoItem );

            assert(it->oldParentLayer);
            it->dropped->droppedRotoItem->setParentLayer(it->oldParentLayer);
            _roto->getContext()->addItem(it->oldParentLayer, it->indexInOldLayer, it->dropped->droppedRotoItem, RotoItem::eSelectionReasonSettingsPanel);
        } else {
            it->dropped->droppedRotoItem->setParentLayer(boost::shared_ptr<RotoLayer>());
        }
    }
    _roto->getContext()->refreshRotoPaintTree();
    _roto->getContext()->evaluateChange();
    
    setText( QObject::tr("Re-organize items of %2").arg( _roto->getNodeName().c_str() ) );
}

void
DragItemsUndoCommand::redo()
{
    for (std::list<Item>::iterator it = _items.begin(); it != _items.end(); ++it) {
        it->oldParentItem->removeChild(it->dropped->dropped);
        if (it->oldParentLayer) {
            it->oldParentLayer->removeItem(it->dropped->droppedRotoItem);
        }
        assert(it->dropped->newParentItem);
        it->dropped->newParentItem->insertChild(it->dropped->insertIndex,it->dropped->dropped);

        createCustomWidgetRecursively( _roto,it->dropped->droppedRotoItem );

        it->dropped->newParentItem->setExpanded(true);
        it->dropped->newParentLayer->insertItem(it->dropped->droppedRotoItem, it->dropped->insertIndex);
    }
    _roto->getContext()->refreshRotoPaintTree();
    _roto->getContext()->evaluateChange();
    setText( QObject::tr("Re-organize items of %2").arg( _roto->getNodeName().c_str() ) );
}

//////////////////////

static std::string
getItemCopyName(RotoPanel* roto,
                const boost::shared_ptr<RotoItem>& originalItem)
{
    int i = 1;
    std::string name = originalItem->getScriptName() + "_copy";
    boost::shared_ptr<RotoItem> foundItemWithName = roto->getContext()->getItemByName(name);

    while (foundItemWithName && foundItemWithName != originalItem) {
        std::stringstream ss;
        ss << originalItem->getScriptName()  << "_copy " << i;
        name = ss.str();
        foundItemWithName = roto->getContext()->getItemByName(name);
        ++i;
    }

    return name;
}

static
void
setItemCopyNameRecursive(RotoPanel* panel,
                         const boost::shared_ptr<RotoItem>& item)
{
    item->setScriptName( getItemCopyName(panel, item) );
    boost::shared_ptr<RotoLayer> isLayer = boost::dynamic_pointer_cast<RotoLayer>(item);

    if (isLayer) {
        for (std::list<boost::shared_ptr<RotoItem> >::const_iterator it = isLayer->getItems().begin(); it != isLayer->getItems().end(); ++it) {
            setItemCopyNameRecursive( panel, *it );
        }
    }
}

PasteItemUndoCommand::PasteItemUndoCommand(RotoPanel* roto,
                                           QTreeWidgetItem* target,
                                           QList<QTreeWidgetItem*> source)
    : QUndoCommand()
      , _roto(roto)
      , _mode()
      , _targetTreeItem(target)
      , _targetItem()
      , _pastedItems()
{
    _targetItem = roto->getRotoItemForTreeItem(target);
    assert(_targetItem);

    for (int i  = 0; i < source.size(); ++i) {
        PastedItem item;
        item.treeItem = source[i];
        item.rotoItem = roto->getRotoItemForTreeItem(item.treeItem);
        assert(item.rotoItem);
        _pastedItems.push_back(item);
    }

    boost::shared_ptr<RotoDrawableItem> isDrawable = boost::dynamic_pointer_cast<RotoDrawableItem>(_targetItem);

    if (isDrawable) {
        _mode = ePasteModeCopyToItem;
        assert(source.size() == 1 && _pastedItems.size() == 1);
        assert( dynamic_cast<RotoDrawableItem*>( _pastedItems.front().rotoItem.get() ) );
    } else {
        _mode = ePasteModeCopyToLayer;
        for (std::list<PastedItem>::iterator it = _pastedItems.begin(); it != _pastedItems.end(); ++it) {
            boost::shared_ptr<Bezier> srcBezier = boost::dynamic_pointer_cast<Bezier>(it->rotoItem);
            boost::shared_ptr<RotoLayer> srcLayer = boost::dynamic_pointer_cast<RotoLayer>(it->rotoItem);
            boost::shared_ptr<RotoStrokeItem> srcStroke = boost::dynamic_pointer_cast<RotoStrokeItem>(it->rotoItem);
            if (srcBezier) {
                std::string name = getItemCopyName(roto, it->rotoItem);
                boost::shared_ptr<Bezier> copy( new Bezier(srcBezier->getContext(),name,
                                                           srcBezier->getParentLayer(), false) );
                copy->clone(srcBezier.get());
                copy->createNodes();
                //clone overwrittes the script name, don't forget to set it back
                copy->setScriptName(name);
                copy->setLabel(name);
                it->itemCopy = copy;
            } else if (srcStroke) {
                std::string name = getItemCopyName(roto, it->rotoItem);
                boost::shared_ptr<RotoStrokeItem> copy(new RotoStrokeItem(srcStroke->getBrushType(),
                                                                          srcStroke->getContext(),
                                                                          name,
                                                                          boost::shared_ptr<RotoLayer>()));
                copy->createNodes();
                if (srcStroke->getParentLayer()) {
                    srcStroke->getParentLayer()->insertItem(copy, 0);
                }
                copy->clone(srcStroke.get());
                copy->createNodes();
                //clone overwrittes the script name, don't forget to set it back
                copy->setScriptName(name);
                copy->setLabel(name);
                it->itemCopy = copy;
            } else {
                assert(srcLayer);
                boost::shared_ptr<RotoLayer> copy( new RotoLayer(srcLayer->getContext(),
                                                                 "",
                                                                 boost::shared_ptr<RotoLayer>()) );
                copy->clone(srcLayer.get());
                setItemCopyNameRecursive( roto, copy );
                it->itemCopy = copy;
            }
        }
    }
}

PasteItemUndoCommand::~PasteItemUndoCommand()
{
}

void
PasteItemUndoCommand::undo()
{
    if (_mode == ePasteModeCopyToItem) {
        assert(_oldTargetItem);
        _roto->getContext()->deselect(_targetItem, RotoItem::eSelectionReasonOther);
        _targetItem->clone(_oldTargetItem.get());
        _roto->updateItemGui(_targetTreeItem);
        _roto->getContext()->select(_targetItem, RotoItem::eSelectionReasonOther);
    } else {
        // check that it is a RotoLayer
        assert( dynamic_cast<RotoLayer*>( _targetItem.get() ) );
        for (std::list<PastedItem>::iterator it = _pastedItems.begin(); it != _pastedItems.end(); ++it) {
            _roto->getContext()->removeItem(it->itemCopy, RotoItem::eSelectionReasonOther);
        }
    }
    _roto->getContext()->evaluateChange();
    setText( QObject::tr("Paste item(s) of %2").arg( _roto->getNodeName().c_str() ) );
}

void
PasteItemUndoCommand::redo()
{
    if (_mode == ePasteModeCopyToItem) {
        Bezier* isBezier = dynamic_cast<Bezier*>( _targetItem.get() );
        RotoStrokeItem* isStroke = dynamic_cast<RotoStrokeItem*>( _targetItem.get() );
        if (isBezier) {
            boost::shared_ptr<Bezier> oldBezier( new Bezier(isBezier->getContext(),isBezier->getScriptName(),isBezier->getParentLayer(), false) );
            oldBezier->createNodes();
            _oldTargetItem = oldBezier;
        } else if (isStroke) {
            boost::shared_ptr<RotoStrokeItem> oldStroke( new RotoStrokeItem(isStroke->getBrushType(),isStroke->getContext(),isStroke->getScriptName(),boost::shared_ptr<RotoLayer>()) );
            oldStroke->createNodes();
            _oldTargetItem = oldStroke;
            if (isStroke->getParentLayer()) {
                //isStroke->getParentLayer()->insertItem(_oldTargetItem, 0);
            }
            
        }
        _oldTargetItem->clone(_targetItem.get());
        assert(_pastedItems.size() == 1);
        PastedItem & front = _pastedItems.front();
        ///If we don't deselct the updateItemGUI call will not function correctly because the knobs GUI
        ///have not been refreshed and the selected item is linked to those dirty knobs
        _roto->getContext()->deselect(_targetItem, RotoItem::eSelectionReasonOther);
        _targetItem->clone(front.rotoItem.get());
        _targetItem->setScriptName( _oldTargetItem->getScriptName() );
        _roto->updateItemGui(_targetTreeItem);
        _roto->getContext()->select(_targetItem, RotoItem::eSelectionReasonOther);
    } else {
        boost::shared_ptr<RotoLayer> isLayer = boost::dynamic_pointer_cast<RotoLayer>(_targetItem);
        assert(isLayer);
        for (std::list<PastedItem>::iterator it = _pastedItems.begin(); it != _pastedItems.end(); ++it) {
            assert(it->itemCopy);
            //it->itemCopy->setParentLayer(isLayer);
            _roto->getContext()->addItem(isLayer, 0, it->itemCopy, RotoItem::eSelectionReasonOther);
        }
    }

    _roto->getContext()->evaluateChange();
    setText( QObject::tr("Paste item(s) of %2").arg( _roto->getNodeName().c_str() ) );
}

//////////////////


DuplicateItemUndoCommand::DuplicateItemUndoCommand(RotoPanel* roto,
                                                   QTreeWidgetItem* items)
    : QUndoCommand()
      , _roto(roto)
      , _item()
{
    _item.treeItem = items;
    _item.item = _roto->getRotoItemForTreeItem(_item.treeItem);
    assert( _item.item->getParentLayer() );
    boost::shared_ptr<Bezier> isBezier = boost::dynamic_pointer_cast<Bezier>( _item.item );
    boost::shared_ptr<RotoStrokeItem> isStroke = boost::dynamic_pointer_cast<RotoStrokeItem>( _item.item);
    boost::shared_ptr<RotoLayer> isLayer = boost::dynamic_pointer_cast<RotoLayer>( _item.item);
    if (isBezier) {
        std::string name = getItemCopyName(roto, isBezier);
        boost::shared_ptr<Bezier> bezierCopy( new Bezier(isBezier->getContext(),name,isBezier->getParentLayer(), false) );
        bezierCopy->createNodes();
        _item.duplicatedItem = bezierCopy;
        _item.duplicatedItem->clone(isBezier.get());
        //clone has overwritten the name
        _item.duplicatedItem->setScriptName(name);
        _item.duplicatedItem->setLabel(name);
    } else if (isStroke) {
        std::string name = getItemCopyName(roto, isStroke);
        boost::shared_ptr<RotoStrokeItem> strokeCopy( new RotoStrokeItem(isStroke->getBrushType(),isStroke->getContext(),name, boost::shared_ptr<RotoLayer>()) );
        strokeCopy->createNodes();
        _item.duplicatedItem = strokeCopy;
        if (isStroke->getParentLayer()) {
            isStroke->getParentLayer()->insertItem(_item.duplicatedItem, 0);
        }
        _item.duplicatedItem->clone(isStroke.get());
        //clone has overwritten the name
        _item.duplicatedItem->setScriptName(name);
        _item.duplicatedItem->setLabel(name);

    } else {
        assert(isLayer);
        _item.duplicatedItem.reset( new RotoLayer(*isLayer) );
        setItemCopyNameRecursive( roto, _item.duplicatedItem );
    }

}

DuplicateItemUndoCommand::~DuplicateItemUndoCommand()
{
}

void
DuplicateItemUndoCommand::undo()
{
    _roto->getContext()->removeItem(_item.duplicatedItem, RotoItem::eSelectionReasonOther);
    _roto->getContext()->evaluateChange();
    setText( QObject::tr("Duplicate item(s) of %2").arg( _roto->getNodeName().c_str() ) );
}

void
DuplicateItemUndoCommand::redo()
{
    _roto->getContext()->addItem(_item.item->getParentLayer(),
                                 0, _item.duplicatedItem, RotoItem::eSelectionReasonOther);

    _roto->getContext()->evaluateChange();
    setText( QObject::tr("Duplicate item(s) of %2").arg( _roto->getNodeName().c_str() ) );
}

LinkToTrackUndoCommand::LinkToTrackUndoCommand(RotoGui* roto,
                                               const SelectedCpList & points,
                                               const boost::shared_ptr<KnobDouble> & track)
    : QUndoCommand()
      , _roto(roto)
      , _points(points)
      , _track(track)
{
}

LinkToTrackUndoCommand::~LinkToTrackUndoCommand()
{
}

void
LinkToTrackUndoCommand::undo()
{
    for (SelectedCpList::iterator it = _points.begin(); it != _points.end(); ++it) {
        it->first->unslave();
        _track->removeSlavedTrack(it->first);
        if ( it->second->isSlaved() ) {
            it->second->unslave();
            _track->removeSlavedTrack(it->second);
        }
    }
    setText( QObject::tr("Link to track") );
    _roto->evaluate(true);
}

void
LinkToTrackUndoCommand::redo()
{
    SequenceTime time = _roto->getContext()->getTimelineCurrentTime();
    //bool featherLinkEnabled = _roto->getContext()->isFeatherLinkEnabled();

    for (SelectedCpList::iterator it = _points.begin(); it != _points.end(); ++it) {
        it->first->slaveTo(time,_track);
        _track->addSlavedTrack(it->first);

      //  if (featherLinkEnabled) {
            it->second->slaveTo(time, _track);
            _track->addSlavedTrack(it->second);
       // }
    }
    setText( QObject::tr("Link to track") );
    _roto->evaluate(true);
}

UnLinkFromTrackUndoCommand::UnLinkFromTrackUndoCommand(RotoGui* roto,
                                                       const std::list<std::pair<boost::shared_ptr<BezierCP>,boost::shared_ptr<BezierCP> > > & points)
    : QUndoCommand()
      , _roto(roto)
      , _points()
{
    for (SelectedCpList::const_iterator it = points.begin(); it != points.end(); ++it) {
        PointToUnlink p;
        p.cp = !it->first->isFeatherPoint() ? it->first : it->second;
        p.fp = !it->first->isFeatherPoint() ? it->second : it->first;
        p.track = p.cp->isSlaved();
        _points.push_back(p);
    }
}

UnLinkFromTrackUndoCommand::~UnLinkFromTrackUndoCommand()
{
}

void
UnLinkFromTrackUndoCommand::undo()
{
    SequenceTime time = _roto->getContext()->getTimelineCurrentTime();
    bool featherLinkEnabled = _roto->getContext()->isFeatherLinkEnabled();

    for (std::list<PointToUnlink>::iterator it = _points.begin(); it != _points.end(); ++it) {
        it->cp->slaveTo(time,it->track);
        it->track->addSlavedTrack(it->cp);

        if (featherLinkEnabled) {
            it->fp->slaveTo(time, it->track);
            it->track->addSlavedTrack(it->fp);
        }
    }
    setText( QObject::tr("Unlink from track") );

    _roto->evaluate(true);
}

void
UnLinkFromTrackUndoCommand::redo()
{
    for (std::list<PointToUnlink>::iterator it = _points.begin(); it != _points.end(); ++it) {
        it->cp->unslave();
        it->track->removeSlavedTrack(it->cp);
        if ( it->fp->isSlaved() ) {
            it->fp->unslave();
            it->track->removeSlavedTrack(it->fp);
        }
    }
    _roto->evaluate(true);
    setText( QObject::tr("Unlink from track") );
}

