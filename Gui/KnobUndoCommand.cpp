//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "KnobUndoCommand.h"

#include "Engine/KnobTypes.h"
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
    for (std::list<Variant>::iterator it = oldValues.begin(); it!=oldValues.end();++it) {
        if ((i == _targetDimension && !_copyAnimation) || _copyAnimation) {
            if (isInt) {
                isInt->setValue(it->toInt(), i);
            } else if (isBool) {
                isBool->setValue(it->toBool(), i);
            } else if (isDouble) {
                isDouble->setValue(it->toDouble(), i);
            } else if (isString) {
                isString->setValue(it->toString().toStdString(), i);
            }
        }
        ++i;
    }
    if (_copyAnimation) {
        
        i = 0;
        for (std::list<boost::shared_ptr<Curve> >::iterator it = oldCurves.begin(); it!=oldCurves.end(); ++it) {
            internalKnob->getCurve(i)->clone(*(*it));
            ++i;
        }
        ///parameters are meaningless here, we just want to update the curve editor.
        _knob->onInternalKeySet(0, 0);

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
    for (std::list<Variant>::iterator it = newValues.begin(); it!=newValues.end();++it) {
        if ((i == _dimensionToFetch && !_copyAnimation) || _copyAnimation) {
            if (isInt) {
                isInt->setValue(it->toInt(), _targetDimension);
            } else if (isBool) {
                isBool->setValue(it->toBool(), _targetDimension);
            } else if (isDouble) {
                isDouble->setValue(it->toDouble(), _targetDimension);
            } else if (isString) {
                isString->setValue(it->toString().toStdString(), _targetDimension);
            }
            break;
        }
        ++i;
    }
    
    i = 0;
    for (std::list<boost::shared_ptr<Curve> >::iterator it = newCurves.begin(); it!=newCurves.end(); ++it) {
        internalKnob->getCurve(i)->clone(*(*it));
        ++i;
    }
    if (_copyAnimation && !newCurves.empty()) {
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

