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


Param::Param(const boost::shared_ptr<KnobI>& knob)
: _knob(knob)
{
    
}

Param::~Param()
{
    
}

Param*
Param::getParent() const
{
    boost::shared_ptr<KnobI> parent = _knob->getParentKnob();
    if (parent) {
        return new Param(parent);
    } else {
        return 0;
    }
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

void
Param::setHelp(const std::string& help)
{
    if (!_knob->isUserKnob()) {
        return;
    }
    _knob->setHintToolTip(help);
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
    if (!_knob->isUserKnob()) {
        return;
    }
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
    if (!_knob->isUserKnob()) {
        return;
    }
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

void
Param::setAnimationEnabled(bool e)
{
    if (!_knob->isUserKnob()) {
        return;
    }
    _knob->setAnimationEnabled(e);
}

bool
Param::getAddNewLine()
{
    return !_knob->isNewLineTurnedOff();
}

void
Param::setAddNewLine(bool a)
{
    if (!_knob->isUserKnob()) {
        return;
    }
    if (!a && !_knob->isNewLineTurnedOff()) {
        _knob->turnOffNewLine();
    } 
}

AnimatedParam::AnimatedParam(const boost::shared_ptr<KnobI>& knob)
: Param(knob)
{
    
}


AnimatedParam::~AnimatedParam()
{
    
}

bool
AnimatedParam::getIsAnimated(int dimension) const
{
    return _knob->isAnimated(dimension);
}

int
AnimatedParam::getNumKeys(int dimension) const
{
    return _knob->getKeyFramesCount(dimension);
}

int
AnimatedParam::getKeyIndex(int time,int dimension) const
{
    return _knob->getKeyFrameIndex(dimension, time);
}

bool
AnimatedParam::getKeyTime(int index,int dimension,double* time) const
{
    return _knob->getKeyFrameTime(index, dimension, time);
}

void
AnimatedParam::deleteValueAtTime(int time,int dimension)
{
    _knob->deleteValueAtTime(time, dimension);
}

void
AnimatedParam::removeAnimation(int dimension)
{
    _knob->removeAnimation(dimension);
}

double
AnimatedParam::getDerivativeAtTime(double time, int dimension) const
{
    return _knob->getDerivativeAtTime(time, dimension);
}

double
AnimatedParam::getIntegrateFromTimeToTime(double time1, double time2, int dimension) const
{
    return _knob->getIntegrateFromTimeToTime(time1, time2, dimension);
}

int
AnimatedParam::getCurrentTime() const
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
AnimatedParam::setExpression(const std::string& expr,bool hasRetVariable,int dimension)
{
    (void)_knob->setExpression(dimension,expr,hasRetVariable);
}

std::string
AnimatedParam::getExpression(int dimension,bool* hasRetVariable) const
{
    std::string ret = _knob->getExpression(dimension);
    *hasRetVariable = _knob->isExpressionUsingRetVariable(dimension);
    return ret;
}

///////////// IntParam

IntParam::IntParam(const boost::shared_ptr<Int_Knob>& knob)
: AnimatedParam(boost::dynamic_pointer_cast<KnobI>(knob))
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

Int2DTuple
Int2DParam::get() const
{
    Int2DTuple ret;
    ret.x = _intKnob->getValue(0);
    ret.y = _intKnob->getValue(1);
    return ret;
}

Int3DTuple
Int3DParam::get() const
{
    Int3DTuple ret;
    ret.x = _intKnob->getValue(0);
    ret.y = _intKnob->getValue(1);
    ret.z = _intKnob->getValue(2);
    return ret;
}

int
IntParam::get(int frame) const
{
    return _intKnob->getValueAtTime(frame,0);
}

Int2DTuple
Int2DParam::get(int frame) const
{
    Int2DTuple ret;
    ret.x = _intKnob->getValueAtTime(frame,0);
    ret.y = _intKnob->getValueAtTime(frame,1);
    return ret;
}


Int3DTuple
Int3DParam::get(int frame) const
{
    Int3DTuple ret;
    ret.x = _intKnob->getValueAtTime(frame,0);
    ret.y = _intKnob->getValueAtTime(frame,1);
    ret.z = _intKnob->getValueAtTime(frame,2);
    return ret;
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
    if (!_intKnob->isUserKnob()) {
        return;
    }
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
    if (!_intKnob->isUserKnob()) {
        return;
    }
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
: AnimatedParam(boost::dynamic_pointer_cast<KnobI>(knob))
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

Double2DTuple
Double2DParam::get() const
{
    Double2DTuple ret;
    ret.x = _doubleKnob->getValue(0);
    ret.y = _doubleKnob->getValue(1);
    return ret;
}

Double3DTuple
Double3DParam::get() const
{
    Double3DTuple ret;
    ret.x = _doubleKnob->getValue(0);
    ret.y = _doubleKnob->getValue(1);
    ret.z = _doubleKnob->getValue(2);
    return ret;
}

double
DoubleParam::get(int frame) const
{
    return _doubleKnob->getValueAtTime(frame, 0);
}

Double2DTuple
Double2DParam::get(int frame) const
{
    Double2DTuple ret;
    ret.x = _doubleKnob->getValueAtTime(frame, 0);
    ret.y = _doubleKnob->getValueAtTime(frame, 1);
    return ret;
}

Double3DTuple
Double3DParam::get(int frame) const
{
    Double3DTuple ret;
    ret.x = _doubleKnob->getValueAtTime(frame, 0);
    ret.y = _doubleKnob->getValueAtTime(frame, 1);
    ret.z = _doubleKnob->getValueAtTime(frame, 2);
    return ret;
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
    if (!_doubleKnob->isUserKnob()) {
        return;
    }
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
    if (!_doubleKnob->isUserKnob()) {
        return;
    }
     _doubleKnob->setDisplayMinimum(minimum,dimension);
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
: AnimatedParam(boost::dynamic_pointer_cast<KnobI>(knob))
, _colorKnob(knob)
{
    
}

ColorParam::~ColorParam()
{
    
}


ColorTuple
ColorParam::get() const
{
    ColorTuple ret;
    ret.r = _colorKnob->getValue(0);
    ret.g = _colorKnob->getValue(1);
    ret.b = _colorKnob->getValue(2);
    ret.a = _colorKnob->getDimension() == 4 ? _colorKnob->getValue(3) : 1.;
    return ret;
}


ColorTuple
ColorParam::get(int frame) const
{
    ColorTuple ret;
    ret.r = _colorKnob->getValueAtTime(frame, 0);
    ret.g = _colorKnob->getValueAtTime(frame, 1);
    ret.b = _colorKnob->getValueAtTime(frame, 2);
    ret.a = _colorKnob->getDimension() == 4 ? _colorKnob->getValueAtTime(frame, 2) : 1.;
    return ret;
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
    if (!_colorKnob->isUserKnob()) {
        return;
    }
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
    if (!_colorKnob->isUserKnob()) {
        return;
    }
    _colorKnob->setDisplayMinimum(minimum,dimension);
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

//////////////// ChoiceParam
ChoiceParam::ChoiceParam(const boost::shared_ptr<Choice_Knob>& knob)
: AnimatedParam(boost::dynamic_pointer_cast<KnobI>(knob))
, _choiceKnob(knob)
{
    
}

ChoiceParam::~ChoiceParam()
{
    
}

int
ChoiceParam::get() const
{
    return _choiceKnob->getValue(0);
}

int
ChoiceParam::get(int frame) const
{
    return _choiceKnob->getValueAtTime(frame,0);
}


void
ChoiceParam::set(int x)
{
    _choiceKnob->setValue(x, 0);
}

void
ChoiceParam::set(int x, int frame)
{
    _choiceKnob->setValueAtTime(frame, x, 0);
}

int
ChoiceParam::getValue(int dimension) const
{
    return _choiceKnob->getValue(dimension);
}

void
ChoiceParam::setValue(int value,int dimension)
{
    _choiceKnob->setValue(value, dimension);
}

int
ChoiceParam::getValueAtTime(int time,int dimension) const
{
    return _choiceKnob->getValueAtTime(time,dimension);
}

void
ChoiceParam::setValueAtTime(int value,int time,int dimension)
{
    _choiceKnob->setValueAtTime(time, value, dimension);
}

void
ChoiceParam::setDefaultValue(int value,int dimension)
{
    _choiceKnob->setDefaultValue(value,dimension);
}

int
ChoiceParam::getDefaultValue(int dimension) const
{
    return _choiceKnob->getDefaultValues_mt_safe()[dimension];
}

void
ChoiceParam::restoreDefaultValue(int dimension)
{
    _choiceKnob->resetToDefaultValue(dimension);
}

void
ChoiceParam::addOption(const std::string& option,const std::string& help)
{
    if (!_choiceKnob->isUserKnob()) {
        return;
    }
    std::vector<std::string> entries = _choiceKnob->getEntries_mt_safe();
    std::vector<std::string> helps = _choiceKnob->getEntriesHelp_mt_safe();
    entries.push_back(option);
    if (!help.empty()) {
        helps.push_back(help);
    }
    _choiceKnob->populateChoices(entries,helps);
    
}

void
ChoiceParam::setOptions(const std::list<std::pair<std::string,std::string> >& options)
{
    if (!_choiceKnob->isUserKnob()) {
        return;
    }
    
    std::vector<std::string> entries,helps;
    for (std::list<std::pair<std::string,std::string> >::const_iterator it = options.begin(); it != options.end(); ++it) {
        entries.push_back(it->first);
        helps.push_back(it->second);
    }
    _choiceKnob->populateChoices(entries,helps);
}

std::string
ChoiceParam::getOption(int index) const
{
    std::vector<std::string> entries =  _choiceKnob->getEntries_mt_safe();
    if (index < 0 || index >= (int)entries.size()) {
        return std::string();
    }
    return entries[index];
}

int
ChoiceParam::addAsDependencyOf(int fromExprDimension,Param* param)
{
    _addAsDependencyOf(fromExprDimension, param);
    return _choiceKnob->getValue();
}

////////////////BooleanParam


BooleanParam::BooleanParam(const boost::shared_ptr<Bool_Knob>& knob)
: AnimatedParam(boost::dynamic_pointer_cast<KnobI>(knob))
, _boolKnob(knob)
{
    
}

BooleanParam::~BooleanParam()
{
    
}

bool
BooleanParam::get() const
{
    return _boolKnob->getValue(0);
}

bool
BooleanParam::get(int frame) const
{
    return _boolKnob->getValueAtTime(frame,0);
}


void
BooleanParam::set(bool x)
{
    _boolKnob->setValue(x, 0);
}

void
BooleanParam::set(bool x, int frame)
{
    _boolKnob->setValueAtTime(frame, x, 0);
}

bool
BooleanParam::getValue() const
{
    return _boolKnob->getValue(0);
}

void
BooleanParam::setValue(bool value)
{
    _boolKnob->setValue(value, 0);
}

bool
BooleanParam::getValueAtTime(int time) const
{
    return _boolKnob->getValueAtTime(time,0);
}

void
BooleanParam::setValueAtTime(bool value,int time)
{
    _boolKnob->setValueAtTime(time, value, 0);
}

void
BooleanParam::setDefaultValue(bool value)
{
    _boolKnob->setDefaultValue(value,0);
}

bool
BooleanParam::getDefaultValue() const
{
    return _boolKnob->getDefaultValues_mt_safe()[0];
}

void
BooleanParam::restoreDefaultValue()
{
    _boolKnob->resetToDefaultValue(0);
}


bool
BooleanParam::addAsDependencyOf(int fromExprDimension,Param* param)
{
    _addAsDependencyOf(fromExprDimension, param);
    return _boolKnob->getValue();
}

////////////// StringParamBase


StringParamBase::StringParamBase(const boost::shared_ptr<Knob<std::string> >& knob)
: AnimatedParam(boost::dynamic_pointer_cast<KnobI>(knob))
, _stringKnob(knob)
{
    
}

StringParamBase::~StringParamBase()
{
    
}


std::string
StringParamBase::get() const
{
    return _stringKnob->getValue(0);
}

std::string
StringParamBase::get(int frame) const
{
    return _stringKnob->getValueAtTime(frame,0);
}


void
StringParamBase::set(const std::string& x)
{
    _stringKnob->setValue(x, 0);
}

void
StringParamBase::set(const std::string& x, int frame)
{
    _stringKnob->setValueAtTime(frame, x, 0);
}

std::string
StringParamBase::getValue() const
{
    return _stringKnob->getValue(0);
}

void
StringParamBase::setValue(const std::string& value)
{
    _stringKnob->setValue(value, 0);
}

std::string
StringParamBase::getValueAtTime(int time) const
{
    return _stringKnob->getValueAtTime(time,0);
}

void
StringParamBase::setValueAtTime(const std::string& value,int time)
{
    _stringKnob->setValueAtTime(time, value, 0);
}

void
StringParamBase::setDefaultValue(const std::string& value)
{
    _stringKnob->setDefaultValue(value,0);
}

std::string
StringParamBase::getDefaultValue() const
{
    return _stringKnob->getDefaultValues_mt_safe()[0];
}

void
StringParamBase::restoreDefaultValue()
{
    _stringKnob->resetToDefaultValue(0);
}


std::string
StringParamBase::addAsDependencyOf(int fromExprDimension,Param* param)
{
    _addAsDependencyOf(fromExprDimension, param);
    return _stringKnob->getValue();
}


////////////////////StringParam

StringParam::StringParam(const boost::shared_ptr<String_Knob>& knob)
: StringParamBase(boost::dynamic_pointer_cast<Knob<std::string> >(knob))
, _sKnob(knob)
{
    
}

StringParam::~StringParam()
{
    
}

void
StringParam::setType(StringParam::TypeEnum type)
{
    if (!_sKnob->isUserKnob()) {
        return;
    }
    switch (type) {
        case eStringTypeLabel:
            _sKnob->setAsLabel();
            break;
        case eStringTypeMultiLine:
            _sKnob->setAsMultiLine();
            break;
        case eStringTypeRichTextMultiLine:
            _sKnob->setAsMultiLine();
            _sKnob->setUsesRichText(true);
            break;
        case eStringTypeCustom:
            _sKnob->setAsCustom();
            break;
        case eStringTypeDefault:
        default:
            break;
    }
}

/////////////////////FileParam

FileParam::FileParam(const boost::shared_ptr<File_Knob>& knob)
: StringParamBase(boost::dynamic_pointer_cast<Knob<std::string> >(knob))
, _sKnob(knob)
{
    
}

FileParam::~FileParam()
{
    
}

void
FileParam::setSequenceEnabled(bool enabled)
{
    if (!_sKnob->isUserKnob()) {
        return;
    }
    if (enabled) {
        _sKnob->setAsInputImage();
    }
    
}

void
FileParam::openFile()
{
    _sKnob->open_file();
}

/////////////////////OutputFileParam

OutputFileParam::OutputFileParam(const boost::shared_ptr<OutputFile_Knob>& knob)
: StringParamBase(boost::dynamic_pointer_cast<Knob<std::string> >(knob))
, _sKnob(knob)
{
    
}

OutputFileParam::~OutputFileParam()
{
    
}

void
OutputFileParam::setSequenceEnabled(bool enabled)
{
    if (!_sKnob->isUserKnob()) {
        return;
    }
    if (enabled) {
        _sKnob->setAsOutputImageFile();
    } else {
        _sKnob->turnOffSequences();
    }
}

void
OutputFileParam::openFile()
{
    _sKnob->open_file();
}

////////////////////PathParam

PathParam::PathParam(const boost::shared_ptr<Path_Knob>& knob)
: StringParamBase(boost::dynamic_pointer_cast<Knob<std::string> >(knob))
, _sKnob(knob)
{
    
}

PathParam::~PathParam()
{
    
}


void
PathParam::setAsMultiPathTable()
{
    if (!_sKnob->isUserKnob()) {
        return;
    }
    _sKnob->setMultiPath(true);
}

////////////////////ButtonParam

ButtonParam::ButtonParam(const boost::shared_ptr<Button_Knob>& knob)
: Param(knob)
, _buttonKnob(boost::dynamic_pointer_cast<Button_Knob>(knob))
{
    
}

ButtonParam::~ButtonParam()
{
    
}

void
ButtonParam::setIconFilePath(const std::string& icon)
{
    _buttonKnob->setIconFilePath(icon);
}

///////////////////GroupParam

GroupParam::GroupParam(const boost::shared_ptr<Group_Knob>& knob)
: Param(knob)
, _groupKnob(boost::dynamic_pointer_cast<Group_Knob>(knob))
{
    
}

GroupParam::~GroupParam()
{
    
}


void
GroupParam::addParam(const Param* param)
{
    if (!param || !param->getInternalKnob()->isUserKnob() || param->getInternalKnob()->getParentKnob()) {
        return;
    }
    _groupKnob->addKnob(param->getInternalKnob());
}


void
GroupParam::setAsTab()
{
    if (!_groupKnob->isUserKnob()) {
        return;
    }
    _groupKnob->setAsTab();
}

//////////////////////PageParam

PageParam::PageParam(const boost::shared_ptr<Page_Knob>& knob)
: Param(knob)
, _pageKnob(boost::dynamic_pointer_cast<Page_Knob>(knob))
{
    
}

PageParam::~PageParam()
{
    
}


void
PageParam::addParam(const Param* param)
{
    if (!param || !param->getInternalKnob()->isUserKnob() || param->getInternalKnob()->getParentKnob()) {
        return;
    }
    _pageKnob->addKnob(param->getInternalKnob());
}

////////////////////ParametricParam
ParametricParam::ParametricParam(const boost::shared_ptr<Parametric_Knob>& knob)
: Param(boost::dynamic_pointer_cast<KnobI>(knob))
, _parametricKnob(knob)
{
    
}

ParametricParam::~ParametricParam()
{
    
}

void
ParametricParam::setCurveColor(int dimension,double r,double g,double b)
{
    _parametricKnob->setCurveColor(dimension, r, g, b);
}

void
ParametricParam::getCurveColor(int dimension, ColorTuple& ret) const
{
    _parametricKnob->getCurveColor(dimension, &ret.r, &ret.g, &ret.b);
    ret.a = 1.;
}

Natron::StatusEnum
ParametricParam::addControlPoint(int dimension,double key,double value)
{
    return _parametricKnob->addControlPoint(dimension, key, value);
}

double
ParametricParam::getValue(int dimension,double parametricPosition) const
{
    double ret;
    Natron::StatusEnum stat =  _parametricKnob->getValue(dimension, parametricPosition, &ret);
    if (stat == Natron::eStatusFailed) {
        ret =  0.;
    }
    return ret;
}

int
ParametricParam::getNControlPoints(int dimension) const
{
    int ret;
    Natron::StatusEnum stat =  _parametricKnob->getNControlPoints(dimension, &ret);
    if (stat == Natron::eStatusFailed) {
        ret = 0;
    }
    return ret;
}

Natron::StatusEnum
ParametricParam::getNthControlPoint(int dimension,
                                    int nthCtl,
                                    double *key,
                                    double *value,
                                    double *leftDerivative,
                                    double *rightDerivative) const
{
    return _parametricKnob->getNthControlPoint(dimension, nthCtl, key, value,leftDerivative, rightDerivative);
}

Natron::StatusEnum
ParametricParam::setNthControlPoint(int dimension,
                                    int nthCtl,
                                    double key,
                                    double value,
                                    double leftDerivative,
                                    double rightDerivative)
{
    return _parametricKnob->setNthControlPoint(dimension, nthCtl, key, value,leftDerivative,rightDerivative);
}

Natron::StatusEnum
ParametricParam::deleteControlPoint(int dimension, int nthCtl)
{
    return _parametricKnob->deleteControlPoint(dimension, nthCtl);
}

Natron::StatusEnum
ParametricParam::deleteAllControlPoints(int dimension)
{
    return _parametricKnob->deleteAllControlPoints(dimension);
}

