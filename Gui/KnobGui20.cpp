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

#include "Gui/KnobGui.h"
#include "Gui/KnobGuiPrivate.h"

#include <cassert>

#include "Gui/KnobUndoCommand.h" // SetExpressionCommand...

NATRON_NAMESPACE_ENTER;

void
KnobGui::onInternalValueChanged(int dimension,
                                int reason)
{
    if (_imp->widgetCreated && (ValueChangedReasonEnum)reason != eValueChangedReasonUserEdited) {
        updateGuiInternal(dimension);
    }
}

void
KnobGui::updateCurveEditorKeyframes()
{
    Q_EMIT keyFrameSet();
}

void
KnobGui::onMultipleKeySet(const std::list<double>& keys,int /*dimension*/, int reason)
{
    
    if ((ValueChangedReasonEnum)reason != eValueChangedReasonUserEdited) {
        KnobPtr knob = getKnob();
        if ( !knob->getIsSecret() && knob->isDeclaredByPlugin()) {
            std::list<SequenceTime> intKeys;
            for (std::list<double>::const_iterator it = keys.begin() ; it != keys.end(); ++it) {
                intKeys.push_back(*it);
            }
            knob->getHolder()->getApp()->addMultipleKeyframeIndicatorsAdded(intKeys, true);
        }
    }
    
    updateCurveEditorKeyframes();

}

void
KnobGui::onInternalKeySet(double time,
                          int /*dimension*/,
                          int reason,
                          bool added )
{
    if ((ValueChangedReasonEnum)reason != eValueChangedReasonUserEdited) {
        if (added) {
            KnobPtr knob = getKnob();
            if ( !knob->getIsSecret() && knob->isDeclaredByPlugin()) {
                knob->getHolder()->getApp()->addKeyframeIndicator(time);
            }
        }

    }
    
    updateCurveEditorKeyframes();
}

void
KnobGui::onInternalKeyRemoved(double time,
                              int /*dimension*/,
                              int /*reason*/)
{
    KnobPtr knob = getKnob();
    if ( !knob->getIsSecret() && knob->isDeclaredByPlugin()) {
        knob->getHolder()->getApp()->removeKeyFrameIndicator(time);
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
    KnobPtr knob = getKnob();

    Knob<int>* isInt = dynamic_cast<Knob<int>*>( knob.get() );
    Knob<bool>* isBool = dynamic_cast<Knob<bool>*>( knob.get() );
    Knob<double>* isDouble = dynamic_cast<Knob<double>*>( knob.get() );
    Knob<std::string>* isString = dynamic_cast<Knob<std::string>*>( knob.get() );
    AnimatingKnobStringHelper* isAnimatingString = dynamic_cast<AnimatingKnobStringHelper*>( knob.get() );
    boost::shared_ptr<KnobParametric> isParametric = boost::dynamic_pointer_cast<KnobParametric>(knob);


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
        EffectInstance* isEffect = dynamic_cast<EffectInstance*>(holder);
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

    KnobPtr knob = getKnob();

    Knob<int>* isInt = dynamic_cast<Knob<int>*>( knob.get() );
    Knob<bool>* isBool = dynamic_cast<Knob<bool>*>( knob.get() );
    Knob<double>* isDouble = dynamic_cast<Knob<double>*>( knob.get() );
    Knob<std::string>* isString = dynamic_cast<Knob<std::string>*>( knob.get() );
    boost::shared_ptr<KnobParametric> isParametric = boost::dynamic_pointer_cast<KnobParametric>(knob);

    int i = 0;
    for (std::list<Variant>::iterator it = values.begin(); it != values.end(); ++it) {
        if (isInt) {
            if ( !it->canConvert(QVariant::Int) ) {
                QString err = tr("Cannot paste values from a parameter of type %1 to a parameter of type Integer").arg( it->typeName() );
                Dialogs::errorDialog( tr("Paste").toStdString(),err.toStdString() );

                return;
            }
        } else if (isBool) {
            if ( !it->canConvert(QVariant::Bool) ) {
                QString err = tr("Cannot paste values from a parameter of type %1 to a parameter of type Boolean").arg( it->typeName() );
                Dialogs::errorDialog( tr("Paste").toStdString(),err.toStdString() );

                return;
            }
        } else if (isDouble) {
            if ( !it->canConvert(QVariant::Double) ) {
                QString err = tr("Cannot paste values from a parameter of type %1 to a parameter of type Double").arg( it->typeName() );
                Dialogs::errorDialog( tr("Paste").toStdString(),err.toStdString() );

                return;
            }
        } else if (isString) {
            if ( !it->canConvert(QVariant::String) ) {
                QString err = tr("Cannot paste values from a parameter of type %1 to a parameter of type String").arg( it->typeName() );
                Dialogs::errorDialog( tr("Paste").toStdString(),err.toStdString() );

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
    
    KnobPtr thisKnob = getKnob();
    assert(thisKnob);
    EffectInstance* isEffect = dynamic_cast<EffectInstance*>(thisKnob->getHolder());
    if (!isEffect) {
        return;
    }
    
    
    for (int i = 0; i < thisKnob->getDimension(); ++i) {
        
        if (i == dimension || dimension == -1) {
            std::string expr = thisKnob->getExpression(dimension);
            if (!expr.empty()) {
                Dialogs::errorDialog(tr("Param Link").toStdString(),tr("This parameter already has an expression set, edit or clear it.").toStdString());
                return;
            }
        }
    }
    
    LinkToKnobDialog dialog( this,_imp->copyRightClickMenu->parentWidget() );

    if ( dialog.exec() ) {
        KnobPtr  otherKnob = dialog.getSelectedKnobs();
        if (otherKnob) {
            if ( !thisKnob->isTypeCompatible(otherKnob) ) {
                Dialogs::errorDialog( tr("Param Link").toStdString(), tr("Types incompatibles!").toStdString() );

                return;
            }
            
            

            for (int i = 0; i < thisKnob->getDimension(); ++i) {
                std::pair<int,KnobPtr > existingLink = thisKnob->getMaster(i);
                if (existingLink.second) {
                    std::string err( tr("Cannot link ").toStdString() );
                    err.append( thisKnob->getLabel() );
                    err.append( " \n " + tr("because the knob is already linked to ").toStdString() );
                    err.append( existingLink.second->getLabel() );
                    Dialogs::errorDialog(tr("Param Link").toStdString(), err);

                    return;
                }
                
            }
            
            EffectInstance* otherEffect = dynamic_cast<EffectInstance*>(otherKnob->getHolder());
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
    KnobPtr knob = getKnob();
    KnobButton* isBtn = dynamic_cast<KnobButton*>( knob.get() );
    KnobPage* isPage = dynamic_cast<KnobPage*>( knob.get() );
    KnobGroup* isGroup = dynamic_cast<KnobGroup*>( knob.get() );
    KnobSeparator* isSeparator = dynamic_cast<KnobSeparator*>( knob.get() );

    if (!isBtn && !isPage && !isGroup && !isSeparator) {
        std::list<KnobPtr > knobs;
        knobs.push_back(knob);
        pushUndoCommand( new RestoreDefaultsCommand(false, knobs) );
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
        if (getKnob()->isEnabled(i)) {
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
    KnobPtr knob = getKnob();

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
    KnobPtr knob = getKnob();
    if ( knob->getHolder() && knob->getHolder()->getApp() && !knob->getIsSecret() && knob->isDeclaredByPlugin()) {
        AppInstance* app = knob->getHolder()->getApp();
        assert(app);
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
            app->removeMultipleKeyframeIndicator(times,true);
        }
    }
}

void
KnobGui::setAllKeyframeMarkersOnTimeline(int dimension)
{
    KnobPtr knob = getKnob();
    AppInstance* app = knob->getHolder()->getApp();
    assert(app);
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
    app->addMultipleKeyframeIndicatorsAdded(times,true);
}

void
KnobGui::setKeyframeMarkerOnTimeline(double time)
{
    KnobPtr knob = getKnob();
    if (knob->isDeclaredByPlugin()) {
        knob->getHolder()->getApp()->addKeyframeIndicator(time);
    }
}

void
KnobGui::onKeyFrameMoved(int /*dimension*/,
                         double oldTime,
                         double newTime)
{
    KnobPtr knob = getKnob();

    if ( !knob->isAnimationEnabled() || !knob->canAnimate() ) {
        return;
    }
    if (knob->isDeclaredByPlugin()) {
        AppInstance* app = knob->getHolder()->getApp();
        assert(app);
        app->removeKeyFrameIndicator(oldTime);
        app->addKeyframeIndicator(newTime);
    }
}

void
KnobGui::onAnimationLevelChanged(int dim,int level)
{
    if (!_imp->customInteract) {
        //std::string expr = getKnob()->getExpression(dim);
        //reflectExpressionState(dim,!expr.empty());
        //if (expr.empty()) {
            reflectAnimationLevel(dim, (AnimationLevelEnum)level);
        //}
        
    }
}

void
KnobGui::onAppendParamEditChanged(int reason,
                                  const Variant & v,
                                  int dim,
                                  double time,
                                  bool createNewCommand,
                                  bool setKeyFrame)
{
    pushUndoCommand( new MultipleKnobEditsUndoCommand(this,(ValueChangedReasonEnum)reason, createNewCommand,setKeyFrame,v,dim,time) );
}

void
KnobGui::onFrozenChanged(bool frozen)
{
    KnobPtr knob = getKnob();
    KnobButton* isBtn = dynamic_cast<KnobButton*>(knob.get());
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
KnobGui::onRedrawGuiCurve(int reason, int /*dimension*/)
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
KnobGui::onExprChanged(int dimension)
{
    if (_imp->guiRemoved) {
        return;
    }
    KnobPtr knob = getKnob();
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
KnobGui::onHasModificationsChanged()
{
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
        KnobPtr knob = getKnob();
        std::string descriptionLabel;
        KnobString* isStringKnob = dynamic_cast<KnobString*>(knob.get());
        bool isLabelKnob = isStringKnob && isStringKnob->isLabel();
        if (isLabelKnob) {
            descriptionLabel = isStringKnob->getValue();
        } else {
            descriptionLabel = knob->getLabel();
        }
        _imp->descriptionLabel->setText_overload(QString(QString(descriptionLabel.c_str()) + ":"));
        onLabelChangedInternal();
    }
}

NATRON_NAMESPACE_EXIT;
