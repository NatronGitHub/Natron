/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 * Copyright (C) 2018-2020 The Natron developers
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

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/case_conv.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
#endif

GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
CLANG_DIAG_OFF(mismatched-tags)
GCC_DIAG_OFF(unused-parameter)
#include "natronengine_python.h"
#include <shiboken.h> // produces many warnings
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
CLANG_DIAG_ON(mismatched-tags)
GCC_DIAG_ON(unused-parameter)

#include "Engine/AppInstance.h"
#include "Engine/EffectInstance.h"
#include "Engine/KnobItemsTable.h"
#include "Engine/Node.h"
#include "Engine/AppInstance.h"

#include "Engine/Curve.h"
#include "Engine/Project.h"
#include "Engine/ViewIdx.h"
#include "Engine/LoadKnobsCompat.h"
#include "Engine/PointOverlayInteract.h"
#include "Engine/PyAppInstance.h"
#include "Engine/PyNode.h"
#include "Engine/PyItemsTable.h"

#include "Serialization/SerializationCompat.h"
#include "Serialization/ProjectSerialization.h"


NATRON_NAMESPACE_ENTER
NATRON_PYTHON_NAMESPACE_ENTER

Param::Param(const KnobIPtr& knob)
    : _knob(knob)
{
}

Param::~Param()
{
}

KnobIPtr
Param::getRenderCloneKnobInternal() const
{
    // The main instance knob
    KnobIPtr mainInstanceKnob = _knob.lock();
    if (!mainInstanceKnob) {
        PythonSetNullError();
        return KnobIPtr();
    }
    EffectInstancePtr mainInstanceHolder = toEffectInstance(mainInstanceKnob->getHolder());
    if (!mainInstanceHolder) {
        return mainInstanceKnob;
    }

    // The main instance is not a render clone!
    assert(!mainInstanceHolder->isRenderClone());


    // Get the last Python API effect caller, to retrieve a pointer to the render clone
    EffectInstancePtr tlsCurrentEffect = appPTR->getLastPythonAPICaller_TLS();
    if (!tlsCurrentEffect) {
        return mainInstanceKnob;
    }

    // The final effect from which to fetch the knob
    EffectInstancePtr finalEffect;

    // If The last Python API caller is this node, return the associated knob directly
    if (tlsCurrentEffect->getNode() == mainInstanceHolder->getNode()) {
        finalEffect = tlsCurrentEffect;
    } else {
        if (!tlsCurrentEffect->isRenderClone()) {
            return mainInstanceKnob;
        } else {
            FrameViewRenderKey key = {tlsCurrentEffect->getCurrentRenderTime(), tlsCurrentEffect->getCurrentRenderView(), tlsCurrentEffect->getCurrentRender()};
            finalEffect = toEffectInstance(mainInstanceHolder->createRenderClone(key));
        }

    }

    assert(finalEffect);
    return mainInstanceKnob->getCloneForHolderInternal(finalEffect);

}

Param*
Param::getParent() const
{
    KnobIPtr knob = getInternalKnob();
    if (!knob) {
        PythonSetNullError();
        return 0;
    }
    KnobIPtr parent = knob->getParentKnob();

    if (parent) {
        return Effect::createParamWrapperForKnob(parent);
    } else {
        return 0;
    }
}

Effect*
Param::getParentEffect() const
{
    KnobIPtr knob = getInternalKnob();
    if (!knob) {
        PythonSetNullError();
        return 0;
    }
    KnobHolderPtr holder = knob->getHolder();
    if (!holder) {
        return 0;
    }
    NodePtr node;
    KnobTableItemPtr isItem = toKnobTableItem(holder);
    EffectInstancePtr isEffect = toEffectInstance(holder);
    if (isItem) {
        KnobItemsTablePtr model = isItem->getModel();
        if (!model) {
            return 0;
        }
        node = model->getNode();
    } else if (isEffect) {
        node = isEffect->getNode();
    }
    if (!node) {
        return 0;
    }
    return App::createEffectFromNodeWrapper(node);
}

ItemBase*
Param::getParentItemBase() const
{
    KnobIPtr knob = getInternalKnob();
    if (!knob) {
        PythonSetNullError();
        return 0;
    }
    KnobHolderPtr holder = knob->getHolder();
    if (!holder) {
        return 0;
    }
    KnobTableItemPtr isItem = toKnobTableItem(holder);
    if (!isItem) {
        return 0;
    }
    return ItemsTable::createPyItemWrapper(isItem);
}

App*
Param::getApp() const
{
    KnobIPtr knob = getInternalKnob();
    if (!knob) {
        PythonSetNullError();
        return 0;
    }
    KnobHolderPtr holder = knob->getHolder();
    if (!holder) {
        return 0;
    }
    return App::createAppFromAppInstance(holder->getApp());
}

int
Param::getNumDimensions() const
{
    KnobIPtr knob = getInternalKnob();
    if (!knob) {
        PythonSetNullError();
        return 0;
    }
    return knob->getNDimensions();
}

QString
Param::getScriptName() const
{
    KnobIPtr knob = getInternalKnob();
    if (!knob) {
        PythonSetNullError();
        return QString();
    }
    return QString::fromUtf8( knob->getName().c_str() );
}

QString
Param::getLabel() const
{
    KnobIPtr knob = getInternalKnob();
    if (!knob) {
        PythonSetNullError();
        return QString();
    }
    return QString::fromUtf8( knob->getLabel().c_str() );
}

void
Param::setLabel(const QString& label)
{
    KnobIPtr knob = getInternalKnob();
    if (!knob) {
        PythonSetNullError();
        return;
    }
    if (knob->getKnobDeclarationType() != KnobI::eKnobDeclarationTypeUser) {
        PythonSetNonUserKnobError();
        return;
    }
    knob->setLabel(label);
}

QString
Param::getTypeName() const
{
    KnobIPtr knob = getInternalKnob();
    if (!knob) {
        PythonSetNullError();
        return QString();
    }
    return QString::fromUtf8( knob->typeName().c_str() );
}

QString
Param::getHelp() const
{
    KnobIPtr knob = getInternalKnob();
    if (!knob) {
        PythonSetNullError();

        return QString();
    }
    return QString::fromUtf8( knob->getHintToolTip().c_str() );
}

void
Param::setHelp(const QString& help)
{
    KnobIPtr knob = getInternalKnob();
    if (!knob) {
        PythonSetNullError();
        return;
    }
    if (knob->getKnobDeclarationType() != KnobI::eKnobDeclarationTypeUser) {
        PythonSetNonUserKnobError();
        return;
    }
    knob->setHintToolTip( help.toStdString() );
}

bool
Param::getIsVisible() const
{
    KnobIPtr knob = getInternalKnob();
    if (!knob) {
        PythonSetNullError();
        return false;
    }
    return !knob->getIsSecret();
}

void
Param::setVisible(bool visible)
{
    KnobIPtr knob = getInternalKnob();
    if (!knob) {
        PythonSetNullError();
        return;
    }
    knob->setSecret(!visible);
}

bool
Param::getIsEnabled() const
{

    KnobIPtr knob = getInternalKnob();
    if (!knob) {
        PythonSetNullError();
        return false;
    }
    return knob->isEnabled();
}

void
Param::setEnabled(bool enabled)
{
    KnobIPtr knob = getInternalKnob();
    if (!knob) {
        PythonSetNullError();
        return;
    }

    knob->setEnabled(enabled);
}

bool
Param::getIsPersistent() const
{
    KnobIPtr knob = getInternalKnob();
    if (!knob) {
        PythonSetNullError();
        return false;
    }
    return knob->getIsPersistent();
}

void
Param::setPersistent(bool persistent)
{
    KnobIPtr knob = getInternalKnob();
    if (!knob) {
        PythonSetNullError();
        return;
    }
    if (knob->getKnobDeclarationType() != KnobI::eKnobDeclarationTypeUser) {
        PythonSetNonUserKnobError();
        return;
    }
    knob->setIsPersistent(persistent);
}

void
Param::setExpressionCacheEnabled(bool enabled)
{
    KnobIPtr knob = getInternalKnob();
    if (!knob) {
        PythonSetNullError();
        return;
    }
    knob->setExpressionsResultsCachingEnabled(enabled);
}

bool
Param::isExpressionCacheEnabled() const
{
    KnobIPtr knob = getInternalKnob();
    if (!knob) {
        PythonSetNullError();
        return false;
    }
    return knob->isExpressionsResultsCachingEnabled();
}

bool
Param::getEvaluateOnChange() const
{
    KnobIPtr knob = getInternalKnob();
    if (!knob) {
        PythonSetNullError();
        return false;
    }
    return knob->getEvaluateOnChange();
}

void
Param::setEvaluateOnChange(bool eval)
{
    KnobIPtr knob = getInternalKnob();
    if (!knob) {
        PythonSetNullError();
        return;
    }
    if (knob->getKnobDeclarationType() != KnobI::eKnobDeclarationTypeUser) {
        PythonSetNonUserKnobError();
        return;
    }
    knob->setEvaluateOnChange(eval);
}

bool
Param::getCanAnimate() const
{
    KnobIPtr knob = getInternalKnob();
    if (!knob) {
        PythonSetNullError();
        return false;
    }
    return knob->canAnimate();
}

bool
Param::getIsAnimationEnabled() const
{
    KnobIPtr knob = getInternalKnob();
    if (!knob) {
        PythonSetNullError();
        return false;
    }
    return knob->isAnimationEnabled();
}

void
Param::setAnimationEnabled(bool e)
{
    KnobIPtr knob = getInternalKnob();
    if (!knob) {
        PythonSetNullError();
        return;
    }
    if (knob->getKnobDeclarationType() != KnobI::eKnobDeclarationTypeUser) {
        PythonSetNonUserKnobError();
        return;
    }
    knob->setAnimationEnabled(e);
}

bool
Param::getAddNewLine()
{
    KnobIPtr knob = getInternalKnob();
    if (!knob) {
        PythonSetNullError();
        return false;
    }
    return knob->isNewLineActivated();
}

void
Param::setAddNewLine(bool a)
{
    KnobIPtr knob = getInternalKnob();
    if (!knob) {
        PythonSetNullError();
        return;
    }
    if (knob->getKnobDeclarationType() != KnobI::eKnobDeclarationTypeUser) {
        PythonSetNonUserKnobError();
        return;
    }


    KnobIPtr parentKnob = knob->getParentKnob();
    if (parentKnob) {
        KnobGroupPtr parentIsGrp = toKnobGroup(parentKnob);
        KnobPagePtr parentIsPage = toKnobPage(parentKnob);
        assert(parentIsGrp || parentIsPage);
        if (!parentIsGrp && !parentIsPage) {
            PythonSetNullError();
            return;
        }
        KnobsVec children;
        if (parentIsGrp) {
            children = parentIsGrp->getChildren();
        } else if (parentIsPage) {
            children = parentIsPage->getChildren();
        }
        for (std::size_t i = 0; i < children.size(); ++i) {
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
        PythonSetNullError();
        return false;
    }

    KnobHolderPtr holder = knob->getHolder();
    if (!holder) {
        PythonSetNullError();
        return false;
    }
    return holder->getInViewerContextKnobIndex(knob) != -1;
}

void
Param::setViewerUIVisible(bool visible)
{
    KnobIPtr knob = getInternalKnob();
    if (!knob) {
        PythonSetNullError();
        return;
    }

    knob->setInViewerContextSecret(!visible);
}


bool
Param::getViewerUIVisible() const
{
    KnobIPtr knob = getInternalKnob();
    if (!knob) {
        PythonSetNullError();
        return false;
    }

    return knob->getInViewerContextSecret();
}

void
Param::setViewerUILayoutType(NATRON_NAMESPACE::ViewerContextLayoutTypeEnum type)
{
    KnobIPtr knob = getInternalKnob();
    if (!knob) {
        PythonSetNullError();
        return ;
    }

    knob->setInViewerContextLayoutType(type);
}

NATRON_NAMESPACE::ViewerContextLayoutTypeEnum
Param::getViewerUILayoutType() const
{
    KnobIPtr knob = getInternalKnob();
    if (!knob) {
        PythonSetNullError();
        return eViewerContextLayoutTypeSpacing;
    }

    return knob->getInViewerContextLayoutType();
}


void
Param::setViewerUIItemSpacing(int spacingPx)
{
    KnobIPtr knob = getInternalKnob();
    if (!knob) {
        PythonSetNullError();
        return ;
    }
    knob->setInViewerContextItemSpacing(spacingPx);
}

int
Param::getViewerUIItemSpacing() const
{
    KnobIPtr knob = getInternalKnob();
    if (!knob) {
        PythonSetNullError();
        return  0;
    }
    return knob->getInViewerContextItemSpacing();
}


void
Param::setViewerUIIconFilePath(const QString& icon, bool checked)
{
    KnobIPtr knob = getInternalKnob();
    if (!knob) {
        PythonSetNullError();
        return;
    }
    if (knob->getKnobDeclarationType() != KnobI::eKnobDeclarationTypeUser) {
        PythonSetNonUserKnobError();
        return;
    }
    knob->setInViewerContextIconFilePath(icon.toStdString(), checked);
}

QString
Param::getViewerUIIconFilePath(bool checked) const
{
    KnobIPtr knob = getInternalKnob();
    if (!knob) {
        PythonSetNullError();
        return QString();
    }
    return QString::fromUtf8(knob->getInViewerContextIconFilePath(checked).c_str());
}


void
Param::setViewerUILabel(const QString& label)
{
    KnobIPtr knob = getInternalKnob();
    if (!knob) {
        PythonSetNullError();
        return;
    }
    if (knob->getKnobDeclarationType() != KnobI::eKnobDeclarationTypeUser) {
        PythonSetNonUserKnobError();
        return;
    }

    knob->setInViewerContextLabel(label);
}

QString
Param::getViewerUILabel() const
{
    KnobIPtr knob = getInternalKnob();
    if (!knob) {
        PythonSetNullError();
        return QString();
    }
    return QString::fromUtf8(knob->getInViewerContextLabel().c_str());
}

template <typename VIEWSPECTYPE>
static bool getViewSpecFromViewNameInternal(const Param* param, bool allowAll, const QString& viewName, VIEWSPECTYPE* view) {

    if (allowAll && viewName == QLatin1String(kPyParamViewSetSpecAll)) {
        *view = VIEWSPECTYPE(ViewSetSpec::all());
        return true;
    } else if (viewName == QLatin1String(kPyParamViewIdxMain) ) {
        *view = VIEWSPECTYPE(0);
        return true;
    }
    KnobIPtr knob = param->getInternalKnob();
    if (!knob) {
        return false;
    }
    KnobHolderPtr holder = knob->getHolder();
    if (!holder) {
        return false;
    }
    AppInstancePtr app = holder->getApp();
    if (!app) {
        return false;
    }
    const std::vector<std::string>& projectViews = app->getProject()->getProjectViewNames();
    bool foundView = false;
    ViewIdx foundViewIdx;
    std::string stdViewName = viewName.toStdString();
    foundView = Project::getViewIndex(projectViews, stdViewName, &foundViewIdx);

    if (!foundView) {
        return false;
    }

    // Now check that the view exist in the knob
    std::list<ViewIdx> splitViews = knob->getViewsList();
    for (std::list<ViewIdx>::const_iterator it = splitViews.begin(); it != splitViews.end(); ++it) {
        if (*it == foundViewIdx) {
            *view = VIEWSPECTYPE(foundViewIdx);
            return true;
        }
    }
    *view = VIEWSPECTYPE(0);
    return true;
}

bool
Param::getViewIdxFromViewName(const QString& viewName, ViewIdx* view) const
{
    return getViewSpecFromViewNameInternal(this, false, viewName, view);
}

bool
Param::getViewSetSpecFromViewName(const QString& viewName, ViewSetSpec* view) const
{
    return getViewSpecFromViewNameInternal(this, true, viewName, view);
}

static DimSpec getDimSpecFromDimensionIndex(int dimension) {
    if (dimension == kPyParamDimSpecAll) {
        return DimSpec::all();
    }
    return DimSpec(dimension);
}

bool
Param::copy(Param* other,
            int thisDimension,
            int otherDimension,
            const QString& thisView,
            const QString& otherView)
{
    KnobIPtr thisKnob = _knob.lock();
    KnobIPtr otherKnob = other->_knob.lock();
    if (!thisKnob || ! otherKnob) {
        PythonSetNullError();
        return false;
    }

    if ((thisDimension == kPyParamDimSpecAll && otherDimension != kPyParamDimSpecAll) ||
        (thisDimension != kPyParamDimSpecAll && otherDimension == kPyParamDimSpecAll)) {
        PyErr_SetString(PyExc_ValueError, tr("thisDimension and otherDimension arguments must be either -1 for both or a valid index").toStdString().c_str());
        return false;
    }
    if (thisDimension < 0 || thisDimension >= thisKnob->getNDimensions()) {
        PythonSetInvalidDimensionError(thisDimension);
        return false;
    }
    if (otherDimension < 0 || otherDimension >= otherKnob->getNDimensions()) {
        PythonSetInvalidDimensionError(otherDimension);
        return false;
    }
    if ((thisView == QLatin1String(kPyParamViewSetSpecAll) && otherView != QLatin1String(kPyParamViewSetSpecAll)) ||
        (thisView != QLatin1String(kPyParamViewSetSpecAll) && otherView == QLatin1String(kPyParamViewSetSpecAll))) {
        PyErr_SetString(PyExc_ValueError, tr("thisView and otherView arguments must be either \"All\" for both or a valid view name").toStdString().c_str());
        return false;
    }
    ViewSetSpec thisViewSpec, otherViewSpec;
    if (!getViewSetSpecFromViewName(thisView, &thisViewSpec)) {
        PythonSetInvalidViewName(thisView);
        return false;
    }
    if (!getViewSetSpecFromViewName(otherView, &otherViewSpec)) {
        PythonSetInvalidViewName(otherView);
        return false;
    }
    DimSpec thisDimSpec = getDimSpecFromDimensionIndex(thisDimension);
    DimSpec otherDimSpec = getDimSpecFromDimensionIndex(otherDimension);

    {
        DimIdx thisDimToCheck = thisDimSpec.isAll() ? DimIdx(0) : DimIdx(thisDimSpec);
        ViewIdx thisViewToCheck = thisDimSpec.isAll() ? ViewIdx(0): ViewIdx(thisDimSpec);
        DimIdx otherDimToCheck = otherDimSpec.isAll() ? DimIdx(0) : DimIdx(otherDimSpec);
        ViewIdx otherViewToCheck = otherViewSpec.isAll() ? ViewIdx(0): ViewIdx(otherViewSpec);
        std::string error;
        if ( !thisKnob->canLinkWith(otherKnob, thisDimToCheck, thisViewToCheck, otherDimToCheck, otherViewToCheck,&error ) ) {
            if (!error.empty()) {
                PyErr_SetString(PyExc_ValueError, error.c_str());
            }
            return false;
        }
    }

    thisKnob->copyKnob(otherKnob, thisViewSpec, thisDimSpec, otherViewSpec, otherDimSpec, /*range*/ 0, /*offset*/ 0);
    
    return true;
}

bool
Param::slaveTo(Param* other,
              int thisDimension,
              int otherDimension,
              const QString& thisView,
              const QString& otherView)
{
    return linkTo(other, thisDimension, otherDimension, thisView, otherView);
}

bool
Param::linkTo(Param* other,
               int thisDimension,
               int otherDimension,
               const QString& thisView,
               const QString& otherView)
{
    KnobIPtr thisKnob = _knob.lock();
    KnobIPtr otherKnob = other->_knob.lock();
    if (!thisKnob || ! otherKnob) {
        PythonSetNullError();
        return false;
    }

    if ((thisDimension == kPyParamDimSpecAll && otherDimension != kPyParamDimSpecAll) ||
        (thisDimension != kPyParamDimSpecAll && otherDimension == kPyParamDimSpecAll)) {
        PyErr_SetString(PyExc_ValueError, tr("thisDimension and otherDimension arguments must be either -1 for both or a valid index").toStdString().c_str());
        return false;
    }
    if (thisDimension < 0 || thisDimension >= thisKnob->getNDimensions()) {
        PythonSetInvalidDimensionError(thisDimension);
        return false;
    }
    if (otherDimension < 0 || otherDimension >= otherKnob->getNDimensions()) {
        PythonSetInvalidDimensionError(otherDimension);
        return false;
    }
    if ((thisView == QLatin1String(kPyParamViewSetSpecAll) && otherView != QLatin1String(kPyParamViewSetSpecAll)) ||
        (thisView != QLatin1String(kPyParamViewSetSpecAll) && otherView == QLatin1String(kPyParamViewSetSpecAll))) {
        PyErr_SetString(PyExc_ValueError, tr("thisView and otherView arguments must be either \"All\" for both or a valid view name").toStdString().c_str());
        return false;
    }
    ViewSetSpec thisViewSpec, otherViewSpec;
    if (!getViewSetSpecFromViewName(thisView, &thisViewSpec)) {
        PythonSetInvalidViewName(thisView);
        return false;
    }
    if (!getViewSetSpecFromViewName(otherView, &otherViewSpec)) {
        PythonSetInvalidViewName(otherView);
        return false;
    }
    DimSpec thisDimSpec = getDimSpecFromDimensionIndex(thisDimension);
    DimSpec otherDimSpec = getDimSpecFromDimensionIndex(otherDimension);

    {
        DimIdx thisDimToCheck = thisDimSpec.isAll() ? DimIdx(0) : DimIdx(thisDimSpec);
        ViewIdx thisViewToCheck = thisDimSpec.isAll() ? ViewIdx(0): ViewIdx(thisDimSpec);
        DimIdx otherDimToCheck = otherDimSpec.isAll() ? DimIdx(0) : DimIdx(otherDimSpec);
        ViewIdx otherViewToCheck = otherViewSpec.isAll() ? ViewIdx(0): ViewIdx(otherViewSpec);
        std::string error;
        if ( !thisKnob->canLinkWith(otherKnob, thisDimToCheck, thisViewToCheck, otherDimToCheck, otherViewToCheck,&error ) ) {
            if (!error.empty()) {
                PyErr_SetString(PyExc_ValueError, error.c_str());
            }
            return false;
        }
    }
    return thisKnob->linkTo(otherKnob, thisDimSpec, otherDimSpec, thisViewSpec, otherViewSpec);
}

void
Param::unlink(int dimension, const QString& viewName)
{
    KnobIPtr thisKnob = _knob.lock();

    if (!thisKnob) {
        PythonSetNullError();
        return;
    }

    ViewSetSpec thisViewSpec;
    if (!getViewSetSpecFromViewName(viewName, &thisViewSpec)) {
        PythonSetInvalidViewName(viewName);
        return;
    }
    if (dimension != kPyParamDimSpecAll && (dimension < 0 || dimension >= thisKnob->getNDimensions())) {
        PythonSetInvalidDimensionError(dimension);
        return;
    }

    DimSpec thisDimSpec = getDimSpecFromDimensionIndex(dimension);
    thisKnob->unlink(thisDimSpec, thisViewSpec, false);
}

double
Param::random(double min,
              double max) const
{
    KnobIPtr knob = getRenderCloneKnobInternal();

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
    KnobIPtr knob = getRenderCloneKnobInternal();

    if (!knob) {
        return 0;
    }
    return knob->random(min, max, TimeValue(time), seed);
}

int
Param::randomInt(int min,
                 int max)
{
    KnobIPtr knob = getRenderCloneKnobInternal();

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
    KnobIPtr knob = getRenderCloneKnobInternal();

    if (!knob) {
        return 0;
    }
    return knob->randomInt(min, max, TimeValue(time), seed);
}

double
Param::curve(double time,
             int dimension,
             const QString& viewName) const
{
    KnobIPtr thisKnob = getRenderCloneKnobInternal();

    if (!thisKnob) {
        PythonSetNullError();
        return 0.;
    }
    if (dimension < 0 || dimension >= thisKnob->getNDimensions()) {
        PythonSetInvalidDimensionError(dimension);
        return false;
    }
    ViewIdx thisViewSpec;
    if (!getViewIdxFromViewName(viewName, &thisViewSpec)) {
        PythonSetInvalidViewName(viewName);
        return 0.;
    }
    KeyFrame key;
    bool gotKey = thisKnob->getCurveKeyFrame(TimeValue(time), DimIdx(dimension), thisViewSpec, true /*clamp*/, &key);
    if (gotKey) {
        return key.getValue();
    } else {
        return 0.;
    }
}

bool
Param::setAsAlias(Param* other)
{
    if (!other) {
        return false;
    }
    KnobIPtr otherKnob = other->_knob.lock();
    KnobIPtr thisKnob = getInternalKnob();
    if (!thisKnob || !otherKnob) {
        PythonSetNullError();
        return false;
    }

    if (otherKnob->typeName() != thisKnob->typeName()) {
        PyErr_SetString(PyExc_ValueError, tr("Cannot alias a parameter of a different kind").toStdString().c_str());
        return false;
    }

    if (otherKnob->getNDimensions() != thisKnob->getNDimensions()) {
        PyErr_SetString(PyExc_ValueError, tr("Cannot alias a parameter with a different number of dimensions").toStdString().c_str());
        return false;
    }
    // First copy the original knob
    bool ok = thisKnob->copyKnob(otherKnob);

    // Now link the original knob to this knob
    ok |= otherKnob->linkTo(thisKnob);
    return ok;
}


void
Param::setIconFilePath(const QString& icon, bool checked)
{
    KnobIPtr knob = getInternalKnob();

    if (!knob) {
        PythonSetNullError();
        return;
    }
    knob->setIconLabel( icon.toStdString(), checked, false );
}

void
Param::beginChanges()
{
    KnobIPtr knob = getInternalKnob();

    if (!knob) {
        PythonSetNullError();
        return;
    }
    knob->beginChanges();
    knob->blockValueChanges();
    knob->setAdjustFoldExpandStateAutomatically(false);
}

void
Param::endChanges()
{
    KnobIPtr knob = getInternalKnob();

    if (!knob) {
        PythonSetNullError();
        return;
    }
    knob->endChanges();
    knob->unblockValueChanges();
    knob->setAdjustFoldExpandStateAutomatically(true);
}

AnimatedParam::AnimatedParam(const KnobIPtr& knob)
    : Param(knob)
{
}

AnimatedParam::~AnimatedParam()
{
}

void
AnimatedParam::splitView(const QString& viewName)
{
    KnobIPtr knob = getInternalKnob();
    if (!knob) {
        PythonSetNullError();
        return;
    }
    ViewIdx thisViewSpec;
    if (!getViewIdxFromViewName(viewName, &thisViewSpec)) {
        PythonSetInvalidViewName(viewName);
        return;
    }
    if (!knob->isAnimationEnabled()) {
        PyErr_SetString(PyExc_ValueError, Param::tr("splitView: Cannot split view for a parameter that cannot animate").toStdString().c_str());
        return;
    }
    knob->splitView(ViewIdx(thisViewSpec.value()));
}

void
AnimatedParam::unSplitView(const QString& viewName)
{
    KnobIPtr knob = getInternalKnob();
    if (!knob) {
        PythonSetNullError();
        return;
    }
    ViewIdx thisViewSpec;
    if (!getViewIdxFromViewName(viewName, &thisViewSpec)) {
        PythonSetInvalidViewName(viewName);
        return;
    }
    if (!knob->isAnimationEnabled()) {
        PyErr_SetString(PyExc_ValueError, Param::tr("unSplitView: Cannot unsplit view for a parameter that cannot animate").toStdString().c_str());
        return;
    }
    knob->unSplitView(ViewIdx(thisViewSpec.value()));
}

QStringList
AnimatedParam::getViewsList() const
{
    QStringList ret;
    KnobIPtr knob = getRenderCloneKnobInternal();
    if (!knob) {
        PythonSetNullError();
        return ret;
    }
    KnobHolderPtr holder = knob->getHolder();
    if (!holder) {
        return ret;
    }
    AppInstancePtr app = holder->getApp();
    if (!app) {
        return ret;
    }
    const std::vector<std::string>& projectViews = app->getProject()->getProjectViewNames();
    std::list<ViewIdx> views = knob->getViewsList();
    for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
        if (*it < 0 && *it >= (int)projectViews.size()) {
            continue;
        }
        ret.push_back(QString::fromUtf8(projectViews[*it].c_str()));
    }
    return ret;

}

bool
AnimatedParam::getIsAnimated(int dimension, const QString& view) const
{
    KnobIPtr knob = getRenderCloneKnobInternal();
    if (!knob) {
        PythonSetNullError();
        return false;
    }
    if (dimension < 0 || dimension >= knob->getNDimensions()) {
        PythonSetInvalidDimensionError(dimension);
        return false;
    }
    ViewIdx thisViewSpec;
    if (!getViewIdxFromViewName(view, &thisViewSpec)) {
        PythonSetInvalidViewName(view);
        return false;
    }

    return knob->isAnimated( DimIdx(dimension), thisViewSpec );
}

int
AnimatedParam::getNumKeys(int dimension, const QString& view) const
{
    KnobIPtr knob = getRenderCloneKnobInternal();
    if (!knob) {
        PythonSetNullError();
        return 0;
    }
    if (dimension < 0 || dimension >= knob->getNDimensions()) {
        PythonSetInvalidDimensionError(dimension);
        return 0;
    }
    ViewIdx thisViewSpec;
    if (!getViewIdxFromViewName(view, &thisViewSpec)) {
        PythonSetInvalidViewName(view);
        return 0;
    }


    return knob->getKeyFramesCount(thisViewSpec, DimIdx(dimension));
}

int
AnimatedParam::getKeyIndex(double time,
                           int dimension, const QString& view) const
{
    KnobIPtr knob = getRenderCloneKnobInternal();
    if (!knob) {
        PythonSetNullError();
        return -1;
    }
    if (dimension < 0 || dimension >= knob->getNDimensions()) {
        PythonSetInvalidDimensionError(dimension);
        return -1;
    }
    ViewIdx thisViewSpec;
    if (!getViewIdxFromViewName(view, &thisViewSpec)) {
        PythonSetInvalidViewName(view);
        return -1;
    }

    return knob->getKeyFrameIndex(thisViewSpec, DimIdx(dimension), TimeValue(time));
}

bool
AnimatedParam::getKeyTime(int index,
                          int dimension,
                          double* time, const QString& view) const
{
    KnobIPtr knob = getRenderCloneKnobInternal();
    if (!knob) {
        PythonSetNullError();
        return false;
    }
    if (dimension < 0 || dimension >= knob->getNDimensions()) {
        PythonSetInvalidDimensionError(dimension);
        return false;
    }
    ViewIdx thisViewSpec;
    if (!getViewIdxFromViewName(view, &thisViewSpec)) {
        PythonSetInvalidViewName(view);
        return false;
    }

    TimeValue tmp;
    bool ok = knob->getKeyFrameTime(thisViewSpec, index, DimIdx(dimension), &tmp);
    if (ok) {
        *time = tmp;
    }
    return ok;

}

void
AnimatedParam::deleteValueAtTime(double time,
                                 int dimension, const QString& view)
{
    KnobIPtr knob = getInternalKnob();
    if (!knob) {
        PythonSetNullError();
        return;
    }

    ViewSetSpec thisViewSpec;
    if (!getViewSetSpecFromViewName(view, &thisViewSpec)) {
        PythonSetInvalidViewName(view);
        return;
    }
    if (dimension != kPyParamDimSpecAll && (dimension < 0 || dimension >= knob->getNDimensions())) {
        PythonSetInvalidDimensionError(dimension);
        return;
    }
    DimSpec dim = getDimSpecFromDimensionIndex(dimension);

    knob->deleteValueAtTime(TimeValue(time), thisViewSpec, dim, eValueChangedReasonUserEdited);

}

void
AnimatedParam::removeAnimation(int dimension, const QString& view)
{
    KnobIPtr knob = getInternalKnob();
    if (!knob) {
        PythonSetNullError();
        return;
    }

    ViewSetSpec thisViewSpec;
    if (!getViewSetSpecFromViewName(view, &thisViewSpec)) {
        PythonSetInvalidViewName(view);
        return;
    }
    if (dimension != kPyParamDimSpecAll && (dimension < 0 || dimension >= knob->getNDimensions())) {
        PythonSetInvalidDimensionError(dimension);
        return;
    }

    DimSpec dim = getDimSpecFromDimensionIndex(dimension);

    knob->removeAnimation(thisViewSpec, dim, eValueChangedReasonUserEdited);

}

double
AnimatedParam::getDerivativeAtTime(double time,
                                   int dimension, const QString& view) const
{
    KnobIPtr knob = getRenderCloneKnobInternal();
    if (!knob) {
        PythonSetNullError();
        return 0.;
    }
    if (dimension < 0 || dimension >= knob->getNDimensions()) {
        PythonSetInvalidDimensionError(dimension);
        return 0.;
    }
    ViewIdx thisViewSpec;
    if (!getViewIdxFromViewName(view, &thisViewSpec)) {
        PythonSetInvalidViewName(view);
        return 0.;
    }

    return knob->getDerivativeAtTime(TimeValue(time), thisViewSpec, DimIdx(dimension));
}

double
AnimatedParam::getIntegrateFromTimeToTime(double time1,
                                          double time2,
                                          int dimension, const QString& view) const
{
    KnobIPtr knob = getRenderCloneKnobInternal();
    if (!knob) {
        PythonSetNullError();
        return false;
    }
    if (dimension < 0 || dimension >= knob->getNDimensions()) {
        PythonSetInvalidDimensionError(dimension);
        return false;
    }
    ViewIdx thisViewSpec;
    if (!getViewIdxFromViewName(view, &thisViewSpec)) {
        PythonSetInvalidViewName(view);
        return false;
    }

    return knob->getIntegrateFromTimeToTime(TimeValue(time1), TimeValue(time2), thisViewSpec, DimIdx(dimension));
}

double
AnimatedParam::getCurrentTime() const
{
    KnobIPtr knob = getRenderCloneKnobInternal();
    if (!knob) {
        PythonSetNullError();
        return 0;
    }
    return knob->getCurrentRenderTime();
}

bool
AnimatedParam::setInterpolationAtTime(double time,
                                      KeyframeTypeEnum interpolation,
                                      int dimension, const QString& view)
{
    KnobIPtr knob = getInternalKnob();
    if (!knob) {
        PythonSetNullError();
        return false;
    }

    ViewSetSpec thisViewSpec;
    if (!getViewSetSpecFromViewName(view, &thisViewSpec)) {
        PythonSetInvalidViewName(view);
        return false;
    }
    if (dimension != kPyParamDimSpecAll && (dimension < 0 || dimension >= knob->getNDimensions())) {
        PythonSetInvalidDimensionError(dimension);
        return false;
    }

    DimSpec dim = getDimSpecFromDimensionIndex(dimension);
    knob->setInterpolationAtTime(thisViewSpec, dim, TimeValue(time), interpolation);
    return true;
}

void
Param::_addAsDependencyOf(Param* param, int fromExprDimension, int thisDimension, const QString& fromExprView, const QString& thisView)
{
    //from expr is in the dimension of expressionKnob
    //thisDimension is in the dimesnion of getValueCallerKnob

    KnobIPtr expressionKnob = param->_knob.lock();
    KnobIPtr getValueCallerKnob = _knob.lock();

    if (!expressionKnob || !getValueCallerKnob) {
        PythonSetNullError();
        return;
    }

    if (getValueCallerKnob == expressionKnob) {
        PyErr_SetString(PyExc_ValueError, tr("Cannot add the parameter as a dependency of itself").toStdString().c_str());
        return;
    }

    if (fromExprDimension < 0 || fromExprDimension >= expressionKnob->getNDimensions()) {
        PythonSetInvalidDimensionError(fromExprDimension);
        return;
    }

    if (thisDimension != -1 && (thisDimension < 0 || thisDimension >= getValueCallerKnob->getNDimensions())) {
        PythonSetInvalidDimensionError(thisDimension);
        return;
    }

    ViewIdx thisViewSpec, fromExprViewSpec;

    if (!getViewIdxFromViewName(thisView, &thisViewSpec)) {
        PythonSetInvalidViewName(thisView);
        return;
    }

    if (!getViewIdxFromViewName(fromExprView, &fromExprViewSpec)) {
        PythonSetInvalidViewName(fromExprView);
        return;
    }

    if (thisDimension == kPyParamDimSpecAll) {
        for (int i = 0; i < getValueCallerKnob->getNDimensions(); ++i) {
            getValueCallerKnob->addListener(DimIdx(fromExprDimension), DimIdx(i), ViewIdx(fromExprViewSpec.value()), ViewIdx(thisViewSpec.value()), expressionKnob, eExpressionLanguagePython);
        }
    } else {
        getValueCallerKnob->addListener(DimIdx(fromExprDimension), DimIdx(thisDimension), ViewIdx(fromExprViewSpec.value()), ViewIdx(thisViewSpec.value()), expressionKnob, eExpressionLanguagePython);
    }
}

bool
AnimatedParam::setExpression(const QString& expr,
                             bool hasRetVariable,
                             int dimension,
                             const QString& view)
{
    KnobIPtr knob = getInternalKnob();
    if (!knob) {
        PythonSetNullError();
        return false;
    }

    ViewSetSpec thisViewSpec;
    if (!getViewSetSpecFromViewName(view, &thisViewSpec)) {
        PythonSetInvalidViewName(view);
        return false;
    }
    if (dimension != kPyParamDimSpecAll && (dimension < 0 || dimension >= knob->getNDimensions())) {
        PythonSetInvalidDimensionError(dimension);
        return false;
    }

    DimSpec dim = getDimSpecFromDimensionIndex(dimension);

    try {
        knob->setExpression(dim, thisViewSpec, expr.toStdString(), eExpressionLanguagePython, hasRetVariable, true);
    } catch (const std::exception& e) {
        PyErr_SetString(PyExc_ValueError, e.what());

        return false;
    }

    return true;
}

QString
AnimatedParam::getExpression(int dimension,
                             bool* hasRetVariable,
                             const QString& view) const
{
    KnobIPtr knob = getRenderCloneKnobInternal();
    if (!knob) {
        PythonSetNullError();
        return QString();
    }
    if (dimension < 0 || dimension >= knob->getNDimensions()) {
        PythonSetInvalidDimensionError(dimension);
        return QString();
    }
    ViewIdx thisViewSpec;
    if (!getViewIdxFromViewName(view, &thisViewSpec)) {
        PythonSetInvalidViewName(view);
        return QString();
    }

    QString ret = QString::fromUtf8( knob->getExpression(DimIdx(dimension), thisViewSpec).c_str() );

    *hasRetVariable = knob->isExpressionUsingRetVariable(thisViewSpec, DimIdx(dimension));


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
IntParam::get(const QString& view) const
{
    KnobIntPtr knob = getRenderCloneKnob<KnobInt>();
    if (!knob) {
        PythonSetNullError();
        return 0;
    }
    ViewIdx thisViewSpec;
    if (!getViewIdxFromViewName(view, &thisViewSpec)) {
        PythonSetInvalidViewName(view);
        return 0;
    }
    return knob->getValue(DimIdx(0), thisViewSpec);

}

Int2DTuple
Int2DParam::get(const QString& view) const
{
    Int2DTuple ret = {0, 0};
    KnobIntPtr knob = getRenderCloneKnob<KnobInt>();
    if (!knob) {
        PythonSetNullError();
        return ret;
    }
    ViewIdx thisViewSpec;
    if (!getViewIdxFromViewName(view, &thisViewSpec)) {
        PythonSetInvalidViewName(view);

        return ret;
    }

    ret.x = knob->getValue(DimIdx(0), thisViewSpec);
    ret.y = knob->getValue(DimIdx(1), thisViewSpec);

    return ret;
}

Int3DTuple
Int3DParam::get(const QString& view) const
{
    Int3DTuple ret;
    KnobIntPtr knob = getRenderCloneKnob<KnobInt>();
    if (!knob) {
        PythonSetNullError();
        return ret;
    }
    ViewIdx thisViewSpec;
    if (!getViewIdxFromViewName(view, &thisViewSpec)) {
        PythonSetInvalidViewName(view);
        return ret;
    }
    ret.x = knob->getValue(DimIdx(0), thisViewSpec);
    ret.y = knob->getValue(DimIdx(1), thisViewSpec);
    ret.z = knob->getValue(DimIdx(2), thisViewSpec);


    return ret;
}

int
IntParam::get(double frame, const QString& view) const
{
    KnobIntPtr knob = getRenderCloneKnob<KnobInt>();
    if (!knob) {
        PythonSetNullError();
        return 0;
    }
    ViewIdx thisViewSpec;
    if (!getViewIdxFromViewName(view, &thisViewSpec)) {
        PythonSetInvalidViewName(view);
        return 0;
    }
    return knob->getValueAtTime(TimeValue(frame), DimIdx(0), thisViewSpec);

}

Int2DTuple
Int2DParam::get(double frame, const QString& view) const
{
    Int2DTuple ret = {0, 0};
    KnobIntPtr knob = getRenderCloneKnob<KnobInt>();
    if (!knob) {
        PythonSetNullError();
        return ret;
    }
    ViewIdx thisViewSpec;
    if (!getViewIdxFromViewName(view, &thisViewSpec)) {
        PythonSetInvalidViewName(view);

        return ret;
    }

    ret.x = knob->getValueAtTime(TimeValue(frame), DimIdx(0), thisViewSpec);
    ret.y = knob->getValueAtTime(TimeValue(frame), DimIdx(1), thisViewSpec);

    return ret;
}

Int3DTuple
Int3DParam::get(double frame, const QString& view) const
{
    Int3DTuple ret = {0, 0, 0};
    KnobIntPtr knob = getRenderCloneKnob<KnobInt>();
    if (!knob) {
        PythonSetNullError();
        return ret;
    }
    ViewIdx thisViewSpec;
    if (!getViewIdxFromViewName(view, &thisViewSpec)) {
        PythonSetInvalidViewName(view);
        return ret;
    }

    ret.x = knob->getValueAtTime(TimeValue(frame), DimIdx(0), thisViewSpec);
    ret.y = knob->getValueAtTime(TimeValue(frame), DimIdx(1), thisViewSpec);
    ret.z = knob->getValueAtTime(TimeValue(frame), DimIdx(2), thisViewSpec);

    return ret;
}

void
IntParam::set(int x, const QString& view)
{
    KnobIntPtr knob = _intKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return;
    }
    ViewSetSpec thisViewSpec;
    if (!getViewSetSpecFromViewName(view, &thisViewSpec)) {
        PythonSetInvalidViewName(view);
        return;
    }

    knob->setValue(x, thisViewSpec, DimIdx(0));

}

void
Int2DParam::set(int x,
                int y, const QString& view)
{
    KnobIntPtr knob = _intKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return;
    }
    ViewSetSpec thisViewSpec;
    if (!getViewSetSpecFromViewName(view, &thisViewSpec)) {
        PythonSetInvalidViewName(view);
        return;
    }
    std::vector<int> values(2);
    values[0] = x;
    values[1] = y;
    knob->setValueAcrossDimensions(values, DimIdx(0), thisViewSpec);

}

void
Int3DParam::set(int x,
                int y,
                int z, const QString& view)
{
    KnobIntPtr knob = _intKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return;
    }
    ViewSetSpec thisViewSpec;
    if (!getViewSetSpecFromViewName(view, &thisViewSpec)) {
        PythonSetInvalidViewName(view);

        return;
    }

    std::vector<int> values(3);
    values[0] = x;
    values[1] = y;
    values[2] = z;
    knob->setValueAcrossDimensions(values, DimIdx(0), thisViewSpec);
}

void
IntParam::set(int x,
              double frame, const QString& view)
{
    KnobIntPtr knob = _intKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return;
    }
    ViewSetSpec thisViewSpec;
    if (!getViewSetSpecFromViewName(view, &thisViewSpec)) {
        PythonSetInvalidViewName(view);
        return;
    }
    knob->setValueAtTime(TimeValue(frame), x, thisViewSpec, DimIdx(0));

}

void
Int2DParam::set(int x,
                int y,
                double frame, const QString& view)
{
    KnobIntPtr knob = _intKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return;
    }
    ViewSetSpec thisViewSpec;
    if (!getViewSetSpecFromViewName(view, &thisViewSpec)) {
        PythonSetInvalidViewName(view);
        return;
    }
    std::vector<int> values(2);
    values[0] = x;
    values[1] = y;
    knob->setValueAtTimeAcrossDimensions(TimeValue(frame), values, DimIdx(0), thisViewSpec);

}

void
Int3DParam::set(int x,
                int y,
                int z,
                double frame, const QString& view)
{
    KnobIntPtr knob = _intKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return;
    }
    ViewSetSpec thisViewSpec;
    if (!getViewSetSpecFromViewName(view, &thisViewSpec)) {
        PythonSetInvalidViewName(view);
        return;
    }
    std::vector<int> values(3);
    values[0] = x;
    values[1] = y;
    values[2] = z;
    knob->setValueAtTimeAcrossDimensions(TimeValue(frame), values, DimIdx(0), thisViewSpec);

}

int
IntParam::getValue(int dimension, const QString& view) const
{
    KnobIntPtr knob = getRenderCloneKnob<KnobInt>();
    if (!knob) {
        PythonSetNullError();
        return 0;
    }
    if (dimension < 0 || dimension >= knob->getNDimensions()) {
        PythonSetInvalidDimensionError(dimension);
        return 0;
    }
    ViewIdx thisViewSpec;
    if (!getViewIdxFromViewName(view, &thisViewSpec)) {
        PythonSetInvalidViewName(view);
        return 0;
    }
    return knob->getValue(DimIdx(dimension), thisViewSpec);

}

void
IntParam::setValue(int value,
                   int dimension, const QString& view)
{
    KnobIntPtr knob = _intKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return;
    }
    if (dimension != kPyParamDimSpecAll && (dimension < 0 || dimension >= knob->getNDimensions())) {
        PythonSetInvalidDimensionError(dimension);
        return;
    }
    ViewSetSpec thisViewSpec;
    if (!getViewSetSpecFromViewName(view, &thisViewSpec)) {
        PythonSetInvalidViewName(view);
        return;
    }
    DimSpec dim = getDimSpecFromDimensionIndex(dimension);
    knob->setValue(value, thisViewSpec, dim);

}

int
IntParam::getValueAtTime(double time,
                         int dimension, const QString& view) const
{
    KnobIntPtr knob = getRenderCloneKnob<KnobInt>();
    if (!knob) {
        PythonSetNullError();
        return 0;
    }
    if (dimension < 0 || dimension >= knob->getNDimensions()) {
        PythonSetInvalidDimensionError(dimension);
        return 0;
    }
    ViewIdx thisViewSpec;
    if (!getViewIdxFromViewName(view, &thisViewSpec)) {
        PythonSetInvalidViewName(view);
        return 0;
    }
    return knob->getValueAtTime(TimeValue(time), DimIdx(dimension), thisViewSpec);

}

void
IntParam::setValueAtTime(int value,
                         double time,
                         int dimension, const QString& view)
{
    KnobIntPtr knob = _intKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return;
    }
    if (dimension != kPyParamDimSpecAll && (dimension < 0 || dimension >= knob->getNDimensions())) {
        PythonSetInvalidDimensionError(dimension);
        return;
    }
    ViewSetSpec thisViewSpec;
    if (!getViewSetSpecFromViewName(view, &thisViewSpec)) {
        PythonSetInvalidViewName(view);
        return;
    }
    DimSpec dim = getDimSpecFromDimensionIndex(dimension);
    knob->setValueAtTime(TimeValue(time), value, thisViewSpec, dim);
}

void
IntParam::setDefaultValue(int value,
                          int dimension)
{
    KnobIntPtr knob = _intKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return;
    }
    if (dimension != kPyParamDimSpecAll && (dimension < 0 || dimension >= knob->getNDimensions())) {
        PythonSetInvalidDimensionError(dimension);
        return;
    }
    DimSpec dim = getDimSpecFromDimensionIndex(dimension);
    knob->setDefaultValueWithoutApplying(value, dim);
}

int
IntParam::getDefaultValue(int dimension) const
{
    KnobIntPtr knob = _intKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return 0;
    }
    if (dimension < 0 || dimension >= knob->getNDimensions()) {
        PythonSetInvalidDimensionError(dimension);
        return 0;
    }

    return knob->getDefaultValue(DimIdx(dimension));
}

void
IntParam::restoreDefaultValue(int dimension, const QString& view)
{
    KnobIntPtr knob = _intKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return;
    }
    if (dimension != kPyParamDimSpecAll && (dimension < 0 || dimension >= knob->getNDimensions())) {
        PythonSetInvalidDimensionError(dimension);
        return;
    }
    ViewSetSpec thisViewSpec;
    if (!getViewSetSpecFromViewName(view, &thisViewSpec)) {
        PythonSetInvalidViewName(view);
        return;
    }
    DimSpec dim = getDimSpecFromDimensionIndex(dimension);

    knob->resetToDefaultValue(dim, thisViewSpec);
}

void
IntParam::setMinimum(int minimum,
                     int dimension)
{
    KnobIntPtr knob = _intKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return;
    }
    if (knob->getKnobDeclarationType() != KnobI::eKnobDeclarationTypeUser) {
        PythonSetNonUserKnobError();
        return;
    }
    if (dimension < 0 || dimension >= knob->getNDimensions()) {
        PythonSetInvalidDimensionError(dimension);
        return;
    }

    knob->setRange(minimum, knob->getMaximum(DimIdx(dimension)), DimIdx(dimension));
}

int
IntParam::getMinimum(int dimension) const
{
    KnobIntPtr knob = _intKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return 0;
    }
    if (dimension < 0 || dimension >= knob->getNDimensions()) {
        PythonSetInvalidDimensionError(dimension);
        return 0;
    }
    return knob->getMinimum(DimIdx(dimension));
}

void
IntParam::setMaximum(int maximum,
                     int dimension)
{
    KnobIntPtr knob = _intKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return;
    }
    if (knob->getKnobDeclarationType() != KnobI::eKnobDeclarationTypeUser) {
        PythonSetNonUserKnobError();
        return;
    }
    if (dimension < 0 || dimension >= knob->getNDimensions()) {
        PythonSetInvalidDimensionError(dimension);
        return;
    }
    knob->setRange(knob->getMinimum(DimIdx(dimension)),maximum, DimIdx(dimension));
}

int
IntParam::getMaximum(int dimension) const
{
    KnobIntPtr knob = _intKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return 0;
    }
    if (dimension < 0 || dimension >= knob->getNDimensions()) {
        PythonSetInvalidDimensionError(dimension);
        return 0;
    }

    return knob->getMaximum(DimIdx(dimension));
}

void
IntParam::setDisplayMinimum(int minimum,
                            int dimension)
{
    KnobIntPtr knob = _intKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return;
    }
    if (knob->getKnobDeclarationType() != KnobI::eKnobDeclarationTypeUser) {
        PythonSetNonUserKnobError();
        return;
    }
    if (dimension < 0 || dimension >= knob->getNDimensions()) {
        PythonSetInvalidDimensionError(dimension);
        return;
    }
    return knob->setDisplayRange(minimum, knob->getDisplayMaximum(DimIdx(dimension)), DimIdx(dimension));
}

int
IntParam::getDisplayMinimum(int dimension) const
{
    KnobIntPtr knob = _intKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return 0;
    }
    if (dimension < 0 || dimension >= knob->getNDimensions()) {
        PythonSetInvalidDimensionError(dimension);
        return 0;
    }

    return knob->getDisplayMinimum(DimIdx(dimension));

}

void
IntParam::setDisplayMaximum(int maximum,
                            int dimension)
{
    KnobIntPtr knob = _intKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return;
    }
    if (knob->getKnobDeclarationType() != KnobI::eKnobDeclarationTypeUser) {
        PythonSetNonUserKnobError();
        return;
    }
    if (dimension < 0 || dimension >= knob->getNDimensions()) {
        PythonSetInvalidDimensionError(dimension);
        return;
    }
    return knob->setDisplayRange(knob->getDisplayMinimum(DimIdx(dimension)), maximum, DimIdx(dimension));
}

int
IntParam::getDisplayMaximum(int dimension) const
{
    KnobIntPtr knob = _intKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return 0;
    }
    if (dimension < 0 || dimension >= knob->getNDimensions()) {
        PythonSetInvalidDimensionError(dimension);
        return 0;
    }

    return knob->getDisplayMaximum(DimIdx(dimension));

}

int
IntParam::addAsDependencyOf(Param* param, int fromExprDimension, int thisDimension, const QString& fromExprView, const QString& thisView)
{
    _addAsDependencyOf(param, fromExprDimension, thisDimension, fromExprView, thisView);

    return getValue();

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
DoubleParam::get(const QString& view) const
{
    KnobDoublePtr knob = getRenderCloneKnob<KnobDouble>();
    if (!knob) {
        PythonSetNullError();
        return 0;
    }
    ViewIdx thisViewSpec;
    if (!getViewIdxFromViewName(view, &thisViewSpec)) {
        PythonSetInvalidViewName(view);
        return 0;
    }
    return knob->getValue(DimIdx(0), thisViewSpec);
}

Double2DTuple
Double2DParam::get(const QString& view) const
{
    Double2DTuple ret;
    KnobDoublePtr knob = getRenderCloneKnob<KnobDouble>();
    if (!knob) {
        PythonSetNullError();
        return ret;
    }
    ViewIdx thisViewSpec;
    if (!getViewIdxFromViewName(view, &thisViewSpec)) {
        PythonSetInvalidViewName(view);

        return ret;
    }

    ret.x = knob->getValue(DimIdx(0), thisViewSpec);
    ret.y = knob->getValue(DimIdx(1), thisViewSpec);

    return ret;
    
}

Double3DTuple
Double3DParam::get(const QString& view) const
{
    Double3DTuple ret;
    KnobDoublePtr knob = getRenderCloneKnob<KnobDouble>();
    if (!knob) {
        PythonSetNullError();
        return ret;
    }
    ViewIdx thisViewSpec;
    if (!getViewIdxFromViewName(view, &thisViewSpec)) {
        PythonSetInvalidViewName(view);
        return ret;
    }

    ret.x = knob->getValue(DimIdx(0), thisViewSpec);
    ret.y = knob->getValue(DimIdx(1), thisViewSpec);
    ret.z = knob->getValue(DimIdx(2), thisViewSpec);

    return ret;}

double
DoubleParam::get(double frame, const QString& view) const
{
    KnobDoublePtr knob = getRenderCloneKnob<KnobDouble>();
    if (!knob) {
        PythonSetNullError();
        return 0;
    }
    ViewIdx thisViewSpec;
    if (!getViewIdxFromViewName(view, &thisViewSpec)) {
        PythonSetInvalidViewName(view);
        return 0;
    }
    return knob->getValueAtTime(TimeValue(frame), DimIdx(0), thisViewSpec);
}

Double2DTuple
Double2DParam::get(double frame, const QString& view) const
{
    Double2DTuple ret = {0., 0.};
    KnobDoublePtr knob = getRenderCloneKnob<KnobDouble>();
    if (!knob) {
        PythonSetNullError();
        return ret;
    }
    ViewIdx thisViewSpec;
    if (!getViewIdxFromViewName(view, &thisViewSpec)) {
        PythonSetInvalidViewName(view);

        return ret;
    }

    ret.x = knob->getValueAtTime(TimeValue(frame), DimIdx(0), thisViewSpec);
    ret.y = knob->getValueAtTime(TimeValue(frame), DimIdx(1), thisViewSpec);

    return ret;
}

Double3DTuple
Double3DParam::get(double frame, const QString& view) const
{
    Double3DTuple ret;
    KnobDoublePtr knob = getRenderCloneKnob<KnobDouble>();
    if (!knob) {
        PythonSetNullError();
        return ret;
    }
    ViewIdx thisViewSpec;
    if (!getViewIdxFromViewName(view, &thisViewSpec)) {
        PythonSetInvalidViewName(view);
        return ret;
    }

    ret.x = knob->getValueAtTime(TimeValue(frame), DimIdx(0), thisViewSpec);
    ret.y = knob->getValueAtTime(TimeValue(frame), DimIdx(1), thisViewSpec);
    ret.z = knob->getValueAtTime(TimeValue(frame), DimIdx(2), thisViewSpec);

    return ret;
}

void
DoubleParam::set(double x, const QString& view)
{
    KnobDoublePtr knob = _doubleKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return;
    }
    ViewSetSpec thisViewSpec;
    if (!getViewSetSpecFromViewName(view, &thisViewSpec)) {
        PythonSetInvalidViewName(view);
        return;
    }

    knob->setValue(x, thisViewSpec, DimIdx(0));
}

void
Double2DParam::set(double x,
                   double y, const QString& view)
{
    KnobDoublePtr knob = _doubleKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return;
    }
    ViewSetSpec thisViewSpec;
    if (!getViewSetSpecFromViewName(view, &thisViewSpec)) {
        PythonSetInvalidViewName(view);
        return;
    }
    std::vector<double> values(2);
    values[0] = x;
    values[1] = y;
    knob->setValueAcrossDimensions(values, DimIdx(0), thisViewSpec);
}

void
Double3DParam::set(double x,
                   double y,
                   double z, const QString& view)
{
    KnobDoublePtr knob = _doubleKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return;
    }
    ViewSetSpec thisViewSpec;
    if (!getViewSetSpecFromViewName(view, &thisViewSpec)) {
        PythonSetInvalidViewName(view);
        return;
    }

    std::vector<double> values(3);
    values[0] = x;
    values[1] = y;
    values[2] = z;
    knob->setValueAcrossDimensions(values, DimIdx(0), thisViewSpec);
}

void
DoubleParam::set(double x,
                 double frame, const QString& view)
{
    KnobDoublePtr knob = _doubleKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return;
    }
    ViewSetSpec thisViewSpec;
    if (!getViewSetSpecFromViewName(view, &thisViewSpec)) {
        PythonSetInvalidViewName(view);
        return;
    }
    knob->setValueAtTime(TimeValue(frame), x, thisViewSpec, DimIdx(0));
}

void
Double2DParam::set(double x,
                   double y,
                   double frame, const QString& view)
{
    KnobDoublePtr knob = _doubleKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return;
    }
    ViewSetSpec thisViewSpec;
    if (!getViewSetSpecFromViewName(view, &thisViewSpec)) {
        PythonSetInvalidViewName(view);
        return;
    }
    std::vector<double> values(2);
    values[0] = x;
    values[1] = y;
    knob->setValueAtTimeAcrossDimensions(TimeValue(frame), values, DimIdx(0), thisViewSpec);
}


void
Double2DParam::setUsePointInteract(bool use)
{
    KnobDoublePtr knob = _doubleKnob.lock();
    if (!knob) {
        return;
    }
    // Natron 2 version:
    return knob->setHasHostOverlayHandle(use);

    // Natron 3 version:
    /*
    KnobHolderPtr holder = knob->getHolder();
    if (!holder) {
        return;
    }
    NodePtr node;
    EffectInstancePtr isEffect = toEffectInstance(holder);

    if (!isEffect) {
        return;
    }
    std::list<OverlayInteractBasePtr> overlays;
    isEffect->getOverlays(eOverlayViewportTypeViewer, &overlays);
    std::string name = knob->getName();
    // check if PointOverlayInteract already exists for this knob
    std::list<OverlayInteractBasePtr>::iterator it;
    PointOverlayInteractPtr found;
    for(it = overlays.begin(); it != overlays.end(); ++it) {
        PointOverlayInteractPtr found = boost::dynamic_pointer_cast<PointOverlayInteract>(*it);
        if (found && found->getName() == name) {
            // we found a PointOverlayInteract associated to this param name
            break;
        }
    }
    if (use) {
        // create it if it doesn't exist, else do nothing
        if ( it == overlays.end() ) {
            assert(!found);
            // same code as in OfxDouble2DInstance::OfxDouble2DInstance()
            boost::shared_ptr<PointOverlayInteract> interact(new PointOverlayInteract());
            std::map<std::string,std::string> knobs;
            knobs["position"] = name;
            isEffect->registerOverlay(eOverlayViewportTypeViewer, interact, knobs);
        }
    } else {
        // remove it if it exists, else do nothing
        if ( it != overlays.end() ) {
            assert(found);
            isEffect->removeOverlay(eOverlayViewportTypeViewer, found);
        }
    }
    */
}


void
Double2DParam::setCanAutoFoldDimensions(bool can)
{
    boost::shared_ptr<KnobDouble> knob = _doubleKnob.lock();
    if (!knob) {
        return;
    }
    return knob->setCanAutoFoldDimensions(can);
}


void
Double3DParam::set(double x,
                   double y,
                   double z,
                   double frame, const QString& view)
{
    KnobDoublePtr knob = _doubleKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return;
    }
    ViewSetSpec thisViewSpec;
    if (!getViewSetSpecFromViewName(view, &thisViewSpec)) {
        PythonSetInvalidViewName(view);
        return;
    }
    std::vector<double> values(3);
    values[0] = x;
    values[1] = y;
    values[2] = z;
    knob->setValueAtTimeAcrossDimensions(TimeValue(frame), values, DimIdx(0), thisViewSpec);
}

double
DoubleParam::getValue(int dimension, const QString& view) const
{
    KnobDoublePtr knob = getRenderCloneKnob<KnobDouble>();
    if (!knob) {
        PythonSetNullError();
        return 0;
    }
    if (dimension < 0 || dimension >= knob->getNDimensions()) {
        PythonSetInvalidDimensionError(dimension);
        return 0;
    }
    ViewIdx thisViewSpec;
    if (!getViewIdxFromViewName(view, &thisViewSpec)) {
        PythonSetInvalidViewName(view);
        return 0;
    }
    return knob->getValue(DimIdx(dimension), thisViewSpec);
}

void
DoubleParam::setValue(double value,
                      int dimension, const QString& view)
{
    KnobDoublePtr knob = _doubleKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return;
    }
    if (dimension != kPyParamDimSpecAll && (dimension < 0 || dimension >= knob->getNDimensions())) {
        PythonSetInvalidDimensionError(dimension);
        return;
    }
    ViewSetSpec thisViewSpec;
    if (!getViewSetSpecFromViewName(view, &thisViewSpec)) {
        PythonSetInvalidViewName(view);
        return;
    }
    DimSpec dim = getDimSpecFromDimensionIndex(dimension);
    knob->setValue(value, thisViewSpec, dim);
}

double
DoubleParam::getValueAtTime(double time,
                            int dimension, const QString& view) const
{
    KnobDoublePtr knob = getRenderCloneKnob<KnobDouble>();
    if (!knob) {
        PythonSetNullError();
        return 0;
    }
    if (dimension < 0 || dimension >= knob->getNDimensions()) {
        PythonSetInvalidDimensionError(dimension);
        return 0;
    }
    ViewIdx thisViewSpec;
    if (!getViewIdxFromViewName(view, &thisViewSpec)) {
        PythonSetInvalidViewName(view);
        return 0;
    }
    return knob->getValueAtTime(TimeValue(time), DimIdx(dimension), thisViewSpec);
}

void
DoubleParam::setValueAtTime(double value,
                            double time,
                            int dimension, const QString& view)
{
    KnobDoublePtr knob = _doubleKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return;
    }
    if (dimension != kPyParamDimSpecAll && (dimension < 0 || dimension >= knob->getNDimensions())) {
        PythonSetInvalidDimensionError(dimension);
        return;
    }
    ViewSetSpec thisViewSpec;
    if (!getViewSetSpecFromViewName(view, &thisViewSpec)) {
        PythonSetInvalidViewName(view);
        return;
    }
    DimSpec dim = getDimSpecFromDimensionIndex(dimension);
    knob->setValueAtTime(TimeValue(time), value, thisViewSpec, dim);
}

void
DoubleParam::setDefaultValue(double value,
                             int dimension)
{
    KnobDoublePtr knob = _doubleKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return;
    }
    if (dimension != kPyParamDimSpecAll && (dimension < 0 || dimension >= knob->getNDimensions())) {
        PythonSetInvalidDimensionError(dimension);
        return;
    }
    DimSpec dim = getDimSpecFromDimensionIndex(dimension);
    knob->setDefaultValueWithoutApplying(value, dim);
}

double
DoubleParam::getDefaultValue(int dimension) const
{
    KnobDoublePtr knob = _doubleKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return 0;
    }
    if (dimension < 0 || dimension >= knob->getNDimensions()) {
        PythonSetInvalidDimensionError(dimension);
        return 0;
    }

    return knob->getDefaultValue(DimIdx(dimension));
}

void
DoubleParam::restoreDefaultValue(int dimension, const QString& view)
{
    KnobDoublePtr knob = _doubleKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return;
    }
    if (dimension != kPyParamDimSpecAll && (dimension < 0 || dimension >= knob->getNDimensions())) {
        PythonSetInvalidDimensionError(dimension);
        return;
    }
    ViewSetSpec thisViewSpec;
    if (!getViewSetSpecFromViewName(view, &thisViewSpec)) {
        PythonSetInvalidViewName(view);
        return;
    }
    DimSpec dim = getDimSpecFromDimensionIndex(dimension);

    knob->resetToDefaultValue(dim, thisViewSpec);
}

void
DoubleParam::setMinimum(double minimum,
                        int dimension)
{
    KnobDoublePtr knob = _doubleKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return;
    }
    if (knob->getKnobDeclarationType() != KnobI::eKnobDeclarationTypeUser) {
        PythonSetNonUserKnobError();
        return;
    }
    if (dimension < 0 || dimension >= knob->getNDimensions()) {
        PythonSetInvalidDimensionError(dimension);
        return;
    }

    knob->setRange(minimum, knob->getMaximum(DimIdx(dimension)), DimIdx(dimension));
}

double
DoubleParam::getMinimum(int dimension) const
{
    KnobDoublePtr knob = _doubleKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return 0;
    }
    if (dimension < 0 || dimension >= knob->getNDimensions()) {
        PythonSetInvalidDimensionError(dimension);
        return 0;
    }
    return knob->getMinimum(DimIdx(dimension));
}

void
DoubleParam::setMaximum(double maximum,
                        int dimension)
{
    KnobDoublePtr knob = _doubleKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return;
    }
    if (knob->getKnobDeclarationType() != KnobI::eKnobDeclarationTypeUser) {
        PythonSetNonUserKnobError();
        return;
    }
    if (dimension < 0 || dimension >= knob->getNDimensions()) {
        PythonSetInvalidDimensionError(dimension);
        return;
    }
    knob->setRange(knob->getMinimum(DimIdx(dimension)),maximum, DimIdx(dimension));
}

double
DoubleParam::getMaximum(int dimension) const
{
    KnobDoublePtr knob = _doubleKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return 0;
    }
    if (dimension < 0 || dimension >= knob->getNDimensions()) {
        PythonSetInvalidDimensionError(dimension);
        return 0;
    }

    return knob->getMaximum(DimIdx(dimension));
}

void
DoubleParam::setDisplayMinimum(double minimum,
                               int dimension)
{
    KnobDoublePtr knob = _doubleKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return;
    }
    if (knob->getKnobDeclarationType() != KnobI::eKnobDeclarationTypeUser) {
        PythonSetNonUserKnobError();
        return;
    }
    if (dimension < 0 || dimension >= knob->getNDimensions()) {
        PythonSetInvalidDimensionError(dimension);
        return;
    }
    return knob->setDisplayRange(minimum, knob->getDisplayMaximum(DimIdx(dimension)), DimIdx(dimension));

}

double
DoubleParam::getDisplayMinimum(int dimension) const
{
    KnobDoublePtr knob = _doubleKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return 0;
    }
    if (dimension < 0 || dimension >= knob->getNDimensions()) {
        PythonSetInvalidDimensionError(dimension);
        return 0;
    }

    return knob->getDisplayMinimum(DimIdx(dimension));
}

void
DoubleParam::setDisplayMaximum(double maximum,
                               int dimension)
{
    KnobDoublePtr knob = _doubleKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return;
    }
    if (knob->getKnobDeclarationType() != KnobI::eKnobDeclarationTypeUser) {
        PythonSetNonUserKnobError();
        return;
    }
    if (dimension < 0 || dimension >= knob->getNDimensions()) {
        PythonSetInvalidDimensionError(dimension);
        return;
    }
    return knob->setDisplayRange(knob->getDisplayMinimum(DimIdx(dimension)), maximum, DimIdx(dimension));

}

double
DoubleParam::getDisplayMaximum(int dimension) const
{
    KnobDoublePtr knob = _doubleKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return 0;
    }
    if (dimension < 0 || dimension >= knob->getNDimensions()) {
        PythonSetInvalidDimensionError(dimension);
        return 0;
    }

    return knob->getDisplayMaximum(DimIdx(dimension));
}

double
DoubleParam::addAsDependencyOf(Param* param, int fromExprDimension, int thisDimension, const QString& fromExprView, const QString& thisView)
{
    _addAsDependencyOf(param, fromExprDimension, thisDimension, fromExprView, thisView);

    return getValue();
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
ColorParam::get(const QString& view) const
{
    ColorTuple ret = {0., 0., 0., 0.};
    KnobColorPtr knob = getRenderCloneKnob<KnobColor>();
    if (!knob) {
        PythonSetNullError();
        return ret;
    }
    ViewIdx thisViewSpec;
    if (!getViewIdxFromViewName(view, &thisViewSpec)) {
        PythonSetInvalidViewName(view);
        return ret;
    }
    ret.r = knob->getValue(DimIdx(0), thisViewSpec);
    ret.g = knob->getValue(DimIdx(1), thisViewSpec);
    ret.b = knob->getValue(DimIdx(2), thisViewSpec);
    ret.a = knob->getNDimensions() == 4 ? knob->getValue(DimIdx(3)) : 1.;


    return ret;
}

ColorTuple
ColorParam::get(double frame, const QString& view) const
{
    ColorTuple ret = {0., 0., 0., 0.};
    KnobColorPtr knob = getRenderCloneKnob<KnobColor>();
    if (!knob) {
        PythonSetNullError();
        return ret;
    }
    ViewIdx thisViewSpec;
    if (!getViewIdxFromViewName(view, &thisViewSpec)) {
        PythonSetInvalidViewName(view);
        return ret;
    }
    ret.r = knob->getValueAtTime(TimeValue(frame), DimIdx(0), thisViewSpec);
    ret.g = knob->getValueAtTime(TimeValue(frame), DimIdx(1), thisViewSpec);
    ret.b = knob->getValueAtTime(TimeValue(frame), DimIdx(2), thisViewSpec);
    ret.a = knob->getNDimensions() == 4 ? knob->getValueAtTime(TimeValue(frame), DimIdx(3)) : 1.;


    return ret;
}

void
ColorParam::set(double r,
                double g,
                double b,
                double a, const QString& view)
{
    KnobColorPtr knob = _colorKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return;
    }
    ViewSetSpec thisViewSpec;
    if (!getViewSetSpecFromViewName(view, &thisViewSpec)) {
        PythonSetInvalidViewName(view);
        return;
    }

    std::vector<double> values(knob->getNDimensions());
    values[0] = r;
    values[1] = g;
    values[2] = b;
    if (knob->getNDimensions() == 4) {
        values[3] = a;
    }
    knob->setValueAcrossDimensions(values, DimIdx(0), thisViewSpec);
}

void
ColorParam::set(double r,
                double g,
                double b,
                double a,
                double frame, const QString& view)
{
    KnobColorPtr knob = _colorKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return;
    }
    ViewSetSpec thisViewSpec;
    if (!getViewSetSpecFromViewName(view, &thisViewSpec)) {
        PythonSetInvalidViewName(view);
        return;
    }

    std::vector<double> values(knob->getNDimensions());
    values[0] = r;
    values[1] = g;
    values[2] = b;
    if (knob->getNDimensions() == 4) {
        values[3] = a;
    }
    knob->setValueAtTimeAcrossDimensions(TimeValue(frame), values, DimIdx(0), thisViewSpec);
}

double
ColorParam::getValue(int dimension, const QString& view) const
{
    KnobColorPtr knob = getRenderCloneKnob<KnobColor>();
    if (!knob) {
        PythonSetNullError();
        return 0;
    }
    if (dimension < 0 || dimension >= knob->getNDimensions()) {
        PythonSetInvalidDimensionError(dimension);
        return 0;
    }
    ViewIdx thisViewSpec;
    if (!getViewIdxFromViewName(view, &thisViewSpec)) {
        PythonSetInvalidViewName(view);
        return 0;
    }
    return knob->getValue(DimIdx(dimension), thisViewSpec);
}

void
ColorParam::setValue(double value,
                     int dimension, const QString& view)
{
    KnobColorPtr knob = _colorKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return;
    }
    if (dimension != kPyParamDimSpecAll && (dimension < 0 || dimension >= knob->getNDimensions())) {
        PythonSetInvalidDimensionError(dimension);
        return;
    }
    ViewSetSpec thisViewSpec;
    if (!getViewSetSpecFromViewName(view, &thisViewSpec)) {
        PythonSetInvalidViewName(view);
        return;
    }
    DimSpec dim = getDimSpecFromDimensionIndex(dimension);
    knob->setValue(value, thisViewSpec, dim);
}

double
ColorParam::getValueAtTime(double time,
                           int dimension, const QString& view) const
{
    KnobColorPtr knob = getRenderCloneKnob<KnobColor>();
    if (!knob) {
        PythonSetNullError();
        return 0;
    }
    if (dimension < 0 || dimension >= knob->getNDimensions()) {
        PythonSetInvalidDimensionError(dimension);
        return 0;
    }
    ViewIdx thisViewSpec;
    if (!getViewIdxFromViewName(view, &thisViewSpec)) {
        PythonSetInvalidViewName(view);
        return 0;
    }
    return knob->getValueAtTime(TimeValue(time), DimIdx(dimension), thisViewSpec);
}

void
ColorParam::setValueAtTime(double value,
                           double time,
                           int dimension, const QString& view)
{
    KnobColorPtr knob = _colorKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return;
    }
    if (dimension != kPyParamDimSpecAll && (dimension < 0 || dimension >= knob->getNDimensions())) {
        PythonSetInvalidDimensionError(dimension);
        return;
    }
    ViewSetSpec thisViewSpec;
    if (!getViewSetSpecFromViewName(view, &thisViewSpec)) {
        PythonSetInvalidViewName(view);
        return;
    }
    DimSpec dim = getDimSpecFromDimensionIndex(dimension);
    knob->setValueAtTime(TimeValue(time), value, thisViewSpec, dim);
}

void
ColorParam::setDefaultValue(double value,
                            int dimension)
{
    KnobColorPtr knob = _colorKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return;
    }
    if (dimension != kPyParamDimSpecAll && (dimension < 0 || dimension >= knob->getNDimensions())) {
        PythonSetInvalidDimensionError(dimension);
        return;
    }
    DimSpec dim = getDimSpecFromDimensionIndex(dimension);
    knob->setDefaultValueWithoutApplying(value, dim);
}

double
ColorParam::getDefaultValue(int dimension) const
{
    KnobColorPtr knob = _colorKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return 0;
    }
    if (dimension < 0 || dimension >= knob->getNDimensions()) {
        PythonSetInvalidDimensionError(dimension);
        return 0;
    }

    return knob->getDefaultValue(DimIdx(dimension));
}

void
ColorParam::restoreDefaultValue(int dimension, const QString& view)
{
    KnobColorPtr knob = _colorKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return;
    }
    if (dimension != kPyParamDimSpecAll && (dimension < 0 || dimension >= knob->getNDimensions())) {
        PythonSetInvalidDimensionError(dimension);
        return;
    }
    ViewSetSpec thisViewSpec;
    if (!getViewSetSpecFromViewName(view, &thisViewSpec)) {
        PythonSetInvalidViewName(view);
        return;
    }
    DimSpec dim = getDimSpecFromDimensionIndex(dimension);

    knob->resetToDefaultValue(dim, thisViewSpec);
}

void
ColorParam::setMinimum(double minimum,
                       int dimension)
{
   KnobColorPtr knob = _colorKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return;
    }
    if (knob->getKnobDeclarationType() != KnobI::eKnobDeclarationTypeUser) {
        PythonSetNonUserKnobError();
        return;
    }
    if (dimension < 0 || dimension >= knob->getNDimensions()) {
        PythonSetInvalidDimensionError(dimension);
        return;
    }

    knob->setRange(minimum, knob->getMaximum(DimIdx(dimension)), DimIdx(dimension));

}

double
ColorParam::getMinimum(int dimension) const
{
    KnobColorPtr knob = _colorKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return 0;
    }
    if (dimension < 0 || dimension >= knob->getNDimensions()) {
        PythonSetInvalidDimensionError(dimension);
        return 0;
    }
    return knob->getMinimum(DimIdx(dimension));
}

void
ColorParam::setMaximum(double maximum,
                       int dimension)
{
    KnobColorPtr knob = _colorKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return;
    }
    if (knob->getKnobDeclarationType() != KnobI::eKnobDeclarationTypeUser) {
        PythonSetNonUserKnobError();
        return;
    }
    if (dimension < 0 || dimension >= knob->getNDimensions()) {
        PythonSetInvalidDimensionError(dimension);
        return;
    }
    knob->setRange(knob->getMinimum(DimIdx(dimension)),maximum, DimIdx(dimension));
}

double
ColorParam::getMaximum(int dimension) const
{
    KnobColorPtr knob = _colorKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return 0;
    }
    if (dimension < 0 || dimension >= knob->getNDimensions()) {
        PythonSetInvalidDimensionError(dimension);
        return 0;
    }

    return knob->getMaximum(DimIdx(dimension));

}

void
ColorParam::setDisplayMinimum(double minimum,
                              int dimension)
{
    KnobColorPtr knob = _colorKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return;
    }
    if (knob->getKnobDeclarationType() != KnobI::eKnobDeclarationTypeUser) {
        PythonSetNonUserKnobError();
        return;
    }
    if (dimension < 0 || dimension >= knob->getNDimensions()) {
        PythonSetInvalidDimensionError(dimension);
        return;
    }
    return knob->setDisplayRange(minimum, knob->getDisplayMaximum(DimIdx(dimension)), DimIdx(dimension));

}

double
ColorParam::getDisplayMinimum(int dimension) const
{
    KnobColorPtr knob = _colorKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return 0;
    }
    if (dimension < 0 || dimension >= knob->getNDimensions()) {
        PythonSetInvalidDimensionError(dimension);
        return 0;
    }

    return knob->getDisplayMinimum(DimIdx(dimension));

}

void
ColorParam::setDisplayMaximum(double maximum,
                              int dimension)
{
    KnobColorPtr knob = _colorKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return;
    }
    if (knob->getKnobDeclarationType() != KnobI::eKnobDeclarationTypeUser) {
        PythonSetNonUserKnobError();
        return;
    }
    if (dimension < 0 || dimension >= knob->getNDimensions()) {
        PythonSetInvalidDimensionError(dimension);
        return;
    }
    return knob->setDisplayRange(knob->getDisplayMinimum(DimIdx(dimension)), maximum, DimIdx(dimension));

}

double
ColorParam::getDisplayMaximum(int dimension) const
{
    KnobColorPtr knob = _colorKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return 0;
    }
    if (dimension < 0 || dimension >= knob->getNDimensions()) {
        PythonSetInvalidDimensionError(dimension);
        return 0;
    }

    return knob->getDisplayMaximum(DimIdx(dimension));

}

double
ColorParam::addAsDependencyOf(Param* param, int fromExprDimension, int thisDimension, const QString& fromExprView, const QString& thisView)
{
    _addAsDependencyOf(param, fromExprDimension, thisDimension, fromExprView, thisView);

    return getValue();
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
ChoiceParam::get(const QString& view) const
{
    KnobChoicePtr knob = getRenderCloneKnob<KnobChoice>();
    if (!knob) {
        PythonSetNullError();
        return 0;
    }
    ViewIdx thisViewSpec;
    if (!getViewIdxFromViewName(view, &thisViewSpec)) {
        PythonSetInvalidViewName(view);
        return 0;
    }
    return knob->getValue(DimIdx(0), thisViewSpec);
}

int
ChoiceParam::get(double frame, const QString& view) const
{
    KnobChoicePtr knob = getRenderCloneKnob<KnobChoice>();
    if (!knob) {
        PythonSetNullError();
        return 0;
    }
    ViewIdx thisViewSpec;
    if (!getViewIdxFromViewName(view, &thisViewSpec)) {
        PythonSetInvalidViewName(view);
        return 0;
    }
    return knob->getValueAtTime(TimeValue(frame), DimIdx(0), thisViewSpec);

}

void
ChoiceParam::set(int x, const QString& view)
{
    KnobChoicePtr knob = _choiceKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return;
    }
    ViewSetSpec thisViewSpec;
    if (!getViewSetSpecFromViewName(view, &thisViewSpec)) {
        PythonSetInvalidViewName(view);
        return;
    }

    knob->setValue(x, thisViewSpec, DimIdx(0));

}

void
ChoiceParam::set(int x,
                 double frame, const QString& view)
{
    KnobChoicePtr knob = _choiceKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return;
    }
    ViewSetSpec thisViewSpec;
    if (!getViewSetSpecFromViewName(view, &thisViewSpec)) {
        PythonSetInvalidViewName(view);
        return;
    }

    knob->setValueAtTime(TimeValue(frame), x, thisViewSpec, DimIdx(0));

}

void
ChoiceParam::set(const QString& label, const QString& view)
{
    KnobChoicePtr knob = _choiceKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return;
    }
    ViewSetSpec thisViewSpec;
    if (!getViewSetSpecFromViewName(view, &thisViewSpec)) {
        PythonSetInvalidViewName(view);
        return;
    }
    try {
        std::string choiceID = label.toStdString();
        bool isCreatingNode = knob->getHolder()->getApp()->isCreatingNode();
        if (isCreatingNode) {
            // Before Natron 2.2.3, all dynamic choice parameters for multiplane had a string parameter.
            // The string parameter had the same name as the choice parameter plus "Choice" appended.
            // If we found such a parameter, retrieve the string from it.
            EffectInstancePtr isEffect = toEffectInstance(knob->getHolder());
            if (isEffect) {
                PluginPtr plugin = isEffect->getNode()->getPyPlugPlugin();
                if (plugin) {
                    SERIALIZATION_NAMESPACE::ProjectBeingLoadedInfo projectInfos;
                    bool gotProjectInfos = isEffect->getApp()->getProject()->getProjectLoadedVersionInfo(&projectInfos);
                    int natronMajor = -1,natronMinor = -1,natronRev =-1;
                    if (gotProjectInfos) {
                        natronMajor = projectInfos.vMajor;
                        natronMinor = projectInfos.vMinor;
                        natronRev = projectInfos.vRev;
                    }

                    filterKnobChoiceOptionCompat(plugin->getPluginID(), plugin->getMajorVersion(), plugin->getMinorVersion(), natronMajor, natronMinor, natronRev, knob->getName(), &choiceID);
                }
            }
        }
        // use eValueChangedReasonPluginEdited for compat with Shuffle setChannelsFromRed()
        ValueChangedReturnCodeEnum s = knob->setValueFromID(choiceID, thisViewSpec, isCreatingNode ? eValueChangedReasonPluginEdited : eValueChangedReasonUserEdited);
        Q_UNUSED(s);
    } catch (const std::exception& e) {
        KnobHolderPtr holder = knob->getHolder();
        AppInstancePtr app;
        if (holder) {
            app = holder->getApp();
        }
        bool creatingNode = false;
        if (app) {
            creatingNode = app->isCreatingNode();
        }
        // Do not report exceptions when creating a node: we are likely trying to load a PyPlug from a Python
        // script and the choices may be outdated in the parameter.
        if (!creatingNode) {
            PyErr_SetString(PyExc_ValueError, e.what());
        }

    }
}

int
ChoiceParam::getValue(const QString& view) const
{
    return get(view);

}

void
ChoiceParam::setValue(int value, const QString& view)
{
    set(value, view);
}


void
ChoiceParam::setValue(const QString& label,  const QString& view)
{
    set(label,view);
}

int
ChoiceParam::getValueAtTime(double time, const QString& view) const
{
    return get(time, view);
}

void
ChoiceParam::setValueAtTime(int value,
                            double time, const QString& view)
{
    set(value, time, view);
}

void
ChoiceParam::setDefaultValue(int value)
{
    KnobChoicePtr knob = _choiceKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return;
    }
    knob->setDefaultValueWithoutApplying(value, DimSpec::all());
}

void
ChoiceParam::setDefaultValue(const QString& value)
{
    KnobChoicePtr knob = _choiceKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return;
    }
    try {
        knob->setDefaultValueFromIDWithoutApplying( value.toStdString() );
    } catch (const std::exception& e) {
        KnobHolderPtr holder = knob->getHolder();
        AppInstancePtr app;
        if (holder) {
            app = holder->getApp();
        }
        bool creatingNode = false;
        if (app) {
            creatingNode = app->isCreatingNode();
        }
        // Do not report exceptions when creating a node: we are likely trying to load a PyPlug from a Python
        // script and the choices may be outdated in the parameter.
        if (!creatingNode) {
            PyErr_SetString(PyExc_ValueError, e.what());
        }
    }

}

int
ChoiceParam::getDefaultValue() const
{
    KnobChoicePtr knob = _choiceKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return 0;
    }
    return knob->getDefaultValue(DimIdx(0));
}

void
ChoiceParam::restoreDefaultValue(const QString& view)
{
    KnobChoicePtr knob = _choiceKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return;
    }
    ViewSetSpec thisViewSpec;
    if (!getViewSetSpecFromViewName(view, &thisViewSpec)) {
        PythonSetInvalidViewName(view);
        return;
    }
    knob->resetToDefaultValue(DimSpec::all(), thisViewSpec);

}

void
ChoiceParam::addOption(const QString& optionID,
                       const QString& optionLabel,
                       const QString& optionHelper)
{
    KnobChoicePtr knob = _choiceKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return;
    }
    if (knob->getKnobDeclarationType() != KnobI::eKnobDeclarationTypeUser) {
        PythonSetNonUserKnobError();
        return;
    }
    if (optionID.isEmpty()) {
        PyErr_SetString(PyExc_ValueError, Param::tr("Cannot add an empty option").toStdString().c_str());
        return;
    }
    ChoiceOption c(optionID.toStdString(), optionLabel.toStdString(), optionHelper.toStdString());
    knob->appendChoice(c);

}

void
ChoiceParam::setOptions(const std::list<std::pair<QString, QString> >& options)
{
    std::list<QString> ids, labels, helps;
    for (std::list<std::pair<QString, QString> >::const_iterator it = options.begin(); it != options.end(); ++it) {
        ids.push_back(it->first);
        labels.push_back(it->first);
        helps.push_back(it->second);
    }
    setOptions(ids, labels, helps);
}

void
ChoiceParam::setOptions(const std::list<QString>& optionIDs,
                        const std::list<QString>& optionLabels,
                        const std::list<QString>& optionHelps)
{
    KnobChoicePtr knob = _choiceKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return;
    }
    if (knob->getKnobDeclarationType() != KnobI::eKnobDeclarationTypeUser) {
        PythonSetNonUserKnobError();
        return;
    }
    if (optionIDs.empty() ||
        optionIDs.size() != optionLabels.size() ||
        optionIDs.size() != optionHelps.size()) {
        return;
    }
    std::vector<ChoiceOption> entries(optionIDs.size());
    int i = 0;
    std::list<QString>::const_iterator itID = optionIDs.begin();
    std::list<QString>::const_iterator itLabel = optionLabels.begin();
    std::list<QString>::const_iterator itHelp = optionHelps.begin();
    for (; itID != optionIDs.end(); ++itID, ++itLabel, ++itHelp, ++i) {
        ChoiceOption& option = entries[i];
        option.id = itID->toStdString();
        option.label = itLabel->toStdString();
        option.tooltip = itHelp->toStdString();

    }
    knob->populateChoices(entries);
}

bool
ChoiceParam::getOption(int index, QString* optionID, QString* optionLabel, QString* optionHelp) const
{
    KnobChoicePtr knob = getRenderCloneKnob<KnobChoice>();
    if (!knob || !optionID || !optionLabel || !optionHelp) {
        PythonSetNullError();
        return false;
    }
    std::vector<ChoiceOption> entries =  knob->getEntries();


    if ( (index < 0) || ( index >= (int)entries.size() ) ) {
        PyErr_SetString(PyExc_IndexError, Param::tr("Option index out of range").toStdString().c_str());
        return false;
    }
    *optionID = QString::fromUtf8(entries[index].id.c_str());
    *optionLabel = QString::fromUtf8(entries[index].label.c_str());
    *optionHelp = QString::fromUtf8(entries[index].tooltip.c_str());
    return true;
}

void
ChoiceParam::getActiveOption(QString* optionID, QString* optionLabel, QString* optionHelp, const QString& view) const
{
    KnobChoicePtr knob = getRenderCloneKnob<KnobChoice>();
    if (!knob) {
        PythonSetNullError();
        return;
    }
    ViewIdx thisViewSpec;
    if (!getViewIdxFromViewName(view, &thisViewSpec)) {
        PythonSetInvalidViewName(view);
        return;
    }
    ChoiceOption opt = knob->getCurrentEntry(thisViewSpec);
    *optionID = QString::fromUtf8(opt.id.c_str());
    *optionLabel = QString::fromUtf8(opt.label.c_str());
    *optionHelp = QString::fromUtf8(opt.tooltip.c_str());

}

int
ChoiceParam::getNumOptions() const
{
    KnobChoicePtr knob = getRenderCloneKnob<KnobChoice>();
    if (!knob) {
        PythonSetNullError();
        return 0;
    }
    return knob->getNumEntries();
}

void
ChoiceParam::getOptions(std::list<QString>* optionIDs,
                        std::list<QString>* optionLabels,
                        std::list<QString>* optionHelps) const
{
    KnobChoicePtr knob = getRenderCloneKnob<KnobChoice>();
    if (!knob || !optionIDs || !optionLabels || !optionHelps) {
        PythonSetNullError();
        return;
    }
    std::vector<ChoiceOption> options =  knob->getEntries();
    for (std::size_t i = 0; i < options.size(); ++i) {
        optionIDs->push_back(QString::fromUtf8(options[i].id.c_str()));
        optionLabels->push_back(QString::fromUtf8(options[i].label.c_str()));
        optionHelps->push_back(QString::fromUtf8(options[i].tooltip.c_str()));
    }

}

int
ChoiceParam::addAsDependencyOf(Param* param, int fromExprDimension, int thisDimension, const QString& fromExprView, const QString& thisView)
{
    _addAsDependencyOf(param, fromExprDimension, thisDimension, fromExprView, thisView);

    return getValue();
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
BooleanParam::get(const QString& view) const
{
    KnobBoolPtr knob = getRenderCloneKnob<KnobBool>();
    if (!knob) {
        PythonSetNullError();
        return false;
    }
    ViewIdx thisViewSpec;
    if (!getViewIdxFromViewName(view, &thisViewSpec)) {
        PythonSetInvalidViewName(view);
        return false;
    }
    return knob->getValue(DimIdx(0), thisViewSpec);

}

bool
BooleanParam::get(double frame, const QString& view) const
{
    KnobBoolPtr knob = getRenderCloneKnob<KnobBool>();
    if (!knob) {
        PythonSetNullError();
        return false;
    }
    ViewIdx thisViewSpec;
    if (!getViewIdxFromViewName(view, &thisViewSpec)) {
        PythonSetInvalidViewName(view);
        return false;
    }
    return knob->getValueAtTime(TimeValue(frame), DimIdx(0), thisViewSpec);

}

void
BooleanParam::set(bool x, const QString& view)
{
    KnobBoolPtr knob = _boolKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return;
    }
    ViewSetSpec thisViewSpec;
    if (!getViewSetSpecFromViewName(view, &thisViewSpec)) {
        PythonSetInvalidViewName(view);
        return;
    }

    knob->setValue(x, thisViewSpec, DimIdx(0));

}

void
BooleanParam::set(bool x,
                  double frame, const QString& view)
{
    KnobBoolPtr knob = _boolKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return;
    }
    ViewSetSpec thisViewSpec;
    if (!getViewSetSpecFromViewName(view, &thisViewSpec)) {
        PythonSetInvalidViewName(view);
        return;
    }

    knob->setValueAtTime(TimeValue(frame), x, thisViewSpec, DimIdx(0));

}

bool
BooleanParam::getValue(const QString& view) const
{
    return get(view);
}

void
BooleanParam::setValue(bool value, const QString& view)
{
    set(value, view);
}

bool
BooleanParam::getValueAtTime(double time, const QString& view) const
{
    return get(time, view);
}

void
BooleanParam::setValueAtTime(bool value,
                             double time, const QString& view)
{
    set(value, time, view);
}

void
BooleanParam::setDefaultValue(bool value)
{
    KnobBoolPtr knob = _boolKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return;
    }
    knob->setDefaultValueWithoutApplying(value, DimSpec::all());
}

bool
BooleanParam::getDefaultValue() const
{
    KnobBoolPtr knob = _boolKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return false;
    }
    return knob->getDefaultValue(DimIdx(0));
}

void
BooleanParam::restoreDefaultValue(const QString& view)
{
    KnobBoolPtr knob = _boolKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return;
    }
    ViewSetSpec thisViewSpec;
    if (!getViewSetSpecFromViewName(view, &thisViewSpec)) {
        PythonSetInvalidViewName(view);
        return;
    }
    knob->resetToDefaultValue(DimSpec::all(), thisViewSpec);
}

bool
BooleanParam::addAsDependencyOf(Param* param, int fromExprDimension, int thisDimension, const QString& fromExprView, const QString& thisView)
{
    _addAsDependencyOf(param, fromExprDimension, thisDimension, fromExprView, thisView);

    return getValue();

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
StringParamBase::get(const QString& view) const
{
    KnobStringBasePtr knob = getRenderCloneKnob<KnobStringBase>();
    if (!knob) {
        PythonSetNullError();
        return QString();
    }
    ViewIdx thisViewSpec;
    if (!getViewIdxFromViewName(view, &thisViewSpec)) {
        PythonSetInvalidViewName(view);
        return QString();
    }
    return QString::fromUtf8(knob->getValue(DimIdx(0), thisViewSpec).c_str());
}

QString
StringParamBase::get(double frame, const QString& view) const
{
    KnobStringBasePtr knob = getRenderCloneKnob<KnobStringBase>();
    if (!knob) {
        PythonSetNullError();
        return QString();
    }
    ViewIdx thisViewSpec;
    if (!getViewIdxFromViewName(view, &thisViewSpec)) {
        PythonSetInvalidViewName(view);
        return QString();
    }
    return QString::fromUtf8(knob->getValueAtTime(TimeValue(frame), DimIdx(0), thisViewSpec).c_str());

}

void
StringParamBase::set(const QString& x, const QString& view)
{
    KnobStringBasePtr knob = _stringKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return;
    }
    ViewSetSpec thisViewSpec;
    if (!getViewSetSpecFromViewName(view, &thisViewSpec)) {
        PythonSetInvalidViewName(view);
        return;
    }
    knob->setValue(x.toStdString(), thisViewSpec);

}

void
StringParamBase::set(const QString& x,
                     double frame, const QString& view)
{
    KnobStringBasePtr knob = _stringKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return;
    }
    ViewSetSpec thisViewSpec;
    if (!getViewSetSpecFromViewName(view, &thisViewSpec)) {
        PythonSetInvalidViewName(view);
        return;
    }
    knob->setValueAtTime(TimeValue(frame), x.toStdString(), thisViewSpec);
}

QString
StringParamBase::getValue(const QString& view) const
{
    return get(view);
}

void
StringParamBase::setValue(const QString& value, const QString& view)
{
    set(value, view);
}

QString
StringParamBase::getValueAtTime(double time, const QString& view) const
{
    return get(time, view);
}

void
StringParamBase::setValueAtTime(const QString& value,
                                double time, const QString& view)
{
    set(value, time, view);

}

void
StringParamBase::setDefaultValue(const QString& value)
{
    KnobStringBasePtr knob = _stringKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return;
    }
    knob->setDefaultValueWithoutApplying(value.toStdString(), DimSpec::all());
}

QString
StringParamBase::getDefaultValue() const
{
    KnobStringBasePtr knob = _stringKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return QString();
    }
    return QString::fromUtf8( knob->getDefaultValue(DimIdx(0)).c_str() );
}

void
StringParamBase::restoreDefaultValue(const QString& view)
{
    KnobStringBasePtr knob = _stringKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return;
    }
    ViewSetSpec thisViewSpec;
    if (!getViewSetSpecFromViewName(view, &thisViewSpec)) {
        PythonSetInvalidViewName(view);
        return;
    }

    _stringKnob.lock()->resetToDefaultValue(DimSpec::all(), thisViewSpec);
}

QString
StringParamBase::addAsDependencyOf(Param* param, int fromExprDimension, int thisDimension, const QString& fromExprView, const QString& thisView)
{
    _addAsDependencyOf(param, fromExprDimension, thisDimension, fromExprView, thisView);

    return getValue();
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
    if (!knob) {
        PythonSetNullError();
        return;
    }
    if (knob->getKnobDeclarationType() != KnobI::eKnobDeclarationTypeUser) {
        PythonSetNonUserKnobError();
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
    if (!k) {
        PythonSetNullError();
        return;
    }
    if (k->getKnobDeclarationType() != KnobI::eKnobDeclarationTypeUser) {
        PythonSetNonUserKnobError();
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
    if (!k) {
        PythonSetNullError();
        return;
    }
    if (k->getKnobDeclarationType() != KnobI::eKnobDeclarationTypeUser) {
        PythonSetNonUserKnobError();
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
    KnobPathPtr k = _sKnob.lock();
    if (!k) {
        PythonSetNullError();
        return;
    }
    if (k->getKnobDeclarationType() != KnobI::eKnobDeclarationTypeUser) {
        PythonSetNonUserKnobError();
        return;
    }

    k->setMultiPath(true);
}

bool
PathParam::isMultiPathTable() const
{
    KnobPathPtr k = _sKnob.lock();
    if (!k) {
        PythonSetNullError();
        return false;
    }
    return k->isMultiPath();
}


void
PathParam::setTable(const std::list<std::vector<std::string> >& table)
{
    KnobPathPtr k = _sKnob.lock();
    if (!k) {
        PythonSetNullError();
        return;
    }
    if (!k->isMultiPath()) {
        PyErr_SetString(PyExc_ValueError, Param::tr("Cannot call setTable on a path parameter which is not multi-path").toStdString().c_str());
        return;
    }
    try {
        k->setTable(table);
    } catch (const std::exception& e) {
        PyErr_SetString(PyExc_ValueError, Param::tr("Error while encoding table: %1").arg(QString::fromUtf8(e.what())).toStdString().c_str());
    }
}


void
PathParam::getTable(std::list<std::vector<std::string> >* table) const
{
    KnobPathPtr k = getRenderCloneKnob<KnobPath>();
    if (!k) {
        PythonSetNullError();
        return;
    }
    if (!k->isMultiPath()) {
        PyErr_SetString(PyExc_ValueError, Param::tr("Cannot call getTable on a path parameter which is not multi-path").toStdString().c_str());
        return;
    }
    try {
        k->getTable(table);
    } catch (const std::exception& e) {
        PyErr_SetString(PyExc_ValueError, Param::tr("Error while decoding table: %1").arg(QString::fromUtf8(e.what())).toStdString().c_str());
    }
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
    KnobButtonPtr k = _buttonKnob.lock();
    if (!k) {
        PythonSetNullError();
        return false;
    }
    return k->getIsCheckable();
}

void
ButtonParam::setDown(bool down)
{
    KnobButtonPtr k = _buttonKnob.lock();
    if (!k) {
        PythonSetNullError();
        return;
    }
    k->setValue(down);
}


bool
ButtonParam::isDown() const
{
    KnobButtonPtr k = getRenderCloneKnob<KnobButton>();
    if (!k) {
        PythonSetNullError();
        return false;
    }
    return k->getValue();
}

void
ButtonParam::trigger()
{
    KnobButtonPtr k = _buttonKnob.lock();
    if (!k) {
        PythonSetNullError();
        return;
    }
    k->trigger();
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
        PythonSetNullError();
        return;
    }
    KnobIPtr otherKnob = param->getInternalKnob();
    if (!otherKnob) {
        PythonSetNullError();
    }
    KnobGroupPtr k = _groupKnob.lock();
    if (!k) {
        PythonSetNullError();
        return;
    }
    if (k->getKnobDeclarationType() != KnobI::eKnobDeclarationTypeUser) {
        PythonSetNonUserKnobError();
        return;
    }
    k->addKnob(otherKnob);
}

void
GroupParam::setAsTab()
{
    KnobGroupPtr k = _groupKnob.lock();
    if (!k) {
        PythonSetNullError();
        return;
    }
    if (k->getKnobDeclarationType() != KnobI::eKnobDeclarationTypeUser) {
        PythonSetNonUserKnobError();
        return;
    }

    k->setAsTab();
}

void
GroupParam::setOpened(bool opened)
{
    KnobGroupPtr k = _groupKnob.lock();
    if (!k) {
        PythonSetNullError();
        return;
    }
    k->setValue(opened);
}

bool
GroupParam::getIsOpened() const
{
    KnobGroupPtr k = getRenderCloneKnob<KnobGroup>();
    if (!k) {
        PythonSetNullError();
        return false;
    }
    return k->getValue();
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
        PythonSetNullError();
        return;
    }
    KnobIPtr otherKnob = param->getInternalKnob();
    if (!otherKnob) {
        PythonSetNullError();
    }
    KnobPagePtr k = _pageKnob.lock();
    if (!k) {
        PythonSetNullError();
        return;
    }
    if (k->getKnobDeclarationType() != KnobI::eKnobDeclarationTypeUser) {
        PythonSetNonUserKnobError();
        return;
    }
    k->addKnob(otherKnob);
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
    KnobParametricPtr knob = _parametricKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return;
    }
    if (dimension < 0 || dimension >= knob->getNDimensions()) {
        PythonSetInvalidDimensionError(dimension);
        return;
    }
    knob->setCurveColor(DimIdx(dimension), r, g, b);
}

void
ParametricParam::getCurveColor(int dimension,
                               ColorTuple& ret) const
{
    KnobParametricPtr knob = getRenderCloneKnob<KnobParametric>();
    if (!knob) {
        PythonSetNullError();
        return;
    }
    if (dimension < 0 || dimension >= knob->getNDimensions()) {
        PythonSetInvalidDimensionError(dimension);
        return;
    }
    knob->getCurveColor(DimIdx(dimension), &ret.r, &ret.g, &ret.b);
    ret.a = 1.;
}

bool
ParametricParam::addControlPoint(int dimension,
                                 double key,
                                 double value,
                                 KeyframeTypeEnum interpolation)
{
    KnobParametricPtr knob = _parametricKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return false;
    }
    if (dimension < 0 || dimension >= knob->getNDimensions()) {
        PythonSetInvalidDimensionError(dimension);
        return false;
    }
    return knob->addControlPoint(eValueChangedReasonUserEdited, DimIdx(dimension), key, value, interpolation);
}

bool
ParametricParam::addControlPoint(int dimension,
                                 double key,
                                 double value,
                                 double leftDerivative,
                                 double rightDerivative,
                                 KeyframeTypeEnum interpolation)
{
    KnobParametricPtr knob = _parametricKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return false;
    }
    if (dimension < 0 || dimension >= knob->getNDimensions()) {
        PythonSetInvalidDimensionError(dimension);
        return false;
    }
    return knob->addControlPoint(eValueChangedReasonUserEdited, DimIdx(dimension), key, value, leftDerivative, rightDerivative, interpolation);
}

double
ParametricParam::getValue(int dimension,
                          double parametricPosition) const
{
    KnobParametricPtr knob = getRenderCloneKnob<KnobParametric>();
    if (!knob) {
        PythonSetNullError();
        return 0.;
    }
    if (dimension < 0 || dimension >= knob->getNDimensions()) {
        PythonSetInvalidDimensionError(dimension);
        return 0.;
    }
    double ret;
    ActionRetCodeEnum stat =  knob->evaluateCurve(DimIdx(dimension), ViewIdx(0), parametricPosition, &ret);

    if (isFailureRetCode(stat)) {
        ret =  0.;
    }

    return ret;
}

int
ParametricParam::getNControlPoints(int dimension) const
{
    KnobParametricPtr knob = getRenderCloneKnob<KnobParametric>();
    if (!knob) {
        PythonSetNullError();
        return 0;
    }
    if (dimension < 0 || dimension >= knob->getNDimensions()) {
        PythonSetInvalidDimensionError(dimension);
        return 0;
    }
    int ret;
    ActionRetCodeEnum stat =  knob->getNControlPoints(DimIdx(dimension), ViewIdx(0), &ret);

    if (isFailureRetCode(stat)) {
        ret = 0;
    }

    return ret;
}

bool
ParametricParam::getNthControlPoint(int dimension,
                                    int nthCtl,
                                    double *key,
                                    double *value,
                                    double *leftDerivative,
                                    double *rightDerivative) const
{
    KnobParametricPtr knob = getRenderCloneKnob<KnobParametric>();
    if (!knob) {
        PythonSetNullError();
        return false;
    }
    if (dimension < 0 || dimension >= knob->getNDimensions()) {
        PythonSetInvalidDimensionError(dimension);
        return false;
    }
    ActionRetCodeEnum stat = knob->getNthControlPoint(DimIdx(dimension), ViewIdx(0), nthCtl, key, value, leftDerivative, rightDerivative);
    return !isFailureRetCode(stat);
}

bool
ParametricParam::setNthControlPoint(int dimension,
                                    int nthCtl,
                                    double key,
                                    double value,
                                    double leftDerivative,
                                    double rightDerivative)
{
    KnobParametricPtr knob = _parametricKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return false;
    }
    if (dimension < 0 || dimension >= knob->getNDimensions()) {
        PythonSetInvalidDimensionError(dimension);
        return false;
    }
    ActionRetCodeEnum stat = knob->setNthControlPoint(eValueChangedReasonUserEdited, DimIdx(dimension), ViewIdx(0), nthCtl, key, value, leftDerivative, rightDerivative);
    return !isFailureRetCode(stat);
}

bool
ParametricParam::setNthControlPointInterpolation(int dimension,
                                                 int nThCtl,
                                                 KeyframeTypeEnum interpolation)
{
    KnobParametricPtr knob = _parametricKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return false;
    }
    if (dimension < 0 || dimension >= knob->getNDimensions()) {
        PythonSetInvalidDimensionError(dimension);
        return false;
    }
    ActionRetCodeEnum stat = knob->setNthControlPointInterpolation(eValueChangedReasonUserEdited, DimIdx(dimension), ViewIdx(0), nThCtl, interpolation);
    return !isFailureRetCode(stat);
}

bool
ParametricParam::deleteControlPoint(int dimension,
                                    int nthCtl)
{
    KnobParametricPtr knob = _parametricKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return false;
    }
    if (dimension < 0 || dimension >= knob->getNDimensions()) {
        PythonSetInvalidDimensionError(dimension);
        return false;
    }
    ActionRetCodeEnum stat = knob->deleteControlPoint(eValueChangedReasonUserEdited, DimIdx(dimension), ViewIdx(0), nthCtl);
    return !isFailureRetCode(stat);
}

bool
ParametricParam::deleteAllControlPoints(int dimension)
{
    KnobParametricPtr knob = _parametricKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return false;
    }
    if (dimension < 0 || dimension >= knob->getNDimensions()) {
        PythonSetInvalidDimensionError(dimension);
        return false;
    }
    ActionRetCodeEnum stat = _parametricKnob.lock()->deleteAllControlPoints(eValueChangedReasonUserEdited, DimIdx(dimension),ViewIdx(0));
    return !isFailureRetCode(stat);
}

void
ParametricParam::setDefaultCurvesFromCurrentCurves()
{
    KnobParametricPtr knob = _parametricKnob.lock();
    if (!knob) {
        PythonSetNullError();
        return;
    }

    knob->setDefaultCurvesFromCurves();
}

NATRON_PYTHON_NAMESPACE_EXIT
NATRON_NAMESPACE_EXIT

