/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#include "Knob.h"
#include "KnobPrivate.h"
#include "KnobImpl.h"
#include "KnobGetValueImpl.h"
#include "KnobSetValueImpl.h"

#include <algorithm> // min, max
#include <cassert>
#include <stdexcept>
#include <sstream> // stringstream

#include <QtCore/QDataStream>
#include <QtCore/QDateTime>
#include <QtCore/QCoreApplication>
#include <QtCore/QThread>
#include <QtCore/QDebug>

#include "Global/GlobalDefines.h"

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/Curve.h"
#include "Engine/DockablePanelI.h"
#include "Engine/FrameViewRequest.h"
#include "Engine/OverlayInteractBase.h"
#include "Engine/LoadKnobsCompat.h"
#include "Engine/Hash64.h"
#include "Engine/KnobFile.h"
#include "Engine/KnobTypes.h"
#include "Engine/TreeRender.h"

#include "Serialization/ProjectSerialization.h"


SERIALIZATION_NAMESPACE_USING


NATRON_NAMESPACE_ENTER


KnobI::KnobI()
: AnimatingObjectI()
, boost::enable_shared_from_this<KnobI>()
, SERIALIZATION_NAMESPACE::SerializableObjectBase()
, HashableObject()
{
}


KnobI::~KnobI()
{
}

KnobSignalSlotHandler::KnobSignalSlotHandler(const KnobIPtr& knob)
    : QObject()
    , k(knob)
{
}




KnobPagePtr
KnobI::getTopLevelPage() const
{
    KnobIPtr parentKnob = getParentKnob();
    KnobIPtr parentKnobTmp = parentKnob;

    while (parentKnobTmp) {
        KnobIPtr parent = parentKnobTmp->getParentKnob();
        if (!parent) {
            break;
        } else {
            parentKnobTmp = parent;
        }
    }

    ////find in which page the knob should be
    KnobPagePtr isTopLevelParentAPage = toKnobPage(parentKnobTmp);

    return isTopLevelParentAPage;
}
KnobHelper::KnobHelper(const KnobHolderPtr& holder,
                       const std::string &scriptName,
                       int nDims)
    : _signalSlotHandler()
    , _imp( new KnobHelperPrivate(this, holder, nDims, scriptName) )
{

}

KnobHelper::KnobHelper(const KnobHolderPtr& holder, const KnobIPtr& mainKnob)
: _signalSlotHandler()
, _imp(new KnobHelperPrivate(this, holder, toKnobHelper(mainKnob)))
{

}

KnobHelper::~KnobHelper()
{
#ifdef DEBUG
    // For a render-clone, check that it is no longer registered in the clones map
    KnobHolderPtr holder = getHolder();
    if (holder && holder->getMainInstance()) {
        QMutexLocker locker(&_imp->common->renderClonesMapMutex);
        std::map<KnobHolderWPtr, KnobIWPtr>::iterator found = _imp->common->renderClonesMap.find(holder);
        assert(found == _imp->common->renderClonesMap.end());
    }

#endif
}

void
KnobHelper::setHolder(const KnobHolderPtr& holder)
{
    _imp->holder = holder;
}

void
KnobHelper::incrementExpressionRecursionLevel() const
{
    _imp->common->expressionRecursionLevelMutex.lock();
    ++_imp->common->expressionRecursionLevel;
}

void
KnobHelper::decrementExpressionRecursionLevel() const
{
    _imp->common->expressionRecursionLevelMutex.unlock();
    --_imp->common->expressionRecursionLevel;
}

int
KnobHelper::getExpressionRecursionLevel() const
{
    QMutexLocker k(&_imp->common->expressionRecursionLevelMutex);
    return _imp->common->expressionRecursionLevel;
}

void
KnobHelper::setHashingStrategy(KnobFrameViewHashingStrategyEnum strategy)
{
    _imp->common->cacheInvalidationStrategy = strategy;
}

KnobFrameViewHashingStrategyEnum
KnobHelper::getHashingStrategy() const
{
    return _imp->common->cacheInvalidationStrategy;
}

void
KnobHelper::deleteKnob()
{
    // Prevent any signal
    blockValueChanges();

    // Invalidate the expression of all listeners
    KnobDimViewKeySet listeners;
    for (KnobDimViewKeySet::iterator it = listeners.begin(); it != listeners.end(); ++it) {
        KnobIPtr knob = it->knob.lock();
        if (!knob) {
            continue;
        }

        // Check if the other knob listens to with an expression
        std::string expression = knob->getExpression(it->dimension, it->view);
        if (expression.empty()) {
            continue;
        }
        knob->setLinkStatus( it->dimension, it->view, false, tr("%1: parameter does not exist").arg( QString::fromUtf8( getName().c_str() ) ).toStdString() );
        if (knob.get() != this) {
            knob->unlink(DimSpec::all(), ViewSetSpec::all(), false);
        }
    }

    KnobHolderPtr holder = getHolder();

    if ( holder && holder->getApp() ) {
        holder->getApp()->recheckInvalidLinks();
    }

    clearExpression(DimSpec::all(), ViewSetSpec::all());


    resetParent();


    if (holder) {

        // For containers also delete children.
        KnobGroup* isGrp =  dynamic_cast<KnobGroup*>(this);
        KnobPage* isPage = dynamic_cast<KnobPage*>(this);
        if (isGrp)     {
            KnobsVec children = isGrp->getChildren();
            for (KnobsVec::iterator it = children.begin(); it != children.end(); ++it) {
                holder->deleteKnob(*it, true);
            }
        } else if (isPage) {
            KnobsVec children = isPage->getChildren();
            for (KnobsVec::iterator it = children.begin(); it != children.end(); ++it) {
                holder->deleteKnob(*it, true);
            }
        }

        EffectInstancePtr effect = toEffectInstance(holder);
        if (effect) {
            NodePtr node = effect->getNode();
            if (node) {
                node->removeParameterFromPython( getName() );
            }
        }
    }
} // KnobHelper::deleteKnob

void
KnobHelper::convertDimViewArgAccordingToKnobState(DimSpec dimIn, ViewSetSpec viewIn, DimSpec* dimOut, ViewSetSpec* viewOut) const
{

    std::list<ViewIdx> targetViews = getViewsList();

    // If target view is all but target is not multi-view, convert back to main view
    *viewOut = viewIn;
    if (targetViews.size() == 1) {
        *viewOut = ViewSetSpec(targetViews.front());
    }
    // If pasting on a folded knob view,
    int nDims = getNDimensions();
    *dimOut = dimIn;
    if (nDims == 1) {
        *dimOut = DimSpec(0);
    }
    if ( (*dimOut == 0) && nDims > 1 && !viewOut->isAll() && !getAllDimensionsVisible(ViewIdx(*viewOut)) ) {
        *dimOut = DimSpec::all();
    }
}

bool
KnobI::hasAnyExpression() const
{
    std::list<ViewIdx> views = getViewsList();
    for (int i = 0; i < getNDimensions(); ++i) {
        for (std::list<ViewIdx>::const_iterator it = views.begin(); it!=views.end(); ++it) {
            if (hasExpression(DimIdx(i), *it)) {
                return true;
            }
        }
    }
    return false;
}

bool
KnobHelper::getAllDimensionsVisible(ViewIdx view) const
{
    ViewIdx view_i = checkIfViewExistsOrFallbackMainView(view);
    QMutexLocker k(&_imp->common->stateMutex);
    PerViewAllDimensionsVisible::const_iterator foundView = _imp->common->allDimensionsVisible.find(view_i);
    if (foundView == _imp->common->allDimensionsVisible.end()) {
        return true;
    }
    return foundView->second;
}

void
KnobHelper::autoAdjustFoldExpandDimensions(ViewIdx view)
{
    // This flag is used to temporarily disable the auto expanding or folding of dimensions.
    // Mainly this is to help the implementation when setting multiple values at once.
    if (!isAdjustFoldExpandStateAutomaticallyEnabled()) {
        return;
    }
    bool currentVisibility = getAllDimensionsVisible(view);
    bool allEqual = areDimensionsEqual(view);
    if (allEqual) {
        // If auto-fold is enabled, fold it
        if (isAutoFoldDimensionsEnabled()) {
            if (currentVisibility) {
                setAllDimensionsVisible(view, false);
            }
        }
    } else {
        // One of the dimension differ: make them all visible
        if (!currentVisibility) {
            setAllDimensionsVisible(view, true);
        }
    }
}


void
KnobHelper::autoFoldDimensions(ViewIdx view)
{
    if (!isAutoFoldDimensionsEnabled()) {
        return;
    }

    bool curVisible = getAllDimensionsVisible(view);

    // If already folded, don't do anything
    if (!curVisible) {
        return;
    }

    bool allEquals = areDimensionsEqual(view);
    if (allEquals) {
        setAllDimensionsVisible(view, false);
    }

}


void
KnobHelper::setCanAutoFoldDimensions(bool enabled)
{
    {
        QMutexLocker k(&_imp->common->stateMutex);
        _imp->common->autoFoldEnabled = enabled;
    }
    if (!enabled) {
        setAllDimensionsVisible(ViewSetSpec::all(), true);
    }
}

bool
KnobHelper::isAutoFoldDimensionsEnabled() const
{
    QMutexLocker k(&_imp->common->stateMutex);
    return _imp->common->autoFoldEnabled;
}


void
KnobHelper::setAdjustFoldExpandStateAutomatically(bool enabled)
{
    {
        QMutexLocker k(&_imp->common->stateMutex);
        _imp->common->autoAdjustFoldExpandEnabled = enabled;
    }
}

bool
KnobHelper::isAdjustFoldExpandStateAutomaticallyEnabled() const
{
    QMutexLocker k(&_imp->common->stateMutex);
    return _imp->common->autoAdjustFoldExpandEnabled;
}


void
KnobHelper::setAllDimensionsVisibleInternal(ViewIdx view, bool visible)
{
    {
        QMutexLocker k(&_imp->common->stateMutex);
        bool& curValue = _imp->common->allDimensionsVisible[view];
        if (curValue == visible) {
            return;
        }
        curValue = visible;
    }
#if 0
    if (!visible) {
        // Prevent copyKnob from recomputing the allDimensionsVisible flag
        setAdjustFoldExpandStateAutomatically(false);
        int nDims = getNDimensions();
        {
            ScopedChanges_RAII changes(this);

            KnobIPtr thisShared = shared_from_this();
            for (int i = 1; i < nDims; ++i) {
                // When folding, copy the values of the first dimension to other dimensions
                copyKnob(thisShared, view, DimIdx(i), view, DimIdx(0));
            }
        }
        setAdjustFoldExpandStateAutomatically(true);
    }
#endif
}

void
KnobHelper::setAllDimensionsVisible(ViewSetSpec view, bool visible)
{
    {
        ScopedChanges_RAII changes(this);
        if (view.isAll()) {
            std::list<ViewIdx> views = getViewsList();
            for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
                setAllDimensionsVisibleInternal(*it, visible);
            }
        } else {
            ViewIdx view_i = checkIfViewExistsOrFallbackMainView(ViewIdx(view));
            setAllDimensionsVisibleInternal(view_i, visible);
        }
    }
    if (_signalSlotHandler) {
        _signalSlotHandler->s_dimensionsVisibilityChanged(view);
    }
}

#ifdef DEBUG
void
KnobHelper::debugHook()
{
    assert(true);
}

#endif

void
KnobHelper::setKnobDeclarationType(KnobI::KnobDeclarationTypeEnum b)
{
    _imp->common->declarationType = b;
}

KnobI::KnobDeclarationTypeEnum
KnobHelper::getKnobDeclarationType() const
{
    return _imp->common->declarationType;
}

void
KnobHelper::setKeyFrameTrackingEnabled(bool enabled)
{
    {
        QMutexLocker k(&_imp->common->stateMutex);
        _imp->common->keyframeTrackingEnabled = enabled;
    }
    if (enabled) {
        _signalSlotHandler->s_curveAnimationChanged(ViewSetSpec::all(), DimSpec::all());
    }

}

bool
KnobHelper::isKeyFrameTrackingEnabled() const
{
    QMutexLocker k(&_imp->common->stateMutex);
    return _imp->common->keyframeTrackingEnabled;
}

static
std::string
unsignedToString(unsigned i)
{
    if (i == 0) {
        return "0";
    }
    std::string nb;
    for (unsigned j = i; j != 0; j /= 10) {
        nb = (char)( '0' + (j % 10) ) + nb;
    }

    return nb;
}

void
KnobHelper::populate()
{
    KnobIPtr thisKnob = shared_from_this();

    KnobHolderPtr holder = getHolder();

    KnobHelperPtr mainInstance = _imp->mainInstance.lock();
    if (holder && !mainInstance) {
        // When knob value changes, the holder needs to be invalidated aswell
        addHashListener(holder);
        holder->addHashDependency(thisKnob);
    }

    // Register the knob in the render clones map
    {
        QMutexLocker locker(&_imp->common->renderClonesMapMutex);
        assert(_imp->common->renderClonesMap.find(holder) == _imp->common->renderClonesMap.end());
        _imp->common->renderClonesMap[holder] = thisKnob;
    }

    // Do not set attributes on a render clone
    if (mainInstance) {
        return;
    }
    boost::shared_ptr<KnobSignalSlotHandler> handler = boost::make_shared<KnobSignalSlotHandler>(thisKnob);

    setSignalSlotHandler(handler);

    if (!isAnimatedByDefault()) {
        _imp->common->isAnimationEnabled = false;
    }

    KnobSeparator* isSep = dynamic_cast<KnobSeparator*>(this);
    KnobPage* isPage = dynamic_cast<KnobPage*>(this);
    KnobGroup* isGrp = dynamic_cast<KnobGroup*>(this);
    if (isPage || isGrp) {
        _imp->common->evaluateOnChange = false;
    }
    if (isSep) {
        _imp->common->IsPersistent = false;
    }

    KnobColorPtr isColor = toKnobColor(thisKnob);
    KnobChoicePtr isChoice = toKnobChoice(thisKnob);
    KnobIntBasePtr isIntBase = toKnobIntBase(thisKnob);
    KnobStringBasePtr isStringBase = toKnobStringBase(thisKnob);
    KnobBoolBasePtr isBoolBase = toKnobBoolBase(thisKnob);


    CurveTypeEnum curveType = getKeyFrameDataType();

    for (int i = 0; i < _imp->common->dimension; ++i) {
        KnobDimViewBasePtr data = createDimViewData();
        data->sharedKnobs.insert(KnobDimViewKey(thisKnob, DimIdx(i), ViewIdx(0)));
        _imp->common->perDimViewData[i][ViewIdx(0)] = data;

        if ( canAnimate() ) {
            data->animationCurve.reset(new Curve(curveType));
        }
    }

    if (_imp->common->dimension > 4) {
        for (int i = 0; i < getNDimensions(); ++i) {
            _imp->common->dimensionNames[i] = unsignedToString(i);
        }
    } else {
        if (!isColor) {
            _imp->common->dimensionNames[0] = "x";
            if (_imp->common->dimensionNames.size() > 1) {
                _imp->common->dimensionNames[1] = "y";
            }
            if (_imp->common->dimensionNames.size() > 2) {
                _imp->common->dimensionNames[2] = "z";
            }
            if (_imp->common->dimensionNames.size() > 3) {
                _imp->common->dimensionNames[3] = "w";
            }
        } else {
            _imp->common->dimensionNames[0] = "r";
            if (_imp->common->dimensionNames.size() > 1) {
                _imp->common->dimensionNames[1] = "g";
            }
            if (_imp->common->dimensionNames.size() > 2) {
                _imp->common->dimensionNames[2] = "b";
            }
            if (_imp->common->dimensionNames.size() > 3) {
                _imp->common->dimensionNames[3] = "a";
            }
        }
    }
} // KnobHelper::populate

std::string
KnobHelper::getDimensionName(DimIdx dimension) const
{
    if ( (dimension < 0) || ( dimension >= (int)_imp->common->dimensionNames.size() ) ) {
        throw std::invalid_argument("KnobHelper::getDimensionName: dimension out of range");
    }
    return _imp->common->dimensionNames[dimension];
}

void
KnobHelper::setDimensionName(DimIdx dimension,
                             const std::string & name)
{
    if ( (dimension < 0) || ( dimension >= (int)_imp->common->dimensionNames.size() ) ) {
        throw std::invalid_argument("KnobHelper::getDimensionName: dimension out of range");
    }
    _imp->common->dimensionNames[dimension] = name;
    _signalSlotHandler->s_dimensionNameChanged(dimension);

}


void
KnobHelper::setSignalSlotHandler(const KnobSignalSlotHandlerPtr & handler)
{
    _signalSlotHandler = handler;
}




bool
KnobHelper::isAnimated(DimIdx dimension,
                       ViewIdx view) const
{
    if (dimension < 0 || dimension >= (int)_imp->common->dimension) {
        throw std::invalid_argument("KnobHelper::isAnimated; dimension out of range");
    }

    if ( !canAnimate() ) {
        return false;
    }
    ViewIdx view_i = checkIfViewExistsOrFallbackMainView(view);
    CurvePtr curve = getAnimationCurve(view_i, dimension);
    return curve ? curve->isAnimated() : false;
}

bool
KnobHelper::canSplitViews() const
{
    return isAnimationEnabled();
}

void
KnobHelper::setExpressionsResultsCachingEnabled(bool enabled)
{
    QMutexLocker k(&_imp->common->expressionMutex);
    _imp->common->enableExpressionCaching = enabled;
}

bool
KnobHelper::isExpressionsResultsCachingEnabled() const
{
    QMutexLocker k(&_imp->common->expressionMutex);
    return _imp->common->enableExpressionCaching;
}

void
KnobDimViewBase::notifyCurveChanged()
{
    KnobDimViewKeySet knobs;
    {
        QMutexLocker k(&valueMutex);
        knobs = sharedKnobs;
    }
    for (KnobDimViewKeySet::const_iterator it = knobs.begin(); it!=knobs.end(); ++it) {
        KnobIPtr knob = it->knob.lock();
        if (knob) {
            boost::shared_ptr<KnobSignalSlotHandler> handler = knob->getSignalSlotHandler();
            if (handler) {
                handler->s_curveAnimationChanged(it->view, it->dimension);
            }
        }
    }
}



KnobDimViewBasePtr
KnobHelper::getDataForDimView(DimIdx dimension, ViewIdx view) const
{
    if (dimension < 0 || dimension >= (int)_imp->common->dimension) {
        throw std::invalid_argument("KnobHelper::getDataForDimView: dimension out of range");
    }
    QMutexLocker k(&_imp->common->perDimViewDataMutex);
    PerViewKnobDataMap::iterator found = _imp->common->perDimViewData[dimension].find(view);
    if (found == _imp->common->perDimViewData[dimension].end()) {
        return KnobDimViewBasePtr();
    }
    return found->second;
}

bool
KnobHelper::splitView(ViewIdx view)
{
    if (!AnimatingObjectI::splitView(view)) {
        return false;
    }
    KnobIPtr thisKnob = shared_from_this();
    int nDims = getNDimensions();
    for (int i = 0; i < nDims; ++i) {
        {
            QMutexLocker k(&_imp->common->perDimViewDataMutex);
            const KnobDimViewBasePtr& mainViewData = _imp->common->perDimViewData[i][ViewIdx(0)];
            if (mainViewData) {
                KnobDimViewBasePtr& viewData = _imp->common->perDimViewData[i][view];
                if (!viewData) {
                    viewData = createDimViewData();
                }
                KnobDimViewBase::CopyInArgs inArgs(*mainViewData);
                viewData->copy(inArgs, 0);
                viewData->sharedKnobs.insert(KnobDimViewKey(thisKnob, DimIdx(i), ViewIdx(0)));
            }
        }
        _signalSlotHandler->s_curveAnimationChanged(view, DimIdx(i));

        {
            QMutexLocker k(&_imp->common->hasModificationsMutex);
            _imp->common->hasModifications[i][view] = _imp->common->hasModifications[i][ViewIdx(0)];
        }
        {
            QMutexLocker k(&_imp->common->stateMutex);
            _imp->common->allDimensionsVisible[view] = _imp->common->allDimensionsVisible[ViewIdx(0)];
        }
    }

    _signalSlotHandler->s_availableViewsChanged();
    return true;
} // splitView

bool
KnobHelper::unSplitView(ViewIdx view)
{
    if (!AnimatingObjectI::unSplitView(view)) {
        return false;
    }
    int nDims = getNDimensions();
    for (int i = 0; i < nDims; ++i) {
        {
            QMutexLocker k(&_imp->common->perDimViewDataMutex);
            PerViewKnobDataMap::iterator foundView = _imp->common->perDimViewData[i].find(view);
            if (foundView != _imp->common->perDimViewData[i].end()) {
                _imp->common->perDimViewData[i].erase(foundView);

            }
            PerViewSavedDataMap::iterator foundSavedData = _imp->common->perDimViewSavedData[i].find(view);
            if (foundSavedData != _imp->common->perDimViewSavedData[i].end()) {
                _imp->common->perDimViewSavedData[i].erase(foundSavedData);
            }
        }

        {
            QMutexLocker k(&_imp->common->hasModificationsMutex);
            PerViewHasModificationMap::iterator foundView = _imp->common->hasModifications[i].find(view);
            if (foundView != _imp->common->hasModifications[i].end()) {
                _imp->common->hasModifications[i].erase(foundView);
            }
        }
        {
            QMutexLocker k(&_imp->common->stateMutex);
            PerViewAllDimensionsVisible::iterator foundView = _imp->common->allDimensionsVisible.find(view);
            if (foundView != _imp->common->allDimensionsVisible.end()) {
                _imp->common->allDimensionsVisible.erase(foundView);
            }
        }
    }
    _signalSlotHandler->s_availableViewsChanged();
    return true;
} // unSplitView

int
KnobHelper::getNDimensions() const
{
    return _imp->common->dimension;
}

void
KnobHelper::beginChanges()
{
    KnobHolderPtr holder = getHolder();
    if (holder) {
        holder->beginChanges();
    }
}

void
KnobHelper::endChanges()
{
    KnobHolderPtr holder = getHolder();
    if (holder) {
        holder->endChanges();
    }
}

void
KnobHelper::blockValueChanges()
{
    {
        QMutexLocker k(&_imp->common->valueChangedBlockedMutex);

        ++_imp->common->valueChangedBlocked;
    }
}

void
KnobHelper::unblockValueChanges()
{
    QMutexLocker k(&_imp->common->valueChangedBlockedMutex);

    --_imp->common->valueChangedBlocked;
}

bool
KnobHelper::isValueChangesBlocked() const
{
    QMutexLocker k(&_imp->common->valueChangedBlockedMutex);

    return _imp->common->valueChangedBlocked > 0;
}

void
KnobHelper::setAutoKeyingEnabled(bool enabled)
{
    QMutexLocker k(&_imp->common->valueChangedBlockedMutex);
    if (enabled) {
        ++_imp->common->autoKeyingDisabled;
    } else {
        --_imp->common->autoKeyingDisabled;
    }
}

bool
KnobHelper::isAutoKeyingEnabledInternal(DimIdx dimension, TimeValue time, ViewIdx view) const
{

    if (dimension < 0 || dimension >= _imp->common->dimension) {
        return false;
    }


    // The knob doesn't have any animation don't start keying automatically
    AnimationLevelEnum level = getAnimationLevel(dimension, time, view);
    if (level == eAnimationLevelNone ||
        level == eAnimationLevelExpression) {
        return false;
    }

    return true;

}

bool
KnobHelper::isAutoKeyingEnabled(DimSpec dimension, TimeValue time, ViewSetSpec view, ValueChangedReasonEnum reason) const
{

    // Knobs without an effect cannot auto-key
    KnobHolderPtr holder = getHolder();
    if (!holder) {
        return false;
    }


    // Hmm this is a custom Knob used somewhere, don't allow auto-keying
    if (!holder->getApp()) {
        return false;
    }

    // Check for reason appropriate for auto-keying
    if ( (reason != eValueChangedReasonUserEdited) &&
        (reason != eValueChangedReasonPluginEdited) &&
        (reason != eValueChangedReasonUserEdited) &&
        (reason != eValueChangedReasonUserEdited)) {
        return false;
    }

    // The knob cannot animate
    if (!isAnimationEnabled()) {
        return false;
    }

    bool hasAutoKeying = false;
    std::list<ViewIdx> views = getViewsList();
    if (dimension.isAll()) {
        for (int i = 0; i < _imp->common->dimension; ++i) {
            if (view.isAll()) {
                for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
                    hasAutoKeying |= isAutoKeyingEnabledInternal(DimIdx(i), time, *it);
                }
            } else {
                ViewIdx view_i = checkIfViewExistsOrFallbackMainView(ViewIdx(view.value()));
                hasAutoKeying |= isAutoKeyingEnabledInternal(DimIdx(i), time, view_i);
            }
        }
    } else {
        if ( ( dimension >= _imp->common->dimension ) || (dimension < 0) ) {
            throw std::invalid_argument("KnobHelper::isAutoKeyingEnabled(): Dimension out of range");
        }
        if (view.isAll()) {
            for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
                hasAutoKeying |= isAutoKeyingEnabledInternal(DimIdx(dimension), time, *it);
            }
        } else {
            ViewIdx view_i = checkIfViewExistsOrFallbackMainView(ViewIdx(view.value()));
            hasAutoKeying |= isAutoKeyingEnabledInternal(DimIdx(dimension), time, view_i);
        }
    }

    if (!hasAutoKeying) {
        return false;
    }

    // Finally return the value set to setAutoKeyingEnabled
    QMutexLocker k(&_imp->common->valueChangedBlockedMutex);
    return !_imp->common->autoKeyingDisabled;
} // isAutoKeyingEnabled

bool
KnobHelper::evaluateValueChangeInternal(DimSpec dimension,
                                        TimeValue time,
                                        ViewSetSpec view,
                                        ValueChangedReasonEnum reason,
                                        KnobDimViewKeySet* evaluatedKnobs)
{

    KnobHolderPtr holder = getHolder();
    if (!holder) {

        // Just refresh the gui
        if (!isValueChangesBlocked()) {
            _signalSlotHandler->s_mustRefreshKnobGui(view, dimension, reason);
        }

        return true;
    }



    KnobIPtr thisShared = shared_from_this();

    // This knob was already evaluated
    {
        DimIdx dimensionIndex = dimension.isAll() ? DimIdx(getNDimensions()) : DimIdx(dimension);
        ViewIdx viewIndex = view.isAll() ? ViewIdx(getViewsList().size()) : ViewIdx(view);
        KnobDimViewKey key(thisShared, dimensionIndex, viewIndex);
        if (evaluatedKnobs->find(key) != evaluatedKnobs->end()) {
            return false;
        }

        evaluatedKnobs->insert(key);
    }

    if (reason == eValueChangedReasonTimeChanged) {
        // Only notify gui must be refreshed when reason is time changed
        if (!isValueChangesBlocked()) {
            _signalSlotHandler->s_mustRefreshKnobGui(view, dimension, reason);
        }
        return true;
    }

    AppInstancePtr app = holder->getApp();
    bool didSomething;
    {
        ScopedChanges_RAII changes(holder.get());

        // Notify gui must be refreshed
        _signalSlotHandler->s_mustRefreshKnobGui(view, dimension, reason);

        // Refresh modifications state
        computeHasModifications();

        // Invalidate the hash cache
        invalidateHashCache();

        // Invalidate expression results
        clearExpressionsResults(dimension, view);

        // Call knobChanged action
        didSomething = holder->onKnobValueChangedInternal(thisShared, time, view, reason);


        // Refresh dependencies
        refreshListenersAfterValueChange(time, view, reason, dimension, evaluatedKnobs);

    }

    return didSomething;
} // evaluateValueChangeInternal

bool
KnobHelper::evaluateValueChange(DimSpec dimension,
                                TimeValue time,
                                ViewSetSpec view,
                                ValueChangedReasonEnum reason)
{
    KnobDimViewKeySet evaluatedKnobs;
    return evaluateValueChangeInternal(dimension, time, view, reason, &evaluatedKnobs);
}

void
KnobHelper::refreshListenersAfterValueChangeInternal(TimeValue time, ViewIdx view, ValueChangedReasonEnum reason, DimIdx dimension, KnobDimViewKeySet* evaluatedKnobs)
{
    KnobDimViewBasePtr data = getDataForDimView(dimension, view);
    if (!data) {
        return;
    }

    KnobDimViewKeySet allListeners;

    // Get all listeners via expressions
    {
        QMutexLocker l(&_imp->common->expressionMutex);
        KnobDimViewKeySet& listeners = _imp->common->listeners[dimension][view];
        allListeners.insert(listeners.begin(), listeners.end());
    }

    // Get all listeners via shared values
    {
        QMutexLocker k(&data->valueMutex);
        allListeners.insert(data->sharedKnobs.begin(), data->sharedKnobs.end());
    }

    for (KnobDimViewKeySet::const_iterator it = allListeners.begin(); it != allListeners.end(); ++it) {
        KnobHelperPtr sharedKnob = toKnobHelper(it->knob.lock());
        if (sharedKnob && sharedKnob.get() != this) {
            sharedKnob->autoAdjustFoldExpandDimensions(view);
            sharedKnob->evaluateValueChangeInternal(it->dimension, time, it->view, reason, evaluatedKnobs);
        }
    }

}

void
KnobHelper::refreshListenersAfterValueChange(TimeValue time, ViewSetSpec view, ValueChangedReasonEnum reason, DimSpec dimension, KnobDimViewKeySet* evaluatedKnobs)
{

    std::list<ViewIdx> views = getViewsList();
    ViewIdx view_i;
    if (!view.isAll()) {
        view_i = checkIfViewExistsOrFallbackMainView(ViewIdx(view));
    }
    int nDims = getNDimensions();
    for (std::list<ViewIdx>::const_iterator it = views.begin(); it!=views.end(); ++it) {
        if (!view.isAll() && *it != view_i) {
            continue;
        }
        for (int i = 0; i < nDims; ++i) {
            if (!dimension.isAll() && i != dimension) {
                continue;
            }
            refreshListenersAfterValueChangeInternal(time, *it, reason, DimIdx(i), evaluatedKnobs);
        }
    }

} // KnobHelper::refreshListenersAfterValueChange

void
KnobHelper::onTimeChanged(bool isPlayback,  TimeValue time)
{
    if ( getIsSecret() ) {
        return;
    }

    if (hasAnimation()) {
         _signalSlotHandler->s_mustRefreshKnobGui(ViewSetSpec::all(), DimSpec::all(), eValueChangedReasonTimeChanged);
    }
    if (evaluateValueChangeOnTimeChange() && !isPlayback) {
        KnobHolderPtr holder = getHolder();
        if (holder) {
            holder->onKnobValueChanged_public(shared_from_this(), eValueChangedReasonTimeChanged, time, ViewSetSpec::all());
        }
    }
} // onTimeChanged

void
KnobHelper::setAddNewLine(bool newLine)
{
    _imp->common->newLine = newLine;
}

bool
KnobHelper::isNewLineActivated() const
{
    return _imp->common->newLine;
}

void
KnobHelper::setAddSeparator(bool addSep)
{
    _imp->common->addSeparator = addSep;
}

bool
KnobHelper::isSeparatorActivated() const
{
    return _imp->common->addSeparator;
}

void
KnobHelper::setSpacingBetweenItems(int spacing)
{
    _imp->common->itemSpacing = spacing;
}

int
KnobHelper::getSpacingBetweenitems() const
{
    return _imp->common->itemSpacing;
}

std::string
KnobHelper::getInViewerContextLabel() const
{
    QMutexLocker k(&_imp->common->labelMutex);

    return _imp->common->inViewerContextLabel;
}

void
KnobHelper::setInViewerContextLabel(const QString& label)
{
    {
        QMutexLocker k(&_imp->common->labelMutex);

        _imp->common->inViewerContextLabel = label.toStdString();
    }
    _signalSlotHandler->s_inViewerContextLabelChanged();
}

std::string
KnobHelper::getInViewerContextIconFilePath(bool checked) const
{
    QMutexLocker k(&_imp->common->labelMutex);
    int idx = !checked ? 0 : 1;

    if ( !_imp->common->inViewerContextIconFilePath[idx].empty() ) {
        return _imp->common->inViewerContextIconFilePath[idx];
    }
    int otherIdx = !checked ? 1 : 0;

    return _imp->common->inViewerContextIconFilePath[otherIdx];
}

void
KnobHelper::setInViewerContextIconFilePath(const std::string& icon, bool checked)
{
    QMutexLocker k(&_imp->common->labelMutex);
    int idx = !checked ? 0 : 1;

    _imp->common->inViewerContextIconFilePath[idx] = icon;
}

void
KnobHelper::setInViewerContextCanHaveShortcut(bool haveShortcut)
{
    _imp->common->inViewerContextHasShortcut = haveShortcut;
}

bool
KnobHelper::getInViewerContextHasShortcut() const
{
    return _imp->common->inViewerContextHasShortcut;
}

void
KnobHelper::addInViewerContextShortcutsReference(const std::string& actionID)
{
    _imp->common->additionalShortcutsInTooltip.push_back(actionID);
}

const std::list<std::string>&
KnobHelper::getInViewerContextAdditionalShortcuts() const
{
    return _imp->common->additionalShortcutsInTooltip;
}

void
KnobHelper::setInViewerContextItemSpacing(int spacing)
{
    _imp->common->inViewerContextItemSpacing = spacing;
}

int
KnobHelper::getInViewerContextItemSpacing() const
{
    return _imp->common->inViewerContextItemSpacing;
}

void
KnobHelper::setInViewerContextLayoutType(ViewerContextLayoutTypeEnum layoutType)
{
    _imp->common->inViewerContextLayoutType = layoutType;
}

ViewerContextLayoutTypeEnum
KnobHelper::getInViewerContextLayoutType() const
{
    return _imp->common->inViewerContextLayoutType;
}

void
KnobHelper::setInViewerContextSecret(bool secret)
{
    {
        QMutexLocker k(&_imp->common->stateMutex);
        _imp->common->inViewerContextSecret = secret;
    }
    _signalSlotHandler->s_viewerContextSecretChanged();
}

bool
KnobHelper::getInViewerContextSecret() const
{
    QMutexLocker k(&_imp->common->stateMutex);

    return _imp->common->inViewerContextSecret;
}

void
KnobHelper::setEnabled(bool b)
{
    {
        QMutexLocker k(&_imp->common->stateMutex);
        _imp->common->enabled = b;
    }
    if (_signalSlotHandler) {
        _signalSlotHandler->s_enabledChanged();
    }
}


void
KnobHelper::setSecret(bool b)
{
    {
        QMutexLocker k(&_imp->common->stateMutex);
        if (_imp->common->IsSecret == b) {
            return;
        }
        _imp->common->IsSecret = b;
    }

    ///the knob was revealed , refresh its gui to the current time
    if (!b) {
        KnobHolderPtr holder = getHolder();
        if (holder) {
            AppInstancePtr app = holder->getApp();
            if (app) {
                onTimeChanged( false, TimeValue(app->getTimeLine()->currentFrame()) );
            }
        }
    }
    if (_signalSlotHandler) {
        _signalSlotHandler->s_secretChanged();
    }
}

int
KnobHelper::determineHierarchySize() const
{
    int ret = 0;
    KnobIPtr current = getParentKnob();

    while (current) {
        ++ret;
        current = current->getParentKnob();
    }

    return ret;
}

std::string
KnobHelper::getLabel() const
{
    QMutexLocker k(&_imp->common->labelMutex);

    return _imp->common->label;
}

void
KnobHelper::setLabel(const std::string& label)
{
    {
        QMutexLocker k(&_imp->common->labelMutex);
        _imp->common->label = label;
    }
    if (_signalSlotHandler) {
        _signalSlotHandler->s_labelChanged();
    }
}

void
KnobHelper::setIconLabel(const std::string& iconFilePath,
                         bool checked,
                         bool alsoSetViewerUIIcon)
{
    {
        QMutexLocker k(&_imp->common->labelMutex);
        int idx = !checked ? 0 : 1;

        _imp->common->iconFilePath[idx] = iconFilePath;
    }
    if (alsoSetViewerUIIcon) {
        setInViewerContextIconFilePath(iconFilePath, checked);
    }
}

const std::string&
KnobHelper::getIconLabel(bool checked) const
{
    QMutexLocker k(&_imp->common->labelMutex);
    int idx = !checked ? 0 : 1;

    if ( !_imp->common->iconFilePath[idx].empty() ) {
        return _imp->common->iconFilePath[idx];
    }
    int otherIdx = !checked ? 1 : 0;

    return _imp->common->iconFilePath[otherIdx];
}

bool
KnobHelper::hasAnimation() const
{
    std::list<ViewIdx> views = getViewsList();
    for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
        for (int i = 0; i < _imp->common->dimension; ++i) {
            KnobDimViewBasePtr value = getDataForDimView(DimIdx(i), *it);
            assert(value);
            {
                QMutexLocker k(&value->valueMutex);
                if (value->animationCurve && value->animationCurve->getKeyFramesCount() > 0) {
                    return true;
                }
            }
            if (!getExpression(DimIdx(i), *it).empty()) {
                return true;
            }
        }
    }

    return false;
}
KnobHolderPtr
KnobHelper::getHolder() const
{
    return _imp->holder.lock();
}

void
KnobHelper::setAnimationEnabled(bool val)
{
    if ( !canAnimate() ) {
        return;
    }
    KnobHolderPtr holder = getHolder();
    if (holder && !holder->canKnobsAnimate() ) {
        return;
    }
    _imp->common->isAnimationEnabled = val;
}

bool
KnobHelper::isAnimationEnabled() const
{
    return canAnimate() && _imp->common->isAnimationEnabled;
}

void
KnobHelper::setName(const std::string & name,
                    bool throwExceptions)
{
    if (name == _imp->common->name) {
        return;
    }
    _imp->common->originalName = name;
    _imp->common->name = NATRON_PYTHON_NAMESPACE::makeNameScriptFriendly(name);
    KnobHolderPtr holder = getHolder();

    if (!holder) {
        return;
    }
    //Try to find a duplicate
    int no = 1;
    bool foundItem;
    std::string finalName;
    do {
        std::stringstream ss;
        ss << _imp->common->name;
        if (no > 1) {
            ss << no;
        }
        finalName = ss.str();
        if ( holder->getKnobByName(finalName) ) {
            foundItem = true;
        } else {
            foundItem = false;
        }
        ++no;
    } while (foundItem);


    EffectInstancePtr effect = toEffectInstance(holder);
    if (effect) {
        NodePtr node = effect->getNode();
        std::string effectScriptName = node->getScriptName_mt_safe();
        if ( !effectScriptName.empty() ) {
            std::string newPotentialQualifiedName = node->getApp()->getAppIDString() +  node->getFullyQualifiedName();
            newPotentialQualifiedName += '.';
            newPotentialQualifiedName += finalName;

            bool isAttrDefined = false;
            PyObject* obj = NATRON_PYTHON_NAMESPACE::getAttrRecursive(newPotentialQualifiedName, appPTR->getMainModule(), &isAttrDefined);
            Q_UNUSED(obj);
            if (isAttrDefined) {
                QString message = tr("A Python attribute with the name %1 already exists.").arg(QString::fromUtf8(newPotentialQualifiedName.c_str()));
                if (throwExceptions) {
                    throw std::runtime_error( message.toStdString() );
                } else {
                    appPTR->writeToErrorLog_mt_safe(QString::fromUtf8(getName().c_str()), QDateTime::currentDateTime(), message );
                    std::cerr << message.toStdString() << std::endl;

                    return;
                }
            }
        }
    }
    _imp->common->name = finalName;
} // KnobHelper::setName

void
KnobHelper::setActualCloneForHolder(const KnobHolderPtr& holder)
{
    // Register the knob in the render clones map
    {
        QMutexLocker locker(&_imp->common->renderClonesMapMutex);
        _imp->common->renderClonesMap[holder] = shared_from_this();
    }

}

KnobIPtr
KnobHelper::getMainInstance() const
{
    return _imp->mainInstance.lock();
}

KnobIPtr
KnobHelper::getCloneForHolderInternal(const KnobHolderPtr& holder) const
{
    KnobHolderPtr thisHolder = getHolder();
    if (!thisHolder || thisHolder == holder) {
        return boost::const_pointer_cast<KnobI>(shared_from_this());
    }
    QMutexLocker k(&_imp->common->renderClonesMapMutex);
    std::map<KnobHolderWPtr, KnobIWPtr>::const_iterator found = _imp->common->renderClonesMap.find(holder);
    if (found != _imp->common->renderClonesMap.end()) {
        KnobIPtr ret = found->second.lock();
        assert(ret);
        return ret;
    }
    return KnobIPtr();
}

const std::string &
KnobHelper::getName() const
{
    return _imp->common->name;
}

const std::string &
KnobHelper::getOriginalName() const
{
    return _imp->common->originalName;
}

void
KnobHelper::resetParent()
{
    KnobIPtr parent = _imp->common->parentKnob.lock();

    if (parent) {
        KnobGroupPtr isGrp =  toKnobGroup(parent);
        KnobPagePtr isPage = toKnobPage(parent);
        if (isGrp) {
            isGrp->removeKnob( shared_from_this() );
        } else if (isPage) {
            isPage->removeKnob( shared_from_this() );
        } else {
            assert(false);
        }
        _imp->common->parentKnob.reset();
    }
}

void
KnobHelper::setParentKnob(KnobIPtr knob)
{
    _imp->common->parentKnob = knob;
}

KnobIPtr
KnobHelper::getParentKnob() const
{
    return _imp->common->parentKnob.lock();
}

bool
KnobHelper::getIsSecret() const
{
    QMutexLocker k(&_imp->common->stateMutex);

    return _imp->common->IsSecret;
}

bool
KnobHelper::getIsSecretRecursive() const
{
    if ( getIsSecret() ) {
        return true;
    }
    KnobIPtr parent = getParentKnob();
    if (parent) {
        return parent->getIsSecretRecursive();
    }

    return false;
}

void
KnobHelper::setIsFrozen(bool frozen)
{
    if (_signalSlotHandler) {
        _signalSlotHandler->s_setFrozen(frozen);
    }
}

bool
KnobHelper::isEnabled() const
{
    QMutexLocker k(&_imp->common->stateMutex);
    return _imp->common->enabled;
} // isEnabled



void
KnobHelper::setKnobSelectedMultipleTimes(bool d)
{
    if (_signalSlotHandler) {
        _signalSlotHandler->s_selectedMultipleTimes(d);
    }
}

void
KnobHelper::setEvaluateOnChange(bool b)
{
    KnobPage* isPage = dynamic_cast<KnobPage*>(this);
    KnobGroup* isGrp = dynamic_cast<KnobGroup*>(this);

    if (isPage || isGrp) {
        b = false;
    }
    {
        QMutexLocker k(&_imp->common->stateMutex);
        _imp->common->evaluateOnChange = b;
    }
    if (_signalSlotHandler) {
        _signalSlotHandler->s_evaluateOnChangeChanged(b);
    }
}

bool
KnobHelper::getIsPersistent() const
{
    return _imp->common->IsPersistent;
}

void
KnobHelper::setIsPersistent(bool b)
{
    _imp->common->IsPersistent = b;
}

void
KnobHelper::setCanUndo(bool val)
{
    _imp->common->CanUndo = val;
}

bool
KnobHelper::getCanUndo() const
{
    return _imp->common->CanUndo;
}

void
KnobHelper::setIsMetadataSlave(bool slave)
{
    _imp->common->isMetadataSlave = slave;
}

bool
KnobHelper::getIsMetadataSlave() const
{
    return _imp->common->isMetadataSlave;
}

bool
KnobHelper::getEvaluateOnChange() const
{
    QMutexLocker k(&_imp->common->stateMutex);

    return _imp->common->evaluateOnChange;
}

void
KnobHelper::setHintToolTip(const std::string & hint)
{
    _imp->common->tooltipHint = hint;
    if (_signalSlotHandler) {
        _signalSlotHandler->s_helpChanged();
    }
}

const std::string &
KnobHelper::getHintToolTip() const
{
    return _imp->common->tooltipHint;
}

bool
KnobHelper::isHintInMarkdown() const
{
    return _imp->common->hintIsMarkdown;
}

void
KnobHelper::setHintIsMarkdown(bool b)
{
    _imp->common->hintIsMarkdown = b;
}

void
KnobHelper::setCustomInteract(const OverlayInteractBasePtr & interactDesc)
{
    assert( QThread::currentThread() == qApp->thread() );
    _imp->common->customInteract = interactDesc;
}

OverlayInteractBasePtr KnobHelper::getCustomInteract() const
{
    assert( QThread::currentThread() == qApp->thread() );

    return _imp->common->customInteract;
}

bool
KnobI::shouldDrawOverlayInteract() const
{
    // If there is one dimension disabled, don't draw it
    if (!isEnabled()) {
        return false;
    }

    // If this knob is secret, don't draw it
    if (getIsSecretRecursive()) {
        return false;
    }

    KnobPagePtr page = getTopLevelPage();
    if (!page) {
        return false;
    }
    // Only draw overlays for knobs in the current page
    return page->isEnabled();
}

bool
KnobHelper::copyKnob(const KnobIPtr& other,
                     ViewSetSpec view,
                     DimSpec dimension,
                     ViewSetSpec otherView,
                     DimSpec otherDimension,
                     const RangeD* range,
                     double offset)
{
    if (!other || (other.get() == this && dimension == otherDimension && view == otherView)) {
        // Cannot clone itself
        return false;
    }
    if ((!dimension.isAll() || !otherDimension.isAll()) && (dimension.isAll() || otherDimension.isAll())) {
        throw std::invalid_argument("KnobHelper::copyKnob: invalid dimension argument");
    }
    if ((!view.isAll() || !otherView.isAll()) && (!view.isViewIdx() || !otherView.isViewIdx())) {
        throw std::invalid_argument("KnobHelper::copyKnob: invalid view argument");
    }

    bool hasChanged = false;

    {
        ScopedChanges_RAII changes(this);

        hasChanged |= cloneValues(other, view, otherView, dimension, otherDimension, range, offset);
        hasChanged |= cloneExpressions(other, view, otherView, dimension, otherDimension);

        ViewIdx view_i;
        if (!view.isAll()) {
            view_i = checkIfViewExistsOrFallbackMainView(ViewIdx(view));
        }
        std::list<ViewIdx> views = getViewsList();
        for (std::list<ViewIdx>::const_iterator it = views.begin(); it!=views.end(); ++it) {
            if (!view.isAll() && *it != view_i) {
                continue;
            }
            autoAdjustFoldExpandDimensions(*it);
        }


        if (hasChanged) {
            TimeValue time = getHolder()->getTimelineCurrentTime();
            evaluateValueChange(dimension, time, view, eValueChangedReasonPluginEdited);
        }
    }

    return hasChanged;
} // copyKnob


bool
KnobHelper::linkToInternal(const KnobIPtr & otherKnob, DimIdx thisDimension, DimIdx otherDimension, ViewIdx thisView, ViewIdx otherView)
{

    assert(otherKnob);

    KnobHelperPtr otherIsHelper = toKnobHelper(otherKnob);
    assert(otherIsHelper);
    if (!otherIsHelper) {
        return false;
    }


    KnobDimViewBasePtr otherData = otherIsHelper->getDataForDimView(otherDimension, otherView);
    KnobDimViewBasePtr thisData = getDataForDimView(thisDimension, thisView);

    // A link is already established for the same data
    if (otherData == thisData) {
        return false;
    }

    KnobIPtr thisShared = shared_from_this();


    {
        QMutexLocker k(&_imp->common->perDimViewDataMutex);

        // Save the old data away
        RedirectionLink& redirection = _imp->common->perDimViewSavedData[thisDimension][thisView];

        // If the savedData pointer is already set this means this knob was already
        // redirected to another knob
        if (!redirection.savedData) {
            redirection.savedData = thisData;
        }

    }


    // Redirect each shared knob/dim/view to the other data
    KnobDimViewKeySet currentSharedKnobs;
    {
        QMutexLocker k2(&thisData->valueMutex);
        currentSharedKnobs = thisData->sharedKnobs;

        // Nobody is referencing this data anymore: clear the sharedKnobs set
        thisData->sharedKnobs.clear();
    }
    for (KnobDimViewKeySet::const_iterator it = currentSharedKnobs.begin(); it!= currentSharedKnobs.end(); ++it) {
        KnobHelperPtr sharedKnob = toKnobHelper(it->knob.lock());
        if (!sharedKnob) {
            continue;
        }
        {
            QMutexLocker k2(&sharedKnob->_imp->common->perDimViewDataMutex);
            KnobDimViewBasePtr& sharedKnobDimViewData = sharedKnob->_imp->common->perDimViewData[it->dimension][it->view];

            // the data was shared with this data
            assert(sharedKnobDimViewData == thisData);

            // redirect it
            sharedKnobDimViewData = otherData;
        }

        // insert this shared knob to the sharedKnobs set of the other data
        {
            QMutexLocker k2(&otherData->valueMutex);
            std::pair<KnobDimViewKeySet::iterator,bool> insertOk = otherData->sharedKnobs.insert(*it);
            assert(insertOk.second);
            (void)insertOk.second;
        }

    }

    // Notify links changed
    {
        KnobDimViewKeySet sharedKnobs;
        {
            QMutexLocker k2(&otherData->valueMutex);
            sharedKnobs = otherData->sharedKnobs;
        }
        for (KnobDimViewKeySet::const_iterator it = sharedKnobs.begin(); it!= sharedKnobs.end(); ++it) {
            KnobHelperPtr sharedKnob = toKnobHelper(it->knob.lock());
            if (!sharedKnob) {
                continue;
            }
            sharedKnob->_signalSlotHandler->s_curveAnimationChanged(thisView, thisDimension);
            sharedKnob->_signalSlotHandler->s_linkChanged();
            sharedKnob->onLinkChanged();

        }
    }


    return true;
} // linkToInternal



bool
KnobHelper::linkTo(const KnobIPtr & otherKnob, DimSpec thisDimension, DimSpec otherDimension, ViewSetSpec thisView, ViewSetSpec otherView)
{
    assert(!_imp->mainInstance.lock());
    if (_imp->mainInstance.lock()) {
        return false;
    }
    if (!otherKnob) {
        return false;
    }

    assert((thisDimension.isAll() && otherDimension.isAll()) || (!thisDimension.isAll() && !otherDimension.isAll()));
    assert((thisView.isAll() && otherView.isAll()) || (thisView.isViewIdx() && otherView.isViewIdx()));

    if ((!thisDimension.isAll() || !otherDimension.isAll()) && (thisDimension.isAll() || otherDimension.isAll())) {
        throw std::invalid_argument("KnobHelper::slaveTo: invalid dimension argument");
    }
    if ((!thisView.isAll() || !otherView.isAll()) && (!thisView.isViewIdx() || !otherView.isViewIdx())) {
        throw std::invalid_argument("KnobHelper::slaveTo: invalid view argument");
    }
    if (otherKnob.get() == this && (thisDimension == otherDimension || thisDimension.isAll() || otherDimension.isAll()) && (thisView == otherView || thisView.isAll() || otherView.isAll())) {
        return false;
    }
    {
        // A non-checkable button cannot link
        KnobButtonPtr isButton = toKnobButton(shared_from_this());
        if (isButton && !isButton->getIsCheckable()) {
            return false;
        }
    }

    bool ok = false;
    {
        ScopedChanges_RAII changes(this);
        std::list<ViewIdx> views = otherKnob->getViewsList();
        if (thisDimension.isAll()) {
            int dimMin = std::min(getNDimensions(), otherKnob->getNDimensions());
            for (int i = 0; i < dimMin; ++i) {
                if (thisView.isAll()) {
                    for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
                        ok |= linkToInternal(otherKnob, DimIdx(i), DimIdx(i), *it, *it);
                    }
                } else {
                    ok |= linkToInternal(otherKnob, DimIdx(i), DimIdx(i), ViewIdx(thisView.value()), ViewIdx(otherView.value()));
                }
            }
        } else {
            if ( ( thisDimension >= getNDimensions() ) || (thisDimension < 0) || (otherDimension >= otherKnob->getNDimensions()) || (otherDimension < 0)) {
                throw std::invalid_argument("KnobHelper::slaveTo(): Dimension out of range");
            }
            if (thisView.isAll()) {
                for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
                    ok |= linkToInternal(otherKnob, DimIdx(thisDimension), DimIdx(otherDimension), *it, *it);
                }
            } else {
                ok |= linkToInternal(otherKnob, DimIdx(thisDimension), DimIdx(otherDimension), ViewIdx(thisView.value()), ViewIdx(otherView.value()));
            }
        }

        TimeValue time = getHolder()->getTimelineCurrentTime();
        evaluateValueChange(thisDimension, time, thisView, eValueChangedReasonUserEdited);
    }
    return ok;
} // slaveTo


void
KnobHelper::unlinkInternal(DimIdx dimension, ViewIdx view, bool copyState)
{

    KnobIPtr thisKnob = shared_from_this();

    RedirectionLink redirectionLink;
    KnobDimViewKeySet currentSharedKnobs;

    {
        QMutexLocker k(&_imp->common->perDimViewDataMutex);
        PerViewSavedDataMap::iterator foundSavedData = _imp->common->perDimViewSavedData[dimension].find(view);

        // A knob may not have saved data if others are linked to it but it is not linked to anything
        if (foundSavedData != _imp->common->perDimViewSavedData[dimension].end()) {
            redirectionLink = foundSavedData->second;
            _imp->common->perDimViewSavedData[dimension].erase(foundSavedData);

            // If this knob is linked to others, its saved value should not be linked to anyone else.
            assert(!redirectionLink.savedData || redirectionLink.savedData->sharedKnobs.empty());
        }

        PerViewKnobDataMap::iterator currentData = _imp->common->perDimViewData[dimension].find(view);
        if (currentData == _imp->common->perDimViewData[dimension].end()) {
            // oops, no data for the view
            return;
        }

        // Remove this knob dim/view from the shared knobs set
        KnobDimViewKey thisKnobKey(thisKnob, dimension, view);
        {
            {
                QMutexLocker k2(&currentData->second->valueMutex);
                currentSharedKnobs = currentData->second->sharedKnobs;

                assert(!currentData->second->sharedKnobs.empty());
                KnobDimViewKeySet::iterator foundThisKnobDimView = currentData->second->sharedKnobs.find(thisKnobKey);
                assert(foundThisKnobDimView != currentData->second->sharedKnobs.end());
                if (foundThisKnobDimView != currentData->second->sharedKnobs.end()) {
                    currentData->second->sharedKnobs.erase(foundThisKnobDimView);
                }
            }

        }



        // If there is a savedData pointer, that means we were linked to another knob: this is easy just set back the pointer, unless
        // the user requested to copy state
        if (redirectionLink.savedData && !copyState) {
            assert(currentData->second != redirectionLink.savedData);
            currentData->second = redirectionLink.savedData;

            // Nobody should have been referencing the saved datas
            assert(redirectionLink.savedData->sharedKnobs.empty());

            // Add this knob in the sharedKnobs set
            currentData->second->sharedKnobs.insert(thisKnobKey);
        } else {

            // Make a copy of the current data so that they are
            // no longer shared with others.

            // We are unlinking other knobs: keyframes did not change
            KnobDimViewBasePtr dataCopy = createDimViewData();
            dataCopy->sharedKnobs.insert(thisKnobKey);

            KnobDimViewBase::CopyInArgs inArgs(*currentData->second);
            dataCopy->copy(inArgs, 0);

            currentData->second = dataCopy;
        }
    }

    // Refresh links on all shared knobs
    for (KnobDimViewKeySet::const_iterator it = currentSharedKnobs.begin(); it!=currentSharedKnobs.end(); ++it) {
        KnobHelperPtr sharedKnob = toKnobHelper(it->knob.lock());
        if (!sharedKnob) {
            continue;
        }
        // The keyframes might have changed, notify it
        sharedKnob->_signalSlotHandler->s_curveAnimationChanged(view, dimension);

        sharedKnob->_signalSlotHandler->s_linkChanged();
        sharedKnob->onLinkChanged();
    }



} // unlinkInternal


void
KnobHelper::unlink(DimSpec dimension, ViewSetSpec view, bool copyState)
{
    assert(!_imp->mainInstance.lock());
    if (_imp->mainInstance.lock()) {
        return;
    }
    {
        ScopedChanges_RAII changes(this);
        std::list<ViewIdx> views = getViewsList();
        if (dimension.isAll()) {
            for (int i = 0; i < _imp->common->dimension; ++i) {
                if (view.isAll()) {
                    for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
                        unlinkInternal(DimIdx(i), *it, copyState);
                    }
                } else {
                    ViewIdx view_i = checkIfViewExistsOrFallbackMainView(ViewIdx(view.value()));
                    unlinkInternal(DimIdx(i), view_i, copyState);
                }
            }
        } else {
            if ( ( dimension >= getNDimensions() ) || (dimension < 0) ) {
                throw std::invalid_argument("KnobHelper::unSlave(): Dimension out of range");
            }
            if (view.isAll()) {
                for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
                    unlinkInternal(DimIdx(dimension), *it, copyState);
                }
            } else {
                ViewIdx view_i = checkIfViewExistsOrFallbackMainView(ViewIdx(view.value()));
                unlinkInternal(DimIdx(dimension), view_i, copyState);
            }
        }
        TimeValue time = getHolder()->getTimelineCurrentTime();
        evaluateValueChange(dimension, time, view, eValueChangedReasonUserEdited);

    }
} // unlink


bool
KnobHelper::getSharingMaster(DimIdx dimension, ViewIdx view, KnobDimViewKey* linkData) const
{
    KnobDimViewBasePtr data = getDataForDimView(dimension, view);
    if (!data) {
        return false;
    }
    assert(!data->sharedKnobs.empty());
    if (data->sharedKnobs.size() == 1) {
        return false;
    }


    // Find the sharing master between the shared knobs.
    // Our heuristic is to return the knob which has a holder being a NodeGroup
    bool foundNodeGroup = false;
    for (KnobDimViewKeySet::const_iterator it = data->sharedKnobs.begin(); it != data->sharedKnobs.end(); ++it) {
        KnobIPtr knob = it->knob.lock();
        if (!knob) {
            continue;
        }
        KnobHolderPtr thisHolder = knob->getHolder();
        EffectInstancePtr thisHolderIsEffect = toEffectInstance(thisHolder);
        NodeGroupPtr thisHolderIsGroup = toNodeGroup(thisHolderIsEffect);
        if (thisHolderIsGroup) {
            *linkData = *it;
            foundNodeGroup = true;
            break;
        }
    }

    if (!foundNodeGroup) {
        // Resort to any of the shared knobs
        const KnobDimViewKey& owner = *data->sharedKnobs.begin();
        *linkData = owner;

    }

    return true;
}

void
KnobHelper::getSharedValues(DimIdx dimension, ViewIdx view, KnobDimViewKeySet* sharedKnobs) const
{
    KnobDimViewBasePtr data = getDataForDimView(dimension, view);
    if (!data) {
        return;
    }
    {
        QMutexLocker k(&data->valueMutex);
        assert(!data->sharedKnobs.empty());
        *sharedKnobs = data->sharedKnobs;
    }

    // Remove this knob from the shared knobs
    KnobIPtr thisKnob = boost::const_pointer_cast<KnobI>(shared_from_this());
    KnobDimViewKey thisKnobDimView(thisKnob, dimension, view);
    KnobDimViewKeySet::iterator foundThisDimView = sharedKnobs->find(thisKnobDimView);
    assert(foundThisDimView != sharedKnobs->end());
    if (foundThisDimView != sharedKnobs->end()) {
        sharedKnobs->erase(foundThisDimView);
    }
}

AnimationLevelEnum
KnobHelper::getAnimationLevel(DimIdx dimension, TimeValue time, ViewIdx view) const
{
    AnimationLevelEnum level = eAnimationLevelNone;

    std::string expr = getExpression(dimension, view);
    if (!expr.empty()) {
        level = eAnimationLevelExpression;
    } else {

        CurvePtr c;
        if (canAnimate() && isAnimationEnabled()) {
            c = getAnimationCurve(view, dimension);
        }

        if (!c || !c->isAnimated()) {
            level = eAnimationLevelNone;
        } else {
            KeyFrame kf;
            int nKeys = c->getNKeyFramesInRange(time, time + 1);
            if (nKeys > 0) {
                level = eAnimationLevelOnKeyframe;
            } else {
                level = eAnimationLevelInterpolatedValue;
            }
        }
    }
    return level;
}



bool
KnobHelper::cloneExpressionInternal(const KnobIPtr& other, ViewIdx view, ViewIdx otherView, DimIdx dimension, DimIdx otherDimension)
{
    std::string otherExpr = other->getExpression(otherDimension, otherView);
    bool otherHasRet = other->isExpressionUsingRetVariable(otherView, otherDimension);
    ExpressionLanguageEnum lang = other->getExpressionLanguage(otherView, otherDimension);
    std::string thisExpr = getExpression(dimension, view);
    ExpressionLanguageEnum thisLang = getExpressionLanguage(view, dimension);

    if (otherExpr != thisExpr || (lang != thisLang)) {
        try {
            setExpression(dimension, view, otherExpr, lang, otherHasRet, false);
        } catch (...) {
            // Ignore errors
        }
        return true;
    }
    return false;
}

bool
KnobHelper::cloneValueInternal(const KnobIPtr& other, ViewIdx view, ViewIdx otherView, DimIdx dimension, DimIdx otherDimension, const RangeD* range, double offset)
{
    KnobHelperPtr otherIsHelper = toKnobHelper(other);
    assert(otherIsHelper);

    KnobDimViewBasePtr thisData = getDataForDimView(dimension, view);
    KnobDimViewBasePtr otherData = otherIsHelper->getDataForDimView(otherDimension, otherView);
    if (!thisData || !otherData) {
        return false;
    }
    KnobDimViewBase::CopyInArgs inArgs(*otherData);
    inArgs.keysToCopyOffset = offset;
    inArgs.keysToCopyRange = range;
    return thisData->copy(inArgs, 0);
}

bool
KnobHelper::cloneValues(const KnobIPtr& other, ViewSetSpec view, ViewSetSpec otherView, DimSpec dimension, DimSpec otherDimension, const RangeD* range, double offset)
{
    if (!other) {
        return false;
    }
    assert((view.isAll() && otherView.isAll()) || (view.isViewIdx() && view.isViewIdx()));
    assert((dimension.isAll() && otherDimension.isAll()) || (!dimension.isAll() && !otherDimension.isAll()));

    std::list<ViewIdx> views = other->getViewsList();
    int dims = std::min( getNDimensions(), other->getNDimensions() );

    bool hasChanged = false;
    if (dimension.isAll()) {
        for (int i = 0; i < dims; ++i) {
            if (view.isAll()) {
                for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
                    hasChanged |= cloneValueInternal(other, *it, *it, DimIdx(i) , DimIdx(i), range, offset);
                }
            } else {
                hasChanged |= cloneValueInternal(other, ViewIdx(view), ViewIdx(otherView), DimIdx(i) , DimIdx(i), range, offset);
            }
        }
    } else {
        if (view.isAll()) {
            for (std::list<ViewIdx>::const_iterator it= views.begin(); it != views.end(); ++it) {
                hasChanged |= cloneValueInternal(other, *it, *it, DimIdx(dimension) , DimIdx(otherDimension), range, offset);
            }
        } else {
            hasChanged |= cloneValueInternal(other, ViewIdx(view), ViewIdx(otherView), DimIdx(dimension) , DimIdx(otherDimension), range, offset);
        }
    }
    return hasChanged;
}

bool
KnobHelper::cloneExpressions(const KnobIPtr& other, ViewSetSpec view, ViewSetSpec otherView, DimSpec dimension, DimSpec otherDimension)
{
    if (!other) {
        return false;
    }
    assert((view.isAll() && otherView.isAll()) || (view.isViewIdx() && view.isViewIdx()));
    assert((dimension.isAll() && otherDimension.isAll()) || (!dimension.isAll() && !otherDimension.isAll()));

    std::list<ViewIdx> views = other->getViewsList();
    int dims = std::min( getNDimensions(), other->getNDimensions() );

    bool hasChanged = false;
    if (dimension.isAll()) {
        for (int i = 0; i < dims; ++i) {
            if (view.isAll()) {
                for (std::list<ViewIdx>::const_iterator it= views.begin(); it != views.end(); ++it) {
                    hasChanged |= cloneExpressionInternal(other, *it, *it, DimIdx(i) , DimIdx(i));
                }
            } else {
                hasChanged |= cloneExpressionInternal(other, ViewIdx(view), ViewIdx(otherView), DimIdx(i) , DimIdx(i));
            }
        }
    } else {
        if (view.isAll()) {
            for (std::list<ViewIdx>::const_iterator it= views.begin(); it != views.end(); ++it) {
                hasChanged |= cloneExpressionInternal(other, *it, *it, DimIdx(dimension) , DimIdx(otherDimension));
            }
        } else {
            hasChanged |= cloneExpressionInternal(other, ViewIdx(view), ViewIdx(otherView), DimIdx(dimension) , DimIdx(otherDimension));
        }
    }

    return hasChanged;
}

bool
KnobHelper::invalidateHashCacheInternal(std::set<HashableObject*>* invalidatedObjects)
{
    return HashableObject::invalidateHashCacheInternal(invalidatedObjects);
}

//The knob in parameter will "listen" to this knob. Hence this knob is a dependency of the knob in parameter.
void
KnobHelper::addListener(const DimIdx listenerDimension,
                        const DimIdx listenedToDimension,
                        const ViewIdx listenerView,
                        const ViewIdx listenedToView,
                        const KnobIPtr& listener,
                        ExpressionLanguageEnum language)
{
    if (!listener || !listener->getHolder() || !getHolder()) {
        return;
    }
    if (listenerDimension < 0 || listenerDimension >= listener->getNDimensions() || listenedToDimension < 0 || listenedToDimension >= getNDimensions()) {
        throw std::invalid_argument("KnobHelper::addListener: dimension out of range");
    }

    KnobHelper* listenerIsHelper = dynamic_cast<KnobHelper*>( listener.get() );
    assert(listenerIsHelper);
    if (!listenerIsHelper) {
        return;
    }

    KnobIPtr thisShared = shared_from_this();


    // Add the listener to the list
    {
        QMutexLocker l(&_imp->common->expressionMutex);
        KnobDimViewKeySet& listenersSet = _imp->common->listeners[listenedToDimension][listenedToView];
        KnobDimViewKey d(listener, listenerDimension, listenerView);
        listenersSet.insert(d);
    }

    // The "listener" knob needs to be invalidated when this knob changes, hence register it as a listener
    // of the hash
    //addHashListener(listener);

    if (language == eExpressionLanguagePython) {
        // Add this knob as a dependency of the expression
        // For ExprTk this is already done in validateExprTkExpression
        QMutexLocker k(&listenerIsHelper->_imp->common->expressionMutex);
        KnobExprPtr& expr = listenerIsHelper->_imp->common->expressions[listenerDimension][listenerView];
        if (expr) {

        }
        KnobExprPython* isPythonExpr = dynamic_cast<KnobExprPython*>(expr.get());

        KnobDimViewKey d(thisShared, listenedToDimension, listenedToView);
        if (isPythonExpr) {
            isPythonExpr->dependencies.insert(d);
        }
    }

    _signalSlotHandler->s_linkChanged();
} // addListener


void
KnobHelper::getListeners(KnobDimViewKeySet& listeners, ListenersTypeFlags flags) const
{
    std::list<ViewIdx> views = getViewsList();
    int nDims = getNDimensions();
    for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
        for (int i = 0; i < nDims; ++i) {

            if ((flags & eListenersTypeExpression) || (flags & eListenersTypeAll)) {
                QMutexLocker l(&_imp->common->expressionMutex);
                const KnobDimViewKeySet& thisDimViewExpressionListeners = _imp->common->listeners[i][*it];
                listeners.insert(thisDimViewExpressionListeners.begin(), thisDimViewExpressionListeners.end());
            }

            if ((flags & eListenersTypeSharedValue) || (flags & eListenersTypeAll)) {
                KnobDimViewBasePtr data = getDataForDimView(DimIdx(i), *it);
                if (!data) {
                    continue;
                }
                KnobDimViewKeySet sharedKnobs;
                getSharedValues(DimIdx(i), *it, &sharedKnobs);
                listeners.insert(sharedKnobs.begin(), sharedKnobs.end());
            }
        }
    }

}

TimeValue
KnobHelper::getCurrentRenderTime() const
{
    KnobHolderPtr holder = getHolder();

    return holder && holder->getApp() ? holder->getCurrentRenderTime() : TimeValue(0);
}

ViewIdx
KnobHelper::getCurrentRenderView() const
{
    KnobHolderPtr holder = getHolder();

    return ( holder && holder->getApp() ) ? holder->getCurrentRenderView() : ViewIdx(0);
}


bool
KnobI::hasDefaultValueChanged() const
{
    for (int i = 0; i < getNDimensions(); ++i) {
        if (hasDefaultValueChanged(DimIdx(i))) {
            return true;
        }
    }
    return false;
}


double
KnobHelper::random(double min,
                   double max,
                   TimeValue time,
                   unsigned int seed) const
{
    randomSeed(time, seed);

    return random(min, max);
}

double
KnobHelper::random(double min,
                   double max) const
{
    QMutexLocker k(&_imp->common->lastRandomHashMutex);

    _imp->common->lastRandomHash = hashFunction(_imp->common->lastRandomHash);

    return ( (double)_imp->common->lastRandomHash / (double)0x100000000LL ) * (max - min)  + min;
}

int
KnobHelper::randomInt(int min,
                      int max,
                      TimeValue time,
                      unsigned int seed) const
{
    randomSeed(time, seed);

    return randomInt(min, max);
}

int
KnobHelper::randomInt(int min,
                      int max) const
{
    return (int)random( (double)min, (double)max );
}


void
KnobHelper::randomSeed(TimeValue time,
                       unsigned int seed) const
{
    // Make the hash vary from seed
    U32 hash32 = seed;

    // Make the hash vary from time
    {
        alias_cast_float ac;
        ac.data = (float)time;
        hash32 += ac.raw;
    }

    QMutexLocker k(&_imp->common->lastRandomHashMutex);
    _imp->common->lastRandomHash = hash32;
}

bool
KnobHelper::hasModifications() const
{
    QMutexLocker k(&_imp->common->hasModificationsMutex);

    for (int i = 0; i < _imp->common->dimension; ++i) {
        for (PerViewHasModificationMap::const_iterator it = _imp->common->hasModifications[i].begin(); it != _imp->common->hasModifications[i].end(); ++it) {
            if (it->second) {
                return true;
            }
        }
    }

    return false;
}


void
KnobHelper::refreshCurveMinMax(ViewSetSpec view, DimSpec dimension)
{
    int nDims = getNDimensions();
    if (view.isAll()) {
        std::list<ViewIdx> views = getViewsList();
        if (dimension.isAll()) {
            for (int i = 0;i < nDims; ++i) {
                for (std::list<ViewIdx>::iterator it = views.begin(); it != views.end(); ++it) {
                    refreshCurveMinMaxInternal(*it, DimIdx(i));
                }
            }
        } else {
            for (std::list<ViewIdx>::iterator it = views.begin(); it != views.end(); ++it) {
                refreshCurveMinMaxInternal(*it, DimIdx(dimension));
            }
        }
    } else {
        ViewIdx view_i = checkIfViewExistsOrFallbackMainView(ViewIdx(view));
        if (dimension.isAll()) {
            for (int i = 0;i < nDims; ++i) {
                refreshCurveMinMaxInternal(view_i, DimIdx(i));
            }
        } else {
            refreshCurveMinMaxInternal(view_i, DimIdx(dimension));
        }
    }

}

bool
KnobHelper::hasModifications(DimIdx dimension) const
{
    if ( (dimension < 0) || (dimension >= _imp->common->dimension) ) {
        throw std::invalid_argument("KnobHelper::hasModifications: Dimension out of range");
    }
    QMutexLocker k(&_imp->common->hasModificationsMutex);
    for (PerViewHasModificationMap::const_iterator it = _imp->common->hasModifications[dimension].begin(); it != _imp->common->hasModifications[dimension].end(); ++it) {
        if (it->second) {
            return true;
        }
    }
    return false;
}

bool
KnobHelper::setHasModifications(DimIdx dimension,
                                ViewIdx view,
                                bool value,
                                bool lock)
{
    if ( (dimension < 0) || (dimension >= _imp->common->dimension) ) {
        throw std::invalid_argument("KnobHelper::setHasModifications: Dimension out of range");
    }

    if (lock) {
        _imp->common->hasModificationsMutex.lock();
    } else {
        assert( !_imp->common->hasModificationsMutex.tryLock() );
    }

    bool ret = false;
    PerViewHasModificationMap::iterator foundView = _imp->common->hasModifications[dimension].find(view);
    if (foundView != _imp->common->hasModifications[dimension].end()) {
        ret = foundView->second != value;
        foundView->second = value;
    }

    if (lock) {
        _imp->common->hasModificationsMutex.unlock();
    }

    return ret;
}

void
KnobHolder::getUserPages(std::list<KnobPagePtr>& userPages) const {
    const KnobsVec& knobs = getKnobs();

    for (KnobsVec::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        if ( (*it)->getKnobDeclarationType() == KnobI::eKnobDeclarationTypeUser) {
            KnobPagePtr isPage = toKnobPage(*it);
            if (isPage) {
                userPages.push_back(isPage);
            }
        }
    }
}

KnobIPtr
KnobHelper::createDuplicateOnHolder(const KnobHolderPtr& otherHolder,
                                    const KnobPagePtr& page,
                                    const KnobGroupPtr& group,
                                    int indexInParent,
                                    KnobI::DuplicateKnobTypeEnum duplicateType,
                                    const std::string& newScriptName,
                                    const std::string& newLabel,
                                    const std::string& newToolTip,
                                    bool refreshParams,
                                    KnobI::KnobDeclarationTypeEnum declarationType)
{
    ///find-out to which node that master knob belongs to
    KnobHolderPtr holder = getHolder();
    if ( !holder || !holder->getApp() ) {
        return KnobIPtr();
    }

    EffectInstancePtr otherIsEffect = toEffectInstance(otherHolder);
    EffectInstancePtr isEffect = toEffectInstance(holder);
    KnobBool* isBool = dynamic_cast<KnobBool*>(this);
    KnobInt* isInt = dynamic_cast<KnobInt*>(this);
    KnobDouble* isDbl = dynamic_cast<KnobDouble*>(this);
    KnobChoice* isChoice = dynamic_cast<KnobChoice*>(this);
    KnobColor* isColor = dynamic_cast<KnobColor*>(this);
    KnobString* isString = dynamic_cast<KnobString*>(this);
    KnobFile* isFile = dynamic_cast<KnobFile*>(this);
    KnobPath* isPath = dynamic_cast<KnobPath*>(this);
    KnobGroup* isGrp = dynamic_cast<KnobGroup*>(this);
    KnobPage* isPage = dynamic_cast<KnobPage*>(this);
    KnobButton* isBtn = dynamic_cast<KnobButton*>(this);
    KnobParametric* isParametric = dynamic_cast<KnobParametric*>(this);
    KnobKeyFrameMarkers* isKeyFrameMarker = dynamic_cast<KnobKeyFrameMarkers*>(this);


    //Ensure the group user page is created
    KnobPagePtr destPage;

    if (page) {
        destPage = page;
    } else {
        if (otherIsEffect) {
            std::list<KnobPagePtr> userPages;
            otherIsEffect->getUserPages(userPages);
            if (userPages.empty()) {
                destPage = otherIsEffect->getOrCreateUserPageKnob();
            } else {
                destPage = userPages.front();
            }

        }
    }

    KnobIPtr output;
    if (isBool) {
        KnobBoolPtr newKnob = otherHolder->createBoolKnob(newScriptName, newLabel, declarationType);
        output = newKnob;
    } else if (isInt) {
        KnobIntPtr newKnob = otherHolder->createIntKnob(newScriptName, newLabel, getNDimensions(), declarationType);
        newKnob->setRangeAcrossDimensions( isInt->getMinimums(), isInt->getMaximums() );
        newKnob->setDisplayRangeAcrossDimensions( isInt->getDisplayMinimums(), isInt->getDisplayMaximums() );
        if ( isInt->isSliderDisabled() ) {
            newKnob->disableSlider();
        }
        output = newKnob;
    } else if (isDbl) {
        KnobDoublePtr newKnob = otherHolder->createDoubleKnob(newScriptName, newLabel, getNDimensions(), declarationType);
        newKnob->setSpatial( isDbl->getIsSpatial() );
        if ( isDbl->isRectangle() ) {
            newKnob->setAsRectangle();
        }
        for (int i = 0; i < getNDimensions(); ++i) {
            newKnob->setValueIsNormalized( DimIdx(i), isDbl->getValueIsNormalized(DimIdx(i)) );
        }
        if ( isDbl->isSliderDisabled() ) {
            newKnob->disableSlider();
        }
        newKnob->setRangeAcrossDimensions( isDbl->getMinimums(), isDbl->getMaximums() );
        newKnob->setDisplayRangeAcrossDimensions( isDbl->getDisplayMinimums(), isDbl->getDisplayMaximums() );
        output = newKnob;
    } else if (isChoice) {
        KnobChoicePtr newKnob = otherHolder->createChoiceKnob(newScriptName, newLabel, declarationType);
        if (duplicateType != eDuplicateKnobTypeAlias) {
            newKnob->populateChoices( isChoice->getEntries());
        }
        output = newKnob;
    } else if (isColor) {
        KnobColorPtr newKnob = otherHolder->createColorKnob(newScriptName, newLabel, getNDimensions(), declarationType);
        newKnob->setRangeAcrossDimensions( isColor->getMinimums(), isColor->getMaximums() );
        newKnob->setDisplayRangeAcrossDimensions( isColor->getDisplayMinimums(), isColor->getDisplayMaximums() );
        output = newKnob;
    } else if (isString) {
        KnobStringPtr newKnob = otherHolder->createStringKnob(newScriptName, newLabel, declarationType);
        if ( isString->isLabel() ) {
            newKnob->setAsLabel();
        }
        if ( isString->isCustomKnob() ) {
            newKnob->setAsCustom();
        }
        if ( isString->isMultiLine() ) {
            newKnob->setAsMultiLine();
        }
        if ( isString->usesRichText() ) {
            newKnob->setUsesRichText(true);
        }
        output = newKnob;
    } else if (isFile) {
        KnobFilePtr newKnob = otherHolder->createFileKnob(newScriptName, newLabel, declarationType);
        newKnob->setDialogType(isFile->getDialogType());
        newKnob->setDialogFilters(isFile->getDialogFilters());
        output = newKnob;
    } else if (isPath) {
        KnobPathPtr newKnob = otherHolder->createPathKnob(newScriptName, newLabel, declarationType);
        if ( isPath->isMultiPath() ) {
            newKnob->setMultiPath(true);
        }
        output = newKnob;
    } else if (isGrp) {
        KnobGroupPtr newKnob = otherHolder->createGroupKnob(newScriptName, newLabel, declarationType);
        if ( isGrp->isTab() ) {
            newKnob->setAsTab();
        }
        output = newKnob;
    } else if (isPage) {
        KnobPagePtr newKnob = otherHolder->createPageKnob(newScriptName, newLabel, declarationType);
        output = newKnob;
    } else if (isBtn) {
        KnobButtonPtr newKnob = otherHolder->createButtonKnob(newScriptName, newLabel, declarationType);
        KnobButton* thisKnobButton = dynamic_cast<KnobButton*>(this);
        newKnob->setCheckable(thisKnobButton->getIsCheckable());
        output = newKnob;
    } else if (isParametric) {
        KnobParametricPtr newKnob = otherHolder->createParametricKnob(newScriptName, newLabel, isParametric->getNDimensions(), declarationType);
        output = newKnob;
        newKnob->setRangeAcrossDimensions( isParametric->getMinimums(), isParametric->getMaximums() );
        newKnob->setDisplayRangeAcrossDimensions( isParametric->getDisplayMinimums(), isParametric->getDisplayMaximums() );
    } else if (isKeyFrameMarker) {
        KnobKeyFrameMarkersPtr param = otherHolder->createKnob<KnobKeyFrameMarkers>(newScriptName, isKeyFrameMarker->getNDimensions());
        param->setLabel(newLabel);
        output = param;
    }
    if (!output) {
        return KnobIPtr();
    }
    for (int i = 0; i < getNDimensions(); ++i) {
        output->setDimensionName( DimIdx(i), getDimensionName(DimIdx(i)) );
    }

    KnobIPtr thisShared = shared_from_this();
    output->setName(newScriptName, true);
    output->cloneDefaultValues( thisShared );
    output->copyKnob( thisShared );
    if ( canAnimate() ) {
        output->setAnimationEnabled( isAnimationEnabled() );
    }
    output->setIconLabel(getIconLabel(false), false);
    output->setIconLabel(getIconLabel(true), true);
    output->setEvaluateOnChange( getEvaluateOnChange() );
    output->setHintToolTip(newToolTip);
    output->setAddNewLine(true);
    output->setHashingStrategy(getHashingStrategy());
    if (group) {
        if (indexInParent == -1) {
            group->addKnob(output);
        } else {
            group->insertKnob(indexInParent, output);
        }
    } else if (destPage) {
        if (indexInParent == -1) {
            destPage->addKnob(output);
        } else {
            destPage->insertKnob(indexInParent, output);
        }
    }
    if (declarationType == KnobI::eKnobDeclarationTypeUser && otherIsEffect) {
        otherIsEffect->getNode()->declarePythonKnobs();
    }
    switch (duplicateType) {
        case KnobI::eDuplicateKnobTypeAlias: {
            bool ok = linkTo(output);
            assert(ok);
            (void)ok;
        }   break;
        case KnobI::eDuplicateKnobTypeExprLinked: {
            if (otherIsEffect && isEffect) {
                NodeCollectionPtr collec;
                collec = isEffect->getNode()->getGroup();

                NodeGroupPtr isCollecGroup = toNodeGroup(collec);
                std::stringstream ss;
                if (isCollecGroup) {
                    ss << "thisGroup." << newScriptName;
                } else {
                    ss << "app." << otherIsEffect->getNode()->getFullyQualifiedName() << "." << newScriptName;
                }
                if (output->getNDimensions() > 1) {
                    ss << ".get()[dimension]";
                } else {
                    ss << ".get()";
                }

                try {
                    std::string script = ss.str();
                    clearExpression(DimSpec::all(), ViewSetSpec::all());
                    setExpression(DimSpec::all(), ViewSetSpec::all(), script, eExpressionLanguagePython, false /*hasRetVariable*/, false /*failIfInvalid*/);
                } catch (...) {
                }
            }
        }   break;
        case KnobI::eDuplicateKnobTypeCopy:
            break;
    }
    if (refreshParams) {
        otherHolder->recreateUserKnobs(true);
    }

    return output;
} // KnobHelper::createDuplicateOnNode


static void
initializeDefaultValueSerializationStorage(const KnobIPtr& knob,
                                           const DimIdx dimension,
                                           KnobSerialization* knobSer,
                                           DefaultValueSerialization* defValue)
{
    // Serialize value and default value
    KnobBoolBasePtr isBoolBase = toKnobBoolBase(knob);
    KnobIntPtr isInt = toKnobInt(knob);
    KnobBoolPtr isBool = toKnobBool(knob);
    KnobButtonPtr isButton = toKnobButton(knob);
    KnobDoubleBasePtr isDoubleBase = toKnobDoubleBase(knob);
    KnobDoublePtr isDouble = toKnobDouble(knob);
    KnobColorPtr isColor = toKnobColor(knob);
    KnobChoicePtr isChoice = toKnobChoice(knob);
    KnobStringBasePtr isStringBase = toKnobStringBase(knob);
    KnobParametricPtr isParametric = toKnobParametric(knob);
    KnobPagePtr isPage = toKnobPage(knob);
    KnobGroupPtr isGrp = toKnobGroup(knob);
    KnobSeparatorPtr isSep = toKnobSeparator(knob);
    KnobButtonPtr btn = toKnobButton(knob);
    KnobPathPtr isPath = toKnobPath(knob);
    KnobTablePtr isTable = toKnobTable(knob);

    // Only serialize default value for the main view
    if (isInt) {

        knobSer->_dataType = eSerializationValueVariantTypeInteger;
        defValue->value.isInt = isInt->getDefaultValue(dimension);
        defValue->serializeDefaultValue = isInt->hasDefaultValueChanged(dimension);

    } else if (isBool || isGrp || isButton) {
        knobSer->_dataType = eSerializationValueVariantTypeBoolean;
        defValue->value.isBool = isBoolBase->getDefaultValue(dimension);
        defValue->serializeDefaultValue = isBoolBase->hasDefaultValueChanged(dimension);
    } else if (isColor || isDouble) {

        knobSer->_dataType = eSerializationValueVariantTypeDouble;
        defValue->value.isDouble = isDoubleBase->getDefaultValue(dimension);
        defValue->serializeDefaultValue = isDoubleBase->hasDefaultValueChanged(dimension);

    } else if ((isPath && isPath->isMultiPath()) || (!isPath && isTable)) {
        knobSer->_dataType = eSerializationValueVariantTypeTable;

        isTable->decodeFromKnobTableFormat(isTable->getDefaultValue(dimension), &defValue->value.isTable);
        defValue->serializeDefaultValue = isTable->hasDefaultValueChanged(dimension);
    } else if (isStringBase) {

        knobSer->_dataType = eSerializationValueVariantTypeString;
        defValue->value.isString = isStringBase->getDefaultValue(dimension);
        defValue->serializeDefaultValue = isStringBase->hasDefaultValueChanged(dimension);
    } else if (isChoice) {
        if (isChoice->isMultiChoiceEnabled()) {
            // This is a string list
            knobSer->_dataType = eSerializationValueVariantTypeTable;
            std::vector<std::string> defaultIds = isChoice->getDefaultEntriesID_multi();

            // Make a list of vectors, because the table format expect for each row a vector of columns.
            // Since this is a string list, there's a single column.
            for (std::size_t i = 0; i < defaultIds.size(); ++i) {
                std::vector<std::string> vec1(1);
                vec1[0] = defaultIds[i];
                defValue->value.isTable.push_back(vec1);
            }
        } else {
            knobSer->_dataType = eSerializationValueVariantTypeString;
            defValue->value.isString = isChoice->getDefaultEntryID();
        }
        defValue->serializeDefaultValue = isChoice->hasDefaultValueChanged(dimension);

    } else {
        knobSer->_dataType = eSerializationValueVariantTypeNone;
    }
} // initializeDefaultValueSerializationStorage


static bool serializeHardLinks(const KnobIPtr& knob,
                               const std::vector<std::string>& viewNames,
                               const DimIdx dimension,
                               const ViewIdx view,
                               ValueSerialization* serialization)
{

    KnobIPtr masterKnob;
    KnobDimViewKey sharedMaster;
    if (knob->getSharingMaster(dimension, view, &sharedMaster)) {
        masterKnob = sharedMaster.knob.lock();
    }

    // Only serialize master link if:
    // - it exists and
    // - the knob wants the slave/master link to be persistent and
    // - the effect is not a clone of another one OR the master knob is an alias of this one
    if (!masterKnob) {
        return false;
    }
    if (masterKnob->getNDimensions() > 1) {
        serialization->_slaveMasterLink.masterDimensionName = masterKnob->getDimensionName(sharedMaster.dimension);
    }

    if (masterKnob == knob) {
        if (sharedMaster.dimension != dimension || sharedMaster.view != view) {
            return true;
        }
        return false;
    }


    // Search for the master knob holder, it might either be an effect or a table item
    NamedKnobHolderPtr masterHolder = boost::dynamic_pointer_cast<NamedKnobHolder>( masterKnob->getHolder() );
    KnobTableItemPtr masterIsTableItem = toKnobTableItem(masterKnob->getHolder());
    EffectInstancePtr masterIsEffect = toEffectInstance(masterKnob->getHolder());
    NodeGroupPtr masterIsGroup = toNodeGroup(masterIsEffect);
    EffectInstancePtr thisHolderIsEffect = toEffectInstance(knob->getHolder());
    NodeGroupPtr thisHolderIsGroup = toNodeGroup(thisHolderIsEffect);

    if (masterIsTableItem) {
        // The master knob is held by a table item
        serialization->_slaveMasterLink.masterTableName = masterIsTableItem->getModel()->getTableIdentifier();
        serialization->_slaveMasterLink.masterTableItemName = masterIsTableItem->getFullyQualifiedName();

        // If the node owning the table item is different than this holder, save the master node name
        if (masterIsTableItem->getModel()->getNode()->getEffectInstance() != knob->getHolder()) {
            serialization->_slaveMasterLink.masterNodeName = masterIsTableItem->getModel()->getNode()->getScriptName_mt_safe();
        }
    } else if (masterIsEffect) {
        // coverity[dead_error_line]
        if (!masterIsEffect->getNode()->isPersistent()) {
            // the knob is linked to a knob of a non persistent node, do not save it
            return false;
        }
        if (masterIsGroup && !masterIsGroup->isSubGraphEditedByUser() && thisHolderIsEffect->getNode()->getGroup() == masterIsGroup) {
            // the master effect is a PyPlug, do not save it
            return false;
        }
        if (thisHolderIsGroup && !thisHolderIsGroup->isSubGraphEditedByUser() && masterIsEffect->getNode()->getGroup() == thisHolderIsGroup) {
            // the master effect is a node within the PyPlug, do not save it
            return false;
        }

        // If the master knob is on  the group containing the node
        // then don't serialize the node name, instead serialize a generic @thisGroup tag so that we can deserialize this into any node
        if (masterIsGroup && masterIsGroup == masterHolder) {
            serialization->_slaveMasterLink.masterNodeName = kKnobMasterNodeIsGroup;
        } else if (masterIsEffect != knob->getHolder()) {
            serialization->_slaveMasterLink.masterNodeName = masterHolder->getScriptName_mt_safe();
        }

    }
    serialization->_slaveMasterLink.masterKnobName = masterKnob->getName();
    if (sharedMaster.view != ViewIdx(0) &&  sharedMaster.view < (int)viewNames.size()) {
        serialization->_slaveMasterLink.masterViewName = viewNames[sharedMaster.view];
    }
    return true;

} // serializeHardLinks

static void
initializeValueSerializationStorage(const KnobIPtr& knob,
                                    const std::vector<std::string>& viewNames,
                                    const DimIdx dimension,
                                    const ViewIdx view,
                                    const DefaultValueSerialization& defValue,
                                    ValueSerialization* serialization)
{


    bool gotValue = !serialization->_expression.empty();

    // Serialize value and default value
    KnobBoolBasePtr isBoolBase = toKnobBoolBase(knob);
    KnobIntPtr isInt = toKnobInt(knob);
    KnobBoolPtr isBool = toKnobBool(knob);
    KnobButtonPtr isButton = toKnobButton(knob);
    KnobDoubleBasePtr isDoubleBase = toKnobDoubleBase(knob);
    KnobDoublePtr isDouble = toKnobDouble(knob);
    KnobColorPtr isColor = toKnobColor(knob);
    KnobChoicePtr isChoice = toKnobChoice(knob);
    KnobStringBasePtr isStringBase = toKnobStringBase(knob);
    KnobFilePtr isFile = toKnobFile(knob);
    KnobParametricPtr isParametric = toKnobParametric(knob);
    KnobPagePtr isPage = toKnobPage(knob);
    KnobGroupPtr isGrp = toKnobGroup(knob);
    KnobSeparatorPtr isSep = toKnobSeparator(knob);
    KnobButtonPtr btn = toKnobButton(knob);
    KnobPathPtr isPath = toKnobPath(knob);
    KnobTablePtr isTable = toKnobTable(knob);

    serialization->_serializeValue = false;

    if (isParametric) {
        return;
    }
    // Serialize slave/master link
    if (!gotValue) {
        if (serializeHardLinks(knob, viewNames, dimension, view, serialization)) {
            serialization->_slaveMasterLink.hasLink = true;
            gotValue = true;
        }
    } // !gotValue


    serialization->_expression = knob->getExpression(dimension, view);
    serialization->_expresionHasReturnVariable = knob->isExpressionUsingRetVariable(view, dimension);
    ExpressionLanguageEnum lang = knob->getExpressionLanguage(view, dimension);
    switch (lang) {
        case eExpressionLanguageExprTk:
            serialization->_expressionLanguage = kKnobSerializationExpressionLanguageExprtk;
            break;
        case eExpressionLanguagePython:
            serialization->_expressionLanguage = kKnobSerializationExpressionLanguagePython;
            break;
    }

    // Serialize curve
    CurvePtr curve = knob->getAnimationCurve(view, dimension);
    if (curve && !gotValue) {
        curve->toSerialization(&serialization->_animationCurve);
        if (!serialization->_animationCurve.keys.empty()) {
            gotValue = true;
        }
    }


    if (!gotValue) {

        if (isInt) {
            serialization->_value.isInt = isInt->getValue(dimension, view);
            serialization->_serializeValue = (serialization->_value.isInt != defValue.value.isInt);
        } else if (isBool || isGrp || isButton) {
            serialization->_value.isBool = isBoolBase->getValue(dimension, view);
            serialization->_serializeValue = (serialization->_value.isBool != defValue.value.isBool);
        } else if (isColor || isDouble) {
            serialization->_value.isDouble = isDoubleBase->getValue(dimension, view);
            serialization->_serializeValue = (serialization->_value.isDouble != defValue.value.isDouble);
        } else if ((isPath && isPath->isMultiPath()) || (!isPath && isTable)) {
            isTable->getTable(&serialization->_value.isTable);
            serialization->_serializeValue = (serialization->_value.isTable != defValue.value.isTable);
        } else if (isStringBase) {
            if (isFile) {
                serialization->_value.isString = isFile->getFileNameWithoutVariablesExpension(dimension, view);
            } else {
                serialization->_value.isString = isStringBase->getValue(dimension, view);
            }
            serialization->_serializeValue = (serialization->_value.isString != defValue.value.isString);

        } else if (isChoice) {
            if (isChoice->isMultiChoiceEnabled()) {
                // Make a list of vectors, because the table format expect for each row a vector of columns.
                // Since this is a string list, there's a single column.
                std::vector<ChoiceOption> options = isChoice->getCurrentEntries_multi(view);
                for (std::size_t i = 0; i < options.size(); ++i) {
                    std::vector<std::string> vec1(1);
                    vec1[0] = options[i].id;
                    serialization->_value.isTable.push_back(vec1);
                }
                serialization->_serializeValue = isChoice->hasModifications(dimension);

            } else {
                serialization->_value.isString = isChoice->getCurrentEntry(view).id;
                serialization->_serializeValue = (serialization->_value.isString != defValue.value.isString);
            }
        }
    }
    // Check if we need to serialize this dimension
    serialization->_mustSerialize = true;

    if (serialization->_expression.empty() && !serialization->_slaveMasterLink.hasLink && serialization->_animationCurve.keys.empty()  && !serialization->_serializeValue && !defValue.serializeDefaultValue) {
        serialization->_mustSerialize = false;
    }

} // initializeValueSerializationStorage

void
KnobHelper::restoreDefaultValueFromSerialization(const SERIALIZATION_NAMESPACE::DefaultValueSerialization& defObj,
                                                 bool applyDefaultValue,
                                                 DimIdx targetDimension)
{
    KnobIPtr thisShared = shared_from_this();
    KnobBoolBasePtr isBoolBase = toKnobBoolBase(thisShared);
    KnobIntPtr isInt = toKnobInt(thisShared);
    KnobBoolPtr isBool = toKnobBool(thisShared);
    KnobButtonPtr isButton = toKnobButton(thisShared);
    KnobDoubleBasePtr isDoubleBase = toKnobDoubleBase(thisShared);
    KnobDoublePtr isDouble = toKnobDouble(thisShared);
    KnobColorPtr isColor = toKnobColor(thisShared);
    KnobChoicePtr isChoice = toKnobChoice(thisShared);
    KnobStringBasePtr isStringBase = toKnobStringBase(thisShared);
    KnobPagePtr isPage = toKnobPage(thisShared);
    KnobGroupPtr isGrp = toKnobGroup(thisShared);
    KnobSeparatorPtr isSep = toKnobSeparator(thisShared);
    KnobButtonPtr btn = toKnobButton(thisShared);
    KnobPathPtr isPath = toKnobPath(thisShared);
    KnobTablePtr isTable = toKnobTable(thisShared);


    if (isInt) {

        if (!applyDefaultValue) {
            isInt->setDefaultValueWithoutApplying(defObj.value.isInt, targetDimension);
        } else {
            isInt->setDefaultValue(defObj.value.isInt, targetDimension);
        }


    } else if (isBool || isGrp || isButton) {

        if (!applyDefaultValue) {
            isBoolBase->setDefaultValueWithoutApplying(defObj.value.isBool, targetDimension);
        } else {
            isBoolBase->setDefaultValue(defObj.value.isBool, targetDimension);
        }

    } else if (isColor || isDouble) {
        if (!applyDefaultValue) {
            isDoubleBase->setDefaultValueWithoutApplying(defObj.value.isDouble, targetDimension);
        } else {
            isDoubleBase->setDefaultValue(defObj.value.isDouble, targetDimension);
        }


    } else if ((isPath && isPath->isMultiPath()) || (!isPath && isTable)) {

        if (!defObj.value.isTable.empty()) {
            if ((int)defObj.value.isTable.begin()->size() != isTable->getColumnsCount()) {
                std::cerr << "Invalid number of columns when decoding " << isTable->getName() << std::endl;
            } else {
                if (!applyDefaultValue) {
                    std::string encoded = isTable->encodeToKnobTableFormat(defObj.value.isTable);
                    isTable->setDefaultValueWithoutApplying(encoded, targetDimension);
                } else {
                    std::string encoded = isTable->encodeToKnobTableFormat(defObj.value.isTable);
                    isTable->setDefaultValue(encoded, targetDimension);
                }
            }
        }
    } else if (isStringBase) {

        if (!applyDefaultValue) {
            isStringBase->setDefaultValueWithoutApplying(defObj.value.isString, targetDimension);
        } else {
            isStringBase->setDefaultValue(defObj.value.isString, targetDimension);
        }

    } else if (isChoice) {
        if (isChoice->isMultiChoiceEnabled()) {
            std::vector<std::string> options;

            for (std::list<std::vector<std::string> >::const_iterator it = defObj.value.isTable.begin(); it != defObj.value.isTable.end(); ++it) {
                if (it->size() != 1) {
                    continue;
                }
                const std::string& choiceID = (*it)[0];
                options.push_back(choiceID);
            }
            if (!applyDefaultValue) {
                isChoice->setDefaultValuesFromIDWithoutApplying_multi(options);
            } else {
                isChoice->setDefaultValuesFromID_multi(options);
            }

        } else {
            int foundDefault = KnobChoice::choiceMatch(defObj.value.isString, isChoice->getEntries(), 0);
            if (foundDefault != -1) {
                if (!applyDefaultValue) {
                    isChoice->setDefaultValueWithoutApplying(foundDefault, DimIdx(0));
                } else {
                    isChoice->setDefaultValue(foundDefault);
                }
            }
        }
    }


}

void
KnobHelper::restoreValueFromSerialization(const SERIALIZATION_NAMESPACE::ValueSerialization& obj,
                                          DimIdx targetDimension,
                                          ViewIdx view)
{

    KnobIPtr thisShared = shared_from_this();
    KnobBoolBasePtr isBoolBase = toKnobBoolBase(thisShared);
    KnobIntPtr isInt = toKnobInt(thisShared);
    KnobBoolPtr isBool = toKnobBool(thisShared);
    KnobButtonPtr isButton = toKnobButton(thisShared);
    KnobDoubleBasePtr isDoubleBase = toKnobDoubleBase(thisShared);
    KnobDoublePtr isDouble = toKnobDouble(thisShared);
    KnobColorPtr isColor = toKnobColor(thisShared);
    KnobStringBasePtr isStringBase = toKnobStringBase(thisShared);
    KnobPagePtr isPage = toKnobPage(thisShared);
    KnobGroupPtr isGrp = toKnobGroup(thisShared);
    KnobSeparatorPtr isSep = toKnobSeparator(thisShared);
    KnobButtonPtr btn = toKnobButton(thisShared);
    KnobPathPtr isPath = toKnobPath(thisShared);
    KnobTablePtr isTable = toKnobTable(thisShared);

    // We do the opposite of what is done in initializeValueSerializationStorage()
    if (isInt) {
        isInt->setValue(obj._value.isInt, view, targetDimension, eValueChangedReasonUserEdited, 0);
    } else if (isBool || isGrp || isButton) {
        assert(isBoolBase);
        isBoolBase->setValue(obj._value.isBool, view, targetDimension, eValueChangedReasonUserEdited, 0);
    } else if (isColor || isDouble) {
        assert(isDoubleBase);
        isDoubleBase->setValue(obj._value.isDouble, view, targetDimension, eValueChangedReasonUserEdited, 0);
    } else if ((isPath && isPath->isMultiPath()) || (!isPath && isTable)) {
        if (!obj._value.isTable.empty()) {
            if ((int)obj._value.isTable.begin()->size() != isTable->getColumnsCount()) {
                std::cerr << "Invalid number of columns when decoding " << isTable->getName() << std::endl;
            } else {
                std::string encoded = isTable->encodeToKnobTableFormat(obj._value.isTable);
                isTable->setValue(encoded, view, targetDimension, eValueChangedReasonUserEdited, 0);
            }
        }
    } else if (isStringBase) {
        isStringBase->setValue(obj._value.isString, view, targetDimension, eValueChangedReasonUserEdited, 0);
    }

} // restoreValueFromSerialization

void
KnobHelper::toSerialization(SerializationObjectBase* serializationBase)
{

    SERIALIZATION_NAMESPACE::KnobSerialization* serialization = dynamic_cast<SERIALIZATION_NAMESPACE::KnobSerialization*>(serializationBase);
    SERIALIZATION_NAMESPACE::GroupKnobSerialization* groupSerialization = dynamic_cast<SERIALIZATION_NAMESPACE::GroupKnobSerialization*>(serializationBase);
    assert(serialization || groupSerialization);
    if (!serialization && !groupSerialization) {
        return;
    }

    if (groupSerialization) {
        KnobGroup* isGrp = dynamic_cast<KnobGroup*>(this);
        KnobPage* isPage = dynamic_cast<KnobPage*>(this);

        assert(isGrp || isPage);

        groupSerialization->_typeName = typeName();
        groupSerialization->_name = getName();
        groupSerialization->_label = getLabel();
        groupSerialization->_secret = getIsSecret();

        if (isGrp) {
            groupSerialization->_isSetAsTab = isGrp->isTab();
            groupSerialization->_isOpened = isGrp->getValue();
        }

        KnobsVec children;

        if (isGrp) {
            children = isGrp->getChildren();
        } else if (isPage) {
            children = isPage->getChildren();
        }
        for (std::size_t i = 0; i < children.size(); ++i) {
            const KnobIPtr& child = children[i];
            if (isPage) {
                // If page, check that the child is a top level child and not child of a sub-group
                // otherwise let the sub group register the child
                KnobIPtr parent = child->getParentKnob();
                if (parent.get() != isPage) {
                    continue;
                }
            }
            KnobGroupPtr isGrp = toKnobGroup(child);
            if (isGrp) {
                boost::shared_ptr<GroupKnobSerialization> childSer = boost::make_shared<GroupKnobSerialization>();
                isGrp->toSerialization(childSer.get());
                groupSerialization->_children.push_back(childSer);
            } else {
                //KnobChoicePtr isChoice = toKnobChoice(children[i].get());
                //bool copyKnob = false;//isChoice != NULL;
                KnobSerializationPtr childSer = boost::make_shared<KnobSerialization>();

                // At this point we might be exporting an already existing PyPlug and knobs that were created
                // by the PyPlugs could be user knobs but were marked declared by plug-in. In order to force the
                // _isUserKnob flag on the serialization object, we set this bit to true.
                childSer->_forceUserKnob = true;
                child->toSerialization(childSer.get());
                assert(childSer->_isUserKnob);
                groupSerialization->_children.push_back(childSer);
            }
        }
    } else {

        KnobIPtr thisShared = shared_from_this();

        serialization->_typeName = typeName();
        serialization->_dimension = getNDimensions();
        serialization->_scriptName = getName();

        serialization->_isUserKnob = serialization->_forceUserKnob || getKnobDeclarationType() == eKnobDeclarationTypeUser;

        bool isFullRecoverySave = appPTR->getCurrentSettings()->getIsFullRecoverySaveModeEnabled();

        std::vector<std::string> viewNames;
        if (getHolder() && getHolder()->getApp()) {
            viewNames = getHolder()->getApp()->getProject()->getProjectViewNames();
        }

        // Serialize default values
        serialization->_defaultValues.resize(serialization->_dimension);
        for (int i = 0; i < serialization->_dimension; ++i) {
            initializeDefaultValueSerializationStorage(thisShared, DimIdx(i), serialization, &serialization->_defaultValues[i]);
        }

        // Values
        std::list<ViewIdx> viewsList = getViewsList();
        for (std::list<ViewIdx>::const_iterator it = viewsList.begin(); it!=viewsList.end(); ++it) {
            std::string view;
            if (*it >= 0 && *it < (int)viewNames.size()) {
                view = viewNames[*it];
            }
            KnobSerialization::PerDimensionValueSerializationVec& dimValues = serialization->_values[view];
            dimValues.resize(serialization->_dimension);

            for (std::size_t i = 0; i < dimValues.size(); ++i) {
                dimValues[i]._serialization = serialization;
                dimValues[i]._dimension = (int)i;
                initializeValueSerializationStorage(thisShared, viewNames, DimIdx(i), *it, serialization->_defaultValues[i], &dimValues[i]);

                // Force default value serialization in those cases
                if (serialization->_isUserKnob || isFullRecoverySave) {
                    serialization->_defaultValues[i].serializeDefaultValue = true;
                    dimValues[i]._mustSerialize = true;
                }
            } // for each dimension

            // If dimensions are equal do not serialize them all, just saved the first.
            // Note that the areDimensionsEqual() funtion will return true even if multiple dimensions are linked to different values.
            // E.g: imagine a Blur.size parameter linked to another Blur.size parameter, each dimension would be respectively linked to x and y
            // and links would be different, even though they appear equal on the interface we have to serialize the 2 different links
            bool allDimensionsEqual = areDimensionsEqual(*it);

            if (serialization->_dimension > 1) {
                bool linksEqual = true;
                for (std::size_t i = 1; i < dimValues.size(); ++i) {
                    if (dimValues[i]._slaveMasterLink.masterDimensionName != dimValues[0]._slaveMasterLink.masterDimensionName ||
                        dimValues[i]._slaveMasterLink.masterViewName != dimValues[0]._slaveMasterLink.masterViewName ||
                        dimValues[i]._slaveMasterLink.masterKnobName != dimValues[0]._slaveMasterLink.masterKnobName ||
                        dimValues[i]._slaveMasterLink.masterTableItemName != dimValues[0]._slaveMasterLink.masterTableItemName ||
                        dimValues[i]._slaveMasterLink.masterNodeName != dimValues[0]._slaveMasterLink.masterNodeName) {
                        linksEqual = false;
                        break;
                    }
                }
                if (!linksEqual) {
                    allDimensionsEqual = false;
                }
            }

            if (allDimensionsEqual) {
                dimValues.resize(1);
            }

        } // for each view

        // User knobs bits
        if (serialization->_isUserKnob) {
            serialization->_label = getLabel();
            serialization->_triggerNewLine = isNewLineActivated();
            serialization->_evaluatesOnChange = getEvaluateOnChange();
            serialization->_isPersistent = getIsPersistent();
            serialization->_animatesChanged = (isAnimationEnabled() != isAnimatedByDefault());
            serialization->_tooltip = getHintToolTip();
            serialization->_iconFilePath[0] = getIconLabel(false);
            serialization->_iconFilePath[1] = getIconLabel(true);

            serialization->_isSecret = getIsSecret();
            serialization->_disabled = !isEnabled();
        }


        // Viewer UI context bits
        if (getHolder()) {
            if (getHolder()->getInViewerContextKnobIndex(thisShared) != -1) {
                serialization->_hasViewerInterface = true;
                serialization->_inViewerContextItemSpacing = getInViewerContextItemSpacing();
                ViewerContextLayoutTypeEnum layout = getInViewerContextLayoutType();
                switch (layout) {
                    case eViewerContextLayoutTypeAddNewLine:
                        serialization->_inViewerContextItemLayout = kInViewerContextItemLayoutNewLine;
                        break;
                    case eViewerContextLayoutTypeSeparator:
                        serialization->_inViewerContextItemLayout = kInViewerContextItemLayoutAddSeparator;
                        break;
                    case eViewerContextLayoutTypeStretchAfter:
                        serialization->_inViewerContextItemLayout = kInViewerContextItemLayoutStretchAfter;
                        break;
                    case eViewerContextLayoutTypeSpacing:
                        serialization->_inViewerContextItemLayout.clear();
                        break;
                }
                serialization->_inViewerContextSecret = getInViewerContextSecret();
                if (serialization->_isUserKnob) {
                    serialization->_inViewerContextLabel = getInViewerContextLabel();
                    serialization->_inViewerContextIconFilePath[0] = getInViewerContextIconFilePath(false);
                    serialization->_inViewerContextIconFilePath[1] = getInViewerContextIconFilePath(true);

                }
            }
        }

        // Per-type specific data
        KnobChoice* isChoice = dynamic_cast<KnobChoice*>(this);
        if (isChoice) {
            ChoiceExtraData* extraData = new ChoiceExtraData;
            std::vector<ChoiceOption> options = isChoice->getEntries();
            std::vector<std::string> ids(options.size()), helps(options.size());
            for (std::size_t i = 0; i < options.size(); ++i) {
                ids[i] = options[i].id;
                helps[i] = options[i].tooltip;
            }
            extraData->_entries = ids;
            extraData->_helpStrings = helps;
            serialization->_extraData.reset(extraData);
        }
        KnobParametric* isParametric = dynamic_cast<KnobParametric*>(this);
        if (isParametric) {
            ParametricExtraData* extraData = new ParametricExtraData;
            isParametric->saveParametricCurves(&extraData->parametricCurves);
            serialization->_extraData.reset(extraData);
        }
        KnobString* isString = dynamic_cast<KnobString*>(this);
        if (isString) {
            TextExtraData* extraData = new TextExtraData;
            serialization->_extraData.reset(extraData);
            extraData->fontFamily = isString->getFontFamily();
            extraData->fontSize = isString->getFontSize();
            isString->getFontColor(&extraData->fontColor[0], &extraData->fontColor[1], &extraData->fontColor[2]);
            extraData->italicActivated = isString->getItalicActivated();
            extraData->boldActivated = isString->getBoldActivated();
        }
        if (serialization->_isUserKnob) {
            if (isString) {
                TextExtraData* extraData = dynamic_cast<TextExtraData*>(serialization->_extraData.get());
                assert(extraData);
                extraData->label = isString->isLabel();
                extraData->multiLine = isString->isMultiLine();
                extraData->richText = isString->usesRichText();
            }
            KnobDouble* isDbl = dynamic_cast<KnobDouble*>(this);
            KnobInt* isInt = dynamic_cast<KnobInt*>(this);
            KnobColor* isColor = dynamic_cast<KnobColor*>(this);
            if (isDbl || isInt || isColor) {
                ValueExtraData* extraData = new ValueExtraData;
                if (isDbl) {
                    extraData->min = isDbl->getMinimum();
                    extraData->max = isDbl->getMaximum();
                    extraData->dmin = isDbl->getDisplayMinimum();
                    extraData->dmax = isDbl->getDisplayMaximum();
                } else if (isInt) {
                    extraData->min = isInt->getMinimum();
                    extraData->max = isInt->getMaximum();
                    extraData->dmin = isInt->getDisplayMinimum();
                    extraData->dmax = isInt->getDisplayMaximum();
                } else if (isColor) {
                    extraData->min = isColor->getMinimum();
                    extraData->max = isColor->getMaximum();
                    extraData->dmin = isColor->getDisplayMinimum();
                    extraData->dmax = isColor->getDisplayMaximum();
                }
                serialization->_extraData.reset(extraData);
            }

            KnobFile* isFile = dynamic_cast<KnobFile*>(this);
            if (isFile) {
                FileExtraData* extraData = new FileExtraData;
                extraData->useSequences = isFile->getDialogType() == KnobFile::eKnobFileDialogTypeOpenFileSequences ||
                isFile->getDialogType() == KnobFile::eKnobFileDialogTypeSaveFileSequences;
                serialization->_extraData.reset(extraData);
            }

            KnobPath* isPath = dynamic_cast<KnobPath*>(this);
            if (isPath) {
                PathExtraData* extraData = new PathExtraData;
                extraData->multiPath = isPath->isMultiPath();
                serialization->_extraData.reset(extraData);
            }
        }

        // Check if we need to serialize this knob
        // We always serialize user knobs and knobs with a viewer interface
        serialization->_mustSerialize = true;
        if (!serialization->_isUserKnob && !serialization->_hasViewerInterface) {
            bool mustSerialize = false;
            for (SERIALIZATION_NAMESPACE::KnobSerialization::PerViewValueSerializationMap::const_iterator it = serialization->_values.begin(); it!=serialization->_values.end(); ++it) {
                for (std::size_t i = 0; i < it->second.size(); ++i) {
                    mustSerialize |= it->second[i]._mustSerialize;
                }

            }

            if (!mustSerialize) {
                // Check if there are extra data
                {
                    const TextExtraData* data = dynamic_cast<const TextExtraData*>(serialization->_extraData.get());
                    if (data) {
                        if (data->fontFamily != NATRON_FONT || data->fontSize != KnobString::getDefaultFontPointSize() || data->fontColor[0] != 0 || data->fontColor[1] != 0 || data->fontColor[2] != 0) {
                            mustSerialize = true;
                        }
                    }
                }
                {
                    const ParametricExtraData* data = dynamic_cast<const ParametricExtraData*>(serialization->_extraData.get());
                    if (data) {
                        if (!data->parametricCurves.empty()) {
                            mustSerialize = true;
                        }
                    }
                }

            }
            serialization->_mustSerialize = mustSerialize;
        }
    } // groupSerialization
} // KnobHelper::toSerialization

void
KnobHelper::fromSerialization(const SerializationObjectBase& serializationBase)
{
    // We allow non persistent knobs to be loaded if we found a valid serialization for them
    const SERIALIZATION_NAMESPACE::KnobSerialization* serialization = dynamic_cast<const SERIALIZATION_NAMESPACE::KnobSerialization*>(&serializationBase);
    assert(serialization);
    if (!serialization) {
        return;
    }

    // Block any instance change action call when loading a knob
    blockValueChanges();

    {
        ScopedChanges_RAII changes(this);



        // Clear any existing animation
        removeAnimation(ViewSetSpec::all(), DimSpec::all(), eValueChangedReasonRestoreDefault);

        // Restore extra datas
        KnobFile* isInFile = dynamic_cast<KnobFile*>(this);
        KnobString* isString = dynamic_cast<KnobString*>(this);
        if (isString) {
            const TextExtraData* data = dynamic_cast<const TextExtraData*>(serialization->_extraData.get());
            if (data) {
                isString->setFontColor(data->fontColor[0], data->fontColor[1], data->fontColor[2]);
                isString->setFontFamily(data->fontFamily);
                isString->setFontSize(std::max(data->fontSize,1));
                isString->setItalicActivated(data->italicActivated);
                isString->setBoldActivated(data->boldActivated);
            }

        }

        // Load parametric parameter's curves
        KnobParametric* isParametric = dynamic_cast<KnobParametric*>(this);
        if (isParametric) {
            const ParametricExtraData* data = dynamic_cast<const ParametricExtraData*>(serialization->_extraData.get());
            if (data) {
                isParametric->loadParametricCurves(data->parametricCurves);
            }
        }


        // Restore user knobs bits
        if (serialization->_isUserKnob) {
            if (serialization->_isSecret) {
                setSecret(true);
            }
            // Restore enabled state
            if (serialization->_disabled) {
                setEnabled(false);
            }
            setIsPersistent(serialization->_isPersistent);
            if (serialization->_animatesChanged) {
                setAnimationEnabled(!isAnimatedByDefault());
            }
            setEvaluateOnChange(serialization->_evaluatesOnChange);
            setName(serialization->_scriptName);
            setHintToolTip(serialization->_tooltip);
            setAddNewLine(serialization->_triggerNewLine);
            setIconLabel(serialization->_iconFilePath[0], false);
            setIconLabel(serialization->_iconFilePath[1], true);

            KnobInt* isInt = dynamic_cast<KnobInt*>(this);
            KnobDouble* isDouble = dynamic_cast<KnobDouble*>(this);
            KnobColor* isColor = dynamic_cast<KnobColor*>(this);
            KnobChoice* isChoice = dynamic_cast<KnobChoice*>(this);
            KnobPath* isPath = dynamic_cast<KnobPath*>(this);

            int nDims = std::min( getNDimensions(), serialization->_dimension );

            if (isInt) {
                const ValueExtraData* data = dynamic_cast<const ValueExtraData*>(serialization->_extraData.get());
                assert(data);
                if (data) {
                    std::vector<int> minimums, maximums, dminimums, dmaximums;
                    for (int i = 0; i < nDims; ++i) {
                        if (data->min == INT_MIN || data->min == -std::numeric_limits<double>::infinity()) {
                            minimums.push_back(INT_MIN);
                        } else {
                            minimums.push_back(data->min);
                        }
                        if (data->dmin == INT_MIN || data->dmin == -std::numeric_limits<double>::infinity()) {
                            dminimums.push_back(INT_MIN);
                        } else {
                            dminimums.push_back(data->dmin);
                        }
                        if (data->max == INT_MAX || data->max == std::numeric_limits<double>::infinity()) {
                            maximums.push_back(INT_MAX);
                        } else {
                            maximums.push_back(data->max);
                        }
                        if (data->dmax == INT_MAX || data->dmax == std::numeric_limits<double>::infinity()) {
                            dmaximums.push_back(INT_MAX);
                        } else {
                            dmaximums.push_back(data->dmax);
                        }
                    }
                    isInt->setRangeAcrossDimensions(minimums, maximums);
                    isInt->setDisplayRangeAcrossDimensions(dminimums, dmaximums);
                }
            } else if (isDouble) {
                const ValueExtraData* data = dynamic_cast<const ValueExtraData*>(serialization->_extraData.get());
                assert(data);
                if (data) {
                    std::vector<double> minimums, maximums, dminimums, dmaximums;
                    for (int i = 0; i < nDims; ++i) {
                        if (data->min == INT_MIN || data->min == -std::numeric_limits<double>::infinity()) {
                            minimums.push_back(-std::numeric_limits<double>::infinity());
                        } else {
                            minimums.push_back(data->min);
                        }
                        if (data->dmin == INT_MIN || data->dmin == -std::numeric_limits<double>::infinity()) {
                            dminimums.push_back(-std::numeric_limits<double>::infinity());
                        } else {
                            dminimums.push_back(data->dmin);
                        }
                        if (data->max == INT_MAX || data->max == std::numeric_limits<double>::infinity()) {
                            maximums.push_back(std::numeric_limits<double>::infinity());
                        } else {
                            maximums.push_back(data->max);
                        }
                        if (data->dmax == INT_MAX || data->dmax == std::numeric_limits<double>::infinity()) {
                            dmaximums.push_back(std::numeric_limits<double>::infinity());
                        } else {
                            dmaximums.push_back(data->dmax);
                        }
                    }
                    isDouble->setRangeAcrossDimensions(minimums, maximums);
                    isDouble->setDisplayRangeAcrossDimensions(dminimums, dmaximums);
                }

            } else if (isChoice) {
                const ChoiceExtraData* data = dynamic_cast<const ChoiceExtraData*>(serialization->_extraData.get());
                if (data) {
                    std::vector<ChoiceOption> options(data->_entries.size());
                    for (std::size_t i = 0; i < data->_entries.size(); ++i) {
                        options[i].id = data->_entries[i];
                        if (i < data->_helpStrings.size()) {
                            options[i].tooltip = data->_helpStrings[i];
                        }
                    }
                    isChoice->populateChoices(options);
                }
            } else if (isColor) {
                const ValueExtraData* data = dynamic_cast<const ValueExtraData*>(serialization->_extraData.get());
                if (data) {
                    std::vector<double> minimums, maximums, dminimums, dmaximums;
                    for (int i = 0; i < nDims; ++i) {
                        if (data->min == INT_MIN || data->min == -std::numeric_limits<double>::infinity()) {
                            minimums.push_back(-std::numeric_limits<double>::infinity());
                        } else {
                            minimums.push_back(data->min);
                        }
                        if (data->dmin == INT_MIN || data->dmin == -std::numeric_limits<double>::infinity()) {
                            dminimums.push_back(-std::numeric_limits<double>::infinity());
                        } else {
                            dminimums.push_back(data->dmin);
                        }
                        if (data->max == INT_MAX || data->max == std::numeric_limits<double>::infinity()) {
                            maximums.push_back(std::numeric_limits<double>::infinity());
                        } else {
                            maximums.push_back(data->max);
                        }
                        if (data->dmax == INT_MAX || data->dmax == std::numeric_limits<double>::infinity()) {
                            dmaximums.push_back(std::numeric_limits<double>::infinity());
                        } else {
                            dmaximums.push_back(data->dmax);
                        }
                    }
                    isColor->setRangeAcrossDimensions(minimums, maximums);
                    isColor->setDisplayRangeAcrossDimensions(dminimums, dmaximums);
                }
            } else if (isString) {
                const TextExtraData* data = dynamic_cast<const TextExtraData*>(serialization->_extraData.get());
                if (data) {
                    if (data->label) {
                        isString->setAsLabel();
                    } else if (data->multiLine) {
                        isString->setAsMultiLine();
                        if (data->richText) {
                            isString->setUsesRichText(true);
                        }
                    }
                }

            } else if (isInFile) {
                const FileExtraData* data = dynamic_cast<const FileExtraData*>(serialization->_extraData.get());
                if (data) {
                    if (data->useExistingFiles) {
                        if (data->useSequences) {
                            isInFile->setDialogType(KnobFile::eKnobFileDialogTypeOpenFileSequences);
                        } else {
                            isInFile->setDialogType(KnobFile::eKnobFileDialogTypeOpenFile);
                        }
                    } else {
                        if (data->useSequences) {
                            isInFile->setDialogType(KnobFile::eKnobFileDialogTypeSaveFileSequences);
                        } else {
                            isInFile->setDialogType(KnobFile::eKnobFileDialogTypeSaveFile);
                        }
                    }
                    isInFile->setDialogFilters(data->filters);
                }
            } else if (isPath) {
                const PathExtraData* data = dynamic_cast<const PathExtraData*>(serialization->_extraData.get());
                if (data && data->multiPath) {
                    isPath->setMultiPath(true);
                }
            }

        } // isUserKnob

        std::vector<std::string> projectViews;
        if (getHolder() && getHolder()->getApp()) {
            projectViews = getHolder()->getApp()->getProject()->getProjectViewNames();
        }


        for (std::size_t i = 0; i < serialization->_defaultValues.size(); ++i) {
            if (serialization->_defaultValues[i].serializeDefaultValue) {
                restoreDefaultValueFromSerialization(serialization->_defaultValues[i], true /*applyDefault*/, DimIdx(i));
            }
        }


        // There is a case where the dimension of a parameter might have changed between versions, e.g:
        // the size parameter of the Blur node was previously a Double1D and has become a Double2D to control
        // both dimensions.
        // For compatibility, we do not load only the first dimension, otherwise the result wouldn't be the same,
        // instead we replicate the last dimension of the serialized knob to all other remaining dimensions to fit the
        // knob's dimensions.
        for (SERIALIZATION_NAMESPACE::KnobSerialization::PerViewValueSerializationMap::const_iterator it = serialization->_values.begin(); it!=serialization->_values.end(); ++it) {

            // Find the view index corresponding to the view name
            ViewIdx view_i(0);
            Project::getViewIndex(projectViews, it->first, &view_i);

            if (view_i != 0) {
                splitView(view_i);
            }

            for (int i = 0; i < _imp->common->dimension; ++i) {

                // Not all dimensions are necessarily saved since they may be folded.
                // In that case replicate the last dimension
                int d = i >= (int)it->second.size() ? it->second.size() - 1 : i;

                DimIdx dimensionIndex(i);

                // Clone animation
                if (!it->second[d]._animationCurve.keys.empty()) {
                    CurvePtr curve = getAnimationCurve(view_i, dimensionIndex);
                    if (curve) {
                        curve->fromSerialization(it->second[d]._animationCurve);
                        _signalSlotHandler->s_curveAnimationChanged(view_i, dimensionIndex);
                    }
                } else if (it->second[d]._expression.empty() && !it->second[d]._slaveMasterLink.hasLink) {
                    // restore value if no expression/link
                    restoreValueFromSerialization(it->second[d], dimensionIndex, view_i);
                }

            }
            autoAdjustFoldExpandDimensions(view_i);

        }

        // Restore viewer UI context
        if (serialization->_hasViewerInterface) {
            setInViewerContextItemSpacing(serialization->_inViewerContextItemSpacing);
            ViewerContextLayoutTypeEnum layoutType = eViewerContextLayoutTypeSpacing;
            if (serialization->_inViewerContextItemLayout == kInViewerContextItemLayoutNewLine) {
                layoutType = eViewerContextLayoutTypeAddNewLine;
            } else if (serialization->_inViewerContextItemLayout == kInViewerContextItemLayoutStretchAfter) {
                layoutType = eViewerContextLayoutTypeStretchAfter;
            } else if (serialization->_inViewerContextItemLayout == kInViewerContextItemLayoutAddSeparator) {
                layoutType = eViewerContextLayoutTypeSeparator;
            }
            setInViewerContextLayoutType(layoutType);
            setInViewerContextSecret(serialization->_inViewerContextSecret);
            if (getKnobDeclarationType() == eKnobDeclarationTypeUser) {
                setInViewerContextLabel(QString::fromUtf8(serialization->_inViewerContextLabel.c_str()));
                setInViewerContextIconFilePath(serialization->_inViewerContextIconFilePath[0], false);
                setInViewerContextIconFilePath(serialization->_inViewerContextIconFilePath[1], true);
            }
        }

        // Allow changes again
    } // changes
    unblockValueChanges();

    TimeValue time = getHolder()->getTimelineCurrentTime();
    evaluateValueChange(DimSpec::all(), time, ViewSetSpec::all(), eValueChangedReasonRestoreDefault);
} // KnobHelper::fromSerialization

// E.G: Imagine a nodegraph as such:
//  App:
//      Blur1:
//          size
//      Group1:
//          Blur2:
//              size
// to reference app.Blur1.size from app.Group1.Blur2.size you would use
// "@thisGroup.@thisGroup.Blur1" for the masterNodeName
static NodePtr findMasterNode(const NodeCollectionPtr& group,
                              int recursionLevel,
                              const std::string& masterNodeName,
                              const std::map<SERIALIZATION_NAMESPACE::NodeSerializationPtr, NodePtr>& allCreatedNodesInGroup)
{
    assert(group);

    // The masterNodeName can be something as @thisGroup.Blur1
    // We read everything until the dot (if any) and then recurse
    std::string token;
    std::size_t foundDot = masterNodeName.find_first_of(".");
    std::string remainingString;
    if (foundDot == std::string::npos) {
        token = masterNodeName;
    } else {
        token = masterNodeName.substr(0, foundDot);
        if (foundDot + 1 < masterNodeName.size()) {
            remainingString = masterNodeName.substr(foundDot + 1);
        }
    }

    if (token != kKnobMasterNodeIsGroup) {
        // Return the node-name in the group

        // The nodes created from the serialization may have changed name if another node with the same script-name already existed.
        // By chance since we created all nodes within the same Group at the same time, we have a list of the old node serialization
        // and the corresponding created node (with its new script-name).
        // If we find a match, make sure we use the new node script-name to restore the input.
        NodePtr foundNode = NodeGroup::findSerializedNodeWithScriptName(masterNodeName, allCreatedNodesInGroup, NodesList(), false);
        if (!foundNode) {
            // We did not find the node in the serialized nodes list, the last resort is to look into already created nodes
            // and find an exact match, hoping the script-name of the node did not change.
            foundNode = group->getNodeByName(masterNodeName);
        }

        if (remainingString.empty()) {
            return foundNode;
        } else {
            // There's stuff left to recurse, this node must be a group otherwise fail
            NodeGroupPtr nodeIsGroup = toNodeGroup(foundNode->getEffectInstance());
            if (!nodeIsGroup) {
                return NodePtr();
            }
            return findMasterNode(nodeIsGroup, recursionLevel + 1, masterNodeName, allCreatedNodesInGroup);
        }
    } else {
        // If there's nothing else to recurse on, the container must a be a Group node
        NodeGroupPtr isGroup = toNodeGroup(group);
        if (remainingString.empty()) {
            if (!isGroup) {
                return NodePtr();
            } else {
                return isGroup->getNode();
            }
        } else {
            // Otherwise recurse on the rest. On the first recursion since we already have a group
            // of the original node in parameter, call this function again with the same group
            // Otherwise, recurse up
            if (recursionLevel == 0) {
                return findMasterNode(group, recursionLevel + 1, masterNodeName, allCreatedNodesInGroup);
            } else {
                if (isGroup) {
                    return findMasterNode(isGroup->getNode()->getGroup(), recursionLevel + 1, masterNodeName, allCreatedNodesInGroup);
                } else {
                    return NodePtr();
                }
            }
        }
    }

} // findMasterNode

KnobIPtr
KnobHelper::findMasterKnob(const std::string& masterKnobName,
                           const std::string& masterNodeName,
                           const std::string& masterTableName,
                           const std::string& masterItemName,
                           const std::map<SERIALIZATION_NAMESPACE::NodeSerializationPtr, NodePtr>& allCreatedNodesInGroup)
{
    KnobTableItemPtr tableItem = toKnobTableItem(getHolder());
    EffectInstancePtr effect = toEffectInstance(getHolder());
    NodePtr thisKnobNode;
    if (tableItem) {
        thisKnobNode = tableItem->getModel()->getNode();
    } else if (effect) {
        thisKnobNode = effect->getNode();
    }
    // A knob that does not belong to a node cannot have links
    if (!thisKnobNode) {
        return KnobIPtr();
    }

    ///we need to cycle through all the nodes of the project to find the real master
    NodePtr masterNode;
    if (masterNodeName.empty()) {
        masterNode = thisKnobNode;
    } else {
        masterNode = findMasterNode(thisKnobNode->getGroup(), 0, masterNodeName, allCreatedNodesInGroup);
    }
    if (!masterNode) {
        qDebug() << "Link slave/master for " << getName().c_str() <<   " failed to restore the following linkage: " << masterNodeName.c_str();

        return KnobIPtr();
    }

    if (!masterTableName.empty() && !masterItemName.empty()) {
        KnobItemsTablePtr table = masterNode->getEffectInstance()->getItemsTable(masterTableName);
        if (table) {
            KnobTableItemPtr item = table->getItemByFullyQualifiedScriptName(masterItemName);
            if (item) {
                return item->getKnobByName(masterKnobName);
            }
        }
    } else {
        // Find the corresponding knob
        const std::vector<KnobIPtr> & otherKnobs = masterNode->getKnobs();
        for (std::size_t j = 0; j < otherKnobs.size(); ++j) {
            if ( (otherKnobs[j]->getName() == masterKnobName) ) {
                return otherKnobs[j];
            }
        }
    }

    qDebug() << "Link slave/master for " << getName().c_str() <<   " failed to restore the following linkage: " << masterNodeName.c_str();

    return KnobIPtr();
} // findMasterKnob


void
KnobHelper::restoreKnobLinks(const boost::shared_ptr<SERIALIZATION_NAMESPACE::KnobSerializationBase>& serialization,
                             const std::map<SERIALIZATION_NAMESPACE::NodeSerializationPtr, NodePtr>& allCreatedNodesInGroup)
{




    SERIALIZATION_NAMESPACE::KnobSerialization* isKnobSerialization = dynamic_cast<SERIALIZATION_NAMESPACE::KnobSerialization*>(serialization.get());
    SERIALIZATION_NAMESPACE::GroupKnobSerialization* isGroupKnobSerialization = dynamic_cast<SERIALIZATION_NAMESPACE::GroupKnobSerialization*>(serialization.get());

    if (isGroupKnobSerialization) {
        for (std::list<boost::shared_ptr<SERIALIZATION_NAMESPACE::KnobSerializationBase> >::const_iterator it = isGroupKnobSerialization->_children.begin(); it != isGroupKnobSerialization->_children.end(); ++it) {
            try {
                restoreKnobLinks(*it, allCreatedNodesInGroup);
            } catch (const std::exception& e) {
                LogEntry::LogEntryColor c;
                EffectInstancePtr effect = toEffectInstance(getHolder());
                if (effect) {
                    if (effect->getNode()->getColor(&c.r, &c.g, &c.b)) {
                        c.colorSet = true;
                    }
                }

                appPTR->writeToErrorLog_mt_safe(QString::fromUtf8(effect->getNode()->getScriptName_mt_safe().c_str() ), QDateTime::currentDateTime(), QString::fromUtf8(e.what()), false, c);


            }
        }
    } else if (isKnobSerialization) {

        KnobTableItemPtr tableItem = toKnobTableItem(getHolder());
        EffectInstancePtr effect = toEffectInstance(getHolder());
        NodePtr thisKnobNode;
        if (tableItem) {
            thisKnobNode = tableItem->getModel()->getNode();
        } else if (effect) {
            thisKnobNode = effect->getNode();
        }
        // A knob that does not belong to a node cannot have links
        if (!thisKnobNode) {
            return;
        }
        // Restore slave/master links first
        {
            const std::vector<std::string>& projectViews = getHolder()->getApp()->getProject()->getProjectViewNames();
            for (SERIALIZATION_NAMESPACE::KnobSerialization::PerViewValueSerializationMap::const_iterator it = isKnobSerialization->_values.begin();
                 it != isKnobSerialization->_values.end(); ++it) {


                // Find a matching view name
                ViewIdx view_i(0);
                Project::getViewIndex(projectViews, it->first, &view_i);

                for (int dimIndex = 0; dimIndex < _imp->common->dimension; ++dimIndex) {


                    // Not all dimensions are necessarily saved since they may be folded.
                    // In that case replicate the last dimension
                    int d = dimIndex >= (int)it->second.size() ? it->second.size() - 1 : dimIndex;


                    if (!it->second[d]._slaveMasterLink.hasLink) {
                        continue;
                    }

                    std::string masterKnobName, masterNodeName, masterTableName, masterTableItemName;
                    if (it->second[d]._slaveMasterLink.masterNodeName.empty()) {
                        // Node name empty, assume this is the same node
                        masterNodeName = thisKnobNode->getScriptName_mt_safe();
                    } else {
                        masterNodeName = it->second[d]._slaveMasterLink.masterNodeName;
                    }

                    if (it->second[d]._slaveMasterLink.masterKnobName.empty()) {
                        // Knob name empty, assume this is the same knob unless it has a single dimension
                        if (getNDimensions() == 1) {
                            continue;
                        }
                        masterKnobName = getName();
                    } else {
                        masterKnobName = it->second[d]._slaveMasterLink.masterKnobName;
                    }

                    masterTableName = it->second[d]._slaveMasterLink.masterTableName;
                    masterTableItemName = it->second[d]._slaveMasterLink.masterTableItemName;
                    KnobIPtr master = findMasterKnob(masterKnobName,
                                                     masterNodeName,
                                                     masterTableName,
                                                     masterTableItemName, allCreatedNodesInGroup);
                    if (master) {
                        // Find dimension in master by name
                        int otherDimIndex = -1;
                        if (master->getNDimensions() == 1) {
                            otherDimIndex = 0;
                        } else {
                            for (int dm = 0; dm < master->getNDimensions(); ++dm) {
                                if ( boost::iequals(master->getDimensionName(DimIdx(dm)), it->second[dimIndex]._slaveMasterLink.masterDimensionName) ) {
                                    otherDimIndex = dm;
                                    break;
                                }
                            }
                            if (otherDimIndex == -1) {
                                // Before Natron 2.2 we serialized the dimension index. Try converting to an int
                                otherDimIndex = QString::fromUtf8(it->second[d]._slaveMasterLink.masterDimensionName.c_str()).toInt();
                            }
                        }
                        ViewIdx otherView(0);
                        Project::getViewIndex(projectViews, it->second[d]._slaveMasterLink.masterViewName, &otherView);

                        if (otherDimIndex >=0 && otherDimIndex < master->getNDimensions()) {
                            (void)linkTo(master, DimIdx(dimIndex), DimIdx(otherDimIndex), view_i, otherView);
                        } else {
                            throw std::invalid_argument(tr("Could not find a dimension named \"%1\" in \"%2\"").arg(QString::fromUtf8(it->second[d]._slaveMasterLink.masterDimensionName.c_str())).arg( QString::fromUtf8( it->second[d]._slaveMasterLink.masterKnobName.c_str() ) ).toStdString());
                        }
                    }

                } // for each dimensions
            } // for each view
        }

        // Restore expressions
        {
            const std::vector<std::string>& projectViews = getHolder()->getApp()->getProject()->getProjectViewNames();
            for (SERIALIZATION_NAMESPACE::KnobSerialization::PerViewValueSerializationMap::const_iterator it = isKnobSerialization->_values.begin();
                 it != isKnobSerialization->_values.end(); ++it) {
                // Find a matching view name
                ViewIdx view_i(0);
                Project::getViewIndex(projectViews, it->first, &view_i);


                for (int dimIndex = 0; dimIndex < _imp->common->dimension; ++dimIndex) {


                    // Not all dimensions are necessarily saved since they may be folded.
                    // In that case replicate the last dimension
                    int d = dimIndex >= (int)it->second.size() ? it->second.size() - 1 : dimIndex;

                    try {
                        if ( !it->second[d]._expression.empty() ) {
                            ExpressionLanguageEnum lang = eExpressionLanguagePython;
                            if (it->second[d]._expressionLanguage == kKnobSerializationExpressionLanguagePython) {
                                lang = eExpressionLanguagePython;
                            } else if (it->second[d]._expressionLanguage == kKnobSerializationExpressionLanguageExprtk) {
                                lang = eExpressionLanguageExprTk;
                            }
                            restoreExpression(DimIdx(dimIndex), view_i,  it->second[d]._expression, lang, it->second[d]._expresionHasReturnVariable);
                        }
                    } catch (const std::exception& e) {
                        QString err = QString::fromUtf8("Failed to restore expression: %1").arg( QString::fromUtf8( e.what() ) );
                        appPTR->writeToErrorLog_mt_safe(QString::fromUtf8( getName().c_str() ), QDateTime::currentDateTime(), err);
                    }
                } // for all dimensions
            } // for all views
        }
    }
} // restoreKnobLinks



/***************************KNOB HOLDER******************************************/

typedef std::map<FrameViewRenderKey, KnobHolderWPtr, FrameViewRenderKey_compare_less> RenderCloneMap;

struct KnobItemsTableData
{
    // Strong ref to the table
    KnobItemsTablePtr table;

    // Name of the parameter before the table in the UI, only relevant
    // if tablePosition is eKnobItemsTablePositionAfterKnob
    std::string previousParamPosition;

    // Where should the table be displayed
    KnobHolder::KnobItemsTablePositionEnum tablePosition;

    KnobItemsTableData()
    : table()
    , previousParamPosition()
    , tablePosition(KnobHolder::eKnobItemsTablePositionBottomOfAllPages)
    {
    }
};

struct KnobHolderCommonData
{
    AppInstanceWPtr app;

    std::vector<KnobIWPtr> knobsWithViewerUI;

    ///Count how many times an overlay needs to be redrawn for the instanceChanged/penMotion/penDown etc... actions
    ///to just redraw it once when the recursion level is back to 0
    mutable QMutex paramsEditLevelMutex;

    struct MultipleParamsEditData {
        std::string commandName;
        int nActionsInBracket;

        MultipleParamsEditData()
        : commandName()
        , nActionsInBracket(0)
        {

        }
    };

    // We use a stack in case user calls it recursively
    std::list<MultipleParamsEditData> paramsEditStack;

    mutable QMutex evaluationBlockedMutex;
    int evaluationBlocked;

    //Set in the begin/endChanges block
    int nbSignificantChangesDuringEvaluationBlock;
    int nbChangesDuringEvaluationBlock;
    int nbChangesRequiringMetadataRefresh;
    int duringEvaluation;
    ValueChangedReasonEnum firstKnobChangeReason;
    QMutex knobsFrozenMutex;
    bool knobsFrozen;

    // Protects hasAnimation
    mutable QMutex hasAnimationMutex;

    // True if one knob held by this object is animated.
    // This is held here for fast access, otherwise it requires looping over all knobs
    bool hasAnimation;
    DockablePanelI* settingsPanel;

    // Map of tables owned by the holder identified with their
    // unique table identifier
    std::map<std::string, KnobItemsTableData> knobsTables;

    std::list<std::pair<KnobIWPtr,OverlaySlaveParamFlags> > overlaySlaves;

    mutable QMutex renderClonesMutex;
    RenderCloneMap renderClones;

    KnobHolderCommonData()
    : app()
    , evaluationBlockedMutex(QMutex::Recursive)
    , evaluationBlocked(0)
    , nbSignificantChangesDuringEvaluationBlock(0)
    , nbChangesDuringEvaluationBlock(0)
    , nbChangesRequiringMetadataRefresh(0)
    , duringEvaluation(0)
    , firstKnobChangeReason(eValueChangedReasonPluginEdited)
    , knobsFrozenMutex()
    , knobsFrozen(false)
    , hasAnimationMutex()
    , hasAnimation(false)
    , settingsPanel(0)
    , knobsTables()
    , overlaySlaves()
    , renderClonesMutex()
    , renderClones()
    {

    }
};

struct KnobHolder::KnobHolderPrivate
{
    boost::shared_ptr<KnobHolderCommonData> common;

    QMutex knobsMutex;

    std::vector<KnobIPtr> knobs;
    std::map<std::string, KnobIWPtr> knobsOrdered;
    bool knobsInitialized;
    bool isInitializingKnobs;

    // If this is a render clone, this is a pointer to the main instance
    KnobHolderPtr mainInstance;

    // This is the current frame/view being requested or rendered by this clone.
    // Note that it is a weak reference: the actual shared references are held by other effects that reference this node.
    // This enables to track dependencies across effects and destroy FrameViewRequest objects when they are no longer references by downstream
    // effects.
    FrameViewRenderKey currentRender;

    KnobHolderPrivate(const AppInstancePtr& appInstance)
    : common(new KnobHolderCommonData)
    , knobsMutex()
    , knobs()
    , knobsOrdered()
    , knobsInitialized(false)
    , isInitializingKnobs(false)
    , mainInstance()
    , currentRender()
    {
        common->app = appInstance;

    }

    KnobHolderPrivate(const KnobHolderPtr& other)
    : common(other->_imp->common)
    , knobsMutex()
    , knobs()
    , knobsOrdered()
    , knobsInitialized(false)
    , isInitializingKnobs(false)
    , mainInstance()
    , currentRender()
    {
        // If the other is also a clone, forward to the other main instance
        mainInstance = other->getMainInstance();
        if (!mainInstance) {
            mainInstance = other;
        }
    }

    void pushUndoCommandInternal(const UndoCommandPtr& command);
};

KnobHolder::KnobHolder(const AppInstancePtr& appInstance)
: QObject()
, _imp( new KnobHolderPrivate(appInstance) )
{

}

KnobHolder::KnobHolder(const KnobHolderPtr& other, const FrameViewRenderKey& key)
    : QObject()
    , boost::enable_shared_from_this<KnobHolder>()
, _imp (new KnobHolderPrivate(other))
{
    _imp->currentRender = key;
}

KnobHolder::~KnobHolder()
{
    if (!_imp->mainInstance) {
        for (std::size_t i = 0; i < _imp->knobs.size(); ++i) {
            KnobHelperPtr helper = boost::dynamic_pointer_cast<KnobHelper>(_imp->knobs[i]);
            assert(helper);
            if (helper) {
                // Make sure nobody is referencing this
                helper->_imp->holder.reset();
                helper->deleteKnob();

            }
        }
    } else {
        for (std::map<std::string, KnobItemsTableData>::iterator it = _imp->common->knobsTables.begin(); it != _imp->common->knobsTables.end(); ++it) {
            std::vector<KnobTableItemPtr> allItems = it->second.table->getAllItems();
            for (std::vector<KnobTableItemPtr>::const_iterator it2 = allItems.begin(); it2 != allItems.end(); ++it2) {
                if ((*it2)->isRenderCloneNeeded()) {
                    (*it2)->removeRenderClone(_imp->currentRender.render.lock());
                }
            }

        }
    }
}

TreeRenderPtr
KnobHolder::getCurrentRender() const
{
    return _imp->currentRender.render.lock();
}

KnobHolderPtr
KnobHolder::getMainInstance() const
{
    return _imp->mainInstance;
}

bool
KnobHolder::isRenderClone() const
{
    return (bool)_imp->mainInstance;
}


void
KnobHolder::addItemsTable(const KnobItemsTablePtr& table, KnobItemsTablePositionEnum position, const std::string& paramScriptNameBefore)
{
    // Ensure the table doesn't exist already
    std::map<std::string, KnobItemsTableData>::const_iterator found = _imp->common->knobsTables.find(table->getTableIdentifier());
    if (found != _imp->common->knobsTables.end()) {
        return;
    }

    KnobItemsTableData data;
    data.table = table;
    data.tablePosition = position;
    data.previousParamPosition = paramScriptNameBefore;
    _imp->common->knobsTables.insert(std::make_pair(table->getTableIdentifier(),data));
}

KnobItemsTablePtr
KnobHolder::getItemsTable(const std::string& tableIdentifier) const
{
    std::map<std::string, KnobItemsTableData>::const_iterator found = _imp->common->knobsTables.find(tableIdentifier);
    if (found != _imp->common->knobsTables.end()) {
        return found->second.table;
    }
    return KnobItemsTablePtr();

}

std::list<KnobItemsTablePtr>
KnobHolder::getAllItemsTables() const
{
    std::list<KnobItemsTablePtr> ret;
    for (std::map<std::string, KnobItemsTableData>::const_iterator it = _imp->common->knobsTables.begin(); it != _imp->common->knobsTables.end(); ++it) {
        ret.push_back(it->second.table);
    }
    return ret;
}

KnobHolder::KnobItemsTablePositionEnum
KnobHolder::getItemsTablePosition(const std::string& tableIdentifier) const
{
    std::map<std::string, KnobItemsTableData>::const_iterator found = _imp->common->knobsTables.find(tableIdentifier);
    if (found != _imp->common->knobsTables.end()) {
        return found->second.tablePosition;
    }
    return eKnobItemsTablePositionBottomOfAllPages;
}

std::string
KnobHolder::getItemsTablePreviousKnobScriptName(const std::string& tableIdentifier) const
{
    std::map<std::string, KnobItemsTableData>::const_iterator found = _imp->common->knobsTables.find(tableIdentifier);
    if (found != _imp->common->knobsTables.end()) {
        return found->second.previousParamPosition;
    }
    return std::string();
}

void
KnobHolder::setViewerUIKnobs(const KnobsVec& knobs)
{
    QMutexLocker k(&_imp->knobsMutex);
    _imp->common->knobsWithViewerUI.clear();
    for (KnobsVec::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        _imp->common->knobsWithViewerUI.push_back(*it);
    }

}

void
KnobHolder::addKnobToViewerUI(const KnobIPtr& knob)
{
    QMutexLocker k(&_imp->knobsMutex);
    _imp->common->knobsWithViewerUI.push_back(knob);
}

void
KnobHolder::insertKnobToViewerUI(const KnobIPtr& knob, int index)
{
    QMutexLocker k(&_imp->knobsMutex);
    if (index < 0 || index >= (int)_imp->common->knobsWithViewerUI.size()) {
        _imp->common->knobsWithViewerUI.push_back(knob);
    } else {
        std::vector<KnobIWPtr>::iterator it = _imp->common->knobsWithViewerUI.begin();
        std::advance(it, index);
        _imp->common->knobsWithViewerUI.insert(it, knob);
    }
}

void
KnobHolder::removeKnobViewerUI(const KnobIPtr& knob)
{
    QMutexLocker k(&_imp->knobsMutex);
    for (std::vector<KnobIWPtr>::iterator it = _imp->common->knobsWithViewerUI.begin(); it!=_imp->common->knobsWithViewerUI.end(); ++it) {
        KnobIPtr p = it->lock();
        if (p == knob) {
            _imp->common->knobsWithViewerUI.erase(it);
            return;
        }
    }

}

int
KnobHolder::getInViewerContextKnobIndex(const KnobIConstPtr& knob) const
{
    QMutexLocker k(&_imp->knobsMutex);
    int i = 0;
    for (std::vector<KnobIWPtr>::const_iterator it = _imp->common->knobsWithViewerUI.begin(); it != _imp->common->knobsWithViewerUI.end(); ++it, ++i) {
        KnobIPtr p = it->lock();
        if (p == knob) {
            return i;
        }
    }
    return -1;
}

KnobsVec
KnobHolder::getViewerUIKnobs() const
{
    QMutexLocker k(&_imp->knobsMutex);
    KnobsVec ret;
    for (std::vector<KnobIWPtr>::const_iterator it = _imp->common->knobsWithViewerUI.begin(); it != _imp->common->knobsWithViewerUI.end(); ++it) {
        KnobIPtr k = it->lock();
        if (k) {
            ret.push_back(k);
        }
    }

    return ret;
}

void
KnobHolder::setIsInitializingKnobs(bool b)
{
    QMutexLocker k(&_imp->knobsMutex);

    _imp->isInitializingKnobs = b;
}

bool
KnobHolder::isInitializingKnobs() const
{
    QMutexLocker k(&_imp->knobsMutex);

    return _imp->isInitializingKnobs;
}

void
KnobHolder::addKnob(const KnobIPtr& k)
{
    {
        QMutexLocker kk(&_imp->knobsMutex);
        {
            std::map<std::string, KnobIWPtr>::iterator found = _imp->knobsOrdered.find(k->getName());
            if (found != _imp->knobsOrdered.end()) {
                return;
            }
            _imp->knobsOrdered[k->getName()] = k;
        }
        _imp->knobs.push_back(k);
        assert(_imp->knobsOrdered.size() == _imp->knobs.size());
    }

}

void
KnobHolder::insertKnob(int index,
                       const KnobIPtr& k)
{
    if (index < 0) {
        return;
    }
    QMutexLocker kk(&_imp->knobsMutex);
    {
        std::map<std::string, KnobIWPtr>::iterator found = _imp->knobsOrdered.find(k->getName());
        if (found != _imp->knobsOrdered.end()) {
            return;
        }
        _imp->knobsOrdered[k->getName()] = k;
    }

    if ( index >= (int)_imp->knobs.size() ) {
        _imp->knobs.push_back(k);
    } else {
        KnobsVec::iterator it = _imp->knobs.begin();
        std::advance(it, index);
        _imp->knobs.insert(it, k);
    }
    assert(_imp->knobsOrdered.size() == _imp->knobs.size());
}

bool
KnobHolder::removeKnobFromList(const KnobIConstPtr& knob)
{
    QMutexLocker kk(&_imp->knobsMutex);
    bool foundInVec = false;
    for (KnobsVec::iterator it = _imp->knobs.begin(); it != _imp->knobs.end(); ++it) {
        if (*it == knob) {
            _imp->knobs.erase(it);
            foundInVec = true;
            break;
        }
    }

    bool foundInMap = false;
    std::map<std::string, KnobIWPtr>::iterator found = _imp->knobsOrdered.find(knob->getName());
    if (found != _imp->knobsOrdered.end() && found->second.lock() == knob) {
        foundInMap = true;
        _imp->knobsOrdered.erase(found);
    }
    assert(_imp->knobsOrdered.size() == _imp->knobs.size());
    assert((foundInVec && foundInMap) || (!foundInVec && !foundInMap));
    return foundInVec || foundInMap;
}

void
KnobHolder::setPanelPointer(DockablePanelI* gui)
{
    assert( QThread::currentThread() == qApp->thread() );
    _imp->common->settingsPanel = gui;
}

void
KnobHolder::discardPanelPointer()
{
    assert( QThread::currentThread() == qApp->thread() );
    _imp->common->settingsPanel = 0;
}

void
KnobHolder::recreateUserKnobs(bool keepCurPageIndex)
{
    assert( QThread::currentThread() == qApp->thread() );
    if (_imp->common->settingsPanel) {
        _imp->common->settingsPanel->recreateUserKnobs(keepCurPageIndex);
        EffectInstance* isEffect = dynamic_cast<EffectInstance*>(this);
        if (isEffect) {
            isEffect->getNode()->declarePythonKnobs();
        }
    }
}

void
KnobHolder::recreateKnobs(bool keepCurPageIndex)
{
    assert( QThread::currentThread() == qApp->thread() );
    if (_imp->common->settingsPanel) {
        _imp->common->settingsPanel->refreshGuiForKnobsChanges(keepCurPageIndex);
        EffectInstance* isEffect = dynamic_cast<EffectInstance*>(this);
        if (isEffect) {
            isEffect->getNode()->declarePythonKnobs();
        }
    }
}

void
KnobHolder::deleteKnob(const KnobIPtr& knob,
                       bool alsoDeleteGui)
{
    assert( QThread::currentThread() == qApp->thread() );


    if (!removeKnobFromList(knob)) {
        return;
    }

    knob->deleteKnob();


    if (alsoDeleteGui && _imp->common->settingsPanel) {
        _imp->common->settingsPanel->deleteKnobGui(knob);
    }
}

void
KnobHolder::addOverlaySlaveParam(const KnobIPtr& knob, OverlaySlaveParamFlags type)
{
    _imp->common->overlaySlaves.push_back(std::make_pair(knob, type));
}

bool
KnobHolder::isOverlaySlaveParam(const KnobIConstPtr& knob, OverlaySlaveParamFlags type) const
{
    for (std::list<std::pair<KnobIWPtr,OverlaySlaveParamFlags> >::const_iterator it = _imp->common->overlaySlaves.begin(); it != _imp->common->overlaySlaves.end(); ++it) {
        if (((type & eOverlaySlaveViewport) && (it->second & eOverlaySlaveViewport)) ||
            ((type & eOverlaySlaveTimeline) && (it->second & eOverlaySlaveTimeline))) {

            KnobIPtr k = it->first.lock();
            if (!k) {
                continue;
            }
            if (k == knob) {
                return true;
            }
        }
    }

    return false;
}


bool
KnobHolder::moveViewerUIKnobOneStepUp(const KnobIPtr& knob)
{
    QMutexLocker k(&_imp->knobsMutex);
    for (std::size_t i = 0; i < _imp->common->knobsWithViewerUI.size(); ++i) {
        if (_imp->common->knobsWithViewerUI[i].lock() == knob) {
            if (i == 0) {
                return false;
            }
            // can't swap weak pointers using std::swap!
            // std::swap(_imp->knobsWithViewerUI[i - 1], _imp->knobsWithViewerUI[i]);
            _imp->common->knobsWithViewerUI[i].swap(_imp->common->knobsWithViewerUI[i - 1]);
            return true;
        }
    }
    return false;
}

bool
KnobHolder::moveViewerUIOneStepDown(const KnobIPtr& knob)
{
    QMutexLocker k(&_imp->knobsMutex);
    for (std::size_t i = 0; i < _imp->common->knobsWithViewerUI.size(); ++i) {
        if (_imp->common->knobsWithViewerUI[i].lock() == knob) {
            if (i == _imp->common->knobsWithViewerUI.size() - 1) {
                return false;
            }
            // can't swap weak pointers using std::swap!
            // std::swap(_imp->knobsWithViewerUI[i + 1], _imp->knobsWithViewerUI[i]);
            _imp->common->knobsWithViewerUI[i].swap(_imp->common->knobsWithViewerUI[i + 1]);
            return true;
        }
    }
    return false;
}

bool
KnobHolder::moveKnobOneStepUp(const KnobIPtr& knob)
{
    if ( knob->getKnobDeclarationType() != KnobI::eKnobDeclarationTypeUser && !toKnobPage(knob) ) {
        return false;
    }
    KnobIPtr parent = knob->getParentKnob();
    KnobGroupPtr parentIsGrp = toKnobGroup(parent);
    KnobPagePtr parentIsPage = toKnobPage(parent);

    //the knob belongs to a group/page , change its index within the group instead
    bool moveOk = false;
    if (!parent) {
        moveOk = true;
    }
    try {
        if (parentIsGrp) {
            moveOk = parentIsGrp->moveOneStepUp(knob);
        } else if (parentIsPage) {
            moveOk = parentIsPage->moveOneStepUp(knob);
        }
    } catch (const std::exception& e) {
        qDebug() << e.what();
        assert(false);

        return false;
    }

    if (moveOk) {
        QMutexLocker k(&_imp->knobsMutex);
        int prevInPage = -1;
        if (parent) {
            for (U32 i = 0; i < _imp->knobs.size(); ++i) {
                if (_imp->knobs[i] == knob) {
                    if (prevInPage != -1) {
                        KnobIPtr tmp = _imp->knobs[prevInPage];
                        _imp->knobs[prevInPage] = _imp->knobs[i];
                        _imp->knobs[i] = tmp;
                    }
                    break;
                } else {
                    if ( _imp->knobs[i]->getKnobDeclarationType() == KnobI::eKnobDeclarationTypeUser && (_imp->knobs[i]->getParentKnob() == parent) ) {
                        prevInPage = i;
                    }
                }
            }
        } else {
            bool foundPrevPage = false;
            for (U32 i = 0; i < _imp->knobs.size(); ++i) {
                if (_imp->knobs[i] == knob) {
                    if (prevInPage != -1) {
                        KnobIPtr tmp = _imp->knobs[prevInPage];
                        _imp->knobs[prevInPage] = _imp->knobs[i];
                        _imp->knobs[i] = tmp;
                        foundPrevPage = true;
                    }
                    break;
                } else {
                    if ( !_imp->knobs[i]->getParentKnob() ) {
                        prevInPage = i;
                    }
                }
            }
            if (!foundPrevPage) {
                moveOk = false;
            }
        }
    }

    return moveOk;
} // KnobHolder::moveKnobOneStepUp

bool
KnobHolder::moveKnobOneStepDown(const KnobIPtr& knob)
{
    if ( knob->getKnobDeclarationType() != KnobI::eKnobDeclarationTypeUser && !toKnobPage(knob) ) {
        return false;
    }
    KnobIPtr parent = knob->getParentKnob();
    KnobGroupPtr parentIsGrp = toKnobGroup(parent);
    KnobPagePtr parentIsPage = toKnobPage(parent);

    //the knob belongs to a group/page , change its index within the group instead
    bool moveOk = false;
    if (!parent) {
        moveOk = true;
    }
    try {
        if (parentIsGrp) {
            moveOk = parentIsGrp->moveOneStepDown(knob);
        } else if (parentIsPage) {
            moveOk = parentIsPage->moveOneStepDown(knob);
        }
    } catch (const std::exception& e) {
        qDebug() << e.what();
        assert(false);

        return false;
    }

    QMutexLocker k(&_imp->knobsMutex);
    int foundIndex = -1;
    for (U32 i = 0; i < _imp->knobs.size(); ++i) {
        if (_imp->knobs[i] == knob) {
            foundIndex = i;
            break;
        }
    }
    assert(foundIndex != -1);
    if (foundIndex < 0) {
        return false;
    }
    if (moveOk) {
        //The knob (or page) could be moved inside the group/page, just move it down
        if (parent) {
            for (int i = foundIndex + 1; i < (int)_imp->knobs.size(); ++i) {
                if ( _imp->knobs[i]->getKnobDeclarationType() == KnobI::eKnobDeclarationTypeUser && (_imp->knobs[i]->getParentKnob() == parent) ) {
                    KnobIPtr tmp = _imp->knobs[foundIndex];
                    _imp->knobs[foundIndex] = _imp->knobs[i];
                    _imp->knobs[i] = tmp;
                    break;
                }
            }
        } else {
            bool foundNextPage = false;
            for (int i = foundIndex + 1; i < (int)_imp->knobs.size(); ++i) {
                if ( !_imp->knobs[i]->getParentKnob() ) {
                    KnobIPtr tmp = _imp->knobs[foundIndex];
                    _imp->knobs[foundIndex] = _imp->knobs[i];
                    _imp->knobs[i] = tmp;
                    foundNextPage = true;
                    break;
                }
            }

            if (!foundNextPage) {
                moveOk = false;
            }
        }
    }

    return moveOk;
} // KnobHolder::moveKnobOneStepDown

KnobPagePtr
KnobHolder::getUserPageKnob() const
{
    {
        QMutexLocker k(&_imp->knobsMutex);
        for (KnobsVec::const_iterator it = _imp->knobs.begin(); it != _imp->knobs.end(); ++it) {
            if ((*it)->getKnobDeclarationType() != KnobI::eKnobDeclarationTypeUser) {
                continue;
            }
            KnobPagePtr isPage = boost::dynamic_pointer_cast<KnobPage>(*it);
            if (isPage) {
                return isPage;
            }
        }
    }

    return KnobPagePtr();
}

KnobPagePtr
KnobHolder::getOrCreateUserPageKnob()
{
    KnobPagePtr ret = getUserPageKnob();

    if (ret) {
        return ret;
    }
    ret = createKnob<KnobPage>(NATRON_USER_MANAGED_KNOBS_PAGE, 1);
    ret->setLabel(tr(NATRON_USER_MANAGED_KNOBS_PAGE_LABEL));
    onUserKnobCreated(ret, KnobI::eKnobDeclarationTypeUser);
    return ret;
}

void
KnobHolder::onUserKnobCreated(const KnobIPtr& knob, KnobI::KnobDeclarationTypeEnum isUserKnob)
{

    knob->setKnobDeclarationType(isUserKnob);
    EffectInstance* isEffect = dynamic_cast<EffectInstance*>(this);
    if (isEffect) {
        if (isUserKnob) {
            isEffect->getNode()->declarePythonKnobs();
        }
    }

}

KnobIntPtr
KnobHolder::createIntKnob(const std::string& name,
                          const std::string& label,
                          int dimension,
                          KnobI::KnobDeclarationTypeEnum userKnob)
{
    KnobIPtr existingKnob = getKnobByName(name);

    if (existingKnob) {
        return toKnobInt(existingKnob);
    }
    KnobIntPtr ret = createKnob<KnobInt>(name, dimension);
    ret->setLabel(label);
    onUserKnobCreated(ret, userKnob);
    return ret;
}

KnobDoublePtr
KnobHolder::createDoubleKnob(const std::string& name,
                             const std::string& label,
                             int dimension,
                             KnobI::KnobDeclarationTypeEnum userKnob)
{
    KnobIPtr existingKnob = getKnobByName(name);

    if (existingKnob) {
        return toKnobDouble(existingKnob);
    }
    KnobDoublePtr ret = createKnob<KnobDouble>(name, dimension);
    ret->setLabel(label);
    onUserKnobCreated(ret, userKnob);
    return ret;
}

KnobColorPtr
KnobHolder::createColorKnob(const std::string& name,
                            const std::string& label,
                            int dimension,
                            KnobI::KnobDeclarationTypeEnum userKnob)
{
    KnobIPtr existingKnob = getKnobByName(name);

    if (existingKnob) {
        return toKnobColor(existingKnob);
    }
    KnobColorPtr ret = createKnob<KnobColor>(name, dimension);
    ret->setLabel(label);
    onUserKnobCreated(ret, userKnob);

    return ret;
}

KnobBoolPtr
KnobHolder::createBoolKnob(const std::string& name,
                           const std::string& label,
                           KnobI::KnobDeclarationTypeEnum userKnob)
{
    KnobIPtr existingKnob = getKnobByName(name);

    if (existingKnob) {
        return toKnobBool(existingKnob);
    }
    KnobBoolPtr ret = createKnob<KnobBool>(name);
    ret->setLabel(label);
    onUserKnobCreated(ret, userKnob);

    return ret;
}

KnobChoicePtr
KnobHolder::createChoiceKnob(const std::string& name,
                             const std::string& label,
                             KnobI::KnobDeclarationTypeEnum userKnob)
{
    KnobIPtr existingKnob = getKnobByName(name);

    if (existingKnob) {
        return toKnobChoice(existingKnob);
    }
    KnobChoicePtr ret = createKnob<KnobChoice>(name);
    ret->setLabel(label);
    onUserKnobCreated(ret, userKnob);

    return ret;
}

KnobButtonPtr
KnobHolder::createButtonKnob(const std::string& name,
                             const std::string& label,
                             KnobI::KnobDeclarationTypeEnum userKnob)
{
    KnobIPtr existingKnob = getKnobByName(name);

    if (existingKnob) {
        return toKnobButton(existingKnob);
    }
    KnobButtonPtr ret = createKnob<KnobButton>(name);
    ret->setLabel(label);
    onUserKnobCreated(ret, userKnob);

    return ret;
}

KnobSeparatorPtr
KnobHolder::createSeparatorKnob(const std::string& name,
                                const std::string& label,
                                KnobI::KnobDeclarationTypeEnum userKnob)
{
    KnobIPtr existingKnob = getKnobByName(name);

    if (existingKnob) {
        return toKnobSeparator(existingKnob);
    }
    KnobSeparatorPtr ret = createKnob<KnobSeparator>(name);
    ret->setLabel(label);
    onUserKnobCreated(ret, userKnob);

    return ret;
}

//Type corresponds to the Type enum defined in StringParamBase in Parameter.h
KnobStringPtr
KnobHolder::createStringKnob(const std::string& name,
                             const std::string& label,
                             KnobI::KnobDeclarationTypeEnum userKnob)
{
    KnobIPtr existingKnob = getKnobByName(name);

    if (existingKnob) {
        return toKnobString(existingKnob);
    }
    KnobStringPtr ret = createKnob<KnobString>(name);
    ret->setLabel(label);
    onUserKnobCreated(ret, userKnob);
    return ret;
}

KnobFilePtr
KnobHolder::createFileKnob(const std::string& name,
                           const std::string& label,
                           KnobI::KnobDeclarationTypeEnum userKnob)
{
    KnobIPtr existingKnob = getKnobByName(name);

    if (existingKnob) {
        return toKnobFile(existingKnob);
    }
    KnobFilePtr ret = createKnob<KnobFile>(name);
    ret->setLabel(label);
    onUserKnobCreated(ret, userKnob);

    return ret;
}



KnobPathPtr
KnobHolder::createPathKnob(const std::string& name,
                           const std::string& label,
                           KnobI::KnobDeclarationTypeEnum userKnob)
{
    KnobIPtr existingKnob = getKnobByName(name);

    if (existingKnob) {
        return toKnobPath(existingKnob);
    }
    KnobPathPtr ret = createKnob<KnobPath>(name);
    ret->setLabel(label);
    onUserKnobCreated(ret, userKnob);
    return ret;
}

KnobGroupPtr
KnobHolder::createGroupKnob(const std::string& name,
                            const std::string& label,
                            KnobI::KnobDeclarationTypeEnum userKnob)
{
    KnobIPtr existingKnob = getKnobByName(name);

    if (existingKnob) {
        return toKnobGroup(existingKnob);
    }
    KnobGroupPtr ret = createKnob<KnobGroup>(name);
    ret->setLabel(label);
    onUserKnobCreated(ret, userKnob);

    return ret;
}

KnobPagePtr
KnobHolder::createPageKnob(const std::string& name,
                           const std::string& label,
                           KnobI::KnobDeclarationTypeEnum userKnob)
{
    KnobIPtr existingKnob = getKnobByName(name);

    if (existingKnob) {
        return toKnobPage(existingKnob);
    }
    KnobPagePtr ret = createKnob<KnobPage>(name);
    ret->setLabel(label);
    onUserKnobCreated(ret, userKnob);
    return ret;
}

KnobParametricPtr
KnobHolder::createParametricKnob(const std::string& name,
                                 const std::string& label,
                                 int nbCurves,
                                 KnobI::KnobDeclarationTypeEnum userKnob)
{
    KnobIPtr existingKnob = getKnobByName(name);

    if (existingKnob) {
        return toKnobParametric(existingKnob);
    }
    KnobParametricPtr ret = createKnob<KnobParametric>(name, nbCurves);
    ret->setLabel(label);
    onUserKnobCreated(ret, userKnob);

    return ret;
}


void
KnobHolder::invalidateCacheHashAndEvaluate(bool isSignificant, bool refreshMetadata)
{
    if (isEvaluationBlocked()) {
        return;
    }
    invalidateHashCache();
    evaluate(isSignificant, refreshMetadata);
}

void
KnobHolder::endChanges(bool discardRendering)
{
    if (QThread::currentThread() != qApp->thread()) {
        return;
    }

    bool hasHadAnyChange = false;
    bool mustRefreshMetadata = false;
    bool hasHadSignificantChange = false;
    int evaluationBlocked;
    ValueChangedReasonEnum firstKnobReason;

    {
        QMutexLocker l(&_imp->common->evaluationBlockedMutex);
        if (_imp->common->evaluationBlocked > 0) {
            --_imp->common->evaluationBlocked;
        }
        evaluationBlocked = _imp->common->evaluationBlocked;
        firstKnobReason = _imp->common->firstKnobChangeReason;
        if (evaluationBlocked == 0) {
            if (_imp->common->nbSignificantChangesDuringEvaluationBlock) {
                hasHadSignificantChange = true;
            }
            if (_imp->common->nbChangesRequiringMetadataRefresh) {
                mustRefreshMetadata = true;
            }
            if (_imp->common->nbChangesDuringEvaluationBlock) {
                hasHadAnyChange = true;
            }
            _imp->common->nbSignificantChangesDuringEvaluationBlock = 0;
            _imp->common->nbChangesDuringEvaluationBlock = 0;
            _imp->common->nbChangesRequiringMetadataRefresh = 0;
        }
    }



    if (hasHadAnyChange) {

        // Update the holder has animation flag
        updateHasAnimation();

        // Call the action
        endKnobsValuesChanged_public(firstKnobReason);

        if (discardRendering) {
            hasHadSignificantChange = false;
        }
        ++_imp->common->duringEvaluation;
        if (_imp->common->duringEvaluation == 1) {
            evaluate(hasHadSignificantChange, mustRefreshMetadata);
        }
        --_imp->common->duringEvaluation;

    }

} // KnobHolder::endChanges


bool
KnobHolder::onKnobValueChangedInternal(const KnobIPtr& knob,
                                       TimeValue time,
                                       ViewSetSpec view,
                                       ValueChangedReasonEnum reason)
{
    // Knobs are not yet initialized, don't bother notifying
    if ( isInitializingKnobs() ) {
        return false;
    }

    // Don't run anything when setValue was called on a thread different than the main thread
    if (QThread::currentThread() != qApp->thread()) {
        return true;
    }
    bool ret = false;

    bool valueChangesBlocked = knob->isValueChangesBlocked();

    {
        QMutexLocker l(&_imp->common->evaluationBlockedMutex);

        if (_imp->common->nbChangesDuringEvaluationBlock == 0) {
            // This is the first change, call begin action
            beginKnobsValuesChanged_public(reason);
        }

        if (knob->getIsMetadataSlave()) {
            ++_imp->common->nbChangesRequiringMetadataRefresh;
        }

        if ( !valueChangesBlocked && knob->getEvaluateOnChange() ) {
            ++_imp->common->nbSignificantChangesDuringEvaluationBlock;
        }
        if (_imp->common->nbChangesDuringEvaluationBlock == 0) {
            _imp->common->firstKnobChangeReason = reason;
        }
        ++_imp->common->nbChangesDuringEvaluationBlock;

    }

    // Call the knobChanged action
    if (!valueChangesBlocked) {
        ret |= onKnobValueChanged_public(knob, reason, time, view);
    }

    return ret;
} // KnobHolder::appendValueChange

void
KnobHolder::beginChanges()
{
    if (QThread::currentThread() != qApp->thread()) {
        return;
    }

    /*
     * Start a begin/end block, actually blocking all evaluations (renders) but not value changed callback.
     */
    QMutexLocker l(&_imp->common->evaluationBlockedMutex);
    ++_imp->common->evaluationBlocked;
}

bool
KnobHolder::isEvaluationBlocked() const
{
    QMutexLocker l(&_imp->common->evaluationBlockedMutex);

    return _imp->common->evaluationBlocked > 0;
}

void
KnobHolder::beginMultipleEdits(const std::string& commandName)
{
    bool mustCallBeginChanges;
    {
        QMutexLocker l(&_imp->common->paramsEditLevelMutex);
        KnobHolderCommonData::MultipleParamsEditData data;
        data.commandName = commandName;
        mustCallBeginChanges = _imp->common->paramsEditStack.empty();
        _imp->common->paramsEditStack.push_back(data);

    }
    if (mustCallBeginChanges) {
        beginChanges();
    }
}

KnobHolder::MultipleParamsEditEnum
KnobHolder::getMultipleEditsLevel() const
{
    QMutexLocker l(&_imp->common->paramsEditLevelMutex);
    if (_imp->common->paramsEditStack.empty()) {
        return eMultipleParamsEditOff;
    }
    const KnobHolderCommonData::MultipleParamsEditData& last = _imp->common->paramsEditStack.back();
    if (last.nActionsInBracket > 0) {
        return eMultipleParamsEditOn;
    } else {
        return eMultipleParamsEditOnCreateNewCommand;
    }
}

std::string
KnobHolder::getCurrentMultipleEditsCommandName() const
{
    QMutexLocker l(&_imp->common->paramsEditLevelMutex);
    if (_imp->common->paramsEditStack.empty()) {
        return std::string();
    }
    return _imp->common->paramsEditStack.back().commandName;
}

void
KnobHolder::endMultipleEdits()
{
    bool mustCallEndChanges = false;
    {
        QMutexLocker l(&_imp->common->paramsEditLevelMutex);
        if (_imp->common->paramsEditStack.empty()) {
            qDebug() << "[BUG]: Call to endMultipleEdits without a matching call to beginMultipleEdits";
            return;
        }
        _imp->common->paramsEditStack.pop_back();
        mustCallEndChanges = _imp->common->paramsEditStack.empty();
    }
    if (mustCallEndChanges) {
        endChanges();
    }
}

AppInstancePtr
KnobHolder::getApp() const
{
    return _imp->common->app.lock();
}

void
KnobHolder::initializeKnobsPublic()
{
    if (_imp->knobsInitialized) {
        return;
    }
    KnobHolderPtr thisShared = shared_from_this();
    if (!_imp->mainInstance) {
        InitializeKnobsFlag_RAII __isInitializingKnobsFlag__(thisShared);
        initializeKnobs();
    } else {
        // For a clone, just make a shallow copy of the main instance knobs
        KnobsVec mainInstanceKnobs = _imp->mainInstance->getKnobs_mt_safe();
        for (KnobsVec::const_iterator it = mainInstanceKnobs.begin(); it != mainInstanceKnobs.end(); ++it) {
            KnobIPtr copy = appPTR->getKnobFactory().createRenderCloneKnob((*it), thisShared);
            assert(copy);
            (void)copy;
        }

        // Force the clone to fetch its parameters
        fetchRenderCloneKnobs();
    }
    _imp->knobsInitialized = true;
}

bool
KnobHolder::removeRenderClone(const TreeRenderPtr& render)
{

    std::list<KnobHolderPtr> clones;
    {
        QMutexLocker locker(&_imp->common->renderClonesMutex);
        RenderCloneMap newMap;
        for (RenderCloneMap::iterator it = _imp->common->renderClones.begin(); it != _imp->common->renderClones.end(); ++it) {
            if (it->first.render.lock() == render) {
                clones.push_back(it->second.lock());
                continue;
            }
            newMap.insert(*it);
        }
        _imp->common->renderClones = newMap;
    }

    if (clones.empty()) {
        return false;
    }
    for (std::list<KnobHolderPtr>::const_iterator it = clones.begin(); it != clones.end(); ++it) {
        // For each knob, remove the clone from the map
        for (std::size_t i = 0; i < _imp->knobs.size(); ++i) {
            KnobHelperPtr k = toKnobHelper(_imp->knobs[i]);
            QMutexLocker locker(&k->_imp->common->renderClonesMapMutex);
            std::map<KnobHolderWPtr, KnobIWPtr>::iterator found = k->_imp->common->renderClonesMap.find(*it);
            if (found != k->_imp->common->renderClonesMap.end()) {
                k->_imp->common->renderClonesMap.erase(found);
            }
        }
    }

    return true;
}

KnobHolderPtr
KnobHolder::createRenderClone(const FrameViewRenderKey& key) const
{
    if (!isRenderCloneNeeded()) {
        KnobHolderConstPtr thisShared = shared_from_this();
        return boost::const_pointer_cast<KnobHolder>(thisShared);
    }

    QMutexLocker k(&_imp->common->renderClonesMutex);
    RenderCloneMap::iterator found = _imp->common->renderClones.find(key);
    if (found != _imp->common->renderClones.end()) {
        KnobHolderPtr clone = found->second.lock();
        if (clone) {
            return clone;
        }
    }


    KnobHolderPtr copy = createRenderCopy(key);
    if (!copy) {
        return copy;
    }
    if (!copy->isRenderClone()) {
        // We may not have really cloned the effect (e.g: NodeGroup)
        return copy;
    }
    key.render.lock()->registerRenderClone(copy);


    _imp->common->renderClones[key] = copy;

    copy->initializeKnobsPublic();
    return copy;
}

KnobHolderPtr
KnobHolder::getRenderClone(const FrameViewRenderKey& key) const
{

    // This must be the main instance!
    assert(!_imp->mainInstance);
    if (!isRenderCloneNeeded()) {
        KnobHolderConstPtr thisShared = shared_from_this();
        return boost::const_pointer_cast<KnobHolder>(thisShared);
    }
    QMutexLocker k(&_imp->common->renderClonesMutex);
    RenderCloneMap::iterator found = _imp->common->renderClones.find(key);
    if (found != _imp->common->renderClones.end()) {
        return found->second.lock();
    }
    return KnobHolderPtr();
}

void
KnobHolder::fetchRenderCloneKnobs()
{
    assert(_imp->mainInstance);
    for (std::map<std::string, KnobItemsTableData>::const_iterator it = _imp->common->knobsTables.begin(); it != _imp->common->knobsTables.end(); ++it) {
        std::vector<KnobTableItemPtr> allItems = it->second.table->getAllItems();
        for (std::vector<KnobTableItemPtr>::const_iterator it2 = allItems.begin(); it2 != allItems.end(); ++it2) {
            if ((*it2)->isRenderCloneNeeded()) {
                (*it2)->createRenderClone(_imp->currentRender);
            }
        }
    }
}

void
KnobHolder::refreshAfterTimeChange(bool isPlayback,
                                   TimeValue time)
{
    assert( QThread::currentThread() == qApp->thread() );
    AppInstancePtr app = getApp();
    if ( !app || app->isGuiFrozen() ) {
        return;
    }
    for (std::size_t i = 0; i < _imp->knobs.size(); ++i) {
        _imp->knobs[i]->onTimeChanged(isPlayback, time);
    }
    for (std::map<std::string, KnobItemsTableData>::const_iterator it = _imp->common->knobsTables.begin(); it != _imp->common->knobsTables.end(); ++it) {
        it->second.table->refreshAfterTimeChange(isPlayback, time);
    }

    refreshExtraStateAfterTimeChanged(isPlayback, time);
}

TimeValue
KnobHolder::getTimelineCurrentTime() const
{
    AppInstancePtr app = getApp();
    if (app) {
        return TimeValue(app->getTimeLine()->currentFrame());
    } else {
        return TimeValue(0);
    }
}


TimeValue
KnobHolder::getCurrentRenderTime() const
{
    if (_imp->mainInstance) {
        return _imp->currentRender.time;
    } else {
        return TimeValue(getTimelineCurrentTime());
    }
}

ViewIdx
KnobHolder::getCurrentRenderView() const
{
    if (_imp->mainInstance) {
        return _imp->currentRender.view;
    } else {
        return ViewIdx(0);
    }
}

void
KnobHolder::refreshAfterTimeChangeOnlyKnobsWithTimeEvaluation(TimeValue time)
{
    assert( QThread::currentThread() == qApp->thread() );
    for (std::size_t i = 0; i < _imp->knobs.size(); ++i) {
        if ( _imp->knobs[i]->evaluateValueChangeOnTimeChange() ) {
            _imp->knobs[i]->onTimeChanged(false, time);
        }
    }
}


KnobIPtr
KnobHolder::getKnobByName(const std::string & name) const
{
    QMutexLocker k(&_imp->knobsMutex);
    std::map<std::string, KnobIWPtr>::iterator found = _imp->knobsOrdered.find(name);
    if (found != _imp->knobsOrdered.end()) {
        return found->second.lock();
    }
    return KnobIPtr();
}

const KnobsVec &
KnobHolder::getKnobs() const
{
    assert( QThread::currentThread() == qApp->thread() );

    return _imp->knobs;
}

KnobsVec
KnobHolder::getKnobs_mt_safe() const
{
    QMutexLocker k(&_imp->knobsMutex);

    return _imp->knobs;
}


void
KnobHolder::beginKnobsValuesChanged_public(ValueChangedReasonEnum reason)
{
    ///cannot run in another thread.
    assert( QThread::currentThread() == qApp->thread() );

    beginKnobsValuesChanged(reason);
}

void
KnobHolder::endKnobsValuesChanged_public(ValueChangedReasonEnum reason)
{
    ///cannot run in another thread.
    assert( QThread::currentThread() == qApp->thread() );

    endKnobsValuesChanged(reason);
}

bool
KnobHolder::onKnobValueChanged_public(const KnobIPtr& k,
                                      ValueChangedReasonEnum reason,
                                      TimeValue time,
                                      ViewSetSpec view)
{
    ///cannot run in another thread.
    assert( QThread::currentThread() == qApp->thread() );
    if (!_imp->knobsInitialized) {
        return false;
    }

    bool ret = onKnobValueChanged(k, reason, time, view);
    if (ret) {
        if (reason != eValueChangedReasonTimeChanged) {
            OverlayInteractBasePtr interact = k->getCustomInteract();
            if (interact) {
                interact->redraw();
            } else {
                if (isOverlaySlaveParam(k, eOverlaySlaveViewport)) {
                    getApp()->redrawAllViewers();
                } else if (isOverlaySlaveParam(k, eOverlaySlaveTimeline)) {
                    getApp()->redrawAllTimelines();
                }

            }
        }
    }
    return ret;
}


int
KnobHolder::getPageIndex(const KnobPagePtr page) const
{
    QMutexLocker k(&_imp->knobsMutex);
    int pageIndex = 0;

    for (std::size_t i = 0; i < _imp->knobs.size(); ++i) {
        KnobPagePtr ispage = toKnobPage(_imp->knobs[i]);
        if (ispage) {
            if (page == ispage) {
                return pageIndex;
            } else {
                ++pageIndex;
            }
        }
    }

    return -1;
}

bool
KnobHolder::getHasAnimation() const
{
    QMutexLocker k(&_imp->common->hasAnimationMutex);

    return _imp->common->hasAnimation;
}

void
KnobHolder::setHasAnimation(bool hasAnimation)
{
    QMutexLocker k(&_imp->common->hasAnimationMutex);

    _imp->common->hasAnimation = hasAnimation;
}

void
KnobHolder::updateHasAnimation()
{
    bool hasAnimation = false;
    {
        QMutexLocker l(&_imp->knobsMutex);

        for (KnobsVec::const_iterator it = _imp->knobs.begin(); it != _imp->knobs.end(); ++it) {
            if ( (*it)->hasAnimation() ) {
                hasAnimation = true;
                break;
            }
        }
    }
    QMutexLocker k(&_imp->common->hasAnimationMutex);

    _imp->common->hasAnimation = hasAnimation;
}

void
KnobHolder::appendToHash(const ComputeHashArgs& args, Hash64* hash)
{
    KnobsVec knobs = getKnobs_mt_safe();
    for (KnobsVec::const_iterator it = knobs.begin(); it!=knobs.end(); ++it) {
        if (!(*it)->getEvaluateOnChange()) {
            continue;
        }
        if (args.hashType == eComputeHashTypeOnlyMetadataSlaves && !(*it)->getIsMetadataSlave()) {
            continue;
        }
        U64 knobHash = (*it)->computeHash(args);
        hash->append(knobHash);

    }

} // appendToHash

bool
KnobHolder::isFullAnimationToHashEnabled() const
{
    return false;
}

void
KnobHolder::KnobHolderPrivate::pushUndoCommandInternal(const UndoCommandPtr& command)
{

    if (common->settingsPanel) {
        common->settingsPanel->pushUndoCommand(command);
    } else {
        command->redo();
    }

}

void
KnobHolder::pushUndoCommand(UndoCommand* command)
{
    UndoCommandPtr ptr(command);
    pushUndoCommand(ptr);
}

void
KnobHolder::pushUndoCommand(const UndoCommandPtr& command)
{
    _imp->pushUndoCommandInternal(command);
}



/***************************KNOB EXPLICIT TEMPLATE INSTANTIATION******************************************/

template class ValueKnobDimView<int>;
template class ValueKnobDimView<double>;
template class ValueKnobDimView<bool>;
template class ValueKnobDimView<std::string>;

template class Knob<int>;
template class Knob<double>;
template class Knob<bool>;
template class Knob<std::string>;

template class AddToUndoRedoStackHelper<int>;
template class AddToUndoRedoStackHelper<double>;
template class AddToUndoRedoStackHelper<bool>;
template class AddToUndoRedoStackHelper<std::string>;


NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_Knob.cpp"
