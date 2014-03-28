//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef NATRON_GUI_KNOBUNDOCOMMAND_H_
#define NATRON_GUI_KNOBUNDOCOMMAND_H_

#include <map>
#include <vector>

#include "Global/GlobalDefines.h"
CLANG_DIAG_OFF(deprecated)
#include <QUndoCommand>
CLANG_DIAG_ON(deprecated)

#include "Engine/Variant.h"
#include "Engine/Curve.h"
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

template<typename T>
class KnobUndoCommand : public QUndoCommand
{
public:
    
    KnobUndoCommand(KnobGui *knob, const T &oldValue, const T &newValue,int dimension = 0, QUndoCommand *parent = 0)
    : QUndoCommand(parent)
    , _dimension(dimension)
    , _oldValue()
    , _newValue()
    , _knob(knob)
    , _valueChangedReturnCode(1)
    , _newKeys(1)
    , _oldKeys(1)
    , _merge(true)
    {
        _oldValue.push_back(oldValue);
        _newValue.push_back(newValue);
    }

    
    ///We moved from std::vector to std::list instead because std::vector<bool> expands to a bit field(std::_Bit_reference)
    ///and produces an unresolved symbol error.
    KnobUndoCommand(KnobGui *knob, const std::list<T> &oldValue, const std::list<T> &newValue, QUndoCommand *parent = 0)
    : QUndoCommand(parent)
    , _dimension(-1)
    , _oldValue(oldValue)
    , _newValue(newValue)
    , _knob(knob)
    , _valueChangedReturnCode(oldValue.size())
    , _newKeys(oldValue.size())
    , _oldKeys(oldValue.size())
    , _merge(true)
    {
        
    }

    virtual ~KnobUndoCommand() OVERRIDE {}

private:
    virtual void undo() OVERRIDE FINAL
    {
        _knob->getKnob()->beginValueChange(Natron::USER_EDITED);
        bool modifiedKeyFrame = false;
        
        int i = 0;
        for (typename std::list<T>::iterator it = _oldValue.begin(); it!=_oldValue.end();++it) {
            
            int dimension = _dimension == -1 ? i : _dimension;
            
            _knob->setValue(dimension,*it,NULL);
            if (_knob->getKnob()->getHolder()->getApp()) {
                if (_valueChangedReturnCode[i] == 1) { //the value change also added a keyframe
                    _knob->removeKeyFrame(_newKeys[i].getTime(),dimension);
                    modifiedKeyFrame = true;
                } else if (_valueChangedReturnCode[i] == 2) {
                    //the value change moved a keyframe
                    _knob->removeKeyFrame(_newKeys[i].getTime(),dimension);
                    _knob->setKeyframe(_oldKeys[i].getTime(), dimension);
                    modifiedKeyFrame = true;
                }
            }
            ++i;
        }
        if (modifiedKeyFrame) {
            _knob->getGui()->getCurveEditor()->getCurveWidget()->refreshSelectedKeys();
        }
        
        
        _knob->getKnob()->endValueChange();
        setText(QObject::tr("Set value of %1")
                .arg(_knob->getKnob()->getDescription().c_str()));
    }

    virtual void redo() OVERRIDE FINAL
    {
        SequenceTime time = 0;
        if (_knob->getKnob()->getHolder()->getApp()) {
            time = _knob->getKnob()->getHolder()->getApp()->getTimeLine()->currentFrame();
        }
        
        _knob->getKnob()->beginValueChange(Natron::USER_EDITED);
        bool modifiedKeyFrames = false;
        
        int i = 0;
        for (typename std::list<T>::iterator it = _newValue.begin(); it!=_newValue.end();++it) {
            
            int dimension = _dimension == -1 ? i : _dimension;
            
            boost::shared_ptr<Curve> c = _knob->getKnob()->getCurve(dimension);
            //find out if there's already an existing keyframe before calling setValue
            bool found = c->getKeyFrameWithTime(time, &_oldKeys[i]);
            (void)found; // we don't care if it existed or not
            
            _valueChangedReturnCode[i] = _knob->setValue(dimension,*it,&_newKeys[i]);
            if(_valueChangedReturnCode[i] != KnobHelper::NO_KEYFRAME_ADDED){
                modifiedKeyFrames = true;
            }
            
            ///if we added a keyframe, prevent this command to merge with any other command
            if (_valueChangedReturnCode[i] == KnobHelper::KEYFRAME_ADDED) {
                _merge = false;
            }
            ++i;
        }
        
        if (modifiedKeyFrames) {
            _knob->getGui()->getCurveEditor()->getCurveWidget()->refreshSelectedKeys();
        }
        
        _knob->getKnob()->endValueChange();
        setText(QObject::tr("Set value of %1")
                .arg(_knob->getKnob()->getDescription().c_str()));

    }

    virtual int id() const OVERRIDE FINAL
    {
        return kKnobUndoChangeCommandCompressionID;
    }
    
    virtual bool mergeWith(const QUndoCommand *command) OVERRIDE FINAL
    {
        const KnobUndoCommand *knobCommand = dynamic_cast<const KnobUndoCommand *>(command);
        if (!knobCommand || command->id() != id()) {
            return false;
        }
        
        KnobGui *knob = knobCommand->_knob;
        if (_knob != knob || !_merge || !knobCommand->_merge || _dimension != knobCommand->_dimension) {
            return false;
        }
        
        
        
        _newValue = knobCommand->_newValue;
        return true;

    }
    
private:
    // TODO: PIMPL
    int _dimension;
    std::list<T> _oldValue;
    std::list<T> _newValue;
    KnobGui *_knob;
    std::vector<int> _valueChangedReturnCode;
    std::vector<KeyFrame> _newKeys;
    std::vector<KeyFrame>  _oldKeys;
    bool _merge;
};


#endif // NATRON_GUI_KNOBUNDOCOMMAND_H_
