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

#include "KnobGui.h"

#include <cassert>
#include <stdexcept>
#include <sstream> // stringstream

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QAction>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/KnobUndoCommand.h"
#include "Engine/ViewIdx.h"
#include "Engine/KnobItemsTable.h"

#include "Gui/KnobGuiPrivate.h"
#include "Gui/AnimationModuleEditor.h"
#include "Gui/AnimationModule.h"
#include "Gui/NodeAnim.h"
#include "Gui/KnobAnim.h"
#include "Gui/Gui.h"
#include "Gui/NodeGraph.h"
#include "Gui/KnobGuiContainerHelper.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/ViewerTab.h"
#include "Gui/ViewerGL.h"

NATRON_NAMESPACE_ENTER

void
KnobGui::onMustRefreshGuiActionTriggered(ViewSetSpec /*view*/, DimSpec /*dimension*/, ValueChangedReasonEnum /*reason*/)
{
    ++_imp->refreshGuiRequests;
    Q_EMIT s_doUpdateGuiLater();
}

void
KnobGui::onDoUpdateGuiLaterReceived()
{
    if (_imp->guiRemoved || !_imp->widgetCreated || !_imp->refreshGuiRequests) {
        return;
    }
    
    _imp->refreshGuiRequests = 0;
    refreshGuiNow();
}

void
KnobGui::refreshGuiNow()
{

    if (getGui()) {
        if (getGui()->isGUIFrozen()) {
            return;
        }
    }
    if (!_imp->customInteract) {
        KnobIPtr knob = getKnob();
        if (!knob) {
            return;
        }
        if (!knob->getHolder()) {
            return;
        }
        TimeValue time = knob->getHolder()->getTimelineCurrentTime();
        int nDims = knob->getNDimensions();
        for (KnobGuiPrivate::PerViewWidgetsMap::const_iterator it = _imp->views.begin(); it != _imp->views.end(); ++it) {
            it->second.widgets->updateGUI();
            for (int i = 0; i < nDims; ++i) {
                it->second.widgets->reflectAnimationLevel(DimIdx(i), knob->getAnimationLevel(DimIdx(i), time, it->first));
            }
        }
    } else {
        _imp->customInteract->update();
    }
}

void
KnobGui::copyAnimationToClipboard(DimSpec dimension, ViewSetSpec view) const
{
    copyToClipBoard(eKnobClipBoardTypeCopyAnim, dimension, view);
}

void
KnobGui::onCopyAnimationActionTriggered()
{
    QAction* act = qobject_cast<QAction*>( sender() );
    if (!act) {
        return;
    }
    ViewSetSpec view;
    DimSpec dimension;
    getDimViewFromActionData(act, &view, &dimension);
    copyAnimationToClipboard(dimension, view);
}

void
KnobGui::copyValuesToClipboard(DimSpec dimension, ViewSetSpec view ) const
{
    copyToClipBoard(eKnobClipBoardTypeCopyValue, dimension, view);
}

void
KnobGui::onCopyValuesActionTriggered()
{
    QAction* act = qobject_cast<QAction*>( sender() );
    if (!act) {
        return;
    }
    ViewSetSpec view;
    DimSpec dimension;
    getDimViewFromActionData(act, &view, &dimension);
    copyValuesToClipboard(dimension, view);
}

void
KnobGui::copyLinkToClipboard(DimSpec dimension, ViewSetSpec view) const
{
    copyToClipBoard(eKnobClipBoardTypeCopyLink, dimension, view);
}

void
KnobGui::onCopyLinksActionTriggered()
{
    QAction* act = qobject_cast<QAction*>( sender() );
    if (!act) {
        return;
    }
    ViewSetSpec view;
    DimSpec dimension;
    getDimViewFromActionData(act, &view, &dimension);
    copyLinkToClipboard(dimension, view);
}

void
KnobGui::copyToClipBoard(KnobClipBoardType type,
                         DimSpec dimension,
                         ViewSetSpec view) const
{
    KnobIPtr knob = getKnob();

    if (!knob) {
        return;
    }

    appPTR->setKnobClipBoard(type, knob, dimension, view);
}


bool
KnobGui::canPasteKnob(const KnobIPtr& fromKnob, KnobClipBoardType type, DimSpec fromDim, ViewSetSpec fromView, DimSpec targetDimensionIn, ViewSetSpec targetViewIn, bool showErrorDialog)
{
    KnobIPtr knob = getKnob();

    if (!knob || !fromKnob) {
        return false;
    }

    // Links are only valid for effects knobs
    if (type == eKnobClipBoardTypeCopyExpressionLink ||
        type == eKnobClipBoardTypeCopyExpressionMultCurveLink ||
        type == eKnobClipBoardTypeCopyLink) {
        EffectInstancePtr fromEffect = toEffectInstance(fromKnob->getHolder());
        EffectInstancePtr toEffect = toEffectInstance(knob->getHolder());

        // Can only link knobs between effects.
        if (!fromEffect || !toEffect) {
            return false;
        }
    }

    // If pasting animation and target knob does not support animation, cancel
    if ( !knob->isAnimationEnabled() && (type == eKnobClipBoardTypeCopyAnim) ) {
        if (showErrorDialog) {
            Dialogs::errorDialog( tr("Paste").toStdString(), tr("%1 parameter does not support animation").arg(QString::fromUtf8(knob->getLabel().c_str())).toStdString() );
        }

        return false;
    }


    // If drag & dropping all source dimensions and destination does not have the same amounto of dimensions, fail
    if ( fromDim.isAll() && ( fromKnob->getNDimensions() != knob->getNDimensions() ) ) {
        if (showErrorDialog) {
            Dialogs::errorDialog( tr("Paste").toStdString(), tr("All dimensions of the original parameter were copied but the target parameter does not have the same amount of dimensions. To overcome this copy them one by one.").toStdString() );
        }

        return false;
    }


    // If pasting links and types are incompatible, cancel
    {
        DimIdx thisDimToCheck = targetDimensionIn.isAll() ? DimIdx(0) : DimIdx(targetDimensionIn);
        ViewIdx thisViewToCheck = targetViewIn.isAll() ? ViewIdx(0): ViewIdx(targetViewIn);
        DimIdx otherDimToCheck = fromDim.isAll() ? DimIdx(0) : DimIdx(fromDim);
        ViewIdx otherViewToCheck = fromView.isAll() ? ViewIdx(0): ViewIdx(fromView);
        std::string error;
        if ( !knob->canLinkWith(fromKnob, thisDimToCheck, thisViewToCheck, otherDimToCheck, otherViewToCheck,&error ) ) {
            if (showErrorDialog && !error.empty()) {
                Dialogs::errorDialog( tr("Paste").toStdString(), error );
            }
            return false;
        }
    }

    return true;
} // canPasteKnob

bool
KnobGui::pasteKnob(const KnobIPtr& fromKnob, KnobClipBoardType type, DimSpec fromDim, ViewSetSpec fromView, DimSpec targetDimension, ViewSetSpec targetView)
{

    if (!canPasteKnob(fromKnob, type, fromDim, fromView, targetDimension, targetView, true)) {
        return false;
    }

    pushUndoCommand( new PasteKnobClipBoardUndoCommand(getKnob(), type, fromDim, targetDimension, fromView, targetView, fromKnob) );
    return true;
} // pasteKnob

void
KnobGui::pasteClipBoard(DimSpec targetDimension, ViewSetSpec view)
{
    KnobIPtr knob = getKnob();

    if (!knob) {
        return;
    }

    //the dimension from which it was copied from
    DimSpec cbDim;
    ViewSetSpec cbView;
    KnobClipBoardType type;
    KnobIPtr fromKnob;
    appPTR->getKnobClipBoard(&type, &fromKnob, &cbDim, &cbView);
    if (!fromKnob) {
        return;
    }
    pasteKnob(fromKnob, type, cbDim, cbView, targetDimension, view);
} // pasteClipBoard



void
KnobGui::onPasteActionTriggered()
{
    QAction* act = qobject_cast<QAction*>( sender() );
    if (!act) {
        return;
    }
    ViewSetSpec view;
    DimSpec dimension;
    getDimViewFromActionData(act, &view, &dimension);
    pasteClipBoard(dimension, view);
}


void
KnobGui::linkTo(DimSpec dimension, ViewSetSpec view)
{
    KnobIPtr thisKnob = getKnob();

    assert(thisKnob);
    EffectInstancePtr isEffect = toEffectInstance( thisKnob->getHolder() );
    if (!isEffect) {
        return;
    }

    std::list<ViewIdx> views = thisKnob->getViewsList();
    for (std::list<ViewIdx>::const_iterator it = views.begin(); it!=views.end(); ++it) {
        if (view.isAll() || ViewIdx(view) == *it) {
            for (int i = 0; i < thisKnob->getNDimensions(); ++i) {
                if ( (i == dimension) || (dimension.isAll()) ) {
                    std::string expr = thisKnob->getExpression(DimIdx(i), *it);
                    if ( !expr.empty() ) {
                        Dialogs::errorDialog( tr("Param Link").toStdString(), tr("This parameter already has an expression set, edit or clear it.").toStdString() );

                        return;
                    }
                }
            }
        }
    }


    LinkToKnobDialog dialog( shared_from_this(), _imp->copyRightClickMenu->parentWidget() );

    if ( dialog.exec() ) {
        KnobIPtr otherKnob = dialog.getSelectedKnobs();
        if (otherKnob) {

            pasteKnob(otherKnob, eKnobClipBoardTypeCopyLink, DimSpec::all(), ViewSetSpec::all(), dimension, view);

        }
    }
} // KnobGui::linkTo

void
KnobGui::onLinkToActionTriggered()
{
    QAction* act = qobject_cast<QAction*>( sender() );
    if (!act) {
        return;
    }
    ViewSetSpec view;
    DimSpec dimension;
    getDimViewFromActionData(act, &view, &dimension);


    linkTo(dimension, view);
}

void
KnobGui::onResetDefaultValuesActionTriggered()
{
    QAction* act = qobject_cast<QAction*>( sender() );
    if (!act) {
        return;
    }
    ViewSetSpec view;
    DimSpec dimension;
    getDimViewFromActionData(act, &view, &dimension);

    resetDefault(dimension, view);
}

void
KnobGui::resetDefault(DimSpec dimension, ViewSetSpec view)
{
    KnobIPtr knob = getKnob();
    KnobButtonPtr isBtn = toKnobButton(knob);
    KnobPagePtr isPage = toKnobPage(knob);
    KnobGroupPtr isGroup = toKnobGroup(knob);
    KnobSeparatorPtr isSeparator = toKnobSeparator(knob);

    if (!isBtn && !isPage && !isGroup && !isSeparator) {
        std::list<KnobIPtr> knobs;
        knobs.push_back(knob);
        pushUndoCommand( new RestoreDefaultsCommand(knobs, dimension, view) );
    }
}



KnobGui::KnobLayoutTypeEnum
KnobGui::getLayoutType() const
{
    return _imp->layoutType;
}


bool
KnobGui::hasWidgetBeenCreated() const
{
    return _imp->widgetCreated;
}

void
KnobGui::onKnobMultipleSelectionChanged(bool d)
{
    if (!_imp->customInteract) {
        for (KnobGuiPrivate::PerViewWidgetsMap::const_iterator it = _imp->views.begin(); it != _imp->views.end(); ++it) {
            if (it->second.widgets) {
                it->second.widgets->reflectMultipleSelection(d);
            }
        }
    }
}



void
KnobGui::setWarningValue(KnobWarningEnum warn,
                         const QString& value)
{
    QString& warning = _imp->warningsMapping[warn];

    if (warning != value) {
        warning = value;
        refreshKnobWarningIndicatorVisibility();
    }
}

void
KnobGui::refreshKnobWarningIndicatorVisibility()
{
    if (!_imp->warningIndicator) {
        return;
    }
    QString fullToolTip;
    bool hasWarning = false;
    for (std::map<KnobGui::KnobWarningEnum, QString>::iterator it = _imp->warningsMapping.begin(); it != _imp->warningsMapping.end(); ++it) {
        if ( !it->second.isEmpty() ) {
            hasWarning = true;
            fullToolTip += QString::fromUtf8("<p>");
            fullToolTip += it->second;
            fullToolTip += QString::fromUtf8("</p>");
        }
    }
    if (hasWarning) {
        _imp->warningIndicator->setToolTip(fullToolTip);
    }
    _imp->warningIndicator->setVisible(hasWarning);
}

void
KnobGui::onExprChanged(DimIdx dimension, ViewIdx view)
{
    if (_imp->guiRemoved || _imp->customInteract) {
        return;
    }
    KnobIPtr knob = getKnob();
    std::string exp = knob->getExpression(dimension, view);

    KnobGuiPrivate::PerViewWidgetsMap::const_iterator foundView = _imp->views.find(view);
    if (foundView == _imp->views.end()) {
        return;
    }
    if (!knob->getHolder()) {
        return;
    }
    foundView->second.widgets->reflectAnimationLevel( dimension, knob->getAnimationLevel(dimension, knob->getHolder()->getTimelineCurrentTime(), view) );
    if ( !exp.empty() ) {

        if (_imp->warningIndicator) {
            bool invalid = false;
            QString fullErrToolTip;
            int dims = knob->getNDimensions();
            for (int i = 0; i < dims; ++i) {
                std::string err;
                if ( !knob->isLinkValid(DimIdx(i), view, &err) ) {
                    invalid = true;
                }
                if ( (dims > 1) && invalid ) {
                    fullErrToolTip += QString::fromUtf8("<p><b>");
                    fullErrToolTip += QString::fromUtf8( knob->getDimensionName(DimIdx(i)).c_str() );
                    fullErrToolTip += QString::fromUtf8("</b></p>");
                }
                if ( !err.empty() ) {
                    fullErrToolTip += QString::fromUtf8( err.c_str() );
                }
            }
            if (invalid) {
                QString toPrepend;
                toPrepend += QString::fromUtf8("<p>");
                toPrepend += tr("Invalid expression(s), value returned is the underlying curve:");
                toPrepend += QString::fromUtf8("</p>");
                fullErrToolTip.prepend(toPrepend);

                setWarningValue(eKnobWarningExpressionInvalid, fullErrToolTip);
            } else {
                setWarningValue( eKnobWarningExpressionInvalid, QString() );
            }
        }
    }
    onHelpChanged();
    foundView->second.widgets->updateGUI();


    // Refresh the animation curve
    knob->getSignalSlotHandler()->s_curveAnimationChanged(view, dimension);

    // Refresh node links
    onInternalKnobLinksChanged();

} // KnobGui::onExprChanged

void
KnobGui::onHelpChanged()
{
    if (_imp->descriptionLabel) {
        toolTip(_imp->descriptionLabel, ViewIdx(0));
    }
    for (KnobGuiPrivate::PerViewWidgetsMap::const_iterator it = _imp->views.begin(); it != _imp->views.end(); ++it) {
        if (it->second.widgets) {
            it->second.widgets->updateToolTip();
        }
    }
}
void
KnobGui::refreshModificationsStateNow()
{
    KnobIPtr knob = getKnob();
    if (!knob) {
        return;
    }
    bool hasModif = knob->hasModifications();
    if (_imp->descriptionLabel) {
        _imp->descriptionLabel->setIsModified(hasModif);
    }

    for (KnobGuiPrivate::PerViewWidgetsMap::const_iterator it = _imp->views.begin(); it != _imp->views.end(); ++it) {
        it->second.viewLabel->setIsModified(hasModif);
        if (it->second.widgets) {
            it->second.widgets->reflectModificationsState();
        }
    }
}

void
KnobGui::onDoUpdateModificationsStateLaterReceived()
{
    if (_imp->guiRemoved || !_imp->refreshModifStateRequests) {
        return;
    }
    _imp->refreshModifStateRequests = 0;

    refreshModificationsStateNow();
}

void
KnobGui::onHasModificationsChanged()
{
    ++_imp->refreshModifStateRequests;
    Q_EMIT s_updateModificationsStateLater();
}

void
KnobGui::onLabelChanged()
{
    if (_imp->descriptionLabel) {
        KnobIPtr knob = getKnob();
        if (!knob) {
            return;
        }
        const std::string& iconLabel = knob->getIconLabel();
        if ( !iconLabel.empty() ) {
            return;
        }

        std::string descriptionLabel;
        if (!_imp->views.empty()) {
            KnobGuiWidgetsPtr firstViewWidget = _imp->views.begin()->second.widgets;
            if (firstViewWidget) {
                descriptionLabel = firstViewWidget->getDescriptionLabel();
            }
        }
        if (descriptionLabel.empty()) {
            _imp->descriptionLabel->hide();
        } else {
            _imp->descriptionLabel->setText( QString::fromUtf8( descriptionLabel.c_str() ) );
            _imp->descriptionLabel->show();
        }
    }
    for (KnobGuiPrivate::PerViewWidgetsMap::const_iterator it = _imp->views.begin(); it != _imp->views.end(); ++it) {
        if (it->second.widgets) {
            it->second.widgets->onLabelChanged();
        }
    }

}

void
KnobGui::onDimensionNameChanged(DimIdx dimension)
{
    for (KnobGuiPrivate::PerViewWidgetsMap::const_iterator it = _imp->views.begin(); it != _imp->views.end(); ++it) {
        if (it->second.widgets) {
            it->second.widgets->refreshDimensionName(dimension);
        }
    }
}


void
KnobGui::onRefreshDimensionsVisibilityLaterReceived()
{
    if (_imp->refreshDimensionVisibilityRequests.empty()) {
        return;
    }
    KnobIPtr knob = getKnob();
    if (!knob) {
        return;
    }
    std::set<ViewIdx> views;
    for (std::list<ViewSetSpec>::const_iterator it = _imp->refreshDimensionVisibilityRequests.begin(); it != _imp->refreshDimensionVisibilityRequests.end(); ++it) {
        if (it->isAll()) {
            std::list<ViewIdx> allViews = knob->getViewsList();
            for (std::list<ViewIdx>::const_iterator it = allViews.begin(); it != allViews.end(); ++it) {
                views.insert(*it);
            }
            break;
        } else {
            views.insert(ViewIdx(*it));
        }
    }
    for (std::set<ViewIdx>::const_iterator it = views.begin(); it!=views.end(); ++it) {
        KnobGuiPrivate::PerViewWidgetsMap::const_iterator found = _imp->views.find(*it);
        if (found == _imp->views.end()) {
            continue;
        }
        if (found->second.widgets) {
            found->second.widgets->setAllDimensionsVisible(knob->getAllDimensionsVisible(found->first));
        }
    }
}

void
KnobGui::onInternalKnobDimensionsVisibilityChanged(ViewSetSpec view)
{
    _imp->refreshDimensionVisibilityRequests.push_back(view);
    Q_EMIT s_refreshDimensionsVisibilityLater();

}

void
KnobGui::onInternalKnobLinksChanged()
{
    KnobIPtr knob = getKnob();
    if (!knob) {
        return;
    }

    // Refresh help tooltip
    onHelpChanged();

    KnobHolderPtr holder = knob->getHolder();
    EffectInstancePtr holderIsEffect = toEffectInstance(holder);
    KnobTableItemPtr holderIsItem = toKnobTableItem(holder);
    if (holderIsItem) {
        holderIsEffect = holderIsItem->getModel()->getNode()->getEffectInstance();
    }

    NodePtr holderNode;
    if (holderIsEffect) {
        holderNode = holderIsEffect->getNode();
    }
    NodeGroupPtr holderIsGroup;
    NodesList groupNodes;
    if (holderNode) {
        holderIsGroup = toNodeGroup(holderIsEffect);
        if (holderIsGroup) {
            groupNodes = holderIsGroup->getNodes();
        }
    }

    if (!_imp->customInteract) {


        // Refresh the linked state of the KnobGui.
        // We cycle through each view and dimension and check the shared knobs.
        // Appear linked if there's at least one shared knob. For Groups, since a PyPlug may have by default some parameters linked,
        // we do not count the links to internal nodes to figure out if we need to appear linked or not.
        int nDims = knob->getNDimensions();
        for (KnobGuiPrivate::PerViewWidgetsMap::const_iterator it = _imp->views.begin(); it != _imp->views.end(); ++it) {
            for (int i = 0;i < nDims; ++i) {
                KnobDimViewKeySet sharedKnobs;
                knob->getSharedValues(DimIdx(i), it->first, &sharedKnobs);

                bool appearLinked = false;
                for (KnobDimViewKeySet::const_iterator it2 = sharedKnobs.begin(); it2 != sharedKnobs.end(); ++it2) {
                    KnobIPtr sharedK = it2->knob.lock();
                    if (!sharedK) {
                        continue;
                    }

                    KnobHolderPtr sharedHolder = sharedK->getHolder();
                    EffectInstancePtr sharedHolderIsEffect = toEffectInstance(sharedHolder);
                    KnobTableItemPtr sharedHolderIsItem = toKnobTableItem(sharedHolder);
                    if (sharedHolderIsItem) {
                        int colIndex = sharedHolderIsItem->getKnobColumnIndex(sharedK, DimIdx(i));
                        if (colIndex == -1) {
                            // If the knob is a knob of a table item and it does not belong to any column of the table
                            // then do not attempt to display the link
                            continue;
                        }
                        sharedHolderIsEffect = sharedHolderIsItem->getModel()->getNode()->getEffectInstance();
                    }
                    if (!sharedHolderIsEffect || !sharedHolderIsEffect->getNode()->isPersistent()) {
                        continue;
                    }

                    // If the shared knob holder is a child node of a Group, don't count it
                    NodePtr sharedHolderNode = sharedHolderIsEffect->getNode();
                    if (!sharedHolderNode) {
                        continue;
                    }
                    if (std::find(groupNodes.begin(), groupNodes.end(), sharedHolderNode) != groupNodes.end()) {
                        continue;
                    }

                    appearLinked = true;
                }
                it->second.widgets->reflectLinkedState(DimIdx(i), appearLinked);
            }
        }
    }
    
    if (!holderNode) {
        return;
    }
    NodeGuiPtr node = toNodeGui(holderNode->getNodeGui());
    if (!node) {
        return;
    }
    node->getDagGui()->refreshNodeLinksLater();
} // onInternalKnobLinksChanged


NATRON_NAMESPACE_EXIT
