//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#include "CurveEditorUndoRedo.h"

#include <QDebug>

#include "Global/GlobalDefines.h"

#include "Gui/CurveEditor.h"
#include "Gui/KnobGui.h"
#include "Gui/CurveWidget.h"

#include "Engine/Knob.h"
#include "Engine/Curve.h"
#include "Engine/KnobTypes.h"


//////////////////////////////ADD MULTIPLE KEYS COMMAND//////////////////////////////////////////////
AddKeysCommand::AddKeysCommand(CurveWidget *editor,
                               const KeysToAddList& keys,
                               QUndoCommand *parent)
    : QUndoCommand(parent)
      , _keys(keys)
      , _curveWidget(editor)
{
}

AddKeysCommand::AddKeysCommand(CurveWidget *editor,
               CurveGui* curve,
               const std::vector<KeyFrame>& keys,
               QUndoCommand *parent)
    : QUndoCommand(parent)
    , _keys()
    , _curveWidget(editor)
{
    boost::shared_ptr<KeysForCurve> k(new KeysForCurve);
    k->curve = curve;
    k->keys = keys;
    _keys.push_back(k);
}

void
AddKeysCommand::addOrRemoveKeyframe(bool add)
{
    KeysToAddList::iterator next = _keys.begin();
    ++next;
    for (KeysToAddList::iterator it = _keys.begin(); it != _keys.end(); ++it,++next) {
        boost::shared_ptr<Parametric_Knob> isParametric = boost::dynamic_pointer_cast<Parametric_Knob>( (*it)->curve->getKnob()->getKnob() );
        (*it)->curve->getKnob()->getKnob()->blockEvaluation();
        bool isUnblocked = false;
        assert( !(*it)->keys.empty() );
        for (U32 i = 0; i < (*it)->keys.size(); ++i) {
            
            if ((i == (*it)->keys.size() - 1) && next == _keys.end()) {
                (*it)->curve->getKnob()->getKnob()->unblockEvaluation();
                isUnblocked = true;
            }
            
            if (add) {
                
                if (isParametric) {
                    Natron::Status st = isParametric->addControlPoint( (*it)->curve->getDimension(),
                                                                      (*it)->keys[i].getTime(),
                                                                      (*it)->keys[i].getValue() );
                    assert(st == Natron::StatOK);
                    (void)st;
                } else {
                    (*it)->curve->getKnob()->setKeyframe( (*it)->keys[i].getTime(), (*it)->curve->getDimension() );
                }
                
            } else {
                
                if (isParametric) {
                    Natron::Status st = isParametric->deleteControlPoint( (*it)->curve->getDimension(),
                                                                         (*it)->curve->getInternalCurve()->keyFrameIndex(
                                                                                                (*it)->keys[i].getTime() ) );
                    assert(st == Natron::StatOK);
                    (void)st;
                } else {
                    (*it)->curve->getKnob()->removeKeyFrame( (*it)->keys[i].getTime(), (*it)->curve->getDimension() );
                }
                
            }
            
        }
        if (next == _keys.end()) {
            --next;
        }
        if (!isUnblocked) {
            (*it)->curve->getKnob()->getKnob()->unblockEvaluation();
        }

    }
    
    _curveWidget->update();

    setText( QObject::tr("Add multiple keyframes") );
}

void
AddKeysCommand::undo()
{
    addOrRemoveKeyframe(false);
}

void
AddKeysCommand::redo()
{
    addOrRemoveKeyframe(true);
}

//////////////////////////////REMOVE  MULTIPLE KEYS COMMAND//////////////////////////////////////////////
RemoveKeysCommand::RemoveKeysCommand(CurveWidget* editor,
                                     const std::vector<std::pair<CurveGui*,KeyFrame> > & keys,
                                     QUndoCommand *parent )
    : QUndoCommand(parent)
      , _keys(keys)
      , _curveWidget(editor)
{
}

void
RemoveKeysCommand::addOrRemoveKeyframe(bool add)
{
 
    for (U32 i = 0; i < _keys.size(); ++i) {
        
        bool hasBlocked = false;
        
        if (i != _keys.size() - 1) {
            _keys[i].first->getKnob()->getKnob()->blockEvaluation();
            hasBlocked = true;
        }
        
        if (add) {
            if ( _keys[i].first->getKnob()->getKnob()->typeName() == Parametric_Knob::typeNameStatic() ) {
                boost::shared_ptr<Parametric_Knob> knob = boost::dynamic_pointer_cast<Parametric_Knob>( _keys[i].first->getKnob()->getKnob() );
                Natron::Status st = knob->addControlPoint( _keys[i].first->getDimension(), _keys[i].second.getTime(),_keys[i].second.getValue() );
                assert(st == Natron::StatOK);
                (void) st;
            } else {
                _keys[i].first->getKnob()->setKeyframe( _keys[i].second.getTime(), _keys[i].first->getDimension() );
            }
        } else {
            if ( _keys[i].first->getKnob()->getKnob()->typeName() == Parametric_Knob::typeNameStatic() ) {
                boost::shared_ptr<Parametric_Knob> knob = boost::dynamic_pointer_cast<Parametric_Knob>( _keys[i].first->getKnob()->getKnob() );
                Natron::Status st = knob->deleteControlPoint( _keys[i].first->getDimension(),
                                                              _keys[i].first->getInternalCurve()->keyFrameIndex( _keys[i].second.getTime() ) );
                assert(st == Natron::StatOK);
                (void)st;
            } else {
                _keys[i].first->getKnob()->removeKeyFrame( _keys[i].second.getTime(), _keys[i].first->getDimension() );
            }
        }
        
        if (hasBlocked) {
            _keys[i].first->getKnob()->getKnob()->unblockEvaluation();
        }
    }

    _curveWidget->update();
    setText( QObject::tr("Remove multiple keyframes") );
}

void
RemoveKeysCommand::undo()
{
    addOrRemoveKeyframe(true);
}

void
RemoveKeysCommand::redo()
{
    addOrRemoveKeyframe(false);
}

namespace  {
static bool
selectedKeyLessFunctor(const KeyPtr & lhs,
                       const KeyPtr & rhs)
{
    return lhs->key.getTime() < rhs->key.getTime();
}
}

//////////////////////////////MOVE MULTIPLE KEYS COMMAND//////////////////////////////////////////////
MoveKeysCommand::MoveKeysCommand(CurveWidget* widget,
                                 const SelectedKeys &keys,
                                 double dt,
                                 double dv,
                                 bool updateOnFirstRedo,
                                 QUndoCommand *parent )
    : QUndoCommand(parent)
      , _firstRedoCalled(false)
      , _updateOnFirstRedo(updateOnFirstRedo)
      , _dt(dt)
      , _dv(dv)
      , _keys(keys)
      , _widget(widget)
{
    ///sort keys by increasing time
    _keys.sort(selectedKeyLessFunctor);
}

static void
moveKey(KeyPtr &k,
        double dt,
        double dv)
{
    std::pair<double,double> curveYRange = k->curve->getInternalCurve()->getCurveYRange();
    double newX = k->key.getTime() + dt;
    double newY = k->key.getValue() + dv;
    boost::shared_ptr<Curve> curve = k->curve->getInternalCurve();

    if ( curve->areKeyFramesValuesClampedToIntegers() ) {
        newY = std::floor(newY + 0.5);
    } else if ( curve->areKeyFramesValuesClampedToBooleans() ) {
        newY = newY < 0.5 ? 0 : 1;
    }

    if (newY > curveYRange.second) {
        newY = k->key.getValue();
    } else if (newY < curveYRange.first) {
        newY = k->key.getValue();
    }

    double oldTime = k->key.getTime();
    int keyframeIndex = curve->keyFrameIndex(oldTime);
    int newIndex;

    k->key = curve->setKeyFrameValueAndTime(newX,newY, keyframeIndex, &newIndex);
    k->curve->getKnob()->onKeyFrameMoved( oldTime, k->key.getTime() );
}

void
MoveKeysCommand::move(double dt,
                      double dv)
{
    std::list<KnobI*> differentKnobs;

    for (SelectedKeys::iterator it = _keys.begin(); it != _keys.end(); ++it) {
        KnobI* k = (*it)->curve->getKnob()->getKnob().get();
        if ( std::find(differentKnobs.begin(), differentKnobs.end(), k) == differentKnobs.end() ) {
            differentKnobs.push_back(k);
            k->blockEvaluation();
        }
    }

    SelectedKeys newSelectedKeys;
    try {
        if (dt <= 0) {
            for (SelectedKeys::iterator it = _keys.begin(); it != _keys.end(); ++it) {
                moveKey(*it, dt, dv);
            }
        } else {
            for (SelectedKeys::reverse_iterator it = _keys.rbegin(); it != _keys.rend(); ++it) {
                moveKey(*it, dt, dv);
            }
        }
    } catch (const std::exception & e) {
        qDebug() << "The keyframe set has changed since this action. This is probably because another user interaction is not "
            "linked to undo/redo stack.";
    }

    for (std::list<KnobI*>::iterator it = differentKnobs.begin(); it != differentKnobs.end(); ++it) {
        (*it)->unblockEvaluation();
        if (_firstRedoCalled || _updateOnFirstRedo) {
            (*it)->getHolder()->evaluate_public(*it, true, Natron::USER_EDITED);
        }
    }

    _widget->refreshSelectedKeys();
}

void
MoveKeysCommand::undo()
{
    move(-_dt,-_dv);
    setText( QObject::tr("Move multiple keys") );
}

void
MoveKeysCommand::redo()
{
    move(_dt,_dv);
    _firstRedoCalled = true;
    setText( QObject::tr("Move multiple keys") );
}

bool
MoveKeysCommand::mergeWith(const QUndoCommand * command)
{
    const MoveKeysCommand* cmd = dynamic_cast<const MoveKeysCommand*>(command);

    if ( cmd && ( cmd->id() == id() ) ) {
        if ( cmd->_keys.size() != _keys.size() ) {
            return false;
        }

        SelectedKeys::const_iterator itother = cmd->_keys.begin();
        for (SelectedKeys::const_iterator it = _keys.begin(); it != _keys.end(); ++it,++itother) {
            if (*itother != *it) {
                return false;
            }
        }

        _dt += cmd->_dt;
        _dv += cmd->_dv;

        return true;
    } else {
        return false;
    }
}

int
MoveKeysCommand::id() const
{
    return kCurveEditorMoveMultipleKeysCommandCompressionID;
}

//////////////////////////////SET MULTIPLE KEYS INTERPOLATION COMMAND//////////////////////////////////////////////
SetKeysInterpolationCommand::SetKeysInterpolationCommand(CurveWidget* widget,
                                                         const std::list< KeyInterpolationChange > & keys,
                                                         QUndoCommand *parent)
    : QUndoCommand(parent)
      , _keys(keys)
      , _widget(widget)
{
}

void
SetKeysInterpolationCommand::setNewInterpolation(bool undo)
{
    std::list<KnobI*> differentKnobs;

    for (std::list< KeyInterpolationChange >::iterator it = _keys.begin(); it != _keys.end(); ++it) {
        KnobI* k = it->key->curve->getKnob()->getKnob().get();
        if ( std::find(differentKnobs.begin(), differentKnobs.end(), k) == differentKnobs.end() ) {
            differentKnobs.push_back(k);
            k->blockEvaluation();
        }
    }

    for (std::list< KeyInterpolationChange >::iterator it = _keys.begin(); it != _keys.end(); ++it) {
        int keyframeIndex = it->key->curve->getInternalCurve()->keyFrameIndex( it->key->key.getTime() );
        if (undo) {
            it->key->key =  it->key->curve->getInternalCurve()->setKeyFrameInterpolation(it->oldInterp, keyframeIndex);
        } else {
            it->key->key = it->key->curve->getInternalCurve()->setKeyFrameInterpolation(it->newInterp, keyframeIndex);
        }
    }

    for (std::list<KnobI*>::iterator it = differentKnobs.begin(); it != differentKnobs.end(); ++it) {
        (*it)->unblockEvaluation();
        (*it)->getHolder()->evaluate_public(*it, true, Natron::USER_EDITED);
    }

    _widget->refreshSelectedKeys();
    setText( QObject::tr("Set multiple keys interpolation") );
}

void
SetKeysInterpolationCommand::undo()
{
    setNewInterpolation(true);
}

void
SetKeysInterpolationCommand::redo()
{
    setNewInterpolation(false);
}

