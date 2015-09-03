/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "ParameterWrapper.h"
#include "Engine/EffectInstance.h"
#include "Engine/Node.h"

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
    boost::shared_ptr<KnobI> parent = getInternalKnob()->getParentKnob();
    if (parent) {
        return new Param(parent);
    } else {
        return 0;
    }
}

int
Param::getNumDimensions() const
{
    return getInternalKnob()->getDimension();
}

std::string
Param::getScriptName() const
{
    return getInternalKnob()->getName();
}

std::string
Param::getLabel() const
{
    return getInternalKnob()->getDescription();
}


std::string
Param::getTypeName() const
{
    return getInternalKnob()->typeName();
}

std::string
Param::getHelp() const
{
    return getInternalKnob()->getHintToolTip();
}

void
Param::setHelp(const std::string& help)
{
    if (!getInternalKnob()->isUserKnob()) {
        return;
    }
    getInternalKnob()->setHintToolTip(help);
}

bool
Param::getIsVisible() const
{
    return !getInternalKnob()->getIsSecret();
}

void
Param::setVisible(bool visible)
{
    getInternalKnob()->setSecret(!visible);
}

bool
Param::getIsEnabled(int dimension) const
{
    return getInternalKnob()->isEnabled(dimension);
}

void
Param::setEnabled(bool enabled,int dimension)
{
    getInternalKnob()->setEnabled(dimension, enabled);
}

bool
Param::getIsPersistant() const
{
    return getInternalKnob()->getIsPersistant();
}

void
Param::setPersistant(bool persistant)
{
    if (!getInternalKnob()->isUserKnob()) {
        return;
    }
    getInternalKnob()->setIsPersistant(persistant);
}

bool
Param::getEvaluateOnChange() const
{
    return getInternalKnob()->getEvaluateOnChange();
}

void
Param::setEvaluateOnChange(bool eval)
{
    if (!getInternalKnob()->isUserKnob()) {
        return;
    }
    getInternalKnob()->setEvaluateOnChange(eval);
}

bool
Param::getCanAnimate() const
{
    return getInternalKnob()->canAnimate();
}

bool
Param::getIsAnimationEnabled() const
{
    return getInternalKnob()->isAnimationEnabled();
}

void
Param::setAnimationEnabled(bool e)
{
    if (!getInternalKnob()->isUserKnob()) {
        return;
    }
    getInternalKnob()->setAnimationEnabled(e);
}

bool
Param::getAddNewLine()
{
    return getInternalKnob()->isNewLineActivated();
}

void
Param::setAddNewLine(bool a)
{
    boost::shared_ptr<KnobI> knob = getInternalKnob();
    if (!knob || !knob->isUserKnob()) {
        return;
    }
    
    boost::shared_ptr<KnobI> parentKnob = knob->getParentKnob();
    if (parentKnob) {
        Group_Knob* parentIsGrp = dynamic_cast<Group_Knob*>(parentKnob.get());
        Page_Knob* parentIsPage = dynamic_cast<Page_Knob*>(parentKnob.get());
        assert(parentIsGrp || parentIsPage);
        std::vector<boost::shared_ptr<KnobI> > children;
        if (parentIsGrp) {
            children = parentIsGrp->getChildren();
        } else if (parentIsPage) {
            children = parentIsPage->getChildren();
        }
        for (U32 i = 0; i < children.size(); ++i) {
            if (children[i] == knob) {
                if (i > 0) {
                    children[i - 1]->setAddNewLine(a);
                }
                break;
            }
        }
    }
}

bool
Param::copy(Param* other, int dimension)
{
    boost::shared_ptr<KnobI> thisKnob = _knob.lock();
    boost::shared_ptr<KnobI> otherKnob = other->_knob.lock();
    if (!thisKnob->isTypeCompatible(otherKnob)) {
        return false;
    }
    thisKnob->cloneAndUpdateGui(otherKnob.get(), dimension);
    return true;
}

double
Param::random(double min,double max) const
{
    if (!getInternalKnob()) {
        return 0;
    }
    return getInternalKnob()->random(min,max);
}

double
Param::random(unsigned int seed) const
{
    if (!getInternalKnob()) {
        return 0;
    }
    return getInternalKnob()->random(seed);
}

int
Param::randomInt(int min, int max)
{
    if (!getInternalKnob()) {
        return 0;
    }
    return getInternalKnob()->randomInt(min,max);
}

int
Param::randomInt(unsigned int seed) const
{
    if (!getInternalKnob()) {
        return 0;
    }
    return getInternalKnob()->randomInt(seed);
}

double
Param::curve(double time, int dimension) const
{
    if (!getInternalKnob()) {
        return 0.;
    }
    return getInternalKnob()->getRawCurveValueAt(time, dimension);
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
    return getInternalKnob()->isAnimated(dimension);
}

int
AnimatedParam::getNumKeys(int dimension) const
{
    return getInternalKnob()->getKeyFramesCount(dimension);
}

int
AnimatedParam::getKeyIndex(int time,int dimension) const
{
    return getInternalKnob()->getKeyFrameIndex(dimension, time);
}

bool
AnimatedParam::getKeyTime(int index,int dimension,double* time) const
{
    return getInternalKnob()->getKeyFrameTime(index, dimension, time);
}

void
AnimatedParam::deleteValueAtTime(int time,int dimension)
{
    getInternalKnob()->deleteValueAtTime(Natron::eCurveChangeReasonInternal,time, dimension);
}

void
AnimatedParam::removeAnimation(int dimension)
{
    getInternalKnob()->removeAnimation(dimension);
}

double
AnimatedParam::getDerivativeAtTime(double time, int dimension) const
{
    return getInternalKnob()->getDerivativeAtTime(time, dimension);
}

double
AnimatedParam::getIntegrateFromTimeToTime(double time1, double time2, int dimension) const
{
    return getInternalKnob()->getIntegrateFromTimeToTime(time1, time2, dimension);
}

int
AnimatedParam::getCurrentTime() const
{
    return getInternalKnob()->getCurrentTime();
}

void
Param::_addAsDependencyOf(int fromExprDimension,Param* param)
{
    boost::shared_ptr<KnobI> otherKnob = param->_knob.lock();
    boost::shared_ptr<KnobI> thisKnob = _knob.lock();
    if (otherKnob == thisKnob) {
        return;
    }
    thisKnob->addListener(true,fromExprDimension, otherKnob);
}

bool
AnimatedParam::setExpression(const std::string& expr,bool hasRetVariable,int dimension)
{
    try {
        _knob.lock()->setExpression(dimension,expr,hasRetVariable);
    } catch (...) {
        return false;
    }
    return true;
}

std::string
AnimatedParam::getExpression(int dimension,bool* hasRetVariable) const
{
    std::string ret = _knob.lock()->getExpression(dimension);
    *hasRetVariable = _knob.lock()->isExpressionUsingRetVariable(dimension);
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
    return _intKnob.lock()->getValue(0);
}

Int2DTuple
Int2DParam::get() const
{
    Int2DTuple ret;
    boost::shared_ptr<Int_Knob> knob = _intKnob.lock();
    ret.x = knob->getValue(0);
    ret.y = knob->getValue(1);
    return ret;
}

Int3DTuple
Int3DParam::get() const
{
    boost::shared_ptr<Int_Knob> knob = _intKnob.lock();
    Int3DTuple ret;
    ret.x = knob->getValue(0);
    ret.y = knob->getValue(1);
    ret.z = knob->getValue(2);
    return ret;
}

int
IntParam::get(int frame) const
{
    boost::shared_ptr<Int_Knob> knob = _intKnob.lock();
    return knob->getValueAtTime(frame,0);
}

Int2DTuple
Int2DParam::get(int frame) const
{
    Int2DTuple ret;
    boost::shared_ptr<Int_Knob> knob = _intKnob.lock();
    ret.x = knob->getValueAtTime(frame,0);
    ret.y = knob->getValueAtTime(frame,1);
    return ret;
}


Int3DTuple
Int3DParam::get(int frame) const
{
    Int3DTuple ret;
    boost::shared_ptr<Int_Knob> knob = _intKnob.lock();
    ret.x = knob->getValueAtTime(frame,0);
    ret.y = knob->getValueAtTime(frame,1);
    ret.z = knob->getValueAtTime(frame,2);
    return ret;
}


void
IntParam::set(int x)
{
    _intKnob.lock()->setValue(x, 0);
}

void
Int2DParam::set(int x, int y)
{
    boost::shared_ptr<Int_Knob> knob = _intKnob.lock();
    knob->beginChanges();
    knob->setValue(x, 0);
    knob->setValue(y, 1);
    knob->endChanges();
}

void
Int3DParam::set(int x, int y, int z)
{
    boost::shared_ptr<Int_Knob> knob = _intKnob.lock();
    knob->beginChanges();
    knob->setValue(x, 0);
    knob->setValue(y, 1);
    knob->setValue(z, 2);
    knob->endChanges();
}

void
IntParam::set(int x, int frame)
{
    _intKnob.lock()->setValueAtTime(frame, x, 0);
}

void
Int2DParam::set(int x, int y, int frame)
{
    boost::shared_ptr<Int_Knob> knob = _intKnob.lock();
    knob->beginChanges();
    knob->setValueAtTime(frame,x, 0);
    knob->setValueAtTime(frame,y, 1);
    knob->endChanges();
}

void
Int3DParam::set(int x, int y, int z, int frame)
{
    boost::shared_ptr<Int_Knob> knob = _intKnob.lock();
    knob->beginChanges();
    knob->setValueAtTime(frame,x, 0);
    knob->setValueAtTime(frame,y, 1);
    knob->setValueAtTime(frame,z, 2);
    knob->endChanges();
}

int
IntParam::getValue(int dimension) const
{
    return _intKnob.lock()->getValue(dimension);
}

void
IntParam::setValue(int value,int dimension)
{
    _intKnob.lock()->setValue(value, dimension);
}

int
IntParam::getValueAtTime(int time,int dimension) const
{
    return _intKnob.lock()->getValueAtTime(time,dimension);
}

void
IntParam::setValueAtTime(int value,int time,int dimension)
{
    _intKnob.lock()->setValueAtTime(time, value, dimension);
}

void
IntParam::setDefaultValue(int value,int dimension)
{
    _intKnob.lock()->setDefaultValue(value,dimension);
}

int
IntParam::getDefaultValue(int dimension) const
{
    return _intKnob.lock()->getDefaultValues_mt_safe()[dimension];
}

void
IntParam::restoreDefaultValue(int dimension)
{
    _intKnob.lock()->resetToDefaultValue(dimension);
}

void
IntParam::setMinimum(int minimum,int dimension)
{
    _intKnob.lock()->setMinimum(minimum,dimension);
}

int
IntParam::getMinimum(int dimension) const
{
    return _intKnob.lock()->getMinimum(dimension);
}

void
IntParam::setMaximum(int maximum,int dimension)
{
    if (!_intKnob.lock()->isUserKnob()) {
        return;
    }
    _intKnob.lock()->setMaximum(maximum,dimension);
}

int
IntParam::getMaximum(int dimension) const
{
    return _intKnob.lock()->getMaximum(dimension);
}

void
IntParam::setDisplayMinimum(int minimum,int dimension)
{
    if (!_intKnob.lock()->isUserKnob()) {
        return;
    }
    return _intKnob.lock()->setDisplayMinimum(minimum,dimension);
}

int
IntParam::getDisplayMinimum(int dimension) const
{
    return _intKnob.lock()->getDisplayMinimum(dimension);
}

void
IntParam::setDisplayMaximum(int maximum,int dimension)
{
    _intKnob.lock()->setDisplayMaximum(maximum,dimension);
}


int
IntParam::getDisplayMaximum(int dimension) const
{
    return _intKnob.lock()->getDisplayMaximum(dimension);
}

int
IntParam::addAsDependencyOf(int fromExprDimension,Param* param)
{
    _addAsDependencyOf(fromExprDimension, param);
    return _intKnob.lock()->getValue();
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
    return _doubleKnob.lock()->getValue(0);
}

Double2DTuple
Double2DParam::get() const
{
    boost::shared_ptr<Double_Knob> knob = _doubleKnob.lock();
    Double2DTuple ret;
    ret.x = knob->getValue(0);
    ret.y = knob->getValue(1);
    return ret;
}

Double3DTuple
Double3DParam::get() const
{
    boost::shared_ptr<Double_Knob> knob = _doubleKnob.lock();
    Double3DTuple ret;
    ret.x = knob->getValue(0);
    ret.y = knob->getValue(1);
    ret.z = knob->getValue(2);
    return ret;
}

double
DoubleParam::get(int frame) const
{
    return _doubleKnob.lock()->getValueAtTime(frame, 0);
}

Double2DTuple
Double2DParam::get(int frame) const
{
    Double2DTuple ret;
    boost::shared_ptr<Double_Knob> knob = _doubleKnob.lock();
    ret.x = knob->getValueAtTime(frame, 0);
    ret.y = knob->getValueAtTime(frame, 1);
    return ret;
}

Double3DTuple
Double3DParam::get(int frame) const
{
    boost::shared_ptr<Double_Knob> knob = _doubleKnob.lock();
    Double3DTuple ret;
    ret.x = knob->getValueAtTime(frame, 0);
    ret.y = knob->getValueAtTime(frame, 1);
    ret.z = knob->getValueAtTime(frame, 2);
    return ret;
}

void
DoubleParam::set(double x)
{
    _doubleKnob.lock()->setValue(x, 0);
}

void
Double2DParam::set(double x, double y)
{
    boost::shared_ptr<Double_Knob> knob = _doubleKnob.lock();
    knob->beginChanges();
    knob->setValue(x, 0);
    knob->setValue(y, 1);
    knob->endChanges();

}

void
Double3DParam::set(double x, double y, double z)
{
    boost::shared_ptr<Double_Knob> knob = _doubleKnob.lock();
    knob->beginChanges();
    knob->setValue(x, 0);
    knob->setValue(y, 1);
    knob->setValue(z, 2);
    knob->endChanges();
}

void
DoubleParam::set(double x, int frame)
{
     _doubleKnob.lock()->setValueAtTime(frame, x, 0);
}

void
Double2DParam::set(double x, double y, int frame)
{
    boost::shared_ptr<Double_Knob> knob = _doubleKnob.lock();
    knob->beginChanges();
    knob->setValueAtTime(frame,x, 0);
    knob->setValueAtTime(frame,y, 1);
    knob->endChanges();
}

void
Double2DParam::setUsePointInteract(bool use)
{
    if (!_doubleKnob.lock()) {
        return;
    }
    _doubleKnob.lock()->setHasNativeOverlayHandle(use);
}

void
Double3DParam::set(double x, double y, double z, int frame)
{
    boost::shared_ptr<Double_Knob> knob = _doubleKnob.lock();
    knob->beginChanges();
    knob->setValueAtTime(frame,x, 0);
    knob->setValueAtTime(frame,y, 1);
    knob->setValueAtTime(frame,z, 2);
    knob->endChanges();
}


double
DoubleParam::getValue(int dimension) const
{
    return _doubleKnob.lock()->getValue(dimension);
}

void
DoubleParam::setValue(double value,int dimension)
{
    _doubleKnob.lock()->setValue(value, dimension);
}

double
DoubleParam::getValueAtTime(int time,int dimension) const
{
    return _doubleKnob.lock()->getValueAtTime(time,dimension);
}

void
DoubleParam::setValueAtTime(double value,int time,int dimension)
{
    _doubleKnob.lock()->setValueAtTime(time, value, dimension);
}

void
DoubleParam::setDefaultValue(double value,int dimension)
{
    _doubleKnob.lock()->setDefaultValue(value,dimension);
}

double
DoubleParam::getDefaultValue(int dimension) const
{
    return _doubleKnob.lock()->getDefaultValues_mt_safe()[dimension];
}

void
DoubleParam::restoreDefaultValue(int dimension)
{
    _doubleKnob.lock()->resetToDefaultValue(dimension);
}

void
DoubleParam::setMinimum(double minimum,int dimension)
{
    _doubleKnob.lock()->setMinimum(minimum,dimension);
}

double
DoubleParam::getMinimum(int dimension) const
{
    return _doubleKnob.lock()->getMinimum(dimension);
}

void
DoubleParam::setMaximum(double maximum,int dimension)
{
    if (!_doubleKnob.lock()->isUserKnob()) {
        return;
    }
    _doubleKnob.lock()->setMaximum(maximum,dimension);
}

double
DoubleParam::getMaximum(int dimension) const
{
    return _doubleKnob.lock()->getMaximum(dimension);
}

void
DoubleParam::setDisplayMinimum(double minimum,int dimension)
{
    if (!_doubleKnob.lock()->isUserKnob()) {
        return;
    }
     _doubleKnob.lock()->setDisplayMinimum(minimum,dimension);
}

double
DoubleParam::getDisplayMinimum(int dimension) const
{
    return _doubleKnob.lock()->getDisplayMinimum(dimension);
}

void
DoubleParam::setDisplayMaximum(double maximum,int dimension)
{
    _doubleKnob.lock()->setDisplayMaximum(maximum,dimension);
}


double
DoubleParam::getDisplayMaximum(int dimension) const
{
    return _doubleKnob.lock()->getDisplayMaximum(dimension);
}

double
DoubleParam::addAsDependencyOf(int fromExprDimension,Param* param)
{
    _addAsDependencyOf(fromExprDimension, param);
    return _doubleKnob.lock()->getValue();
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
    boost::shared_ptr<Color_Knob> knob = _colorKnob.lock();
    ret.r = knob->getValue(0);
    ret.g = knob->getValue(1);
    ret.b = knob->getValue(2);
    ret.a = knob->getDimension() == 4 ? knob->getValue(3) : 1.;
    return ret;
}


ColorTuple
ColorParam::get(int frame) const
{
    ColorTuple ret;
    boost::shared_ptr<Color_Knob> knob = _colorKnob.lock();
    ret.r = knob->getValueAtTime(frame, 0);
    ret.g = knob->getValueAtTime(frame, 1);
    ret.b = knob->getValueAtTime(frame, 2);
    ret.a = knob->getDimension() == 4 ? knob->getValueAtTime(frame, 2) : 1.;
    return ret;
}

void
ColorParam::set(double r, double g, double b, double a)
{
    boost::shared_ptr<Color_Knob> knob = _colorKnob.lock();
    knob->beginChanges();
    knob->setValue(r, 0);
    knob->setValue(g, 1);
    knob->setValue(b, 2);
    if (knob->getDimension() == 4) {
        knob->setValue(a, 3);
    }
    knob->endChanges();
}

void
ColorParam::set(double r, double g, double b, double a, int frame)
{
    boost::shared_ptr<Color_Knob> knob = _colorKnob.lock();
    knob->beginChanges();
    knob->setValueAtTime(frame, r, 0);
    knob->setValueAtTime(frame,g, 1);
    int dims = knob->getDimension();
    knob->setValueAtTime(frame,b, 2);
    if (dims == 4) {
        knob->setValueAtTime(frame,a, 3);
    }
    knob->endChanges();
}

void
ColorParam::set(double r, double g, double b)
{
    set(r,g,b,1.);
}

void
ColorParam::set(double r, double g, double b, int frame)
{
    set(r,g,b,1.,frame);
}


double
ColorParam::getValue(int dimension) const
{
    return _colorKnob.lock()->getValue(dimension);
}

void
ColorParam::setValue(double value,int dimension)
{
    _colorKnob.lock()->setValue(value, dimension);
}

double
ColorParam::getValueAtTime(int time,int dimension) const
{
    return _colorKnob.lock()->getValueAtTime(time,dimension);
}

void
ColorParam::setValueAtTime(double value,int time,int dimension)
{
    _colorKnob.lock()->setValueAtTime(time, value, dimension);
}

void
ColorParam::setDefaultValue(double value,int dimension)
{
    _colorKnob.lock()->setDefaultValue(value,dimension);
}

double
ColorParam::getDefaultValue(int dimension) const
{
    return _colorKnob.lock()->getDefaultValues_mt_safe()[dimension];
}

void
ColorParam::restoreDefaultValue(int dimension)
{
    _colorKnob.lock()->resetToDefaultValue(dimension);
}

void
ColorParam::setMinimum(double minimum,int dimension)
{
    _colorKnob.lock()->setMinimum(minimum,dimension);
}

double
ColorParam::getMinimum(int dimension) const
{
    return _colorKnob.lock()->getMinimum(dimension);
}

void
ColorParam::setMaximum(double maximum,int dimension)
{
    if (!_colorKnob.lock()->isUserKnob()) {
        return;
    }
    _colorKnob.lock()->setMaximum(maximum,dimension);
}

double
ColorParam::getMaximum(int dimension) const
{
    return _colorKnob.lock()->getMaximum(dimension);
}

void
ColorParam::setDisplayMinimum(double minimum,int dimension)
{
    if (!_colorKnob.lock()->isUserKnob()) {
        return;
    }
    _colorKnob.lock()->setDisplayMinimum(minimum,dimension);
}

double
ColorParam::getDisplayMinimum(int dimension) const
{
    return _colorKnob.lock()->getDisplayMinimum(dimension);
}

void
ColorParam::setDisplayMaximum(double maximum,int dimension)
{
    _colorKnob.lock()->setDisplayMaximum(maximum,dimension);
}


double
ColorParam::getDisplayMaximum(int dimension) const
{
    return _colorKnob.lock()->getDisplayMaximum(dimension);
}

double
ColorParam::addAsDependencyOf(int fromExprDimension,Param* param)
{
    _addAsDependencyOf(fromExprDimension, param);
    return _colorKnob.lock()->getValue();
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
    return _choiceKnob.lock()->getValue(0);
}

int
ChoiceParam::get(int frame) const
{
    return _choiceKnob.lock()->getValueAtTime(frame,0);
}


void
ChoiceParam::set(int x)
{
    _choiceKnob.lock()->setValue(x, 0);
}

void
ChoiceParam::set(int x, int frame)
{
    _choiceKnob.lock()->setValueAtTime(frame, x, 0);
}

void
ChoiceParam::set(const std::string& label)
{
    KnobHelper::ValueChangedReturnCodeEnum s = _choiceKnob.lock()->setValueFromLabel(label, 0);
    Q_UNUSED(s);
}

int
ChoiceParam::getValue() const
{
    return _choiceKnob.lock()->getValue(0);
}

void
ChoiceParam::setValue(int value)
{
    _choiceKnob.lock()->setValue(value, 0);
}

int
ChoiceParam::getValueAtTime(int time) const
{
    return _choiceKnob.lock()->getValueAtTime(time, 0);
}

void
ChoiceParam::setValueAtTime(int value,int time)
{
    _choiceKnob.lock()->setValueAtTime(time, value, 0);
}

void
ChoiceParam::setDefaultValue(int value)
{
    _choiceKnob.lock()->setDefaultValue(value,0);
}

void
ChoiceParam::setDefaultValue(const std::string& value)
{
    _choiceKnob.lock()->setDefaultValueFromLabel(value);
}

int
ChoiceParam::getDefaultValue() const
{
    return _choiceKnob.lock()->getDefaultValues_mt_safe()[0];
}

void
ChoiceParam::restoreDefaultValue()
{
    _choiceKnob.lock()->resetToDefaultValue(0);
}

void
ChoiceParam::addOption(const std::string& option,const std::string& help)
{
    boost::shared_ptr<Choice_Knob> knob = _choiceKnob.lock();
    if (!knob->isUserKnob()) {
        return;
    }
    std::vector<std::string> entries = knob->getEntries_mt_safe();
    std::vector<std::string> helps = knob->getEntriesHelp_mt_safe();
    entries.push_back(option);
    if (!help.empty()) {
        helps.push_back(help);
    }
    knob->populateChoices(entries,helps);
    
}

void
ChoiceParam::setOptions(const std::list<std::pair<std::string,std::string> >& options)
{
    boost::shared_ptr<Choice_Knob> knob = _choiceKnob.lock();
    if (!knob->isUserKnob()) {
        return;
    }
    
    std::vector<std::string> entries,helps;
    for (std::list<std::pair<std::string,std::string> >::const_iterator it = options.begin(); it != options.end(); ++it) {
        entries.push_back(it->first);
        helps.push_back(it->second);
    }
    knob->populateChoices(entries,helps);
}

std::string
ChoiceParam::getOption(int index) const
{
    std::vector<std::string> entries =  _choiceKnob.lock()->getEntries_mt_safe();
    if (index < 0 || index >= (int)entries.size()) {
        return std::string();
    }
    return entries[index];
}


int
ChoiceParam::getNumOptions() const
{
    return _choiceKnob.lock()->getNumEntries();
}

std::vector<std::string>
ChoiceParam::getOptions() const
{
    return _choiceKnob.lock()->getEntries_mt_safe();
}

int
ChoiceParam::addAsDependencyOf(int fromExprDimension,Param* param)
{
    _addAsDependencyOf(fromExprDimension, param);
    return _choiceKnob.lock()->getValue();
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
    return _boolKnob.lock()->getValue(0);
}

bool
BooleanParam::get(int frame) const
{
    return _boolKnob.lock()->getValueAtTime(frame,0);
}


void
BooleanParam::set(bool x)
{
    _boolKnob.lock()->setValue(x, 0);
}

void
BooleanParam::set(bool x, int frame)
{
    _boolKnob.lock()->setValueAtTime(frame, x, 0);
}

bool
BooleanParam::getValue() const
{
    return _boolKnob.lock()->getValue(0);
}

void
BooleanParam::setValue(bool value)
{
    _boolKnob.lock()->setValue(value, 0);
}

bool
BooleanParam::getValueAtTime(int time) const
{
    return _boolKnob.lock()->getValueAtTime(time,0);
}

void
BooleanParam::setValueAtTime(bool value,int time)
{
    _boolKnob.lock()->setValueAtTime(time, value, 0);
}

void
BooleanParam::setDefaultValue(bool value)
{
    _boolKnob.lock()->setDefaultValue(value,0);
}

bool
BooleanParam::getDefaultValue() const
{
    return _boolKnob.lock()->getDefaultValues_mt_safe()[0];
}

void
BooleanParam::restoreDefaultValue()
{
    _boolKnob.lock()->resetToDefaultValue(0);
}


bool
BooleanParam::addAsDependencyOf(int fromExprDimension,Param* param)
{
    _addAsDependencyOf(fromExprDimension, param);
    return _boolKnob.lock()->getValue();
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
    return _stringKnob.lock()->getValue(0);
}

std::string
StringParamBase::get(int frame) const
{
    return _stringKnob.lock()->getValueAtTime(frame,0);
}


void
StringParamBase::set(const std::string& x)
{
    _stringKnob.lock()->setValue(x, 0);
}

void
StringParamBase::set(const std::string& x, int frame)
{
    _stringKnob.lock()->setValueAtTime(frame, x, 0);
}

std::string
StringParamBase::getValue() const
{
    return _stringKnob.lock()->getValue(0);
}

void
StringParamBase::setValue(const std::string& value)
{
    _stringKnob.lock()->setValue(value, 0);
}

std::string
StringParamBase::getValueAtTime(int time) const
{
    return _stringKnob.lock()->getValueAtTime(time,0);
}

void
StringParamBase::setValueAtTime(const std::string& value,int time)
{
    _stringKnob.lock()->setValueAtTime(time, value, 0);
}

void
StringParamBase::setDefaultValue(const std::string& value)
{
    _stringKnob.lock()->setDefaultValue(value,0);
}

std::string
StringParamBase::getDefaultValue() const
{
    return _stringKnob.lock()->getDefaultValues_mt_safe()[0];
}

void
StringParamBase::restoreDefaultValue()
{
    _stringKnob.lock()->resetToDefaultValue(0);
}


std::string
StringParamBase::addAsDependencyOf(int fromExprDimension,Param* param)
{
    _addAsDependencyOf(fromExprDimension, param);
    return _stringKnob.lock()->getValue();
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
    boost::shared_ptr<String_Knob> knob = _sKnob.lock();
    if (!knob->isUserKnob()) {
        return;
    }
    switch (type) {
        case eStringTypeLabel:
            knob->setAsLabel();
            break;
        case eStringTypeMultiLine:
            knob->setAsMultiLine();
            break;
        case eStringTypeRichTextMultiLine:
            knob->setAsMultiLine();
            knob->setUsesRichText(true);
            break;
        case eStringTypeCustom:
            knob->setAsCustom();
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
    boost::shared_ptr<OutputFile_Knob> knob = _sKnob.lock();
    if (!knob->isUserKnob()) {
        return;
    }
    if (enabled) {
        knob->setAsOutputImageFile();
    } else {
        knob->turnOffSequences();
    }
}

void
OutputFileParam::openFile()
{
    _sKnob.lock()->open_file();
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
    if (!_sKnob.lock()->isUserKnob()) {
        return;
    }
    _sKnob.lock()->setMultiPath(true);
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
    _buttonKnob.lock()->setIconFilePath(icon);
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
    _groupKnob.lock()->addKnob(param->getInternalKnob());
}


void
GroupParam::setAsTab()
{
    if (!_groupKnob.lock()->isUserKnob()) {
        return;
    }
    _groupKnob.lock()->setAsTab();
}

void
GroupParam::setOpened(bool opened)
{
    _groupKnob.lock()->setValue(opened, 0);
}

bool
GroupParam::getIsOpened() const
{
    return _groupKnob.lock()->getValue();
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
    _pageKnob.lock()->addKnob(param->getInternalKnob());
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
    _parametricKnob.lock()->setCurveColor(dimension, r, g, b);
}

void
ParametricParam::getCurveColor(int dimension, ColorTuple& ret) const
{
    _parametricKnob.lock()->getCurveColor(dimension, &ret.r, &ret.g, &ret.b);
    ret.a = 1.;
}

Natron::StatusEnum
ParametricParam::addControlPoint(int dimension,double key,double value)
{
    return _parametricKnob.lock()->addControlPoint(dimension, key, value);
}

double
ParametricParam::getValue(int dimension,double parametricPosition) const
{
    double ret;
    Natron::StatusEnum stat =  _parametricKnob.lock()->getValue(dimension, parametricPosition, &ret);
    if (stat == Natron::eStatusFailed) {
        ret =  0.;
    }
    return ret;
}

int
ParametricParam::getNControlPoints(int dimension) const
{
    int ret;
    Natron::StatusEnum stat =  _parametricKnob.lock()->getNControlPoints(dimension, &ret);
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
    return _parametricKnob.lock()->getNthControlPoint(dimension, nthCtl, key, value,leftDerivative, rightDerivative);
}

Natron::StatusEnum
ParametricParam::setNthControlPoint(int dimension,
                                    int nthCtl,
                                    double key,
                                    double value,
                                    double leftDerivative,
                                    double rightDerivative)
{
    return _parametricKnob.lock()->setNthControlPoint(dimension, nthCtl, key, value,leftDerivative,rightDerivative);
}

Natron::StatusEnum
ParametricParam::deleteControlPoint(int dimension, int nthCtl)
{
    return _parametricKnob.lock()->deleteControlPoint(dimension, nthCtl);
}

Natron::StatusEnum
ParametricParam::deleteAllControlPoints(int dimension)
{
    return _parametricKnob.lock()->deleteAllControlPoints(dimension);
}

