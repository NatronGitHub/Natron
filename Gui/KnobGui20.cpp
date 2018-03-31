/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
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

#include "KnobGui.h"

#include <cassert>
#include <stdexcept>
#include <sstream> // stringstream

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QAction>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/ViewIdx.h"

#include "Gui/KnobGuiPrivate.h"
#include "Gui/Gui.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/KnobUndoCommand.h" // SetExpressionCommand...


NATRON_NAMESPACE_ENTER

void
KnobGui::onInternalValueChanged(ViewSpec /*view*/,
                                int dimension,
                                int reason)
{
    if (_imp->guiRemoved) {
        return;
    }
    if ( _imp->widgetCreated && ( (ValueChangedReasonEnum)reason != eValueChangedReasonUserEdited ) ) {
        updateGuiInternal(dimension);
        if ( !getKnob()->getExpression(dimension).empty() ) {
            Q_EMIT refreshCurveEditor();
        }
    }
}

void
KnobGui::updateCurveEditorKeyframes()
{
    Q_EMIT keyFrameSet();
}

void
KnobGui::onMultipleKeySet(const std::list<double>& keys,
                          ViewSpec /*view*/,
                          int /*dimension*/,
                          int reason)
{
    if ( (ValueChangedReasonEnum)reason != eValueChangedReasonUserEdited ) {
        KnobIPtr knob = getKnob();
        if ( !knob->getIsSecret() && ( knob->isDeclaredByPlugin()  || knob->isUserKnob() ) ) {
            std::list<SequenceTime> intKeys;
            for (std::list<double>::const_iterator it = keys.begin(); it != keys.end(); ++it) {
                intKeys.push_back(*it);
            }
            knob->getHolder()->getApp()->addMultipleKeyframeIndicatorsAdded(intKeys, true);
        }
    }

    updateCurveEditorKeyframes();
}

void
KnobGui::onMultipleKeyRemoved(const std::list<double>& keys,
                              ViewSpec /*view*/,
                              int /*dimension*/,
                              int reason)
{
    if ( (ValueChangedReasonEnum)reason != eValueChangedReasonUserEdited ) {
        KnobIPtr knob = getKnob();
        if ( !knob->getIsSecret() && ( knob->isDeclaredByPlugin()  || knob->isUserKnob() ) ) {
            std::list<SequenceTime> intKeys;
            for (std::list<double>::const_iterator it = keys.begin(); it != keys.end(); ++it) {
                intKeys.push_back(*it);
            }
            knob->getHolder()->getApp()->removeMultipleKeyframeIndicator(intKeys, true);
        }
    }

    Q_EMIT keyFrameRemoved();
}

void
KnobGui::onInternalKeySet(double time,
                          ViewSpec /*view*/,
                          int /*dimension*/,
                          int reason,
                          bool added )
{
    if ( (ValueChangedReasonEnum)reason != eValueChangedReasonUserEdited ) {
        if (added) {
            KnobIPtr knob = getKnob();
            if ( !knob->getIsSecret() && ( knob->isDeclaredByPlugin() || knob->isUserKnob() ) ) {
                knob->getHolder()->getApp()->addKeyframeIndicator(time);
            }
        }
    }

    updateCurveEditorKeyframes();
}

void
KnobGui::onInternalKeyRemoved(double time,
                              ViewSpec /*view*/,
                              int /*dimension*/,
                              int /*reason*/)
{
    KnobIPtr knob = getKnob();

    if ( !knob->getIsSecret() && ( knob->isDeclaredByPlugin() || knob->isUserKnob() ) ) {
        knob->getHolder()->getApp()->removeKeyFrameIndicator(time);
    }
    Q_EMIT keyFrameRemoved();
}

void
KnobGui::copyAnimationToClipboard(int dimension) const
{
    copyToClipBoard(eKnobClipBoardTypeCopyAnim, dimension);
}

void
KnobGui::onCopyAnimationActionTriggered()
{
    QAction* act = qobject_cast<QAction*>( sender() );

    if (!act) {
        return;
    }
    int dim = act->data().toInt();
    copyAnimationToClipboard(dim);
}

void
KnobGui::copyValuesToClipboard(int dimension ) const
{
    copyToClipBoard(eKnobClipBoardTypeCopyValue, dimension);
}

void
KnobGui::onCopyValuesActionTriggered()
{
    QAction* act = qobject_cast<QAction*>( sender() );

    if (!act) {
        return;
    }
    int dim = act->data().toInt();
    copyValuesToClipboard(dim);
}

void
KnobGui::copyLinkToClipboard(int dimension) const
{
    copyToClipBoard(eKnobClipBoardTypeCopyLink, dimension);
}

void
KnobGui::onCopyLinksActionTriggered()
{
    QAction* act = qobject_cast<QAction*>( sender() );

    if (!act) {
        return;
    }
    int dim = act->data().toInt();
    copyLinkToClipboard(dim);
}

void
KnobGui::copyToClipBoard(KnobClipBoardType type,
                         int dimension) const
{
    KnobIPtr knob = getKnob();

    if (!knob) {
        return;
    }

    appPTR->setKnobClipBoard(type, knob, dimension);
}

void
KnobGui::pasteClipBoard(int targetDimension)
{
    KnobIPtr knob = getKnob();

    if (!knob) {
        return;
    }

    //the dimension from which it was copied from
    int cbDim;
    KnobClipBoardType type;
    KnobIPtr fromKnob;
    appPTR->getKnobClipBoard(&type, &fromKnob, &cbDim);
    if (!fromKnob) {
        return;
    }

    if ( (targetDimension == 0) && !getAllDimensionsVisible() ) {
        targetDimension = -1;
    }

    if ( !knob->isAnimationEnabled() && (type == eKnobClipBoardTypeCopyAnim) ) {
        Dialogs::errorDialog( tr("Paste").toStdString(), tr("This parameter does not support animation").toStdString() );

        return;
    }

    if ( !KnobI::areTypesCompatibleForSlave( fromKnob.get(), knob.get() ) ) {
        Dialogs::errorDialog( tr("Paste").toStdString(), tr("You can only copy/paste between parameters of the same type. To overcome this, use an expression instead.").toStdString() );

        return;
    }

    if ( (cbDim != -1) && (targetDimension == -1) ) {
        Dialogs::errorDialog( tr("Paste").toStdString(), tr("When copy/pasting on all dimensions, original and target parameters must have the same dimension.").toStdString() );

        return;
    }

    if ( ( (targetDimension == -1) || (cbDim == -1) ) && ( fromKnob->getDimension() != knob->getDimension() ) ) {
        Dialogs::errorDialog( tr("Paste").toStdString(), tr("When copy/pasting on all dimensions, original and target parameters must have the same dimension.").toStdString() );

        return;
    }

    pushUndoCommand( new PasteUndoCommand(shared_from_this(), type, cbDim, targetDimension, fromKnob) );
} // pasteClipBoard

void
KnobGui::onPasteActionTriggered()
{
    QAction* act = qobject_cast<QAction*>( sender() );

    if (!act) {
        return;
    }

    pasteClipBoard( act->data().toInt() );
}

void
KnobGui::onKnobSlavedChanged(int /*dimension*/,
                             bool b)
{
    if (b) {
        Q_EMIT keyFrameRemoved();
    } else {
        Q_EMIT keyFrameSet();
    }
}

void
KnobGui::linkTo(int dimension)
{
    KnobIPtr thisKnob = getKnob();

    assert(thisKnob);
    EffectInstance* isEffect = dynamic_cast<EffectInstance*>( thisKnob->getHolder() );
    if (!isEffect) {
        return;
    }


    for (int i = 0; i < thisKnob->getDimension(); ++i) {
        if ( (i == dimension) || (dimension == -1) ) {
            std::string expr = thisKnob->getExpression(dimension);
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


            for (int i = 0; i < thisKnob->getDimension(); ++i) {
                std::pair<int, KnobIPtr> existingLink = thisKnob->getMaster(i);
                if (existingLink.second) {
                    Dialogs::errorDialog( tr("Param Link").toStdString(),
                                          tr("Cannot link %1 because the knob is already linked to %2.")
                                          .arg( QString::fromUtf8( thisKnob->getLabel().c_str() ) )
                                          .arg( QString::fromUtf8( existingLink.second->getLabel().c_str() ) )
                                          .toStdString() );

                    return;
                }
            }

            EffectInstance* otherEffect = dynamic_cast<EffectInstance*>( otherKnob->getHolder() );
            if (!otherEffect) {
                return;
            }

            std::stringstream expr;
            NodeCollectionPtr thisCollection = isEffect->getNode()->getGroup();
            NodeGroup* otherIsGroup = dynamic_cast<NodeGroup*>(otherEffect);
            if ( otherIsGroup == thisCollection.get() ) {
                expr << "thisGroup"; // make expression generic if possible
            } else {
                expr << otherEffect->getNode()->getFullyQualifiedName();
            }
            expr << "." << otherKnob->getName() << ".get()";
            if (otherKnob->getDimension() > 1) {
                expr << "[dimension]";
            }

            thisKnob->beginChanges();
            for (int i = 0; i < thisKnob->getDimension(); ++i) {
                if ( (i == dimension) || (dimension == -1) ) {
                    thisKnob->setExpression(i, expr.str(), false, false);
                }
            }
            thisKnob->endChanges();


            thisKnob->getHolder()->getApp()->triggerAutoSave();
        }
    }
} // KnobGui::linkTo

void
KnobGui::onLinkToActionTriggered()
{
    QAction* action = qobject_cast<QAction*>( sender() );

    assert(action);

    linkTo( action->data().toInt() );
}

void
KnobGui::onResetDefaultValuesActionTriggered()
{
    QAction *action = qobject_cast<QAction *>( sender() );

    if (action) {
        resetDefault( action->data().toInt() );
    }
}

void
KnobGui::resetDefault(int dimension)
{
    KnobIPtr knob = getKnob();
    KnobButton* isBtn = dynamic_cast<KnobButton*>( knob.get() );
    KnobPage* isPage = dynamic_cast<KnobPage*>( knob.get() );
    KnobGroup* isGroup = dynamic_cast<KnobGroup*>( knob.get() );
    KnobSeparator* isSeparator = dynamic_cast<KnobSeparator*>( knob.get() );

    if (!isBtn && !isPage && !isGroup && !isSeparator) {
        std::list<KnobIPtr> knobs;
        knobs.push_back(knob);
        pushUndoCommand( new RestoreDefaultsCommand(false, knobs, dimension) );
    }
}

void
KnobGui::setReadOnly_(bool readOnly,
                      int dimension)
{
    if (!_imp->customInteract) {
        setReadOnly(readOnly, dimension);
    }
    ///This code doesn't work since the knob dimensions are still enabled even if readonly
    bool hasDimensionEnabled = false;
    for (int i = 0; i < getKnob()->getDimension(); ++i) {
        if ( getKnob()->isEnabled(i) ) {
            hasDimensionEnabled = true;
        }
    }
    if (_imp->descriptionLabel) {
        _imp->descriptionLabel->setReadOnly(!hasDimensionEnabled);
    }
}

bool
KnobGui::triggerNewLine() const
{
    return _imp->triggerNewLine;
}

bool
KnobGui::isViewerUIKnob() const
{
    return _imp->isInViewerUIKnob;
}

void
KnobGui::turnOffNewLine()
{
    _imp->triggerNewLine = false;
}

/*Set the spacing between items in the layout*/
void
KnobGui::setSpacingBetweenItems(int spacing)
{
    _imp->spacingBetweenItems = spacing;
}

int
KnobGui::getSpacingBetweenItems() const
{
    return _imp->spacingBetweenItems;
}

bool
KnobGui::hasWidgetBeenCreated() const
{
    return _imp->widgetCreated;
}

void
KnobGui::onSetValueUsingUndoStack(const Variant & v,
                                  ViewSpec /*view*/,
                                  int dim)
{
    KnobIPtr knob = getKnob();

    KnobIntBase* isInt = dynamic_cast<KnobIntBase*>( knob.get() );
    KnobBoolBase* isBool = dynamic_cast<KnobBoolBase*>( knob.get() );
    KnobDoubleBase* isDouble = dynamic_cast<KnobDoubleBase*>( knob.get() );
    KnobStringBase* isString = dynamic_cast<KnobStringBase*>( knob.get() );

    if (isInt) {
        pushUndoCommand( new KnobUndoCommand<int>(shared_from_this(), isInt->getValue(dim), v.toInt(), dim) );
    } else if (isBool) {
        pushUndoCommand( new KnobUndoCommand<bool>(shared_from_this(), isBool->getValue(dim), v.toBool(), dim) );
    } else if (isDouble) {
        pushUndoCommand( new KnobUndoCommand<double>(shared_from_this(), isDouble->getValue(dim), v.toDouble(), dim) );
    } else if (isString) {
        pushUndoCommand( new KnobUndoCommand<std::string>(shared_from_this(), isString->getValue(dim), v.toString().toStdString(), dim) );
    }
}

void
KnobGui::onSetDirty(bool d)
{
    if (!_imp->customInteract) {
        setDirty(d);
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

#ifdef OFX_EXTENSIONS_NATRON
double
KnobGui::getScreenPixelRatio() const
{
    if (_imp->customInteract) {
        return _imp->customInteract->getScreenPixelRatio();
    }
    return 1.;
}
#endif

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
KnobGui::onInternalAnimationAboutToBeRemoved(ViewSpec /*view*/,
                                             int dimension)
{
    removeAllKeyframeMarkersOnTimeline(dimension);
}

void
KnobGui::onInternalAnimationRemoved()
{
    Q_EMIT keyFrameRemoved();
}

void
KnobGui::removeAllKeyframeMarkersOnTimeline(int dimension)
{
    KnobIPtr knob = getKnob();

    if ( knob->getHolder() && knob->getHolder()->getApp() && !knob->getIsSecret() && ( knob->isDeclaredByPlugin() || knob->isUserKnob() ) ) {
        AppInstancePtr app = knob->getHolder()->getApp();
        assert(app);
        std::list<SequenceTime> times;
        std::set<SequenceTime> tmpTimes;
        if (dimension == -1) {
            int dim = knob->getDimension();
            for (int i = 0; i < dim; ++i) {
                boost::shared_ptr<Curve> curve = knob->getCurve(ViewIdx(0), i);
                if (curve) {
                    KeyFrameSet kfs = curve->getKeyFrames_mt_safe();
                    for (KeyFrameSet::iterator it = kfs.begin(); it != kfs.end(); ++it) {
                        tmpTimes.insert( it->getTime() );
                    }
                }
            }
            for (std::set<SequenceTime>::iterator it = tmpTimes.begin(); it != tmpTimes.end(); ++it) {
                times.push_back(*it);
            }
        } else {
            boost::shared_ptr<Curve> curve = knob->getCurve(ViewIdx(0), dimension);
            if (curve) {
                KeyFrameSet kfs = curve->getKeyFrames_mt_safe();
                for (KeyFrameSet::iterator it = kfs.begin(); it != kfs.end(); ++it) {
                    times.push_back( it->getTime() );
                }
            }
        }
        if ( !times.empty() ) {
            app->removeMultipleKeyframeIndicator(times, true);
        }
    }
}

void
KnobGui::setAllKeyframeMarkersOnTimeline(int dimension)
{
    KnobIPtr knob = getKnob();
    AppInstancePtr app = knob->getHolder()->getApp();

    assert(app);
    std::list<SequenceTime> times;

    if (dimension == -1) {
        int dim = knob->getDimension();
        for (int i = 0; i < dim; ++i) {
            KeyFrameSet kfs = knob->getCurve(ViewIdx(0), i)->getKeyFrames_mt_safe();
            for (KeyFrameSet::iterator it = kfs.begin(); it != kfs.end(); ++it) {
                times.push_back( it->getTime() );
            }
        }
    } else {
        KeyFrameSet kfs = knob->getCurve(ViewIdx(0), dimension)->getKeyFrames_mt_safe();
        for (KeyFrameSet::iterator it = kfs.begin(); it != kfs.end(); ++it) {
            times.push_back( it->getTime() );
        }
    }
    app->addMultipleKeyframeIndicatorsAdded(times, true);
}

void
KnobGui::setKeyframeMarkerOnTimeline(double time)
{
    KnobIPtr knob = getKnob();

    if ( knob->isDeclaredByPlugin() || knob->isUserKnob() ) {
        knob->getHolder()->getApp()->addKeyframeIndicator(time);
    }
}

void
KnobGui::onKeyFrameMoved(ViewSpec /*view*/,
                         int /*dimension*/,
                         double oldTime,
                         double newTime)
{
    KnobIPtr knob = getKnob();

    if ( !knob->isAnimationEnabled() || !knob->canAnimate() ) {
        return;
    }
    if ( knob->isDeclaredByPlugin() || knob->isUserKnob() ) {
        AppInstancePtr app = knob->getHolder()->getApp();
        assert(app);
        app->removeKeyFrameIndicator(oldTime);
        app->addKeyframeIndicator(newTime);
    }
}

void
KnobGui::onAnimationLevelChanged(ViewSpec /*idx*/,
                                 int dimension)
{
    if (!_imp->customInteract) {
        KnobIPtr knob = getKnob();
        int dim = knob->getDimension();
        for (int i = 0; i < dim; ++i) {
            if ( (i == dimension) || (dimension == -1) ) {
                reflectAnimationLevel( i, knob->getAnimationLevel(i) );
            }
        }
    }
}

void
KnobGui::onAppendParamEditChanged(int reason,
                                  const Variant & v,
                                  ViewSpec /*view*/,
                                  int dimension,
                                  double time,
                                  bool createNewCommand,
                                  bool setKeyFrame)
{
    pushUndoCommand( new MultipleKnobEditsUndoCommand(shared_from_this(), (ValueChangedReasonEnum)reason, createNewCommand, setKeyFrame, v, dimension, time) );
}

void
KnobGui::onFrozenChanged(bool frozen)
{
    KnobIPtr knob = getKnob();
    KnobButton* isBtn = dynamic_cast<KnobButton*>( knob.get() );

    if ( isBtn && !isBtn->isRenderButton() ) {
        return;
    }
    int dims = knob->getDimension();

    for (int i = 0; i < dims; ++i) {
        ///Do not unset read only if the knob is slaved in this dimension because we are still using it.
        if ( !frozen && knob->isSlave(i) ) {
            continue;
        }
        if ( knob->isEnabled(i) ) {
            setReadOnly_(frozen, i);
        }
    }
}

bool
KnobGui::isGuiFrozenForPlayback() const
{
    return getGui() ? getGui()->isGUIFrozen() : false;
}

boost::shared_ptr<Curve>
KnobGui::getCurve(ViewSpec /*view*/,
                  int dimension) const
{
    return _imp->guiCurves[dimension];
}

void
KnobGui::onRedrawGuiCurve(int reason,
                          ViewSpec /*view*/,
                          int /*dimension*/)
{
    CurveChangeReason curveChangeReason = (CurveChangeReason)reason;

    switch (curveChangeReason) {
    case eCurveChangeReasonCurveEditor:
        Q_EMIT refreshDopeSheet();
        break;
    case eCurveChangeReasonDopeSheet:
        Q_EMIT refreshCurveEditor();
        break;
    case eCurveChangeReasonInternal:
        Q_EMIT refreshDopeSheet();
        Q_EMIT refreshCurveEditor();
        break;
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
KnobGui::onExprChanged(int dimension)
{
    if (_imp->guiRemoved || _imp->customInteract) {
        return;
    }
    KnobIPtr knob = getKnob();
    std::string exp = knob->getExpression(dimension);
    reflectExpressionState( dimension, !exp.empty() );
    if ( exp.empty() ) {
        reflectAnimationLevel( dimension, knob->getAnimationLevel(dimension) );
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
            int dims = knob->getDimension();
            for (int i = 0; i < dims; ++i) {
                std::string err;
                if ( !knob->isExpressionValid(i, &err) ) {
                    invalid = true;
                }
                if ( (dims > 1) && invalid ) {
                    fullErrToolTip += QString::fromUtf8("<p><b>");
                    fullErrToolTip += QString::fromUtf8( knob->getDimensionName(i).c_str() );
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
        Q_EMIT expressionChanged();
    }
    onHelpChanged();
    updateGUI(dimension);
} // KnobGui::onExprChanged

void
KnobGui::onHelpChanged()
{
    if (_imp->descriptionLabel) {
        _imp->descriptionLabel->setToolTip( toolTip() );
    }
    updateToolTip();
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
    reflectModificationsState();
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

        std::string descriptionLabel = getDescriptionLabel();
        if (descriptionLabel.empty()) {
            _imp->descriptionLabel->hide();
        } else {
            _imp->descriptionLabel->setText_overload( QString::fromUtf8( descriptionLabel.c_str() ) );
            _imp->descriptionLabel->show();
        }
        onLabelChangedInternal();
    } else {
        onLabelChangedInternal();
    }
}

void
KnobGui::onDimensionNameChanged(int dimension)
{
    refreshDimensionName(dimension);
}

NATRON_NAMESPACE_EXIT
