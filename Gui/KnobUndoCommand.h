//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef NATRON_GUI_KNOBUNDOCOMMAND_H_
#define NATRON_GUI_KNOBUNDOCOMMAND_H_

#include <map>
#include <vector>

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QUndoCommand>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/GlobalDefines.h"

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
    
    KnobUndoCommand(KnobGui *knob, const T &oldValue, const T &newValue,int dimension = 0,bool refreshGui = true, QUndoCommand *parent = 0)
    : QUndoCommand(parent)
    , _dimension(dimension)
    , _oldValue()
    , _newValue()
    , _knob(knob)
    , _valueChangedReturnCode(1)
    , _newKeys(1)
    , _oldKeys(1)
    , _merge(true)
    , _refreshGuiFirstTime(refreshGui)
    , _firstRedoCalled(false)
    {
        _oldValue.push_back(oldValue);
        _newValue.push_back(newValue);
    }

    
    ///We moved from std::vector to std::list instead because std::vector<bool> expands to a bit field(std::_Bit_reference)
    ///and produces an unresolved symbol error.
    KnobUndoCommand(KnobGui *knob, const std::list<T> &oldValue, const std::list<T> &newValue,bool refreshGui = true, QUndoCommand *parent = 0)
    : QUndoCommand(parent)
    , _dimension(-1)
    , _oldValue(oldValue)
    , _newValue(newValue)
    , _knob(knob)
    , _valueChangedReturnCode(oldValue.size())
    , _newKeys(oldValue.size())
    , _oldKeys(oldValue.size())
    , _merge(true)
    , _refreshGuiFirstTime(refreshGui)
    , _firstRedoCalled(false)
    {
        
    }

    virtual ~KnobUndoCommand() OVERRIDE {}

private:
    virtual void undo() OVERRIDE FINAL
    {
        _knob->getKnob()->beginValueChange(Natron::USER_EDITED);
        bool modifiedKeyFrame = false;
        
        int i = 0;
        typename std::list<T>::iterator next = _oldValue.begin();
        ++next;
        for (typename std::list<T>::iterator it = _oldValue.begin(); it!=_oldValue.end();++it,++next) {
            
            int dimension = _dimension == -1 ? i : _dimension;
            bool triggerOnKnobChanged = next == _oldValue.end();
            _knob->setValue(dimension,*it,NULL,true,triggerOnKnobChanged);
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
        typename std::list<T>::iterator next = _newValue.begin();
        ++next;
        for (typename std::list<T>::iterator it = _newValue.begin(); it!=_newValue.end();++it,++next) {
            
            int dimension = _dimension == -1 ? i : _dimension;
            bool triggerOnKnobChanged = next == _newValue.end();
            boost::shared_ptr<Curve> c = _knob->getKnob()->getCurve(dimension);
            //find out if there's already an existing keyframe before calling setValue
            bool found = c->getKeyFrameWithTime(time, &_oldKeys[i]);
            (void)found; // we don't care if it existed or not
            
            bool refreshGui;
            if (_firstRedoCalled) {
                refreshGui = true;
            } else {
                refreshGui = _refreshGuiFirstTime;
            }
            _valueChangedReturnCode[i] = _knob->setValue(dimension,*it,&_newKeys[i],refreshGui,triggerOnKnobChanged);
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

        _firstRedoCalled = true;
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
    bool _refreshGuiFirstTime;
    bool _firstRedoCalled;
};


class MultipleKnobEditsUndoCommand : public QUndoCommand
{
    struct ValueToSet {
        boost::shared_ptr<KnobI> copy;
        std::list<Variant> newValues;
        int time;
        bool setKeyFrame;
    };
    
    ///For each knob, the second member points to a clone of the knob before the first redo() call was made
    typedef std::map < KnobGui* , ValueToSet >  ParamsMap;
    ParamsMap knobs;
    bool createNew;
    bool firstRedoCalled;
    bool triggerOnKnobChanged;
public:
    
    /**
     * @brief Make a new command
     * @param createNew If true this command will not merge with a previous same command
     * @param setKeyFrame if true, the command will use setValueAtTime instead of setValue in the redo() command.
     **/
    MultipleKnobEditsUndoCommand(KnobGui* knob,bool createNew,bool setKeyFrame,bool triggerOnKnobChanged,
                                 const std::list<Variant>& values,int time);
    
    virtual ~MultipleKnobEditsUndoCommand();
  
    virtual void undo() OVERRIDE FINAL;
    
    virtual void redo() OVERRIDE FINAL;
    
    virtual int id() const OVERRIDE FINAL;
    
    virtual bool mergeWith(const QUndoCommand *command) OVERRIDE FINAL;
    
private:
    
    boost::shared_ptr<KnobI> createCopyForKnob(const boost::shared_ptr<KnobI>& originalKnob) const;
};

class PasteUndoCommand : public QUndoCommand
{
    KnobGui* _knob;
    
    std::list<Variant> newValues,oldValues;
    std::list<boost::shared_ptr<Curve> > newCurves,oldCurves;
    std::list<boost::shared_ptr<Curve> > newParametricCurves,oldParametricCurves;
    std::map<int,std::string> newStringAnimation,oldStringAnimation;

    int _targetDimension;
    int _dimensionToFetch;
    bool _copyAnimation;
public:
    
    PasteUndoCommand(KnobGui* knob,int targetDimension,
                     int dimensionToFetch,
                     bool copyAnimation,
                     const std::list<Variant>& values,
                     const std::list<boost::shared_ptr<Curve> >& curves,
                     const std::list<boost::shared_ptr<Curve> >& parametricCurves,
                     const std::map<int,std::string>& stringAnimation);
    
    virtual ~PasteUndoCommand() {}
    
    virtual void undo() OVERRIDE FINAL;
    
    virtual void redo() OVERRIDE FINAL;
    
};


#endif // NATRON_GUI_KNOBUNDOCOMMAND_H_
