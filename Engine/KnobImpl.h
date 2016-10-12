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
#include "Engine/StringAnimationManager.h"


#define EXPR_RECURSION_LEVEL() KnobHelper::ExprRecursionLevel_RAII __recursionLevelIncrementer__(this)

NATRON_NAMESPACE_ENTER;

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
    for (int i = 0; i < getNDimensions(); ++i) {
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
    for (int i = 0; i < getNDimensions(); ++i) {
        _minimums[i] = INT_MIN;
        _maximums[i] = INT_MAX;
        _displayMins[i] = INT_MIN;
        _displayMaxs[i] = INT_MAX;
    }
}

template <typename T>
Knob<T>::Knob(const KnobHolderPtr& holder,
              const std::string & description,
              int dimension,
              bool declaredByPlugin )
    : KnobHelper(holder, description, dimension, declaredByPlugin)
    , _valueMutex(QMutex::Recursive)
    , _values(dimension)
    , _guiValues(dimension)
    , _defaultValues(dimension)
    , _exprRes(dimension)
    , _minMaxMutex(QReadWriteLock::Recursive)
    , _minimums(dimension)
    , _maximums(dimension)
    , _displayMins(dimension)
    , _displayMaxs(dimension)
    , _setValuesQueueMutex()
    , _setValuesQueue()
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
                             DimIdx dimension)
{
    if (_signalSlotHandler) {
        _signalSlotHandler->s_minMaxChanged(mini, maxi, dimension);
    }
}

template <typename T>
void
Knob<T>::signalDisplayMinMaxChanged(const T& mini,
                                    const T& maxi,
                                    DimIdx dimension)
{
    if (_signalSlotHandler) {
        _signalSlotHandler->s_displayMinMaxChanged(mini, maxi, dimension);
    }
}

template <>
void
KnobStringBase::signalMinMaxChanged(const std::string& /*mini*/,
                                    const std::string& /*maxi*/,
                                    DimIdx /*dimension*/)
{
}

template <>
void
KnobStringBase::signalDisplayMinMaxChanged(const std::string& /*mini*/,
                                           const std::string& /*maxi*/,
                                           DimIdx /*dimension*/)
{
}

template <typename T>
void
Knob<T>::setMinimum(const T& mini,
                    DimIdx dimension)
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
                    DimIdx dimension)
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
                           DimIdx dimension)
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
                           DimIdx dimension)
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
Knob<T>::getMinimum(DimIdx dimension) const
{
    QReadLocker k(&_minMaxMutex);

    return _minimums[dimension];
}

template <typename T>
T
Knob<T>::getMaximum(DimIdx dimension) const
{
    QReadLocker k(&_minMaxMutex);

    return _maximums[dimension];
}

template <typename T>
T
Knob<T>::getDisplayMinimum(DimIdx dimension) const
{
    QReadLocker k(&_minMaxMutex);

    return _displayMins[dimension];
}

template <typename T>
T
Knob<T>::getDisplayMaximum(DimIdx dimension) const
{
    QReadLocker k(&_minMaxMutex);

    return _displayMaxs[dimension];
}

template <typename T>
T
Knob<T>::clampToMinMax(const T& value,
                       DimIdx dimension) const
{
    QReadLocker k(&_minMaxMutex);

    return std::max( (double)_minimums[dimension], std::min( (double)_maximums[dimension], (double)value ) );
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
KnobHelper::pyObjectToType(PyObject* o) const
{
    return (int)PyInt_AsLong(o);
}

template <>
bool
KnobHelper::pyObjectToType(PyObject* o) const
{
    if (PyObject_IsTrue(o) == 1) {
        return true;
    } else {
        return false;
    }
}

template <>
double
KnobHelper::pyObjectToType(PyObject* o) const
{
    return (double)PyFloat_AsDouble(o);
}

template <>
std::string
KnobHelper::pyObjectToType(PyObject* o) const
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
    } else if ( PyString_Check(o) ) {
        return std::string( PyString_AsString(o) );
    }

    int index = 0;
    if ( PyFloat_Check(o) ) {
        index = std::floor( (double)PyFloat_AsDouble(o) + 0.5 );
    } else if ( PyLong_Check(o) ) {
        index = (int)PyInt_AsLong(o);
    } else if (PyObject_IsTrue(o) == 1) {
        index = 1;
    }

    const AnimatingKnobStringHelper* isStringAnimated = dynamic_cast<const AnimatingKnobStringHelper* >(this);
    if (!isStringAnimated) {
        return std::string();
    }
    std::string ret;
    isStringAnimated->stringFromInterpolatedValue(index, ViewSpec::current(), &ret);

    return ret;
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
void
Knob<T>::makeKeyFrame(double time,
                      const T& v,
                      ViewIdx /*view*/,
                      KeyFrame* key)
{
    AnimatingObjectI::KeyframeDataTypeEnum type = getKeyFrameDataType();
    double keyFrameValue;
    switch (type) {
        case AnimatingObjectI::eKeyframeDataTypeBool:
            keyFrameValue = (bool)v;
            break;
        case AnimatingObjectI::eKeyframeDataTypeInt:
            keyFrameValue = std::floor(v + 0.5);
            break;
        default:
            keyFrameValue = (double)v;
            break;
    }

    // check for NaN or infinity
    if ( (keyFrameValue != keyFrameValue) || boost::math::isinf(keyFrameValue) ) {
        *key = KeyFrame( (double)time, getMaximum(0) );
    } else {
        *key = KeyFrame( (double)time, keyFrameValue );
    }
}

template <>
void
KnobStringBase::makeKeyFrame(double time,
                             const std::string& v,
                             ViewIdx view,
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
void
Knob<T>::unSlaveInternal(int dimension,
                         ValueChangedReasonEnum reason,
                         bool copyState)
{
    if ( !isSlave(dimension) ) {
        return;
    }
    std::pair<int, KnobIPtr > master = getMaster(dimension);
    KnobHelperPtr masterHelper = boost::dynamic_pointer_cast<KnobHelper>(master.second);

    QObject::disconnect( masterHelper->_signalSlotHandler.get(), SIGNAL(curveAnimationChanged(std::list<double>, std::list<double>,ViewIdx,int,CurveChangeReason)),
                         _signalSlotHandler.get(), SLOT(onMasterCurveAnimationChanged(std::list<double>, std::list<double>,ViewIdx,int,CurveChangeReason)));



    resetMaster(dimension);
    bool hasChanged = false;
    setEnabled(dimension, true);
    if (copyState) {
        ///clone the master
        hasChanged |= clone( master.second, dimension, master.first );
    }

    if (reason == eValueChangedReasonPluginEdited) {
        _signalSlotHandler->s_knobSlaved(dimension, false);
    }

    if (getHolder() && _signalSlotHandler) {
        getHolder()->onKnobSlaved( shared_from_this(), master.second, dimension, false );
    }
    if (masterHelper) {
        masterHelper->removeListener(shared_from_this(), dimension);
    }
    if (!hasChanged) {
        // At least refresh animation level if clone did not change anything
        refreshAnimationLevel(ViewSpec::all(), dimension);
    }
}

template<typename T>
T
Knob<T>::getDefaultValue(int dimension) const
{
    QMutexLocker l(&_valueMutex);

    return _defaultValues[dimension].value;
}

template<typename T>
T
Knob<T>::getInitialDefaultValue(int dimension) const
{
    QMutexLocker l(&_valueMutex);

    return _defaultValues[dimension].initialValue;

}

template<typename T>
void
Knob<T>::setCurrentDefaultValueAsInitialValue()
{
    int nDims = getNDimensions();
    QMutexLocker l(&_valueMutex);
    for (int i = 0; i < nDims; ++i) {
        _defaultValues[i].initialValue = _defaultValues[i].value;
        _defaultValues[i].defaultValueSet = true;
    }
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
    assert( dimension < getNDimensions() );
    {
        QMutexLocker l(&_valueMutex);
        _defaultValues[dimension].value = v;
        if (!_defaultValues[dimension].defaultValueSet) {
            _defaultValues[dimension].defaultValueSet = true;
            _defaultValues[dimension].initialValue = v;
        }
    }
    resetToDefaultValue(dimension, ViewSpec::all());
    computeHasModifications();
}

template<typename T>
void
Knob<T>::setDefaultValues(const std::vector<T>& values, int dimensionStartOffset)
{
    if (values.empty()) {
        return;
    }
    int nDims = getNDimensions();
    assert(nDims >= (int)values.size());
    if (dimensionStartOffset < 0 || dimensionStartOffset + (int)values.size() > nDims) {
        throw std::invalid_argument("Knob<T>::setDefaultValuesWithoutApplying: Invalid arguments");
    }
    QMutexLocker l(&_valueMutex);
    for (std::size_t i = 0; i < values.size(); ++i) {
        int dimension = i + dimensionStartOffset;
        _defaultValues[dimension].value = values[i];
        if (!_defaultValues[dimension].defaultValueSet) {
            _defaultValues[dimension].defaultValueSet = true;
            _defaultValues[dimension].initialValue = values[i];
        }

    }
    computeHasModifications();


}

template <typename T>
void
Knob<T>::setDefaultValueWithoutApplying(const T& v,
                                        int dimension)
{
    assert( dimension < getNDimensions() );
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
Knob<T>::setDefaultValuesWithoutApplying(const std::vector<T>& values, int dimensionStartOffset)
{
    if (values.empty()) {
        return;
    }
    int nDims = getNDimensions();
    assert(nDims >= (int)values.size());
    if (dimensionStartOffset < 0 || dimensionStartOffset + (int)values.size() > nDims) {
        throw std::invalid_argument("Knob<T>::setDefaultValuesWithoutApplying: Invalid arguments");
    }

    QMutexLocker l(&_valueMutex);
    for (std::size_t i = 0; i < values.size(); ++i) {
        int dimension = i + dimensionStartOffset;
        _defaultValues[dimension].value = values[i];
        if (!_defaultValues[dimension].defaultValueSet) {
            _defaultValues[dimension].defaultValueSet = true;
            _defaultValues[dimension].initialValue = values[i];
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


template<>
void
Knob<bool>::populate()
{
    for (int i = 0; i < getNDimensions(); ++i) {
        _values[i][ViewIdx(0)] = false;
        _guiValues[i][ViewIdx(0)] = false;
        _defaultValues[i].value = false;
        _defaultValues[i].defaultValueSet = false;
    }
    KnobHelper::populate();
}


template<typename T>
void
Knob<T>::populate()
{
    for (int i = 0; i < getNDimensions(); ++i) {
        initDefaultValue<T>(&_values[i][ViewIdx(0)]);
        initDefaultValue<T>(&_guiValues[i][ViewIdx(0)]);
        initDefaultValue<T>(&_defaultValues[i].value);
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
void
Knob<T>::onTimeChanged(bool isPlayback,
                       double time)
{
    int dims = getNDimensions();

    if ( getIsSecret() ) {
        return;
    }
    bool shouldRefresh = false;
    for (int i = 0; i < dims; ++i) {
        if (isAnimated( i, ViewIdx(0) )) {
            shouldRefresh = true;
        }
    }

    if (shouldRefresh) {
        refreshAnimationLevel(ViewIdx(0), -1);
        _signalSlotHandler->s_valueChanged(ViewSpec::all(), -1, eValueChangedReasonTimeChanged);
    }
    if (evaluateValueChangeOnTimeChange() && !isPlayback) {
        KnobHolderPtr holder = getHolder();
        if (holder) {
            //Some knobs like KnobFile do not animate but the plug-in may need to know the time has changed
            if ( holder->isEvaluationBlocked() ) {
                holder->appendValueChange(shared_from_this(), -1, time, ViewIdx(0), eValueChangedReasonTimeChanged, eValueChangedReasonTimeChanged);
            } else {
                holder->beginChanges();
                holder->appendValueChange(shared_from_this(), -1, time, ViewIdx(0), eValueChangedReasonTimeChanged, eValueChangedReasonTimeChanged);
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
    if ( ( dimension > getNDimensions() ) || (dimension < 0) ) {
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
    std::pair<int, KnobIPtr > master = getMaster(dimension);
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
    if ( ( dimension > getNDimensions() ) || (dimension < 0) ) {
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
    std::pair<int, KnobIPtr > master = getMaster(dimension);
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


template <typename T>
void
Knob<T>::resetToDefaultValue(DimSpec dimension, ViewSpec view)
{
    removeAnimation(view, dimension);
    T defaultV;
    {
        QMutexLocker l(&_valueMutex);
        defaultV = _defaultValues[dimension].value;
    }

    clearExpression(dimension, view, true);
    resetExtraToDefaultValue(dimension, view);
    ignore_result( setValue(defaultV, view, dimension, eValueChangedReasonRestoreDefault, NULL) );
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
KnobDoubleBase::resetToDefaultValue(DimSpec dimension, ViewSpec view)
{
    KnobI::removeAnimation(view, dimension, eCurveChangeReasonInternal);

    ///A KnobDoubleBase is not always a KnobDouble (it can also be a KnobColor)
    KnobDouble* isDouble = dynamic_cast<KnobDouble*>(this);

    clearExpression(dimension, view, true);


    resetExtraToDefaultValue(dimension, view);

    if (isDouble) {
        double time = getCurrentTime();

        // see http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#kOfxParamPropDefaultCoordinateSystem
        int nDims = getNDimensions();
        std::vector<double> defValues(nDims);
        for (int i = 0; i < nDims; ++i) {
            if (dimension.isAll() || i == dimension) {
                {
                    QMutexLocker l(&_valueMutex);
                    defValues[i] = _defaultValues[i].value;
                }

                if ( isDouble->getDefaultValuesAreNormalized() ) {
                    if (isDouble->getValueIsNormalized(i) == eValueIsNormalizedNone) {
                        // default is normalized, value is non-normalized: denormalize it!
                        defValues[i] = isDouble->denormalize(i, time, def);
                    }
                } else {
                    if (isDouble->getValueIsNormalized(i) != eValueIsNormalizedNone) {
                        // default is non-normalized, value is normalized: normalize it!
                        defValues[i] = isDouble->normalize(i, time, def);
                    }
                }

            }
        }
        if (dimension.isAll()) {
            ignore_result( setValueAcrossDimensions(defValues, DimIdx(0), view, eValueChangedReasonRestoreDefault) );
        } else {
            ignore_result( setValue(defValues[dimension], view, dimension, eValueChangedReasonRestoreDefault, NULL) );
        }
    }


    
}

template <typename T>
typename Knob<T>::PerDimensionValuesVec
Knob<T>::getRawValues() const
{
    QMutexLocker ok(&_valueMutex);
    return _values;
}

template <typename T>
T
Knob<T>::getRawValue(int dimension, ViewIdx view) const
{
    assert(dimension >= 0 && dimension < getNDimensions());
    QMutexLocker ok(&_valueMutex);
    typename PerViewValueMap::const_iterator foundView = _values[dimension].find(view);
    if (foundView == _values[dimension].end()) {
        return T();
    }
    return foundView->second;
}


template<typename T>
template<typename OTHERTYPE>
bool
Knob<T>::copyValueForType(const boost::shared_ptr<Knob<OTHERTYPE> >& other,
                          ViewIdx view,
                          int dimension,
                          int otherDimension)
{
    assert( (dimension == otherDimension) || (dimension != -1) );
    bool ret = false;
    QMutexLocker k(&_valueMutex);
    if (dimension == -1) {
        int dimMin = std::min( getNDimensions(), other->getNDimensions() );
        typename Knob<OTHERTYPE>::PerDimensionValuesVec v = other->getRawValues();
        for (int i = 0; i < dimMin; ++i) {
            typename Knob<OTHERTYPE>::PerViewValueMap::const_iterator foundOtherView = v[i].find(view);
            if (foundOtherView == v[i].end()) {
                // Other view does not exist
                continue;
            }
            typename PerViewValueMap::const_iterator foundView = _values[i].find(view);
            if (foundView == _values[i].end()) {
                ret = true;
                _values[i].insert(std::make_pair(view, (T)foundOtherView->second));
            } else {
                ret |= foundView->second != (T)foundOtherView->second;
                foundView->second = (T)foundOtherView->second;
            }
            _guiValues[i][view] = (T)foundOtherView->second;

        }
    } else {
        if (otherDimension == -1) {
            otherDimension = dimension;
        }
        assert( dimension >= 0 && dimension < getNDimensions() &&
                otherDimension >= 0 && otherDimension < other->getNDimensions() );

        T otherValue = (T)other->getRawValue(otherDimension, view);
        typename PerViewValueMap::const_iterator foundView = _values[dimension].find(view);
        if (foundView == _values[dimension].end()) {
            ret = true;
            _values[dimension].insert(std::make_pair(view, otherValue));
        } else {
            ret |= foundView->second != otherValue;
            foundView->second = otherValue;
        }
        _guiValues[dimension][view] = otherValue;
    }

    return ret;
}

template<>
bool
KnobStringBase::cloneValues(const KnobIPtr& other,
                            ViewIdx view,
                            int dimension,
                            int otherDimension)
{
    KnobStringBasePtr isString = toKnobStringBase(other);
    ///Can only clone strings
    assert(isString);
    if (isString) {
        return copyValueForType<std::string>(isString, view, dimension, otherDimension);
    }
}


template<typename T>
bool
Knob<T>::cloneValues(const KnobIPtr& other,
                     ViewIdx view,
                     int dimension,
                     int otherDimension)
{
    KnobIntBasePtr isInt = toKnobIntBase(other);
    KnobBoolBasePtr isBool = toKnobBoolBase(other);
    KnobDoubleBasePtr isDouble = toKnobDoubleBase(other);
    assert(isInt || isBool || isDouble);

    if (isInt) {
        return copyValueForType<int>(isInt, view, dimension, otherDimension);
    } else if (isBool) {
        return copyValueForType<bool>(isBool, view, dimension, otherDimension);
    } else if (isDouble) {
        return copyValueForType<double>(isDouble, view, dimension, otherDimension);
    }

    return false;
}

template <typename T>
void
Knob<T>::cloneExpressionsResults(const KnobIPtr& other,
                                 int dimension,
                                 int otherDimension)
{
    assert( (dimension == otherDimension) || (dimension != -1) );
    boost::shared_ptr<Knob<T> > otherKnob = boost::dynamic_pointer_cast<Knob<T> >(other);;

    //Only clone expr results of the same type
    if (!otherKnob) {
        return;
    }
    if (dimension == -1) {
        int dimMin = std::min( getNDimensions(), other->getNDimensions() );
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
bool
Knob<T>::clone(const KnobIPtr& other,
               ViewSpec view,
               DimSpec dimension,
               DimSpec otherDimension,
               const RangeD* range,
               double offset)
{
    if (!other || other.get() == this) {
        // Cannot clone itself
        return false;
    }
    beginChanges();
    bool hasChanged = false;
    hasChanged |= cloneValues(other, view, dimension, otherDimension);
    hasChanged |= cloneExpressions(other, dimension, otherDimension);
    hasChanged |= cloneCurves(other, offset, range, dimension, otherDimension);
    hasChanged |= cloneExtraData(other, offset, range, dimension, otherDimension);

    if ( getHolder() ) {
        getHolder()->updateHasAnimation();
    }
    evaluateValueChange(dimension, getCurrentTime(), ViewIdx(0), eValueChangedReasonNatronInternalEdited);
    endChanges();

    return hasChanged;
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
        for (typename std::list<QueuedSetValuePtr >::const_iterator it = _setValuesQueue.begin(); it != _setValuesQueue.end(); ++it) {
            QueuedSetValueAtTimePtr isAtTime = boost::dynamic_pointer_cast<QueuedSetValueAtTime>(*it);
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

    for (int i = 0; i < getNDimensions(); ++i) {
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
            std::pair<int, KnobIPtr > master = getMaster(i);
            if (master.second) {
                hasModif = true;
            }
        }

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
    assert(dim >= 0 && dim < getNDimensions());
    T v = getValueAtTime(time, dim);
    QMutexLocker l(&_valueMutex);
    _guiValues[dim] = _values[dim] = v;

}

template <typename T>
void handleAnimatedHashing(Knob<T>* knob, ViewIdx view, int dimension, Hash64* hash)
{
    CurvePtr curve = knob->getCurve(view, dimension);
    assert(curve);
    Hash64::appendCurve(curve, hash);

}

template <>
void handleAnimatedHashing(Knob<std::string>* knob, ViewIdx view, int dimension, Hash64* hash)
{
    AnimatingKnobStringHelper* isAnimated = dynamic_cast<AnimatingKnobStringHelper*>(knob);
    assert(isAnimated);
    if (isAnimated) {
        StringAnimationManagerPtr mng = isAnimated->getStringAnimation();
        std::map<int, std::string> keys;
        mng->save(&keys);
        for (std::map<int, std::string>::iterator it = keys.begin(); it!=keys.end(); ++it) {
            Hash64::appendQString(QString::fromUtf8(it->second.c_str()), hash);
        }
    } else {
        CurvePtr curve = knob->getCurve(view, dimension);
        assert(curve);
        Hash64::appendCurve(curve, hash);
    }

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
void
Knob<T>::appendToHash(double time, ViewIdx view, Hash64* hash)
{
    int nDims = getNDimensions();

    KnobFrameViewHashingStrategyEnum hashingStrat = getHashingStrategy();


    for (int i = 0; i < nDims; ++i) {
        if (hashingStrat == eKnobHashingStrategyAnimation && isAnimated(i)) {
            handleAnimatedHashing(this, view, i, hash);
        } else {
            T v = getValueAtTime(time, i, view);
            appendValueToHash(v, hash);
        }
    }
}

template <>
AnimatingObjectI::KeyframeDataTypeEnum
Knob<int>::getKeyFrameDataType() const
{
    return AnimatingObjectI::eKeyframeDataTypeInt;
}

template <>
AnimatingObjectI::KeyframeDataTypeEnum
Knob<bool>::getKeyFrameDataType() const
{
    return AnimatingObjectI::eKeyframeDataTypeBool;
}

template <>
AnimatingObjectI::KeyframeDataTypeEnum
Knob<double>::getKeyFrameDataType() const
{
    return AnimatingObjectI::eKeyframeDataTypeDouble;
}

template <>
AnimatingObjectI::KeyframeDataTypeEnum
Knob<std::string>::getKeyFrameDataType() const
{
    return AnimatingObjectI::eKeyframeDataTypeString;
}

template <typename T>
void
Knob<T>::clearExpressionsResults(int dimension, ViewSpec view)
{
    if (dimension < 0 || dimension >= getNDimensions()) {
        return;
    }
    QMutexLocker k(&_valueMutex);
    const PerViewFrameValueMap& retPerView = _exprRes[dimension];
    if (view.isAll()) {
        for (typename PerViewFrameValueMap::const_iterator it = _exprRes[dimension].begin(); it!=_exprRes[dimension].end(); ++it) {
            it->second.clear();
        }
    } else {
        ViewIdx view_i(0);
        if (view.isCurrent() && _exprRes[dimension].size() > 1) {
            view_i = getCurrentView();
        } else {
            view_i = ViewIdx(view.value());
        }
        typename PerViewFrameValueMap::const_iterator foundView = _exprRes[dimension].find(view_i);
        if (foundView == _exprRes[dimension].end()) {
            return;
        }
        foundView->second.clear();
    }
}

template <typename T>
void
Knob<T>::getExpressionResults(int dim, ViewSpec view, FrameValueMap& map) const
{
    if (dim < 0 || dim >= getNDimensions()) {
        return;
    }
    assert(!view.isAll());
    if (view.isAll()) {
        throw std::invalid_argument("Knob::getExpressionResults: Invalid ViewSpec: all views has no meaning in this function");
    }
    QMutexLocker k(&_valueMutex);


    ViewIdx view_i(0);
    if (view.isCurrent() && _exprRes[dim].size() > 1) {
        view_i = getCurrentView();
    } else {
        view_i = ViewIdx(view.value());
    }
    const PerViewFrameValueMap& retPerView = _exprRes[dim];
    typename PerViewFrameValueMap::const_iterator foundView = _exprRes[dim].find(view);
    if (foundView == _exprRes[dim].end()) {
        return;
    }
    map = foundView->second;
}


NATRON_NAMESPACE_EXIT;

#endif // KNOBIMPL_H
