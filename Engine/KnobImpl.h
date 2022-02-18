/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2022 The Natron developers
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

#ifndef KNOBIMPL_H
#define KNOBIMPL_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include "Knob.h"

#include <cfloat>
#include <stdexcept>
#include <string>
#include <algorithm> // min, max

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
#include <boost/math/special_functions/fpclassify.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
#endif

#include <QtCore/QString>
#include <QtCore/QDebug>
#include <QtCore/QCoreApplication>
#include <QtCore/QThread>

CLANG_DIAG_OFF(mismatched-tags)
GCC_DIAG_OFF(unused-parameter)
#include <shiboken.h>
CLANG_DIAG_ON(mismatched-tags)
GCC_DIAG_ON(unused-parameter)

#include "Global/PythonUtils.h"

#include "Engine/Curve.h"
#include "Engine/AppInstance.h"
#include "Engine/Project.h"
#include "Engine/EffectInstance.h"
#include "Engine/KnobTypes.h"
#include "Engine/ViewIdx.h"
#include "Engine/EngineFwd.h"


#define EXPR_RECURSION_LEVEL() KnobHelper::ExprRecursionLevel_RAII __recursionLevelIncrementer__(this)

NATRON_NAMESPACE_ENTER

///template specializations

template <typename T>
void
Knob<T>::initMinMax()
{
}

template <>
void
KnobDoubleBase::initMinMax()
{
    for (int i = 0; i < getDimension(); ++i) {
        _minimums[i] = -DBL_MAX;
        _maximums[i] = DBL_MAX;
        _displayMins[i] = -DBL_MAX;
        _displayMaxs[i] = DBL_MAX;
    }
}

template <>
void
KnobIntBase::initMinMax()
{
    for (int i = 0; i < getDimension(); ++i) {
        _minimums[i] = INT_MIN;
        _maximums[i] = INT_MAX;
        _displayMins[i] = INT_MIN;
        _displayMaxs[i] = INT_MAX;
    }
}

template <typename T>
Knob<T>::Knob(KnobHolder*  holder,
              const std::string & description,
              int dimension,
              bool declaredByPlugin )
    : KnobHelper(holder, description, dimension, declaredByPlugin)
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    , _valueMutex()
#else
    , _valueMutex(QMutex::Recursive)
#endif
    , _values(dimension)
    , _guiValues(dimension)
    , _defaultValues(dimension)
    , _exprRes(dimension)
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    , _minMaxMutex()
#else
    , _minMaxMutex(QReadWriteLock::Recursive)
#endif
    , _minimums(dimension)
    , _maximums(dimension)
    , _displayMins(dimension)
    , _displayMaxs(dimension)
    , _setValuesQueueMutex()
    , _setValuesQueue()
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    , _setValueRecursionLevelMutex()
#else
    , _setValueRecursionLevelMutex(QMutex::Recursive)
#endif
    , _setValueRecursionLevel(0)
{
    initMinMax();
}

template <typename T>
Knob<T>::~Knob()
{
}

template <typename T>
void
Knob<T>::signalMinMaxChanged(const T& mini,
                             const T& maxi,
                             int dimension)
{
    if (_signalSlotHandler) {
        _signalSlotHandler->s_minMaxChanged(mini, maxi, dimension);
    }
}

template <typename T>
void
Knob<T>::signalDisplayMinMaxChanged(const T& mini,
                                    const T& maxi,
                                    int dimension)
{
    if (_signalSlotHandler) {
        _signalSlotHandler->s_displayMinMaxChanged(mini, maxi, dimension);
    }
}

template <>
void
KnobStringBase::signalMinMaxChanged(const std::string& /*mini*/,
                                       const std::string& /*maxi*/,
                                       int /*dimension*/)
{
}

template <>
void
KnobStringBase::signalDisplayMinMaxChanged(const std::string& /*mini*/,
                                              const std::string& /*maxi*/,
                                              int /*dimension*/)
{
}

template <typename T>
void
Knob<T>::setMinimum(const T& mini,
                    int dimension)
{
    T maxi;
    {
        QWriteLocker k(&_minMaxMutex);
        _minimums[dimension] = mini;
        maxi = _maximums[dimension];
    }

    signalMinMaxChanged(mini, maxi, dimension);
}

template <typename T>
void
Knob<T>::setMaximum(const T& maxi,
                    int dimension)
{
    T mini;
    {
        QWriteLocker k(&_minMaxMutex);
        _maximums[dimension] = maxi;
        mini = _minimums[dimension];
    }

    signalMinMaxChanged(mini, maxi, dimension);
}

template <typename T>
void
Knob<T>::setDisplayMinimum(const T& mini,
                           int dimension)
{
    T maxi;
    {
        QWriteLocker k(&_minMaxMutex);
        _displayMins[dimension] = mini;
        maxi = _displayMaxs[dimension];
    }

    signalDisplayMinMaxChanged(mini, maxi, dimension);
}

template <typename T>
void
Knob<T>::setDisplayMaximum(const T& maxi,
                           int dimension)
{
    T mini;
    {
        QWriteLocker k(&_minMaxMutex);
        _displayMaxs[dimension] = maxi;
        mini = _displayMins[dimension];
    }

    signalDisplayMinMaxChanged(mini, maxi, dimension);
}

template <typename T>
void
Knob<T>::setMinimumsAndMaximums(const std::vector<T> &minis,
                                const std::vector<T> &maxis)
{
    {
        QWriteLocker k(&_minMaxMutex);
        _minimums = minis;
        _maximums = maxis;
    }
    for (unsigned int i = 0; i < minis.size(); ++i) {
        signalMinMaxChanged(minis[i], maxis[i], i);
    }
}

template <typename T>
void
Knob<T>::setDisplayMinimumsAndMaximums(const std::vector<T> &minis,
                                       const std::vector<T> &maxis)
{
    {
        QWriteLocker k(&_minMaxMutex);
        _displayMins = minis;
        _displayMaxs = maxis;
    }
    for (unsigned int i = 0; i < minis.size(); ++i) {
        signalDisplayMinMaxChanged(minis[i], maxis[i], i);
    }
}

template <typename T>
const std::vector<T>&
Knob<T>::getMinimums() const
{
    return _minimums;
}

template <typename T>
const std::vector<T>&
Knob<T>::getMaximums() const
{
    return _maximums;
}

template <typename T>
const std::vector<T>&
Knob<T>::getDisplayMinimums() const
{
    return _displayMins;
}

template <typename T>
const std::vector<T>&
Knob<T>::getDisplayMaximums() const
{
    return _displayMaxs;
}

template <typename T>
T
Knob<T>::getMinimum(int dimension) const
{
    QReadLocker k(&_minMaxMutex);

    return _minimums[dimension];
}

template <typename T>
T
Knob<T>::getMaximum(int dimension) const
{
    QReadLocker k(&_minMaxMutex);

    return _maximums[dimension];
}

template <typename T>
T
Knob<T>::getDisplayMinimum(int dimension) const
{
    QReadLocker k(&_minMaxMutex);

    return _displayMins[dimension];
}

template <typename T>
T
Knob<T>::getDisplayMaximum(int dimension) const
{
    QReadLocker k(&_minMaxMutex);

    return _displayMaxs[dimension];
}

template <typename T>
T
Knob<T>::clampToMinMax(const T& value,
                       int dimension) const
{
    QReadLocker k(&_minMaxMutex);

    return std::max( (double)_minimums[dimension], std::min( (double)_maximums[dimension], (double)value ) );
}

template <>
std::string
KnobStringBase::clampToMinMax(const std::string& value,
                                 int /*dimension*/) const
{
    return value;
}

template <>
bool
KnobBoolBase::clampToMinMax(const bool& value,
                          int /*dimension*/) const
{
    return value;
}

template <>
int
KnobHelper::pyObjectToType(PyObject* o)
{
    return (int)PyInt_AsLong(o);
}

template <>
bool
KnobHelper::pyObjectToType(PyObject* o)
{
    if (PyObject_IsTrue(o) == 1) {
        return true;
    } else {
        return false;
    }
}

template <>
double
KnobHelper::pyObjectToType(PyObject* o)
{
    return (double)PyFloat_AsDouble(o);
}

template <>
std::string
KnobHelper::pyObjectToType(PyObject* o)
{
    return NATRON_PYTHON_NAMESPACE::PyStringToStdString(o);
}

inline unsigned int
hashFunction(unsigned int a)
{
    a = (a ^ 61) ^ (a >> 16);
    a = a + (a << 3);
    a = a ^ (a >> 4);
    a = a * 0x27d4eb2d;
    a = a ^ (a >> 15);

    return a;
}

template <typename T>
bool
Knob<T>::evaluateExpression(const std::string& expr,
                            T* value,
                            std::string* error)
{
    PythonGILLocker pgl;
    PyObject *ret;

    ///Reset the random state to reproduce the sequence
    //randomSeed( time, hashFunction(dimension) );
    bool exprOk = executeExpression(expr, &ret, error);
    if (!exprOk) {
        return false;
    }
    *value =  pyObjectToType<T>(ret);
    Py_DECREF(ret); //< new ref
    return true;
}

template <typename T>
bool
Knob<T>::evaluateExpression(double time,
                            ViewIdx view,
                            int dimension,
                            T* value,
                            std::string* error)
{
    PythonGILLocker pgl;
    PyObject *ret;

    ///Reset the random state to reproduce the sequence
    randomSeed( time, hashFunction(dimension) );
    bool exprOk = executeExpression(time, view, dimension, &ret, error);
    if (!exprOk) {
        return false;
    }
    *value =  pyObjectToType<T>(ret);
    Py_DECREF(ret); //< new ref
    return true;
}

template <typename T>
bool
Knob<T>::evaluateExpression_pod(double time,
                                ViewIdx view,
                                int dimension,
                                double* value,
                                std::string* error)
{
    PythonGILLocker pgl;
    PyObject *ret;

    ///Reset the random state to reproduce the sequence
    randomSeed( time, hashFunction(dimension) );
    bool exprOk = executeExpression(time, view, dimension, &ret, error);
    if (!exprOk) {
        return false;
    }

    if ( PyFloat_Check(ret) ) {
        *value =  (double)PyFloat_AsDouble(ret);
    } else if ( PyInt_Check(ret) ) {
        *value = (int)PyInt_AsLong(ret);
    } else if ( PyLong_Check(ret) ) {
        *value = (int)PyLong_AsLong(ret);
    } else if (PyObject_IsTrue(ret) == 1) {
        *value = 1;
    } else {
        //Strings should always fall here
        *value = 0.;
    }

    return true;
}

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
}

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
}

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
}

template <>
std::string
KnobStringBase::getValueFromMasterAt(double time,
                                        ViewSpec view,
                                        int dimension,
                                        KnobI* master)
{
    KnobStringBase* isString = dynamic_cast<KnobStringBase* >(master);
    assert(isString); //< other data types aren't supported
    if (isString) {
        return isString->getValueAtTime(time, dimension, view, false);
    }

    // coverity[dead_error_line]
    return std::string();
}

template <>
std::string
KnobStringBase::getValueFromMaster(ViewSpec view,
                                      int dimension,
                                      KnobI* master,
                                      bool /*clamp*/)
{
    KnobStringBase* isString = dynamic_cast<KnobStringBase* >(master);
    assert(isString); //< other data types aren't supported
    if (isString) {
        return isString->getValue(dimension, view, false);
    }

    // coverity[dead_error_line]
    return std::string();
}

template <typename T>
T
Knob<T>::getValueFromMasterAt(double time,
                              ViewSpec view,
                              int dimension,
                              KnobI* master)
{
    KnobIntBase* isInt = dynamic_cast<KnobIntBase* >(master);
    KnobBoolBase* isBool = dynamic_cast<KnobBoolBase* >(master);
    KnobDoubleBase* isDouble = dynamic_cast<KnobDoubleBase* >(master);
    assert( master->isTypePOD() && (isInt || isBool || isDouble) ); //< other data types aren't supported
    if (isInt) {
        return isInt->getValueAtTime(time, dimension, view);
    } else if (isBool) {
        return isBool->getValueAtTime(time, dimension, view);
    } else if (isDouble) {
        return isDouble->getValueAtTime(time, dimension, view);
    }

    return T();
}

template <typename T>
T
Knob<T>::getValueFromMaster(ViewSpec view,
                            int dimension,
                            KnobI* master,
                            bool clamp)
{
    KnobIntBase* isInt = dynamic_cast<KnobIntBase* >(master);
    KnobBoolBase* isBool = dynamic_cast<KnobBoolBase* >(master);
    KnobDoubleBase* isDouble = dynamic_cast<KnobDoubleBase* >(master);
    assert( master->isTypePOD() && (isInt || isBool || isDouble) ); //< other data types aren't supported
    if (isInt) {
        return (T)isInt->getValue(dimension, view, clamp);
    } else if (isBool) {
        return (T)isBool->getValue(dimension, view, clamp);
    } else if (isDouble) {
        return (T)isDouble->getValue(dimension, view, clamp);
    }

    return T();
}

template <typename T>
T
Knob<T>::getValue(int dimension,
                  ViewSpec view,
                  bool clamp)
{
    assert( !view.isAll() );
    bool useGuiValues = QThread::currentThread() == qApp->thread();

#ifdef DEBUG
    // to put a breakpoint on a getValue on a specific Knob
    //if (getName() == "multi") {
    //    debugHook();
    //}
#endif

    if ( ( dimension >= (int)_values.size() ) || (dimension < 0) ) {
        return T();
    }
    std::string hasExpr = getExpression(dimension);
    if ( !hasExpr.empty() ) {
        T ret;
        double time = getCurrentTime();
        if ( getValueFromExpression(time, /*view*/ ViewIdx(0), dimension, clamp, &ret) ) {
            return ret;
        }
    }

    if ( isAnimated(dimension, view) ) {
        return getValueAtTime(getCurrentTime(), dimension, view, clamp);
    }

    ///if the knob is slaved to another knob, returns the other knob value
    std::pair<int, KnobIPtr> master = getMaster(dimension);
    if (master.second) {
        return getValueFromMaster(view, master.first, master.second.get(), clamp);
    }

    QMutexLocker l(&_valueMutex);
    if (useGuiValues) {
        if (clamp) {
            T ret = _guiValues[dimension];

            return clampToMinMax(ret, dimension);
        } else {
            return _guiValues[dimension];
        }
    } else {
        if (clamp) {
            T ret = _values[dimension];

            return clampToMinMax(ret, dimension);
        } else {
            return _values[dimension];
        }
    }
}

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
}

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
}

template<typename T>
T
Knob<T>::getValueAtTime(double time,
                        int dimension,
                        ViewSpec view,
                        bool clamp,
                        bool byPassMaster)
{
    assert( !view.isAll() );
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
    std::pair<int, KnobIPtr> master = getMaster(dimension);
    if (!byPassMaster && master.second) {
        return getValueFromMasterAt( time, view, master.first, master.second.get() );
    }

    T ret;
    if ( getValueFromCurve(time, view, dimension, useGuiValues, byPassMaster, clamp, &ret) ) {
        return ret;
    }

    /*if the knob as no keys at this dimension, return the value
       at the requested dimension.*/
    if (master.second && !byPassMaster) {
        return getValueFromMaster(view, master.first, master.second.get(), clamp);
    }
    QMutexLocker l(&_valueMutex);
    if (clamp) {
        ret = _values[dimension];

        return clampToMinMax(ret, dimension);
    } else {
        return _values[dimension];
    }
}

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
}

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
}

template <typename T>
void
Knob<T>::valueToVariant(const T & v,
                        Variant* vari)
{
    KnobIntBase* isInt = dynamic_cast<KnobIntBase*>(this);
    KnobBoolBase* isBool = dynamic_cast<KnobBoolBase*>(this);
    KnobDoubleBase* isDouble = dynamic_cast<KnobDoubleBase*>(this);
    if (isInt) {
        vari->setValue(v);
    } else if (isBool) {
        vari->setValue(v);
    } else if (isDouble) {
        vari->setValue(v);
    }
}

template <>
void
KnobStringBase::valueToVariant(const std::string & v,
                                  Variant* vari)
{
    KnobStringBase* isString = dynamic_cast<KnobStringBase*>(this);
    if (isString) {
        vari->setValue<QString>( QString::fromUtf8( v.c_str() ) );
    }
}

template<typename T>
struct Knob<T>::QueuedSetValuePrivate
{
    int dimension;
    ViewSpec view;
    T value;
    KeyFrame key;
    bool useKey;
    ValueChangedReasonEnum reason;
    bool valueChangesBlocked;

    QueuedSetValuePrivate(ViewSpec view, int dimension, const T &value, const KeyFrame &key_, bool useKey, ValueChangedReasonEnum reason_,
                          bool blockValueChanges)
        : dimension(dimension)
          , view(view)
          , value(value)
          , key(key_)
          , useKey(useKey)
          , reason(reason_)
          , valueChangesBlocked(blockValueChanges)
    {
    }
};


template<typename T>
Knob<T>::QueuedSetValue::QueuedSetValue(ViewSpec view,
                                        int dimension,
                                        const T& value,
                                        const KeyFrame& key,
                                        bool useKey,
                                        ValueChangedReasonEnum reason_,
                                        bool valueChangesBlocked)
    : _imp( new QueuedSetValuePrivate(view, dimension, value, key, useKey, reason_, valueChangesBlocked) )
{
}

template<typename T>
Knob<T>::QueuedSetValue::~QueuedSetValue()
{
}

template<typename T>
int
Knob<T>::QueuedSetValue::dimension() const
{
    return _imp->dimension;
}

template<typename T>
ViewSpec
Knob<T>::QueuedSetValue::view() const
{
    return _imp->view;
}

template<typename T>
const T &
Knob<T>::QueuedSetValue::value() const
{
    return _imp->value;
}

template<typename T>
const KeyFrame &
Knob<T>::QueuedSetValue::key() const
{
    return _imp->key;
}

template<typename T>
bool
Knob<T>::QueuedSetValue::useKey() const
{
    return _imp->useKey;
}


template<typename T>
ValueChangedReasonEnum
Knob<T>::QueuedSetValue::reason() const
{
    return _imp->reason;
}

template<typename T>
bool
Knob<T>::QueuedSetValue::valueChangesBlocked() const
{
    return _imp->valueChangesBlocked;
}

template <typename T>
KnobHelper::ValueChangedReturnCodeEnum
Knob<T>::setValue(const T & v,
                  ViewSpec view,
                  int dimension,
                  ValueChangedReasonEnum reason,
                  KeyFrame* newKey,
                  bool hasChanged)
{
    if ( (dimension < 0) || ( dimension >= (int)_values.size() ) ) {
        return eValueChangedReturnCodeNothingChanged;
    }

    KnobHelper::ValueChangedReturnCodeEnum ret = eValueChangedReturnCodeNoKeyframeAdded;
    KnobHolder* holder =  getHolder();

#ifdef DEBUG
    if ( holder && (reason == eValueChangedReasonPluginEdited) ) {
        EffectInstance* isEffect = dynamic_cast<EffectInstance*>(holder);
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
                if (_signalSlotHandler) {
                    _signalSlotHandler->s_appendParamEditChange(reason, vari, view, dimension, 0, true, false);
                }
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
                if (_signalSlotHandler) {
                    _signalSlotHandler->s_appendParamEditChange(reason, vari, view, dimension, 0, false, false);
                }
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
                if (_signalSlotHandler) {
                    _signalSlotHandler->s_setValueWithUndoStack(vari, view, dimension);
                }

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

        KnobHelper::ValueChangedReturnCodeEnum returnValue;
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

        QueuedSetValuePtr qv = boost::make_shared<QueuedSetValue>( view, dimension, v, k, returnValue != eValueChangedReturnCodeNoKeyframeAdded, reason, isValueChangesBlocked() );
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
                holder->onKnobValueChanged_public(this, reason, time, view, true);
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

    {
        QMutexLocker l(&_valueMutex);
        hasChanged |= (v != _values[dimension]);
        _values[dimension] = v;
        _guiValues[dimension] = v;
    }

    double time;
    bool timeSet = false;

    ///Add automatically a new keyframe
    if ( isAnimationEnabled() &&
         ( getAnimationLevel(dimension) != eAnimationLevelNone) && //< if the knob is animated
         holder && //< the knob is part of a KnobHolder
         holder->getApp() && //< the app pointer is not NULL
         !holder->getApp()->getProject()->isLoadingProject() && //< we're not loading the project
         ( (reason == eValueChangedReasonUserEdited) ||
           ( reason == eValueChangedReasonPluginEdited) ||
           ( reason == eValueChangedReasonNatronGuiEdited) ||
           ( reason == eValueChangedReasonNatronInternalEdited) ) && //< the change was made by the user or plugin
         ( newKey != NULL) ) { //< the keyframe to set is not null
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
Knob<T>::setValues(const T& value0,
                   const T& value1,
                   ViewSpec view,
                   ValueChangedReasonEnum reason,
                   int dimensionOffset)
{
    assert( dimensionOffset + 2 <= getDimension() );

    KnobHolder* holder = getHolder();
    EffectInstance* effect = 0;
    bool doEditEnd = false;

    if (holder) {
        effect = dynamic_cast<EffectInstance*>(holder);
        if (effect) {
            if ( effect->isDoingInteractAction() && !effect->getApp()->isCreatingPythonGroup() ) {
                effect->setMultipleParamsEditLevel(KnobHolder::eMultipleParamsEditOnCreateNewCommand);
                doEditEnd = true;
            }
        }
    }
    KeyFrame newKey;
    KnobHelper::ValueChangedReturnCodeEnum ret;
    bool hasChanged = false;
    assert(getDimension() == 2);
    beginChanges();
    blockValueChanges();
    ret = setValue(value0, view, 0 + dimensionOffset, reason, &newKey, hasChanged);
    hasChanged |= (ret != eValueChangedReturnCodeNothingChanged);
    unblockValueChanges();
    ret = setValue(value1, view, 1 + dimensionOffset, reason, &newKey, hasChanged);
    hasChanged |= (ret != eValueChangedReturnCodeNothingChanged);
    Q_UNUSED(hasChanged);
    endChanges();
    if (doEditEnd) {
        effect->setMultipleParamsEditLevel(KnobHolder::eMultipleParamsEditOff);
    }
}

template <typename T>
void
Knob<T>::setValues(const T& value0,
                   const T& value1,
                   ViewSpec view,
                   ValueChangedReasonEnum reason)
{
    setValues(value0, value1, view, reason, 0);
}

template <typename T>
void
Knob<T>::setValues(const T& value0,
                   const T& value1,
                   const T& value2,
                   ViewSpec view,
                   ValueChangedReasonEnum reason)
{
    setValues(value0, value1, value2, view, reason, 0);
}

template <typename T>
void
Knob<T>::setValues(const T& value0,
                   const T& value1,
                   const T& value2,
                   ViewSpec view,
                   ValueChangedReasonEnum reason,
                   int dimensionOffset)
{
    assert( dimensionOffset + 3 <= getDimension() );

    KnobHolder* holder = getHolder();
    EffectInstance* effect = 0;
    bool doEditEnd = false;

    if (holder) {
        effect = dynamic_cast<EffectInstance*>(holder);
        if (effect) {
            if ( effect->isDoingInteractAction() && !effect->getApp()->isCreatingPythonGroup() ) {
                effect->setMultipleParamsEditLevel(KnobHolder::eMultipleParamsEditOnCreateNewCommand);
                doEditEnd = true;
            }
        }
    }

    KeyFrame newKey;
    KnobHelper::ValueChangedReturnCodeEnum ret;
    bool hasChanged = false;
    assert(getDimension() == 3);
    beginChanges();
    blockValueChanges();
    ret = setValue(value0, view,  0 + dimensionOffset, reason, &newKey, hasChanged);
    hasChanged |= (ret != eValueChangedReturnCodeNothingChanged);
    ret = setValue(value1, view, 1 + dimensionOffset, reason, &newKey, hasChanged);
    hasChanged |= (ret != eValueChangedReturnCodeNothingChanged);
    unblockValueChanges();
    ret = setValue(value2, view, 2 + dimensionOffset, reason, &newKey, hasChanged);
    hasChanged |= (ret != eValueChangedReturnCodeNothingChanged);
    Q_UNUSED(hasChanged);
    endChanges();
    if (doEditEnd) {
        effect->setMultipleParamsEditLevel(KnobHolder::eMultipleParamsEditOff);
    }
}

template <typename T>
void
Knob<T>::setValues(const T& value0,
                   const T& value1,
                   const T& value2,
                   const T& value3,
                   ViewSpec view,
                   ValueChangedReasonEnum reason)
{
    KnobHolder* holder = getHolder();
    EffectInstance* effect = 0;
    bool doEditEnd = false;

    if (holder) {
        effect = dynamic_cast<EffectInstance*>(holder);
        if (effect) {
            if ( effect->isDoingInteractAction() && !effect->getApp()->isCreatingPythonGroup() ) {
                effect->setMultipleParamsEditLevel(KnobHolder::eMultipleParamsEditOnCreateNewCommand);
                doEditEnd = true;
            }
        }
    }

    KeyFrame newKey;
    KnobHelper::ValueChangedReturnCodeEnum ret;
    bool hasChanged = false;
    assert(getDimension() == 4);
    beginChanges();
    blockValueChanges();
    ret = setValue(value0, view, 0, reason, &newKey, hasChanged);
    hasChanged |= (ret != eValueChangedReturnCodeNothingChanged);
    ret = setValue(value1, view, 1, reason, &newKey, hasChanged);
    hasChanged |= (ret != eValueChangedReturnCodeNothingChanged);
    ret = setValue(value2, view, 2, reason, &newKey, hasChanged);
    hasChanged |= (ret != eValueChangedReturnCodeNothingChanged);
    unblockValueChanges();
    setValue(value3, view, 3, reason, &newKey, hasChanged);
    hasChanged |= (ret != eValueChangedReturnCodeNothingChanged);
    Q_UNUSED(hasChanged);
    endChanges();
    if (doEditEnd) {
        effect->setMultipleParamsEditLevel(KnobHolder::eMultipleParamsEditOff);
    }
}

template <typename T>
void
Knob<T>::makeKeyFrame(Curve* curve,
                      double time,
                      ViewSpec /*view*/,
                      const T& v,
                      KeyFrame* key)
{
    double keyFrameValue;

    if ( curve->areKeyFramesValuesClampedToIntegers() ) {
        keyFrameValue = std::floor(v + 0.5);
    } else if ( curve->areKeyFramesValuesClampedToBooleans() ) {
        keyFrameValue = (bool)v;
    } else {
        keyFrameValue = (double)v;
    }
    if ( (keyFrameValue != keyFrameValue) || (boost::math::isinf)(keyFrameValue) ) { // check for NaN or infinity
        *key = KeyFrame( (double)time, getMaximum(0) );
    } else {
        *key = KeyFrame( (double)time, keyFrameValue );
    }
}

template <>
void
KnobStringBase::makeKeyFrame(Curve* /*curve*/,
                                double time,
                                ViewSpec view,
                                const std::string& v,
                                KeyFrame* key)
{
    double keyFrameValue = 0.;
    AnimatingKnobStringHelper* isStringAnimatedKnob = dynamic_cast<AnimatingKnobStringHelper*>(this);

    assert(isStringAnimatedKnob);
    if (isStringAnimatedKnob) {
        isStringAnimatedKnob->stringToKeyFrameValue(time, view, v, &keyFrameValue);
    }

    *key = KeyFrame( (double)time, keyFrameValue );
}

template<typename T>
KnobHelper::ValueChangedReturnCodeEnum
Knob<T>::setValueAtTime(double time,
                        const T & v,
                        ViewSpec view,
                        int dimension,
                        ValueChangedReasonEnum reason,
                        KeyFrame* newKey,
                        bool hasChanged)
{
    if ( (dimension < 0) || ( dimension >= (int)_values.size() ) ) {
        return eValueChangedReturnCodeNothingChanged;
    }
    assert(newKey);

    if ( !canAnimate() || !isAnimationEnabled() ) {
        qDebug() << "WARNING: Attempting to call setValueAtTime on " << getName().c_str() << " which does not have animation enabled.";
        setValue(v, view, dimension, reason, newKey);
    }

    KnobHelper::ValueChangedReturnCodeEnum ret = eValueChangedReturnCodeNoKeyframeAdded;
    KnobHolder* holder =  getHolder();

#ifdef DEBUG
    if ( holder && (reason == eValueChangedReasonPluginEdited) ) {
        EffectInstance* isEffect = dynamic_cast<EffectInstance*>(holder);
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
                if (_signalSlotHandler) {
                    _signalSlotHandler->s_appendParamEditChange(reason, vari, view, dimension, time, true, true);
                }
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
                if (_signalSlotHandler) {
                    _signalSlotHandler->s_appendParamEditChange(reason, vari, view, dimension, time, false, true);
                }
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
    makeKeyFrame(curve.get(), time, view, v, newKey);


    if ( holder && !holder->isSetValueCurrentlyPossible() ) {
        ///If we cannot set value, queue it
        if ( holder && getEvaluateOnChange() ) {
            //We explicitly abort rendering now and do not wait for it to be done in EffectInstance::evaluate()
            //because the actual value change (which will call evaluate()) may arise well later
            holder->abortAnyEvaluation();
        }

        QueuedSetValueAtTimePtr qv = boost::make_shared<QueuedSetValueAtTime>( time, view, dimension, v, *newKey, reason, isValueChangesBlocked() );
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

    KeyFrame existingKey;
    bool hasExistingKey = curve->getKeyFrameWithTime(time, &existingKey);
    if (!hasExistingKey) {
        hasChanged = true;
        ret = eValueChangedReturnCodeKeyframeAdded;
    } else {
        bool modifiedKeyFrame = ( ( existingKey.getValue() != newKey->getValue() ) ||
                                  ( existingKey.getLeftDerivative() != newKey->getLeftDerivative() ) ||
                                  ( existingKey.getRightDerivative() != newKey->getRightDerivative() ) );
        if (modifiedKeyFrame) {
            ret = eValueChangedReturnCodeKeyframeModified;
        }
        hasChanged |= modifiedKeyFrame;
    }
    bool newKeyFrame = curve->addKeyFrame(*newKey);
    if (newKeyFrame) {
        ret = eValueChangedReturnCodeKeyframeAdded;
    }


    if (holder) {
        holder->setHasAnimation(true);
    }
    guiCurveCloneInternalCurve(eCurveChangeReasonInternal, view, dimension, reason);

    if (_signalSlotHandler && newKeyFrame) {
        _signalSlotHandler->s_keyFrameSet(time, view, dimension, (int)reason, ret);
    }
    if (hasChanged) {
        evaluateValueChange(dimension, time, view, reason);
    } else {
        return eValueChangedReturnCodeNothingChanged;
    }

    return ret;
} // setValueAtTime

template<typename T>
void
Knob<T>::setValuesAtTime(double time,
                         const T& value0,
                         const T& value1,
                         ViewSpec view,
                         ValueChangedReasonEnum reason,
                         int dimensionOffset)
{
    assert( dimensionOffset + 2 <= getDimension() );
    KnobHolder* holder = getHolder();
    EffectInstance* effect = 0;
    bool doEditEnd = false;

    if (holder) {
        effect = dynamic_cast<EffectInstance*>(holder);
        if (effect) {
            if ( effect->isDoingInteractAction() && !effect->getApp()->isCreatingPythonGroup() ) {
                effect->setMultipleParamsEditLevel(KnobHolder::eMultipleParamsEditOnCreateNewCommand);
                doEditEnd = true;
            }
        }
    }
    KeyFrame newKey;
    KnobHelper::ValueChangedReturnCodeEnum ret;
    bool hasChanged = false;
    assert(getDimension() == 2);
    beginChanges();
    blockValueChanges();
    ret = setValueAtTime(time, value0, view, 0 + dimensionOffset, reason, &newKey, hasChanged);
    hasChanged |= (ret != eValueChangedReturnCodeNothingChanged);
    unblockValueChanges();
    ret = setValueAtTime(time, value1, view, 1 + dimensionOffset, reason, &newKey, hasChanged);
    hasChanged |= (ret != eValueChangedReturnCodeNothingChanged);
    Q_UNUSED(hasChanged);
    endChanges();
    if (doEditEnd) {
        effect->setMultipleParamsEditLevel(KnobHolder::eMultipleParamsEditOff);
    }
}

template<typename T>
void
Knob<T>::setValuesAtTime(double time,
                         const T& value0,
                         const T& value1,
                         ViewSpec view,
                         ValueChangedReasonEnum reason)
{
    setValuesAtTime(time, value0, value1, view, reason, 0);
}

template<typename T>
void
Knob<T>::setValuesAtTime(double time,
                         const T& value0,
                         const T& value1,
                         const T& value2,
                         ViewSpec view,
                         ValueChangedReasonEnum reason,
                         int dimensionOffset)
{
    assert( dimensionOffset + 3 <= getDimension() );
    KnobHolder* holder = getHolder();
    EffectInstance* effect = 0;
    bool doEditEnd = false;

    if (holder) {
        effect = dynamic_cast<EffectInstance*>(holder);
        if (effect) {
            if ( effect->isDoingInteractAction() && !effect->getApp()->isCreatingPythonGroup() ) {
                effect->setMultipleParamsEditLevel(KnobHolder::eMultipleParamsEditOnCreateNewCommand);
                doEditEnd = true;
            }
        }
    }
    KeyFrame newKey;
    KnobHelper::ValueChangedReturnCodeEnum ret;
    bool hasChanged = false;
    assert(getDimension() == 3);
    beginChanges();
    blockValueChanges();
    ret = setValueAtTime(time, value0, view, 0 + dimensionOffset, reason, &newKey, hasChanged);
    hasChanged |= (ret != eValueChangedReturnCodeNothingChanged);
    ret = setValueAtTime(time, value1, view, 1 + dimensionOffset, reason, &newKey, hasChanged);
    hasChanged |= (ret != eValueChangedReturnCodeNothingChanged);
    unblockValueChanges();
    ret = setValueAtTime(time, value2, view, 2 + dimensionOffset, reason, &newKey, hasChanged);
    hasChanged |= (ret != eValueChangedReturnCodeNothingChanged);
    Q_UNUSED(hasChanged);
    endChanges();
    if (doEditEnd) {
        effect->setMultipleParamsEditLevel(KnobHolder::eMultipleParamsEditOff);
    }
}

template<typename T>
void
Knob<T>::setValuesAtTime(double time,
                         const T& value0,
                         const T& value1,
                         const T& value2,
                         ViewSpec view,
                         ValueChangedReasonEnum reason)
{
    setValuesAtTime(time, value0, value1, value2, view, reason, 0);
}

template<typename T>
void
Knob<T>::setValuesAtTime(double time,
                         const T& value0,
                         const T& value1,
                         const T& value2,
                         const T& value3,
                         ViewSpec view,
                         ValueChangedReasonEnum reason)
{
    KnobHolder* holder = getHolder();
    EffectInstance* effect = 0;
    bool doEditEnd = false;

    if (holder) {
        effect = dynamic_cast<EffectInstance*>(holder);
        if (effect) {
            if ( effect->isDoingInteractAction() && !effect->getApp()->isCreatingPythonGroup() ) {
                effect->setMultipleParamsEditLevel(KnobHolder::eMultipleParamsEditOnCreateNewCommand);
                doEditEnd = true;
            }
        }
    }
    KeyFrame newKey;
    KnobHelper::ValueChangedReturnCodeEnum ret;
    bool hasChanged = false;
    assert(getDimension() == 4);
    beginChanges();
    blockValueChanges();
    ret = setValueAtTime(time, value0, view, 0, reason, &newKey, hasChanged);
    hasChanged |= (ret != eValueChangedReturnCodeNothingChanged);
    ret = setValueAtTime(time, value1, view, 1, reason, &newKey, hasChanged);
    hasChanged |= (ret != eValueChangedReturnCodeNothingChanged);
    ret = setValueAtTime(time, value2, view, 2, reason, &newKey, hasChanged);
    hasChanged |= (ret != eValueChangedReturnCodeNothingChanged);
    unblockValueChanges();
    ret = setValueAtTime(time, value3, view, 3, reason, &newKey, hasChanged);
    hasChanged |= (ret != eValueChangedReturnCodeNothingChanged);
    Q_UNUSED(hasChanged);
    endChanges();
    if (doEditEnd) {
        effect->setMultipleParamsEditLevel(KnobHolder::eMultipleParamsEditOff);
    }
}

template<typename T>
void
Knob<T>::unSlaveInternal(int dimension,
                         ValueChangedReasonEnum reason,
                         bool copyState)
{
    if ( !isSlave(dimension) ) {
        return;
    }
    std::pair<int, KnobIPtr> master = getMaster(dimension);
    KnobHelperPtr masterHelper = boost::dynamic_pointer_cast<KnobHelper>(master.second);

    if (masterHelper->getSignalSlotHandler() && _signalSlotHandler) {
        QObject::disconnect( masterHelper->getSignalSlotHandler().get(), SIGNAL( keyFrameSet(double, ViewSpec, int, int, bool) ),
                             _signalSlotHandler.get(), SLOT( onMasterKeyFrameSet(double, ViewSpec, int, int, bool) ) );
        QObject::disconnect( masterHelper->getSignalSlotHandler().get(), SIGNAL( keyFrameRemoved(double, ViewSpec, int, int) ),
                             _signalSlotHandler.get(), SLOT( onMasterKeyFrameRemoved(double, ViewSpec, int, int) ) );
        QObject::disconnect( masterHelper->getSignalSlotHandler().get(), SIGNAL( keyFrameMoved(ViewSpec, int, double, double) ),
                             _signalSlotHandler.get(), SLOT( onMasterKeyFrameMoved(ViewSpec, int, double, double) ) );
        QObject::disconnect( masterHelper->getSignalSlotHandler().get(), SIGNAL( animationRemoved(ViewSpec, int) ),
                             _signalSlotHandler.get(), SLOT( onMasterAnimationRemoved(ViewSpec, int) ) );
    }

    resetMaster(dimension);
    bool hasChanged = false;
    setEnabled(dimension, true);
    if (copyState) {
        ///clone the master
        hasChanged |= cloneAndCheckIfChanged( master.second.get(), dimension, master.first );
    }

    if (_signalSlotHandler) {
        if (reason == eValueChangedReasonPluginEdited && _signalSlotHandler) {
            _signalSlotHandler->s_knobSlaved(dimension, false);
        }
    }
    if (getHolder() && _signalSlotHandler) {
        getHolder()->onKnobSlaved( shared_from_this(), master.second, dimension, false );
    }
    if (masterHelper) {
        masterHelper->removeListener(this, dimension);
    }
    if (hasChanged) {
        evaluateValueChange(dimension, getCurrentTime(), ViewIdx(0), reason);
    } else {
        checkAnimationLevel(ViewSpec::all(), dimension);
    }
}

template<typename T>
KnobHelper::ValueChangedReturnCodeEnum
Knob<T>::setValue(const T & value,
                  ViewSpec view,
                  int dimension,
                  bool turnOffAutoKeying)
{
    if (turnOffAutoKeying) {
        return setValue(value, view, dimension, eValueChangedReasonNatronInternalEdited, NULL);
    } else {
        KeyFrame k;

        return setValue(value, view, dimension, eValueChangedReasonNatronInternalEdited, &k);
    }
}

template<typename T>
KnobHelper::ValueChangedReturnCodeEnum
Knob<T>::onValueChanged(const T & value,
                        ViewSpec view,
                        int dimension,
                        ValueChangedReasonEnum reason,
                        KeyFrame* newKey)
{
    assert(reason == eValueChangedReasonNatronGuiEdited || reason == eValueChangedReasonUserEdited);

    return setValue(value, view, dimension, reason, newKey);
}

template<typename T>
KnobHelper::ValueChangedReturnCodeEnum
Knob<T>::setValueFromPlugin(const T & value,
                            ViewSpec view,
                            int dimension)
{
    KeyFrame newKey;

    return setValue(value, view, dimension, eValueChangedReasonPluginEdited, &newKey);
}

template<typename T>
void
Knob<T>::setValueAtTime(double time,
                        const T & v,
                        ViewSpec view,
                        int dimension)
{
    KeyFrame k;

    ignore_result( setValueAtTime(time, v, view, dimension, eValueChangedReasonNatronInternalEdited, &k) );
}

template<typename T>
void
Knob<T>::setValueAtTimeFromPlugin(double time,
                                  const T & v,
                                  ViewSpec view,
                                  int dimension)
{
    KeyFrame k;

    ignore_result( setValueAtTime(time, v, view, dimension, eValueChangedReasonPluginEdited, &k) );
}

template<typename T>
T
Knob<T>::getKeyFrameValueByIndex(ViewSpec view,
                                 int dimension,
                                 int index,
                                 bool* ok) const
{
    ///if the knob is slaved to another knob, returns the other knob value
    std::pair<int, KnobIPtr> master = getMaster(dimension);

    if (master.second) {
        KnobIntBase* isInt = dynamic_cast<KnobIntBase* >( master.second.get() );
        KnobBoolBase* isBool = dynamic_cast<KnobBoolBase* >( master.second.get() );
        KnobDoubleBase* isDouble = dynamic_cast<KnobDoubleBase* >( master.second.get() );
        assert( master.second->isTypePOD() && (isInt || isBool || isDouble) ); //< other data types aren't supported
        if (isInt) {
            return isInt->getKeyFrameValueByIndex(view, master.first, index, ok);
        } else if (isBool) {
            return isBool->getKeyFrameValueByIndex(view, master.first, index, ok);
        } else if (isDouble) {
            return isDouble->getKeyFrameValueByIndex(view, master.first, index, ok);
        }
    }

    assert( dimension < getDimension() );
    if ( !getKeyFramesCount(view, dimension) ) {
        *ok = false;

        return T();
    }

    CurvePtr curve = getCurve(view, dimension);
    assert(curve);
    KeyFrame kf;
    *ok =  curve->getKeyFrameWithIndex(index, &kf);

    return kf.getValue();
}

template<>
std::string
KnobStringBase::getKeyFrameValueByIndex(ViewSpec view,
                                           int dimension,
                                           int index,
                                           bool* ok) const
{
    ///if the knob is slaved to another knob, returns the other knob value
    std::pair<int, KnobIPtr> master = getMaster(dimension);

    if (master.second) {
        KnobStringBase* isString = dynamic_cast<KnobStringBase* >( master.second.get() );
        assert(isString); //< other data types aren't supported
        if (isString) {
            return isString->getKeyFrameValueByIndex(view, master.first, index, ok);
        }
    }

    assert( dimension < getDimension() );
    if ( !getKeyFramesCount(view, dimension) ) {
        *ok = false;

        return "";
    }

    std::string value;
    const AnimatingKnobStringHelper* animatedString = dynamic_cast<const AnimatingKnobStringHelper*>(this);
    assert(animatedString);
    if (animatedString) {
        CurvePtr curve = getCurve(view, dimension);
        assert(curve);
        KeyFrame kf;
        *ok =  curve->getKeyFrameWithIndex(index, &kf);

        if (*ok) {
            animatedString->stringFromInterpolatedValue(kf.getValue(), view, &value);
        }
    }

    return value;
}

template<typename T>
std::list<T>
Knob<T>::getValueForEachDimension_mt_safe() const
{
    QMutexLocker l(&_valueMutex);
    std::list<T> ret;

    for (U32 i = 0; i < _values.size(); ++i) {
        ret.push_back(_values[i]);
    }

    return ret;
}

template<typename T>
std::vector<T>
Knob<T>::getValueForEachDimension_mt_safe_vector() const
{
    QMutexLocker l(&_valueMutex);

    return _values;
}

template<typename T>
T
Knob<T>::getRawValue(int dimension) const
{
    assert( dimension >= 0 && dimension < getDimension() );
    QMutexLocker l(&_valueMutex);

    return _values[dimension];
}

template<typename T>
std::vector<T>
Knob<T>::getDefaultValues_mt_safe() const
{
    QMutexLocker l(&_valueMutex);
    int dims = getDimension();
    std::vector<T> ret(dims);
    for (int i = 0; i < dims; ++i) {
        ret[i] = _defaultValues[i].value;
    }
    return ret;
}

template<typename T>
T
Knob<T>::getDefaultValue(int dimension) const
{
    QMutexLocker l(&_valueMutex);

    return _defaultValues[dimension].value;
}

template<typename T>
bool
Knob<T>::hasDefaultValueChanged(int dimension) const
{
    QMutexLocker l(&_valueMutex);
    return _defaultValues[dimension].initialValue != _defaultValues[dimension].value;
}

template<typename T>
bool
Knob<T>::isDefaultValueSet(int dimension) const
{
    QMutexLocker l(&_valueMutex);
    return _defaultValues[dimension].defaultValueSet;
}

template<typename T>
void
Knob<T>::setDefaultValue(const T & v,
                         int dimension)
{
    assert( dimension < getDimension() );
    {
        QMutexLocker l(&_valueMutex);
        _defaultValues[dimension].value = v;
        if (!_defaultValues[dimension].defaultValueSet) {
            _defaultValues[dimension].defaultValueSet = true;
            _defaultValues[dimension].initialValue = v;
        }
    }
    resetToDefaultValueWithoutSecretNessAndEnabledNess(dimension);
}

template <typename T>
void
Knob<T>::setDefaultValueWithoutApplying(const T& v,
                                        int dimension)
{
    assert( dimension < getDimension() );
    {
        QMutexLocker l(&_valueMutex);
        _defaultValues[dimension].value = v;
        if (!_defaultValues[dimension].defaultValueSet) {
            _defaultValues[dimension].defaultValueSet = true;
            _defaultValues[dimension].initialValue = v;
        }

    }
    computeHasModifications();
}

template <typename T>
void
Knob<T>::setDefaultValuesWithoutApplying(const T& v1, const T& v2)
{
    assert(getDimension() == 2);
    {
        QMutexLocker l(&_valueMutex);
        _defaultValues[0].value = v1;
        if (!_defaultValues[0].defaultValueSet) {
            _defaultValues[0].defaultValueSet = true;
            _defaultValues[0].initialValue = v1;
        }

        _defaultValues[1].value = v2;
        if (!_defaultValues[1].defaultValueSet) {
            _defaultValues[1].defaultValueSet = true;
            _defaultValues[1].initialValue = v2;
        }

    }
    computeHasModifications();
}

template <typename T>
void
Knob<T>::setDefaultValuesWithoutApplying(const T& v1, const T& v2, const T& v3)
{
    assert(getDimension() == 3);
    {
        QMutexLocker l(&_valueMutex);
        _defaultValues[0].value = v1;
        if (!_defaultValues[0].defaultValueSet) {
            _defaultValues[0].defaultValueSet = true;
            _defaultValues[0].initialValue = v1;
        }

        _defaultValues[1].value = v2;
        if (!_defaultValues[1].defaultValueSet) {
            _defaultValues[1].defaultValueSet = true;
            _defaultValues[1].initialValue = v2;
        }

        _defaultValues[2].value = v3;
        if (!_defaultValues[2].defaultValueSet) {
            _defaultValues[2].defaultValueSet = true;
            _defaultValues[2].initialValue = v3;
        }

    }
    computeHasModifications();
}

template <typename T>
void
Knob<T>::setDefaultValuesWithoutApplying(const T& v1, const T& v2, const T& v3, const T& v4)
{
    assert(getDimension() == 4);
    {
        QMutexLocker l(&_valueMutex);
        _defaultValues[0].value = v1;
        if (!_defaultValues[0].defaultValueSet) {
            _defaultValues[0].defaultValueSet = true;
            _defaultValues[0].initialValue = v1;
        }

        _defaultValues[1].value = v2;
        if (!_defaultValues[1].defaultValueSet) {
            _defaultValues[1].defaultValueSet = true;
            _defaultValues[1].initialValue = v2;
        }

        _defaultValues[2].value = v3;
        if (!_defaultValues[2].defaultValueSet) {
            _defaultValues[2].defaultValueSet = true;
            _defaultValues[2].initialValue = v3;
        }

        _defaultValues[3].value = v4;
        if (!_defaultValues[3].defaultValueSet) {
            _defaultValues[3].defaultValueSet = true;
            _defaultValues[3].initialValue = v4;
        }

    }
    computeHasModifications();
}


template<typename T>
void
Knob<T>::populate()
{
    for (int i = 0; i < getDimension(); ++i) {
        _values[i] = T();
        _guiValues[i] = T();
        _defaultValues[i].value = T();
        _defaultValues[i].defaultValueSet = false;
    }
    KnobHelper::populate();
}

template<typename T>
bool
Knob<T>::isTypePOD() const
{
    return false;
}

template<>
bool
KnobIntBase::isTypePOD() const
{
    return true;
}

template<>
bool
KnobBoolBase::isTypePOD() const
{
    return true;
}

template<>
bool
KnobDoubleBase::isTypePOD() const
{
    return true;
}

template<typename T>
bool
Knob<T>::isTypeCompatible(const KnobIPtr & other) const
{
    return isTypePOD() == other->isTypePOD();
}

template<typename T>
bool
Knob<T>::onKeyFrameSet(double time,
                       ViewSpec view,
                       int dimension)
{
    KeyFrame key;
    CurvePtr curve;
    KnobHolder* holder = getHolder();
    bool useGuiCurve = ( !holder || !holder->isSetValueCurrentlyPossible() ) && getKnobGuiPointer();

    if (!useGuiCurve) {
        assert(holder);
        curve = getCurve(view, dimension);
    } else {
        curve = getGuiCurve(view, dimension);
        setGuiCurveHasChanged(view, dimension, true);
    }

    makeKeyFrame(curve.get(), time, view, getValueAtTime(time, dimension, view), &key);

    return setKeyFrame(key, view, dimension, eValueChangedReasonUserEdited);
}

template<typename T>
bool
Knob<T>::setKeyFrame(const KeyFrame& key,
                     ViewSpec view,
                     int dimension,
                     ValueChangedReasonEnum reason)
{
    CurvePtr curve;
    KnobHolder* holder = getHolder();
    bool useGuiCurve = ( !holder || !holder->isSetValueCurrentlyPossible() ) && getKnobGuiPointer();

    if (!useGuiCurve) {
        assert(holder);
        curve = getCurve(view, dimension);
    } else {
        curve = getGuiCurve(view, dimension);
        setGuiCurveHasChanged(view, dimension, true);
    }

    bool ret = curve->addKeyFrame(key);

    if (!useGuiCurve) {
        guiCurveCloneInternalCurve(eCurveChangeReasonInternal, view, dimension, reason);
        evaluateValueChange(dimension, key.getTime(), view, reason);
    }

    return ret;
}

template<typename T>
bool
Knob<T>::onKeyFrameSet(double /*time*/,
                       ViewSpec view,
                       const KeyFrame& key,
                       int dimension)
{
    return setKeyFrame(key, view, dimension, eValueChangedReasonUserEdited);
}

template<typename T>
void
Knob<T>::onTimeChanged(bool isPlayback,
                       double time)
{
    int dims = getDimension();

    if ( getIsSecret() ) {
        return;
    }
    bool shouldRefresh = false;
    for (int i = 0; i < dims; ++i) {
        if ( _signalSlotHandler && ( isAnimated( i, ViewIdx(0) ) || !getExpression(i).empty() ) ) {
            shouldRefresh = true;
        }
    }

    if (shouldRefresh) {
        checkAnimationLevel(ViewIdx(0), -1);
        if (_signalSlotHandler) {
            _signalSlotHandler->s_valueChanged(ViewSpec::all(), -1, eValueChangedReasonTimeChanged);
        }
    }
    if (evaluateValueChangeOnTimeChange() && !isPlayback) {
        KnobHolder* holder = getHolder();
        if (holder) {
            //Some knobs like KnobFile do not animate but the plug-in may need to know the time has changed
            if ( holder->isEvaluationBlocked() ) {
                holder->appendValueChange(shared_from_this(), -1, false, time, ViewIdx(0), eValueChangedReasonTimeChanged, eValueChangedReasonTimeChanged);
            } else {
                holder->beginChanges();
                holder->appendValueChange(shared_from_this(), -1, false, time, ViewIdx(0), eValueChangedReasonTimeChanged, eValueChangedReasonTimeChanged);
                holder->endChanges();
            }
        }
    }
}

template<typename T>
double
Knob<T>::getDerivativeAtTime(double time,
                             ViewSpec view,
                             int dimension)
{
    if ( ( dimension > getDimension() ) || (dimension < 0) ) {
        throw std::invalid_argument("Knob::getDerivativeAtTime(): Dimension out of range");
    }
    {
        std::string expr = getExpression(dimension);
        if ( !expr.empty() ) {
            // Compute derivative by finite differences, using values at t-0.5 and t+0.5
            return ( getValueAtTime(time + 0.5, dimension, view) - getValueAtTime(time - 0.5, dimension, view) ) / 2.;
        }
    }

    ///if the knob is slaved to another knob, returns the other knob value
    std::pair<int, KnobIPtr> master = getMaster(dimension);
    if (master.second) {
        return master.second->getDerivativeAtTime(time, view, master.first);
    }

    CurvePtr curve  = getCurve(view, dimension);
    if (curve->getKeyFramesCount() > 0) {
        return curve->getDerivativeAt(time);
    } else {
        /*if the knob as no keys at this dimension, the derivative is 0.*/
        return 0.;
    }
}

template<>
double
KnobStringBase::getDerivativeAtTime(double /*time*/,
                                       ViewSpec /*view*/,
                                       int /*dimension*/)
{
    throw std::invalid_argument("Knob<string>::getDerivativeAtTime() not available");
}

// Compute integral using Simpsons rule:
// \int_a^b f(x) dx = (b-a)/6 * (f(a) + 4f((a+b)/2) + f(b))
template<typename T>
double
Knob<T>::getIntegrateFromTimeToTimeSimpson(double time1,
                                           double time2,
                                           ViewSpec view,
                                           int dimension)
{
    double fa = getValueAtTime(time1, dimension, view);
    double fm = getValueAtTime( (time1 + time2) / 2, dimension, view );
    double fb = getValueAtTime(time2, dimension, view);

    return (time2 - time1) / 6 * (fa + 4 * fm + fb);
}

template<typename T>
double
Knob<T>::getIntegrateFromTimeToTime(double time1,
                                    double time2,
                                    ViewSpec view,
                                    int dimension)
{
    if ( ( dimension > getDimension() ) || (dimension < 0) ) {
        throw std::invalid_argument("Knob::getIntegrateFromTimeToTime(): Dimension out of range");
    }
    {
        std::string expr = getExpression(dimension);
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
                val += getIntegrateFromTimeToTimeSimpson(time1, i, view, dimension);
            }
            // integer chunks
            for (int t = i; t < j; ++t) {
                val += getIntegrateFromTimeToTimeSimpson(t, t + 1, view, dimension);
            }
            // last chunk
            if (j < time2) {
                val += getIntegrateFromTimeToTimeSimpson(j, time2, view, dimension);
            }

            return val;
        }
    }


    ///if the knob is slaved to another knob, returns the other knob value
    std::pair<int, KnobIPtr> master = getMaster(dimension);
    if (master.second) {
        assert( master.second->isTypePOD() ); //< other data types aren't supported
        if ( master.second->isTypePOD() ) {
            return master.second->getIntegrateFromTimeToTime(time1, time2, view, master.first);
        }
    }

    CurvePtr curve  = getCurve(view, dimension);
    if (curve->getKeyFramesCount() > 0) {
        return curve->getIntegrateFromTo(time1, time2);
    } else {
        // if the knob as no keys at this dimension, the integral is trivial
        QMutexLocker l(&_valueMutex);

        return (double)_values[dimension] * (time2 - time1);
    }
} // >::getIntegrateFromTimeToTime

template<>
double
KnobStringBase::getIntegrateFromTimeToTimeSimpson(double /*time1*/,
                                                     double /*time2*/,
                                                     ViewSpec /*view*/,
                                                     int /*dimension*/)
{
    return 0; // dummy value
}

template<>
double
KnobStringBase::getIntegrateFromTimeToTime(double /*time1*/,
                                              double /*time2*/,
                                              ViewSpec /*view*/,
                                              int /*dimension*/)
{
    throw std::invalid_argument("Knob<string>::getIntegrateFromTimeToTime() not available");
}


template<>
void
KnobDoubleBase::resetToDefaultValueWithoutSecretNessAndEnabledNess(int dimension)
{
    KnobI::removeAnimation(ViewSpec::all(), dimension);
    double defaultV;
    {
        QMutexLocker l(&_valueMutex);
        defaultV = _defaultValues[dimension].value;
    }

    ///A KnobDoubleBase is not always a KnobDouble (it can also be a KnobColor)
    KnobDouble* isDouble = dynamic_cast<KnobDouble*>(this);


    clearExpression(dimension, true);
    resetExtraToDefaultValue(dimension);
    // see http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#kOfxParamPropDefaultCoordinateSystem
    if (isDouble) {
        if ( isDouble->getDefaultValuesAreNormalized() ) {
            if (isDouble->getValueIsNormalized(dimension) == eValueIsNormalizedNone) {
                // default is normalized, value is non-normalized: denormalize it!
                double time = getCurrentTime();
                defaultV = isDouble->denormalize(dimension, time, defaultV);
            }
        } else {
            if (isDouble->getValueIsNormalized(dimension) != eValueIsNormalizedNone) {
                // default is non-normalized, value is normalized: normalize it!
                double time = getCurrentTime();
                defaultV = isDouble->normalize(dimension, time, defaultV);
            }
        }
    }
    ignore_result( setValue(defaultV, ViewSpec::all(), dimension, eValueChangedReasonRestoreDefault, NULL) );
    if (_signalSlotHandler) {
        _signalSlotHandler->s_valueChanged(ViewSpec::all(), dimension, eValueChangedReasonRestoreDefault);
    }
}

template<typename T>
void
Knob<T>::resetToDefaultValueWithoutSecretNessAndEnabledNess(int dimension)
{
    KnobI::removeAnimation(ViewSpec::all(), dimension);
    T defaultV;
    {
        QMutexLocker l(&_valueMutex);
        defaultV = _defaultValues[dimension].value;
    }

    clearExpression(dimension, true);
    resetExtraToDefaultValue(dimension);
    ignore_result( setValue(defaultV, ViewSpec::all(), dimension, eValueChangedReasonRestoreDefault, NULL) );
    if (_signalSlotHandler) {
        _signalSlotHandler->s_valueChanged(ViewSpec::all(), dimension, eValueChangedReasonRestoreDefault);
    }
}

template<typename T>
void
Knob<T>::resetToDefaultValue(int dimension)
{
    resetToDefaultValueWithoutSecretNessAndEnabledNess(dimension);
    setSecret( getDefaultIsSecret() );
    setEnabled( dimension, isDefaultEnabled(dimension) );
}

// If *all* the following conditions hold:
// - this is a double value
// - this is a non normalised spatial double parameter, i.e. kOfxParamPropDoubleType is set to one of
//   - kOfxParamDoubleTypeX
//   - kOfxParamDoubleTypeXAbsolute
//   - kOfxParamDoubleTypeY
//   - kOfxParamDoubleTypeYAbsolute
//   - kOfxParamDoubleTypeXY
//   - kOfxParamDoubleTypeXYAbsolute
// - kOfxParamPropDefaultCoordinateSystem is set to kOfxParamCoordinatesNormalised
// Knob<T>::resetToDefaultValue should denormalize
// the default value, using the input size.
// Input size be defined as the first available of:
// - the RoD of the "Source" clip
// - the RoD of the first non-mask non-optional input clip (in case there is no "Source" clip) (note that if these clips are not connected, you get the current project window, which is the default value for GetRegionOfDefinition)

// see http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#kOfxParamPropDefaultCoordinateSystem
// and http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#APIChanges_1_2_SpatialParameters
template<>
void
KnobDoubleBase::resetToDefaultValue(int dimension)
{
    KnobI::removeAnimation(ViewSpec::all(), dimension);

    ///A KnobDoubleBase is not always a KnobDouble (it can also be a KnobColor)
    KnobDouble* isDouble = dynamic_cast<KnobDouble*>(this);
    double def;

    clearExpression(dimension, true);

    {
        QMutexLocker l(&_valueMutex);
        def = _defaultValues[dimension].value;
    }

    resetExtraToDefaultValue(dimension);

    // see http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#kOfxParamPropDefaultCoordinateSystem
    if (isDouble) {
        if ( isDouble->getDefaultValuesAreNormalized() ) {
            if (isDouble->getValueIsNormalized(dimension) == eValueIsNormalizedNone) {
                // default is normalized, value is non-normalized: denormalize it!
                double time = getCurrentTime();
                def = isDouble->denormalize(dimension, time, def);
            }
        } else {
            if (isDouble->getValueIsNormalized(dimension) != eValueIsNormalizedNone) {
                // default is non-normalized, value is normalized: normalize it!
                double time = getCurrentTime();
                def = isDouble->normalize(dimension, time, def);
            }
        }
    }
    ignore_result( setValue(def, ViewSpec::all(), dimension, eValueChangedReasonRestoreDefault, NULL) );
    if (_signalSlotHandler) {
        _signalSlotHandler->s_valueChanged(ViewSpec::all(), dimension, eValueChangedReasonRestoreDefault);
    }
    setSecret( getDefaultIsSecret() );
    setEnabled( dimension, isDefaultEnabled(dimension) );
}

template<typename T>
template<typename OTHERTYPE>
void
Knob<T>::copyValueForType(Knob<OTHERTYPE>* other,
                          int dimension,
                          int otherDimension)
{
    assert( (dimension == otherDimension) || (dimension != -1) );
    QMutexLocker k(&_valueMutex);
    if (dimension == -1) {
        int dimMin = std::min( getDimension(), other->getDimension() );
        std::vector<OTHERTYPE> v = other->getValueForEachDimension_mt_safe_vector();
        for (int i = 0; i < dimMin; ++i) {
            _values[i] = v[i];
            _guiValues[i] = v[i];
        }
    } else {
        if (otherDimension == -1) {
            otherDimension = dimension;
        }
        assert( dimension >= 0 && dimension < getDimension() &&
                otherDimension >= 0 && otherDimension < other->getDimension() );
        _values[dimension] = _guiValues[dimension] = T( other->getRawValue(otherDimension) );
    }
}

template<typename T>
template<typename OTHERTYPE>
bool
Knob<T>::copyValueForTypeAndCheckIfChanged(Knob<OTHERTYPE>* other,
                                           int dimension,
                                           int otherDimension)
{
    assert( (dimension == otherDimension) || (dimension != -1) );
    bool ret = false;
    QMutexLocker k(&_valueMutex);
    if (dimension == -1) {
        int dimMin = std::min( getDimension(), other->getDimension() );
        std::vector<OTHERTYPE> v = other->getValueForEachDimension_mt_safe_vector();
        for (int i = 0; i < dimMin; ++i) {
            if (_values[i] != v[i]) {
                _values[i] = v[i];
                _guiValues[i] = v[i];
                ret = true;
            }
        }
    } else {
        if (otherDimension == -1) {
            otherDimension = dimension;
        }
        assert( dimension >= 0 && dimension < getDimension() &&
                otherDimension >= 0 && otherDimension < other->getDimension() );

        T otherValue = (T)other->getRawValue(otherDimension);
        if (otherValue != _values[dimension]) {
            _values[dimension] = _guiValues[dimension] = otherValue;
            ret = true;
        }
    }

    return ret;
}

template<>
void
KnobStringBase::cloneValues(KnobI* other,
                               int dimension,
                               int otherDimension)
{
    KnobStringBase* isString = dynamic_cast<KnobStringBase* >(other);
    ///Can only clone strings
    assert(isString);
    if (isString) {
        copyValueForType<std::string>(isString, dimension, otherDimension);
    }
}

template<typename T>
void
Knob<T>::cloneValues(KnobI* other,
                     int dimension,
                     int otherDimension)
{
    KnobIntBase* isInt = dynamic_cast<KnobIntBase* >(other);
    KnobBoolBase* isBool = dynamic_cast<KnobBoolBase* >(other);
    KnobDoubleBase* isDouble = dynamic_cast<KnobDoubleBase* >(other);
    assert(isInt || isBool || isDouble);

    QMutexLocker k(&_valueMutex);
    if (isInt) {
        copyValueForType<int>(isInt, dimension, otherDimension);
    } else if (isBool) {
        copyValueForType<bool>(isBool, dimension, otherDimension);
    } else if (isDouble) {
        copyValueForType<double>(isDouble, dimension, otherDimension);
    }
}

template<>
bool
KnobStringBase::cloneValuesAndCheckIfChanged(KnobI* other,
                                                int dimension,
                                                int otherDimension)
{
    KnobStringBase* isString = dynamic_cast<KnobStringBase* >(other);
    assert( (dimension == otherDimension) || (dimension != -1) );
    ///Can only clone strings
    assert(isString);
    if (isString) {
        return copyValueForTypeAndCheckIfChanged<std::string>(isString, dimension, otherDimension);
    }

    return false;
}

template<typename T>
bool
Knob<T>::cloneValuesAndCheckIfChanged(KnobI* other,
                                      int dimension,
                                      int otherDimension)
{
    KnobIntBase* isInt = dynamic_cast<KnobIntBase* >(other);
    KnobBoolBase* isBool = dynamic_cast<KnobBoolBase* >(other);
    KnobDoubleBase* isDouble = dynamic_cast<KnobDoubleBase* >(other);
    assert(isInt || isBool || isDouble);

    QMutexLocker k(&_valueMutex);
    if (isInt) {
        return copyValueForTypeAndCheckIfChanged<int>(isInt, dimension, otherDimension);
    } else if (isBool) {
        return copyValueForTypeAndCheckIfChanged<bool>(isBool, dimension, otherDimension);
    } else if (isDouble) {
        return copyValueForTypeAndCheckIfChanged<double>(isDouble, dimension, otherDimension);
    }

    return false;
}

template <typename T>
void
Knob<T>::cloneExpressionsResults(KnobI* other,
                                 int dimension,
                                 int otherDimension)
{
    assert( (dimension == otherDimension) || (dimension != -1) );
    Knob<T>* otherKnob = dynamic_cast<Knob<T>* >(other);

    //Only clone expr results of the same type
    if (!otherKnob) {
        return;
    }
    if (dimension == -1) {
        int dimMin = std::min( getDimension(), other->getDimension() );
        for (int i = 0; i < dimMin; ++i) {
            FrameValueMap results;
            otherKnob->getExpressionResults(i, results);
            QMutexLocker k(&_valueMutex);
            _exprRes[i] = results;
        }
    } else {
        if (otherDimension == -1) {
            otherDimension = dimension;
        }
        FrameValueMap results;
        otherKnob->getExpressionResults(otherDimension, results);
        QMutexLocker k(&_valueMutex);
        _exprRes[dimension] = results;
    }
}

template<typename T>
void
Knob<T>::clone(KnobI* other,
               int dimension,
               int otherDimension)
{
    if (other == this) {
        return;
    }
    cloneValues(other, dimension, otherDimension);
    cloneExpressions(other, dimension, otherDimension);
    cloneCurves(other, 0, 0, dimension, otherDimension);

    if ( !isValueChangesBlocked() && _signalSlotHandler ) {
        _signalSlotHandler->s_valueChanged(ViewSpec::all(), dimension, eValueChangedReasonNatronInternalEdited);
    }
    if ( !isListenersNotificationBlocked() ) {
        refreshListenersAfterValueChange(ViewSpec::all(), eValueChangedReasonNatronInternalEdited, dimension);
    }


    cloneExtraData(other, dimension, otherDimension);
    if ( getHolder() ) {
        getHolder()->updateHasAnimation();
    }
    computeHasModifications();
}

template <typename T>
bool
Knob<T>::cloneAndCheckIfChanged(KnobI* other,
                                int dimension,
                                int otherDimension)
{
    if (other == this) {
        return false;
    }

    bool hasChanged = cloneValuesAndCheckIfChanged(other, dimension, otherDimension);
    hasChanged |= cloneExpressionsAndCheckIfChanged(other, dimension, otherDimension);
    hasChanged |= cloneCurvesAndCheckIfChanged(other, false, dimension, otherDimension);

    hasChanged |= cloneExtraDataAndCheckIfChanged(other, dimension, otherDimension);
    if (hasChanged) {
        if ( getHolder() ) {
            getHolder()->updateHasAnimation();
        }
        computeHasModifications();
    }
    if (hasChanged) {
        if (_signalSlotHandler) {
            _signalSlotHandler->s_valueChanged(ViewSpec::all(), dimension, eValueChangedReasonNatronInternalEdited);
        }
        if ( !isListenersNotificationBlocked() ) {
            refreshListenersAfterValueChange(ViewSpec::all(), eValueChangedReasonNatronInternalEdited, dimension);
        }
    }

    return hasChanged;
}

template<typename T>
void
Knob<T>::clone(KnobI* other,
               double offset,
               const RangeD* range,
               int dimension,
               int otherDimension)
{
    if (other == this) {
        return;
    }
    cloneValues(other, dimension, otherDimension);
    cloneExpressions(other, dimension, otherDimension);
    cloneCurves(other, offset, range, dimension, otherDimension);

    cloneExtraData(other, offset, range, dimension, otherDimension);
    if ( getHolder() ) {
        getHolder()->updateHasAnimation();
    }
    if (_signalSlotHandler) {
        _signalSlotHandler->s_valueChanged(ViewSpec::all(), dimension, eValueChangedReasonNatronInternalEdited);
    }
    if ( !isListenersNotificationBlocked() ) {
        refreshListenersAfterValueChange(ViewSpec::all(), eValueChangedReasonNatronInternalEdited, dimension);
    }
}

template<typename T>
void
Knob<T>::cloneAndUpdateGui(KnobI* other,
                           int dimension,
                           int otherDimension)
{
    if (other == this) {
        return;
    }

    bool hasChanged = cloneValuesAndCheckIfChanged(other, dimension, otherDimension);
    hasChanged |= cloneExpressionsAndCheckIfChanged(other, dimension, otherDimension);
    hasChanged |= cloneCurvesAndCheckIfChanged(other, true, dimension, otherDimension);

    hasChanged |= cloneExtraDataAndCheckIfChanged(other, dimension, otherDimension);

    if (hasChanged) {
        if ( getHolder() ) {
            getHolder()->updateHasAnimation();
        }
        computeHasModifications();
    }
    if (hasChanged) {
        if (_signalSlotHandler) {
            _signalSlotHandler->s_valueChanged(ViewSpec::all(), dimension, eValueChangedReasonNatronInternalEdited);
        }
        if ( !isListenersNotificationBlocked() ) {
            refreshListenersAfterValueChange(ViewSpec::all(), eValueChangedReasonNatronInternalEdited, dimension);
        }
    }
} // >::cloneAndUpdateGui

template <typename T>
void
Knob<T>::cloneDefaultValues(KnobI* other)
{
    int dims = std::min( getDimension(), other->getDimension() );

    Knob<T>* otherT = dynamic_cast<Knob<T>*>(other);
    assert(otherT);
    if (!otherT) {
        // coverity[dead_error_line]
        return;
    }

    std::vector<DefaultValue> otherDef;
    {
        QMutexLocker l(&otherT->_valueMutex);
        otherDef = otherT->_defaultValues;
    }
    for (int i = 0; i < dims; ++i) {
        if (otherDef[i].defaultValueSet) {
            setDefaultValueWithoutApplying(otherDef[i].value, i);
        }
    }
}

template <typename T>
bool
Knob<T>::dequeueValuesSet(bool disableEvaluation)
{
    std::map<int, ValueChangedReasonEnum> dimensionChanged;
    bool ret = false;

    cloneGuiCurvesIfNeeded(dimensionChanged);
    {
        QMutexLocker kql(&_setValuesQueueMutex);
        QMutexLocker k(&_valueMutex);
        for (typename std::list<QueuedSetValuePtr>::const_iterator it = _setValuesQueue.begin(); it != _setValuesQueue.end(); ++it) {
            QueuedSetValueAtTime* isAtTime = dynamic_cast<QueuedSetValueAtTime*>( it->get() );
            bool blockValueChanges = (*it)->valueChangesBlocked();

            if (!isAtTime) {
                if ( (*it)->useKey() ) {
                    CurvePtr curve = getCurve( (*it)->view(), (*it)->dimension() );
                    if (curve) {
                        curve->addKeyFrame( (*it)->key() );
                    }

                    if ( getHolder() ) {
                        getHolder()->setHasAnimation(true);
                    }
                } else {
                    if ( _values[(*it)->dimension()] != (*it)->value() ) {
                        _values[(*it)->dimension()] = (*it)->value();
                        _guiValues[(*it)->dimension()] = (*it)->value();
                        if (!blockValueChanges) {
                            dimensionChanged.insert( std::make_pair( (*it)->dimension(), (*it)->reason() ) );
                        }
                    }
                }
            } else {
                CurvePtr curve = getCurve( (*it)->view(), (*it)->dimension() );
                if (curve) {
                    KeyFrame existingKey;
                    const KeyFrame& key = (*it)->key();
                    bool hasKey = curve->getKeyFrameWithTime( key.getTime(), &existingKey );
                    if ( !hasKey || ( existingKey.getTime() != key.getTime() ) || ( existingKey.getValue() != key.getValue() ) ||
                         ( existingKey.getLeftDerivative() != key.getLeftDerivative() ) ||
                         ( existingKey.getRightDerivative() != key.getRightDerivative() ) ) {
                        if (!blockValueChanges) {
                            dimensionChanged.insert( std::make_pair( (*it)->dimension(), (*it)->reason() ) );
                        }
                        curve->addKeyFrame( key );
                    }
                }

                if ( getHolder() ) {
                    getHolder()->setHasAnimation(true);
                }
            }
        }
        _setValuesQueue.clear();
    }
    cloneInternalCurvesIfNeeded(dimensionChanged);

    clearExpressionsResultsIfNeeded(dimensionChanged);

    ret |= !dimensionChanged.empty();

    if ( !disableEvaluation && !dimensionChanged.empty() ) {
        beginChanges();
        double time = getCurrentTime();
        for (std::map<int, ValueChangedReasonEnum>::iterator it = dimensionChanged.begin(); it != dimensionChanged.end(); ++it) {
            evaluateValueChange(it->first, time, ViewIdx(0), it->second);
        }
        endChanges();
    }

    return ret;
} // >::dequeueValuesSet

template <typename T>
bool
Knob<T>::computeValuesHaveModifications(int /*dimension*/,
                                        const T& value,
                                        const T& defaultValue) const
{
    return value != defaultValue;
}

template <typename T>
void
Knob<T>::computeHasModifications()
{
    bool oneChanged = false;

    for (int i = 0; i < getDimension(); ++i) {
        bool hasModif = false;
        std::string expr = getExpression(i);
        if ( !expr.empty() ) {
            hasModif = true;
        }

        if (!hasModif) {
            CurvePtr c = getCurve(ViewIdx(0), i);
            if ( c && c->isAnimated() ) {
                hasModif = true;
            }
        }

        if (!hasModif) {
            std::pair<int, KnobIPtr> master = getMaster(i);
            if (master.second) {
                hasModif = true;
            }
        }

        ///Check expressions too in the future
        if (!hasModif) {
            QMutexLocker k(&_valueMutex);
            if ( computeValuesHaveModifications(i, _values[i], _defaultValues[i].value) ) {
                hasModif = true;
            }
        }


        if (!hasModif) {
            hasModif |= hasModificationsVirtual(i);
        }

        oneChanged |= setHasModifications(i, hasModif, true);
    }
    if (oneChanged && _signalSlotHandler) {
        _signalSlotHandler->s_hasModificationsChanged();
    }
}

template <typename T>
void
Knob<T>::copyValuesFromCurve(int dim)
{
    double time = getCurrentTime();
    assert(dim >= 0 && dim < getDimension());
    T v = getValueAtTime(time, dim);
    QMutexLocker l(&_valueMutex);
    _guiValues[dim] = _values[dim] = v;

}

NATRON_NAMESPACE_EXIT

#endif // KNOBIMPL_H
