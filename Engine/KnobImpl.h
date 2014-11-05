//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#ifndef KNOBIMPL_H
#define KNOBIMPL_H

#include "Knob.h"

#include <cfloat>
#include <stdexcept>
#include <string>
#include <boost/math/special_functions/fpclassify.hpp>
#include <QString>
#include "Engine/Curve.h"
#include "Engine/AppInstance.h"
#include "Engine/Project.h"
#include "Engine/TimeLine.h"
#include "Engine/EffectInstance.h"
#include "Engine/KnobTypes.h"


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
      , _valueMutex(QReadWriteLock::Recursive)
      , _values(dimension)
      , _defaultValues(dimension)
      , _minMaxMutex(QReadWriteLock::Recursive)
      , _minimums(dimension)
      , _maximums(dimension)
      , _displayMins(dimension)
      , _displayMaxs(dimension)
      , _setValueRecursionLevel(0)
      , _setValueRecursionLevelMutex(QMutex::Recursive)
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

//Declare the specialization before defining it to avoid the following
//error: explicit specialization of 'getValueAtTime' after instantiation
template<>
std::string Knob<std::string>::getValueAtTime(double time, int dimension,bool clamp,bool byPassMaster) const;

template<>
std::string
Knob<std::string>::getValue(int dimension,bool /*clampToMinMax*/) const
{
    if ( isAnimated(dimension) ) {
        SequenceTime time;
        if ( !getHolder() || !getHolder()->getApp() ) {
            time = 0;
        } else {
            Natron::EffectInstance* isEffect = dynamic_cast<Natron::EffectInstance*>( getHolder() );
            if (isEffect) {
                time = isEffect->getThreadLocalRenderTime();
            } else {
                time = getHolder()->getApp()->getTimeLine()->currentFrame();
            }
        }

        return getValueAtTime(time, dimension,false);
    }

    if ( ( dimension > (int)_values.size() ) || (dimension < 0) ) {
        throw std::invalid_argument("Knob::getValue(): Dimension out of range");
    }
    ///if the knob is slaved to another knob, returns the other knob value
    std::pair<int,boost::shared_ptr<KnobI> > master = getMaster(dimension);
    if (master.second) {
        Knob<std::string>* isString = dynamic_cast<Knob<std::string>* >( master.second.get() );
        assert(isString); //< other data types aren't supported
        return isString->getValue(master.first,false);
    }

    QReadLocker l(&_valueMutex);

    return _values[dimension];
}

template <typename T>
T
Knob<T>::getValue(int dimension,bool clamp) const
{
    if ( isAnimated(dimension) ) {
        return getValueAtTime(getCurrentTime(), dimension,clamp);
    }

    if ( ( dimension > (int)_values.size() ) || (dimension < 0) ) {
        throw std::invalid_argument("Knob::getValue(): Dimension out of range");
    }
    ///if the knob is slaved to another knob, returns the other knob value
    std::pair<int,boost::shared_ptr<KnobI> > master = getMaster(dimension);
    if (master.second) {
        Knob<int>* isInt = dynamic_cast<Knob<int>* >( master.second.get() );
        Knob<bool>* isBool = dynamic_cast<Knob<bool>* >( master.second.get() );
        Knob<double>* isDouble = dynamic_cast<Knob<double>* >( master.second.get() );
        assert(isInt || isBool || isDouble); //< other data types aren't supported
        if (isInt) {
            return (T)isInt->getValue(master.first,clamp);
        } else if (isBool) {
            return (T)isBool->getValue(master.first,clamp);
        } else if (isDouble) {
            return (T)isDouble->getValue(master.first,clamp);
        }
    }
    QReadLocker l(&_valueMutex);
    if (clamp ) {
        T ret = _values[dimension];
        return clampToMinMax(ret,dimension);
    } else {
        return _values[dimension];
    }
}

template<>
std::string
Knob<std::string>::getValueAtTime(double time,
                                  int dimension,bool /*clampToMinMax*/,bool byPassMaster) const
{
    if ( ( dimension > getDimension() ) || (dimension < 0) ) {
        throw std::invalid_argument("Knob::getValueAtTime(): Dimension out of range");
    }


    ///if the knob is slaved to another knob, returns the other knob value
    std::pair<int,boost::shared_ptr<KnobI> > master = getMaster(dimension);
    if (!byPassMaster && master.second) {
        Knob<std::string>* isString = dynamic_cast<Knob<std::string>* >( master.second.get() );
        assert(isString); //< other data types aren't supported
        return isString->getValueAtTime(time,master.first,false);
    }
    std::string ret;
    const AnimatingString_KnobHelper* isStringAnimated = dynamic_cast<const AnimatingString_KnobHelper* >(this);
    if (isStringAnimated) {
        ret = isStringAnimated->getStringAtTime(time,dimension);
        ///ret is not empty if the animated string knob has a custom interpolation
        if ( !ret.empty() ) {
            return ret;
        }
    }
    assert( ret.empty() );

    boost::shared_ptr<Curve> curve  = getCurve(dimension,byPassMaster);
    if (curve->getKeyFramesCount() > 0) {
        assert(isStringAnimated);
        isStringAnimated->stringFromInterpolatedValue(curve->getValueAt(time), &ret);

        return ret;
    } else {
        /*if the knob as no keys at this dimension, return the value
           at the requested dimension.*/
        if (master.second) {
            Knob<std::string>* isString = dynamic_cast<Knob<std::string>* >( master.second.get() );
            assert(isString); //< other data types aren't supported
            return isString->getValue(master.first);
        }
        
        QReadLocker l(&_valueMutex);
        
        return _values[dimension];

    }
}

template<typename T>
T
Knob<T>::getValueAtTime(double time,
                        int dimension,bool clamp ,bool byPassMaster) const
{
    if ( ( dimension > getDimension() ) || (dimension < 0) ) {
        throw std::invalid_argument("Knob::getValueAtTime(): Dimension out of range");
    }


    ///if the knob is slaved to another knob, returns the other knob value
    std::pair<int,boost::shared_ptr<KnobI> > master = getMaster(dimension);
    if (!byPassMaster && master.second) {
        Knob<int>* isInt = dynamic_cast<Knob<int>* >( master.second.get() );
        Knob<bool>* isBool = dynamic_cast<Knob<bool>* >( master.second.get() );
        Knob<double>* isDouble = dynamic_cast<Knob<double>* >( master.second.get() );
        assert(isInt || isBool || isDouble); //< other data types aren't supported
        if (isInt) {
            return isInt->getValueAtTime(time,master.first);
        } else if (isBool) {
            return isBool->getValueAtTime(time,master.first);
        } else if (isDouble) {
            return isDouble->getValueAtTime(time,master.first);
        }
    }
    boost::shared_ptr<Curve> curve  = getCurve(dimension,byPassMaster);
    if (curve->getKeyFramesCount() > 0) {
        //getValueAt already clamps to the range for us
        return (T)curve->getValueAt(time,clamp);
    } else {
        /*if the knob as no keys at this dimension, return the value
           at the requested dimension.*/
        if (master.second) {
            Knob<int>* isInt = dynamic_cast<Knob<int>* >( master.second.get() );
            Knob<bool>* isBool = dynamic_cast<Knob<bool>* >( master.second.get() );
            Knob<double>* isDouble = dynamic_cast<Knob<double>* >( master.second.get() );
            assert(isInt || isBool || isDouble); //< other data types aren't supported
            if (isInt) {
                return isInt->getValue(master.first);
            } else if (isBool) {
                return isBool->getValue(master.first);
            } else if (isDouble) {
                return isDouble->getValue(master.first);
            }
        }
        QReadLocker l(&_valueMutex);
        if (clamp) {
            T ret = _values[dimension];
            return clampToMinMax(ret,dimension);
        } else {
            return _values[dimension];
        }
    }
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
    T value;
    KeyFrame key;
    bool useKey;
    
    QueuedSetValuePrivate(int dimension,const T& value,const KeyFrame& key_,bool useKey)
    : dimension(dimension)
    , value(value)
    , key(key_)
    , useKey(useKey)
    {
        
    }
    
};


template<typename T>
Knob<T>::QueuedSetValue::QueuedSetValue(int dimension,const T& value,const KeyFrame& key,bool useKey)
: _imp(new QueuedSetValuePrivate(dimension,value,key,useKey))
{
    
}

template<typename T>
Knob<T>::QueuedSetValue::~QueuedSetValue()
{
    
}

template <typename T>
KnobHelper::ValueChangedReturnCode
Knob<T>::setValue(const T & v,
                  int dimension,
                  Natron::ValueChangedReasonEnum reason,
                  KeyFrame* newKey)
{
    if ( (0 > dimension) || ( dimension > (int)_values.size() ) ) {
        throw std::invalid_argument("Knob::setValue(): Dimension out of range");
    }

    KnobHelper::ValueChangedReturnCode ret = NO_KEYFRAME_ADDED;
    Natron::EffectInstance* holder = dynamic_cast<Natron::EffectInstance*>( getHolder() );
    
#ifdef DEBUG
    if (holder) {
        holder->checkCanSetValueAndWarn();
    }
#endif
    
    if ( holder && (reason == Natron::eValueChangedReasonPluginEdited) && getKnobGuiPointer() ) {
        KnobHolder::MultipleParamsEditLevel paramEditLevel = holder->getMultipleParamsEditLevel();
        switch (paramEditLevel) {
        case KnobHolder::PARAM_EDIT_OFF:
        default:
            break;
        case KnobHolder::PARAM_EDIT_ON_CREATE_NEW_COMMAND: {
            if ( !get_SetValueRecursionLevel() ) {
                Variant vari;
                valueToVariant(v, &vari);
                holder->setMultipleParamsEditLevel(KnobHolder::PARAM_EDIT_ON);
                {
                    QMutexLocker l(&_setValueRecursionLevelMutex);
                    ++_setValueRecursionLevel;
                }
                _signalSlotHandler->s_appendParamEditChange(vari, dimension, 0, true,false);
                {
                    QMutexLocker l(&_setValueRecursionLevelMutex);
                    --_setValueRecursionLevel;
                }

                return ret;
            }
            break;
        }
        case KnobHolder::PARAM_EDIT_ON: {
            if ( !get_SetValueRecursionLevel() ) {
                Variant vari;
                valueToVariant(v, &vari);
                {
                    QMutexLocker l(&_setValueRecursionLevelMutex);
                    ++_setValueRecursionLevel;
                }
                _signalSlotHandler->s_appendParamEditChange(vari, dimension,0, false,false);
                {
                    QMutexLocker l(&_setValueRecursionLevelMutex);
                    --_setValueRecursionLevel;
                }

                return ret;
            }
            break;
        }
        }

        ///basically if we enter this if condition, for each dimension the undo stack will create a new command.
        ///the caller should have tested this prior to calling this function and correctly called editBegin() editEnd()
        ///to bracket the setValue()  calls within the same undo/redo command.
        if ( holder->isDoingInteractAction() ) {
            if ( !get_SetValueRecursionLevel() ) {
                Variant vari;
                valueToVariant(v, &vari);
                _signalSlotHandler->s_setValueWithUndoStack(vari, dimension);

                return ret;
            }
        }
    }
    
    ///If we cannot set value, queue it
    if (holder && !holder->canSetValue()) {
        
        KnobHelper::ValueChangedReturnCode returnValue;
        
        SequenceTime time = getCurrentTime();
        KeyFrame k;
        
        boost::shared_ptr<Curve> curve;
        if (newKey) {
            curve = getCurve(dimension);
            assert(curve);

            makeKeyFrame(curve.get(),time,v,&k);
            bool hasAnimation = curve->isAnimated();
            bool hasKeyAtTime;
            {
                KeyFrame existingKey;
                hasKeyAtTime = curve->getKeyFrameWithTime(time, &existingKey);
            }
            if (hasAnimation && hasKeyAtTime) {
                returnValue =  KEYFRAME_MODIFIED;
                setInternalCurveHasChanged(dimension, true);
            } else if (hasAnimation) {
                returnValue =  KEYFRAME_ADDED;
                setInternalCurveHasChanged(dimension, true);
            } else {
                returnValue =  NO_KEYFRAME_ADDED;
            }

        } else {
            returnValue =  NO_KEYFRAME_ADDED;
        }
    
        
        boost::shared_ptr<QueuedSetValue> qv(new QueuedSetValue(dimension,v,k,returnValue != NO_KEYFRAME_ADDED));
        
        {
            QMutexLocker kql(&_setValuesQueueMutex);
            _setValuesQueue.push_back(qv);
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
        QWriteLocker l(&_valueMutex);
        _values[dimension] = v;
    }

    ///Add automatically a new keyframe
    if ( (getAnimationLevel(dimension) != Natron::eAnimationLevelNone) && //< if the knob is animated
         holder && //< the knob is part of a KnobHolder
         holder->getApp() && //< the app pointer is not NULL
         !holder->getApp()->getProject()->isLoadingProject() && //< we're not loading the project
         ( reason == Natron::eValueChangedReasonUserEdited ||
           reason == Natron::eValueChangedReasonPluginEdited ||
           reason == Natron::eValueChangedReasonNatronGuiEdited ||
           reason == Natron::eValueChangedReasonNatronInternalEdited ) && //< the change was made by the user or plugin
         ( newKey != NULL) ) { //< the keyframe to set is not null
        
        SequenceTime time = getCurrentTime();
        
        bool addedKeyFrame = setValueAtTime(time, v, dimension,reason,newKey);
        if (addedKeyFrame) {
            ret = KEYFRAME_ADDED;
        } else {
            ret = KEYFRAME_MODIFIED;
        }
    }

    if (ret == NO_KEYFRAME_ADDED) { //the other cases already called this in setValueAtTime()
        evaluateValueChange(dimension,reason);
    }
    {
        QMutexLocker l(&_setValueRecursionLevelMutex);
        --_setValueRecursionLevel;
        assert(_setValueRecursionLevel >= 0);
    }


    return ret;
} // setValue

template <typename T>
void
Knob<T>::makeKeyFrame(Curve* curve,double time,const T& v,KeyFrame* key)
{
    double keyFrameValue;
    if ( curve->areKeyFramesValuesClampedToIntegers() ) {
        keyFrameValue = std::floor(v + 0.5);
    } else if ( curve->areKeyFramesValuesClampedToBooleans() ) {
        keyFrameValue = (bool)v;
    } else {
        keyFrameValue = (double)v;
    }
    if (boost::math::isnan(keyFrameValue) || boost::math::isinf(keyFrameValue)) {
        *key = KeyFrame( (double)time,getMaximum(0) );
    } else {
        *key = KeyFrame( (double)time,keyFrameValue );
    }
}

template <>
void
Knob<std::string>::makeKeyFrame(Curve* /*curve*/,double time,const std::string& v,KeyFrame* key)
{
    double keyFrameValue;
    AnimatingString_KnobHelper* isStringAnimatedKnob = dynamic_cast<AnimatingString_KnobHelper*>(this);
    assert(isStringAnimatedKnob);
    isStringAnimatedKnob->stringToKeyFrameValue(time,v,&keyFrameValue);
    
    *key = KeyFrame( (double)time,keyFrameValue );
}

template<typename T>
bool
Knob<T>::setValueAtTime(int time,
                        const T & v,
                        int dimension,
                        Natron::ValueChangedReasonEnum reason,
                        KeyFrame* newKey)
{
    if ( ( dimension > getDimension() ) || (dimension < 0) ) {
        throw std::invalid_argument("Knob::setValueAtTime(): Dimension out of range");
    }

    Natron::EffectInstance* holder = dynamic_cast<Natron::EffectInstance*>( getHolder() );
    
#ifdef DEBUG
    if (holder) {
        holder->checkCanSetValueAndWarn();
    }
#endif
    
    if ( holder && (reason == Natron::eValueChangedReasonPluginEdited) && getKnobGuiPointer() ) {
        KnobHolder::MultipleParamsEditLevel paramEditLevel = holder->getMultipleParamsEditLevel();
        switch (paramEditLevel) {
        case KnobHolder::PARAM_EDIT_OFF:
        default:
            break;
        case KnobHolder::PARAM_EDIT_ON_CREATE_NEW_COMMAND: {
            if ( !get_SetValueRecursionLevel() ) {
                Variant vari;
                valueToVariant(v, &vari);
                holder->setMultipleParamsEditLevel(KnobHolder::PARAM_EDIT_ON);
                _signalSlotHandler->s_appendParamEditChange(vari, dimension, time, true,true);

                return true;
            }
            break;
        }
        case KnobHolder::PARAM_EDIT_ON: {
            if ( !get_SetValueRecursionLevel() ) {
                Variant vari;
                valueToVariant(v, &vari);
                _signalSlotHandler->s_appendParamEditChange(vari, dimension,time, false,true);

                return true;
            }
            break;
        }
        }
    }


    boost::shared_ptr<Curve> curve = getCurve(dimension,true);
    makeKeyFrame(curve.get(), time, v, newKey);
    
    ///If we cannot set value, queue it
    if (holder && !holder->canSetValue()) {
        
    
        boost::shared_ptr<QueuedSetValueAtTime> qv(new QueuedSetValueAtTime(time,dimension,v,*newKey));
        
        {
            QMutexLocker kql(&_setValuesQueueMutex);
            _setValuesQueue.push_back(qv);
        }
        
        assert(curve);
        
        setInternalCurveHasChanged(dimension, true);
        
        KeyFrame k;
        bool hasAnimation = curve->isAnimated();
        bool hasKeyAtTime = curve->getKeyFrameWithTime(time, &k);
        if (hasAnimation && hasKeyAtTime) {
            return KEYFRAME_MODIFIED;
        } else {
            return KEYFRAME_ADDED;
        }

    } else {
        ///There might be stuff in the queue that must be processed first
        dequeueValuesSet(true);
    }
    
    

    bool ret = curve->addKeyFrame(*newKey);
    
    guiCurveCloneInternalCurve(dimension);
    
    if (_signalSlotHandler && ret) {
        _signalSlotHandler->s_keyFrameSet(time,dimension,(int)reason,ret);
    }
    evaluateValueChange(dimension, reason);

    return ret;
} // setValueAtTime


template<typename T>
void
Knob<T>::unSlave(int dimension,
                 Natron::ValueChangedReasonEnum reason,
                 bool copyState)
{
    if ( !isSlave(dimension) ) {
        return;
    }
    std::pair<int,boost::shared_ptr<KnobI> > master = getMaster(dimension);
    boost::shared_ptr<KnobHelper> helper = boost::dynamic_pointer_cast<KnobHelper>(master.second);

    if (helper->getSignalSlotHandler() && _signalSlotHandler) {
        QObject::disconnect( helper->getSignalSlotHandler().get(), SIGNAL( updateSlaves(int) ), _signalSlotHandler.get(),
                             SLOT( onMasterChanged(int) ) );
        QObject::disconnect( helper->getSignalSlotHandler().get(), SIGNAL( keyFrameSet(SequenceTime,int,int,bool) ),
                         _signalSlotHandler.get(), SLOT( onMasterKeyFrameSet(SequenceTime,int,int,bool) ) );
        QObject::disconnect( helper->getSignalSlotHandler().get(), SIGNAL( keyFrameRemoved(SequenceTime,int,int) ),
                         _signalSlotHandler.get(), SLOT( onMasterKeyFrameRemoved(SequenceTime,int,int)) );
        
        QObject::disconnect( helper->getSignalSlotHandler().get(), SIGNAL( keyFrameMoved(int,int,int) ),
                         _signalSlotHandler.get(), SLOT( onMasterKeyFrameMoved(int,int,int) ) );
        QObject::disconnect( helper->getSignalSlotHandler().get(), SIGNAL(animationRemoved(int) ),
                         _signalSlotHandler.get(), SLOT(onMasterAnimationRemoved(int)) );
    }

    resetMaster(dimension);
    if (copyState) {
        ///clone the master
        clone( master.second.get() );
    }

    if (_signalSlotHandler) {
        if (reason == Natron::eValueChangedReasonPluginEdited) {
            _signalSlotHandler->s_knobSlaved(dimension, false);
        }
    }
    if (getHolder() && _signalSlotHandler) {
        getHolder()->onKnobSlaved( _signalSlotHandler->getKnob(),dimension,false, master.second->getHolder() );
    }
    evaluateValueChange(dimension, reason);
}

template<>
void
Knob<std::string>::unSlave(int dimension,
                           Natron::ValueChangedReasonEnum reason,
                           bool copyState)
{
    assert( isSlave(dimension) );

    std::pair<int,boost::shared_ptr<KnobI> > master = getMaster(dimension);

    if (copyState) {
        ///clone the master
        {
            Knob<std::string>* isString = dynamic_cast<Knob<std::string>* >( master.second.get() );
            assert(isString); //< other data types aren't supported
            QWriteLocker l1(&_valueMutex);
            _values[dimension] =  isString->getValue(master.first);
        }
        getCurve(dimension)->clone( *( master.second->getCurve(master.first) ) );

        cloneExtraData( master.second.get() );
    }
    KnobHelper* helper = dynamic_cast<KnobHelper*>( master.second.get() );
    QObject::disconnect( helper->getSignalSlotHandler().get(), SIGNAL( updateSlaves(int) ), _signalSlotHandler.get(),
                         SLOT( onMasterChanged(int) ) );
    QObject::disconnect( helper->getSignalSlotHandler().get(), SIGNAL( keyFrameSet(SequenceTime,int,int,bool) ),
                        _signalSlotHandler.get(), SLOT( onMasterKeyFrameSet(SequenceTime,int,int,bool) ) );
    QObject::disconnect( helper->getSignalSlotHandler().get(), SIGNAL( keyFrameRemoved(SequenceTime,int,int) ),
                        _signalSlotHandler.get(), SLOT( onMasterKeyFrameRemoved(SequenceTime,int,int)) );
    
    QObject::disconnect( helper->getSignalSlotHandler().get(), SIGNAL( keyFrameMoved(int,int,int) ),
                        _signalSlotHandler.get(), SLOT( onMasterKeyFrameMoved(int,int,int) ) );
    QObject::disconnect( helper->getSignalSlotHandler().get(), SIGNAL(animationRemoved(int) ),
                        _signalSlotHandler.get(), SLOT(onMasterAnimationRemoved(int)) );
    
    resetMaster(dimension);

    _signalSlotHandler->s_valueChanged(dimension,reason);
    if (reason == Natron::eValueChangedReasonPluginEdited) {
        _signalSlotHandler->s_knobSlaved(dimension, false);
    }
    helper->removeListener(this);
}

template<typename T>
KnobHelper::ValueChangedReturnCode
Knob<T>::setValue(const T & value,
                  int dimension,
                  bool turnOffAutoKeying)
{
    if (turnOffAutoKeying) {
        return setValue(value,dimension,Natron::eValueChangedReasonNatronInternalEdited,NULL);
    } else {
        KeyFrame k;

        return setValue(value,dimension,Natron::eValueChangedReasonNatronInternalEdited,&k);
    }
}

template<typename T>
KnobHelper::ValueChangedReturnCode
Knob<T>::onValueChanged(const T & value,
                        int dimension,
                        Natron::ValueChangedReasonEnum reason,
                        KeyFrame* newKey)
{
    assert(reason == Natron::eValueChangedReasonNatronGuiEdited || reason == Natron::eValueChangedReasonUserEdited);
    return setValue(value, dimension,reason,newKey);
}

template<typename T>
KnobHelper::ValueChangedReturnCode
Knob<T>::setValueFromPlugin(const T & value,int dimension)
{
    KeyFrame newKey;
    return setValue(value,dimension,Natron::eValueChangedReasonPluginEdited,&newKey);
}

template<typename T>
void
Knob<T>::setValueAtTime(int time,
                        const T & v,
                        int dimension)
{
    KeyFrame k;

    (void)setValueAtTime(time,v,dimension,Natron::eValueChangedReasonNatronInternalEdited,&k);
}

template<typename T>
void
Knob<T>::setValueAtTimeFromPlugin(int time,const T & v,int dimension)
{
    KeyFrame k;
    
    (void)setValueAtTime(time,v,dimension,Natron::eValueChangedReasonPluginEdited,&k);

}

template<typename T>
T
Knob<T>::getKeyFrameValueByIndex(int dimension,
                                 int index,
                                 bool* ok) const
{
    ///if the knob is slaved to another knob, returns the other knob value
    std::pair<int,boost::shared_ptr<KnobI> > master = getMaster(dimension);

    if (master.second) {
        Knob<int>* isInt = dynamic_cast<Knob<int>* >( master.second.get() );
        Knob<bool>* isBool = dynamic_cast<Knob<bool>* >( master.second.get() );
        Knob<double>* isDouble = dynamic_cast<Knob<double>* >( master.second.get() );
        assert(isInt || isBool || isDouble); //< other data types aren't supported
        if (isInt) {
            return isInt->getKeyFrameValueByIndex(master.first,index,ok);
        } else if (isBool) {
            return isBool->getKeyFrameValueByIndex(master.first,index,ok);
        } else if (isDouble) {
            return isDouble->getKeyFrameValueByIndex(master.first,index,ok);
        }
    }

    assert( dimension < getDimension() );
    if ( !getKeyFramesCount(dimension) ) {
        *ok = false;

        return T();
    }

    boost::shared_ptr<Curve> curve = getCurve(dimension);
    assert(curve);
    KeyFrame kf;
    *ok =  curve->getKeyFrameWithIndex(index, &kf);

    return kf.getValue();
}

template<>
std::string
Knob<std::string>::getKeyFrameValueByIndex(int dimension,
                                           int index,
                                           bool* ok) const
{
    ///if the knob is slaved to another knob, returns the other knob value
    std::pair<int,boost::shared_ptr<KnobI> > master = getMaster(dimension);

    if (master.second) {
        Knob<std::string>* isString = dynamic_cast<Knob<std::string>* >( master.second.get() );
        assert(isString); //< other data types aren't supported
        return isString->getKeyFrameValueByIndex(master.first,index,ok);
    }

    assert( dimension < getDimension() );
    if ( !getKeyFramesCount(dimension) ) {
        *ok = false;

        return "";
    }

    const AnimatingString_KnobHelper* animatedString = dynamic_cast<const AnimatingString_KnobHelper*>(this);
    assert(animatedString);

    boost::shared_ptr<Curve> curve = getCurve(dimension);
    assert(curve);
    KeyFrame kf;
    *ok =  curve->getKeyFrameWithIndex(index, &kf);
    std::string value;

    if (*ok) {
        animatedString->stringFromInterpolatedValue(kf.getValue(),&value);
    }

    return value;
}

template<typename T>
std::list<T> Knob<T>::getValueForEachDimension_mt_safe() const
{
    QReadLocker l(&_valueMutex);
    std::list<T> ret;

    for (U32 i = 0; i < _values.size(); ++i) {
        ret.push_back(_values[i]);
    }

    return ret;
}

template<typename T>
std::vector<T> Knob<T>::getValueForEachDimension_mt_safe_vector() const
{
    QReadLocker l(&_valueMutex);

    return _values;
}

template<typename T>
std::vector<T>
Knob<T>::getDefaultValues_mt_safe() const
{
    
    QReadLocker l(&_valueMutex);
    
    return _defaultValues;
}

template<typename T>
void
Knob<T>::setDefaultValue(const T & v,
                         int dimension)
{
    assert( dimension < getDimension() );
    {
        QWriteLocker l(&_valueMutex);
        _defaultValues[dimension] = v;
    }
    resetToDefaultValue(dimension);
}

template<typename T>
void
Knob<T>::populate()
{
    for (int i = 0; i < getDimension(); ++i) {
        _values[i] = T();
        _defaultValues[i] = T();
    }
    KnobHelper::populate();
}

template<typename T>
bool
Knob<T>::isTypeCompatible(const boost::shared_ptr<KnobI> & other) const
{
    assert(other);
    bool isPod = false;
    if ( dynamic_cast<const Knob<int>* >(this) || dynamic_cast<const Knob<bool>* >(this) || dynamic_cast<const Knob<double>* >(this) ) {
        isPod = true;
    } else if ( dynamic_cast<const Knob<std::string>* >(this) ) {
        isPod = false;
    } else {
        ///unrecognized type
        assert(false);
    }

    bool otherIsPod = false;
    if ( boost::dynamic_pointer_cast< Knob<int> >(other) || boost::dynamic_pointer_cast< Knob<bool> >(other) ||
         boost::dynamic_pointer_cast< Knob<double> >(other) ) {
        otherIsPod = true;
    } else if ( boost::dynamic_pointer_cast< Knob<std::string> >(other) ) {
        otherIsPod = false;
    } else {
        ///unrecognized type
        assert(false);
    }

    return isPod == otherIsPod;
}

template<typename T>
bool
Knob<T>::onKeyFrameSet(SequenceTime time,
                       int dimension)
{
    KeyFrame k;
    boost::shared_ptr<Curve> curve;
    KnobHolder* holder = getHolder();
    bool useGuiCurve = (!holder || !holder->canSetValue()) && getKnobGuiPointer();
    
    if (!useGuiCurve) {
        curve = getCurve(dimension);
    } else {
        curve = getGuiCurve(dimension);
        setGuiCurveHasChanged(dimension,true);
    }

    makeKeyFrame(curve.get(), time, getValueAtTime(time,dimension), &k);

    bool ret = curve->addKeyFrame(k);
    
    if (!useGuiCurve) {
        guiCurveCloneInternalCurve(dimension);
        evaluateValueChange(dimension, Natron::eValueChangedReasonUserEdited);
    }
    return ret;
}

template<typename T>
bool
Knob<T>::onKeyFrameSet(SequenceTime /*time*/,const KeyFrame& key,int dimension)
{
    boost::shared_ptr<Curve> curve;
    KnobHolder* holder = getHolder();
    bool useGuiCurve = (!holder || !holder->canSetValue()) && getKnobGuiPointer();
    
    if (!useGuiCurve) {
        curve = getCurve(dimension);
    } else {
        curve = getGuiCurve(dimension);
        setGuiCurveHasChanged(dimension,true);
    }
    
    bool ret = curve->addKeyFrame(key);
    
    if (!useGuiCurve) {
        guiCurveCloneInternalCurve(dimension);
        evaluateValueChange(dimension, Natron::eValueChangedReasonUserEdited);
    }
    return ret;
}

template<typename T>
void
Knob<T>::onTimeChanged(SequenceTime /*time*/)
{
    int dims = getDimension();

    if (getIsSecret()) {
        return;
    }
    for (int i = 0; i < dims; ++i) {
        
        if (_signalSlotHandler && isAnimated(i)) {
            _signalSlotHandler->s_valueChanged(i, Natron::eValueChangedReasonTimeChanged);
            _signalSlotHandler->s_updateSlaves(i);
        }
        checkAnimationLevel(i);
    }
}


template<typename T>
double
Knob<T>::getIntegrateFromTimeToTime(double time1,
                                    double time2,
                                    int dimension) const
{
    if ( ( dimension > getDimension() ) || (dimension < 0) ) {
        throw std::invalid_argument("Knob::getDerivativeAtTime(): Dimension out of range");
    }

    ///if the knob is slaved to another knob, returns the other knob value
    std::pair<int,boost::shared_ptr<KnobI> > master = getMaster(dimension);
    if (master.second) {
        Knob<int>* isInt = dynamic_cast<Knob<int>* >( master.second.get() );
        Knob<bool>* isBool = dynamic_cast<Knob<bool>* >( master.second.get() );
        Knob<double>* isDouble = dynamic_cast<Knob<double>* >( master.second.get() );
        assert(isInt || isBool || isDouble); //< other data types aren't supported
        if (isInt) {
            return isInt->getIntegrateFromTimeToTime(time1, time2, master.first);
        } else if (isBool) {
            return isBool->getIntegrateFromTimeToTime(time1, time2, master.first);
        } else if (isDouble) {
            return isDouble->getIntegrateFromTimeToTime(time1, time2, master.first);
        }
    }

    boost::shared_ptr<Curve> curve  = getCurve(dimension);
    if (curve->getKeyFramesCount() > 0) {
        return curve->getIntegrateFromTo(time1, time2);
    } else {
        // if the knob as no keys at this dimension, the integral is trivial
        QReadLocker l(&_valueMutex);

        return (double)_values[dimension] * (time2 - time1);
    }
}

template<>
double
Knob<std::string>::getIntegrateFromTimeToTime(double time1,
                                              double time2,
                                              int dimension) const
{
    if ( ( dimension > getDimension() ) || (dimension < 0) ) {
        throw std::invalid_argument("Knob::getDerivativeAtTime(): Dimension out of range");
    }

    ///if the knob is slaved to another knob, returns the other knob value
    std::pair<int,boost::shared_ptr<KnobI> > master = getMaster(dimension);
    if (master.second) {
        Knob<std::string>* isString = dynamic_cast<Knob<std::string>* >( master.second.get() );
        assert(isString); //< other data types aren't supported
        return isString->getIntegrateFromTimeToTime(time1, time2, master.first);
    }

    boost::shared_ptr<Curve> curve  = getCurve(dimension);
    if (curve->getKeyFramesCount() > 0) {
        return curve->getIntegrateFromTo(time1, time2);
    } else {
        return 0;
    }
}

template<typename T>
void
Knob<T>::resetToDefaultValue(int dimension)
{
    KnobI::removeAnimation(dimension);
    T defaultV;
    {
        QReadLocker l(&_valueMutex);
        defaultV = _defaultValues[dimension];
    }
    (void)setValue(defaultV, dimension,Natron::eValueChangedReasonRestoreDefault,NULL);
    if (_signalSlotHandler) {
        _signalSlotHandler->s_valueChanged(dimension,Natron::eValueChangedReasonRestoreDefault);
    }
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
    KnobI::removeAnimation(dimension);

    ///A Knob<double> is not always a Double_Knob (it can also be a Color_Knob)
    Double_Knob* isDouble = dynamic_cast<Double_Knob*>(this);
    double def;
    
    {
        QReadLocker l(&_valueMutex);
        def = _defaultValues[dimension];
    }

    resetExtraToDefaultValue(dimension);
    
    if ( isDouble && isDouble->areDefaultValuesNormalized() ) {
        assert( getHolder() );
        Natron::EffectInstance* holder = dynamic_cast<Natron::EffectInstance*>( getHolder() );
        assert(holder);
        isDouble->denormalize(dimension, holder->getThreadLocalRenderTime(), &def);
    }
    (void)setValue(def, dimension,Natron::eValueChangedReasonRestoreDefault,NULL);
    if (_signalSlotHandler) {
        _signalSlotHandler->s_valueChanged(dimension,Natron::eValueChangedReasonRestoreDefault);
    }
}

template<>
void
Knob<int>::cloneValues(KnobI* other)
{
    Knob<int>* isInt = dynamic_cast<Knob<int>* >(other);
    Knob<bool>* isBool = dynamic_cast<Knob<bool>* >(other);
    Knob<double>* isDouble = dynamic_cast<Knob<double>* >(other);
    assert(isInt || isBool || isDouble);
    QWriteLocker k(&_valueMutex);
    if (isInt) {
        _values = isInt->getValueForEachDimension_mt_safe_vector();
    } else if (isBool) {
        std::vector<bool> v = isBool->getValueForEachDimension_mt_safe_vector();
        assert( v.size() == _values.size() );
        for (U32 i = 0; i < v.size(); ++i) {
            _values[i] = v[i];
        }
    } else {
        std::vector<double> v = isDouble->getValueForEachDimension_mt_safe_vector();
        assert( v.size() == _values.size() );
        for (U32 i = 0; i < v.size(); ++i) {
            _values[i] = v[i];
        }
    }
}

template<>
void
Knob<bool>::cloneValues(KnobI* other)
{
    Knob<int>* isInt = dynamic_cast<Knob<int>* >(other);
    Knob<bool>* isBool = dynamic_cast<Knob<bool>* >(other);
    Knob<double>* isDouble = dynamic_cast<Knob<double>* >(other);
    assert(isInt || isBool || isDouble);
    QWriteLocker k(&_valueMutex);
    if (isInt) {
        std::vector<int> v = isInt->getValueForEachDimension_mt_safe_vector();
        assert( v.size() == _values.size() );
        for (U32 i = 0; i < v.size(); ++i) {
            _values[i] = v[i];
        }
    } else if (isBool) {
        _values = isBool->getValueForEachDimension_mt_safe_vector();
    } else {
        std::vector<double> v = isDouble->getValueForEachDimension_mt_safe_vector();
        assert( v.size() == _values.size() );
        for (U32 i = 0; i < v.size(); ++i) {
            _values[i] = v[i];
        }
    }
}

template<>
void
Knob<double>::cloneValues(KnobI* other)
{
    Knob<int>* isInt = dynamic_cast<Knob<int>* >(other);
    Knob<bool>* isBool = dynamic_cast<Knob<bool>* >(other);
    Knob<double>* isDouble = dynamic_cast<Knob<double>* >(other);
    
    ///can only clone pod
    assert(isInt || isBool || isDouble);
    QWriteLocker k(&_valueMutex);
    if (isInt) {
        std::vector<int> v = isInt->getValueForEachDimension_mt_safe_vector();
        //std::vector<int> defaultV = isInt->getDefaultValues_mt_safe();
        //assert(defaultV.size() == v.size());
        assert( v.size() == _values.size() );
        for (U32 i = 0; i < v.size(); ++i) {
            _values[i] = v[i];
            //_defaultValues[i] = defaultV[i];
        }
    } else if (isBool) {
        std::vector<bool> v = isBool->getValueForEachDimension_mt_safe_vector();
        //std::vector<bool> defaultV = isBool->getDefaultValues_mt_safe();
        //assert(defaultV.size() == v.size());
        assert( v.size() == _values.size() );
        for (U32 i = 0; i < v.size(); ++i) {
            _values[i] = v[i];
            //_defaultValues[i] = defaultV[i];
        }
    } else {
        _values = isDouble->getValueForEachDimension_mt_safe_vector();
        //_defaultValues = isDouble->getDefaultValues_mt_safe();
    }
}

template<>
void
Knob<std::string>::cloneValues(KnobI* other)
{
    Knob<std::string>* isString = dynamic_cast<Knob<std::string>* >(other);
    
    ///Can only clone strings
    assert(isString);
    QWriteLocker k(&_valueMutex);
    std::vector<std::string> v = isString->getValueForEachDimension_mt_safe_vector();
    //std::vector<std::string> defaultV = isString->getDefaultValues_mt_safe();
    //assert(defaultV.size() == v.size());
    for (U32 i = 0; i < v.size(); ++i) {
        _values[i] = v[i];
        //_defaultValues[i] = defaultV[i];
    }
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
    cloneValues(other);
    for (int i = 0; i < dimMin; ++i) {
        if (i == dimension || dimension == -1) {
            getCurve(i,true)->clone( *other->getCurve(i,true) );
            boost::shared_ptr<Curve> guiCurve = getGuiCurve(i);
            boost::shared_ptr<Curve> otherGuiCurve = other->getGuiCurve(i);
            if (guiCurve && otherGuiCurve) {
                guiCurve->clone(*otherGuiCurve);
            }
            checkAnimationLevel(i);
            if (_signalSlotHandler) {
                _signalSlotHandler->s_valueChanged(i,Natron::eValueChangedReasonPluginEdited);
            }
        }
    }
    cloneExtraData(other,dimension);
}

template<typename T>
void
Knob<T>::clone(KnobI* other,
               SequenceTime offset,
               const RangeD* range,
               int dimension)
{
    if (other == this) {
        return;
    }
    cloneValues(other);
    int dimMin = std::min( getDimension(), other->getDimension() );
    for (int i = 0; i < dimMin; ++i) {
        if (dimension == -1 || i == dimension) {
            getCurve(i,true)->clone(*other->getCurve(i,true), offset, range);
            boost::shared_ptr<Curve> guiCurve = getGuiCurve(i);
            boost::shared_ptr<Curve> otherGuiCurve = other->getGuiCurve(i);
            if (guiCurve && otherGuiCurve) {
                guiCurve->clone(*otherGuiCurve,offset,range);
            }
            checkAnimationLevel(i);
            if (_signalSlotHandler) {
                _signalSlotHandler->s_valueChanged(i,Natron::eValueChangedReasonPluginEdited);
            }
        }
    }
    cloneExtraData(other,offset,range,dimension);
}

template<typename T>
void
Knob<T>::cloneAndUpdateGui(KnobI* other,int dimension)
{
    if (other == this) {
        return;
    }
    int dimMin = std::min( getDimension(), other->getDimension() );
    cloneValues(other);
    for (int i = 0; i < dimMin; ++i) {
        if (dimension == -1 || i == dimension) {
            if (_signalSlotHandler) {
                int nKeys = getKeyFramesCount(i);
                for (int k = 0; k < nKeys; ++k) {
                    double time;
                    bool ok = getKeyFrameTime(k, i, &time);
                    assert(ok);
                    if (ok) {
                        _signalSlotHandler->s_keyFrameRemoved(time, i,(int)Natron::eValueChangedReasonNatronInternalEdited);
                    }
                }
            }
            getCurve(i,true)->clone( *other->getCurve(i,true) );
            boost::shared_ptr<Curve> guiCurve = getGuiCurve(i);
            boost::shared_ptr<Curve> otherGuiCurve = other->getGuiCurve(i);
            if (guiCurve && otherGuiCurve) {
                guiCurve->clone(*otherGuiCurve);
            }
            if (_signalSlotHandler) {
                int nKeys = getKeyFramesCount(i);
                for (int k = 0; k < nKeys; ++k) {
                    double time;
                    bool ok = getKeyFrameTime(k, i, &time);
                    assert(ok);
                    if (ok) {
                        _signalSlotHandler->s_keyFrameSet(time, i,(int)Natron::eValueChangedReasonNatronInternalEdited,true);
                    }
                }
                _signalSlotHandler->s_valueChanged(i,Natron::eValueChangedReasonPluginEdited);
            }
            checkAnimationLevel(i);
        }
    }
    cloneExtraData(other,dimension);
}

template <typename T>
void
Knob<T>::deepClone(KnobI* other)
{
    cloneAndUpdateGui(other);
    for (int i = 0; i < getDimension(); ++i) {
        if (!other->isEnabled(i)) {
            setEnabled(i,false);
        }
    }
    
    if (other->getIsSecret()) {
        setSecret(true);
    }
    
    setName(other->getName());
    deepCloneExtraData(other);

}

template <typename T>
void
Knob<T>::dequeueValuesSet(bool disableEvaluation)
{
    
    std::set<int> dimensionChanged;
    
    cloneGuiCurvesIfNeeded(dimensionChanged);
    {
        QMutexLocker kql(&_setValuesQueueMutex);
   
        
        QWriteLocker k(&_valueMutex);
        for (typename std::list<boost::shared_ptr<QueuedSetValue> >::iterator it = _setValuesQueue.begin(); it!=_setValuesQueue.end(); ++it) {
           
            QueuedSetValueAtTime* isAtTime = dynamic_cast<QueuedSetValueAtTime*>(it->get());
           
            dimensionChanged.insert((*it)->_imp->dimension);
            
            if (!isAtTime) {
                
                if ((*it)->_imp->useKey) {
                    boost::shared_ptr<Curve> curve = getCurve((*it)->_imp->dimension);
                    curve->addKeyFrame((*it)->_imp->key);
                } else {
                    _values[(*it)->_imp->dimension] = (*it)->_imp->value;
                }
            } else {
                boost::shared_ptr<Curve> curve = getCurve((*it)->_imp->dimension);
                curve->addKeyFrame((*it)->_imp->key);
            }
        }
        _setValuesQueue.clear();
    }
    cloneInternalCurvesIfNeeded(dimensionChanged);

    if (!disableEvaluation && !dimensionChanged.empty()) {
        
        blockEvaluation();
        std::set<int>::iterator next = dimensionChanged.begin();
        if (!dimensionChanged.empty()) {
            ++next;
        }
        for (std::set<int>::iterator it = dimensionChanged.begin();it!=dimensionChanged.end();++it) {
            
            if (next == dimensionChanged.end()) {
                unblockEvaluation();
            }
            
            evaluateValueChange(*it, Natron::eValueChangedReasonNatronInternalEdited);
            
            if (next != dimensionChanged.end()) {
                ++next;
            }
        }
        
    }
    
}

#endif // KNOBIMPL_H
