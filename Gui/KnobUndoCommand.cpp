//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "Gui/KnobUndoCommand.h"

#include "Global/GlobalDefines.h" // SequenceTime

#include "Engine/TimeLine.h"
#include "Engine/Curve.h"
#include "Engine/Knob.h"
#include "Engine/EffectInstance.h"

#include "Gui/KnobGui.h"
#include "Gui/Gui.h"
#include "Gui/CurveEditor.h"
#include "Gui/CurveWidget.h"
#include "Gui/GuiAppInstance.h"
//================================================================


KnobUndoCommand::KnobUndoCommand(KnobGui *knob,  const std::vector<Variant> &oldValue, const std::vector<Variant> &newValue, QUndoCommand *parent)
: QUndoCommand(parent)
, _oldValue(oldValue)
, _newValue(newValue)
, _knob(knob)
, _valueChangedReturnCode(oldValue.size())
, _newKeys(oldValue.size())
, _oldKeys(oldValue.size())
, _merge(true)
{
    Natron::EffectInstance* effect = dynamic_cast<Natron::EffectInstance*>(knob->getKnob()->getHolder());
    if (effect) {
        _oldAge = effect->knobsAge();
    } else {
        _oldAge = 0;
    }
}

void KnobUndoCommand::undo()
{
    _knob->getKnob()->beginValueChange(Natron::USER_EDITED);
    bool modifiedKeyFrame = false;
    for (U32 i = 0 ; i < _oldValue.size();++i) {
        _knob->setValue(i,_oldValue[i],NULL);
        if (_knob->getKnob()->getHolder()->getApp()) {
           if (_valueChangedReturnCode[i] == 1) { //the value change also added a keyframe
               _knob->removeKeyFrame(_newKeys[i].getTime(),i);
               modifiedKeyFrame = true;
           } else if (_valueChangedReturnCode[i] == 2) {
               //the value change moved a keyframe
               _knob->removeKeyFrame(_newKeys[i].getTime(),i);
               _knob->setKeyframe(_oldKeys[i].getTime(), i);
               modifiedKeyFrame = true;
           }
        }
        
    }
    if (modifiedKeyFrame) {
        _knob->getGui()->getCurveEditor()->getCurveWidget()->refreshSelectedKeys();
    }
    
    ///before calling endValueChange (which will trigger evaluate() and increment the knobs age), set the knobs age to the old age
    ///it had before having this redo() made (minus 1), and then call evaluate on it which will make the knobs age equivalent to the age
    /// as if nothing were done in the first place.
    Natron::EffectInstance* effect = dynamic_cast<Natron::EffectInstance*>(_knob->getKnob()->getHolder());
    if (effect) {
        effect->setKnobsAge(_oldAge - 1);
    }

    _knob->getKnob()->endValueChange();
    setText(QObject::tr("Set value of %1")
            .arg(_knob->getKnob()->getDescription().c_str()));
}

void KnobUndoCommand::redo()
{
    SequenceTime time = 0;
    if (_knob->getKnob()->getHolder()->getApp()) {
        time = _knob->getKnob()->getHolder()->getApp()->getTimeLine()->currentFrame();
    }
    
    _knob->getKnob()->beginValueChange(Natron::USER_EDITED);
    bool modifiedKeyFrames = false;
    for (U32 i = 0; i < _newValue.size();++i) {
        boost::shared_ptr<Curve> c = _knob->getKnob()->getCurve(i);
        //find out if there's already an existing keyframe before calling setValue
        bool found = c->getKeyFrameWithTime(time, &_oldKeys[i]);
        (void)found; // we don't care if it existed or not

        _valueChangedReturnCode[i] = _knob->setValue(i,_newValue[i],&_newKeys[i]);
        if(_valueChangedReturnCode[i] != Knob::NO_KEYFRAME_ADDED){
            modifiedKeyFrames = true;
        }
        
        ///if we added a keyframe, prevent this command to merge with any other command
        if (_valueChangedReturnCode[i] == Knob::KEYFRAME_ADDED) {
            _merge = false;
        }
    }
    
    if (modifiedKeyFrames) {
        _knob->getGui()->getCurveEditor()->getCurveWidget()->refreshSelectedKeys();
    }
    
    _knob->getKnob()->endValueChange();
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
    if (_knob != knob || !_merge || !knobCommand->_merge) {
        return false;
    }
    
    
    
    _newValue = knobCommand->_newValue;
    return true;
}

