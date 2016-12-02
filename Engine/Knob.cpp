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

#include "Knob.h"
#include "KnobPrivate.h"
#include "KnobImpl.h"
#include "KnobGetValueImpl.h"
#include "KnobSetValueImpl.h"

#include "Engine/KnobTypes.h"

SERIALIZATION_NAMESPACE_USING

NATRON_NAMESPACE_ENTER;


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
                       const std::string &label,
                       int nDims,
                       bool declaredByPlugin)
    : _signalSlotHandler()
    , _imp( new KnobHelperPrivate(this, holder, nDims, label, declaredByPlugin) )
{
    if (holder) {
        // When knob value changes, the holder needs to be invalidated aswell
        addHashListener(holder);
    }
}

KnobHelper::~KnobHelper()
{
}

void
KnobHelper::setHolder(const KnobHolderPtr& holder)
{
    _imp->holder = holder;
}

void
KnobHelper::incrementExpressionRecursionLevel() const
{
    KnobDataTLSPtr tls = _imp->tlsData->getOrCreateTLSData();

    assert(tls);
    ++tls->expressionRecursionLevel;
}

void
KnobHelper::decrementExpressionRecursionLevel() const
{
    KnobDataTLSPtr tls = _imp->tlsData->getTLSData();

    assert(tls);
    assert(tls->expressionRecursionLevel > 0);
    --tls->expressionRecursionLevel;
}

int
KnobHelper::getExpressionRecursionLevel() const
{
    KnobDataTLSPtr tls = _imp->tlsData->getTLSData();

    if (!tls) {
        return 0;
    }

    return tls->expressionRecursionLevel;
}

void
KnobHelper::setHashingStrategy(KnobFrameViewHashingStrategyEnum strategy)
{
    _imp->cacheInvalidationStrategy = strategy;
}

KnobFrameViewHashingStrategyEnum
KnobHelper::getHashingStrategy() const
{
    return _imp->cacheInvalidationStrategy;
}

void
KnobHelper::deleteKnob()
{
    // Prevent any signal 
    blockValueChanges();

    KnobI::ListenerDimsMap listenersCpy = _imp->listeners;

    for (ListenerDimsMap::iterator it = listenersCpy.begin(); it != listenersCpy.end(); ++it) {
        KnobIPtr knob = it->first.lock();
        if (!knob) {
            continue;
        }
        KnobIPtr aliasKnob = knob->getAliasMaster();
        if (aliasKnob.get() == this) {
            knob->setKnobAsAliasOfThis(aliasKnob, false);
        }

        knob->setExpressionInvalid( DimSpec::all(), ViewSetSpec::all(), false, tr("%1: parameter does not exist").arg( QString::fromUtf8( getName().c_str() ) ).toStdString() );
        if (knob.get() != this) {
            knob->unSlave(DimSpec::all(), ViewSetSpec::all(), false);
        }
    }

    KnobHolderPtr holder = getHolder();

    if ( holder && holder->getApp() ) {
        holder->getApp()->recheckInvalidExpressions();
    }

    clearExpression(DimSpec::all(), ViewSetSpec::all(), true);


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
                if ( useHostOverlayHandle() ) {
                    node->removePositionHostOverlay( shared_from_this() );
                }
                node->removeParameterFromPython( getName() );
            }
        }
    }
} // KnobHelper::deleteKnob

void
KnobHelper::setKnobGuiPointer(const KnobGuiIPtr& ptr)
{
    assert( QThread::currentThread() == qApp->thread() );
    _imp->gui = ptr;
}

KnobGuiIPtr
KnobHelper::getKnobGuiPointer() const
{
    return _imp->gui.lock();
}


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
KnobHelper::getAllDimensionsVisible(ViewGetSpec view) const
{
    ViewIdx view_i = getViewIdxFromGetSpec(view);
    QMutexLocker k(&_imp->stateMutex);
    PerViewAllDimensionsVisible::const_iterator foundView = _imp->allDimensionsVisible.find(view_i);
    if (foundView == _imp->allDimensionsVisible.end()) {
        return true;
    }
    return foundView->second;
}

void
KnobI::autoAdjustFoldExpandDimensions(ViewIdx view)
{
    bool currentVisibility = getAllDimensionsVisible(view);
    bool allEqual = areDimensionsEqual(view);
    if (allEqual) {
        if (currentVisibility) {
            setAllDimensionsVisible(view, false);
        }
    } else {
        if (!currentVisibility) {
            setAllDimensionsVisible(view, true);
        }
    }
}

void
KnobHelper::setCanAutoFoldDimensions(bool enabled)
{
    {
        QMutexLocker k(&_imp->stateMutex);
        _imp->autoFoldEnabled = enabled;
    }
    if (!enabled) {
        setAllDimensionsVisible(ViewSetSpec::all(), true);
    }
}

bool
KnobHelper::isAutoFoldDimensionsEnabled() const
{
    QMutexLocker k(&_imp->stateMutex);
    return _imp->autoFoldEnabled;
}

void
KnobHelper::setCanAutoExpandDimensions(bool enabled)
{
    {
        QMutexLocker k(&_imp->stateMutex);
        _imp->autoExpandEnabled = enabled;
    }
}

bool
KnobHelper::isAutoExpandDimensionsEnabled() const
{
    QMutexLocker k(&_imp->stateMutex);
    return _imp->autoExpandEnabled;
}


void
KnobHelper::setAllDimensionsVisibleInternal(ViewIdx view, bool visible)
{
    {
        QMutexLocker k(&_imp->stateMutex);
        bool& curValue = _imp->allDimensionsVisible[view];
        if (curValue == visible) {
            return;
        }
        curValue = visible;
    }
    onAllDimensionsMadeVisible(view, visible);
}

void
KnobHelper::setAllDimensionsVisible(ViewSetSpec view, bool visible)
{
    beginChanges();
    if (view.isAll()) {
        std::list<ViewIdx> views = getViewsList();
        for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
            setAllDimensionsVisibleInternal(*it, visible);
        }
    } else {
        ViewIdx view_i = getViewIdxFromGetSpec(ViewGetSpec(view));
        setAllDimensionsVisibleInternal(view_i, visible);
    }
    endChanges();
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
KnobHelper::setDeclaredByPlugin(bool b)
{
    _imp->declaredByPlugin = b;
}

bool
KnobHelper::isDeclaredByPlugin() const
{
    return _imp->declaredByPlugin;
}

void
KnobHelper::setAsUserKnob(bool b)
{
    _imp->userKnob = b;
}

bool
KnobHelper::isUserKnob() const
{
    return _imp->userKnob;
}

void
KnobHelper::populate()
{
    KnobIPtr thisKnob = shared_from_this();
    boost::shared_ptr<KnobSignalSlotHandler> handler( new KnobSignalSlotHandler(thisKnob) );

    setSignalSlotHandler(handler);

    if (!isAnimatedByDefault()) {
        _imp->isAnimationEnabled = false;
    }

    KnobColor* isColor = dynamic_cast<KnobColor*>(this);
    KnobSeparator* isSep = dynamic_cast<KnobSeparator*>(this);
    KnobPage* isPage = dynamic_cast<KnobPage*>(this);
    KnobGroup* isGrp = dynamic_cast<KnobGroup*>(this);
    if (isPage || isGrp) {
        _imp->evaluateOnChange = false;
    }
    if (isSep) {
        _imp->IsPersistent = false;
    }
    for (int i = 0; i < _imp->dimension; ++i) {
        if ( canAnimate() ) {
            _imp->curves[i][ViewIdx(0)].reset( new Curve(shared_from_this(), DimIdx(i), ViewIdx(0)) );
        }


        if (!isColor) {
            switch (i) {
            case 0:
                _imp->dimensionNames[i] = "x";
                break;
            case 1:
                _imp->dimensionNames[i] = "y";
                break;
            case 2:
                _imp->dimensionNames[i] = "z";
                break;
            case 3:
                _imp->dimensionNames[i] = "w";
                break;
            default:
                break;
            }
        } else {
            switch (i) {
            case 0:
                _imp->dimensionNames[i] = "r";
                break;
            case 1:
                _imp->dimensionNames[i] = "g";
                break;
            case 2:
                _imp->dimensionNames[i] = "b";
                break;
            case 3:
                _imp->dimensionNames[i] = "a";
                break;
            default:
                break;
            }
        }
    }
} // KnobHelper::populate

std::string
KnobHelper::getDimensionName(DimIdx dimension) const
{
    if ( (dimension < 0) || ( dimension >= (int)_imp->dimensionNames.size() ) ) {
        throw std::invalid_argument("KnobHelper::getDimensionName: dimension out of range");
    }
    return _imp->dimensionNames[dimension];
}

void
KnobHelper::setDimensionName(DimIdx dimension,
                             const std::string & name)
{
    if ( (dimension < 0) || ( dimension >= (int)_imp->dimensionNames.size() ) ) {
        throw std::invalid_argument("KnobHelper::getDimensionName: dimension out of range");
    }
    _imp->dimensionNames[dimension] = name;
    _signalSlotHandler->s_dimensionNameChanged(dimension);

}


void
KnobHelper::setSignalSlotHandler(const boost::shared_ptr<KnobSignalSlotHandler> & handler)
{
    _signalSlotHandler = handler;
}




bool
KnobHelper::isAnimated(DimIdx dimension,
                       ViewGetSpec view) const
{
    if (dimension < 0 || dimension >= (int)_imp->dimension) {
        throw std::invalid_argument("KnobHelper::isAnimated; dimension out of range");
    }

    if ( !canAnimate() ) {
        return false;
    }
    ViewIdx view_i = getViewIdxFromGetSpec(view);
    CurvePtr curve = getCurve(view_i, dimension);
    return curve ? curve->isAnimated() : false;
}

bool
KnobHelper::canSplitViews() const
{
    return isAnimationEnabled();
}


bool
KnobHelper::splitView(ViewIdx view)
{
    if (!AnimatingObjectI::splitView(view)) {
        return false;
    }
    int nDims = getNDimensions();
    for (int i = 0; i < nDims; ++i) {
        {
            QMutexLocker k(&_imp->curvesMutex);
            const CurvePtr& mainViewCurve = _imp->curves[i][ViewIdx(0)];
            if (mainViewCurve) {
                CurvePtr& viewCurve = _imp->curves[i][view];
                if (!viewCurve) {
                    viewCurve.reset(new Curve(shared_from_this(), DimIdx(i), view));
                    viewCurve->clone(*mainViewCurve);
                }
            }
        }
        {
            QMutexLocker k(&_imp->expressionMutex);
            const Expr& mainViewExpr = _imp->expressions[i][ViewIdx(0)];
            Expr& viewExpr = _imp->expressions[i][view];
            viewExpr = mainViewExpr;
        }
        {
            QWriteLocker k(&_imp->mastersMutex);
            const MasterKnobLink& mainViewLink = _imp->masters[i][ViewIdx(0)];
            MasterKnobLink& viewLink = _imp->masters[i][view];
            viewLink = mainViewLink;
        }
        {
            QMutexLocker k(&_imp->animationLevelMutex);
            _imp->animationLevel[i][view] = _imp->animationLevel[i][ViewIdx(0)];

        }
        {
            QMutexLocker k(&_imp->hasModificationsMutex);
            _imp->hasModifications[i][view] = _imp->hasModifications[i][ViewIdx(0)];
        }
        {
            QMutexLocker k(&_imp->stateMutex);
            _imp->enabled[i][view] = _imp->enabled[i][ViewIdx(0)];
        }
        {
            QMutexLocker k(&_imp->stateMutex);
            _imp->allDimensionsVisible[view] = _imp->allDimensionsVisible[ViewIdx(0)];
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
            QMutexLocker k(&_imp->curvesMutex);
            PerViewCurveMap::iterator foundView = _imp->curves[i].find(view);
            if (foundView != _imp->curves[i].end()) {
                _imp->curves[i].erase(foundView);

            }
        }
        {
            QMutexLocker k(&_imp->expressionMutex);
            ExprPerViewMap::iterator foundView = _imp->expressions[i].find(view);
            if (foundView != _imp->expressions[i].end()) {
                _imp->expressions[i].erase(foundView);
            }
        }
        {
            QMutexLocker k(&_imp->animationLevelMutex);
            PerViewMasterLink::iterator foundView = _imp->masters[i].find(view);
            if (foundView != _imp->masters[i].end()) {
                _imp->masters[i].erase(foundView);
            }
        }
        {
            QMutexLocker k(&_imp->animationLevelMutex);
            PerViewAnimLevelMap::iterator foundView = _imp->animationLevel[i].find(view);
            if (foundView != _imp->animationLevel[i].end()) {
                _imp->animationLevel[i].erase(foundView);
            }
        }
        {
            QMutexLocker k(&_imp->hasModificationsMutex);
            PerViewHasModificationMap::iterator foundView = _imp->hasModifications[i].find(view);
            if (foundView != _imp->hasModifications[i].end()) {
                _imp->hasModifications[i].erase(foundView);
            }
        }
        {
            QMutexLocker k(&_imp->stateMutex);
            PerViewEnabledMap::iterator foundView = _imp->enabled[i].find(view);
            if (foundView != _imp->enabled[i].end()) {
                _imp->enabled[i].erase(foundView);
            }
        }
        {
            QMutexLocker k(&_imp->stateMutex);
            PerViewAllDimensionsVisible::iterator foundView = _imp->allDimensionsVisible.find(view);
            if (foundView != _imp->allDimensionsVisible.end()) {
                _imp->allDimensionsVisible.erase(foundView);
            }
        }
    }
    _signalSlotHandler->s_availableViewsChanged();
    return true;
} // unSplitView

int
KnobHelper::getNDimensions() const
{
    return _imp->dimension;
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
        QMutexLocker k(&_imp->valueChangedBlockedMutex);

        ++_imp->valueChangedBlocked;
    }
}

void
KnobHelper::unblockValueChanges()
{
    QMutexLocker k(&_imp->valueChangedBlockedMutex);

    --_imp->valueChangedBlocked;
}

bool
KnobHelper::isValueChangesBlocked() const
{
    QMutexLocker k(&_imp->valueChangedBlockedMutex);

    return _imp->valueChangedBlocked > 0;
}

void
KnobHelper::blockListenersNotification()
{
    QMutexLocker k(&_imp->valueChangedBlockedMutex);

    ++_imp->listenersNotificationBlocked;
}

void
KnobHelper::unblockListenersNotification()
{
    QMutexLocker k(&_imp->valueChangedBlockedMutex);

    --_imp->listenersNotificationBlocked;
}

bool
KnobHelper::isListenersNotificationBlocked() const
{
    QMutexLocker k(&_imp->valueChangedBlockedMutex);

    return _imp->listenersNotificationBlocked > 0;
}

void
KnobHelper::setAutoKeyingEnabled(bool enabled)
{
    QMutexLocker k(&_imp->valueChangedBlockedMutex);
    if (enabled) {
        ++_imp->autoKeyingDisabled;
    } else {
        --_imp->autoKeyingDisabled;
    }
}

bool
KnobHelper::isAutoKeyingEnabledInternal(DimIdx dimension, ViewIdx view) const
{

    if (dimension < 0 || dimension >= _imp->dimension) {
        return false;
    }


    // The knob doesn't have any animation don't start keying automatically
    if (getAnimationLevel(dimension, view) == eAnimationLevelNone) {
        return false;
    }
    
    return true;

}

bool
KnobHelper::isAutoKeyingEnabled(DimSpec dimension, ViewSetSpec view, ValueChangedReasonEnum reason) const
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
        (reason != eValueChangedReasonNatronGuiEdited) &&
        (reason != eValueChangedReasonNatronInternalEdited)) {
        return false;
    }

    // The knob cannot animate
    if (!isAnimationEnabled()) {
        return false;
    }

    bool hasAutoKeying = false;
    std::list<ViewIdx> views = getViewsList();
    if (dimension.isAll()) {
        for (int i = 0; i < _imp->dimension; ++i) {
            if (view.isAll()) {
                for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
                    hasAutoKeying |= isAutoKeyingEnabledInternal(DimIdx(i), *it);
                }
            } else {
                ViewIdx view_i = getViewIdxFromGetSpec(ViewGetSpec(view.value()));
                hasAutoKeying |= isAutoKeyingEnabledInternal(DimIdx(i), view_i);
            }
        }
    } else {
        if ( ( dimension >= (int)_imp->curves.size() ) || (dimension < 0) ) {
            throw std::invalid_argument("KnobHelper::isAutoKeyingEnabled(): Dimension out of range");
        }
        if (view.isAll()) {
            for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
                hasAutoKeying |= isAutoKeyingEnabledInternal(DimIdx(dimension), *it);
            }
        } else {
            ViewIdx view_i = getViewIdxFromGetSpec(ViewGetSpec(view.value()));
            hasAutoKeying |= isAutoKeyingEnabledInternal(DimIdx(dimension), view_i);
        }
    }

    if (!hasAutoKeying) {
        return false;
    }

    // Finally return the value set to setAutoKeyingEnabled
    QMutexLocker k(&_imp->valueChangedBlockedMutex);
    return !_imp->autoKeyingDisabled;
} // isAutoKeyingEnabled

bool
KnobHelper::evaluateValueChangeInternal(DimSpec dimension,
                                        double time,
                                        ViewSetSpec view,
                                        ValueChangedReasonEnum reason,
                                        ValueChangedReasonEnum originalReason)
{
    AppInstancePtr app;
    KnobHolderPtr holder = getHolder();
    if (holder) {
        app = holder->getApp();
    }



    // If the change is not due to a timeline time change and the knob has a holder then queue the value change
    bool ret = false;
    if ( ( (originalReason != eValueChangedReasonTimeChanged) || evaluateValueChangeOnTimeChange() ) && holder ) {
        holder->beginChanges();
        KnobIPtr thisShared = shared_from_this();
        assert(thisShared);
        holder->appendValueChange(thisShared, dimension, time, view, originalReason, reason);

        // This will call knobChanged handler internally
        ret |= holder->endChanges();
    }

    // If the knob has no holder, still refresh its state
    if (!holder) {
        computeHasModifications();
        if (!isValueChangesBlocked()) {
            _signalSlotHandler->s_mustRefreshKnobGui(view, dimension, reason);
        }
        if ( !isListenersNotificationBlocked() ) {
            refreshListenersAfterValueChange(view, originalReason, dimension);
        }
        refreshAnimationLevel(view, dimension);
    }

    return ret;
}

bool
KnobHelper::evaluateValueChange(DimSpec dimension,
                                double time,
                                ViewSetSpec view,
                                ValueChangedReasonEnum reason)
{
    return evaluateValueChangeInternal(dimension, time, view, reason, reason);
}

void
KnobHelper::setAddNewLine(bool newLine)
{
    _imp->newLine = newLine;
}

bool
KnobHelper::isNewLineActivated() const
{
    return _imp->newLine;
}

void
KnobHelper::setAddSeparator(bool addSep)
{
    _imp->addSeparator = addSep;
}

bool
KnobHelper::isSeparatorActivated() const
{
    return _imp->addSeparator;
}

void
KnobHelper::setSpacingBetweenItems(int spacing)
{
    _imp->itemSpacing = spacing;
}

int
KnobHelper::getSpacingBetweenitems() const
{
    return _imp->itemSpacing;
}

std::string
KnobHelper::getInViewerContextLabel() const
{
    QMutexLocker k(&_imp->labelMutex);

    return _imp->inViewerContextLabel;
}

void
KnobHelper::setInViewerContextLabel(const QString& label)
{
    {
        QMutexLocker k(&_imp->labelMutex);

        _imp->inViewerContextLabel = label.toStdString();
    }
    _signalSlotHandler->s_inViewerContextLabelChanged();
}

std::string
KnobHelper::getInViewerContextIconFilePath(bool checked) const
{
    QMutexLocker k(&_imp->labelMutex);
    int idx = !checked ? 0 : 1;

    if ( !_imp->inViewerContextIconFilePath[idx].empty() ) {
        return _imp->inViewerContextIconFilePath[idx];
    }
    int otherIdx = !checked ? 1 : 0;

    return _imp->inViewerContextIconFilePath[otherIdx];
}

void
KnobHelper::setInViewerContextIconFilePath(const std::string& icon, bool checked)
{
    QMutexLocker k(&_imp->labelMutex);
    int idx = !checked ? 0 : 1;

    _imp->inViewerContextIconFilePath[idx] = icon;
}

void
KnobHelper::setInViewerContextCanHaveShortcut(bool haveShortcut)
{
    _imp->inViewerContextHasShortcut = haveShortcut;
}

bool
KnobHelper::getInViewerContextHasShortcut() const
{
    return _imp->inViewerContextHasShortcut;
}

void
KnobHelper::addInViewerContextShortcutsReference(const std::string& actionID)
{
    _imp->additionalShortcutsInTooltip.push_back(actionID);
}

const std::list<std::string>&
KnobHelper::getInViewerContextAdditionalShortcuts() const
{
    return _imp->additionalShortcutsInTooltip;
}

void
KnobHelper::setInViewerContextItemSpacing(int spacing)
{
    _imp->inViewerContextItemSpacing = spacing;
}

int
KnobHelper::getInViewerContextItemSpacing() const
{
    return _imp->inViewerContextItemSpacing;
}

void
KnobHelper::setInViewerContextLayoutType(ViewerContextLayoutTypeEnum layoutType)
{
    _imp->inViewerContextLayoutType = layoutType;
}

ViewerContextLayoutTypeEnum
KnobHelper::getInViewerContextLayoutType() const
{
    return _imp->inViewerContextLayoutType;
}

void
KnobHelper::setInViewerContextSecret(bool secret)
{
    {
        QMutexLocker k(&_imp->stateMutex);
        _imp->inViewerContextSecret = secret;
    }
    _signalSlotHandler->s_viewerContextSecretChanged();
}

bool
KnobHelper::getInViewerContextSecret() const
{
    QMutexLocker k(&_imp->stateMutex);

    return _imp->inViewerContextSecret;
}

void
KnobHelper::setEnabled(bool b, DimSpec dimension, ViewSetSpec view)
{
    {
        QMutexLocker k(&_imp->stateMutex);
        if (dimension.isAll()) {
            if (view.isAll()) {
                std::list<ViewIdx> views = getViewsList();
                for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
                    for (int i = 0; i < _imp->dimension; ++i) {
                        _imp->enabled[i][*it] = b;
                    }
                }
            } else {
                ViewIdx view_i = getViewIdxFromGetSpec(ViewGetSpec(view));
                for (int i = 0; i < _imp->dimension; ++i) {
                    _imp->enabled[i][view_i] = b;
                }
            }

        } else {
            if (view.isAll()) {
                std::list<ViewIdx> views = getViewsList();
                for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
                    _imp->enabled[dimension][*it] = b;
                }
            } else {
                ViewIdx view_i = getViewIdxFromGetSpec(ViewGetSpec(view));
                _imp->enabled[dimension][view_i] = b;

            }
        }
    }
    if (_signalSlotHandler) {
        _signalSlotHandler->s_enabledChanged();
    }
}


void
KnobHelper::setSecret(bool b)
{
    {
        QMutexLocker k(&_imp->stateMutex);
        if (_imp->IsSecret == b) {
            return;
        }
        _imp->IsSecret = b;
    }

    ///the knob was revealed , refresh its gui to the current time
    if (!b) {
        KnobHolderPtr holder = getHolder();
        if (holder) {
            AppInstancePtr app = holder->getApp();
            if (app) {
                onTimeChanged( false, app->getTimeLine()->currentFrame() );
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
    QMutexLocker k(&_imp->labelMutex);

    return _imp->label;
}

void
KnobHelper::setLabel(const std::string& label)
{
    {
        QMutexLocker k(&_imp->labelMutex);
        _imp->label = label;
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
        QMutexLocker k(&_imp->labelMutex);
        int idx = !checked ? 0 : 1;

        _imp->iconFilePath[idx] = iconFilePath;
    }
    if (alsoSetViewerUIIcon) {
        setInViewerContextIconFilePath(iconFilePath, checked);
    }
}

const std::string&
KnobHelper::getIconLabel(bool checked) const
{
    QMutexLocker k(&_imp->labelMutex);
    int idx = !checked ? 0 : 1;

    if ( !_imp->iconFilePath[idx].empty() ) {
        return _imp->iconFilePath[idx];
    }
    int otherIdx = !checked ? 1 : 0;

    return _imp->iconFilePath[otherIdx];
}

bool
KnobHelper::hasAnimation() const
{
    QMutexLocker k(&_imp->curvesMutex);
    QMutexLocker k2(&_imp->expressionMutex);
    for (int i = 0; i < getNDimensions(); ++i) {
        const PerViewCurveMap& curvesPerView = _imp->curves[i];
        for (PerViewCurveMap::const_iterator it = curvesPerView.begin(); it!=curvesPerView.end(); ++it) {
            if (it->second && it->second->getKeyFramesCount() > 0) {
                return true;
            }
        }
        const ExprPerViewMap& exprPerView = _imp->expressions[i];
        for (ExprPerViewMap::const_iterator it = exprPerView.begin(); it!=exprPerView.end(); ++it) {
            if (!it->second.originalExpression.empty()) {
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
    _imp->isAnimationEnabled = val;
}

bool
KnobHelper::isAnimationEnabled() const
{
    return canAnimate() && _imp->isAnimationEnabled;
}

void
KnobHelper::setName(const std::string & name,
                    bool throwExceptions)
{
    _imp->originalName = name;
    _imp->name = NATRON_PYTHON_NAMESPACE::makeNameScriptFriendly(name);
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
        ss << _imp->name;
        if (no > 1) {
            ss << no;
        }
        finalName = ss.str();
        if ( holder->getOtherKnobByName( finalName, shared_from_this() ) ) {
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
    _imp->name = finalName;
} // KnobHelper::setName

const std::string &
KnobHelper::getName() const
{
    return _imp->name;
}

const std::string &
KnobHelper::getOriginalName() const
{
    return _imp->originalName;
}

void
KnobHelper::resetParent()
{
    KnobIPtr parent = _imp->parentKnob.lock();

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
        _imp->parentKnob.reset();
    }
}

void
KnobHelper::setParentKnob(KnobIPtr knob)
{
    _imp->parentKnob = knob;
}

KnobIPtr
KnobHelper::getParentKnob() const
{
    return _imp->parentKnob.lock();
}

bool
KnobHelper::getIsSecret() const
{
    QMutexLocker k(&_imp->stateMutex);

    return _imp->IsSecret;
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
KnobHelper::isEnabled(DimIdx dimension, ViewGetSpec view) const
{
    if (dimension < 0 || dimension >= getNDimensions()) {
        throw std::invalid_argument("KnobHelper::isEnabled: dimension out of range");
    }

    // Check the enabled bit, if true check link state
    {
        QMutexLocker k(&_imp->stateMutex);
        ViewIdx view_i = getViewIdxFromGetSpec(view);
        PerViewEnabledMap::const_iterator foundView = _imp->enabled[dimension].find(view_i);
        if (foundView == _imp->enabled[dimension].end()) {
            return false;
        }
        if (!foundView->second) {
            return false;
        }
    }

    // Check expression
    std::string hasExpr = getExpression(dimension, view);
    if (!hasExpr.empty()) {
        return false;
    }

    // check hard link
    MasterKnobLink linkData;
    ViewIdx view_i = getViewIdxFromGetSpec(view);
    if (getMaster(dimension, view_i, &linkData)) {
        return false;
    }

    return true;
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
        QMutexLocker k(&_imp->stateMutex);
        _imp->evaluateOnChange = b;
    }
    if (_signalSlotHandler) {
        _signalSlotHandler->s_evaluateOnChangeChanged(b);
    }
}

bool
KnobHelper::getIsPersistent() const
{
    return _imp->IsPersistent;
}

void
KnobHelper::setIsPersistent(bool b)
{
    _imp->IsPersistent = b;
}

void
KnobHelper::setCanUndo(bool val)
{
    _imp->CanUndo = val;
}

bool
KnobHelper::getCanUndo() const
{
    return _imp->CanUndo;
}

void
KnobHelper::setIsMetadataSlave(bool slave)
{
    _imp->isClipPreferenceSlave = slave;
}

bool
KnobHelper::getIsMetadataSlave() const
{
    return _imp->isClipPreferenceSlave;
}

bool
KnobHelper::getEvaluateOnChange() const
{
    QMutexLocker k(&_imp->stateMutex);

    return _imp->evaluateOnChange;
}

void
KnobHelper::setHintToolTip(const std::string & hint)
{
    _imp->tooltipHint = hint;
    if (_signalSlotHandler) {
        _signalSlotHandler->s_helpChanged();
    }
}

const std::string &
KnobHelper::getHintToolTip() const
{
    return _imp->tooltipHint;
}

bool
KnobHelper::isHintInMarkdown() const
{
    return _imp->hintIsMarkdown;
}

void
KnobHelper::setHintIsMarkdown(bool b)
{
    _imp->hintIsMarkdown = b;
}

void
KnobHelper::setCustomInteract(const OfxParamOverlayInteractPtr & interactDesc)
{
    assert( QThread::currentThread() == qApp->thread() );
    _imp->customInteract = interactDesc;
}

OfxParamOverlayInteractPtr KnobHelper::getCustomInteract() const
{
    assert( QThread::currentThread() == qApp->thread() );

    return _imp->customInteract;
}

void
KnobHelper::swapOpenGLBuffers()
{
    KnobGuiIPtr hasGui = getKnobGuiPointer();

    if (hasGui) {
        hasGui->swapOpenGLBuffers();
    }
}

void
KnobHelper::redraw()
{
    KnobGuiIPtr hasGui = getKnobGuiPointer();

    if (hasGui) {
        hasGui->redraw();
    }
}

void
KnobHelper::getOpenGLContextFormat(int* depthPerComponents, bool* hasAlpha) const
{
    KnobGuiIPtr hasGui = getKnobGuiPointer();
    if (hasGui) {
        hasGui->getOpenGLContextFormat(depthPerComponents, hasAlpha);
    } else {
        *depthPerComponents = 8;
        *hasAlpha = false;
    }
}

void
KnobHelper::getViewportSize(double &width,
                            double &height) const
{
    KnobGuiIPtr hasGui = getKnobGuiPointer();

    if (hasGui) {
        hasGui->getViewportSize(width, height);
    } else {
        width = 0;
        height = 0;
    }
}

void
KnobHelper::getPixelScale(double & xScale,
                          double & yScale) const
{
    KnobGuiIPtr hasGui = getKnobGuiPointer();

    if (hasGui) {
        hasGui->getPixelScale(xScale, yScale);
    } else {
        xScale = 0;
        yScale = 0;
    }
}

void
KnobHelper::getBackgroundColour(double &r,
                                double &g,
                                double &b) const
{
    KnobGuiIPtr hasGui = getKnobGuiPointer();

    if (hasGui) {
        hasGui->getBackgroundColour(r, g, b);
    } else {
        r = 0;
        g = 0;
        b = 0;
    }
}

int
KnobHelper::getWidgetFontHeight() const
{
    KnobGuiIPtr hasGui = getKnobGuiPointer();

    if (hasGui) {
        return hasGui->getWidgetFontHeight();
    }

    return 0;
}

int
KnobHelper::getStringWidthForCurrentFont(const std::string& string) const
{
    KnobGuiIPtr hasGui = getKnobGuiPointer();

    if (hasGui) {
        return hasGui->getStringWidthForCurrentFont(string);
    }

    return 0;
}

void
KnobHelper::toWidgetCoordinates(double *x,
                                double *y) const
{
    KnobGuiIPtr hasGui = getKnobGuiPointer();

    if (hasGui) {
        hasGui->toWidgetCoordinates(x, y);
    }
}

void
KnobHelper::toCanonicalCoordinates(double *x,
                                   double *y) const
{
    KnobGuiIPtr hasGui = getKnobGuiPointer();

    if (hasGui) {
        hasGui->toCanonicalCoordinates(x, y);
    }
}

RectD
KnobHelper::getViewportRect() const
{
    KnobGuiIPtr hasGui = getKnobGuiPointer();

    if (hasGui) {
        return hasGui->getViewportRect();
    } else {
        return RectD();
    }
}

void
KnobHelper::getCursorPosition(double& x,
                              double& y) const
{
    KnobGuiIPtr hasGui = getKnobGuiPointer();

    if (hasGui) {
        return hasGui->getCursorPosition(x, y);
    } else {
        x = 0;
        y = 0;
    }
}

void
KnobHelper::saveOpenGLContext()
{
    KnobGuiIPtr hasGui = getKnobGuiPointer();

    if (hasGui) {
        hasGui->saveOpenGLContext();
    }
}

void
KnobHelper::restoreOpenGLContext()
{
    KnobGuiIPtr hasGui = getKnobGuiPointer();

    if (hasGui) {
        hasGui->restoreOpenGLContext();
    }
}

bool
KnobI::shouldDrawOverlayInteract() const
{
    // If there is one dimension disabled, don't draw it
    int nDims = getNDimensions();
    for (int i = 0; i < nDims; ++i) {
        if (!isEnabled(DimIdx(i))) {
            return false;
        }
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
    return page->isEnabled(DimIdx(0));
}

void
KnobHelper::setOfxParamHandle(void* ofxParamHandle)
{
    assert( QThread::currentThread() == qApp->thread() );
    _imp->ofxParamHandle = ofxParamHandle;
}

void*
KnobHelper::getOfxParamHandle() const
{
    assert( QThread::currentThread() == qApp->thread() );

    return _imp->ofxParamHandle;
}

bool
KnobHelper::isMastersPersistenceIgnored() const
{
    QReadLocker l(&_imp->mastersMutex);
    return _imp->ignoreMasterPersistence;
}

void
KnobHelper::setMastersPersistenceIgnore(bool ignored)
{
    QWriteLocker l(&_imp->mastersMutex);
    _imp->ignoreMasterPersistence = ignored;
}


bool
KnobHelper::slaveToInternal(const KnobIPtr & otherKnob, DimIdx thisDimension, DimIdx otherDimension, ViewIdx view, ViewIdx otherView)
{
    if (!otherKnob || (otherKnob.get() == this && (thisDimension == otherDimension) && (view == otherView))) {
        return false;
    }
    if (thisDimension < 0 || thisDimension >= (int)_imp->masters.size()) {
        return false;
    }


    {
        MasterKnobLink otherLinkData;
        if (otherKnob->getMaster(otherDimension, otherView, &otherLinkData)) {
            if (otherLinkData.masterKnob.lock().get() == this) {
                // Avoid recursion
                return false;
            }
        }
    }

    {

        QWriteLocker l(&_imp->mastersMutex);
        MasterKnobLink& linkData = _imp->masters[thisDimension][view];
        if ( linkData.masterKnob.lock() ) {
            // Already slaved
            return false;
        }

        linkData.masterKnob = otherKnob;
        linkData.masterDimension = otherDimension;
        linkData.masterView = otherView;
    }


    // Copy the master state for the dimension/view
    bool hasChanged = copyKnob(otherKnob, view, thisDimension, otherView, otherDimension, 0 /*range*/, 0 /*offset*/);
    (void)hasChanged;

    // Disable the slaved knob to the user
    // Do not disable buttons when they are slaved

    KnobButton* isBtn = dynamic_cast<KnobButton*>(this);
    if (!isBtn) {
        setEnabled(false, thisDimension);
    }

    if (!isMastersPersistenceIgnored()) {
        // Force the parameter to be persistent if it's not, otherwise thestate will not be saved
        setIsPersistent(true);
    }

    _signalSlotHandler->s_knobSlaved(thisDimension, view, true /* slaved*/);


    // Register this as a listener of the master
    otherKnob->addListener( false, thisDimension, otherDimension, view, otherView, shared_from_this() );


    return true;
} // KnobHelper::slaveToInternal



bool
KnobHelper::slaveTo(const KnobIPtr & otherKnob, DimSpec thisDimension, DimSpec otherDimension, ViewSetSpec thisView, ViewSetSpec otherView)
{

    assert((thisDimension.isAll() && otherDimension.isAll()) || (!thisDimension.isAll() && !otherDimension.isAll()));
    assert((thisView.isAll() && otherView.isAll()) || (thisView.isViewIdx() && otherView.isViewIdx()));

    if ((!thisDimension.isAll() || !otherDimension.isAll()) && (thisDimension.isAll() || otherDimension.isAll())) {
        throw std::invalid_argument("KnobHelper::slaveTo: invalid dimension argument");
    }
    if ((!thisView.isAll() || !otherView.isAll()) && (!thisView.isViewIdx() || !otherView.isViewIdx())) {
        throw std::invalid_argument("KnobHelper::slaveTo: invalid view argument");
    }
    if (!otherKnob || (otherKnob.get() == this && (thisDimension == otherDimension || thisDimension.isAll() || otherDimension.isAll()) && (thisView == otherView || thisView.isAll() || otherView.isAll()))) {
        return false;
    }
    bool ok = false;
    beginChanges();
    std::list<ViewIdx> views = otherKnob->getViewsList();
    if (thisDimension.isAll()) {
        int dimMin = std::min(getNDimensions(), otherKnob->getNDimensions());
        for (int i = 0; i < dimMin; ++i) {
            if (thisView.isAll()) {
                for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
                    ok |= slaveToInternal(otherKnob, DimIdx(i), DimIdx(i), *it, *it);
                }
            } else {
                ok |= slaveToInternal(otherKnob, DimIdx(i), DimIdx(i), ViewIdx(thisView.value()), ViewIdx(otherView.value()));
            }
        }
    } else {
        if ( ( thisDimension >= getNDimensions() ) || (thisDimension < 0) || (otherDimension >= otherKnob->getNDimensions()) || (otherDimension < 0)) {
            throw std::invalid_argument("KnobHelper::slaveTo(): Dimension out of range");
        }
        if (thisView.isAll()) {
            for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
                ok |= slaveToInternal(otherKnob, DimIdx(thisDimension), DimIdx(otherDimension), *it, *it);
            }
        } else {
            ok |= slaveToInternal(otherKnob, DimIdx(thisDimension), DimIdx(otherDimension), ViewIdx(thisView.value()), ViewIdx(otherView.value()));
        }
    }
    evaluateValueChange(thisDimension, getCurrentTime(), thisView, eValueChangedReasonNatronInternalEdited);
    endChanges();
    return ok;
} // slaveTo


void
KnobHelper::unSlave(DimSpec dimension,
                    ViewSetSpec view,
                    bool copyState)
{
    beginChanges();
    std::list<ViewIdx> views = getViewsList();
    if (dimension.isAll()) {
        for (int i = 0; i < _imp->dimension; ++i) {
            if (view.isAll()) {
                for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
                    unSlaveInternal(DimIdx(i), *it, copyState);
                }
            } else {
                ViewIdx view_i = getViewIdxFromGetSpec(ViewGetSpec(view.value()));
                unSlaveInternal(DimIdx(i), view_i, copyState);
            }
        }
    } else {
        if ( ( dimension >= getNDimensions() ) || (dimension < 0) ) {
            throw std::invalid_argument("KnobHelper::unSlave(): Dimension out of range");
        }
        if (view.isAll()) {
            for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
                unSlaveInternal(DimIdx(dimension), *it, copyState);
            }
        } else {
            ViewIdx view_i = getViewIdxFromGetSpec(ViewGetSpec(view.value()));
            unSlaveInternal(DimIdx(dimension), view_i, copyState);
        }
    }
    endChanges();
} // unSlave


bool
KnobHelper::getMaster(DimIdx dimension, ViewIdx view, MasterKnobLink* masterLink) const
{
    if (!masterLink) {
        return false;
    }
    if (dimension < 0 || dimension >= (int)_imp->masters.size()) {
        return false;
    }
    QReadLocker l(&_imp->mastersMutex);
    PerViewMasterLink::const_iterator foundView = _imp->masters[dimension].find(view);
    if (foundView == _imp->masters[dimension].end()) {
        return false;
    }
    *masterLink = foundView->second;
    return bool(foundView->second.masterKnob.lock());
}

void
KnobHelper::resetMaster(DimIdx dimension, ViewIdx view)
{
    if (dimension < 0 || dimension >= (int)_imp->masters.size()) {
        throw std::invalid_argument("KnobHelper::resetMaster: dimension out of range");
    }
    QWriteLocker l(&_imp->mastersMutex);
    PerViewMasterLink::iterator foundView = _imp->masters[dimension].find(view);
    if (foundView == _imp->masters[dimension].end()) {
        return;
    }
    foundView->second.masterKnob.reset();
}

bool
KnobHelper::isSlave(DimIdx dimension, ViewIdx view) const
{
    if (dimension < 0 || dimension >= (int)_imp->masters.size()) {
        return false;
    }
    QReadLocker l(&_imp->mastersMutex);
    PerViewMasterLink::const_iterator foundView = _imp->masters[dimension].find(view);
    if (foundView == _imp->masters[dimension].end()) {
        return false;
    }
    return bool( foundView->second.masterKnob.lock() );
}

void
KnobHelper::refreshAnimationLevelInternal(ViewIdx view, DimIdx dimension)
{
    AnimationLevelEnum level = eAnimationLevelNone;

    std::string expr = getExpression(dimension, view);
    if (!expr.empty()) {
        level = eAnimationLevelExpression;
    } else {

        CurvePtr c;
        if (canAnimate() && isAnimationEnabled()) {
            c = getCurve(view, dimension);
        }

        if (!c || !c->isAnimated()) {
            level = eAnimationLevelNone;
        } else {
            double time = getCurrentTime();
            KeyFrame kf;
            int nKeys = c->getNKeyFramesInRange(time, time + 1);
            if (nKeys > 0) {
                level = eAnimationLevelOnKeyframe;
            } else {
                level = eAnimationLevelInterpolatedValue;
            }
        }
    }

    bool changed = false;
    {
        QMutexLocker l(&_imp->animationLevelMutex);
        assert( dimension < (int)_imp->animationLevel.size() );
        AnimationLevelEnum& curValue = _imp->animationLevel[dimension][view];
        if (curValue != level) {
            curValue = level;
            changed = true;
        }
    }

    if (changed) {
        KnobGuiIPtr hasGui = getKnobGuiPointer();
        if (hasGui && !hasGui->isGuiFrozenForPlayback()) {
            _signalSlotHandler->s_animationLevelChanged(view, dimension );
        }
    }
}

void
KnobHelper::refreshAnimationLevel(ViewSetSpec view,
                                  DimSpec dimension)
{
    if (!canAnimate()) {
        return;
    }
    if (!getHolder() || !getHolder()->getApp()) {
        return;
    }
    KnobHolderPtr holder = getHolder();
    std::list<ViewIdx> views = getViewsList();
    if (dimension.isAll()) {
        for (int i = 0; i < _imp->dimension; ++i) {
            if (view.isAll()) {
                for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
                    refreshAnimationLevelInternal(*it, DimIdx(i));
                }
            } else {
                ViewIdx view_i = getViewIdxFromGetSpec(ViewGetSpec(view.value()));
                refreshAnimationLevelInternal(view_i, DimIdx(i));
            }
        }
    } else {
        if ( ( dimension >= (int)_imp->animationLevel.size() ) || (dimension < 0) ) {
            throw std::invalid_argument("KnobHelper::refreshAnimationLevel(): Dimension out of range");
        }
        if (view.isAll()) {
            for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
                refreshAnimationLevelInternal(*it, DimIdx(dimension));
            }
        } else {
            ViewIdx view_i = getViewIdxFromGetSpec(ViewGetSpec(view.value()));
            refreshAnimationLevelInternal(view_i, DimIdx(dimension));
        }
    }
}

AnimationLevelEnum
KnobHelper::getAnimationLevel(DimIdx dimension, ViewGetSpec view) const
{
    // If the knob is slaved to another knob, returns the other knob value
    ViewIdx view_i = getViewIdxFromGetSpec(view);
    MasterKnobLink linkdata;
    if (getMaster(dimension, view_i, &linkdata)) {
        KnobIPtr masterKnob = linkdata.masterKnob.lock();
        if (masterKnob) {
            return masterKnob->getAnimationLevel(linkdata.masterDimension, linkdata.masterView);
        }
    }


    QMutexLocker l(&_imp->animationLevelMutex);
    if ( dimension < 0 || dimension >= (int)_imp->animationLevel.size() ) {
        throw std::invalid_argument("Knob::getAnimationLevel(): Dimension out of range");
    }
    PerViewAnimLevelMap::const_iterator foundView = _imp->animationLevel[dimension].find(view_i);
    if (foundView == _imp->animationLevel[dimension].end()) {
        return eAnimationLevelNone;
    }

    return foundView->second;
}

bool
KnobHelper::getKeyFrameTime(ViewGetSpec view,
                            int index,
                            DimIdx dimension,
                            double* time) const
{
    if ( dimension < 0 || dimension >= (int)_imp->curves.size() ) {
        throw std::invalid_argument("Knob::getKeyFrameTime(): Dimension out of range");
    }

    CurvePtr curve = getCurve(view, dimension);
    if (!curve) {
        return false;
    }

    KeyFrame kf;
    bool ret = curve->getKeyFrameWithIndex(index, &kf);
    if (ret) {
        *time = kf.getTime();
    }

    return ret;
}

bool
KnobHelper::getLastKeyFrameTime(ViewGetSpec view,
                                DimIdx dimension,
                                double* time) const
{
    if ( dimension < 0 || dimension >= (int)_imp->curves.size() ) {
        throw std::invalid_argument("Knob::getLastKeyFrameTime(): Dimension out of range");
    }
    if ( !canAnimate() || !isAnimated(dimension, view) ) {
        return false;
    }

    CurvePtr curve = getCurve(view, dimension);  //< getCurve will return the master's curve if any
    if (!curve) {
        return false;
    }
    *time = curve->getMaximumTimeCovered();

    return true;
}

bool
KnobHelper::getFirstKeyFrameTime(ViewGetSpec view,
                                 DimIdx dimension,
                                 double* time) const
{
    return getKeyFrameTime(view, 0, dimension, time);
}

int
KnobHelper::getKeyFramesCount(ViewGetSpec view,
                              DimIdx dimension) const
{
    if ( !canAnimate() || !isAnimated(dimension, view) ) {
        return 0;
    }

    CurvePtr curve = getCurve(view, dimension);  //< getCurve will return the master's curve if any
    if (!curve) {
        return 0;
    }

    return curve->getKeyFramesCount();
}

bool
KnobHelper::getNearestKeyFrameTime(ViewGetSpec view,
                                   DimIdx dimension,
                                   double time,
                                   double* nearestTime) const
{
    if ( dimension < 0 || dimension >= (int)_imp->curves.size() ) {
        throw std::invalid_argument("Knob::getNearestKeyFrameTime(): Dimension out of range");
    }
    if ( !canAnimate() || !isAnimated(dimension, view) ) {
        return false;
    }


    CurvePtr curve = getCurve(view, dimension);  //< getCurve will return the master's curve if any
    if (!curve) {
        return false;
    }

    KeyFrame kf;
    bool ret = curve->getNearestKeyFrameWithTime(time, &kf);
    if (ret) {
        *nearestTime = kf.getTime();
    }

    return ret;
}

int
KnobHelper::getKeyFrameIndex(ViewGetSpec view,
                             DimIdx dimension,
                             double time) const
{
    if ( dimension < 0 || dimension >= (int)_imp->curves.size() ) {
        throw std::invalid_argument("Knob::getKeyFrameIndex(): Dimension out of range");
    }
    if ( !canAnimate() || !isAnimated(dimension, view) ) {
        return -1;
    }

    CurvePtr curve = getCurve(view, dimension);  //< getCurve will return the master's curve if any
    if (!curve) {
        return -1;
    }

    return curve->keyFrameIndex(time);
}

void
KnobHelper::refreshListenersAfterValueChange(ViewSetSpec view,
                                             ValueChangedReasonEnum reason,
                                             DimSpec dimension)
{
    // Call evaluateValueChange on every knob that is listening to this view/dimension
    ListenerDimsMap listeners;
    getListeners(listeners);

    if ( listeners.empty() ) {
        return;
    }

    double time = getCurrentTime();
    for (ListenerDimsMap::iterator it = listeners.begin(); it != listeners.end(); ++it) {
        KnobHelper* slaveKnob = dynamic_cast<KnobHelper*>( it->first.lock().get() );
        if (!slaveKnob || slaveKnob == this) {
            continue;
        }

        if (it->second.empty()) {
            continue;
        }

        // List of dimension and views listening to the given view/dimension in argument
        DimensionViewPairSet dimViewPairToEvaluate;
        for (ListenerLinkList::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
            if ((dimension.isAll() || it2->targetDim == dimension) && (view.isAll() || (view.isViewIdx() && it2->targetView == view) || (view.isCurrent() && it2->targetView == getCurrentView()))) {
                DimensionViewPair p;
                p.view = it2->targetView;
                p.dimension = it2->targetDim;
                dimViewPairToEvaluate.insert(p);
            }
        }


        if ( dimViewPairToEvaluate.empty() ) {
            continue;
        }

        // If multiple dimensions are listening to this knob at the given view/dimension, then assume all dimensions have changed in evaluateValueChange
        if (dimViewPairToEvaluate.size() > 1) {
            slaveKnob->beginChanges();
        }

        for (DimensionViewPairSet::iterator it = dimViewPairToEvaluate.begin(); it != dimViewPairToEvaluate.end(); ++it) {
            slaveKnob->clearExpressionsResults(it->dimension, it->view);
            slaveKnob->evaluateValueChangeInternal(it->dimension, time, it->view, eValueChangedReasonSlaveRefresh, reason);
        }

        if (dimViewPairToEvaluate.size() > 1) {
            slaveKnob->endChanges();
        }


        /*if (mustClone) {
            ///We still want to clone the master's dimension because otherwise we couldn't edit the curve e.g in the curve editor
            ///For example we use it for roto knobs where selected beziers have their knobs slaved to the gui knobs
            slaveKnob->clone(shared_from_this(), dimChanged);
        }*/



        // Call recursively
        /*if ( !slaveKnob->isListenersNotificationBlocked() ) {
            slaveKnob->refreshListenersAfterValueChange(viewsChanged, reason, dimChanged);
        }*/
    } // for all listeners
} // KnobHelper::refreshListenersAfterValueChange

bool
KnobHelper::cloneExpressionInternal(const KnobIPtr& other, ViewIdx view, ViewIdx otherView, DimIdx dimension, DimIdx otherDimension)
{
    std::string otherExpr = other->getExpression(otherDimension, otherView);
    bool otherHasRet = other->isExpressionUsingRetVariable(otherView, otherDimension);

    Expr thisExpr;
    {
        QMutexLocker k(&_imp->expressionMutex);
        thisExpr = _imp->expressions[dimension][view];
    }

    if ( !otherExpr.empty() && ( (otherExpr != thisExpr.originalExpression) || (otherHasRet != thisExpr.hasRet) ) ) {
        try {
            setExpression(dimension, view, otherExpr, otherHasRet, false);
        } catch (...) {
            // Ignore errors
        }
        return true;
    }
    return false;
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

    if (hasChanged) {
        cloneExpressionsResults(other, view, otherView, dimension, otherDimension);
    }

    return hasChanged;
}


//The knob in parameter will "listen" to this knob. Hence this knob is a dependency of the knob in parameter.
void
KnobHelper::addListener(const bool isExpression,
                        const DimIdx listenerDimension,
                        const DimIdx listenedToDimension,
                        const ViewIdx listenerView,
                        const ViewIdx listenedToView,
                        const KnobIPtr& listener)
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

    listenerIsHelper->getHolder()->onKnobSlaved(listener, thisShared, listenerDimension, listenerView, true );

    // If this knob is already a dependency of the knob, add it to its dimension vector
    {
        QWriteLocker l(&_imp->mastersMutex);
        ListenerLinkList& linksList = _imp->listeners[listener];
        ListenerLink link;
        link.isExpr = isExpression;
        link.listenerDimension = listenerDimension;
        link.listenerView = listenerView;
        link.targetDim = listenedToDimension;
        link.targetView = listenedToView;
        linksList.push_back(link);
    }

    if (isExpression) {
        QMutexLocker k(&listenerIsHelper->_imp->expressionMutex);
        Expr& expr = listenerIsHelper->_imp->expressions[listenerDimension][listenerView];
        Expr::Dependency d;
        d.knob = thisShared;
        d.dimension = listenedToDimension;
        d.view = listenedToView;
        expr.dependencies.push_back(d);
    }
} // addListener

void
KnobHelper::removeListener(const KnobIPtr& listener,
                           DimIdx listenerDimension,
                           ViewIdx listenerView)
{
    KnobHelperPtr listenerHelper = boost::dynamic_pointer_cast<KnobHelper>(listener);
    assert(listenerHelper);

    QWriteLocker l(&_imp->mastersMutex);
    ListenerDimsMap::iterator foundListener = _imp->listeners.find(listener);
    if (foundListener != _imp->listeners.end()) {

        for (ListenerLinkList::iterator it = foundListener->second.begin(); it != foundListener->second.end(); ++it) {
            if (it->listenerDimension == listenerDimension && it->listenerView == listenerView) {
                foundListener->second.erase(it);
                break;
            }
        }
        if (foundListener->second.empty()) {
            _imp->listeners.erase(foundListener);
        }
    }
}

void
KnobHelper::getListeners(KnobI::ListenerDimsMap & listeners) const
{
    QReadLocker l(&_imp->mastersMutex);

    listeners = _imp->listeners;
}

double
KnobHelper::getCurrentTime() const
{
    KnobHolderPtr holder = getHolder();

    return holder && holder->getApp() ? holder->getCurrentTime() : 0;
}

ViewIdx
KnobHelper::getCurrentView() const
{
    KnobHolderPtr holder = getHolder();

    return ( holder && holder->getApp() ) ? holder->getCurrentView() : ViewIdx(0);
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
KnobHelper::random(double time,
                   unsigned int seed) const
{
    randomSeed(time, seed);

    return random();
}

double
KnobHelper::random(double min,
                   double max) const
{
    QMutexLocker k(&_imp->lastRandomHashMutex);

    _imp->lastRandomHash = hashFunction(_imp->lastRandomHash);

    return ( (double)_imp->lastRandomHash / (double)0x100000000LL ) * (max - min)  + min;
}

int
KnobHelper::randomInt(double time,
                      unsigned int seed) const
{
    randomSeed(time, seed);

    return randomInt();
}

int
KnobHelper::randomInt(int min,
                      int max) const
{
    return (int)random( (double)min, (double)max );
}

struct alias_cast_float
{
    alias_cast_float()
        : raw(0)
    {
    };                          // initialize to 0 in case sizeof(T) < 8

    union
    {
        U32 raw;
        float data;
    };
};

void
KnobHelper::randomSeed(double time,
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

    QMutexLocker k(&_imp->lastRandomHashMutex);
    _imp->lastRandomHash = hash32;
}

bool
KnobHelper::hasModifications() const
{
    QMutexLocker k(&_imp->hasModificationsMutex);

    for (int i = 0; i < _imp->dimension; ++i) {
        for (PerViewHasModificationMap::const_iterator it = _imp->hasModifications[i].begin(); it != _imp->hasModifications[i].end(); ++it) {
            if (it->second) {
                return true;
            }
        }
    }

    return false;
}

RenderValuesCachePtr
KnobHelper::getHolderRenderValuesCache(double* currentTime, ViewIdx* currentView) const
{
    KnobHolderPtr holder = getHolder();
    if (!holder) {
        return RenderValuesCachePtr();
    }
    EffectInstancePtr isEffect = toEffectInstance(holder);
    KnobTableItemPtr isTableItem = toKnobTableItem(holder);
    if (isTableItem) {
        KnobItemsTablePtr model = isTableItem->getModel();
        if (!model) {
            return RenderValuesCachePtr();
        }
        NodePtr modelNode = model->getNode();
        if (!modelNode) {
            return RenderValuesCachePtr();
        }

        isEffect = modelNode->getEffectInstance();
    }
    if (!isEffect) {
        return RenderValuesCachePtr();
    }
    return isEffect->getRenderValuesCacheTLS(currentTime, currentView);
}

bool
KnobHelper::hasModifications(DimIdx dimension) const
{
    if ( (dimension < 0) || (dimension >= _imp->dimension) ) {
        throw std::invalid_argument("KnobHelper::hasModifications: Dimension out of range");
    }
    QMutexLocker k(&_imp->hasModificationsMutex);
    for (PerViewHasModificationMap::const_iterator it = _imp->hasModifications[dimension].begin(); it != _imp->hasModifications[dimension].end(); ++it) {
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
    if ( (dimension < 0) || (dimension >= _imp->dimension) ) {
        throw std::invalid_argument("KnobHelper::setHasModifications: Dimension out of range");
    }

    if (lock) {
        _imp->hasModificationsMutex.lock();
    } else {
        assert( !_imp->hasModificationsMutex.tryLock() );
    }

    bool ret = false;
    PerViewHasModificationMap::iterator foundView = _imp->hasModifications[dimension].find(view);
    if (foundView != _imp->hasModifications[dimension].end()) {
        ret = foundView->second != value;
        foundView->second = value;
    }

    if (lock) {
        _imp->hasModificationsMutex.unlock();
    }

    return ret;
}

void
KnobHolder::getUserPages(std::list<KnobPagePtr>& userPages) const {
    const KnobsVec& knobs = getKnobs();

    for (KnobsVec::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        if ( (*it)->isUserKnob() ) {
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
                                    bool isUserKnob)
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
        KnobBoolPtr newKnob = otherHolder->createBoolKnob(newScriptName, newLabel, isUserKnob);
        output = newKnob;
    } else if (isInt) {
        KnobIntPtr newKnob = otherHolder->createIntKnob(newScriptName, newLabel, getNDimensions(), isUserKnob);
        newKnob->setRangeAcrossDimensions( isInt->getMinimums(), isInt->getMaximums() );
        newKnob->setDisplayRangeAcrossDimensions( isInt->getDisplayMinimums(), isInt->getDisplayMaximums() );
        if ( isInt->isSliderDisabled() ) {
            newKnob->disableSlider();
        }
        output = newKnob;
    } else if (isDbl) {
        KnobDoublePtr newKnob = otherHolder->createDoubleKnob(newScriptName, newLabel, getNDimensions(), isUserKnob);
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
        KnobChoicePtr newKnob = otherHolder->createChoiceKnob(newScriptName, newLabel, isUserKnob);
        if (duplicateType != eDuplicateKnobTypeAlias) {
            newKnob->populateChoices( isChoice->getEntries(), isChoice->getEntriesHelp() );
        }
        output = newKnob;
    } else if (isColor) {
        KnobColorPtr newKnob = otherHolder->createColorKnob(newScriptName, newLabel, getNDimensions(), isUserKnob);
        newKnob->setRangeAcrossDimensions( isColor->getMinimums(), isColor->getMaximums() );
        newKnob->setDisplayRangeAcrossDimensions( isColor->getDisplayMinimums(), isColor->getDisplayMaximums() );
        output = newKnob;
    } else if (isString) {
        KnobStringPtr newKnob = otherHolder->createStringKnob(newScriptName, newLabel, isUserKnob);
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
        KnobFilePtr newKnob = otherHolder->createFileKnob(newScriptName, newLabel, isUserKnob);
        newKnob->setDialogType(isFile->getDialogType());
        newKnob->setDialogFilters(isFile->getDialogFilters());
        output = newKnob;
    } else if (isPath) {
        KnobPathPtr newKnob = otherHolder->createPathKnob(newScriptName, newLabel, isUserKnob);
        if ( isPath->isMultiPath() ) {
            newKnob->setMultiPath(true);
        }
        output = newKnob;
    } else if (isGrp) {
        KnobGroupPtr newKnob = otherHolder->createGroupKnob(newScriptName, newLabel, isUserKnob);
        if ( isGrp->isTab() ) {
            newKnob->setAsTab();
        }
        output = newKnob;
    } else if (isPage) {
        KnobPagePtr newKnob = otherHolder->createPageKnob(newScriptName, newLabel, isUserKnob);
        output = newKnob;
    } else if (isBtn) {
        KnobButtonPtr newKnob = otherHolder->createButtonKnob(newScriptName, newLabel, isUserKnob);
        KnobButton* thisKnobButton = dynamic_cast<KnobButton*>(this);
        newKnob->setCheckable(thisKnobButton->getIsCheckable());
        output = newKnob;
    } else if (isParametric) {
        KnobParametricPtr newKnob = otherHolder->createParametricKnob(newScriptName, newLabel, isParametric->getNDimensions(), isUserKnob);
        output = newKnob;
        newKnob->setRangeAcrossDimensions( isParametric->getMinimums(), isParametric->getMaximums() );
        newKnob->setDisplayRangeAcrossDimensions( isParametric->getDisplayMinimums(), isParametric->getDisplayMaximums() );
    }
    if (!output) {
        return KnobIPtr();
    }
    for (int i = 0; i < getNDimensions(); ++i) {
        output->setDimensionName( DimIdx(i), getDimensionName(DimIdx(i)) );
    }
    output->setName(newScriptName, true);
    output->cloneDefaultValues( shared_from_this() );
    output->copyKnob( shared_from_this() );
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
    if (isUserKnob && otherIsEffect) {
        otherIsEffect->getNode()->declarePythonKnobs();
    }
    switch (duplicateType) {
        case KnobI::eDuplicateKnobTypeAlias: {
            setKnobAsAliasOfThis(output, true);
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
                    clearExpression(DimSpec::all(), ViewSetSpec::all(), true);
                    setExpression(DimSpec::all(), ViewSetSpec::all(), script, false /*hasRetVariable*/, false /*failIfInvalid*/);
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


bool
KnobHelper::setKnobAsAliasOfThis(const KnobIPtr& master,
                                 bool doAlias)
{
    //Sanity check
    if ( !master || ( master->getNDimensions() != getNDimensions() ) ||
         ( master->typeName() != typeName() ) ) {
        return false;
    }

    /*
       For choices, copy exactly the menu entries because they have to be the same
     */
    if (doAlias) {
        master->onKnobAboutToAlias( shared_from_this() );
    }
    beginChanges();
    unSlave(DimSpec::all(), ViewSetSpec::all(), false);

    // Don't save the slave link
    setMastersPersistenceIgnore(doAlias);

    for (int i = 0; i < getNDimensions(); ++i) {
        if (doAlias) {
            //master->clone(this, i);
            unSlave(DimIdx(i), ViewSetSpec::all(), false);
            bool ok = slaveTo(master);
            assert(ok);
            Q_UNUSED(ok);
        }
    }
    for (int i = 0; i < getNDimensions(); ++i) {
        master->setDimensionName( DimIdx(i), getDimensionName(DimIdx(i)) );
    }
    handleSignalSlotsForAliasLink(master, doAlias);

    endChanges();

    QWriteLocker k(&_imp->mastersMutex);
    if (doAlias) {
        _imp->slaveForAlias = master;
    } else {
        _imp->slaveForAlias.reset();
    }
    master->computeHasModifications();

    return true;
}

KnobIPtr
KnobHelper::getAliasMaster()  const
{
    QReadLocker k(&_imp->mastersMutex);

    return _imp->slaveForAlias.lock();
}

void
KnobHelper::getAllExpressionDependenciesRecursive(std::set<NodePtr >& nodes) const
{
    std::set<KnobIPtr> deps;
    {
        QMutexLocker k(&_imp->expressionMutex);
        for (int i = 0; i < _imp->dimension; ++i) {
            for (ExprPerViewMap::const_iterator it = _imp->expressions[i].begin(); it != _imp->expressions[i].end(); ++it) {
                for (std::list<Expr::Dependency>::const_iterator it2 = it->second.dependencies.begin();
                     it2 != it->second.dependencies.end(); ++it2) {
                    KnobIPtr knob = it2->knob.lock();
                    if (knob && knob.get() != this) {
                        deps.insert(knob);
                    }
                }
            }

        }
    }
    {
        QReadLocker k(&_imp->mastersMutex);
        for (int i = 0; i < _imp->dimension; ++i) {
            for (PerViewMasterLink::const_iterator it = _imp->masters[i].begin(); it!=_imp->masters[i].end(); ++it) {
                KnobIPtr master = it->second.masterKnob.lock();
                if (master && master.get() != this) {
                    if ( std::find(deps.begin(), deps.end(), master) == deps.end() ) {
                        deps.insert(master);
                    }
                }
            }

        }
    }
    std::list<KnobIPtr> knobsToInspectRecursive;

    for (std::set<KnobIPtr>::iterator it = deps.begin(); it != deps.end(); ++it) {
        EffectInstancePtr effect  = toEffectInstance( (*it)->getHolder() );
        if (effect) {
            NodePtr node = effect->getNode();

            nodes.insert(node);
            knobsToInspectRecursive.push_back(*it);
        }
    }


    for (std::list<KnobIPtr>::iterator it = knobsToInspectRecursive.begin(); it != knobsToInspectRecursive.end(); ++it) {
        (*it)->getAllExpressionDependenciesRecursive(nodes);
    }
}

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

    } else if (isStringBase) {

        knobSer->_dataType = eSerializationValueVariantTypeString;
        defValue->value.isString = isStringBase->getDefaultValue(dimension);
        defValue->serializeDefaultValue = isStringBase->hasDefaultValueChanged(dimension);
    } else if (isChoice) {
        knobSer->_dataType = eSerializationValueVariantTypeString;
        //serialization->_defaultValue.isString
        std::vector<std::string> entries = isChoice->getEntries();
        int defIndex = isChoice->getDefaultValue(dimension);
        std::string defaultValueChoice;
        if (defIndex >= 0 && defIndex < (int)entries.size()) {
            defaultValueChoice = entries[defIndex];
        }
        defValue->value.isString = defaultValueChoice;
        defValue->serializeDefaultValue = isChoice->hasDefaultValueChanged(dimension);
    }
} // initializeDefaultValueSerializationStorage


static void
initializeValueSerializationStorage(const KnobIPtr& knob,
                                    const std::vector<std::string>& viewNames,
                                    const DimIdx dimension,
                                    const ViewIdx view,
                                    const DefaultValueSerialization& defValue,
                                    ValueSerialization* serialization)
{
    serialization->_expression = knob->getExpression(dimension, view);
    serialization->_expresionHasReturnVariable = knob->isExpressionUsingRetVariable(view, dimension);

    bool gotValue = !serialization->_expression.empty();

    // Serialize curve
    CurvePtr curve = knob->getCurve(view, dimension);
    if (curve && !gotValue) {
        curve->toSerialization(&serialization->_animationCurve);
        if (!serialization->_animationCurve.keys.empty()) {
            gotValue = true;
        }
    }

    // Serialize slave/master link
    if (!gotValue) {
        EffectInstancePtr isHolderEffect = toEffectInstance(knob->getHolder());
        bool isEffectCloned = false;
        if (isHolderEffect) {
            isEffectCloned = isHolderEffect->getNode()->getMasterNode().get() != 0;
        }

        MasterKnobLink linkData;
        KnobIPtr masterKnob;
        if (knob->getMaster(dimension, view, &linkData)) {

            // Don't save the knob slave state if it is slaved because the user folded the dimensions
            if (dimension > 0 && !knob->getAllDimensionsVisible(view)) {
                masterKnob.reset();
            } else {
                masterKnob = linkData.masterKnob.lock();
            }

        }

        // Only serialize master link if:
        // - it exists and
        // - the knob wants the slave/master link to be persistent and
        // - the effect is not a clone of another one OR the master knob is an alias of this one
        if ( masterKnob && !knob->isMastersPersistenceIgnored() && (!isEffectCloned || knob->getAliasMaster())) {
            if (masterKnob->getNDimensions() > 1) {
                serialization->_slaveMasterLink.masterDimensionName = masterKnob->getDimensionName(linkData.masterDimension);
            }
            serialization->_slaveMasterLink.hasLink = true;
            gotValue = true;
            if (masterKnob != knob) {
                NamedKnobHolderPtr holder = boost::dynamic_pointer_cast<NamedKnobHolder>( masterKnob->getHolder() );
                assert(holder);
                KnobTableItemPtr isTableItem = toKnobTableItem(masterKnob->getHolder());
                if (isTableItem) {
                    if (isTableItem) {
                        serialization->_slaveMasterLink.masterTableItemName = isTableItem->getFullyQualifiedName();
                        if (isTableItem->getModel()->getNode()->getEffectInstance() != holder) {
                            serialization->_slaveMasterLink.masterNodeName = isTableItem->getModel()->getNode()->getScriptName_mt_safe();
                        }
                    }
                } else {
                    // coverity[dead_error_line]
                    if (holder && holder != knob->getHolder()) {
                        // If the master knob is on  the group containing the node holding this knob
                        // then don't serialize the node name

                        EffectInstancePtr thisHolderIsEffect = toEffectInstance(knob->getHolder());
                        if (thisHolderIsEffect) {
                            NodeGroupPtr isGrp = toNodeGroup(thisHolderIsEffect->getNode()->getGroup());
                            if (isGrp && isGrp == holder) {
                                serialization->_slaveMasterLink.masterNodeName = kKnobMasterNodeIsGroup;
                            }
                        }
                        if (serialization->_slaveMasterLink.masterNodeName.empty()) {
                            serialization->_slaveMasterLink.masterNodeName = holder->getScriptName_mt_safe();
                        }
                    }
                }
                serialization->_slaveMasterLink.masterKnobName = masterKnob->getName();
                if (linkData.masterView != ViewIdx(0) &&  linkData.masterView < (int)viewNames.size()) {
                    serialization->_slaveMasterLink.masterViewName = viewNames[linkData.masterView];
                }
            }
        }
    } // !gotValue


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


    serialization->_serializeValue = false;

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
        } else if (isStringBase) {
            serialization->_value.isString = isStringBase->getValue(dimension, view);
            serialization->_serializeValue = (serialization->_value.isString != defValue.value.isString);

        } else if (isChoice) {
            serialization->_value.isString = isChoice->getActiveEntryText(view);
            serialization->_serializeValue = (serialization->_value.isString != defValue.value.isString);
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


    } else if (isStringBase) {

        if (!applyDefaultValue) {
            isStringBase->setDefaultValueWithoutApplying(defObj.value.isString, targetDimension);
        } else {
            isStringBase->setDefaultValue(defObj.value.isString, targetDimension);
        }

    } else if (isChoice) {
        int foundDefault = -1;
        std::vector<std::string> entries = isChoice->getEntries();
        for (std::size_t i = 0; i < entries.size(); ++i) {
            if (boost::iequals(entries[i], defObj.value.isString)) {
                foundDefault = i;
                break;
            }
        }
        if (foundDefault != -1) {
            if (!applyDefaultValue) {
                isChoice->setDefaultValueWithoutApplying(foundDefault, DimIdx(0));
            } else {
                isChoice->setDefaultValue(foundDefault);
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
    KnobChoicePtr isChoice = toKnobChoice(thisShared);
    KnobStringBasePtr isStringBase = toKnobStringBase(thisShared);
    KnobPagePtr isPage = toKnobPage(thisShared);
    KnobGroupPtr isGrp = toKnobGroup(thisShared);
    KnobSeparatorPtr isSep = toKnobSeparator(thisShared);
    KnobButtonPtr btn = toKnobButton(thisShared);

    // We do the opposite of what is done in initializeValueSerializationStorage()
    if (isInt) {
        isInt->setValue(obj._value.isInt, view, targetDimension, eValueChangedReasonNatronInternalEdited, 0);
    } else if (isBool || isGrp || isButton) {
        assert(isBoolBase);
        isBoolBase->setValue(obj._value.isBool, view, targetDimension, eValueChangedReasonNatronInternalEdited, 0);
    } else if (isColor || isDouble) {
        assert(isDoubleBase);
        isDoubleBase->setValue(obj._value.isDouble, view, targetDimension, eValueChangedReasonNatronInternalEdited, 0);
    } else if (isStringBase) {
        isStringBase->setValue(obj._value.isString, view, targetDimension, eValueChangedReasonNatronInternalEdited, 0);
    } else if (isChoice) {
        int foundValue = -1;

        std::vector<std::string> entries = isChoice->getEntries();
        for (std::size_t i = 0; i < entries.size(); ++i) {
            if (boost::iequals(entries[i], obj._value.isString) ) {
                foundValue = i;
                break;
            }
        }

        if (foundValue == -1) {
            // Just remember the active entry if not found
            isChoice->setActiveEntryText(obj._value.isString, view);
        } else {
            isChoice->setValue(foundValue, view, targetDimension, eValueChangedReasonNatronInternalEdited, 0);
        }

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
                boost::shared_ptr<GroupKnobSerialization> childSer( new GroupKnobSerialization );
                isGrp->toSerialization(childSer.get());
                groupSerialization->_children.push_back(childSer);
            } else {
                //KnobChoicePtr isChoice = toKnobChoice(children[i].get());
                //bool copyKnob = false;//isChoice != NULL;
                KnobSerializationPtr childSer( new KnobSerialization );

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

        serialization->_isUserKnob = serialization->_forceUserKnob || (isUserKnob() && !isDeclaredByPlugin());

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

            bool allDimensionsVisible = getAllDimensionsVisible(*it);
            if (allDimensionsVisible) {
                dimValues.resize(serialization->_dimension);
            } else {
                dimValues.resize(1);
            }
            for (std::size_t i = 0; i < dimValues.size(); ++i) {
                dimValues[i]._serialization = serialization;
                dimValues[i]._dimension = (int)i;
                initializeValueSerializationStorage(thisShared, viewNames, DimIdx(i), *it, serialization->_defaultValues[i], &dimValues[i]);

                // Force default value serialization in those cases
                if (serialization->_isUserKnob || isFullRecoverySave) {
                    serialization->_defaultValues[i].serializeDefaultValue = true;
                    dimValues[i]._mustSerialize = true;
                }
            }

        }

        serialization->_masterIsAlias = getAliasMaster().get() != 0;

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

            int nDimsDisabled = 0;
            for (int i = 0; i < serialization->_dimension; ++i) {
                // If the knob is slaved, it will be disabled so do not take it into account
                if (!isSlave(DimIdx(i), ViewIdx(0)) && (!isEnabled(DimIdx(i)))) {
                    ++nDimsDisabled;
                }
            }
            if (nDimsDisabled == serialization->_dimension) {
                serialization->_disabled = true;
            }
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
            extraData->_entries = isChoice->getEntries();
            extraData->_helpStrings = isChoice->getEntriesHelp();
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
            isString->saveAnimation(&extraData->keyframes);
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
                    extraData->useHostOverlayHandle = serialization->_dimension == 2 && isDbl->getHasHostOverlayHandle();
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
        serialization->_mustSerialize = true;
        if (!serialization->_isUserKnob && !serialization->_masterIsAlias && !serialization->_hasViewerInterface) {
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
                        if (!data->keyframes.empty() || data->fontFamily != NATRON_FONT || data->fontSize != KnobString::getDefaultFontPointSize() || data->fontColor[0] != 0 || data->fontColor[1] != 0 || data->fontColor[2] != 0) {
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
    beginChanges();



    // Restore extra datas
    KnobFile* isInFile = dynamic_cast<KnobFile*>(this);
    KnobString* isString = dynamic_cast<KnobString*>(this);
    if (isString) {
        const TextExtraData* data = dynamic_cast<const TextExtraData*>(serialization->_extraData.get());
        if (data) {
            isString->loadAnimation(data->keyframes);
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
        setAsUserKnob(true);
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
                    minimums.push_back(data->min);
                    maximums.push_back(data->max);
                    dminimums.push_back(data->dmin);
                    dmaximums.push_back(data->dmax);
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
                    minimums.push_back(data->min);
                    maximums.push_back(data->max);
                    dminimums.push_back(data->dmin);
                    dmaximums.push_back(data->dmax);
                }
                isDouble->setRangeAcrossDimensions(minimums, maximums);
                isDouble->setDisplayRangeAcrossDimensions(dminimums, dmaximums);
                if (data->useHostOverlayHandle) {
                    isDouble->setHasHostOverlayHandle(true);
                }
            }

        } else if (isChoice) {
            const ChoiceExtraData* data = dynamic_cast<const ChoiceExtraData*>(serialization->_extraData.get());
            if (data) {
                isChoice->populateChoices(data->_entries, data->_helpStrings);
            }
        } else if (isColor) {
            const ValueExtraData* data = dynamic_cast<const ValueExtraData*>(serialization->_extraData.get());
            if (data) {
                std::vector<double> minimums, maximums, dminimums, dmaximums;
                for (int i = 0; i < nDims; ++i) {
                    minimums.push_back(data->min);
                    maximums.push_back(data->max);
                    dminimums.push_back(data->dmin);
                    dmaximums.push_back(data->dmax);
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

    // Clear any existing animation
    removeAnimation(ViewSetSpec::all(), DimSpec::all());

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

        for (int i = 0; i < _imp->dimension; ++i) {

            // Not all dimensions are necessarily saved since they may be folded.
            // In that case replicate the last dimenion
            int d = i >= (int)it->second.size() ? it->second.size() - 1 : i;

            DimIdx dimensionIndex(i);

            // Clone animation
            if (!it->second[d]._animationCurve.keys.empty()) {
                CurvePtr curve = getCurve(view_i, dimensionIndex);
                if (curve) {
                    curve->fromSerialization(it->second[d]._animationCurve);
                    std::list<double> keysAdded, keysRemoved;
                    KeyFrameSet keys = curve->getKeyFrames_mt_safe();
                    for (KeyFrameSet::iterator it = keys.begin(); it != keys.end(); ++it) {
                        keysAdded.push_back(it->getTime());
                    }
                    _signalSlotHandler->s_curveAnimationChanged(keysAdded, keysRemoved, view_i, dimensionIndex);
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
        if (isUserKnob()) {
            setInViewerContextLabel(QString::fromUtf8(serialization->_inViewerContextLabel.c_str()));
            setInViewerContextIconFilePath(serialization->_inViewerContextIconFilePath[0], false);
            setInViewerContextIconFilePath(serialization->_inViewerContextIconFilePath[1], true);
        }
    }
    
    // Allow changes again
    endChanges();
    unblockValueChanges();
    evaluateValueChange(DimSpec::all(), getCurrentTime(), ViewSetSpec::all(), eValueChangedReasonRestoreDefault);
} // KnobHelper::fromSerialization



/***************************KNOB HOLDER******************************************/

struct KnobHolder::KnobHolderPrivate
{
    AppInstanceWPtr app;
    QMutex knobsMutex;
    // When rendering, the render thread makes a (shallow) copy of this item:
    // knobs are not copied
    bool isShallowRenderCopy;

    std::vector< KnobIPtr > knobs;
    bool knobsInitialized;
    bool isInitializingKnobs;
    bool isSlave;
    std::vector<KnobIWPtr> knobsWithViewerUI;

    ///Count how many times an overlay needs to be redrawn for the instanceChanged/penMotion/penDown etc... actions
    ///to just redraw it once when the recursion level is back to 0
    QMutex overlayRedrawStackMutex;
    int overlayRedrawStack;
    bool isDequeingValuesSet;
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
    KnobChanges knobChanged;
    int nbSignificantChangesDuringEvaluationBlock;
    int nbChangesDuringEvaluationBlock;
    int nbChangesRequiringMetadataRefresh;
    QMutex knobsFrozenMutex;
    bool knobsFrozen;

    // Protects hasAnimation
    mutable QMutex hasAnimationMutex;

    // True if one knob held by this object is animated.
    // This is held here for fast access, otherwise it requires looping over all knobs
    bool hasAnimation;
    DockablePanelI* settingsPanel;

    std::list<KnobIWPtr> overlaySlaves;

    // A knobs table owned by the holder
    KnobItemsTablePtr knobsTable;

    // The script-name of the knob right before where the table should be inserted in the gui
    std::string knobsTableParamBefore;


    KnobHolderPrivate(const AppInstancePtr& appInstance_)
        : app(appInstance_)
        , knobsMutex()
        , isShallowRenderCopy(false)
        , knobs()
        , knobsInitialized(false)
        , isInitializingKnobs(false)
        , isSlave(false)
        , overlayRedrawStackMutex()
        , overlayRedrawStack(0)
        , isDequeingValuesSet(false)
        , evaluationBlockedMutex(QMutex::Recursive)
        , evaluationBlocked(0)
        , knobChanged()
        , nbSignificantChangesDuringEvaluationBlock(0)
        , nbChangesDuringEvaluationBlock(0)
        , nbChangesRequiringMetadataRefresh(0)
        , knobsFrozenMutex()
        , knobsFrozen(false)
        , hasAnimationMutex()
        , hasAnimation(false)
        , settingsPanel(0)
        , overlaySlaves()
        , knobsTable()
        , knobsTableParamBefore()

    {
    }

    KnobHolderPrivate(const KnobHolderPrivate& other)
    : app(other.app)
    , knobsMutex()
    , isShallowRenderCopy(true)
    , knobs(other.knobs)
    , knobsInitialized(other.knobsInitialized)
    , isInitializingKnobs(other.isInitializingKnobs)
    , isSlave(other.isSlave)
    , overlayRedrawStackMutex()
    , overlayRedrawStack(0)
    , isDequeingValuesSet(other.isDequeingValuesSet)
    , evaluationBlockedMutex(QMutex::Recursive)
    , evaluationBlocked(0)
    , knobChanged()
    , nbSignificantChangesDuringEvaluationBlock(0)
    , nbChangesDuringEvaluationBlock(0)
    , nbChangesRequiringMetadataRefresh(0)
    , knobsFrozenMutex()
    , knobsFrozen(false)
    , hasAnimationMutex()
    , hasAnimation(other.hasAnimation)
    , settingsPanel(other.settingsPanel)
    {

    }
};

KnobHolder::KnobHolder(const AppInstancePtr& appInstance)
: QObject()
, _imp( new KnobHolderPrivate(appInstance) )
{
    bool isMT = QThread::currentThread() == qApp->thread();
    if (isMT) {
        QObject::connect( this, SIGNAL(doEndChangesOnMainThread()), this, SLOT(onDoEndChangesOnMainThreadTriggered()) );
        QObject::connect( this, SIGNAL(doEvaluateOnMainThread(bool,bool)), this,
                         SLOT(onDoEvaluateOnMainThread(bool,bool)) );
        QObject::connect( this, SIGNAL(doValueChangeOnMainThread(KnobIPtr,ValueChangedReasonEnum,double,ViewSetSpec,bool)), this,
                         SLOT(onDoValueChangeOnMainThread(KnobIPtr,ValueChangedReasonEnum,double,ViewSetSpec,bool)) );

        QObject::connect( this, SIGNAL(doBeginKnobsValuesChangedActionOnMainThread(ValueChangedReasonEnum)), this, SLOT(onDoBeginKnobsValuesChangedActionOnMainThread(ValueChangedReasonEnum)) );
        QObject::connect( this, SIGNAL(doEndKnobsValuesChangedActionOnMainThread(ValueChangedReasonEnum)), this, SLOT(onDoEndKnobsValuesChangedActionOnMainThread(ValueChangedReasonEnum)) );
    }
}

KnobHolder::KnobHolder(const KnobHolder& other)
    : QObject()
    , boost::enable_shared_from_this<KnobHolder>()
, _imp (new KnobHolderPrivate(*other._imp))
{
    bool isMT = QThread::currentThread() == qApp->thread();
    if (isMT) {
        QObject::connect( this, SIGNAL( doEndChangesOnMainThread() ), this, SLOT( onDoEndChangesOnMainThreadTriggered() ) );
        QObject::connect( this, SIGNAL( doEvaluateOnMainThread(bool, bool) ), this,
                         SLOT( onDoEvaluateOnMainThread(bool, bool) ) );
        QObject::connect( this, SIGNAL( doValueChangeOnMainThread(KnobIPtr, ValueChangedReasonEnum, double, ViewSetSpec, bool) ), this,
                         SLOT( onDoValueChangeOnMainThread(KnobIPtr, ValueChangedReasonEnum, double, ViewSetSpec, bool) ) );
    }
}

KnobHolder::~KnobHolder()
{
    if (!_imp->isShallowRenderCopy) {
        for (std::size_t i = 0; i < _imp->knobs.size(); ++i) {
            KnobHelperPtr helper = boost::dynamic_pointer_cast<KnobHelper>(_imp->knobs[i]);
            assert(helper);
            if (helper) {
                // Make sure nobody is referencing this
                helper->_imp->holder.reset();
                helper->deleteKnob();
                
            }
        }
    }
}

bool
KnobHolder::isRenderClone() const
{
    return _imp->isShallowRenderCopy;
}


void
KnobHolder::setItemsTable(const KnobItemsTablePtr& table, const std::string& paramScriptNameBefore)
{
    assert(!paramScriptNameBefore.empty());
    _imp->knobsTableParamBefore = paramScriptNameBefore;
    _imp->knobsTable = table;
}

KnobItemsTablePtr
KnobHolder::getItemsTable() const
{
    return _imp->knobsTable;
}

std::string
KnobHolder::getItemsTablePreviousKnobScriptName() const
{
    return _imp->knobsTableParamBefore;
}

void
KnobHolder::setViewerUIKnobs(const KnobsVec& knobs)
{
    QMutexLocker k(&_imp->knobsMutex);
    _imp->knobsWithViewerUI.clear();
    for (KnobsVec::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        _imp->knobsWithViewerUI.push_back(*it);
    }

}

void
KnobHolder::addKnobToViewerUI(const KnobIPtr& knob)
{
    QMutexLocker k(&_imp->knobsMutex);
    _imp->knobsWithViewerUI.push_back(knob);
}

void
KnobHolder::insertKnobToViewerUI(const KnobIPtr& knob, int index)
{
    QMutexLocker k(&_imp->knobsMutex);
    if (index < 0 || index >= (int)_imp->knobsWithViewerUI.size()) {
        _imp->knobsWithViewerUI.push_back(knob);
    } else {
        std::vector<KnobIWPtr>::iterator it = _imp->knobsWithViewerUI.begin();
        std::advance(it, index);
        _imp->knobsWithViewerUI.insert(it, knob);
    }
}

void
KnobHolder::removeKnobViewerUI(const KnobIPtr& knob)
{
    QMutexLocker k(&_imp->knobsMutex);
    for (std::vector<KnobIWPtr>::iterator it = _imp->knobsWithViewerUI.begin(); it!=_imp->knobsWithViewerUI.end(); ++it) {
        KnobIPtr p = it->lock();
        if (p == knob) {
            _imp->knobsWithViewerUI.erase(it);
            return;
        }
    }

}

int
KnobHolder::getInViewerContextKnobIndex(const KnobIConstPtr& knob) const
{
    QMutexLocker k(&_imp->knobsMutex);
    int i = 0;
    for (std::vector<KnobIWPtr>::const_iterator it = _imp->knobsWithViewerUI.begin(); it!=_imp->knobsWithViewerUI.end(); ++it, ++i) {
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
    for (std::vector<KnobIWPtr>::const_iterator it = _imp->knobsWithViewerUI.begin(); it != _imp->knobsWithViewerUI.end(); ++it) {
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
    QMutexLocker kk(&_imp->knobsMutex);
    for (KnobsVec::iterator it = _imp->knobs.begin(); it != _imp->knobs.end(); ++it) {
        if (*it == k) {
            return;
        }
    }
    _imp->knobs.push_back(k);
}

void
KnobHolder::insertKnob(int index,
                       const KnobIPtr& k)
{
    if (index < 0) {
        return;
    }
    QMutexLocker kk(&_imp->knobsMutex);
    for (KnobsVec::iterator it = _imp->knobs.begin(); it != _imp->knobs.end(); ++it) {
        if (*it == k) {
            return;
        }
    }
    if ( index >= (int)_imp->knobs.size() ) {
        _imp->knobs.push_back(k);
    } else {
        KnobsVec::iterator it = _imp->knobs.begin();
        std::advance(it, index);
        _imp->knobs.insert(it, k);
    }
}

void
KnobHolder::removeKnobFromList(const KnobIConstPtr& knob)
{
    QMutexLocker kk(&_imp->knobsMutex);

    for (KnobsVec::iterator it = _imp->knobs.begin(); it != _imp->knobs.end(); ++it) {
        if (*it == knob) {
            _imp->knobs.erase(it);

            return;
        }
    }
}

void
KnobHolder::setPanelPointer(DockablePanelI* gui)
{
    assert( QThread::currentThread() == qApp->thread() );
    _imp->settingsPanel = gui;
}

void
KnobHolder::discardPanelPointer()
{
    assert( QThread::currentThread() == qApp->thread() );
    _imp->settingsPanel = 0;
}

void
KnobHolder::recreateUserKnobs(bool keepCurPageIndex)
{
    assert( QThread::currentThread() == qApp->thread() );
    if (_imp->settingsPanel) {
        _imp->settingsPanel->recreateUserKnobs(keepCurPageIndex);
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
    if (_imp->settingsPanel) {
        _imp->settingsPanel->refreshGuiForKnobsChanges(keepCurPageIndex);
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

    KnobsVec knobs;
    {
        QMutexLocker k(&_imp->knobsMutex);
        knobs = _imp->knobs;
    }
    KnobIPtr sharedKnob;
    for (KnobsVec::iterator it = knobs.begin(); it != knobs.end(); ++it) {
        if (*it == knob) {
            (*it)->deleteKnob();
            sharedKnob = *it;
            break;
        }
    }

    {
        QMutexLocker k(&_imp->knobsMutex);
        for (KnobsVec::iterator it2 = _imp->knobs.begin(); it2 != _imp->knobs.end(); ++it2) {
            if (*it2 == knob) {
                _imp->knobs.erase(it2);
                break;
            }
        }
    }

    if (sharedKnob && alsoDeleteGui && _imp->settingsPanel) {
        _imp->settingsPanel->deleteKnobGui(sharedKnob);
    }
}

void
KnobHolder::addOverlaySlaveParam(const KnobIPtr& knob)
{
    _imp->overlaySlaves.push_back(knob);
}

bool
KnobHolder::isOverlaySlaveParam(const KnobIConstPtr& knob) const
{
    for (std::list<KnobIWPtr >::const_iterator it = _imp->overlaySlaves.begin(); it != _imp->overlaySlaves.end(); ++it) {
        KnobIPtr k = it->lock();
        if (!k) {
            continue;
        }
        if (k == knob) {
            return true;
        }
    }

    return false;
}

void
KnobHolder::redrawOverlayInteract()
{
    if ( isDoingInteractAction() ) {
        getApp()->queueRedrawForAllViewers();
    } else {
        getApp()->redrawAllViewers();
    }
}


bool
KnobHolder::moveViewerUIKnobOneStepUp(const KnobIPtr& knob)
{
    QMutexLocker k(&_imp->knobsMutex);
    for (std::size_t i = 0; i < _imp->knobsWithViewerUI.size(); ++i) {
        if (_imp->knobsWithViewerUI[i].lock() == knob) {
            if (i == 0) {
                return false;
            }
            // can't swap weak pointers using std::swap!
            // std::swap(_imp->knobsWithViewerUI[i - 1], _imp->knobsWithViewerUI[i]);
            _imp->knobsWithViewerUI[i].swap(_imp->knobsWithViewerUI[i - 1]);
            return true;
        }
    }
    return false;
}

bool
KnobHolder::moveViewerUIOneStepDown(const KnobIPtr& knob)
{
    QMutexLocker k(&_imp->knobsMutex);
    for (std::size_t i = 0; i < _imp->knobsWithViewerUI.size(); ++i) {
        if (_imp->knobsWithViewerUI[i].lock() == knob) {
            if (i == _imp->knobsWithViewerUI.size() - 1) {
                return false;
            }
            // can't swap weak pointers using std::swap!
            // std::swap(_imp->knobsWithViewerUI[i + 1], _imp->knobsWithViewerUI[i]);
            _imp->knobsWithViewerUI[i].swap(_imp->knobsWithViewerUI[i + 1]);
            return true;
        }
    }
    return false;
}

bool
KnobHolder::moveKnobOneStepUp(const KnobIPtr& knob)
{
    if ( !knob->isUserKnob() && !toKnobPage(knob) ) {
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
                    if ( _imp->knobs[i]->isUserKnob() && (_imp->knobs[i]->getParentKnob() == parent) ) {
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
    if ( !knob->isUserKnob() && !toKnobPage(knob) ) {
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
                if ( _imp->knobs[i]->isUserKnob() && (_imp->knobs[i]->getParentKnob() == parent) ) {
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
            if (!(*it)->isUserKnob()) {
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
    ret = AppManager::createKnob<KnobPage>(shared_from_this(), tr(NATRON_USER_MANAGED_KNOBS_PAGE_LABEL), 1, false);
    ret->setName(NATRON_USER_MANAGED_KNOBS_PAGE);
    onUserKnobCreated(ret, true);
    return ret;
}

void
KnobHolder::onUserKnobCreated(const KnobIPtr& knob, bool isUserKnob)
{

    knob->setAsUserKnob(isUserKnob);
    EffectInstance* isEffect = dynamic_cast<EffectInstance*>(this);
    if (isEffect) {
        if (isEffect->getNode()->isPyPlug() && getApp()->isCreatingNode()) {
            knob->setDeclaredByPlugin(true);
        }
        if (isUserKnob) {
            isEffect->getNode()->declarePythonKnobs();
        }
    }

}

KnobIntPtr
KnobHolder::createIntKnob(const std::string& name,
                          const std::string& label,
                          int dimension,
                          bool userKnob)
{
    KnobIPtr existingKnob = getKnobByName(name);

    if (existingKnob) {
        return toKnobInt(existingKnob);
    }
    KnobIntPtr ret = AppManager::createKnob<KnobInt>(shared_from_this(), label, dimension, false);
    ret->setName(name);
    onUserKnobCreated(ret, userKnob);
    return ret;
}

KnobDoublePtr
KnobHolder::createDoubleKnob(const std::string& name,
                             const std::string& label,
                             int dimension,
                             bool userKnob)
{
    KnobIPtr existingKnob = getKnobByName(name);

    if (existingKnob) {
        return toKnobDouble(existingKnob);
    }
    KnobDoublePtr ret = AppManager::createKnob<KnobDouble>(shared_from_this(), label, dimension, false);
    ret->setName(name);
    onUserKnobCreated(ret, userKnob);
    return ret;
}

KnobColorPtr
KnobHolder::createColorKnob(const std::string& name,
                            const std::string& label,
                            int dimension,
                            bool userKnob)
{
    KnobIPtr existingKnob = getKnobByName(name);

    if (existingKnob) {
        return toKnobColor(existingKnob);
    }
    KnobColorPtr ret = AppManager::createKnob<KnobColor>(shared_from_this(), label, dimension, false);
    ret->setName(name);
    onUserKnobCreated(ret, userKnob);

    return ret;
}

KnobBoolPtr
KnobHolder::createBoolKnob(const std::string& name,
                           const std::string& label,
                           bool userKnob)
{
    KnobIPtr existingKnob = getKnobByName(name);

    if (existingKnob) {
        return toKnobBool(existingKnob);
    }
    KnobBoolPtr ret = AppManager::createKnob<KnobBool>(shared_from_this(), label, 1, false);
    ret->setName(name);
    onUserKnobCreated(ret, userKnob);

    return ret;
}

KnobChoicePtr
KnobHolder::createChoiceKnob(const std::string& name,
                             const std::string& label,
                             bool userKnob)
{
    KnobIPtr existingKnob = getKnobByName(name);

    if (existingKnob) {
        return toKnobChoice(existingKnob);
    }
    KnobChoicePtr ret = AppManager::createKnob<KnobChoice>(shared_from_this(), label, 1, false);
    ret->setName(name);
    onUserKnobCreated(ret, userKnob);

    return ret;
}

KnobButtonPtr
KnobHolder::createButtonKnob(const std::string& name,
                             const std::string& label,
                             bool userKnob)
{
    KnobIPtr existingKnob = getKnobByName(name);

    if (existingKnob) {
        return toKnobButton(existingKnob);
    }
    KnobButtonPtr ret = AppManager::createKnob<KnobButton>(shared_from_this(), label, 1, false);
    ret->setName(name);
    onUserKnobCreated(ret, userKnob);

    return ret;
}

KnobSeparatorPtr
KnobHolder::createSeparatorKnob(const std::string& name,
                                const std::string& label,
                                bool userKnob)
{
    KnobIPtr existingKnob = getKnobByName(name);

    if (existingKnob) {
        return toKnobSeparator(existingKnob);
    }
    KnobSeparatorPtr ret = AppManager::createKnob<KnobSeparator>(shared_from_this(), label, 1, false);
    ret->setName(name);
    onUserKnobCreated(ret, userKnob);

    return ret;
}

//Type corresponds to the Type enum defined in StringParamBase in Parameter.h
KnobStringPtr
KnobHolder::createStringKnob(const std::string& name,
                             const std::string& label,
                             bool userKnob)
{
    KnobIPtr existingKnob = getKnobByName(name);

    if (existingKnob) {
        return toKnobString(existingKnob);
    }
    KnobStringPtr ret = AppManager::createKnob<KnobString>(shared_from_this(), label, 1, false);
    ret->setName(name);
    onUserKnobCreated(ret, userKnob);
    return ret;
}

KnobFilePtr
KnobHolder::createFileKnob(const std::string& name,
                           const std::string& label,
                           bool userKnob)
{
    KnobIPtr existingKnob = getKnobByName(name);

    if (existingKnob) {
        return toKnobFile(existingKnob);
    }
    KnobFilePtr ret = AppManager::createKnob<KnobFile>(shared_from_this(), label, 1, false);
    ret->setName(name);
    onUserKnobCreated(ret, userKnob);

    return ret;
}



KnobPathPtr
KnobHolder::createPathKnob(const std::string& name,
                           const std::string& label,
                           bool userKnob)
{
    KnobIPtr existingKnob = getKnobByName(name);

    if (existingKnob) {
        return toKnobPath(existingKnob);
    }
    KnobPathPtr ret = AppManager::createKnob<KnobPath>(shared_from_this(), label, 1, false);
    ret->setName(name);
    onUserKnobCreated(ret, userKnob);
    return ret;
}

KnobGroupPtr
KnobHolder::createGroupKnob(const std::string& name,
                            const std::string& label,
                            bool userKnob)
{
    KnobIPtr existingKnob = getKnobByName(name);

    if (existingKnob) {
        return toKnobGroup(existingKnob);
    }
    KnobGroupPtr ret = AppManager::createKnob<KnobGroup>(shared_from_this(), label, 1, false);
    ret->setName(name);
    onUserKnobCreated(ret, userKnob);

    return ret;
}

KnobPagePtr
KnobHolder::createPageKnob(const std::string& name,
                           const std::string& label,
                           bool userKnob)
{
    KnobIPtr existingKnob = getKnobByName(name);

    if (existingKnob) {
        return toKnobPage(existingKnob);
    }
    KnobPagePtr ret = AppManager::createKnob<KnobPage>(shared_from_this(), label, 1, false);
    ret->setName(name);
    onUserKnobCreated(ret, userKnob);
    return ret;
}

KnobParametricPtr
KnobHolder::createParametricKnob(const std::string& name,
                                 const std::string& label,
                                 int nbCurves,
                                 bool userKnob)
{
    KnobIPtr existingKnob = getKnobByName(name);

    if (existingKnob) {
        return toKnobParametric(existingKnob);
    }
    KnobParametricPtr ret = AppManager::createKnob<KnobParametric>(shared_from_this(), label, nbCurves, false);
    ret->setName(name);
    onUserKnobCreated(ret, userKnob);

    return ret;
}

void
KnobHolder::onDoEvaluateOnMainThread(bool significant,
                                     bool refreshMetadata)
{
    assert( QThread::currentThread() == qApp->thread() );
    evaluate(significant, refreshMetadata);
}

void
KnobHolder::invalidateCacheHashAndEvaluate(bool isSignificant,
                                bool refreshMetadatas)
{
    if (isEvaluationBlocked()) {
        return;
    }
    onSignificantEvaluateAboutToBeCalled(KnobIPtr(), eValueChangedReasonNatronInternalEdited, DimSpec::all(), getCurrentTime(), ViewSetSpec::all());
    evaluate(isSignificant, refreshMetadatas);
}

void
KnobHolder::onDoEndChangesOnMainThreadTriggered()
{
    assert( QThread::currentThread() == qApp->thread() );
    endChanges();
}

bool
KnobHolder::endChanges(bool discardRendering)
{
    bool isMT = QThread::currentThread() == qApp->thread();

    if ( !isMT && !canHandleEvaluateOnChangeInOtherThread() ) {
        Q_EMIT doEndChangesOnMainThread();

        return true;
    }


    bool thisChangeSignificant = false;
    bool thisBracketHadChange = false;
    KnobChanges knobChanged;
    {
        QMutexLocker l(&_imp->evaluationBlockedMutex);

        knobChanged = _imp->knobChanged;
        for (KnobChanges::iterator it = knobChanged.begin(); it != knobChanged.end(); ++it) {
            if ( it->knob->getEvaluateOnChange() ) {
                thisChangeSignificant = true;
            }

            if ( !it->valueChangeBlocked && it->knob->getIsMetadataSlave() ) {
                ++_imp->nbChangesRequiringMetadataRefresh;
            }
        }
        if (thisChangeSignificant) {
            ++_imp->nbSignificantChangesDuringEvaluationBlock;
        }
        if ( !knobChanged.empty() ) {
            ++_imp->nbChangesDuringEvaluationBlock;
            thisBracketHadChange = true;
        }

        _imp->knobChanged.clear();
    }
    KnobIPtr firstKnobChanged;
    ValueChangedReasonEnum firstKnobReason = eValueChangedReasonNatronGuiEdited;
    DimSpec firstKnobDimension = DimSpec::all();
    ViewSetSpec firstKnobView = ViewSetSpec::all();
    double firstKnobTime = 0;
    if ( !knobChanged.empty() ) {
        KnobChange& first = knobChanged.front();
        firstKnobChanged = first.knob;
        firstKnobReason = first.reason;
        if (!first.dimensionChanged.empty()) {
            firstKnobDimension = *(first.dimensionChanged.begin());
        }
        firstKnobTime = first.time;
        firstKnobView = first.view;
    }
    bool isChangeDueToTimeChange = firstKnobReason == eValueChangedReasonTimeChanged;
    bool isLoadingProject = false;
    if ( getApp() ) {
        isLoadingProject = getApp()->getProject()->isLoadingProject();
    }

    // If the node is currently modifying its input, do not ask for a render
    // because at then end of the inputChanged handler, it will ask for a refresh
    // and a rebuild of the inputs tree.
    EffectInstance* isEffect = dynamic_cast<EffectInstance*>(this);
    bool duringInputChangeAction = false;
    if (isEffect) {
        NodePtr node = isEffect->getNode();
        if ( isMT && node->duringInputChangedAction() ) {
            duringInputChangeAction = true;
        }
    }


    // Increment hash only if significant
    if (thisChangeSignificant && thisBracketHadChange && !isLoadingProject && !duringInputChangeAction && !isChangeDueToTimeChange) {
        onSignificantEvaluateAboutToBeCalled(firstKnobChanged, firstKnobReason, firstKnobDimension, firstKnobTime, firstKnobView);
    }

    bool guiFrozen = firstKnobChanged ? getApp() && firstKnobChanged->getKnobGuiPointer() && firstKnobChanged->getKnobGuiPointer()->isGuiFrozenForPlayback() : false;

    // Call instanceChanged on each knob
    bool ret = false;
    for (KnobChanges::iterator it = knobChanged.begin(); it != knobChanged.end(); ++it) {

        it->knob->computeHasModifications();

        if (it->knob && !it->valueChangeBlocked && !isLoadingProject) {
            if ( !it->originatedFromMainThread && !canHandleEvaluateOnChangeInOtherThread() ) {
                Q_EMIT doValueChangeOnMainThread(it->knob, it->originalReason, it->time, it->view, it->originatedFromMainThread);
            } else {
                ret |= onKnobValueChanged_public(it->knob, it->originalReason, it->time, it->view, it->originatedFromMainThread);
            }
        }


        DimSpec dimension = DimSpec::all();
        if (it->dimensionChanged.size() == 1) {
            dimension = *it->dimensionChanged.begin();
        }

        if (!guiFrozen) {
            boost::shared_ptr<KnobSignalSlotHandler> handler = it->knob->getSignalSlotHandler();
            if (!it->valueChangeBlocked) {
                handler->s_mustRefreshKnobGui(it->view, dimension, it->reason);
            }

            it->knob->refreshAnimationLevel(it->view, dimension);
        }

        if ( !it->valueChangeBlocked && !it->knob->isListenersNotificationBlocked() && firstKnobReason != eValueChangedReasonTimeChanged) {
            it->knob->refreshListenersAfterValueChange(it->view, it->originalReason, dimension);
        }
    }

    int evaluationBlocked;
    bool hasHadSignificantChange = false;
    bool hasHadAnyChange = false;
    bool mustRefreshMetadatas = false;

    {
        QMutexLocker l(&_imp->evaluationBlockedMutex);
        if (_imp->evaluationBlocked > 0) {
            --_imp->evaluationBlocked;
        }
        evaluationBlocked = _imp->evaluationBlocked;
        if (evaluationBlocked == 0) {
            if (_imp->nbSignificantChangesDuringEvaluationBlock) {
                hasHadSignificantChange = true;
            }
            if (_imp->nbChangesRequiringMetadataRefresh) {
                mustRefreshMetadatas = true;
            }
            if (_imp->nbChangesDuringEvaluationBlock) {
                hasHadAnyChange = true;
            }
            _imp->nbSignificantChangesDuringEvaluationBlock = 0;
            _imp->nbChangesDuringEvaluationBlock = 0;
            _imp->nbChangesRequiringMetadataRefresh = 0;
        }
    }


    // Update the holder has animation flag
    if (thisBracketHadChange) {
        updateHasAnimation();
    }


    // Call getClipPreferences & render
    if ( hasHadAnyChange && !discardRendering && !isLoadingProject && !duringInputChangeAction && !isChangeDueToTimeChange && (evaluationBlocked == 0) ) {
        if (isMT) {
            endKnobsValuesChanged_public(firstKnobReason);
        } else {
            Q_EMIT doEndKnobsValuesChangedActionOnMainThread(firstKnobReason);
        }
        if (!isMT) {
            Q_EMIT doEvaluateOnMainThread(hasHadSignificantChange, mustRefreshMetadatas);
        } else {
            evaluate(hasHadSignificantChange, mustRefreshMetadatas);
        }

    }

    return ret;
} // KnobHolder::endChanges

void
KnobHolder::onDoValueChangeOnMainThread(const KnobIPtr& knob,
                                        ValueChangedReasonEnum reason,
                                        double time,
                                        ViewSetSpec view,
                                        bool originatedFromMT)
{
    assert( QThread::currentThread() == qApp->thread() );
    onKnobValueChanged_public(knob, (ValueChangedReasonEnum)reason, time, view, originatedFromMT);
}

void
KnobHolder::onDoBeginKnobsValuesChangedActionOnMainThread(ValueChangedReasonEnum reason)
{
    beginKnobsValuesChanged_public(reason);
}

void
KnobHolder::onDoEndKnobsValuesChangedActionOnMainThread(ValueChangedReasonEnum reason)
{
    endKnobsValuesChanged_public(reason);
}

void
KnobHolder::appendValueChange(const KnobIPtr& knob,
                              DimSpec dimension,
                              double time,
                              ViewSetSpec view,
                              ValueChangedReasonEnum originalReason,
                              ValueChangedReasonEnum reason)
{
    if ( isInitializingKnobs() ) {
        return;
    }

    bool isMT = QThread::currentThread() == qApp->thread();

    {
        QMutexLocker l(&_imp->evaluationBlockedMutex);

        if (_imp->nbChangesDuringEvaluationBlock == 0) {
            // This is the first change, call begin action
            if (isMT) {
                beginKnobsValuesChanged_public(originalReason);
            } else {
                Q_EMIT doBeginKnobsValuesChangedActionOnMainThread(originalReason);
            }
        }

        KnobChange* foundChange = 0;
        for (KnobChanges::iterator it = _imp->knobChanged.begin(); it != _imp->knobChanged.end(); ++it) {
            if (it->knob == knob) {
                foundChange = &*it;
                break;
            }
        }
        if (!foundChange) {
            KnobChange p;
            _imp->knobChanged.push_back(p);
            foundChange = &_imp->knobChanged.back();
        }
        assert(foundChange);

        foundChange->reason = reason;
        foundChange->originalReason = originalReason;
        foundChange->originatedFromMainThread = isMT;
        foundChange->time = time;
        foundChange->view = view;
        foundChange->knob = knob;
        foundChange->valueChangeBlocked = knob->isValueChangesBlocked();
        if (dimension.isAll()) {
            for (int i = 0; i < knob->getNDimensions(); ++i) {
                foundChange->dimensionChanged.insert(DimIdx(i));
            }
        } else {
            foundChange->dimensionChanged.insert(DimIdx(dimension));
        }

        // Clear expression now before anything else gets updated
        knob->clearExpressionsResults(dimension, view);

        if ( !foundChange->valueChangeBlocked && knob->getIsMetadataSlave() ) {
            ++_imp->nbChangesRequiringMetadataRefresh;
        }

        if ( knob->getEvaluateOnChange() ) {
            ++_imp->nbSignificantChangesDuringEvaluationBlock;
        }
        ++_imp->nbChangesDuringEvaluationBlock;

    }
} // KnobHolder::appendValueChange

void
KnobHolder::beginChanges()
{
    /*
     * Start a begin/end block, actually blocking all evaluations (renders) but not value changed callback.
     */
    QMutexLocker l(&_imp->evaluationBlockedMutex);
    ++_imp->evaluationBlocked;
}

bool
KnobHolder::isEvaluationBlocked() const
{
    QMutexLocker l(&_imp->evaluationBlockedMutex);

    return _imp->evaluationBlocked > 0;
}

void
KnobHolder::getAllExpressionDependenciesRecursive(std::set<NodePtr >& nodes) const
{
    QMutexLocker k(&_imp->knobsMutex);

    for (KnobsVec::const_iterator it = _imp->knobs.begin(); it != _imp->knobs.end(); ++it) {
        (*it)->getAllExpressionDependenciesRecursive(nodes);
    }
}


void
KnobHolder::beginMultipleEdits(const std::string& commandName)
{
    bool mustCallBeginChanges;
    {
        QMutexLocker l(&_imp->paramsEditLevelMutex);
        KnobHolderPrivate::MultipleParamsEditData data;
        data.commandName = commandName;
        mustCallBeginChanges = _imp->paramsEditStack.empty();
        _imp->paramsEditStack.push_back(data);

    }
    if (mustCallBeginChanges) {
        beginChanges();
    }
}

KnobHolder::MultipleParamsEditEnum
KnobHolder::getMultipleEditsLevel() const
{
    QMutexLocker l(&_imp->paramsEditLevelMutex);
    if (_imp->paramsEditStack.empty()) {
        return eMultipleParamsEditOff;
    }
    const KnobHolderPrivate::MultipleParamsEditData& last = _imp->paramsEditStack.back();
    if (last.nActionsInBracket > 0) {
        return eMultipleParamsEditOn;
    } else {
        return eMultipleParamsEditOnCreateNewCommand;
    }
}

std::string
KnobHolder::getCurrentMultipleEditsCommandName() const
{
    QMutexLocker l(&_imp->paramsEditLevelMutex);
    if (_imp->paramsEditStack.empty()) {
        return std::string();
    }
    return _imp->paramsEditStack.back().commandName;
}

void
KnobHolder::endMultipleEdits()
{
    bool mustCallEndChanges = false;
    {
        QMutexLocker l(&_imp->paramsEditLevelMutex);
        if (_imp->paramsEditStack.empty()) {
            qDebug() << "[BUG]: Call to endMultipleEdits without a matching call to beginMultipleEdits";
            return;
        }
        _imp->paramsEditStack.pop_back();
        mustCallEndChanges = _imp->paramsEditStack.empty();
    }
    if (mustCallEndChanges) {
        endChanges();
    }
}

AppInstancePtr
KnobHolder::getApp() const
{
    return _imp->app.lock();
}

void
KnobHolder::initializeKnobsPublic()
{
    if (_imp->knobsInitialized) {
        return;
    }
    {
        InitializeKnobsFlag_RAII __isInitializingKnobsFlag__( shared_from_this() );
        initializeKnobs();
    }
    _imp->knobsInitialized = true;
}

void
KnobHolder::refreshAfterTimeChange(bool isPlayback,
                                   double time)
{
    assert( QThread::currentThread() == qApp->thread() );
    AppInstancePtr app = getApp();
    if ( !app || app->isGuiFrozen() ) {
        return;
    }
    for (std::size_t i = 0; i < _imp->knobs.size(); ++i) {
        _imp->knobs[i]->onTimeChanged(isPlayback, time);
    }
    if (_imp->knobsTable) {
        _imp->knobsTable->refreshAfterTimeChange(isPlayback, time);
    }
    refreshExtraStateAfterTimeChanged(isPlayback, time);
}

void
KnobHolder::refreshAfterTimeChangeOnlyKnobsWithTimeEvaluation(double time)
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

    for (U32 i = 0; i < _imp->knobs.size(); ++i) {
        if (_imp->knobs[i]->getName() == name) {
            return _imp->knobs[i];
        }
    }

    return KnobIPtr();
}

// Same as getKnobByName expect that if we find the caller, we skip it
KnobIPtr
KnobHolder::getOtherKnobByName(const std::string & name,
                               const KnobIConstPtr& caller) const
{
    QMutexLocker k(&_imp->knobsMutex);

    for (U32 i = 0; i < _imp->knobs.size(); ++i) {
        if (_imp->knobs[i] == caller) {
            continue;
        }
        if (_imp->knobs[i]->getName() == name) {
            return _imp->knobs[i];
        }
    }

    return KnobIPtr();
}

const std::vector< KnobIPtr > &
KnobHolder::getKnobs() const
{
    assert( QThread::currentThread() == qApp->thread() );

    return _imp->knobs;
}

std::vector< KnobIPtr >
KnobHolder::getKnobs_mt_safe() const
{
    QMutexLocker k(&_imp->knobsMutex);

    return _imp->knobs;
}

void
KnobHolder::slaveAllKnobs(const KnobHolderPtr& other)
{
    assert( QThread::currentThread() == qApp->thread() );
    if (_imp->isSlave) {
        return;
    }
    ///Call it prior to slaveTo: it will set the master pointer as pointing to other
    onAllKnobsSlaved(true, other);

    ///When loading a project, we don't need to slave all knobs here because the serialization of each knob separatly
    ///will reslave it correctly if needed
    beginChanges();

    const KnobsVec & otherKnobs = other->getKnobs();
    const KnobsVec & thisKnobs = getKnobs();
    for (std::size_t i = 0; i < otherKnobs.size(); ++i) {
        if ( otherKnobs[i]->isDeclaredByPlugin() || otherKnobs[i]->isUserKnob() ) {
            KnobIPtr foundKnob;
            for (std::size_t j = 0; j < thisKnobs.size(); ++j) {
                if ( thisKnobs[j]->getName() == otherKnobs[i]->getName() ) {
                    foundKnob = thisKnobs[j];
                    break;
                }
            }
            assert(foundKnob);
            if (!foundKnob) {
                continue;
            }
            foundKnob->slaveTo(otherKnobs[i]);
        }
    }
    endChanges();

    _imp->isSlave = true;
}

bool
KnobHolder::isSlave() const
{
    return _imp->isSlave;
}

void
KnobHolder::unslaveAllKnobs()
{
    if (!_imp->isSlave) {
        return;
    }
    const KnobsVec & thisKnobs = getKnobs();
    beginChanges();
    for (std::size_t i = 0; i < thisKnobs.size(); ++i) {
        thisKnobs[i]->unSlave(DimSpec::all(), ViewSetSpec::all(), true /*copyState*/);
    }
    endChanges();
    _imp->isSlave = false;
}

void
KnobHolder::beginKnobsValuesChanged_public(ValueChangedReasonEnum reason)
{
    ///cannot run in another thread.
    assert( QThread::currentThread() == qApp->thread() );

    RECURSIVE_ACTION();
    beginKnobsValuesChanged(reason);
}

void
KnobHolder::endKnobsValuesChanged_public(ValueChangedReasonEnum reason)
{
    ///cannot run in another thread.
    assert( QThread::currentThread() == qApp->thread() );

    RECURSIVE_ACTION();
    endKnobsValuesChanged(reason);
}

bool
KnobHolder::onKnobValueChanged_public(const KnobIPtr& k,
                                      ValueChangedReasonEnum reason,
                                      double time,
                                      ViewSetSpec view,
                                      bool originatedFromMainThread)
{
    ///cannot run in another thread.
    assert( QThread::currentThread() == qApp->thread() );
    if (!_imp->knobsInitialized) {
        return false;
    }
    RECURSIVE_ACTION();

    bool ret = onKnobValueChanged(k, reason, time, view, originatedFromMainThread);
    if (ret) {
        if (QThread::currentThread() == qApp->thread() && originatedFromMainThread && reason != eValueChangedReasonTimeChanged) {
            if (isOverlaySlaveParam(k)) {
                k->redraw();
            }
        }
    }
    return ret;
}

void
KnobHolder::checkIfRenderNeeded()
{
    ///cannot run in another thread.
    assert( QThread::currentThread() == qApp->thread() );
    if ( (getRecursionLevel() == 0) ) {
        endChanges();
    }
}

void
KnobHolder::incrementRedrawNeededCounter()
{
    {
        QMutexLocker k(&_imp->overlayRedrawStackMutex);
        ++_imp->overlayRedrawStack;
    }
}

bool
KnobHolder::checkIfOverlayRedrawNeeded()
{
    {
        QMutexLocker k(&_imp->overlayRedrawStackMutex);
        bool ret = _imp->overlayRedrawStack > 0;
        _imp->overlayRedrawStack = 0;

        return ret;
    }
}


void
KnobHolder::setKnobsFrozen(bool frozen)
{
    {
        QMutexLocker l(&_imp->knobsFrozenMutex);
        if (frozen == _imp->knobsFrozen) {
            return;
        }
        _imp->knobsFrozen = frozen;
    }
    KnobsVec knobs = getKnobs_mt_safe();

    for (U32 i = 0; i < knobs.size(); ++i) {
        knobs[i]->setIsFrozen(frozen);
    }
}

bool
KnobHolder::areKnobsFrozen() const
{
    QMutexLocker l(&_imp->knobsFrozenMutex);

    return _imp->knobsFrozen;
}

bool
KnobHolder::isDequeueingValuesSet() const
{
    {
        QMutexLocker k(&_imp->overlayRedrawStackMutex);

        return _imp->isDequeingValuesSet;
    }
}


double
KnobHolder::getCurrentTime() const
{
    return getApp() ? getApp()->getTimeLine()->currentFrame() : 0;
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
    QMutexLocker k(&_imp->hasAnimationMutex);

    return _imp->hasAnimation;
}

void
KnobHolder::setHasAnimation(bool hasAnimation)
{
    QMutexLocker k(&_imp->hasAnimationMutex);

    _imp->hasAnimation = hasAnimation;
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
    QMutexLocker k(&_imp->hasAnimationMutex);

    _imp->hasAnimation = hasAnimation;
}

void
KnobHolder::appendToHash(double time, ViewIdx view, Hash64* hash)
{
    KnobsVec knobs = getKnobs_mt_safe();
    for (KnobsVec::const_iterator it = knobs.begin(); it!=knobs.end(); ++it) {
        if (!(*it)->getEvaluateOnChange()) {
            continue;
        }
        U64 knobHash = (*it)->computeHash(time, view);
        hash->append(knobHash);

    }

} // appendToHash



/***************************STRING ANIMATION******************************************/



bool
AnimatingKnobStringHelper::cloneExtraData(const KnobIPtr& other,
                                          ViewSetSpec view,
                                          ViewSetSpec otherView,
                                          DimSpec /*dimension*/,
                                          DimSpec /*otherDimension*/,
                                          double offset,
                                          const RangeD* range)
{
    AnimatingKnobStringHelperPtr isAnimatedString = boost::dynamic_pointer_cast<AnimatingKnobStringHelper>(other);
    if (!isAnimatedString) {
        return false;
    }
    assert((view.isAll() && otherView.isAll()) || (!view.isAll() && !otherView.isAll()));
    bool ret = false;
    if (view.isAll()) {
        std::list<ViewIdx> views = getViewsList();
        for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
            ret |= _animation->clone( *isAnimatedString->getStringAnimation(), *it, *it, offset, range );
        }
    } else {
        ret = _animation->clone( *isAnimatedString->getStringAnimation(), ViewIdx(view), ViewIdx(otherView), offset, range );
    }

    return ret;

}

AnimatingKnobStringHelper::AnimatingKnobStringHelper(const KnobHolderPtr& holder,
                                                     const std::string &description,
                                                     int dimension,
                                                     bool declaredByPlugin)
    : KnobStringBase(holder, description, dimension, declaredByPlugin)
    , _animation()
{

}

AnimatingKnobStringHelper::~AnimatingKnobStringHelper()
{
}

void
AnimatingKnobStringHelper::populate()
{
    KnobStringBase::populate();
    _animation.reset(new StringAnimationManager( shared_from_this() ) );
}

void
AnimatingKnobStringHelper::stringToKeyFrameValue(double time,
                                                 ViewIdx view,
                                                 const std::string & v,
                                                 double* returnValue)
{
    _animation->insertKeyFrame(time, view, v, returnValue);
}

void
AnimatingKnobStringHelper::stringFromInterpolatedValue(double interpolated,
                                                       ViewGetSpec view,
                                                       std::string* returnValue) const
{
    Q_UNUSED(view);
    ViewIdx view_i(view);
    if (view.isCurrent()) {
        view_i = getCurrentView();
    }
    _animation->stringFromInterpolatedIndex(interpolated, view_i, returnValue);
}

void
AnimatingKnobStringHelper::onKeyframesRemoved( const std::list<double>& keysRemoved,
                                          ViewSetSpec view,
                                          DimSpec /*dimension*/)
{
    if (view.isAll()) {
        std::list<ViewIdx> views = getViewsList();
        for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
            _animation->removeKeyframes(keysRemoved, *it);
        }
    } else {
        ViewIdx view_i(view);
        if (view.isCurrent()) {
            view_i = getCurrentView();
        }
        _animation->removeKeyframes(keysRemoved, view_i);
    }

}

std::string
AnimatingKnobStringHelper::getStringAtTime(double time,
                                           ViewGetSpec view)
{
    std::string ret;
    ViewIdx view_i(view);
    if (view.isCurrent()) {
        view_i = getCurrentView();
    }
    bool succeeded = false;
    if ( _animation->hasCustomInterp() ) {
        try {
            succeeded = _animation->customInterpolation(time, view_i, &ret);
        } catch (...) {
        }

    }
    if (!succeeded) {
        ret = getValue(DimIdx(0), view_i);
    }

    return ret;
}

void
AnimatingKnobStringHelper::setCustomInterpolation(customParamInterpolationV1Entry_t func,
                                                  void* ofxParamHandle)
{
    _animation->setCustomInterpolation(func, ofxParamHandle);
}

void
AnimatingKnobStringHelper::loadAnimation(const std::map<std::string,std::map<double, std::string> > & keyframes)
{
    std::vector<std::string> projectViews;
    if (getHolder() && getHolder()->getApp()) {
        projectViews = getHolder()->getApp()->getProject()->getProjectViewNames();
    }
    std::map<ViewIdx,std::map<double, std::string> > indexMap;
    for (std::map<std::string,std::map<double, std::string> >::const_iterator it = keyframes.begin(); it != keyframes.end(); ++it) {
        ViewIdx view_i(0);
        Project::getViewIndex(projectViews, it->first, &view_i);
        indexMap[view_i] = it->second;
    }
    _animation->load(indexMap);
}

void
AnimatingKnobStringHelper::saveAnimation(std::map<std::string,std::map<double, std::string> >* keyframes) const
{
    std::map<ViewIdx,std::map<double, std::string> > indexMap;
    _animation->save(&indexMap);

    std::vector<std::string> projectViews;
    if (getHolder() && getHolder()->getApp()) {
        projectViews = getHolder()->getApp()->getProject()->getProjectViewNames();
    }
    for (std::map<ViewIdx,std::map<double, std::string> >::const_iterator it = indexMap.begin(); it != indexMap.end(); ++it) {
        std::string viewName;
        if (it->first >= 0 && it->first < (int)projectViews.size()) {
            viewName = projectViews[it->first];
        } else {
            viewName = "Main";
        }
        (*keyframes)[viewName] = it->second;
    }

}

bool
AnimatingKnobStringHelper::splitView(ViewIdx view)
{
    if (!KnobStringBase::splitView(view)) {
        return false;
    }
    _animation->splitView(view);
    return true;
}

bool
AnimatingKnobStringHelper::unSplitView(ViewIdx view)
{
    if (!KnobStringBase::unSplitView(view)) {
        return false;
    }
    _animation->unSplitView(view);
    return true;

}


/***************************KNOB EXPLICIT TEMPLATE INSTANTIATION******************************************/


template class Knob<int>;
template class Knob<double>;
template class Knob<bool>;
template class Knob<std::string>;

template class AddToUndoRedoStackHelper<int>;
template class AddToUndoRedoStackHelper<double>;
template class AddToUndoRedoStackHelper<bool>;
template class AddToUndoRedoStackHelper<std::string>;

NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_Knob.cpp"
