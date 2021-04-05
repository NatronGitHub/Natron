/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2021 The Natron developers
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


#include "KnobGui.h"

#include <cassert>
#include <stdexcept>

#include <boost/weak_ptr.hpp>

#include "Engine/KnobTypes.h"
#include "Engine/TrackMarker.h"
#include "Engine/TrackerContext.h"
#include "Engine/EffectInstance.h"
#include "Engine/ViewIdx.h"

#include "Gui/KnobGuiPrivate.h"
#include "Gui/GuiDefines.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/ClickableLabel.h"


NATRON_NAMESPACE_ENTER


/////////////// KnobGui
KnobGui::KnobGui(const KnobIPtr& /*knob*/,
                 KnobGuiContainerI* container)
    : QObject()
    , KnobGuiI()
    , boost::enable_shared_from_this<KnobGui>()
    , _imp( new KnobGuiPrivate(container) )
{
}

KnobGui::~KnobGui()
{
}

void
KnobGui::initialize()
{
    KnobIPtr knob = getKnob();
    KnobGuiPtr thisShared = shared_from_this();

    assert(thisShared);
    if (!_imp->isInViewerUIKnob) {
        knob->setKnobGuiPointer(thisShared);
    }
    KnobHelper* helper = dynamic_cast<KnobHelper*>( knob.get() );
    assert(helper);
    if (helper) {
        KnobSignalSlotHandler* handler = helper->getSignalSlotHandler().get();
        QObject::connect( handler, SIGNAL(redrawGuiCurve(int,ViewSpec,int)), this, SLOT(onRedrawGuiCurve(int,ViewSpec,int)) );
        QObject::connect( handler, SIGNAL(valueChanged(ViewSpec,int,int)), this, SLOT(onInternalValueChanged(ViewSpec,int,int)) );
        QObject::connect( handler, SIGNAL(keyFrameSet(double,ViewSpec,int,int,bool)), this, SLOT(onInternalKeySet(double,ViewSpec,int,int,bool)) );
        QObject::connect( handler, SIGNAL(keyFrameRemoved(double,ViewSpec,int,int)), this, SLOT(onInternalKeyRemoved(double,ViewSpec,int,int)) );
        QObject::connect( handler, SIGNAL(keyFrameMoved(ViewSpec,int,double,double)), this, SLOT(onKeyFrameMoved(ViewSpec,int,double,double)) );
        QObject::connect( handler, SIGNAL(multipleKeyFramesSet(std::list<double>,ViewSpec,int,int)), this,
                          SLOT(onMultipleKeySet(std::list<double>,ViewSpec,int,int)) );
        QObject::connect( handler, SIGNAL(multipleKeyFramesRemoved(std::list<double>,ViewSpec,int,int)), this,
                          SLOT(onMultipleKeyRemoved(std::list<double>,ViewSpec,int,int)) );
        QObject::connect( handler, SIGNAL(secretChanged()), this, SLOT(setSecret()) );
        QObject::connect( handler, SIGNAL(enabledChanged()), this, SLOT(setEnabledSlot()) );
        QObject::connect( handler, SIGNAL(knobSlaved(int,bool)), this, SLOT(onKnobSlavedChanged(int,bool)) );
        QObject::connect( handler, SIGNAL(animationAboutToBeRemoved(ViewSpec,int)), this, SLOT(onInternalAnimationAboutToBeRemoved(ViewSpec,int)) );
        QObject::connect( handler, SIGNAL(animationRemoved(ViewSpec,int)), this, SLOT(onInternalAnimationRemoved()) );
        QObject::connect( handler, SIGNAL(setValueWithUndoStack(Variant,ViewSpec,int)), this, SLOT(onSetValueUsingUndoStack(Variant,ViewSpec,int)) );
        QObject::connect( handler, SIGNAL(dirty(bool)), this, SLOT(onSetDirty(bool)) );
        QObject::connect( handler, SIGNAL(animationLevelChanged(ViewSpec,int)), this, SLOT(onAnimationLevelChanged(ViewSpec,int)) );
        QObject::connect( handler, SIGNAL(appendParamEditChange(int,Variant,ViewSpec,int,double,bool,bool)), this,
                          SLOT(onAppendParamEditChanged(int,Variant,ViewSpec,int,double,bool,bool)) );
        QObject::connect( handler, SIGNAL(frozenChanged(bool)), this, SLOT(onFrozenChanged(bool)) );
        QObject::connect( handler, SIGNAL(helpChanged()), this, SLOT(onHelpChanged()) );
        QObject::connect( handler, SIGNAL(expressionChanged(int)), this, SLOT(onExprChanged(int)) );
        QObject::connect( handler, SIGNAL(hasModificationsChanged()), this, SLOT(onHasModificationsChanged()) );
        QObject::connect( handler, SIGNAL(labelChanged()), this, SLOT(onLabelChanged()) );
        QObject::connect( handler, SIGNAL(dimensionNameChanged(int)), this, SLOT(onDimensionNameChanged(int)) );
        QObject::connect( handler, SIGNAL(viewerContextSecretChanged()), this, SLOT(onViewerContextSecretChanged()) );
    }
    if (!_imp->isInViewerUIKnob) {
        _imp->guiCurves.resize( knob->getDimension() );
        if ( knob->canAnimate() ) {
            for (int i = 0; i < knob->getDimension(); ++i) {
                _imp->guiCurves[i].reset( new Curve( *( knob->getCurve(ViewIdx(0), i) ) ) );
            }
        }
    }
}

KnobGuiContainerI*
KnobGui::getContainer()
{
    return _imp->container;
}

void
KnobGui::removeGui()
{
    assert(!_imp->guiRemoved);
    for (std::vector<KnobIWPtr>::iterator it = _imp->knobsOnSameLine.begin(); it != _imp->knobsOnSameLine.end(); ++it) {
        KnobGuiPtr kg = _imp->container->getKnobGui( it->lock() );
        if (kg) {
            kg->_imp->removeFromKnobsOnSameLineVector( getKnob() );
        }
    }

    if ( _imp->knobsOnSameLine.empty() ) {
        if (_imp->isOnNewLine) {
            if (_imp->labelContainer) {
                _imp->labelContainer->deleteLater();
                _imp->labelContainer = 0;
            }
        }
        if (_imp->field) {
            _imp->field->deleteLater();
            _imp->field = 0;
        }
    } else {
        if (_imp->descriptionLabel) {
            _imp->descriptionLabel->deleteLater();
            _imp->descriptionLabel = 0;
        }
    }
    removeSpecificGui();
    _imp->guiRemoved = true;
}

void
KnobGui::setGuiRemoved()
{
    _imp->guiRemoved = true;
}

Gui*
KnobGui::getGui() const
{
    return _imp->container->getGui();
}

const QUndoCommand*
KnobGui::getLastUndoCommand() const
{
    return _imp->container->getLastUndoCommand();
}

void
KnobGui::pushUndoCommand(QUndoCommand* cmd)
{
    if ( getKnob()->getCanUndo() && getKnob()->getEvaluateOnChange() ) {
        _imp->container->pushUndoCommand(cmd);
    } else {
        cmd->redo();
        delete cmd;
    }
}

void
KnobGui::createGUI(QWidget* fieldContainer,
                   QWidget* labelContainer,
                   KnobClickableLabel* label,
                   Label* warningIndicator,
                   QHBoxLayout* layout,
                   bool isOnNewLine,
                   int lastKnobSpacing,
                   const std::vector<KnobIPtr> & knobsOnSameLine)
{
    _imp->guiRemoved = false;
    KnobIPtr knob = getKnob();

    _imp->fieldLayout = layout;
    for (std::vector<KnobIPtr>::const_iterator it = knobsOnSameLine.begin(); it != knobsOnSameLine.end(); ++it) {
        _imp->knobsOnSameLine.push_back(*it);
    }
    _imp->field = fieldContainer;
    _imp->labelContainer = labelContainer;
    _imp->descriptionLabel = label;
    _imp->warningIndicator = warningIndicator;
    _imp->isOnNewLine = isOnNewLine;
    if (!isOnNewLine) {
        //layout->addStretch();
        int spacing;
        bool isViewerParam = _imp->container->isInViewerUIKnob();
        if (isViewerParam) {
            spacing = _imp->container->getItemsSpacingOnSameLine();
        } else {
            spacing = lastKnobSpacing;//knob->getSpacingBetweenitems();
            // Default sapcing is 0 on knobs, but use the default for the widget container so the UI doesn't appear cluttered
            // The minimum allowed spacing should be 1px
            if (spacing == 0) {
                spacing = _imp->container->getItemsSpacingOnSameLine();;
            }
        }
        if (spacing > 0) {
            layout->addSpacing( TO_DPIX(spacing) );
        }
        if (label) {
            if (_imp->warningIndicator) {
                layout->addWidget(_imp->warningIndicator);
            }
            layout->addWidget(label);
        }
    }


    if (label) {
        label->setToolTip( toolTip() );
    }

    // Parmetric knobs use the customInteract to actually draw something on top of the background
    KnobParametric* isParametric = dynamic_cast<KnobParametric*>( knob.get() );
    OfxParamOverlayInteractPtr customInteract = knob->getCustomInteract();
    if (customInteract && !isParametric) {
        _imp->customInteract = new CustomParamInteract(shared_from_this(), knob->getOfxParamHandle(), customInteract);
        layout->addWidget(_imp->customInteract);
    } else {
        createWidget(layout);
        onHasModificationsChanged();
        updateToolTip();
    }


    _imp->widgetCreated = true;

    for (int i = 0; i < knob->getDimension(); ++i) {
        onExprChanged(i);

        /*updateGuiInternal(i);
           std::string exp = knob->getExpression(i);
           reflectExpressionState(i,!exp.empty());
           if (exp.empty()) {
            onAnimationLevelChanged(ViewSpec::all(), i);
           }*/
    }
} // KnobGui::createGUI

void
KnobGui::updateGuiInternal(int dimension)
{
    if (!_imp->customInteract) {
        updateGUI(dimension);
    } else {
        _imp->customInteract->update();
    }
}

void
KnobGui::onRightClickClicked(const QPoint & pos)
{
    QWidget *widget = qobject_cast<QWidget *>( sender() );

    if (widget) {
        QString objName = widget->objectName();
        objName = objName.remove( QString::fromUtf8("dim-") );
        showRightClickMenuForDimension( pos, objName.toInt() );
    }
}

void
KnobGui::enableRightClickMenu(QWidget* widget,
                              int dimension)
{
    QString name = QString::fromUtf8("dim-");

    name.append( QString::number(dimension) );
    widget->setContextMenuPolicy(Qt::CustomContextMenu);
    widget->setObjectName(name);
    QObject::connect( widget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(onRightClickClicked(QPoint)) );
}

bool
KnobGui::shouldCreateLabel() const
{
    return true;
}

bool
KnobGui::isLabelOnSameColumn() const
{
    return false;
}

bool
KnobGui::isLabelBold() const
{
    return false;
}

std::string
KnobGui::getDescriptionLabel() const
{
    return getKnob()->getLabel();
}

void
KnobGui::showRightClickMenuForDimension(const QPoint &,
                                        int dimension)
{
    KnobIPtr knob = getKnob();

    if ( knob->getIsSecret() ) {
        return;
    }

    createAnimationMenu(_imp->copyRightClickMenu, dimension);
    addRightClickMenuEntries(_imp->copyRightClickMenu);
    _imp->copyRightClickMenu->exec( QCursor::pos() );
} // showRightClickMenuForDimension

Menu*
KnobGui::createInterpolationMenu(QMenu* menu,
                                 int dimension,
                                 bool isEnabled)
{
    Menu* interpolationMenu = new Menu(menu);
    QString title;

    if (dimension == -1) {
        title = tr("Interpolation (all dimensions)");
    } else {
        title = tr("Interpolation");
    }
    interpolationMenu->setTitle(title);
    if (!isEnabled) {
        interpolationMenu->menuAction()->setEnabled(false);
    }

    QAction* constantInterpAction = new QAction(tr("Constant"), interpolationMenu);
    constantInterpAction->setData( QVariant(dimension) );
    QObject::connect( constantInterpAction, SIGNAL(triggered()), this, SLOT(onConstantInterpActionTriggered()) );
    interpolationMenu->addAction(constantInterpAction);

    QAction* linearInterpAction = new QAction(tr("Linear"), interpolationMenu);
    linearInterpAction->setData( QVariant(dimension) );
    QObject::connect( linearInterpAction, SIGNAL(triggered()), this, SLOT(onLinearInterpActionTriggered()) );
    interpolationMenu->addAction(linearInterpAction);

    QAction* smoothInterpAction = new QAction(tr("Smooth"), interpolationMenu);
    smoothInterpAction->setData( QVariant(dimension) );
    QObject::connect( smoothInterpAction, SIGNAL(triggered()), this, SLOT(onSmoothInterpActionTriggered()) );
    interpolationMenu->addAction(smoothInterpAction);

    QAction* catmullRomInterpAction = new QAction(tr("Catmull-Rom"), interpolationMenu);
    catmullRomInterpAction->setData( QVariant(dimension) );
    QObject::connect( catmullRomInterpAction, SIGNAL(triggered()), this, SLOT(onCatmullromInterpActionTriggered()) );
    interpolationMenu->addAction(catmullRomInterpAction);

    QAction* cubicInterpAction = new QAction(tr("Cubic"), interpolationMenu);
    cubicInterpAction->setData( QVariant(dimension) );
    QObject::connect( cubicInterpAction, SIGNAL(triggered()), this, SLOT(onCubicInterpActionTriggered()) );
    interpolationMenu->addAction(cubicInterpAction);

    QAction* horizInterpAction = new QAction(tr("Horizontal"), interpolationMenu);
    horizInterpAction->setData( QVariant(dimension) );
    QObject::connect( horizInterpAction, SIGNAL(triggered()), this, SLOT(onHorizontalInterpActionTriggered()) );
    interpolationMenu->addAction(horizInterpAction);

    menu->addAction( interpolationMenu->menuAction() );

    return interpolationMenu;
}

void
KnobGui::createAnimationMenu(QMenu* menu,
                             int dimension)
{
    if ( (dimension == 0) && !getAllDimensionsVisible() ) {
        dimension = -1;
    }

    KnobIPtr knob = getKnob();
    assert( dimension >= -1 && dimension < knob->getDimension() );
    menu->clear();
    bool dimensionHasKeyframeAtTime = false;
    bool hasAllKeyframesAtTime = true;
    int nDims = knob->getDimension();


    if (dimension == -1 && nDims == 1) {
        dimension = 0;
    }


    for (int i = 0; i < nDims; ++i) {
        AnimationLevelEnum lvl = knob->getAnimationLevel(i);
        if (lvl != eAnimationLevelOnKeyframe) {
            hasAllKeyframesAtTime = false;
        } else if ( dimension == i && (lvl == eAnimationLevelOnKeyframe) ) {
            dimensionHasKeyframeAtTime = true;
        }
    }

    bool hasDimensionSlaved = false;
    bool hasAnimation = false;
    bool dimensionHasAnimation = false;
    bool isEnabled = true;
    bool dimensionIsSlaved = false;

    for (int i = 0; i < nDims; ++i) {
        if ( knob->isSlave(i) ) {
            hasDimensionSlaved = true;

            if (i == dimension) {
                dimensionIsSlaved = true;
            }
        }
        if (knob->getKeyFramesCount(ViewIdx(0), i) > 0) {
            hasAnimation = true;
            if (dimension == i) {
                dimensionHasAnimation = true;
            }
        }

        if (hasDimensionSlaved && hasAnimation) {
            break;
        }
        if ( !knob->isEnabled(i) ) {
            isEnabled = false;
        }
    }

    bool isAppKnob = knob->getHolder() && knob->getHolder()->getApp();

    if ( (knob->getDimension() > 1) && knob->isAnimationEnabled() && !hasDimensionSlaved && isAppKnob ) {
        ///Multi-dim actions
        if (!hasAllKeyframesAtTime) {
            QAction* setKeyAction = new QAction(tr("Set Key") + QLatin1Char(' ') + tr("(all dimensions)"), menu);
            setKeyAction->setData(-1);
            QObject::connect( setKeyAction, SIGNAL(triggered()), this, SLOT(onSetKeyActionTriggered()) );
            menu->addAction(setKeyAction);
            if (!isEnabled) {
                setKeyAction->setEnabled(false);
            }
        } else {
            QAction* removeKeyAction = new QAction(tr("Remove Key") + QLatin1Char(' ') + tr("(all dimensions)"), menu);
            removeKeyAction->setData(-1);
            QObject::connect( removeKeyAction, SIGNAL(triggered()), this, SLOT(onRemoveKeyActionTriggered()) );
            menu->addAction(removeKeyAction);
            if (!isEnabled) {
                removeKeyAction->setEnabled(false);
            }
        }

        if (hasAnimation) {
            QAction* removeAnyAnimationAction = new QAction(tr("Remove animation") + QLatin1Char(' ') + tr("(all dimensions)"), menu);
            removeAnyAnimationAction->setData(-1);
            QObject::connect( removeAnyAnimationAction, SIGNAL(triggered()), this, SLOT(onRemoveAnimationActionTriggered()) );
            if (!isEnabled) {
                removeAnyAnimationAction->setEnabled(false);
            }
            menu->addAction(removeAnyAnimationAction);
        }
    }
    if ( ( (dimension != -1) || (knob->getDimension() == 1) ) && knob->isAnimationEnabled() && !dimensionIsSlaved && isAppKnob ) {
        if ( !menu->isEmpty() ) {
            menu->addSeparator();
        }
        {
            ///Single dim action
            if (!dimensionHasKeyframeAtTime) {
                QAction* setKeyAction = new QAction(tr("Set Key"), menu);
                setKeyAction->setData(dimension);
                QObject::connect( setKeyAction, SIGNAL(triggered()), this, SLOT(onSetKeyActionTriggered()) );
                menu->addAction(setKeyAction);
                if (!isEnabled) {
                    setKeyAction->setEnabled(false);
                }
            } else {
                QAction* removeKeyAction = new QAction(tr("Remove Key"), menu);
                removeKeyAction->setData(dimension);
                QObject::connect( removeKeyAction, SIGNAL(triggered()), this, SLOT(onRemoveKeyActionTriggered()) );
                menu->addAction(removeKeyAction);
                if (!isEnabled) {
                    removeKeyAction->setEnabled(false);
                }
            }

            if (dimensionHasAnimation) {
                QAction* removeAnyAnimationAction = new QAction(tr("Remove animation"), menu);
                removeAnyAnimationAction->setData(dimension);
                QObject::connect( removeAnyAnimationAction, SIGNAL(triggered()), this, SLOT(onRemoveAnimationActionTriggered()) );
                menu->addAction(removeAnyAnimationAction);
                if (!isEnabled) {
                    removeAnyAnimationAction->setEnabled(false);
                }
            }
        }
    }
    if ( !menu->isEmpty() ) {
        menu->addSeparator();
    }

    if (hasAnimation && isAppKnob) {
        QAction* showInCurveEditorAction = new QAction(tr("Show in curve editor"), menu);
        QObject::connect( showInCurveEditorAction, SIGNAL(triggered()), this, SLOT(onShowInCurveEditorActionTriggered()) );
        menu->addAction(showInCurveEditorAction);
        if (!isEnabled) {
            showInCurveEditorAction->setEnabled(false);
        }

        if ( (knob->getDimension() > 1) && !hasDimensionSlaved ) {
            Menu* interpMenu = createInterpolationMenu(menu, -1, isEnabled);
            Q_UNUSED(interpMenu);
        }
        if (dimensionHasAnimation && !dimensionIsSlaved) {
            if ( (dimension != -1) || (knob->getDimension() == 1) ) {
                Menu* interpMenu = createInterpolationMenu(menu, dimension != -1 ? dimension : 0, isEnabled);
                Q_UNUSED(interpMenu);
            }
        }
    }


    {
        Menu* copyMenu = new Menu(menu);
        copyMenu->setTitle( tr("Copy") );

        if (dimension != -1 || nDims == 1) {
            if (hasAnimation && isAppKnob) {
                QAction* copyAnimationAction = new QAction(tr("Copy Animation"), copyMenu);
                copyAnimationAction->setData(dimension);
                QObject::connect( copyAnimationAction, SIGNAL(triggered()), this, SLOT(onCopyAnimationActionTriggered()) );
                copyMenu->addAction(copyAnimationAction);
            }


            QAction* copyValuesAction = new QAction(tr("Copy Value"), copyMenu);
            copyValuesAction->setData( QVariant(dimension) );
            copyMenu->addAction(copyValuesAction);
            QObject::connect( copyValuesAction, SIGNAL(triggered()), this, SLOT(onCopyValuesActionTriggered()) );


            if (isAppKnob) {
                QAction* copyLinkAction = new QAction(tr("Copy Link"), copyMenu);
                copyLinkAction->setData( QVariant(dimension) );
                copyMenu->addAction(copyLinkAction);
                QObject::connect( copyLinkAction, SIGNAL(triggered()), this, SLOT(onCopyLinksActionTriggered()) );
            }
        }

        if (knob->getDimension() > 1) {
            if (hasAnimation && isAppKnob) {
                QString title = tr("Copy Animation");
                title += QLatin1Char(' ');
                title += tr("(all dimensions)");
                QAction* copyAnimationAction = new QAction(title, copyMenu);
                copyAnimationAction->setData(-1);
                QObject::connect( copyAnimationAction, SIGNAL(triggered()), this, SLOT(onCopyAnimationActionTriggered()) );
                copyMenu->addAction(copyAnimationAction);
            }
            {
                QString title = tr("Copy Values");
                title += QLatin1Char(' ');
                title += tr("(all dimensions)");
                QAction* copyValuesAction = new QAction(title, copyMenu);
                copyValuesAction->setData(-1);
                copyMenu->addAction(copyValuesAction);
                QObject::connect( copyValuesAction, SIGNAL(triggered()), this, SLOT(onCopyValuesActionTriggered()) );
            }

            if (isAppKnob) {
                QString title = tr("Copy Link");
                title += QLatin1Char(' ');
                title += tr("(all dimensions)");
                QAction* copyLinkAction = new QAction(title, copyMenu);
                copyLinkAction->setData(-1);
                copyMenu->addAction(copyLinkAction);
                QObject::connect( copyLinkAction, SIGNAL(triggered()), this, SLOT(onCopyLinksActionTriggered()) );
            }

        }

        menu->addAction( copyMenu->menuAction() );
    }

    ///If the clipboard is either empty or has no animation, disable the Paste animation action.
    KnobIPtr fromKnob;
    KnobClipBoardType type;

    //cbDim is ignored for now
    int cbDim;
    appPTR->getKnobClipBoard(&type, &fromKnob, &cbDim);


    if (fromKnob && (fromKnob != knob) && isAppKnob) {
        if ( fromKnob->typeName() == knob->typeName() ) {
            QString titlebase;
            if (type == eKnobClipBoardTypeCopyValue) {
                titlebase = tr("Paste Value");
            } else if (type == eKnobClipBoardTypeCopyAnim) {
                titlebase = tr("Paste Animation");
            } else if (type == eKnobClipBoardTypeCopyLink) {
                titlebase = tr("Paste Link");
            }

            bool ignorePaste = (!knob->isAnimationEnabled() && type == eKnobClipBoardTypeCopyAnim) ||
                               ( (dimension == -1 || cbDim == -1) && knob->getDimension() != fromKnob->getDimension() );
            if (!ignorePaste) {
                if ( (cbDim == -1) && ( fromKnob->getDimension() == knob->getDimension() ) && !hasDimensionSlaved ) {
                    QString title = titlebase;
                    if (knob->getDimension() > 1) {
                        title += QLatin1Char(' ');
                        title += tr("(all dimensions)");
                    }
                    QAction* pasteAction = new QAction(title, menu);
                    pasteAction->setData(-1);
                    QObject::connect( pasteAction, SIGNAL(triggered()), this, SLOT(onPasteActionTriggered()) );
                    menu->addAction(pasteAction);
                    if (!isEnabled) {
                        pasteAction->setEnabled(false);
                    }
                }

                if ( ( (dimension != -1) || (knob->getDimension() == 1) ) && !dimensionIsSlaved ) {
                    QAction* pasteAction = new QAction(titlebase, menu);
                    pasteAction->setData(dimension != -1 ? dimension : 0);
                    QObject::connect( pasteAction, SIGNAL(triggered()), this, SLOT(onPasteActionTriggered()) );
                    menu->addAction(pasteAction);
                    if (!isEnabled) {
                        pasteAction->setEnabled(false);
                    }
                }
            }
        }
    }

    if ( (knob->getDimension() > 1) && !hasDimensionSlaved ) {
        QAction* resetDefaultAction = new QAction(tr("Reset to default") + QLatin1Char(' ') + tr("(all dimensions)"), _imp->copyRightClickMenu);
        resetDefaultAction->setData( QVariant(-1) );
        QObject::connect( resetDefaultAction, SIGNAL(triggered()), this, SLOT(onResetDefaultValuesActionTriggered()) );
        menu->addAction(resetDefaultAction);
        if (!isEnabled) {
            resetDefaultAction->setEnabled(false);
        }
    }
    if ( ( (dimension != -1) || (knob->getDimension() == 1) ) && !dimensionIsSlaved ) {
        QAction* resetDefaultAction = new QAction(tr("Reset to default"), _imp->copyRightClickMenu);
        resetDefaultAction->setData( QVariant(dimension) );
        QObject::connect( resetDefaultAction, SIGNAL(triggered()), this, SLOT(onResetDefaultValuesActionTriggered()) );
        menu->addAction(resetDefaultAction);
        if (!isEnabled) {
            resetDefaultAction->setEnabled(false);
        }
    }

    if ( !menu->isEmpty() ) {
        menu->addSeparator();
    }


    bool dimensionHasExpression = false;
    bool hasExpression = false;
    for (int i = 0; i < knob->getDimension(); ++i) {
        std::string dimExpr = knob->getExpression(i);
        if (i == dimension) {
            dimensionHasExpression = !dimExpr.empty();
        }
        hasExpression |= !dimExpr.empty();
    }
    if ( (knob->getDimension() > 1) && !hasDimensionSlaved && isAppKnob ) {
        QAction* setExprsAction = new QAction( ( hasExpression ? tr("Edit expression") :
                                                 tr("Set expression") ) + QLatin1Char(' ') + tr("(all dimensions)"), menu );
        setExprsAction->setData(-1);
        QObject::connect( setExprsAction, SIGNAL(triggered()), this, SLOT(onSetExprActionTriggered()) );
        if (!isEnabled) {
            setExprsAction->setEnabled(false);
        }
        menu->addAction(setExprsAction);

        if (hasExpression) {
            QAction* clearExprAction = new QAction(tr("Clear expression") + QLatin1Char(' ') + tr("(all dimensions)"), menu);
            QObject::connect( clearExprAction, SIGNAL(triggered()), this, SLOT(onClearExprActionTriggered()) );
            clearExprAction->setData(-1);
            if (!isEnabled) {
                clearExprAction->setEnabled(false);
            }
            menu->addAction(clearExprAction);
        }
    }
    if ( ( (dimension != -1) || (knob->getDimension() == 1) ) && !dimensionIsSlaved && isAppKnob ) {
        QAction* setExprAction = new QAction(dimensionHasExpression ? tr("Edit expression...") : tr("Set expression..."), menu);
        QObject::connect( setExprAction, SIGNAL(triggered()), this, SLOT(onSetExprActionTriggered()) );
        setExprAction->setData(dimension);
        if (!isEnabled) {
            setExprAction->setEnabled(false);
        }
        menu->addAction(setExprAction);

        if (dimensionHasExpression) {
            QAction* clearExprAction = new QAction(tr("Clear expression"), menu);
            QObject::connect( clearExprAction, SIGNAL(triggered()), this, SLOT(onClearExprActionTriggered()) );
            clearExprAction->setData(dimension);
            if (!isEnabled) {
                clearExprAction->setEnabled(false);
            }
            menu->addAction(clearExprAction);
        }
    }


    ///find-out to which node that master knob belongs to
    KnobHolder* holder = knob->getHolder();
    EffectInstance* isEffect = dynamic_cast<EffectInstance*>(holder);
    NodeCollectionPtr collec;
    NodeGroup* isCollecGroup = 0;
    if (isEffect) {
        collec = isEffect->getNode()->getGroup();
        isCollecGroup = dynamic_cast<NodeGroup*>( collec.get() );
    }


    if ( isAppKnob && ( ( hasDimensionSlaved && (dimension == -1) ) || dimensionIsSlaved ) ) {
        menu->addSeparator();

        KnobIPtr aliasMaster = knob->getAliasMaster();
        std::string knobName;
        if ( aliasMaster || ((dimension != -1 || knob->getDimension() == 1) && dimensionIsSlaved) ) {

            KnobIPtr masterKnob;
            std::pair<int, KnobIPtr> master;
            if (aliasMaster) {
                masterKnob = aliasMaster;
            } else {
                master = knob->getMaster(dimension);
                masterKnob = master.second;
            }

            KnobHolder* masterHolder = masterKnob->getHolder();
            if (masterHolder) {
                TrackMarker* isTrackMarker = dynamic_cast<TrackMarker*>(masterHolder);
                EffectInstance* isEffect = dynamic_cast<EffectInstance*>(masterHolder);
                if (isTrackMarker) {
                    knobName.append( isTrackMarker->getContext()->getNode()->getScriptName() );
                    knobName += '.';
                    knobName += isTrackMarker->getScriptName_mt_safe();
                } else if (isEffect) {
                    knobName += isEffect->getScriptName_mt_safe();
                }

            }


            knobName.append(".");
            knobName.append( masterKnob->getName() );
            if ( !aliasMaster && (masterKnob->getDimension() > 1) ) {
                knobName.append(".");
                knobName.append( masterKnob->getDimensionName(master.first) );
            }
        }
        QString actionText;
        if (aliasMaster) {
            actionText.append( tr("Remove Alias link") );
        } else {
            actionText.append( tr("Unlink") );
        }
        if ( !knobName.empty() ) {
            actionText.append( QString::fromUtf8(" from ") );
            actionText.append( QString::fromUtf8( knobName.c_str() ) );
        }
        QAction* unlinkAction = new QAction(actionText, menu);
        unlinkAction->setData( QVariant(dimension) );
        QObject::connect( unlinkAction, SIGNAL(triggered()), this, SLOT(onUnlinkActionTriggered()) );
        menu->addAction(unlinkAction);
    }
    KnobI::ListenerDimsMap listeners;
    knob->getListeners(listeners);
    if ( !listeners.empty() ) {
        KnobIPtr listener = listeners.begin()->first.lock();
        if ( listener && (listener->getAliasMaster() == knob) ) {
            QAction* removeAliasLink = new QAction(tr("Remove alias link"), menu);
            QObject::connect( removeAliasLink, SIGNAL(triggered()), this, SLOT(onRemoveAliasLinkActionTriggered()) );
            menu->addAction(removeAliasLink);
        }
    }
    if ( isCollecGroup && !knob->getAliasMaster() ) {
        QAction* createMasterOnGroup = new QAction(tr("Create alias on group"), menu);
        QObject::connect( createMasterOnGroup, SIGNAL(triggered()), this, SLOT(onCreateAliasOnGroupActionTriggered()) );
        menu->addAction(createMasterOnGroup);
    }
} // createAnimationMenu

KnobIPtr
KnobGui::createDuplicateOnNode(EffectInstance* effect,
                               bool makeAlias,
                               const KnobPagePtr& page,
                               const KnobGroupPtr& group,
                               int indexInParent)
{
    ///find-out to which node that master knob belongs to
    assert( getKnob()->getHolder()->getApp() );
    KnobIPtr knob = getKnob();

    if (!makeAlias) {
        for (int i = 0; i < knob->getDimension(); ++i) {
            std::string expr = knob->getExpression(i);
            if ( !expr.empty() ) {
                StandardButtonEnum rep = Dialogs::questionDialog( tr("Expression").toStdString(), tr("This operation will create "
                                                                                                     "an expression link between this parameter and the new parameter on the group"
                                                                                                     " which will wipe the current expression(s).\n"
                                                                                                     "Continue anyway?").toStdString(), false,
                                                                  StandardButtons(eStandardButtonOk | eStandardButtonCancel) );
                if (rep != eStandardButtonYes) {
                    return KnobIPtr();
                }
            }
        }
    }

    EffectInstance* isEffect = dynamic_cast<EffectInstance*>( knob->getHolder() );
    if (!isEffect) {
        return KnobIPtr();
    }
    const std::string& nodeScriptName = isEffect->getNode()->getScriptName();
    std::string newKnobName = nodeScriptName +  knob->getName();
    KnobIPtr ret;
    try {
        ret = knob->createDuplicateOnHolder(effect,
                                            page,
                                            group,
                                            indexInParent,
                                            makeAlias,
                                            newKnobName,
                                            knob->getLabel(),
                                            knob->getHintToolTip(),
                                            true,
                                            true);
    } catch (const std::exception& e) {
        Dialogs::errorDialog( tr("Error while creating parameter").toStdString(), e.what() );

        return KnobIPtr();
    }

    if (ret) {
        NodeGuiIPtr groupNodeGuiI = effect->getNode()->getNodeGui();
        NodeGui* groupNodeGui = dynamic_cast<NodeGui*>( groupNodeGuiI.get() );
        assert(groupNodeGui);
        if (groupNodeGui) {
            groupNodeGui->ensurePanelCreated();
        }
    }
    effect->getApp()->triggerAutoSave();

    return ret;
} // KnobGui::createDuplicateOnNode

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_KnobGui.cpp"
