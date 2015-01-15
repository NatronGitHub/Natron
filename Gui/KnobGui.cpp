//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "Gui/KnobGui.h"

#include "Gui/KnobUndoCommand.h"

#include <cassert>
#include <climits>
#include <cfloat>
#include <stdexcept>

#include <QtCore/QString>
#include <QHBoxLayout>
#include <QPushButton>
#include <QFormLayout>
#include <QFileDialog>
#include <QTextEdit>
#include <QStyle> // in QtGui on Qt4, in QtWidgets on Qt5
#include <QTimer>

CLANG_DIAG_OFF(unused-private-field)
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QKeyEvent>
CLANG_DIAG_ON(unused-private-field)
#include <QColorDialog>
#include <QGroupBox>
#include <QtGui/QVector4D>
#include <QStyleFactory>
#include <QLabel>
#include <QMenu>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QCompleter>

#include "Engine/LibraryBinary.h"
#include "Global/GlobalDefines.h"

#include "Engine/Node.h"
#include "Engine/ViewerInstance.h"
#include "Engine/Settings.h"
#include "Engine/KnobFile.h"
#include "Engine/Curve.h"
#include "Engine/KnobTypes.h"
#include "Engine/TimeLine.h"
#include "Engine/KnobSerialization.h"
#include "Engine/Project.h"
#include "Engine/Variant.h"
#include "Engine/NodeGroup.h"

#include "Gui/AnimationButton.h"
#include "Gui/DockablePanel.h"
#include "Gui/ViewerTab.h"
#include "Gui/TimeLineGui.h"
#include "Gui/MenuWithToolTips.h"
#include "Gui/Gui.h"
#include "Gui/SequenceFileDialog.h"
#include "Gui/TabWidget.h"
#include "Gui/NodeGui.h"
#include "Gui/SpinBox.h"
#include "Gui/ComboBox.h"
#include "Gui/LineEdit.h"
#include "Gui/CurveEditor.h"
#include "Gui/KnobGuiTypes.h"
#include "Gui/CurveWidget.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/CustomParamInteract.h"
#include "Gui/NodeCreationDialog.h"
#include "Gui/ScriptTextEdit.h"

using namespace Natron;


struct KnobGui::KnobGuiPrivate
{
    bool triggerNewLine;
    int spacingBetweenItems;
    bool widgetCreated;
    DockablePanel* const container;
    QMenu* animationMenu;
    AnimationButton* animationButton;
    QMenu* copyRightClickMenu;
    QHBoxLayout* fieldLayout; //< the layout containing the widgets of the knob

    ////A vector of all other knobs on the same line.
    std::vector< boost::shared_ptr< KnobI > > knobsOnSameLine;
    QGridLayout* containerLayout;
    QWidget* field;
    QWidget* descriptionLabel;
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
        copyRightClickMenu->setFont( QFont(appFont,appFontSize) );
    }
    
    void removeFromKnobsOnSameLineVector(const boost::shared_ptr<KnobI>& knob)
    {
        for (std::vector< boost::shared_ptr< KnobI > >::iterator it = knobsOnSameLine.begin(); it != knobsOnSameLine.end(); ++it) {
            if (*it == knob) {
                knobsOnSameLine.erase(it);
                break;
            }
        }
    }
};

/////////////// KnobGui
KnobGui::KnobGui(boost::shared_ptr<KnobI> knob,
                 DockablePanel* container)
    : _imp( new KnobGuiPrivate(container) )
{
    knob->setKnobGuiPointer(this);
    KnobHelper* helper = dynamic_cast<KnobHelper*>( knob.get() );
    assert(helper);
    KnobSignalSlotHandler* handler = helper->getSignalSlotHandler().get();
    QObject::connect( handler,SIGNAL( refreshGuiCurve(int)),this,SLOT( onRefreshGuiCurve(int) ) );
    QObject::connect( handler,SIGNAL( valueChanged(int,int) ),this,SLOT( onInternalValueChanged(int,int) ) );
    QObject::connect( handler,SIGNAL( keyFrameSet(SequenceTime,int,int,bool) ),this,SLOT( onInternalKeySet(SequenceTime,int,int,bool) ) );
    QObject::connect( handler,SIGNAL( keyFrameRemoved(SequenceTime,int,int) ),this,SLOT( onInternalKeyRemoved(SequenceTime,int,int) ) );
    QObject::connect( handler,SIGNAL( keyFrameMoved(int,int,int)), this, SLOT( onKeyFrameMoved(int,int,int)));
    QObject::connect( handler,SIGNAL( secretChanged() ),this,SLOT( setSecret() ) );
    QObject::connect( handler,SIGNAL( enabledChanged() ),this,SLOT( setEnabledSlot() ) );
    QObject::connect( handler,SIGNAL( knobSlaved(int,bool) ),this,SLOT( onKnobSlavedChanged(int,bool) ) );
    QObject::connect( handler,SIGNAL( animationAboutToBeRemoved(int) ),this,SLOT( onInternalAnimationAboutToBeRemoved() ) );
    QObject::connect( handler,SIGNAL( animationRemoved(int) ),this,SLOT( onInternalAnimationRemoved() ) );
    QObject::connect( handler,SIGNAL( setValueWithUndoStack(Variant,int) ),this,SLOT( onSetValueUsingUndoStack(Variant,int) ) );
    QObject::connect( handler,SIGNAL( dirty(bool) ),this,SLOT( onSetDirty(bool) ) );
    QObject::connect( handler,SIGNAL( animationLevelChanged(int,int) ),this,SLOT( onAnimationLevelChanged(int,int) ) );
    QObject::connect( handler,SIGNAL( appendParamEditChange(Variant,int,int,bool,bool) ),this,
                      SLOT( onAppendParamEditChanged(Variant,int,int,bool,bool) ) );
    QObject::connect( handler,SIGNAL( frozenChanged(bool) ),this,SLOT( onFrozenChanged(bool) ) );
    QObject::connect( handler,SIGNAL( helpChanged() ),this,SLOT( onHelpChanged() ) );
    QObject::connect( handler,SIGNAL( expressionChanged(int) ),this,SLOT( onExprChanged(int) ) );

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

void
KnobGui::removeGui()
{
    for (std::vector< boost::shared_ptr< KnobI > >::iterator it = _imp->knobsOnSameLine.begin(); it!=_imp->knobsOnSameLine.end(); ++it) {
        KnobGui* kg = _imp->container->getKnobGui(*it);
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
                   QLabel* label,
                   QHBoxLayout* layout,
                   bool isOnNewLine,
                   const std::vector< boost::shared_ptr< KnobI > > & knobsOnSameLine)
{
    boost::shared_ptr<KnobI> knob = getKnob();

    _imp->containerLayout = containerLayout;
    _imp->fieldLayout = layout;
    _imp->knobsOnSameLine = knobsOnSameLine;
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
        updateToolTip();
    }
    if ( knob->isAnimationEnabled() ) {
        createAnimationButton(layout);
    }
    
   
    _imp->widgetCreated = true;

    for (int i = 0; i < knob->getDimension(); ++i) {
        updateGuiInternal(i);
        onAnimationLevelChanged(i, knob->getAnimationLevel(i) );
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
    _imp->animationMenu = new QMenu( layout->parentWidget() );
    _imp->animationMenu->setFont( QFont(appFont,appFontSize) );
    QPixmap pix;
    appPTR->getIcon(Natron::NATRON_PIXMAP_CURVE, &pix);
    _imp->animationButton = new AnimationButton( this,QIcon(pix),"",layout->parentWidget() );
    _imp->animationButton->setFixedSize(17, 17);
    _imp->animationButton->setToolTip( Qt::convertFromPlainText(tr("Animation menu"), Qt::WhiteSpaceNormal) );
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

    createAnimationMenu(_imp->copyRightClickMenu,dimension);

    _imp->copyRightClickMenu->addSeparator();

    bool isSlave = knob->isSlave(dimension);
    
    int dim = knob->getDimension();
    
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

    QAction* resetDefaultAction = new QAction(tr("Reset to default"),_imp->copyRightClickMenu);
    resetDefaultAction->setData( QVariant(dimension) );
    QObject::connect( resetDefaultAction,SIGNAL( triggered() ),this,SLOT( onResetDefaultValuesActionTriggered() ) );
    _imp->copyRightClickMenu->addAction(resetDefaultAction);
    if (isSlave || !enabled) {
        resetDefaultAction->setEnabled(false);
    }

    if (!isSlave && enabled) {
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
        
    } else if (isSlave) {
        
        EffectInstance* effect = dynamic_cast<EffectInstance*>(knob->getHolder());
        if (effect) {
            _imp->copyRightClickMenu->addSeparator();
            QAction* unlinkAction = new QAction(tr("Unlink"),_imp->copyRightClickMenu);
            unlinkAction->setData( QVariant(dimension) );
            QObject::connect( unlinkAction,SIGNAL( triggered() ),this,SLOT( onUnlinkActionTriggered() ) );
            _imp->copyRightClickMenu->addAction(unlinkAction);
            
            
            ///a stub action just to indicate what is the master knob.
            QAction* masterNameAction = new QAction("",_imp->copyRightClickMenu);
            std::pair<int,boost::shared_ptr<KnobI> > master = knob->getMaster(dimension);
            assert(master.second);
            
            ///find-out to which node that master knob belongs to
            std::string nodeName("Linked to: ");
            
            
            Natron::EffectInstance* masterEffect = dynamic_cast<Natron::EffectInstance*>(master.second->getHolder());
            assert(masterEffect);
            nodeName.append(masterEffect->getNode()->getFullyQualifiedName());
            nodeName.append(".");
            nodeName.append( master.second->getName() );
            if (master.second->getDimension() > 1) {
                nodeName.append(".");
                nodeName.append( master.second->getDimensionName(master.first) );
            }
            masterNameAction->setText( nodeName.c_str() );
            masterNameAction->setEnabled(false);
            _imp->copyRightClickMenu->addAction(masterNameAction);
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
    bool isOnKeyFrame = false;
    for (int i = 0; i < knob->getDimension(); ++i) {
        if (knob->getAnimationLevel(i) == Natron::eAnimationLevelOnKeyframe) {
            isOnKeyFrame = true;
            break;
        }
    }

    bool isSlave = false;
    bool hasAnimation = false;
    bool isEnabled = true;
    for (int i = 0; i < knob->getDimension(); ++i) {
        if ( knob->isSlave(i) ) {
            isSlave = true;
        }
        if (knob->getKeyFramesCount(i) > 0) {
            hasAnimation = true;
        }
        if (isSlave && hasAnimation) {
            break;
        }
        if ( !knob->isEnabled(i) ) {
            isEnabled = false;
        }
    }
    if ( knob->isAnimationEnabled() ) {
        if (!isSlave) {
            if (knob->getDimension() > 1) {
                ///Multi-dim actions
                if (!isOnKeyFrame) {
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
                    if (!isOnKeyFrame) {
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
        

        if (!isSlave) {
            QAction* showInCurveEditorAction = new QAction(tr("Show in curve editor"),menu);
            QObject::connect( showInCurveEditorAction,SIGNAL( triggered() ),this,SLOT( onShowInCurveEditorActionTriggered() ) );
            menu->addAction(showInCurveEditorAction);
            if (!hasAnimation || !isEnabled) {
                showInCurveEditorAction->setEnabled(false);
            }

            QMenu* interpolationMenu = new QMenu(menu);
            interpolationMenu->setFont( QFont(appFont,appFontSize) );
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

        if (!isSlave) {
            ///If the clipboard is either empty or has no animation, disable the Paste animation action.
            bool isClipBoardEmpty = appPTR->isClipBoardEmpty();
            std::list<Variant> values;
            std::list<boost::shared_ptr<Curve> > curves;
            std::list<boost::shared_ptr<Curve> > parametricCurves;
            std::map<int,std::string> stringAnimation;
            bool copyAnimation;

            appPTR->getKnobClipBoard(&copyAnimation,&values,&curves,&stringAnimation,&parametricCurves);

            QAction* pasteAction = new QAction(tr("Paste animation"),menu);
            QObject::connect( pasteAction,SIGNAL( triggered() ),this,SLOT( onPasteAnimationActionTriggered() ) );
            menu->addAction(pasteAction);
            if (!copyAnimation || isClipBoardEmpty || !isEnabled) {
                pasteAction->setEnabled(false);
            }
        }
    } //if ( knob->isAnimationEnabled() ) {
    
    menu->addSeparator();
    std::string hasExpr = knob->getExpression(0);
    if (dimension != -1 || knob->getDimension() == 1) {
        
        
        QAction* setExprAction = new QAction(!hasExpr.empty() ? tr("Edit expression...") : tr("Set expression..."),menu);
        QObject::connect(setExprAction,SIGNAL(triggered() ),this,SLOT(onSetExprActionTriggered()));
        setExprAction->setData(dimension);
        menu->addAction(setExprAction);
        
        if (!hasExpr.empty()) {
            QAction* clearExprAction = new QAction(tr("Clear expression"),menu);
            QObject::connect(clearExprAction,SIGNAL(triggered() ),this,SLOT(onClearExprActionTriggered()));
            clearExprAction->setData(dimension);
            menu->addAction(clearExprAction);
        }
    }
    
    if (knob->getDimension() > 1) {
            QAction* setExprsAction = new QAction(!hasExpr.empty() ? tr("Edit expression (all dimensions)") :
                                                  tr("Set expression (all dimensions)"),menu);
            setExprsAction->setData(-1);
            QObject::connect(setExprsAction,SIGNAL(triggered() ),this,SLOT(onSetExprActionTriggered()));
            menu->addAction(setExprsAction);
        if (!hasExpr.empty()) {
            
            QAction* clearExprAction = new QAction(tr("Clear expression (all dimensions)"),menu);
            QObject::connect(clearExprAction,SIGNAL(triggered() ),this,SLOT(onClearExprActionTriggered()));
            clearExprAction->setData(-1);
            menu->addAction(clearExprAction);
        }
        
    }
} // createAnimationMenu

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
    std::vector<std::pair<CurveGui *, KeyFrame > > toRemove;
    
    
    for (int i = 0; i < knob->getDimension(); ++i) {
        
        if (dim == -1 || dim == i) {
            std::list<CurveGui*> curves = getGui()->getCurveEditor()->findCurve(this, i);
            for (std::list<CurveGui*>::iterator it = curves.begin(); it != curves.end(); ++it) {
                KeyFrameSet keys = (*it)->getInternalCurve()->getKeyFrames_mt_safe();
                for (KeyFrameSet::const_iterator it2 = keys.begin(); it2 != keys.end(); ++it2) {
                    toRemove.push_back( std::make_pair(*it,*it2) );
                }
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
            
            std::list<CurveGui*> curves = getGui()->getCurveEditor()->findCurve(this, i);
            for (std::list<CurveGui*>::iterator it = curves.begin(); it != curves.end(); ++it) {
                boost::shared_ptr<AddKeysCommand::KeysForCurve> kfc(new AddKeysCommand::KeysForCurve());

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
                
                kfc->keys.push_back(kf);
                kfc->curve = *it;
                toAdd.push_back(kfc);
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
        realTt = Qt::convertFromPlainText(realTt,Qt::WhiteSpaceNormal);
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
    std::vector<std::pair<CurveGui*,KeyFrame> > toRemove;
    for (int i = 0; i < knob->getDimension(); ++i) {
        
        if (dim == -1 || i == dim) {
            std::list<CurveGui*> curves = getGui()->getCurveEditor()->findCurve(this, i);
            for (std::list<CurveGui*>::iterator it = curves.begin(); it != curves.end(); ++it) {
                
                KeyFrame kf;
                bool foundKey = knob->getCurve(i)->getKeyFrameWithTime(time, &kf);
                
                if (foundKey) {
                    toRemove.push_back( std::make_pair(*it,kf) );
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
    //also  hide the curve from the curve editor if there's any
    if ( getKnob()->getHolder()->getApp() ) {
        getGui()->getCurveEditor()->hideCurves(this);
    }

    ////In order to remove the row of the layout we have to make sure ALL the knobs on the row
    ////are hidden.
    bool shouldRemoveWidget = true;
    for (U32 i = 0; i < _imp->knobsOnSameLine.size(); ++i) {
        KnobGui* sibling = _imp->container->getKnobGui(_imp->knobsOnSameLine[i]);
        if ( sibling && !sibling->isSecretRecursive() ) {
            shouldRemoveWidget = false;
        }
    }

    if (shouldRemoveWidget) {
        _imp->field->hide();
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

    knob->getHolder()->getApp()->getTimeLine()->removeKeyFrameIndicator(time);
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

    appPTR->setKnobClipBoard(copyAnimation,values,curves,stringAnimation,parametricCurves);
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

    appPTR->getKnobClipBoard(&copyAnimation,&values,&curves,&stringAnimation,&parametricCurves);

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

struct LinkToKnobDialogPrivate
{
    KnobGui* fromKnob;
    QVBoxLayout* mainLayout;
    QHBoxLayout* firstLineLayout;
    QWidget* firstLine;
    QLabel* selectNodeLabel;
    CompleterLineEdit* nodeSelectionCombo;
    ComboBox* knobSelectionCombo;
    QDialogButtonBox* buttons;
    NodeList allNodes;
    std::map<QString,boost::shared_ptr<KnobI > > allKnobs;

    LinkToKnobDialogPrivate(KnobGui* from)
        : fromKnob(from)
        , mainLayout(0)
        , firstLineLayout(0)
        , firstLine(0)
        , selectNodeLabel(0)
        , nodeSelectionCombo(0)
        , knobSelectionCombo(0)
        , buttons(0)
        , allNodes()
        , allKnobs()
    {
    }
};

LinkToKnobDialog::LinkToKnobDialog(KnobGui* from,
                                   QWidget* parent)
    : QDialog(parent)
      , _imp( new LinkToKnobDialogPrivate(from) )
{
    _imp->mainLayout = new QVBoxLayout(this);

    _imp->firstLine = new QWidget(this);
    _imp->firstLineLayout = new QHBoxLayout(_imp->firstLine);

    _imp->mainLayout->addWidget(_imp->firstLine);

    _imp->buttons = new QDialogButtonBox(QDialogButtonBox::StandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel),
                                         Qt::Horizontal,this);
    QObject::connect( _imp->buttons, SIGNAL( accepted() ), this, SLOT( accept() ) );
    QObject::connect( _imp->buttons, SIGNAL( rejected() ), this, SLOT( reject() ) );
    _imp->mainLayout->addWidget(_imp->buttons);

    _imp->selectNodeLabel = new QLabel(tr("Parent:"),_imp->firstLine);
    _imp->firstLineLayout->addWidget(_imp->selectNodeLabel);


    EffectInstance* isEffect = dynamic_cast<EffectInstance*>(from->getKnob()->getHolder());
    assert(isEffect);
    boost::shared_ptr<NodeCollection> group = isEffect->getNode()->getGroup();
    
    group->getActiveNodes(&_imp->allNodes);
    QStringList nodeNames;
    for (NodeList::iterator it = _imp->allNodes.begin(); it != _imp->allNodes.end(); ++it) {
        QString name( (*it)->getLabel().c_str() );
        nodeNames.push_back(name);
        //_imp->nodeSelectionCombo->addItem(name);
    }
    nodeNames.sort();
    _imp->nodeSelectionCombo = new CompleterLineEdit(nodeNames,nodeNames,false,this);
    _imp->nodeSelectionCombo->setToolTip( tr("Input the name of a node in the current project.") );
    _imp->firstLineLayout->addWidget(_imp->nodeSelectionCombo);


    _imp->nodeSelectionCombo->setFocus(Qt::PopupFocusReason);
    QTimer::singleShot( 25, _imp->nodeSelectionCombo, SLOT( showCompleter() ) );

    _imp->knobSelectionCombo = new ComboBox(_imp->firstLine);
    _imp->firstLineLayout->addWidget(_imp->knobSelectionCombo);

    QObject::connect( _imp->nodeSelectionCombo,SIGNAL( itemCompletionChosen() ),this,SLOT( onNodeComboEditingFinished() ) );

    _imp->firstLineLayout->addStretch();
}

LinkToKnobDialog::~LinkToKnobDialog()
{
}

void
LinkToKnobDialog::onNodeComboEditingFinished()
{
    QString index = _imp->nodeSelectionCombo->text();

    _imp->knobSelectionCombo->clear();
    boost::shared_ptr<Natron::Node> selectedNode;
    std::string currentNodeName = index.toStdString();
    for (NodeList::iterator it = _imp->allNodes.begin(); it != _imp->allNodes.end(); ++it) {
        if ((*it)->getLabel() == currentNodeName) {
            selectedNode = *it;
            break;
        }
    }
    if (!selectedNode) {
        return;
    }

    const std::vector< boost::shared_ptr<KnobI> > & knobs = selectedNode->getKnobs();
    boost::shared_ptr<KnobI> from = _imp->fromKnob->getKnob();
    for (U32 j = 0; j < knobs.size(); ++j) {
        if ( !knobs[j]->getIsSecret() && (knobs[j] != from) ) {
            Button_Knob* isButton = dynamic_cast<Button_Knob*>( knobs[j].get() );
            Page_Knob* isPage = dynamic_cast<Page_Knob*>( knobs[j].get() );
            Group_Knob* isGroup = dynamic_cast<Group_Knob*>( knobs[j].get() );
            if (from->isTypeCompatible(knobs[j]) && !isButton && !isPage && !isGroup) {
                QString name( knobs[j]->getDescription().c_str() );

                bool canInsertKnob = true;
                for (int k = 0; k < knobs[j]->getDimension(); ++k) {
                    if ( knobs[j]->isSlave(k) || !knobs[j]->isEnabled(k) || name.isEmpty() ) {
                        canInsertKnob = false;
                    }
                }
                if (canInsertKnob) {
                    _imp->allKnobs.insert( std::make_pair( name, knobs[j]) );
                    _imp->knobSelectionCombo->addItem(name);
                }
            }
        }
    }
}

boost::shared_ptr<KnobI> LinkToKnobDialog::getSelectedKnobs() const
{
    QString str = _imp->knobSelectionCombo->itemText( _imp->knobSelectionCombo->activeIndex() );
    std::map<QString,boost::shared_ptr<KnobI> >::const_iterator it = _imp->allKnobs.find(str);

    if ( it != _imp->allKnobs.end() ) {
        return it->second;
    } else {
        return boost::shared_ptr<KnobI>();
    }
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
    LinkToKnobDialog dialog( this,_imp->copyRightClickMenu->parentWidget() );

    if ( dialog.exec() ) {
        boost::shared_ptr<KnobI> thisKnob = getKnob();
        boost::shared_ptr<KnobI>  otherKnob = dialog.getSelectedKnobs();
        if (otherKnob) {
            if ( !thisKnob->isTypeCompatible(otherKnob) ) {
                errorDialog( tr("Knob Link").toStdString(), tr("Types incompatibles!").toStdString() );

                return;
            }

            for (int i = 0; i < thisKnob->getDimension(); ++i) {
                std::pair<int,boost::shared_ptr<KnobI> > existingLink = thisKnob->getMaster(i);
                if (existingLink.second) {
                    std::string err( tr("Cannot link ").toStdString() );
                    err.append( thisKnob->getDescription() );
                    err.append( " \n " + tr("because the knob is already linked to ").toStdString() );
                    err.append( existingLink.second->getDescription() );
                    errorDialog(tr("Knob Link").toStdString(), err);

                    return;
                }
            }

            thisKnob->blockEvaluation();
            int dims = thisKnob->getDimension();
            for (int i = 0; i < dims; ++i) {
                if (i == dims - 1) {
                    thisKnob->unblockEvaluation();
                }
                if ((i == dimension || dimension == -1) && i < otherKnob->getDimension()) {
                    thisKnob->onKnobSlavedTo(i, otherKnob,i);
                    onKnobSlavedChanged(i, true);
                }
            }
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
KnobGui::unlink()
{
    boost::shared_ptr<KnobI> thisKnob = getKnob();
    int dims = thisKnob->getDimension();

    thisKnob->blockEvaluation();
    for (int i = 0; i < dims; ++i) {
        std::pair<int,boost::shared_ptr<KnobI> > other = thisKnob->getMaster(i);
        if (i == dims - 1) {
            thisKnob->unblockEvaluation();
        }
        thisKnob->onKnobUnSlaved(i);
        onKnobSlavedChanged(i, false);
    }
    getKnob()->getHolder()->getApp()->triggerAutoSave();
}

void
KnobGui::onUnlinkActionTriggered()
{
    unlink();
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
KnobGui::onInternalAnimationAboutToBeRemoved()
{
    removeAllKeyframeMarkersOnTimeline(-1);
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

    if ( knob->getHolder() && knob->getHolder()->getApp() ) {
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
        timeline->removeMultipleKeyframeIndicator(times,true);
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
        std::string expr = getKnob()->getExpression(dim);
        reflectExpressionState(dim,!expr.empty());
        if (expr.empty()) {
            reflectAnimationLevel(dim, (Natron::AnimationLevelEnum)level);
        }
        
    }
}

void
KnobGui::onAppendParamEditChanged(const Variant & v,
                                  int dim,
                                  int time,
                                  bool createNewCommand,
                                  bool setKeyFrame)
{
    pushUndoCommand( new MultipleKnobEditsUndoCommand(this,createNewCommand,setKeyFrame,v,dim,time) );
}

void
KnobGui::onFrozenChanged(bool frozen)
{
    boost::shared_ptr<KnobI> knob = getKnob();
    int dims = knob->getDimension();

    for (int i = 0; i < dims; ++i) {
        ///Do not unset read only if the knob is slaved in this dimension because we are still using it.
        if ( !frozen && knob->isSlave(i) ) {
            continue;
        }
        setReadOnly_(frozen, i);
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

struct EditScriptDialogPrivate
{
    QVBoxLayout* mainLayout;
    
    QLabel* expressionLabel;
    ScriptTextEdit* expressionEdit;
    
    QWidget* midButtonsContainer;
    QHBoxLayout* midButtonsLayout;
    
    Button* useRetButton;
    Button* helpButton;
    
    QLabel* resultLabel;
    ScriptTextEdit* resultEdit;
    
    QDialogButtonBox* buttons;
    
    EditScriptDialogPrivate()
    : mainLayout(0)
    , expressionLabel(0)
    , expressionEdit(0)
    , midButtonsContainer(0)
    , midButtonsLayout(0)
    , useRetButton(0)
    , helpButton(0)
    , resultLabel(0)
    , resultEdit(0)
    , buttons(0)
    {
        
    }
};

EditScriptDialog::EditScriptDialog(QWidget* parent)
: QDialog(parent)
, _imp(new EditScriptDialogPrivate())
{
    
    
}

void
EditScriptDialog::create(const QString& initialScript,bool makeUseRetButton)
{
    setTitle();
    
    QFont font(appFont,appFontSize);
    _imp->mainLayout = new QVBoxLayout(this);
    
    QStringList modules;
    getImportedModules(modules);
    std::list<std::pair<QString,QString> > variables;
    getDeclaredVariables(variables);
    QString labelHtml(tr("<br><b>Python</b> script: </br>"));
    if (!modules.empty()) {
        labelHtml.append(tr("<br>For convenience the following module(s) have been imported: </br> <br/>"));
        for (int i = 0; i < modules.size(); ++i) {
            QString toAppend = QString("<br><i><font color=orange>from %1 import *</font></i></br>").arg(modules[i]);
            labelHtml.append(toAppend);
        }
        labelHtml.append("<br/>");
    }
    if (!variables.empty()) {
        labelHtml.append(tr("<br>Also the following variables have been declared: </br>"
                         "<br/>"));
        for (std::list<std::pair<QString,QString> > ::iterator it = variables.begin(); it != variables.end(); ++it) {
            QString toAppend = QString("<br><b>%1</b>: %2</br>").arg(it->first).arg(it->second);
            labelHtml.append(toAppend);
        }
    }
    
    _imp->expressionLabel = new QLabel(labelHtml,this);
    _imp->expressionLabel->setFont(font);
    _imp->mainLayout->addWidget(_imp->expressionLabel);
    
    _imp->expressionEdit = new ScriptTextEdit(this);
    QFontMetrics fm = _imp->expressionEdit->fontMetrics();
    _imp->expressionEdit->setTabStopWidth(4 * fm.width(' '));
    _imp->mainLayout->addWidget(_imp->expressionEdit);
    _imp->expressionEdit->setPlainText(initialScript);
    
    _imp->midButtonsContainer = new QWidget(this);
    _imp->midButtonsLayout = new QHBoxLayout(_imp->midButtonsContainer);

    
    if (makeUseRetButton) {
        
        bool retVariable = hasRetVariable();
        _imp->useRetButton = new Button(tr("Multi-line"),_imp->midButtonsContainer);
        _imp->useRetButton->setToolTip(Qt::convertFromPlainText(tr("When checked the Python expression will be interpreted "
                                                                   "as series of statement. The return value should be then assigned to the "
                                                                   "\"ret\" variable. When unchecked the expression must not contain "
                                                                   "any new line character and the result will be interpreted from the "
                                                                   "interpretation of the single line."),Qt::WhiteSpaceNormal));
        _imp->useRetButton->setCheckable(true);
        bool checked = !initialScript.isEmpty() && retVariable;
        _imp->useRetButton->setChecked(checked);
        _imp->useRetButton->setDown(checked);
        QObject::connect(_imp->useRetButton, SIGNAL(clicked(bool)), this, SLOT(onUseRetButtonClicked(bool)));
        _imp->midButtonsLayout->addWidget(_imp->useRetButton);

    }
    
    
    _imp->helpButton = new Button(tr("Help"),_imp->midButtonsContainer);
    QObject::connect(_imp->helpButton, SIGNAL(clicked(bool)), this, SLOT(onHelpRequested()));
    _imp->midButtonsLayout->addWidget(_imp->helpButton);
    _imp->midButtonsLayout->addStretch();
    
    _imp->mainLayout->addWidget(_imp->midButtonsContainer);
    
    _imp->resultLabel = new QLabel(tr("Result:"),this);
    _imp->resultLabel->setFont(font);
    _imp->mainLayout->addWidget(_imp->resultLabel);
    
    _imp->resultEdit = new ScriptTextEdit(this);
    _imp->resultEdit->setOutput(true);
    _imp->resultEdit->setFixedHeight(80);
    _imp->resultEdit->setReadOnly(true);
    _imp->mainLayout->addWidget(_imp->resultEdit);
    
    _imp->buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,Qt::Horizontal,this);
    _imp->mainLayout->addWidget(_imp->buttons);
    QObject::connect(_imp->buttons,SIGNAL(accepted()),this,SLOT(accept()));
    QObject::connect(_imp->buttons,SIGNAL(rejected()),this,SLOT(reject()));
    
    if (!initialScript.isEmpty()) {
        compileAndSetResult(initialScript);
    }
    QObject::connect(_imp->expressionEdit, SIGNAL(textChanged()), this, SLOT(onTextEditChanged()));
    _imp->expressionEdit->setFocus();
}


void
EditScriptDialog::compileAndSetResult(const QString& script)
{
    QString ret = compileExpression(script);
    _imp->resultEdit->setPlainText(ret);
}

QString
EditScriptDialog::getHelpPart1()
{
    return tr("<br>Each node in the scope already has a variable declared with its name, e.g if you have a node named "
              "<b>Transform1</b> in your project, then you can type <i>Transform1</i> to reference that node.</br>"
              "<br>Note that the scope includes all nodes within the same group as thisNode and the parent group node itself, "
              "if the node belongs to a group. If the node itself is a group, then it can also have expressions depending "
              "on parameters of its children.</br>"
              "<br/>"
              "<br>Each node has all its parameters declared as fields and you can reference a specific parameter by typing it's <b>script name</b>, e.g:</br>"
              "<br>Transform1.rotate</br>"
              "<br>The script-name of a parameter is the name in bold that is shown in the tooltip when hovering a parameter with the mouse, this is what "
              "identifies a parameter internally.</br>");
}

QString
EditScriptDialog::getHelpThisNodeVariable()
{
    return tr("<br>The current node which expression is being edited can be referenced by the variable <i>thisNode</i> for convenience.</br>");
}

QString
EditScriptDialog::getHelpThisGroupVariable()
{
     return tr("<br>The parent group containing the thisNode can be referenced by the variable <i>thisGroup</i> for convenience, if and "
               "only if thisNode belongs to a group.</br>");
}

QString
EditScriptDialog::getHelpThisParamVariable()
{
    return tr("<br>The <i>thisParam</i> variable has been defined for convenience when editing an expression. It refers to the current parameter.</br>");
}

QString
EditScriptDialog::getHelpDimensionVariable()
{
    return tr("<br>In the same way the <i>dimension</i> variable has been defined and references the current dimension of the parameter which expression is being set"
              ".</br>"
              "<br>The <i>dimension</i> is a 0-based index identifying a specific field of a parameter. For instance if we're editing the expression of the y "
              "field of the translate parameter of Transform1, the <i>dimension</i> would be 1. </br>");

}

QString
EditScriptDialog::getHelpPart2()
{
    return tr("<br>To access values of a parameter several functions are made accessible: </br>"
              "<br/>"
              "<br>The <b>get()</b> function will return a Tuple containing all the values for each dimension of the parameter. For instance "
              "let's say we have a node Transform1 in our comp, we could then reference the x value of the <i>center</i> parameter this way:</br>"
              "<br/>"
              "<br>Transform1.center.get().x</br>"
              "<br/>"
              "<br>The <b>get(</b><i>frame</i><b>)</b> works exactly like the <b>get()</b> function excepts that it takes an extra "
              "<i>frame</i> parameter corresponding to the time at which we want to fetch the value. For parameters with an animation "
              "it would then return their value at the corresponding timeline position. That value would then be either interpolated "
              "with the current interpolation filter, or the exact keyframe at that time if one exists.</br>");
}

void
EditScriptDialog::onHelpRequested()
{
    QString help = getCustomHelp();
    Natron::informationDialog(tr("Help").toStdString(), help.toStdString(),true);
}


QString
EditScriptDialog::getExpression(bool* hasRetVariable) const
{
    if (_imp->useRetButton) {
        *hasRetVariable = _imp->useRetButton->isChecked();
    }
    return _imp->expressionEdit->toPlainText();
}

bool
EditScriptDialog::isUseRetButtonChecked() const
{
    return _imp->useRetButton->isChecked();
}

void
EditScriptDialog::onTextEditChanged()
{
    compileAndSetResult(_imp->expressionEdit->toPlainText());
}

void
EditScriptDialog::onUseRetButtonClicked(bool useRet)
{
    compileAndSetResult(_imp->expressionEdit->toPlainText());
    _imp->useRetButton->setDown(useRet);
}

EditScriptDialog::~EditScriptDialog()
{
    
}

void
EditScriptDialog::keyPressEvent(QKeyEvent* e)
{
    if ( (e->key() == Qt::Key_Return) || (e->key() == Qt::Key_Enter) ) {
        accept();
    } else if (e->key() == Qt::Key_Escape) {
        reject();
    } else {
        QDialog::keyPressEvent(e);
    }
    
}


EditExpressionDialog::EditExpressionDialog(int dimension,KnobGui* knob,QWidget* parent)
: EditScriptDialog(parent)
, _dimension(dimension)
, _knob(knob)
{
    
}

int
EditExpressionDialog::getDimension() const
{
    return _dimension;
}


void
EditExpressionDialog::setTitle()
{
    boost::shared_ptr<KnobI> k = _knob->getKnob();
    
    QString title(tr("Set expression on "));
    title.append(k->getName().c_str());
    if (_dimension != -1 && k->getDimension() > 1) {
        title.append(".");
        title.append(k->getDimensionName(_dimension).c_str());
    }
    setWindowTitle(title);

}

bool
EditExpressionDialog::hasRetVariable() const
{
    return _knob->getKnob()->isExpressionUsingRetVariable(_dimension == -1 ? 0 : _dimension);
}

QString
EditExpressionDialog::compileExpression(const QString& expr)
{
    
    std::string exprResult;
    try {
        _knob->getKnob()->validateExpression(expr.toStdString(),_dimension == -1 ? 0 : _dimension,isUseRetButtonChecked()
                                                  ,&exprResult);
    } catch(const std::exception& e) {
        QString err = QString(tr("ERROR") + ": %1").arg(e.what());
        return err;
    }
    return exprResult.c_str();
}


QString
EditExpressionDialog::getCustomHelp()
{
    return getHelpPart1() + "<br/>" +
    getHelpThisNodeVariable() + "<br/>" +
    getHelpThisGroupVariable() + "<br/>" +
    getHelpThisParamVariable() + "<br/>" +
    getHelpDimensionVariable() + "<br/>" +
    getHelpPart2();
}


void
EditExpressionDialog::getImportedModules(QStringList& modules) const
{
    modules.push_back("math");
}

void
EditExpressionDialog::getDeclaredVariables(std::list<std::pair<QString,QString> >& variables) const
{
    variables.push_back(std::make_pair("thisNode", tr("the current node")));
    variables.push_back(std::make_pair("thisGroup", tr("Defined only if thisNode belongs to a group, it references the parent group node")));
    variables.push_back(std::make_pair("thisParam", tr("the current param being edited")));
    variables.push_back(std::make_pair("dimension", tr("Defined only if the parameter is multi-dimensional, it references the dimension of the parameter being edited (0-based index")));
    variables.push_back(std::make_pair("frame", tr("the current time on the timeline")));
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


