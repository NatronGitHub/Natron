/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2020 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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


#include "Global/GlobalDefines.h"

#include "Engine/Variant.h"
#include "Engine/Curve.h"
#include "Engine/TimeLine.h"
#include "Engine/Curve.h"
#include "Engine/Knob.h"
#include "Engine/EffectInstance.h"
#include "Engine/UndoCommand.h"
#include "Engine/ViewIdx.h"
#include "Engine/TimeValue.h"

NATRON_NAMESPACE_ENTER

/**
 * @brief This is the undo/redo command that any KnobGui implementation should use to push the value onto the internal knob
 **/
template<typename T>
class KnobUndoCommand
    : public UndoCommand
{
    Q_DECLARE_TR_FUNCTIONS(KnobUndoCommand)

public:

    /**
     * @brief Set the knob value for the given dimension and view
     **/
    KnobUndoCommand(const KnobIPtr& knob,
                    const T &oldValue,
                    const T &newValue,
                    DimIdx dimension,
                    ViewSetSpec view,
                    ValueChangedReasonEnum reason = eValueChangedReasonUserEdited,
                    const QString& commandName = QString())
    : UndoCommand()
    , _dimension(dimension)
    , _view(view)
    , _reason(reason)
    , _oldValue()
    , _newValue()
    , _knob(knob)
    , _valueChangedReturnCode(1)
    , _merge(true)
    , _firstRedoCalled(false)
    {
        assert(dimension >= 0 && dimension < knob->getNDimensions());
        _oldValue.push_back(oldValue);
        _newValue.push_back(newValue);

        if (!commandName.isEmpty()) {
            setText(commandName.toStdString());
        } else {
            setText( tr("Set %1").arg( QString::fromUtf8( knob->getLabel().c_str() ) ).toStdString() );
        }
    }

    /**
     * @brief Set the knob value across all dimensions for the given view
     **/
    KnobUndoCommand(const KnobIPtr& knob,
                    const std::vector<T> &oldValue,
                    const std::vector<T> &newValue,
                    ViewSetSpec view,
                    ValueChangedReasonEnum reason = eValueChangedReasonUserEdited,
                    const QString& commandName = QString())
    : UndoCommand()
    , _dimension(-1)
    , _view(view)
    , _reason(reason)
    , _oldValue(oldValue)
    , _newValue(newValue)
    , _knob(knob)
    , _valueChangedReturnCode( oldValue.size() )
    , _merge(true)
    , _firstRedoCalled(false)
    , _timelineTime(knob->getHolder()->getTimelineCurrentTime())
    {
        assert(oldValue.size() == newValue.size() && (int)oldValue.size() == knob->getNDimensions());


        if (!commandName.isEmpty()) {
            setText(commandName.toStdString());
        } else {
            setText( tr("Set %1").arg( QString::fromUtf8( knob->getLabel().c_str() ) ).toStdString() );
        }

    }

    virtual ~KnobUndoCommand() OVERRIDE
    {
    }

private:
    
    virtual void undo() OVERRIDE FINAL
    {

        boost::shared_ptr<Knob<T> > knob = boost::dynamic_pointer_cast<Knob<T> >(_knob.lock());
        if (!knob) {
            return;
        }


        assert( (int)_oldValue.size() == knob->getNDimensions() || !_dimension.isAll() );

        // wrap changes in a begin/end because we might do 2 API calls with deleteValueAtTime and setValue
        ScopedChanges_RAII changes(knob.get());

        // If any keyframe was added due to auto-keying, remove it
        for (std::size_t i = 0; i < _oldValue.size(); ++i) {
            if (_dimension.isAll() || (int)i == _dimension) {
                if (_valueChangedReturnCode[i] == eValueChangedReturnCodeKeyframeAdded) {
                    knob->deleteValueAtTime(_timelineTime, _view, DimIdx(i), eValueChangedReasonUserEdited);
                }
            }
        }

        // Ensure that no keyframe gets added by the setValue
        knob->setAutoKeyingEnabled(false);
        if (_dimension.isAll()) {
            // Multiple dimensions set at once
            knob->setValueAcrossDimensions(_oldValue, DimIdx(0), _view, _reason);
        } else {
            // Only one dimension set
            ignore_result(knob->setValue(_oldValue[0], _view, _dimension, _reason));

        }

        knob->setAutoKeyingEnabled(true);

    } // undo

    virtual void redo() OVERRIDE FINAL
    {
        boost::shared_ptr<Knob<T> > knob = boost::dynamic_pointer_cast<Knob<T> >(_knob.lock());
        if (!knob) {
            return;
        }


        assert( (int)_oldValue.size() == knob->getNDimensions() || !_dimension.isAll()  );


        if (_dimension.isAll() && (int)_newValue.size() > 1) {
            // Multiple dimensions set at once
            knob->setValueAcrossDimensions(_newValue, DimIdx(0), _view, _reason, &_valueChangedReturnCode);
        } else {
            // Only one dimension set
            _valueChangedReturnCode[0] = knob->setValue(_newValue[0], _view, _dimension, _reason);
        }


        // If we added a keyframe, prevent this command to merge with any other command
        for (std::size_t i = 0; i < _valueChangedReturnCode.size(); ++i) {
            if (_valueChangedReturnCode[i] == eValueChangedReturnCodeKeyframeAdded) {
                _merge = false;
            }
        }

    } // redo

    virtual bool mergeWith(const UndoCommandPtr& command) OVERRIDE FINAL
    {
        const KnobUndoCommand *knobCommand = dynamic_cast<const KnobUndoCommand *>(command.get());

        if (!knobCommand) {
            return false;
        }

        KnobIPtr knob = knobCommand->_knob.lock();
        if ((_knob.lock() != knob) || !_merge || !knobCommand->_merge || (_dimension != knobCommand->_dimension) || (_view != knobCommand->_view)) {
            return false;
        }


        _newValue = knobCommand->_newValue;

        return true;
    }

private:

    void refreshAnimationModuleSelectedKeyframe()
    {

    }

    // If all, _oldValue.size() and _newValue.size() is expected to be of the number of
    // dimensions of the knob
    DimSpec _dimension;

    // The view to modify
    ViewSetSpec _view;

    // The reason to call setValue with
    ValueChangedReasonEnum _reason;

    // The raw values
    std::vector<T> _oldValue;
    std::vector<T> _newValue;

    // Ptr to the knob
    KnobIWPtr _knob;

    // Remember for each dimension the setValue function status code
    // to check if undo we need to remove keyframes
    std::vector<ValueChangedReturnCodeEnum> _valueChangedReturnCode;

    // If false we prevent this command from merging with new commands on the same knob/dimension/view
    bool _merge;

    // True once redo() has been called once
    bool _firstRedoCalled;

    // The timeline time at which initially we called redo() the first time
    TimeValue _timelineTime;
};

// Equivalent of KnobUndoCommand but for a multi-choice knob. this is needed because it doesn't use the setvalue/getvalue API
class SetMultiChoiceKnobValueUndoCommand : public UndoCommand
{

    Q_DECLARE_TR_FUNCTIONS(SetMultiChoiceKnobValueUndoCommand)

public:

    SetMultiChoiceKnobValueUndoCommand(const KnobChoicePtr& knob,
                                       const std::vector<ChoiceOption> &oldSelection,
                                       const std::vector<ChoiceOption> &newSelection,
                                       ViewSetSpec view,
                                       ValueChangedReasonEnum reason = eValueChangedReasonUserEdited);

    virtual ~SetMultiChoiceKnobValueUndoCommand();

    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;
    virtual bool mergeWith(const UndoCommandPtr& command) OVERRIDE FINAL;

private:

    KnobChoiceWPtr _knob;

    ViewSetSpec _view;

    ValueChangedReasonEnum _reason;

    std::vector<ChoiceOption> _oldIndices, _newIndices;

    // The timeline time at which initially we called redo() the first time
    TimeValue _timelineTime;
};

/**
 * @brief This class is used by the internal knob when it wants to group multiple edits into a single undo/redo action.
 * It is not used by the GUI
 **/
class MultipleKnobEditsUndoCommand
    : public UndoCommand
{
    Q_DECLARE_TR_FUNCTIONS(MultipleKnobEditsUndoCommand)

public:
    

    struct ValueToSet
    {
        // Since knobs with different types may be grouped here, we need to use variants
        KeyFrame newValue;
        PerDimViewKeyFramesMap oldValues;
        DimSpec dimension;
        ViewSetSpec view;
        bool setKeyFrame;
        ValueChangedReturnCodeEnum setValueRetCode;
        ValueChangedReasonEnum reason;
    };

    
    // For each knob, the second member points to a clone of the knob before the first redo() call was made
    typedef std::map <KnobIWPtr, std::list<ValueToSet> >  ParamsMap;

private:

    ParamsMap knobs;
    bool createNew;
    bool firstRedoCalled;

public:

    /**
     * @brief Create or append a setValue action to the undo/Redo command.
     * This should be called once the setValue call has been done. The first redo()
     * call won't do anything.
     *
     * @param knob The knob on which the setValue call is made
     * @param commandName The name of the command as displayed in the "Edit" menu
     * @param reason the reason given to the corresponding setValue call
     * @param setValueRetCode The return code given by setValue the first time it was called.
     * @param createNew If true this command will not merge with a previous command and create a new one
     * @param setKeyFrame If true, the command will use setValueAtTime instead of setValue in the redo() command.
     * @param value The value set in the corresponding setValue call
     * @param dimension The dimension argument passed to setValue
     * @param view The view argument passed to setValue
     **/
    MultipleKnobEditsUndoCommand(const KnobIPtr& knob,
                                 const QString& commandName,
                                 ValueChangedReasonEnum reason,
                                 ValueChangedReturnCodeEnum setValueRetCode,
                                 bool createNew,
                                 bool setKeyFrame,
                                 const PerDimViewKeyFramesMap& oldValue,
                                 const KeyFrame & newValue,
                                 DimSpec dimension,
                                 ViewSetSpec view);

    virtual ~MultipleKnobEditsUndoCommand();

    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;
    virtual bool mergeWith(const UndoCommandPtr& command) OVERRIDE FINAL;
};

struct PasteKnobClipBoardUndoCommandPrivate;
class PasteKnobClipBoardUndoCommand
    : public UndoCommand
{
    Q_DECLARE_TR_FUNCTIONS(PasteKnobClipBoardUndoCommand)

private:
    boost::scoped_ptr<PasteKnobClipBoardUndoCommandPrivate> _imp;

public:

    PasteKnobClipBoardUndoCommand(const KnobIPtr& knob,
                     KnobClipBoardType type,
                     DimSpec fromDimension,
                     DimSpec targetDimension,
                     ViewSetSpec fromView,
                     ViewSetSpec targetView,
                     const KnobIPtr& fromKnob);

    virtual ~PasteKnobClipBoardUndoCommand();

    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;

    static std::string makeLinkExpression(const std::vector<std::string>& projectViewNames,
                                          const KnobIPtr& targetKnob,
                                          bool multCurve,
                                          ExpressionLanguageEnum language,
                                          const KnobIPtr& fromKnob,
                                          DimSpec fromDimension,
                                          ViewSetSpec fromView,
                                          DimSpec targetDimension,
                                          ViewSetSpec targetView);

    void copyFrom(const SERIALIZATION_NAMESPACE::KnobSerializationPtr& fromKnobSerialization, const KnobIPtr& fromKnob, bool isRedo);
};


class RestoreDefaultsCommand
    : public UndoCommand
{
    Q_DECLARE_TR_FUNCTIONS(RestoreDefaultsCommand)

public:

    RestoreDefaultsCommand(const std::list<KnobIPtr> & knobs,
                           DimSpec targetDim,
                           ViewSetSpec targetView);
    virtual void undo();
    virtual void redo();

private:

    DimSpec _targetDim;
    ViewSetSpec _targetView;
    std::list<KnobIWPtr> _knobs;
    SERIALIZATION_NAMESPACE::KnobSerializationList _serializations;
};

class SetExpressionCommand
    : public UndoCommand
{
    Q_DECLARE_TR_FUNCTIONS(SetExpressionCommand)

public:

    SetExpressionCommand(const KnobIPtr & knob,
                         ExpressionLanguageEnum language,
                         bool hasRetVar,
                         DimSpec dimension,
                         ViewSetSpec view,
                         const std::string& expr);
    virtual void undo();
    virtual void redo();

    struct Expr
    {
        std::string expression;
        ExpressionLanguageEnum language;
        bool hasRetVar;
    };

    typedef std::map<DimensionViewPair, Expr, DimensionViewPairCompare> PerDimViewExprMap;

private:


    KnobIWPtr _knob;
    PerDimViewExprMap _oldExprs;
    std::string _newExpr;
    ExpressionLanguageEnum _newLang;
    bool _hasRetVar;
    DimSpec _dimension;
    ViewSetSpec _view;
};

NATRON_NAMESPACE_EXIT

#endif // NATRON_GUI_KNOBUNDOCOMMAND_H
