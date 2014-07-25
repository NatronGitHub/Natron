//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#ifndef KNOBIMPL_H
#define KNOBIMPL_H

#include "Knob.h"

#include <stdexcept>
#include <string>
#include <QString>
#include "Engine/Curve.h"
#include "Engine/AppInstance.h"
#include "Engine/Project.h"
#include "Engine/TimeLine.h"
#include "Engine/EffectInstance.h"

///template specializations

template <typename T>
Knob<T>::Knob(KnobHolder*  holder,const std::string& description,int dimension,bool declaredByPlugin )
    : KnobHelper(holder,description,dimension,declaredByPlugin)
    , _valueMutex(QReadWriteLock::Recursive)
    , _values(dimension)
    , _defaultValues(dimension)
    , _setValueRecursionLevel(false)
    , _setValueRecursionLevelMutex(QMutex::Recursive)
{
    
}

//Declare the specialization before defining it to avoid the following
//error: explicit specialization of 'getValueAtTime' after instantiation
template<>
std::string Knob<std::string>::getValueAtTime(double time, int dimension) const;

template<>
std::string Knob<std::string>::getValue(int dimension) const
{
    if (isAnimated(dimension)) {
        SequenceTime time;
        if (!getHolder() || !getHolder()->getApp()) {
            time = 0;
        } else {
            Natron::EffectInstance* isEffect = dynamic_cast<Natron::EffectInstance*>(getHolder());
            if (isEffect) {
                time = isEffect->getCurrentFrameRecursive();
            } else {
                time = getHolder()->getApp()->getTimeLine()->currentFrame();
            }
        }
        return getValueAtTime(time, dimension);
    }
    
    if (dimension > (int)_values.size() || dimension < 0) {
        throw std::invalid_argument("Knob::getValue(): Dimension out of range");
    }
    ///if the knob is slaved to another knob, returns the other knob value
    std::pair<int,boost::shared_ptr<KnobI> > master = getMaster(dimension);
    if (master.second) {
        Knob<std::string>* isString = dynamic_cast<Knob<std::string>* >(master.second.get());
        assert(isString); //< other data types aren't supported
        return isString->getValue(master.first);
    }
    
    QReadLocker l(&_valueMutex);
    return _values[dimension];
    
}

template <typename T>
T Knob<T>::getValue(int dimension) const
{
    if (isAnimated(dimension)) {
        SequenceTime time;
        if (!getHolder() || !getHolder()->getApp()) {
            time = 0;
        } else {
            Natron::EffectInstance* isEffect = dynamic_cast<Natron::EffectInstance*>(getHolder());
            if (isEffect) {
                time = isEffect->getCurrentFrameRecursive();
            } else {
                time = getHolder()->getApp()->getTimeLine()->currentFrame();
            }
        }
        return getValueAtTime(time, dimension);
    }

    if (dimension > (int)_values.size() || dimension < 0) {
        throw std::invalid_argument("Knob::getValue(): Dimension out of range");
    }
    ///if the knob is slaved to another knob, returns the other knob value
    std::pair<int,boost::shared_ptr<KnobI> > master = getMaster(dimension);
    if (master.second) {
        Knob<int>* isInt = dynamic_cast<Knob<int>* >(master.second.get());
        Knob<bool>* isBool = dynamic_cast<Knob<bool>* >(master.second.get());
        Knob<double>* isDouble = dynamic_cast<Knob<double>* >(master.second.get());
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
    return _values[dimension];
}



template<>
std::string Knob<std::string>::getValueAtTime(double time, int dimension) const
{
    if (dimension > getDimension() || dimension < 0) {
        throw std::invalid_argument("Knob::getValueAtTime(): Dimension out of range");
    }
    
    
    ///if the knob is slaved to another knob, returns the other knob value
    std::pair<int,boost::shared_ptr<KnobI> > master = getMaster(dimension);
    if (master.second) {
        Knob<std::string>* isString = dynamic_cast<Knob<std::string>* >(master.second.get());
        assert(isString); //< other data types aren't supported
        return isString->getValueAtTime(time,master.first);
        
    }
    std::string ret;
    const AnimatingString_KnobHelper* isStringAnimated = dynamic_cast<const AnimatingString_KnobHelper* >(this);
    if (isStringAnimated) {
        ret = isStringAnimated->getStringAtTime(time,dimension);
        ///ret is not empty if the animated string knob has a custom interpolation
        if (!ret.empty()) {
            return ret;
        }
    }
    assert(ret.empty());
    
    boost::shared_ptr<Curve> curve  = getCurve(dimension);
    if (curve->getKeyFramesCount() > 0) {
        assert(isStringAnimated);
        isStringAnimated->stringFromInterpolatedValue(curve->getValueAt(time), &ret);
        return ret;
    } else {
        /*if the knob as no keys at this dimension, return the value
         at the requested dimension.*/
        QReadLocker l(&_valueMutex);
        return _values[dimension];
    }
    
    
}


template<typename T>
T Knob<T>::getValueAtTime(double time, int dimension ) const
{
    if (dimension > getDimension() || dimension < 0) {
        throw std::invalid_argument("Knob::getValueAtTime(): Dimension out of range");
    }


    ///if the knob is slaved to another knob, returns the other knob value
    std::pair<int,boost::shared_ptr<KnobI> > master = getMaster(dimension);
    if (master.second) {
        Knob<int>* isInt = dynamic_cast<Knob<int>* >(master.second.get());
        Knob<bool>* isBool = dynamic_cast<Knob<bool>* >(master.second.get());
        Knob<double>* isDouble = dynamic_cast<Knob<double>* >(master.second.get());
        assert(isInt || isBool || isDouble); //< other data types aren't supported
        if (isInt) {
            return isInt->getValueAtTime(time,master.first);
        } else if (isBool) {
            return isBool->getValueAtTime(time,master.first);
        } else if (isDouble) {
            return isDouble->getValueAtTime(time,master.first);
        }
    }
    boost::shared_ptr<Curve> curve  = getCurve(dimension);
    if (curve->getKeyFramesCount() > 0) {
        return curve->getValueAt(time);
    } else {
        /*if the knob as no keys at this dimension, return the value
     at the requested dimension.*/
        QReadLocker l(&_valueMutex);
        return _values[dimension];
    }

}

template <typename T>
void Knob<T>::valueToVariant(const T& v,Variant* vari)
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
void Knob<std::string>::valueToVariant(const std::string& v,Variant* vari)
{
    Knob<std::string>* isString = dynamic_cast<Knob<std::string>*>(this);
    if (isString) {
        vari->setValue<QString>(v.c_str());
    }
}

template <typename T>
KnobHelper::ValueChangedReturnCode Knob<T>::setValue(const T& v,int dimension,Natron::ValueChangedReason reason,
                                                     KeyFrame* newKey,bool triggerKnobChanged)
{
    if (0 > dimension || dimension > (int)_values.size()) {
        throw std::invalid_argument("Knob::setValue(): Dimension out of range");
    }

    KnobHelper::ValueChangedReturnCode  ret = NO_KEYFRAME_ADDED;
    
    Natron::EffectInstance* holder = dynamic_cast<Natron::EffectInstance*>(getHolder());
    if (holder && reason == Natron::PLUGIN_EDITED && getKnobGuiPointer()) {
        KnobHolder::MultipleParamsEditLevel paramEditLevel = holder->getMultipleParamsEditLevel();
        switch (paramEditLevel) {
            case KnobHolder::PARAM_EDIT_OFF:
            default:
                break;
            case KnobHolder::PARAM_EDIT_ON_CREATE_NEW_COMMAND:
            {
                if (!get_SetValueRecursionLevel()) {
                    Variant vari;
                    valueToVariant(v, &vari);
                    holder->setMultipleParamsEditLevel(KnobHolder::PARAM_EDIT_ON);
                    _signalSlotHandler->s_appendParamEditChange(vari, dimension, 0, true,false,triggerKnobChanged);
                    return ret;
                }
            }     break;
            case KnobHolder::PARAM_EDIT_ON:
            {
                if (!get_SetValueRecursionLevel()) {
                    Variant vari;
                    valueToVariant(v, &vari);
                    _signalSlotHandler->s_appendParamEditChange(vari, dimension,0, false,false,triggerKnobChanged);
                    return ret;
                }
            }   break;
        }
        
        ///basically if we enter this if condition, for each dimension the undo stack will create a new command.
        ///the caller should have tested this prior to calling this function and correctly called editBegin() editEnd()
        ///to bracket the setValue()  calls within the same undo/redo command.
        if (holder->isDoingInteractAction()) {
            if (!get_SetValueRecursionLevel()) {
                Variant vari;
                valueToVariant(v, &vari);
                _signalSlotHandler->s_setValueWithUndoStack(vari, dimension);
                return ret;
                
            }
        }
    }
    
    {
        QMutexLocker l(&_setValueRecursionLevelMutex);
        ++_setValueRecursionLevel;
    }

    ///if the knob is slaved to another knob,return, because we don't want the
    ///gui to be unsynchronized with what lies internally.
    // if (!isSlave(dimension)) {

        {
            QWriteLocker l(&_valueMutex);
            _values[dimension] = v;
        }

        ///Add automatically a new keyframe
        if (getAnimationLevel(dimension) != Natron::NO_ANIMATION && //< if the knob is animated
            getHolder() &&
                getHolder()->getApp() && //< the app pointer is not NULL
                !getHolder()->getApp()->getProject()->isLoadingProject() && //< we're not loading the project
                (reason == Natron::USER_EDITED || reason == Natron::PLUGIN_EDITED) && //< the change was made by the user or plugin
                newKey != NULL) { //< the keyframe to set is not null

            SequenceTime time;
            Natron::EffectInstance* isEffect = dynamic_cast<Natron::EffectInstance*>(getHolder());
            if (isEffect) {
                time = isEffect->getCurrentFrameRecursive();
            } else {
                time = getHolder()->getApp()->getTimeLine()->currentFrame();
            }
            bool addedKeyFrame = setValueAtTime(time, v, dimension,reason,newKey,triggerKnobChanged);
            if (addedKeyFrame) {
                ret = KEYFRAME_ADDED;
            } else {
                ret = KEYFRAME_MODIFIED;
            }

        }
    // }
    if (ret == NO_KEYFRAME_ADDED && triggerKnobChanged) { //the other cases already called this in setValueAtTime()
        evaluateValueChange(dimension,reason);
    }
    {
        QMutexLocker l(&_setValueRecursionLevelMutex);
        --_setValueRecursionLevel;
        assert(_setValueRecursionLevel >= 0); 
    }

    
    return ret;

}

template<typename T>
bool Knob<T>::setValueAtTime(int time,const T& v,int dimension,Natron::ValueChangedReason reason,KeyFrame* newKey,
                             bool triggerOnKnobChanged)
{
    if (dimension > getDimension() || dimension < 0) {
        throw std::invalid_argument("Knob::setValueAtTime(): Dimension out of range");
    }

    Natron::EffectInstance* holder = dynamic_cast<Natron::EffectInstance*>(getHolder());
    if (holder && reason == Natron::PLUGIN_EDITED && getKnobGuiPointer()) {
        KnobHolder::MultipleParamsEditLevel paramEditLevel = holder->getMultipleParamsEditLevel();
        switch (paramEditLevel) {
            case KnobHolder::PARAM_EDIT_OFF:
            default:
                break;
            case KnobHolder::PARAM_EDIT_ON_CREATE_NEW_COMMAND:
            {
                if (!get_SetValueRecursionLevel()) {
                    Variant vari;
                    valueToVariant(v, &vari);
                    holder->setMultipleParamsEditLevel(KnobHolder::PARAM_EDIT_ON);
                    _signalSlotHandler->s_appendParamEditChange(vari, dimension, time, true,true,triggerOnKnobChanged);
                    return true;
                }
            }     break;
            case KnobHolder::PARAM_EDIT_ON:
            {
                if (!get_SetValueRecursionLevel()) {
                    Variant vari;
                    valueToVariant(v, &vari);
                    _signalSlotHandler->s_appendParamEditChange(vari, dimension,time, false,true,triggerOnKnobChanged);
                    return true;
                }
            }   break;
        }

    }
    
    ///if the knob is slaved to another knob,return, because we don't want the
    ///gui to be unsynchronized with what lies internally.
    if (isSlave(dimension)) {
        return false;
    }

    boost::shared_ptr<Curve> curve = getCurve(dimension);
    double keyFrameValue;

    if (curve->areKeyFramesValuesClampedToIntegers()) {
        keyFrameValue = std::floor(v + 0.5);
    } else if (curve->areKeyFramesValuesClampedToBooleans()) {
        keyFrameValue = (bool)v;
    } else {
        keyFrameValue = (double)v;
    }

    *newKey = KeyFrame((double)time,keyFrameValue);

    bool ret = curve->addKeyFrame(*newKey);
    if (reason == Natron::PLUGIN_EDITED) {
        (void)setValue(v, dimension,reason,NULL,true);
    }

    if (_signalSlotHandler && ret) {
        if (reason != Natron::USER_EDITED) {
            _signalSlotHandler->s_keyFrameSet(time,dimension);
        }
        _signalSlotHandler->s_updateSlaves(dimension);
    }
    checkAnimationLevel(dimension);
    return ret;

}

template<>
bool Knob<std::string>::setValueAtTime(int time,const std::string& v,int dimension,Natron::ValueChangedReason reason,KeyFrame* newKey,
                                       bool triggerOnKnobChanged)
{
    if (dimension > getDimension() || dimension < 0) {
        throw std::invalid_argument("Knob::setValueAtTime(): Dimension out of range");
    }

    Natron::EffectInstance* holder = dynamic_cast<Natron::EffectInstance*>(getHolder());
    if (holder && reason == Natron::PLUGIN_EDITED && getKnobGuiPointer()) {
        KnobHolder::MultipleParamsEditLevel paramEditLevel = holder->getMultipleParamsEditLevel();
        switch (paramEditLevel) {
            case KnobHolder::PARAM_EDIT_OFF:
            default:
                break;
            case KnobHolder::PARAM_EDIT_ON_CREATE_NEW_COMMAND:
            {
                if (!get_SetValueRecursionLevel()) {
                    Variant vari;
                    valueToVariant(v, &vari);
                    holder->setMultipleParamsEditLevel(KnobHolder::PARAM_EDIT_ON);
                    _signalSlotHandler->s_appendParamEditChange(vari, dimension, time, true,true,triggerOnKnobChanged);
                    return true;
                }
            }     break;
            case KnobHolder::PARAM_EDIT_ON:
            {
                if (!get_SetValueRecursionLevel()) {
                    Variant vari;
                    valueToVariant(v, &vari);
                    _signalSlotHandler->s_appendParamEditChange(vari, dimension,time, false,true,triggerOnKnobChanged);
                    return true;
                }
            }   break;
        }
        
    }

    
    ///if the knob is slaved to another knob,return, because we don't want the
    ///gui to be unsynchronized with what lies internally.
    if (isSlave(dimension)) {
        return false;
    }

    boost::shared_ptr<Curve> curve = getCurve(dimension);
    double keyFrameValue;
    AnimatingString_KnobHelper* isStringAnimatedKnob = dynamic_cast<AnimatingString_KnobHelper*>(this);
    assert(isStringAnimatedKnob);
    isStringAnimatedKnob->stringToKeyFrameValue(time,v,&keyFrameValue);

    *newKey = KeyFrame((double)time,keyFrameValue);

    bool ret = curve->addKeyFrame(*newKey);
    if (reason == Natron::PLUGIN_EDITED) {
        (void)setValue(v, dimension,reason,NULL,true);
    }

    if (reason != Natron::USER_EDITED && ret) {
        _signalSlotHandler->s_keyFrameSet(time,dimension);
    }

    _signalSlotHandler->s_updateSlaves(dimension);
    return ret;

}

template<typename T>
void Knob<T>::unSlave(int dimension,Natron::ValueChangedReason reason,bool copyState)
{
    assert(isSlave(dimension));
    std::pair<int,boost::shared_ptr<KnobI> > master = getMaster(dimension);

    boost::shared_ptr<KnobHelper> helper = boost::dynamic_pointer_cast<KnobHelper>(master.second);
    
    if (helper->getSignalSlotHandler() && _signalSlotHandler) {
        QObject::disconnect(helper->getSignalSlotHandler().get(), SIGNAL(updateSlaves(int)), _signalSlotHandler.get(),
                            SLOT(onMasterChanged(int)));
    }
    
    resetMaster(dimension);
    if (copyState) {
        ///clone the master
        clone(master.second);
    }

    if (_signalSlotHandler) {
        //_signalSlotHandler->s_valueChanged(dimension);
        if (reason == Natron::PLUGIN_EDITED) {
            _signalSlotHandler->s_knobSlaved(dimension, false);
            checkAnimationLevel(dimension);
        }
    }
    
}

template<>
void Knob<std::string>::unSlave(int dimension,Natron::ValueChangedReason reason,bool copyState)
{
    assert(isSlave(dimension));
    
    std::pair<int,boost::shared_ptr<KnobI> > master = getMaster(dimension);

    if (copyState) {
        ///clone the master
        {
            Knob<std::string>* isString = dynamic_cast<Knob<std::string>* >(master.second.get());
            assert(isString); //< other data types aren't supported
            QWriteLocker l1(&_valueMutex);
            _values[dimension] =  isString->getValue(master.first);
            
        }
        getCurve(dimension)->clone(*(master.second->getCurve(master.first)));
        
        cloneExtraData(master.second);
    }
    boost::shared_ptr<KnobHelper> helper = boost::dynamic_pointer_cast<KnobHelper>(master.second);
    QObject::disconnect(helper->getSignalSlotHandler().get(), SIGNAL(updateSlaves(int)), _signalSlotHandler.get(),
                        SLOT(onMasterChanged(int)));
    resetMaster(dimension);
    
    _signalSlotHandler->s_valueChanged(dimension);
    if (reason == Natron::PLUGIN_EDITED) {
        _signalSlotHandler->s_knobSlaved(dimension, false);
    }
}

template<typename T>
void Knob<T>::setValue(const T& value,int dimension,bool turnOffAutoKeying,bool triggerOnKnobChanged)
{
    if (turnOffAutoKeying) {
        (void)setValue(value,dimension,Natron::PLUGIN_EDITED,NULL,triggerOnKnobChanged);
    } else {
        KeyFrame k;
        (void)setValue(value,dimension,Natron::PLUGIN_EDITED,&k,triggerOnKnobChanged);
    }
}

template<typename T>
KnobHelper::ValueChangedReturnCode Knob<T>::onValueChanged(int dimension,const T& v,KeyFrame* newKey,bool triggerKnobChanged)
{
    return setValue(v, dimension,Natron::USER_EDITED,newKey,triggerKnobChanged);
}

template<typename T>
void Knob<T>::setValueAtTime(int time,const T& v,int dimension,bool triggerOnKnobChanged)
{
    KeyFrame k;
    (void)setValueAtTime(time,v,dimension,Natron::PLUGIN_EDITED,&k,triggerOnKnobChanged);
}

template<typename T>
T Knob<T>::getKeyFrameValueByIndex(int dimension,int index,bool* ok) const
{
    ///if the knob is slaved to another knob, returns the other knob value
    std::pair<int,boost::shared_ptr<KnobI> > master = getMaster(dimension);
    if (master.second) {
        Knob<int>* isInt = dynamic_cast<Knob<int>* >(master.second.get());
        Knob<bool>* isBool = dynamic_cast<Knob<bool>* >(master.second.get());
        Knob<double>* isDouble = dynamic_cast<Knob<double>* >(master.second.get());
        assert(isInt || isBool || isDouble); //< other data types aren't supported
        if (isInt) {
            return isInt->getKeyFrameValueByIndex(master.first,index,ok);
        } else if (isBool) {
            return isBool->getKeyFrameValueByIndex(master.first,index,ok);
        } else if (isDouble) {
            return isDouble->getKeyFrameValueByIndex(master.first,index,ok);
        }
    }

    assert(dimension < getDimension());
    if (!getKeyFramesCount(dimension)) {
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
std::string Knob<std::string>::getKeyFrameValueByIndex(int dimension,int index,bool* ok) const
{
    ///if the knob is slaved to another knob, returns the other knob value
    std::pair<int,boost::shared_ptr<KnobI> > master = getMaster(dimension);
    if (master.second) {
        Knob<std::string>* isString = dynamic_cast<Knob<std::string>* >(master.second.get());
        assert(isString); //< other data types aren't supported
        return isString->getKeyFrameValueByIndex(master.first,index,ok);

    }

    assert(dimension < getDimension());
    if (!getKeyFramesCount(dimension)) {
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
const std::vector<T>& Knob<T>::getValueForEachDimension() const
{
    return _values;
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
void Knob<T>::setDefaultValue(const T& v,int dimension)
{
    assert(dimension < getDimension());
    _defaultValues[dimension] = v;
    _values[dimension] = v;
    processNewValue(Natron::PLUGIN_EDITED);
}

template<typename T>
void Knob<T>::populate()
{
    for (int i = 0; i < getDimension() ; ++i) {
        _values[i] = T();
        _defaultValues[i] = T();
    }
    KnobHelper::populate();
}


template<typename T>
bool Knob<T>::isTypeCompatible(const boost::shared_ptr<KnobI>& other) const
{
    assert(other);
    bool isPod = false;
    if (dynamic_cast<const Knob<int>* >(this) || dynamic_cast<const Knob<bool>* >(this) || dynamic_cast<const Knob<double>* >(this)) {
        isPod = true;
    } else if (dynamic_cast<const Knob<std::string>* >(this)) {
        isPod = false;
    } else {
        ///unrecognized type
        assert(false);
    }

    bool otherIsPod = false;
    if (boost::dynamic_pointer_cast< Knob<int> >(other) || boost::dynamic_pointer_cast< Knob<bool> >(other) ||
            boost::dynamic_pointer_cast< Knob<double> >(other)) {
        otherIsPod = true;
    } else if (boost::dynamic_pointer_cast< Knob<std::string> >(other)) {
        otherIsPod = false;
    } else {
        ///unrecognized type
        assert(false);
    }

    return isPod == otherIsPod;
}

template<typename T>
void Knob<T>::onKeyFrameSet(SequenceTime time,int dimension)
{
    KeyFrame k;
    (void)setValueAtTime(time,getValue(dimension),dimension,Natron::USER_EDITED,&k,true);
}

template<typename T>
void Knob<T>::onTimeChanged(SequenceTime time)
{
    //setValue's calls compression is taken care of above.
    for (int i = 0; i < getDimension(); ++i) {
        boost::shared_ptr<Curve> c = getCurve(i);
        if (c->getKeyFramesCount() > 0 && !getIsSecret()) {
            T v = getValueAtTime(time,i);
            (void)setValue(v,i,Natron::TIME_CHANGED,NULL,true);
        }
        checkAnimationLevel(i);
    }
}

template<typename T>
void Knob<T>::evaluateAnimationChange()
{
    //the holder cannot be a global holder(i.e: it cannot be tied application wide, e.g like Settings)
    SequenceTime time;
    if (getHolder() && getHolder()->getApp()) {
        Natron::EffectInstance* isEffect = dynamic_cast<Natron::EffectInstance*>(getHolder());
        if (isEffect) {
            time = isEffect->getCurrentFrameRecursive();
        } else {
            time = getHolder()->getApp()->getTimeLine()->currentFrame();
        }
    } else {
        time = 0;
    }

    beginValueChange(Natron::PLUGIN_EDITED);
    bool hasEvaluatedOnce = false;
    for (int i = 0; i < getDimension();++i) {
        if (isAnimated(i)) {
            T v = getValueAtTime(time,i);
            (void)setValue(v,i,Natron::PLUGIN_EDITED,NULL,true);
            hasEvaluatedOnce = true;
        }
    }
    if (!hasEvaluatedOnce) {
        evaluateValueChange(0, Natron::PLUGIN_EDITED);
    }

    endValueChange();
}

template<typename T>
double Knob<T>::getIntegrateFromTimeToTime(double time1, double time2, int dimension) const
{
    if (dimension > getDimension() || dimension < 0) {
        throw std::invalid_argument("Knob::getDerivativeAtTime(): Dimension out of range");
    }

    ///if the knob is slaved to another knob, returns the other knob value
    std::pair<int,boost::shared_ptr<KnobI> > master = getMaster(dimension);
    if (master.second) {
        Knob<int>* isInt = dynamic_cast<Knob<int>* >(master.second.get());
        Knob<bool>* isBool = dynamic_cast<Knob<bool>* >(master.second.get());
        Knob<double>* isDouble = dynamic_cast<Knob<double>* >(master.second.get());
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
double Knob<std::string>::getIntegrateFromTimeToTime(double time1, double time2, int dimension) const
{
    if (dimension > getDimension() || dimension < 0) {
        throw std::invalid_argument("Knob::getDerivativeAtTime(): Dimension out of range");
    }

    ///if the knob is slaved to another knob, returns the other knob value
    std::pair<int,boost::shared_ptr<KnobI> > master = getMaster(dimension);
    if (master.second) {
        Knob<std::string>* isString = dynamic_cast<Knob<std::string>* >(master.second.get());
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
void Knob<T>::resetToDefaultValue(int dimension)
{
    KnobI::removeAnimation(dimension);
    (void)setValue(_defaultValues[dimension], dimension,Natron::PROJECT_LOADING,NULL,false);
    if (_signalSlotHandler) {
        _signalSlotHandler->s_valueChanged(dimension);
    }
}


template<>
void Knob<int>::cloneValues(const boost::shared_ptr<KnobI>& other)
{
    Knob<int>* isInt = dynamic_cast<Knob<int>* >(other.get());
    assert(isInt);
    _values = isInt->_values;
}
template<>
void Knob<bool>::cloneValues(const boost::shared_ptr<KnobI>& other)
{
    Knob<bool>* isBool = dynamic_cast<Knob<bool>* >(other.get());
    assert(isBool),
    _values = isBool->_values;
}
template<>
void Knob<double>::cloneValues(const boost::shared_ptr<KnobI>& other)
{
    Knob<double>* isDouble = dynamic_cast<Knob<double>* >(other.get());
    assert(isDouble);
    _values = isDouble->_values;
}
template<>
void Knob<std::string>::cloneValues(const boost::shared_ptr<KnobI>& other)
{
    Knob<std::string>* isString = dynamic_cast<Knob<std::string>* >(other.get());
    assert(isString);
    _values = isString->_values;
}

template<typename T>
void Knob<T>::clone(const boost::shared_ptr<KnobI>& other)
{
    int dimMin = std::min(getDimension() , other->getDimension());
    cloneValues(other);
    for (int i = 0; i < dimMin;++i) {
        getCurve(i)->clone(*other->getCurve(i));
        if (_signalSlotHandler) {
            _signalSlotHandler->s_valueChanged(i);
        }
        setEnabled(i, other->isEnabled(i));
    }
    cloneExtraData(other);
    setSecret(other->getIsSecret());
}

template<typename T>
void Knob<T>::clone(const boost::shared_ptr<KnobI>& other, SequenceTime offset, const RangeD* range)
{
    cloneValues(other);
    int dimMin = std::min(getDimension() , other->getDimension());
    for (int i = 0; i < dimMin; ++i) {
        getCurve(i)->clone(*other->getCurve(i), offset, range);
        if (_signalSlotHandler) {
            _signalSlotHandler->s_valueChanged(i);
        }
    }
    cloneExtraData(other,offset,range);
}

#endif // KNOBIMPL_H
