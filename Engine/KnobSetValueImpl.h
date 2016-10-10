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
ValueChangedReturnCodeEnum
Knob<T>::setValue(const T & v,
                  ViewSpec view,
                  int dimension,
                  ValueChangedReasonEnum reason,
                  KeyFrame* newKey,
                  bool forceHandlerEvenIfNoChange)
{
    if ( (dimension < 0) || ( dimension >= (int)_values.size() ) ) {
        return eValueChangedReturnCodeNothingChanged;
    }

    ValueChangedReturnCodeEnum ret = eValueChangedReturnCodeNoKeyframeAdded;
    KnobHolderPtr holder = getHolder();

#ifdef DEBUG
    if ( holder && (reason == eValueChangedReasonPluginEdited) ) {
        EffectInstancePtr isEffect = toEffectInstance(holder);
        if (isEffect) {
            isEffect->checkCanSetValueAndWarn();
        }
    }
#endif

    if ( holder && (reason == eValueChangedReasonPluginEdited) && getKnobGuiPointer() ) {
        KnobHolder::MultipleParamsEditEnum paramEditLevel = holder->getMultipleParamsEditLevel();
        switch (paramEditLevel) {
            case KnobHolder::eMultipleParamsEditOff:
            default:
                break;
            case KnobHolder::eMultipleParamsEditOnCreateNewCommand: {
                if ( !get_SetValueRecursionLevel() ) {
                    Variant vari;
                    valueToVariant(v, &vari);
                    holder->setMultipleParamsEditLevel(KnobHolder::eMultipleParamsEditOn);
                    {
                        QMutexLocker l(&_setValueRecursionLevelMutex);
                        ++_setValueRecursionLevel;
                    }
                    _signalSlotHandler->s_appendParamEditChange(reason, vari, view, dimension, 0, true, false);
                    {
                        QMutexLocker l(&_setValueRecursionLevelMutex);
                        --_setValueRecursionLevel;
                    }

                    return ret;
                }
                break;
            }
            case KnobHolder::eMultipleParamsEditOn: {
                if ( !get_SetValueRecursionLevel() ) {
                    Variant vari;
                    valueToVariant(v, &vari);
                    {
                        QMutexLocker l(&_setValueRecursionLevelMutex);
                        ++_setValueRecursionLevel;
                    }
                    _signalSlotHandler->s_appendParamEditChange(reason, vari, view, dimension, 0, false, false);
                    {
                        QMutexLocker l(&_setValueRecursionLevelMutex);
                        --_setValueRecursionLevel;
                    }

                    return ret;
                }
                break;
            }
        } // switch (paramEditLevel) {

        ///basically if we enter this if condition, for each dimension the undo stack will create a new command.
        ///the caller should have tested this prior to calling this function and correctly called editBegin() editEnd()
        ///to bracket the setValue()  calls within the same undo/redo command.
        if ( holder->isDoingInteractAction() ) {
            if ( !get_SetValueRecursionLevel() ) {
                Variant vari;
                valueToVariant(v, &vari);
                _signalSlotHandler->s_setValueWithUndoStack(vari, view, dimension);

                return ret;
            }
        }
    } // if ( holder && (reason == eValueChangedReasonPluginEdited) && getKnobGuiPointer() ) {


    if ( holder && !holder->isSetValueCurrentlyPossible() ) {
        ///If we cannot set value, queue it
        if ( holder && getEvaluateOnChange() ) {
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
                    makeKeyFrame(curve.get(), time, view, v, &k);
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
    } else {
        ///There might be stuff in the queue that must be processed first
        dequeueValuesSet(true);
    }

    {
        QMutexLocker l(&_setValueRecursionLevelMutex);
        ++_setValueRecursionLevel;
    }

    bool hasChanged = forceHandlerEvenIfNoChange;
    {
        QMutexLocker l(&_valueMutex);
        hasChanged |= checkIfValueChanged(v, _values[dimension]);
        _values[dimension] = v;
        _guiValues[dimension] = v;
    }

    double time;
    bool timeSet = false;

    // Check if we can add automatically a new keyframe
    if ( isAnimationEnabled() && (getAnimationLevel(dimension) != eAnimationLevelNone) && //< if the knob is animated
        holder && //< the knob is part of a KnobHolder
        holder->getApp() && //< the app pointer is not NULL
        isAutoKeyingEnabled() &&
        ( (reason == eValueChangedReasonUserEdited) ||
         ( reason == eValueChangedReasonPluginEdited) ||
         ( reason == eValueChangedReasonNatronGuiEdited) ||
         ( reason == eValueChangedReasonNatronInternalEdited) ) //< the change was made by the user or plugin
        ) {
        time = getCurrentTime();
        timeSet = true;
        ret = setValueAtTime(time, v, view, dimension, reason, newKey);
        hasChanged = true;
    }

    if ( hasChanged && (ret == eValueChangedReturnCodeNoKeyframeAdded) ) { //the other cases already called this in setValueAtTime()
        if (!timeSet) {
            time = getCurrentTime();
        }
        evaluateValueChange(dimension, time, view, reason);
    }
    {
        QMutexLocker l(&_setValueRecursionLevelMutex);
        --_setValueRecursionLevel;
        assert(_setValueRecursionLevel >= 0);
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
                effect->setMultipleParamsEditLevel(KnobHolder::eMultipleParamsEditOnCreateNewCommand);
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
        effect->setMultipleParamsEditLevel(KnobHolder::eMultipleParamsEditOff);
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
    if ( !canAnimate() || !isAnimationEnabled() ) {
        qDebug() << "WARNING: Attempting to call setValueAtTime on " << getName().c_str() << " which does not have animation enabled.";
        setValue(v, view, dimension, reason, newKey);
    }

    ValueChangedReturnCodeEnum ret = eValueChangedReturnCodeNoKeyframeAdded;
    KnobHolderPtr holder = getHolder();

#ifdef DEBUG
    if ( holder && (reason == eValueChangedReasonPluginEdited) ) {
        EffectInstancePtr isEffect = toEffectInstance(holder);
        if (isEffect) {
            isEffect->checkCanSetValueAndWarn();
        }
    }
#endif

    if ( holder && (reason == eValueChangedReasonPluginEdited) && getKnobGuiPointer() ) {
        KnobHolder::MultipleParamsEditEnum paramEditLevel = holder->getMultipleParamsEditLevel();
        switch (paramEditLevel) {
            case KnobHolder::eMultipleParamsEditOff:
            default:
                break;
            case KnobHolder::eMultipleParamsEditOnCreateNewCommand: {
                if ( !get_SetValueRecursionLevel() ) {
                    {
                        QMutexLocker l(&_setValueRecursionLevelMutex);
                        ++_setValueRecursionLevel;
                    }

                    Variant vari;
                    valueToVariant(v, &vari);
                    holder->setMultipleParamsEditLevel(KnobHolder::eMultipleParamsEditOn);
                    _signalSlotHandler->s_appendParamEditChange(reason, vari, view, dimension, time, true, true);
                    {
                        QMutexLocker l(&_setValueRecursionLevelMutex);
                        --_setValueRecursionLevel;
                    }

                    return eValueChangedReturnCodeKeyframeAdded;
                }
                break;
            }
            case KnobHolder::eMultipleParamsEditOn: {
                if ( !get_SetValueRecursionLevel() ) {
                    {
                        QMutexLocker l(&_setValueRecursionLevelMutex);
                        ++_setValueRecursionLevel;
                    }

                    Variant vari;
                    valueToVariant(v, &vari);
                    _signalSlotHandler->s_appendParamEditChange(reason, vari, view, dimension, time, false, true);
                    {
                        QMutexLocker l(&_setValueRecursionLevelMutex);
                        --_setValueRecursionLevel;
                    }

                    return eValueChangedReturnCodeKeyframeAdded;
                }
                break;
            }
        } // switch (paramEditLevel) {
    }


    CurvePtr curve = getCurve(view, dimension, true);
    assert(curve);

    KeyFrame key;
    makeKeyFrame(curve.get(), time, view, v, &key);

    if (newKey) {
        *newKey = key;
    }


    if ( holder && !holder->isSetValueCurrentlyPossible() ) {
        ///If we cannot set value, queue it
        if ( holder && getEvaluateOnChange() ) {
            //We explicitly abort rendering now and do not wait for it to be done in EffectInstance::evaluate()
            //because the actual value change (which will call evaluate()) may arise well later
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

        assert(curve);

        setInternalCurveHasChanged(view, dimension, true);

        KeyFrame k;
        bool hasAnimation = curve->isAnimated();
        bool hasKeyAtTime = curve->getKeyFrameWithTime(time, &k);
        if (hasAnimation && hasKeyAtTime) {
            return eValueChangedReturnCodeKeyframeModified;
        } else {
            return eValueChangedReturnCodeKeyframeAdded;
        }
    } else {
        ///There might be stuff in the queue that must be processed first
        dequeueValuesSet(true);
    }

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
    bool newKeyFrame = curve->addKeyFrame(key);
    if (newKeyFrame) {
        ret = eValueChangedReturnCodeKeyframeAdded;
    }


    if (holder) {
        holder->setHasAnimation(true);
    }
    guiCurveCloneInternalCurve(eCurveChangeReasonInternal, view, dimension, reason);

    if (newKeyFrame) {
        std::list<double> keysAdded, keysRemoved;
        keysAdded.push_back(time);
        _signalSlotHandler->s_curveAnimationChanged(keysAdded, keysRemoved, view, dimension, eCurveChangeReasonInternal);
    } else {
        _signalSlotHandler->s_redrawGuiCurve(eCurveChangeReasonInternal, view, dimension);
    }
    if (hasChanged) {

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
        if (holder->getMultipleParamsEditLevel() == KnobHolder::eMultipleParamsEditOff) {
            holder->setMultipleParamsEditLevel(KnobHolder::eMultipleParamsEditOnCreateNewCommand);
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
        holder->setMultipleParamsEditLevel(KnobHolder::eMultipleParamsEditOff);
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
        if (holder->getMultipleParamsEditLevel() == KnobHolder::eMultipleParamsEditOff) {
            holder->setMultipleParamsEditLevel(KnobHolder::eMultipleParamsEditOnCreateNewCommand);
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
        holder->setMultipleParamsEditLevel(KnobHolder::eMultipleParamsEditOff);
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
        if (holder->getMultipleParamsEditLevel() == KnobHolder::eMultipleParamsEditOff) {
            holder->setMultipleParamsEditLevel(KnobHolder::eMultipleParamsEditOnCreateNewCommand);
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
            ret = setValueAtTime(time, it->value, view, dimensionStartIndex + i, reason, 0 /*outKey*/, hasChanged);
            hasChanged |= (ret != eValueChangedReturnCodeNothingChanged);

            if (next != keysPerDimension[i].end()) {
                ++next;
            }
        }
    }

    if (doEditEnd) {
        holder->setMultipleParamsEditLevel(KnobHolder::eMultipleParamsEditOff);
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
