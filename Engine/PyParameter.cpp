/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2023 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#include "Engine/EffectInstance.h"
#include "Engine/Node.h"
#include "Engine/Project.h"
#include "Engine/AppInstance.h"
#include "Engine/KnobFile.h"
#include "Engine/KnobSerialization.h"
#include "Engine/Curve.h"
#include "Engine/ViewIdx.h"

NATRON_NAMESPACE_ENTER
NATRON_PYTHON_NAMESPACE_ENTER

// While an undo group is open (App.beginUndoGroup()), records the state of a knob before the Python API
// changes it, so that the group can undo the change (see PythonUndoGroupCommand).
template <typename K>
static void
recordForUndoGroup(const std::shared_ptr<K>& knob)
{
    if (!knob) {
        return;
    }
    KnobHolder* holder = knob->getHolder();
    AppInstancePtr app = holder ? holder->getApp() : AppInstancePtr();
    // (recordKnobForUndoGroup() does nothing off the main thread, e.g. in a render callback)
    if ( app && (app->getUndoGroupDepth() > 0) ) {
        app->recordKnobForUndoGroup(knob);
    }
}

// A Param refers to its parameter (knob) weakly: the knob goes away with its node (Effect.destroy()), or with
// UserParamHolder.removeParam(). Using such a Param raises RuntimeError, as Shiboken does for a deleted C++ object,
// instead of returning 0, "" or False, or doing nothing (#1615). This sets the Python error and returns the null
// knob: the caller then returns a dummy value, which the binding drops, since it checks PyErr_Occurred() after the
// call (the code injected by typesystem_engine.xml must check it too).
template <typename K>
static std::shared_ptr<K>
knobOrRaise(std::shared_ptr<K> knob)
{
    if (!knob) {
        PyErr_SetString(PyExc_RuntimeError, "The parameter was deleted: its node was destroyed, or removeParam() removed it");
    }

    return knob;
}

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
    KnobIPtr knob = knobOrRaise( getInternalKnob() );

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
    KnobIPtr knob = knobOrRaise( getInternalKnob() );

    if (!knob) {
        return 0;
    }
    return knob->getDimension();
}

QString
Param::getScriptName() const
{
    KnobIPtr knob = knobOrRaise( getInternalKnob() );

    if (!knob) {
        return QString();
    }
    return QString::fromUtf8( knob->getName().c_str() );
}

QString
Param::getLabel() const
{
    KnobIPtr knob = knobOrRaise( getInternalKnob() );

    if (!knob) {
        return QString();
    }
    return QString::fromUtf8( knob->getLabel().c_str() );
}

QString
Param::getTypeName() const
{
    KnobIPtr knob = knobOrRaise( getInternalKnob() );

    if (!knob) {
        return QString();
    }
    return QString::fromUtf8( knob->typeName().c_str() );
}

QString
Param::getHelp() const
{
    KnobIPtr knob = knobOrRaise( getInternalKnob() );

    if (!knob) {
        return QString();
    }
    return QString::fromUtf8( knob->getHintToolTip().c_str() );
}

void
Param::setHelp(const QString& help)
{
    KnobIPtr knob = knobOrRaise( getInternalKnob() );

    if ( !knob || !knob->isUserKnob() ) {
        return;
    }
    knob->setHintToolTip( help.toStdString() );
}

bool
Param::getIsVisible() const
{
    KnobIPtr knob = knobOrRaise( getInternalKnob() );

    if (!knob) {
        return false;
    }
    return !knob->getIsSecret();
}

void
Param::setVisible(bool visible)
{
    KnobIPtr knob = knobOrRaise( getInternalKnob() );

    if (!knob) {
        return;
    }
    knob->setSecret(!visible);
}

void
Param::setVisibleByDefault(bool visible)
{
    KnobIPtr knob = knobOrRaise( getInternalKnob() );

    if (!knob) {
        return;
    }
    knob->setSecretByDefault(!visible);
}

bool
Param::getIsEnabled(int dimension) const
{
    KnobIPtr knob = knobOrRaise( getInternalKnob() );

    if (!knob) {
        return false;
    }
    return knob->isEnabled(dimension);
}

void
Param::setEnabled(bool enabled,
                  int dimension)
{
    KnobIPtr knob = knobOrRaise( getInternalKnob() );

    if (!knob) {
        return;
    }
    knob->setEnabled(dimension, enabled);
}

void
Param::setEnabledByDefault(bool enabled)
{
    KnobIPtr knob = knobOrRaise( getInternalKnob() );

    if (!knob) {
        return;
    }
    knob->setDefaultAllDimensionsEnabled(enabled);
}

bool
Param::getIsPersistent() const
{
    KnobIPtr knob = knobOrRaise( getInternalKnob() );

    if (!knob) {
        return false;
    }
    return knob->getIsPersistent();
}

void
Param::setPersistent(bool persistent)
{
    KnobIPtr knob = knobOrRaise( getInternalKnob() );

    if ( !knob || !knob->isUserKnob() ) {
        return;
    }
    knob->setIsPersistent(persistent);
}

bool
Param::getEvaluateOnChange() const
{
    KnobIPtr knob = knobOrRaise( getInternalKnob() );

    if (!knob) {
        return false;
    }
    return knob->getEvaluateOnChange();
}

void
Param::setEvaluateOnChange(bool eval)
{
    KnobIPtr knob = knobOrRaise( getInternalKnob() );

    if ( !knob || !knob->isUserKnob() ) {
        return;
    }
    knob->setEvaluateOnChange(eval);
}

bool
Param::getCanAnimate() const
{
    KnobIPtr knob = knobOrRaise( getInternalKnob() );

    if (!knob) {
        return false;
    }
    return knob->canAnimate();
}

bool
Param::getIsAnimationEnabled() const
{
    KnobIPtr knob = knobOrRaise( getInternalKnob() );

    if (!knob) {
        return false;
    }
    return knob->isAnimationEnabled();
}

void
Param::setAnimationEnabled(bool e)
{
    KnobIPtr knob = knobOrRaise( getInternalKnob() );

    if ( !knob || !knob->isUserKnob() ) {
        return;
    }
    knob->setAnimationEnabled(e);
}

bool
Param::getAddNewLine()
{
    KnobIPtr knob = knobOrRaise( getInternalKnob() );

    if (!knob) {
        return false;
    }
    return knob->isNewLineActivated();
}

void
Param::setAddNewLine(bool a)
{
    KnobIPtr knob = knobOrRaise( getInternalKnob() );

    if ( !knob || !knob->isUserKnob() ) {
        return;
    }

    KnobIPtr parentKnob = knob->getParentKnob();
    if (parentKnob) {
        KnobGroup* parentIsGrp = dynamic_cast<KnobGroup*>( parentKnob.get() );
        KnobPage* parentIsPage = dynamic_cast<KnobPage*>( parentKnob.get() );
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
Param::copy(Param* other,
            int dimension)
{
    if (!other) {
        return false;
    }
    KnobIPtr thisKnob = knobOrRaise( _knob.lock() );
    KnobIPtr otherKnob = knobOrRaise( other->_knob.lock() );

    if ( !thisKnob || !otherKnob || !thisKnob->isTypeCompatible(otherKnob) ) {
        return false;
    }
    recordForUndoGroup(thisKnob);
    thisKnob->cloneAndUpdateGui(otherKnob.get(), dimension);

    return true;
}

bool
Param::slaveTo(Param* other,
               int thisDimension,
               int otherDimension)
{
    if (!other) {
        return false;
    }
    KnobIPtr thisKnob = knobOrRaise( _knob.lock() );
    KnobIPtr otherKnob = knobOrRaise( other->_knob.lock() );

    if ( !thisKnob || !otherKnob || !KnobI::areTypesCompatibleForSlave( thisKnob.get(), otherKnob.get() ) ) {
        return false;
    }
    if ( (thisDimension < 0) || ( thisDimension >= thisKnob->getDimension() ) || (otherDimension < 0) || ( otherDimension >= otherKnob->getDimension() ) ) {
        return false;
    }

    recordForUndoGroup(thisKnob);
    return thisKnob->slaveTo(thisDimension, otherKnob, otherDimension);
}

void
Param::unslave(int dimension)
{
    KnobIPtr thisKnob = knobOrRaise( _knob.lock() );

    if (!thisKnob) {
        return;
    }
    recordForUndoGroup(thisKnob);
    thisKnob->unSlave(dimension, false);
}

double
Param::random(double min,
              double max) const
{
    KnobIPtr knob = knobOrRaise( getInternalKnob() );

    if (!knob) {
        return 0;
    }
    return knob->random(min, max);
}

double
Param::random(double min,
              double max,
              double time,
              unsigned int seed) const
{
    KnobIPtr knob = knobOrRaise( getInternalKnob() );

    if (!knob) {
        return 0;
    }
    return knob->random(min, max, time, seed);
}

int
Param::randomInt(int min,
                 int max)
{
    KnobIPtr knob = knobOrRaise( getInternalKnob() );

    if (!knob) {
        return 0;
    }
    return knob->randomInt(min, max);
}

int
Param::randomInt(int min,
                 int max,
                 double time,
                 unsigned int seed) const
{
    KnobIPtr knob = knobOrRaise( getInternalKnob() );

    if (!knob) {
        return 0;
    }
    return knob->randomInt(min, max, time, seed);
}

double
Param::curve(double time,
             int dimension) const
{
    KnobIPtr knob = knobOrRaise( getInternalKnob() );

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
    KnobIPtr otherKnob = knobOrRaise( other->_knob.lock() );
    KnobIPtr thisKnob = knobOrRaise( getInternalKnob() );
    if ( !otherKnob || !thisKnob || ( otherKnob->typeName() != thisKnob->typeName() ) ||
         ( otherKnob->getDimension() != thisKnob->getDimension() ) ) {
        return false;
    }
    // the alias is a link of each dimension (getMaster()), which an undo group restores
    recordForUndoGroup(thisKnob);
    recordForUndoGroup(otherKnob);

    return otherKnob->setKnobAsAliasOfThis(thisKnob, true);
}

void
Param::setIconFilePath(const QString& icon)
{
    KnobIPtr thisKnob = knobOrRaise( _knob.lock() );
    if (!thisKnob) {
        return;
    }
    thisKnob->setIconLabel( icon.toStdString() );
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
    KnobIPtr knob = knobOrRaise( getInternalKnob() );

    if (!knob) {
        return false;
    }
    return knob->isAnimated( dimension, ViewSpec::current() );
}

int
AnimatedParam::getNumKeys(int dimension) const
{
    KnobIPtr knob = knobOrRaise( getInternalKnob() );

    if (!knob) {
        return 0;
    }
    return knob->getKeyFramesCount(ViewSpec::current(), dimension);
}

int
AnimatedParam::getKeyIndex(double time,
                           int dimension) const
{
    KnobIPtr knob = knobOrRaise( getInternalKnob() );

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
    KnobIPtr knob = knobOrRaise( getInternalKnob() );

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
    KnobIPtr knob = knobOrRaise( getInternalKnob() );

    if (!knob) {
        return;
    }
    recordForUndoGroup(knob);
    knob->deleteValueAtTime(eCurveChangeReasonInternal, time, ViewSpec::all(), dimension, false);
}

void
AnimatedParam::removeAnimation(int dimension)
{
    KnobIPtr knob = knobOrRaise( getInternalKnob() );

    if (!knob) {
        return;
    }
    recordForUndoGroup(knob);
    knob->removeAnimation(ViewSpec::all(), dimension);
}

double
AnimatedParam::getDerivativeAtTime(double time,
                                   int dimension) const
{
    KnobIPtr knob = knobOrRaise( getInternalKnob() );

    if ( !knob || (dimension < 0) || ( dimension >= knob->getDimension() ) ) {
        return 0.; // as getValue(dimension): the std::invalid_argument of the knob can't go through Python
    }
    return knob->getDerivativeAtTime(time, ViewSpec::current(), dimension);
}

double
AnimatedParam::getIntegrateFromTimeToTime(double time1,
                                          double time2,
                                          int dimension) const
{
    KnobIPtr knob = knobOrRaise( getInternalKnob() );

    if ( !knob || (dimension < 0) || ( dimension >= knob->getDimension() ) ) {
        return 0.; // as getValue(dimension): the std::invalid_argument of the knob can't go through Python
    }
    return knob->getIntegrateFromTimeToTime(time1, time2, ViewSpec::current(), dimension);
}

int
AnimatedParam::getCurrentTime() const
{
    KnobIPtr knob = knobOrRaise( getInternalKnob() );

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
    KnobIPtr knob = knobOrRaise( getInternalKnob() );

    if (!knob) {
        return false;
    }
    KeyFrame newKey;

    recordForUndoGroup(knob);
    return knob->setInterpolationAtTime(eCurveChangeReasonInternal, ViewSpec::current(), dimension, time, interpolation, &newKey);
}

KeyframeTypeEnum
AnimatedParam::getInterpolationAtTime(double time,
                                      int dimension) const
{
    KnobIPtr knob = knobOrRaise( getInternalKnob() );

    if (!knob) {
        return eKeyframeTypeNone;
    }
    // NULL if the dimension does not exist
    CurvePtr curve = knob->getCurve(ViewSpec::current(), dimension);
    KeyFrame key;
    if ( !curve || !curve->getKeyFrameWithTime(time, &key) ) {
        return eKeyframeTypeNone;
    }

    return key.getInterpolation();
}

bool
AnimatedParam::setDerivativesAtTime(double time,
                                    double left,
                                    double right,
                                    int dimension)
{
    KnobIPtr knob = knobOrRaise( getInternalKnob() );

    // False, not an exception, for a dimension that does not exist; int, bool and choice curves are constant
    if ( !knob || !dynamic_cast<KnobDoubleBase*>( knob.get() ) || (dimension < 0) || ( dimension >= knob->getDimension() ) ) {
        return false;
    }
    recordForUndoGroup(knob);

    return knob->moveDerivativesAtTime(eCurveChangeReasonInternal, ViewSpec::current(), dimension, time, left, right);
}

void
Param::_addAsDependencyOf(int fromExprDimension,
                          Param* param,
                          int thisDimension)
{
    //from expr is in the dimension of expressionKnob
    //thisDimension is in the dimesnion of getValueCallerKnob

    KnobIPtr expressionKnob = knobOrRaise( param->_knob.lock() );
    KnobIPtr getValueCallerKnob = knobOrRaise( _knob.lock() );

    if (!expressionKnob || !getValueCallerKnob) {
        return;
    }
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
    KnobIPtr thisKnob = knobOrRaise( _knob.lock() );
    if (!thisKnob) {
        return false;
    }
    if ( (dimension < 0) || ( dimension >= thisKnob->getDimension() ) ) {
        return false;
    }
    const std::string oldExpression = thisKnob->getExpression(dimension);
    const bool oldHasRetVariable = thisKnob->isExpressionUsingRetVariable(dimension);
    const bool oldIsValid = thisKnob->isExpressionValid(dimension, 0);
    bool ok = false;
    try {
        recordForUndoGroup(thisKnob);
        thisKnob->setExpression(dimension, expr.toStdString(), hasRetVariable, true);
        ok = true;
    } catch (const std::exception& e) {
        // Keep returning False (no exception, for the scripts that test the result), but say why,
        // else the failure is easy to miss (#1250)
        KnobHolder* holder = thisKnob->getHolder();
        AppInstancePtr app = holder ? holder->getApp() : AppInstancePtr();
        if (app) {
            std::string name = thisKnob->getName();
            EffectInstance* effect = dynamic_cast<EffectInstance*>(holder);
            if ( effect && effect->getNode() ) {
                name = effect->getNode()->getFullyQualifiedName() + '.' + name;
            }
            // In GUI sessions, the error starts with the whole generated script (with two lines per
            // node of the group): only keep the Python error
            std::string error = e.what();
            const std::string pythonError("Python error:\n");
            std::size_t found = error.find(pythonError);
            if (found != std::string::npos) {
                error = error.substr( found + pythonError.size() );
            }
            app->appendToScriptEditor( QString::fromUtf8("setExpression(): invalid expression on %1 (dimension %2): %3")
                                       .arg( QString::fromUtf8( name.c_str() ) )
                                       .arg(dimension)
                                       .arg( QString::fromUtf8( error.c_str() ) ).toStdString() );
        }
    } catch (...) {
    }

    // The value of the parameter changes with its expression: tell its holder (e.g. kOfxActionInstanceChanged for an
    // OpenFX plug-in, and the onParamChanged callback) and re-render, as the GUI does (SetExpressionCommand), with the
    // reason of setValue() (#1336). As setValue(), only if something changed, e.g. an invalid expression of a loaded
    // project that the same text makes valid again. A failed setExpression() also removes the previous expression.
    const std::string newExpression = thisKnob->getExpression(dimension);
    if ( (newExpression != oldExpression) ||
         ( !newExpression.empty() && ( (thisKnob->isExpressionUsingRetVariable(dimension) != oldHasRetVariable) ||
                                       (thisKnob->isExpressionValid(dimension, 0) != oldIsValid) ) ) ) {
        thisKnob->evaluateValueChange(dimension, thisKnob->getCurrentTime(), ViewIdx(0), eValueChangedReasonNatronInternalEdited);
    }

    return ok;
}

QString
AnimatedParam::getExpression(int dimension,
                             bool* hasRetVariable) const
{
    KnobIPtr thisKnob = knobOrRaise( _knob.lock() );
    if (!thisKnob) {
        return QString();
    }
    QString ret = QString::fromUtf8( thisKnob->getExpression(dimension).c_str() );

    *hasRetVariable = thisKnob->isExpressionUsingRetVariable(dimension);

    return ret;
}

///////////// IntParam

IntParam::IntParam(const KnobIntPtr& knob)
    : AnimatedParam( std::dynamic_pointer_cast<KnobI>(knob) )
    , _intKnob(knob)
{
}

IntParam::~IntParam()
{
}

int
IntParam::get() const
{
    KnobIntPtr knob = knobOrRaise( _intKnob.lock() );
    if (!knob) {
        return 0;
    }

    return knob->getValue();
}

Int2DTuple
Int2DParam::get() const
{
    Int2DTuple ret = {0, 0};
    KnobIntPtr knob = knobOrRaise( _intKnob.lock() );
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
    KnobIntPtr knob = knobOrRaise( _intKnob.lock() );
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
    KnobIntPtr knob = knobOrRaise( _intKnob.lock() );
    if (!knob) {
        return 0;
    }

    return knob->getValueAtTime(frame, 0);
}

Int2DTuple
Int2DParam::get(double frame) const
{
    Int2DTuple ret = {0, 0};
    KnobIntPtr knob = knobOrRaise( _intKnob.lock() );
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
    KnobIntPtr knob = knobOrRaise( _intKnob.lock() );
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
    KnobIntPtr knob = knobOrRaise( _intKnob.lock() );
    if (!knob) {
        return;
    }
    recordForUndoGroup(knob);
    knob->setValue(x, ViewSpec::current(), 0);
}

void
Int2DParam::set(int x,
                int y)
{
    KnobIntPtr knob = knobOrRaise( _intKnob.lock() );
    if (!knob) {
        return;
    }
    knob->beginChanges();
    recordForUndoGroup(knob);
    knob->setValue(x, ViewSpec::current(), 0);
    recordForUndoGroup(knob);
    knob->setValue(y, ViewSpec::current(), 1);
    knob->endChanges();
}

void
Int3DParam::set(int x,
                int y,
                int z)
{
    KnobIntPtr knob = knobOrRaise( _intKnob.lock() );
    if (!knob) {
        return;
    }

    knob->beginChanges();
    recordForUndoGroup(knob);
    knob->setValue(x, ViewSpec::current(), 0);
    recordForUndoGroup(knob);
    knob->setValue(y, ViewSpec::current(), 1);
    recordForUndoGroup(knob);
    knob->setValue(z, ViewSpec::current(), 2);
    knob->endChanges();
}

void
IntParam::set(int x,
              double frame)
{
    KnobIntPtr knob = knobOrRaise( _intKnob.lock() );
    if (!knob) {
        return;
    }
    recordForUndoGroup(knob);
    knob->setValueAtTime(frame, x, ViewSpec::current(), 0);
}

void
Int2DParam::set(int x,
                int y,
                double frame)
{
    KnobIntPtr knob = knobOrRaise( _intKnob.lock() );
    if (!knob) {
        return;
    }
    recordForUndoGroup(knob);
    knob->setValuesAtTime(frame, x, y, ViewSpec::current(), eValueChangedReasonNatronInternalEdited);
}

void
Int3DParam::set(int x,
                int y,
                int z,
                double frame)
{
    KnobIntPtr knob = knobOrRaise( _intKnob.lock() );
    if (!knob) {
        return;
    }
    recordForUndoGroup(knob);
    knob->setValuesAtTime(frame, x, y, z, ViewSpec::current(), eValueChangedReasonNatronInternalEdited);
}

int
IntParam::getValue(int dimension) const
{
    KnobIntPtr knob = knobOrRaise( _intKnob.lock() );
    if (!knob) {
        return 0;
    }
    return knob->getValue(dimension);
}

void
IntParam::setValue(int value,
                   int dimension)
{
    KnobIntPtr knob = knobOrRaise( _intKnob.lock() );
    if (!knob) {
        return;
    }
    recordForUndoGroup(knob);
    knob->setValue(value, ViewSpec::current(), dimension);
}

int
IntParam::getValueAtTime(double time,
                         int dimension) const
{
    KnobIntPtr knob = knobOrRaise( _intKnob.lock() );
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
    KnobIntPtr knob = knobOrRaise( _intKnob.lock() );
    if (!knob) {
        return;
    }
    recordForUndoGroup(knob);
    knob->setValueAtTime(time, value, ViewSpec::current(), dimension);
}

void
IntParam::setDefaultValue(int value,
                          int dimension)
{
    KnobIntPtr knob = knobOrRaise( _intKnob.lock() );
    if (!knob) {
        return;
    }
    knob->setDefaultValueWithoutApplying(value, dimension);
}

// The default value of a dimension of knob, or 0, as getValue(dimension), without the knob or the dimension
template <typename T, typename KNOB>
static T
defaultValueOf(const std::shared_ptr<KNOB>& knob,
               int dimension)
{
    if (!knob) {
        return T();
    }
    const std::vector<T> defaultValues = knob->getDefaultValues_mt_safe();

    return ( (dimension >= 0) && ( dimension < (int)defaultValues.size() ) ) ? defaultValues[dimension] : T();
}

int
IntParam::getDefaultValue(int dimension) const
{
    return defaultValueOf<int>(knobOrRaise( _intKnob.lock() ), dimension);
}

void
IntParam::restoreDefaultValue(int dimension)
{
    KnobIntPtr knob = knobOrRaise( _intKnob.lock() );
    if (!knob) {
        return;
    }
    recordForUndoGroup(knob);
    knob->resetToDefaultValueWithoutSecretNessAndEnabledNess(dimension);
}

void
IntParam::setMinimum(int minimum,
                     int dimension)
{
    KnobIntPtr knob = knobOrRaise( _intKnob.lock() );
    if (!knob) {
        return;
    }
    knob->setMinimum(minimum, dimension);
}

int
IntParam::getMinimum(int dimension) const
{
    KnobIntPtr knob = knobOrRaise( _intKnob.lock() );
    if (!knob) {
        return 0;
    }
    return knob->getMinimum(dimension);
}

void
IntParam::setMaximum(int maximum,
                     int dimension)
{
    KnobIntPtr knob = knobOrRaise( _intKnob.lock() );
    if ( !knob || !knob->isUserKnob() ) {
        return;
    }
    knob->setMaximum(maximum, dimension);
}

int
IntParam::getMaximum(int dimension) const
{
    KnobIntPtr knob = knobOrRaise( _intKnob.lock() );
    if (!knob) {
        return 0;
    }
    return knob->getMaximum(dimension);
}

void
IntParam::setDisplayMinimum(int minimum,
                            int dimension)
{
    KnobIntPtr knob = knobOrRaise( _intKnob.lock() );
    if ( !knob || !knob->isUserKnob() ) {
        return;
    }
    knob->setDisplayMinimum(minimum, dimension);
}

int
IntParam::getDisplayMinimum(int dimension) const
{
    KnobIntPtr knob = knobOrRaise( _intKnob.lock() );
    if (!knob) {
        return 0;
    }
    return knob->getDisplayMinimum(dimension);
}

void
IntParam::setDisplayMaximum(int maximum,
                            int dimension)
{
    KnobIntPtr knob = knobOrRaise( _intKnob.lock() );
    if (!knob) {
        return;
    }
    knob->setDisplayMaximum(maximum, dimension);
}

int
IntParam::getDisplayMaximum(int dimension) const
{
    KnobIntPtr knob = knobOrRaise( _intKnob.lock() );
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

    KnobIntPtr knob = knobOrRaise( _intKnob.lock() );
    if (!knob) {
        return 0;
    }
    return knob->getValue();
}

//////////// DoubleParam

DoubleParam::DoubleParam(const KnobDoublePtr& knob)
    : AnimatedParam( std::dynamic_pointer_cast<KnobI>(knob) )
    , _doubleKnob(knob)
{
}

DoubleParam::~DoubleParam()
{
}

double
DoubleParam::get() const
{
    KnobDoublePtr knob = knobOrRaise( _doubleKnob.lock() );
    if (!knob) {
        return 0.;
    }
    return knob->getValue(0);
}

Double2DTuple
Double2DParam::get() const
{
    KnobDoublePtr knob = knobOrRaise( _doubleKnob.lock() );
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
    KnobDoublePtr knob = knobOrRaise( _doubleKnob.lock() );
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
    KnobDoublePtr knob = knobOrRaise( _doubleKnob.lock() );
    if (!knob) {
        return 0.;
    }
    return knob->getValueAtTime(frame, 0);
}

Double2DTuple
Double2DParam::get(double frame) const
{
    Double2DTuple ret = {0., 0.};
    KnobDoublePtr knob = knobOrRaise( _doubleKnob.lock() );
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
    KnobDoublePtr knob = knobOrRaise( _doubleKnob.lock() );
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
    KnobDoublePtr knob = knobOrRaise( _doubleKnob.lock() );
    if (!knob) {
        return;
    }
    recordForUndoGroup(knob);
    knob->setValue(x, ViewSpec::current(), 0);
}

void
Double2DParam::set(double x,
                   double y)
{
    KnobDoublePtr knob = knobOrRaise( _doubleKnob.lock() );
    if (!knob) {
        return;
    }

    recordForUndoGroup(knob);
    knob->setValues(x, y, ViewSpec::current(), eValueChangedReasonNatronInternalEdited);
}

void
Double3DParam::set(double x,
                   double y,
                   double z)
{
    KnobDoublePtr knob = knobOrRaise( _doubleKnob.lock() );
    if (!knob) {
        return;
    }

    recordForUndoGroup(knob);
    knob->setValues(x, y, z, ViewSpec::current(), eValueChangedReasonNatronInternalEdited);
}

void
DoubleParam::set(double x,
                 double frame)
{
    KnobDoublePtr knob = knobOrRaise( _doubleKnob.lock() );
    if (!knob) {
        return;
    }
    recordForUndoGroup(knob);
    return knob->setValueAtTime(frame, x, ViewSpec::current(), 0);
}

void
Double2DParam::set(double x,
                   double y,
                   double frame)
{
    KnobDoublePtr knob = knobOrRaise( _doubleKnob.lock() );
    if (!knob) {
        return;
    }

    recordForUndoGroup(knob);
    knob->setValuesAtTime(frame, x, y, ViewSpec::current(), eValueChangedReasonNatronInternalEdited);
}

void
Double2DParam::setUsePointInteract(bool use)
{
    KnobDoublePtr knob = knobOrRaise( _doubleKnob.lock() );
    if (!knob) {
        return;
    }
    return knob->setHasHostOverlayHandle(use);
}


void
Double2DParam::setCanAutoFoldDimensions(bool can)
{
    std::shared_ptr<KnobDouble> knob = knobOrRaise( _doubleKnob.lock() );
    if (!knob) {
        return;
    }
    return knob->setCanAutoFoldDimensions(can);
}


void
Double3DParam::set(double x,
                   double y,
                   double z,
                   double frame)
{
    KnobDoublePtr knob = knobOrRaise( _doubleKnob.lock() );
    if (!knob) {
        return;
    }

    recordForUndoGroup(knob);
    knob->setValuesAtTime(frame, x, y, z, ViewSpec::current(), eValueChangedReasonNatronInternalEdited);
}

double
DoubleParam::getValue(int dimension) const
{
    KnobDoublePtr knob = knobOrRaise( _doubleKnob.lock() );
    if (!knob) {
        return 0.;
    }
    return knob->getValue(dimension);
}

void
DoubleParam::setValue(double value,
                      int dimension)
{
    KnobDoublePtr knob = knobOrRaise( _doubleKnob.lock() );
    if (!knob) {
        return;
    }
    recordForUndoGroup(knob);
    knob->setValue(value, ViewSpec::current(), dimension);
}

double
DoubleParam::getValueAtTime(double time,
                            int dimension) const
{
    KnobDoublePtr knob = knobOrRaise( _doubleKnob.lock() );
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
    KnobDoublePtr knob = knobOrRaise( _doubleKnob.lock() );
    if (!knob) {
        return;
    }
    recordForUndoGroup(knob);
    return knob->setValueAtTime(time, value, ViewSpec::current(), dimension);
}

void
DoubleParam::setDefaultValue(double value,
                             int dimension)
{
    KnobDoublePtr knob = knobOrRaise( _doubleKnob.lock() );
    if (!knob) {
        return;
    }
    return knob->setDefaultValueWithoutApplying(value, dimension);
}

double
DoubleParam::getDefaultValue(int dimension) const
{
    return defaultValueOf<double>(knobOrRaise( _doubleKnob.lock() ), dimension);
}

void
DoubleParam::restoreDefaultValue(int dimension)
{
    KnobDoublePtr knob = knobOrRaise( _doubleKnob.lock() );
    if (!knob) {
        return;
    }
    recordForUndoGroup(knob);
    return knob->resetToDefaultValueWithoutSecretNessAndEnabledNess(dimension);
}

void
DoubleParam::setMinimum(double minimum,
                        int dimension)
{
    KnobDoublePtr knob = knobOrRaise( _doubleKnob.lock() );
    if (!knob) {
        return;
    }
    knob->setMinimum(minimum, dimension);
}

double
DoubleParam::getMinimum(int dimension) const
{
    KnobDoublePtr knob = knobOrRaise( _doubleKnob.lock() );
    if (!knob) {
        return 0.;
    }
    return knob->getMinimum(dimension);
}

void
DoubleParam::setMaximum(double maximum,
                        int dimension)
{
    KnobDoublePtr knob = knobOrRaise( _doubleKnob.lock() );
    if ( !knob || !knob->isUserKnob() ) {
        return;
    }
    knob->setMaximum(maximum, dimension);
}

double
DoubleParam::getMaximum(int dimension) const
{
    KnobDoublePtr knob = knobOrRaise( _doubleKnob.lock() );
    if (!knob) {
        return 0.;
    }
    return knob->getMaximum(dimension);
}

void
DoubleParam::setDisplayMinimum(double minimum,
                               int dimension)
{
    KnobDoublePtr knob = knobOrRaise( _doubleKnob.lock() );
    if ( !knob || !knob->isUserKnob() ) {
        return;
    }
    knob->setDisplayMinimum(minimum, dimension);
}

double
DoubleParam::getDisplayMinimum(int dimension) const
{
    KnobDoublePtr knob = knobOrRaise( _doubleKnob.lock() );
    if (!knob) {
        return 0.;
    }
    return knob->getDisplayMinimum(dimension);
}

void
DoubleParam::setDisplayMaximum(double maximum,
                               int dimension)
{
    KnobDoublePtr knob = knobOrRaise( _doubleKnob.lock() );
    if (!knob) {
        return;
    }
    return knob->setDisplayMaximum(maximum, dimension);
}

double
DoubleParam::getDisplayMaximum(int dimension) const
{
    KnobDoublePtr knob = knobOrRaise( _doubleKnob.lock() );
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

    KnobDoublePtr knob = knobOrRaise( _doubleKnob.lock() );
    if (!knob) {
        return 0.;
    }
    return knob->getValue();
}

////////ColorParam

ColorParam::ColorParam(const KnobColorPtr& knob)
    : AnimatedParam( std::dynamic_pointer_cast<KnobI>(knob) )
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
    KnobColorPtr knob = knobOrRaise( _colorKnob.lock() );
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
    KnobColorPtr knob = knobOrRaise( _colorKnob.lock() );
    if (!knob) {
        return ret;
    }
    ret.r = knob->getValueAtTime(frame, 0);
    ret.g = knob->getValueAtTime(frame, 1);
    ret.b = knob->getValueAtTime(frame, 2);
    ret.a = knob->getDimension() == 4 ? knob->getValueAtTime(frame, 3) : 1.;

    return ret;
}

void
ColorParam::set(double r,
                double g,
                double b,
                double a)
{
    KnobColorPtr knob = knobOrRaise( _colorKnob.lock() );
    if (!knob) {
        return;
    }
    knob->beginChanges();
    recordForUndoGroup(knob);
    knob->setValue(r, ViewSpec::current(), 0);
    recordForUndoGroup(knob);
    knob->setValue(g, ViewSpec::current(), 1);
    recordForUndoGroup(knob);
    knob->setValue(b, ViewSpec::current(), 2);
    if (knob->getDimension() == 4) {
        recordForUndoGroup(knob);
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
    KnobColorPtr knob = knobOrRaise( _colorKnob.lock() );
    if (!knob) {
        return;
    }
    knob->beginChanges();
    recordForUndoGroup(knob);
    knob->setValueAtTime(frame, r, ViewSpec::current(), 0);
    recordForUndoGroup(knob);
    knob->setValueAtTime(frame, g, ViewSpec::current(), 1);
    int dims = knob->getDimension();
    recordForUndoGroup(knob);
    knob->setValueAtTime(frame, b, ViewSpec::current(), 2);
    if (dims == 4) {
        recordForUndoGroup(knob);
        knob->setValueAtTime(frame, a, ViewSpec::current(), 3);
    }
    knob->endChanges();
}

double
ColorParam::getValue(int dimension) const
{
    KnobColorPtr knob = knobOrRaise( _colorKnob.lock() );
    if (!knob) {
        return 0.;
    }
    return knob->getValue(dimension);
}

void
ColorParam::setValue(double value,
                     int dimension)
{
    KnobColorPtr knob = knobOrRaise( _colorKnob.lock() );
    if (!knob) {
        return;
    }
    recordForUndoGroup(knob);
    knob->setValue(value, ViewSpec::current(), dimension);
}

double
ColorParam::getValueAtTime(double time,
                           int dimension) const
{
    KnobColorPtr knob = knobOrRaise( _colorKnob.lock() );
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
    KnobColorPtr knob = knobOrRaise( _colorKnob.lock() );
    if (!knob) {
        return;
    }
    recordForUndoGroup(knob);
    knob->setValueAtTime(time, value, ViewSpec::current(), dimension);
}

void
ColorParam::setDefaultValue(double value,
                            int dimension)
{
    KnobColorPtr knob = knobOrRaise( _colorKnob.lock() );
    if (!knob) {
        return;
    }
    knob->setDefaultValueWithoutApplying(value, dimension);
}

double
ColorParam::getDefaultValue(int dimension) const
{
    return defaultValueOf<double>(knobOrRaise( _colorKnob.lock() ), dimension);
}

void
ColorParam::restoreDefaultValue(int dimension)
{
    KnobColorPtr knob = knobOrRaise( _colorKnob.lock() );
    if (!knob) {
        return;
    }
    recordForUndoGroup(knob);
    knob->resetToDefaultValueWithoutSecretNessAndEnabledNess(dimension);
}

void
ColorParam::setMinimum(double minimum,
                       int dimension)
{
    KnobColorPtr knob = knobOrRaise( _colorKnob.lock() );
    if (!knob) {
        return;
    }
    knob->setMinimum(minimum, dimension);
}

double
ColorParam::getMinimum(int dimension) const
{
    KnobColorPtr knob = knobOrRaise( _colorKnob.lock() );
    if (!knob) {
        return 0.;
    }
    return knob->getMinimum(dimension);
}

void
ColorParam::setMaximum(double maximum,
                       int dimension)
{
    KnobColorPtr knob = knobOrRaise( _colorKnob.lock() );
    if ( !knob || !knob->isUserKnob() ) {
        return;
    }
    knob->setMaximum(maximum, dimension);
}

double
ColorParam::getMaximum(int dimension) const
{
    KnobColorPtr knob = knobOrRaise( _colorKnob.lock() );
    if (!knob) {
        return 0.;
    }
    return knob->getMaximum(dimension);
}

void
ColorParam::setDisplayMinimum(double minimum,
                              int dimension)
{
    KnobColorPtr knob = knobOrRaise( _colorKnob.lock() );
    if ( !knob || !knob->isUserKnob() ) {
        return;
    }
    knob->setDisplayMinimum(minimum, dimension);
}

double
ColorParam::getDisplayMinimum(int dimension) const
{
    KnobColorPtr knob = knobOrRaise( _colorKnob.lock() );
    if (!knob) {
        return 0.;
    }
    return knob->getDisplayMinimum(dimension);
}

void
ColorParam::setDisplayMaximum(double maximum,
                              int dimension)
{
    KnobColorPtr knob = knobOrRaise( _colorKnob.lock() );
    if (!knob) {
        return;
    }
    knob->setDisplayMaximum(maximum, dimension);
}

double
ColorParam::getDisplayMaximum(int dimension) const
{
    KnobColorPtr knob = knobOrRaise( _colorKnob.lock() );
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

    KnobColorPtr knob = knobOrRaise( _colorKnob.lock() );
    if (!knob) {
        return 0.;
    }
    return knob->getValue();
}

//////////////// ChoiceParam
ChoiceParam::ChoiceParam(const KnobChoicePtr& knob)
    : AnimatedParam( std::dynamic_pointer_cast<KnobI>(knob) )
    , _choiceKnob(knob)
{
}

ChoiceParam::~ChoiceParam()
{
}

int
ChoiceParam::get() const
{
    KnobChoicePtr knob = knobOrRaise( _choiceKnob.lock() );
    if (!knob) {
        return 0;
    }
    return knob->getValue(0);
}

int
ChoiceParam::get(double frame) const
{
    KnobChoicePtr knob = knobOrRaise( _choiceKnob.lock() );
    if (!knob) {
        return 0;
    }
    return knob->getValueAtTime(frame, 0);
}

void
ChoiceParam::set(int x)
{
    KnobChoicePtr knob = knobOrRaise( _choiceKnob.lock() );
    if (!knob) {
        return;
    }
    recordForUndoGroup(knob);
    knob->setValue(x, ViewSpec::current(), 0);
}

void
ChoiceParam::set(int x,
                 double frame)
{
    KnobChoicePtr knob = knobOrRaise( _choiceKnob.lock() );
    if (!knob) {
        return;
    }
    recordForUndoGroup(knob);
    knob->setValueAtTime(frame, x, ViewSpec::current(), 0);
}

void
ChoiceParam::set(const QString& label)
{
    KnobChoicePtr knob = knobOrRaise( _choiceKnob.lock() );
    if (!knob) {
        return;
    }
    std::string choiceID = label.toStdString();
    // the parameters of a Roto shape (Bezier.getCompositingOperatorParam()) have no holder
    KnobHolder* holder = knob->getHolder();
    AppInstancePtr app = holder ? holder->getApp() : AppInstancePtr();
    if (app && app->isCreatingPythonGroup()) {
        // Before Natron 2.2.3, all dynamic choice parameters for multiplane had a string parameter.
        // The string parameter had the same name as the choice parameter plus "Choice" appended.
        // If we found such a parameter, retrieve the string from it.
        filterKnobChoiceOptionCompat(std::string(), -1, -1, -1, -1, -1, knob->getName(), &choiceID);
    }
    recordForUndoGroup(knob);
    KnobHelper::ValueChangedReturnCodeEnum s = knob->setValueFromID(choiceID, 0);

    Q_UNUSED(s);
    if ( app && app->isCreatingPythonGroup() &&
         (knob->getOptionRestoration() == KnobChoice::eOptionRestorationMatch) &&
         ( KnobChoice::choiceMatch(choiceID, knob->getEntries_mt_safe(), 0) < 0 ) ) {
        // A PyPlug or a Python project file: the option may not exist yet, e.g. a layer of an input that is
        // connected later. Keep it, as the .ntp loader does (KnobChoice::choiceRestoration()): it is
        // selected when the options are refreshed (KnobChoice::findAndSetOldChoice()). The options of a
        // StrChoice param are fixed by the plug-in: an option that it removed leaves the value unchanged.
        knob->setActiveEntry( ChoiceOption(choiceID) );
    }
}

int
ChoiceParam::getValue() const
{
    KnobChoicePtr knob = knobOrRaise( _choiceKnob.lock() );
    if (!knob) {
        return 0;
    }
    return knob->getValue(0);
}

void
ChoiceParam::setValue(int value)
{
    KnobChoicePtr knob = knobOrRaise( _choiceKnob.lock() );
    if (!knob) {
        return;
    }
    recordForUndoGroup(knob);
    knob->setValue(value, ViewSpec::current(), 0);
}

void
ChoiceParam::setValue(const QString& label)
{
    set(label);
}

int
ChoiceParam::getValueAtTime(double time) const
{
    KnobChoicePtr knob = knobOrRaise( _choiceKnob.lock() );
    if (!knob) {
        return 0;
    }
    return knob->getValueAtTime(time, 0);
}

void
ChoiceParam::setValueAtTime(int value,
                            double time)
{
    KnobChoicePtr knob = knobOrRaise( _choiceKnob.lock() );
    if (!knob) {
        return;
    }
    recordForUndoGroup(knob);
    knob->setValueAtTime(time, value, ViewSpec::current(), 0);
}

void
ChoiceParam::setDefaultValue(int value)
{
    KnobChoicePtr knob = knobOrRaise( _choiceKnob.lock() );
    if (!knob) {
        return;
    }
    knob->setDefaultValueWithoutApplying(value, 0);
}

void
ChoiceParam::setDefaultValue(const QString& value)
{
    KnobChoicePtr knob = knobOrRaise( _choiceKnob.lock() );
    if (!knob) {
        return;
    }
    knob->setDefaultValueFromIDWithoutApplying( value.toStdString() );
}

int
ChoiceParam::getDefaultValue() const
{
    KnobChoicePtr knob = knobOrRaise( _choiceKnob.lock() );
    if (!knob) {
        return 0;
    }
    return knob->getDefaultValues_mt_safe()[0];
}

void
ChoiceParam::restoreDefaultValue()
{
    KnobChoicePtr knob = knobOrRaise( _choiceKnob.lock() );
    if (!knob) {
        return;
    }
    recordForUndoGroup(knob);
    knob->resetToDefaultValueWithoutSecretNessAndEnabledNess(0);
}

void
ChoiceParam::addOption(const QString& option,
                       const QString& help)
{
    KnobChoicePtr knob = knobOrRaise( _choiceKnob.lock() );

    if ( !knob || !knob->isUserKnob() ) {
        return;
    }

    ChoiceOption opt(option.toStdString(), "", help.toStdString());
    knob->appendChoice(opt);
}

void
ChoiceParam::setOptions(const std::list<std::pair<QString, QString> >& options)
{
    KnobChoicePtr knob = knobOrRaise( _choiceKnob.lock() );

    if ( !knob || !knob->isUserKnob() ) {
        return;
    }

    std::vector<ChoiceOption> entries;
    for (std::list<std::pair<QString, QString> >::const_iterator it = options.begin(); it != options.end(); ++it) {
        entries.push_back( ChoiceOption(it->first.toStdString(), "", it->second.toStdString()));
    }
    knob->populateChoices(entries);
}

QString
ChoiceParam::getOption(int index) const
{
    KnobChoicePtr knob = knobOrRaise( _choiceKnob.lock() );
    if (!knob) {
        return QString();
    }
    std::vector<ChoiceOption> entries = knob->getEntries_mt_safe();

    if ( (index < 0) || ( index >= (int)entries.size() ) ) {
        return QString();
    }

    return QString::fromUtf8( entries[index].id.c_str() );
}

int
ChoiceParam::getNumOptions() const
{
    KnobChoicePtr knob = knobOrRaise( _choiceKnob.lock() );
    if (!knob) {
        return 0;
    }
    return knob->getNumEntries();
}

QStringList
ChoiceParam::getOptions() const
{
    QStringList ret;
    KnobChoicePtr knob = knobOrRaise( _choiceKnob.lock() );
    if (!knob) {
        return ret;
    }
    std::vector<ChoiceOption> entries = knob->getEntries_mt_safe();

    for (std::size_t i = 0; i < entries.size(); ++i) {
        ret.push_back( QString::fromUtf8( entries[i].id.c_str() ) );
    }

    return ret;
}

int
ChoiceParam::addAsDependencyOf(int fromExprDimension,
                               Param* param,
                               int thisDimension)
{
    _addAsDependencyOf(fromExprDimension, param, thisDimension);

    KnobChoicePtr knob = knobOrRaise( _choiceKnob.lock() );
    if (!knob) {
        return 0;
    }
    return knob->getValue();
}

////////////////BooleanParam


BooleanParam::BooleanParam(const KnobBoolPtr& knob)
    : AnimatedParam( std::dynamic_pointer_cast<KnobI>(knob) )
    , _boolKnob(knob)
{
}

BooleanParam::~BooleanParam()
{
}

bool
BooleanParam::get() const
{
    KnobBoolPtr knob = knobOrRaise( _boolKnob.lock() );
    if (!knob) {
        return false;
    }

    return knob->getValue(0);
}

bool
BooleanParam::get(double frame) const
{
    KnobBoolPtr knob = knobOrRaise( _boolKnob.lock() );
    if (!knob) {
        return false;
    }

    return knob->getValueAtTime(frame, 0);
}

void
BooleanParam::set(bool x)
{
    KnobBoolPtr knob = knobOrRaise( _boolKnob.lock() );
    if (!knob) {
        return;
    }

    recordForUndoGroup(knob);
    knob->setValue(x, ViewSpec::current(), 0);
}

void
BooleanParam::set(bool x,
                  double frame)
{
    KnobBoolPtr knob = knobOrRaise( _boolKnob.lock() );
    if (!knob) {
        return;
    }

    recordForUndoGroup(knob);
    knob->setValueAtTime(frame, x, ViewSpec::current(), 0);
}

bool
BooleanParam::getValue() const
{
    KnobBoolPtr knob = knobOrRaise( _boolKnob.lock() );
    if (!knob) {
        return false;
    }

    return knob->getValue(0);
}

void
BooleanParam::setValue(bool value)
{
    KnobBoolPtr knob = knobOrRaise( _boolKnob.lock() );
    if (!knob) {
        return;
    }

    recordForUndoGroup(knob);
    knob->setValue(value, ViewSpec::current(), 0);
}

bool
BooleanParam::getValueAtTime(double time) const
{
    KnobBoolPtr knob = knobOrRaise( _boolKnob.lock() );
    if (!knob) {
        return false;
    }

    return knob->getValueAtTime(time, 0);
}

void
BooleanParam::setValueAtTime(bool value,
                             double time)
{
    KnobBoolPtr knob = knobOrRaise( _boolKnob.lock() );
    if (!knob) {
        return;
    }

    recordForUndoGroup(knob);
    knob->setValueAtTime(time, value, ViewSpec::current(), 0);
}

void
BooleanParam::setDefaultValue(bool value)
{
    KnobBoolPtr knob = knobOrRaise( _boolKnob.lock() );
    if (!knob) {
        return;
    }

    knob->setDefaultValueWithoutApplying(value, 0);
}

bool
BooleanParam::getDefaultValue() const
{
    KnobBoolPtr knob = knobOrRaise( _boolKnob.lock() );
    if (!knob) {
        return false;
    }

    return knob->getDefaultValues_mt_safe()[0];
}

void
BooleanParam::restoreDefaultValue()
{
    KnobBoolPtr knob = knobOrRaise( _boolKnob.lock() );
    if (!knob) {
        return;
    }

    recordForUndoGroup(knob);
    knob->resetToDefaultValueWithoutSecretNessAndEnabledNess(0);
}

bool
BooleanParam::addAsDependencyOf(int fromExprDimension,
                                Param* param,
                                int thisDimension)
{
    _addAsDependencyOf(fromExprDimension, param, thisDimension);

    KnobBoolPtr knob = knobOrRaise( _boolKnob.lock() );
    if (!knob) {
        return false;
    }

    return knob->getValue();
}

////////////// StringParamBase


StringParamBase::StringParamBase(const KnobStringBasePtr& knob)
    : AnimatedParam( std::dynamic_pointer_cast<KnobI>(knob) )
    , _stringKnob(knob)
{
}

StringParamBase::~StringParamBase()
{
}

QString
StringParamBase::get() const
{
    KnobStringBasePtr knob = knobOrRaise( _stringKnob.lock() );
    if (!knob) {
        return QString();
    }

    return QString::fromUtf8( knob->getValue(0).c_str() );
}

QString
StringParamBase::get(double frame) const
{
    KnobStringBasePtr knob = knobOrRaise( _stringKnob.lock() );
    if (!knob) {
        return QString();
    }

    return QString::fromUtf8( knob->getValueAtTime(frame, 0).c_str() );
}

// A file parameter refuses a path that uses [Project] before the project is saved, when [Project] has no value
// (#1539): it raises ValueError and returns false. Not while Natron itself runs Python to create nodes: the script of a
// PyPlug (also pasted or duplicated) or of a project being loaded sets the values it was saved with.
static bool
checkProjectPathIsSet(const KnobStringBasePtr& knob,
                      const QString& value)
{
    if ( !std::dynamic_pointer_cast<KnobFile>(knob) && !std::dynamic_pointer_cast<KnobOutputFile>(knob) ) {
        return true;
    }
    KnobHolder* holder = knob->getHolder();
    AppInstancePtr app = holder ? holder->getApp() : AppInstancePtr();
    const std::string path = value.toStdString();
    if ( !app || app->isCreatingPythonGroup() || app->getProject()->isLoadingProject() ||
         !app->getProject()->usesUnsetProjectPath(path) ) {
        return true;
    }
    PyErr_SetString( PyExc_ValueError, Project::unsetProjectPathMessage(path).toStdString().c_str() );

    return false;
}

void
StringParamBase::set(const QString& x)
{
    KnobStringBasePtr knob = knobOrRaise( _stringKnob.lock() );
    if (!knob) {
        return;
    }

    if ( !checkProjectPathIsSet(knob, x) ) {
        return;
    }
    recordForUndoGroup(knob);
    knob->setValue(x.toStdString(), ViewSpec::current(), 0);
}

void
StringParamBase::set(const QString& x,
                     double frame)
{
    KnobStringBasePtr knob = knobOrRaise( _stringKnob.lock() );
    if (!knob) {
        return;
    }

    if ( !checkProjectPathIsSet(knob, x) ) {
        return;
    }
    recordForUndoGroup(knob);
    knob->setValueAtTime(frame, x.toStdString(), ViewSpec::current(), 0);
}

QString
StringParamBase::getValue() const
{
    KnobStringBasePtr knob = knobOrRaise( _stringKnob.lock() );
    if (!knob) {
        return QString();
    }

    return QString::fromUtf8( knob->getValue(0).c_str() );
}

void
StringParamBase::setValue(const QString& value)
{
    KnobStringBasePtr knob = knobOrRaise( _stringKnob.lock() );
    if (!knob) {
        return;
    }

    if ( !checkProjectPathIsSet(knob, value) ) {
        return;
    }
    recordForUndoGroup(knob);
    knob->setValue(value.toStdString(), ViewSpec::current(), 0);
}

QString
StringParamBase::getValueAtTime(double time) const
{
    KnobStringBasePtr knob = knobOrRaise( _stringKnob.lock() );
    if (!knob) {
        return QString();
    }

    return QString::fromUtf8( knob->getValueAtTime(time, 0).c_str() );
}

void
StringParamBase::setValueAtTime(const QString& value,
                                double time)
{
    KnobStringBasePtr knob = knobOrRaise( _stringKnob.lock() );
    if (!knob) {
        return;
    }

    if ( !checkProjectPathIsSet(knob, value) ) {
        return;
    }
    recordForUndoGroup(knob);
    knob->setValueAtTime(time, value.toStdString(), ViewSpec::current(), 0);
}

void
StringParamBase::setDefaultValue(const QString& value)
{
    KnobStringBasePtr knob = knobOrRaise( _stringKnob.lock() );
    if (!knob) {
        return;
    }
    if ( !checkProjectPathIsSet(knob, value) ) {
        return;
    }

    knob->setDefaultValueWithoutApplying(value.toStdString(), 0);
}

QString
StringParamBase::getDefaultValue() const
{
    KnobStringBasePtr knob = knobOrRaise( _stringKnob.lock() );
    if (!knob) {
        return QString();
    }

    return QString::fromUtf8( knob->getDefaultValues_mt_safe()[0].c_str() );
}

void
StringParamBase::restoreDefaultValue()
{
    KnobStringBasePtr knob = knobOrRaise( _stringKnob.lock() );
    if (!knob) {
        return;
    }

    recordForUndoGroup(knob);
    return knob->resetToDefaultValueWithoutSecretNessAndEnabledNess(0);
}

QString
StringParamBase::addAsDependencyOf(int fromExprDimension,
                                   Param* param,
                                   int thisDimension)
{
    _addAsDependencyOf(fromExprDimension, param, thisDimension);

    KnobStringBasePtr knob = knobOrRaise( _stringKnob.lock() );
    if (!knob) {
        return QString();
    }

    return QString::fromUtf8( knob->getValue().c_str() );
}

////////////////////StringParam

StringParam::StringParam(const KnobStringPtr& knob)
    : StringParamBase( std::dynamic_pointer_cast<KnobStringBase>(knob) )
    , _sKnob(knob)
{
}

StringParam::~StringParam()
{
}

void
StringParam::setType(StringParam::TypeEnum type)
{
    KnobStringPtr knob = knobOrRaise( _sKnob.lock() );

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
    : StringParamBase( std::dynamic_pointer_cast<KnobStringBase>(knob) )
    , _sKnob(knob)
{
}

FileParam::~FileParam()
{
}

void
FileParam::setSequenceEnabled(bool enabled)
{
    KnobFilePtr k = knobOrRaise( _sKnob.lock() );

    if ( !k || !k->isUserKnob() ) {
        return;
    }
    if (enabled) {
        k->setAsInputImage();
    }
}

void
FileParam::openFile()
{
    KnobFilePtr k = knobOrRaise( _sKnob.lock() );

    if (k) {
        k->open_file();
    }
}

void
FileParam::reloadFile()
{
    KnobFilePtr k = knobOrRaise( _sKnob.lock() );

    if (k) {
        k->reloadFile();
    }
}

/////////////////////OutputFileParam

OutputFileParam::OutputFileParam(const KnobOutputFilePtr& knob)
    : StringParamBase( std::dynamic_pointer_cast<KnobStringBase>(knob) )
    , _sKnob(knob)
{
}

OutputFileParam::~OutputFileParam()
{
}

void
OutputFileParam::setSequenceEnabled(bool enabled)
{
    KnobOutputFilePtr knob = knobOrRaise( _sKnob.lock() );

    if ( !knob || !knob->isUserKnob() ) {
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
    KnobOutputFilePtr knob = knobOrRaise( _sKnob.lock() );

    if ( !knob || !knob->isUserKnob() ) {
        return;
    }
    knob->open_file();
}

////////////////////PathParam

PathParam::PathParam(const KnobPathPtr& knob)
    : StringParamBase( std::dynamic_pointer_cast<KnobStringBase>(knob) )
    , _sKnob(knob)
{
}

PathParam::~PathParam()
{
}

void
PathParam::setAsMultiPathTable()
{
    KnobPathPtr knob = knobOrRaise( _sKnob.lock() );

    if ( !knob || !knob->isUserKnob() ) {
        return;
    }
    knob->setMultiPath(true);
}

void
PathParam::setTable(const std::list<std::vector<std::string> >& table)
{
    KnobPathPtr k = knobOrRaise( _sKnob.lock() );
    if (!k) {
        return;
    }
    if (!k->isMultiPath()) {
        return;
    }
    try {
        recordForUndoGroup(k);
        k->setTable(table);
    } catch (...) {

    }
}

void
PathParam::getTable(std::list<std::vector<std::string> >* table) const
{
    KnobPathPtr k = knobOrRaise( _sKnob.lock() );
    if (!k) {
        return;
    }
    if (!k->isMultiPath()) {
        return;
    }
    try {
        k->getTable(table);
    } catch (const std::exception& /*e*/) {
        return;
    }
}


////////////////////ButtonParam

ButtonParam::ButtonParam(const KnobButtonPtr& knob)
    : Param(knob)
    , _buttonKnob( std::dynamic_pointer_cast<KnobButton>(knob) )
{
}

ButtonParam::~ButtonParam()
{
}

void
ButtonParam::trigger()
{
    KnobButtonPtr knob = knobOrRaise( _buttonKnob.lock() );
    if (!knob) {
        return;
    }
    knob->trigger();
}

////////////////////SeparatorParam

SeparatorParam::SeparatorParam(const KnobSeparatorPtr& knob)
    : Param(knob)
    , _separatorKnob( std::dynamic_pointer_cast<KnobSeparator>(knob) )
{
}

SeparatorParam::~SeparatorParam()
{
}

///////////////////BytesParam

BytesParam::BytesParam(const KnobBytesPtr& knob)
    : Param(knob)
    , _bytesKnob(knob)
{
}

BytesParam::~BytesParam()
{
}

std::string
BytesParam::getValue() const
{
    KnobBytesPtr knob = knobOrRaise( _bytesKnob.lock() );
    if (!knob) {
        return std::string();
    }

    return knob->getValue(0);
}

void
BytesParam::setValue(const std::string& value)
{
    KnobBytesPtr knob = knobOrRaise( _bytesKnob.lock() );
    if ( !knob || ( value.size() > KnobBytes::maxLength() ) ) {
        return;
    }

    recordForUndoGroup(knob);
    knob->setValue(value, ViewSpec::current(), 0);
}

std::string
BytesParam::getDefaultValue() const
{
    KnobBytesPtr knob = knobOrRaise( _bytesKnob.lock() );
    if (!knob) {
        return std::string();
    }

    return knob->getDefaultValue(0);
}

void
BytesParam::restoreDefaultValue()
{
    KnobBytesPtr knob = knobOrRaise( _bytesKnob.lock() );
    if (!knob) {
        return;
    }

    recordForUndoGroup(knob);
    knob->resetToDefaultValueWithoutSecretNessAndEnabledNess(0);
}

std::size_t
BytesParam::getMaxLength() const
{
    return KnobBytes::maxLength();
}

///////////////////GroupParam

GroupParam::GroupParam(const KnobGroupPtr& knob)
    : Param(knob)
    , _groupKnob( std::dynamic_pointer_cast<KnobGroup>(knob) )
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
    KnobIPtr knob = knobOrRaise( param->getInternalKnob() );

    if ( !knob || !knob->isUserKnob() ) {
        return;
    }
    KnobGroupPtr group = knobOrRaise( _groupKnob.lock() );
    if (!group) {
        return;
    }
    group->addKnob(knob);
}

void
GroupParam::setAsTab()
{
    KnobGroupPtr group = knobOrRaise( _groupKnob.lock() );
    if ( !group || !group->isUserKnob() ) {
        return;
    }
    group->setAsTab();
}

void
GroupParam::setOpened(bool opened)
{
    KnobGroupPtr group = knobOrRaise( _groupKnob.lock() );
    if (!group) {
        return;
    }
    recordForUndoGroup(group);
    group->setValue(opened, ViewSpec::current(), 0);
}

bool
GroupParam::getIsOpened() const
{
    KnobGroupPtr group = knobOrRaise( _groupKnob.lock() );
    if (!group) {
        return false;
    }
    return group->getValue();
}

//////////////////////PageParam

PageParam::PageParam(const KnobPagePtr& knob)
    : Param(knob)
    , _pageKnob( std::dynamic_pointer_cast<KnobPage>(knob) )
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
    KnobIPtr knob = knobOrRaise( param->getInternalKnob() );

    if ( !knob || !knob->isUserKnob() ) {
        return;
    }
    KnobPagePtr pageKnob = knobOrRaise( _pageKnob.lock() );

    if ( !pageKnob ) {
        return;
    }
    pageKnob->addKnob( knob );
}

////////////////////ParametricParam
ParametricParam::ParametricParam(const KnobParametricPtr& knob)
    : Param( std::dynamic_pointer_cast<KnobI>(knob) )
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
    KnobParametricPtr param = knobOrRaise( _parametricKnob.lock() );
    if (!param) {
        return;
    }
    param->setCurveColor(dimension, r, g, b);
}

void
ParametricParam::getCurveColor(int dimension,
                               ColorTuple& ret) const
{
    KnobParametricPtr param = knobOrRaise( _parametricKnob.lock() );
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
    KnobParametricPtr param = knobOrRaise( _parametricKnob.lock() );
    if (!param) {
        return eStatusFailed;
    }
    recordForUndoGroup(param);
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
    KnobParametricPtr param = knobOrRaise( _parametricKnob.lock() );
    if (!param) {
        return eStatusFailed;
    }
    recordForUndoGroup(param);
    return param->addControlPoint(eValueChangedReasonNatronInternalEdited, dimension, key, value, leftDerivative, rightDerivative, interpolation);
}

double
ParametricParam::getValue(int dimension,
                          double parametricPosition) const
{
    KnobParametricPtr param = knobOrRaise( _parametricKnob.lock() );
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
    KnobParametricPtr param = knobOrRaise( _parametricKnob.lock() );
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
    KnobParametricPtr param = knobOrRaise( _parametricKnob.lock() );
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
    KnobParametricPtr param = knobOrRaise( _parametricKnob.lock() );
    if (!param) {
        return eStatusFailed;
    }
    recordForUndoGroup(param);
    return param->setNthControlPoint(eValueChangedReasonNatronInternalEdited, dimension, nthCtl, key, value, leftDerivative, rightDerivative);
}

StatusEnum
ParametricParam::setNthControlPointInterpolation(int dimension,
                                                 int nThCtl,
                                                 KeyframeTypeEnum interpolation)
{
    KnobParametricPtr param = knobOrRaise( _parametricKnob.lock() );
    if (!param) {
        return eStatusFailed;
    }
    return param->setNthControlPointInterpolation(eValueChangedReasonNatronInternalEdited, dimension, nThCtl, interpolation);
}

StatusEnum
ParametricParam::deleteControlPoint(int dimension,
                                    int nthCtl)
{
    KnobParametricPtr param = knobOrRaise( _parametricKnob.lock() );
    if (!param) {
        return eStatusFailed;
    }
    recordForUndoGroup(param);
    return param->deleteControlPoint(eValueChangedReasonNatronInternalEdited, dimension, nthCtl);
}

StatusEnum
ParametricParam::deleteAllControlPoints(int dimension)
{
    KnobParametricPtr param = knobOrRaise( _parametricKnob.lock() );
    if (!param) {
        return eStatusFailed;
    }
    recordForUndoGroup(param);
    return param->deleteAllControlPoints(eValueChangedReasonNatronInternalEdited, dimension);
}

void
ParametricParam::setDefaultCurvesFromCurrentCurves()
{
    KnobParametricPtr param = knobOrRaise( _parametricKnob.lock() );
    if (!param) {
        return;
    }
    param->setDefaultCurvesFromCurves();
}

NATRON_PYTHON_NAMESPACE_EXIT
NATRON_NAMESPACE_EXIT

