//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#include "ParameterWrapper.h"

#include "Engine/KnobTypes.h"

Param::Param(const boost::shared_ptr<KnobI>& knob)
: _knob(knob)
{
    
}

Param::~Param()
{
    
}

int
Param::getNumDimensions() const
{
    return _knob->getDimension();
}

std::string
Param::getScriptName() const
{
    return _knob->getName();
}

std::string
Param::getLabel() const
{
    return _knob->getDescription();
}

std::string
Param::getTypeName() const
{
    return _knob->typeName();
}

std::string
Param::getHelp() const
{
    return _knob->getHintToolTip();
}

bool
Param::getIsVisible() const
{
    return !_knob->getIsSecret();
}

void
Param::setVisible(bool visible)
{
    _knob->setSecret(!visible);
}

bool
Param::getIsEnabled(int dimension) const
{
    return _knob->isEnabled(dimension);
}

void
Param::setEnabled(bool enabled,int dimension)
{
    _knob->setEnabled(dimension, enabled);
}

bool
Param::getIsPersistant() const
{
    return _knob->getIsPersistant();
}

void
Param::setPersistant(bool persistant)
{
    _knob->setIsPersistant(persistant);
}

bool
Param::getEvaluateOnChange() const
{
    return _knob->getEvaluateOnChange();
}

void
Param::setEvaluateOnChange(bool eval)
{
    _knob->setEvaluateOnChange(eval);
}

bool
Param::getCanAnimate() const
{
    return _knob->canAnimate();
}

bool
Param::getIsAnimationEnabled() const
{
    return _knob->isAnimationEnabled();
}

bool
Param::getIsAnimated(int dimension) const
{
    return _knob->isAnimated(dimension);
}

int
Param::getNumKeys(int dimension) const
{
    return _knob->getKeyFramesCount(dimension);
}

int
Param::getKeyIndex(int time,int dimension) const
{
    return _knob->getKeyFrameIndex(dimension, time);
}

bool
Param::getKeyTime(int index,int dimension,double* time) const
{
    return _knob->getKeyFrameTime(index, dimension, time);
}

void
Param::deleteValueAtTime(int time,int dimension)
{
    _knob->deleteValueAtTime(time, dimension);
}

void
Param::removeAnimation(int dimension)
{
    _knob->removeAnimation(dimension);
}

double
Param::getDerivativeAtTime(double time, int dimension) const
{
    return _knob->getDerivativeAtTime(time, dimension);
}

double
Param::getIntegrateFromTimeToTime(double time1, double time2, int dimension) const
{
    return _knob->getIntegrateFromTimeToTime(time1, time2, dimension);
}

int
Param::getCurrentTime() const
{
    return _knob->getCurrentTime();
}

void
Param::addDependency(const Param* param)
{
    _knob->addListener(param->_knob.get());
}

void
Param::addAsDependencyOf(Param* param)
{
    param->addDependency(this);
}

void
Param::setExpression(const std::string& expr,bool hasRetVariable,int dimension)
{
    _knob->setExpression(dimension,expr,hasRetVariable);
}
///////////// IntParam

IntParam::IntParam(const boost::shared_ptr<Int_Knob>& knob)
: Param(boost::dynamic_pointer_cast<KnobI>(knob))
, _intKnob(knob)
{
    
}

IntParam::~IntParam()
{

}

int
IntParam::getValue(int dimension) const
{
    return _intKnob->getValue(dimension);
}

void
IntParam::setValue(int value,int dimension)
{
    _intKnob->setValue(value, dimension);
}

int
IntParam::getValueAtTime(int time,int dimension) const
{
    return _intKnob->getValueAtTime(time,dimension);
}

void
IntParam::setValueAtTime(int value,int time,int dimension)
{
    _intKnob->setValueAtTime(time, value, dimension);
}

void
IntParam::setDefaultValue(int value,int dimension)
{
    _intKnob->setDefaultValue(value,dimension);
}

int
IntParam::getDefaultValue(int dimension) const
{
    return _intKnob->getDefaultValues_mt_safe()[dimension];
}

void
IntParam::restoreDefaultValue(int dimension)
{
    _intKnob->resetToDefaultValue(dimension);
}

void
IntParam::setMinimum(int minimum,int dimension)
{
    _intKnob->setMinimum(minimum,dimension);
}

int
IntParam::getMinimum(int dimension) const
{
    return _intKnob->getMinimum(dimension);
}

void
IntParam::setMaximum(int maximum,int dimension)
{
    _intKnob->setMaximum(maximum,dimension);
}

int
IntParam::getMaximum(int dimension) const
{
    return _intKnob->getMaximum(dimension);
}

void
IntParam::setDisplayMinimum(int minimum,int dimension)
{
    return _intKnob->setDisplayMinimum(minimum,dimension);
}

int
IntParam::getDisplayMinimum(int dimension) const
{
    return _intKnob->getDisplayMinimum(dimension);
}

void
IntParam::setDisplayMaximum(int maximum,int dimension)
{
    _intKnob->setDisplayMaximum(maximum,dimension);
}


int
IntParam::getDisplayMaximum(int dimension) const
{
    return _intKnob->getDisplayMaximum(dimension);
}