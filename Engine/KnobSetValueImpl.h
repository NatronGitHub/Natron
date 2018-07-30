/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#ifndef KNOBSETVALUEIMPL_H
#define KNOBSETVALUEIMPL_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include "Knob.h"

#include <QThread>
#include <QCoreApplication>

#include "Engine/EngineFwd.h"
#include "Engine/KnobItemsTable.h"
#include "Engine/Node.h"
#include "Engine/EffectInstance.h"
#include "Engine/KnobUndoCommand.h"

NATRON_NAMESPACE_ENTER

template <typename T>
AddToUndoRedoStackHelper<T>::AddToUndoRedoStackHelper(Knob<T>* k)
: _knob(k)
, _mustEndEdits(false)
, _isUndoRedoStackOpened(false)
{
    _holder = k->getHolder();
    if (_holder && QThread::currentThread() == qApp->thread()) {
        KnobTableItemPtr isTableItem = toKnobTableItem(_holder);
        // Use the effect undo stack if this is a table item
        if (isTableItem) {
            KnobItemsTablePtr model = isTableItem->getModel();
            if (model) {
                _holder = model->getNode()->getEffectInstance();
            }
        }
        KnobHolder::MultipleParamsEditEnum paramEditLevel = _holder->getMultipleEditsLevel();

        // If we are under an overlay interact, the plug-in is going to do setValue calls, make sure the user can undo/redo
        if (paramEditLevel == KnobHolder::eMultipleParamsEditOff && _holder->isDoingInteractAction()) {
            _holder->beginMultipleEdits(KnobHelper::tr("%1 changed").arg(QString::fromUtf8(k->getName().c_str())).toStdString());
            _mustEndEdits = true;
            _isUndoRedoStackOpened = true;
        } else if (paramEditLevel != KnobHolder::eMultipleParamsEditOff) {
            _isUndoRedoStackOpened = true;
        }
    }
}

template <typename T>
bool
AddToUndoRedoStackHelper<T>::canAddValueToUndoStack() const
{
    return _isUndoRedoStackOpened;
}

template <typename T>
AddToUndoRedoStackHelper<T>::~AddToUndoRedoStackHelper()
{
    if (_mustEndEdits) {
        _holder->endMultipleEdits();
    }
}

template <typename T>
void
AddToUndoRedoStackHelper<T>::prepareOldValueToUndoRedoStack(ViewSetSpec view, DimSpec dimension, TimeValue time, ValueChangedReasonEnum reason, bool setKeyFrame)
{
    // Should be checked first before calling this function.
    assert(canAddValueToUndoStack());
    if (!_holder) {
        return;
    }

    // Ensure the state has not been changed while this object is alive.
    assert(_holder->getMultipleEditsLevel() == KnobHolder::eMultipleParamsEditOnCreateNewCommand ||
           _holder->getMultipleEditsLevel() == KnobHolder::eMultipleParamsEditOn);

    ValueToPush data;
    data.view = view;
    data.dimension = dimension;
    data.reason = reason;
    data.setKeyframe = setKeyFrame;

    ValueKnobDimView<T>* valueData = dynamic_cast<ValueKnobDimView<T>*>(_knob->getDataForDimView(DimIdx(0), ViewIdx(0)).get());
    assert(valueData);

    int nDims = _knob->getNDimensions();
    std::list<ViewIdx> allViews = _knob->getViewsList();
    for (std::list<ViewIdx>::const_iterator it = allViews.begin(); it!=allViews.end(); ++it) {
        if (!view.isAll() && *it != view) {
            continue;
        }
        for (int i = 0; i < nDims; ++i) {
            if (!dimension.isAll() && i != dimension) {
                continue;
            }
            DimensionViewPair k = {DimIdx(i), *it};
            T oldValue = _knob->getValueAtTime(time, DimIdx(i), *it);
            data.oldValues[k].push_back(valueData->makeKeyFrame(time, oldValue));
        }
    }


    _valuesToPushQueue.push_back(data);
} // prepareOldValueToUndoRedoStack

template <typename T>
void
AddToUndoRedoStackHelper<T>::addSetValueToUndoRedoStackIfNeeded(const KeyFrame& value, ValueChangedReturnCodeEnum setValueRetCode)
{

    // Should be checked first before calling this function.
    assert(canAddValueToUndoStack());


    if (!_holder) {
        return;
    }

    // Ensure the state has not been changed while this object is alive.
    assert(_holder->getMultipleEditsLevel() == KnobHolder::eMultipleParamsEditOnCreateNewCommand ||
           _holder->getMultipleEditsLevel() == KnobHolder::eMultipleParamsEditOn);

    // prepareOldValueToUndoRedoStack must have been called.
    assert(!_valuesToPushQueue.empty());

    const ValueToPush& data = _valuesToPushQueue.back();

    bool createNewCommand = _holder->getMultipleEditsLevel() == KnobHolder::eMultipleParamsEditOnCreateNewCommand;
    QString commandName = QString::fromUtf8(_holder->getCurrentMultipleEditsCommandName().c_str());
    _holder->pushUndoCommand( new MultipleKnobEditsUndoCommand(_knob->shared_from_this(), commandName, data.reason, setValueRetCode, createNewCommand, data.setKeyframe, data.oldValues, value, data.dimension, data.view) );


}

template <typename T>
bool
ValueKnobDimView<T>::setValueAndCheckIfChanged(const T& v)
{
    QMutexLocker k(&valueMutex);
    if (value != v) {
        value = v;
        return true;
    }
    return false;
}

template<typename T>
ValueChangedReturnCodeEnum
Knob<T>::setValueFromKeyFrame(const SetKeyFrameArgs& args, const KeyFrame & k)
{

    ValueKnobDimView<T>* data = dynamic_cast<ValueKnobDimView<T>*>(getDataForDimView(DimIdx(0), ViewIdx(0)).get());
    T value = data->getValueFromKeyFrame(k);
    return setValue(value, args.view, args.dimension, args.reason, args.callKnobChangedHandlerEvenIfNothingChanged);
}


template <typename T>
ValueChangedReturnCodeEnum
Knob<T>::setValue(const T & v,
                  ViewSetSpec view,
                  DimSpec dimension,
                  ValueChangedReasonEnum reason,
                  bool forceHandlerEvenIfNoChange)
{


    KnobHolderPtr holder = getHolder();

#ifdef DEBUG
    // Check that setValue is possible in this context (only in debug mode)
    // Some actions (e.g: render) do not allow setting values in OpenFX spec.
    if ( holder && (reason == eValueChangedReasonPluginEdited) ) {
        EffectInstancePtr isEffect = toEffectInstance(holder);
        if (isEffect) {
            isEffect->checkCanSetValueAndWarn();
        }
    }
#endif

    // Apply changes
    bool hasChanged = forceHandlerEvenIfNoChange;

    TimeValue time = getCurrentRenderTime();

    // Check if we can add automatically a new keyframe
    if (isAutoKeyingEnabled(dimension, time, view, reason)) {
        return setValueAtTime(time, v, view, dimension, reason);
    }

    // Check if we need to use the undo/redo stack
    AddToUndoRedoStackHelper<T> undoRedoStackHelperRAII(this);
    if (undoRedoStackHelperRAII.canAddValueToUndoStack()) {
        undoRedoStackHelperRAII.prepareOldValueToUndoRedoStack(view, dimension, time, reason, false /*setKeyframe*/);
    }

    // Actually push the value to the data for each dimension/view requested
    {
        std::list<ViewIdx> views = getViewsList();
        int nDims = getNDimensions();
        ViewIdx view_i;
        if (!view.isAll()) {
            view_i = checkIfViewExistsOrFallbackMainView(ViewIdx(view));
        }
        for (std::list<ViewIdx>::const_iterator it = views.begin(); it!=views.end(); ++it) {
            if (!view.isAll()) {
                if (view_i != *it) {
                    continue;
                }
            }

            DimSpec thisDimension = dimension;
            // If the item has its dimensions folded and the user modifies dimension 0, also modify other dimensions
            if (reason == eValueChangedReasonUserEdited && thisDimension == 0 && !getAllDimensionsVisible(*it)) {
                thisDimension = DimSpec::all();
            }


            for (int i = 0; i < nDims; ++i) {
                if (!thisDimension.isAll() && thisDimension != i) {
                    continue;
                }


                ValueKnobDimView<T>* data = dynamic_cast<ValueKnobDimView<T>*>(getDataForDimView(DimIdx(i), *it).get());
                if (data) {
                    hasChanged |= data->setValueAndCheckIfChanged(v);
                }
                // Auto-expand the parameter dimensions if needed
                if (hasChanged) {
                    // If dimensions are folded but a setValue call is made on one of them, expand them
                    if (((thisDimension.isAll() && i == nDims - 1) || (!thisDimension.isAll() && i == thisDimension)) && nDims > 1) {
                        autoAdjustFoldExpandDimensions(*it);
                    }
                }

            }
        }

    }


    ValueChangedReturnCodeEnum ret;
    if (hasChanged) {
        ret = eValueChangedReturnCodeNoKeyframeAdded;
    } else {
        ret = eValueChangedReturnCodeNothingChanged;
    }


    if (undoRedoStackHelperRAII.canAddValueToUndoStack()) {
        ValueKnobDimView<T>* data = dynamic_cast<ValueKnobDimView<T>*>(getDataForDimView(DimIdx(0), ViewIdx(0)).get());
        KeyFrame k = data->makeKeyFrame(time, v);
        undoRedoStackHelperRAII.addSetValueToUndoRedoStackIfNeeded(k, ret);
    }

    // Evaluate the change
    if (ret != eValueChangedReturnCodeNothingChanged) {
        evaluateValueChange(dimension, time, view, reason);
    }

    return ret;


} // setValue


template <typename T>
void
Knob<T>::setValueAcrossDimensions(const std::vector<T>& values,
                                  DimIdx dimensionStartOffset,
                                  ViewSetSpec view,
                                  ValueChangedReasonEnum reason,
                                  std::vector<ValueChangedReturnCodeEnum>* retCodes)
{
    if (values.empty()) {
        return;
    }
    int nDims = getNDimensions();
    assert(nDims >= (int)values.size());
    if (dimensionStartOffset < 0 || dimensionStartOffset + (int)values.size() > nDims) {
        throw std::invalid_argument("Knob<T>::setValues: Invalid arguments");
    }
    
    KnobHolderPtr holder = getHolder();

    // Group value changes under the same undo/redo command if possible
    AddToUndoRedoStackHelper<T> undoRedoStackHelperRAII(this);

    ValueChangedReturnCodeEnum ret;
    bool hasChanged = false;
    if (values.size() > 1) {

        // Make sure we evaluate once
        beginChanges();

        // Make sure we call knobChanged handler and refresh the GUI once
        blockValueChanges();

        // Since we are about to set multiple values, disable auto-expand/fold until we set the last value
        setAdjustFoldExpandStateAutomatically(false);
    }

    if (retCodes) {
        retCodes->resize(values.size());
    }

    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i == values.size() - 1 && values.size() > 1) {
            // Make sure the last setValue call triggers a knobChanged call and refreshes the gui
            unblockValueChanges();
            setAdjustFoldExpandStateAutomatically(true);
        }
        ret = setValue(values[i], view, DimSpec(dimensionStartOffset + i), reason, hasChanged);
        if (retCodes) {
            (*retCodes)[i] = ret;
        }
        hasChanged |= (ret != eValueChangedReturnCodeNothingChanged);
    }
    if (values.size() > 1) {
        endChanges();
    }


} // setValueAcrossDimensions

template <>
KeyFrame
ValueKnobDimView<std::string>::makeKeyFrame(TimeValue time, const std::string& value)
{
    KeyFrame k(time, 0.);
    k.setProperty(kKeyFramePropString, value, 0, false /*failIfnotExist*/);
    return k;
}

template <typename T>
KeyFrame
ValueKnobDimView<T>::makeKeyFrame(TimeValue time, const T& value)
{
    KeyFrame k(time, value);
    return k;
}

template <typename T>
KeyFrame
Knob<T>::makeKeyFrame(TimeValue time, const T& value, DimIdx dimension, ViewIdx view) const
{
    ValueKnobDimView<T>* data = dynamic_cast<ValueKnobDimView<T>*>(getDataForDimView(dimension, view).get());
    assert(data);
    return data->makeKeyFrame(time, value);
}



template<typename T>
ValueChangedReturnCodeEnum
Knob<T>::setValueAtTime(TimeValue time,
                        const T & v,
                        ViewSetSpec view,
                        DimSpec dimension,
                        ValueChangedReasonEnum reason,
                        bool forceHandlerEvenIfNoChange)
{

    SetKeyFrameArgs args;
    args.view = view;
    args.dimension = dimension;
    args.reason = reason;
    args.callKnobChangedHandlerEvenIfNothingChanged = forceHandlerEvenIfNoChange;

    KeyFrame k = makeKeyFrame(time, v, DimIdx(0), ViewIdx(0));
    return setKeyFrame(args, k);

} // setValueAtTime



template<typename T>
ValueChangedReturnCodeEnum
Knob<T>::setKeyFrame(const SetKeyFrameArgs& args, const KeyFrame & key)
{
    // If no animated, do not even set a keyframe
    // Currently parametric knobs only have the same API than traditional keyframes to manage their control points
    KnobParametric* parametricKnob = dynamic_cast<KnobParametric*>(this);
    if (( !canAnimate() || !isAnimationEnabled()) && !parametricKnob) {
        return setValueFromKeyFrame(args, key);
    }

    KnobHolderPtr holder = getHolder();

#ifdef DEBUG
    // Check that setValue is possible in this context (only in debug mode)
    // Some actions (e.g: render) do not allow setting values in OpenFX spec.
    if ( holder && (args.reason == eValueChangedReasonPluginEdited) ) {
        EffectInstancePtr isEffect = toEffectInstance(holder);
        if (isEffect) {
            isEffect->checkCanSetValueAndWarn();
        }
    }
#endif

    AddToUndoRedoStackHelper<T> undoRedoStackHelperRAII(this);
    if (undoRedoStackHelperRAII.canAddValueToUndoStack()) {
        undoRedoStackHelperRAII.prepareOldValueToUndoRedoStack(args.view, args.dimension, key.getTime(), args.reason, true /*setKeyframe*/);
    }


    ValueChangedReturnCodeEnum ret = !args.callKnobChangedHandlerEvenIfNothingChanged ? eValueChangedReturnCodeNothingChanged : eValueChangedReturnCodeKeyframeModified;

    {
        std::list<ViewIdx> views = getViewsList();
        int nDims = getNDimensions();
        ViewIdx view_i;
        if (!args.view.isAll()) {
            view_i = checkIfViewExistsOrFallbackMainView(ViewIdx(args.view));
        }
        for (std::list<ViewIdx>::const_iterator it = views.begin(); it!=views.end(); ++it) {
            if (!args.view.isAll()) {
                if (view_i != *it) {
                    continue;
                }
            }

            DimSpec thisDimension = args.dimension;
            // If the item has its dimensions folded and the user modifies dimension 0, also modify other dimensions
            if (args.reason == eValueChangedReasonUserEdited && thisDimension == 0 && !getAllDimensionsVisible(*it)) {
                thisDimension = DimSpec::all();
            }

            for (int i = 0; i < nDims; ++i) {
                if (!thisDimension.isAll() && thisDimension != i) {
                    continue;
                }

                KnobDimViewBasePtr data = getDataForDimView(DimIdx(i), *it);
                assert(data);
                ValueChangedReturnCodeEnum addKeyRet = data->setKeyFrame(key, args.flags);
                if (addKeyRet == eValueChangedReturnCodeKeyframeAdded) {
                    ret = addKeyRet;
                } else if (addKeyRet == eValueChangedReturnCodeKeyframeModified) {
                    if (ret == eValueChangedReturnCodeNothingChanged) {
                        ret = eValueChangedReturnCodeKeyframeModified;
                    }
                }

                // If dimensions are folded but a setValue call is made on one of them, auto-compute fold/expand
                if (((thisDimension.isAll() && i == nDims - 1) || (!thisDimension.isAll() && i == thisDimension)) && nDims > 1) {
                    autoAdjustFoldExpandDimensions(*it);
                }
            }
        }

    }


    // Refresh holder animation flag
    if (holder) {
        holder->setHasAnimation(true);
    }


    if (ret != eValueChangedReturnCodeNothingChanged) {

        if (undoRedoStackHelperRAII.canAddValueToUndoStack()) {
            undoRedoStackHelperRAII.addSetValueToUndoRedoStackIfNeeded(key, ret);
        }
        
        evaluateValueChange(args.dimension, key.getTime(), args.view, args.reason);
    }
    
    return ret;
} // setKeyFrame


template <typename T>
void
Knob<T>::setMultipleKeyFrames(const SetKeyFrameArgs& args, const std::list<KeyFrame>& keys)
{
    if (keys.empty()) {
        return;
    }

    if (keys.size() > 1) {
        // Make sure we evaluate once
        beginChanges();

        // Make sure we call knobChanged handler and refresh the GUI once
        blockValueChanges();

        // Since we are about to set multiple values, disable auto-expand/fold until we set the last value
        setAdjustFoldExpandStateAutomatically(false);
    }

    // Group changes under the same undo/redo action if possible
    AddToUndoRedoStackHelper<T> undoRedoStackHelperRAII(this);
    
    typename std::list<KeyFrame>::const_iterator next = keys.begin();
    ++next;
    KeyFrame k;
    ValueChangedReturnCodeEnum ret;

    bool hasChanged = false;
    for (std::list<KeyFrame>::const_iterator it = keys.begin(); it!= keys.end(); ++it) {
        if (next == keys.end() && keys.size() > 1) {
            // Make sure the last setValue call triggers a knobChanged call
            unblockValueChanges();

            setAdjustFoldExpandStateAutomatically(true);
            
        }
        SetKeyFrameArgs subArgs = args;
        subArgs.callKnobChangedHandlerEvenIfNothingChanged = hasChanged;
        ret = setKeyFrame(subArgs, *it);
        hasChanged |= (ret != eValueChangedReturnCodeNothingChanged);

        if (next != keys.end()) {
            ++next;
        }
    }

    if (keys.size() > 1) {
        endChanges();
    }

} // setMultipleKeyFrames

template <typename T>
void
Knob<T>::setKeyFramesAcrossDimensions(const SetKeyFrameArgs& args, const std::vector<KeyFrame>& values, DimIdx dimensionStartOffset, std::vector<ValueChangedReturnCodeEnum>* retCodes)
{
    if (values.empty()) {
        return;
    }
    int nDims = getNDimensions();
    assert(nDims >= (int)values.size());
    if (dimensionStartOffset < 0 || dimensionStartOffset + (int)values.size() > nDims) {
        throw std::invalid_argument("Knob<T>::setValuesAtTime: Invalid arguments");
    }

    // Group changes under the same undo/redo action if possible
    AddToUndoRedoStackHelper<T> undoRedoStackHelperRAII(this);

    KeyFrame newKey;
    ValueChangedReturnCodeEnum ret;
    bool hasChanged = false;

    if (values.size() > 1) {
        // Make sure we evaluate once
        beginChanges();

        // Make sure we call knobChanged handler and refresh the GUI once
        blockValueChanges();

        // Since we are about to set multiple values, disable auto-expand/fold until we set the last value
        setAdjustFoldExpandStateAutomatically(false);

    }
    if (retCodes) {
        retCodes->resize(values.size());
    }
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i == values.size() - 1 && values.size() > 1) {
            unblockValueChanges();
            setAdjustFoldExpandStateAutomatically(true);
        }

        SetKeyFrameArgs subArgs = args;
        subArgs.dimension = DimSpec(dimensionStartOffset + i);
        subArgs.callKnobChangedHandlerEvenIfNothingChanged = hasChanged;
        ret = setKeyFrame(subArgs, values[i]);
        if (retCodes) {
            (*retCodes)[i] = ret;
        }
        hasChanged |= (ret != eValueChangedReturnCodeNothingChanged);
    }
    if (values.size() > 1) {
        endChanges();
    }

} // setKeyFramesAcrossDimensions

template <typename T>
void
Knob<T>::setValueAtTimeAcrossDimensions(TimeValue time,
                                        const std::vector<T>& values,
                                        DimIdx dimensionStartOffset,
                                        ViewSetSpec view,
                                        ValueChangedReasonEnum reason,
                                        std::vector<ValueChangedReturnCodeEnum>* retCodes)
{
    ValueKnobDimView<T>* data = dynamic_cast<ValueKnobDimView<T>*>(getDataForDimView(DimIdx(0), ViewIdx(0)).get());
    assert(data);
    SetKeyFrameArgs args;
    std::vector<KeyFrame> keys(values.size());
    for (std::size_t i = 0; i < values.size(); ++i) {
        keys[i] = data->makeKeyFrame(time, values[i]);
    }
    args.view = view;
    args.reason = reason;
    setKeyFramesAcrossDimensions(args, keys, dimensionStartOffset, retCodes);
} // setValueAtTimeAcrossDimensions



template <typename T>
void
Knob<T>::resetToDefaultValue(DimSpec dimension, ViewSetSpec view)
{
    removeAnimation(view, dimension, eValueChangedReasonRestoreDefault);


    // Prevent clearExpression from triggereing evaluateValueChange with a reason different than eValueChangedReasonRestoreDefault
    blockValueChanges();
    clearExpression(dimension, view);
    resetExtraToDefaultValue(dimension, view);
    unblockValueChanges();

    int nDims = getNDimensions();
    std::vector<T> defValues(nDims);
    {
        QMutexLocker l(&_data->defaultValueMutex);
        for (int i = 0; i < nDims; ++i) {
            if (dimension.isAll() || i == dimension) {
                defValues[i] = _data->defaultValues[i].value;
            }
        }
    }


    if (dimension.isAll()) {
        setValueAcrossDimensions(defValues, DimIdx(0), view, eValueChangedReasonRestoreDefault);
    } else {
        ignore_result( setValue(defValues[dimension], view, dimension, eValueChangedReasonRestoreDefault, NULL) );
    }


    if (view.isAll()) {
        std::list<ViewIdx> views = getViewsList();
        for (std::list<ViewIdx>::const_iterator it = views.begin(); it!=views.end(); ++it) {
            autoAdjustFoldExpandDimensions(*it);
        }
    } else {
        ViewIdx view_i = checkIfViewExistsOrFallbackMainView(ViewIdx(view));
        autoAdjustFoldExpandDimensions(view_i);
    }

}

template<>
void
KnobDoubleBase::resetToDefaultValue(DimSpec dimension, ViewSetSpec view)
{
    removeAnimation(view, dimension, eValueChangedReasonRestoreDefault);

    ///A KnobDoubleBase is not always a KnobDouble (it can also be a KnobColor)
    KnobDouble* isDouble = dynamic_cast<KnobDouble*>(this);

    clearExpression(dimension, view);


    resetExtraToDefaultValue(dimension, view);

    TimeValue time = getCurrentRenderTime();

    // Double default values may be normalized so denormalize it
    // see http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#kOfxParamPropDefaultCoordinateSystem
    int nDims = getNDimensions();
    std::vector<double> defValues(nDims);
    for (int i = 0; i < nDims; ++i) {
        if (dimension.isAll() || i == dimension) {
            {
                QMutexLocker l(&_data->defaultValueMutex);
                defValues[i] = _data->defaultValues[i].value;
            }

            if (isDouble) {

                if ( isDouble->getDefaultValuesAreNormalized() ) {
                    if (isDouble->getValueIsNormalized(DimIdx(i)) == eValueIsNormalizedNone) {
                        // default is normalized, value is non-normalized: denormalize it!
                        defValues[i] = isDouble->denormalize(DimIdx(i), time, defValues[i]);
                    }
                } else {
                    if (isDouble->getValueIsNormalized(DimIdx(i)) != eValueIsNormalizedNone) {
                        // default is non-normalized, value is normalized: normalize it!
                        defValues[i] = isDouble->normalize(DimIdx(i), time, defValues[i]);
                    }
                }
            } // isDouble

        }
    }

    if (dimension.isAll()) {
        setValueAcrossDimensions(defValues, DimIdx(0), view, eValueChangedReasonRestoreDefault);
    } else {
        ignore_result( setValue(defValues[dimension], view, dimension, eValueChangedReasonRestoreDefault, NULL) );
    }


    if (view.isAll()) {
        std::list<ViewIdx> views = getViewsList();
        for (std::list<ViewIdx>::const_iterator it = views.begin(); it!=views.end(); ++it) {
            autoAdjustFoldExpandDimensions(*it);
        }
    } else {
        ViewIdx view_i = checkIfViewExistsOrFallbackMainView(ViewIdx(view));
        autoAdjustFoldExpandDimensions(view_i);
    }
}




NATRON_NAMESPACE_EXIT

#endif // KNOBSETVALUEIMPL_H
