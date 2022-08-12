/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2022 The Natron developers
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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "RotoUndoCommand.h"

#include <stdexcept>

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QDebug>
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
#include "Engine/RotoPaintInteract.h"
#include "Engine/RotoPaint.h"
#include "Engine/Transform.h"
#include "Engine/ViewIdx.h"


NATRON_NAMESPACE_ENTER

typedef BezierCPPtr CpPtr;
typedef std::pair<CpPtr, CpPtr> SelectedCp;
typedef std::list<SelectedCp> SelectedCpList;
typedef BezierPtr BezierPtr;
typedef std::list<BezierPtr> BezierList;

MoveControlPointsUndoCommand::MoveControlPointsUndoCommand(const RotoPaintInteractPtr& roto,
                                                           const std::list<std::pair<BezierCPPtr, BezierCPPtr> > & toDrag
                                                           ,
                                                           double dx,
                                                           double dy,
                                                           double time)
    : UndoCommand()
    , _firstRedoCalled(false)
    , _roto(roto)
    , _dx(dx)
    , _dy(dy)
    , _featherLinkEnabled( roto->getContext()->isFeatherLinkEnabled() )
    , _rippleEditEnabled( roto->getContext()->isRippleEditEnabled() )
    , _selectedTool(roto->selectedToolAction )
    , _time(time)
    , _pointsToDrag(toDrag)
{
    setText( tr("Move control points").toStdString() );

    assert(roto);

    roto->getSelection(&_selectedCurves, &_selectedPoints);

    ///we make a copy of the points
    for (SelectedCpList::iterator it = _pointsToDrag.begin(); it != _pointsToDrag.end(); ++it) {
        CpPtr first, second;
        if ( it->first->isFeatherPoint() ) {
            first.reset( new FeatherPoint( it->first->getBezier() ) );
            first->clone( *(it->first) );
        } else {
            first.reset( new BezierCP( *(it->first) ) );
        }

        if (it->second) {
            if ( it->second->isFeatherPoint() ) {
                second.reset( new FeatherPoint( it->second->getBezier() ) );
                second->clone( *(it->second) );
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
        beziers.insert( it->first->getBezier().get() );
    }

    for (SelectedCpList::iterator it = _pointsToDrag.begin(); it != _pointsToDrag.end(); ++it, ++cpIt) {
        it->first->clone(*cpIt->first);
        if (it->second) {
            it->second->clone(*cpIt->second);
        }
    }

    for (std::set<Bezier*>::iterator it = beziers.begin(); it != beziers.end(); ++it) {
        (*it)->incrementNodesAge();
    }

    RotoPaintInteractPtr roto = _roto.lock();
    roto->evaluate(true);
    roto->setCurrentTool( _selectedTool.lock() );
    roto->setSelection(_selectedCurves, _selectedPoints);
}

void
MoveControlPointsUndoCommand::redo()
{
    SelectedCpList::iterator itPoints = _pointsToDrag.begin();

    assert( _pointsToDrag.size() == _indexesToMove.size() );

    RotoPaintInteractPtr roto = _roto.lock();
    if (!roto) {
        return;
    }
    RotoToolEnum selectedTool;
    bool ok = roto->getToolForAction(_selectedTool.lock(), &selectedTool);

    try {
        for (std::list<int>::iterator it = _indexesToMove.begin(); it != _indexesToMove.end(); ++it, ++itPoints) {
            if ( itPoints->first->isFeatherPoint() ) {
                if ( ok && ( ( selectedTool == eRotoToolSelectFeatherPoints ) ||
                             ( selectedTool == eRotoToolSelectAll ) ||
                             ( selectedTool == eRotoToolDrawBezier ) ) ) {
                    itPoints->first->getBezier()->moveFeatherByIndex(*it, _time, _dx, _dy);
                }
            } else {
                if ( ok && ( ( selectedTool == eRotoToolSelectPoints ) ||
                             ( selectedTool == eRotoToolSelectAll ) ||
                             ( selectedTool == eRotoToolDrawBezier ) ) ) {
                    itPoints->first->getBezier()->movePointByIndex(*it, _time, _dx, _dy);
                }
            }
        }
    } catch (const std::exception & e) {
        qDebug() << "Exception while operating MoveControlPointsUndoCommand::redo(): " << e.what();
    }

    if (_firstRedoCalled) {
        roto->setSelection(_selectedCurves, _selectedPoints);
        roto->evaluate(true);
    }

    _firstRedoCalled = true;
}

bool
MoveControlPointsUndoCommand::mergeWith(const UndoCommandPtr& other)
{
    const MoveControlPointsUndoCommand* mvCmd = dynamic_cast<const MoveControlPointsUndoCommand*>( other.get() );

    if (!mvCmd) {
        return false;
    }

    if ( ( mvCmd->_pointsToDrag.size() != _pointsToDrag.size() ) || (mvCmd->_time != _time) || ( mvCmd->_selectedTool.lock() != _selectedTool.lock() )
         || ( mvCmd->_rippleEditEnabled != _rippleEditEnabled) || ( mvCmd->_featherLinkEnabled != _featherLinkEnabled) ) {
        return false;
    }

    std::list<std::pair<BezierCPPtr, BezierCPPtr> >::const_iterator it = _pointsToDrag.begin();
    std::list<std::pair<BezierCPPtr, BezierCPPtr> >::const_iterator oIt = mvCmd->_pointsToDrag.begin();
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

TransformUndoCommand::TransformUndoCommand(const RotoPaintInteractPtr& roto,
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
    : UndoCommand()
    , _firstRedoCalled(false)
    , _roto(roto)
    , _rippleEditEnabled( roto->getContext()->isRippleEditEnabled() )
    , _selectedTool(roto->selectedToolAction)
    , _matrix()
    , _time(time)
    , _selectedCurves()
    , _originalPoints()
    , _selectedPoints()
{
    _matrix = std::make_shared<Transform::Matrix3x3>();

    std::list<std::pair<BezierCPPtr, BezierCPPtr> > selected;

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

    setText( tr("Transform control points").toStdString() );
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
        beziers.insert( it->first->getBezier().get() );
    }

    for (std::set<Bezier*>::iterator it = beziers.begin(); it != beziers.end(); ++it) {
        (*it)->incrementNodesAge();
    }


    for (SelectedCpList::iterator it = _selectedPoints.begin(); it != _selectedPoints.end(); ++it, ++cpIt) {
        it->first->clone(*cpIt->first);
        if (it->second) {
            it->second->clone(*cpIt->second);
        }
    }

    RotoPaintInteractPtr roto = _roto.lock();
    if (!roto) {
        return;
    }

    roto->evaluate(true);
    roto->setCurrentTool( _selectedTool.lock() );
    roto->setSelection(_selectedCurves, _selectedPoints);
}

void
TransformUndoCommand::transformPoint(const BezierCPPtr & point)
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

    RotoPaintInteractPtr roto = _roto.lock();
    if (!roto) {
        return;
    }
    if (_firstRedoCalled) {
        roto->setSelection(_selectedCurves, _selectedPoints);
        roto->evaluate(true);
    } else {
        roto->computeSelectedCpsBBOX();
        roto->redrawOverlays();
    }

    _firstRedoCalled = true;
}

bool
TransformUndoCommand::mergeWith(const UndoCommandPtr& other)
{
    const TransformUndoCommand* cmd = dynamic_cast<const TransformUndoCommand*>( other.get() );

    if (!cmd) {
        return false;
    }

    if ( ( cmd->_selectedPoints.size() != _selectedPoints.size() ) || (cmd->_time != _time) || ( cmd->_selectedTool.lock() != _selectedTool.lock() )
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


AddPointUndoCommand::AddPointUndoCommand(const RotoPaintInteractPtr& roto,
                                         const BezierPtr & curve,
                                         int index,
                                         double t)
    : UndoCommand()
    , _firstRedoCalled(false)
    , _roto(roto)
    , _curve(curve)
    , _index(index)
    , _t(t)
{
    setText( tr("Add control point").toStdString() );
}

AddPointUndoCommand::~AddPointUndoCommand()
{
}

void
AddPointUndoCommand::undo()
{
    RotoPaintInteractPtr roto = _roto.lock();

    if (!roto) {
        return;
    }
    _curve->removeControlPointByIndex(_index + 1);
    roto->setSelection( _curve, std::make_pair( CpPtr(), CpPtr() ) );
    roto->evaluate(true);
}

void
AddPointUndoCommand::redo()
{
    BezierCPPtr cp = _curve->addControlPointAfterIndex(_index, _t);
    BezierCPPtr newFp = _curve->getFeatherPointAtIndex(_index + 1);
    RotoPaintInteractPtr roto = _roto.lock();

    if (!roto) {
        return;
    }
    roto->setSelection( _curve, std::make_pair(cp, newFp) );
    if (_firstRedoCalled) {
        roto->evaluate(true);
    }

    _firstRedoCalled = true;
}

////////////////////////

RemovePointUndoCommand::RemovePointUndoCommand(const RotoPaintInteractPtr& roto,
                                               const BezierPtr & curve,
                                               const BezierCPPtr & cp)
    : UndoCommand()
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
        std::dynamic_pointer_cast<RotoLayer>( roto->getContext()->getItemByName( curve->getParentLayer()->getScriptName() ) );
    assert(desc.parentLayer);
    desc.curve = curve;
    desc.points.push_back(indexToRemove);
    desc.oldCurve.reset( new Bezier(curve->getContext(), curve->getScriptName(), curve->getParentLayer(), false) );
    desc.oldCurve->clone( curve.get() );
    _curves.push_back(desc);
}

RemovePointUndoCommand::RemovePointUndoCommand(const RotoPaintInteractPtr& roto,
                                               const SelectedCpList & points)
    : UndoCommand()
    , _roto(roto)
    , _firstRedoCalled(false)
    , _curves()
{
    for (SelectedCpList::const_iterator it = points.begin(); it != points.end(); ++it) {
        BezierCPPtr cp;
        if ( it->first->isFeatherPoint() ) {
            cp = it->second;
        } else {
            cp = it->first;
        }
        assert( cp && cp->getBezier() && roto && roto->getContext() );
        BezierPtr curve = std::dynamic_pointer_cast<Bezier>( roto->getContext()->getItemByName( cp->getBezier()->getScriptName() ) );
        assert(curve);
        RotoStrokeItem* isStroke = dynamic_cast<RotoStrokeItem*>( curve.get() );
        std::list<CurveDesc >::iterator foundCurve = _curves.end();
        for (std::list<CurveDesc >::iterator it2 = _curves.begin(); it2 != _curves.end(); ++it2) {
            if (it2->curve == curve) {
                foundCurve = it2;
                break;
            }
        }
        assert(curve);
        assert(cp);
        if (!curve) {
            continue;
        }
        int indexToRemove = curve->getControlPointIndex(cp);
        if ( foundCurve == _curves.end() ) {
            CurveDesc curveDesc;
            curveDesc.curveRemoved = false; //set in the redo()
            curveDesc.parentLayer =
                std::dynamic_pointer_cast<RotoLayer>( roto->getContext()->getItemByName( cp->getBezier()->getParentLayer()->getScriptName() ) );
            assert(curveDesc.parentLayer);
            curveDesc.points.push_back(indexToRemove);
            curveDesc.curve = curve;
            if (!isStroke) {
                curveDesc.oldCurve.reset( new Bezier(curve->getContext(), curve->getScriptName(), curve->getParentLayer(), false) );
            } else {
                curveDesc.oldCurve.reset( new RotoStrokeItem( isStroke->getBrushType(), curve->getContext(), curve->getScriptName(), curve->getParentLayer() ) );
            }
            curveDesc.oldCurve->clone( curve.get() );
            _curves.push_back(curveDesc);
        } else {
            foundCurve->points.push_back(indexToRemove);
        }
    }
    for (std::list<CurveDesc>::iterator it = _curves.begin(); it != _curves.end(); ++it) {
        it->points.sort();
    }

    setText( tr("Remove control points").toStdString() );
}

RemovePointUndoCommand::~RemovePointUndoCommand()
{
}

void
RemovePointUndoCommand::undo()
{
    RotoPaintInteractPtr roto = _roto.lock();

    if (!roto) {
        return;
    }

    std::list<RotoDrawableItemPtr> selection;
    SelectedCpList cpSelection;

    for (std::list<CurveDesc >::iterator it = _curves.begin(); it != _curves.end(); ++it) {
        ///clone the curve
        it->curve->clone( it->oldCurve.get() );
        if (it->curveRemoved) {
            roto->getContext()->addItem(it->parentLayer, it->indexInLayer, it->curve, RotoItem::eSelectionReasonOverlayInteract);
        }
        selection.push_back(it->curve);
    }

    roto->setSelection(selection, cpSelection);
    roto->evaluate(true);
}

void
RemovePointUndoCommand::redo()
{
    RotoPaintInteractPtr roto = _roto.lock();

    if (!roto) {
        return;
    }

    ///clone the curve
    for (std::list<CurveDesc >::iterator it = _curves.begin(); it != _curves.end(); ++it) {
        it->oldCurve->clone( it->curve.get() );
    }

    std::list<BezierPtr> toRemove;
    for (std::list<CurveDesc >::iterator it = _curves.begin(); it != _curves.end(); ++it) {
        BezierPtr isBezier = std::dynamic_pointer_cast<Bezier>(it->curve);
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
                std::list<BezierPtr>::iterator found = std::find( toRemove.begin(), toRemove.end(), it->curve );
                if ( found == toRemove.end() ) {
                    toRemove.push_back(isBezier);
                }
            }
        }
        isBezier->setAutoOrientationComputation(true);
        isBezier->refreshPolygonOrientation(true);
    }

    for (std::list<BezierPtr>::iterator it = toRemove.begin(); it != toRemove.end(); ++it) {
        roto->removeCurve(*it);
    }


    roto->setSelection( BezierPtr(), std::make_pair( CpPtr(), CpPtr() ) );
    roto->evaluate(_firstRedoCalled);
    _firstRedoCalled = true;
}

//////////////////////////

RemoveCurveUndoCommand::RemoveCurveUndoCommand(const RotoPaintInteractPtr& roto,
                                               const std::list<RotoDrawableItemPtr> & curves)
    : UndoCommand()
    , _roto(roto)
    , _firstRedoCalled(false)
    , _curves()
{
    for (std::list<RotoDrawableItemPtr>::const_iterator it = curves.begin(); it != curves.end(); ++it) {
        RemovedCurve r;
        r.curve = *it;
        r.layer = std::dynamic_pointer_cast<RotoLayer>( roto->getContext()->getItemByName( (*it)->getParentLayer()->getScriptName() ) );
        assert(r.layer);
        r.indexInLayer = r.layer->getChildIndex(*it);
        assert(r.indexInLayer != -1);
        _curves.push_back(r);
    }
    setText( tr("Remove shape(s)").toStdString() );
}

RemoveCurveUndoCommand::~RemoveCurveUndoCommand()
{
}

void
RemoveCurveUndoCommand::undo()
{
    RotoPaintInteractPtr roto = _roto.lock();

    if (!roto) {
        return;
    }
    std::list<RotoDrawableItemPtr> selection;

    for (std::list<RemovedCurve>::iterator it = _curves.begin(); it != _curves.end(); ++it) {
        roto->getContext()->addItem(it->layer, it->indexInLayer, it->curve, RotoItem::eSelectionReasonOverlayInteract);
        BezierPtr isBezier = std::dynamic_pointer_cast<Bezier>(it->curve);
        if (isBezier) {
            selection.push_back(isBezier);
        }
    }

    SelectedCpList cpList;
    if ( !selection.empty() ) {
        roto->setSelection(selection, cpList);
    }
    roto->evaluate(true);
}

void
RemoveCurveUndoCommand::redo()
{
    RotoPaintInteractPtr roto = _roto.lock();

    if (!roto) {
        return;
    }

    for (std::list<RemovedCurve>::iterator it = _curves.begin(); it != _curves.end(); ++it) {
        roto->removeCurve(it->curve);
    }
    roto->evaluate(_firstRedoCalled);
    roto->setSelection( BezierPtr(), std::make_pair( CpPtr(), CpPtr() ) );
    _firstRedoCalled = true;
}

////////////////////////////////


AddStrokeUndoCommand::AddStrokeUndoCommand(const RotoPaintInteractPtr& roto,
                                           const RotoStrokeItemPtr& item)
    : UndoCommand()
    , _roto(roto)
    , _firstRedoCalled(false)
    , _item(item)
    , _layer( item->getParentLayer() )
    , _indexInLayer(_layer ? _layer->getChildIndex(_item) : -1)
{
    assert(_indexInLayer != -1);
    setText( tr("Paint Stroke").toStdString() );
}

AddStrokeUndoCommand::~AddStrokeUndoCommand()
{
}

void
AddStrokeUndoCommand::undo()
{
    RotoPaintInteractPtr roto = _roto.lock();

    if (!roto) {
        return;
    }

    roto->removeCurve(_item);
    roto->evaluate(true);
}

void
AddStrokeUndoCommand::redo()
{
    RotoPaintInteractPtr roto = _roto.lock();

    if (!roto) {
        return;
    }

    if (_firstRedoCalled) {
        roto->getContext()->addItem(_layer, _indexInLayer, _item, RotoItem::eSelectionReasonOverlayInteract);
    }
    if (_firstRedoCalled) {
        roto->evaluate(true);
    }
    _firstRedoCalled = true;
}

AddMultiStrokeUndoCommand::AddMultiStrokeUndoCommand(const RotoPaintInteractPtr& roto,
                                                     const RotoStrokeItemPtr& item)
    : UndoCommand()
    , _roto(roto)
    , _firstRedoCalled(false)
    , _item(item)
    , _layer( item->getParentLayer() )
    , _indexInLayer(_layer ? _layer->getChildIndex(_item) : -1)
    , isRemoved(false)
{
    assert(_indexInLayer != -1);
    setText( tr("Paint Stroke").toStdString() );
}

AddMultiStrokeUndoCommand::~AddMultiStrokeUndoCommand()
{
}

void
AddMultiStrokeUndoCommand::undo()
{
    RotoPaintInteractPtr roto = _roto.lock();

    if (!roto) {
        return;
    }

    if ( _item->removeLastStroke(&_xCurve, &_yCurve, &_pCurve) ) {
        roto->removeCurve(_item);
        isRemoved = true;
    }

    roto->evaluate(true);
}

void
AddMultiStrokeUndoCommand::redo()
{
    RotoPaintInteractPtr roto = _roto.lock();

    if (!roto) {
        return;
    }

    if (_firstRedoCalled) {
        if (_xCurve) {
            _item->addStroke(_xCurve, _yCurve, _pCurve);
        }
        if (isRemoved) {
            roto->getContext()->addItem(_layer, _indexInLayer, _item, RotoItem::eSelectionReasonOverlayInteract);
        }
        roto->evaluate(true);
    }

    _firstRedoCalled = true;
}

MoveTangentUndoCommand::MoveTangentUndoCommand(const RotoPaintInteractPtr& roto,
                                               double dx,
                                               double dy,
                                               double time,
                                               const BezierCPPtr & cp,
                                               bool left,
                                               bool breakTangents)
    : UndoCommand()
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
    BezierCPPtr counterPart;
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

    setText( tr("Move control point tangent").toStdString() );
}

MoveTangentUndoCommand::~MoveTangentUndoCommand()
{
}

NATRON_NAMESPACE_ANONYMOUS_ENTER

static void
getDeltaForPoint(const BezierCP& p,
                 const double time,
                 const Transform::Matrix3x3& transform,
                 const double dx,
                 const double dy,
                 const bool isLeft,
                 const bool breakTangents,
                 double* otherDiffX,
                 double* otherDiffY,
                 bool *isOnKeyframe)
{
    Transform::Point3D ltan, rtan, pos;

    ltan.z = rtan.z = pos.z = 1;
    *isOnKeyframe = p.getLeftBezierPointAtTime(true, time, ViewIdx(0), &ltan.x, &ltan.y);
    p.getRightBezierPointAtTime(true, time, ViewIdx(0), &rtan.x, &rtan.y);
    p.getPositionAtTime(true, time, ViewIdx(0), &pos.x, &pos.y);

    pos = Transform::matApply(transform, pos);
    ltan = Transform::matApply(transform, ltan);
    rtan = Transform::matApply(transform, rtan);

    double dist = isLeft ?  sqrt( (rtan.x - pos.x) * (rtan.x - pos.x) + (rtan.y - pos.y) * (rtan.y - pos.y) )
                  : sqrt( (ltan.x - pos.x) * (ltan.x - pos.x) + (ltan.y - pos.y) * (ltan.y - pos.y) );
    if (isLeft) {
        ltan.x += dx;
        ltan.y += dy;
    } else {
        rtan.x += dx;
        rtan.y += dy;
    }
    double alpha = isLeft ? std::atan2(pos.y - ltan.y, pos.x - ltan.x) : std::atan2(pos.y - rtan.y, pos.x - rtan.x);

    if (isLeft) {
        *otherDiffX = breakTangents ? 0 : pos.x + std::cos(alpha) * dist - rtan.x;
        *otherDiffY = breakTangents ? 0 : pos.y + std::sin(alpha) * dist - rtan.y;
    } else {
        *otherDiffX = breakTangents ? 0 : pos.x + std::cos(alpha) * dist - ltan.x;
        *otherDiffY = breakTangents ? 0 : pos.y + std::sin(alpha) * dist - ltan.y;
    }
}

static void
dragTangent(double time,
            BezierCP & cp,
            BezierCP & fp,
            const Transform::Matrix3x3& transform,
            double dx,
            double dy,
            bool both, //< should both tangents be moved symmetrically?
            bool left, //< which one is being controlled
            bool autoKeying,
            bool breakTangents,
            bool draggedPointIsFeather)
{
    bool isOnKeyframe;
    double otherDiffX, otherDiffY, otherFpDiffX, otherFpDiffY;

    getDeltaForPoint(cp, time, transform, dx, dy, left, breakTangents, &otherDiffX, &otherDiffY, &isOnKeyframe);
    getDeltaForPoint(fp, time, transform, dx, dy, left, breakTangents, &otherFpDiffX, &otherFpDiffY, &isOnKeyframe);

    if (autoKeying || isOnKeyframe) {
        if (both) {
            if (!left) {
                dx = -dx;
                dy = -dy;
            }
            cp.getBezier()->movePointLeftAndRightIndex(cp, fp, time, dx, dy, -dx, -dy, dx, dy, -dx, -dy, breakTangents, draggedPointIsFeather);
        } else {
            if (left) {
                cp.getBezier()->movePointLeftAndRightIndex(cp, fp, time, dx, dy, otherDiffX, otherDiffY, dx, dy, otherFpDiffX, otherFpDiffY, breakTangents, draggedPointIsFeather);
            } else {
                cp.getBezier()->movePointLeftAndRightIndex(cp, fp, time, otherDiffX, otherDiffY, dx, dy, otherFpDiffX, otherFpDiffY, dx, dy, breakTangents, draggedPointIsFeather);
            }
        }
    }
}

NATRON_NAMESPACE_ANONYMOUS_EXIT


void
MoveTangentUndoCommand::undo()
{
    RotoPaintInteractPtr roto = _roto.lock();

    if (!roto) {
        return;
    }

    BezierCPPtr counterPart;

    if ( _tangentBeingDragged->isFeatherPoint() ) {
        counterPart = _tangentBeingDragged->getBezier()->getControlPointForFeatherPoint(_tangentBeingDragged);
        if (counterPart) {
            counterPart->clone(*_oldCp);
        }
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
        roto->setSelection(_selectedCurves, _selectedPoints);
    }

    roto->evaluate(true);
}

void
MoveTangentUndoCommand::redo()
{
    RotoPaintInteractPtr roto = _roto.lock();

    if (!roto) {
        return;
    }

    BezierCPPtr cp, fp;

    if ( _tangentBeingDragged->isFeatherPoint() ) {
        cp = _tangentBeingDragged->getBezier()->getControlPointForFeatherPoint(_tangentBeingDragged);
        fp = _tangentBeingDragged;
        _oldCp->clone(*cp);
        _oldFp->clone(*fp);
    } else {
        cp = _tangentBeingDragged;
        fp = _tangentBeingDragged->getBezier()->getFeatherPointForControlPoint(_tangentBeingDragged);
        if (fp) {
            _oldFp->clone(*fp);
        }
        _oldCp->clone(*cp);
    }

    _tangentBeingDragged->getBezier()->incrementNodesAge();

    Transform::Matrix3x3 transform;
    _tangentBeingDragged->getBezier()->getTransformAtTime(_time, &transform);

    bool autoKeying = roto->getContext()->isAutoKeyingEnabled();


    dragTangent( _time, *cp, *fp, transform, _dx, _dy, false, _left, autoKeying, _breakTangents, _tangentBeingDragged->isFeatherPoint() );


    if (_firstRedoCalled) {
        roto->setSelection(_selectedCurves, _selectedPoints);
        roto->evaluate(true);
    } else {
        roto->computeSelectedCpsBBOX();
    }


    _firstRedoCalled = true;
}

bool
MoveTangentUndoCommand::mergeWith(const UndoCommandPtr &other)
{
    const MoveTangentUndoCommand* mvCmd = dynamic_cast<const MoveTangentUndoCommand*>( other.get() );

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


MoveFeatherBarUndoCommand::MoveFeatherBarUndoCommand(const RotoPaintInteractPtr& roto,
                                                     double dx,
                                                     double dy,
                                                     const std::pair<BezierCPPtr, BezierCPPtr> & point,
                                                     double time)
    : UndoCommand()
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
    _curve = std::dynamic_pointer_cast<Bezier>( roto->getContext()->getItemByName( point.first->getBezier()->getScriptName() ) );
    assert(_curve);
    _oldPoint.first.reset( new BezierCP(*_newPoint.first) );
    _oldPoint.second.reset( new BezierCP(*_newPoint.second) );
    setText( tr("Move control point feather bar").toStdString() );
}

MoveFeatherBarUndoCommand::~MoveFeatherBarUndoCommand()
{
}

void
MoveFeatherBarUndoCommand::undo()
{
    RotoPaintInteractPtr roto = _roto.lock();

    if (!roto) {
        return;
    }
    _newPoint.first->clone(*_oldPoint.first);
    _newPoint.second->clone(*_oldPoint.second);
    _newPoint.first->getBezier()->incrementNodesAge();
    roto->evaluate(true);
    roto->setSelection(_curve, _newPoint);
}

void
MoveFeatherBarUndoCommand::redo()
{
    RotoPaintInteractPtr roto = _roto.lock();

    if (!roto) {
        return;
    }
    BezierCPPtr p = _newPoint.first->isFeatherPoint() ?
                                    _newPoint.second : _newPoint.first;
    BezierCPPtr fp = _newPoint.first->isFeatherPoint() ?
                                     _newPoint.first : _newPoint.second;
    Point delta;
    Transform::Matrix3x3 transform;

    p->getBezier()->getTransformAtTime(_time, &transform);

    Transform::Point3D featherPoint, controlPoint;
    featherPoint.z = controlPoint.z = 1.;
    p->getPositionAtTime(true, _time, ViewIdx(0), &controlPoint.x, &controlPoint.y);
    bool isOnKeyframe = fp->getPositionAtTime(true, _time, ViewIdx(0), &featherPoint.x, &featherPoint.y);

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
        const std::list<BezierCPPtr> & cps = p->getBezier()->getFeatherPoints();
        assert(cps.size() > 1);

        std::list<BezierCPPtr>::const_iterator cur = std::find(cps.begin(), cps.end(), fp);
        if ( cur == cps.end() ) {
            return;
        }
        // compute previous and next element in the cyclic list
        std::list<BezierCPPtr>::const_iterator prev = cur;
        if ( prev == cps.begin() ) {
            prev = cps.end();
        }
        --prev; // the list has at least one element
        std::list<BezierCPPtr>::const_iterator next = cur;
        ++next; // the list has at least one element
        if ( next == cps.end() ) {
            next = cps.begin();
        }

        double leftX, leftY, rightX, rightY, norm;
        Bezier::leftDerivativeAtPoint(true, _time, **cur, **prev, transform, &leftX, &leftY);
        Bezier::rightDerivativeAtPoint(true, _time, **cur, **next, transform, &rightX, &rightY);
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

    if (roto->getContext()->isAutoKeyingEnabled() || isOnKeyframe) {
        int index = fp->getBezier()->getFeatherPointIndex(fp);
        fp->getBezier()->moveFeatherByIndex(index, _time, delta.x, delta.y);
    }

    if (_firstRedoCalled) {
        roto->evaluate(true);
    }

    roto->setSelection(_curve, _newPoint);

    _firstRedoCalled = true;
} // redo

bool
MoveFeatherBarUndoCommand::mergeWith(const UndoCommandPtr& other)
{
    const MoveFeatherBarUndoCommand* mvCmd = dynamic_cast<const MoveFeatherBarUndoCommand*>( other.get() );

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

RemoveFeatherUndoCommand::RemoveFeatherUndoCommand(const RotoPaintInteractPtr& roto,
                                                   const std::list<RemoveFeatherData> & datas)
    : UndoCommand()
    , _roto(roto)
    , _firstRedocalled(false)
    , _datas(datas)
{
    for (std::list<RemoveFeatherData>::iterator it = _datas.begin(); it != _datas.end(); ++it) {
        for (std::list<BezierCPPtr>::const_iterator it2 = it->newPoints.begin(); it2 != it->newPoints.end(); ++it2) {
            it->oldPoints.push_back( BezierCPPtr( new BezierCP(**it2) ) );
        }
    }
    setText( tr("Remove feather").toStdString() );
}

RemoveFeatherUndoCommand::~RemoveFeatherUndoCommand()
{
}

void
RemoveFeatherUndoCommand::undo()
{
    for (std::list<RemoveFeatherData>::iterator it = _datas.begin(); it != _datas.end(); ++it) {
        std::list<BezierCPPtr>::const_iterator itOld = it->oldPoints.begin();
        for (std::list<BezierCPPtr>::const_iterator itNew = it->newPoints.begin();
             itNew != it->newPoints.end(); ++itNew, ++itOld) {
            (*itNew)->clone(**itOld);
        }
        it->curve->incrementNodesAge();
    }
    RotoPaintInteractPtr roto = _roto.lock();
    if (!roto) {
        return;
    }
    roto->evaluate(true);
}

void
RemoveFeatherUndoCommand::redo()
{
    for (std::list<RemoveFeatherData>::iterator it = _datas.begin(); it != _datas.end(); ++it) {
        std::list<BezierCPPtr>::const_iterator itOld = it->oldPoints.begin();
        for (std::list<BezierCPPtr>::const_iterator itNew = it->newPoints.begin();
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
    RotoPaintInteractPtr roto = _roto.lock();
    if (!roto) {
        return;
    }

    roto->evaluate(_firstRedocalled);


    _firstRedocalled = true;
}

////////////////////////////


OpenCloseUndoCommand::OpenCloseUndoCommand(const RotoPaintInteractPtr& roto,
                                           const BezierPtr & curve)
    : UndoCommand()
    , _roto(roto)
    , _firstRedoCalled(false)
    , _selectedTool(roto->selectedToolAction )
    , _curve(curve)
{
    setText( tr("Open/Close bezier").toStdString() );
}

OpenCloseUndoCommand::~OpenCloseUndoCommand()
{
}

void
OpenCloseUndoCommand::undo()
{
    RotoPaintInteractPtr roto = _roto.lock();

    if (!roto) {
        return;
    }
    if (_firstRedoCalled) {
        roto->setCurrentTool( _selectedTool.lock() );
        if ( _selectedTool.lock() == roto->drawBezierAction.lock() ) {
            roto->setBuiltBezier(_curve);
        }
    }
    _curve->setCurveFinished( !_curve->isCurveFinished() );
    roto->evaluate(true);
    roto->setSelection( _curve, std::make_pair( CpPtr(), CpPtr() ) );
}

void
OpenCloseUndoCommand::redo()
{
    RotoPaintInteractPtr roto = _roto.lock();

    if (!roto) {
        return;
    }
    if (_firstRedoCalled) {
        roto->setCurrentTool( _selectedTool.lock() );
    }
    _curve->setCurveFinished( !_curve->isCurveFinished() );
    roto->evaluate(_firstRedoCalled);
    roto->setSelection( _curve, std::make_pair( CpPtr(), CpPtr() ) );
    _firstRedoCalled = true;
}

////////////////////////////

SmoothCuspUndoCommand::SmoothCuspUndoCommand(const RotoPaintInteractPtr& roto,
                                             const std::list<SmoothCuspCurveData> & data,
                                             double time,
                                             bool cusp,
                                             const std::pair<double, double>& pixelScale)
    : UndoCommand()
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
            BezierCPPtr firstCpy( new BezierCP(*(*it2).first) );
            BezierCPPtr secondCpy( new BezierCP(*(*it2).second) );
            it->oldPoints.push_back( std::make_pair(firstCpy, secondCpy) );
        }
    }
    if (_cusp) {
        setText( tr("Cusp points").toStdString() );
    } else {
        setText( tr("Smooth points").toStdString() );
    }
}

SmoothCuspUndoCommand::~SmoothCuspUndoCommand()
{
}

void
SmoothCuspUndoCommand::undo()
{
    RotoPaintInteractPtr roto = _roto.lock();

    if (!roto) {
        return;
    }
    for (std::list<SmoothCuspCurveData>::iterator it = curves.begin(); it != curves.end(); ++it) {
        SelectedPointList::const_iterator itOld = it->oldPoints.begin();
        for (SelectedPointList::const_iterator itNew = it->newPoints.begin();
             itNew != it->newPoints.end(); ++itNew, ++itOld) {
            itNew->first->clone(*itOld->first);
            itNew->second->clone(*itOld->second);
            it->curve->incrementNodesAge();
        }
    }

    roto->evaluate(true);
}

void
SmoothCuspUndoCommand::redo()
{
    RotoPaintInteractPtr roto = _roto.lock();

    if (!roto) {
        return;
    }
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

    roto->evaluate(_firstRedoCalled);

    _firstRedoCalled = true;
}

bool
SmoothCuspUndoCommand::mergeWith(const UndoCommandPtr& other)
{
    const SmoothCuspUndoCommand* sCmd = dynamic_cast<const SmoothCuspUndoCommand*>( other.get() );

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

MakeBezierUndoCommand::MakeBezierUndoCommand(const RotoPaintInteractPtr& roto,
                                             const BezierPtr & curve,
                                             bool isOpenBezier,
                                             bool createPoint,
                                             double dx,
                                             double dy,
                                             double time)
    : UndoCommand()
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
        _oldCurve.reset( new Bezier(_newCurve->getContext(), _newCurve->getScriptName(), _newCurve->getParentLayer(), false) );
        _oldCurve->clone( _newCurve.get() );
    }
    setText( tr("Draw Bezier").toStdString() );
}

MakeBezierUndoCommand::~MakeBezierUndoCommand()
{
}

void
MakeBezierUndoCommand::undo()
{
    RotoPaintInteractPtr roto = _roto.lock();

    if (!roto) {
        return;
    }

    assert(_createdPoint);
    roto->setCurrentTool( roto->drawBezierAction.lock() );
    assert(_lastPointAdded != -1);
    _oldCurve->clone( _newCurve.get() );
    if (_newCurve->getControlPointsCount() == 1) {
        _curveNonExistant = true;
        roto->removeCurve(_newCurve);
    }
    _newCurve->removeControlPointByIndex(_lastPointAdded);

    if (!_curveNonExistant) {
        roto->setSelection( _newCurve, std::make_pair( CpPtr(), CpPtr() ) );
        roto->setBuiltBezier(_newCurve);
    } else {
        roto->setSelection( BezierPtr(), std::make_pair( CpPtr(), CpPtr() ) );
    }
    roto->evaluate(true);
}

void
MakeBezierUndoCommand::redo()
{
    RotoPaintInteractPtr roto = _roto.lock();

    if (!roto) {
        return;
    }
    if (_firstRedoCalled) {
        roto->setCurrentTool( roto->drawBezierAction.lock() );
    }


    if (!_firstRedoCalled) {
        if (_createdPoint) {
            if (!_newCurve) {
                _newCurve = roto->getContext()->makeBezier(_x, _y, _isOpenBezier ? kRotoOpenBezierBaseName : kRotoBezierBaseName, _time, _isOpenBezier);
                assert(_newCurve);
                _oldCurve.reset( new Bezier(_newCurve->getContext(), _newCurve->getScriptName(), _newCurve->getParentLayer(), false) );
                _oldCurve->clone( _newCurve.get() );
                _lastPointAdded = 0;
                _curveNonExistant = false;
            } else {
                _oldCurve->clone( _newCurve.get() );
                _newCurve->addControlPoint(_x, _y, _time);
                int lastIndex = _newCurve->getControlPointsCount() - 1;
                assert(lastIndex > 0);
                _lastPointAdded = lastIndex;
            }
        } else {
            assert(_newCurve);
            _oldCurve->clone( _newCurve.get() );
            int lastIndex = _newCurve->getControlPointsCount() - 1;
            assert(lastIndex >= 0);
            _lastPointAdded = lastIndex;
            Transform::Matrix3x3 transform;
            _newCurve->getTransformAtTime(_time, &transform);
            BezierCPPtr cp = _newCurve->getControlPointAtIndex(lastIndex);
            BezierCPPtr fp = _newCurve->getFeatherPointAtIndex(lastIndex);

            dragTangent(_time, *cp, *fp, transform, _dx, _dy, true, false, false, false, false);
        }

        RotoItemPtr parentItem;
        if ( _newCurve->getParentLayer() ) {
            parentItem =  roto->getContext()->getItemByName( _newCurve->getParentLayer()->getScriptName() );
        }
        if (parentItem) {
            _parentLayer = std::dynamic_pointer_cast<RotoLayer>(parentItem);
            _indexInLayer = _parentLayer->getChildIndex(_newCurve);
        }
    } else {
        _newCurve->clone( _oldCurve.get() );
        if (_curveNonExistant) {
            roto->getContext()->addItem(_parentLayer, _indexInLayer, _newCurve, RotoItem::eSelectionReasonOverlayInteract);
        }
    }


    BezierCPPtr cp = _newCurve->getControlPointAtIndex(_lastPointAdded);
    BezierCPPtr fp = _newCurve->getFeatherPointAtIndex(_lastPointAdded);
    roto->setSelection( _newCurve, std::make_pair(cp, fp) );
    roto->autoSaveAndRedraw();
    _firstRedoCalled = true;
} // redo

bool
MakeBezierUndoCommand::mergeWith(const UndoCommandPtr& other)
{
    const MakeBezierUndoCommand* sCmd = dynamic_cast<const MakeBezierUndoCommand*>( other.get() );

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


MakeEllipseUndoCommand::MakeEllipseUndoCommand(const RotoPaintInteractPtr& roto,
                                               bool create,
                                               bool fromCenter,
                                               bool constrained,
                                               double fromx,
                                               double fromy,
                                               double tox,
                                               double toy,
                                               double time)
    : UndoCommand()
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
        _curve = roto->getBezierBeingBuild();
    }
    setText( tr("Draw Ellipse").toStdString() );
}

MakeEllipseUndoCommand::~MakeEllipseUndoCommand()
{
}

void
MakeEllipseUndoCommand::undo()
{
    RotoPaintInteractPtr roto = _roto.lock();

    if (!roto) {
        return;
    }
    roto->removeCurve(_curve);
    roto->evaluate(true);
    roto->setSelection( BezierPtr(), std::make_pair( CpPtr(), CpPtr() ) );
}

void
MakeEllipseUndoCommand::redo()
{
    RotoPaintInteractPtr roto = _roto.lock();

    if (!roto) {
        return;
    }
    if (_firstRedoCalled) {
        roto->getContext()->addItem(_parentLayer, _indexInLayer, _curve, RotoItem::eSelectionReasonOverlayInteract);
        roto->evaluate(true);
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
            _curve = roto->getContext()->makeBezier(xmid, ytop, kRotoEllipseBaseName, _time, false); //top
            assert(_curve);
            _curve->addControlPoint(xright, ymid, _time); // right
            _curve->addControlPoint(xmid, ybottom, _time); // bottom
            _curve->addControlPoint(xleft, ymid, _time); //left
            _curve->setCurveFinished(true);
        } else {
            _curve->setPointByIndex(0, _time, xmid, ytop); // top
            _curve->setPointByIndex(1, _time, xright, ymid); // right
            _curve->setPointByIndex(2, _time, xmid, ybottom); // bottom
            _curve->setPointByIndex(3, _time, xleft, ymid); // left
        }
        BezierCPPtr top = _curve->getControlPointAtIndex(0);
        BezierCPPtr right = _curve->getControlPointAtIndex(1);
        BezierCPPtr bottom = _curve->getControlPointAtIndex(2);
        BezierCPPtr left = _curve->getControlPointAtIndex(3);
        double topX, topY, rightX, rightY, btmX, btmY, leftX, leftY;
        top->getPositionAtTime(true, _time, ViewIdx(0), &topX, &topY);
        right->getPositionAtTime(true, _time, ViewIdx(0), &rightX, &rightY);
        bottom->getPositionAtTime(true, _time, ViewIdx(0), &btmX, &btmY);
        left->getPositionAtTime(true, _time, ViewIdx(0), &leftX, &leftY);

        // The bezier control points should be:
        // P_0 = (0,1), P_1 = (c,1), P_2 = (1,c), P_3 = (1,0)
        // with c = 0.551915024494
        // See http://spencermortensen.com/articles/bezier-circle/

        const double c = 0.551915024494;
        // top
        _curve->setLeftBezierPoint(0, _time,  topX + (leftX  - topX) * c, topY);
        _curve->setRightBezierPoint(0, _time, topX + (rightX - topX) * c, topY);

        // right
        _curve->setLeftBezierPoint(1, _time,  rightX, rightY + (topY - rightY) * c);
        _curve->setRightBezierPoint(1, _time, rightX, rightY + (btmY - rightY) * c);

        // btm
        _curve->setLeftBezierPoint(2, _time,  btmX + (rightX - btmX) * c, btmY);
        _curve->setRightBezierPoint(2, _time, btmX + (leftX  - btmX) * c, btmY);

        // left
        _curve->setLeftBezierPoint(3, _time,  leftX, leftY + (btmY - leftY) * c);
        _curve->setRightBezierPoint(3, _time, leftX, leftY + (topY - leftY) * c);

        RotoItemPtr parentItem =  roto->getContext()->getItemByName( _curve->getParentLayer()->getScriptName() );
        if (parentItem) {
            _parentLayer = std::dynamic_pointer_cast<RotoLayer>(parentItem);
            _indexInLayer = _parentLayer->getChildIndex(_curve);
        }
    }
    roto->setBuiltBezier(_curve);
    _firstRedoCalled = true;
    roto->setSelection( _curve, std::make_pair( CpPtr(), CpPtr() ) );
} // redo

bool
MakeEllipseUndoCommand::mergeWith(const UndoCommandPtr &other)
{
    const MakeEllipseUndoCommand* sCmd = dynamic_cast<const MakeEllipseUndoCommand*>( other.get() );

    if (!sCmd) {
        return false;
    }
    if ( (sCmd->_curve != _curve) || sCmd->_create ) {
        return false;
    }
    if (_curve != sCmd->_curve) {
        _curve->clone( sCmd->_curve.get() );
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


MakeRectangleUndoCommand::MakeRectangleUndoCommand(const RotoPaintInteractPtr& roto,
                                                   bool create,
                                                   bool fromCenter,
                                                   bool constrained,
                                                   double fromx,
                                                   double fromy,
                                                   double tox,
                                                   double toy,
                                                   double time)
    : UndoCommand()
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
        _curve = roto->getBezierBeingBuild();
    }
    setText( tr("Draw Rectangle").toStdString() );
}

MakeRectangleUndoCommand::~MakeRectangleUndoCommand()
{
}

void
MakeRectangleUndoCommand::undo()
{
    RotoPaintInteractPtr roto = _roto.lock();

    if (!roto) {
        return;
    }
    roto->removeCurve(_curve);
    roto->evaluate(true);
    roto->setSelection( BezierPtr(), std::make_pair( CpPtr(), CpPtr() ) );
}

void
MakeRectangleUndoCommand::redo()
{
    RotoPaintInteractPtr roto = _roto.lock();

    if (!roto) {
        return;
    }
    if (_firstRedoCalled) {
        roto->getContext()->addItem(_parentLayer, _indexInLayer, _curve, RotoItem::eSelectionReasonOverlayInteract);
        roto->evaluate(true);
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
            _curve = roto->getContext()->makeBezier(xleft, ytop, kRotoRectangleBaseName, _time, false); //topleft
            assert(_curve);
            _curve->addControlPoint(xright, ytop, _time); // topright
            _curve->addControlPoint(xright, ybottom, _time); // bottomright
            _curve->addControlPoint(xleft, ybottom, _time); // bottomleft
            _curve->setCurveFinished(true);
        } else {
            _curve->setPointByIndex(0, _time, xleft, ytop); // topleft
            _curve->setPointByIndex(1, _time, xright, ytop); // topright
            _curve->setPointByIndex(2, _time, xright, ybottom); // bottomright
            _curve->setPointByIndex(3, _time, xleft, ybottom); // bottomleft
        }
        RotoItemPtr parentItem =  roto->getContext()->getItemByName( _curve->getParentLayer()->getScriptName() );
        if (parentItem) {
            _parentLayer = std::dynamic_pointer_cast<RotoLayer>(parentItem);
            _indexInLayer = _parentLayer->getChildIndex(_curve);
        }
    }
    roto->setBuiltBezier(_curve);
    _firstRedoCalled = true;
    roto->setSelection( _curve, std::make_pair( CpPtr(), CpPtr() ) );
} // MakeRectangleUndoCommand::redo

bool
MakeRectangleUndoCommand::mergeWith(const UndoCommandPtr &other)
{
    const MakeRectangleUndoCommand* sCmd = dynamic_cast<const MakeRectangleUndoCommand*>( other.get() );

    if (!sCmd) {
        return false;
    }
    if ( (sCmd->_curve != _curve) || sCmd->_create ) {
        return false;
    }
    if (_curve != sCmd->_curve) {
        _curve->clone( sCmd->_curve.get() );
    }
    _fromx = sCmd->_fromx;
    _fromy = sCmd->_fromy;
    _tox = sCmd->_tox;
    _toy = sCmd->_toy;
    _fromCenter = sCmd->_fromCenter;
    _constrained = sCmd->_constrained;

    return true;
}

NATRON_NAMESPACE_EXIT
