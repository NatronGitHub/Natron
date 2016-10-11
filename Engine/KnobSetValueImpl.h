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


#ifndef KNOBSETVALUEIMPL_H
#define KNOBSETVALUEIMPL_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Knob.h"

NATRON_NAMESPACE_ENTER

template <typename T>
void
Knob<T>::addSetValueToUndoRedoStackIfNeeded(const T& value, ValueChangedReasonEnum reason, ViewSpec view, int dimension, double time, bool setKeyFrame)
{

    // If the user called beginMultipleEdits to group undo/redo, then use the undo/stack
    // This can only be done if the knob has a GUI though...
    KnobHolderPtr holder = getHolder();
    if (!holder) {
        return;
    }
    KnobGuiIPtr guiPtr = getKnobGuiPointer();
    if (!guiPtr) {
        return;
    }
    KnobHolder::MultipleParamsEditEnum paramEditLevel = holder->getMultipleEditsLevel();

    // If we are under an overlay interact, the plug-in is going to do setValue calls, make sure the user can undo/redo
    bool mustEndEdits = false;
    if (paramEditLevel == KnobHolder::eMultipleParamsEditOff && holder->isDoingInteractAction()) {
        holder->beginMultipleEdits(tr("%1 changed").arg(QString::fromUtf8(getName().c_str())).toStdString());
        mustEndEdits = true;
    }
    switch (paramEditLevel) {
        case KnobHolder::eMultipleParamsEditOnCreateNewCommand:
        case KnobHolder::eMultipleParamsEditOn: {
            // Add the set value using the undo/redo stack
            Variant vari;
            valueToVariant(value, &vari);
            _signalSlotHandler->s_appendParamEditChange(reason, vari, view, dimension, time, setKeyFrame);
            break;
        }
        case KnobHolder::eMultipleParamsEditOff:
        default:
            // Multiple params undo/redo is disabled, don't do anything special
            break;
    } // switch (paramEditLevel) {

    if (mustEndEdits) {
        holder->endMultipleEdits();
    }
}

template <typename T>
ValueChangedReturnCodeEnum
Knob<T>::setValue(const T & v,
                  ViewSpec view,
                  int dimension,
                  ValueChangedReasonEnum reason,
                  KeyFrame* newKey,
                  bool forceHandlerEvenIfNoChange)
{
    // Check dimension index
    if ( (dimension < 0) || ( dimension >= (int)_values.size() ) ) {
        return eValueChangedReturnCodeNothingChanged;
    }

    ValueChangedReturnCodeEnum ret = eValueChangedReturnCodeNoKeyframeAdded;
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

    if ( !holder || holder->isSetValueCurrentlyPossible() ) {
        // Set value is possible
        // There might be stuff in the queue that must be processed first
        dequeueValuesSet(true);
    } else {

        // If we cannot set value, queue it
        if (getEvaluateOnChange()) {
            //We explicitly abort rendering now and do not wait for it to be done in EffectInstance::evaluate()
            //because the actual value change (which will call evaluate()) may arise well later
            holder->abortAnyEvaluation();
        }

        ValueChangedReturnCodeEnum returnValue;
        double time = getCurrentTime();
        KeyFrame k;
        CurvePtr curve;
        if (newKey) {
            curve = getCurve(view, dimension);
            if (curve) {
                bool hasAnimation = curve->isAnimated();
                bool hasKeyAtTime;
                {
                    KeyFrame existingKey;
                    hasKeyAtTime = curve->getKeyFrameWithTime(time, &existingKey);
                }
                if (hasAnimation) {
#pragma message WARN("setValue does not support multi-view yet")
                    makeKeyFrame(time, v,ViewIdx(view.value()), &k);
                }
                if (hasAnimation && hasKeyAtTime) {
                    returnValue =  eValueChangedReturnCodeKeyframeModified;
                    setInternalCurveHasChanged(view, dimension, true);
                } else if (hasAnimation) {
                    returnValue =  eValueChangedReturnCodeKeyframeAdded;
                    setInternalCurveHasChanged(view, dimension, true);
                } else {
                    returnValue =  eValueChangedReturnCodeNoKeyframeAdded;
                }
            } else {
                returnValue =  eValueChangedReturnCodeNoKeyframeAdded;
            }
        } else {
            returnValue =  eValueChangedReturnCodeNoKeyframeAdded;
        }

        QueuedSetValuePtr qv( new QueuedSetValue( view, dimension, v, k, returnValue != eValueChangedReturnCodeNoKeyframeAdded, reason, isValueChangesBlocked() ) );
#ifdef DEBUG
        debugHook();
#endif
        {
            QMutexLocker kql(&_setValuesQueueMutex);
            _setValuesQueue.push_back(qv);
        }
        if ( QThread::currentThread() == qApp->thread() ) {
            {
                QMutexLocker k(&_valueMutex);
                _guiValues[dimension] = v;
            }
            if ( !isValueChangesBlocked() ) {
                holder->onKnobValueChanged_public(shared_from_this(), reason, time, view, true);
            }

            if (_signalSlotHandler) {
                _signalSlotHandler->s_valueChanged(view, dimension, (int)reason);
            }
        }

        return returnValue;
    } // setvalue possible


    // Check if setValue actually changed something
    bool hasChanged = forceHandlerEvenIfNoChange;
    {
        QMutexLocker l(&_valueMutex);
        hasChanged |= checkIfValueChanged(v, _values[dimension]);
        _values[dimension] = v;
        _guiValues[dimension] = v;
    }


    // Check if we can add automatically a new keyframe
    if (isAutoKeyingEnabled(dimension, reason)) {
        double time = getCurrentTime();
        ret = setValueAtTime(time, v, view, dimension, reason, newKey);
    } else {
        double time = getCurrentTime();
        addSetValueToUndoRedoStackIfNeeded(v, reason, view, dimension, time, false);
        evaluateValueChange(dimension, time, view, reason);
    }


    if ( !hasChanged && (ret == eValueChangedReturnCodeNoKeyframeAdded) ) {
        return eValueChangedReturnCodeNothingChanged;
    }

    return ret;
} // setValue



template <typename T>
void
Knob<T>::setValueAcrossDimensions(const std::vector<T>& values,
                   int dimensionStartOffset,
                   ViewSpec view,
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
    EffectInstancePtr effect;
    bool doEditEnd = false;

    if (holder) {
        effect = toEffectInstance(holder);
        if (effect) {
            if ( effect->isDoingInteractAction() ) {
                effect->beginMultipleEdits(tr("%1 changed").arg(QString::fromUtf8(getName().c_str())).toStdString());
                doEditEnd = true;
            }
        }
    }

    KeyFrame newKey;
    ValueChangedReturnCodeEnum ret;
    bool hasChanged = false;

    if (values.size() > 1) {
        beginChanges();
        blockValueChanges();
    }
    if (retCodes) {
        retCodes->resize(values.size());
    }
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i == values.size() - 1 && values.size() > 1) {
            unblockValueChanges();
        }
        ret = setValue(values[i], view, dimensionStartOffset + i, reason, &newKey, hasChanged);
        if (retCodes) {
            (*retCodes)[i] = ret;
        }
        hasChanged |= (ret != eValueChangedReturnCodeNothingChanged);
    }
    if (values.size() > 1) {
        endChanges();
    }
    if (doEditEnd) {
        effect->endMultipleEdits();
    }
} // setValueAcrossDimensions



template<typename T>
ValueChangedReturnCodeEnum
Knob<T>::setValueAtTime(double time,
                        const T & v,
                        ViewSpec view,
                        int dimension,
                        ValueChangedReasonEnum reason,
                        KeyFrame* newKey,
                        bool forceHandlerEvenIfNoChange)
{
    if ( (dimension < 0) || ( dimension >= (int)_values.size() ) ) {
        return eValueChangedReturnCodeNothingChanged;
    }

    // If no animated, do not even set a keyframe
    if ( !canAnimate() || !isAnimationEnabled() ) {
        setValue(v, view, dimension, reason, newKey);
    }

    ValueChangedReturnCodeEnum ret = eValueChangedReturnCodeNothingChanged;
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

#pragma message WARN("setValueAtTime does not support multi-view yet")
    KeyFrame key;
    makeKeyFrame(time, v, ViewIdx(view.value()), &key);
    if (newKey) {
        *newKey = key;
    }


    // Get the curve for the given view/dimension
    CurvePtr curve = getCurve(view, dimension, true);
    assert(curve);

    if (!holder || holder->isSetValueCurrentlyPossible()) {
        // SetValue is possible
        // There might be stuff in the queue that must be processed first
        dequeueValuesSet(true);
    }
    if ( holder && !holder->isSetValueCurrentlyPossible() ) {
        // If we cannot set value, queue it
        if ( holder && getEvaluateOnChange() ) {
            // We explicitly abort rendering now and do not wait for it to be done in EffectInstance::evaluate()
            // because the actual value change (which will call evaluate()) may arise well later
            holder->abortAnyEvaluation();
        }

        boost::shared_ptr<QueuedSetValueAtTime> qv( new QueuedSetValueAtTime( time, view, dimension, v, key, reason, isValueChangesBlocked() ) );
#ifdef DEBUG
        debugHook();
#endif
        {
            QMutexLocker kql(&_setValuesQueueMutex);
            _setValuesQueue.push_back(qv);
        }

        setInternalCurveHasChanged(view, dimension, true);

        KeyFrame k;
        bool hasAnimation = curve->isAnimated();
        bool hasKeyAtTime = curve->getKeyFrameWithTime(time, &k);
        if (hasAnimation && hasKeyAtTime) {
            return eValueChangedReturnCodeKeyframeModified;
        } else {
            return eValueChangedReturnCodeKeyframeAdded;
        }
    }

    // Check if a keyframe already exists at this time, if so modify it
    bool hasChanged = forceHandlerEvenIfNoChange;
    KeyFrame existingKey;
    bool hasExistingKey = curve->getKeyFrameWithTime(time, &existingKey);
    if (!hasExistingKey) {
        hasChanged = true;
        ret = eValueChangedReturnCodeKeyframeAdded;
    } else {
        bool modifiedKeyFrame = ( ( existingKey.getValue() != key.getValue() ) ||
                                 ( existingKey.getLeftDerivative() != key.getLeftDerivative() ) ||
                                 ( existingKey.getRightDerivative() != key.getRightDerivative() ) );
        if (modifiedKeyFrame) {
            ret = eValueChangedReturnCodeKeyframeModified;
        }
        hasChanged |= modifiedKeyFrame;
    }

    // Add or modify keyframe
    bool newKeyFrame = curve->addKeyFrame(key);
    if (newKeyFrame) {
        ret = eValueChangedReturnCodeKeyframeAdded;
    }

    // Refresh holder animation flag
    if (holder) {
        holder->setHasAnimation(true);
    }

    // Sync the gui curve to this internal curve
    guiCurveCloneInternalCurve(eCurveChangeReasonInternal, view, dimension, reason);

    if (newKeyFrame) {
        // Notify animation changed
        std::list<double> keysAdded, keysRemoved;
        keysAdded.push_back(time);
        _signalSlotHandler->s_curveAnimationChanged(keysAdded, keysRemoved, view, dimension, eCurveChangeReasonInternal);
    } else {
        _signalSlotHandler->s_redrawGuiCurve(eCurveChangeReasonInternal, view, dimension);
    }

    if (hasChanged) {
        addSetValueToUndoRedoStackIfNeeded(v, reason, view, dimension, time, true);
        evaluateValueChange(dimension, time, view, reason);
    } else {
        return eValueChangedReturnCodeNothingChanged;
    }

    return ret;
} // setValueAtTime


template <typename T>
void
Knob<T>::setMultipleValueAtTime(const std::list<TimeValuePair<T> >& keys, ViewSpec view, int dimension, ValueChangedReasonEnum reason, std::vector<KeyFrame>* newKeys)
{
    if (keys.empty()) {
        return;
    }
    if (keys.size() > 1) {
        beginChanges();
        blockValueChanges();
    }

    // Group changes under the same undo/redo action if possible
    KnobHolderPtr holder = getHolder();
    bool doEditEnd = false;
    if (holder) {
        if (holder->getMultipleEditsLevel() == KnobHolder::eMultipleParamsEditOff) {
            holder->beginMultipleEdits(tr("%1 changed").arg(QString::fromUtf8(getName().c_str())).toStdString());
            doEditEnd = true;
        }
    }

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
            unblockValueChanges();
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

    if (doEditEnd) {
        holder->endMultipleEdits();
    }
    if (keys.size() > 1) {
        endChanges();
    }

} // setMultipleValueAtTime

template <typename T>
void
Knob<T>::setValueAtTimeAcrossDimensions(double time,
                                        const std::vector<T>& values,
                                        int dimensionStartOffset,
                                        ViewSpec view,
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
    KnobHolderPtr holder = getHolder();
    bool doEditEnd = false;
    if (holder) {
        if (holder->getMultipleEditsLevel() == KnobHolder::eMultipleParamsEditOff) {
            holder->beginMultipleEdits(tr("%1 changed").arg(QString::fromUtf8(getName().c_str())).toStdString());
            doEditEnd = true;
        }
    }

    KeyFrame newKey;
    ValueChangedReturnCodeEnum ret;
    bool hasChanged = false;

    if (values.size() > 1) {
        beginChanges();
        blockValueChanges();
    }
    if (retCodes) {
        retCodes->resize(values.size());
    }
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i == values.size() - 1 && values.size() > 1) {
            unblockValueChanges();
        }
        ret = setValueAtTime(time, values[i], view, dimensionStartOffset + i, reason, &newKey, hasChanged);
        if (retCodes) {
            (*retCodes)[i] = ret;
        }
        hasChanged |= (ret != eValueChangedReturnCodeNothingChanged);
    }
    if (doEditEnd) {
        holder->endMultipleEdits();
    }
    if (values.size() > 1) {
        endChanges();
    }
    

} // setValueAtTimeAcrossDimensions

template <typename T>
void
Knob<T>::setMultipleValueAtTimeAcrossDimensions(const std::vector<std::list<TimeValuePair<T> > >& keysPerDimension, int dimensionStartIndex, ViewSpec view, ValueChangedReasonEnum reason)
{
    if (keysPerDimension.empty()) {
        return;
    }
    int nDims = getNDimensions();
    assert(nDims >= (int)keysPerDimension.size());
    if (dimensionStartIndex < 0 || dimensionStartIndex + (int)keysPerDimension.size() > nDims) {
        throw std::invalid_argument("Knob<T>::setMultipleValueAtTimeAcrossDimensions: Invalid arguments");
    }
    // Group changes under the same undo/redo action if possible
    KnobHolderPtr holder = getHolder();
    bool doEditEnd = false;
    if (holder) {
        if (holder->getMultipleEditsLevel() == KnobHolder::eMultipleParamsEditOff) {
            holder->beginMultipleEdits(tr("%1 changed").arg(QString::fromUtf8(getName().c_str())).toStdString());
            doEditEnd = true;
        }
    }

    bool hasChanged = false;
    ValueChangedReturnCodeEnum ret;

    beginChanges();
    blockValueChanges();


    for (std::size_t i = 0; i < keysPerDimension.size(); ++i) {

        if (keysPerDimension[i].empty()) {
            continue;
        }
        typename std::list<TimeValuePair<T> >::const_iterator next = keysPerDimension[i].begin();
        ++next;
        for (typename std::list<TimeValuePair<T> >::const_iterator it = keysPerDimension[i].begin(); it!=keysPerDimension[i].end(); ++it) {
            if (i == keysPerDimension.size() - 1 && next == keysPerDimension[i].end()) {
                unblockValueChanges();
            }
            ret = setValueAtTime(it->time, it->value, view, dimensionStartIndex + i, reason, 0 /*outKey*/, hasChanged);
            hasChanged |= (ret != eValueChangedReturnCodeNothingChanged);

            if (next != keysPerDimension[i].end()) {
                ++next;
            }
        }
    }

    if (doEditEnd) {
        holder->endMultipleEdits();
    }
    endChanges();

} // setMultipleValueAtTimeAcrossDimensions

///////////// Explicit template instanciation for INT type
template <>
ValueChangedReturnCodeEnum
Knob<int>::setIntValue(int value,
                 ViewSpec view,
                 int dimension,
                 ValueChangedReasonEnum reason,
                 KeyFrame* newKey,
                 bool forceHandlerEvenIfNoChange)
{
    return setValue(value, view, dimension, reason, newKey, forceHandlerEvenIfNoChange);
}

template <typename T>
ValueChangedReturnCodeEnum
Knob<T>::setIntValue(int /*value*/,
                     ViewSpec /*view*/,
                     int /*dimension*/,
                     ValueChangedReasonEnum /*reason*/,
                     KeyFrame* /*newKey*/,
                     bool /*forceHandlerEvenIfNoChange*/)
{
    throw std::invalid_argument("Attempt to call a function for a wrong Knob A.P.I type");
}

template <>
void
Knob<int>::setIntValueAcrossDimensions(const std::vector<int>& values,
                                       int dimensionStartIndex,
                                       ViewSpec view,
                                       ValueChangedReasonEnum reason,
                                       std::vector<ValueChangedReturnCodeEnum>* retCodes)
{
    setValueAcrossDimensions(values, dimensionStartIndex, view, reason, retCodes);
}

template <typename T>
void
Knob<T>::setIntValueAcrossDimensions(const std::vector<int>& /*values*/,
                                       int /*dimensionStartIndex*/,
                                       ViewSpec /*view*/,
                                       ValueChangedReasonEnum /*reason*/,
                                       std::vector<ValueChangedReturnCodeEnum>* /*retCodes*/)
{
    throw std::invalid_argument("Attempt to call a function for a wrong Knob A.P.I type");
}

template <>
ValueChangedReturnCodeEnum
Knob<int>::setIntValueAtTime(double time, int value, ViewSpec view, int dimension, ValueChangedReasonEnum reason, KeyFrame* newKey)
{
    return setValueAtTime(time, value, view, dimension, reason, newKey, false);
}

template <typename T>
ValueChangedReturnCodeEnum
Knob<T>::setIntValueAtTime(double /*time*/, int /*value*/, ViewSpec /*view*/, int /*dimension*/, ValueChangedReasonEnum /*reason*/, KeyFrame* /*newKey*/)
{
    throw std::invalid_argument("Attempt to call a function for a wrong Knob A.P.I type");
}

template <>
void
Knob<int>::setMultipleIntValueAtTime(const std::list<IntTimeValuePair>& keys, ViewSpec view, int dimension, ValueChangedReasonEnum reason, std::vector<KeyFrame>* newKeys)
{
    setMultipleValueAtTime(keys, view, dimension, reason, newKeys);
}

template <typename T>
void
Knob<T>::setMultipleIntValueAtTime(const std::list<IntTimeValuePair>& /*keys*/, ViewSpec /*view*/, int /*dimension*/, ValueChangedReasonEnum /*reason*/, std::vector<KeyFrame>* /*newKey*/)
{
    throw std::invalid_argument("Attempt to call a function for a wrong Knob A.P.I type");
}

template <>
void
Knob<int>::setIntValueAtTimeAcrossDimensions(double time, const std::vector<int>& values, int dimensionStartIndex, ViewSpec view, ValueChangedReasonEnum reason, std::vector<ValueChangedReturnCodeEnum>* retCodes)
{
    setValueAtTimeAcrossDimensions(time, values, dimensionStartIndex, view, reason, retCodes);
}

template <typename T>
void
Knob<T>::setIntValueAtTimeAcrossDimensions(double /*time*/, const std::vector<int>& /*values*/, int /*dimensionStartIndex*/, ViewSpec /*view*/, ValueChangedReasonEnum /*reason*/, std::vector<ValueChangedReturnCodeEnum>* /*retCodes*/)
{
    throw std::invalid_argument("Attempt to call a function for a wrong Knob A.P.I type");
}

template <>
void
Knob<int>::setMultipleIntValueAtTimeAcrossDimensions(const std::vector<std::list<IntTimeValuePair> >& keysPerDimension, int dimensionStartIndex, ViewSpec view, ValueChangedReasonEnum reason)
{
    setMultipleValueAtTimeAcrossDimensions(keysPerDimension, dimensionStartIndex, view, reason);
}

template <typename T>
void
Knob<T>::setMultipleIntValueAtTimeAcrossDimensions(const std::vector<std::list<IntTimeValuePair> >& /*keysPerDimension*/, int /*dimensionStartIndex*/, ViewSpec /*view*/, ValueChangedReasonEnum /*reason*/)
{
    throw std::invalid_argument("Attempt to call a function for a wrong Knob A.P.I type");
}


///////////// End explicit template instanciation for INT type

///////////// Explicit template instanciation for DOUBLE type
template <>
ValueChangedReturnCodeEnum
Knob<double>::setDoubleValue(double value,
                             ViewSpec view,
                             int dimension,
                             ValueChangedReasonEnum reason,
                             KeyFrame* newKey,
                             bool forceHandlerEvenIfNoChange)
{
    return setValue(value, view, dimension, reason, newKey, forceHandlerEvenIfNoChange);
}

template <typename T>
ValueChangedReturnCodeEnum
Knob<T>::setDoubleValue(double /*value*/,
                        ViewSpec /*view*/,
                        int /*dimension*/,
                        ValueChangedReasonEnum /*reason*/,
                        KeyFrame* /*newKey*/,
                        bool /*forceHandlerEvenIfNoChange*/)
{
    throw std::invalid_argument("Attempt to call a function for a wrong Knob A.P.I type");
}

template <>
void
Knob<double>::setDoubleValueAcrossDimensions(const std::vector<double>& values,
                                             int dimensionStartIndex,
                                             ViewSpec view,
                                             ValueChangedReasonEnum reason,
                                             std::vector<ValueChangedReturnCodeEnum>* retCodes)
{
    setValueAcrossDimensions(values, dimensionStartIndex, view, reason, retCodes);
}

template <typename T>
void
Knob<T>::setDoubleValueAcrossDimensions(const std::vector<double>& /*values*/,
                                        int /*dimensionStartIndex*/,
                                        ViewSpec /*view*/,
                                        ValueChangedReasonEnum /*reason*/,
                                        std::vector<ValueChangedReturnCodeEnum>* /*retCodes*/)
{
    throw std::invalid_argument("Attempt to call a function for a wrong Knob A.P.I type");
}

template <>
ValueChangedReturnCodeEnum
Knob<double>::setDoubleValueAtTime(double time, double value, ViewSpec view, int dimension, ValueChangedReasonEnum reason, KeyFrame* newKey)
{
    return setValueAtTime(time, value, view, dimension, reason, newKey, false);
}

template <typename T>
ValueChangedReturnCodeEnum
Knob<T>::setDoubleValueAtTime(double /*time*/, double /*value*/, ViewSpec /*view*/, int /*dimension*/, ValueChangedReasonEnum /*reason*/, KeyFrame* /*newKey*/)
{
    throw std::invalid_argument("Attempt to call a function for a wrong Knob A.P.I type");
}

template <>
void
Knob<double>::setMultipleDoubleValueAtTime(const std::list<DoubleTimeValuePair>& keys, ViewSpec view, int dimension, ValueChangedReasonEnum reason, std::vector<KeyFrame>* newKeys)
{
    setMultipleValueAtTime(keys, view, dimension, reason, newKeys);
}

template <typename T>
void
Knob<T>::setMultipleDoubleValueAtTime(const std::list<DoubleTimeValuePair>& /*keys*/, ViewSpec /*view*/, int /*dimension*/, ValueChangedReasonEnum /*reason*/, std::vector<KeyFrame>* /*newKey*/)
{
    throw std::invalid_argument("Attempt to call a function for a wrong Knob A.P.I type");
}

template <>
void
Knob<double>::setDoubleValueAtTimeAcrossDimensions(double time, const std::vector<double>& values, int dimensionStartIndex, ViewSpec view, ValueChangedReasonEnum reason, std::vector<ValueChangedReturnCodeEnum>* retCodes)
{
    setValueAtTimeAcrossDimensions(time, values, dimensionStartIndex, view, reason, retCodes);
}

template <typename T>
void
Knob<T>::setDoubleValueAtTimeAcrossDimensions(double /*time*/, const std::vector<double>& /*values*/, int /*dimensionStartIndex*/, ViewSpec /*view*/, ValueChangedReasonEnum /*reason*/, std::vector<ValueChangedReturnCodeEnum>* /*retCodes*/)
{
    throw std::invalid_argument("Attempt to call a function for a wrong Knob A.P.I type");
}

template <>
void
Knob<double>::setMultipleDoubleValueAtTimeAcrossDimensions(const std::vector<std::list<DoubleTimeValuePair> >& keysPerDimension, int dimensionStartIndex, ViewSpec view, ValueChangedReasonEnum reason)
{
    setMultipleValueAtTimeAcrossDimensions(keysPerDimension, dimensionStartIndex, view, reason);
}

template <typename T>
void
Knob<T>::setMultipleDoubleValueAtTimeAcrossDimensions(const std::vector<std::list<DoubleTimeValuePair> >& /*keysPerDimension*/, int /*dimensionStartIndex*/, ViewSpec /*view*/, ValueChangedReasonEnum /*reason*/)
{
    throw std::invalid_argument("Attempt to call a function for a wrong Knob A.P.I type");
}


///////////// End explicit template instanciation for DOUBLE type

///////////// Explicit template instanciation for BOOL type

template <>
ValueChangedReturnCodeEnum
Knob<bool>::setBoolValue(bool value,
                         ViewSpec view,
                         int dimension,
                         ValueChangedReasonEnum reason,
                         KeyFrame* newKey,
                         bool forceHandlerEvenIfNoChange)
{
    return setValue(value, view, dimension, reason, newKey, forceHandlerEvenIfNoChange);
}

template <typename T>
ValueChangedReturnCodeEnum
Knob<T>::setBoolValue(bool /*value*/,
                      ViewSpec /*view*/,
                      int /*dimension*/,
                      ValueChangedReasonEnum /*reason*/,
                      KeyFrame* /*newKey*/,
                      bool /*forceHandlerEvenIfNoChange*/)
{
    throw std::invalid_argument("Attempt to call a function for a wrong Knob A.P.I type");
}

template <>
void
Knob<bool>::setBoolValueAcrossDimensions(const std::vector<bool>& values,
                                         int dimensionStartIndex,
                                         ViewSpec view,
                                         ValueChangedReasonEnum reason,
                                         std::vector<ValueChangedReturnCodeEnum>* retCodes)
{
    setValueAcrossDimensions(values, dimensionStartIndex, view, reason, retCodes);
}

template <typename T>
void
Knob<T>::setBoolValueAcrossDimensions(const std::vector<bool>& /*values*/,
                                      int /*dimensionStartIndex*/,
                                      ViewSpec /*view*/,
                                      ValueChangedReasonEnum /*reason*/,
                                      std::vector<ValueChangedReturnCodeEnum>* /*retCodes*/)
{
    throw std::invalid_argument("Attempt to call a function for a wrong Knob A.P.I type");
}


template <>
ValueChangedReturnCodeEnum
Knob<bool>::setBoolValueAtTime(double time, bool value, ViewSpec view, int dimension, ValueChangedReasonEnum reason, KeyFrame* newKey)
{
    return setValueAtTime(time, value, view, dimension, reason, newKey, false);
}

template <typename T>
ValueChangedReturnCodeEnum
Knob<T>::setBoolValueAtTime(double /*time*/, bool /*value*/, ViewSpec /*view*/, int /*dimension*/, ValueChangedReasonEnum /*reason*/, KeyFrame* /*newKey*/)
{
    throw std::invalid_argument("Attempt to call a function for a wrong Knob A.P.I type");
}

template <>
void
Knob<bool>::setMultipleBoolValueAtTime(const std::list<BoolTimeValuePair>& keys, ViewSpec view, int dimension, ValueChangedReasonEnum reason, std::vector<KeyFrame>* newKeys)
{
    setMultipleValueAtTime(keys, view, dimension, reason, newKeys);
}

template <typename T>
void
Knob<T>::setMultipleBoolValueAtTime(const std::list<BoolTimeValuePair>& /*keys*/, ViewSpec /*view*/, int /*dimension*/, ValueChangedReasonEnum /*reason*/, std::vector<KeyFrame>* /*newKey*/)
{
    throw std::invalid_argument("Attempt to call a function for a wrong Knob A.P.I type");
}

template <>
void
Knob<bool>::setBoolValueAtTimeAcrossDimensions(double time, const std::vector<bool>& values, int dimensionStartIndex, ViewSpec view, ValueChangedReasonEnum reason, std::vector<ValueChangedReturnCodeEnum>* retCodes)
{
    setValueAtTimeAcrossDimensions(time, values, dimensionStartIndex, view, reason, retCodes);
}

template <typename T>
void
Knob<T>::setBoolValueAtTimeAcrossDimensions(double /*time*/, const std::vector<bool>& /*values*/, int /*dimensionStartIndex*/, ViewSpec /*view*/, ValueChangedReasonEnum /*reason*/, std::vector<ValueChangedReturnCodeEnum>* /*retCodes*/)
{
    throw std::invalid_argument("Attempt to call a function for a wrong Knob A.P.I type");
}

template <>
void
Knob<bool>::setMultipleBoolValueAtTimeAcrossDimensions(const std::vector<std::list<BoolTimeValuePair> >& keysPerDimension, int dimensionStartIndex, ViewSpec view, ValueChangedReasonEnum reason)
{
    setMultipleValueAtTimeAcrossDimensions(keysPerDimension, dimensionStartIndex, view, reason);
}

template <typename T>
void
Knob<T>::setMultipleBoolValueAtTimeAcrossDimensions(const std::vector<std::list<BoolTimeValuePair> >& /*keysPerDimension*/, int /*dimensionStartIndex*/, ViewSpec /*view*/, ValueChangedReasonEnum /*reason*/)
{
    throw std::invalid_argument("Attempt to call a function for a wrong Knob A.P.I type");
}


///////////// End explicit template instanciation for BOOL type

///////////// Explicit template instanciation for STRING type

template <>
ValueChangedReturnCodeEnum
Knob<std::string>::setStringValue(const std::string& value,
                             ViewSpec view,
                             int dimension,
                             ValueChangedReasonEnum reason,
                             KeyFrame* newKey,
                             bool forceHandlerEvenIfNoChange)
{
    return setValue(value, view, dimension, reason, newKey, forceHandlerEvenIfNoChange);
}

template <typename T>
ValueChangedReturnCodeEnum
Knob<T>::setStringValue(const std::string& /*value*/,
                        ViewSpec /*view*/,
                        int /*dimension*/,
                        ValueChangedReasonEnum /*reason*/,
                        KeyFrame* /*newKey*/,
                        bool /*forceHandlerEvenIfNoChange*/)
{
    throw std::invalid_argument("Attempt to call a function for a wrong Knob A.P.I type");
}

template <>
void
Knob<std::string>::setStringValueAcrossDimensions(const std::vector<std::string>& values,
                                             int dimensionStartIndex,
                                             ViewSpec view,
                                             ValueChangedReasonEnum reason,
                                             std::vector<ValueChangedReturnCodeEnum>* retCodes)
{
    setValueAcrossDimensions(values, dimensionStartIndex, view, reason, retCodes);
}

template <typename T>
void
Knob<T>::setStringValueAcrossDimensions(const std::vector<std::string>& /*values*/,
                                        int /*dimensionStartIndex*/,
                                        ViewSpec /*view*/,
                                        ValueChangedReasonEnum /*reason*/,
                                        std::vector<ValueChangedReturnCodeEnum>* /*retCodes*/)
{
    throw std::invalid_argument("Attempt to call a function for a wrong Knob A.P.I type");
}


template <>
ValueChangedReturnCodeEnum
Knob<std::string>::setStringValueAtTime(double time, const std::string& value, ViewSpec view, int dimension, ValueChangedReasonEnum reason, KeyFrame* newKey)
{
    return setValueAtTime(time, value, view, dimension, reason, newKey, false);
}

template <typename T>
ValueChangedReturnCodeEnum
Knob<T>::setStringValueAtTime(double /*time*/, const std::string& /*value*/, ViewSpec /*view*/, int /*dimension*/, ValueChangedReasonEnum /*reason*/, KeyFrame* /*newKey*/)
{
    throw std::invalid_argument("Attempt to call a function for a wrong Knob A.P.I type");
}

template <>
void
Knob<std::string>::setMultipleStringValueAtTime(const std::list<StringTimeValuePair>& keys, ViewSpec view, int dimension, ValueChangedReasonEnum reason, std::vector<KeyFrame>* newKeys)
{
    setMultipleValueAtTime(keys, view, dimension, reason, newKeys);
}

template <typename T>
void
Knob<T>::setMultipleStringValueAtTime(const std::list<StringTimeValuePair>& /*keys*/, ViewSpec /*view*/, int /*dimension*/, ValueChangedReasonEnum /*reason*/, std::vector<KeyFrame>* /*newKey*/)
{
    throw std::invalid_argument("Attempt to call a function for a wrong Knob A.P.I type");
}

template <>
void
Knob<std::string>::setStringValueAtTimeAcrossDimensions(double time, const std::vector<std::string>& values, int dimensionStartIndex, ViewSpec view, ValueChangedReasonEnum reason, std::vector<ValueChangedReturnCodeEnum>* retCodes)
{
    setValueAtTimeAcrossDimensions(time, values, dimensionStartIndex, view, reason, retCodes);
}

template <typename T>
void
Knob<T>::setStringValueAtTimeAcrossDimensions(double /*time*/, const std::vector<std::string>& /*values*/, int /*dimensionStartIndex*/, ViewSpec /*view*/, ValueChangedReasonEnum /*reason*/, std::vector<ValueChangedReturnCodeEnum>* /*retCodes*/)
{
    throw std::invalid_argument("Attempt to call a function for a wrong Knob A.P.I type");
}

template <>
void
Knob<std::string>::setMultipleStringValueAtTimeAcrossDimensions(const std::vector<std::list<StringTimeValuePair> >& keysPerDimension, int dimensionStartIndex, ViewSpec view, ValueChangedReasonEnum reason)
{
    setMultipleValueAtTimeAcrossDimensions(keysPerDimension, dimensionStartIndex, view, reason);
}

template <typename T>
void
Knob<T>::setMultipleStringValueAtTimeAcrossDimensions(const std::vector<std::list<StringTimeValuePair> >& /*keysPerDimension*/, int /*dimensionStartIndex*/, ViewSpec /*view*/, ValueChangedReasonEnum /*reason*/)
{
    throw std::invalid_argument("Attempt to call a function for a wrong Knob A.P.I type");
}


///////////// End explicit template instanciation for STRING type

NATRON_NAMESPACE_EXIT

#endif // KNOBSETVALUEIMPL_H
