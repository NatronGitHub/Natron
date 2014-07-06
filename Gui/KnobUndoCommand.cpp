//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "KnobUndoCommand.h"

#include "Engine/KnobTypes.h"
#include "Engine/KnobFile.h"
#include "Engine/Node.h"
#include "Gui/GuiApplicationManager.h"

PasteUndoCommand::PasteUndoCommand(KnobGui* knob,int targetDimension,
                                   int dimensionToFetch,
                                   bool copyAnimation,
                                   const std::list<Variant>& values,
                                   const std::list<boost::shared_ptr<Curve> >& curves,
                                   const std::list<boost::shared_ptr<Curve> >& parametricCurves,
                                   const std::map<int,std::string>& stringAnimation)
: QUndoCommand(0)
, _knob(knob)
, newValues(values)
, oldValues()
, newCurves(curves)
, oldCurves()
, newParametricCurves(parametricCurves)
, oldParametricCurves()
, newStringAnimation(stringAnimation)
, oldStringAnimation()
, _targetDimension(targetDimension)
, _dimensionToFetch(dimensionToFetch)
, _copyAnimation(copyAnimation)
{
    assert(!appPTR->isClipBoardEmpty());
    assert((!copyAnimation && newCurves.empty()) || copyAnimation);
    
    boost::shared_ptr<KnobI> internalKnob = knob->getKnob();
    Knob<int>* isInt = dynamic_cast<Knob<int>*>(internalKnob.get());
    Knob<bool>* isBool = dynamic_cast<Knob<bool>*>(internalKnob.get());
    Knob<double>* isDouble = dynamic_cast<Knob<double>*>(internalKnob.get());
    Knob<std::string>* isString = dynamic_cast<Knob<std::string>*>(internalKnob.get());
    AnimatingString_KnobHelper* isAnimatingString = dynamic_cast<AnimatingString_KnobHelper*>(internalKnob.get());
    boost::shared_ptr<Parametric_Knob> isParametric = boost::dynamic_pointer_cast<Parametric_Knob>(internalKnob);

    
    for (int i = 0; i < internalKnob->getDimension(); ++i) {
        if (isInt) {
            oldValues.push_back(Variant(isInt->getValue(i)));
        } else if (isBool) {
            oldValues.push_back(Variant(isBool->getValue(i)));
        } else if (isDouble) {
            oldValues.push_back(Variant(isDouble->getValue(i)));
        } else if (isString) {
            oldValues.push_back(Variant(isString->getValue(i).c_str()));
        }
        boost::shared_ptr<Curve> c(new Curve);
        c->clone(*internalKnob->getCurve(i));
        oldCurves.push_back(c);
    }
    
    if (isAnimatingString) {
        isAnimatingString->saveAnimation(&oldStringAnimation);
    }
    
    if (isParametric) {
        std::list< Curve > tmpCurves;
        isParametric->saveParametricCurves(&tmpCurves);
        for (std::list< Curve >::iterator it = tmpCurves.begin(); it!=tmpCurves.end(); ++it) {
            boost::shared_ptr<Curve> c(new Curve);
            c->clone(*it);
            oldParametricCurves.push_back(c);
        }
    }

    
}

void PasteUndoCommand::undo()
{
    boost::shared_ptr<KnobI> internalKnob = _knob->getKnob();
    Knob<int>* isInt = dynamic_cast<Knob<int>*>(internalKnob.get());
    Knob<bool>* isBool = dynamic_cast<Knob<bool>*>(internalKnob.get());
    Knob<double>* isDouble = dynamic_cast<Knob<double>*>(internalKnob.get());
    Knob<std::string>* isString = dynamic_cast<Knob<std::string>*>(internalKnob.get());
    AnimatingString_KnobHelper* isAnimatingString = dynamic_cast<AnimatingString_KnobHelper*>(internalKnob.get());
    boost::shared_ptr<Parametric_Knob> isParametric = boost::dynamic_pointer_cast<Parametric_Knob>(internalKnob);
    
    internalKnob->beginValueChange(Natron::PLUGIN_EDITED);
    
    int i = 0;
    std::list<Variant>::iterator next = oldValues.begin();
    ++next;
    for (std::list<Variant>::iterator it = oldValues.begin(); it!=oldValues.end();++it,++next) {
        if ((i == _targetDimension && !_copyAnimation) || _copyAnimation) {
            bool triggerOnKnobChanged = next == oldValues.end();
            if (isInt) {
                isInt->setValue(it->toInt(), i,false,triggerOnKnobChanged);
            } else if (isBool) {
                isBool->setValue(it->toBool(), i,false,triggerOnKnobChanged);
            } else if (isDouble) {
                isDouble->setValue(it->toDouble(), i,false,triggerOnKnobChanged);
            } else if (isString) {
                isString->setValue(it->toString().toStdString(), i,false,triggerOnKnobChanged);
            }
        }
        ++i;
    }
    if (_copyAnimation) {
        _knob->removeAllKeyframeMarkersOnTimeline(-1);
        i = 0;
        for (std::list<boost::shared_ptr<Curve> >::iterator it = oldCurves.begin(); it!=oldCurves.end(); ++it) {
            internalKnob->getCurve(i)->clone(*(*it));
            ++i;
        }
        ///parameters are meaningless here, we just want to update the curve editor.
        _knob->onInternalKeySet(0, 0);
        _knob->setAllKeyframeMarkersOnTimeline(-1);
    }
    
    if (isAnimatingString) {
        isAnimatingString->loadAnimation(oldStringAnimation);
    }
    
    if (isParametric) {
        std::list<Curve> tmpCurves;
        for(std::list<boost::shared_ptr<Curve> >::iterator it = oldParametricCurves.begin();it!=oldParametricCurves.end();++it) {
            Curve c;
            c.clone(*(*it));
            tmpCurves.push_back(c);
        }
        isParametric->loadParametricCurves(tmpCurves);
    }
    
    internalKnob->endValueChange();

    if (!_copyAnimation) {
        setText(QObject::tr("Paste value of %1")
                .arg(_knob->getKnob()->getDescription().c_str()));
    } else {
        setText(QObject::tr("Paste animation of %1")
                .arg(_knob->getKnob()->getDescription().c_str()));
    }

}

void PasteUndoCommand::redo()
{
    
    
    boost::shared_ptr<KnobI> internalKnob = _knob->getKnob();
    Knob<int>* isInt = dynamic_cast<Knob<int>*>(internalKnob.get());
    Knob<bool>* isBool = dynamic_cast<Knob<bool>*>(internalKnob.get());
    Knob<double>* isDouble = dynamic_cast<Knob<double>*>(internalKnob.get());
    Knob<std::string>* isString = dynamic_cast<Knob<std::string>*>(internalKnob.get());
    AnimatingString_KnobHelper* isAnimatingString = dynamic_cast<AnimatingString_KnobHelper*>(internalKnob.get());
    boost::shared_ptr<Parametric_Knob> isParametric = boost::dynamic_pointer_cast<Parametric_Knob>(internalKnob);
    
    internalKnob->beginValueChange(Natron::PLUGIN_EDITED);
    
    int i = 0;
    std::list<Variant>::iterator next = newValues.begin();
    ++next;
    for (std::list<Variant>::iterator it = newValues.begin(); it!=newValues.end();++it,++next) {
        if ((i == _dimensionToFetch && !_copyAnimation) || _copyAnimation) {
            bool triggerOnKnobChanged = next == newValues.end();
            if (isInt) {
                isInt->setValue(it->toInt(), (i == _dimensionToFetch && !_copyAnimation) ? _targetDimension : i,false,
                                triggerOnKnobChanged);
            } else if (isBool) {
                isBool->setValue(it->toBool(),  (i == _dimensionToFetch && !_copyAnimation) ? _targetDimension : i,false,
                                 triggerOnKnobChanged);
            } else if (isDouble) {
                isDouble->setValue(it->toDouble(),  (i == _dimensionToFetch && !_copyAnimation) ? _targetDimension : i,false,
                                   triggerOnKnobChanged);
            } else if (isString) {
                isString->setValue(it->toString().toStdString(),  (i == _dimensionToFetch && !_copyAnimation) ? _targetDimension : i,false,triggerOnKnobChanged);
            }
        }
        ++i;
    }
    if (!newCurves.empty()) {
        _knob->removeAllKeyframeMarkersOnTimeline(-1);
    }
    i = 0;
    bool hasKeyframeData = false;
    for (std::list<boost::shared_ptr<Curve> >::iterator it = newCurves.begin(); it!=newCurves.end(); ++it) {
        internalKnob->getCurve(i)->clone(*(*it));
        if ((*it)->getKeyFramesCount() > 0) {
            hasKeyframeData = true;
        }
        ++i;
    }
    if (!newCurves.empty()) {
        _knob->setAllKeyframeMarkersOnTimeline(-1);
    }
    if (_copyAnimation && hasKeyframeData && !newCurves.empty()) {
        ///parameters are meaningless here, we just want to update the curve editor.
        _knob->onInternalKeySet(0, 0);
    }
    
    if (isAnimatingString) {
        isAnimatingString->loadAnimation(newStringAnimation);
    }
    
    if (isParametric) {
        std::list<Curve> tmpCurves;
        for(std::list<boost::shared_ptr<Curve> >::iterator it = newParametricCurves.begin();it!=newParametricCurves.end();++it) {
            Curve c;
            c.clone(*(*it));
            tmpCurves.push_back(c);
        }
        isParametric->loadParametricCurves(tmpCurves);
    }
    
    internalKnob->endValueChange();
    
    if (!_copyAnimation) {
        setText(QObject::tr("Paste value of %1")
            .arg(_knob->getKnob()->getDescription().c_str()));
    } else {
        setText(QObject::tr("Paste animation of %1")
                .arg(_knob->getKnob()->getDescription().c_str()));
    }
    
}


MultipleKnobEditsUndoCommand::MultipleKnobEditsUndoCommand(KnobGui* knob,bool createNew,bool setKeyFrame,
                                                           bool triggerOnKnobChanged,
                                                           const std::list<Variant>& values,int time)
: QUndoCommand()
, knobs()
, createNew(createNew)
, firstRedoCalled(false)
, triggerOnKnobChanged(triggerOnKnobChanged)
{
    assert(knob);
    boost::shared_ptr<KnobI> originalKnob = knob->getKnob();
    boost::shared_ptr<KnobI> copy = createCopyForKnob(originalKnob);
    
    ValueToSet v;
    v.newValues = values;
    v.time = time;
    v.copy = copy;
    v.setKeyFrame = setKeyFrame;
    knobs.insert(std::make_pair(knob, v));

}

MultipleKnobEditsUndoCommand::~MultipleKnobEditsUndoCommand()
{
    
}

boost::shared_ptr<KnobI> MultipleKnobEditsUndoCommand::createCopyForKnob(const boost::shared_ptr<KnobI>& originalKnob) const
{
    const std::string& typeName = originalKnob->typeName();
    boost::shared_ptr<KnobI> copy;
    int dimension = originalKnob->getDimension();
    if (typeName == Int_Knob::typeNameStatic()) {
        copy.reset(new Int_Knob(NULL,"",dimension,false));
    } else if (typeName == Bool_Knob::typeNameStatic()) {
        copy.reset(new Bool_Knob(NULL,"",dimension,false));
    } else if (typeName == Double_Knob::typeNameStatic()) {
        copy.reset(new Double_Knob(NULL,"",dimension,false));
    } else if (typeName == Choice_Knob::typeNameStatic()) {
        copy.reset(new Choice_Knob(NULL,"",dimension,false));
    } else if (typeName == String_Knob::typeNameStatic()) {
        copy.reset(new String_Knob(NULL,"",dimension,false));
    } else if (typeName == Parametric_Knob::typeNameStatic()) {
        copy.reset(new Parametric_Knob(NULL,"",dimension,false));
    } else if (typeName == Color_Knob::typeNameStatic()) {
        copy.reset(new Color_Knob(NULL,"",dimension,false));
    } else if (typeName == Path_Knob::typeNameStatic()) {
        copy.reset(new Path_Knob(NULL,"",dimension,false));
    } else if (typeName == File_Knob::typeNameStatic()) {
        copy.reset(new File_Knob(NULL,"",dimension,false));
    } else if (typeName == OutputFile_Knob::typeNameStatic()) {
        copy.reset(new OutputFile_Knob(NULL,"",dimension,false));
    }
    
    ///If this is another type of knob this is wrong since they do not hold any value
    assert(copy);
    copy->populate();
    
    ///make a clone of the original knob at that time and stash it
    copy->clone(originalKnob);
    return copy;
}

void MultipleKnobEditsUndoCommand::undo()
{
    
    
    ///keep track of all different knobs and call instance changed action only once for all of them
    std::set <KnobI*> knobsUnique;
    for (ParamsMap::iterator it = knobs.begin(); it!= knobs.end(); ++it) {
        ///clone the copy for all knobs
        boost::shared_ptr<KnobI> originalKnob = it->first->getKnob();
        boost::shared_ptr<KnobI> copyWithNewValues = createCopyForKnob(originalKnob);
        
        ///clone the original knob back to its old state
        originalKnob->clone(it->second.copy);
        
        ///clone the copy to the new values
        it->second.copy->clone(copyWithNewValues);
        
        knobsUnique.insert(originalKnob.get());
    }
    
    for (std::set <KnobI*>::iterator it = knobsUnique.begin(); it!=knobsUnique.end(); ++it) {
        (*it)->getHolder()->onKnobValueChanged_public(*it, Natron::USER_EDITED);
    }
    
    assert(!knobs.empty());
    KnobHolder* holder = knobs.begin()->first->getKnob()->getHolder();
    QString holderName;
    if (holder) {
        Natron::EffectInstance* effect = dynamic_cast<Natron::EffectInstance*>(holder);
        if (effect) {
            
            effect->evaluate_public(NULL, true, Natron::USER_EDITED);
            
            holderName = effect->getName().c_str();
        }
    }
    setText(QObject::tr("Multiple edits for %1").arg(holderName));

}

void MultipleKnobEditsUndoCommand::redo()
{
    if (firstRedoCalled) {
        ///just clone
        std::set <KnobI*> knobsUnique;
        for (ParamsMap::iterator it = knobs.begin(); it!= knobs.end(); ++it) {
            boost::shared_ptr<KnobI> originalKnob = it->first->getKnob();
            boost::shared_ptr<KnobI> copyWithOldValues = createCopyForKnob(originalKnob);
            
            ///clone the original knob back to its old state
            originalKnob->clone(it->second.copy);
            
            ///clone the copy to the old values
            it->second.copy->clone(copyWithOldValues);
            
            knobsUnique.insert(originalKnob.get());
            
            for (std::set <KnobI*>::iterator it = knobsUnique.begin(); it!=knobsUnique.end(); ++it) {
                (*it)->getHolder()->onKnobValueChanged_public(*it, Natron::USER_EDITED);
            }
        }

    } else {
        ///this is the first redo command, set values
        for (ParamsMap::iterator it = knobs.begin(); it!= knobs.end(); ++it) {
            int i = 0;
            boost::shared_ptr<KnobI> knob = it->first->getKnob();
            
            std::list<Variant>::iterator next = it->second.newValues.begin();
            ++next;
            for (std::list<Variant>::iterator it2 = it->second.newValues.begin(); it2!=it->second.newValues.end(); ++it2,++i,++next) {
                KeyFrame k;
                Knob<int>* isInt = dynamic_cast<Knob<int>*>(knob.get());
                Knob<bool>* isBool = dynamic_cast<Knob<bool>*>(knob.get());
                Knob<double>* isDouble = dynamic_cast<Knob<double>*>(knob.get());
                Knob<std::string>* isString = dynamic_cast<Knob<std::string>*>(knob.get());
                bool doKnobChanged = next == it->second.newValues.end() && triggerOnKnobChanged;
                if (isInt) {
                    it->first->setValue<int>(i, it2->toInt(), &k,true,doKnobChanged);
                } else if (isBool) {
                    it->first->setValue<bool>(i, it2->toBool(), &k,true,doKnobChanged);
                } else if (isDouble) {
                    it->first->setValue<double>(i, it2->toDouble(), &k,true,doKnobChanged);
                } else if (isString) {
                    it->first->setValue<std::string>(i, it2->toString().toStdString(), &k,true,doKnobChanged);
                } else {
                    assert(false);
                }
                if (it->second.setKeyFrame) {
                    it->first->setKeyframe(it->second.time, i);
                }
            }
        }
    }
    
    assert(!knobs.empty());
    KnobHolder* holder = knobs.begin()->first->getKnob()->getHolder();
    QString holderName;
    if (holder) {
        Natron::EffectInstance* effect = dynamic_cast<Natron::EffectInstance*>(holder);
        if (effect) {
            if (firstRedoCalled) {
                effect->evaluate_public(NULL, true, Natron::USER_EDITED);
            }
            holderName = effect->getName().c_str();
        }
    }
    firstRedoCalled = true;

    setText(QObject::tr("Multiple edits for %1").arg(holderName));

}

int MultipleKnobEditsUndoCommand::id() const
{
    return kMultipleKnobsUndoChangeCommandCompressionID;
}

bool MultipleKnobEditsUndoCommand::mergeWith(const QUndoCommand *command)
{
    const MultipleKnobEditsUndoCommand *knobCommand = dynamic_cast<const MultipleKnobEditsUndoCommand *>(command);
    if (!knobCommand || command->id() != id()) {
        return false;
    }
    
    if (knobCommand->createNew) {
        return false;
    }
    
    knobs.insert(knobCommand->knobs.begin(), knobCommand->knobs.end());
    return true;
}


