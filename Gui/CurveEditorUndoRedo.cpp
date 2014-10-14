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

#include <cmath>
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
                               const KeysToAddList & keys,
                               QUndoCommand *parent)
    : QUndoCommand(parent)
      , _keys(keys)
      , _curveWidget(editor)
{
}

AddKeysCommand::AddKeysCommand(CurveWidget *editor,
                               CurveGui* curve,
                               const std::vector<KeyFrame> & keys,
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
            if ( (i == (*it)->keys.size() - 1) && ( next == _keys.end() ) ) {
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
        if ( next == _keys.end() ) {
            --next;
        }
        if (!isUnblocked) {
            (*it)->curve->getKnob()->getKnob()->unblockEvaluation();
        }
    }

    _curveWidget->update();

    setText( QObject::tr("Add multiple keyframes") );
} // addOrRemoveKeyframe

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
                KnobGui* guiKnob = _keys[i].first->getKnob();
                guiKnob->setKeyframe( _keys[i].second.getTime(),_keys[i].second, _keys[i].first->getDimension() );

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
    boost::shared_ptr<KnobI> knob = k->curve->getKnob()->getKnob();
    Parametric_Knob* isParametric = dynamic_cast<Parametric_Knob*>(knob.get());
    
    if (isParametric) {
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
    } else {
        k->curve->getKnob()->getKnob()->moveValueAtTime(k->key.getTime(), k->curve->getDimension(), dt, dv,&k->key);
    }
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
    
    if (dt <= 0) {
        for (SelectedKeys::iterator it = _keys.begin(); it != _keys.end(); ++it) {
            moveKey(*it, dt, dv);
        }
    } else {
        for (SelectedKeys::reverse_iterator it = _keys.rbegin(); it != _keys.rend(); ++it) {
            moveKey(*it, dt, dv);
        }
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
        
        Natron::KeyframeType interp = undo ? it->oldInterp : it->newInterp;
        
        boost::shared_ptr<KnobI> knob = it->key->curve->getKnob()->getKnob();
        Parametric_Knob* isParametric = dynamic_cast<Parametric_Knob*>(knob.get());
        
        if (isParametric) {
            
            int keyframeIndex = it->key->curve->getInternalCurve()->keyFrameIndex( it->key->key.getTime() );
            if (keyframeIndex != -1) {
                it->key->key =  it->key->curve->getInternalCurve()->setKeyFrameInterpolation(interp, keyframeIndex);
            }
            
        } else {
            it->key->curve->getKnob()->getKnob()->setInterpolationAtTime(it->key->curve->getDimension(), it->key->key.getTime(), interp, &it->key->key);
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

/////////////////////////// MoveTangentCommand

MoveTangentCommand::MoveTangentCommand(CurveWidget* widget,
                                       SelectedDerivative deriv,
                                       const KeyPtr& key,
                                       double dx,double dy, //< dx dy relative to the center of the keyframe
                                       bool updateOnFirstRedo,
                                       QUndoCommand *parent)
: QUndoCommand(parent)
, _widget(widget)
, _key(key)
, _deriv(deriv)
, _oldInterp(key->key.getInterpolation())
, _oldLeft(key->key.getLeftDerivative())
, _oldRight(key->key.getRightDerivative())
, _setBoth(false)
, _updateOnFirstRedo(updateOnFirstRedo)
, _firstRedoCalled(false)
{
    KeyFrameSet keys = key->curve->getInternalCurve()->getKeyFrames_mt_safe();
    KeyFrameSet::const_iterator cur = keys.find(key->key);
    assert( cur != keys.end() );

    //find next and previous keyframes
    KeyFrameSet::const_iterator prev = cur;
    if ( cur != keys.begin() ) {
        --prev;
    } else {
        prev = keys.end();
    }
    KeyFrameSet::const_iterator next = cur;
    ++next;
    
    // handle first and last keyframe correctly:
    // - if their interpolation was KEYFRAME_CATMULL_ROM or KEYFRAME_CUBIC, then it becomes KEYFRAME_FREE
    // - in all other cases it becomes KEYFRAME_BROKEN
    Natron::KeyframeType interp = key->key.getInterpolation();
    bool keyframeIsFirstOrLast = ( prev == keys.end() || next == keys.end() );
    bool interpIsNotBroken = (interp != Natron::KEYFRAME_BROKEN);
    bool interpIsCatmullRomOrCubicOrFree = (interp == Natron::KEYFRAME_CATMULL_ROM ||
                                            interp == Natron::KEYFRAME_CUBIC ||
                                            interp == Natron::KEYFRAME_FREE);
    _setBoth = keyframeIsFirstOrLast ? interpIsCatmullRomOrCubicOrFree : interpIsNotBroken;

    bool isLeft;
    if (deriv == LEFT_TANGENT) {
        //if dx is not of the good sign it would make the curve uncontrollable
        if (dx < 0) {
            dx = 0.0001;
        }
        isLeft = true;
    } else {
        //if dx is not of the good sign it would make the curve uncontrollable
        if (dx > 0) {
            dx = -0.0001;
        }
        isLeft = false;
    }
    double derivative = dy / dx;
    
    if (_setBoth) {
        _newInterp = Natron::KEYFRAME_FREE;
        _newLeft = derivative;
        _newRight = derivative;
    } else {
        if (isLeft) {
            _newLeft = derivative;
            _newRight = _oldRight;
        } else {
            _newLeft = _oldLeft;
            _newRight = derivative;
        }
        _newInterp = Natron::KEYFRAME_BROKEN;
    }
}


void
MoveTangentCommand::setNewDerivatives(bool undo)
{
    boost::shared_ptr<KnobI> attachedKnob = _key->curve->getKnob()->getKnob();
    assert(attachedKnob);
    Parametric_Knob* isParametric = dynamic_cast<Parametric_Knob*>(attachedKnob.get());
    
    
    double left = undo ? _oldLeft : _newLeft;
    double right = undo ? _oldRight : _newRight;
    Natron::KeyframeType interp = undo ? _oldInterp : _newInterp;
    
    if (!isParametric) {
        attachedKnob->blockEvaluation();
        if (_setBoth) {
            attachedKnob->moveDerivativesAtTime(_key->curve->getDimension(), _key->key.getTime(), left, right);
        } else {
            attachedKnob->moveDerivativeAtTime(_key->curve->getDimension(), _key->key.getTime(),
                                                                        _deriv == LEFT_TANGENT ? left : right,
                                                                        _deriv == LEFT_TANGENT);
            
        }
        attachedKnob->setInterpolationAtTime(_key->curve->getDimension(), _key->key.getTime(), interp, &_key->key);
        attachedKnob->unblockEvaluation();
    } else {
        int keyframeIndexInCurve = _key->curve->getInternalCurve()->keyFrameIndex( _key->key.getTime() );
        _key->key = _key->curve->getInternalCurve()->setKeyFrameInterpolation(interp, keyframeIndexInCurve);
        _key->key = _key->curve->getInternalCurve()->setKeyFrameDerivatives(left, right,keyframeIndexInCurve);
    }
    
    if (_firstRedoCalled || _updateOnFirstRedo) {
        attachedKnob->evaluateValueChange(_key->curve->getDimension(), Natron::USER_EDITED);
    }
    _widget->refreshDisplayedTangents();
    
}


void
MoveTangentCommand::undo()
{
    setNewDerivatives(true);
    setText( QObject::tr("Move keyframe slope") );
}


void
MoveTangentCommand::redo()
{
    setNewDerivatives(false);
    _firstRedoCalled = true;
    setText( QObject::tr("Move keyframe slope") );
}

int
MoveTangentCommand::id() const
{
    return kCurveEditorMoveTangentsCommandCompressionID;
}

bool
MoveTangentCommand::mergeWith(const QUndoCommand * command)
{
    const MoveTangentCommand* cmd = dynamic_cast<const MoveTangentCommand*>(command);
    
    if ( cmd && ( cmd->id() == id() ) ) {
        if ( cmd->_key != _key ) {
            return false;
        }
        
        
        _newInterp = cmd->_newInterp;
        _newLeft = cmd->_newLeft;
        _newRight = cmd->_newRight;
        
        return true;
    } else {
        return false;
    }
}
