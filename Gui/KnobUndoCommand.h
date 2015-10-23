/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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

#ifndef NATRON_GUI_KNOBUNDOCOMMAND_H
#define NATRON_GUI_KNOBUNDOCOMMAND_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

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
class KnobUndoCommand
    : public QUndoCommand
{
public:

    KnobUndoCommand(KnobGui *knob,
                    const T &oldValue,
                    const T &newValue,
                    int dimension = 0,
                    bool refreshGui = true,
                    QUndoCommand *parent = 0)
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
    KnobUndoCommand(KnobGui *knob,
                    const std::list<T> &oldValue,
                    const std::list<T> &newValue,
                    bool refreshGui = true,
                    QUndoCommand *parent = 0)
        : QUndoCommand(parent)
          , _dimension(-1)
          , _oldValue(oldValue)
          , _newValue(newValue)
          , _knob(knob)
          , _valueChangedReturnCode( oldValue.size() )
          , _newKeys( oldValue.size() )
          , _oldKeys( oldValue.size() )
          , _merge(true)
          , _refreshGuiFirstTime(refreshGui)
          , _firstRedoCalled(false)
    {
    }

    virtual ~KnobUndoCommand() OVERRIDE
    {
    }

private:
    virtual void undo() OVERRIDE FINAL
    {
        bool modifiedKeyFrame = false;
        int i = 0;

        _knob->getKnob()->beginChanges();
        
        for (typename std::list<T>::iterator it = _oldValue.begin(); it != _oldValue.end(); ++it) {
            int dimension = _dimension == -1 ? i : _dimension;
          
            _knob->setValue(dimension,*it,NULL,false,Natron::eValueChangedReasonUserEdited);
            if ( _knob->getKnob()->getHolder()->getApp() ) {
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
        
        ///This will refresh all dimensions
        _knob->onInternalValueChanged(-1, Natron::eValueChangedReasonNatronGuiEdited);
        
        _knob->getKnob()->endChanges();
        if (modifiedKeyFrame) {
            _knob->getGui()->getCurveEditor()->getCurveWidget()->refreshSelectedKeys();
        }

        setText( QObject::tr("Set value of %1")
                 .arg( _knob->getKnob()->getDescription().c_str() ) );
    }

    virtual void redo() OVERRIDE FINAL
    {
        SequenceTime time = 0;

        if ( _knob->getKnob()->getHolder()->getApp() ) {
            time = _knob->getKnob()->getHolder()->getApp()->getTimeLine()->currentFrame();
        }

        bool modifiedKeyFrames = false;

        _knob->getKnob()->beginChanges();
        int i = 0;
        
        for (typename std::list<T>::iterator it = _newValue.begin(); it != _newValue.end(); ++it) {
            int dimension = _dimension == -1 ? i : _dimension;
    

            boost::shared_ptr<Curve> c = _knob->getKnob()->getCurve(dimension);
            //find out if there's already an existing keyframe before calling setValue
            if (c) {
                bool found = c->getKeyFrameWithTime(time, &_oldKeys[i]);
                Q_UNUSED(found); // we don't care if it existed or not
            }

 
            _valueChangedReturnCode[i] = _knob->setValue(dimension,*it,&_newKeys[i],false,Natron::eValueChangedReasonUserEdited);
            if (_valueChangedReturnCode[i] != KnobHelper::eValueChangedReturnCodeNoKeyframeAdded) {
                modifiedKeyFrames = true;
            }
    
            ///if we added a keyframe, prevent this command to merge with any other command
            if (_valueChangedReturnCode[i] == KnobHelper::eValueChangedReturnCodeKeyframeAdded) {
                _merge = false;
            }
            ++i;
        
        }
        
        
        ///This will refresh all dimensions
        if (_firstRedoCalled || _refreshGuiFirstTime) {
            _knob->onInternalValueChanged(-1, Natron::eValueChangedReasonNatronGuiEdited);
        }

        
        _knob->getKnob()->endChanges();

        if (modifiedKeyFrames) {
            _knob->getGui()->getCurveEditor()->getCurveWidget()->refreshSelectedKeys();
        }

        setText( QObject::tr("Set value of %1")
                 .arg( _knob->getKnob()->getDescription().c_str() ) );

        _firstRedoCalled = true;
    } // redo

    virtual int id() const OVERRIDE FINAL
    {
        return kKnobUndoChangeCommandCompressionID;
    }

    virtual bool mergeWith(const QUndoCommand *command) OVERRIDE FINAL
    {
        const KnobUndoCommand *knobCommand = dynamic_cast<const KnobUndoCommand *>(command);

        if ( !knobCommand || ( command->id() != id() ) ) {
            return false;
        }

        KnobGui *knob = knobCommand->_knob;
        if ( (_knob != knob) || !_merge || !knobCommand->_merge || (_dimension != knobCommand->_dimension) ) {
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


/**
 * @brief This class is used by the internal knob when it wants to group multiple edits into a single undo/redo action.
 * It is not used by the GUI
 **/
class MultipleKnobEditsUndoCommand
    : public QUndoCommand
{
    struct ValueToSet
    {
        boost::shared_ptr<KnobI> copy;
        Variant newValue;
        int dimension;
        int time;
        bool setKeyFrame;
    };

    ///For each knob, the second member points to a clone of the knob before the first redo() call was made
    typedef std::map < KnobGui*, ValueToSet >  ParamsMap;
    ParamsMap knobs;
    bool createNew;
    bool firstRedoCalled;
    Natron::ValueChangedReasonEnum _reason;
public:

    /**
     * @brief Make a new command
     * @param createNew If true this command will not merge with a previous same command
     * @param setKeyFrame if true, the command will use setValueAtTime instead of setValue in the redo() command.
     **/
    MultipleKnobEditsUndoCommand(KnobGui* knob,
                                 Natron::ValueChangedReasonEnum reason,
                                 bool createNew,
                                 bool setKeyFrame,
                                 const Variant & value,
                                 int dimension,
                                 int time);

    virtual ~MultipleKnobEditsUndoCommand();

    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;
    virtual int id() const OVERRIDE FINAL;
    virtual bool mergeWith(const QUndoCommand *command) OVERRIDE FINAL;
    static boost::shared_ptr<KnobI> createCopyForKnob(const boost::shared_ptr<KnobI> & originalKnob);
};

class PasteUndoCommand
    : public QUndoCommand
{
    KnobGui* _knob;
    std::list<Variant> newValues,oldValues;
    std::list<boost::shared_ptr<Curve> > newCurves,oldCurves;
    std::list<boost::shared_ptr<Curve> > newParametricCurves,oldParametricCurves;
    std::map<int,std::string> newStringAnimation,oldStringAnimation;
    bool _copyAnimation;

public:

    PasteUndoCommand(KnobGui* knob,
                     bool copyAnimation,
                     const std::list<Variant> & values,
                     const std::list<boost::shared_ptr<Curve> > & curves,
                     const std::list<boost::shared_ptr<Curve> > & parametricCurves,
                     const std::map<int,std::string> & stringAnimation);

    virtual ~PasteUndoCommand()
    {
    }

    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;
};


class RestoreDefaultsCommand
    : public QUndoCommand
{
public:

    RestoreDefaultsCommand(const std::list<boost::shared_ptr<KnobI> > & knobs,
                           QUndoCommand *parent = 0);
    virtual void undo();
    virtual void redo();

private:

    std::list<boost::shared_ptr<KnobI> > _knobs,_clones;
};

class SetExpressionCommand
: public QUndoCommand
{
public:
    
    SetExpressionCommand(const boost::shared_ptr<KnobI> & knob,
                         bool hasRetVar,
                         int dimension,
                         const std::string& expr,
                         QUndoCommand *parent = 0);
    virtual void undo();
    virtual void redo();
    
private:
    
    boost::shared_ptr<KnobI > _knob;
    std::vector<std::string> _oldExprs;
    std::vector<bool> _hadRetVar;
    std::string _newExpr;
    bool _hasRetVar;
    int _dimension;
};

#endif // NATRON_GUI_KNOBUNDOCOMMAND_H
