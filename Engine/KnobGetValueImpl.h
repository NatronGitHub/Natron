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

#ifndef KNOBGETVALUEIMPL_H
#define KNOBGETVALUEIMPL_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include "KnobPrivate.h"

#include "Engine/EffectInstance.h"
#include "Engine/KnobItemsTable.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

template <typename T>
bool
Knob<T>::getValueFromExpression(TimeValue time,
                                ViewIdx view,
                                DimIdx dimension,
                                bool clamp,
                                T* ret)
{
    ///Prevent recursive call of the expression
    if (getExpressionRecursionLevel() > 0) {
        return false;
    }

    if (dimension < 0 || dimension >= getNDimensions()) {
        throw std::invalid_argument("Knob::getValueFromExpression: dimension out of range");
    }

    ViewIdx view_i = checkIfViewExistsOrFallbackMainView(view);

    // Check for a cached expression result
    // TODO

    bool cachingEnabled = isExpressionsResultsCachingEnabled();
    bool exprWasValid = isLinkValid(dimension, view_i, 0);
    {
        EXPR_RECURSION_LEVEL();

        TimeViewPair key = {time, view};

        bool exprOk = false;
        if (cachingEnabled) {
            QMutexLocker k(&_data->expressionResultsMutex);
            assert(dimension < (int)_data->expressionResults.size());
            typename ExpressionCache::const_iterator foundCached = _data->expressionResults[dimension][view].find(key);
            if (foundCached != _data->expressionResults[dimension][view].end()) {
                exprOk = true;
                *ret = foundCached->second;
            }
        }
        std::string error;
        if (!exprOk) {
            if (getExpressionLanguage(view, dimension) == eExpressionLanguagePython) {
                EffectInstancePtr effect = toEffectInstance(getHolder());
                if (effect) {
                    appPTR->setLastPythonAPICaller_TLS(effect);
                }
            }
            
            exprOk = evaluateExpression(time, view_i,  dimension, ret, &error);
            if (exprOk && cachingEnabled) {
                QMutexLocker k(&_data->expressionResultsMutex);
                _data->expressionResults[dimension][view].insert(std::make_pair(key, *ret));
            }
        }
        if (!exprOk) {
            setLinkStatus(dimension, view_i, false, error);

            return false;
        } else {
            if (!exprWasValid) {
                setLinkStatus(dimension, view_i, true, error);
            }
        }
    }

    if (clamp) {
        *ret =  clampToMinMax(*ret, dimension);
    }

  
    return true;
} // getValueFromExpression

template <>
bool
KnobStringBase::getValueFromExpression_pod(TimeValue time,
                                           ViewIdx view,
                                           DimIdx dimension,
                                           bool /*clamp*/,
                                           double* ret)
{
    ///Prevent recursive call of the expression


    if (getExpressionRecursionLevel() > 0) {
        return false;
    }

    if (dimension < 0 || dimension >= getNDimensions()) {
        throw std::invalid_argument("Knob::getValueFromExpression_pod: dimension out of range");
    }

    ViewIdx view_i = checkIfViewExistsOrFallbackMainView(view);

    bool exprWasValid = isLinkValid(dimension, view_i, 0);
    {
        EXPR_RECURSION_LEVEL();
        std::string error;
        bool exprOk = evaluateExpression_pod(time, view_i,  dimension, ret, &error);
        if (!exprOk) {
            setLinkStatus(dimension, ViewSetSpec(view_i), false, error);

            return false;
        } else {
            if (!exprWasValid) {
                setLinkStatus(dimension, ViewSetSpec(view_i), true, error);
            }
        }
    }

    return true;
} // getValueFromExpression_pod

template <typename T>
bool
Knob<T>::getValueFromExpression_pod(TimeValue time,
                                    ViewIdx view,
                                    DimIdx dimension,
                                    bool clamp,
                                    double* ret)
{
    ///Prevent recursive call of the expression


    if (getExpressionRecursionLevel() > 0) {
        return false;
    }

    if (dimension < 0 || dimension >= getNDimensions()) {
        throw std::invalid_argument("Knob::getValueFromExpression_pod: dimension out of range");
    }

    ViewIdx view_i = checkIfViewExistsOrFallbackMainView(view);
    
    // Check for a cached expression result
    // Todo

    bool cachingEnabled = isExpressionsResultsCachingEnabled();
    bool exprWasValid = isLinkValid(dimension, view_i, 0);
    {
        EXPR_RECURSION_LEVEL();
        std::string error;
        bool exprOk = false;
        TimeViewPair key = {time, view};
        if (cachingEnabled) {
            QMutexLocker k(&_data->expressionResultsMutex);
            assert(dimension < (int)_data->expressionResults.size());
            typename ExpressionCache::const_iterator foundCached = _data->expressionResults[dimension][view].find(key);
            if (foundCached != _data->expressionResults[dimension][view].end()) {
                exprOk = true;
                *ret = (double)foundCached->second;
            }
        }
        if (!exprOk) {
            exprOk = evaluateExpression_pod(time, view_i, dimension, ret, &error);
            if (exprOk && cachingEnabled) {
                QMutexLocker k(&_data->expressionResultsMutex);
                _data->expressionResults[dimension][view].insert(std::make_pair(key, (T)*ret));
            }
        }
        if (!exprOk) {
            setLinkStatus(dimension, ViewSetSpec(view_i), false, error);

            return false;
        } else {
            if (!exprWasValid) {
                setLinkStatus(dimension, ViewSetSpec(view_i), true, error);
            }
        }
    }

    if (clamp) {
        *ret =  clampToMinMax(*ret, dimension);
    }
    

    
    return true;
} // getValueFromExpression_pod

template <typename T>
T
Knob<T>::getValueInternal(TimeValue tlsCurrentTime,
                          DimIdx dimension,
                          ViewIdx view,
                          bool clamp)
{
    ValueKnobDimView<T>* dataForDimView = dynamic_cast<ValueKnobDimView<T>*>(getDataForDimView(dimension, view).get());
    if (!dataForDimView) {
        return T();
    }
    {
        QMutexLocker k(&dataForDimView->valueMutex);
        if (clamp) {
            T ret = clampToMinMax(dataForDimView->value, dimension);
            if (_valuesCache) {
                DimTimeView key;
                key.time = tlsCurrentTime;
                key.view = view;
                key.dimension = dimension;
                (*_valuesCache)[key] = ret;
            }
            return ret;
        } else {
            const T& ret = dataForDimView->value;
            if (_valuesCache) {
                DimTimeView key;
                key.time = tlsCurrentTime;
                key.view = view;
                key.dimension = dimension;
                (*_valuesCache)[key] = ret;
            }
            return ret;
        }
    }
} // getValueInternal

template <typename T>
T
Knob<T>::getValue(DimIdx dimension,
                  ViewIdx view,
                  bool clamp)
{

    if ( ( dimension >= getNDimensions() ) || (dimension < 0) ) {
        throw std::invalid_argument("Knob::getValue: dimension out of range");
    }



    // Figure out the view to read
    ViewIdx view_i = checkIfViewExistsOrFallbackMainView(view);

    boost::shared_ptr<Knob<T> > thisShared = boost::dynamic_pointer_cast<Knob<T> >(shared_from_this());


    TimeValue currentTime = getCurrentRenderTime();
    // Check if value is already in TLS when rendering
    if (_valuesCache) {
        DimTimeView key;
        key.time = currentTime;
        key.view = view_i;
        key.dimension = dimension;
        typename ValuesCacheMap::const_iterator foundCached = _valuesCache->find(key);
        if (foundCached != _valuesCache->end()) {
            return foundCached->second;
        }
    }


    // If an expression is set, read from expression
    std::string hasExpr = getExpression(dimension, view_i);
    if ( !hasExpr.empty() ) {
        T ret;
        if ( getValueFromExpression(currentTime, view, dimension, clamp, &ret) ) {
            if (_valuesCache) {
                DimTimeView key;
                key.time = currentTime;
                key.view = view_i;
                key.dimension = dimension;
                (*_valuesCache)[key] = ret;
            }
            return ret;
        }
    }

    // If animated, call getValueAtTime instead
    if ( isAnimated(dimension, view) ) {
        return getValueAtTime(currentTime, dimension, view, clamp);
    }

    return getValueInternal(currentTime, dimension, view_i, clamp);

} // getValue


template <>
std::string
ValueKnobDimView<std::string>::getValueFromKeyFrame(const KeyFrame& k)
{
    std::string str;
    k.getPropertySafe(kKeyFramePropString, 0, &str);
    return str;
}



template <typename T>
T
ValueKnobDimView<T>::getValueFromKeyFrame(const KeyFrame& k)
{
    return k.getValue();
}

template <typename T>
T
Knob<T>::getValueFromKeyFrame(const KeyFrame& key, DimIdx dimension, ViewIdx view) const
{
    ValueKnobDimView<T>* data = dynamic_cast<ValueKnobDimView<T>*>(getDataForDimView(dimension, view).get());
    assert(data);
    return data->getValueFromKeyFrame(key);
}


template <typename T>
bool
Knob<T>::getCurveKeyFrame(TimeValue time,
                          DimIdx dimension,
                          ViewIdx view,
                          bool clampToMinMax,
                          KeyFrame* ret)
{

    ViewIdx view_i = checkIfViewExistsOrFallbackMainView(view);
    CurvePtr curve = getAnimationCurve(view_i, dimension);

    if ( curve && (curve->getKeyFramesCount() > 0) ) {
        //getValueAt already clamps to the range for us
        *ret = curve->getValueAt(time, clampToMinMax);

        return true;
    }

    return false;
} // getValueFromCurve



template <typename T>
double
Knob<T>::getValueAtWithExpression(TimeValue time,
                                  ViewIdx view,
                                  DimIdx dimension)
{
    bool exprValid = isLinkValid(dimension, view, 0);
    std::string expr = getExpression(dimension, view);

    if (!expr.empty() && exprValid) {
        double ret;
        if ( getValueFromExpression_pod(time, view, dimension, false, &ret) ) {
            return ret;
        }
    }

    KeyFrame key;
    bool gotKey = getCurveKeyFrame(time, dimension, view, true, &key);
    if (gotKey) {
        return key.getValue();
    } else {
        return 0.;
    }
} // getValueAtWithExpression


template<typename T>
T
Knob<T>::getValueAtTime(TimeValue time,
                        DimIdx dimension,
                        ViewIdx view,
                        bool clamp)
{

    if  ( ( dimension >= getNDimensions() ) || (dimension < 0) ) {
        throw std::invalid_argument("Knob::getValueAtTime: dimension out of range");
    }


    // Figure out the view to read
    ViewIdx view_i = checkIfViewExistsOrFallbackMainView(view);

    boost::shared_ptr<Knob<T> > thisShared = boost::dynamic_pointer_cast<Knob<T> >(shared_from_this());

    // Check if value is already in TLS when rendering
    if (_valuesCache) {
        DimTimeView key;
        key.time = time;
        key.view = view_i;
        key.dimension = dimension;
        typename ValuesCacheMap::const_iterator foundCached = _valuesCache->find(key);
        if (foundCached != _valuesCache->end()) {
            return foundCached->second;
        }
    }

    std::string hasExpr = getExpression(dimension, view);
    if ( !hasExpr.empty() ) {
        T ret;
        if ( getValueFromExpression(time, /*view*/ ViewIdx(0), dimension, clamp, &ret) ) {
            if (_valuesCache) {
                DimTimeView key;
                key.time = time;
                key.view = view_i;
                key.dimension = dimension;
                (*_valuesCache)[key] = ret;
            }
            return ret;
        }
    }



    KeyFrame keyframe;
    if ( getCurveKeyFrame(time, dimension, view_i, clamp, &keyframe) ) {
        T ret = getValueFromKeyFrame(keyframe, dimension, view_i);
        if (_valuesCache) {
            DimTimeView key;
            key.time = time;
            key.view = view_i;
            key.dimension = dimension;
            (*_valuesCache)[key] = ret;
        }
        return ret;
    }

    // If the knob as no keys at this dimension/view, return the underlying value
    return getValueInternal(time, dimension, view_i, clamp);
} // getValueAtTime


template<typename T>
double
Knob<T>::getDerivativeAtTime(TimeValue time,
                             ViewIdx view,
                             DimIdx dimension)
{
    if ( ( dimension > getNDimensions() ) || (dimension < 0) ) {
        throw std::invalid_argument("Knob::getDerivativeAtTime(): Dimension out of range");
    }
    {
        std::string expr = getExpression(dimension, view);
        if ( !expr.empty() ) {
            // Compute derivative by finite differences, using values at t-0.5 and t+0.5
            return ( getValueAtTime(TimeValue(time + 0.5), dimension, view) - getValueAtTime(TimeValue(time - 0.5), dimension, view) ) / 2.;
        }
    }

    ViewIdx view_i = checkIfViewExistsOrFallbackMainView(view);

    CurvePtr curve  = getAnimationCurve(view_i, dimension);
    if (curve->getKeyFramesCount() > 0) {
        return curve->getDerivativeAt(time);
    } else {
        /*if the knob as no keys at this dimension, the derivative is 0.*/
        return 0.;
    }
}

template<>
double
KnobStringBase::getDerivativeAtTime(TimeValue /*time*/,
                                    ViewIdx /*view*/,
                                    DimIdx /*dimension*/)
{
    throw std::invalid_argument("Knob<string>::getDerivativeAtTime() not available");
}

// Compute integral using Simpsons rule:
// \int_a^b f(x) dx = (b-a)/6 * (f(a) + 4f((a+b)/2) + f(b))
template<typename T>
double
Knob<T>::getIntegrateFromTimeToTimeSimpson(TimeValue time1,
                                           TimeValue time2,
                                           ViewIdx view,
                                           DimIdx dimension)
{
    double fa = getValueAtTime(time1, dimension, view);
    double fm = getValueAtTime( TimeValue((time1 + time2) / 2.), dimension, view );
    double fb = getValueAtTime(time2, dimension, view);

    return (time2 - time1) / 6 * (fa + 4 * fm + fb);
}

template<typename T>
double
Knob<T>::getIntegrateFromTimeToTime(TimeValue time1,
                                    TimeValue time2,
                                    ViewIdx view,
                                    DimIdx dimension)
{
    if ( ( dimension > getNDimensions() ) || (dimension < 0) ) {
        throw std::invalid_argument("Knob::getIntegrateFromTimeToTime(): Dimension out of range");
    }
    {
        std::string expr = getExpression(dimension, view);
        if ( !expr.empty() ) {
            // Compute integral using Simpsons rule:
            // \int_a^b f(x) dx = (b-a)/6 * (f(a) + 4f((a+b)/2) + f(b))
            // The interval from time1 to time2 is split into intervals where bounds are at integer values
            int i = std::ceil(time1);
            int j = std::floor(time2);
            if (i > j) { // no integer values in the interval
                return getIntegrateFromTimeToTimeSimpson(time1, time2, view, dimension);
            }
            double val = 0.;
            // start integrating over the interval
            // first chunk
            if (time1 < i) {
                val += getIntegrateFromTimeToTimeSimpson(time1, TimeValue(i), view, dimension);
            }
            // integer chunks
            for (int t = i; t < j; ++t) {
                val += getIntegrateFromTimeToTimeSimpson(TimeValue(t), TimeValue(t + 1), view, dimension);
            }
            // last chunk
            if (j < time2) {
                val += getIntegrateFromTimeToTimeSimpson(TimeValue(j), time2, view, dimension);
            }

            return val;
        }
    }

    ViewIdx view_i = checkIfViewExistsOrFallbackMainView(view);

    CurvePtr curve  = getAnimationCurve(view_i, dimension);
    if (curve->getKeyFramesCount() > 0) {
        return curve->getIntegrateFromTo(time1, time2);
    } else {
        // if the knob as no keys at this dimension, the integral is trivial
        T value = getValue(dimension, view_i);
        return value * (time2 - time1);
    }
} // getIntegrateFromTimeToTime

template<>
double
KnobStringBase::getIntegrateFromTimeToTimeSimpson(TimeValue /*time1*/,
                                                  TimeValue /*time2*/,
                                                  ViewIdx /*view*/,
                                                  DimIdx /*dimension*/)
{
    return 0; // dummy value
}

template<>
double
KnobStringBase::getIntegrateFromTimeToTime(TimeValue /*time1*/,
                                           TimeValue /*time2*/,
                                           ViewIdx /*view*/,
                                           DimIdx /*dimension*/)
{
    throw std::invalid_argument("Knob<string>::getIntegrateFromTimeToTime() not available");
}



NATRON_NAMESPACE_EXIT

#endif // KNOBGETVALUEIMPL_H
