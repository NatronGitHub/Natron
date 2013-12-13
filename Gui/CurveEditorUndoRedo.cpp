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
    _element->getKnob()->getKnob()->beginValueChange(Natron::USER_EDITED);
    for(U32 i = 0; i < _keys.size();++i){
        _curveWidget->removeKeyFrame(curve,_keys[i]);
    }
    _element->getKnob()->getKnob()->endValueChange(Natron::USER_EDITED);
    _element->checkVisibleState();
    _curveWidget->updateGL();

    setText(QObject::tr("Add multiple keyframes"));
}

void PasteKeysCommand::redo(){
    CurveGui* curve = _element->getCurve();
    assert(curve);
    _element->getKnob()->getKnob()->beginValueChange(Natron::USER_EDITED);
    for(U32 i = 0; i < _keys.size();++i){
        _curveWidget->addKeyFrame(curve,_keys[i]);
    }
    _element->getKnob()->getKnob()->endValueChange(Natron::USER_EDITED);
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
        _keys[i].first->getKnob()->getKnob()->beginValueChange(Natron::USER_EDITED);
        _curveWidget->addKeyFrame(_keys[i].first->getCurve(),_keys[i].second);
        _keys[i].first->checkVisibleState();
    }
    for(U32 i = 0 ; i < _keys.size();++i){
        _keys[i].first->getKnob()->getKnob()->endValueChange(Natron::USER_EDITED);
    }

    _curveWidget->updateGL();
    setText(QObject::tr("Remove multiple keyframes"));



}
void RemoveMultipleKeysCommand::redo(){
    for(U32 i = 0 ; i < _keys.size();++i){
        _keys[i].first->getKnob()->getKnob()->beginValueChange(Natron::USER_EDITED);
        _curveWidget->removeKeyFrame(_keys[i].first->getCurve(),_keys[i].second);
        _keys[i].first->checkVisibleState();
    }
    for(U32 i = 0 ; i < _keys.size();++i){
        _keys[i].first->getKnob()->getKnob()->endValueChange(Natron::USER_EDITED);
    }
    _curveWidget->updateGL();
    setText(QObject::tr("Remove multiple keyframes"));;

}

//////////////////////////////MOVE MULTIPLE KEYS COMMAND//////////////////////////////////////////////
MoveMultipleKeysCommand::MoveMultipleKeysCommand(CurveWidget* editor, const KeyMoveV &keys, int dt, double dv,
                                                 QUndoCommand *parent )
    : QUndoCommand(parent)
    , _merge(!keys.empty())
    , _dt(dt)
    , _dv(dv)
    , _keys(keys)
    , _curveWidget(editor)
{
}

void MoveMultipleKeysCommand::move(int dt,double dv, bool isundo){
    SelectedKeys newSelectedKeys;
    if(dt < 0){
        for(KeyMoveV::iterator it = _keys.begin();it!= _keys.end();++it){
            it->knob->getKnob()->beginValueChange(Natron::USER_EDITED);
            int newX = it->oldPos.getTime() + dt;
            double newY = it->oldPos.getValue() + dv;
            std::cout << "move " << (isundo ? newX : it->oldPos.getTime()) << " to " << (isundo ? it->oldPos.getTime() : newX) <<std::endl;
            it->oldPos = it->curve->getInternalCurve()->setKeyFrameValueAndTime(
                        isundo ? it->oldPos.getTime() : newX,
                        isundo ? it->oldPos.getValue() : newY,
                        isundo ? newX : it->oldPos.getTime());
        }

    }else{

        for(KeyMoveV::reverse_iterator it = _keys.rbegin();it!= _keys.rend();++it){
            it->knob->getKnob()->beginValueChange(Natron::USER_EDITED);
            int newX = it->oldPos.getTime() + dt;
            double newY = it->oldPos.getValue() + dv;
            std::cout << "move " << (isundo ? newX : it->oldPos.getTime()) << " to " << (isundo ? it->oldPos.getTime() : newX) <<std::endl;
            it->oldPos = it->curve->getInternalCurve()->setKeyFrameValueAndTime(
                        isundo ? it->oldPos.getTime() : newX,
                        isundo ? it->oldPos.getValue() : newY,
                        isundo ? newX : it->oldPos.getTime());

        }
    }


    //copy back the modified keyframes to the selectd keys
    for(KeyMoveV::iterator it = _keys.begin();it!= _keys.end();++it){
        const Curve& c = *(it->curve->getInternalCurve());
        newSelectedKeys.insert(SelectedKey(it->curve,*(c.find(it->oldPos.getTime()))));
        it->knob->getKnob()->endValueChange(Natron::USER_EDITED);

    }
    if(!_keys.empty()){
        _curveWidget->setSelectedKeys(newSelectedKeys);
        _curveWidget->refreshSelectedKeysBbox();
        _curveWidget->refreshDisplayedTangents();
    }

}

void MoveMultipleKeysCommand::undo(){
    move(_dt,_dv,true);
    setText(QObject::tr("Move multiple keys"));
}

void MoveMultipleKeysCommand::redo(){
    move(_dt,_dv,false);
    setText(QObject::tr("Move multiple keys"));

}

bool MoveMultipleKeysCommand::mergeWith(const QUndoCommand * command){
    const MoveMultipleKeysCommand* cmd = dynamic_cast<const MoveMultipleKeysCommand*>(command);
    if(cmd && cmd->id() == id()){
        if(!cmd->_merge){
            return false;
        }
        _dt += cmd->_dt;
        _dv += cmd->_dv;
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
    SelectedKeys newSelectedKeys;
    _change.key = _change.curve->getInternalCurve()->setKeyFrameInterpolation(_change.oldInterp, _change.key.getTime());
    newSelectedKeys.insert(SelectedKey(_change.curve,_change.key));
    _curveWidget->setSelectedKeys(newSelectedKeys);
    _curveWidget->refreshDisplayedTangents();
    setText(QObject::tr("Set key interpolation"));
}

void SetKeyInterpolationCommand::redo(){
    SelectedKeys newSelectedKeys;
    _change.key = _change.curve->getInternalCurve()->setKeyFrameInterpolation(_change.newInterp, _change.key.getTime());
    newSelectedKeys.insert(SelectedKey(_change.curve,_change.key));
    _curveWidget->setSelectedKeys(newSelectedKeys);
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
    SelectedKeys newSelectedKeys;

    for(U32 i = 0; i < _oldInterp.size();++i){
        _oldInterp[i].knob->getKnob()->beginValueChange(Natron::USER_EDITED);
        _oldInterp[i].key =  _oldInterp[i].curve->getInternalCurve()->setKeyFrameInterpolation(
                    _oldInterp[i].oldInterp, _oldInterp[i].key.getTime());
        newSelectedKeys.insert(SelectedKey(_oldInterp[i].curve,_oldInterp[i].key));
    }
    for (U32 i = 0; i < _oldInterp.size();++i) {
        const Curve& c = *(_oldInterp[i].curve->getInternalCurve());
        newSelectedKeys.insert(SelectedKey(_oldInterp[i].curve,*(c.find(_oldInterp[i].key.getTime()))));
        _oldInterp[i].knob->getKnob()->endValueChange(Natron::USER_EDITED);
    }
    _curveWidget->setSelectedKeys(newSelectedKeys);
    _curveWidget->refreshDisplayedTangents();
    setText(QObject::tr("Set multiple keys interpolation"));
}

void SetMultipleKeysInterpolationCommand::redo(){
    SelectedKeys newSelectedKeys;
    for(U32 i = 0; i < _oldInterp.size();++i){
        _oldInterp[i].knob->getKnob()->beginValueChange(Natron::USER_EDITED);
        _oldInterp[i].key = _oldInterp[i].curve->getInternalCurve()->setKeyFrameInterpolation(
                    _oldInterp[i].newInterp, _oldInterp[i].key.getTime());
    }
    for (U32 i = 0; i < _oldInterp.size();++i) {
        const Curve& c = *(_oldInterp[i].curve->getInternalCurve());
        newSelectedKeys.insert(SelectedKey(_oldInterp[i].curve,*(c.find(_oldInterp[i].key.getTime()))));
        _oldInterp[i].knob->getKnob()->endValueChange(Natron::USER_EDITED);
    }
    _curveWidget->setSelectedKeys(newSelectedKeys);
    _curveWidget->refreshDisplayedTangents();
    setText(QObject::tr("Set multiple keys interpolation"));
}
