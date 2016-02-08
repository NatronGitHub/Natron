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

#include <QString>
#include <QDebug>
#include <QCoreApplication>
#include <QThread>

#include "Global/Macros.h"
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
#include "Engine/ViewIdx.h"
#include "Engine/EngineFwd.h"


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
Knob<double>::initMinMax()
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
Knob<int>::initMinMax()
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
    : KnobHelper(holder,description,dimension,declaredByPlugin)
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
      , _setValueRecursionLevelMutex(QMutex::Recursive)
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
Knob<T>::signalMinMaxChanged(const T& mini,const T& maxi,int dimension)
{
    if (_signalSlotHandler) {
        _signalSlotHandler->s_minMaxChanged(mini,maxi,dimension);
    }
}

template <typename T>
void
Knob<T>::signalDisplayMinMaxChanged(const T& mini,const T& maxi,int dimension)
{
    if (_signalSlotHandler) {
        _signalSlotHandler->s_displayMinMaxChanged(mini,maxi,dimension);
    }
}

template <>
void
Knob<std::string>::signalMinMaxChanged(const std::string& /*mini*/,const std::string& /*maxi*/,int /*dimension*/)
{
    
}

template <>
void
Knob<std::string>::signalDisplayMinMaxChanged(const std::string& /*mini*/,const std::string& /*maxi*/,int /*dimension*/)
{
    
}


template <typename T>
void
Knob<T>::setMinimum(const T& mini, int dimension)
{
    T maxi;
    {
        QWriteLocker k(&_minMaxMutex);
        _minimums[dimension] = mini;
        maxi = _maximums[dimension];
    }
    signalMinMaxChanged(mini,maxi,dimension);
}

template <typename T>
void Knob<T>::setMaximum(const T& maxi, int dimension)
{
    T mini;
    {
        QWriteLocker k(&_minMaxMutex);
        _maximums[dimension] = maxi;
        mini = _minimums[dimension];
    }
    signalMinMaxChanged(mini,maxi,dimension);
}

template <typename T>
void Knob<T>::setDisplayMinimum(const T& mini, int dimension)
{
    T maxi;
    {
        QWriteLocker k(&_minMaxMutex);
        _displayMins[dimension] = mini;
        maxi = _displayMaxs[dimension];
    }
    signalDisplayMinMaxChanged(mini,maxi,dimension);
}

template <typename T>
void Knob<T>::setDisplayMaximum(const T& maxi, int dimension)
{
    T mini;
    {
        QWriteLocker k(&_minMaxMutex);
        _displayMaxs[dimension] = maxi;
        mini = _displayMins[dimension];
    }
    signalDisplayMinMaxChanged(mini,maxi,dimension);
}

template <typename T>
void Knob<T>::setMinimumsAndMaximums(const std::vector<T> &minis, const std::vector<T> &maxis)
{
    {
        QWriteLocker k(&_minMaxMutex);
        _minimums = minis;
        _maximums = maxis;
    }
    for (unsigned int i = 0; i < minis.size() ; ++i) {
        signalMinMaxChanged(minis[i],maxis[i],i);
    }
    
}

template <typename T>
void Knob<T>::setDisplayMinimumsAndMaximums(const std::vector<T> &minis, const std::vector<T> &maxis)
{
    {
        QWriteLocker k(&_minMaxMutex);
        _displayMins = minis;
        _displayMaxs = maxis;
    }
    for (unsigned int i = 0; i < minis.size() ; ++i) {
        signalDisplayMinMaxChanged(minis[i],maxis[i],i);
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
Knob<T>::clampToMinMax(const T& value,int dimension) const
{
    QReadLocker k(&_minMaxMutex);
    return std::max((double)_minimums[dimension], std::min((double)_maximums[dimension],(double)value));
}

template <>
std::string
Knob<std::string>::clampToMinMax(const std::string& value,int /*dimension*/) const
{
    return value;
}

template <>
bool
Knob<bool>::clampToMinMax(const bool& value,int /*dimension*/) const
{
    return value;
}

template <>
int
Knob<int>::pyObjectToType(PyObject* o) const
{
    return (int)PyInt_AsLong(o);
}

template <>
bool
Knob<bool>::pyObjectToType(PyObject* o) const
{
    if (PyObject_IsTrue(o) == 1) {
        return true;
    } else {
        return false;
    }
}

template <>
double
Knob<double>::pyObjectToType(PyObject* o) const
{
    return (double)PyFloat_AsDouble(o);
}

template <>
std::string
Knob<std::string>::pyObjectToType(PyObject* o) const
{
#ifndef IS_PYTHON_2
    if (PyUnicode_Check(o)) {
        return Python::PY3String_asString(o);
    }
#else
    if (PyString_Check(o)) {
        return std::string(PyString_AsString(o));
    }
#endif
    
    int index = 0;
    if (PyFloat_Check(o)) {
        index = std::floor((double)PyFloat_AsDouble(o) + 0.5);
    } else if (PyLong_Check(o)) {
        index = (int)PyInt_AsLong(o);
    } else if (PyObject_IsTrue(o) == 1) {
        index = 1;
    }
    
    const AnimatingKnobStringHelper* isStringAnimated = dynamic_cast<const AnimatingKnobStringHelper* >(this);
    if (!isStringAnimated) {
        return std::string();
    }
    std::string ret;
    isStringAnimated->stringFromInterpolatedValue(index, /*view=*/ViewIdx::current(), &ret);
    return ret;
}


inline unsigned int hashFunction(unsigned int a)
{
    a = (a ^ 61) ^ (a >> 16);
    a = a + (a << 3);
    a = a ^ (a >> 4);
    a = a * 0x27d4eb2d;
    a = a ^ (a >> 15);
    return a;
}

template <typename T>
T Knob<T>::evaluateExpression(double time, ViewIdx view, int dimension) const
{
    PythonGILLocker pgl;
    PyObject *ret;
    
    ///Reset the random state to reproduce the sequence
    randomSeed(time, hashFunction(dimension));
    try {
        ret = executeExpression(time, view,  dimension);
    } catch (...) {
        return T();
    }
    
    T val =  pyObjectToType(ret);
    Py_DECREF(ret); //< new ref
    return val;
}

template <typename T>
double
Knob<T>::evaluateExpression_pod(double time, ViewIdx view, int dimension) const
{
    PythonGILLocker pgl;
    PyObject *ret;
    
    ///Reset the random state to reproduce the sequence
    randomSeed(time, hashFunction(dimension));
    try {
        ret = executeExpression(time, view, dimension);
    } catch (...) {
        return 0.;
    }
    
    if (PyFloat_Check(ret)) {
        return (double)PyFloat_AsDouble(ret);
    } else if (PyLong_Check(ret)) {
        return (int)PyInt_AsLong(ret);
    } else if (PyObject_IsTrue(ret) == 1) {
        return 1;
    } else {
        //Strings should always fall here
        return 0.;
    }

}

template <typename T>
bool Knob<T>::getValueFromExpression(double time, ViewIdx view, int dimension,bool clamp,T* ret) const
{
    
    ///Prevent recursive call of the expression
    
    
    if (getExpressionRecursionLevel() > 0) {
        return false;
    }
    
    
    ///Check first if a value was already computed:
    
    {
        QMutexLocker k(&_valueMutex);
        typename FrameValueMap::iterator found = _exprRes[dimension].find(time);
        if (found != _exprRes[dimension].end()) {
            *ret = found->second;
            return true;
        }
    }
    
    
    {
        EXPR_RECURSION_LEVEL();
        *ret = evaluateExpression(time, view,  dimension);
    }
    
    if (clamp) {
        *ret =  clampToMinMax(*ret,dimension);
    }
    
    QMutexLocker k(&_valueMutex);
    _exprRes[dimension].insert(std::make_pair(time,*ret));
    return true;

}

template <>
bool Knob<std::string>::getValueFromExpression_pod(double time, ViewIdx view,  int dimension,bool /*clamp*/,double* ret) const
{
    ///Prevent recursive call of the expression
    
    
    if (getExpressionRecursionLevel() > 0) {
        return false;
    }
    
    {
        EXPR_RECURSION_LEVEL();
        *ret = evaluateExpression_pod(time, view, dimension);
    }
    return true;
    
}


template <typename T>
bool Knob<T>::getValueFromExpression_pod(double time, ViewIdx view, int dimension,bool clamp,double* ret) const
{
    ///Prevent recursive call of the expression
    
    
    if (getExpressionRecursionLevel() > 0) {
        return false;
    }
    
    
    ///Check first if a value was already computed:
    
    
    QMutexLocker k(&_valueMutex);
    typename FrameValueMap::iterator found = _exprRes[dimension].find(time);
    if (found != _exprRes[dimension].end()) {
        *ret = found->second;
        return true;
    }
    
    
    {
        EXPR_RECURSION_LEVEL();
        *ret = evaluateExpression_pod(time, view, dimension);
    }
    
    if (clamp) {
        *ret =  clampToMinMax(*ret,dimension);
    }
    
    //QWriteLocker k(&_valueMutex);
    _exprRes[dimension].insert(std::make_pair(time,*ret));
    return true;

}

template <>
std::string
Knob<std::string>::getValueFromMasterAt(double time, ViewIdx view, int dimension, KnobI* master) const
{
    Knob<std::string>* isString = dynamic_cast<Knob<std::string>* >(master);
    assert(isString); //< other data types aren't supported
    if (isString) {
        return isString->getValueAtTime(time,dimension,view,false);
    }
    // coverity[dead_error_line]
    return std::string();
}

template <>
std::string
Knob<std::string>::getValueFromMaster(ViewIdx view,int dimension,  KnobI* master, bool /*clamp*/) const
{
    Knob<std::string>* isString = dynamic_cast<Knob<std::string>* >(master);
    assert(isString); //< other data types aren't supported
    if (isString) {
        return isString->getValue(dimension,view,false);
    }
    // coverity[dead_error_line]
    return std::string();
}

template <typename T>
T
Knob<T>::getValueFromMasterAt(double time, ViewIdx view, int dimension, KnobI* master) const
{
    Knob<int>* isInt = dynamic_cast<Knob<int>* >(master);
    Knob<bool>* isBool = dynamic_cast<Knob<bool>* >(master);
    Knob<double>* isDouble = dynamic_cast<Knob<double>* >(master);
    assert(master->isTypePOD() && (isInt || isBool || isDouble)); //< other data types aren't supported
    if (isInt) {
        return isInt->getValueAtTime(time, dimension, view);
    } else if (isBool) {
        return isBool->getValueAtTime(time,dimension, view);
    } else if (isDouble) {
        return isDouble->getValueAtTime(time,dimension, view);
    }
    return T();
}

template <typename T>
T
Knob<T>::getValueFromMaster(ViewIdx view, int dimension, KnobI* master,bool clamp) const
{
    Knob<int>* isInt = dynamic_cast<Knob<int>* >(master);
    Knob<bool>* isBool = dynamic_cast<Knob<bool>* >(master);
    Knob<double>* isDouble = dynamic_cast<Knob<double>* >(master);
    assert(master->isTypePOD() && (isInt || isBool || isDouble)); //< other data types aren't supported
    if (isInt) {
        return (T)isInt->getValue(dimension,view,clamp);
    } else if (isBool) {
        return (T)isBool->getValue(dimension,view, clamp);
    } else if (isDouble) {
        return (T)isDouble->getValue(dimension,view, clamp);
    }
    return T();
}




template <typename T>
T
Knob<T>::getValue(int dimension, ViewIdx view,bool clamp) const
{
    assert(!ViewIdx::isAll());
    bool useGuiValues = QThread::currentThread() == qApp->thread();
    if (useGuiValues) {
        //Never clamp when using gui values
        clamp = false;
    }
 
    if (dimension >= (int)_values.size() || dimension < 0) {
        return T();
    }
    std::string hasExpr = getExpression(dimension);
    if (!hasExpr.empty()) {
        T ret;
        double time = getCurrentTime();
        if (getValueFromExpression(time, view, dimension,clamp,&ret)) {
            return ret;
        }
    }
    
    if ( isAnimated(dimension, view) ) {
        return getValueAtTime(getCurrentTime(), dimension , view,clamp);
    }
    
    ///if the knob is slaved to another knob, returns the other knob value
    std::pair<int,KnobPtr > master = getMaster(dimension);
    if (master.second) {
        return getValueFromMaster(view, master.first, master.second.get(), clamp);
    }

    QMutexLocker l(&_valueMutex);
    if (useGuiValues) {
        return _guiValues[dimension];
    } else {
        if (clamp ) {
            T ret = _values[dimension];
            return clampToMinMax(ret,dimension);
        } else {
            return _values[dimension];
        }
    }
}


template <typename T>
bool Knob<T>::getValueFromCurve(double time, ViewIdx view, int dimension, bool useGuiCurve, bool byPassMaster, bool clamp, T* ret) const
{
    boost::shared_ptr<Curve> curve;
    
    if (useGuiCurve) {
        curve = getGuiCurve(view, dimension,byPassMaster);
    }
    if (!curve) {
        curve = getCurve(view, dimension,byPassMaster);
    }
    if (curve && curve->getKeyFramesCount() > 0) {
        //getValueAt already clamps to the range for us
        *ret = (T)curve->getValueAt(time,clamp);
        return true;
    }
    return false;
}

template <>
bool Knob<std::string>::getValueFromCurve(double time, ViewIdx view,  int dimension, bool useGuiCurve, bool byPassMaster, bool /*clamp*/, std::string* ret) const
{
    
    const AnimatingKnobStringHelper* isStringAnimated = dynamic_cast<const AnimatingKnobStringHelper* >(this);
    if (isStringAnimated) {
        *ret = isStringAnimated->getStringAtTime(time,view, dimension);
        ///ret is not empty if the animated string knob has a custom interpolation
        if ( !ret->empty() ) {
            return true;
        }
    }
    assert(ret->empty());
    boost::shared_ptr<Curve> curve;
    if (useGuiCurve) {
        curve = getGuiCurve(view, dimension,byPassMaster);
    }
    if (!curve) {
        curve = getCurve(view, dimension,byPassMaster);
    }
    if (curve && curve->getKeyFramesCount() > 0) {
        assert(isStringAnimated);
        if (isStringAnimated) {
            isStringAnimated->stringFromInterpolatedValue(curve->getValueAt(time),view, ret);
        }
        return true;
    }
    return false;
}


template<typename T>
T
Knob<T>::getValueAtTime(double time,  int dimension, ViewIdx view, bool clamp ,bool byPassMaster) const
{
    assert(!ViewIdx::isAll());
    if  (dimension >= (int)_values.size() || dimension < 0) {
        return T();
    }
    
    bool useGuiValues = QThread::currentThread() == qApp->thread();
    
    std::string hasExpr = getExpression(dimension);
    if (!hasExpr.empty()) {
        T ret;
        if (getValueFromExpression(time, view, dimension,clamp,&ret)) {
            return ret;
        }
    }
    
    ///if the knob is slaved to another knob, returns the other knob value
    std::pair<int,KnobPtr > master = getMaster(dimension);
    if (!byPassMaster && master.second) {
        return getValueFromMasterAt(time, view, master.first, master.second.get());
    }
    
    T ret;
    if (getValueFromCurve(time, view, dimension, useGuiValues, byPassMaster, clamp, &ret)) {
        return ret;
    }
    
    /*if the knob as no keys at this dimension, return the value
     at the requested dimension.*/
    if (master.second) {
        return getValueFromMaster(view, master.first, master.second.get(), clamp);
    }
    QMutexLocker l(&_valueMutex);
    if (clamp) {
        ret = _values[dimension];
        return clampToMinMax(ret,dimension);
    } else {
        return _values[dimension];
    }
    
}

template <>
double Knob<std::string>::getRawCurveValueAt(double time, ViewIdx view,  int dimension) const
{
    boost::shared_ptr<Curve> curve  = getCurve(view, dimension,true);
    if (curve && curve->getKeyFramesCount() > 0) {
        //getValueAt already clamps to the range for us
        return curve->getValueAt(time,false); //< no clamping to range!
    }
    return 0;
}

template <typename T>
double Knob<T>::getRawCurveValueAt(double time, ViewIdx view, int dimension) const
{
    boost::shared_ptr<Curve> curve  = getCurve(view, dimension,true);
    if (curve && curve->getKeyFramesCount() > 0) {
        //getValueAt already clamps to the range for us
        return curve->getValueAt(time,false);//< no clamping to range!
    }
    QMutexLocker l(&_valueMutex);
    T ret = _values[dimension];
    return clampToMinMax(ret,dimension);
}

template <typename T>
double Knob<T>::getValueAtWithExpression(double time, ViewIdx view, int dimension) const
{
    std::string expr = getExpression(dimension);
    if (!expr.empty()) {
        double ret;
        if (getValueFromExpression_pod(time, view, dimension,false, &ret)) {
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
    Knob<int>* isInt = dynamic_cast<Knob<int>*>(this);
    Knob<bool>* isBool = dynamic_cast<Knob<bool>*>(this);
    Knob<double>* isDouble = dynamic_cast<Knob<double>*>(this);
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
Knob<std::string>::valueToVariant(const std::string & v,
                                  Variant* vari)
{
    Knob<std::string>* isString = dynamic_cast<Knob<std::string>*>(this);
    if (isString) {
        vari->setValue<QString>( v.c_str() );
    }
}


template<typename T>
struct Knob<T>::QueuedSetValuePrivate
{
    
    int dimension;
    ViewIdx view;
    T value;
    KeyFrame key;
    bool useKey;
    ValueChangedReasonEnum reason;
    
    QueuedSetValuePrivate(ViewIdx view, int dimension,const T& value,const KeyFrame& key_,bool useKey,ValueChangedReasonEnum reason_)
    : dimension(dimension)
    , view(view)
    , value(value)
    , key(key_)
    , useKey(useKey)
    , reason(reason_)
    {
        
    }
    
};


template<typename T>
Knob<T>::QueuedSetValue::QueuedSetValue(ViewIdx view, int dimension,const T& value,const KeyFrame& key,bool useKey,ValueChangedReasonEnum reason_)
: _imp(new QueuedSetValuePrivate(view, dimension,value,key,useKey,reason_))
{
    
}

template<typename T>
Knob<T>::QueuedSetValue::~QueuedSetValue()
{
    
}

template <typename T>
KnobHelper::ValueChangedReturnCodeEnum
Knob<T>::setValue(const T & v,
                  ViewIdx view,
                  int dimension,
                  ValueChangedReasonEnum reason,
                  KeyFrame* newKey)
{
    if ( (0 > dimension) || ( dimension > (int)_values.size() ) ) {
        throw std::invalid_argument("Knob::setValue(): Dimension out of range");
    }

    KnobHelper::ValueChangedReturnCodeEnum ret = eValueChangedReturnCodeNoKeyframeAdded;
    EffectInstance* holder = dynamic_cast<EffectInstance*>( getHolder() );
    
#ifdef DEBUG
    if (holder && reason == eValueChangedReasonPluginEdited) {
        holder->checkCanSetValueAndWarn();
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
                _signalSlotHandler->s_appendParamEditChange(reason, vari, view, dimension, 0, true,false);
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
                _signalSlotHandler->s_appendParamEditChange(reason, vari, view, dimension,0, false,false);
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
    
    
    
   
    if (holder && !holder->isSetValueCurrentlyPossible()) {
        ///If we cannot set value, queue it
        if (holder && getEvaluateOnChange()) {
            //We explicitly abort rendering now and do not wait for it to be done in EffectInstance::evaluate()
            //because the actual value change (which will call evaluate()) may arise well later
            holder->abortAnyEvaluation();
        }
        
        KnobHelper::ValueChangedReturnCodeEnum returnValue;
        
        double time = getCurrentTime();
        KeyFrame k;
        
        boost::shared_ptr<Curve> curve;
        if (newKey) {
            curve = getCurve(view, dimension);
            if (curve) {
                
                makeKeyFrame(curve.get(),time,view, v,&k);
                bool hasAnimation = curve->isAnimated();
                bool hasKeyAtTime;
                {
                    KeyFrame existingKey;
                    hasKeyAtTime = curve->getKeyFrameWithTime(time, &existingKey);
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
    
        boost::shared_ptr<QueuedSetValue> qv(new QueuedSetValue(view, dimension,v,k,returnValue != eValueChangedReturnCodeNoKeyframeAdded,reason));
        
        {
            QMutexLocker kql(&_setValuesQueueMutex);
            _setValuesQueue.push_back(qv);
        }
        if (QThread::currentThread() == qApp->thread()) {
            {
                QMutexLocker k(&_valueMutex);
                _guiValues[dimension] = v;
            }
            if (!isValueChangesBlocked()) {
                holder->onKnobValueChanged_public(this, reason, time, view, true);
            }

            if (_signalSlotHandler) {
                _signalSlotHandler->s_valueChanged(view, dimension,(int)reason);
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

    bool hasChanged;
    {
        QMutexLocker l(&_valueMutex);
        hasChanged = v != _values[dimension];
        _values[dimension] = v;
        _guiValues[dimension] = v;
    }

    double time;
    bool timeSet = false;
    
    ///Add automatically a new keyframe
    if (isAnimationEnabled() &&
        getAnimationLevel(dimension) != eAnimationLevelNone && //< if the knob is animated
         holder && //< the knob is part of a KnobHolder
         holder->getApp() && //< the app pointer is not NULL
         !holder->getApp()->getProject()->isLoadingProject() && //< we're not loading the project
         ( reason == eValueChangedReasonUserEdited ||
           reason == eValueChangedReasonPluginEdited ||
           reason == eValueChangedReasonNatronGuiEdited ||
           reason == eValueChangedReasonNatronInternalEdited ) && //< the change was made by the user or plugin
         ( newKey != NULL) ) { //< the keyframe to set is not null
        
        time = getCurrentTime();
        timeSet = true;
        bool addedKeyFrame = setValueAtTime(time, view, v, dimension,reason,newKey);
        if (addedKeyFrame) {
            ret = eValueChangedReturnCodeKeyframeAdded;
        } else {
            ret = eValueChangedReturnCodeKeyframeModified;
        }
        hasChanged = true;
    }

    if (hasChanged && ret == eValueChangedReturnCodeNoKeyframeAdded) { //the other cases already called this in setValueAtTime()
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
    if (!hasChanged && ret == eValueChangedReturnCodeNoKeyframeAdded) {
        return eValueChangedReturnCodeNothingChanged;
    }

    return ret;
} // setValue


template <typename T>
void
Knob<T>::setValues(const T& value0, const T& value1, ViewIdx view, ValueChangedReasonEnum reason)
{
    KnobHolder* holder = getHolder();
    EffectInstance* effect = 0;
    bool doEditEnd = false;
    if (holder) {
        effect = dynamic_cast<EffectInstance*>(holder);
        if (effect) {
            if (effect->isDoingInteractAction() && !effect->getApp()->isCreatingPythonGroup()) {
                effect->setMultipleParamsEditLevel(KnobHolder::eMultipleParamsEditOnCreateNewCommand);
                doEditEnd = true;
            }
        }
    }
    KeyFrame newKey;
    assert(getDimension() == 2);
    beginChanges();
    blockValueChanges();
    setValue(value0, view, 0, reason, &newKey);
    unblockValueChanges();
    setValue(value1, view, 1, reason, &newKey);
    endChanges();
    if (doEditEnd) {
        effect->setMultipleParamsEditLevel(KnobHolder::eMultipleParamsEditOff);
    }
}

template <typename T>
void
Knob<T>::setValues(const T& value0, const T& value1, const T& value2, ViewIdx view,ValueChangedReasonEnum reason)
{
    KnobHolder* holder = getHolder();
    EffectInstance* effect = 0;
    bool doEditEnd = false;
    if (holder) {
        effect = dynamic_cast<EffectInstance*>(holder);
        if (effect) {
            if (effect->isDoingInteractAction() && !effect->getApp()->isCreatingPythonGroup()) {
                effect->setMultipleParamsEditLevel(KnobHolder::eMultipleParamsEditOnCreateNewCommand);
                doEditEnd = true;
            }
        }
    }

    KeyFrame newKey;
    assert(getDimension() == 3);
    beginChanges();
    blockValueChanges();
    setValue(value0, view,  0, reason, &newKey);
    setValue(value1, view, 1, reason, &newKey);
    unblockValueChanges();
    setValue(value2, view, 2, reason, &newKey);
    endChanges();
    if (doEditEnd) {
        effect->setMultipleParamsEditLevel(KnobHolder::eMultipleParamsEditOff);
    }
}

template <typename T>
void
Knob<T>::setValues(const T& value0, const T& value1, const T& value2, const T& value3, ViewIdx view, ValueChangedReasonEnum reason)
{
    KnobHolder* holder = getHolder();
    EffectInstance* effect = 0;
    bool doEditEnd = false;
    if (holder) {
        effect = dynamic_cast<EffectInstance*>(holder);
        if (effect) {
            if (effect->isDoingInteractAction() && !effect->getApp()->isCreatingPythonGroup()) {
                effect->setMultipleParamsEditLevel(KnobHolder::eMultipleParamsEditOnCreateNewCommand);
                doEditEnd = true;
            }
        }
    }

    KeyFrame newKey;
    assert(getDimension() == 4);
    beginChanges();
    blockValueChanges();
    setValue(value0,view, 0, reason, &newKey);
    setValue(value1,view, 1, reason, &newKey);
    setValue(value2,view, 2, reason, &newKey);
    unblockValueChanges();
    setValue(value3, view,3, reason, &newKey);
    endChanges();
    if (doEditEnd) {
        effect->setMultipleParamsEditLevel(KnobHolder::eMultipleParamsEditOff);
    }
}

template <typename T>
void
Knob<T>::makeKeyFrame(Curve* curve,double time, ViewIdx /*view*/, const T& v,KeyFrame* key)
{
    double keyFrameValue;
    if ( curve->areKeyFramesValuesClampedToIntegers() ) {
        keyFrameValue = std::floor(v + 0.5);
    } else if ( curve->areKeyFramesValuesClampedToBooleans() ) {
        keyFrameValue = (bool)v;
    } else {
        keyFrameValue = (double)v;
    }
    if (keyFrameValue != keyFrameValue || boost::math::isinf(keyFrameValue)) { // check for NaN or infinity
        *key = KeyFrame( (double)time,getMaximum(0) );
    } else {
        *key = KeyFrame( (double)time,keyFrameValue );
    }
}

template <>
void
Knob<std::string>::makeKeyFrame(Curve* /*curve*/,double time, ViewIdx view, const std::string& v,KeyFrame* key)
{
    double keyFrameValue = 0.;
    AnimatingKnobStringHelper* isStringAnimatedKnob = dynamic_cast<AnimatingKnobStringHelper*>(this);
    assert(isStringAnimatedKnob);
    if (isStringAnimatedKnob) {
        isStringAnimatedKnob->stringToKeyFrameValue(time,view, v,&keyFrameValue);
    }
    
    *key = KeyFrame( (double)time,keyFrameValue );
}

template<typename T>
bool
Knob<T>::setValueAtTime(double time,
                        ViewIdx view,
                        const T & v,
                        int dimension,
                        ValueChangedReasonEnum reason,
                        KeyFrame* newKey)
{
    assert(dimension >= 0 && dimension < getDimension());
    if (!canAnimate() || !isAnimationEnabled()) {
        qDebug() << "WARNING: Attempting to call setValueAtTime on " << getName().c_str() << " which does not have animation enabled.";
        setValue(v, view, dimension, reason, newKey);
    }

    EffectInstance* holder = dynamic_cast<EffectInstance*>( getHolder() );
    
#ifdef DEBUG
    if (holder && reason == eValueChangedReasonPluginEdited) {
        holder->checkCanSetValueAndWarn();
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
                _signalSlotHandler->s_appendParamEditChange(reason, vari,view, dimension, time, true,true);
                {
                    QMutexLocker l(&_setValueRecursionLevelMutex);
                    --_setValueRecursionLevel;
                }

                return true;
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
                _signalSlotHandler->s_appendParamEditChange(reason, vari,view, dimension,time, false,true);
                {
                    QMutexLocker l(&_setValueRecursionLevelMutex);
                    --_setValueRecursionLevel;
                }
                
                return true;
            }
            break;
        }
        } // switch (paramEditLevel) {
    }


    boost::shared_ptr<Curve> curve = getCurve(view,dimension,true);
    assert(curve);
    makeKeyFrame(curve.get(), time, view, v, newKey);
    
    
    if (holder && !holder->canSetValue()) {
        
        ///If we cannot set value, queue it
        if (holder && getEvaluateOnChange()) {
            //We explicitly abort rendering now and do not wait for it to be done in EffectInstance::evaluate()
            //because the actual value change (which will call evaluate()) may arise well later
            holder->abortAnyEvaluation();
        }
        
        boost::shared_ptr<QueuedSetValueAtTime> qv(new QueuedSetValueAtTime(time, view,dimension,v,*newKey,reason));
        
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
    bool hasChanged = true;
    if (hasExistingKey && existingKey.getValue() == newKey->getValue() && existingKey.getLeftDerivative() == newKey->getLeftDerivative() &&
        existingKey.getRightDerivative() == newKey->getRightDerivative()) {
        hasChanged = false;
    }
    bool ret = curve->addKeyFrame(*newKey);
    if (holder) {
        holder->setHasAnimation(true);
    }
    guiCurveCloneInternalCurve(eCurveChangeReasonInternal, view, dimension, reason);
    
    if (_signalSlotHandler && ret) {
        _signalSlotHandler->s_keyFrameSet(time,view, dimension,(int)reason,ret);
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
Knob<T>::setValuesAtTime(double time, ViewIdx view,const T& value0, const T& value1, ValueChangedReasonEnum reason)
{
    
    KnobHolder* holder = getHolder();
    EffectInstance* effect = 0;
    bool doEditEnd = false;
    if (holder) {
        effect = dynamic_cast<EffectInstance*>(holder);
        if (effect) {
            if (effect->isDoingInteractAction() && !effect->getApp()->isCreatingPythonGroup()) {
                effect->setMultipleParamsEditLevel(KnobHolder::eMultipleParamsEditOnCreateNewCommand);
                doEditEnd = true;
            }
        }
    }
    KeyFrame newKey;
    assert(getDimension() == 2);
    beginChanges();
    blockValueChanges();
    setValueAtTime(time, view, value0, 0, reason, &newKey);
    unblockValueChanges();
    setValueAtTime(time, view, value1, 1, reason, &newKey);
    endChanges();
    if (doEditEnd) {
        effect->setMultipleParamsEditLevel(KnobHolder::eMultipleParamsEditOff);
    }
}

template<typename T>
void
Knob<T>::setValuesAtTime(double time, ViewIdx view,const T& value0, const T& value1, const T& value2, ValueChangedReasonEnum reason)
{
    KnobHolder* holder = getHolder();
    EffectInstance* effect = 0;
    bool doEditEnd = false;
    if (holder) {
        effect = dynamic_cast<EffectInstance*>(holder);
        if (effect) {
            if (effect->isDoingInteractAction() && !effect->getApp()->isCreatingPythonGroup()) {
                effect->setMultipleParamsEditLevel(KnobHolder::eMultipleParamsEditOnCreateNewCommand);
                doEditEnd = true;
            }
        }
    }
    KeyFrame newKey;
    assert(getDimension() == 3);
    beginChanges();
    blockValueChanges();
    setValueAtTime(time, view, value0, 0, reason, &newKey);
    setValueAtTime(time, view, value1, 1, reason, &newKey);
    unblockValueChanges();
    setValueAtTime(time, view, value2, 2, reason, &newKey);
    endChanges();
    if (doEditEnd) {
        effect->setMultipleParamsEditLevel(KnobHolder::eMultipleParamsEditOff);
    }

}

template<typename T>
void
Knob<T>::setValuesAtTime(double time, ViewIdx view, const T& value0, const T& value1, const T& value2, const T& value3, ValueChangedReasonEnum reason)
{
    KnobHolder* holder = getHolder();
    EffectInstance* effect = 0;
    bool doEditEnd = false;
    if (holder) {
        effect = dynamic_cast<EffectInstance*>(holder);
        if (effect) {
            if (effect->isDoingInteractAction() && !effect->getApp()->isCreatingPythonGroup()) {
                effect->setMultipleParamsEditLevel(KnobHolder::eMultipleParamsEditOnCreateNewCommand);
                doEditEnd = true;
            }
        }
    }
    KeyFrame newKey;
    assert(getDimension() == 4);
    beginChanges();
    blockValueChanges();
    setValueAtTime(time, view, value0, 0, reason, &newKey);
    setValueAtTime(time, view, value1, 1, reason, &newKey);
    setValueAtTime(time, view, value2, 2, reason, &newKey);
    unblockValueChanges();
    setValueAtTime(time, view, value3, 3, reason, &newKey);
    endChanges();
    if (doEditEnd) {
        effect->setMultipleParamsEditLevel(KnobHolder::eMultipleParamsEditOff);
    }
}

template<typename T>
void
Knob<T>::unSlave(int dimension,
                 ValueChangedReasonEnum reason,
                 bool copyState)
{
    if ( !isSlave(dimension) ) {
        return;
    }
    std::pair<int,KnobPtr > master = getMaster(dimension);
    boost::shared_ptr<KnobHelper> masterHelper = boost::dynamic_pointer_cast<KnobHelper>(master.second);

    if (masterHelper->getSignalSlotHandler() && _signalSlotHandler) {
        QObject::disconnect( masterHelper->getSignalSlotHandler().get(), SIGNAL( keyFrameSet(double, ViewIdx,int,int,bool) ),
                         _signalSlotHandler.get(), SLOT( onMasterKeyFrameSet(double, ViewIdx,int,int,bool) ) );
        QObject::disconnect( masterHelper->getSignalSlotHandler().get(), SIGNAL( keyFrameRemoved(double, ViewIdx,int,int) ),
                         _signalSlotHandler.get(), SLOT( onMasterKeyFrameRemoved(double, ViewIdx,int,int)) );
        
        QObject::disconnect( masterHelper->getSignalSlotHandler().get(), SIGNAL( keyFrameMoved(ViewIdx,int,double,double) ),
                         _signalSlotHandler.get(), SLOT( onMasterKeyFrameMoved(ViewIdx,int,double,double) ) );
        QObject::disconnect( masterHelper->getSignalSlotHandler().get(), SIGNAL(animationRemoved(ViewIdx,int) ),
                         _signalSlotHandler.get(), SLOT(onMasterAnimationRemoved(ViewIdx,int)) );
    }

    resetMaster(dimension);
    bool hasChanged = false;
    setEnabled(dimension, true);
    if (copyState) {
        ///clone the master
        hasChanged |= cloneAndCheckIfChanged(master.second.get());
    }

    if (_signalSlotHandler) {
        if (reason == eValueChangedReasonPluginEdited) {
            _signalSlotHandler->s_knobSlaved(dimension, false);
        }
    }
    if (getHolder() && _signalSlotHandler) {
        getHolder()->onKnobSlaved( this, master.second.get(),dimension,false );
    }
    if (masterHelper) {
        masterHelper->removeListener(this, dimension);
    }
    if (hasChanged) {
        evaluateValueChange(dimension, getCurrentTime(), ViewIdx(0), reason);
    }
}


template<typename T>
KnobHelper::ValueChangedReturnCodeEnum
Knob<T>::setValue(const T & value,
                  ViewIdx view,
                  int dimension,
                  bool turnOffAutoKeying)
{
    if (turnOffAutoKeying) {
        return setValue(value,view, dimension,eValueChangedReasonNatronInternalEdited,NULL);
    } else {
        KeyFrame k;

        return setValue(value,view, dimension,eValueChangedReasonNatronInternalEdited,&k);
    }
}

template<typename T>
KnobHelper::ValueChangedReturnCodeEnum
Knob<T>::onValueChanged(const T & value,
                        ViewIdx view,
                        int dimension,
                        ValueChangedReasonEnum reason,
                        KeyFrame* newKey)
{
    assert(reason == eValueChangedReasonNatronGuiEdited || reason == eValueChangedReasonUserEdited);
    return setValue(value, view, dimension,reason,newKey);
}

template<typename T>
KnobHelper::ValueChangedReturnCodeEnum
Knob<T>::setValueFromPlugin(const T & value, ViewIdx view,int dimension)
{
    KeyFrame newKey;
    return setValue(value,view, dimension,eValueChangedReasonPluginEdited,&newKey);
}

template<typename T>
void
Knob<T>::setValueAtTime(double time,
                        ViewIdx view,
                        const T & v,
                        int dimension)
{
    KeyFrame k;

    ignore_result(setValueAtTime(time, view, v,dimension,eValueChangedReasonNatronInternalEdited,&k));
}

template<typename T>
void
Knob<T>::setValueAtTimeFromPlugin(double time, ViewIdx view,const T & v,int dimension)
{
    KeyFrame k;
    
    ignore_result(setValueAtTime(time,view, v,dimension,eValueChangedReasonPluginEdited,&k));

}

template<typename T>
T
Knob<T>::getKeyFrameValueByIndex(ViewIdx view,
                                 int dimension,
                                 int index,
                                 bool* ok) const
{
    ///if the knob is slaved to another knob, returns the other knob value
    std::pair<int,KnobPtr > master = getMaster(dimension);

    if (master.second) {
        Knob<int>* isInt = dynamic_cast<Knob<int>* >( master.second.get() );
        Knob<bool>* isBool = dynamic_cast<Knob<bool>* >( master.second.get() );
        Knob<double>* isDouble = dynamic_cast<Knob<double>* >( master.second.get() );
        assert(master.second->isTypePOD() && (isInt || isBool || isDouble)); //< other data types aren't supported
        if (isInt) {
            return isInt->getKeyFrameValueByIndex(view, master.first,index,ok);
        } else if (isBool) {
            return isBool->getKeyFrameValueByIndex(view, master.first,index,ok);
        } else if (isDouble) {
            return isDouble->getKeyFrameValueByIndex(view, master.first,index,ok);
        }
    }

    assert( dimension < getDimension() );
    if ( !getKeyFramesCount(view, dimension) ) {
        *ok = false;

        return T();
    }

    boost::shared_ptr<Curve> curve = getCurve(view, dimension);
    assert(curve);
    KeyFrame kf;
    *ok =  curve->getKeyFrameWithIndex(index, &kf);

    return kf.getValue();
}

template<>
std::string
Knob<std::string>::getKeyFrameValueByIndex(ViewIdx view,
                                           int dimension,
                                           int index,
                                           bool* ok) const
{
    ///if the knob is slaved to another knob, returns the other knob value
    std::pair<int,KnobPtr > master = getMaster(dimension);

    if (master.second) {
        Knob<std::string>* isString = dynamic_cast<Knob<std::string>* >( master.second.get() );
        assert(isString); //< other data types aren't supported
        if (isString) {
            return isString->getKeyFrameValueByIndex(view, master.first,index,ok);
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
        boost::shared_ptr<Curve> curve = getCurve(view, dimension);
        assert(curve);
        KeyFrame kf;
        *ok =  curve->getKeyFrameWithIndex(index, &kf);

        if (*ok) {
            animatedString->stringFromInterpolatedValue(kf.getValue(),view, &value);
        }
    }

    return value;
}

template<typename T>
std::list<T> Knob<T>::getValueForEachDimension_mt_safe() const
{
    QMutexLocker l(&_valueMutex);
    std::list<T> ret;

    for (U32 i = 0; i < _values.size(); ++i) {
        ret.push_back(_values[i]);
    }

    return ret;
}

template<typename T>
std::vector<T> Knob<T>::getValueForEachDimension_mt_safe_vector() const
{
    QMutexLocker l(&_valueMutex);

    return _values;
}

template<typename T>
std::vector<T>
Knob<T>::getDefaultValues_mt_safe() const
{
    
    QMutexLocker l(&_valueMutex);
    
    return _defaultValues;
}

template<typename T>
T
Knob<T>::getDefaultValue(int dimension) const
{
    QMutexLocker l(&_valueMutex);
    return _defaultValues[dimension];
}

template<typename T>
void
Knob<T>::setDefaultValue(const T & v,
                         int dimension)
{
    assert( dimension < getDimension() );
    {
        QMutexLocker l(&_valueMutex);
        _defaultValues[dimension] = v;
    }
    resetToDefaultValue(dimension);
}

template <typename T>
void
Knob<T>::setDefaultValueWithoutApplying(const T& v,int dimension)
{
    assert( dimension < getDimension() );
    {
        QMutexLocker l(&_valueMutex);
        _defaultValues[dimension] = v;
    }
}

template<typename T>
void
Knob<T>::populate()
{
    for (int i = 0; i < getDimension(); ++i) {
        _values[i] = T();
        _guiValues[i] = T();
        _defaultValues[i] = T();
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
Knob<int>::isTypePOD() const
{
    return true;
}

template<>
bool
Knob<bool>::isTypePOD() const
{
    return true;
}

template<>
bool
Knob<double>::isTypePOD() const
{
    return true;
}

template<typename T>
bool
Knob<T>::isTypeCompatible(const KnobPtr & other) const
{
    return isTypePOD() == other->isTypePOD();
}

template<typename T>
bool
Knob<T>::onKeyFrameSet(double time,
                       ViewIdx view,
                       int dimension)
{
    KeyFrame key;
    boost::shared_ptr<Curve> curve;
    KnobHolder* holder = getHolder();
    bool useGuiCurve = (!holder || !holder->isSetValueCurrentlyPossible()) && getKnobGuiPointer();
    
    if (!useGuiCurve) {
        assert(holder);
        curve = getCurve(view, dimension);
    } else {
        curve = getGuiCurve(view, dimension);
        setGuiCurveHasChanged(view, dimension,true);
    }
    
    makeKeyFrame(curve.get(), time, view, getValueAtTime(time, dimension, view), &key);
    return setKeyFrame(key,view, dimension, eValueChangedReasonUserEdited);
}

template<typename T>
bool
Knob<T>::setKeyFrame(const KeyFrame& key, ViewIdx view, int dimension,ValueChangedReasonEnum reason)
{
    boost::shared_ptr<Curve> curve;
    KnobHolder* holder = getHolder();
    bool useGuiCurve = (!holder || !holder->isSetValueCurrentlyPossible()) && getKnobGuiPointer();
    
    if (!useGuiCurve) {
        assert(holder);
        curve = getCurve(view,dimension);
    } else {
        curve = getGuiCurve(view,dimension);
        setGuiCurveHasChanged(view, dimension,true);
    }
    
    bool ret = curve->addKeyFrame(key);
    
    if (!useGuiCurve) {
        guiCurveCloneInternalCurve(eCurveChangeReasonInternal, view,dimension, reason);
        evaluateValueChange(dimension, key.getTime(), view, reason);
    }
    return ret;
}

template<typename T>
bool
Knob<T>::onKeyFrameSet(double /*time*/, ViewIdx view,const KeyFrame& key,int dimension)
{
    return setKeyFrame(key, view, dimension, eValueChangedReasonUserEdited);
}

template<typename T>
void
Knob<T>::onTimeChanged(bool isPlayback, double time)
{
    int dims = getDimension();

    if (getIsSecret()) {
        return;
    }
    bool shouldRefresh = false;
    for (int i = 0; i < dims; ++i) {
        if (getKnobGuiPointer() && _signalSlotHandler && (isAnimated(i, ViewIdx(0)) || !getExpression(i).empty())) {
            shouldRefresh = true;
        }
        checkAnimationLevel(ViewIdx(0),i);
    }
    if (shouldRefresh) {
        _signalSlotHandler->s_valueChanged(ViewIdx::all(), -1, eValueChangedReasonTimeChanged);
    }
    if (evaluateValueChangeOnTimeChange() && !isPlayback) {
        //Some knobs like KnobFile do not animate but the plug-in may need to know the time has changed
        evaluateValueChange(0, time, ViewIdx(0), eValueChangedReasonTimeChanged);
    }
}

template<typename T>
double
Knob<T>::getDerivativeAtTime(double time,
                             ViewIdx view,
                             int dimension) const
{
    if ( ( dimension > getDimension() ) || (dimension < 0) ) {
        throw std::invalid_argument("Knob::getDerivativeAtTime(): Dimension out of range");
    }
    {
        std::string expr = getExpression(dimension);
        if (!expr.empty()) {
            // Compute derivative by finite differences, using values at t-0.5 and t+0.5
            return (getValueAtTime(time + 0.5,dimension, view) - getValueAtTime(time - 0.5, dimension, view))/2.;
        }
    }

    ///if the knob is slaved to another knob, returns the other knob value
    std::pair<int,KnobPtr > master = getMaster(dimension);
    if (master.second) {
        return master.second->getDerivativeAtTime(time,view, master.first);
    }

    boost::shared_ptr<Curve> curve  = getCurve(view, dimension);
    if (curve->getKeyFramesCount() > 0) {
        return curve->getDerivativeAt(time);
    } else {
        /*if the knob as no keys at this dimension, the derivative is 0.*/
        return 0.;
    }
}

template<>
double
Knob<std::string>::getDerivativeAtTime(double /*time*/,
                                       ViewIdx /*view*/,
                                       int /*dimension*/) const
{
    throw std::invalid_argument("Knob<string>::getDerivativeAtTime() not available");
}

// Compute integral using Simpsons rule:
// \int_a^b f(x) dx = (b-a)/6 * (f(a) + 4f((a+b)/2) + f(b))
template<typename T>
double
Knob<T>::getIntegrateFromTimeToTimeSimpson(double time1,
                                           double time2,
                                           ViewIdx view,
                                           int dimension) const
{
    double fa = getValueAtTime(time1, dimension, view);
    double fm = getValueAtTime((time1+time2)/2, dimension, view);
    double fb = getValueAtTime(time2, dimension, view);
    return (time2-time1)/6 * (fa + 4*fm + fb);
}

template<typename T>
double
Knob<T>::getIntegrateFromTimeToTime(double time1,
                                    double time2,
                                    ViewIdx view,
                                    int dimension) const
{
    if ( ( dimension > getDimension() ) || (dimension < 0) ) {
        throw std::invalid_argument("Knob::getIntegrateFromTimeToTime(): Dimension out of range");
    }
    {
        std::string expr = getExpression(dimension);
        if (!expr.empty()) {
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
                val += getIntegrateFromTimeToTimeSimpson(t, t+1, view, dimension);
            }
            // last chunk
            if (j < time2) {
                val += getIntegrateFromTimeToTimeSimpson(j, time2, view, dimension);
            }
            return val;
        }
    }


    ///if the knob is slaved to another knob, returns the other knob value
    std::pair<int,KnobPtr > master = getMaster(dimension);
    if (master.second) {
        assert(master.second->isTypePOD()); //< other data types aren't supported
        if (master.second->isTypePOD()) {
            return master.second->getIntegrateFromTimeToTime(time1, time2, view, master.first);
        }
    }

    boost::shared_ptr<Curve> curve  = getCurve(view, dimension);
    if (curve->getKeyFramesCount() > 0) {
        return curve->getIntegrateFromTo(time1, time2);
    } else {
        // if the knob as no keys at this dimension, the integral is trivial
        QMutexLocker l(&_valueMutex);

        return (double)_values[dimension] * (time2 - time1);
    }
}

template<>
double
Knob<std::string>::getIntegrateFromTimeToTimeSimpson(double /*time1*/,
                                                     double /*time2*/,
                                                     ViewIdx /*view*/,
                                                     int /*dimension*/) const
{
    return 0; // dummy value
}

template<>
double
Knob<std::string>::getIntegrateFromTimeToTime(double /*time1*/,
                                              double /*time2*/,
                                              ViewIdx /*view*/,
                                              int /*dimension*/) const
{
    throw std::invalid_argument("Knob<string>::getIntegrateFromTimeToTime() not available");
}

template<typename T>
void
Knob<T>::resetToDefaultValue(int dimension)
{
    KnobI::removeAnimation(ViewIdx::all(), dimension);
    T defaultV;
    {
        QMutexLocker l(&_valueMutex);
        defaultV = _defaultValues[dimension];
    }
    clearExpression(dimension,true);
    resetExtraToDefaultValue(dimension);
    ignore_result(setValue(defaultV, ViewIdx::all(), dimension, eValueChangedReasonRestoreDefault, NULL));
    if (_signalSlotHandler) {
        _signalSlotHandler->s_valueChanged(ViewIdx::all(), dimension, eValueChangedReasonRestoreDefault);
    }
    setSecret(getDefaultIsSecret());
    setEnabled(dimension, isDefaultEnabled(dimension));
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
Knob<double>::resetToDefaultValue(int dimension)
{
    KnobI::removeAnimation(ViewIdx::all(), dimension);

    ///A Knob<double> is not always a KnobDouble (it can also be a KnobColor)
    KnobDouble* isDouble = dynamic_cast<KnobDouble*>(this);
    double def;
    
    clearExpression(dimension,true);
    
    {
        QMutexLocker l(&_valueMutex);
        def = _defaultValues[dimension];
    }

    resetExtraToDefaultValue(dimension);

    // see http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#kOfxParamPropDefaultCoordinateSystem
    if (isDouble) {
        if (isDouble->getDefaultValuesAreNormalized()) {
            if (isDouble->getValueIsNormalized(dimension) == KnobDouble::eValueIsNormalizedNone) {
                // default is normalized, value is non-normalized: denormalize it!
                double time = getCurrentTime();
                isDouble->denormalize(dimension, time, &def);
            }
        } else {
            if (isDouble->getValueIsNormalized(dimension) != KnobDouble::eValueIsNormalizedNone) {
                // default is non-normalized, value is normalized: normalize it!
                double time = getCurrentTime();
                isDouble->normalize(dimension, time, &def);
            }
        }
    }
    ignore_result(setValue(def, ViewIdx::all(), dimension, eValueChangedReasonRestoreDefault, NULL));
    if (_signalSlotHandler) {
        _signalSlotHandler->s_valueChanged(ViewIdx::all(), dimension, eValueChangedReasonRestoreDefault);
    }
    setSecret(getDefaultIsSecret());
    setEnabled(dimension, isDefaultEnabled(dimension));

}

template<>
void
Knob<int>::cloneValues(KnobI* other, int dimension)
{
    Knob<int>* isInt = dynamic_cast<Knob<int>* >(other);
    Knob<bool>* isBool = dynamic_cast<Knob<bool>* >(other);
    Knob<double>* isDouble = dynamic_cast<Knob<double>* >(other);
    assert(isInt || isBool || isDouble);
    QMutexLocker k(&_valueMutex);
    if (isInt) {
        _values = isInt->getValueForEachDimension_mt_safe_vector();
        _guiValues = _values;
    } else if (isBool) {
        std::vector<bool> v = isBool->getValueForEachDimension_mt_safe_vector();
        assert( v.size() == _values.size() );
        for (unsigned i = 0; i < v.size(); ++i) {
            if ((int)i == dimension || dimension == -1) {
                _values[i] = v[i];
                _guiValues[i] = v[i];
            }
        }
    } else if (isDouble) {
        std::vector<double> v = isDouble->getValueForEachDimension_mt_safe_vector();
        assert( v.size() == _values.size() );
        for (unsigned i = 0; i < v.size(); ++i) {
            if ((int)i == dimension || dimension == -1) {
                _values[i] = v[i];
                _guiValues[i] = v[i];
            }
        }
    }
}

template<>
void
Knob<bool>::cloneValues(KnobI* other,int dimension)
{
    Knob<int>* isInt = dynamic_cast<Knob<int>* >(other);
    Knob<bool>* isBool = dynamic_cast<Knob<bool>* >(other);
    Knob<double>* isDouble = dynamic_cast<Knob<double>* >(other);
    
    int dimMin = std::min( getDimension(), other->getDimension() );
    assert(other->isTypePOD() && (isInt || isBool || isDouble)); //< other data types aren't supported
    QMutexLocker k(&_valueMutex);
    if (isInt) {
        std::vector<int> v = isInt->getValueForEachDimension_mt_safe_vector();
        
        for (int i = 0; i < dimMin; ++i) {
            if (i == dimension || dimension == -1) {
                _values[i] = v[i];
                _guiValues[i] = v[i];
            }
        }
    } else if (isBool) {
        _values = isBool->getValueForEachDimension_mt_safe_vector();
        _guiValues = _values;
    } else if (isDouble) {
        std::vector<double> v = isDouble->getValueForEachDimension_mt_safe_vector();

        for (int i = 0; i < dimMin; ++i) {
            if (i == dimension || dimension == -1) {
                _values[i] = v[i];
                _guiValues[i] = v[i];
            }
        }
    }
}

template<>
void
Knob<double>::cloneValues(KnobI* other, int dimension)
{
    Knob<int>* isInt = dynamic_cast<Knob<int>* >(other);
    Knob<bool>* isBool = dynamic_cast<Knob<bool>* >(other);
    Knob<double>* isDouble = dynamic_cast<Knob<double>* >(other);
    
    int dimMin = std::min( getDimension(), other->getDimension() );
    ///can only clone pod
    assert(other->isTypePOD() && (isInt || isBool || isDouble)); //< other data types aren't supported
    QMutexLocker k(&_valueMutex);
    if (isInt) {
        std::vector<int> v = isInt->getValueForEachDimension_mt_safe_vector();

        for (int i = 0; i < dimMin; ++i) {
            if (i == dimension || dimension == -1) {
                _values[i] = v[i];
                _guiValues[i] = v[i];
            }
        }
    } else if (isBool) {
        std::vector<bool> v = isBool->getValueForEachDimension_mt_safe_vector();

        int dimMin = std::min( getDimension(), other->getDimension() );
        for (int i = 0; i < dimMin; ++i) {
            if (i == dimension || dimension == -1) {
                _values[i] = v[i];
                _guiValues[i] = v[i];
            }
        }
    } else if (isDouble) {
        std::vector<double> v = isDouble->getValueForEachDimension_mt_safe_vector();
        
        for (int i = 0; i < dimMin; ++i) {
            if (i == dimension || dimension == -1) {
                _values[i] = v[i];
                _guiValues[i] = v[i];
            }
        }
    }
}



template<>
void
Knob<std::string>::cloneValues(KnobI* other, int dimension)
{
    Knob<std::string>* isString = dynamic_cast<Knob<std::string>* >(other);
    int dimMin = std::min( getDimension(), other->getDimension() );
    ///Can only clone strings
    assert(isString);
    if (isString) {
        QMutexLocker k(&_valueMutex);
        std::vector<std::string> v = isString->getValueForEachDimension_mt_safe_vector();
        for (int i = 0; i < dimMin; ++i) {
            if (i == dimension || dimension == -1) {
                _values[i] = v[i];
                _guiValues[i] = v[i];
            }
        }
    }
}

template<>
bool
Knob<int>::cloneValuesAndCheckIfChanged(KnobI* other, int dimension)
{
    Knob<int>* isInt = dynamic_cast<Knob<int>* >(other);
    Knob<bool>* isBool = dynamic_cast<Knob<bool>* >(other);
    Knob<double>* isDouble = dynamic_cast<Knob<double>* >(other);
    assert(isInt || isBool || isDouble);
    bool ret = false;
    QMutexLocker k(&_valueMutex);
    if (isInt) {
        _values = isInt->getValueForEachDimension_mt_safe_vector();
        _guiValues = _values;
    } else if (isBool) {
        std::vector<bool> v = isBool->getValueForEachDimension_mt_safe_vector();
        assert( v.size() == _values.size() );
        for (unsigned i = 0; i < v.size(); ++i) {
            if ((int)i == dimension || dimension == -1) {
                _guiValues[i] = v[i];
                if (_values[i] != v[i]) {
                    _values[i] = v[i];
                    ret = true;
                }
            }
        }
    } else if (isDouble) {
        std::vector<double> v = isDouble->getValueForEachDimension_mt_safe_vector();
        assert( v.size() == _values.size() );
        for (unsigned i = 0; i < v.size(); ++i) {
            if ((int)i == dimension || dimension == -1) {
                _guiValues[i] = v[i];
                if (_values[i] != v[i]) {
                    _values[i] = v[i];
                    ret = true;
                }
            }
        }
    }
    return ret;
}

template<>
bool
Knob<bool>::cloneValuesAndCheckIfChanged(KnobI* other,int dimension)
{
    Knob<int>* isInt = dynamic_cast<Knob<int>* >(other);
    Knob<bool>* isBool = dynamic_cast<Knob<bool>* >(other);
    Knob<double>* isDouble = dynamic_cast<Knob<double>* >(other);
    bool ret = false;
    int dimMin = std::min( getDimension(), other->getDimension() );
    assert(other->isTypePOD() && (isInt || isBool || isDouble)); //< other data types aren't supported
    QMutexLocker k(&_valueMutex);
    if (isInt) {
        std::vector<int> v = isInt->getValueForEachDimension_mt_safe_vector();
        
        for (int i = 0; i < dimMin; ++i) {
            if (i == dimension || dimension == -1) {
                _guiValues[i] = v[i];
                if (_values[i] != v[i]) {
                    _values[i] = v[i];
                    ret = true;
                }
            }
        }
    } else if (isBool) {
        _values = isBool->getValueForEachDimension_mt_safe_vector();
        _guiValues = _values;
    } else if (isDouble) {
        std::vector<double> v = isDouble->getValueForEachDimension_mt_safe_vector();
        
        for (int i = 0; i < dimMin; ++i) {
            if (i == dimension || dimension == -1) {
                _guiValues[i] = v[i];
                if (_values[i] != v[i]) {
                    _values[i] = v[i];
                    ret = true;
                }
            }
        }
    }
    return ret;
}

template<>
bool
Knob<double>::cloneValuesAndCheckIfChanged(KnobI* other, int dimension)
{
    Knob<int>* isInt = dynamic_cast<Knob<int>* >(other);
    Knob<bool>* isBool = dynamic_cast<Knob<bool>* >(other);
    Knob<double>* isDouble = dynamic_cast<Knob<double>* >(other);
    
    bool ret = false;
    
    int dimMin = std::min( getDimension(), other->getDimension() );
    ///can only clone pod
    assert(other->isTypePOD() && (isInt || isBool || isDouble)); //< other data types aren't supported
    QMutexLocker k(&_valueMutex);
    if (isInt) {
        std::vector<int> v = isInt->getValueForEachDimension_mt_safe_vector();
        
        for (int i = 0; i < dimMin; ++i) {
            if (i == dimension || dimension == -1) {
                _guiValues[i] = v[i];
                if (_values[i] != v[i]) {
                    _values[i] = v[i];
                    ret = true;
                }
            }
        }
    } else if (isBool) {
        std::vector<bool> v = isBool->getValueForEachDimension_mt_safe_vector();
        
        int dimMin = std::min( getDimension(), other->getDimension() );
        for (int i = 0; i < dimMin; ++i) {
            if (i == dimension || dimension == -1) {
                _guiValues[i] = v[i];
                if (_values[i] != v[i]) {
                    _values[i] = v[i];
                    ret = true;
                }
            }
        }
    } else if (isDouble) {
        std::vector<double> v = isDouble->getValueForEachDimension_mt_safe_vector();
        
        for (int i = 0; i < dimMin; ++i) {
            if (i == dimension || dimension == -1) {
                _guiValues[i] = v[i];
                if (_values[i] != v[i]) {
                    _values[i] = v[i];
                    ret = true;
                }
            }
        }
    }
    return ret;
}

template<>
bool
Knob<std::string>::cloneValuesAndCheckIfChanged(KnobI* other,int dimension)
{
    Knob<std::string>* isString = dynamic_cast<Knob<std::string>* >(other);
    int dimMin = std::min( getDimension(), other->getDimension() );
    ///Can only clone strings
    bool ret = false;
    assert(isString);
    if (isString) {
        QMutexLocker k(&_valueMutex);
        std::vector<std::string> v = isString->getValueForEachDimension_mt_safe_vector();
        for (int i = 0; i < dimMin; ++i) {
            if (i == dimension || dimension == -1) {
                _guiValues[i] = v[i];
                if (_values[i] != v[i]) {
                    _values[i] = v[i];
                    ret = true;
                }
            }
        }
    }
    return ret;
}

template <typename T>
void
Knob<T>::cloneExpressionsResults(KnobI* other,int dimension)
{
    Knob<T>* knob = dynamic_cast<Knob<T>* >(other);
    
    //Only clone expr results of the same type
    if (!knob) {
        return;
    }
    
    FrameValueMap results;
    knob->getExpressionResults(dimension,results);
    QMutexLocker k(&_valueMutex);
    _exprRes[dimension] = results;
}

template<typename T>
void
Knob<T>::clone(KnobI* other,
               int dimension)
{
    if (other == this) {
        return;
    }
    int dimMin = std::min( getDimension(), other->getDimension() );
    cloneValues(other,dimension);
    cloneExpressions(other,dimension);
    for (int i = 0; i < dimMin; ++i) {
        if (i == dimension || dimension == -1) {
            boost::shared_ptr<Curve> thisCurve = getCurve(ViewIdx(0), i,true);
            boost::shared_ptr<Curve> otherCurve = other->getCurve(ViewIdx(0),i,true);
            if (thisCurve && otherCurve) {
                thisCurve->clone(*otherCurve);
            }

            boost::shared_ptr<Curve> guiCurve = getGuiCurve(ViewIdx(0),i);
            boost::shared_ptr<Curve> otherGuiCurve = other->getGuiCurve(ViewIdx(0),i);
            if (guiCurve && otherGuiCurve) {
                guiCurve->clone(*otherGuiCurve);
            }
            checkAnimationLevel(ViewIdx(0),i);
        }
    }
    if (_signalSlotHandler) {
        _signalSlotHandler->s_valueChanged(ViewIdx::all(), dimension, eValueChangedReasonNatronInternalEdited);
        refreshListenersAfterValueChange(ViewIdx::all(), dimension);
    }
    cloneExtraData(other,dimension);
    if (getHolder()) {
        getHolder()->updateHasAnimation();
    }
    computeHasModifications();
}

template <typename T>
bool
Knob<T>::cloneAndCheckIfChanged(KnobI* other,int dimension)
{
    if (other == this) {
        return false;
    }
    
    bool hasChanged = cloneValuesAndCheckIfChanged(other,dimension);
    hasChanged |= cloneExpressionsAndCheckIfChanged(other,dimension);
    
    int dimMin = std::min( getDimension(), other->getDimension() );
    for (int i = 0; i < dimMin; ++i) {
        if (dimension == -1 || i == dimension) {
            boost::shared_ptr<Curve> thisCurve = getCurve(ViewIdx(0),i,true);
            boost::shared_ptr<Curve> otherCurve = other->getCurve(ViewIdx(0),i,true);
            if (thisCurve && otherCurve) {
                hasChanged |= thisCurve->cloneAndCheckIfChanged(*otherCurve);
            }
            boost::shared_ptr<Curve> guiCurve = getGuiCurve(ViewIdx(0),i);
            boost::shared_ptr<Curve> otherGuiCurve = other->getGuiCurve(ViewIdx(0),i);
            if (guiCurve && otherGuiCurve) {
                hasChanged |= guiCurve->cloneAndCheckIfChanged(*otherGuiCurve);
            }
            
            if (hasChanged) {
                checkAnimationLevel(ViewIdx::all(), i);
            }
        }
    }
    if (hasChanged) {
        if (_signalSlotHandler) {
            _signalSlotHandler->s_valueChanged(ViewIdx::all(), dimension, eValueChangedReasonNatronInternalEdited);
            refreshListenersAfterValueChange(ViewIdx::all(), dimension);
        }
    }
    hasChanged |= cloneExtraDataAndCheckIfChanged(other);
    if (hasChanged) {
        if (getHolder()) {
            getHolder()->updateHasAnimation();
        }
        computeHasModifications();
    }
    return hasChanged;
}

template<typename T>
void
Knob<T>::clone(KnobI* other,
               double offset,
               const RangeD* range,
               int dimension)
{
    if (other == this) {
        return;
    }
    cloneValues(other,dimension);
    cloneExpressions(other,dimension);
    int dimMin = std::min( getDimension(), other->getDimension() );
    for (int i = 0; i < dimMin; ++i) {
        if (dimension == -1 || i == dimension) {
            boost::shared_ptr<Curve> thisCurve = getCurve(ViewIdx(0),i,true);
            boost::shared_ptr<Curve> otherCurve = other->getCurve(ViewIdx(0),i,true);
            if (thisCurve && otherCurve) {
                thisCurve->clone(*otherCurve, offset, range);
            }
            boost::shared_ptr<Curve> guiCurve = getGuiCurve(ViewIdx(0),i);
            boost::shared_ptr<Curve> otherGuiCurve = other->getGuiCurve(ViewIdx(0),i);
            if (guiCurve && otherGuiCurve) {
                guiCurve->clone(*otherGuiCurve,offset,range);
            }
            checkAnimationLevel(ViewIdx::all(), i);
        }
    }
    if (_signalSlotHandler) {
        _signalSlotHandler->s_valueChanged(ViewIdx::all(), dimension, eValueChangedReasonNatronInternalEdited);
        refreshListenersAfterValueChange(ViewIdx::all(), dimension);
    }
    cloneExtraData(other,offset,range,dimension);
    if (getHolder()) {
        getHolder()->updateHasAnimation();
    }
}

template<typename T>
void
Knob<T>::cloneAndUpdateGui(KnobI* other,int dimension)
{
    if (other == this) {
        return;
    }
    int dimMin = std::min( getDimension(), other->getDimension() );
    cloneValues(other,dimension);
    cloneExpressions(other);
    for (int i = 0; i < dimMin; ++i) {
        if (dimension == -1 || i == dimension) {
            if (_signalSlotHandler && isAnimated(i, ViewIdx(0))) {
                _signalSlotHandler->s_animationAboutToBeRemoved(ViewIdx::all(), i);
                _signalSlotHandler->s_animationRemoved(ViewIdx::all(), i);
            }
            boost::shared_ptr<Curve> curve = getCurve(ViewIdx(0),i,true);
            boost::shared_ptr<Curve> otherCurve = other->getCurve(ViewIdx(0),i,true);
            if (curve && otherCurve) {
                curve->clone( *other->getCurve(ViewIdx(0),i,true) );
            }
            boost::shared_ptr<Curve> guiCurve = getGuiCurve(ViewIdx(0),i);
            boost::shared_ptr<Curve> otherGuiCurve = other->getGuiCurve(ViewIdx(0),i);
            if (guiCurve && otherGuiCurve) {
                guiCurve->clone(*otherGuiCurve);
            }
            if (_signalSlotHandler) {
                std::list<double> keysList;
                KeyFrameSet keys;
                if (curve) {
                    keys = curve->getKeyFrames_mt_safe();
                }
                for (KeyFrameSet::iterator it = keys.begin(); it!=keys.end(); ++it) {
                    keysList.push_back(it->getTime());
                }
                if (!keysList.empty()) {
                    _signalSlotHandler->s_multipleKeyFramesSet(keysList, ViewIdx::all(), i, (int)eValueChangedReasonNatronInternalEdited);
                }
            }
            checkAnimationLevel(ViewIdx::all(), i);
        }
    }
    if (_signalSlotHandler) {
        _signalSlotHandler->s_valueChanged(ViewIdx::all(), dimension, eValueChangedReasonNatronInternalEdited);
    }
    refreshListenersAfterValueChange(ViewIdx::all(), dimension);
    cloneExtraData(other,dimension);
    if (getHolder()) {
        getHolder()->updateHasAnimation();
        computeHasModifications();
    }
}

template <typename T>
void
Knob<T>::cloneDefaultValues(KnobI* other)
{
    int dims = std::min(getDimension(), other->getDimension());
    Knob<T>* otherT = dynamic_cast<Knob<T>*>(other);
    assert(otherT);
    if (!otherT) {
        // coverity[dead_error_line]
        return;
    }
    
    for (int i = 0; i < dims; ++i) {
        setDefaultValue(otherT->getDefaultValue(i),i);
    }


}

template <typename T>
bool
Knob<T>::dequeueValuesSet(bool disableEvaluation)
{
    
    std::map<int,ValueChangedReasonEnum> dimensionChanged;
    bool ret = false;
    cloneGuiCurvesIfNeeded(dimensionChanged);
    {
        QMutexLocker kql(&_setValuesQueueMutex);
   
        
        QMutexLocker k(&_valueMutex);
        for (typename std::list<boost::shared_ptr<QueuedSetValue> >::iterator it = _setValuesQueue.begin(); it != _setValuesQueue.end(); ++it) {
           
            QueuedSetValueAtTime* isAtTime = dynamic_cast<QueuedSetValueAtTime*>(it->get());
           
            
            if (!isAtTime) {
                
                if ((*it)->_imp->useKey) {
                    boost::shared_ptr<Curve> curve = getCurve((*it)->_imp->view, (*it)->_imp->dimension);
                    if (curve) {
                        curve->addKeyFrame((*it)->_imp->key);
                    }

                    if (getHolder()) {
                        getHolder()->setHasAnimation(true);
                    }
                } else {
                    if (_values[(*it)->_imp->dimension] != (*it)->_imp->value) {
                        _values[(*it)->_imp->dimension] = (*it)->_imp->value;
                        _guiValues[(*it)->_imp->dimension] = (*it)->_imp->value;
                        dimensionChanged.insert(std::make_pair((*it)->_imp->dimension,(*it)->_imp->reason));
                    }
                }
            } else {
                boost::shared_ptr<Curve> curve = getCurve((*it)->_imp->view,(*it)->_imp->dimension);
                if (curve) {
                    KeyFrame existingKey;
                    bool hasKey = curve->getKeyFrameWithTime((*it)->_imp->key.getTime(),&existingKey);
                    if (!hasKey || existingKey.getTime() != (*it)->_imp->key.getTime() || existingKey.getValue() != (*it)->_imp->key.getValue() ||
                        existingKey.getLeftDerivative() != (*it)->_imp->key.getLeftDerivative() ||
                        existingKey.getRightDerivative() != (*it)->_imp->key.getRightDerivative()) {
                        dimensionChanged.insert(std::make_pair((*it)->_imp->dimension,(*it)->_imp->reason));
                            curve->addKeyFrame((*it)->_imp->key);
                        }
                }

                if (getHolder()) {
                    getHolder()->setHasAnimation(true);
                }
            }
        }
        _setValuesQueue.clear();
    }
    cloneInternalCurvesIfNeeded(dimensionChanged);

    clearExpressionsResultsIfNeeded(dimensionChanged);
    
    ret |= !dimensionChanged.empty();
    
    if (!disableEvaluation && !dimensionChanged.empty()) {
        beginChanges();
        double time = getCurrentTime();
        for (std::map<int,ValueChangedReasonEnum>::iterator it = dimensionChanged.begin(); it != dimensionChanged.end(); ++it) {
            evaluateValueChange(it->first, time, ViewIdx(0), it->second);
        }
        endChanges();
    }
    return ret;
}

template <typename T>
void Knob<T>::computeHasModifications()
{
    bool oneChanged = false;
    for (int i = 0; i < getDimension(); ++i) {
        
        bool hasModif = false;
        std::string expr = getExpression(i);
        if (!expr.empty()) {
            hasModif = true;
        }
        
        if (!hasModif) {
            boost::shared_ptr<Curve> c = getCurve(ViewIdx(0),i);
            if (c && c->isAnimated()) {
                hasModif = true;
            }
        }
        
        if (!hasModif) {
            std::pair<int,KnobPtr > master = getMaster(i);
            if (master.second) {
                hasModif = true;
            }
        }
        
        ///Check expressions too in the future
        if (!hasModif) {
            QMutexLocker k(&_valueMutex);
            if (_values[i] != _defaultValues[i]) {
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

NATRON_NAMESPACE_EXIT;

#endif // KNOBIMPL_H
