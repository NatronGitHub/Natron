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
AddKeysCommand::AddKeysCommand(CurveWidget *editor, CurveGui* curve,
                               const std::vector< KeyFrame >& keys,
                               QUndoCommand *parent)
: QUndoCommand(parent)
, _curve(curve)
, _keys(keys)
, _curveWidget(editor)
{
}

void AddKeysCommand::addOrRemoveKeyframe(bool add) {
    boost::shared_ptr<Parametric_Knob> isParametric = boost::dynamic_pointer_cast<Parametric_Knob>(_curve->getKnob()->getKnob());
    
    for(U32 i = 0 ; i < _keys.size();++i){
        _curve->getKnob()->getKnob()->beginValueChange(Natron::USER_EDITED);
        if(add){
            if (isParametric) {
                Natron::Status st = isParametric->addControlPoint(_curve->getDimension(), _keys[i].getTime(),_keys[i].getValue());
                assert(st == Natron::StatOK);
            }else{
                _curve->getKnob()->setKeyframe(_keys[i].getTime(), _curve->getDimension());
            }
        }else{
            if (isParametric) {
                Natron::Status st = isParametric->deleteControlPoint(_curve->getDimension(),
                                                                     _curve->getInternalCurve()->keyFrameIndex(_keys[i].getTime()));
                assert(st == Natron::StatOK);
            }else{
                _curve->getKnob()->removeKeyFrame(_keys[i].getTime(), _curve->getDimension());
            }
        }
    }
    for(U32 i = 0 ; i < _keys.size();++i){
        _curve->getKnob()->getKnob()->endValueChange();
    }
    
    _curveWidget->update();
    
    setText(QObject::tr("Add multiple keyframes"));
}

void AddKeysCommand::undo(){
    addOrRemoveKeyframe(false);
}

void AddKeysCommand::redo(){
    addOrRemoveKeyframe(true);
}

//////////////////////////////REMOVE  MULTIPLE KEYS COMMAND//////////////////////////////////////////////
RemoveKeysCommand::RemoveKeysCommand(CurveWidget* editor,const std::vector<std::pair<CurveGui*,KeyFrame> >& keys,QUndoCommand *parent )
: QUndoCommand(parent)
, _keys(keys)
, _curveWidget(editor)
{
}

void RemoveKeysCommand::addOrRemoveKeyframe(bool add){
    ///this can be called either by a parametric curve widget or by the global curve editor, so we need to handle the different
    ///cases here. Maybe this is bad design, i don't know how we could change it otherwise.
    
    for(U32 i = 0 ; i < _keys.size();++i){
        _keys[i].first->getKnob()->getKnob()->beginValueChange(Natron::USER_EDITED);
        if(add){
            if (_keys[i].first->getKnob()->getKnob()->typeName() == Parametric_Knob::typeNameStatic()) {
                boost::shared_ptr<Parametric_Knob> knob = boost::dynamic_pointer_cast<Parametric_Knob>(_keys[i].first->getKnob()->getKnob());
                Natron::Status st = knob->addControlPoint(_keys[i].first->getDimension(), _keys[i].second.getTime(),_keys[i].second.getValue());
                assert(st == Natron::StatOK);
            }else{
                _keys[i].first->getKnob()->setKeyframe(_keys[i].second.getTime(), _keys[i].first->getDimension());
            }
        }else{
            if (_keys[i].first->getKnob()->getKnob()->typeName() == Parametric_Knob::typeNameStatic()) {
                boost::shared_ptr<Parametric_Knob> knob = boost::dynamic_pointer_cast<Parametric_Knob>(_keys[i].first->getKnob()->getKnob());
                Natron::Status st = knob->deleteControlPoint(_keys[i].first->getDimension(),
                                                             _keys[i].first->getInternalCurve()->keyFrameIndex(_keys[i].second.getTime()));
                assert(st == Natron::StatOK);
            }else{
                _keys[i].first->getKnob()->removeKeyFrame(_keys[i].second.getTime(), _keys[i].first->getDimension());
            }
        }
    }
    for(U32 i = 0 ; i < _keys.size();++i){
        _keys[i].first->getKnob()->getKnob()->endValueChange();
    }
    
    _curveWidget->update();
    setText(QObject::tr("Remove multiple keyframes"));
}

void RemoveKeysCommand::undo(){
    addOrRemoveKeyframe(true);
}
void RemoveKeysCommand::redo(){
    addOrRemoveKeyframe(false);
}

namespace  {
    static bool selectedKeyLessFunctor(const KeyPtr& lhs,const KeyPtr& rhs)
    {
        return lhs->key.getTime() < rhs->key.getTime();
    }
}

//////////////////////////////MOVE MULTIPLE KEYS COMMAND//////////////////////////////////////////////
MoveKeysCommand::MoveKeysCommand(CurveWidget* widget,const SelectedKeys &keys, double dt, double dv,bool updateOnFirstRedo,
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
moveKey(KeyPtr&k, double dt, double dv)
{    
    std::pair<double,double> curveYRange = k->curve->getInternalCurve()->getCurveYRange();
    
    double newX = k->key.getTime() + dt;
    double newY = k->key.getValue() + dv;
    boost::shared_ptr<Curve> curve = k->curve->getInternalCurve();
    if (curve->areKeyFramesValuesClampedToIntegers()) {
        newY = std::floor(newY + 0.5);
    } else if (curve->areKeyFramesValuesClampedToBooleans()) {
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
    k->curve->getKnob()->onKeyFrameMoved(oldTime, k->key.getTime());
}

void MoveKeysCommand::move(double dt, double dv)
{
    
    ///For each knob, the old value of "evaluateOnChange" that we remember to set it back after the move.
    std::map<KnobGui*, bool> oldEvaluateOnChange;
    
    
    for (SelectedKeys::iterator it = _keys.begin(); it!= _keys.end(); ++it) {
        KnobGui* knob = (*it)->curve->getKnob();
        assert(knob);
        oldEvaluateOnChange.insert(std::make_pair(knob, knob->getKnob()->getEvaluateOnChange()));
    }
    
    
    for (std::map<KnobGui*, bool>::iterator it = oldEvaluateOnChange.begin(); it!=oldEvaluateOnChange.end(); ++it) {
        it->first->getKnob()->beginValueChange(Natron::USER_EDITED);
        
        ///set all the knobs to setEvaluateOnChange(false) so that the node doesn't get to render.
        ///remember to set it back afterwards
        ///now set to false the evaluateOnChange
        if (!_firstRedoCalled && !_updateOnFirstRedo) {
            it->first->getKnob()->setEvaluateOnChange(false);
        }
    }



    SelectedKeys newSelectedKeys;
    try {
        if(dt <= 0) {
            for (SelectedKeys::iterator it = _keys.begin(); it!= _keys.end(); ++it) {
                moveKey(*it, dt, dv);
            }
        } else {
            for(SelectedKeys::reverse_iterator it = _keys.rbegin(); it!= _keys.rend(); ++it){
                moveKey(*it, dt, dv);
            }
        }
    } catch (const std::exception& e) {
        qDebug() << "The keyframe set has changed since this action. This is probably because another user interaction is not "
        "linked to undo/redo stack.";
    }
    
    
    for (std::map<KnobGui*, bool>::iterator it = oldEvaluateOnChange.begin(); it!=oldEvaluateOnChange.end(); ++it) {
        it->first->getKnob()->endValueChange();
        if (!_firstRedoCalled && !_updateOnFirstRedo) {
            it->first->getKnob()->setEvaluateOnChange(it->second);
        }
    }
        
    _widget->refreshSelectedKeys();

}

void MoveKeysCommand::undo()
{
    move(-_dt,-_dv);
    setText(QObject::tr("Move multiple keys"));
}

void MoveKeysCommand::redo()
{
   
    move(_dt,_dv);
    _firstRedoCalled = true;
    setText(QObject::tr("Move multiple keys"));
    
}

bool MoveKeysCommand::mergeWith(const QUndoCommand * command)
{
    const MoveKeysCommand* cmd = dynamic_cast<const MoveKeysCommand*>(command);
    if (cmd && cmd->id() == id()) {
        
        if (cmd->_keys.size() != _keys.size()) {
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

int  MoveKeysCommand::id() const
{
    return kCurveEditorMoveMultipleKeysCommandCompressionID;
}


//////////////////////////////SET MULTIPLE KEYS INTERPOLATION COMMAND//////////////////////////////////////////////
SetKeysInterpolationCommand::SetKeysInterpolationCommand(CurveWidget* widget,const std::list< KeyInterpolationChange >& keys,
                                                         QUndoCommand *parent)
: QUndoCommand(parent)
, _keys(keys)
, _widget(widget)
{
}

void SetKeysInterpolationCommand::setNewInterpolation(bool undo)
{
    for (std::list< KeyInterpolationChange >::iterator it = _keys.begin();it!=_keys.end();++it) {
        it->key->curve->getKnob()->getKnob()->beginValueChange(Natron::USER_EDITED);
        int keyframeIndex = it->key->curve->getInternalCurve()->keyFrameIndex(it->key->key.getTime());
        if (undo) {
            it->key->key =  it->key->curve->getInternalCurve()->setKeyFrameInterpolation(it->oldInterp, keyframeIndex);
        } else {
            it->key->key = it->key->curve->getInternalCurve()->setKeyFrameInterpolation(it->newInterp, keyframeIndex);
        }
    }
    for (std::list< KeyInterpolationChange >::iterator it = _keys.begin();it!=_keys.end();++it) {
        it->key->curve->getKnob()->getKnob()->endValueChange();
    }
    _widget->refreshSelectedKeys();
    setText(QObject::tr("Set multiple keys interpolation"));
}

void SetKeysInterpolationCommand::undo(){
    setNewInterpolation(true);
}

void SetKeysInterpolationCommand::redo(){
    setNewInterpolation(false);
}
