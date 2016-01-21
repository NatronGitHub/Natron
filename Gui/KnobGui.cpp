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

#include <cassert>
#include <boost/weak_ptr.hpp>

#include "Engine/KnobTypes.h"

#include "Gui/KnobGui.h"
#include "Gui/KnobGuiPrivate.h"
#include "Gui/GuiDefines.h"
#include "Gui/ClickableLabel.h"


NATRON_NAMESPACE_ENTER;


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
        QObject::connect( handler,SIGNAL( redrawGuiCurve(int,int)),this,SLOT( onRedrawGuiCurve(int,int) ) );
        QObject::connect( handler,SIGNAL( valueChanged(int,int) ),this,SLOT( onInternalValueChanged(int,int) ) );
        QObject::connect( handler,SIGNAL( keyFrameSet(double,int,int,bool) ),this,SLOT( onInternalKeySet(double,int,int,bool) ) );
        QObject::connect( handler,SIGNAL( keyFrameRemoved(double,int,int) ),this,SLOT( onInternalKeyRemoved(double,int,int) ) );
        QObject::connect( handler,SIGNAL( keyFrameMoved(int,double,double)), this, SLOT( onKeyFrameMoved(int,double,double)));
        QObject::connect( handler,SIGNAL( multipleKeyFramesSet(std::list<double>,int,int)), this,
                         SLOT(onMultipleKeySet(std::list<double> , int, int)));
        QObject::connect( handler,SIGNAL( secretChanged() ),this,SLOT( setSecret() ) );
        QObject::connect( handler,SIGNAL( enabledChanged() ),this,SLOT( setEnabledSlot() ) );
        QObject::connect( handler,SIGNAL( knobSlaved(int,bool) ),this,SLOT( onKnobSlavedChanged(int,bool) ) );
        QObject::connect( handler,SIGNAL( animationAboutToBeRemoved(int) ),this,SLOT( onInternalAnimationAboutToBeRemoved(int) ) );
        QObject::connect( handler,SIGNAL( animationRemoved(int) ),this,SLOT( onInternalAnimationRemoved() ) );
        QObject::connect( handler,SIGNAL( setValueWithUndoStack(Variant,int) ),this,SLOT( onSetValueUsingUndoStack(Variant,int) ) );
        QObject::connect( handler,SIGNAL( dirty(bool) ),this,SLOT( onSetDirty(bool) ) );
        QObject::connect( handler,SIGNAL( animationLevelChanged(int,int) ),this,SLOT( onAnimationLevelChanged(int,int) ) );
        QObject::connect( handler,SIGNAL( appendParamEditChange(int,Variant,int,double,bool,bool) ),this,
                         SLOT( onAppendParamEditChanged(int,Variant,int,double,bool,bool) ) );
        QObject::connect( handler,SIGNAL( frozenChanged(bool) ),this,SLOT( onFrozenChanged(bool) ) );
        QObject::connect( handler,SIGNAL( helpChanged() ),this,SLOT( onHelpChanged() ) );
        QObject::connect( handler,SIGNAL( expressionChanged(int) ),this,SLOT( onExprChanged(int) ) );
        QObject::connect( handler,SIGNAL( hasModificationsChanged() ),this,SLOT( onHasModificationsChanged() ) );
        QObject::connect(handler,SIGNAL(labelChanged()), this, SLOT(onLabelChanged()));
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
                   ClickableLabel* label,
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
    _imp->animationMenu = new Menu( layout->parentWidget() );
    //_imp->animationMenu->setFont( QFont(appFont,appFontSize) );
    QPixmap pix;

    int iconSize = TO_DPIX(NATRON_MEDIUM_BUTTON_ICON_SIZE);
    QSize buttonSize(TO_DPIX(NATRON_MEDIUM_BUTTON_ICON_SIZE),TO_DPIY(NATRON_MEDIUM_BUTTON_ICON_SIZE));
    QSize buttonIconSize(TO_DPIX(NATRON_MEDIUM_BUTTON_ICON_SIZE),TO_DPIY(NATRON_MEDIUM_BUTTON_ICON_SIZE));

    appPTR->getIcon(NATRON_PIXMAP_CURVE, iconSize, &pix);
    _imp->animationButton = new AnimationButton( this,QIcon(pix),"",layout->parentWidget() );
    _imp->animationButton->setFixedSize(buttonSize);
    _imp->animationButton->setIconSize(buttonIconSize);
    _imp->animationButton->setToolTip( GuiUtils::convertFromPlainText(tr("Animation menu."), Qt::WhiteSpaceNormal) );
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
KnobGui::isLabelVisible() const
{
    boost::shared_ptr<KnobI> knob = getKnob();
    KnobString* isStringKnob = dynamic_cast<KnobString*>(knob.get());
    return isStringKnob || (!knob->getLabel().empty() && knob->isLabelVisible());
}

void
KnobGui::showRightClickMenuForDimension(const QPoint &,
                                        int dimension)
{
    boost::shared_ptr<KnobI> knob = getKnob();
    bool enabled = knob->isEnabled(dimension == -1 ? 0 : dimension);

    if ( knob->getIsSecret() ) {
        return;
    }

    bool isAppKnob = knob->getHolder() && knob->getHolder()->getApp();
    
    createAnimationMenu(_imp->copyRightClickMenu,dimension);

    _imp->copyRightClickMenu->addSeparator();

    bool isSlave = knob->isSlave(dimension == -1 ? 0 : dimension) || !knob->getExpression(dimension == -1 ? 0 : dimension).empty();
    
    
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
        AnimationLevelEnum lvl = knob->getAnimationLevel(i);
        if (lvl != eAnimationLevelOnKeyframe) {
            hasAllKeyframes = false;
        } else if (dimension == i && lvl == eAnimationLevelOnKeyframe) {
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

            Menu* interpolationMenu = new Menu(menu);
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
            
            boost::shared_ptr<KnobI> aliasMaster = knob->getAliasMaster();

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
                if (!aliasMaster && master.second->getDimension() > 1) {
                    knobName.append(".");
                    knobName.append( master.second->getDimensionName(master.first) );
                }
                
            }
            QString actionText;
            if (aliasMaster) {
                actionText.append(tr("Remove Alias link"));
            } else {
                actionText.append(tr("Unlink"));
            }
            if (!knobName.empty()) {
                actionText.append(" from ");
                actionText.append(knobName.c_str());
            }
            QAction* unlinkAction = new QAction(actionText,menu);
            unlinkAction->setData( QVariant(dimension) );
            QObject::connect( unlinkAction,SIGNAL( triggered() ),this,SLOT( onUnlinkActionTriggered() ) );
            menu->addAction(unlinkAction);
        }
        KnobI::ListenerDimsMap listeners;
        knob->getListeners(listeners);
        if (!listeners.empty()) {
            boost::shared_ptr<KnobI> listener = listeners.begin()->first.lock();
            if (listener && listener->getAliasMaster() == knob) {
                QAction* removeAliasLink = new QAction(tr("Remove alias link"),menu);
                QObject::connect( removeAliasLink,SIGNAL( triggered() ),this,SLOT( onRemoveAliasLinkActionTriggered() ) );
                menu->addAction(removeAliasLink);
            }
        }
        if (isCollecGroup && !knob->getAliasMaster()) {
            QAction* createMasterOnGroup = new QAction(tr("Create alias on group"),menu);
            QObject::connect( createMasterOnGroup,SIGNAL( triggered() ),this,SLOT( onCreateAliasOnGroupActionTriggered() ) );
            menu->addAction(createMasterOnGroup);
        }
    } // if (isAppKnob)
} // createAnimationMenu

boost::shared_ptr<KnobI>
KnobGui::createDuplicateOnNode(EffectInstance* effect,
                               bool makeAlias,
                               const boost::shared_ptr<KnobPage>& page,
                               const boost::shared_ptr<KnobGroup>& group,
                               int indexInParent)
{
    ///find-out to which node that master knob belongs to
    assert( getKnob()->getHolder()->getApp() );
    boost::shared_ptr<KnobI> knob = getKnob();
    
    if (!makeAlias) {
        for (int i = 0; i < knob->getDimension(); ++i) {
            std::string expr = knob->getExpression(i);
            if (!expr.empty()) {
                StandardButtonEnum rep = Dialogs::questionDialog(tr("Expression").toStdString(), tr("This operation will create "
                                                                                                           "an expression link between this parameter and the new parameter on the group"
                                                                                                           " which will wipe the current expression(s).\n"
                                                                                                           "Continue anyway ?").toStdString(),false,
                                                                        StandardButtons(eStandardButtonOk | eStandardButtonCancel));
                if (rep != eStandardButtonYes) {
                    return boost::shared_ptr<KnobI>();
                }
            }
        }
    }
    
    EffectInstance* isEffect = dynamic_cast<EffectInstance*>(knob->getHolder());
    if (!isEffect) {
        return boost::shared_ptr<KnobI>();
    }
    const std::string& nodeScriptName = isEffect->getNode()->getScriptName();
    std::string newKnobName = nodeScriptName +  knob->getName();
    boost::shared_ptr<KnobI> ret;
    try {
        ret = knob->createDuplicateOnNode(effect,
                                          page,
                                          group,
                                          indexInParent,
                                          makeAlias,
                                          newKnobName,
                                          knob->getLabel(),
                                          knob->getHintToolTip(),
                                          true);
    } catch (const std::exception& e) {
        Dialogs::errorDialog(tr("Error while creating parameter").toStdString(), e.what());
        return boost::shared_ptr<KnobI>();
    }
    if (ret) {
        boost::shared_ptr<NodeGuiI> groupNodeGuiI = effect->getNode()->getNodeGui();
        NodeGui* groupNodeGui = dynamic_cast<NodeGui*>(groupNodeGuiI.get());
        assert(groupNodeGui);
        groupNodeGui->ensurePanelCreated();
    }
    effect->getApp()->triggerAutoSave();
    return ret;
}

NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_KnobGui.cpp"
