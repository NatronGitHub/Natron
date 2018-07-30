/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://natrongithub.github.io/>,
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

#include "Engine/Curve.h"
#include "Engine/AppInstance.h"
#include "Engine/Project.h"
#include "Engine/EffectInstance.h"
#include "Engine/KnobTypes.h"
#include "Engine/Hash64.h"
#include "Engine/ViewIdx.h"

#include "Engine/EngineFwd.h"


NATRON_NAMESPACE_ENTER


template <typename T>
const std::string &
Knob<T>::typeName() const
{
    static std::string knobNoTypeName("NoType");

    return knobNoTypeName;
}


template <typename T>
bool
Knob<T>::canAnimate() const
{
    return false;
}


template <typename T>
void
Knob<T>::initMinMax()
{
}

template <>
void
KnobDoubleBase::initMinMax()
{
    for (int i = 0; i < getNDimensions(); ++i) {
        _data->minimums[i] = -DBL_MAX;
        _data->maximums[i] = DBL_MAX;
        _data->displayMins[i] = -DBL_MAX;
        _data->displayMaxs[i] = DBL_MAX;
    }
}

template <>
void
KnobIntBase::initMinMax()
{
    for (int i = 0; i < getNDimensions(); ++i) {
        _data->minimums[i] = INT_MIN;
        _data->maximums[i] = INT_MAX;
        _data->displayMins[i] = INT_MIN;
        _data->displayMaxs[i] = INT_MAX;
    }
}

template <typename T>
Knob<T>::Knob(const KnobHolderPtr& holder,
              const std::string & name,
              int dimension)
    : KnobHelper(holder, name, dimension)
    , _data(new Data(dimension))
{

    initMinMax();
}

template <typename T>
Knob<T>::Knob(const KnobHolderPtr& holder, const KnobIPtr& mainKnob)
: KnobHelper(holder, mainKnob)
, _data(boost::dynamic_pointer_cast<Knob<T> >(mainKnob)->_data)
{

}

template <typename T>
Knob<T>::~Knob()
{
}

template <>
void
Knob<std::string>::refreshCurveMinMaxInternal(ViewIdx /*view*/, DimIdx /*dimension*/)
{
    // No minmax for strings
}

template <>
void
Knob<bool>::refreshCurveMinMaxInternal(ViewIdx /*view*/, DimIdx /*dimension*/)
{
    // No minmax for booleans
}

template <typename T>
void
Knob<T>::refreshCurveMinMaxInternal(ViewIdx view, DimIdx dimension)
{
    CurvePtr curve = getAnimationCurve(view, dimension);
    if (!curve) {
        return;
    }

    QMutexLocker k(&_data->minMaxMutex);
    curve->setYRange(_data->minimums[dimension], _data->maximums[dimension]);
    curve->setDisplayYRange(_data->displayMins[dimension], _data->displayMaxs[dimension]);

}

template <typename T>
void
Knob<T>::setMinimum(const T& mini, DimSpec dimension)
{
    {
        QMutexLocker k(&_data->minMaxMutex);
        if (dimension.isAll()) {
            for (int i = 0; i < getNDimensions(); ++i) {
                _data->minimums[i] = mini;
            }
        } else {
            _data->minimums[dimension] = mini;
        }
    }
    refreshCurveMinMax(ViewSetSpec::all(), dimension);
    _signalSlotHandler->s_minMaxChanged(dimension);
}

template <typename T>
void
Knob<T>::setMaximum(const T& maxi, DimSpec dimension)
{
    {
        QMutexLocker k(&_data->minMaxMutex);
        if (dimension.isAll()) {
            for (int i = 0; i < getNDimensions(); ++i) {
                _data->maximums[i] = maxi;
            }
        } else {
            _data->maximums[dimension] = maxi;
        }
    }
    refreshCurveMinMax(ViewSetSpec::all(), dimension);
    _signalSlotHandler->s_minMaxChanged(dimension);
}

template <typename T>
void
Knob<T>::setRange(const T& mini,
                  const T& maxi,
                 DimSpec dimension)
{
    {
        QMutexLocker k(&_data->minMaxMutex);
        if (dimension.isAll()) {
            for (int i = 0; i < getNDimensions(); ++i) {
                _data->minimums[i] = mini;
                _data->maximums[i] = maxi;
            }
        } else {
            _data->minimums[dimension] = mini;
            _data->maximums[dimension] = maxi;
        }
    }
    refreshCurveMinMax(ViewSetSpec::all(), dimension);
    _signalSlotHandler->s_minMaxChanged(dimension);
}

template <typename T>
void
Knob<T>::setDisplayRange(const T& mini,
                         const T& maxi,
                        DimSpec dimension)
{
    {
        QMutexLocker k(&_data->minMaxMutex);
        if (dimension.isAll()) {
            for (int i = 0; i < getNDimensions(); ++i) {
                _data->displayMins[i] = mini;
                _data->displayMaxs[i] = maxi;
            }
        } else {
            _data->displayMins[dimension] = mini;
            _data->displayMaxs[dimension] = maxi;
        }
    }
    refreshCurveMinMax(ViewSetSpec::all(), dimension);
    _signalSlotHandler->s_displayMinMaxChanged(dimension);
}


template <typename T>
void
Knob<T>::setRangeAcrossDimensions(const std::vector<T> &minis,
                                const std::vector<T> &maxis)
{
    {
        QMutexLocker k(&_data->minMaxMutex);
        _data->minimums = minis;
        _data->maximums = maxis;
    }
    refreshCurveMinMax(ViewSetSpec::all(), DimSpec::all());
    _signalSlotHandler->s_minMaxChanged(DimSpec::all());
}

template <typename T>
void
Knob<T>::setDisplayRangeAcrossDimensions(const std::vector<T> &minis,
                                       const std::vector<T> &maxis)
{
    {
        QMutexLocker k(&_data->minMaxMutex);
        _data->displayMins = minis;
        _data->displayMaxs = maxis;
    }
    refreshCurveMinMax(ViewSetSpec::all(), DimSpec::all());
    _signalSlotHandler->s_displayMinMaxChanged(DimSpec::all());
}

template <typename T>
const std::vector<T>&
Knob<T>::getMinimums() const
{
    return _data->minimums;
}

template <typename T>
const std::vector<T>&
Knob<T>::getMaximums() const
{
    return _data->maximums;
}

template <typename T>
const std::vector<T>&
Knob<T>::getDisplayMinimums() const
{
    return _data->displayMins;
}

template <typename T>
const std::vector<T>&
Knob<T>::getDisplayMaximums() const
{
    return _data->displayMaxs;
}

template <typename T>
T
Knob<T>::getMinimum(DimIdx dimension) const
{
    QMutexLocker k(&_data->minMaxMutex);

    return _data->minimums[dimension];
}

template <typename T>
T
Knob<T>::getMaximum(DimIdx dimension) const
{
    QMutexLocker k(&_data->minMaxMutex);

    return _data->maximums[dimension];
}

template <typename T>
T
Knob<T>::getDisplayMinimum(DimIdx dimension) const
{
    QMutexLocker k(&_data->minMaxMutex);

    return _data->displayMins[dimension];
}

template <typename T>
T
Knob<T>::getDisplayMaximum(DimIdx dimension) const
{
    QMutexLocker k(&_data->minMaxMutex);

    return _data->displayMaxs[dimension];
}

template <typename T>
T
Knob<T>::clampToMinMax(const T& value,
                       DimIdx dimension) const
{
    if (dimension < 0 || dimension >= (int)_data->minimums.size()) {
        throw std::invalid_argument("Knob::clampToMinMax: dimension out of range");
    }
    QMutexLocker k(&_data->minMaxMutex);

    return std::max( (double)_data->minimums[dimension], std::min( (double)_data->maximums[dimension], (double)value ) );
}

template <>
std::string
KnobStringBase::clampToMinMax(const std::string& value,
                              DimIdx /*dimension*/) const
{
    return value;
}

template <>
bool
KnobBoolBase::clampToMinMax(const bool& value,
                            DimIdx /*dimension*/) const
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
    if ( PyUnicode_Check(o) ) {
        std::string ret;
        PyObject* utf8pyobj = PyUnicode_AsUTF8String(o); // newRef
        if (utf8pyobj) {
            char* cstr = PyBytes_AS_STRING(utf8pyobj); // Borrowed pointer
            ret.append(cstr);
            Py_DECREF(utf8pyobj);
        }

        return ret;
    }

    //assert( PyString_Check(o) );
    return std::string( PyString_AsString(o) );
}

template <>
std::string
KnobHelper::pyObjectToType(PyObject* o, DimIdx /*dim*/, ViewIdx /*view*/) const
{
    if (PyUnicode_Check(o) || PyString_Check(o)) {
        return pyObjectToType<std::string>(o);
    }
    return std::string();
#if 0
    int index = 0;
    if ( PyFloat_Check(o) ) {
        index = std::floor( (double)PyFloat_AsDouble(o) + 0.5 );
    } else if ( PyLong_Check(o) ) {
        index = (int)PyInt_AsLong(o);
    } else if (PyObject_IsTrue(o) == 1) {
        index = 1;
    }

    getAnimationCurve(view, dim);
    if (!isStringAnimated) {
        return std::string();
    }
    std::string ret;
    isStringAnimated->stringFromInterpolatedValue(index, view, &ret);

    return ret;
#endif
}


template <typename T>
bool
Knob<T>::handleExprtkReturnValue(KnobHelper::ExpressionReturnValueTypeEnum type, double valueAsDouble, const std::string& /*valueAsString*/, T* finalValue, std::string* error)
{
    if (type == KnobHelper::eExpressionReturnValueTypeString) {
        *error = "This expression cannot return a string value";
        return false;
    }
    *finalValue = (T)valueAsDouble;
    return true;

}

template <>
bool
Knob<int>::handleExprtkReturnValue(KnobHelper::ExpressionReturnValueTypeEnum type, double valueAsDouble, const std::string& valueAsString, int* finalValue, std::string* error)
{
    KnobChoice* isChoice = dynamic_cast<KnobChoice*>(this);
    if (!isChoice) {
        if (type == KnobHelper::eExpressionReturnValueTypeString) {
            *error = "This expression cannot return a scalar value";
            return false;
        }
        *finalValue = (int)valueAsDouble;
        return true;
    } else {
        if (type == KnobHelper::eExpressionReturnValueTypeScalar) {
            *finalValue = (int)valueAsDouble;
            return true;
        } else {
            // Find a matching option
            std::vector<ChoiceOption> entries = isChoice->getEntries();
            for (std::size_t i = 0; i < entries.size(); ++i) {
                if (entries[i].id == valueAsString) {
                    *finalValue = i;
                    return true;
                }
            }
            *error = "Invalid value " + valueAsString;
        }

    }
    return false;
}


template <>
bool
Knob<std::string>::handleExprtkReturnValue(KnobHelper::ExpressionReturnValueTypeEnum type, double /*valueAsDouble*/, const std::string& valueAsString, std::string* finalValue, std::string* error)
{
    if (type == KnobHelper::eExpressionReturnValueTypeScalar) {
        *error = "This expression cannot return a scalar value";
        return false;
    }
    *finalValue = valueAsString;
    return true;
}


template <typename T>
bool
Knob<T>::evaluateExpression(TimeValue time,
                            ViewIdx view,
                            DimIdx dimension,
                            T* value,
                            std::string* error)
{

    if (dimension < 0 || dimension >= getNDimensions()) {
        throw std::invalid_argument("KnobHelper::evaluateExpression(): Dimension out of range");
    }

    ExpressionLanguageEnum lang = getExpressionLanguage(view, dimension);
    switch (lang) {
        case eExpressionLanguagePython: {
            PythonGILLocker pgl;
            PyObject *ret;
            ///Reset the random state to reproduce the sequence
            bool exprOk = executePythonExpression(time, view, dimension, &ret, error);
            if (!exprOk) {
                return false;
            }
            *value =  pyObjectToType<T>(ret, dimension, view);
            Py_DECREF(ret); //< new ref
            return true;
        }
        case eExpressionLanguageExprTk: {
            double valueAsDouble = 0.;
            std::string valueAsString;
            KnobHelper::ExpressionReturnValueTypeEnum ret = executeExprTkExpression(time, view, dimension, &valueAsDouble, &valueAsString, error);
            if (ret == eExpressionReturnValueTypeError) {
                return false;
            }
            return handleExprtkReturnValue(ret, valueAsDouble, valueAsString, value, error);
        }
    }

    throw std::invalid_argument("KnobHelper::evaluateExpression(): Unknown expression type");
} // evaluateExpression

template <typename T>
bool
Knob<T>::evaluateExpression_pod(TimeValue time,
                                ViewIdx view,
                                DimIdx dimension,
                                double* value,
                                std::string* error)
{

    if (dimension < 0 || dimension >= getNDimensions()) {
        throw std::invalid_argument("KnobHelper::evaluateExpression_pod(): Dimension out of range");
    }

    ExpressionLanguageEnum lang = getExpressionLanguage(view, dimension);
    switch (lang) {
        case eExpressionLanguagePython: {
            PythonGILLocker pgl;
            PyObject *ret;
            ///Reset the random state to reproduce the sequence
            bool exprOk = executePythonExpression(time, view, dimension, &ret, error);
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
        case eExpressionLanguageExprTk: {
            double valueAsDouble = 0.;
            std::string valueAsString;
            KnobHelper::ExpressionReturnValueTypeEnum ret = executeExprTkExpression(time, view, dimension, &valueAsDouble, &valueAsString, error);
            switch (ret) {
                case eExpressionReturnValueTypeError:
                    return false;
                case eExpressionReturnValueTypeString:
                    *value = 0.;
                    return true;
                case eExpressionReturnValueTypeScalar:
                    *value = valueAsDouble;
                    return true;
            }

        }
    }

    throw std::invalid_argument("KnobHelper::evaluateExpression_pod(): Unknown expression type");
} // evaluateExpression_pod

template <typename T>
void
Knob<T>::clearExpressionsResults(DimSpec dimension, ViewSetSpec view)
{
    QMutexLocker k(&_data->expressionResultsMutex);
    std::list<ViewIdx> views = getViewsList();
    ViewIdx view_i;
    if (!view.isAll()) {
        view_i = checkIfViewExistsOrFallbackMainView(ViewIdx(view));
    }
    int nDims = getNDimensions();
    for (std::list<ViewIdx>::const_iterator it = views.begin(); it!=views.end(); ++it) {
        if (!view.isAll() && *it != view_i) {
            continue;
        }
        for (int i = 0; i < nDims; ++i) {
            if (!dimension.isAll() && i != dimension) {
                continue;
            }
            _data->expressionResults[i][*it].clear();
        }
    }

}


template<typename T>
T
Knob<T>::getDefaultValue(DimIdx dimension) const
{
    if (dimension < 0 || dimension >= (int)_data->defaultValues.size()) {
        throw std::invalid_argument("Knob::getDefaultValue: Invalid dimension");
    }
    QMutexLocker l(&_data->defaultValueMutex);

    return _data->defaultValues[dimension].value;
}

template<typename T>
T
Knob<T>::getInitialDefaultValue(DimIdx dimension) const
{
    if (dimension < 0 || dimension >= (int)_data->defaultValues.size()) {
        throw std::invalid_argument("Knob::getInitialDefaultValue: Invalid dimension");
    }
    QMutexLocker l(&_data->defaultValueMutex);

    return _data->defaultValues[dimension].initialValue;

}

template<typename T>
void
Knob<T>::setCurrentDefaultValueAsInitialValue()
{
    int nDims = getNDimensions();
    QMutexLocker l(&_data->defaultValueMutex);
    for (int i = 0; i < nDims; ++i) {
        _data->defaultValues[i].initialValue = _data->defaultValues[i].value;
        _data->defaultValues[i].defaultValueSet = true;
    }
}

template<typename T>
bool
Knob<T>::hasDefaultValueChanged(DimIdx dimension) const
{
    if (dimension < 0 || dimension >= (int)_data->defaultValues.size()) {
        throw std::invalid_argument("Knob::hasDefaultValueChanged: Invalid dimension");
    }
    QMutexLocker l(&_data->defaultValueMutex);
    return _data->defaultValues[dimension].initialValue != _data->defaultValues[dimension].value;
}

template<typename T>
bool
Knob<T>::isDefaultValueSet(DimIdx dimension) const
{
    if (dimension < 0 || dimension >= (int)_data->defaultValues.size()) {
        throw std::invalid_argument("Knob::isDefaultValueSet: Invalid dimension");
    }
    QMutexLocker l(&_data->defaultValueMutex);
    return _data->defaultValues[dimension].defaultValueSet;
}


template<typename T>
void
Knob<T>::setDefaultValue(const T & v,
                         DimSpec dimension)
{
    {
        QMutexLocker l(&_data->defaultValueMutex);
        if (dimension.isAll()) {
            int nDims = getNDimensions();
            for (int i = 0; i < nDims; ++i) {
                _data->defaultValues[i].value = v;
                if (!_data->defaultValues[i].defaultValueSet) {
                    _data->defaultValues[i].defaultValueSet = true;
                    _data->defaultValues[i].initialValue = v;
                }
            }
        } else {
            if (dimension < 0 || dimension >= (int)_data->defaultValues.size()) {
                throw std::invalid_argument("Knob::setDefaultValue: Invalid dimension");
            }
            _data->defaultValues[dimension].value = v;
            if (!_data->defaultValues[dimension].defaultValueSet) {
                _data->defaultValues[dimension].defaultValueSet = true;
                _data->defaultValues[dimension].initialValue = v;
            }
        }
    }
    onDefaultValueChanged(dimension);
    resetToDefaultValue(dimension, ViewSetSpec::all());
    computeHasModifications();
}

template<typename T>
void
Knob<T>::setDefaultValues(const std::vector<T>& values, DimIdx dimensionStartOffset)
{
    if (values.empty()) {
        return;
    }
    int nDims = getNDimensions();
    assert(nDims >= (int)values.size());
    if (dimensionStartOffset < 0 || dimensionStartOffset + (int)values.size() > nDims) {
        throw std::invalid_argument("Knob<T>::setDefaultValuesWithoutApplying: Invalid arguments");
    }
    {
        QMutexLocker l(&_data->defaultValueMutex);
        for (std::size_t i = 0; i < values.size(); ++i) {
            int dimension = i + dimensionStartOffset;
            _data->defaultValues[dimension].value = values[i];
            if (!_data->defaultValues[dimension].defaultValueSet) {
                _data->defaultValues[dimension].defaultValueSet = true;
                _data->defaultValues[dimension].initialValue = values[i];
            }

        }
    }
    onDefaultValueChanged(DimSpec::all());
    resetToDefaultValue(DimSpec::all(), ViewSetSpec::all());
    computeHasModifications();

}

template <typename T>
void
Knob<T>::setDefaultValueWithoutApplying(const T& v,
                                        DimSpec dimension)
{
    assert( dimension < getNDimensions() );
    {
        QMutexLocker l(&_data->defaultValueMutex);
        if (dimension.isAll()) {
            int nDims = getNDimensions();
            for (int i = 0; i < nDims; ++i) {
                _data->defaultValues[i].value = v;
                if (!_data->defaultValues[i].defaultValueSet) {
                    _data->defaultValues[i].defaultValueSet = true;
                    _data->defaultValues[i].initialValue = v;
                }
            }
        } else {
            if (dimension < 0 || dimension >= (int)_data->defaultValues.size()) {
                throw std::invalid_argument("Knob::setDefaultValueWithoutApplying: Invalid dimension");
            }
            _data->defaultValues[dimension].value = v;
            if (!_data->defaultValues[dimension].defaultValueSet) {
                _data->defaultValues[dimension].defaultValueSet = true;
                _data->defaultValues[dimension].initialValue = v;
            }
        }

    }
    onDefaultValueChanged(dimension);
    computeHasModifications();
}

template <typename T>
void
Knob<T>::setDefaultValuesWithoutApplying(const std::vector<T>& values, DimIdx dimensionStartOffset)
{
    if (values.empty()) {
        return;
    }
    int nDims = getNDimensions();
    assert(nDims >= (int)values.size());
    if (dimensionStartOffset < 0 || dimensionStartOffset + (int)values.size() > nDims) {
        throw std::invalid_argument("Knob<T>::setDefaultValuesWithoutApplying: Invalid arguments");
    }

    QMutexLocker l(&_data->defaultValueMutex);
    for (std::size_t i = 0; i < values.size(); ++i) {
        int dimension = i + dimensionStartOffset;
        _data->defaultValues[dimension].value = values[i];
        if (!_data->defaultValues[dimension].defaultValueSet) {
            _data->defaultValues[dimension].defaultValueSet = true;
            _data->defaultValues[dimension].initialValue = values[i];
        }

    }
    computeHasModifications();
} // setDefaultValuesWithoutApplying


template <typename T>
void initDefaultValue(T* value)
{
    *value = T();
}


template <>
void initDefaultValue(double* value)
{
    *value = 0.;
}

template <>
void initDefaultValue(int* value)
{
    *value = 0;
}

template <>
void initDefaultValue(bool* value)
{
    *value = false;
}

template <typename T>
ValueKnobDimView<T>::ValueKnobDimView()
: KnobDimViewBase()
, value()
{
    initDefaultValue(&value);
}

template <typename T>
KnobDimViewBasePtr
Knob<T>::createDimViewData() const
{
    KnobDimViewBasePtr ret(new ValueKnobDimView<T>);
    return ret;
}

template <typename FROM_TYPE, typename TO_TYPE>
bool copyValueForKnownType(const ValueKnobDimView<FROM_TYPE>& from,
                      ValueKnobDimView<TO_TYPE>* to)
{
    if (to->value != from.value) {
        to->value = from.value;
        return true;
    }
    return false;
}

template <typename TO_TYPE>
bool copyValueForTypeGuess(const KnobDimViewBase& from, ValueKnobDimView<TO_TYPE>* to);

template <>
bool copyValueForTypeGuess(const KnobDimViewBase& from, ValueKnobDimView<std::string>* to)
{
    const ValueKnobDimView<std::string>* fromIsString = dynamic_cast<const ValueKnobDimView<std::string>*>(&from);
    if (!fromIsString) {
        return false;
    }
    return copyValueForKnownType<std::string, std::string>(*fromIsString, to);
}

template <typename TO_TYPE>
bool copyValueForTypeGuess(const KnobDimViewBase& from, ValueKnobDimView<TO_TYPE>* to)
{
    const ValueKnobDimView<bool>* fromIsBool = dynamic_cast<const ValueKnobDimView<bool>*>(&from);
    const ValueKnobDimView<int>* fromIsInt = dynamic_cast<const ValueKnobDimView<int>*>(&from);
    const ValueKnobDimView<double>* fromIsDouble = dynamic_cast<const ValueKnobDimView<double>*>(&from);

    if (!fromIsBool && !fromIsInt && !fromIsDouble) {
        return false;
    }
    if (fromIsDouble) {
        return copyValueForKnownType<double, TO_TYPE>(*fromIsDouble, to);
    } else if (fromIsInt) {
        return copyValueForKnownType<int, TO_TYPE>(*fromIsInt, to);
    } else if (fromIsBool) {
        return copyValueForKnownType<bool, TO_TYPE>(*fromIsBool, to);
    }
    return false;
}

template <typename T>
bool
ValueKnobDimView<T>::copy(const CopyInArgs& inArgs, CopyOutArgs* outArgs)
{
    bool hasChanged = KnobDimViewBase::copy(inArgs, outArgs);
    if (!inArgs.other) {
        return hasChanged;
    }
    QMutexLocker k(&valueMutex);
    QMutexLocker k2(&inArgs.other->valueMutex);

    hasChanged |= copyValueForTypeGuess(*inArgs.other, this);
    return hasChanged;
}


template<typename T>
void
Knob<T>::populate()
{
    KnobHelper::populate();

    if (getHolder() && getHolder()->isRenderClone()) {
        _valuesCache.reset(new ValuesCacheMap);
    }
    int nDims = getNDimensions();
    _data->expressionResults.resize(nDims);

    if (!getMainInstance()) {
        for (int i = 0; i < nDims; ++i) {
            T defValue;
            initDefaultValue<T>(&defValue);
            _data->defaultValues[i].value = defValue;
            _data->defaultValues[i].defaultValueSet = false;
        }
        refreshCurveMinMax(ViewSetSpec::all(), DimSpec::all());
    }
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
Knob<T>::canLinkWith(const KnobIPtr & other, DimIdx thisDimension, ViewIdx thisView,  DimIdx otherDim, ViewIdx otherView, std::string* error) const
{
    Knob<T>* otherType = dynamic_cast<Knob<T>*>(other.get());
    if (otherType == 0) {
        if (error) {
            *error = KnobHelper::tr("You can only copy/paste between parameters of the same type. To overcome this, use an expression instead.").toStdString();
        }
        return false;
    }
    std::string hasExpr = getExpression(thisDimension, thisView);
    std::string otherExpr = other->getExpression(otherDim, otherView);
    if (!hasExpr.empty() || !otherExpr.empty()) {
        if (error) {
            *error = KnobHelper::tr("An expression is already set on the parameter, please remove it first.").toStdString();
        }
    }
    return true;
}


template <typename T>
void
Knob<T>::cloneDefaultValues(const KnobIPtr& other)
{
    int dims = std::min( getNDimensions(), other->getNDimensions() );

    boost::shared_ptr<Knob<T> > otherT = boost::dynamic_pointer_cast<Knob<T> >(other);
    assert(otherT);
    if (!otherT) {
        // coverity[dead_error_line]
        return;
    }

    std::vector<DefaultValue> otherDef;
    {
        QMutexLocker l(&otherT->_data->defaultValueMutex);
        otherDef = otherT->_data->defaultValues;
    }
    for (int i = 0; i < dims; ++i) {
        if (otherDef[i].defaultValueSet) {
            setDefaultValueWithoutApplying(otherDef[i].value, DimIdx(i));
        }
    }
}



template <typename T>
bool
Knob<T>::hasModificationsVirtual(const KnobDimViewBasePtr& data, DimIdx dimension) const
{
    if (data->animationCurve && data->animationCurve->isAnimated()) {
        return true;
    }
    ValueKnobDimView<T>* isDataType = dynamic_cast<ValueKnobDimView<T>*>(data.get());
    if (!isDataType) {
        return false;
    }
    bool hasModif = isDataType->value != getDefaultValue(dimension);
    return hasModif;
}

template <typename T>
void
Knob<T>::computeHasModifications()
{
    bool oneChanged = false;

    int nDims = getNDimensions();
    std::list<ViewIdx> views = getViewsList();

    for (int i = 0; i < nDims; ++i) {
        for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {

            bool hasModif = false;
            std::string expr = getExpression(DimIdx(i), *it);
            if ( !expr.empty() ) {
                hasModif = true;
            }

            KnobDimViewBasePtr data = getDataForDimView(DimIdx(i), *it);
            assert(data);
            if (!hasModif && data) {
                hasModif |= hasModificationsVirtual(data, DimIdx(i));
            }


            oneChanged |= setHasModifications(DimIdx(i), *it, hasModif, true);
        } // for all views

    } // for all dimensions
    if (oneChanged && _signalSlotHandler) {
        _signalSlotHandler->s_hasModificationsChanged();
    }
}

template <typename T>
void
Knob<T>::copyValuesFromCurve(DimIdx dim, ViewIdx view)
{
    TimeValue time = getHolder()->getTimelineCurrentTime();
    T v = getValueAtTime(time, dim, view);

    KnobDimViewBasePtr data = getDataForDimView(dim, view);
    assert(data);
    ValueKnobDimView<T>* dataType = dynamic_cast<ValueKnobDimView<T>*>(data.get());
    assert(dataType);

    QMutexLocker l(&data->valueMutex);
    dataType->value = v;

} // copyValuesFromCurve


// All keyframes of the curve for all dimensions are added to the cache.
// Typically this is needed for animated parameters that have an effect overtime which is
// integrated such as a speed param of a retimer: the speed could be 5 at frame 1 and 10 at frame 100
// but if the speed changes to 6 at frame 1, then output at frame 100 should change as well
template <typename T>
void handleAnimatedHashing(Knob<T>* knob, ViewIdx view, DimIdx dimension, Hash64* hash)
{
    CurvePtr curve = knob->getAnimationCurve(view, dimension);
    assert(curve);
    Hash64::appendCurve(curve, hash);

}


template <typename T>
void appendValueToHash(const T& v, Hash64* hash)
{
    hash->append(v);
}

template <>
void appendValueToHash(const bool& v, Hash64* hash)
{
    hash->append(v);
}


template <>
void appendValueToHash(const std::string& v, Hash64* hash)
{
    Hash64::appendQString(QString::fromUtf8(v.c_str()), hash);
}

template <typename T>
T
Knob<T>::getValueForHash(DimIdx dim, ViewIdx view)
{
    return getValue(dim, view);
}

template <typename T>
void
Knob<T>::appendToHash(const ComputeHashArgs& args, Hash64* hash)
{
    int nDims = getNDimensions();

    // If the holder wants to force full curve hashing, we do so.
    // This is useful for OFX plug-ins that do not set the cache invalidation property
    // correctly on all their parameters
    bool forceHashingFullAnim = getHolder()->isFullAnimationToHashEnabled();

    KnobFrameViewHashingStrategyEnum hashingStrat;
    if (forceHashingFullAnim) {
        hashingStrat = eKnobHashingStrategyAnimation;
    } else {
        hashingStrat = getHashingStrategy();
    }
    bool isMetadataSlave = getIsMetadataSlave();

    for (int i = 0; i < nDims; ++i) {
        switch (args.hashType) {
            case HashableObject::eComputeHashTypeTimeViewVariant: {
                if (isAnimated(DimIdx(i), args.view)) {
                    if (hashingStrat == eKnobHashingStrategyAnimation) {
                        // A parameter such as the speed param of the Retime node need to serialize the entire Curve
                        // because a value change at another time can influence the result at the current time.
                        handleAnimatedHashing(this, args.view, DimIdx(i), hash);
                    } else {
                        T v = getValueAtTime(args.time, DimIdx(i), args.view);
                        appendValueToHash(v, hash);
                    }
                } else {
                    T v = getValueForHash(DimIdx(i), args.view);
                    appendValueToHash(v, hash);
                }
            }   break;
            case HashableObject::eComputeHashTypeTimeViewInvariant: {

                // Ignore animated parameters for time view invariant
                if (isAnimated(DimIdx(i), ViewIdx(0))) {
                    continue;
                }

                T v = getValueForHash(DimIdx(i), ViewIdx(0));
                appendValueToHash(v, hash);

            }   break;
            case HashableObject::eComputeHashTypeOnlyMetadataSlaves: {

                // Ignore non metadata slave parameters
                if (!isMetadataSlave) {
                    continue;
                }
                // Ignore animated parameters for time view invariant
                if (isAnimated(DimIdx(i), ViewIdx(0))) {
                    continue;
                }

                T v = getValueForHash(DimIdx(i), ViewIdx(0));
                appendValueToHash(v, hash);

            }   break;
        }
    }
} // appendToHash

template <>
CurveTypeEnum
Knob<int>::getKeyFrameDataType() const
{
    return eCurveTypeInt;
}

template <>
CurveTypeEnum
Knob<bool>::getKeyFrameDataType() const
{
    return eCurveTypeBool;
}

template <>
CurveTypeEnum
Knob<double>::getKeyFrameDataType() const
{
    return eCurveTypeDouble;
}

template <>
CurveTypeEnum
Knob<std::string>::getKeyFrameDataType() const
{
    return eCurveTypeString;
}


inline bool compareKnobDimVieKeySetEquality(const KnobDimViewKeySet& a, const KnobDimViewKeySet& b)
{
    if (a.size() != b.size()) {
        return false;
    }
    KnobDimViewKeySet::const_iterator ita = a.begin();
    for (;ita != a.end(); ++ita) {
        KnobDimViewKeySet::const_iterator foundB = b.find(*ita);
        if (foundB == b.end()) {
            return false;
        }
    }
    return true;

}

template <typename T>
bool
Knob<T>::areDimensionsEqual(ViewIdx view)
{
    int nDims = getNDimensions();
    if (nDims == 1) {
        return true;
    }

    {
        // First check if there's an expression
        std::string dim0Expr = getExpression(DimIdx(0), view);
        for (int i = 1; i < nDims; ++i) {
            std::string dimExpr = getExpression(DimIdx(i), view);
            bool valuesEqual = dimExpr == dim0Expr;
            if (!valuesEqual) {
                return false;
            }
        }
    }

    // Check links
#if 0
    {
        KnobDimViewKeySet dim0Knobs;
        getSharedValues(DimIdx(0), view, &dim0Knobs);

        for (int i = 1; i < nDims; ++i) {
            KnobDimViewKeySet dimKnobs;
            getSharedValues(DimIdx(i), view, &dimKnobs);
            if (!compareKnobDimVieKeySetEquality(dim0Knobs, dimKnobs)) {
                return false;
            }
        }
    }
#endif 
    ValueKnobDimView<T>* dim0Data = dynamic_cast<ValueKnobDimView<T>*>(getDataForDimView(DimIdx(0), view).get());
    CurvePtr curve0 = dim0Data->animationCurve;
    T val0 = getValue(DimIdx(0), view, true /*doClamp*/);

    for (int i = 1; i < nDims; ++i) {
        ValueKnobDimView<T>* dimData = dynamic_cast<ValueKnobDimView<T>*>(getDataForDimView(DimIdx(i), view).get());
        if (dimData == dim0Data) {
            // If they point to the same data, don't bother checking values
            continue;
        }


        // Check animation curves
        CurvePtr dimCurve = dimData->animationCurve;
        if (dimCurve && curve0) {
            if (*dimCurve != *curve0) {
                return false;
            }
        }


        // Check if values are equal
        T val = getValue(DimIdx(i), view, true /*doClamp*/);
        if (val0 != val) {
            return false;
        }
        
    }
    return true;
} // areDimensionsEqual


NATRON_NAMESPACE_EXIT

#endif // KNOBIMPL_H
