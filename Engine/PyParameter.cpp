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
    KnobIPtr knob = getInternalKnob();

    if (!knob) {
        return 0;
    }
    KnobIPtr parent = knob->getParentKnob();

    if (parent) {
        return new Param(parent);
    } else {
        return 0;
    }
}

int
Param::getNumDimensions() const
{
    KnobIPtr knob = getInternalKnob();

    if (!knob) {
        return 0;
    }
    return knob->getDimension();
}

QString
Param::getScriptName() const
{
    KnobIPtr knob = getInternalKnob();

    if (!knob) {
        return QString();
    }
    return QString::fromUtf8( knob->getName().c_str() );
}

QString
Param::getLabel() const
{
    KnobIPtr knob = getInternalKnob();

    if (!knob) {
        return QString();
    }
    return QString::fromUtf8( knob->getLabel().c_str() );
}

void
Param::setLabel(const QString& label)
{
    if ( !getInternalKnob()->isUserKnob() ) {
        return;
    }
    getInternalKnob()->setLabel(label);
}

QString
Param::getTypeName() const
{
    KnobIPtr knob = getInternalKnob();

    if (!knob) {
        return QString();
    }
    return QString::fromUtf8( knob->typeName().c_str() );
}

QString
Param::getHelp() const
{
    KnobIPtr knob = getInternalKnob();

    if (!knob) {
        return QString();
    }
    return QString::fromUtf8( knob->getHintToolTip().c_str() );
}

void
Param::setHelp(const QString& help)
{
    KnobIPtr knob = getInternalKnob();

    if ( !knob || !knob->isUserKnob() ) {
        return;
    }
    knob->setHintToolTip( help.toStdString() );
}

bool
Param::getIsVisible() const
{
    KnobIPtr knob = getInternalKnob();

    if (!knob) {
        return false;
    }
    return knob->getIsSecret();
}

void
Param::setVisible(bool visible)
{
    KnobIPtr knob = getInternalKnob();

    if (!knob) {
        return;
    }
    knob->setSecret(!visible);
}

bool
Param::getIsEnabled(int dimension) const
{
    KnobIPtr knob = getInternalKnob();

    if (!knob) {
        return false;
    }
    return knob->isEnabled(dimension);
}

void
Param::setEnabled(bool enabled,
                  int dimension)
{
    KnobIPtr knob = getInternalKnob();

    if (!knob) {
        return;
    }
    knob->setEnabled(dimension, enabled);
}

bool
Param::getIsPersistent() const
{
    KnobIPtr knob = getInternalKnob();

    if (!knob) {
        return false;
    }
    return knob->getIsPersistent();
}

void
Param::setPersistent(bool persistent)
{
    KnobIPtr knob = getInternalKnob();

    if ( !knob || !knob->isUserKnob() ) {
        return;
    }
    knob->setIsPersistent(persistent);
}

bool
Param::getEvaluateOnChange() const
{
    KnobIPtr knob = getInternalKnob();

    if (!knob) {
        return false;
    }
    return knob->getEvaluateOnChange();
}

void
Param::setEvaluateOnChange(bool eval)
{
    KnobIPtr knob = getInternalKnob();

    if ( !knob || !knob->isUserKnob() ) {
        return;
    }
    knob->setEvaluateOnChange(eval);
}

bool
Param::getCanAnimate() const
{
    KnobIPtr knob = getInternalKnob();

    if (!knob) {
        return false;
    }
    return knob->canAnimate();
}

bool
Param::getIsAnimationEnabled() const
{
    KnobIPtr knob = getInternalKnob();

    if (!knob) {
        return false;
    }
    return knob->isAnimationEnabled();
}

void
Param::setAnimationEnabled(bool e)
{
    KnobIPtr knob = getInternalKnob();

    if ( !knob || !knob->isUserKnob() ) {
        return;
    }
    knob->setAnimationEnabled(e);
}

bool
Param::getAddNewLine()
{
    KnobIPtr knob = getInternalKnob();

    if (!knob) {
        return false;
    }
    return knob->isNewLineActivated();
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
    KnobIPtr knob = getInternalKnob();

    if (!knob) {
        return 0;
    }
    return knob->random(min, max);
}

double
Param::random(unsigned int seed) const
{
    KnobIPtr knob = getInternalKnob();

    if (!knob) {
        return 0;
    }
    return knob->random(seed);
}

int
Param::randomInt(int min,
                 int max)
{
    KnobIPtr knob = getInternalKnob();

    if (!knob) {
        return 0;
    }
    return knob->randomInt(min, max);
}

int
Param::randomInt(unsigned int seed) const
{
    KnobIPtr knob = getInternalKnob();

    if (!knob) {
        return 0;
    }
    return knob->randomInt(seed);
}

double
Param::curve(double time,
             int dimension) const
{
    KnobIPtr knob = getInternalKnob();

    if (!knob) {
        return 0.;
    }
    return knob->getRawCurveValueAt(time, ViewSpec::current(), dimension);
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
    KnobIPtr knob = getInternalKnob();

    if (!knob) {
        return false;
    }
    return knob->isAnimated( dimension, ViewSpec::current() );
}

int
AnimatedParam::getNumKeys(int dimension) const
{
    KnobIPtr knob = getInternalKnob();

    if (!knob) {
        return 0;
    }
    return knob->getKeyFramesCount(ViewSpec::current(), dimension);
}

int
AnimatedParam::getKeyIndex(double time,
                           int dimension) const
{
    KnobIPtr knob = getInternalKnob();

    if (!knob) {
        return 0;
    }
    return knob->getKeyFrameIndex(ViewSpec::current(), dimension, time);
}

bool
AnimatedParam::getKeyTime(int index,
                          int dimension,
                          double* time) const
{
    KnobIPtr knob = getInternalKnob();

    if (!knob) {
        *time = 0.;
        return false;
    }
    return knob->getKeyFrameTime(ViewSpec::current(), index, dimension, time);
}

void
AnimatedParam::deleteValueAtTime(double time,
                                 int dimension)
{
    KnobIPtr knob = getInternalKnob();

    if (!knob) {
        return;
    }
    knob->deleteValueAtTime(eCurveChangeReasonInternal, time, ViewSpec::all(), dimension, false);
}

void
AnimatedParam::removeAnimation(int dimension)
{
    KnobIPtr knob = getInternalKnob();

    if (!knob) {
        return;
    }
    knob->removeAnimation(ViewSpec::all(), dimension);
}

double
AnimatedParam::getDerivativeAtTime(double time,
                                   int dimension) const
{
    KnobIPtr knob = getInternalKnob();

    if (!knob) {
        return 0.;
    }
    return knob->getDerivativeAtTime(time, ViewSpec::current(), dimension);
}

double
AnimatedParam::getIntegrateFromTimeToTime(double time1,
                                          double time2,
                                          int dimension) const
{
    KnobIPtr knob = getInternalKnob();

    if (!knob) {
        return 0.;
    }
    return knob->getIntegrateFromTimeToTime(time1, time2, ViewSpec::current(), dimension);
}

int
AnimatedParam::getCurrentTime() const
{
    KnobIPtr knob = getInternalKnob();

    if (!knob) {
        return 0;
    }
    return knob->getCurrentTime();
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
    if (!knob) {
        return 0;
    }

    return knob->getValue();
}

Int2DTuple
Int2DParam::get() const
{
    Int2DTuple ret = {0, 0};
    KnobIntPtr knob = _intKnob.lock();
    if (!knob) {
        return ret;
    }

    ret.x = knob->getValue(0);
    ret.y = knob->getValue(1);

    return ret;
}

Int3DTuple
Int3DParam::get() const
{
    KnobIntPtr knob = _intKnob.lock();
    Int3DTuple ret = {0, 0, 0};
    if (!knob) {
        return ret;
    }

    ret.x = knob->getValue(0);
    ret.y = knob->getValue(1);
    ret.z = knob->getValue(2);

    return ret;
}

int
IntParam::get(double frame) const
{
    KnobIntPtr knob = _intKnob.lock();
    if (!knob) {
        return 0;
    }

    return knob->getValueAtTime(frame, 0);
}

Int2DTuple
Int2DParam::get(double frame) const
{
    Int2DTuple ret = {0, 0};
    KnobIntPtr knob = _intKnob.lock();
    if (!knob) {
        return ret;
    }

    ret.x = knob->getValueAtTime(frame, 0);
    ret.y = knob->getValueAtTime(frame, 1);

    return ret;
}

Int3DTuple
Int3DParam::get(double frame) const
{
    Int3DTuple ret = {0, 0, 0};
    KnobIntPtr knob = _intKnob.lock();
    if (!knob) {
        return ret;
    }

    ret.x = knob->getValueAtTime(frame, 0);
    ret.y = knob->getValueAtTime(frame, 1);
    ret.z = knob->getValueAtTime(frame, 2);

    return ret;
}

void
IntParam::set(int x)
{
    KnobIntPtr knob = _intKnob.lock();
    if (!knob) {
        return;
    }
    knob->setValue(x, ViewSpec::current(), 0);
}

void
Int2DParam::set(int x,
                int y)
{
    KnobIntPtr knob = _intKnob.lock();
    if (!knob) {
        return;
    }
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
    if (!knob) {
        return;
    }

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
    KnobIntPtr knob = _intKnob.lock();
    if (!knob) {
        return;
    }
    knob->setValueAtTime(frame, x, ViewSpec::current(), 0);
}

void
Int2DParam::set(int x,
                int y,
                double frame)
{
    KnobIntPtr knob = _intKnob.lock();
    if (!knob) {
        return;
    }
    knob->setValuesAtTime(frame, x, y, ViewSpec::current(), eValueChangedReasonNatronInternalEdited);
}

void
Int3DParam::set(int x,
                int y,
                int z,
                double frame)
{
    KnobIntPtr knob = _intKnob.lock();
    if (!knob) {
        return;
    }
    knob->setValuesAtTime(frame, x, y, z, ViewSpec::current(), eValueChangedReasonNatronInternalEdited);
}

int
IntParam::getValue(int dimension) const
{
    KnobIntPtr knob = _intKnob.lock();
    if (!knob) {
        return 0;
    }
    return knob->getValue(dimension);
}

void
IntParam::setValue(int value,
                   int dimension)
{
    KnobIntPtr knob = _intKnob.lock();
    if (!knob) {
        return;
    }
    knob->setValue(value, ViewSpec::current(), dimension);
}

int
IntParam::getValueAtTime(double time,
                         int dimension) const
{
    KnobIntPtr knob = _intKnob.lock();
    if (!knob) {
        return 0;
    }
    return knob->getValueAtTime(time, dimension);
}

void
IntParam::setValueAtTime(int value,
                         double time,
                         int dimension)
{
    KnobIntPtr knob = _intKnob.lock();
    if (!knob) {
        return;
    }
    knob->setValueAtTime(time, value, ViewSpec::current(), dimension);
}

void
IntParam::setDefaultValue(int value,
                          int dimension)
{
    KnobIntPtr knob = _intKnob.lock();
    if (!knob) {
        return;
    }
    knob->setDefaultValueWithoutApplying(value, dimension);
}

int
IntParam::getDefaultValue(int dimension) const
{
    KnobIntPtr knob = _intKnob.lock();
    if (!knob) {
        return 0;
    }
    return knob->getDefaultValues_mt_safe()[dimension];
}

void
IntParam::restoreDefaultValue(int dimension)
{
    KnobIntPtr knob = _intKnob.lock();
    if (!knob) {
        return;
    }
    knob->resetToDefaultValue(dimension);
}

void
IntParam::setMinimum(int minimum,
                     int dimension)
{
    KnobIntPtr knob = _intKnob.lock();
    if (!knob) {
        return;
    }
    knob->setMinimum(minimum, dimension);
}

int
IntParam::getMinimum(int dimension) const
{
    KnobIntPtr knob = _intKnob.lock();
    if (!knob) {
        return 0;
    }
    return knob->getMinimum(dimension);
}

void
IntParam::setMaximum(int maximum,
                     int dimension)
{
    KnobIntPtr knob = _intKnob.lock();
    if ( !knob || !knob->isUserKnob() ) {
        return;
    }
    knob->setMaximum(maximum, dimension);
}

int
IntParam::getMaximum(int dimension) const
{
    KnobIntPtr knob = _intKnob.lock();
    if (!knob) {
        return 0;
    }
    return knob->getMaximum(dimension);
}

void
IntParam::setDisplayMinimum(int minimum,
                            int dimension)
{
    KnobIntPtr knob = _intKnob.lock();
    if ( !knob || !knob->isUserKnob() ) {
        return;
    }
    knob->setDisplayMinimum(minimum, dimension);
}

int
IntParam::getDisplayMinimum(int dimension) const
{
    KnobIntPtr knob = _intKnob.lock();
    if (!knob) {
        return 0;
    }
    return knob->getDisplayMinimum(dimension);
}

void
IntParam::setDisplayMaximum(int maximum,
                            int dimension)
{
    KnobIntPtr knob = _intKnob.lock();
    if (!knob) {
        return;
    }
    knob->setDisplayMaximum(maximum, dimension);
}

int
IntParam::getDisplayMaximum(int dimension) const
{
    KnobIntPtr knob = _intKnob.lock();
    if (!knob) {
        return 0;
    }
    return knob->getDisplayMaximum(dimension);
}

int
IntParam::addAsDependencyOf(int fromExprDimension,
                            Param* param,
                            int thisDimension)
{
    _addAsDependencyOf(fromExprDimension, param, thisDimension);

    KnobIntPtr knob = _intKnob.lock();
    if (!knob) {
        return 0;
    }
    return knob->getValue();
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
    KnobDoublePtr knob = _doubleKnob.lock();
    if (!knob) {
        return 0.;
    }
    return knob->getValue(0);
}

Double2DTuple
Double2DParam::get() const
{
    KnobDoublePtr knob = _doubleKnob.lock();
    Double2DTuple ret = {0., 0.};
    if (!knob) {
        return ret;
    }

    ret.x = knob->getValue(0);
    ret.y = knob->getValue(1);

    return ret;
}

Double3DTuple
Double3DParam::get() const
{
    KnobDoublePtr knob = _doubleKnob.lock();
    Double3DTuple ret = {0., 0., 0.};
    if (!knob) {
        return ret;
    }

    ret.x = knob->getValue(0);
    ret.y = knob->getValue(1);
    ret.z = knob->getValue(2);

    return ret;
}

double
DoubleParam::get(double frame) const
{
    KnobDoublePtr knob = _doubleKnob.lock();
    if (!knob) {
        return 0.;
    }
    return knob->getValueAtTime(frame, 0);
}

Double2DTuple
Double2DParam::get(double frame) const
{
    Double2DTuple ret = {0., 0.};
    KnobDoublePtr knob = _doubleKnob.lock();
    if (!knob) {
        return ret;
    }

    ret.x = knob->getValueAtTime(frame, 0);
    ret.y = knob->getValueAtTime(frame, 1);

    return ret;
}

Double3DTuple
Double3DParam::get(double frame) const
{
    KnobDoublePtr knob = _doubleKnob.lock();
    Double3DTuple ret = {0., 0., 0.};
    if (!knob) {
        return ret;
    }

    ret.x = knob->getValueAtTime(frame, 0);
    ret.y = knob->getValueAtTime(frame, 1);
    ret.z = knob->getValueAtTime(frame, 2);

    return ret;
}

void
DoubleParam::set(double x)
{
    KnobDoublePtr knob = _doubleKnob.lock();
    if (!knob) {
        return;
    }
    knob->setValue(x, ViewSpec::current(), 0);
}

void
Double2DParam::set(double x,
                   double y)
{
    KnobDoublePtr knob = _doubleKnob.lock();
    if (!knob) {
        return;
    }

    knob->setValues(x, y, ViewSpec::current(), eValueChangedReasonNatronInternalEdited);
}

void
Double3DParam::set(double x,
                   double y,
                   double z)
{
    KnobDoublePtr knob = _doubleKnob.lock();
    if (!knob) {
        return;
    }

    knob->setValues(x, y, z, ViewSpec::current(), eValueChangedReasonNatronInternalEdited);
}

void
DoubleParam::set(double x,
                 double frame)
{
    KnobDoublePtr knob = _doubleKnob.lock();
    if (!knob) {
        return;
    }
    return knob->setValueAtTime(frame, x, ViewSpec::current(), 0);
}

void
Double2DParam::set(double x,
                   double y,
                   double frame)
{
    KnobDoublePtr knob = _doubleKnob.lock();
    if (!knob) {
        return;
    }

    knob->setValuesAtTime(frame, x, y, ViewSpec::current(), eValueChangedReasonNatronInternalEdited);
}

void
Double2DParam::setUsePointInteract(bool use)
{
    KnobDoublePtr knob = _doubleKnob.lock();
    if (!knob) {
        return;
    }
    return knob->setHasHostOverlayHandle(use);
}

void
Double3DParam::set(double x,
                   double y,
                   double z,
                   double frame)
{
    KnobDoublePtr knob = _doubleKnob.lock();
    if (!knob) {
        return;
    }

    knob->setValuesAtTime(frame, x, y, z, ViewSpec::current(), eValueChangedReasonNatronInternalEdited);
}

double
DoubleParam::getValue(int dimension) const
{
    KnobDoublePtr knob = _doubleKnob.lock();
    if (!knob) {
        return 0.;
    }
    return knob->getValue(dimension);
}

void
DoubleParam::setValue(double value,
                      int dimension)
{
    KnobDoublePtr knob = _doubleKnob.lock();
    if (!knob) {
        return;
    }
    knob->setValue(value, ViewSpec::current(), dimension);
}

double
DoubleParam::getValueAtTime(double time,
                            int dimension) const
{
    KnobDoublePtr knob = _doubleKnob.lock();
    if (!knob) {
        return 0.;
    }
    return knob->getValueAtTime(time, dimension);
}

void
DoubleParam::setValueAtTime(double value,
                            double time,
                            int dimension)
{
    KnobDoublePtr knob = _doubleKnob.lock();
    if (!knob) {
        return;
    }
    return knob->setValueAtTime(time, value, ViewSpec::current(), dimension);
}

void
DoubleParam::setDefaultValue(double value,
                             int dimension)
{
    KnobDoublePtr knob = _doubleKnob.lock();
    if (!knob) {
        return;
    }
    return knob->setDefaultValueWithoutApplying(value, dimension);
}

double
DoubleParam::getDefaultValue(int dimension) const
{
    KnobDoublePtr knob = _doubleKnob.lock();
    if (!knob) {
        return 0.;
    }
    return knob->getDefaultValues_mt_safe()[dimension];
}

void
DoubleParam::restoreDefaultValue(int dimension)
{
    KnobDoublePtr knob = _doubleKnob.lock();
    if (!knob) {
        return;
    }
    return knob->resetToDefaultValue(dimension);
}

void
DoubleParam::setMinimum(double minimum,
                        int dimension)
{
    KnobDoublePtr knob = _doubleKnob.lock();
    if (!knob) {
        return;
    }
    knob->setMinimum(minimum, dimension);
}

double
DoubleParam::getMinimum(int dimension) const
{
    KnobDoublePtr knob = _doubleKnob.lock();
    if (!knob) {
        return 0.;
    }
    return knob->getMinimum(dimension);
}

void
DoubleParam::setMaximum(double maximum,
                        int dimension)
{
    KnobDoublePtr knob = _doubleKnob.lock();
    if ( !knob || !knob->isUserKnob() ) {
        return;
    }
    knob->setMaximum(maximum, dimension);
}

double
DoubleParam::getMaximum(int dimension) const
{
    KnobDoublePtr knob = _doubleKnob.lock();
    if (!knob) {
        return 0.;
    }
    return knob->getMaximum(dimension);
}

void
DoubleParam::setDisplayMinimum(double minimum,
                               int dimension)
{
    KnobDoublePtr knob = _doubleKnob.lock();
    if ( !knob || !knob->isUserKnob() ) {
        return;
    }
    knob->setDisplayMinimum(minimum, dimension);
}

double
DoubleParam::getDisplayMinimum(int dimension) const
{
    KnobDoublePtr knob = _doubleKnob.lock();
    if (!knob) {
        return 0.;
    }
    return knob->getDisplayMinimum(dimension);
}

void
DoubleParam::setDisplayMaximum(double maximum,
                               int dimension)
{
    KnobDoublePtr knob = _doubleKnob.lock();
    if (!knob) {
        return;
    }
    return knob->setDisplayMaximum(maximum, dimension);
}

double
DoubleParam::getDisplayMaximum(int dimension) const
{
    KnobDoublePtr knob = _doubleKnob.lock();
    if (!knob) {
        return 0.;
    }
    return knob->getDisplayMaximum(dimension);
}

double
DoubleParam::addAsDependencyOf(int fromExprDimension,
                               Param* param,
                               int thisDimension)
{
    _addAsDependencyOf(fromExprDimension, param, thisDimension);

    KnobDoublePtr knob = _doubleKnob.lock();
    if (!knob) {
        return 0.;
    }
    return knob->getValue();
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
    ColorTuple ret = {0., 0., 0., 0.};
    KnobColorPtr knob = _colorKnob.lock();
    if (!knob) {
        return ret;
    }
    ret.r = knob->getValue(0);
    ret.g = knob->getValue(1);
    ret.b = knob->getValue(2);
    ret.a = knob->getDimension() == 4 ? knob->getValue(3) : 1.;

    return ret;
}

ColorTuple
ColorParam::get(double frame) const
{
    ColorTuple ret = {0., 0., 0., 0.};
    KnobColorPtr knob = _colorKnob.lock();
    if (!knob) {
        return ret;
    }
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
    if (!knob) {
        return;
    }
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
    if (!knob) {
        return;
    }
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
    KnobColorPtr knob = _colorKnob.lock();
    if (!knob) {
        return 0.;
    }
    return knob->getValue(dimension);
}

void
ColorParam::setValue(double value,
                     int dimension)
{
    KnobColorPtr knob = _colorKnob.lock();
    if (!knob) {
        return;
    }
    knob->setValue(value, ViewSpec::current(), dimension);
}

double
ColorParam::getValueAtTime(double time,
                           int dimension) const
{
    KnobColorPtr knob = _colorKnob.lock();
    if (!knob) {
        return 0.;
    }
    return knob->getValueAtTime(time, dimension);
}

void
ColorParam::setValueAtTime(double value,
                           double time,
                           int dimension)
{
    KnobColorPtr knob = _colorKnob.lock();
    if (!knob) {
        return;
    }
    knob->setValueAtTime(time, value, ViewSpec::current(), dimension);
}

void
ColorParam::setDefaultValue(double value,
                            int dimension)
{
    KnobColorPtr knob = _colorKnob.lock();
    if (!knob) {
        return;
    }
    knob->setDefaultValueWithoutApplying(value, dimension);
}

double
ColorParam::getDefaultValue(int dimension) const
{
    KnobColorPtr knob = _colorKnob.lock();
    if (!knob) {
        return 0.;
    }
    return knob->getDefaultValues_mt_safe()[dimension];
}

void
ColorParam::restoreDefaultValue(int dimension)
{
    KnobColorPtr knob = _colorKnob.lock();
    if (!knob) {
        return;
    }
    knob->resetToDefaultValue(dimension);
}

void
ColorParam::setMinimum(double minimum,
                       int dimension)
{
    KnobColorPtr knob = _colorKnob.lock();
    if (!knob) {
        return;
    }
    knob->setMinimum(minimum, dimension);
}

double
ColorParam::getMinimum(int dimension) const
{
    KnobColorPtr knob = _colorKnob.lock();
    if (!knob) {
        return 0.;
    }
    return knob->getMinimum(dimension);
}

void
ColorParam::setMaximum(double maximum,
                       int dimension)
{
    KnobColorPtr knob = _colorKnob.lock();
    if ( !knob || !knob->isUserKnob() ) {
        return;
    }
    knob->setMaximum(maximum, dimension);
}

double
ColorParam::getMaximum(int dimension) const
{
    KnobColorPtr knob = _colorKnob.lock();
    if (!knob) {
        return 0.;
    }
    return knob->getMaximum(dimension);
}

void
ColorParam::setDisplayMinimum(double minimum,
                              int dimension)
{
    KnobColorPtr knob = _colorKnob.lock();
    if ( !knob || !knob->isUserKnob() ) {
        return;
    }
    knob->setDisplayMinimum(minimum, dimension);
}

double
ColorParam::getDisplayMinimum(int dimension) const
{
    KnobColorPtr knob = _colorKnob.lock();
    if (!knob) {
        return 0.;
    }
    return knob->getDisplayMinimum(dimension);
}

void
ColorParam::setDisplayMaximum(double maximum,
                              int dimension)
{
    KnobColorPtr knob = _colorKnob.lock();
    if (!knob) {
        return;
    }
    knob->setDisplayMaximum(maximum, dimension);
}

double
ColorParam::getDisplayMaximum(int dimension) const
{
    KnobColorPtr knob = _colorKnob.lock();
    if (!knob) {
        return 0.;
    }
    return knob->getDisplayMaximum(dimension);
}

double
ColorParam::addAsDependencyOf(int fromExprDimension,
                              Param* param,
                              int thisDimension)
{
    _addAsDependencyOf(fromExprDimension, param, thisDimension);

    KnobColorPtr knob = _colorKnob.lock();
    if (!knob) {
        return;
    }
    return knob->getValue();
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

    if ( !knob || !knob->isUserKnob() ) {
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

// Deprecated
void
FileParam::setSequenceEnabled(bool enabled)
{
    KnobFilePtr k = _sKnob.lock();

    if ( !k || !k->isUserKnob() ) {
        return;
    }
    if (enabled) {
        if (k->getDialogType() == KnobFile::eKnobFileDialogTypeOpenFile) {
            k->setDialogType(KnobFile::eKnobFileDialogTypeOpenFileSequences);
        } else if (k->getDialogType() == KnobFile::eKnobFileDialogTypeSaveFile) {
            k->setDialogType(KnobFile::eKnobFileDialogTypeOpenFileSequences);
        }
    } else {
        if (k->getDialogType() == KnobFile::eKnobFileDialogTypeOpenFileSequences) {
            k->setDialogType(KnobFile::eKnobFileDialogTypeOpenFile);
        } else if (k->getDialogType() == KnobFile::eKnobFileDialogTypeSaveFileSequences) {
            k->setDialogType(KnobFile::eKnobFileDialogTypeOpenFile);
        }
    }
}

void
FileParam::setDialogType(bool fileExisting, bool useSequences, const std::vector<std::string>& filters)
{
    KnobFilePtr k = _sKnob.lock();

    if ( !k->isUserKnob() ) {
        return;
    }
    if (fileExisting) {
        if (useSequences) {
            k->setDialogType(KnobFile::eKnobFileDialogTypeOpenFileSequences);
        } else {
            k->setDialogType(KnobFile::eKnobFileDialogTypeOpenFile);
        }
    } else {
        if (useSequences) {
            k->setDialogType(KnobFile::eKnobFileDialogTypeSaveFileSequences);
        } else {
            k->setDialogType(KnobFile::eKnobFileDialogTypeSaveFile);
        }
    }
    k->setDialogFilters(filters);
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
    KnobPathPtr knob = _sKnob.lock();

    if ( !knob || !knob->isUserKnob() ) {
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
    if (!param) {
        return;
    }
    KnobIPtr knob = param->getInternalKnob();

    if ( !knob || !knob->isUserKnob() ) {
        return;
    }
    KnobGroupPtr group = _groupKnob.lock();
    if (!group) {
        return;
    }
    group->addKnob(knob);
}

void
GroupParam::setAsTab()
{
    KnobGroupPtr group = _groupKnob.lock();
    if ( !group || !group->isUserKnob() ) {
        return;
    }
    group->setAsTab();
}

void
GroupParam::setOpened(bool opened)
{
    KnobGroupPtr group = _groupKnob.lock();
    if (!group) {
        return;
    }
    group->setValue(opened, ViewSpec::current(), 0);
}

bool
GroupParam::getIsOpened() const
{
    KnobGroupPtr group = _groupKnob.lock();
    if (!group) {
        return false;
    }
    return group->getValue();
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
    if (!param) {
        return;
    }
    KnobIPtr knob = param->getInternalKnob();

    if ( !knob || !knob->isUserKnob() ) {
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
    KnobParametricPtr param = _parametricKnob.lock();
    if (!param) {
        return;
    }
    param->setCurveColor(dimension, r, g, b);
}

void
ParametricParam::getCurveColor(int dimension,
                               ColorTuple& ret) const
{
    KnobParametricPtr param = _parametricKnob.lock();
    if (!param) {
        return;
    }
    param->getCurveColor(dimension, &ret.r, &ret.g, &ret.b);
    ret.a = 1.;
}

StatusEnum
ParametricParam::addControlPoint(int dimension,
                                 double key,
                                 double value,
                                 KeyframeTypeEnum interpolation)
{
    KnobParametricPtr param = _parametricKnob.lock();
    if (!param) {
        return eStatusFailed;
    }
    return param->addControlPoint(eValueChangedReasonNatronInternalEdited, dimension, key, value, interpolation);
}

StatusEnum
ParametricParam::addControlPoint(int dimension,
                                 double key,
                                 double value,
                                 double leftDerivative,
                                 double rightDerivative,
                                 KeyframeTypeEnum interpolation)
{
    KnobParametricPtr param = _parametricKnob.lock();
    if (!param) {
        return eStatusFailed;
    }
    return param->addControlPoint(eValueChangedReasonNatronInternalEdited, dimension, key, value, leftDerivative, rightDerivative, interpolation);
}

double
ParametricParam::getValue(int dimension,
                          double parametricPosition) const
{
    KnobParametricPtr param = _parametricKnob.lock();
    if (!param) {
        return 0.;
    }
    double ret;
    StatusEnum stat = param->getValue(dimension, parametricPosition, &ret);

    if (stat == eStatusFailed) {
        ret =  0.;
    }

    return ret;
}

int
ParametricParam::getNControlPoints(int dimension) const
{
    KnobParametricPtr param = _parametricKnob.lock();
    if (!param) {
        return 0;
    }
    int ret;
    StatusEnum stat = param->getNControlPoints(dimension, &ret);

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
    KnobParametricPtr param = _parametricKnob.lock();
    if (!param) {
        return eStatusFailed;
    }
    return param->getNthControlPoint(dimension, nthCtl, key, value, leftDerivative, rightDerivative);
}

StatusEnum
ParametricParam::setNthControlPoint(int dimension,
                                    int nthCtl,
                                    double key,
                                    double value,
                                    double leftDerivative,
                                    double rightDerivative)
{
    KnobParametricPtr param = _parametricKnob.lock();
    if (!param) {
        return eStatusFailed;
    }
    return param->setNthControlPoint(eValueChangedReasonNatronInternalEdited, dimension, nthCtl, key, value, leftDerivative, rightDerivative);
}

StatusEnum
ParametricParam::setNthControlPointInterpolation(int dimension,
                                                 int nThCtl,
                                                 KeyframeTypeEnum interpolation)
{
    KnobParametricPtr param = _parametricKnob.lock();
    if (!param) {
        return eStatusFailed;
    }
    return param->setNthControlPointInterpolation(eValueChangedReasonNatronInternalEdited, dimension, nThCtl, interpolation);
}

StatusEnum
ParametricParam::deleteControlPoint(int dimension,
                                    int nthCtl)
{
    KnobParametricPtr param = _parametricKnob.lock();
    if (!param) {
        return eStatusFailed;
    }
    return param->deleteControlPoint(eValueChangedReasonNatronInternalEdited, dimension, nthCtl);
}

StatusEnum
ParametricParam::deleteAllControlPoints(int dimension)
{
    KnobParametricPtr param = _parametricKnob.lock();
    if (!param) {
        return eStatusFailed;
    }
    return param->deleteAllControlPoints(eValueChangedReasonNatronInternalEdited, dimension);
}

void
ParametricParam::setDefaultCurvesFromCurrentCurves()
{
    KnobParametricPtr param = _parametricKnob.lock();
    if (!param) {
        return;
    }
    param->setDefaultCurvesFromCurves();
}

NATRON_PYTHON_NAMESPACE_EXIT;
NATRON_NAMESPACE_EXIT;

