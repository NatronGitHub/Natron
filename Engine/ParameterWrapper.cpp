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
Param::_addAsDependencyOf(int fromExprDimension,Param* param)
{
    if (param->_knob == _knob) {
        return;
    }
    _knob->addListener(true,fromExprDimension, param->_knob.get());
}

void
Param::setExpression(const std::string& expr,bool hasRetVariable,int dimension)
{
    (void)_knob->setExpression(dimension,expr,hasRetVariable);
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
IntParam::get() const
{
    return _intKnob->getValue(0);
}

void
Int2DParam::get(Int2DTuple& ret) const
{
    ret.x = _intKnob->getValue(0);
    ret.y = _intKnob->getValue(1);
}

void
Int3DParam::get(Int3DTuple& ret) const
{
    ret.x = _intKnob->getValue(0);
    ret.y = _intKnob->getValue(1);
    ret.z = _intKnob->getValue(2);
}

int
IntParam::get(int frame) const
{
    return _intKnob->getValueAtTime(frame,0);
}

void
Int2DParam::get(int frame,Int2DTuple &ret) const
{
    ret.x = _intKnob->getValueAtTime(frame,0);
    ret.y = _intKnob->getValueAtTime(frame,1);
}


void
Int3DParam::get(int frame, Int3DTuple& ret) const
{
    ret.x = _intKnob->getValueAtTime(frame,0);
    ret.y = _intKnob->getValueAtTime(frame,1);
    ret.z = _intKnob->getValueAtTime(frame,2);
}


void
IntParam::set(int x)
{
    _intKnob->setValue(x, 0);
}

void
Int2DParam::set(int x, int y)
{
    _intKnob->blockEvaluation();
    _intKnob->setValue(x, 0);
    _intKnob->unblockEvaluation();
    _intKnob->setValue(y, 1);
}

void
Int3DParam::set(int x, int y, int z)
{
    _intKnob->blockEvaluation();
    _intKnob->setValue(x, 0);
    _intKnob->setValue(y, 1);
    _intKnob->unblockEvaluation();
    _intKnob->setValue(z, 2);
}

void
IntParam::set(int x, int frame)
{
    _intKnob->setValueAtTime(frame, x, 0);
}

void
Int2DParam::set(int x, int y, int frame)
{
    _intKnob->blockEvaluation();
    _intKnob->setValueAtTime(frame,x, 0);
    _intKnob->unblockEvaluation();
    _intKnob->setValueAtTime(frame,y, 1);
}

void
Int3DParam::set(int x, int y, int z, int frame)
{
    _intKnob->blockEvaluation();
    _intKnob->setValueAtTime(frame,x, 0);
    _intKnob->setValueAtTime(frame,y, 1);
    _intKnob->unblockEvaluation();
    _intKnob->setValueAtTime(frame,z, 2);
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

int
IntParam::addAsDependencyOf(int fromExprDimension,Param* param)
{
    _addAsDependencyOf(fromExprDimension, param);
    return _intKnob->getValue();
}

//////////// DoubleParam

DoubleParam::DoubleParam(const boost::shared_ptr<Double_Knob>& knob)
: Param(boost::dynamic_pointer_cast<KnobI>(knob))
, _doubleKnob(knob)
{
    
}

DoubleParam::~DoubleParam()
{
    
}

double
DoubleParam::get() const
{
    return _doubleKnob->getValue(0);
}

void
Double2DParam::get(Double2DTuple& ret) const
{
    ret.x = _doubleKnob->getValue(0);
    ret.y = _doubleKnob->getValue(1);
}

void
Double3DParam::get(Double3DTuple & ret) const
{
    ret.x = _doubleKnob->getValue(0);
    ret.y = _doubleKnob->getValue(1);
    ret.z = _doubleKnob->getValue(2);
}

double
DoubleParam::get(int frame) const
{
    return _doubleKnob->getValueAtTime(frame, 0);
}

void
Double2DParam::get(int frame, Double2DTuple& ret) const
{
    ret.x = _doubleKnob->getValueAtTime(frame, 0);
    ret.y = _doubleKnob->getValueAtTime(frame, 1);
}

void
Double3DParam::get(int frame, Double3DTuple &ret) const
{

    ret.x = _doubleKnob->getValueAtTime(frame, 0);
    ret.y = _doubleKnob->getValueAtTime(frame, 1);
    ret.z = _doubleKnob->getValueAtTime(frame, 2);
}

void
DoubleParam::set(double x)
{
    _doubleKnob->setValue(x, 0);
}

void
Double2DParam::set(double x, double y)
{
    _doubleKnob->blockEvaluation();
    _doubleKnob->setValue(x, 0);
    _doubleKnob->unblockEvaluation();
    _doubleKnob->setValue(y, 1);

}

void
Double3DParam::set(double x, double y, double z)
{
    _doubleKnob->blockEvaluation();
    _doubleKnob->setValue(x, 0);
    _doubleKnob->setValue(y, 1);
    _doubleKnob->unblockEvaluation();
    _doubleKnob->setValue(z, 2);
}

void
DoubleParam::set(double x, int frame)
{
     _doubleKnob->setValueAtTime(frame, x, 0);
}

void
Double2DParam::set(double x, double y, int frame)
{
    _doubleKnob->blockEvaluation();
    _doubleKnob->setValueAtTime(frame,x, 0);
    _doubleKnob->unblockEvaluation();
    _doubleKnob->setValueAtTime(frame,y, 1);
}

void
Double3DParam::set(double x, double y, double z, int frame)
{
    _doubleKnob->blockEvaluation();
    _doubleKnob->setValueAtTime(frame,x, 0);
    _doubleKnob->setValueAtTime(frame,y, 1);
    _doubleKnob->unblockEvaluation();
    _doubleKnob->setValueAtTime(frame,z, 2);
}

double
DoubleParam::getValue(int dimension) const
{
    return _doubleKnob->getValue(dimension);
}

void
DoubleParam::setValue(double value,int dimension)
{
    _doubleKnob->setValue(value, dimension);
}

double
DoubleParam::getValueAtTime(int time,int dimension) const
{
    return _doubleKnob->getValueAtTime(time,dimension);
}

void
DoubleParam::setValueAtTime(double value,int time,int dimension)
{
    _doubleKnob->setValueAtTime(time, value, dimension);
}

void
DoubleParam::setDefaultValue(double value,int dimension)
{
    _doubleKnob->setDefaultValue(value,dimension);
}

double
DoubleParam::getDefaultValue(int dimension) const
{
    return _doubleKnob->getDefaultValues_mt_safe()[dimension];
}

void
DoubleParam::restoreDefaultValue(int dimension)
{
    _doubleKnob->resetToDefaultValue(dimension);
}

void
DoubleParam::setMinimum(double minimum,int dimension)
{
    _doubleKnob->setMinimum(minimum,dimension);
}

double
DoubleParam::getMinimum(int dimension) const
{
    return _doubleKnob->getMinimum(dimension);
}

void
DoubleParam::setMaximum(double maximum,int dimension)
{
    _doubleKnob->setMaximum(maximum,dimension);
}

double
DoubleParam::getMaximum(int dimension) const
{
    return _doubleKnob->getMaximum(dimension);
}

void
DoubleParam::setDisplayMinimum(double minimum,int dimension)
{
    return _doubleKnob->setDisplayMinimum(minimum,dimension);
}

double
DoubleParam::getDisplayMinimum(int dimension) const
{
    return _doubleKnob->getDisplayMinimum(dimension);
}

void
DoubleParam::setDisplayMaximum(double maximum,int dimension)
{
    _doubleKnob->setDisplayMaximum(maximum,dimension);
}


double
DoubleParam::getDisplayMaximum(int dimension) const
{
    return _doubleKnob->getDisplayMaximum(dimension);
}

double
DoubleParam::addAsDependencyOf(int fromExprDimension,Param* param)
{
    _addAsDependencyOf(fromExprDimension, param);
    return _doubleKnob->getValue();
}


////////ColorParam

ColorParam::ColorParam(const boost::shared_ptr<Color_Knob>& knob)
: Param(boost::dynamic_pointer_cast<KnobI>(knob))
, _colorKnob(knob)
{
    
}

ColorParam::~ColorParam()
{
    
}


void
ColorParam::get(ColorTuple & ret) const
{
    ret.r = _colorKnob->getValue(0);
    ret.g = _colorKnob->getValue(1);
    ret.b = _colorKnob->getValue(2);
    ret.a = _colorKnob->getDimension() == 4 ? _colorKnob->getValue(3) : 1.;
}


void
ColorParam::get(int frame, ColorTuple &ret) const
{
    
    ret.r = _colorKnob->getValueAtTime(frame, 0);
    ret.g = _colorKnob->getValueAtTime(frame, 1);
    ret.b = _colorKnob->getValueAtTime(frame, 2);
    ret.a = _colorKnob->getDimension() == 4 ? _colorKnob->getValueAtTime(frame, 2) : 1.;
}

void
ColorParam::set(double r, double g, double b, double a)
{
    _colorKnob->blockEvaluation();
    _colorKnob->setValue(r, 0);
    _colorKnob->setValue(g, 1);
    int dims = _colorKnob->getDimension();
    if (dims == 3) {
        _colorKnob->unblockEvaluation();
    }
    _colorKnob->setValue(b, 2);
    if (dims == 4) {
        _colorKnob->unblockEvaluation();
        _colorKnob->setValue(a, 3);
    }
}

void
ColorParam::set(double r, double g, double b, double a, int frame)
{
    _colorKnob->blockEvaluation();
    _colorKnob->setValueAtTime(frame, r, 0);
    _colorKnob->setValueAtTime(frame,g, 1);
    int dims = _colorKnob->getDimension();
    if (dims == 3) {
        _colorKnob->unblockEvaluation();
    }
    _colorKnob->setValueAtTime(frame,b, 2);
    if (dims == 4) {
        _colorKnob->unblockEvaluation();
        _colorKnob->setValueAtTime(frame,a, 3);
    }

}


double
ColorParam::getValue(int dimension) const
{
    return _colorKnob->getValue(dimension);
}

void
ColorParam::setValue(double value,int dimension)
{
    _colorKnob->setValue(value, dimension);
}

double
ColorParam::getValueAtTime(int time,int dimension) const
{
    return _colorKnob->getValueAtTime(time,dimension);
}

void
ColorParam::setValueAtTime(double value,int time,int dimension)
{
    _colorKnob->setValueAtTime(time, value, dimension);
}

void
ColorParam::setDefaultValue(double value,int dimension)
{
    _colorKnob->setDefaultValue(value,dimension);
}

double
ColorParam::getDefaultValue(int dimension) const
{
    return _colorKnob->getDefaultValues_mt_safe()[dimension];
}

void
ColorParam::restoreDefaultValue(int dimension)
{
    _colorKnob->resetToDefaultValue(dimension);
}

void
ColorParam::setMinimum(double minimum,int dimension)
{
    _colorKnob->setMinimum(minimum,dimension);
}

double
ColorParam::getMinimum(int dimension) const
{
    return _colorKnob->getMinimum(dimension);
}

void
ColorParam::setMaximum(double maximum,int dimension)
{
    _colorKnob->setMaximum(maximum,dimension);
}

double
ColorParam::getMaximum(int dimension) const
{
    return _colorKnob->getMaximum(dimension);
}

void
ColorParam::setDisplayMinimum(double minimum,int dimension)
{
    return _colorKnob->setDisplayMinimum(minimum,dimension);
}

double
ColorParam::getDisplayMinimum(int dimension) const
{
    return _colorKnob->getDisplayMinimum(dimension);
}

void
ColorParam::setDisplayMaximum(double maximum,int dimension)
{
    _colorKnob->setDisplayMaximum(maximum,dimension);
}


double
ColorParam::getDisplayMaximum(int dimension) const
{
    return _colorKnob->getDisplayMaximum(dimension);
}

double
ColorParam::addAsDependencyOf(int fromExprDimension,Param* param)
{
    _addAsDependencyOf(fromExprDimension, param);
    return _colorKnob->getValue();
}

