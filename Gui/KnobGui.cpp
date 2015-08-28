/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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

#include "Gui/KnobGui.h"

#include "Gui/KnobUndoCommand.h"

#include <cassert>
#include <climits>
#include <cfloat>
#include <stdexcept>

#include <boost/weak_ptr.hpp>


#include <QtCore/QString>
#include <QHBoxLayout>
#include <QPushButton>
#include <QFormLayout>
#include <QFileDialog>
#include <QTextEdit>
#include <QStyle> // in QtGui on Qt4, in QtWidgets on Qt5
#include <QTimer>

GCC_DIAG_UNUSED_PRIVATE_FIELD_OFF
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QKeyEvent>
GCC_DIAG_UNUSED_PRIVATE_FIELD_ON
#include <QColorDialog>
#include <QGroupBox>
#include <QtGui/QVector4D>
#include <QStyleFactory>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QCompleter>

#include "Global/GlobalDefines.h"

#include "Engine/Curve.h"
#include "Engine/KnobFile.h"
#include "Engine/KnobSerialization.h"
#include "Engine/KnobTypes.h"
#include "Engine/LibraryBinary.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/Project.h"
#include "Engine/Settings.h"
#include "Engine/TimeLine.h"
#include "Engine/Variant.h"
#include "Engine/ViewerInstance.h"

#include "Gui/AnimationButton.h"
#include "Gui/ComboBox.h"
#include "Gui/CurveEditor.h"
#include "Gui/CurveGui.h"
#include "Gui/CustomParamInteract.h"
#include "Gui/DockablePanel.h"
#include "Gui/EditExpressionDialog.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/Group_KnobGui.h"
#include "Gui/Label.h"
#include "Gui/LineEdit.h"
#include "Gui/LinkToKnobDialog.h"
#include "Gui/Menu.h"
#include "Gui/Menu.h"
#include "Gui/NodeCreationDialog.h"
#include "Gui/NodeGui.h"
#include "Gui/NodeSettingsPanel.h"
#include "Gui/ScriptTextEdit.h"
#include "Gui/SequenceFileDialog.h"
#include "Gui/SpinBox.h"
#include "Gui/TabWidget.h"
#include "Gui/TimeLineGui.h"
#include "Gui/Utils.h"
#include "Gui/ViewerTab.h"

using namespace Natron;


struct KnobGui::KnobGuiPrivate
{
    bool triggerNewLine;
    int spacingBetweenItems;
    bool widgetCreated;
    DockablePanel*  container;
    Natron::Menu* animationMenu;
    AnimationButton* animationButton;
    QMenu* copyRightClickMenu;
    QHBoxLayout* fieldLayout; //< the layout containing the widgets of the knob

    ////A vector of all other knobs on the same line.
    std::vector< boost::weak_ptr< KnobI > > knobsOnSameLine;
    QGridLayout* containerLayout;
    QWidget* field;
    Natron::Label* descriptionLabel;
    bool isOnNewLine;
    CustomParamInteract* customInteract;

    std::vector< boost::shared_ptr<Curve> > guiCurves;
    
    bool guiRemoved;
    
    KnobGuiPrivate(DockablePanel* container)
    : triggerNewLine(true)
    , spacingBetweenItems(0)
    , widgetCreated(false)
    , container(container)
    , animationMenu(NULL)
    , animationButton(NULL)
    , copyRightClickMenu( new MenuWithToolTips(container) )
    , fieldLayout(NULL)
    , knobsOnSameLine()
    , containerLayout(NULL)
    , field(NULL)
    , descriptionLabel(NULL)
    , isOnNewLine(false)
    , customInteract(NULL)
    , guiCurves()
    , guiRemoved(false)
    {
        //copyRightClickMenu->setFont( QFont(appFont,appFontSize) );
    }
    
    void removeFromKnobsOnSameLineVector(const boost::shared_ptr<KnobI>& knob)
    {
        for (std::vector< boost::weak_ptr< KnobI > >::iterator it = knobsOnSameLine.begin(); it != knobsOnSameLine.end(); ++it) {
            if (it->lock() == knob) {
                knobsOnSameLine.erase(it);
                break;
            }
        }
    }
};

/////////////// KnobGui
KnobGui::KnobGui(const boost::shared_ptr<KnobI>& knob,
                 DockablePanel* container)
: _imp( new KnobGuiPrivate(container) )
{
    knob->setKnobGuiPointer(this);
    KnobHelper* helper = dynamic_cast<KnobHelper*>( knob.get() );
    assert(helper);
    if (helper) {
        KnobSignalSlotHandler* handler = helper->getSignalSlotHandler().get();
        QObject::connect( handler,SIGNAL( refreshGuiCurve(int)),this,SLOT( onRefreshGuiCurve(int) ) );
        QObject::connect( handler,SIGNAL( valueChanged(int,int) ),this,SLOT( onInternalValueChanged(int,int) ) );
        QObject::connect( handler,SIGNAL( keyFrameSet(SequenceTime,int,int,bool) ),this,SLOT( onInternalKeySet(SequenceTime,int,int,bool) ) );
        QObject::connect( handler,SIGNAL( keyFrameRemoved(SequenceTime,int,int) ),this,SLOT( onInternalKeyRemoved(SequenceTime,int,int) ) );
        QObject::connect( handler,SIGNAL( keyFrameMoved(int,int,int)), this, SLOT( onKeyFrameMoved(int,int,int)));
        QObject::connect( handler,SIGNAL( multipleKeyFramesSet(std::list<SequenceTime>,int,int)), this,
                         SLOT(onMultipleKeySet(std::list<SequenceTime> , int, int)));
        QObject::connect( handler,SIGNAL( secretChanged() ),this,SLOT( setSecret() ) );
        QObject::connect( handler,SIGNAL( enabledChanged() ),this,SLOT( setEnabledSlot() ) );
        QObject::connect( handler,SIGNAL( knobSlaved(int,bool) ),this,SLOT( onKnobSlavedChanged(int,bool) ) );
        QObject::connect( handler,SIGNAL( animationAboutToBeRemoved(int) ),this,SLOT( onInternalAnimationAboutToBeRemoved(int) ) );
        QObject::connect( handler,SIGNAL( animationRemoved(int) ),this,SLOT( onInternalAnimationRemoved() ) );
        QObject::connect( handler,SIGNAL( setValueWithUndoStack(Variant,int) ),this,SLOT( onSetValueUsingUndoStack(Variant,int) ) );
        QObject::connect( handler,SIGNAL( dirty(bool) ),this,SLOT( onSetDirty(bool) ) );
        QObject::connect( handler,SIGNAL( animationLevelChanged(int,int) ),this,SLOT( onAnimationLevelChanged(int,int) ) );
        QObject::connect( handler,SIGNAL( appendParamEditChange(int,Variant,int,int,bool,bool) ),this,
                         SLOT( onAppendParamEditChanged(int,Variant,int,int,bool,bool) ) );
        QObject::connect( handler,SIGNAL( frozenChanged(bool) ),this,SLOT( onFrozenChanged(bool) ) );
        QObject::connect( handler,SIGNAL( helpChanged() ),this,SLOT( onHelpChanged() ) );
        QObject::connect( handler,SIGNAL( expressionChanged(int) ),this,SLOT( onExprChanged(int) ) );
        QObject::connect( handler,SIGNAL( hasModificationsChanged() ),this,SLOT( onHasModificationsChanged() ) );
        QObject::connect(handler,SIGNAL(descriptionChanged()), this, SLOT(onDescriptionChanged()));
    }
    _imp->guiCurves.resize(knob->getDimension());
    if (knob->canAnimate()) {
        for (int i = 0; i < knob->getDimension(); ++i) {
            _imp->guiCurves[i].reset(new Curve(*(knob->getCurve(i))));
        }
    }
}

KnobGui::~KnobGui()
{

}

DockablePanel*
KnobGui::getContainer()
{
    return _imp->container;
}

void
KnobGui::removeGui()
{
    for (std::vector< boost::weak_ptr< KnobI > >::iterator it = _imp->knobsOnSameLine.begin(); it != _imp->knobsOnSameLine.end(); ++it) {
        KnobGui* kg = _imp->container->getKnobGui(it->lock());
        assert(kg);
        kg->_imp->removeFromKnobsOnSameLineVector(getKnob());
    }
    
    if (_imp->knobsOnSameLine.empty()) {
        if (_imp->isOnNewLine) {
            delete _imp->descriptionLabel;
        }
        delete _imp->field;
    } else {
        delete _imp->descriptionLabel;
        delete _imp->animationMenu;
        delete _imp->animationButton;
        removeSpecificGui();
    }
    _imp->guiRemoved = true;
}

void
KnobGui::callDeleteLater()
{
    _imp->guiRemoved = true;
    deleteLater();
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
    }
}

void
KnobGui::createGUI(QGridLayout* containerLayout,
                   QWidget* fieldContainer,
                   Natron::Label* label,
                   QHBoxLayout* layout,
                   bool isOnNewLine,
                   const std::vector< boost::shared_ptr< KnobI > > & knobsOnSameLine)
{
    boost::shared_ptr<KnobI> knob = getKnob();

    _imp->containerLayout = containerLayout;
    _imp->fieldLayout = layout;
    for (std::vector< boost::shared_ptr< KnobI > >::const_iterator it = knobsOnSameLine.begin(); it != knobsOnSameLine.end(); ++it) {
        _imp->knobsOnSameLine.push_back(*it);
    }
    _imp->field = fieldContainer;
    _imp->descriptionLabel = label;
    _imp->isOnNewLine = isOnNewLine;
    if (!isOnNewLine) {
        //layout->addStretch();
        layout->addSpacing(15);
        if (label) {
            layout->addWidget(label);
        }
    }


    if (label) {
        label->setToolTip( toolTip() );
    }

    boost::shared_ptr<OfxParamOverlayInteract> customInteract = knob->getCustomInteract();
    if (customInteract != 0) {
        _imp->customInteract = new CustomParamInteract(this,knob->getOfxParamHandle(),customInteract);
        layout->addWidget(_imp->customInteract);
    } else {
        createWidget(layout);
        onHasModificationsChanged();
        updateToolTip();
    }
    if ( knob->isAnimationEnabled() ) {
        createAnimationButton(layout);
    }
    
   
    _imp->widgetCreated = true;

    for (int i = 0; i < knob->getDimension(); ++i) {
        updateGuiInternal(i);
        std::string exp = knob->getExpression(i);
        reflectExpressionState(i,!exp.empty());
        if (exp.empty()) {
            onAnimationLevelChanged(i, knob->getAnimationLevel(i) );
        }
        if (knob->isSlave(i)) {
            setReadOnly_(true, i);
        }
    }
}

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
KnobGui::createAnimationButton(QHBoxLayout* layout)
{
    _imp->animationMenu = new Natron::Menu( layout->parentWidget() );
    //_imp->animationMenu->setFont( QFont(appFont,appFontSize) );
    QPixmap pix;
    appPTR->getIcon(Natron::NATRON_PIXMAP_CURVE, &pix);
    _imp->animationButton = new AnimationButton( this,QIcon(pix),"",layout->parentWidget() );
    _imp->animationButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    _imp->animationButton->setToolTip( Natron::convertFromPlainText(tr("Animation menu."), Qt::WhiteSpaceNormal) );
    QObject::connect( _imp->animationButton,SIGNAL( animationMenuRequested() ),this,SLOT( showAnimationMenu() ) );
    layout->addWidget(_imp->animationButton);

    if ( getKnob()->getIsSecret() ) {
        _imp->animationButton->hide();
    }
}

void
KnobGui::onRightClickClicked(const QPoint & pos)
{
    QWidget *widget = qobject_cast<QWidget *>( sender() );

    if (widget) {
        QString objName = widget->objectName();
        objName = objName.remove("dim-");
        showRightClickMenuForDimension( pos, objName.toInt() );
    }
}

void
KnobGui::enableRightClickMenu(QWidget* widget,
                              int dimension)
{
    QString name("dim-");

    name.append( QString::number(dimension) );
    widget->setContextMenuPolicy(Qt::CustomContextMenu);
    widget->setObjectName(name);
    QObject::connect( widget,SIGNAL( customContextMenuRequested(QPoint) ),this,SLOT( onRightClickClicked(QPoint) ) );
}

bool
KnobGui::showDescriptionLabel() const
{
    boost::shared_ptr<KnobI> knob = getKnob();
    return !knob->getDescription().empty() && knob->isDescriptionVisible();
}

void
KnobGui::showRightClickMenuForDimension(const QPoint &,
                                        int dimension)
{
    boost::shared_ptr<KnobI> knob = getKnob();
    bool enabled = knob->isEnabled(dimension);

    if ( knob->getIsSecret() ) {
        return;
    }

    bool isAppKnob = knob->getHolder() && knob->getHolder()->getApp();
    
    createAnimationMenu(_imp->copyRightClickMenu,dimension);

    _imp->copyRightClickMenu->addSeparator();

    bool isSlave = knob->isSlave(dimension) || !knob->getExpression(dimension).empty();
    
    
    int dim = knob->getDimension();
    
    if (isAppKnob) {
        const char* copyValuesStr = dim > 1 ? "Copy Values" : "Copy Value";
        const char* pasteValuesStr = dim > 1 ? "Paste Values" : "Paste Value";
        
        QAction* copyValuesAction = new QAction(tr(copyValuesStr),_imp->copyRightClickMenu);
        copyValuesAction->setData( QVariant(dimension) );
        QObject::connect( copyValuesAction,SIGNAL( triggered() ),this,SLOT( onCopyValuesActionTriggered() ) );
        _imp->copyRightClickMenu->addAction(copyValuesAction);
        
        if (!isSlave) {
            bool isClipBoardEmpty = appPTR->isClipBoardEmpty();
            QAction* pasteAction = new QAction(tr(pasteValuesStr),_imp->copyRightClickMenu);
            pasteAction->setData( QVariant(dimension) );
            QObject::connect( pasteAction,SIGNAL( triggered() ),this,SLOT( onPasteValuesActionTriggered() ) );
            _imp->copyRightClickMenu->addAction(pasteAction);
            if (isClipBoardEmpty || !enabled) {
                pasteAction->setEnabled(false);
            }
        }
    }
    QAction* resetDefaultAction = new QAction(tr("Reset to default"),_imp->copyRightClickMenu);
    resetDefaultAction->setData( QVariant(dimension) );
    QObject::connect( resetDefaultAction,SIGNAL( triggered() ),this,SLOT( onResetDefaultValuesActionTriggered() ) );
    _imp->copyRightClickMenu->addAction(resetDefaultAction);
    if (isSlave || !enabled) {
        resetDefaultAction->setEnabled(false);
    }

    if (isAppKnob && !isSlave && enabled) {
        _imp->copyRightClickMenu->addSeparator();
        QAction* linkToAction = new QAction(tr("Link to"),_imp->copyRightClickMenu);
        linkToAction->setData(dimension);
        QObject::connect( linkToAction,SIGNAL( triggered() ),this,SLOT( onLinkToActionTriggered() ) );
        _imp->copyRightClickMenu->addAction(linkToAction);
        
        if (dim > 1) {
            QAction* linkToAction = new QAction(tr("Link to (all dimensions)"),_imp->copyRightClickMenu);
            linkToAction->setData(-1);
            QObject::connect( linkToAction,SIGNAL( triggered() ),this,SLOT( onLinkToActionTriggered() ) );
            _imp->copyRightClickMenu->addAction(linkToAction);
        }
        
    }

    addRightClickMenuEntries(_imp->copyRightClickMenu);
    _imp->copyRightClickMenu->exec( QCursor::pos() );
} // showRightClickMenuForDimension

void
KnobGui::createAnimationMenu(QMenu* menu,int dimension)
{
    boost::shared_ptr<KnobI> knob = getKnob();
    menu->clear();
    bool dimensionHasKeyframe = false;
    bool hasAllKeyframes = true;
    for (int i = 0; i < knob->getDimension(); ++i) {
        Natron::AnimationLevelEnum lvl = knob->getAnimationLevel(i);
        if (lvl != Natron::eAnimationLevelOnKeyframe) {
            hasAllKeyframes = false;
        } else if (dimension == i && lvl == Natron::eAnimationLevelOnKeyframe) {
            dimensionHasKeyframe = true;
        }
    }

    bool hasDimensionSlaved = false;
    bool hasAnimation = false;
    bool isEnabled = true;
    bool isSlaved0 = false;

    for (int i = 0; i < knob->getDimension(); ++i) {
        if ( knob->isSlave(i) ) {
            hasDimensionSlaved = true;
            if (i == 0) {
                isSlaved0 = true;
            }
        }
        if (knob->getKeyFramesCount(i) > 0) {
            hasAnimation = true;
        }
        if (hasDimensionSlaved && hasAnimation) {
            break;
        }
        if ( !knob->isEnabled(i) ) {
            isEnabled = false;
        }
    }
    bool isSlaved = (dimension == -1 || dimension == 0) ? isSlaved0 : knob->isSlave(dimension);

    bool isAppKnob = knob->getHolder() && knob->getHolder()->getApp() != 0;
    
    if ( isAppKnob && knob->isAnimationEnabled() ) {
        if (!hasDimensionSlaved) {
            if (knob->getDimension() > 1) {
                ///Multi-dim actions
                if (!hasAllKeyframes) {
                    QAction* setKeyAction = new QAction(tr("Set Key (all dimensions)"),menu);
                    setKeyAction->setData(-1);
                    QObject::connect( setKeyAction,SIGNAL( triggered() ),this,SLOT( onSetKeyActionTriggered() ) );
                    menu->addAction(setKeyAction);
                    if (!isEnabled) {
                        setKeyAction->setEnabled(false);
                    }
                } else {
                    QAction* removeKeyAction = new QAction(tr("Remove Key (all dimensions)"),menu);
                    removeKeyAction->setData(-1);
                    QObject::connect( removeKeyAction,SIGNAL( triggered() ),this,SLOT( onRemoveKeyActionTriggered() ) );
                    menu->addAction(removeKeyAction);
                    if (!isEnabled) {
                        removeKeyAction->setEnabled(false);
                    }
                }
                
                QAction* removeAnyAnimationAction = new QAction(tr("Remove animation (all dimensions)"),menu);
                removeAnyAnimationAction->setData(-1);
                QObject::connect( removeAnyAnimationAction,SIGNAL( triggered() ),this,SLOT( onRemoveAnimationActionTriggered() ) );
                menu->addAction(removeAnyAnimationAction);
                if (!hasAnimation || !isEnabled) {
                    removeAnyAnimationAction->setEnabled(false);
                }
            }
            if (dimension != -1 || knob->getDimension() == 1) {
                menu->addSeparator();
                {
                    ///Single dim action
                    if (!dimensionHasKeyframe) {
                        QAction* setKeyAction = new QAction(tr("Set Key"),menu);
                        setKeyAction->setData(dimension);
                        QObject::connect( setKeyAction,SIGNAL( triggered() ),this,SLOT( onSetKeyActionTriggered() ) );
                        menu->addAction(setKeyAction);
                        if (!isEnabled) {
                            setKeyAction->setEnabled(false);
                        }
                    } else {
                        QAction* removeKeyAction = new QAction(tr("Remove Key"),menu);
                        removeKeyAction->setData(dimension);
                        QObject::connect( removeKeyAction,SIGNAL( triggered() ),this,SLOT( onRemoveKeyActionTriggered() ) );
                        menu->addAction(removeKeyAction);
                        if (!isEnabled) {
                            removeKeyAction->setEnabled(false);
                        }
                    }
                    
                    QAction* removeAnyAnimationAction = new QAction(tr("Remove animation"),menu);
                    removeAnyAnimationAction->setData(dimension);
                    QObject::connect( removeAnyAnimationAction,SIGNAL( triggered() ),this,SLOT( onRemoveAnimationActionTriggered() ) );
                    menu->addAction(removeAnyAnimationAction);
                    if (!hasAnimation || !isEnabled) {
                        removeAnyAnimationAction->setEnabled(false);
                    }
                    
                }
            }
            menu->addSeparator();
        } // if (!isSlave)
        

        if (!hasDimensionSlaved) {
            QAction* showInCurveEditorAction = new QAction(tr("Show in curve editor"),menu);
            QObject::connect( showInCurveEditorAction,SIGNAL( triggered() ),this,SLOT( onShowInCurveEditorActionTriggered() ) );
            menu->addAction(showInCurveEditorAction);
            if (!hasAnimation || !isEnabled) {
                showInCurveEditorAction->setEnabled(false);
            }

            Natron::Menu* interpolationMenu = new Natron::Menu(menu);
            //interpolationMenu->setFont( QFont(appFont,appFontSize) );
            interpolationMenu->setTitle("Interpolation");
            menu->addAction( interpolationMenu->menuAction() );
            if (!isEnabled) {
                interpolationMenu->menuAction()->setEnabled(false);
            }

            QAction* constantInterpAction = new QAction(tr("Constant"),interpolationMenu);
            QObject::connect( constantInterpAction,SIGNAL( triggered() ),this,SLOT( onConstantInterpActionTriggered() ) );
            interpolationMenu->addAction(constantInterpAction);

            QAction* linearInterpAction = new QAction(tr("Linear"),interpolationMenu);
            QObject::connect( linearInterpAction,SIGNAL( triggered() ),this,SLOT( onLinearInterpActionTriggered() ) );
            interpolationMenu->addAction(linearInterpAction);

            QAction* smoothInterpAction = new QAction(tr("Smooth"),interpolationMenu);
            QObject::connect( smoothInterpAction,SIGNAL( triggered() ),this,SLOT( onSmoothInterpActionTriggered() ) );
            interpolationMenu->addAction(smoothInterpAction);

            QAction* catmullRomInterpAction = new QAction(tr("Catmull-Rom"),interpolationMenu);
            QObject::connect( catmullRomInterpAction,SIGNAL( triggered() ),this,SLOT( onCatmullromInterpActionTriggered() ) );
            interpolationMenu->addAction(catmullRomInterpAction);

            QAction* cubicInterpAction = new QAction(tr("Cubic"),interpolationMenu);
            QObject::connect( cubicInterpAction,SIGNAL( triggered() ),this,SLOT( onCubicInterpActionTriggered() ) );
            interpolationMenu->addAction(cubicInterpAction);

            QAction* horizInterpAction = new QAction(tr("Horizontal"),interpolationMenu);
            QObject::connect( horizInterpAction,SIGNAL( triggered() ),this,SLOT( onHorizontalInterpActionTriggered() ) );
            interpolationMenu->addAction(horizInterpAction);
        }

        QAction* copyAnimationAction = new QAction(tr("Copy animation"),menu);
        QObject::connect( copyAnimationAction,SIGNAL( triggered() ),this,SLOT( onCopyAnimationActionTriggered() ) );
        menu->addAction(copyAnimationAction);
        if (!hasAnimation) {
            copyAnimationAction->setEnabled(false);
        }

        if (!hasDimensionSlaved) {
            ///If the clipboard is either empty or has no animation, disable the Paste animation action.
            bool isClipBoardEmpty = appPTR->isClipBoardEmpty();
            std::list<Variant> values;
            std::list<boost::shared_ptr<Curve> > curves;
            std::list<boost::shared_ptr<Curve> > parametricCurves;
            std::map<int,std::string> stringAnimation;
            bool copyAnimation;
            
            std::string appID;
            std::string nodeFullyQualifiedName;
            std::string paramName;
            
            appPTR->getKnobClipBoard(&copyAnimation,&values,&curves,&stringAnimation,&parametricCurves,&appID,&nodeFullyQualifiedName,&paramName);

            QAction* pasteAction = new QAction(tr("Paste animation"),menu);
            QObject::connect( pasteAction,SIGNAL( triggered() ),this,SLOT( onPasteAnimationActionTriggered() ) );
            menu->addAction(pasteAction);
            if (!copyAnimation || isClipBoardEmpty || !isEnabled) {
                pasteAction->setEnabled(false);
            }
        }
    } //if ( knob->isAnimationEnabled() ) {
    
    if (isAppKnob) {
        menu->addSeparator();
        std::string hasExpr = knob->getExpression(0);
        if ((dimension != -1 || knob->getDimension() == 1) && isEnabled) {
            
            
            QAction* setExprAction = new QAction(!hasExpr.empty() ? tr("Edit expression...") : tr("Set expression..."),menu);
            QObject::connect(setExprAction,SIGNAL(triggered() ),this,SLOT(onSetExprActionTriggered()));
            setExprAction->setData(dimension);
            menu->addAction(setExprAction);
            
            QAction* clearExprAction = new QAction(tr("Clear expression"),menu);
            QObject::connect(clearExprAction,SIGNAL(triggered() ),this,SLOT(onClearExprActionTriggered()));
            clearExprAction->setData(dimension);
            clearExprAction->setEnabled(!hasExpr.empty());
            menu->addAction(clearExprAction);
            
        }
        
        if (knob->getDimension() > 1 && isEnabled) {
            QAction* setExprsAction = new QAction(!hasExpr.empty() ? tr("Edit expression (all dimensions)") :
                                                  tr("Set expression (all dimensions)"),menu);
            setExprsAction->setData(-1);
            QObject::connect(setExprsAction,SIGNAL(triggered() ),this,SLOT(onSetExprActionTriggered()));
            menu->addAction(setExprsAction);
            
            QAction* clearExprAction = new QAction(tr("Clear expression (all dimensions)"),menu);
            QObject::connect(clearExprAction,SIGNAL(triggered() ),this,SLOT(onClearExprActionTriggered()));
            clearExprAction->setData(-1);
            menu->addAction(clearExprAction);
            
            
        }
        
        ///find-out to which node that master knob belongs to
        KnobHolder* holder = knob->getHolder();
        EffectInstance* isEffect = dynamic_cast<EffectInstance*>(holder);
        
        boost::shared_ptr<NodeCollection> collec ;
        NodeGroup* isCollecGroup = 0;
        if (isEffect) {
            collec = isEffect->getNode()->getGroup();
            isCollecGroup = dynamic_cast<NodeGroup*>(collec.get());
        }
        
        
        if (isSlaved) {
            menu->addSeparator();
            
            std::string knobName;
            if (dimension != -1 || knob->getDimension() == 1) {
                std::pair<int,boost::shared_ptr<KnobI> > master = knob->getMaster(dimension);
                assert(master.second);
                
                assert(collec);
                NodeList nodes = collec->getNodes();
                if (isCollecGroup) {
                    nodes.push_back(isCollecGroup->getNode());
                }
                for (NodeList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
                    const std::vector< boost::shared_ptr<KnobI> > & knobs = (*it)->getKnobs();
                    bool shouldStop = false;
                    for (U32 j = 0; j < knobs.size(); ++j) {
                        if ( knobs[j].get() == master.second.get() ) {
                            knobName.append((*it)->getScriptName() );
                            shouldStop = true;
                            break;
                        }
                    }
                    if (shouldStop) {
                        break;
                    }
                }
                knobName.append(".");
                knobName.append( master.second->getName() );
                if (master.second->getDimension() > 1) {
                    knobName.append(".");
                    knobName.append( master.second->getDimensionName(master.first) );
                }
                
            }
            QString actionText = tr("Unlink");
            if (!knobName.empty()) {
                actionText.append(" from ");
                actionText.append(knobName.c_str());
            }
            QAction* unlinkAction = new QAction(actionText,menu);
            unlinkAction->setData( QVariant(dimension) );
            QObject::connect( unlinkAction,SIGNAL( triggered() ),this,SLOT( onUnlinkActionTriggered() ) );
            menu->addAction(unlinkAction);
        }
        
        if (isCollecGroup) {
            QAction* createMasterOnGroup = new QAction(tr("Create master on group"),menu);
            QObject::connect( createMasterOnGroup,SIGNAL( triggered() ),this,SLOT( onCreateMasterOnGroupActionTriggered() ) );
            menu->addAction(createMasterOnGroup);
        }
    } // if (isAppKnob)
} // createAnimationMenu

void
KnobGui::createDuplicateOnNode(Natron::EffectInstance* effect,bool linkExpression)
{
    ///find-out to which node that master knob belongs to
    assert( getKnob()->getHolder()->getApp() );
    boost::shared_ptr<KnobI> knob = getKnob();
    
    if (linkExpression) {
        for (int i = 0; i < knob->getDimension(); ++i) {
            std::string expr = knob->getExpression(i);
            if (!expr.empty()) {
                Natron::StandardButtonEnum rep = Natron::questionDialog(tr("Expression").toStdString(), tr("This operation will create "
                                                                                                           "an expression link between this parameter and the new parameter on the group"
                                                                                                           " which will wipe the current expression(s).\n"
                                                                                                           "Continue anyway ?").toStdString(),false,
                                                                        Natron::StandardButtons(Natron::eStandardButtonOk | Natron::eStandardButtonCancel));
                if (rep != Natron::eStandardButtonYes) {
                    return;
                }
            }
        }
    }
    KnobHolder* holder = knob->getHolder();
    EffectInstance* isEffect = dynamic_cast<EffectInstance*>(holder);
    assert(isEffect);

    Bool_Knob* isBool = dynamic_cast<Bool_Knob*>(knob.get());
    Int_Knob* isInt = dynamic_cast<Int_Knob*>(knob.get());
    Double_Knob* isDbl = dynamic_cast<Double_Knob*>(knob.get());
    Choice_Knob* isChoice = dynamic_cast<Choice_Knob*>(knob.get());
    Color_Knob* isColor = dynamic_cast<Color_Knob*>(knob.get());
    String_Knob* isString = dynamic_cast<String_Knob*>(knob.get());
    File_Knob* isFile = dynamic_cast<File_Knob*>(knob.get());
    OutputFile_Knob* isOutputFile = dynamic_cast<OutputFile_Knob*>(knob.get());
    Path_Knob* isPath = dynamic_cast<Path_Knob*>(knob.get());
    Group_Knob* isGrp = dynamic_cast<Group_Knob*>(knob.get());
    Page_Knob* isPage = dynamic_cast<Page_Knob*>(knob.get());
    Button_Knob* isBtn = dynamic_cast<Button_Knob*>(knob.get());
    Parametric_Knob* isParametric = dynamic_cast<Parametric_Knob*>(knob.get());
    
    const std::string& nodeScriptName = isEffect->getNode()->getScriptName();
    std::string newKnobName = nodeScriptName +  knob->getName();
    
    //Ensure the group user page is created
    boost::shared_ptr<NodeGuiI> groupNodeGuiI = effect->getNode()->getNodeGui();
    NodeGui* groupNodeGui = dynamic_cast<NodeGui*>(groupNodeGuiI.get());
    assert(groupNodeGui);
    groupNodeGui->ensurePanelCreated();
    NodeSettingsPanel* groupNodePanel = groupNodeGui->getSettingPanel();
    assert(groupNodePanel);
    boost::shared_ptr<Page_Knob> groupUserPageNode = groupNodePanel->getUserPageKnob();
    
    boost::shared_ptr<KnobI> output;
    if (isBool) {
        boost::shared_ptr<Bool_Knob> newKnob = effect->createBoolKnob(newKnobName, knob->getDescription());
        output = newKnob;
    } else if (isInt) {
        boost::shared_ptr<Int_Knob> newKnob = effect->createIntKnob(newKnobName, knob->getDescription(),knob->getDimension());
        newKnob->setMinimumsAndMaximums(isInt->getMinimums(), isInt->getMaximums());
        newKnob->setDisplayMinimumsAndMaximums(isInt->getDisplayMinimums(),isInt->getDisplayMaximums());
        output = newKnob;
    } else if (isDbl) {
        boost::shared_ptr<Double_Knob> newKnob = effect->createDoubleKnob(newKnobName, knob->getDescription(),knob->getDimension());
        newKnob->setMinimumsAndMaximums(isDbl->getMinimums(), isDbl->getMaximums());
        newKnob->setDisplayMinimumsAndMaximums(isDbl->getDisplayMinimums(),isDbl->getDisplayMaximums());
        output = newKnob;
    } else if (isChoice) {
        boost::shared_ptr<Choice_Knob> newKnob = effect->createChoiceKnob(newKnobName, knob->getDescription());
        newKnob->populateChoices(isChoice->getEntries_mt_safe(),isChoice->getEntriesHelp_mt_safe());
        output = newKnob;
    } else if (isColor) {
        boost::shared_ptr<Color_Knob> newKnob = effect->createColorKnob(newKnobName, knob->getDescription(),knob->getDimension());
        newKnob->setMinimumsAndMaximums(isColor->getMinimums(), isColor->getMaximums());
        newKnob->setDisplayMinimumsAndMaximums(isColor->getDisplayMinimums(),isColor->getDisplayMaximums());
        output = newKnob;
    } else if (isString) {
        boost::shared_ptr<String_Knob> newKnob = effect->createStringKnob(newKnobName, knob->getDescription());
        if (isString->isLabel()) {
            newKnob->setAsLabel();
        }
        if (isString->isCustomKnob()) {
            newKnob->setAsCustom();
        }
        if (isString->isMultiLine()) {
            newKnob->setAsMultiLine();
        }
        if (isString->usesRichText()) {
            newKnob->setUsesRichText(true);
        }
        output = newKnob;
    } else if (isFile) {
        boost::shared_ptr<File_Knob> newKnob = effect->createFileKnob(newKnobName, knob->getDescription());
        if (isFile->isInputImageFile()) {
            newKnob->setAsInputImage();
        }
        output = newKnob;
    } else if (isOutputFile) {
        boost::shared_ptr<OutputFile_Knob> newKnob = effect->createOuptutFileKnob(newKnobName, knob->getDescription());
        if (isOutputFile->isOutputImageFile()) {
            newKnob->setAsOutputImageFile();
        }
        output = newKnob;
    } else if (isPath) {
        boost::shared_ptr<Path_Knob> newKnob = effect->createPathKnob(newKnobName, knob->getDescription());
        if (isPath->isMultiPath()) {
            newKnob->setMultiPath(true);
        }
        output = newKnob;
        
    } else if (isGrp) {
        boost::shared_ptr<Group_Knob> newKnob = effect->createGroupKnob(newKnobName, knob->getDescription());
        if (isGrp->isTab()) {
            newKnob->setAsTab();
        }
        output = newKnob;
        
    } else if (isPage) {
        boost::shared_ptr<Page_Knob> newKnob = effect->createPageKnob(newKnobName, knob->getDescription());
        output = newKnob;
        
    } else if (isBtn) {
        boost::shared_ptr<Button_Knob> newKnob = effect->createButtonKnob(newKnobName, knob->getDescription());
        output = newKnob;
        
    } else if (isParametric) {
        boost::shared_ptr<Parametric_Knob> newKnob = effect->createParametricKnob(newKnobName, knob->getDescription(), isParametric->getDimension());
        output = newKnob;
    }
    output->cloneDefaultValues(knob.get());
    output->clone(knob.get());
    if (knob->canAnimate()) {
        output->setAnimationEnabled(knob->isAnimationEnabled());
    }
    output->setEvaluateOnChange(knob->getEvaluateOnChange());
    output->setHintToolTip(knob->getHintToolTip());
    output->setAddNewLine(knob->isNewLineActivated());
    groupUserPageNode->addKnob(output);
    
    if (linkExpression) {
        effect->getNode()->declarePythonFields();
        
        boost::shared_ptr<NodeCollection> collec = isEffect->getNode()->getGroup();
        NodeGroup* isCollecGroup = dynamic_cast<NodeGroup*>(collec.get());
        
        std::stringstream ss;
        if (isCollecGroup) {
            ss << "thisGroup." << newKnobName;
        } else {
            ss << "app." << effect->getNode()->getFullyQualifiedName() << "." << newKnobName;
        }
        if (output->getDimension() > 1) {
            ss << ".get()[dimension]";
        } else {
            ss << ".get()";
        }
        
        try {
            std::string script = ss.str();
            for (int i = 0; i < knob->getDimension(); ++i) {
                knob->clearExpression(i, true);
                knob->setExpression(i, script, false);
            }
        } catch (...) {
            
        }
    }
    effect->refreshKnobs();
    holder->getApp()->triggerAutoSave();
}

void
KnobGui::onCreateMasterOnGroupActionTriggered()
{
    KnobHolder* holder = getKnob()->getHolder();
    EffectInstance* isEffect = dynamic_cast<EffectInstance*>(holder);
    assert(isEffect);
    boost::shared_ptr<NodeCollection> collec = isEffect->getNode()->getGroup();
    NodeGroup* isCollecGroup = dynamic_cast<NodeGroup*>(collec.get());
    assert(isCollecGroup);
    createDuplicateOnNode(isCollecGroup,true);
}

void
KnobGui::onUnlinkActionTriggered()
{
    QAction* action = qobject_cast<QAction*>(sender());
    if (!action) {
        return;
    }
    int dim = action->data().toInt();
    boost::shared_ptr<KnobI> thisKnob = getKnob();
    int dims = thisKnob->getDimension();
    
    thisKnob->beginChanges();
    for (int i = 0; i < dims; ++i) {
        if (dim == -1 || i == dim) {
            std::pair<int,boost::shared_ptr<KnobI> > other = thisKnob->getMaster(i);
            thisKnob->onKnobUnSlaved(i);
            onKnobSlavedChanged(i, false);
        }
    }
    thisKnob->endChanges();
    getKnob()->getHolder()->getApp()->triggerAutoSave();
}

void
KnobGui::onSetExprActionTriggered()
{
    QAction* action = qobject_cast<QAction*>(sender());
    assert(action);
    
    int dim = action->data().toInt();
    
  
    EditExpressionDialog* dialog = new EditExpressionDialog(dim,this,_imp->container);
    dialog->create(getKnob()->getExpression(dim == -1 ? 0 : dim).c_str(), true);
    QObject::connect( dialog,SIGNAL( accepted() ),this,SLOT( onEditExprDialogFinished() ) );
    QObject::connect( dialog,SIGNAL( rejected() ),this,SLOT( onEditExprDialogFinished() ) );
    
    dialog->show();
    
}

void
KnobGui::onClearExprActionTriggered()
{
    QAction* act = qobject_cast<QAction*>(sender());
    assert(act);
    int dim = act->data().toInt();
    pushUndoCommand(new SetExpressionCommand(getKnob(),false,dim,""));
}

void
KnobGui::onEditExprDialogFinished()
{
    
    EditExpressionDialog* dialog = dynamic_cast<EditExpressionDialog*>( sender() );
    
    if (dialog) {
        QDialog::DialogCode ret = (QDialog::DialogCode)dialog->result();
        
        switch (ret) {
            case QDialog::Accepted: {
                bool hasRetVar;
                QString expr = dialog->getExpression(&hasRetVar);
                std::string stdExpr = expr.toStdString();
                int dim = dialog->getDimension();
                std::string existingExpr = getKnob()->getExpression(dim);
                if (existingExpr != stdExpr) {
                    pushUndoCommand(new SetExpressionCommand(getKnob(),hasRetVar,dim,stdExpr));
                }
            } break;
            case QDialog::Rejected:
                break;
        }
        
        dialog->deleteLater();

    }
}

void
KnobGui::setSecret()
{
    bool showit = !isSecretRecursive();
    if (showit) {
        show(); //
    } else {
        hide();
    }
    Group_KnobGui* isGrp = dynamic_cast<Group_KnobGui*>(this);
    if (isGrp) {
        const std::list<KnobGui*>& children = isGrp->getChildren();
        for (std::list<KnobGui*>::const_iterator it = children.begin(); it != children.end(); ++it) {
            (*it)->setSecret();
        }
    }
}

bool
KnobGui::isSecretRecursive() const
{
    // If the Knob is within a group, only show it if the group is unfolded!
    // To test it:
    // try TuttlePinning: fold all groups, then switch from perspective to affine to perspective.
    //  VISIBILITY is different from SECRETNESS. The code considers that both things are equivalent, which is wrong.
    // Of course, this check has to be *recursive* (in case the group is within a folded group)
    boost::shared_ptr<KnobI> knob = getKnob();
    bool showit = !knob->getIsSecret();
    boost::shared_ptr<KnobI> parentKnob = knob->getParentKnob();
    
    while (showit && parentKnob && parentKnob->typeName() == "Group") {
        Group_KnobGui* parentGui = dynamic_cast<Group_KnobGui*>( _imp->container->getKnobGui(parentKnob) );
        assert(parentGui);
        // check for secretness and visibility of the group
        if ( parentKnob->getIsSecret() || ( parentGui && !parentGui->isChecked() ) ) {
            showit = false; // one of the including groups is folder, so this item is hidden
        }
        // prepare for next loop iteration
        parentKnob = parentKnob->getParentKnob();
    }
    return !showit;
}

void
KnobGui::showAnimationMenu()
{
    createAnimationMenu(_imp->animationMenu,-1);
    _imp->animationMenu->exec( _imp->animationButton->mapToGlobal( QPoint(0,0) ) );
}

void
KnobGui::onShowInCurveEditorActionTriggered()
{
    boost::shared_ptr<KnobI> knob = getKnob();

    assert( knob->getHolder()->getApp() );
    getGui()->setCurveEditorOnTop();
    std::vector<boost::shared_ptr<Curve> > curves;
    for (int i = 0; i < knob->getDimension(); ++i) {
        boost::shared_ptr<Curve> c = getCurve(i);
        if ( c->isAnimated() ) {
            curves.push_back(c);
        }
    }
    if ( !curves.empty() ) {
        getGui()->getCurveEditor()->centerOn(curves);
    }
}

void
KnobGui::onRemoveAnimationActionTriggered()
{
    QAction* action = qobject_cast<QAction*>(sender());
    assert(action);
    int dim = action->data().toInt();
    
    boost::shared_ptr<KnobI> knob = getKnob();
    std::map<boost::shared_ptr<CurveGui> , std::vector<KeyFrame > > toRemove;
    
    
    for (int i = 0; i < knob->getDimension(); ++i) {
        
        if (dim == -1 || dim == i) {
            std::list<boost::shared_ptr<CurveGui> > curves = getGui()->getCurveEditor()->findCurve(this, i);
            for (std::list<boost::shared_ptr<CurveGui> >::iterator it = curves.begin(); it != curves.end(); ++it) {
                KeyFrameSet keys = (*it)->getInternalCurve()->getKeyFrames_mt_safe();
                
                std::vector<KeyFrame > vect;
                for (KeyFrameSet::const_iterator it2 = keys.begin(); it2 != keys.end(); ++it2) {
                    vect.push_back(*it2);
                }
                toRemove.insert(std::make_pair(*it, vect));
            }
            
        }
    }
    pushUndoCommand( new RemoveKeysCommand(getGui()->getCurveEditor()->getCurveWidget(),
                                           toRemove) );
    //refresh the gui so it doesn't indicate the parameter is animated anymore
    for (int i = 0; i < knob->getDimension(); ++i) {
        if (dim == -1 || dim == i) {
            updateGUI(i);
        }
    }
}

void
KnobGui::setInterpolationForDimensions(const std::vector<int> & dimensions,
                                       Natron::KeyframeTypeEnum interp)
{
    boost::shared_ptr<KnobI> knob = getKnob();
    
    for (U32 i = 0; i < dimensions.size(); ++i) {
        boost::shared_ptr<Curve> c = knob->getCurve(dimensions[i]);
        if (c) {
            int kfCount = c->getKeyFramesCount();
            for (int j = 0; j < kfCount; ++j) {
                c->setKeyFrameInterpolation(interp, j);
            }
            boost::shared_ptr<Curve> guiCurve = getCurve(dimensions[i]);
            if (guiCurve) {
                guiCurve->clone(*c);
            }
        }
    }
    if (knob->getHolder()) {
        knob->getHolder()->evaluate_public(knob.get(), knob->getEvaluateOnChange(), Natron::eValueChangedReasonNatronGuiEdited);
    }
    Q_EMIT keyInterpolationChanged();

}

void
KnobGui::onConstantInterpActionTriggered()
{
    boost::shared_ptr<KnobI> knob = getKnob();
    std::vector<int> dims;

    for (int i = 0; i < knob->getDimension(); ++i) {
        dims.push_back(i);
    }
    setInterpolationForDimensions(dims, Natron::eKeyframeTypeConstant);
}

void
KnobGui::onLinearInterpActionTriggered()
{
    boost::shared_ptr<KnobI> knob = getKnob();
    std::vector<int> dims;

    for (int i = 0; i < knob->getDimension(); ++i) {
        dims.push_back(i);
    }
    setInterpolationForDimensions(dims, Natron::eKeyframeTypeLinear);
}

void
KnobGui::onSmoothInterpActionTriggered()
{
    boost::shared_ptr<KnobI> knob = getKnob();
    std::vector<int> dims;

    for (int i = 0; i < knob->getDimension(); ++i) {
        dims.push_back(i);
    }
    setInterpolationForDimensions(dims, Natron::eKeyframeTypeSmooth);
}

void
KnobGui::onCatmullromInterpActionTriggered()
{
    boost::shared_ptr<KnobI> knob = getKnob();
    std::vector<int> dims;

    for (int i = 0; i < knob->getDimension(); ++i) {
        dims.push_back(i);
    }
    setInterpolationForDimensions(dims, Natron::eKeyframeTypeCatmullRom);
}

void
KnobGui::onCubicInterpActionTriggered()
{
    boost::shared_ptr<KnobI> knob = getKnob();
    std::vector<int> dims;

    for (int i = 0; i < knob->getDimension(); ++i) {
        dims.push_back(i);
    }
    setInterpolationForDimensions(dims, Natron::eKeyframeTypeCubic);
}

void
KnobGui::onHorizontalInterpActionTriggered()
{
    boost::shared_ptr<KnobI> knob = getKnob();
    std::vector<int> dims;

    for (int i = 0; i < knob->getDimension(); ++i) {
        dims.push_back(i);
    }
    setInterpolationForDimensions(dims, Natron::eKeyframeTypeHorizontal);
}

void
KnobGui::setKeyframe(double time,
                     int dimension)
{
    boost::shared_ptr<KnobI> knob = getKnob();

    assert( knob->getHolder()->getApp() );
    
    bool keyAdded = knob->onKeyFrameSet(time, dimension);
    
    Q_EMIT keyFrameSet();
    
    if ( !knob->getIsSecret() && keyAdded && knob->isDeclaredByPlugin()) {
        knob->getHolder()->getApp()->getTimeLine()->addKeyframeIndicator(time);
    }
}

void
KnobGui::setKeyframe(double time,const KeyFrame& key,int dimension)
{
    boost::shared_ptr<KnobI> knob = getKnob();
    
    assert( knob->getHolder()->getApp() );
    
    bool keyAdded = knob->onKeyFrameSet(time, key, dimension);
    
    Q_EMIT keyFrameSet();
    if ( !knob->getIsSecret() && keyAdded && knob->isDeclaredByPlugin() ) {
        knob->getHolder()->getApp()->getTimeLine()->addKeyframeIndicator(time);
    }
}

void
KnobGui::onSetKeyActionTriggered()
{
    QAction* action = qobject_cast<QAction*>(sender());
    assert(action);
    int dim = action->data().toInt();

    boost::shared_ptr<KnobI> knob = getKnob();

    assert( knob->getHolder()->getApp() );
    //get the current time on the global timeline
    SequenceTime time = knob->getHolder()->getApp()->getTimeLine()->currentFrame();

    AddKeysCommand::KeysToAddList toAdd;
    
    for (int i = 0; i < knob->getDimension(); ++i) {
        
        if (dim == -1 || i == dim) {
            
            std::list<boost::shared_ptr<CurveGui> > curves = getGui()->getCurveEditor()->findCurve(this, i);
            for (std::list<boost::shared_ptr<CurveGui> >::iterator it = curves.begin(); it != curves.end(); ++it) {
                KeyFrame kf;
                kf.setTime(time);
                Knob<int>* isInt = dynamic_cast<Knob<int>*>( knob.get() );
                Knob<bool>* isBool = dynamic_cast<Knob<bool>*>( knob.get() );
                AnimatingString_KnobHelper* isString = dynamic_cast<AnimatingString_KnobHelper*>( knob.get() );
                Knob<double>* isDouble = dynamic_cast<Knob<double>*>( knob.get() );
                
                if (isInt) {
                    kf.setValue( isInt->getValue(i) );
                } else if (isBool) {
                    kf.setValue( isBool->getValue(i) );
                } else if (isDouble) {
                    kf.setValue( isDouble->getValue(i) );
                } else if (isString) {
                    std::string v = isString->getValue(i);
                    double dv;
                    isString->stringToKeyFrameValue(time, v, &dv);
                    kf.setValue(dv);
                }
                
                std::vector<KeyFrame> kvec;
                kvec.push_back(kf);
                toAdd.insert(std::make_pair(*it,kvec));
            }
        }
    }
    pushUndoCommand( new AddKeysCommand(getGui()->getCurveEditor()->getCurveWidget(), toAdd) );
}

void
KnobGui::removeKeyFrame(double time,
                        int dimension)
{
    boost::shared_ptr<KnobI> knob = getKnob();
    knob->onKeyFrameRemoved(time, dimension);
    Q_EMIT keyFrameRemoved();
    

    assert( knob->getHolder()->getApp() );
    if ( !knob->getIsSecret() ) {
        knob->getHolder()->getApp()->getTimeLine()->removeKeyFrameIndicator(time);
    }
    updateGUI(dimension);
}

void
KnobGui::setKeyframes(const std::vector<KeyFrame>& keys, int dimension)
{
    boost::shared_ptr<KnobI> knob = getKnob();
    
    assert( knob->getHolder()->getApp() );
    
    std::list<SequenceTime> times;
    for (std::size_t i = 0; i < keys.size(); ++i) {
        bool keyAdded = knob->onKeyFrameSet(keys[i].getTime(), keys[i], dimension);
        if (keyAdded) {
            times.push_back(keys[i].getTime());
        }
    }
    Q_EMIT keyFrameSet();
    if ( !knob->getIsSecret() && knob->isDeclaredByPlugin() ) {
        knob->getHolder()->getApp()->getTimeLine()->addMultipleKeyframeIndicatorsAdded(times, true);
    }
}

void
KnobGui::removeKeyframes(const std::vector<KeyFrame>& keys, int dimension)
{
    boost::shared_ptr<KnobI> knob = getKnob();
    for (std::size_t i = 0; i < keys.size(); ++i) {
        knob->onKeyFrameRemoved(keys[i].getTime(), dimension);
    }
    
    assert( knob->getHolder()->getApp() );
    if ( !knob->getIsSecret() ) {
        std::list<SequenceTime> times;
        for (std::size_t i = 0; i < keys.size(); ++i) {
            times.push_back(keys[i].getTime());
        }
        knob->getHolder()->getApp()->getTimeLine()->removeMultipleKeyframeIndicator(times, true);
    }

    Q_EMIT keyFrameRemoved();
    updateGUI(dimension);
}

QString
KnobGui::getScriptNameHtml() const
{
    return  QString("<font size = 4><b>%1</b></font>").arg( getKnob()->getName().c_str() );
}

QString
KnobGui::toolTip() const
{
    boost::shared_ptr<KnobI> knob = getKnob();
    Choice_Knob* isChoice = dynamic_cast<Choice_Knob*>(knob.get());
    QString tt = getScriptNameHtml();
    
    QString realTt;
    if (!isChoice) {
        realTt.append( knob->getHintToolTip().c_str() );
    } else {
        realTt.append( isChoice->getHintToolTipFull().c_str() );
    }
    
    std::vector<std::string> expressions;
    bool exprAllSame = true;
    for (int i = 0; i < knob->getDimension(); ++i) {
        expressions.push_back(knob->getExpression(i));
        if (i > 0 && expressions[i] != expressions[0]) {
            exprAllSame = false;
        }
    }
    
    QString exprTt;
    if (exprAllSame) {
        if (!expressions[0].empty()) {
            exprTt = QString("<br>ret = <b>%1</b></br>").arg(expressions[0].c_str());
        }
    } else {
        for (int i = 0; i < knob->getDimension(); ++i) {
            std::string dimName = knob->getDimensionName(i);
            QString toAppend = QString("<br>%1 = <b>%2</b></br>").arg(dimName.c_str()).arg(expressions[i].c_str());
            exprTt.append(toAppend);
        }
    }
    
    if (!exprTt.isEmpty()) {
        tt.append(exprTt);
    }

    if ( !realTt.isEmpty() ) {
        realTt = Natron::convertFromPlainText(realTt.trimmed(), Qt::WhiteSpaceNormal);
        tt.append(realTt);
    }

    return tt;
}

bool
KnobGui::hasToolTip() const
{
    //Always true now that we display the script name in the tooltip
    return true; //!getKnob()->getHintToolTip().empty();
}

void
KnobGui::onRemoveKeyActionTriggered()
{
    QAction* action = qobject_cast<QAction*>(sender());
    assert(action);
    int dim = action->data().toInt();
    
    boost::shared_ptr<KnobI> knob = getKnob();
    
    assert( knob->getHolder()->getApp() );
    //get the current time on the global timeline
    SequenceTime time = knob->getHolder()->getApp()->getTimeLine()->currentFrame();
    std::map<boost::shared_ptr<CurveGui> , std::vector<KeyFrame > > toRemove;
    for (int i = 0; i < knob->getDimension(); ++i) {
        
        if (dim == -1 || i == dim) {
            std::list<boost::shared_ptr<CurveGui> > curves = getGui()->getCurveEditor()->findCurve(this, i);
            for (std::list<boost::shared_ptr<CurveGui> >::iterator it = curves.begin(); it != curves.end(); ++it) {
                
                KeyFrame kf;
                bool foundKey = knob->getCurve(i)->getKeyFrameWithTime(time, &kf);
                
                if (foundKey) {
                    std::vector<KeyFrame > vect;
                    vect.push_back(kf);
                    toRemove.insert( std::make_pair(*it,vect) );
                }
            }
            
        }
    }
    pushUndoCommand( new RemoveKeysCommand(getGui()->getCurveEditor()->getCurveWidget(),
                                           toRemove) );
}

int
KnobGui::getKnobsCountOnSameLine() const
{
    return _imp->knobsOnSameLine.size();
}

void
KnobGui::hide()
{
    if (!_imp->customInteract) {
        _hide();
    } else {
        _imp->customInteract->hide();
    }
    if (_imp->animationButton) {
        _imp->animationButton->hide();
    }
    //also  hide the curve from the curve editor if there's any and the knob is not inside a group
    if ( getKnob()->getHolder()->getApp() ) {
        boost::shared_ptr<KnobI> parent = getKnob()->getParentKnob();
        bool isSecret = true;
        while (parent) {
            if (!parent->getIsSecret()) {
                isSecret = false;
                break;
            }
            parent = parent->getParentKnob();
        }
        if (isSecret) {
            getGui()->getCurveEditor()->hideCurves(this);
        }
    }

    ////In order to remove the row of the layout we have to make sure ALL the knobs on the row
    ////are hidden.
    bool shouldRemoveWidget = true;
    for (U32 i = 0; i < _imp->knobsOnSameLine.size(); ++i) {
        KnobGui* sibling = _imp->container->getKnobGui(_imp->knobsOnSameLine[i].lock());
        if ( sibling && !sibling->isSecretRecursive() ) {
            shouldRemoveWidget = false;
        }
    }

    if (shouldRemoveWidget) {
        _imp->field->hide();
    } else {
        if (!_imp->field->isVisible()) {
            _imp->field->show();
        }
    }
    if (_imp->descriptionLabel) {
        _imp->descriptionLabel->hide();
    }
}

void
KnobGui::show(int /*index*/)
{
    if (!getGui()) {
        return;
    }
    if (!_imp->customInteract) {
        _show();
    } else {
        _imp->customInteract->show();
    }
    if (_imp->animationButton) {
        _imp->animationButton->show();
    }
    //also show the curve from the curve editor if there's any
    if ( getKnob()->getHolder()->getApp() ) {
        getGui()->getCurveEditor()->showCurves(this);
    }

    if (_imp->isOnNewLine) {
        _imp->field->show();
    }
    
    if (_imp->descriptionLabel) {
        _imp->descriptionLabel->show();
    }
}

int
KnobGui::getActualIndexInLayout() const
{
    if (!_imp->containerLayout) {
        return -1;
    }
    for (int i = 0; i < _imp->containerLayout->rowCount(); ++i) {
        QLayoutItem* item = _imp->containerLayout->itemAtPosition(i, 1);
        if ( item && (item->widget() == _imp->field) ) {
            return i;
        }
    }

    return -1;
}

bool
KnobGui::isOnNewLine() const
{
    return _imp->isOnNewLine;
}

void
KnobGui::setEnabledSlot()
{
    if (!getGui()) {
        return;
    }
    if (!_imp->customInteract) {
        setEnabled();
    }
    boost::shared_ptr<KnobI> knob = getKnob();
    if (_imp->descriptionLabel) {
        _imp->descriptionLabel->setEnabled( knob->isEnabled(0) );
    }
    if ( knob->getHolder()->getApp() ) {
        for (int i = 0; i < knob->getDimension(); ++i) {
            if ( !knob->isEnabled(i) ) {
                getGui()->getCurveEditor()->hideCurve(this,i);
            } else {
                getGui()->getCurveEditor()->showCurve(this,i);
            }
        }
    }
}

QWidget*
KnobGui::getFieldContainer() const
{
    return _imp->field;
}

void
KnobGui::onInternalValueChanged(int dimension,
                                int reason)
{
    if (_imp->widgetCreated && (Natron::ValueChangedReasonEnum)reason != Natron::eValueChangedReasonUserEdited) {
        updateGuiInternal(dimension);
    }
}

void
KnobGui::updateCurveEditorKeyframes()
{
    Q_EMIT keyFrameSet();
}

void
KnobGui::onMultipleKeySet(const std::list<SequenceTime>& keys,int /*dimension*/, int reason)
{
    
    if ((Natron::ValueChangedReasonEnum)reason != Natron::eValueChangedReasonUserEdited) {
        boost::shared_ptr<KnobI> knob = getKnob();
        if ( !knob->getIsSecret() && knob->isDeclaredByPlugin()) {
            knob->getHolder()->getApp()->getTimeLine()->addMultipleKeyframeIndicatorsAdded(keys, true);
        }
    }
    
    updateCurveEditorKeyframes();

}

void
KnobGui::onInternalKeySet(SequenceTime time,
                          int /*dimension*/,
                          int reason,
                          bool added )
{
    if ((Natron::ValueChangedReasonEnum)reason != Natron::eValueChangedReasonUserEdited) {
        if (added) {
            boost::shared_ptr<KnobI> knob = getKnob();
            if ( !knob->getIsSecret() && knob->isDeclaredByPlugin()) {
                knob->getHolder()->getApp()->getTimeLine()->addKeyframeIndicator(time);
            }
        }

    }
    
    updateCurveEditorKeyframes();
}

void
KnobGui::onInternalKeyRemoved(SequenceTime time,
                              int /*dimension*/,
                              int /*reason*/)
{
    boost::shared_ptr<KnobI> knob = getKnob();
    if ( !knob->getIsSecret() && knob->isDeclaredByPlugin()) {
        knob->getHolder()->getApp()->getTimeLine()->removeKeyFrameIndicator(time);
    }
    Q_EMIT keyFrameRemoved();
}

void
KnobGui::copyAnimationToClipboard() const
{
    copyToClipBoard(true);
}

void
KnobGui::onCopyValuesActionTriggered()
{
    copyValuesToCliboard();
}

void
KnobGui::copyValuesToCliboard()
{
    copyToClipBoard(false);
}

void
KnobGui::onCopyAnimationActionTriggered()
{
    copyAnimationToClipboard();
}

void
KnobGui::copyToClipBoard(bool copyAnimation) const
{
    std::list<Variant> values;
    std::list<boost::shared_ptr<Curve> > curves;
    std::list<boost::shared_ptr<Curve> > parametricCurves;
    std::map<int,std::string> stringAnimation;
    boost::shared_ptr<KnobI> knob = getKnob();

    Knob<int>* isInt = dynamic_cast<Knob<int>*>( knob.get() );
    Knob<bool>* isBool = dynamic_cast<Knob<bool>*>( knob.get() );
    Knob<double>* isDouble = dynamic_cast<Knob<double>*>( knob.get() );
    Knob<std::string>* isString = dynamic_cast<Knob<std::string>*>( knob.get() );
    AnimatingString_KnobHelper* isAnimatingString = dynamic_cast<AnimatingString_KnobHelper*>( knob.get() );
    boost::shared_ptr<Parametric_Knob> isParametric = boost::dynamic_pointer_cast<Parametric_Knob>(knob);


    for (int i = 0; i < knob->getDimension(); ++i) {
        if (isInt) {
            values.push_back( Variant( isInt->getValue(i) ) );
        } else if (isBool) {
            values.push_back( Variant( isBool->getValue(i) ) );
        } else if (isDouble) {
            values.push_back( Variant( isDouble->getValue(i) ) );
        } else if (isString) {
            values.push_back( Variant( isString->getValue(i).c_str() ) );
        }
        if (copyAnimation) {
            boost::shared_ptr<Curve> c(new Curve);
            c->clone( *knob->getCurve(i) );
            curves.push_back(c);
        }
    }

    if (isAnimatingString) {
        isAnimatingString->saveAnimation(&stringAnimation);
    }

    if (isParametric) {
        std::list< Curve > tmpCurves;
        isParametric->saveParametricCurves(&tmpCurves);
        for (std::list< Curve >::iterator it = tmpCurves.begin(); it != tmpCurves.end(); ++it) {
            boost::shared_ptr<Curve> c(new Curve);
            c->clone(*it);
            parametricCurves.push_back(c);
        }
    }
    
    std::string appID = getGui()->getApp()->getAppIDString();
    std::string nodeFullyQualifiedName;
    KnobHolder* holder = getKnob()->getHolder();
    if (holder) {
        Natron::EffectInstance* isEffect = dynamic_cast<Natron::EffectInstance*>(holder);
        if (isEffect) {
            nodeFullyQualifiedName = isEffect->getNode()->getFullyQualifiedName();
        }
    }
    std::string paramName = getKnob()->getName();

    appPTR->setKnobClipBoard(copyAnimation,values,curves,stringAnimation,parametricCurves,appID,nodeFullyQualifiedName,paramName);
}

void
KnobGui::pasteClipBoard()
{
    if ( appPTR->isClipBoardEmpty() ) {
        return;
    }

    std::list<Variant> values;
    std::list<boost::shared_ptr<Curve> > curves;
    std::list<boost::shared_ptr<Curve> > parametricCurves;
    std::map<int,std::string> stringAnimation;
    bool copyAnimation;

    std::string appID;
    std::string nodeFullyQualifiedName;
    std::string paramName;
    
    appPTR->getKnobClipBoard(&copyAnimation,&values,&curves,&stringAnimation,&parametricCurves,&appID,&nodeFullyQualifiedName,&paramName);

    boost::shared_ptr<KnobI> knob = getKnob();

    Knob<int>* isInt = dynamic_cast<Knob<int>*>( knob.get() );
    Knob<bool>* isBool = dynamic_cast<Knob<bool>*>( knob.get() );
    Knob<double>* isDouble = dynamic_cast<Knob<double>*>( knob.get() );
    Knob<std::string>* isString = dynamic_cast<Knob<std::string>*>( knob.get() );
    boost::shared_ptr<Parametric_Knob> isParametric = boost::dynamic_pointer_cast<Parametric_Knob>(knob);

    int i = 0;
    for (std::list<Variant>::iterator it = values.begin(); it != values.end(); ++it) {
        if (isInt) {
            if ( !it->canConvert(QVariant::Int) ) {
                QString err = tr("Cannot paste values from a parameter of type %1 to a parameter of type Integer").arg( it->typeName() );
                Natron::errorDialog( tr("Paste").toStdString(),err.toStdString() );

                return;
            }
        } else if (isBool) {
            if ( !it->canConvert(QVariant::Bool) ) {
                QString err = tr("Cannot paste values from a parameter of type %1 to a parameter of type Boolean").arg( it->typeName() );
                Natron::errorDialog( tr("Paste").toStdString(),err.toStdString() );

                return;
            }
        } else if (isDouble) {
            if ( !it->canConvert(QVariant::Double) ) {
                QString err = tr("Cannot paste values from a parameter of type %1 to a parameter of type Double").arg( it->typeName() );
                Natron::errorDialog( tr("Paste").toStdString(),err.toStdString() );

                return;
            }
        } else if (isString) {
            if ( !it->canConvert(QVariant::String) ) {
                QString err = tr("Cannot paste values from a parameter of type %1 to a parameter of type String").arg( it->typeName() );
                Natron::errorDialog( tr("Paste").toStdString(),err.toStdString() );

                return;
            }
        }

        ++i;
    }

    pushUndoCommand( new PasteUndoCommand(this,copyAnimation,values,curves,parametricCurves,stringAnimation) );
} // pasteClipBoard

void
KnobGui::onPasteAnimationActionTriggered()
{
    pasteClipBoard();
}

void
KnobGui::pasteValuesFromClipboard()
{
    pasteClipBoard();
}

void
KnobGui::onPasteValuesActionTriggered()
{
    pasteValuesFromClipboard();
}

void
KnobGui::onKnobSlavedChanged(int dimension,
                             bool b)
{
    if (b) {
        Q_EMIT keyFrameRemoved();
    } else {
        Q_EMIT keyFrameSet();
    }
    setReadOnly_(b, dimension);
}

void
KnobGui::linkTo(int dimension)
{
    
    boost::shared_ptr<KnobI> thisKnob = getKnob();
    assert(thisKnob);
    Natron::EffectInstance* isEffect = dynamic_cast<Natron::EffectInstance*>(thisKnob->getHolder());
    if (!isEffect) {
        return;
    }
    
    
    for (int i = 0; i < thisKnob->getDimension(); ++i) {
        
        if (i == dimension || dimension == -1) {
            std::string expr = thisKnob->getExpression(dimension);
            if (!expr.empty()) {
                errorDialog(tr("Param Link").toStdString(),tr("This parameter already has an expression set, edit or clear it.").toStdString());
                return;
            }
        }
    }
    
    LinkToKnobDialog dialog( this,_imp->copyRightClickMenu->parentWidget() );

    if ( dialog.exec() ) {
        boost::shared_ptr<KnobI>  otherKnob = dialog.getSelectedKnobs();
        if (otherKnob) {
            if ( !thisKnob->isTypeCompatible(otherKnob) ) {
                errorDialog( tr("Param Link").toStdString(), tr("Types incompatibles!").toStdString() );

                return;
            }
            
            

            for (int i = 0; i < thisKnob->getDimension(); ++i) {
                std::pair<int,boost::shared_ptr<KnobI> > existingLink = thisKnob->getMaster(i);
                if (existingLink.second) {
                    std::string err( tr("Cannot link ").toStdString() );
                    err.append( thisKnob->getDescription() );
                    err.append( " \n " + tr("because the knob is already linked to ").toStdString() );
                    err.append( existingLink.second->getDescription() );
                    errorDialog(tr("Param Link").toStdString(), err);

                    return;
                }
                
            }
            
            Natron::EffectInstance* otherEffect = dynamic_cast<Natron::EffectInstance*>(otherKnob->getHolder());
            if (!otherEffect) {
                return;
            }
            
            std::stringstream expr;
            boost::shared_ptr<NodeCollection> thisCollection = isEffect->getNode()->getGroup();
            NodeGroup* otherIsGroup = dynamic_cast<NodeGroup*>(otherEffect);
            if (otherIsGroup == thisCollection.get()) {
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
                if (i == dimension || dimension == -1) {
                    thisKnob->setExpression(i, expr.str(), false);
                }
            }
            thisKnob->endChanges();


            thisKnob->getHolder()->getApp()->triggerAutoSave();
        }
    }
}

void
KnobGui::onLinkToActionTriggered()
{
    QAction* action = qobject_cast<QAction*>(sender());
    assert(action);
    
    linkTo(action->data().toInt());
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
KnobGui::resetDefault(int /*dimension*/)
{
    boost::shared_ptr<KnobI> knob = getKnob();
    Button_Knob* isBtn = dynamic_cast<Button_Knob*>( knob.get() );
    Page_Knob* isPage = dynamic_cast<Page_Knob*>( knob.get() );
    Group_Knob* isGroup = dynamic_cast<Group_Knob*>( knob.get() );
    Separator_Knob* isSeparator = dynamic_cast<Separator_Knob*>( knob.get() );

    if (!isBtn && !isPage && !isGroup && !isSeparator) {
        std::list<boost::shared_ptr<KnobI> > knobs;
        knobs.push_back(knob);
        pushUndoCommand( new RestoreDefaultsCommand(knobs) );
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
//    bool hasDimensionEnabled = false;
//    for (int i = 0; i < getKnob()->getDimension(); ++i) {
//        if (getKnob()->isEnabled(i)) {
//            hasDimensionEnabled = true;
//        }
//    }
//    _descriptionLabel->setEnabled(hasDimensionEnabled);
}

bool
KnobGui::triggerNewLine() const
{
    return _imp->triggerNewLine;
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
                                  int dim)
{
    boost::shared_ptr<KnobI> knob = getKnob();

    Knob<int>* isInt = dynamic_cast<Knob<int>*>( knob.get() );
    Knob<bool>* isBool = dynamic_cast<Knob<bool>*>( knob.get() );
    Knob<double>* isDouble = dynamic_cast<Knob<double>*>( knob.get() );
    Knob<std::string>* isString = dynamic_cast<Knob<std::string>*>( knob.get() );

    if (isInt) {
        pushUndoCommand( new KnobUndoCommand<int>(this,isInt->getValue(dim),v.toInt(),dim) );
    } else if (isBool) {
        pushUndoCommand( new KnobUndoCommand<bool>(this,isBool->getValue(dim),v.toBool(),dim) );
    } else if (isDouble) {
        pushUndoCommand( new KnobUndoCommand<double>(this,isDouble->getValue(dim),v.toDouble(),dim) );
    } else if (isString) {
        pushUndoCommand( new KnobUndoCommand<std::string>(this,isString->getValue(dim),v.toString().toStdString(),dim) );
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

///Should set to the underlying knob the gui ptr
void
KnobGui::setKnobGuiPointer()
{
    getKnob()->setKnobGuiPointer(this);
}

void
KnobGui::onInternalAnimationAboutToBeRemoved(int dimension)
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
    boost::shared_ptr<KnobI> knob = getKnob();
    if ( knob->getHolder() && knob->getHolder()->getApp() && !knob->getIsSecret() && knob->isDeclaredByPlugin()) {
        boost::shared_ptr<TimeLine> timeline = knob->getHolder()->getApp()->getTimeLine();
        std::list<SequenceTime> times;
        std::set<SequenceTime> tmpTimes;
        if (dimension == -1) {
            int dim = knob->getDimension();
            for (int i = 0; i < dim; ++i) {
                boost::shared_ptr<Curve> curve = knob->getCurve(i);
                if (curve) {
                    KeyFrameSet kfs = curve->getKeyFrames_mt_safe();
                    for (KeyFrameSet::iterator it = kfs.begin(); it != kfs.end(); ++it) {
                        tmpTimes.insert( it->getTime() );
                    }
                }
            }
            for (std::set<SequenceTime>::iterator it=tmpTimes.begin(); it!=tmpTimes.end(); ++it) {
                times.push_back(*it);
            }
        } else {
            boost::shared_ptr<Curve> curve = knob->getCurve(dimension);
            if (curve) {
                KeyFrameSet kfs = curve->getKeyFrames_mt_safe();
                for (KeyFrameSet::iterator it = kfs.begin(); it != kfs.end(); ++it) {
                    times.push_back( it->getTime() );
                }
            }
        }
        if (!times.empty()) {
            timeline->removeMultipleKeyframeIndicator(times,true);
        }
    }
}

void
KnobGui::setAllKeyframeMarkersOnTimeline(int dimension)
{
    boost::shared_ptr<KnobI> knob = getKnob();
    boost::shared_ptr<TimeLine> timeline = knob->getHolder()->getApp()->getTimeLine();
    std::list<SequenceTime> times;

    if (dimension == -1) {
        int dim = knob->getDimension();
        for (int i = 0; i < dim; ++i) {
            KeyFrameSet kfs = knob->getCurve(i)->getKeyFrames_mt_safe();
            for (KeyFrameSet::iterator it = kfs.begin(); it != kfs.end(); ++it) {
                times.push_back( it->getTime() );
            }
        }
    } else {
        KeyFrameSet kfs = knob->getCurve(dimension)->getKeyFrames_mt_safe();
        for (KeyFrameSet::iterator it = kfs.begin(); it != kfs.end(); ++it) {
            times.push_back( it->getTime() );
        }
    }
    timeline->addMultipleKeyframeIndicatorsAdded(times,true);
}

void
KnobGui::setKeyframeMarkerOnTimeline(int time)
{
    boost::shared_ptr<KnobI> knob = getKnob();
    if (knob->isDeclaredByPlugin()) {
        knob->getHolder()->getApp()->getTimeLine()->addKeyframeIndicator(time);
    }
}

void
KnobGui::onKeyFrameMoved(int /*dimension*/,
                         int oldTime,
                         int newTime)
{
    boost::shared_ptr<KnobI> knob = getKnob();

    if ( !knob->isAnimationEnabled() || !knob->canAnimate() ) {
        return;
    }
    if (knob->isDeclaredByPlugin()) {
        boost::shared_ptr<TimeLine> timeline = knob->getHolder()->getApp()->getTimeLine();
        timeline->removeKeyFrameIndicator(oldTime);
        timeline->addKeyframeIndicator(newTime);
    }
}

void
KnobGui::onAnimationLevelChanged(int dim,int level)
{
    if (!_imp->customInteract) {
        //std::string expr = getKnob()->getExpression(dim);
        //reflectExpressionState(dim,!expr.empty());
        //if (expr.empty()) {
            reflectAnimationLevel(dim, (Natron::AnimationLevelEnum)level);
        //}
        
    }
}

void
KnobGui::onAppendParamEditChanged(int reason,
                                  const Variant & v,
                                  int dim,
                                  int time,
                                  bool createNewCommand,
                                  bool setKeyFrame)
{
    pushUndoCommand( new MultipleKnobEditsUndoCommand(this,(Natron::ValueChangedReasonEnum)reason, createNewCommand,setKeyFrame,v,dim,time) );
}

void
KnobGui::onFrozenChanged(bool frozen)
{
    boost::shared_ptr<KnobI> knob = getKnob();
    Button_Knob* isBtn = dynamic_cast<Button_Knob*>(knob.get());
    if (isBtn) {
        return;
    }
    int dims = knob->getDimension();

    for (int i = 0; i < dims; ++i) {
        ///Do not unset read only if the knob is slaved in this dimension because we are still using it.
        if ( !frozen && knob->isSlave(i) ) {
            continue;
        }
        if (knob->isEnabled(i)) {
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
KnobGui::getCurve(int dimension) const
{
    return _imp->guiCurves[dimension];
}

void
KnobGui::onRefreshGuiCurve(int /*dimension*/)
{
    Q_EMIT refreshCurveEditor();
}


void
KnobGui::onExprChanged(int dimension)
{
    if (_imp->guiRemoved) {
        return;
    }
    boost::shared_ptr<KnobI> knob = getKnob();
    std::string exp = knob->getExpression(dimension);
    reflectExpressionState(dimension,!exp.empty());
    if (exp.empty()) {
        reflectAnimationLevel(dimension, knob->getAnimationLevel(dimension));
        
    }
    
    onHelpChanged();
    
    updateGUI(dimension);
    
    Q_EMIT expressionChanged();
}

void
KnobGui::onHelpChanged()
{
    if (_imp->descriptionLabel) {
        _imp->descriptionLabel->setToolTip( toolTip() );
    }
    updateToolTip();
}

void
KnobGui::onKnobDeletion()
{
    _imp->container->deleteKnobGui(getKnob());
}


void
KnobGui::onHasModificationsChanged()
{
    if (_imp->descriptionLabel) {
        bool hasModif = getKnob()->hasModifications();
        _imp->descriptionLabel->setAltered(!hasModif);
    }
    reflectModificationsState();
}

void
KnobGui::onDescriptionChanged()
{
    if (_imp->descriptionLabel) {
        _imp->descriptionLabel->setText(getKnob()->getDescription().c_str());
        onLabelChanged();
    }
}
