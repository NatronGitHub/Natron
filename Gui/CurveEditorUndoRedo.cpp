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

#include "Global/GlobalDefines.h"

#include "Gui/CurveEditor.h"
#include "Gui/KnobGui.h"
#include "Gui/CurveWidget.h"

#include "Engine/Knob.h"
#include "Engine/Curve.h"
//////////////////////////////ADD KEY COMMAND//////////////////////////////////////////////
AddKeyCommand::AddKeyCommand(CurveWidget *editor,  NodeCurveEditorElement *curveEditorElement,
                             const std::string &actionName,const KeyFrame & key, QUndoCommand *parent)
    : QUndoCommand(parent)
    , _actionName(actionName)
    , _key(key)
    , _element(curveEditorElement)
    , _curveWidget(editor)
{

}

void AddKeyCommand::undo(){


    CurveGui* curve = _element->getCurve();
    assert(curve);
    _curveWidget->removeKeyFrame(curve,_key);
    _element->checkVisibleState();
    _curveWidget->updateGL();
    setText(QObject::tr("Add keyframe to %1")
            .arg(_actionName.c_str()));

}
void AddKeyCommand::redo(){
    CurveGui* curve = _element->getCurve();
    _curveWidget->addKeyFrame(curve, _key);
    _element->checkVisibleState();
    _curveWidget->updateGL();

    setText(QObject::tr("Add keyframe to %1")
            .arg(_actionName.c_str()));

}

//////////////////////////////ADD MULTIPLE KEYS COMMAND//////////////////////////////////////////////
PasteKeysCommand::PasteKeysCommand(CurveWidget *editor, NodeCurveEditorElement* element,
                                   const std::vector< KeyFrame >& keys,
                                   QUndoCommand *parent)
    : QUndoCommand(parent)
    , _element(element)
    , _keys(keys)
    , _curveWidget(editor)
{
}

void PasteKeysCommand::undo(){
    CurveGui* curve = _element->getCurve();
    assert(curve);

    for(U32 i = 0; i < _keys.size();++i){
        _curveWidget->removeKeyFrame(curve,_keys[i]);
    }
    _element->checkVisibleState();
    _curveWidget->updateGL();

    setText(QObject::tr("Add multiple keyframes"));
}

void PasteKeysCommand::redo(){
    CurveGui* curve = _element->getCurve();
    assert(curve);
    for(U32 i = 0; i < _keys.size();++i){
        _curveWidget->addKeyFrame(curve,_keys[i]);
    }
    _element->checkVisibleState();
    _curveWidget->updateGL();
    setText(QObject::tr("Add multiple keyframes"));

}


//////////////////////////////REMOVE KEY COMMAND//////////////////////////////////////////////

RemoveKeyCommand::RemoveKeyCommand(CurveWidget *editor, NodeCurveEditorElement *curveEditorElement,
                                   const KeyFrame& key, QUndoCommand *parent)
    : QUndoCommand(parent)
    , _element(curveEditorElement)
    , _key(key)
    , _curveWidget(editor)
{

}


void RemoveKeyCommand::undo(){
    _curveWidget->addKeyFrame(_element->getCurve(),_key);
    _element->checkVisibleState();
    _curveWidget->updateGL();
    setText(QObject::tr("Remove keyframe from %1.%2")
            .arg(_element->getKnob()->getKnob()->getDescription().c_str())
            .arg(_element->getKnob()->getKnob()->getDimensionName(_element->getDimension()).c_str()));


}
void RemoveKeyCommand::redo(){
    _curveWidget->removeKeyFrame(_element->getCurve(),_key);
    _element->checkVisibleState();
    _curveWidget->updateGL();

    setText(QObject::tr("Remove keyframe from %1.%2")
            .arg(_element->getKnob()->getKnob()->getDescription().c_str())
            .arg(_element->getKnob()->getKnob()->getDimensionName(_element->getDimension()).c_str()));

}

//////////////////////////////REMOVE  MULTIPLE KEYS COMMAND//////////////////////////////////////////////
RemoveMultipleKeysCommand::RemoveMultipleKeysCommand(CurveWidget* editor,const std::vector<std::pair<NodeCurveEditorElement*,KeyFrame> >& keys,QUndoCommand *parent )
    : QUndoCommand(parent)
    , _keys(keys)
    , _curveWidget(editor)
{
}
void RemoveMultipleKeysCommand::undo(){
    for(U32 i = 0 ; i < _keys.size();++i){
        _curveWidget->addKeyFrame(_keys[i].first->getCurve(),_keys[i].second);
        _keys[i].first->checkVisibleState();
    }
    _curveWidget->updateGL();
    setText(QObject::tr("Remove multiple keyframes"));



}
void RemoveMultipleKeysCommand::redo(){
    for(U32 i = 0 ; i < _keys.size();++i){
        _curveWidget->removeKeyFrame(_keys[i].first->getCurve(),_keys[i].second);
        _keys[i].first->checkVisibleState();
    }
    _curveWidget->updateGL();
    setText(QObject::tr("Remove multiple keyframes"));;

}

//////////////////////////////MOVE KEY COMMAND//////////////////////////////////////////////
MoveKeyCommand::MoveKeyCommand(CurveWidget* editor, const KeyMove &move, QUndoCommand *parent)
    : QUndoCommand(parent)
    , _move(move)
    , _curveWidget(editor)
{

}
void MoveKeyCommand::undo(){

    _move.curve->getInternalCurve()->setKeyFrameValueAndTime(_move.oldX, _move.oldY, _move.newX);
    _curveWidget->refreshDisplayedTangents();
    setText(QObject::tr("Move keyframe"));
}
void MoveKeyCommand::redo(){
    _move.curve->getInternalCurve()->setKeyFrameValueAndTime(_move.newX, _move.newY, _move.oldX);
    _curveWidget->refreshDisplayedTangents();
    setText(QObject::tr("Move keyframe"));
}
bool MoveKeyCommand::mergeWith(const QUndoCommand * command){
    const MoveKeyCommand* cmd = dynamic_cast<const MoveKeyCommand*>(command);
    if(cmd && cmd->id() == id()){
        _move.newX = cmd->_move.newX;
        _move.newY = cmd->_move.newY;
        return true;
    }else{
        return false;
    }
}

int MoveKeyCommand::id() const { return kCurveEditorMoveKeyCommandCompressionID; }


//////////////////////////////MOVE MULTIPLE KEYS COMMAND//////////////////////////////////////////////

MoveMultipleKeysCommand::MoveMultipleKeysCommand(CurveWidget* editor,const std::vector< KeyMove >& keys,QUndoCommand *parent )
    : QUndoCommand(parent)
    , _keys(keys)
    , _curveWidget(editor)
{
}
void MoveMultipleKeysCommand::undo(){
    for(U32 i = 0; i < _keys.size();++i){
        _keys[i].knob->getKnob()->beginValueChange(Natron::USER_EDITED);

    }
    for(U32 i = 0; i < _keys.size();++i){
        _keys[i].curve->getInternalCurve()->setKeyFrameValueAndTime(_keys[i].oldX, _keys[i].oldY, _keys[i].newX);
    }
    for(U32 i = 0; i < _keys.size();++i){
        _keys[i].knob->getKnob()->endValueChange(Natron::USER_EDITED);

    }
    _curveWidget->refreshSelectedKeysBbox();
    _curveWidget->refreshDisplayedTangents();
    setText(QObject::tr("Move multiple keys"));
}
void MoveMultipleKeysCommand::redo(){
    for(U32 i = 0; i < _keys.size();++i){
        _keys[i].knob->getKnob()->beginValueChange(Natron::USER_EDITED);

    }
    for(U32 i = 0; i < _keys.size();++i){
        _keys[i].curve->getInternalCurve()->setKeyFrameValueAndTime(_keys[i].newX, _keys[i].newY, _keys[i].oldX);
    }
    for(U32 i = 0; i < _keys.size();++i){
        _keys[i].knob->getKnob()->endValueChange(Natron::USER_EDITED);

    }
    _curveWidget->refreshSelectedKeysBbox();
    _curveWidget->refreshDisplayedTangents();

    setText(QObject::tr("Move multiple keys"));
}
bool MoveMultipleKeysCommand::mergeWith(const QUndoCommand * command){
    const MoveMultipleKeysCommand* cmd = dynamic_cast<const MoveMultipleKeysCommand*>(command);
    if(cmd && cmd->id() == id()){
        if(_keys.size() != cmd->_keys.size()){
            return false;
        }
        for(U32 i = 0; i < _keys.size();++i){
            _keys[i].newX = cmd->_keys[i].newX;
            _keys[i].newY = cmd->_keys[i].newY;
        }
        return true;
    }else{
        return false;
    }
}

int  MoveMultipleKeysCommand::id() const { return kCurveEditorMoveMultipleKeysCommandCompressionID; }

//////////////////////////////SET KEY INTERPOLATION COMMAND//////////////////////////////////////////////

SetKeyInterpolationCommand::SetKeyInterpolationCommand(CurveWidget* editor,
                                                       const KeyInterpolationChange &change,
                                                       QUndoCommand *parent)
    : QUndoCommand(parent)
    , _change(change)
    , _curveWidget(editor)
{

}

void SetKeyInterpolationCommand::undo(){
    _change.curve->getInternalCurve()->setKeyFrameInterpolation(_change.oldInterp, _change.time);
    _curveWidget->refreshDisplayedTangents();
    setText(QObject::tr("Set key interpolation"));
}

void SetKeyInterpolationCommand::redo(){
    _change.curve->getInternalCurve()->setKeyFrameInterpolation(_change.newInterp, _change.time);
    _curveWidget->refreshDisplayedTangents();
    setText(QObject::tr("Set key interpolation"));
}

//////////////////////////////SET MULTIPLE KEYS INTERPOLATION COMMAND//////////////////////////////////////////////
SetMultipleKeysInterpolationCommand::SetMultipleKeysInterpolationCommand(CurveWidget* editor,const std::vector< KeyInterpolationChange >& keys,
                                                                         QUndoCommand *parent)
    : QUndoCommand(parent)
    , _oldInterp(keys)
    , _curveWidget(editor)
{
}

void SetMultipleKeysInterpolationCommand::undo(){
    for (U32 i = 0; i < _oldInterp.size();++i) {
        _oldInterp[i].knob->getKnob()->beginValueChange(Natron::USER_EDITED);
    }
    for(U32 i = 0; i < _oldInterp.size();++i){
        _oldInterp[i].curve->getInternalCurve()->setKeyFrameInterpolation(_oldInterp[i].oldInterp, _oldInterp[i].time);
    }
    for (U32 i = 0; i < _oldInterp.size();++i) {
        _oldInterp[i].knob->getKnob()->endValueChange(Natron::USER_EDITED);
    }
    _curveWidget->refreshDisplayedTangents();
    setText(QObject::tr("Set multiple keys interpolation"));
}

void SetMultipleKeysInterpolationCommand::redo(){
    for (U32 i = 0; i < _oldInterp.size();++i) {
        _oldInterp[i].knob->getKnob()->beginValueChange(Natron::USER_EDITED);
    }
    for(U32 i = 0; i < _oldInterp.size();++i){
        _oldInterp[i].curve->getInternalCurve()->setKeyFrameInterpolation(_oldInterp[i].newInterp, _oldInterp[i].time);
    }
    for (U32 i = 0; i < _oldInterp.size();++i) {
        _oldInterp[i].knob->getKnob()->endValueChange(Natron::USER_EDITED);
    }

    _curveWidget->refreshDisplayedTangents();
    setText(QObject::tr("Set multiple keys interpolation"));
}
