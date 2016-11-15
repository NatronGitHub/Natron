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

#include "KnobGui.h"

#include <cassert>
#include <stdexcept>

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QAction>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/ViewIdx.h"

#include "Gui/KnobGuiPrivate.h"
#include "Gui/AnimationModuleEditor.h"
#include "Gui/AnimationModule.h"
#include "Gui/NodeAnim.h"
#include "Gui/KnobAnim.h"
#include "Gui/Gui.h"
#include "Gui/KnobGuiContainerHelper.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/KnobUndoCommand.h" // SetExpressionCommand...
#include "Gui/ViewerTab.h"
#include "Gui/ViewerGL.h"

NATRON_NAMESPACE_ENTER;

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

    if (!_imp->customInteract) {
        for (KnobGuiPrivate::PerViewWidgetsMap::const_iterator it = _imp->views.begin(); it != _imp->views.end(); ++it) {
            it->second.widgets->updateGUI();
        }
    } else {
        _imp->customInteract->update();
    }
}

void
KnobGui::onCurveAnimationChangedInternally(const std::list<double>& keysAdded,
                                           const std::list<double>& keysRemoved,
                                           ViewIdx /*view*/,
                                           DimIdx /*dimension*/)
{
    if (!getGui()) {
        return;
    }
    if (!getGui()->getAnimationModuleEditor()) {
        return;
    }
    AnimationModulePtr model = getGui()->getAnimationModuleEditor()->getModel();
    if (!model) {
        return;
    }
    KnobIPtr internalKnob = getKnob();
    if (!internalKnob) {
        return;
    }
    EffectInstancePtr isEffect = toEffectInstance(internalKnob->getHolder());
    if (!isEffect) {
        return;
    }
    NodeAnimPtr node = model->findNodeAnim(isEffect->getNode());
    if (!node) {
        return;
    }

    const std::vector<KnobAnimPtr>& knobs = node->getKnobs();

    KnobAnimPtr knobAnim;
    for (std::vector<KnobAnimPtr>::const_iterator it = knobs.begin(); it!=knobs.end(); ++it) {
        if ((*it)->getInternalKnob() == internalKnob) {
            knobAnim = *it;
            break;
        }
    }

    if (!knobAnim) {
        return;
    }

    if (!keysAdded.empty() || !keysRemoved.empty()) {
        node->getNodeGui()->onKnobKeyFramesChanged(internalKnob, keysAdded, keysRemoved);
    }

    // Refresh the knob anim visibility in a queued connection
    knobAnim->emit_s_refreshKnobVisibilityLater();


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
KnobGui::canPasteKnob(const KnobIPtr& fromKnob, KnobClipBoardType type, DimSpec fromDim, ViewSetSpec /*fromView*/, DimSpec /*targetDimensionIn*/, ViewSetSpec /*targetViewIn*/, bool showErrorDialog)
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

    // If target knob does not support animation, cancel
    if ( !knob->isAnimationEnabled() && (type == eKnobClipBoardTypeCopyAnim) ) {
        if (showErrorDialog) {
            Dialogs::errorDialog( tr("Paste").toStdString(), tr("%1 parameter does not support animation").arg(QString::fromUtf8(knob->getLabel().c_str())).toStdString() );
        }

        return false;
    }

    // If types are incompatible, cancel
    if ( !fromKnob->isTypeCompatible(knob) ) {
        if (showErrorDialog) {
            Dialogs::errorDialog( tr("Paste").toStdString(), tr("You can only copy/paste between parameters of the same type. To overcome this, use an expression instead.").toStdString() );
        }

        return false;
    }

    // If drag & dropping all source dimensions and destination does not have the same amounto of dimensions, fail
    if ( fromDim.isAll() && ( fromKnob->getNDimensions() != knob->getNDimensions() ) ) {
        if (showErrorDialog) {
            Dialogs::errorDialog( tr("Paste").toStdString(), tr("When copy/pasting on all dimensions, original and target parameters must have the same dimension.").toStdString() );
        }
        
        return false;
    }


    return true;
}

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
        std::list<KnobIPtr > knobs;
        knobs.push_back(knob);
        pushUndoCommand( new RestoreDefaultsCommand(knobs, dimension, view) );
    }
}

void
KnobGui::setReadOnly_(bool readOnly,
                      DimSpec dimension, ViewIdx view)
{
    if (!_imp->customInteract) {
        KnobGuiPrivate::PerViewWidgetsMap::const_iterator foundView = _imp->views.find(view);
        if (foundView != _imp->views.end()) {
            if (foundView->second.widgets) {
                foundView->second.widgets->setReadOnly(readOnly, dimension);
            }
        }
    }
    ///This code doesn't work since the knob dimensions are still enabled even if readonly
    bool hasDimensionEnabled = false;
    for (int i = 0; i < getKnob()->getNDimensions(); ++i) {
        if (dimension.isAll() || i == dimension) {
            if ( getKnob()->isEnabled(DimIdx(i)) ) {
                hasDimensionEnabled = true;
            }
        }
    }
    if (_imp->descriptionLabel) {
        _imp->descriptionLabel->setReadOnly(!hasDimensionEnabled);
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
KnobGui::onSetDirty(bool d)
{
    if (!_imp->customInteract) {
        for (KnobGuiPrivate::PerViewWidgetsMap::const_iterator it = _imp->views.begin(); it != _imp->views.end(); ++it) {
            if (it->second.widgets) {
                it->second.widgets->setDirty(d);
            }
        }
    }
}

void
KnobGui::swapOpenGLBuffers()
{
    if (_imp->customInteract) {
        _imp->customInteract->swapOpenGLBuffers();
    }
}

void
KnobGui::redraw()
{
    if (_imp->customInteract) {
        _imp->customInteract->redraw();
    }
}


void
KnobGui::getOpenGLContextFormat(int* depthPerComponents, bool* hasAlpha) const
{
    if (_imp->customInteract) {
        _imp->customInteract->getOpenGLContextFormat(depthPerComponents, hasAlpha);
    } else {
        // Get the viewer OpenGL format
        ViewerTab* lastViewer = _imp->container->getGui()->getActiveViewer();
        if (lastViewer) {
            lastViewer->getViewer()->getOpenGLContextFormat(depthPerComponents, hasAlpha);
        }
    }
}

void
KnobGui::getViewportSize(double &width,
                         double &height) const
{
    if (_imp->customInteract) {
        _imp->customInteract->getViewportSize(width, height);
    }
}

void
KnobGui::getPixelScale(double & xScale,
                       double & yScale) const
{
    if (_imp->customInteract) {
        _imp->customInteract->getPixelScale(xScale, yScale);
    }
}

void
KnobGui::getBackgroundColour(double &r,
                             double &g,
                             double &b) const
{
    if (_imp->customInteract) {
        _imp->customInteract->getBackgroundColour(r, g, b);
    }
}

void
KnobGui::toWidgetCoordinates(double *x,
                             double *y) const
{
    if (_imp->customInteract) {
        _imp->customInteract->toWidgetCoordinates(x, y);
    }
}

void
KnobGui::toCanonicalCoordinates(double *x,
                                double *y) const
{
    if (_imp->customInteract) {
        _imp->customInteract->toCanonicalCoordinates(x, y);
    }
}

/**
 * @brief Returns the font height, i.e: the height of the highest letter for this font
 **/
int
KnobGui::getWidgetFontHeight() const
{
    if (_imp->customInteract) {
        return _imp->customInteract->getWidgetFontHeight();
    }

    return getGui()->fontMetrics().height();
}

/**
 * @brief Returns for a string the estimated pixel size it would take on the widget
 **/
int
KnobGui::getStringWidthForCurrentFont(const std::string& string) const
{
    if (_imp->customInteract) {
        return _imp->customInteract->getStringWidthForCurrentFont(string);
    }

    return getGui()->fontMetrics().width( QString::fromUtf8( string.c_str() ) );
}

void
KnobGui::saveOpenGLContext()
{
    if (_imp->customInteract) {
        _imp->customInteract->saveOpenGLContext();
    }
}

void
KnobGui::restoreOpenGLContext()
{
    if (_imp->customInteract) {
        _imp->customInteract->restoreOpenGLContext();
    }
}

RectD
KnobGui::getViewportRect() const
{
    if (_imp->customInteract) {
        return _imp->customInteract->getViewportRect();
    }

    return RectD();
}

void
KnobGui::getCursorPosition(double& x,
                           double& y) const
{
    if (_imp->customInteract) {
        _imp->customInteract->getCursorPosition(x, y);
    }
}

///Should set to the underlying knob the gui ptr
void
KnobGui::setKnobGuiPointer()
{
    getKnob()->setKnobGuiPointer( shared_from_this() );
}

void
KnobGui::onDoUpdateAnimationLevelLaterReceived()
{
    if (_imp->customInteract || !_imp->refreshAnimationLevelRequests) {
        return;
    }
    _imp->refreshAnimationLevelRequests = 0;
    refreshAnimationLevelNow();
}

void
KnobGui::refreshAnimationLevelNow()
{
    KnobIPtr knob = getKnob();
    int nDim = knob->getNDimensions();
    for (KnobGuiPrivate::PerViewWidgetsMap::const_iterator it = _imp->views.begin(); it != _imp->views.end(); ++it) {
        for (int i = 0; i < nDim; ++i) {
            if (it->second.widgets) {
                it->second.widgets->reflectAnimationLevel(DimIdx(i), knob->getAnimationLevel(DimIdx(i), it->first) );
            }
        }
    }
}

void
KnobGui::onInternalKnobAnimationLevelChanged(ViewSetSpec /*view*/, DimSpec /*dimension*/)
{
    ++_imp->refreshAnimationLevelRequests;
    Q_EMIT s_updateAnimationLevelLater();
}

void
KnobGui::onAppendParamEditChanged(ValueChangedReasonEnum reason,
                                  ValueChangedReturnCodeEnum setValueRetCode,
                                  Variant v,
                                  ViewSetSpec view,
                                  DimSpec dim,
                                  double time,
                                  bool setKeyFrame)
{
    KnobIPtr knob = getKnob();
    if (!knob) {
        return;
    }
    KnobHolderPtr holder = knob->getHolder();
    if (!holder) {
        return;
    }
    bool createNewCommand = holder->getMultipleEditsLevel() == KnobHolder::eMultipleParamsEditOnCreateNewCommand;
    QString commandName = QString::fromUtf8(holder->getCurrentMultipleEditsCommandName().c_str());
    pushUndoCommand( new MultipleKnobEditsUndoCommand(knob, commandName, reason, setValueRetCode, createNewCommand, setKeyFrame, v, dim, time, view) );
}

void
KnobGui::onFrozenChanged(bool frozen)
{
    KnobIPtr knob = getKnob();
    KnobButtonPtr isBtn = toKnobButton(knob);

    if ( isBtn && !isBtn->isRenderButton() ) {
        return;
    }
    int nDim = knob->getNDimensions();
    std::list<ViewIdx> views = knob->getViewsList();
    for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
        for (int i = 0; i < nDim; ++i) {
            if ( frozen || (!knob->isSlave(DimIdx(i), *it) && knob->isEnabled(DimIdx(i), *it)) ) {
                setReadOnly_(frozen, DimIdx(i), *it);
            }
        }
    }
}

bool
KnobGui::isGuiFrozenForPlayback() const
{
    return getGui() ? getGui()->isGUIFrozen() : false;
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
    foundView->second.widgets->reflectExpressionState( dimension, !exp.empty() );
    if ( exp.empty() ) {
        foundView->second.widgets->reflectAnimationLevel( dimension, knob->getAnimationLevel(dimension, view) );
    } else {
        NodeSettingsPanel* isNodeSettings = dynamic_cast<NodeSettingsPanel*>(_imp->container);
        if (isNodeSettings) {
            NodeGuiPtr node = isNodeSettings->getNode();
            if (node) {
                node->onKnobExpressionChanged(this);
            }
        }

        if (_imp->warningIndicator) {
            bool invalid = false;
            QString fullErrToolTip;
            int dims = knob->getNDimensions();
            for (int i = 0; i < dims; ++i) {
                std::string err;
                if ( !knob->isExpressionValid(DimIdx(i), view, &err) ) {
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
    onCurveAnimationChangedInternally(std::list<double>(), std::list<double>(), view, dimension);

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
        _imp->descriptionLabel->setAltered(!hasModif);
    }

    for (KnobGuiPrivate::PerViewWidgetsMap::const_iterator it = _imp->views.begin(); it != _imp->views.end(); ++it) {
        it->second.viewLabel->setAltered(!hasModif);
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
            _imp->descriptionLabel->setText_overload( QString::fromUtf8( descriptionLabel.c_str() ) );
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
            assert(!it->isCurrent());
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

NATRON_NAMESPACE_EXIT;
