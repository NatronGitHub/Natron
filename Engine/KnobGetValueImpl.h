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

#ifndef KNOBGETVALUEIMPL_H
#define KNOBGETVALUEIMPL_H


// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Knob.h"

NATRON_NAMESPACE_ENTER

template <typename T>
bool
Knob<T>::getValueFromExpression(double time,
                                ViewIdx view,
                                int dimension,
                                bool clamp,
                                T* ret)
{
    ///Prevent recursive call of the expression
    if (getExpressionRecursionLevel() > 0) {
        return false;
    }


    ///Check first if a value was already computed:

    {
        QMutexLocker k(&_valueMutex);
        typename FrameValueMap::iterator found = _exprRes[dimension].find(time);
        if ( found != _exprRes[dimension].end() ) {
            *ret = found->second;

            return true;
        }
    }

    bool exprWasValid = isExpressionValid(dimension, 0);
    {
        EXPR_RECURSION_LEVEL();
        std::string error;
        bool exprOk = evaluateExpression(time, view,  dimension, ret, &error);
        if (!exprOk) {
            setExpressionInvalid(dimension, false, error);

            return false;
        } else {
            if (!exprWasValid) {
                setExpressionInvalid(dimension, true, error);
            }
        }
    }

    if (clamp) {
        *ret =  clampToMinMax(*ret, dimension);
    }

    QMutexLocker k(&_valueMutex);
    _exprRes[dimension].insert( std::make_pair(time, *ret) );

    return true;
} // getValueFromExpression

template <>
bool
KnobStringBase::getValueFromExpression_pod(double time,
                                           ViewIdx view,
                                           int dimension,
                                           bool /*clamp*/,
                                           double* ret)
{
    ///Prevent recursive call of the expression


    if (getExpressionRecursionLevel() > 0) {
        return false;
    }

    bool exprWasValid = isExpressionValid(dimension, 0);
    {
        EXPR_RECURSION_LEVEL();
        std::string error;
        bool exprOk = evaluateExpression_pod(time, view,  dimension, ret, &error);
        if (!exprOk) {
            setExpressionInvalid(dimension, false, error);

            return false;
        } else {
            if (!exprWasValid) {
                setExpressionInvalid(dimension, true, error);
            }
        }
    }

    return true;
} // getValueFromExpression_pod

template <typename T>
bool
Knob<T>::getValueFromExpression_pod(double time,
                                    ViewIdx view,
                                    int dimension,
                                    bool clamp,
                                    double* ret)
{
    ///Prevent recursive call of the expression


    if (getExpressionRecursionLevel() > 0) {
        return false;
    }


    ///Check first if a value was already computed:


    QMutexLocker k(&_valueMutex);
    typename FrameValueMap::iterator found = _exprRes[dimension].find(time);
    if ( found != _exprRes[dimension].end() ) {
        *ret = found->second;

        return true;
    }


    bool exprWasValid = isExpressionValid(dimension, 0);
    {
        EXPR_RECURSION_LEVEL();
        std::string error;
        bool exprOk = evaluateExpression_pod(time, view, dimension, ret, &error);
        if (!exprOk) {
            setExpressionInvalid(dimension, false, error);

            return false;
        } else {
            if (!exprWasValid) {
                setExpressionInvalid(dimension, true, error);
            }
        }
    }

    if (clamp) {
        *ret =  clampToMinMax(*ret, dimension);
    }
    
    //QWriteLocker k(&_valueMutex);
    _exprRes[dimension].insert( std::make_pair(time, *ret) );
    
    return true;
} // getValueFromExpression_pod

template <typename T>
T
Knob<T>::getValue(int dimension,
                  ViewSpec view,
                  bool clamp)
{
    assert( !view.isAll() );
    if (view.isAll()) {
        throw std::invalid_argument("Knob::getValue: Invalid ViewSpec: all views has no meaning in this function");
    }

    if ( ( dimension >= (int)_values.size() ) || (dimension < 0) ) {
        return T();
    }

    bool useGuiValues = QThread::currentThread() == qApp->thread();
    if (useGuiValues) {
        //Never clamp when using gui values
        clamp = false;
    }

    // If an expression is set, read from expression
    std::string hasExpr = getExpression(dimension);
    if ( !hasExpr.empty() ) {
        T ret;
        double time = getCurrentTime();
        if ( getValueFromExpression(time, /*view*/ view, dimension, clamp, &ret) ) {
            return ret;
        }
    }

    // If animated, call getValueAtTime instead
    if ( isAnimated(dimension, view) ) {
        return getValueAtTime(getCurrentTime(), dimension, view, clamp);
    }

    // If the knob is slaved to another knob, returns the other knob value
    std::pair<int, KnobIPtr > master = getMaster(dimension);
    if (master.second) {
        return getValueFromMaster(view, master.first, master.second, clamp);
    }


    // Figure out the view to read
    // Read main view by default if the knob is not multi-view
    ViewIdx view_i(0);
    {
        QMutexLocker k(&_valueMutex);
        // If the knob has at least one view split-off, figure out which view we are going to read
        if (hasViewsSplitOff(dimension)) {
            if (view.isCurrent()) {
                view_i = getCurrentView();
            } else {
                assert(view.isViewIdx());
                view_i = ViewIdx(view.value());
            }
        }
        typename PerViewValueMap::const_iterator foundView;
        if (useGuiValues) {
            foundView = _guiValues[dimension].find(view_i);
        } else {
            foundView = _values[dimension].find(view_i);
        }
        if (clamp) {
            const T& ret = foundView->second;
            return clampToMinMax(ret, dimension);
        } else {
            return foundView->second;
        }
    } // QMutexLocker k(&_valueMutex);


} // getValue



template <>
std::string
KnobStringBase::getValueFromMasterAt(double time,
                                     ViewSpec view,
                                     int dimension,
                                     const KnobIPtr& master)
{
    KnobStringBasePtr isString = toKnobStringBase(master);
    assert(isString); //< other data types aren't supported
    if (isString) {
        return isString->getValueAtTime(time, dimension, view, false);
    }

    // coverity[dead_error_line]
    return std::string();
} // getValueFromMasterAt

template <>
std::string
KnobStringBase::getValueFromMaster(ViewSpec view,
                                   int dimension,
                                   const KnobIPtr& master,
                                   bool /*clamp*/)
{
    KnobStringBasePtr isString = toKnobStringBase(master);
    assert(isString); //< other data types aren't supported
    if (isString) {
        return isString->getValue(dimension, view, false);
    }

    // coverity[dead_error_line]
    return std::string();
} // getValueFromMaster

template <typename T>
T
Knob<T>::getValueFromMasterAt(double time,
                              ViewSpec view,
                              int dimension,
                              const KnobIPtr& master)
{
    KnobIntBasePtr isInt = toKnobIntBase(master);
    KnobBoolBasePtr isBool = toKnobBoolBase(master);
    KnobDoubleBasePtr isDouble = toKnobDoubleBase(master);
    assert( master->isTypePOD() && (isInt || isBool || isDouble) ); //< other data types aren't supported
    if (isInt) {
        return isInt->getValueAtTime(time, dimension, view);
    } else if (isBool) {
        return isBool->getValueAtTime(time, dimension, view);
    } else if (isDouble) {
        return isDouble->getValueAtTime(time, dimension, view);
    }

    return T();
} // getValueFromMasterAt

template <typename T>
T
Knob<T>::getValueFromMaster(ViewSpec view,
                            int dimension,
                            const KnobIPtr& master,
                            bool clamp)
{
    KnobIntBasePtr isInt = toKnobIntBase(master);
    KnobBoolBasePtr isBool = toKnobBoolBase(master);
    KnobDoubleBasePtr isDouble = toKnobDoubleBase(master);
    assert( master->isTypePOD() && (isInt || isBool || isDouble) ); //< other data types aren't supported
    if (isInt) {
        return (T)isInt->getValue(dimension, view, clamp);
    } else if (isBool) {
        return (T)isBool->getValue(dimension, view, clamp);
    } else if (isDouble) {
        return (T)isDouble->getValue(dimension, view, clamp);
    }

    return T();
} // getValueFromMaster



template <typename T>
bool
Knob<T>::getValueFromCurve(double time,
                           ViewSpec view,
                           int dimension,
                           bool useGuiCurve,
                           bool byPassMaster,
                           bool clamp,
                           T* ret)
{
    CurvePtr curve;

    if (useGuiCurve) {
        curve = getGuiCurve(view, dimension, byPassMaster);
    }
    if (!curve) {
        curve = getCurve(view, dimension, byPassMaster);
    }
    if ( curve && (curve->getKeyFramesCount() > 0) ) {
        //getValueAt already clamps to the range for us
        *ret = (T)curve->getValueAt(time, clamp);

        return true;
    }

    return false;
} // getValueFromCurve

template <>
bool
KnobStringBase::getValueFromCurve(double time,
                                  ViewSpec view,
                                  int dimension,
                                  bool useGuiCurve,
                                  bool byPassMaster,
                                  bool /*clamp*/,
                                  std::string* ret)
{
    AnimatingKnobStringHelper* isStringAnimated = dynamic_cast<AnimatingKnobStringHelper* >(this);

    if (isStringAnimated) {
        *ret = isStringAnimated->getStringAtTime(time, view, dimension);
        ///ret is not empty if the animated string knob has a custom interpolation
        if ( !ret->empty() ) {
            return true;
        }
    }
    assert( ret->empty() );
    CurvePtr curve;
    if (useGuiCurve) {
        curve = getGuiCurve(view, dimension, byPassMaster);
    }
    if (!curve) {
        curve = getCurve(view, dimension, byPassMaster);
    }
    if ( curve && (curve->getKeyFramesCount() > 0) ) {
        assert(isStringAnimated);
        if (isStringAnimated) {
            isStringAnimated->stringFromInterpolatedValue(curve->getValueAt(time), view, ret);
        }

        return true;
    }

    return false;
} // getValueFromCurve


template <>
double
KnobStringBase::getRawCurveValueAt(double time,
                                   ViewSpec view,
                                   int dimension)
{
    CurvePtr curve  = getCurve(view, dimension, true);

    if ( curve && (curve->getKeyFramesCount() > 0) ) {
        //getValueAt already clamps to the range for us
        return curve->getValueAt(time, false); //< no clamping to range!
    }

    return 0;
}

template <typename T>
double
Knob<T>::getRawCurveValueAt(double time,
                            ViewSpec view,
                            int dimension)
{
    CurvePtr curve  = getCurve(view, dimension, true);

    if ( curve && (curve->getKeyFramesCount() > 0) ) {
        //getValueAt already clamps to the range for us
        return curve->getValueAt(time, false); //< no clamping to range!
    }
    QMutexLocker l(&_valueMutex);
    T ret = _values[dimension];

    return clampToMinMax(ret, dimension);
} // getRawCurveValueAt

template <typename T>
double
Knob<T>::getValueAtWithExpression(double time,
                                  ViewSpec view,
                                  int dimension)
{
    bool exprValid = isExpressionValid(dimension, 0);
    std::string expr = getExpression(dimension);

    if (!expr.empty() && exprValid) {
        double ret;
        if ( getValueFromExpression_pod(time, /*view*/ ViewIdx(0), dimension, false, &ret) ) {
            return ret;
        }
    }

    return getRawCurveValueAt(time, view, dimension);
} // getValueAtWithExpression

template<typename T>
T
Knob<T>::getValueAtTime(double time,
                        int dimension,
                        ViewSpec view,
                        bool clamp,
                        bool byPassMaster)
{
    assert( !view.isAll() );
    if (view.isAll()) {
        throw std::invalid_argument("Knob::getValueAtTime: Invalid ViewSpec: all views has no meaning in this function");
    }
    if  ( ( dimension >= (int)_values.size() ) || (dimension < 0) ) {
        return T();
    }

    bool useGuiValues = QThread::currentThread() == qApp->thread();
    std::string hasExpr = getExpression(dimension);
    if ( !hasExpr.empty() ) {
        T ret;
        if ( getValueFromExpression(time, /*view*/ ViewIdx(0), dimension, clamp, &ret) ) {
            return ret;
        }
    }

    ///if the knob is slaved to another knob, returns the other knob value
    std::pair<int, KnobIPtr > master = getMaster(dimension);
    if (!byPassMaster && master.second) {
        return getValueFromMasterAt(time, view, master.first, master.second);
    }

    T ret;
    if ( getValueFromCurve(time, view, dimension, useGuiValues, byPassMaster, clamp, &ret) ) {
        return ret;
    }

    /*if the knob as no keys at this dimension, return the value
     at the requested dimension.*/
    if (master.second && !byPassMaster) {
        return getValueFromMaster(view, master.first, master.second, clamp);
    }
    QMutexLocker l(&_valueMutex);
    if (clamp) {
        ret = _values[dimension];

        return clampToMinMax(ret, dimension);
    } else {
        return _values[dimension];
    }
} // getValueAtTime


NATRON_NAMESPACE_EXIT

#endif // KNOBGETVALUEIMPL_H
