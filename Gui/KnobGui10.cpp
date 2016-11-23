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

#include <QDebug>

#include "Engine/Knob.h"
#include "Engine/KnobTypes.h"
#include "Engine/Utils.h" // convertFromPlainText
#include "Engine/ViewIdx.h"
#include "Engine/Plugin.h"
#include "Engine/Project.h"
#include "Engine/KnobItemsTable.h"
#include "Engine/Node.h"
#include "Engine/StringAnimationManager.h"
#include "Engine/EffectInstance.h"

#include "Gui/ActionShortcuts.h"
#include "Gui/AnimationModuleUndoRedo.h"
#include "Gui/AnimationModuleEditor.h"
#include "Gui/AnimationModuleSelectionModel.h"
#include "Gui/AnimationModuleTreeView.h"
#include "Gui/AnimationModuleView.h"
#include "Gui/AnimationModule.h"
#include "Gui/KnobAnim.h"
#include "Gui/NodeAnim.h"
#include "Gui/TableItemAnim.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/KnobGuiPrivate.h"
#include "Gui/KnobUndoCommand.h" // SetExpressionCommand...
#include "Gui/TabGroup.h"

NATRON_NAMESPACE_ENTER;


void
KnobGui::onCreateAliasOnGroupActionTriggered()
{
    KnobHolderPtr holder = getKnob()->getHolder();
    EffectInstancePtr isEffect = toEffectInstance(holder);

    assert(isEffect);
    if (!isEffect) {
        throw std::logic_error("");
    }
    NodeCollectionPtr collec = isEffect->getNode()->getGroup();
    NodeGroupPtr isCollecGroup = toNodeGroup(collec);
    assert(isCollecGroup);
    if (isCollecGroup) {
        createDuplicateOnNode(isCollecGroup, true, KnobPagePtr(), KnobGroupPtr(), -1);
    }
}

void
KnobGui::onRemoveAliasLinkActionTriggered()
{
    KnobIPtr thisKnob = getKnob();
    KnobI::ListenerDimsMap listeners;

    thisKnob->getListeners(listeners);
    KnobIPtr aliasMaster;
    KnobIPtr listener;
    if ( !listeners.empty() ) {
        listener = listeners.begin()->first.lock();
        if (listener) {
            aliasMaster = listener->getAliasMaster();
        }
        if (aliasMaster != thisKnob) {
            aliasMaster.reset();
        }
    }
    if (aliasMaster && listener) {
        listener->setKnobAsAliasOfThis(aliasMaster, false);
    }
}

void
KnobGui::onUnlinkActionTriggered()
{
    QAction* action = qobject_cast<QAction*>( sender() );

    if (!action) {
        return;
    }
    ViewSetSpec view;
    DimSpec dimension;
    getDimViewFromActionData(action, &view, &dimension);

    KnobIPtr thisKnob = getKnob();
    int dims = thisKnob->getNDimensions();
    KnobIPtr aliasMaster = thisKnob->getAliasMaster();
    if (aliasMaster) {
        thisKnob->setKnobAsAliasOfThis(aliasMaster, false);
    } else {
        thisKnob->beginChanges();
        for (int i = 0; i < dims; ++i) {
            if ( dimension.isAll() || (i == DimIdx(dimension)) ) {
                thisKnob->unSlave(DimIdx(i), ViewSetSpec(view), true /*copyState*/);
            }
        }
        thisKnob->endChanges();
    }
    getKnob()->getHolder()->getApp()->triggerAutoSave();
}

void
KnobGui::onSetExprActionTriggered()
{
    QAction* action = qobject_cast<QAction*>( sender() );

    if (!action) {
        return;
    }
    ViewSetSpec view;
    DimSpec dimension;
    getDimViewFromActionData(action, &view, &dimension);

    EditExpressionDialog* dialog = new EditExpressionDialog(getGui(), dimension, view, shared_from_this(), _imp->container->getContainerWidget());
    dialog->create(QString::fromUtf8( getKnob()->getExpression(dimension.isAll() ? DimIdx(0) : DimIdx(dimension), view.isAll() ? ViewIdx(0) : ViewIdx(view)).c_str() ), true);
    QObject::connect( dialog, SIGNAL(dialogFinished(bool)), this, SLOT(onEditExprDialogFinished(bool)) );

    dialog->show();
}

void
KnobGui::onClearExprActionTriggered()
{
    QAction* act = qobject_cast<QAction*>( sender() );
    if (!act) {
        return;
    }
    ViewSetSpec view;
    DimSpec dimension;
    getDimViewFromActionData(act, &view, &dimension);
    pushUndoCommand( new SetExpressionCommand(getKnob(), false, dimension, view, "") );
}

void
KnobGui::onEditExprDialogFinished(bool accepted)
{
    EditExpressionDialog* dialog = dynamic_cast<EditExpressionDialog*>( sender() );

    if (dialog) {

        if (accepted) {

            bool hasRetVar;
            QString expr = dialog->getExpression(&hasRetVar);
            std::string stdExpr = expr.toStdString();
            DimSpec dim = dialog->getDimension();
            ViewSetSpec view = dialog->getView();

            pushUndoCommand( new SetExpressionCommand(getKnob(), hasRetVar, dim, view, stdExpr) );

        }

        dialog->close();
    }
}

void
KnobGui::setSecret()
{

    KnobIPtr knob = getKnob();
    if (!knob)  {
        return;
    }


    bool showit = !isSecretRecursive();

    if (showit) {
        show(); //
    } else {
        hide();
    }
    KnobHolderPtr holder = knob->getHolder();
    EffectInstancePtr isEffect = toEffectInstance(holder);
    if (isEffect) {
        NodePtr node = isEffect->getNode();
        if (node) {
            NodeGuiPtr nodeUi = boost::dynamic_pointer_cast<NodeGui>(node->getNodeGui());
            if (nodeUi) {
                nodeUi->onKnobSecretChanged(knob, knob->getIsSecret());
            }
        }
    }


    boost::shared_ptr<KnobGuiGroup> isGrp =  boost::dynamic_pointer_cast<KnobGuiGroup>(getWidgetsForView(ViewIdx(0)));
    if (isGrp) {
        const std::list<KnobGuiWPtr>& children = isGrp->getChildren();
        for (std::list<KnobGuiWPtr>::const_iterator it = children.begin(); it != children.end(); ++it) {
            KnobGuiPtr k = it->lock();
            if (!k) {
                continue;
            }
            k->setSecret();
        }
    }

}

void
KnobGui::onViewerContextSecretChanged()
{
    setSecret();
}

bool
KnobGui::isSecretRecursive() const
{
    // If the Knob is within a group, only show it if the group is unfolded!
    // To test it:
    // try TuttlePinning: fold all groups, then switch from perspective to affine to perspective.
    //  VISIBILITY is different from SECRETNESS. The code considers that both things are equivalent, which is wrong.
    // Of course, this check has to be *recursive* (in case the group is within a folded group)
    KnobIPtr knob = getKnob();
    bool isViewerKnob = _imp->container->isInViewerUIKnob();
    bool showit = isViewerKnob ? !knob->getInViewerContextSecret() : !knob->getIsSecret();
    KnobIPtr parentKnob = knob->getParentKnob();
    KnobGroupPtr parentIsGroup = toKnobGroup(parentKnob);

    while (showit && parentKnob && parentIsGroup) {
        KnobGuiPtr parentKnobGui = _imp->container->getKnobGui(parentKnob);
        if (!parentKnobGui) {
            break;
        }
        boost::shared_ptr<KnobGuiGroup> isGrp =  boost::dynamic_pointer_cast<KnobGuiGroup>(parentKnobGui->getWidgetsForView(ViewIdx(0)));
        // check for secretness and visibility of the group
        bool parentSecret = isViewerKnob ? parentKnob->getInViewerContextSecret() : parentKnob->getIsSecret();
        if ( parentSecret || ( isGrp && !isGrp->isChecked() ) ) {
            showit = false; // one of the including groups is folder, so this item is hidden
        }
        // prepare for next loop iteration
        parentKnob = parentKnob->getParentKnob();
        parentIsGroup = toKnobGroup(parentKnob);
    }

    return !showit;
}

void
KnobGui::onShowInCurveEditorActionTriggered()
{

    QAction* act = qobject_cast<QAction*>( sender() );
    if (!act) {
        return;
    }
    ViewSetSpec view;
    DimSpec dimension;
    getDimViewFromActionData(act, &view, &dimension);

    KnobIPtr knob = getKnob();
    assert( knob->getHolder()->getApp() );
    getGui()->setAnimationEditorOnTop();

    AnimationModulePtr model = getGui()->getAnimationModuleEditor()->getModel();
    if (!model) {
        return;
    }
    KnobAnimPtr knobAnim = findKnobAnim();
    if (!knobAnim) {
        return;
    }


    std::vector<CurveGuiPtr> curves;
    std::list<QTreeWidgetItem*> treeItems;
    int nDims = knob->getNDimensions();
    std::list<ViewIdx> views = knob->getViewsList();
    for (std::list<ViewIdx>::const_iterator it = views.begin(); it!=views.end(); ++it) {
        if (!view.isAll() && *it != view) {
            continue;
        }
        for (int i = 0; i < nDims; ++i) {
            if (!dimension.isAll() && dimension != i) {
                continue;
            }
            CurveGuiPtr curve = knobAnim->getCurveGui(DimIdx(i), *it);
            if ( curve && curve->getInternalCurve()->isAnimated() ) {
                curves.push_back(curve);
                QTreeWidgetItem* item = knobAnim->getTreeItem(DimIdx(i), *it);
                if (item) {
                    treeItems.push_back(item);
                }
            }

        }
    }

    if ( !curves.empty() ) {
        model->getEditor()->setOtherItemsVisibility(treeItems, false);
        getGui()->getAnimationModuleEditor()->setItemsVisibility(treeItems, true, true /*recurseOnParent*/);

        AnimItemDimViewKeyFramesMap keys;
        std::vector<TableItemAnimPtr> tableItems;
        std::vector<NodeAnimPtr> nodes;
        model->getSelectionModel()->makeSelection(keys, tableItems, nodes, (AnimationModuleSelectionModel::SelectionTypeAdd | AnimationModuleSelectionModel::SelectionTypeClear));
        getGui()->getAnimationModuleEditor()->getView()->centerOnCurves(curves, false);
    }
}

void
KnobGui::onRemoveAnimationActionTriggered()
{
    QAction* act = qobject_cast<QAction*>( sender() );
    if (!act) {
        return;
    }
    ViewSetSpec view;
    DimSpec dimension;
    getDimViewFromActionData(act, &view, &dimension);

    AnimationModulePtr model = getGui()->getAnimationModuleEditor()->getModel();
    if (!model) {
        return;
    }
    KnobIPtr internalKnob = getKnob();
    if (!internalKnob) {
        return;
    }
    KnobAnimPtr knobAnim = findKnobAnim();
    if (!knobAnim) {
        return;
    }

    AnimItemDimViewKeyFramesMap keysToRemove;

    int nDims = internalKnob->getNDimensions();
    std::list<ViewIdx> views = internalKnob->getViewsList();

    for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
        if (!view.isAll() && *it != view) {
            continue;
        }
        for (int i = 0; i < nDims; ++i) {
            if (!dimension.isAll() && dimension != i) {
                continue;
            }
            AnimItemDimViewIndexID id(knobAnim, *it, DimIdx(i));
            KeyFrameWithStringSet& keys = keysToRemove[id];
            knobAnim->getKeyframes(DimIdx(i), *it, AnimItemBase::eGetKeyframesTypeMerged, &keys);

        }
    }


    pushUndoCommand( new RemoveKeysCommand(keysToRemove, model) );
   
}

void
KnobGui::onInterpolationActionTriggered()
{
    QAction* action = qobject_cast<QAction*>(sender());
    if (!action) {
        return;
    }
    QList<QVariant> actionData = action->data().toList();
    if (actionData.size() != 3) {
        return;
    }
    QList<QVariant>::iterator it = actionData.begin();
    DimSpec dimension = DimSpec(it->toInt());
    ++it;
    ViewIdx view = ViewIdx(it->toInt());
    ++it;
    KeyframeTypeEnum interp = (KeyframeTypeEnum)it->toInt();

    AnimationModulePtr model = getGui()->getAnimationModuleEditor()->getModel();
    if (!model) {
        return;
    }
    KnobIPtr internalKnob = getKnob();
    if (!internalKnob) {
        return;
    }
    KnobAnimPtr knobAnim = findKnobAnim();
    if (!knobAnim) {
        return;
    }

    AnimItemDimViewKeyFramesMap keysToSet;

    int nDims = internalKnob->getNDimensions();

    for (int i = 0; i < nDims; ++i) {
        if (dimension.isAll() ||  dimension == i) {
            AnimItemDimViewIndexID id(knobAnim, view,  DimIdx(i));
            KeyFrameWithStringSet& keys = keysToSet[id];
            knobAnim->getKeyframes(DimIdx(i), view, AnimItemBase::eGetKeyframesTypeMerged, &keys);
        }
    }

    pushUndoCommand(new SetKeysInterpolationCommand(keysToSet, model, interp));
}

KnobAnimPtr
KnobGui::findKnobAnim() const
{

    AnimationModulePtr model = getGui()->getAnimationModuleEditor()->getModel();
    if (!model) {
        return KnobAnimPtr();
    }
    KnobIPtr internalKnob = getKnob();

    EffectInstancePtr isEffect = toEffectInstance(internalKnob->getHolder());
    KnobTableItemPtr isTableItem = toKnobTableItem(internalKnob->getHolder());
    if (isEffect) {
        NodeAnimPtr node = model->findNodeAnim(isEffect->getNode());
        if (!node) {
            return KnobAnimPtr();
        }
        return node->findKnobAnim(internalKnob);
    } else if (isTableItem) {
        TableItemAnimPtr item = model->findTableItemAnim(isTableItem);
        if (!item) {
            return KnobAnimPtr();
        }
        return item->findKnobAnim(internalKnob);
    }
    return KnobAnimPtr();
}

void
KnobGui::onSetKeyActionTriggered()
{
    QAction* act = qobject_cast<QAction*>( sender() );
    if (!act) {
        return;
    }
    ViewSetSpec view;
    DimSpec dimension;
    getDimViewFromActionData(act, &view, &dimension);

    AnimationModulePtr model = getGui()->getAnimationModuleEditor()->getModel();
    if (!model) {
        return;
    }
    KnobIPtr internalKnob = getKnob();
    if (!internalKnob) {
        return;
    }

    KnobAnimPtr knobAnim = findKnobAnim();
    if (!knobAnim) {
        return;
    }

    KnobIntBasePtr isInt = toKnobIntBase(internalKnob);
    KnobBoolBasePtr isBool = toKnobBoolBase(internalKnob);
    AnimatingKnobStringHelperPtr isString = boost::dynamic_pointer_cast<AnimatingKnobStringHelper>(internalKnob);
    KnobDoubleBasePtr isDouble = toKnobDoubleBase(internalKnob);


    double time = internalKnob->getCurrentTime();
    AnimItemDimViewKeyFramesMap keysToAdd;

    int nDims = internalKnob->getNDimensions();
    std::list<ViewIdx> views = internalKnob->getViewsList();


    for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
        if (!view.isAll() && *it != view) {
            continue;
        }
        for (int i = 0; i < nDims; ++i) {
            if (!dimension.isAll() && dimension != i) {
                continue;
            }
            AnimItemDimViewIndexID id(knobAnim, *it, DimIdx(i));
            KeyFrameWithStringSet& keys = keysToAdd[id];

            KeyFrameWithString kf;
            kf.key.setTime(time);
            if (isInt) {
                kf.key.setValue( isInt->getValue(DimIdx(i), *it) );
            } else if (isBool) {
                kf.key.setValue( isBool->getValue(DimIdx(i), *it) );
            } else if (isDouble) {
                kf.key.setValue( isDouble->getValue(DimIdx(i), *it) );
            } else if (isString) {
                std::string v = isString->getValue(DimIdx(i), *it);
                double dv;
                isString->stringToKeyFrameValue(time, ViewIdx(0), v, &dv);
                kf.string = v;
                kf.key.setValue(dv);
            }
            keys.insert(kf);
        }
    }

    if (!keysToAdd.empty()) {
        pushUndoCommand( new AddKeysCommand(keysToAdd, model, false/*replaceAnimation*/));
    }
}

void
KnobGui::onRemoveKeyActionTriggered()
{
    QAction* act = qobject_cast<QAction*>( sender() );
    if (!act) {
        return;
    }
    ViewSetSpec view;
    DimSpec dimension;
    getDimViewFromActionData(act, &view, &dimension);

    AnimationModulePtr model = getGui()->getAnimationModuleEditor()->getModel();
    if (!model) {
        return;
    }
    KnobIPtr internalKnob = getKnob();
    if (!internalKnob) {
        return;
    }
    KnobAnimPtr knobAnim = findKnobAnim();
    if (!knobAnim) {
        return;
    }

    AnimatingKnobStringHelperPtr isString = boost::dynamic_pointer_cast<AnimatingKnobStringHelper>(internalKnob);


    double time = internalKnob->getCurrentTime();
    AnimItemDimViewKeyFramesMap keysToRemove;

    int nDims = internalKnob->getNDimensions();

    std::list<ViewIdx> views = internalKnob->getViewsList();


    for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
        if (!view.isAll() && *it != view) {
            continue;
        }
        for (int i = 0; i < nDims; ++i) {
            if (!dimension.isAll() && dimension != i) {
                continue;
            }
            CurvePtr curve = knobAnim->getCurve(DimIdx(i), *it);
            if (!curve) {
                continue;
            }

            KeyFrameWithString kf;
            if (curve->getKeyFrameWithTime(time, &kf.key)) {
                if (isString) {
                    isString->getStringAnimation()->stringFromInterpolatedIndex(time, *it, &kf.string);
                }

                AnimItemDimViewIndexID id(knobAnim, *it, DimIdx(i));
                KeyFrameWithStringSet& keys = keysToRemove[id];
                keys.insert(kf);
            }

        }
    }

    if (!keysToRemove.empty()) {
        pushUndoCommand( new RemoveKeysCommand(keysToRemove, model));
    }
}

QString
KnobGui::getScriptNameHtml() const
{
    return QString::fromUtf8("<font size = 4><b>%1</b></font>").arg( QString::fromUtf8( getKnob()->getName().c_str() ) );
}

QString
KnobGui::toolTip(QWidget* w, ViewIdx view) const
{
    KnobIPtr knob = getKnob();
    KnobChoicePtr isChoice = toKnobChoice(knob);
    bool isMarkdown = knob.get()->isHintInMarkdown();
    QString tt;
    QString realTt;

    if (isMarkdown) {
        tt = QString::fromUtf8( knob.get()->getName().c_str() );
        tt.append( QString::fromUtf8("\n==========\n\n") );
    } else {
        tt = getScriptNameHtml();
    }

    if (!isChoice) {
        realTt.append( QString::fromUtf8( knob->getHintToolTip().c_str() ) );
    } else {
        realTt.append( QString::fromUtf8( isChoice->getHintToolTipFull().c_str() ) );
    }




    std::vector<std::string> expressions;
    bool exprAllSame = true;
    for (int i = 0; i < knob->getNDimensions(); ++i) {
        expressions.push_back( knob->getExpression(DimIdx(i), view) );
        if ( (i > 0) && (expressions[i] != expressions[0]) ) {
            exprAllSame = false;
        }
    }

    QString exprTt;
    if (exprAllSame) {
        if ( !expressions[0].empty() ) {
            if (isMarkdown) {
                exprTt = QString::fromUtf8("ret = **%1**\n\n").arg( QString::fromUtf8( expressions[0].c_str() ) );
            } else {
                exprTt = QString::fromUtf8("<br />ret = <b>%1</b>").arg( QString::fromUtf8( expressions[0].c_str() ) );
            }
        }
    } else {
        for (int i = 0; i < knob->getNDimensions(); ++i) {
            std::string dimName = knob->getDimensionName(DimIdx(i));
            QString toAppend;
            if (isMarkdown) {
                toAppend = QString::fromUtf8("%1 = **%2**\n\n").arg( QString::fromUtf8( dimName.c_str() ) ).arg( QString::fromUtf8( expressions[i].c_str() ) );
            } else {
                toAppend = QString::fromUtf8("<br />%1 = <b>%2</b>").arg( QString::fromUtf8( dimName.c_str() ) ).arg( QString::fromUtf8( expressions[i].c_str() ) );
            }
            exprTt.append(toAppend);
        }
    }

    if ( !exprTt.isEmpty() ) {
        tt.append(exprTt);
    }

    if ( !realTt.isEmpty() ) {
        if (!isMarkdown) {
            realTt = NATRON_NAMESPACE::convertFromPlainText(realTt.trimmed(), NATRON_NAMESPACE::WhiteSpaceNormal);
        }
        tt.append(realTt);
    }

    if (isMarkdown) {
        tt = Markdown::convert2html(tt);
        // Shrink H1/H2 (Can't do it in qt stylesheet)
        tt.replace( QString::fromUtf8("<h1>"), QString::fromUtf8("<h1 style=\"font-size:large;\">") );
        tt.replace( QString::fromUtf8("<h2>"), QString::fromUtf8("<h2 style=\"font-size:large;\">") );
    }

    bool setToolTip = false;
    if (!tt.isEmpty() && getLayoutType() == KnobGui::eKnobLayoutTypeViewerUI) {
        std::list<std::string> additionalShortcuts = knob->getInViewerContextAdditionalShortcuts();
        EffectInstancePtr isEffect = toEffectInstance(knob->getHolder());
        if (isEffect) {
            QString additionalKeyboardShortcutString;
            QString thisActionID = QString::fromUtf8( knob->getName().c_str() );

            std::list<QKeySequence> keybinds;

            // Try with the shortcut group of the original plugin.
            QString pluginShortcutGroup = QString::fromUtf8(isEffect->getNode()->getOriginalPlugin()->getPluginShortcutGroup().c_str());
            keybinds = getKeybind( pluginShortcutGroup, thisActionID );
            /*
             // If this is a pyplug, try again with the pyplug group
             if (keybinds.empty()) {
                PluginPtr pyPlug = isEffect->getNode()->getPyPlugPlugin();
                if (pyPlug) {
                    pluginShortcutGroup = QString::fromUtf8(pyPlug->getPluginShortcutGroup().c_str());
                    keybinds = getKeybind( pluginShortcutGroup, thisActionID );
                }
            }*/

            if (keybinds.size() > 0) {
                if (!isMarkdown) {
                    additionalKeyboardShortcutString += QLatin1String("<br/>");
                } else {
                    additionalKeyboardShortcutString += QLatin1String("\n");
                }
                additionalKeyboardShortcutString += QString::fromUtf8("<b>Keyboard shortcut: %1</b>");
            }

            if (keybinds.empty()) {
                // add a fake %1 because additional shortcuts start at %2
                additionalKeyboardShortcutString.push_back(QString::fromUtf8("%1"));
            }
            tt += additionalKeyboardShortcutString;
            additionalShortcuts.push_front(thisActionID.toStdString());

            if (w) {
                QList<QAction*> actions = w->actions();
                for (QList<QAction*>::iterator it = actions.begin(); it != actions.end(); ++it) {
                    ToolTipActionShortcut* isToolTipAction = dynamic_cast<ToolTipActionShortcut*>(*it);
                    if (isToolTipAction) {
                        w->removeAction(isToolTipAction);
                        delete isToolTipAction;
                    }
                }
                w->addAction( new ToolTipActionShortcut(pluginShortcutGroup.toStdString(), additionalShortcuts, tt.toStdString(), w));
                setToolTip = true;
            }
            // Ok we set the tooltip with the formating args (%1, %2) on the widget, we can now replace them to make the return value
            for (std::list<std::string>::iterator it = additionalShortcuts.begin(); it!=additionalShortcuts.end(); ++it) {
                std::list<QKeySequence> keybinds = getKeybind( pluginShortcutGroup, QString::fromUtf8( it->c_str() ) );
                QString argValue;
                if (keybinds.size() > 0) {
                    argValue = keybinds.front().toString(QKeySequence::NativeText);
                }
                tt = tt.arg(argValue);
            }

        }
    }
    if (!setToolTip && w) {
        w->setToolTip(tt);
    }
    return tt;
} // KnobGui::toolTip

bool
KnobGui::hasToolTip() const
{
    //Always true now that we display the script name in the tooltip
    return true; //!getKnob()->getHintToolTip().empty();
}



int
KnobGui::getKnobsCountOnSameLine() const
{
    if (!_imp->otherKnobsInMainLayout.empty()) {
        return _imp->otherKnobsInMainLayout.size();
    } else {
        KnobGuiPtr firstKnobOnLine = _imp->firstKnobOnLine.lock();
        if (!firstKnobOnLine) {
            return 0;
        } else {
            return firstKnobOnLine->_imp->otherKnobsInMainLayout.size();
        }
    }
}

void
KnobGui::hide()
{
    if (_imp->otherKnobsInMainLayout.empty()) {
        _imp->mainContainer->hide();
    } else {
        // We cannot hide the mainContainer because it might contain other knobs on the same layout line. Instead we just hide this knob widgets
        if (_imp->viewsContainer) {
            _imp->viewsContainer->hide();
        }
    }
    if (_imp->labelContainer) {
        _imp->labelContainer->hide();
    }

    // Also hide the tabwidget if it has any
    if (_imp->tabGroup) {
        _imp->tabGroup->hide();
    }
} // KnobGui::hide

void
KnobGui::show()
{
    if (_imp->otherKnobsInMainLayout.empty()) {
        _imp->mainContainer->show();
    } else {
        // We cannot hide the mainContainer because it might contain other knobs on the same layout line. Instead we just hide this knob widgets
        if (_imp->viewsContainer) {
            _imp->viewsContainer->show();
        }
    }
    if (_imp->labelContainer) {
        _imp->labelContainer->show();
    }

    // Also show the tabwidget if it has any
    if (_imp->tabGroup) {
        _imp->tabGroup->show();
    }
}

void
KnobGui::onMultiViewUnfoldClicked(bool expanded)
{
    for (KnobGuiPrivate::PerViewWidgetsMap::const_iterator it = _imp->views.begin(); it != _imp->views.end(); ++it) {
        if (it->first == ViewIdx(0)) {
            continue;
        }
        it->second.field->setVisible(expanded);
    }
}

#if 0
int
KnobGui::getActualIndexInLayout() const
{
    QWidget* parent = isLabelOnSameColumn() ? _imp->labelContainer->parentWidget() : _imp->field->parentWidget();

    if (!parent) {
        return -1;
    }
    QGridLayout* layout = dynamic_cast<QGridLayout*>( parent->layout() );
    if (!layout) {
        return -1;
    }
    for (int i = 0; i < layout->rowCount(); ++i) {
        QLayoutItem* leftItem = layout->itemAtPosition(i, 0);
        QLayoutItem* rightItem = layout->itemAtPosition(i, 1);
        QWidget* widgets[2] = {0, 0};
        if (leftItem) {
            widgets[0] = leftItem->widget();
        }
        if (rightItem) {
            widgets[1] = rightItem->widget();
        }
        for (int j = 0; j  < 2; ++j) {
            if (widgets[j] == _imp->labelContainer ||
                widgets[j] == _imp->field) {
                return i;
            }
        }
    }

    return -1;
}
#endif

bool
KnobGui::isOnNewLine() const
{
    return _imp->isOnNewLine;
}

void
KnobGui::reflectKnobSelectionState(bool selected)
{
    if (_imp->customInteract) {
        return;
    }
    for (KnobGuiPrivate::PerViewWidgetsMap::const_iterator it = _imp->views.begin(); it != _imp->views.end(); ++it) {
        if (it->second.widgets) {
            it->second.widgets->reflectSelectionState(selected);
        }
    }
}

void
KnobGui::setViewEnabledInternal(const std::vector<bool>& perDimEnabled, ViewIdx view)
{
    if ( !getGui() ) {
        return;
    }
    if (!_imp->customInteract) {
        KnobGuiPrivate::PerViewWidgetsMap::iterator foundView = _imp->views.find(view);
        if (foundView != _imp->views.end()) {
            if (foundView->second.widgets) {
                foundView->second.widgets->setEnabled(perDimEnabled);
            }
        }

    }

}

void
KnobGui::setEnabledSlot()
{
    KnobIPtr knob = getKnob();

    bool labelEnabled = false;
    for (KnobGuiPrivate::PerViewWidgetsMap::const_iterator it = _imp->views.begin(); it != _imp->views.end(); ++it) {
        std::vector<bool> enabled;
        for (int i = 0; i < knob->getNDimensions(); ++i) {
            enabled.push_back(knob->isEnabled(DimIdx(i), it->first));
            if (enabled.back()) {
                // If all dim/view are disabled, draw label disabled
                labelEnabled = true;
            }
        }
        setViewEnabledInternal(enabled, it->first);
    }

    if (_imp->descriptionLabel) {
        _imp->descriptionLabel->setEnabled( labelEnabled );
    }
    if (_imp->tabGroup) {
        _imp->tabGroup->setEnabled(labelEnabled);
    }
}

void
KnobGui::onFrozenChanged(bool frozen)
{
    KnobIPtr knob = getKnob();
    KnobButtonPtr isBtn = toKnobButton(knob);

    if ( isBtn && !isBtn->isRenderButton() ) {
        return;
    }

    bool labelEnabled = false;
    for (KnobGuiPrivate::PerViewWidgetsMap::const_iterator it = _imp->views.begin(); it != _imp->views.end(); ++it) {
        std::vector<bool> enabled;
        for (int i = 0; i < knob->getNDimensions(); ++i) {
            if (frozen) {
                enabled.push_back(false);
            } else {
                enabled.push_back(knob->isEnabled(DimIdx(i), it->first));
            }
            if (enabled.back()) {
                // If all dim/view are disabled, draw label disabled
                labelEnabled = true;
            }
        }
        setViewEnabledInternal(enabled, it->first);
    }

    if (_imp->descriptionLabel) {
        _imp->descriptionLabel->setEnabled( labelEnabled );
    }
    if (_imp->tabGroup) {
        _imp->tabGroup->setEnabled(labelEnabled);
    }

}

bool
KnobGui::isGuiFrozenForPlayback() const
{
    return getGui() ? getGui()->isGUIFrozen() : false;
}


void
KnobGui::onSplitViewActionTriggered()
{
    QAction* action = dynamic_cast<QAction*>(sender());
    if (!action) {
        return;
    }
    ViewIdx view = ViewIdx(action->data().toInt());
    KnobIPtr internalKnob = getKnob();
    if (!internalKnob) {
        return;
    }
    internalKnob->splitView(view);
}

void
KnobGui::onUnSplitViewActionTriggered()
{
    QAction* action = dynamic_cast<QAction*>(sender());
    if (!action) {
        return;
    }
    ViewIdx view = ViewIdx(action->data().toInt());
    KnobIPtr internalKnob = getKnob();
    if (!internalKnob) {
        return;
    }
    internalKnob->unSplitView(view);
}

void
KnobGui::onInternalKnobAvailableViewsChanged()
{
    // Sync views to what's inside the knob
    KnobIPtr internalKnob = getKnob();
    if (!internalKnob) {
        return;
    }
    std::list<ViewIdx> views = internalKnob->getViewsList();

    // Remove un-needed views
    KnobGuiPrivate::PerViewWidgetsMap viewsToKeep;
    for (KnobGuiPrivate::PerViewWidgetsMap::iterator it = _imp->views.begin(); it!=_imp->views.end(); ++it) {
        std::list<ViewIdx>::iterator found = std::find(views.begin(), views.end(), it->first);
        if (found != views.end()) {
            viewsToKeep.insert(*it);
            continue;
        }
        _imp->removeViewGui(it);
    }

    _imp->views = viewsToKeep;

    // Add new views
    for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
        KnobGuiPrivate::PerViewWidgetsMap::iterator  found = _imp->views.find(*it);
        if (found == _imp->views.end()) {
            createViewContainers(*it);
            KnobGuiPrivate::PerViewWidgetsMap::iterator found = _imp->views.find(*it);
            assert(found != _imp->views.end());
            _imp->createViewWidgets(found);

            // This will reflect animation level and view
            for (int i = 0; i < internalKnob->getNDimensions(); ++i) {
                onExprChanged(DimIdx(i), *it);
            }

        }
    }

    // Refresh view label visibility
    for (KnobGuiPrivate::PerViewWidgetsMap::iterator it = _imp->views.begin(); it!=_imp->views.end(); ++it) {
        it->second.viewLabel->setVisible(_imp->views.size() > 1);
    }

    // Reflect modif state
    refreshModificationsStateNow();

    _imp->viewUnfoldArrow->setVisible(_imp->views.size() > 1);
}

void
KnobGui::onProjectViewsChanged()
{
    KnobIPtr knob = getKnob();
    if (!knob) {
        return;
    }
    if (!knob->getHolder()) {
        return;
    }
    assert(knob->getHolder() && knob->getHolder()->getApp());
    KnobHolderPtr holder = knob->getHolder();
    AppInstancePtr app;
    if (holder) {
        app = holder->getApp();
    }
    if (!app) {
        return;
    }

    const std::vector<std::string>& projectViewNames = app->getProject()->getProjectViewNames();
    for (KnobGuiPrivate::PerViewWidgetsMap::iterator it = _imp->views.begin(); it != _imp->views.end(); ++it) {
        QString viewLongName, viewShortName;
        if (it->first >= 0 && it->first < (int)projectViewNames.size()) {
            viewLongName = QString::fromUtf8(projectViewNames[it->first].c_str());
            if (!viewLongName.isEmpty()) {
                viewShortName.append(viewLongName[0]);
            } else {
                viewShortName += QString::fromUtf8("(*)");
            }
        } else {
            viewLongName = it->second.viewLabel->text();
            QString notFoundSuffix = QString::fromUtf8("(*)");
            if (!viewLongName.endsWith(notFoundSuffix)) {
                viewLongName.push_back(QLatin1Char(' '));
                viewLongName += notFoundSuffix;
            }
            if (!viewLongName.isEmpty()) {
                viewShortName.append(viewLongName[0]);
                viewShortName.push_back(QLatin1Char(' '));
            }
            viewShortName += QString::fromUtf8("(*)");

        }
        it->second.viewLongName = viewLongName;
        it->second.viewLabel->setText(viewShortName);
    }


    AnimationModulePtr model = getGui()->getAnimationModuleEditor()->getModel();
    if (!model) {
        return;
    }

    KnobAnimPtr knobAnim = findKnobAnim();
    if (!knobAnim) {
        return;
    }
    knobAnim->refreshViewLabels(projectViewNames);
}

NATRON_NAMESPACE_EXIT;
