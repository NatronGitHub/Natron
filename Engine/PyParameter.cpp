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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "PyParameter.h"

#include <cassert>
#include <stdexcept>

#include "Engine/AppInstance.h"
#include "Engine/EffectInstance.h"
#include "Engine/Node.h"
#include "Engine/Curve.h"
#include "Engine/ViewIdx.h"

NATRON_NAMESPACE_ENTER;
NATRON_PYTHON_NAMESPACE_ENTER;

Param::Param(const KnobIPtr& knob)
    : _knob(knob)
{
}

Param::~Param()
{
}

Param*
Param::getParent() const
{
    KnobIPtr parent = getInternalKnob()->getParentKnob();

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

QString
Param::getScriptName() const
{
    return QString::fromUtf8( getInternalKnob()->getName().c_str() );
}

QString
Param::getLabel() const
{
    return QString::fromUtf8( getInternalKnob()->getLabel().c_str() );
}

QString
Param::getTypeName() const
{
    return QString::fromUtf8( getInternalKnob()->typeName().c_str() );
}

QString
Param::getHelp() const
{
    return QString::fromUtf8( getInternalKnob()->getHintToolTip().c_str() );
}

void
Param::setHelp(const QString& help)
{
    if ( !getInternalKnob()->isUserKnob() ) {
        return;
    }
    getInternalKnob()->setHintToolTip( help.toStdString() );
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
Param::setEnabled(bool enabled,
                  int dimension)
{
    getInternalKnob()->setEnabled(dimension, enabled);
}

bool
Param::getIsPersistent() const
{
    return getInternalKnob()->getIsPersistent();
}

void
Param::setPersistent(bool persistent)
{
    if ( !getInternalKnob()->isUserKnob() ) {
        return;
    }
    getInternalKnob()->setIsPersistent(persistent);
}

bool
Param::getEvaluateOnChange() const
{
    return getInternalKnob()->getEvaluateOnChange();
}

void
Param::setEvaluateOnChange(bool eval)
{
    if ( !getInternalKnob()->isUserKnob() ) {
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
    if ( !getInternalKnob()->isUserKnob() ) {
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
    KnobIPtr knob = getInternalKnob();

    if ( !knob || !knob->isUserKnob() ) {
        return;
    }

    KnobIPtr parentKnob = knob->getParentKnob();
    if (parentKnob) {
        KnobGroupPtr parentIsGrp = toKnobGroup(parentKnob);
        KnobPagePtr parentIsPage = toKnobPage(parentKnob);
        assert(parentIsGrp || parentIsPage);
        KnobsVec children;
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
Param::getHasViewerUI() const
{
    KnobIPtr knob = getInternalKnob();

    if (!knob) {
        return false;
    }
    KnobHolderPtr holder = knob->getHolder();
    if (!holder) {
        return false;
    }
    return holder->getInViewerContextKnobIndex(knob) != -1;
}

void
Param::setViewerUIVisible(bool visible)
{
    KnobIPtr knob = getInternalKnob();

    if (!knob) {
        return;
    }
    knob->setInViewerContextSecret(!visible);
}


bool
Param::getViewerUIVisible() const
{
    KnobIPtr knob = getInternalKnob();

    if (!knob) {
        return false;
    }
    return knob->getInViewerContextSecret();
}

void
Param::setViewerUILayoutType(NATRON_NAMESPACE::ViewerContextLayoutTypeEnum type)
{
    KnobIPtr knob = getInternalKnob();

    if (!knob) {
        return;
    }
    knob->setInViewerContextLayoutType(type);
}

NATRON_NAMESPACE::ViewerContextLayoutTypeEnum
Param::getViewerUILayoutType() const
{
    KnobIPtr knob = getInternalKnob();

    if (!knob) {
        return eViewerContextLayoutTypeSpacing;
    }
    return knob->getInViewerContextLayoutType();
}


void
Param::setViewerUIItemSpacing(int spacingPx)
{
    KnobIPtr knob = getInternalKnob();

    if (!knob) {
        return;
    }
    knob->setInViewerContextItemSpacing(spacingPx);
}

int
Param::getViewerUIItemSpacing() const
{
    KnobIPtr knob = getInternalKnob();

    if (!knob) {
        return 0;
    }
    return knob->getInViewerContextItemSpacing();
}


void
Param::setViewerUIIconFilePath(const QString& icon, bool checked)
{
    KnobIPtr knob = getInternalKnob();

    if (!knob || !knob->isUserKnob()) {
        return;
    }
    knob->setInViewerContextIconFilePath(icon.toStdString(), checked);
}

QString
Param::getViewerUIIconFilePath(bool checked) const
{
    KnobIPtr knob = getInternalKnob();

    if (!knob) {
        return QString();
    }
    return QString::fromUtf8(knob->getInViewerContextIconFilePath(checked).c_str());
}


void
Param::setViewerUILabel(const QString& label)
{
    KnobIPtr knob = getInternalKnob();

    if (!knob || !knob->isUserKnob()) {
        return;
    }
    knob->setInViewerContextLabel(label);
}

QString
Param::getViewerUILabel() const
{
    KnobIPtr knob = getInternalKnob();

    if (!knob) {
        return QString();
    }
    return QString::fromUtf8(knob->getInViewerContextLabel().c_str());
}

bool
Param::copy(Param* other,
            int dimension)
{
    KnobIPtr thisKnob = _knob.lock();
    KnobIPtr otherKnob = other->_knob.lock();

    if ( !thisKnob->isTypeCompatible(otherKnob) ) {
        return false;
    }
    thisKnob->cloneAndUpdateGui(otherKnob, dimension);

    return true;
}

bool
Param::slaveTo(Param* other,
               int thisDimension,
               int otherDimension)
{
    KnobIPtr thisKnob = _knob.lock();
    KnobIPtr otherKnob = other->_knob.lock();

    if ( !KnobI::areTypesCompatibleForSlave(thisKnob, otherKnob) ) {
        return false;
    }
    if ( (thisDimension < 0) || ( thisDimension >= thisKnob->getDimension() ) || (otherDimension < 0) || ( otherDimension >= otherKnob->getDimension() ) ) {
        return false;
    }

    return thisKnob->slaveTo(thisDimension, otherKnob, otherDimension);
}

void
Param::unslave(int dimension)
{
    KnobIPtr thisKnob = _knob.lock();

    if (!thisKnob) {
        return;
    }
    thisKnob->unSlave(dimension, false);
}

double
Param::random(double min,
              double max) const
{
    if ( !getInternalKnob() ) {
        return 0;
    }

    return getInternalKnob()->random(min, max);
}

double
Param::random(unsigned int seed) const
{
    if ( !getInternalKnob() ) {
        return 0;
    }

    return getInternalKnob()->random(seed);
}

int
Param::randomInt(int min,
                 int max)
{
    if ( !getInternalKnob() ) {
        return 0;
    }

    return getInternalKnob()->randomInt(min, max);
}

int
Param::randomInt(unsigned int seed) const
{
    if ( !getInternalKnob() ) {
        return 0;
    }

    return getInternalKnob()->randomInt(seed);
}

double
Param::curve(double time,
             int dimension) const
{
    if ( !getInternalKnob() ) {
        return 0.;
    }

    return getInternalKnob()->getRawCurveValueAt(time, ViewSpec::current(), dimension);
}

bool
Param::setAsAlias(Param* other)
{
    if (!other) {
        return false;
    }
    KnobIPtr otherKnob = other->_knob.lock();
    KnobIPtr thisKnob = getInternalKnob();
    if ( !otherKnob || !thisKnob || ( otherKnob->typeName() != thisKnob->typeName() ) ||
         ( otherKnob->getDimension() != thisKnob->getDimension() ) ) {
        return false;
    }

    return otherKnob->setKnobAsAliasOfThis(thisKnob, true);
}

void
Param::setIconFilePath(const QString& icon, bool checked)
{
    _knob.lock()->setIconLabel( icon.toStdString(), checked, false );
}

AnimatedParam::AnimatedParam(const KnobIPtr& knob)
    : Param(knob)
{
}

AnimatedParam::~AnimatedParam()
{
}

bool
AnimatedParam::getIsAnimated(int dimension) const
{
    return getInternalKnob()->isAnimated( dimension, ViewSpec::current() );
}

int
AnimatedParam::getNumKeys(int dimension) const
{
    return getInternalKnob()->getKeyFramesCount(ViewSpec::current(), dimension);
}

int
AnimatedParam::getKeyIndex(double time,
                           int dimension) const
{
    return getInternalKnob()->getKeyFrameIndex(ViewSpec::current(), dimension, time);
}

bool
AnimatedParam::getKeyTime(int index,
                          int dimension,
                          double* time) const
{
    return getInternalKnob()->getKeyFrameTime(ViewSpec::current(), index, dimension, time);
}

void
AnimatedParam::deleteValueAtTime(double time,
                                 int dimension)
{
    getInternalKnob()->deleteValueAtTime(eCurveChangeReasonInternal, time, ViewSpec::all(), dimension, false);
}

void
AnimatedParam::removeAnimation(int dimension)
{
    getInternalKnob()->removeAnimation(ViewSpec::all(), dimension);
}

double
AnimatedParam::getDerivativeAtTime(double time,
                                   int dimension) const
{
    return getInternalKnob()->getDerivativeAtTime(time, ViewSpec::current(), dimension);
}

double
AnimatedParam::getIntegrateFromTimeToTime(double time1,
                                          double time2,
                                          int dimension) const
{
    return getInternalKnob()->getIntegrateFromTimeToTime(time1, time2, ViewSpec::current(), dimension);
}

int
AnimatedParam::getCurrentTime() const
{
    return getInternalKnob()->getCurrentTime();
}

bool
AnimatedParam::setInterpolationAtTime(double time,
                                      KeyframeTypeEnum interpolation,
                                      int dimension)
{
    KnobIPtr knob = getInternalKnob();

    if (!knob) {
        return false;
    }
    KeyFrame newKey;

    return knob->setInterpolationAtTime(eCurveChangeReasonInternal, ViewSpec::current(), dimension, time, interpolation, &newKey);
}

void
Param::_addAsDependencyOf(int fromExprDimension,
                          Param* param,
                          int thisDimension)
{
    //from expr is in the dimension of expressionKnob
    //thisDimension is in the dimesnion of getValueCallerKnob

    KnobIPtr expressionKnob = param->_knob.lock();
    KnobIPtr getValueCallerKnob = _knob.lock();

    if ( (fromExprDimension < 0) || ( fromExprDimension >= expressionKnob->getDimension() ) ) {
        return;
    }
    if ( (thisDimension != -1) && (thisDimension != 0) && ( thisDimension >= getValueCallerKnob->getDimension() ) ) {
        return;
    }
    if (getValueCallerKnob == expressionKnob) {
        return;
    }

    getValueCallerKnob->addListener(true, fromExprDimension, thisDimension, expressionKnob);
}

bool
AnimatedParam::setExpression(const QString& expr,
                             bool hasRetVariable,
                             int dimension)
{
    try {
        _knob.lock()->setExpression(dimension, expr.toStdString(), hasRetVariable, true);
    } catch (...) {
        return false;
    }

    return true;
}

QString
AnimatedParam::getExpression(int dimension,
                             bool* hasRetVariable) const
{
    QString ret = QString::fromUtf8( _knob.lock()->getExpression(dimension).c_str() );

    *hasRetVariable = _knob.lock()->isExpressionUsingRetVariable(dimension);

    return ret;
}

///////////// IntParam

IntParam::IntParam(const KnobIntPtr& knob)
    : AnimatedParam( boost::dynamic_pointer_cast<KnobI>(knob) )
    , _intKnob(knob)
{
}

IntParam::~IntParam()
{
}

int
IntParam::get() const
{
    KnobIntPtr knob = _intKnob.lock();

    return knob->getValue();
}

Int2DTuple
Int2DParam::get() const
{
    Int2DTuple ret;
    KnobIntPtr knob = _intKnob.lock();

    ret.x = knob->getValue(0);
    ret.y = knob->getValue(1);

    return ret;
}

Int3DTuple
Int3DParam::get() const
{
    KnobIntPtr knob = _intKnob.lock();
    Int3DTuple ret;

    ret.x = knob->getValue(0);
    ret.y = knob->getValue(1);
    ret.z = knob->getValue(2);

    return ret;
}

int
IntParam::get(double frame) const
{
    KnobIntPtr knob = _intKnob.lock();

    return knob->getValueAtTime(frame, 0);
}

Int2DTuple
Int2DParam::get(double frame) const
{
    Int2DTuple ret;
    KnobIntPtr knob = _intKnob.lock();

    ret.x = knob->getValueAtTime(frame, 0);
    ret.y = knob->getValueAtTime(frame, 1);

    return ret;
}

Int3DTuple
Int3DParam::get(double frame) const
{
    Int3DTuple ret;
    KnobIntPtr knob = _intKnob.lock();

    ret.x = knob->getValueAtTime(frame, 0);
    ret.y = knob->getValueAtTime(frame, 1);
    ret.z = knob->getValueAtTime(frame, 2);

    return ret;
}

void
IntParam::set(int x)
{
    _intKnob.lock()->setValue(x, ViewSpec::current(), 0);
}

void
Int2DParam::set(int x,
                int y)
{
    KnobIntPtr knob = _intKnob.lock();

    knob->beginChanges();
    knob->setValue(x, ViewSpec::current(), 0);
    knob->setValue(y, ViewSpec::current(), 1);
    knob->endChanges();
}

void
Int3DParam::set(int x,
                int y,
                int z)
{
    KnobIntPtr knob = _intKnob.lock();

    knob->beginChanges();
    knob->setValue(x, ViewSpec::current(), 0);
    knob->setValue(y, ViewSpec::current(), 1);
    knob->setValue(z, ViewSpec::current(), 2);
    knob->endChanges();
}

void
IntParam::set(int x,
              double frame)
{
    _intKnob.lock()->setValueAtTime(frame, x, ViewSpec::current(), 0);
}

void
Int2DParam::set(int x,
                int y,
                double frame)
{
    KnobIntPtr knob = _intKnob.lock();

    knob->setValuesAtTime(frame, x, y, ViewSpec::current(), eValueChangedReasonNatronInternalEdited);
}

void
Int3DParam::set(int x,
                int y,
                int z,
                double frame)
{
    KnobIntPtr knob = _intKnob.lock();

    knob->setValuesAtTime(frame, x, y, z, ViewSpec::current(), eValueChangedReasonNatronInternalEdited);
}

int
IntParam::getValue(int dimension) const
{
    return _intKnob.lock()->getValue(dimension);
}

void
IntParam::setValue(int value,
                   int dimension)
{
    _intKnob.lock()->setValue(value, ViewSpec::current(), dimension);
}

int
IntParam::getValueAtTime(double time,
                         int dimension) const
{
    return _intKnob.lock()->getValueAtTime(time, dimension);
}

void
IntParam::setValueAtTime(int value,
                         double time,
                         int dimension)
{
    _intKnob.lock()->setValueAtTime(time, value, ViewSpec::current(), dimension);
}

void
IntParam::setDefaultValue(int value,
                          int dimension)
{
    _intKnob.lock()->setDefaultValueWithoutApplying(value, dimension);
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
IntParam::setMinimum(int minimum,
                     int dimension)
{
    _intKnob.lock()->setMinimum(minimum, dimension);
}

int
IntParam::getMinimum(int dimension) const
{
    return _intKnob.lock()->getMinimum(dimension);
}

void
IntParam::setMaximum(int maximum,
                     int dimension)
{
    if ( !_intKnob.lock()->isUserKnob() ) {
        return;
    }
    _intKnob.lock()->setMaximum(maximum, dimension);
}

int
IntParam::getMaximum(int dimension) const
{
    return _intKnob.lock()->getMaximum(dimension);
}

void
IntParam::setDisplayMinimum(int minimum,
                            int dimension)
{
    if ( !_intKnob.lock()->isUserKnob() ) {
        return;
    }

    return _intKnob.lock()->setDisplayMinimum(minimum, dimension);
}

int
IntParam::getDisplayMinimum(int dimension) const
{
    return _intKnob.lock()->getDisplayMinimum(dimension);
}

void
IntParam::setDisplayMaximum(int maximum,
                            int dimension)
{
    _intKnob.lock()->setDisplayMaximum(maximum, dimension);
}

int
IntParam::getDisplayMaximum(int dimension) const
{
    return _intKnob.lock()->getDisplayMaximum(dimension);
}

int
IntParam::addAsDependencyOf(int fromExprDimension,
                            Param* param,
                            int thisDimension)
{
    _addAsDependencyOf(fromExprDimension, param, thisDimension);

    return _intKnob.lock()->getValue();
}

//////////// DoubleParam

DoubleParam::DoubleParam(const KnobDoublePtr& knob)
    : AnimatedParam( boost::dynamic_pointer_cast<KnobI>(knob) )
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
    KnobDoublePtr knob = _doubleKnob.lock();
    Double2DTuple ret;

    ret.x = knob->getValue(0);
    ret.y = knob->getValue(1);

    return ret;
}

Double3DTuple
Double3DParam::get() const
{
    KnobDoublePtr knob = _doubleKnob.lock();
    Double3DTuple ret;

    ret.x = knob->getValue(0);
    ret.y = knob->getValue(1);
    ret.z = knob->getValue(2);

    return ret;
}

double
DoubleParam::get(double frame) const
{
    return _doubleKnob.lock()->getValueAtTime(frame, 0);
}

Double2DTuple
Double2DParam::get(double frame) const
{
    Double2DTuple ret;
    KnobDoublePtr knob = _doubleKnob.lock();

    ret.x = knob->getValueAtTime(frame, 0);
    ret.y = knob->getValueAtTime(frame, 1);

    return ret;
}

Double3DTuple
Double3DParam::get(double frame) const
{
    KnobDoublePtr knob = _doubleKnob.lock();
    Double3DTuple ret;

    ret.x = knob->getValueAtTime(frame, 0);
    ret.y = knob->getValueAtTime(frame, 1);
    ret.z = knob->getValueAtTime(frame, 2);

    return ret;
}

void
DoubleParam::set(double x)
{
    _doubleKnob.lock()->setValue(x, ViewSpec::current(), 0);
}

void
Double2DParam::set(double x,
                   double y)
{
    KnobDoublePtr knob = _doubleKnob.lock();

    knob->setValues(x, y, ViewSpec::current(), eValueChangedReasonNatronInternalEdited);
}

void
Double3DParam::set(double x,
                   double y,
                   double z)
{
    KnobDoublePtr knob = _doubleKnob.lock();

    knob->setValues(x, y, z, ViewSpec::current(), eValueChangedReasonNatronInternalEdited);
}

void
DoubleParam::set(double x,
                 double frame)
{
    _doubleKnob.lock()->setValueAtTime(frame, x, ViewSpec::current(), 0);
}

void
Double2DParam::set(double x,
                   double y,
                   double frame)
{
    KnobDoublePtr knob = _doubleKnob.lock();

    knob->setValuesAtTime(frame, x, y, ViewSpec::current(), eValueChangedReasonNatronInternalEdited);
}

void
Double2DParam::setUsePointInteract(bool use)
{
    if ( !_doubleKnob.lock() ) {
        return;
    }
    _doubleKnob.lock()->setHasHostOverlayHandle(use);
}

void
Double3DParam::set(double x,
                   double y,
                   double z,
                   double frame)
{
    KnobDoublePtr knob = _doubleKnob.lock();

    knob->setValuesAtTime(frame, x, y, z, ViewSpec::current(), eValueChangedReasonNatronInternalEdited);
}

double
DoubleParam::getValue(int dimension) const
{
    return _doubleKnob.lock()->getValue(dimension);
}

void
DoubleParam::setValue(double value,
                      int dimension)
{
    _doubleKnob.lock()->setValue(value, ViewSpec::current(), dimension);
}

double
DoubleParam::getValueAtTime(double time,
                            int dimension) const
{
    return _doubleKnob.lock()->getValueAtTime(time, dimension);
}

void
DoubleParam::setValueAtTime(double value,
                            double time,
                            int dimension)
{
    _doubleKnob.lock()->setValueAtTime(time, value, ViewSpec::current(), dimension);
}

void
DoubleParam::setDefaultValue(double value,
                             int dimension)
{
    _doubleKnob.lock()->setDefaultValueWithoutApplying(value, dimension);
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
DoubleParam::setMinimum(double minimum,
                        int dimension)
{
    _doubleKnob.lock()->setMinimum(minimum, dimension);
}

double
DoubleParam::getMinimum(int dimension) const
{
    return _doubleKnob.lock()->getMinimum(dimension);
}

void
DoubleParam::setMaximum(double maximum,
                        int dimension)
{
    if ( !_doubleKnob.lock()->isUserKnob() ) {
        return;
    }
    _doubleKnob.lock()->setMaximum(maximum, dimension);
}

double
DoubleParam::getMaximum(int dimension) const
{
    return _doubleKnob.lock()->getMaximum(dimension);
}

void
DoubleParam::setDisplayMinimum(double minimum,
                               int dimension)
{
    if ( !_doubleKnob.lock()->isUserKnob() ) {
        return;
    }
    _doubleKnob.lock()->setDisplayMinimum(minimum, dimension);
}

double
DoubleParam::getDisplayMinimum(int dimension) const
{
    return _doubleKnob.lock()->getDisplayMinimum(dimension);
}

void
DoubleParam::setDisplayMaximum(double maximum,
                               int dimension)
{
    _doubleKnob.lock()->setDisplayMaximum(maximum, dimension);
}

double
DoubleParam::getDisplayMaximum(int dimension) const
{
    return _doubleKnob.lock()->getDisplayMaximum(dimension);
}

double
DoubleParam::addAsDependencyOf(int fromExprDimension,
                               Param* param,
                               int thisDimension)
{
    _addAsDependencyOf(fromExprDimension, param, thisDimension);

    return _doubleKnob.lock()->getValue();
}

////////ColorParam

ColorParam::ColorParam(const KnobColorPtr& knob)
    : AnimatedParam( boost::dynamic_pointer_cast<KnobI>(knob) )
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
    KnobColorPtr knob = _colorKnob.lock();

    ret.r = knob->getValue(0);
    ret.g = knob->getValue(1);
    ret.b = knob->getValue(2);
    ret.a = knob->getDimension() == 4 ? knob->getValue(3) : 1.;

    return ret;
}

ColorTuple
ColorParam::get(double frame) const
{
    ColorTuple ret;
    KnobColorPtr knob = _colorKnob.lock();

    ret.r = knob->getValueAtTime(frame, 0);
    ret.g = knob->getValueAtTime(frame, 1);
    ret.b = knob->getValueAtTime(frame, 2);
    ret.a = knob->getDimension() == 4 ? knob->getValueAtTime(frame, 2) : 1.;

    return ret;
}

void
ColorParam::set(double r,
                double g,
                double b,
                double a)
{
    KnobColorPtr knob = _colorKnob.lock();

    knob->beginChanges();
    knob->setValue(r, ViewSpec::current(), 0);
    knob->setValue(g, ViewSpec::current(), 1);
    knob->setValue(b, ViewSpec::current(), 2);
    if (knob->getDimension() == 4) {
        knob->setValue(a, ViewSpec::current(), 3);
    }
    knob->endChanges();
}

void
ColorParam::set(double r,
                double g,
                double b,
                double a,
                double frame)
{
    KnobColorPtr knob = _colorKnob.lock();

    knob->beginChanges();
    knob->setValueAtTime(frame, r, ViewSpec::current(), 0);
    knob->setValueAtTime(frame, g, ViewSpec::current(), 1);
    int dims = knob->getDimension();
    knob->setValueAtTime(frame, b, ViewSpec::current(), 2);
    if (dims == 4) {
        knob->setValueAtTime(frame, a, ViewSpec::current(), 3);
    }
    knob->endChanges();
}

double
ColorParam::getValue(int dimension) const
{
    return _colorKnob.lock()->getValue(dimension);
}

void
ColorParam::setValue(double value,
                     int dimension)
{
    _colorKnob.lock()->setValue(value, ViewSpec::current(), dimension);
}

double
ColorParam::getValueAtTime(double time,
                           int dimension) const
{
    return _colorKnob.lock()->getValueAtTime(time, dimension);
}

void
ColorParam::setValueAtTime(double value,
                           double time,
                           int dimension)
{
    _colorKnob.lock()->setValueAtTime(time, value, ViewSpec::current(), dimension);
}

void
ColorParam::setDefaultValue(double value,
                            int dimension)
{
    _colorKnob.lock()->setDefaultValueWithoutApplying(value, dimension);
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
ColorParam::setMinimum(double minimum,
                       int dimension)
{
    _colorKnob.lock()->setMinimum(minimum, dimension);
}

double
ColorParam::getMinimum(int dimension) const
{
    return _colorKnob.lock()->getMinimum(dimension);
}

void
ColorParam::setMaximum(double maximum,
                       int dimension)
{
    if ( !_colorKnob.lock()->isUserKnob() ) {
        return;
    }
    _colorKnob.lock()->setMaximum(maximum, dimension);
}

double
ColorParam::getMaximum(int dimension) const
{
    return _colorKnob.lock()->getMaximum(dimension);
}

void
ColorParam::setDisplayMinimum(double minimum,
                              int dimension)
{
    if ( !_colorKnob.lock()->isUserKnob() ) {
        return;
    }
    _colorKnob.lock()->setDisplayMinimum(minimum, dimension);
}

double
ColorParam::getDisplayMinimum(int dimension) const
{
    return _colorKnob.lock()->getDisplayMinimum(dimension);
}

void
ColorParam::setDisplayMaximum(double maximum,
                              int dimension)
{
    _colorKnob.lock()->setDisplayMaximum(maximum, dimension);
}

double
ColorParam::getDisplayMaximum(int dimension) const
{
    return _colorKnob.lock()->getDisplayMaximum(dimension);
}

double
ColorParam::addAsDependencyOf(int fromExprDimension,
                              Param* param,
                              int thisDimension)
{
    _addAsDependencyOf(fromExprDimension, param, thisDimension);

    return _colorKnob.lock()->getValue();
}

//////////////// ChoiceParam
ChoiceParam::ChoiceParam(const KnobChoicePtr& knob)
    : AnimatedParam( boost::dynamic_pointer_cast<KnobI>(knob) )
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
ChoiceParam::get(double frame) const
{
    return _choiceKnob.lock()->getValueAtTime(frame, 0);
}

void
ChoiceParam::set(int x)
{
    _choiceKnob.lock()->setValue(x, ViewSpec::current(), 0);
}

void
ChoiceParam::set(int x,
                 double frame)
{
    _choiceKnob.lock()->setValueAtTime(frame, x, ViewSpec::current(), 0);
}

void
ChoiceParam::set(const QString& label)
{
    KnobChoicePtr k = _choiceKnob.lock();
    if (!k) {
        return;
    }
    try {
        KnobHelper::ValueChangedReturnCodeEnum s = k->setValueFromLabel(label.toStdString(), 0);
        Q_UNUSED(s);
    } catch (const std::exception& e) {
        KnobHolderPtr holder =  k->getHolder();
        AppInstancePtr app;
        if (holder) {
            app = holder->getApp();
        }
        if (app) {
            app->appendToScriptEditor(e.what());
        } else {
            std::cerr << e.what() << std::endl;
        }
    }

}

int
ChoiceParam::getValue() const
{
    return _choiceKnob.lock()->getValue(0);
}

void
ChoiceParam::setValue(int value)
{
    _choiceKnob.lock()->setValue(value, ViewSpec::current(), 0);
}

int
ChoiceParam::getValueAtTime(double time) const
{
    return _choiceKnob.lock()->getValueAtTime(time, 0);
}

void
ChoiceParam::setValueAtTime(int value,
                            double time)
{
    _choiceKnob.lock()->setValueAtTime(time, value, ViewSpec::current(), 0);
}

void
ChoiceParam::setDefaultValue(int value)
{
    _choiceKnob.lock()->setDefaultValueWithoutApplying(value, 0);
}

void
ChoiceParam::setDefaultValue(const QString& value)
{
    _choiceKnob.lock()->setDefaultValueFromLabelWithoutApplying( value.toStdString() );
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
ChoiceParam::addOption(const QString& option,
                       const QString& help)
{
    KnobChoicePtr knob = _choiceKnob.lock();

    if ( !knob->isUserKnob() ) {
        return;
    }
    std::vector<std::string> entries = knob->getEntries_mt_safe();
    std::vector<std::string> helps = knob->getEntriesHelp_mt_safe();
    knob->appendChoice( option.toStdString(), help.toStdString() );
}

void
ChoiceParam::setOptions(const std::list<std::pair<QString, QString> >& options)
{
    KnobChoicePtr knob = _choiceKnob.lock();

    if ( !knob->isUserKnob() ) {
        return;
    }

    std::vector<std::string> entries, helps;
    for (std::list<std::pair<QString, QString> >::const_iterator it = options.begin(); it != options.end(); ++it) {
        entries.push_back( it->first.toStdString() );
        helps.push_back( it->second.toStdString() );
    }
    knob->populateChoices(entries, helps);
}

QString
ChoiceParam::getOption(int index) const
{
    std::vector<std::string> entries =  _choiceKnob.lock()->getEntries_mt_safe();

    if ( (index < 0) || ( index >= (int)entries.size() ) ) {
        return QString();
    }

    return QString::fromUtf8( entries[index].c_str() );
}

int
ChoiceParam::getNumOptions() const
{
    return _choiceKnob.lock()->getNumEntries();
}

QStringList
ChoiceParam::getOptions() const
{
    QStringList ret;
    std::vector<std::string> entries = _choiceKnob.lock()->getEntries_mt_safe();

    for (std::size_t i = 0; i < entries.size(); ++i) {
        ret.push_back( QString::fromUtf8( entries[i].c_str() ) );
    }

    return ret;
}

int
ChoiceParam::addAsDependencyOf(int fromExprDimension,
                               Param* param,
                               int thisDimension)
{
    _addAsDependencyOf(fromExprDimension, param, thisDimension);

    return _choiceKnob.lock()->getValue();
}

////////////////BooleanParam


BooleanParam::BooleanParam(const KnobBoolPtr& knob)
    : AnimatedParam( boost::dynamic_pointer_cast<KnobI>(knob) )
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
BooleanParam::get(double frame) const
{
    return _boolKnob.lock()->getValueAtTime(frame, 0);
}

void
BooleanParam::set(bool x)
{
    _boolKnob.lock()->setValue(x, ViewSpec::current(), 0);
}

void
BooleanParam::set(bool x,
                  double frame)
{
    _boolKnob.lock()->setValueAtTime(frame, x, ViewSpec::current(), 0);
}

bool
BooleanParam::getValue() const
{
    return _boolKnob.lock()->getValue(0);
}

void
BooleanParam::setValue(bool value)
{
    _boolKnob.lock()->setValue(value, ViewSpec::current(), 0);
}

bool
BooleanParam::getValueAtTime(double time) const
{
    return _boolKnob.lock()->getValueAtTime(time, 0);
}

void
BooleanParam::setValueAtTime(bool value,
                             double time)
{
    _boolKnob.lock()->setValueAtTime(time, value, ViewSpec::current(), 0);
}

void
BooleanParam::setDefaultValue(bool value)
{
    _boolKnob.lock()->setDefaultValueWithoutApplying(value, 0);
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
BooleanParam::addAsDependencyOf(int fromExprDimension,
                                Param* param,
                                int thisDimension)
{
    _addAsDependencyOf(fromExprDimension, param, thisDimension);

    return _boolKnob.lock()->getValue();
}

////////////// StringParamBase


StringParamBase::StringParamBase(const KnobStringBasePtr& knob)
    : AnimatedParam( boost::dynamic_pointer_cast<KnobI>(knob) )
    , _stringKnob(knob)
{
}

StringParamBase::~StringParamBase()
{
}

QString
StringParamBase::get() const
{
    return QString::fromUtf8( _stringKnob.lock()->getValue(0).c_str() );
}

QString
StringParamBase::get(double frame) const
{
    return QString::fromUtf8( _stringKnob.lock()->getValueAtTime(frame, 0).c_str() );
}

void
StringParamBase::set(const QString& x)
{
    _stringKnob.lock()->setValue(x.toStdString(), ViewSpec::current(), 0);
}

void
StringParamBase::set(const QString& x,
                     double frame)
{
    _stringKnob.lock()->setValueAtTime(frame, x.toStdString(), ViewSpec::current(), 0);
}

QString
StringParamBase::getValue() const
{
    return QString::fromUtf8( _stringKnob.lock()->getValue(0).c_str() );
}

void
StringParamBase::setValue(const QString& value)
{
    _stringKnob.lock()->setValue(value.toStdString(), ViewSpec::current(), 0);
}

QString
StringParamBase::getValueAtTime(double time) const
{
    return QString::fromUtf8( _stringKnob.lock()->getValueAtTime(time, 0).c_str() );
}

void
StringParamBase::setValueAtTime(const QString& value,
                                double time)
{
    _stringKnob.lock()->setValueAtTime(time, value.toStdString(), ViewSpec::current(), 0);
}

void
StringParamBase::setDefaultValue(const QString& value)
{
    _stringKnob.lock()->setDefaultValueWithoutApplying(value.toStdString(), 0);
}

QString
StringParamBase::getDefaultValue() const
{
    return QString::fromUtf8( _stringKnob.lock()->getDefaultValues_mt_safe()[0].c_str() );
}

void
StringParamBase::restoreDefaultValue()
{
    _stringKnob.lock()->resetToDefaultValue(0);
}

QString
StringParamBase::addAsDependencyOf(int fromExprDimension,
                                   Param* param,
                                   int thisDimension)
{
    _addAsDependencyOf(fromExprDimension, param, thisDimension);

    return QString::fromUtf8( _stringKnob.lock()->getValue().c_str() );
}

////////////////////StringParam

StringParam::StringParam(const KnobStringPtr& knob)
    : StringParamBase( toKnobStringBase(knob) )
    , _sKnob(knob)
{
}

StringParam::~StringParam()
{
}

void
StringParam::setType(StringParam::TypeEnum type)
{
    KnobStringPtr knob = _sKnob.lock();

    if ( !knob->isUserKnob() ) {
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

FileParam::FileParam(const KnobFilePtr& knob)
    : StringParamBase( toKnobStringBase(knob) )
    , _sKnob(knob)
{
}

FileParam::~FileParam()
{
}

void
FileParam::setSequenceEnabled(bool enabled)
{
    KnobFilePtr k = _sKnob.lock();

    if ( !k->isUserKnob() ) {
        return;
    }
    if (enabled) {
        k->setAsInputImage();
    }
}

void
FileParam::openFile()
{
    KnobFilePtr k = _sKnob.lock();

    if (k) {
        k->open_file();
    }
}

void
FileParam::reloadFile()
{
    KnobFilePtr k = _sKnob.lock();

    if (k) {
        k->reloadFile();
    }
}

/////////////////////OutputFileParam

OutputFileParam::OutputFileParam(const KnobOutputFilePtr& knob)
    : StringParamBase( toKnobStringBase(knob) )
    , _sKnob(knob)
{
}

OutputFileParam::~OutputFileParam()
{
}

void
OutputFileParam::setSequenceEnabled(bool enabled)
{
    KnobOutputFilePtr knob = _sKnob.lock();

    if ( !knob->isUserKnob() ) {
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

PathParam::PathParam(const KnobPathPtr& knob)
    : StringParamBase( toKnobStringBase(knob) )
    , _sKnob(knob)
{
}

PathParam::~PathParam()
{
}

void
PathParam::setAsMultiPathTable()
{
    if ( !_sKnob.lock()->isUserKnob() ) {
        return;
    }
    _sKnob.lock()->setMultiPath(true);
}

////////////////////ButtonParam

ButtonParam::ButtonParam(const KnobButtonPtr& knob)
    : Param(knob)
    , _buttonKnob( toKnobButton(knob) )
{
}

ButtonParam::~ButtonParam()
{
}

bool
ButtonParam::isCheckable() const
{
    return _buttonKnob.lock()->getIsCheckable();
}

void
ButtonParam::setDown(bool down)
{
    _buttonKnob.lock()->setValue(down);
}


bool
ButtonParam::isDown() const
{
    return _buttonKnob.lock()->getValue();
}

void
ButtonParam::trigger()
{
    _buttonKnob.lock()->trigger();
}

////////////////////SeparatorParam

SeparatorParam::SeparatorParam(const KnobSeparatorPtr& knob)
    : Param(knob)
    , _separatorKnob( toKnobSeparator(knob) )
{
}

SeparatorParam::~SeparatorParam()
{
}

///////////////////GroupParam

GroupParam::GroupParam(const KnobGroupPtr& knob)
    : Param(knob)
    , _groupKnob( toKnobGroup(knob) )
{
}

GroupParam::~GroupParam()
{
}

void
GroupParam::addParam(const Param* param)
{
    if ( !param || !param->getInternalKnob()->isUserKnob() ) {
        return;
    }
    _groupKnob.lock()->addKnob( param->getInternalKnob() );
}

void
GroupParam::setAsTab()
{
    if ( !_groupKnob.lock()->isUserKnob() ) {
        return;
    }
    _groupKnob.lock()->setAsTab();
}

void
GroupParam::setOpened(bool opened)
{
    _groupKnob.lock()->setValue(opened, ViewSpec::current(), 0);
}

bool
GroupParam::getIsOpened() const
{
    return _groupKnob.lock()->getValue();
}

//////////////////////PageParam

PageParam::PageParam(const KnobPagePtr& knob)
    : Param(knob)
    , _pageKnob( toKnobPage(knob) )
{
}

PageParam::~PageParam()
{
}

void
PageParam::addParam(const Param* param)
{
    if ( !param || !param->getInternalKnob()->isUserKnob() ) {
        return;
    }
    _pageKnob.lock()->addKnob( param->getInternalKnob() );
}

////////////////////ParametricParam
ParametricParam::ParametricParam(const KnobParametricPtr& knob)
    : Param( boost::dynamic_pointer_cast<KnobI>(knob) )
    , _parametricKnob(knob)
{
}

ParametricParam::~ParametricParam()
{
}

void
ParametricParam::setCurveColor(int dimension,
                               double r,
                               double g,
                               double b)
{
    _parametricKnob.lock()->setCurveColor(dimension, r, g, b);
}

void
ParametricParam::getCurveColor(int dimension,
                               ColorTuple& ret) const
{
    _parametricKnob.lock()->getCurveColor(dimension, &ret.r, &ret.g, &ret.b);
    ret.a = 1.;
}

StatusEnum
ParametricParam::addControlPoint(int dimension,
                                 double key,
                                 double value,
                                 NATRON_NAMESPACE::KeyframeTypeEnum interpolation)
{
    return _parametricKnob.lock()->addControlPoint(eValueChangedReasonNatronInternalEdited, dimension, key, value, interpolation);
}

NATRON_NAMESPACE::StatusEnum
ParametricParam::addControlPoint(int dimension,
                                 double key,
                                 double value,
                                 double leftDerivative,
                                 double rightDerivative,
                                 NATRON_NAMESPACE::KeyframeTypeEnum interpolation)
{
    return _parametricKnob.lock()->addControlPoint(eValueChangedReasonNatronInternalEdited, dimension, key, value, leftDerivative, rightDerivative, interpolation);
}

double
ParametricParam::getValue(int dimension,
                          double parametricPosition) const
{
    double ret;
    StatusEnum stat =  _parametricKnob.lock()->getValue(dimension, parametricPosition, &ret);

    if (stat == eStatusFailed) {
        ret =  0.;
    }

    return ret;
}

int
ParametricParam::getNControlPoints(int dimension) const
{
    int ret;
    StatusEnum stat =  _parametricKnob.lock()->getNControlPoints(dimension, &ret);

    if (stat == eStatusFailed) {
        ret = 0;
    }

    return ret;
}

StatusEnum
ParametricParam::getNthControlPoint(int dimension,
                                    int nthCtl,
                                    double *key,
                                    double *value,
                                    double *leftDerivative,
                                    double *rightDerivative) const
{
    return _parametricKnob.lock()->getNthControlPoint(dimension, nthCtl, key, value, leftDerivative, rightDerivative);
}

StatusEnum
ParametricParam::setNthControlPoint(int dimension,
                                    int nthCtl,
                                    double key,
                                    double value,
                                    double leftDerivative,
                                    double rightDerivative)
{
    return _parametricKnob.lock()->setNthControlPoint(eValueChangedReasonNatronInternalEdited, dimension, nthCtl, key, value, leftDerivative, rightDerivative);
}

NATRON_NAMESPACE::StatusEnum
ParametricParam::setNthControlPointInterpolation(int dimension,
                                                 int nThCtl,
                                                 KeyframeTypeEnum interpolation)
{
    return _parametricKnob.lock()->setNthControlPointInterpolation(eValueChangedReasonNatronInternalEdited, dimension, nThCtl, interpolation);
}

StatusEnum
ParametricParam::deleteControlPoint(int dimension,
                                    int nthCtl)
{
    return _parametricKnob.lock()->deleteControlPoint(eValueChangedReasonNatronInternalEdited, dimension, nthCtl);
}

StatusEnum
ParametricParam::deleteAllControlPoints(int dimension)
{
    return _parametricKnob.lock()->deleteAllControlPoints(eValueChangedReasonNatronInternalEdited, dimension);
}

void
ParametricParam::setDefaultCurvesFromCurrentCurves()
{
    _parametricKnob.lock()->setDefaultCurvesFromCurves();
}

NATRON_PYTHON_NAMESPACE_EXIT;
NATRON_NAMESPACE_EXIT;

