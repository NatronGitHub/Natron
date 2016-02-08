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
#include <stdexcept>

#include <boost/weak_ptr.hpp>

#include "Engine/KnobTypes.h"

#include "Gui/KnobGui.h"
#include "Gui/KnobGuiPrivate.h"
#include "Gui/GuiDefines.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/ClickableLabel.h"


NATRON_NAMESPACE_ENTER;


/////////////// KnobGui
KnobGui::KnobGui(const KnobPtr& knob,
                 DockablePanel* container)
: _imp( new KnobGuiPrivate(container) )
{
    knob->setKnobGuiPointer(this);
    KnobHelper* helper = dynamic_cast<KnobHelper*>( knob.get() );
    assert(helper);
    if (helper) {
        KnobSignalSlotHandler* handler = helper->getSignalSlotHandler().get();
        QObject::connect( handler,SIGNAL( redrawGuiCurve(int,ViewIdx,int)),this,SLOT( onRedrawGuiCurve(int,ViewIdx,int) ) );
        QObject::connect( handler,SIGNAL( valueChanged(ViewIdx,int,int) ),this,SLOT( onInternalValueChanged(ViewIdx,int,int) ) );
        QObject::connect( handler,SIGNAL( keyFrameSet(double,ViewIdx,int,int,bool) ),this,SLOT( onInternalKeySet(double,ViewIdx,int,int,bool) ) );
        QObject::connect( handler,SIGNAL( keyFrameRemoved(double,ViewIdx,int,int) ),this,SLOT( onInternalKeyRemoved(double,ViewIdx,int,int) ) );
        QObject::connect( handler,SIGNAL( keyFrameMoved(ViewIdx,int,double,double)), this, SLOT( onKeyFrameMoved(ViewIdx,int,double,double)));
        QObject::connect( handler,SIGNAL( multipleKeyFramesSet(std::list<double>,ViewIdx,int,int)), this,
                         SLOT(onMultipleKeySet(std::list<double> , ViewIdx,int, int)));
        QObject::connect( handler,SIGNAL( secretChanged() ),this,SLOT( setSecret() ) );
        QObject::connect( handler,SIGNAL( enabledChanged() ),this,SLOT( setEnabledSlot() ) );
        QObject::connect( handler,SIGNAL( knobSlaved(int,bool) ),this,SLOT( onKnobSlavedChanged(int,bool) ) );
        QObject::connect( handler,SIGNAL( animationAboutToBeRemoved(ViewIdx,int) ),this,SLOT( onInternalAnimationAboutToBeRemoved(ViewIdx,int) ) );
        QObject::connect( handler,SIGNAL( animationRemoved(ViewIdx,int) ),this,SLOT( onInternalAnimationRemoved() ) );
        QObject::connect( handler,SIGNAL( setValueWithUndoStack(Variant,ViewIdx, int) ),this,SLOT( onSetValueUsingUndoStack(Variant,ViewIdx, int) ) );
        QObject::connect( handler,SIGNAL( dirty(bool) ),this,SLOT( onSetDirty(bool) ) );
        QObject::connect( handler,SIGNAL( animationLevelChanged(ViewIdx,int,int) ),this,SLOT( onAnimationLevelChanged(ViewIdx,int,int) ) );
        QObject::connect( handler,SIGNAL( appendParamEditChange(int,Variant,ViewIdx,int,double,bool,bool) ),this,
                         SLOT( onAppendParamEditChanged(int,Variant,ViewIdx,int,double,bool,bool) ) );
        QObject::connect( handler,SIGNAL( frozenChanged(bool) ),this,SLOT( onFrozenChanged(bool) ) );
        QObject::connect( handler,SIGNAL( helpChanged() ),this,SLOT( onHelpChanged() ) );
        QObject::connect( handler,SIGNAL( expressionChanged(int) ),this,SLOT( onExprChanged(int) ) );
        QObject::connect( handler,SIGNAL( hasModificationsChanged() ),this,SLOT( onHasModificationsChanged() ) );
        QObject::connect(handler,SIGNAL(labelChanged()), this, SLOT(onLabelChanged()));
    }
    _imp->guiCurves.resize(knob->getDimension());
    if (knob->canAnimate()) {
        for (int i = 0; i < knob->getDimension(); ++i) {
            _imp->guiCurves[i].reset(new Curve(*(knob->getCurve(ViewIdx(0),i))));
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
            delete _imp->labelContainer;
        }
        delete _imp->field;
    } else {
        delete _imp->descriptionLabel;
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
                   QWidget* labelContainer,
                   KnobClickableLabel* label,
                   QHBoxLayout* layout,
                   bool isOnNewLine,
                   const std::vector< boost::shared_ptr< KnobI > > & knobsOnSameLine)
{
    KnobPtr knob = getKnob();

    _imp->containerLayout = containerLayout;
    _imp->fieldLayout = layout;
    for (std::vector< boost::shared_ptr< KnobI > >::const_iterator it = knobsOnSameLine.begin(); it != knobsOnSameLine.end(); ++it) {
        _imp->knobsOnSameLine.push_back(*it);
    }
    _imp->field = fieldContainer;
    _imp->labelContainer = labelContainer;
    _imp->descriptionLabel = label;
    _imp->isOnNewLine = isOnNewLine;
    if (!isOnNewLine) {
        //layout->addStretch();
        layout->addSpacing(TO_DPIX(15));
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
    
   
    _imp->widgetCreated = true;

    for (int i = 0; i < knob->getDimension(); ++i) {
        updateGuiInternal(i);
        std::string exp = knob->getExpression(i);
        reflectExpressionState(i,!exp.empty());
        if (exp.empty()) {
            onAnimationLevelChanged(ViewIdx::ALL_VIEWS, i, knob->getAnimationLevel(i) );
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
    KnobPtr knob = getKnob();
    KnobString* isStringKnob = dynamic_cast<KnobString*>(knob.get());
    return isStringKnob || (!knob->getLabel().empty() && knob->isLabelVisible());
}

void
KnobGui::showRightClickMenuForDimension(const QPoint &,
                                        int dimension)
{
    KnobPtr knob = getKnob();

    if ( knob->getIsSecret() ) {
        return;
    }

    createAnimationMenu(_imp->copyRightClickMenu,dimension);
    addRightClickMenuEntries(_imp->copyRightClickMenu);
    _imp->copyRightClickMenu->exec( QCursor::pos() );
} // showRightClickMenuForDimension

Menu*
KnobGui::createInterpolationMenu(QMenu* menu, int dimension, bool isEnabled)
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
    
    QAction* constantInterpAction = new QAction(tr("Constant"),interpolationMenu);
    constantInterpAction->setData(QVariant(dimension));
    QObject::connect( constantInterpAction,SIGNAL( triggered() ),this,SLOT( onConstantInterpActionTriggered() ) );
    interpolationMenu->addAction(constantInterpAction);
    
    QAction* linearInterpAction = new QAction(tr("Linear"),interpolationMenu);
    linearInterpAction->setData(QVariant(dimension));
    QObject::connect( linearInterpAction,SIGNAL( triggered() ),this,SLOT( onLinearInterpActionTriggered() ) );
    interpolationMenu->addAction(linearInterpAction);
    
    QAction* smoothInterpAction = new QAction(tr("Smooth"),interpolationMenu);
    smoothInterpAction->setData(QVariant(dimension));
    QObject::connect( smoothInterpAction,SIGNAL( triggered() ),this,SLOT( onSmoothInterpActionTriggered() ) );
    interpolationMenu->addAction(smoothInterpAction);
    
    QAction* catmullRomInterpAction = new QAction(tr("Catmull-Rom"),interpolationMenu);
    catmullRomInterpAction->setData(QVariant(dimension));
    QObject::connect( catmullRomInterpAction,SIGNAL( triggered() ),this,SLOT( onCatmullromInterpActionTriggered() ) );
    interpolationMenu->addAction(catmullRomInterpAction);
    
    QAction* cubicInterpAction = new QAction(tr("Cubic"),interpolationMenu);
    cubicInterpAction->setData(QVariant(dimension));
    QObject::connect( cubicInterpAction,SIGNAL( triggered() ),this,SLOT( onCubicInterpActionTriggered() ) );
    interpolationMenu->addAction(cubicInterpAction);
    
    QAction* horizInterpAction = new QAction(tr("Horizontal"),interpolationMenu);
    horizInterpAction->setData(QVariant(dimension));
    QObject::connect( horizInterpAction,SIGNAL( triggered() ),this,SLOT( onHorizontalInterpActionTriggered() ) );
    interpolationMenu->addAction(horizInterpAction);
    
    menu->addAction( interpolationMenu->menuAction() );
    return interpolationMenu;
}

void
KnobGui::createAnimationMenu(QMenu* menu,int dimension)
{
    
    KnobPtr knob = getKnob();
    assert(dimension >= -1 && dimension < knob->getDimension());
    menu->clear();
    bool dimensionHasKeyframeAtTime = false;
    bool hasAllKeyframesAtTime = true;
    for (int i = 0; i < knob->getDimension(); ++i) {
        AnimationLevelEnum lvl = knob->getAnimationLevel(i);
        if (lvl != eAnimationLevelOnKeyframe) {
            hasAllKeyframesAtTime = false;
        } else if (dimension == i && lvl == eAnimationLevelOnKeyframe) {
            dimensionHasKeyframeAtTime = true;
        }
    }

    bool hasDimensionSlaved = false;
    bool hasAnimation = false;
    bool dimensionHasAnimation = false;
    bool isEnabled = true;
    bool dimensionIsSlaved = false;

    for (int i = 0; i < knob->getDimension(); ++i) {
        if ( knob->isSlave(i) ) {
            hasDimensionSlaved = true;
         
            if (i == dimension) {
                dimensionIsSlaved = true;
            }
        }
        if (knob->getKeyFramesCount(ViewIdx(0),i) > 0) {
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
    
    bool isAppKnob = knob->getHolder() && knob->getHolder()->getApp() != 0;
    if (!isAppKnob) {
        return;
    }
    if (knob->getDimension() > 1 && knob->isAnimationEnabled() && !hasDimensionSlaved) {
        ///Multi-dim actions
        if (!hasAllKeyframesAtTime) {
            QAction* setKeyAction = new QAction(tr("Set Key")+' '+tr("(all dimensions)"),menu);
            setKeyAction->setData(-1);
            QObject::connect( setKeyAction,SIGNAL( triggered() ),this,SLOT( onSetKeyActionTriggered() ) );
            menu->addAction(setKeyAction);
            if (!isEnabled) {
                setKeyAction->setEnabled(false);
            }
        } else {
            QAction* removeKeyAction = new QAction(tr("Remove Key")+' '+tr("(all dimensions)"),menu);
            removeKeyAction->setData(-1);
            QObject::connect( removeKeyAction,SIGNAL( triggered() ),this,SLOT( onRemoveKeyActionTriggered() ) );
            menu->addAction(removeKeyAction);
            if (!isEnabled) {
                removeKeyAction->setEnabled(false);
            }
        }
        
        if (hasAnimation) {
            QAction* removeAnyAnimationAction = new QAction(tr("Remove animation")+' '+tr("(all dimensions)"),menu);
            removeAnyAnimationAction->setData(-1);
            QObject::connect( removeAnyAnimationAction,SIGNAL( triggered() ),this,SLOT( onRemoveAnimationActionTriggered() ) );
            if (!isEnabled) {
                removeAnyAnimationAction->setEnabled(false);
            }
            menu->addAction(removeAnyAnimationAction);
        }
        
    }
    if ((dimension != -1 || knob->getDimension() == 1) && knob->isAnimationEnabled() && !dimensionIsSlaved) {
        if (!menu->isEmpty()) {
            menu->addSeparator();
        }
        {
            ///Single dim action
            if (!dimensionHasKeyframeAtTime) {
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
            
            if (dimensionHasAnimation) {
                QAction* removeAnyAnimationAction = new QAction(tr("Remove animation"),menu);
                removeAnyAnimationAction->setData(dimension);
                QObject::connect( removeAnyAnimationAction,SIGNAL( triggered() ),this,SLOT( onRemoveAnimationActionTriggered() ) );
                menu->addAction(removeAnyAnimationAction);
                if (!isEnabled) {
                    removeAnyAnimationAction->setEnabled(false);
                }
            }
            
        }
    }
    if (!menu->isEmpty()) {
        menu->addSeparator();
    }
    
    if (hasAnimation) {
        QAction* showInCurveEditorAction = new QAction(tr("Show in curve editor"),menu);
        QObject::connect( showInCurveEditorAction,SIGNAL( triggered() ),this,SLOT( onShowInCurveEditorActionTriggered() ) );
        menu->addAction(showInCurveEditorAction);
        if (!isEnabled) {
            showInCurveEditorAction->setEnabled(false);
        }
        
        if (knob->getDimension() > 1 && !hasDimensionSlaved) {
            (void)createInterpolationMenu(menu, -1, isEnabled);
        }
        if (dimensionHasAnimation && !dimensionIsSlaved) {
            if (dimension != -1 || knob->getDimension() == 1) {
                (void)createInterpolationMenu(menu, dimension != -1 ? dimension : 0, isEnabled);
            }
        }
    }
    
    
    
    {
        
        Menu* copyMenu = new Menu(menu);
        copyMenu->setTitle(tr("Copy"));
        if (hasAnimation) {
            
            QAction* copyAnimationAction = new QAction(tr("Copy Animation"),copyMenu);
            copyAnimationAction->setData(-1);
            QObject::connect( copyAnimationAction,SIGNAL( triggered() ),this,SLOT( onCopyAnimationActionTriggered() ) );
            copyMenu->addAction(copyAnimationAction);
            
        }
        
        
        QAction* copyValuesAction = new QAction(tr("Copy Value(s)"),copyMenu);
        copyValuesAction->setData( QVariant(-1) );
        copyMenu->addAction(copyValuesAction);
        QObject::connect( copyValuesAction,SIGNAL( triggered() ), this, SLOT( onCopyValuesActionTriggered() ) );
        
        
        
        QAction* copyLinkAction = new QAction(tr("Copy Link"),copyMenu);
        copyLinkAction->setData( QVariant(-1) );
        copyMenu->addAction(copyLinkAction);
        QObject::connect( copyLinkAction,SIGNAL( triggered() ), this, SLOT( onCopyLinksActionTriggered() ) );
        
        
        menu->addAction(copyMenu->menuAction());
    }
    ///If the clipboard is either empty or has no animation, disable the Paste animation action.
    KnobPtr fromKnob;
    KnobClipBoardType type;
    
    //cbDim is ignored for now
    int cbDim;
    appPTR->getKnobClipBoard(&type, &fromKnob, &cbDim);
    
    
    if (fromKnob && fromKnob != knob) {
        if (fromKnob->typeName() == knob->typeName()) {
            
            QString titlebase;
            if (type == eKnobClipBoardTypeCopyValue) {
                titlebase = tr("Paste Value");
            } else if (type == eKnobClipBoardTypeCopyAnim) {
                titlebase = tr("Paste Animation");
            } else if (type == eKnobClipBoardTypeCopyLink) {
                titlebase = tr("Paste Link");
            }
            
            bool ignorePaste = (!knob->isAnimationEnabled() && type == eKnobClipBoardTypeCopyAnim) ||
            ((dimension == -1 || cbDim == -1) && knob->getDimension() != fromKnob->getDimension()) ;
            if (!ignorePaste) {
                
                if (cbDim == -1 && fromKnob->getDimension() == knob->getDimension() && !hasDimensionSlaved) {
                    QString title = titlebase;
                    if (knob->getDimension() > 1) {
                        title += ' ';
                        title += tr("(all dimensions)");
                    }
                    QAction* pasteAction = new QAction(title,menu);
                    pasteAction->setData(-1);
                    QObject::connect( pasteAction,SIGNAL( triggered() ),this,SLOT( onPasteActionTriggered() ) );
                    menu->addAction(pasteAction);
                    if (!isEnabled) {
                        pasteAction->setEnabled(false);
                    }
                }
                
                if ((dimension != -1 || knob->getDimension() == 1) && !dimensionIsSlaved) {
                    QAction* pasteAction = new QAction(titlebase,menu);
                    pasteAction->setData(dimension != -1 ? dimension : 0);
                    QObject::connect( pasteAction,SIGNAL( triggered() ),this,SLOT( onPasteActionTriggered() ) );
                    menu->addAction(pasteAction);
                    if (!isEnabled) {
                        pasteAction->setEnabled(false);
                    }
                }
            }
        }
    }
    
    if (knob->getDimension() > 1 && !hasDimensionSlaved) {
        QAction* resetDefaultAction = new QAction(tr("Reset to default")+' '+tr("(all dimensions)"), _imp->copyRightClickMenu);
        resetDefaultAction->setData( QVariant(-1) );
        QObject::connect( resetDefaultAction,SIGNAL( triggered() ), this, SLOT( onResetDefaultValuesActionTriggered() ) );
        menu->addAction(resetDefaultAction);
        if (!isEnabled) {
            resetDefaultAction->setEnabled(false);
        }
    }
    if ((dimension != -1 || knob->getDimension() == 1) && !dimensionIsSlaved) {
        QAction* resetDefaultAction = new QAction(tr("Reset to default"), _imp->copyRightClickMenu);
        resetDefaultAction->setData(QVariant(dimension));
        QObject::connect( resetDefaultAction,SIGNAL( triggered() ), this, SLOT( onResetDefaultValuesActionTriggered() ) );
        menu->addAction(resetDefaultAction);
        if (!isEnabled) {
            resetDefaultAction->setEnabled(false);
        }
    }
    
    if (!menu->isEmpty()) {
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
    if (knob->getDimension() > 1 && !hasDimensionSlaved) {
        QAction* setExprsAction = new QAction((hasExpression ? tr("Edit expression") :
                                               tr("Set expression"))+' '+tr("(all dimensions)"),menu);
        setExprsAction->setData(-1);
        QObject::connect(setExprsAction,SIGNAL(triggered() ),this,SLOT(onSetExprActionTriggered()));
        if (!isEnabled) {
            setExprsAction->setEnabled(false);
        }
        menu->addAction(setExprsAction);
        
        if (hasExpression) {
            QAction* clearExprAction = new QAction(tr("Clear expression")+' '+tr("(all dimensions)"),menu);
            QObject::connect(clearExprAction,SIGNAL(triggered() ),this,SLOT(onClearExprActionTriggered()));
            clearExprAction->setData(-1);
            if (!isEnabled) {
                clearExprAction->setEnabled(false);
            }
            menu->addAction(clearExprAction);
        }
    }
    if ((dimension != -1 || knob->getDimension() == 1) && !dimensionIsSlaved) {
        
        
        QAction* setExprAction = new QAction(dimensionHasExpression ? tr("Edit expression...") : tr("Set expression..."),menu);
        QObject::connect(setExprAction,SIGNAL(triggered() ),this,SLOT(onSetExprActionTriggered()));
        setExprAction->setData(dimension);
        if (!isEnabled) {
            setExprAction->setEnabled(false);
        }
        menu->addAction(setExprAction);
        
        if (dimensionHasExpression) {
            QAction* clearExprAction = new QAction(tr("Clear expression"),menu);
            QObject::connect(clearExprAction,SIGNAL(triggered() ),this,SLOT(onClearExprActionTriggered()));
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
    
    boost::shared_ptr<NodeCollection> collec ;
    NodeGroup* isCollecGroup = 0;
    if (isEffect) {
        collec = isEffect->getNode()->getGroup();
        isCollecGroup = dynamic_cast<NodeGroup*>(collec.get());
    }
    
    
    if ((hasDimensionSlaved && dimension == -1) || dimensionIsSlaved) {
        menu->addSeparator();
        
        KnobPtr aliasMaster = knob->getAliasMaster();
        
        std::string knobName;
        if (dimension != -1 || knob->getDimension() == 1) {
            std::pair<int,KnobPtr > master = knob->getMaster(dimension);
            assert(master.second);
            
            assert(collec);
            NodesList nodes = collec->getNodes();
            if (isCollecGroup) {
                nodes.push_back(isCollecGroup->getNode());
            }
            for (NodesList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
                const std::vector< KnobPtr > & knobs = (*it)->getKnobs();
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
        KnobPtr listener = listeners.begin()->first.lock();
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
} // createAnimationMenu

KnobPtr
KnobGui::createDuplicateOnNode(EffectInstance* effect,
                               bool makeAlias,
                               const boost::shared_ptr<KnobPage>& page,
                               const boost::shared_ptr<KnobGroup>& group,
                               int indexInParent)
{
    ///find-out to which node that master knob belongs to
    assert( getKnob()->getHolder()->getApp() );
    KnobPtr knob = getKnob();
    
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
                    return KnobPtr();
                }
            }
        }
    }
    
    EffectInstance* isEffect = dynamic_cast<EffectInstance*>(knob->getHolder());
    if (!isEffect) {
        return KnobPtr();
    }
    const std::string& nodeScriptName = isEffect->getNode()->getScriptName();
    std::string newKnobName = nodeScriptName +  knob->getName();
    KnobPtr ret;
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
        return KnobPtr();
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
