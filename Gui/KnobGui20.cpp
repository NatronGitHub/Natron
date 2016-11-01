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
KnobGui::onMustRefreshGuiActionTriggered(ViewSetSpec view, DimSpec dimension, ValueChangedReasonEnum /*reason*/)
{
#pragma message WARN("Make this slot in a queued connection to avoid many redraws requests")
    if (_imp->guiRemoved) {
        return;
    }
    if (_imp->widgetCreated) {
        updateGuiInternal(dimension, view);
    }
}


void
KnobGui::onCurveAnimationChangedInternally(const std::list<double>& keysAdded,
                                           const std::list<double>& keysRemoved,
                                           ViewIdx /*view*/,
                                           DimIdx /*dimension*/)
{
    AnimationModulePtr model = getGui()->getAnimationModuleEditor()->getModel();
    if (!model) {
        return;
    }
    KnobIPtr internalKnob = getKnob();
    if (!internalKnob) {
        return;
    }
    EffectInstancePtr isEffect = toEffectInstance(internalKnob->getHolder());
    if (isEffect) {
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


    node->getNodeGui()->onKnobKeyFramesChanged(internalKnob, keysAdded, keysRemoved);

    // Refresh the knob anim visibility in a queued connection
    knobAnim->onKnobSignalReceivedInDirectConnection();


}


void
KnobGui::copyAnimationToClipboard(DimSpec dimension, ViewIdx view) const
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
    ViewIdx view;
    DimSpec dimension;
    getDimViewFromActionData(act, &view, &dimension);
    copyAnimationToClipboard(dimension, view);
}

void
KnobGui::copyValuesToClipboard(DimSpec dimension, ViewIdx view ) const
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
    ViewIdx view;
    DimSpec dimension;
    getDimViewFromActionData(act, &view, &dimension);
    copyValuesToClipboard(dimension, view);
}

void
KnobGui::copyLinkToClipboard(DimSpec dimension, ViewIdx view) const
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
    ViewIdx view;
    DimSpec dimension;
    getDimViewFromActionData(act, &view, &dimension);
    copyLinkToClipboard(dimension, view);
}

void
KnobGui::copyToClipBoard(KnobClipBoardType type,
                         DimSpec dimension,
                         ViewIdx view) const
{
    KnobIPtr knob = getKnob();

    if (!knob) {
        return;
    }

    appPTR->setKnobClipBoard(type, knob, dimension, view);
}

void
KnobGui::pasteClipBoard(DimSpec targetDimension, ViewIdx view)
{
    KnobIPtr knob = getKnob();

    if (!knob) {
        return;
    }

    //the dimension from which it was copied from
    DimSpec cbDim;
    ViewIdx cbView;
    KnobClipBoardType type;
    KnobIPtr fromKnob;
    appPTR->getKnobClipBoard(&type, &fromKnob, &cbDim, &cbView);
    if (!fromKnob) {
        return;
    }

    if ( (targetDimension == 0) && !getAllDimensionsVisible(view) ) {
        targetDimension = DimSpec::all();
    }

    if ( !knob->isAnimationEnabled() && (type == eKnobClipBoardTypeCopyAnim) ) {
        Dialogs::errorDialog( tr("Paste").toStdString(), tr("This parameter does not support animation").toStdString() );

        return;
    }

    if ( !KnobI::areTypesCompatibleForSlave( fromKnob, knob ) ) {
        Dialogs::errorDialog( tr("Paste").toStdString(), tr("You can only copy/paste between parameters of the same type. To overcome this, use an expression instead.").toStdString() );

        return;
    }

    if ( !cbDim.isAll() && targetDimension.isAll() ) {
        Dialogs::errorDialog( tr("Paste").toStdString(), tr("When copy/pasting on all dimensions, original and target parameters must have the same dimension.").toStdString() );

        return;
    }

    if ( ( targetDimension.isAll() || cbDim.isAll()) && ( fromKnob->getNDimensions() != knob->getNDimensions() ) ) {
        Dialogs::errorDialog( tr("Paste").toStdString(), tr("When copy/pasting on all dimensions, original and target parameters must have the same dimension.").toStdString() );

        return;
    }

    pushUndoCommand( new PasteKnobClipBoardUndoCommand(knob, type, cbDim, targetDimension, cbView, ViewSetSpec(view), fromKnob) );
} // pasteClipBoard

void
KnobGui::onPasteActionTriggered()
{
    QAction* act = qobject_cast<QAction*>( sender() );
    if (!act) {
        return;
    }
    ViewIdx view;
    DimSpec dimension;
    getDimViewFromActionData(act, &view, &dimension);
    pasteClipBoard(dimension, view);
}


void
KnobGui::linkTo(DimSpec dimension, ViewIdx view)
{
    KnobIPtr thisKnob = getKnob();

    assert(thisKnob);
    EffectInstancePtr isEffect = toEffectInstance( thisKnob->getHolder() );
    if (!isEffect) {
        return;
    }


    for (int i = 0; i < thisKnob->getNDimensions(); ++i) {
        if ( (i == dimension) || (dimension.isAll()) ) {
            std::string expr = thisKnob->getExpression(DimIdx(i), view);
            if ( !expr.empty() ) {
                Dialogs::errorDialog( tr("Param Link").toStdString(), tr("This parameter already has an expression set, edit or clear it.").toStdString() );

                return;
            }
        }
    }

    LinkToKnobDialog dialog( shared_from_this(), _imp->copyRightClickMenu->parentWidget() );

    if ( dialog.exec() ) {
        KnobIPtr otherKnob = dialog.getSelectedKnobs();
        if (otherKnob) {
            if ( !thisKnob->isTypeCompatible(otherKnob) ) {
                Dialogs::errorDialog( tr("Param Link").toStdString(), tr("Types are incompatible!").toStdString() );

                return;
            }


            for (int i = 0; i < thisKnob->getNDimensions(); ++i) {
                if ( (i == dimension) || (dimension.isAll()) ) {
                    MasterKnobLink linkData;
                    if (thisKnob->getMaster(DimIdx(i), view, &linkData)) {

                        KnobIPtr masterKnob = linkData.masterKnob.lock();
                        Dialogs::errorDialog( tr("Param Link").toStdString(),
                                             tr("Cannot link %1 because the knob is already linked to %2.")
                                             .arg( QString::fromUtf8( thisKnob->getLabel().c_str() ) )
                                             .arg( QString::fromUtf8( masterKnob->getLabel().c_str() ) )
                                             .toStdString() );

                        return;
                    }
                }
            }

            EffectInstancePtr otherEffect = toEffectInstance( otherKnob->getHolder() );
            if (!otherEffect) {
                return;
            }

            std::stringstream expr;
            NodeCollectionPtr thisCollection = isEffect->getNode()->getGroup();
            NodeGroupPtr otherIsGroup = toNodeGroup(otherEffect);
            if ( otherIsGroup == thisCollection ) {
                expr << "thisGroup"; // make expression generic if possible
            } else {
                expr << otherEffect->getNode()->getFullyQualifiedName();
            }
            expr << "." << otherKnob->getName() << ".get()";
            if (otherKnob->getNDimensions() > 1) {
                expr << "[dimension]";
            }

            thisKnob->setExpression(dimension, view, expr.str(), false, false);
            thisKnob->getHolder()->getApp()->triggerAutoSave();
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
    ViewIdx view;
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
    ViewIdx view;
    DimSpec dimension;
    getDimViewFromActionData(act, &view, &dimension);

    resetDefault(dimension, view);
}

void
KnobGui::resetDefault(DimSpec dimension, ViewIdx view)
{
    KnobIPtr knob = getKnob();
    KnobButtonPtr isBtn = toKnobButton(knob);
    KnobPagePtr isPage = toKnobPage(knob);
    KnobGroupPtr isGroup = toKnobGroup(knob);
    KnobSeparatorPtr isSeparator = toKnobSeparator(knob);

    if (!isBtn && !isPage && !isGroup && !isSeparator) {
        std::list<KnobIPtr > knobs;
        knobs.push_back(knob);
        pushUndoCommand( new RestoreDefaultsCommand(false, knobs, dimension, view) );
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
KnobGui::onAnimationLevelChanged(ViewSetSpec view, DimSpec dimension)
{
#pragma message WARN("Make this a queued connection")
    if (_imp->customInteract) {
        return;
    }
    KnobIPtr knob = getKnob();
    int nDim = knob->getNDimensions();
    if (dimension.isAll()) {
        if (view.isAll()) {
            for (KnobGuiPrivate::PerViewWidgetsMap::const_iterator it = _imp->views.begin(); it != _imp->views.end(); ++it) {
                for (int i = 0; i < nDim; ++i) {
                    if (it->second.widgets) {
                        it->second.widgets->reflectAnimationLevel(DimIdx(i), knob->getAnimationLevel(DimIdx(i), it->first) );
                    }
                }
            }
        } else {
            ViewIdx view_i = knob->getViewIdxFromGetSpec(ViewGetSpec(view));
            KnobGuiPrivate::PerViewWidgetsMap::const_iterator foundView = _imp->views.find(view_i);
            if (foundView != _imp->views.end()) {
                for (int i = 0; i < nDim; ++i) {
                    foundView->second.widgets->reflectAnimationLevel(DimIdx(i), knob->getAnimationLevel(DimIdx(i), view_i) );
                }
            }
        }
    } else {
        if (view.isAll()) {
            for (KnobGuiPrivate::PerViewWidgetsMap::const_iterator it = _imp->views.begin(); it != _imp->views.end(); ++it) {
                if (it->second.widgets) {
                    it->second.widgets->reflectAnimationLevel(DimIdx(dimension), knob->getAnimationLevel(DimIdx(dimension), it->first) );
                }
            }
        } else {
            ViewIdx view_i = knob->getViewIdxFromGetSpec(ViewGetSpec(view));
            KnobGuiPrivate::PerViewWidgetsMap::const_iterator foundView = _imp->views.find(view_i);
            if (foundView != _imp->views.end()) {
                foundView->second.widgets->reflectAnimationLevel(DimIdx(dimension), knob->getAnimationLevel(DimIdx(dimension), view_i) );
            }
        }
    }
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
    if (knob) {
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
    foundView->second.widgets->updateGUI(dimension);
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
KnobGui::onHasModificationsChanged()
{
    if (_imp->guiRemoved) {
        return;
    }
    if (_imp->descriptionLabel) {
        bool hasModif = getKnob()->hasModifications();
        _imp->descriptionLabel->setAltered(!hasModif);
    }
    if (!_imp->customInteract) {
        for (KnobGuiPrivate::PerViewWidgetsMap::const_iterator it = _imp->views.begin(); it != _imp->views.end(); ++it) {
            if (it->second.widgets) {
                it->second.widgets->reflectModificationsState();
            }
        }
    }
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

NATRON_NAMESPACE_EXIT;
