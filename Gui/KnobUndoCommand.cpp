//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "Gui/KnobUndoCommand.h"

#include "Global/GlobalDefines.h" // SequenceTime
#include "Global/AppManager.h" // AppInstance

#include "Engine/TimeLine.h"
#include "Engine/Curve.h"
#include "Engine/Knob.h"

#include "Gui/KnobGui.h"

//================================================================


KnobUndoCommand::KnobUndoCommand(KnobGui *knob,  const std::vector<Variant> &oldValue, const std::vector<Variant> &newValue, QUndoCommand *parent)
: QUndoCommand(parent)
, _oldValue(oldValue)
, _newValue(newValue)
, _knob(knob)
, _valueChangedReturnCode(oldValue.size())
, _newKeys(oldValue.size())
{
}

void KnobUndoCommand::undo()
{
    _knob->getKnob()->beginValueChange(Natron::USER_EDITED);
    for (U32 i = 0 ; i < _oldValue.size();++i) {
        _knob->setValue(i,_oldValue[i],NULL);
        if(_knob->getKnob()->getHolder()->getApp()){
           if(_valueChangedReturnCode[i] == 2 ){ //the value change also added a keyframe
               _knob->removeKeyFrame(_newKeys[i].getTime(),i);
           }else if(_valueChangedReturnCode[i] == 1){
               _knob->setKeyframe(_newKeys[i].getTime(), i);
           }
        }
        
    }
    _knob->getKnob()->endValueChange(Natron::USER_EDITED);
    setText(QObject::tr("Set value of %1")
            .arg(_knob->getKnob()->getDescription().c_str()));
}

void KnobUndoCommand::redo()
{
    
    SequenceTime time = 0;
    if(_knob->getKnob()->getHolder()->getApp()){
        time = _knob->getKnob()->getHolder()->getApp()->getTimeLine()->currentFrame();
    }
    
    _knob->getKnob()->beginValueChange(Natron::USER_EDITED);
    for (U32 i = 0; i < _newValue.size();++i) {
        boost::shared_ptr<Curve> c = _knob->getKnob()->getCurve(i);
        _valueChangedReturnCode[i] = _knob->setValue(i,_newValue[i],&_newKeys[i]);
        
    }
    _knob->getKnob()->endValueChange(Natron::USER_EDITED);
    setText(QObject::tr("Set value of %1")
            .arg(_knob->getKnob()->getDescription().c_str()));
}


int KnobUndoCommand::id() const
{
    return kKnobUndoChangeCommandCompressionID;
}

bool KnobUndoCommand::mergeWith(const QUndoCommand *command)
{
    const KnobUndoCommand *knobCommand = dynamic_cast<const KnobUndoCommand *>(command);
    if (!knobCommand || command->id() != id()) {
        return false;
    }
    
    KnobGui *knob = knobCommand->_knob;
    if (_knob != knob) {
        return false;
    }
    _newValue = knobCommand->_newValue;
    return true;
}

