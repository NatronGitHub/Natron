/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2017 INRIA and Alexandre Gauthier-Foichat
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

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

template <typename T>
AddToUndoRedoStackHelper<T>::AddToUndoRedoStackHelper(Knob<T>* k)
: _knob(k)
, _mustEndEdits(false)
, _isUndoRedoStackOpened(false)
{
    KnobHolderPtr holder = k->getHolder();
    if (holder) {
        KnobHolder::MultipleParamsEditEnum paramEditLevel = holder->getMultipleEditsLevel();

        // If we are under an overlay interact, the plug-in is going to do setValue calls, make sure the user can undo/redo
        if (paramEditLevel == KnobHolder::eMultipleParamsEditOff && holder->isDoingInteractAction()) {
            holder->beginMultipleEdits(k->tr("%1 changed").arg(QString::fromUtf8(k->getName().c_str())).toStdString());
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
        _knob->getHolder()->endMultipleEdits();
    }
}

template <typename T>
void
AddToUndoRedoStackHelper<T>::prepareOldValueToUndoRedoStack(ViewSetSpec view, DimSpec dimension, TimeValue time, ValueChangedReasonEnum reason, bool setKeyFrame)
{
    // Should be checked first before calling this function.
    assert(canAddValueToUndoStack());

    // Ensure the state has not been changed while this object is alive.
    assert(_knob->getHolder()->getMultipleEditsLevel() == KnobHolder::eMultipleParamsEditOnCreateNewCommand ||
           _knob->getHolder()->getMultipleEditsLevel() == KnobHolder::eMultipleParamsEditOn);

    ValueToPush data;
    data.view = view;
    data.dimension = dimension;
    data.time = time;
    data.reason = reason;
    data.setKeyframe = setKeyFrame;

    int nDims = _knob->getNDimensions();
    std::list<ViewIdx> allViews = _knob->getViewsList();
    if (dimension.isAll()) {
        for (int i = 0; i < nDims; ++i) {
            if (view.isAll()) {
                for (std::list<ViewIdx>::const_iterator it = allViews.begin(); it!=allViews.end(); ++it) {
                    DimensionViewPair k = {DimIdx(i), *it};
                    Variant& p = data.oldValues[k];
                    p.setValue(_knob->getValueAtTime(time, DimIdx(i), *it));
                }
            } else {
                ViewIdx view_i = _knob->getViewIdxFromGetSpec(ViewIdx(view));
                DimensionViewPair k = {DimIdx(i), view_i};
                Variant& p = data.oldValues[k];
                p.setValue(_knob->getValueAtTime(time, DimIdx(i), view_i));
            }
        }

    } else {
        if (view.isAll()) {
            for (std::list<ViewIdx>::const_iterator it = allViews.begin(); it!=allViews.end(); ++it) {
                DimensionViewPair k = {DimIdx(dimension), *it};
                Variant& p = data.oldValues[k];
                p.setValue(_knob->getValueAtTime(time, DimIdx(dimension), *it));
            }
        } else {
            ViewIdx view_i = _knob->getViewIdxFromGetSpec(ViewIdx(view));
            DimensionViewPair k = {DimIdx(dimension), view_i};
            Variant& p = data.oldValues[k];
            p.setValue(_knob->getValueAtTime(time, DimIdx(dimension), view_i));
        }
    }

    _valuesToPushQueue.push_back(data);
} // prepareOldValueToUndoRedoStack

template <typename T>
void
AddToUndoRedoStackHelper<T>::addSetValueToUndoRedoStackIfNeeded(const T& value, ValueChangedReturnCodeEnum setValueRetCode)
{

    // Should be checked first before calling this function.
    assert(canAddValueToUndoStack());

    // Ensure the state has not been changed while this object is alive.
    assert(_knob->getHolder()->getMultipleEditsLevel() == KnobHolder::eMultipleParamsEditOnCreateNewCommand ||
           _knob->getHolder()->getMultipleEditsLevel() == KnobHolder::eMultipleParamsEditOn);

    // prepareOldValueToUndoRedoStack must have been called.
    assert(!_valuesToPushQueue.empty());

    const ValueToPush& data = _valuesToPushQueue.back();

    // Add the set value using the undo/redo stack
    Variant newVariant;
    newVariant.setValue(value);

    _knob->getSignalSlotHandler()->s_appendParamEditChange(data.reason, setValueRetCode, data.oldValues, newVariant, data.view, data.dimension, data.time, data.setKeyframe);

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


template <typename T>
ValueChangedReturnCodeEnum
Knob<T>::setValue(const T & v,
                  ViewSetSpec view,
                  DimSpec dimension,
                  ValueChangedReasonEnum reason,
                  KeyFrame* newKey,
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
        return setValueAtTime(time, v, view, dimension, reason, newKey);
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
            view_i = getViewIdxFromGetSpec(ViewIdx(view));
        }
        for (std::list<ViewIdx>::const_iterator it = views.begin(); it!=views.end(); ++it) {
            if (!view.isAll()) {
                if (view_i != *it) {
                    continue;
                }
            }
            for (int i = 0; i < nDims; ++i) {
                if (!dimension.isAll() && dimension != i) {
                    continue;
                }


                ValueKnobDimView<T>* data = dynamic_cast<ValueKnobDimView<T>*>(getDataForDimView(DimIdx(i), *it).get());
                if (data) {
                    hasChanged |= data->setValueAndCheckIfChanged(v);
                }
                // Auto-expand the parameter dimensions if needed
                if (hasChanged) {
                    // If dimensions are folded but a setValue call is made on one of them, expand them
                    if (((dimension.isAll() && i == nDims - 1) || (!dimension.isAll() && i == dimension)) && nDims > 1) {
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
        undoRedoStackHelperRAII.addSetValueToUndoRedoStackIfNeeded(v, ret);
    }

    // Evaluate the change
    evaluateValueChange(dimension, time, view, reason);

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
            // Make sure the last setValue call triggers a knobChanged call and refreshes the gui
            unblockValueChanges();
            setAdjustFoldExpandStateAutomatically(true);
        }
        ret = setValue(values[i], view, DimSpec(dimensionStartOffset + i), reason, &newKey, hasChanged);
        if (retCodes) {
            (*retCodes)[i] = ret;
        }
        hasChanged |= (ret != eValueChangedReturnCodeNothingChanged);
    }
    if (values.size() > 1) {
        endChanges();
    }


} // setValueAcrossDimensions


template<typename T>
void
Knob<T>::setValueOnCurveInternal(TimeValue time, const T& v, DimIdx dimension, ViewIdx view, KeyFrame* newKey, ValueChangedReturnCodeEnum* ret)
{
    KeyFrame key;
    makeKeyFrame(time, v, view, &key);
    if (newKey) {
        *newKey = key;
    }

    KnobDimViewBasePtr data = getDataForDimView(dimension, view);
    assert(data);

    // Get the curve for the given view/dimension
    CurvePtr curve = data->animationCurve;
    assert(curve);
    ValueChangedReturnCodeEnum addKeyRet = curve->setOrAddKeyframe(key);
    if (addKeyRet == eValueChangedReturnCodeKeyframeAdded) {
        *ret = addKeyRet;
    } else if (addKeyRet == eValueChangedReturnCodeKeyframeModified) {
        if (*ret == eValueChangedReturnCodeNothingChanged) {
            *ret = eValueChangedReturnCodeKeyframeModified;
        }
    }

    // At least redraw the curve if we did not add/remove keyframes

    data->notifyCurveChanged();

}


template<typename T>
ValueChangedReturnCodeEnum
Knob<T>::setValueAtTime(TimeValue time,
                        const T & v,
                        ViewSetSpec view,
                        DimSpec dimension,
                        ValueChangedReasonEnum reason,
                        KeyFrame* newKey,
                        bool forceHandlerEvenIfNoChange)
{

    // If no animated, do not even set a keyframe
    if ( !canAnimate() || !isAnimationEnabled() ) {
        setValue(v, view, dimension, reason, newKey);
    }

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

    AddToUndoRedoStackHelper<T> undoRedoStackHelperRAII(this);
    if (undoRedoStackHelperRAII.canAddValueToUndoStack()) {
        undoRedoStackHelperRAII.prepareOldValueToUndoRedoStack(view, dimension, time, reason, true /*setKeyframe*/);
    }


    ValueChangedReturnCodeEnum ret = !forceHandlerEvenIfNoChange ? eValueChangedReturnCodeNothingChanged : eValueChangedReturnCodeKeyframeModified;

    {
        std::list<ViewIdx> views = getViewsList();
        int nDims = getNDimensions();
        ViewIdx view_i;
        if (!view.isAll()) {
            view_i = getViewIdxFromGetSpec(ViewIdx(view));
        }
        for (std::list<ViewIdx>::const_iterator it = views.begin(); it!=views.end(); ++it) {
            if (!view.isAll()) {
                if (view_i != *it) {
                    continue;
                }
            }

            for (int i = 0; i < nDims; ++i) {
                if (!dimension.isAll() && dimension != i) {
                    continue;
                }

                setValueOnCurveInternal(time, v, DimIdx(i), *it, newKey, &ret);

                // If dimensions are folded but a setValue call is made on one of them, auto-compute fold/expand
                if (((dimension.isAll() && i == nDims - 1) || (!dimension.isAll() && i == dimension)) && nDims > 1) {
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
            undoRedoStackHelperRAII.addSetValueToUndoRedoStackIfNeeded(v, ret);
        }

        evaluateValueChange(dimension, time, view, reason);
    }

    return ret;
} // setValueAtTime


template <typename T>
void
Knob<T>::setMultipleValueAtTime(const std::list<TimeValuePair<T> >& keys, ViewSetSpec view, DimSpec dimension, ValueChangedReasonEnum reason, std::vector<KeyFrame>* newKeys)
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
    
    typename std::list<TimeValuePair<T> >::const_iterator next = keys.begin();
    ++next;
    KeyFrame k;
    ValueChangedReturnCodeEnum ret;
    if (newKeys) {
        newKeys->clear();
    }
    bool hasChanged = false;
    for (typename std::list<TimeValuePair<T> >::const_iterator it = keys.begin(); it!= keys.end(); ++it, ++next) {
        if (next == keys.end() && keys.size() > 1) {
            // Make sure the last setValue call triggers a knobChanged call
            unblockValueChanges();

            setAdjustFoldExpandStateAutomatically(true);
            
        }
        ret = setValueAtTime(it->time, it->value, view, dimension, reason, newKeys ? &k : 0, hasChanged);
        hasChanged |= (ret != eValueChangedReturnCodeNothingChanged);
        if (newKeys) {
            newKeys->push_back(k);
        }
        if (next != keys.end()) {
            ++next;
        }
    }

    if (keys.size() > 1) {
        endChanges();
    }

} // setMultipleValueAtTime

template <typename T>
void
Knob<T>::setValueAtTimeAcrossDimensions(TimeValue time,
                                        const std::vector<T>& values,
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
        ret = setValueAtTime(time, values[i], view, DimSpec(dimensionStartOffset + i), reason, &newKey, hasChanged);
        if (retCodes) {
            (*retCodes)[i] = ret;
        }
        hasChanged |= (ret != eValueChangedReturnCodeNothingChanged);
    }
    if (values.size() > 1) {
        endChanges();
    }
    

} // setValueAtTimeAcrossDimensions

template <typename T>
void
Knob<T>::setMultipleValueAtTimeAcrossDimensions(const std::vector<std::pair<DimensionViewPair, std::list<TimeValuePair<T> > > >& keysPerDimension, ValueChangedReasonEnum reason)
{
    if (keysPerDimension.empty()) {
        return;
    }

    // Group changes under the same undo/redo action if possible
    AddToUndoRedoStackHelper<T> undoRedoStackHelperRAII(this);

    bool hasChanged = false;
    ValueChangedReturnCodeEnum ret;

    beginChanges();
    blockValueChanges();
    // Since we are about to set multiple values, disable auto-expand/fold until we set the last value
    setAdjustFoldExpandStateAutomatically(false);

    for (std::size_t i = 0; i < keysPerDimension.size(); ++i) {

        if (keysPerDimension[i].second.empty()) {
            continue;
        }


        DimIdx dimension = keysPerDimension[i].first.dimension;
        ViewIdx view = keysPerDimension[i].first.view;


        typename std::list<TimeValuePair<T> >::const_iterator next = keysPerDimension[i].second.begin();
        ++next;
        for (typename std::list<TimeValuePair<T> >::const_iterator it = keysPerDimension[i].second.begin(); it!=keysPerDimension[i].second.end(); ++it) {
            if (i == keysPerDimension.size() - 1 && next == keysPerDimension[i].second.end()) {
                unblockValueChanges();
                setAdjustFoldExpandStateAutomatically(true);
            }
            ret = setValueAtTime(it->time, it->value, view, dimension, reason, 0 /*outKey*/, hasChanged);
            hasChanged |= (ret != eValueChangedReturnCodeNothingChanged);

            if (next != keysPerDimension[i].second.end()) {
                ++next;
            }
        }
    }

    endChanges();

} // setMultipleValueAtTimeAcrossDimensions



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
        ViewIdx view_i = getViewIdxFromGetSpec(ViewIdx(view));
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
        ViewIdx view_i = getViewIdxFromGetSpec(ViewIdx(view));
        autoAdjustFoldExpandDimensions(view_i);
    }
}


///////////// Explicit template instanciation for INT type
template <>
ValueChangedReturnCodeEnum
Knob<int>::setIntValue(int value,
                 ViewSetSpec view,
                 DimSpec dimension,
                 ValueChangedReasonEnum reason,
                 KeyFrame* newKey,
                 bool forceHandlerEvenIfNoChange)
{
    return setValue(value, view, dimension, reason, newKey, forceHandlerEvenIfNoChange);
}

template <typename T>
ValueChangedReturnCodeEnum
Knob<T>::setIntValue(int /*value*/,
                     ViewSetSpec /*view*/,
                     DimSpec /*dimension*/,
                     ValueChangedReasonEnum /*reason*/,
                     KeyFrame* /*newKey*/,
                     bool /*forceHandlerEvenIfNoChange*/)
{
    throw std::invalid_argument("Attempt to call a function for a wrong Knob A.P.I type");
}

template <>
void
Knob<int>::setIntValueAcrossDimensions(const std::vector<int>& values,
                                       DimIdx dimensionStartIndex,
                                       ViewSetSpec view,
                                       ValueChangedReasonEnum reason,
                                       std::vector<ValueChangedReturnCodeEnum>* retCodes)
{
    setValueAcrossDimensions(values, dimensionStartIndex, view, reason, retCodes);
}

template <typename T>
void
Knob<T>::setIntValueAcrossDimensions(const std::vector<int>& /*values*/,
                                       DimIdx /*dimensionStartIndex*/,
                                       ViewSetSpec /*view*/,
                                       ValueChangedReasonEnum /*reason*/,
                                       std::vector<ValueChangedReturnCodeEnum>* /*retCodes*/)
{
    throw std::invalid_argument("Attempt to call a function for a wrong Knob A.P.I type");
}

template <>
ValueChangedReturnCodeEnum
Knob<int>::setIntValueAtTime(TimeValue time, int value, ViewSetSpec view, DimSpec dimension, ValueChangedReasonEnum reason, KeyFrame* newKey)
{
    return setValueAtTime(time, value, view, dimension, reason, newKey, false);
}

template <typename T>
ValueChangedReturnCodeEnum
Knob<T>::setIntValueAtTime(TimeValue /*time*/, int /*value*/, ViewSetSpec /*view*/, DimSpec /*dimension*/, ValueChangedReasonEnum /*reason*/, KeyFrame* /*newKey*/)
{
    throw std::invalid_argument("Attempt to call a function for a wrong Knob A.P.I type");
}

template <>
void
Knob<int>::setMultipleIntValueAtTime(const std::list<IntTimeValuePair>& keys, ViewSetSpec view, DimSpec dimension, ValueChangedReasonEnum reason, std::vector<KeyFrame>* newKeys)
{
    setMultipleValueAtTime(keys, view, dimension, reason, newKeys);
}

template <typename T>
void
Knob<T>::setMultipleIntValueAtTime(const std::list<IntTimeValuePair>& /*keys*/, ViewSetSpec /*view*/, DimSpec /*dimension*/, ValueChangedReasonEnum /*reason*/, std::vector<KeyFrame>* /*newKey*/)
{
    throw std::invalid_argument("Attempt to call a function for a wrong Knob A.P.I type");
}

template <>
void
Knob<int>::setIntValueAtTimeAcrossDimensions(TimeValue time, const std::vector<int>& values, DimIdx dimensionStartIndex, ViewSetSpec view, ValueChangedReasonEnum reason, std::vector<ValueChangedReturnCodeEnum>* retCodes)
{
    setValueAtTimeAcrossDimensions(time, values, dimensionStartIndex, view, reason, retCodes);
}

template <typename T>
void
Knob<T>::setIntValueAtTimeAcrossDimensions(TimeValue /*time*/, const std::vector<int>& /*values*/, DimIdx /*dimensionStartIndex*/, ViewSetSpec /*view*/, ValueChangedReasonEnum /*reason*/, std::vector<ValueChangedReturnCodeEnum>* /*retCodes*/)
{
    throw std::invalid_argument("Attempt to call a function for a wrong Knob A.P.I type");
}

template <>
void
Knob<int>::setMultipleIntValueAtTimeAcrossDimensions(const PerCurveIntValuesList& keysPerDimension,  ValueChangedReasonEnum reason)
{
    setMultipleValueAtTimeAcrossDimensions(keysPerDimension, reason);
}

template <typename T>
void
Knob<T>::setMultipleIntValueAtTimeAcrossDimensions(const PerCurveIntValuesList& /*keysPerDimension*/,  ValueChangedReasonEnum /*reason*/)
{
    throw std::invalid_argument("Attempt to call a function for a wrong Knob A.P.I type");
}


///////////// End explicit template instanciation for INT type

///////////// Explicit template instanciation for DOUBLE type
template <>
ValueChangedReturnCodeEnum
Knob<double>::setDoubleValue(double value,
                             ViewSetSpec view,
                             DimSpec dimension,
                             ValueChangedReasonEnum reason,
                             KeyFrame* newKey,
                             bool forceHandlerEvenIfNoChange)
{
    return setValue(value, view, dimension, reason, newKey, forceHandlerEvenIfNoChange);
}

template <typename T>
ValueChangedReturnCodeEnum
Knob<T>::setDoubleValue(double /*value*/,
                        ViewSetSpec /*view*/,
                        DimSpec /*dimension*/,
                        ValueChangedReasonEnum /*reason*/,
                        KeyFrame* /*newKey*/,
                        bool /*forceHandlerEvenIfNoChange*/)
{
    throw std::invalid_argument("Attempt to call a function for a wrong Knob A.P.I type");
}

template <>
void
Knob<double>::setDoubleValueAcrossDimensions(const std::vector<double>& values,
                                             DimIdx dimensionStartIndex,
                                             ViewSetSpec view,
                                             ValueChangedReasonEnum reason,
                                             std::vector<ValueChangedReturnCodeEnum>* retCodes)
{
    setValueAcrossDimensions(values, dimensionStartIndex, view, reason, retCodes);
}

template <typename T>
void
Knob<T>::setDoubleValueAcrossDimensions(const std::vector<double>& /*values*/,
                                        DimIdx /*dimensionStartIndex*/,
                                        ViewSetSpec /*view*/,
                                        ValueChangedReasonEnum /*reason*/,
                                        std::vector<ValueChangedReturnCodeEnum>* /*retCodes*/)
{
    throw std::invalid_argument("Attempt to call a function for a wrong Knob A.P.I type");
}

template <>
ValueChangedReturnCodeEnum
Knob<double>::setDoubleValueAtTime(TimeValue time, double value, ViewSetSpec view, DimSpec dimension, ValueChangedReasonEnum reason, KeyFrame* newKey)
{
    return setValueAtTime(time, value, view, dimension, reason, newKey, false);
}

template <typename T>
ValueChangedReturnCodeEnum
Knob<T>::setDoubleValueAtTime(TimeValue /*time*/, double /*value*/, ViewSetSpec /*view*/, DimSpec /*dimension*/, ValueChangedReasonEnum /*reason*/, KeyFrame* /*newKey*/)
{
    throw std::invalid_argument("Attempt to call a function for a wrong Knob A.P.I type");
}

template <>
void
Knob<double>::setMultipleDoubleValueAtTime(const std::list<DoubleTimeValuePair>& keys, ViewSetSpec view, DimSpec dimension, ValueChangedReasonEnum reason, std::vector<KeyFrame>* newKeys)
{
    setMultipleValueAtTime(keys, view, dimension, reason, newKeys);
}

template <typename T>
void
Knob<T>::setMultipleDoubleValueAtTime(const std::list<DoubleTimeValuePair>& /*keys*/, ViewSetSpec /*view*/, DimSpec /*dimension*/, ValueChangedReasonEnum /*reason*/, std::vector<KeyFrame>* /*newKey*/)
{
    throw std::invalid_argument("Attempt to call a function for a wrong Knob A.P.I type");
}

template <>
void
Knob<double>::setDoubleValueAtTimeAcrossDimensions(TimeValue time, const std::vector<double>& values, DimIdx dimensionStartIndex, ViewSetSpec view, ValueChangedReasonEnum reason, std::vector<ValueChangedReturnCodeEnum>* retCodes)
{
    setValueAtTimeAcrossDimensions(time, values, dimensionStartIndex, view, reason, retCodes);
}

template <typename T>
void
Knob<T>::setDoubleValueAtTimeAcrossDimensions(TimeValue /*time*/, const std::vector<double>& /*values*/, DimIdx /*dimensionStartIndex*/, ViewSetSpec /*view*/, ValueChangedReasonEnum /*reason*/, std::vector<ValueChangedReturnCodeEnum>* /*retCodes*/)
{
    throw std::invalid_argument("Attempt to call a function for a wrong Knob A.P.I type");
}

template <>
void
Knob<double>::setMultipleDoubleValueAtTimeAcrossDimensions(const PerCurveDoubleValuesList& keysPerDimension, ValueChangedReasonEnum reason)
{
    setMultipleValueAtTimeAcrossDimensions(keysPerDimension, reason);
}

template <typename T>
void
Knob<T>::setMultipleDoubleValueAtTimeAcrossDimensions(const PerCurveDoubleValuesList& /*keysPerDimension*/, ValueChangedReasonEnum /*reason*/)
{
    throw std::invalid_argument("Attempt to call a function for a wrong Knob A.P.I type");
}


///////////// End explicit template instanciation for DOUBLE type

///////////// Explicit template instanciation for BOOL type

template <>
ValueChangedReturnCodeEnum
Knob<bool>::setBoolValue(bool value,
                         ViewSetSpec view,
                         DimSpec dimension,
                         ValueChangedReasonEnum reason,
                         KeyFrame* newKey,
                         bool forceHandlerEvenIfNoChange)
{
    return setValue(value, view, dimension, reason, newKey, forceHandlerEvenIfNoChange);
}

template <typename T>
ValueChangedReturnCodeEnum
Knob<T>::setBoolValue(bool /*value*/,
                      ViewSetSpec /*view*/,
                      DimSpec /*dimension*/,
                      ValueChangedReasonEnum /*reason*/,
                      KeyFrame* /*newKey*/,
                      bool /*forceHandlerEvenIfNoChange*/)
{
    throw std::invalid_argument("Attempt to call a function for a wrong Knob A.P.I type");
}

template <>
void
Knob<bool>::setBoolValueAcrossDimensions(const std::vector<bool>& values,
                                         DimIdx dimensionStartIndex,
                                         ViewSetSpec view,
                                         ValueChangedReasonEnum reason,
                                         std::vector<ValueChangedReturnCodeEnum>* retCodes)
{
    setValueAcrossDimensions(values, dimensionStartIndex, view, reason, retCodes);
}

template <typename T>
void
Knob<T>::setBoolValueAcrossDimensions(const std::vector<bool>& /*values*/,
                                      DimIdx /*dimensionStartIndex*/,
                                      ViewSetSpec /*view*/,
                                      ValueChangedReasonEnum /*reason*/,
                                      std::vector<ValueChangedReturnCodeEnum>* /*retCodes*/)
{
    throw std::invalid_argument("Attempt to call a function for a wrong Knob A.P.I type");
}


template <>
ValueChangedReturnCodeEnum
Knob<bool>::setBoolValueAtTime(TimeValue time, bool value, ViewSetSpec view, DimSpec dimension, ValueChangedReasonEnum reason, KeyFrame* newKey)
{
    return setValueAtTime(time, value, view, dimension, reason, newKey, false);
}

template <typename T>
ValueChangedReturnCodeEnum
Knob<T>::setBoolValueAtTime(TimeValue /*time*/, bool /*value*/, ViewSetSpec /*view*/, DimSpec /*dimension*/, ValueChangedReasonEnum /*reason*/, KeyFrame* /*newKey*/)
{
    throw std::invalid_argument("Attempt to call a function for a wrong Knob A.P.I type");
}

template <>
void
Knob<bool>::setMultipleBoolValueAtTime(const std::list<BoolTimeValuePair>& keys, ViewSetSpec view, DimSpec dimension, ValueChangedReasonEnum reason, std::vector<KeyFrame>* newKeys)
{
    setMultipleValueAtTime(keys, view, dimension, reason, newKeys);
}

template <typename T>
void
Knob<T>::setMultipleBoolValueAtTime(const std::list<BoolTimeValuePair>& /*keys*/, ViewSetSpec /*view*/, DimSpec /*dimension*/, ValueChangedReasonEnum /*reason*/, std::vector<KeyFrame>* /*newKey*/)
{
    throw std::invalid_argument("Attempt to call a function for a wrong Knob A.P.I type");
}

template <>
void
Knob<bool>::setBoolValueAtTimeAcrossDimensions(TimeValue time, const std::vector<bool>& values, DimIdx dimensionStartIndex, ViewSetSpec view, ValueChangedReasonEnum reason, std::vector<ValueChangedReturnCodeEnum>* retCodes)
{
    setValueAtTimeAcrossDimensions(time, values, dimensionStartIndex, view, reason, retCodes);
}

template <typename T>
void
Knob<T>::setBoolValueAtTimeAcrossDimensions(TimeValue /*time*/, const std::vector<bool>& /*values*/, DimIdx /*dimensionStartIndex*/, ViewSetSpec /*view*/, ValueChangedReasonEnum /*reason*/, std::vector<ValueChangedReturnCodeEnum>* /*retCodes*/)
{
    throw std::invalid_argument("Attempt to call a function for a wrong Knob A.P.I type");
}

template <>
void
Knob<bool>::setMultipleBoolValueAtTimeAcrossDimensions(const PerCurveBoolValuesList& keysPerDimension, ValueChangedReasonEnum reason)
{
    setMultipleValueAtTimeAcrossDimensions(keysPerDimension, reason);
}

template <typename T>
void
Knob<T>::setMultipleBoolValueAtTimeAcrossDimensions(const PerCurveBoolValuesList& /*keysPerDimension*/, ValueChangedReasonEnum /*reason*/)
{
    throw std::invalid_argument("Attempt to call a function for a wrong Knob A.P.I type");
}


///////////// End explicit template instanciation for BOOL type

///////////// Explicit template instanciation for STRING type

template <>
ValueChangedReturnCodeEnum
Knob<std::string>::setStringValue(const std::string& value,
                             ViewSetSpec view,
                             DimSpec dimension,
                             ValueChangedReasonEnum reason,
                             KeyFrame* newKey,
                             bool forceHandlerEvenIfNoChange)
{
    return setValue(value, view, dimension, reason, newKey, forceHandlerEvenIfNoChange);
}

template <typename T>
ValueChangedReturnCodeEnum
Knob<T>::setStringValue(const std::string& /*value*/,
                        ViewSetSpec /*view*/,
                        DimSpec /*dimension*/,
                        ValueChangedReasonEnum /*reason*/,
                        KeyFrame* /*newKey*/,
                        bool /*forceHandlerEvenIfNoChange*/)
{
    throw std::invalid_argument("Attempt to call a function for a wrong Knob A.P.I type");
}

template <>
void
Knob<std::string>::setStringValueAcrossDimensions(const std::vector<std::string>& values,
                                             DimIdx dimensionStartIndex,
                                             ViewSetSpec view,
                                             ValueChangedReasonEnum reason,
                                             std::vector<ValueChangedReturnCodeEnum>* retCodes)
{
    setValueAcrossDimensions(values, dimensionStartIndex, view, reason, retCodes);
}

template <typename T>
void
Knob<T>::setStringValueAcrossDimensions(const std::vector<std::string>& /*values*/,
                                        DimIdx /*dimensionStartIndex*/,
                                        ViewSetSpec /*view*/,
                                        ValueChangedReasonEnum /*reason*/,
                                        std::vector<ValueChangedReturnCodeEnum>* /*retCodes*/)
{
    throw std::invalid_argument("Attempt to call a function for a wrong Knob A.P.I type");
}


template <>
ValueChangedReturnCodeEnum
Knob<std::string>::setStringValueAtTime(TimeValue time, const std::string& value, ViewSetSpec view, DimSpec dimension, ValueChangedReasonEnum reason, KeyFrame* newKey)
{
    return setValueAtTime(time, value, view, dimension, reason, newKey, false);
}

template <typename T>
ValueChangedReturnCodeEnum
Knob<T>::setStringValueAtTime(TimeValue /*time*/, const std::string& /*value*/, ViewSetSpec /*view*/, DimSpec /*dimension*/, ValueChangedReasonEnum /*reason*/, KeyFrame* /*newKey*/)
{
    throw std::invalid_argument("Attempt to call a function for a wrong Knob A.P.I type");
}

template <>
void
Knob<std::string>::setMultipleStringValueAtTime(const std::list<StringTimeValuePair>& keys, ViewSetSpec view, DimSpec dimension, ValueChangedReasonEnum reason, std::vector<KeyFrame>* newKeys)
{
    setMultipleValueAtTime(keys, view, dimension, reason, newKeys);
}

template <typename T>
void
Knob<T>::setMultipleStringValueAtTime(const std::list<StringTimeValuePair>& /*keys*/, ViewSetSpec /*view*/, DimSpec /*dimension*/, ValueChangedReasonEnum /*reason*/, std::vector<KeyFrame>* /*newKey*/)
{
    throw std::invalid_argument("Attempt to call a function for a wrong Knob A.P.I type");
}

template <>
void
Knob<std::string>::setStringValueAtTimeAcrossDimensions(TimeValue time, const std::vector<std::string>& values, DimIdx dimensionStartIndex, ViewSetSpec view, ValueChangedReasonEnum reason, std::vector<ValueChangedReturnCodeEnum>* retCodes)
{
    setValueAtTimeAcrossDimensions(time, values, dimensionStartIndex, view, reason, retCodes);
}

template <typename T>
void
Knob<T>::setStringValueAtTimeAcrossDimensions(TimeValue /*time*/, const std::vector<std::string>& /*values*/, DimIdx /*dimensionStartIndex*/, ViewSetSpec /*view*/, ValueChangedReasonEnum /*reason*/, std::vector<ValueChangedReturnCodeEnum>* /*retCodes*/)
{
    throw std::invalid_argument("Attempt to call a function for a wrong Knob A.P.I type");
}

template <>
void
Knob<std::string>::setMultipleStringValueAtTimeAcrossDimensions(const PerCurveStringValuesList& keysPerDimension, ValueChangedReasonEnum reason)
{
    setMultipleValueAtTimeAcrossDimensions(keysPerDimension, reason);
}

template <typename T>
void
Knob<T>::setMultipleStringValueAtTimeAcrossDimensions(const PerCurveStringValuesList& /*keysPerDimension*/, ValueChangedReasonEnum /*reason*/)
{
    throw std::invalid_argument("Attempt to call a function for a wrong Knob A.P.I type");
}


///////////// End explicit template instanciation for STRING type

NATRON_NAMESPACE_EXIT

#endif // KNOBSETVALUEIMPL_H
