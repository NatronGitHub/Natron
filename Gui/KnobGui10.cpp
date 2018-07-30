/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
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

#include <QDebug>

#include "Engine/Knob.h"
#include "Engine/KnobTypes.h"
#include "Engine/Utils.h" // convertFromPlainText
#include "Engine/ViewIdx.h"

#include "Gui/KnobGuiPrivate.h"
#include "Gui/KnobUndoCommand.h" // SetExpressionCommand...

NATRON_NAMESPACE_ENTER


void
KnobGui::onCreateAliasOnGroupActionTriggered()
{
    KnobHolder* holder = getKnob()->getHolder();
    EffectInstance* isEffect = dynamic_cast<EffectInstance*>(holder);

    assert(isEffect);
    if (!isEffect) {
        throw std::logic_error("");
    }
    NodeCollectionPtr collec = isEffect->getNode()->getGroup();
    NodeGroup* isCollecGroup = dynamic_cast<NodeGroup*>( collec.get() );
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
    int dim = action->data().toInt();
    KnobIPtr thisKnob = getKnob();
    int dims = thisKnob->getDimension();
    KnobIPtr aliasMaster = thisKnob->getAliasMaster();
    if (aliasMaster) {
        thisKnob->setKnobAsAliasOfThis(aliasMaster, false);
    } else {
        thisKnob->beginChanges();
        for (int i = 0; i < dims; ++i) {
            if ( (dim == -1) || (i == dim) ) {
                std::pair<int, KnobIPtr> other = thisKnob->getMaster(i);
                thisKnob->onKnobUnSlaved(i);
                onKnobSlavedChanged(i, false);
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

    assert(action);

    int dim = action->data().toInt();
    EditExpressionDialog* dialog = new EditExpressionDialog(getGui(), dim, shared_from_this(), _imp->field);
    dialog->create(QString::fromUtf8( getKnob()->getExpression(dim == -1 ? 0 : dim).c_str() ), true);
    QObject::connect( dialog, SIGNAL(accepted()), this, SLOT(onEditExprDialogFinished()) );
    QObject::connect( dialog, SIGNAL(rejected()), this, SLOT(onEditExprDialogFinished()) );

    dialog->show();
}

void
KnobGui::onClearExprActionTriggered()
{
    QAction* act = qobject_cast<QAction*>( sender() );

    assert(act);
    int dim = act->data().toInt();
    pushUndoCommand( new SetExpressionCommand(getKnob(), false, dim, "") );
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
                pushUndoCommand( new SetExpressionCommand(getKnob(), hasRetVar, dim, stdExpr) );
            }
            break;
        }
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
    KnobGuiGroup* isGrp = dynamic_cast<KnobGuiGroup*>(this);
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
    KnobGroup* parentIsGroup = dynamic_cast<KnobGroup*>( parentKnob.get() );

    while (showit && parentKnob && parentIsGroup) {
        KnobGuiGroup* parentGui = dynamic_cast<KnobGuiGroup*>( _imp->container->getKnobGui(parentKnob).get() );
        // check for secretness and visibility of the group
        bool parentSecret = isViewerKnob ? parentKnob->getInViewerContextSecret() : parentKnob->getIsSecret();
        if ( parentSecret || ( parentGui && !parentGui->isChecked() ) ) {
            showit = false; // one of the including groups is folder, so this item is hidden
        }
        // prepare for next loop iteration
        parentKnob = parentKnob->getParentKnob();
        parentIsGroup = dynamic_cast<KnobGroup*>( parentKnob.get() );
    }

    return !showit;
}

void
KnobGui::onShowInCurveEditorActionTriggered()
{
    KnobIPtr knob = getKnob();

    assert( knob->getHolder()->getApp() );
    getGui()->setCurveEditorOnTop();
    std::vector<CurvePtr> curves;
    for (int i = 0; i < knob->getDimension(); ++i) {
        CurvePtr c = getCurve(ViewIdx(0), i);
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
    QAction* action = qobject_cast<QAction*>( sender() );

    assert(action);
    int dim = action->data().toInt();
    KnobIPtr knob = getKnob();
    std::map<CurveGuiPtr, std::vector<KeyFrame > > toRemove;
    KnobGuiPtr thisShared = shared_from_this();
    for (int i = 0; i < knob->getDimension(); ++i) {
        if ( (dim == -1) || (dim == i) ) {
            std::list<CurveGuiPtr> curves = getGui()->getCurveEditor()->findCurve(thisShared, i);
            for (std::list<CurveGuiPtr>::iterator it = curves.begin(); it != curves.end(); ++it) {
                KeyFrameSet keys = (*it)->getInternalCurve()->getKeyFrames_mt_safe();
                std::vector<KeyFrame > vect;
                for (KeyFrameSet::const_iterator it2 = keys.begin(); it2 != keys.end(); ++it2) {
                    vect.push_back(*it2);
                }
                toRemove.insert( std::make_pair(*it, vect) );
            }
        }
    }
    pushUndoCommand( new RemoveKeysCommand(getGui()->getCurveEditor()->getCurveWidget(),
                                           toRemove) );
    //refresh the gui so it doesn't indicate the parameter is animated anymore
    updateGUI(dim);
}

void
KnobGui::setInterpolationForDimensions(QAction* action,
                                       KeyframeTypeEnum interp)
{
    if (!action) {
        return;
    }
    int dimension = action->data().toInt();
    KnobIPtr knob = getKnob();


    for (int i = 0; i < knob->getDimension(); ++i) {
        if ( (dimension == -1) || (dimension == i) ) {
            CurvePtr c = knob->getCurve(ViewIdx(0), i);
            if (c) {
                int kfCount = c->getKeyFramesCount();
                for (int j = 0; j < kfCount; ++j) {
                    c->setKeyFrameInterpolation(interp, j);
                }
                CurvePtr guiCurve = getCurve(ViewIdx(0), i);
                if (guiCurve) {
                    guiCurve->clone(*c);
                }
            }
        }
    }
    if ( knob->getHolder() ) {
        knob->getHolder()->incrHashAndEvaluate(knob->getEvaluateOnChange(), false);
    }
    Q_EMIT keyInterpolationChanged();
}

void
KnobGui::onConstantInterpActionTriggered()
{
    setInterpolationForDimensions(qobject_cast<QAction*>( sender() ), eKeyframeTypeConstant);
}

void
KnobGui::onLinearInterpActionTriggered()
{
    setInterpolationForDimensions(qobject_cast<QAction*>( sender() ), eKeyframeTypeLinear);
}

void
KnobGui::onSmoothInterpActionTriggered()
{
    setInterpolationForDimensions(qobject_cast<QAction*>( sender() ), eKeyframeTypeSmooth);
}

void
KnobGui::onCatmullromInterpActionTriggered()
{
    setInterpolationForDimensions(qobject_cast<QAction*>( sender() ), eKeyframeTypeCatmullRom);
}

void
KnobGui::onCubicInterpActionTriggered()
{
    setInterpolationForDimensions(qobject_cast<QAction*>( sender() ), eKeyframeTypeCubic);
}

void
KnobGui::onHorizontalInterpActionTriggered()
{
    setInterpolationForDimensions(qobject_cast<QAction*>( sender() ), eKeyframeTypeHorizontal);
}

void
KnobGui::setKeyframe(double time,
                     int dimension,
                     ViewSpec view)
{
    KnobIPtr knob = getKnob();

    assert( knob->getHolder()->getApp() );

    bool keyAdded = knob->onKeyFrameSet(time, view, dimension);
    Q_EMIT keyFrameSet();

    if ( !knob->getIsSecret() && keyAdded && knob->isDeclaredByPlugin() ) {
        knob->getHolder()->getApp()->addKeyframeIndicator(time);
    }
}

void
KnobGui::setKeyframe(double time,
                     const KeyFrame& key,
                     int dimension,
                     ViewSpec view)
{
    KnobIPtr knob = getKnob();

    assert( knob->getHolder()->getApp() );

    bool keyAdded = knob->onKeyFrameSet(time, view, key, dimension);
    Q_EMIT keyFrameSet();
    if ( !knob->getIsSecret() && keyAdded && knob->isDeclaredByPlugin() ) {
        knob->getHolder()->getApp()->addKeyframeIndicator(time);
    }
}

void
KnobGui::onSetKeyActionTriggered()
{
    QAction* action = qobject_cast<QAction*>( sender() );

    assert(action);
    int dim = action->data().toInt();
    KnobIPtr knob = getKnob();

    assert( knob->getHolder()->getApp() );
    //get the current time on the global timeline
    SequenceTime time = knob->getHolder()->getApp()->getTimeLine()->currentFrame();
    AddKeysCommand::KeysToAddList toAdd;
    KnobGuiPtr thisShared = shared_from_this();
    for (int i = 0; i < knob->getDimension(); ++i) {
        if ( (dim == -1) || (i == dim) ) {
            std::list<CurveGuiPtr> curves = getGui()->getCurveEditor()->findCurve(thisShared, i);
            for (std::list<CurveGuiPtr>::iterator it = curves.begin(); it != curves.end(); ++it) {
                AddKeysCommand::KeyToAdd keyToAdd;
                KeyFrame kf;
                kf.setTime(time);
                KnobIntBase* isInt = dynamic_cast<KnobIntBase*>( knob.get() );
                KnobBoolBase* isBool = dynamic_cast<KnobBoolBase*>( knob.get() );
                AnimatingKnobStringHelper* isString = dynamic_cast<AnimatingKnobStringHelper*>( knob.get() );
                KnobDoubleBase* isDouble = dynamic_cast<KnobDoubleBase*>( knob.get() );

                if (isInt) {
                    kf.setValue( isInt->getValue(i) );
                } else if (isBool) {
                    kf.setValue( isBool->getValue(i) );
                } else if (isDouble) {
                    kf.setValue( isDouble->getValue(i) );
                } else if (isString) {
                    std::string v = isString->getValue(i);
                    double dv;
                    isString->stringToKeyFrameValue(time, ViewIdx(0), v, &dv);
                    kf.setValue(dv);
                }

                keyToAdd.keyframes.push_back(kf);
                keyToAdd.curveUI = *it;
                keyToAdd.knobUI = thisShared;
                keyToAdd.dimension = i;
                toAdd.push_back(keyToAdd);
            }
        }
    }
    pushUndoCommand( new AddKeysCommand(getGui()->getCurveEditor()->getCurveWidget(), toAdd) );
}

void
KnobGui::removeKeyFrame(double time,
                        int dimension,
                        ViewSpec view)
{
    KnobIPtr knob = getKnob();

    CurvePtr curve = knob->getCurve(view, dimension);
    bool copyCurveValueAtTimeToInternalValue = false;
    if (curve) {
        copyCurveValueAtTimeToInternalValue = curve->getKeyFramesCount() == 1;
    }
    knob->onKeyFrameRemoved(time, view, dimension, copyCurveValueAtTimeToInternalValue);
    Q_EMIT keyFrameRemoved();


    assert( knob->getHolder()->getApp() );
    if ( !knob->getIsSecret() ) {
        knob->getHolder()->getApp()->removeKeyFrameIndicator(time);
    }
    updateGUI(dimension);
}

void
KnobGui::setKeyframes(const std::vector<KeyFrame>& keys,
                      int dimension,
                      ViewSpec view)
{
    KnobIPtr knob = getKnob();

    assert( knob->getHolder()->getApp() );
    knob->beginChanges();
    std::list<SequenceTime> times;
    for (std::size_t i = 0; i < keys.size(); ++i) {
        bool keyAdded = knob->onKeyFrameSet(keys[i].getTime(), view, keys[i], dimension);
        if (keyAdded) {
            times.push_back( keys[i].getTime() );
        }
    }
    knob->endChanges();
    Q_EMIT keyFrameSet();
    if ( !knob->getIsSecret() && knob->isDeclaredByPlugin() ) {
        knob->getHolder()->getApp()->addMultipleKeyframeIndicatorsAdded(times, true);
    }
}

void
KnobGui::removeKeyframes(const std::vector<KeyFrame>& keys,
                         int dimension,
                         ViewSpec view)
{
    KnobIPtr knob = getKnob();

    knob->beginChanges();
    for (std::size_t i = 0; i < keys.size(); ++i) {
        knob->onKeyFrameRemoved(keys[i].getTime(), view, dimension, i == 0);
    }
    knob->endChanges();
    /*assert( knob->getHolder()->getApp() );
       if ( !knob->getIsSecret() ) {
        std::list<SequenceTime> times;
        for (std::size_t i = 0; i < keys.size(); ++i) {
            times.push_back(keys[i].getTime());
        }
        knob->getHolder()->getApp()->getTimeLine()->removeMultipleKeyframeIndicator(times, true);
       }*/

    Q_EMIT keyFrameRemoved();
    updateGUI(dimension);
}

QString
KnobGui::getScriptNameHtml() const
{
    return QString::fromUtf8("<font size = 4><b>%1</b></font>").arg( QString::fromUtf8( getKnob()->getName().c_str() ) );
}

QString
KnobGui::toolTip() const
{
    KnobIPtr knob = getKnob();
    KnobChoice* isChoice = dynamic_cast<KnobChoice*>( knob.get() );
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
    for (int i = 0; i < knob->getDimension(); ++i) {
        expressions.push_back( knob->getExpression(i) );
        if ( (i > 0) && (expressions[i] != expressions[0]) ) {
            exprAllSame = false;
        }
    }

    QString exprTt;
    if (exprAllSame) {
        if ( !expressions[0].empty() ) {
            if (isMarkdown) {
                exprTt = QString::fromUtf8("ret = **%1**\n\n").arg( QString::fromUtf8( expressions[0].c_str() ).trimmed() );
            } else {
                exprTt = QString::fromUtf8("<br />ret = <b>%1</b>").arg( QString::fromUtf8( expressions[0].c_str() ) );
            }
        }
    } else {
        for (int i = 0; i < knob->getDimension(); ++i) {
            std::string dimName = knob->getDimensionName(i);
            QString toAppend;
            if (isMarkdown) {
                toAppend = QString::fromUtf8("%1 = **%2**\n\n").arg( QString::fromUtf8( dimName.c_str() ) ).arg( QString::fromUtf8( expressions[i].c_str() ).trimmed() );
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

    return tt;
} // KnobGui::toolTip

bool
KnobGui::hasToolTip() const
{
    //Always true now that we display the script name in the tooltip
    return true; //!getKnob()->getHintToolTip().empty();
}

void
KnobGui::onRemoveKeyActionTriggered()
{
    QAction* action = qobject_cast<QAction*>( sender() );

    assert(action);
    int dim = action->data().toInt();
    KnobIPtr knob = getKnob();

    assert( knob->getHolder()->getApp() );
    //get the current time on the global timeline
    SequenceTime time = knob->getHolder()->getApp()->getTimeLine()->currentFrame();
    std::map<CurveGuiPtr, std::vector<KeyFrame > > toRemove;
    KnobGuiPtr thisShared = shared_from_this();

    for (int i = 0; i < knob->getDimension(); ++i) {
        if ( (dim == -1) || (i == dim) ) {
            std::list<CurveGuiPtr> curves = getGui()->getCurveEditor()->findCurve(thisShared, i);
            for (std::list<CurveGuiPtr>::iterator it = curves.begin(); it != curves.end(); ++it) {
                KeyFrame kf;
                bool foundKey = knob->getCurve(ViewIdx(0), i)->getKeyFrameWithTime(time, &kf);

                if (foundKey) {
                    std::vector<KeyFrame > vect;
                    vect.push_back(kf);
                    toRemove.insert( std::make_pair(*it, vect) );
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

    //also  hide the curve from the curve editor if there's any and the knob is not inside a group
    KnobIPtr knob = getKnob();
    if ( knob && knob->getHolder() && knob->getHolder()->getApp() ) {
        KnobIPtr parent = getKnob()->getParentKnob();
        bool isSecret = true;
        while (parent) {
            if ( !parent->getIsSecret() ) {
                isSecret = false;
                break;
            }
            parent = parent->getParentKnob();
        }
        if (isSecret) {
            getGui()->getCurveEditor()->hideCurves( shared_from_this() );
        }
    }

    ////In order to remove the row of the layout we have to make sure ALL the knobs on the row
    ////are hidden.
    bool shouldRemoveWidget = true;
    for (U32 i = 0; i < _imp->knobsOnSameLine.size(); ++i) {
        KnobGuiPtr sibling = _imp->container->getKnobGui( _imp->knobsOnSameLine[i].lock() );
        if ( sibling && !sibling->isSecretRecursive() ) {
            shouldRemoveWidget = false;
        }
    }

    if (shouldRemoveWidget) {
        if (_imp->field) {
            _imp->field->hide();
        }
        if (_imp->container) {
            _imp->container->refreshTabWidgetMaxHeight();
        }
    } else {
        if ( _imp->field && !_imp->field->isVisible() ) {
            _imp->field->show();
        }
    }
    if (_imp->labelContainer) {
        _imp->labelContainer->hide();
    } else if (_imp->descriptionLabel) {
        _imp->descriptionLabel->hide();
    }
} // KnobGui::hide

void
KnobGui::show(int /*index*/)
{
    if ( !getGui() ) {
        return;
    }
    if (!_imp->customInteract) {
        _show();
    } else {
        _imp->customInteract->show();
    }

    //also show the curve from the curve editor if there's any
    KnobIPtr knob = getKnob();
    if ( knob && knob->getHolder() && knob->getHolder()->getApp() ) {
        getGui()->getCurveEditor()->showCurves( shared_from_this() );
    }

    //if (_imp->isOnNewLine) {
    if (_imp->field) {
        _imp->field->show();
    }
    if (_imp->container) {
        _imp->container->refreshTabWidgetMaxHeight();
    }
    //}

    if (_imp->labelContainer) {
        _imp->labelContainer->show();
    } else if (_imp->descriptionLabel) {
        _imp->descriptionLabel->show();
    }
}

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

bool
KnobGui::isOnNewLine() const
{
    return _imp->isOnNewLine;
}

void
KnobGui::setEnabledSlot()
{
    if ( !getGui() ) {
        return;
    }
    if (!_imp->customInteract) {
        setEnabled();
    }
    KnobIPtr knob = getKnob();
    if (_imp->descriptionLabel) {
        _imp->descriptionLabel->setReadOnly( !knob->isEnabled(0) );
    }
    KnobGuiPtr thisShared = shared_from_this();
    if ( knob->getHolder() && knob->getHolder()->getApp() ) {
        for (int i = 0; i < knob->getDimension(); ++i) {
            if ( !knob->isEnabled(i) ) {
                getGui()->getCurveEditor()->hideCurve(thisShared, i);
            } else {
                getGui()->getCurveEditor()->showCurve(thisShared, i);
            }
        }
    }
}

QWidget*
KnobGui::getFieldContainer() const
{
    return _imp->field;
}

NATRON_NAMESPACE_EXIT
