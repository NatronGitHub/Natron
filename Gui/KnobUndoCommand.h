/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
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

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#endif

#include <map>
#include <vector>

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
#include "Engine/ViewIdx.h"

#include "Gui/KnobGui.h"
#include "Gui/Gui.h"
#include "Gui/CurveEditor.h"
#include "Gui/CurveWidget.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiFwd.h"

NATRON_NAMESPACE_ENTER;

//================================================================

template<typename T>
class KnobUndoCommand
    : public QUndoCommand
{
    Q_DECLARE_TR_FUNCTIONS(KnobUndoCommand)

public:

    KnobUndoCommand(const KnobGuiPtr& knob,
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
        , _merge(true)
        , _refreshGuiFirstTime(refreshGui)
        , _firstRedoCalled(false)
    {
        assert(dimension >= 0 && dimension < knob->getKnob()->getNDimensions());
        _oldValue.push_back(oldValue);
        _newValue.push_back(newValue);
    }

    ///We moved from std::vector to std::list instead because std::vector<bool> expands to a bit field(std::_Bit_reference)
    ///and produces an unresolved symbol error.
    KnobUndoCommand(const KnobGuiPtr& knob,
                    const std::vector<T> &oldValue,
                    const std::vector<T> &newValue,
                    bool refreshGui = true,
                    QUndoCommand *parent = 0)
        : QUndoCommand(parent)
        , _dimension(-1)
        , _oldValue(oldValue)
        , _newValue(newValue)
        , _knob(knob)
        , _valueChangedReturnCode( oldValue.size() )
        , _merge(true)
        , _refreshGuiFirstTime(refreshGui)
        , _firstRedoCalled(false)
        , _timelineTime(knob->getKnob()->getCurrentTime())
    {
        assert(oldValue.size() == newValue.size() && oldValue.size() == knob->getKnob()->getNDimensions());


        setText( tr("Set value of %1").arg( QString::fromUtf8( knob->getKnob()->getLabel().c_str() ) ) );
    }

    virtual ~KnobUndoCommand() OVERRIDE
    {
    }

private:
    virtual void undo() OVERRIDE FINAL
    {
        KnobGuiPtr knobUI = _knob.lock();
        if (!knobUI) {
            return;
        }
        boost::shared_ptr<Knob<T> > knob = boost::dynamic_pointer_cast<Knob<T> >(knobUI->getKnob());
        assert(knob);


        assert( (int)_oldValue.size() == knob->getNDimensions() || _dimension != -1 );

        // If any keyframe was added due to auto-keying, remove it
        for (std::size_t i = 0; i < _oldValue.size(); ++i) {
            if (_dimension == -1 || i == _dimension) {
                if (_valueChangedReturnCode[i] == KnobHelper::eValueChangedReturnCodeKeyframeAdded) {
                    knob->deleteValueAtTime(eCurveChangeReasonInternal, _timelineTime, ViewSpec::all(), i);
                }
            }
        }

        // Ensure that no keyframe gets added by the setValue
        knob->setAutoKeyingEnabled(false);
        if (_dimension == -1) {
            // Multiple dimensions set at once
            knob->setValues(_oldValue, ViewSpec::all(), eValueChangedReasonUserEdited, 0);
        } else {
            // Only one dimension set
            ignore_result(knob->setValue(_oldValue[0], ViewSpec::all(), _dimension, eValueChangedReasonUserEdited, 0));

        }

        knob->setAutoKeyingEnabled(true);

    } // undo

    virtual void redo() OVERRIDE FINAL
    {
        KnobGuiPtr knobUI = _knob.lock();
        if (!knobUI) {
            return;
        }
        boost::shared_ptr<Knob<T> > knob = boost::dynamic_pointer_cast<Knob<T> >(knobUI->getKnob());
        assert(knob);


        assert( (int)_oldValue.size() == knob->getNDimensions() || _dimension != -1 );

        if (_dimension == -1 && (int)_newValue.size() > 1) {
            // Multiple dimensions set at once
            knob->setValues(_newValue, ViewSpec::all(), eValueChangedReasonUserEdited, &_valueChangedReturnCode);
        } else {
            // Only one dimension set
            _valueChangedReturnCode[0] = knob->setValue(_newValue[0], ViewSpec::all(), _dimension, eValueChangedReasonUserEdited, 0);
        }


        // If we added a keyframe, prevent this command to merge with any other command
        for (std::size_t i = 0; i < _valueChangedReturnCode.size(); ++i) {
            if (_valueChangedReturnCode[i] == KnobHelper::eValueChangedReturnCodeKeyframeAdded) {
                _merge = false;
            }
        }

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

        KnobGuiPtr knob = knobCommand->_knob.lock();
        if ( (_knob.lock() != knob) || !_merge || !knobCommand->_merge || (_dimension != knobCommand->_dimension) ) {
            return false;
        }


        _newValue = knobCommand->_newValue;

        return true;
    }

private:

    int _dimension;
    std::vector<T> _oldValue;
    std::vector<T> _newValue;
    KnobGuiWPtr _knob;
    std::vector<KnobHelper::ValueChangedReturnCodeEnum> _valueChangedReturnCode;
    bool _merge;
    bool _refreshGuiFirstTime;
    bool _firstRedoCalled;
    double _timelineTime;
};


/**
 * @brief This class is used by the internal knob when it wants to group multiple edits into a single undo/redo action.
 * It is not used by the GUI
 **/
class MultipleKnobEditsUndoCommand
    : public QUndoCommand
{
    Q_DECLARE_TR_FUNCTIONS(MultipleKnobEditsUndoCommand)

private:
    struct ValueToSet
    {
        Variant newValue, oldValue;
        int dimension;
        double time;
        bool setKeyFrame;
        int setValueRetCode;
    };

    ///For each knob, the second member points to a clone of the knob before the first redo() call was made
    typedef std::map < KnobGuiWPtr, std::list<ValueToSet> >  ParamsMap;
    ParamsMap knobs;
    bool createNew;
    bool firstRedoCalled;
    ValueChangedReasonEnum _reason;

public:

    /**
     * @brief Make a new command
     * @param createNew If true this command will not merge with a previous same command
     * @param setKeyFrame if true, the command will use setValueAtTime instead of setValue in the redo() command.
     **/
    MultipleKnobEditsUndoCommand(const KnobGuiPtr& knob,
                                 ValueChangedReasonEnum reason,
                                 bool createNew,
                                 bool setKeyFrame,
                                 const Variant & value,
                                 int dimension,
                                 double time);

    virtual ~MultipleKnobEditsUndoCommand();

    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;
    virtual int id() const OVERRIDE FINAL;
    virtual bool mergeWith(const QUndoCommand *command) OVERRIDE FINAL;
    static KnobIPtr createCopyForKnob(const KnobIPtr & originalKnob);
};

struct PasteUndoCommandPrivate;
class PasteUndoCommand
    : public QUndoCommand
{
    Q_DECLARE_TR_FUNCTIONS(PasteUndoCommand)

private:
    boost::scoped_ptr<PasteUndoCommandPrivate> _imp;

public:

    PasteUndoCommand(const KnobGuiPtr& knob,
                     KnobClipBoardType type,
                     int fromDimension,
                     int targetDimension,
                     const KnobIPtr& fromKnob);

    virtual ~PasteUndoCommand();

    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;

    void copyFrom(const SERIALIZATION_NAMESPACE::KnobSerializationPtr& fromKnobSerialization, const KnobIPtr& fromKnob, bool isRedo);
};


class RestoreDefaultsCommand
    : public QUndoCommand
{
    Q_DECLARE_TR_FUNCTIONS(RestoreDefaultsCommand)

public:

    RestoreDefaultsCommand(bool isNodeReset,
                           const std::list<KnobIPtr > & knobs,
                           int targetDim,
                           QUndoCommand *parent = 0);
    virtual void undo();
    virtual void redo();

private:

    bool _isNodeReset;
    int _targetDim;
    std::list<KnobIWPtr> _knobs;
    SERIALIZATION_NAMESPACE::KnobSerializationList _serializations;
};

class SetExpressionCommand
    : public QUndoCommand
{
    Q_DECLARE_TR_FUNCTIONS(SetExpressionCommand)

public:

    SetExpressionCommand(const KnobIPtr & knob,
                         bool hasRetVar,
                         int dimension,
                         const std::string& expr,
                         QUndoCommand *parent = 0);
    virtual void undo();
    virtual void redo();

private:

    KnobIWPtr _knob;
    std::vector<std::string> _oldExprs;
    std::vector<bool> _hadRetVar;
    std::string _newExpr;
    bool _hasRetVar;
    int _dimension;
};

NATRON_NAMESPACE_EXIT;

#endif // NATRON_GUI_KNOBUNDOCOMMAND_H
