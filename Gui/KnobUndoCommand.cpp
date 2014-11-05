//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "KnobUndoCommand.h"

#include "Engine/KnobTypes.h"
#include "Engine/KnobFile.h"
#include "Engine/Node.h"
#include "Gui/GuiApplicationManager.h"

PasteUndoCommand::PasteUndoCommand(KnobGui* knob,
                                   bool copyAnimation,
                                   const std::list<Variant> & values,
                                   const std::list<boost::shared_ptr<Curve> > & curves,
                                   const std::list<boost::shared_ptr<Curve> > & parametricCurves,
                                   const std::map<int,std::string> & stringAnimation)
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
      , _copyAnimation(copyAnimation)
{
    assert( !appPTR->isClipBoardEmpty() );
    assert( ( !copyAnimation && newCurves.empty() ) || copyAnimation );

    boost::shared_ptr<KnobI> internalKnob = knob->getKnob();
    Knob<int>* isInt = dynamic_cast<Knob<int>*>( internalKnob.get() );
    Knob<bool>* isBool = dynamic_cast<Knob<bool>*>( internalKnob.get() );
    Knob<double>* isDouble = dynamic_cast<Knob<double>*>( internalKnob.get() );
    Knob<std::string>* isString = dynamic_cast<Knob<std::string>*>( internalKnob.get() );
    AnimatingString_KnobHelper* isAnimatingString = dynamic_cast<AnimatingString_KnobHelper*>( internalKnob.get() );
    boost::shared_ptr<Parametric_Knob> isParametric = boost::dynamic_pointer_cast<Parametric_Knob>(internalKnob);


    for (int i = 0; i < internalKnob->getDimension(); ++i) {
        if (isInt) {
            oldValues.push_back( Variant( isInt->getValue(i) ) );
        } else if (isBool) {
            oldValues.push_back( Variant( isBool->getValue(i) ) );
        } else if (isDouble) {
            oldValues.push_back( Variant( isDouble->getValue(i) ) );
        } else if (isString) {
            oldValues.push_back( Variant( isString->getValue(i).c_str() ) );
        }
        boost::shared_ptr<Curve> c(new Curve);
        c->clone( *internalKnob->getCurve(i) );
        oldCurves.push_back(c);
    }

    if (isAnimatingString) {
        isAnimatingString->saveAnimation(&oldStringAnimation);
    }

    if (isParametric) {
        std::list< Curve > tmpCurves;
        isParametric->saveParametricCurves(&tmpCurves);
        for (std::list< Curve >::iterator it = tmpCurves.begin(); it != tmpCurves.end(); ++it) {
            boost::shared_ptr<Curve> c(new Curve);
            c->clone(*it);
            oldParametricCurves.push_back(c);
        }
    }
}

void
PasteUndoCommand::undo()
{
    boost::shared_ptr<KnobI> internalKnob = _knob->getKnob();
    int targetDimension = internalKnob->getDimension();

    Knob<int>* isInt = dynamic_cast<Knob<int>*>( internalKnob.get() );
    Knob<bool>* isBool = dynamic_cast<Knob<bool>*>( internalKnob.get() );
    Knob<double>* isDouble = dynamic_cast<Knob<double>*>( internalKnob.get() );
    Knob<std::string>* isString = dynamic_cast<Knob<std::string>*>( internalKnob.get() );
    AnimatingString_KnobHelper* isAnimatingString = dynamic_cast<AnimatingString_KnobHelper*>( internalKnob.get() );
    boost::shared_ptr<Parametric_Knob> isParametric = boost::dynamic_pointer_cast<Parametric_Knob>(internalKnob);
    if (_copyAnimation) {
        bool hasKeyframes = false;
        _knob->removeAllKeyframeMarkersOnTimeline(-1);
        std::list<boost::shared_ptr<Curve> >::iterator it = oldCurves.begin();
        for (int i = 0;i < targetDimension; ++it, ++i) {
            internalKnob->getCurve(i)->clone( *(*it) );
            if (internalKnob->getKeyFramesCount(i) > 0) {
                hasKeyframes = true;
            }
        }
        ///parameters are meaningless here, we just want to update the curve editor.
        _knob->onInternalKeySet(0, 0,Natron::eValueChangedReasonNatronGuiEdited,false);
        _knob->setAllKeyframeMarkersOnTimeline(-1);

        if (hasKeyframes) {
            _knob->updateCurveEditorKeyframes();
        }
    }

    std::list<Variant>::iterator it = oldValues.begin();
    internalKnob->blockEvaluation();
    for (int i = 0; i < targetDimension; ++it,++i) {
        bool isLast = i == targetDimension - 1;
        if (isLast) {
            internalKnob->unblockEvaluation();
        }
        if (isInt) {
            isInt->setValue(it->toInt(), i,true);
        } else if (isBool) {
            isBool->setValue(it->toBool(), i,true);
        } else if (isDouble) {
            isDouble->setValue(it->toDouble(), i,true);
        } else if (isString) {
            isString->setValue(it->toString().toStdString(), i,true);
        }

    }


    if (isAnimatingString) {
        isAnimatingString->loadAnimation(oldStringAnimation);
    }

    if (isParametric) {
        std::list<Curve> tmpCurves;
        for (std::list<boost::shared_ptr<Curve> >::iterator it = oldParametricCurves.begin(); it != oldParametricCurves.end(); ++it) {
            Curve c;
            c.clone( *(*it) );
            tmpCurves.push_back(c);
        }
        isParametric->loadParametricCurves(tmpCurves);
    }


    if (!_copyAnimation) {
        setText( QObject::tr("Paste value of %1")
                 .arg( _knob->getKnob()->getDescription().c_str() ) );
    } else {
        setText( QObject::tr("Paste animation of %1")
                 .arg( _knob->getKnob()->getDescription().c_str() ) );
    }
} // undo

void
PasteUndoCommand::redo()
{
    
    boost::shared_ptr<KnobI> internalKnob = _knob->getKnob();

    int targetDimension = internalKnob->getDimension();
    
    Knob<int>* isInt = dynamic_cast<Knob<int>*>( internalKnob.get() );
    Knob<bool>* isBool = dynamic_cast<Knob<bool>*>( internalKnob.get() );
    Knob<double>* isDouble = dynamic_cast<Knob<double>*>( internalKnob.get() );
    Knob<std::string>* isString = dynamic_cast<Knob<std::string>*>( internalKnob.get() );
    AnimatingString_KnobHelper* isAnimatingString = dynamic_cast<AnimatingString_KnobHelper*>( internalKnob.get() );
    boost::shared_ptr<Parametric_Knob> isParametric = boost::dynamic_pointer_cast<Parametric_Knob>(internalKnob);
    bool hasKeyframeData = false;
    if ( !newCurves.empty() ) {
        _knob->removeAllKeyframeMarkersOnTimeline(-1);
        
        std::list<boost::shared_ptr<Curve> >::iterator it = newCurves.begin();
        for (int i = 0;i  < targetDimension; ++it,++i) {
            
            internalKnob->getCurve(i)->clone( *(*it) );
            if ( (*it)->getKeyFramesCount() > 0 ) {
                hasKeyframeData = true;
            }
        }
        _knob->setAllKeyframeMarkersOnTimeline(-1);
    }

    std::list<Variant>::iterator it = newValues.begin();
    internalKnob->blockEvaluation();
    for (int i = 0; i < targetDimension; ++it,++i) {
        
        bool isLast = i == targetDimension - 1;
        if (isLast) {
            internalKnob->unblockEvaluation();
        }
        if (isInt) {
            isInt->setValue(it->toInt(), i, true);
        } else if (isBool) {
            isBool->setValue(it->toBool(), i, true);
        } else if (isDouble) {
            isDouble->setValue(it->toDouble(), i, true);
        } else if (isString) {
            isString->setValue(it->toString().toStdString(), i, true);
        }
    }

    if ( _copyAnimation && hasKeyframeData && !newCurves.empty() ) {
        _knob->updateCurveEditorKeyframes();
    }

    if (isAnimatingString) {
        isAnimatingString->loadAnimation(newStringAnimation);
    }

    if (isParametric) {
        std::list<Curve> tmpCurves;
        for (std::list<boost::shared_ptr<Curve> >::iterator it = newParametricCurves.begin(); it != newParametricCurves.end(); ++it) {
            Curve c;
            c.clone( *(*it) );
            tmpCurves.push_back(c);
        }
        isParametric->loadParametricCurves(tmpCurves);
    }

    if (!_copyAnimation) {
        setText( QObject::tr("Paste value of %1")
                 .arg( _knob->getKnob()->getDescription().c_str() ) );
    } else {
        setText( QObject::tr("Paste animation of %1")
                 .arg( _knob->getKnob()->getDescription().c_str() ) );
    }
} // redo

MultipleKnobEditsUndoCommand::MultipleKnobEditsUndoCommand(KnobGui* knob,
                                                           bool createNew,
                                                           bool setKeyFrame,
                                                           const Variant & value,
                                                           int dimension,
                                                           int time)
    : QUndoCommand()
      , knobs()
      , createNew(createNew)
      , firstRedoCalled(false)
{
    assert(knob);
    boost::shared_ptr<KnobI> originalKnob = knob->getKnob();
    boost::shared_ptr<KnobI> copy = createCopyForKnob(originalKnob);
    ValueToSet v;
    v.newValue = value;
    v.dimension = dimension;
    v.time = time;
    v.copy = copy;
    v.setKeyFrame = setKeyFrame;
    knobs.insert( std::make_pair(knob, v) );
}

MultipleKnobEditsUndoCommand::~MultipleKnobEditsUndoCommand()
{
}

boost::shared_ptr<KnobI> MultipleKnobEditsUndoCommand::createCopyForKnob(const boost::shared_ptr<KnobI> & originalKnob)
{
    const std::string & typeName = originalKnob->typeName();
    boost::shared_ptr<KnobI> copy;
    int dimension = originalKnob->getDimension();

    if ( typeName == Int_Knob::typeNameStatic() ) {
        copy.reset( new Int_Knob(NULL,"",dimension,false) );
    } else if ( typeName == Bool_Knob::typeNameStatic() ) {
        copy.reset( new Bool_Knob(NULL,"",dimension,false) );
    } else if ( typeName == Double_Knob::typeNameStatic() ) {
        copy.reset( new Double_Knob(NULL,"",dimension,false) );
    } else if ( typeName == Choice_Knob::typeNameStatic() ) {
        copy.reset( new Choice_Knob(NULL,"",dimension,false) );
    } else if ( typeName == String_Knob::typeNameStatic() ) {
        copy.reset( new String_Knob(NULL,"",dimension,false) );
    } else if ( typeName == Parametric_Knob::typeNameStatic() ) {
        copy.reset( new Parametric_Knob(NULL,"",dimension,false) );
    } else if ( typeName == Color_Knob::typeNameStatic() ) {
        copy.reset( new Color_Knob(NULL,"",dimension,false) );
    } else if ( typeName == Path_Knob::typeNameStatic() ) {
        copy.reset( new Path_Knob(NULL,"",dimension,false) );
    } else if ( typeName == File_Knob::typeNameStatic() ) {
        copy.reset( new File_Knob(NULL,"",dimension,false) );
    } else if ( typeName == OutputFile_Knob::typeNameStatic() ) {
        copy.reset( new OutputFile_Knob(NULL,"",dimension,false) );
    }

    ///If this is another type of knob this is wrong since they do not hold any value
    assert(copy);
    copy->populate();

    ///make a clone of the original knob at that time and stash it
    copy->clone(originalKnob);

    return copy;
}

void
MultipleKnobEditsUndoCommand::undo()
{
    ///keep track of all different knobs and call instance changed action only once for all of them
    std::set <KnobI*> knobsUnique;

    for (ParamsMap::iterator it = knobs.begin(); it != knobs.end(); ++it) {
        ///clone the copy for all knobs
        boost::shared_ptr<KnobI> originalKnob = it->first->getKnob();
        boost::shared_ptr<KnobI> copyWithNewValues = createCopyForKnob(originalKnob);

        ///clone the original knob back to its old state
        originalKnob->clone(it->second.copy);

        ///clone the copy to the new values
        it->second.copy->clone(copyWithNewValues);

        knobsUnique.insert( originalKnob.get() );
    }


    assert( !knobs.empty() );
    KnobHolder* holder = knobs.begin()->first->getKnob()->getHolder();
    QString holderName;


    if (holder) {
        int currentFrame = holder->getApp()->getTimeLine()->currentFrame();
        for (std::set <KnobI*>::iterator it = knobsUnique.begin(); it != knobsUnique.end(); ++it) {
            (*it)->getHolder()->onKnobValueChanged_public(*it, Natron::eValueChangedReasonUserEdited,currentFrame);
        }

        Natron::EffectInstance* effect = dynamic_cast<Natron::EffectInstance*>(holder);
        if (effect) {
            effect->evaluate_public(NULL, true, Natron::eValueChangedReasonUserEdited);

            holderName = effect->getName().c_str();
        }
    }
    setText( QObject::tr("Multiple edits for %1").arg(holderName) );
}

void
MultipleKnobEditsUndoCommand::redo()
{
    if (firstRedoCalled) {
        ///just clone
        std::set <KnobI*> knobsUnique;
        for (ParamsMap::iterator it = knobs.begin(); it != knobs.end(); ++it) {
            boost::shared_ptr<KnobI> originalKnob = it->first->getKnob();
            boost::shared_ptr<KnobI> copyWithOldValues = createCopyForKnob(originalKnob);

            ///clone the original knob back to its old state
            originalKnob->clone(it->second.copy);

            ///clone the copy to the old values
            it->second.copy->clone(copyWithOldValues);

            knobsUnique.insert( originalKnob.get() );


            for (std::set <KnobI*>::iterator it = knobsUnique.begin(); it != knobsUnique.end(); ++it) {
                int currentFrame = (*it)->getHolder()->getApp()->getTimeLine()->currentFrame();
                (*it)->getHolder()->onKnobValueChanged_public(*it, Natron::eValueChangedReasonUserEdited,currentFrame);
            }
        }
    } else {
        ///this is the first redo command, set values
        for (ParamsMap::iterator it = knobs.begin(); it != knobs.end(); ++it) {
            boost::shared_ptr<KnobI> knob = it->first->getKnob();
            KeyFrame k;
            Knob<int>* isInt = dynamic_cast<Knob<int>*>( knob.get() );
            Knob<bool>* isBool = dynamic_cast<Knob<bool>*>( knob.get() );
            Knob<double>* isDouble = dynamic_cast<Knob<double>*>( knob.get() );
            Knob<std::string>* isString = dynamic_cast<Knob<std::string>*>( knob.get() );
            if (isInt) {
                it->first->setValue<int>(it->second.dimension, it->second.newValue.toInt(), &k,true,Natron::eValueChangedReasonPluginEdited);
            } else if (isBool) {
                it->first->setValue<bool>(it->second.dimension, it->second.newValue.toBool(), &k,true,Natron::eValueChangedReasonPluginEdited);
            } else if (isDouble) {
                it->first->setValue<double>(it->second.dimension, it->second.newValue.toDouble(), &k,true,Natron::eValueChangedReasonPluginEdited);
            } else if (isString) {
                it->first->setValue<std::string>(it->second.dimension, it->second.newValue.toString().toStdString(),
                                                 &k,true,Natron::eValueChangedReasonPluginEdited);
            } else {
                assert(false);
            }
            if (it->second.setKeyFrame) {
                it->first->setKeyframe(it->second.time, it->second.dimension);
            }
        }
    }

    assert( !knobs.empty() );
    KnobHolder* holder = knobs.begin()->first->getKnob()->getHolder();
    QString holderName;
    if (holder) {
        Natron::EffectInstance* effect = dynamic_cast<Natron::EffectInstance*>(holder);
        if (effect) {
            if (firstRedoCalled) {
                effect->evaluate_public(NULL, true, Natron::eValueChangedReasonUserEdited);
            } else {
                effect->getApp()->triggerAutoSave();
            }
            holderName = effect->getName().c_str();
        }
    }
    firstRedoCalled = true;

    setText( QObject::tr("Multiple edits for %1").arg(holderName) );
} // redo

int
MultipleKnobEditsUndoCommand::id() const
{
    return kMultipleKnobsUndoChangeCommandCompressionID;
}

bool
MultipleKnobEditsUndoCommand::mergeWith(const QUndoCommand *command)
{
    const MultipleKnobEditsUndoCommand *knobCommand = dynamic_cast<const MultipleKnobEditsUndoCommand *>(command);

    if ( !knobCommand || ( command->id() != id() ) ) {
        return false;
    }

    ///if all knobs are the same between the old and new command, ignore the createNew flag and merge them anyway
    bool ignoreCreateNew = false;
    if ( knobs.size() == knobCommand->knobs.size() ) {
        ParamsMap::const_iterator thisIt = knobs.begin();
        ParamsMap::const_iterator otherIt = knobCommand->knobs.begin();
        bool oneDifferent = false;
        for (; thisIt != knobs.end(); ++thisIt,++otherIt) {
            if (thisIt->first != otherIt->first) {
                oneDifferent = true;
                break;
            }
        }
        if (!oneDifferent) {
            ignoreCreateNew = true;
        }
    }

    if (!ignoreCreateNew && knobCommand->createNew) {
        return false;
    }

    knobs.insert( knobCommand->knobs.begin(), knobCommand->knobs.end() );

    return true;
}

RestoreDefaultsCommand::RestoreDefaultsCommand(const std::list<boost::shared_ptr<KnobI> > & knobs,
                                               QUndoCommand *parent)
    : QUndoCommand(parent)
      , _knobs(knobs)
{
    for (std::list<boost::shared_ptr<KnobI> >::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        _clones.push_back( MultipleKnobEditsUndoCommand::createCopyForKnob(*it) );
    }
}

void
RestoreDefaultsCommand::undo()
{
    assert( _clones.size() == _knobs.size() );

    std::list<SequenceTime> times;
    const boost::shared_ptr<KnobI> & first = _knobs.front();
    boost::shared_ptr<TimeLine> timeline = first->getHolder()->getApp()->getTimeLine();
    std::list<boost::shared_ptr<KnobI> >::const_iterator itClone = _clones.begin();
    for (std::list<boost::shared_ptr<KnobI> >::const_iterator it = _knobs.begin(); it != _knobs.end(); ++it,++itClone) {
        (*it)->cloneAndUpdateGui( itClone->get() );

        if ( (*it)->getHolder()->getApp() ) {
            int dim = (*it)->getDimension();
            for (int i = 0; i < dim; ++i) {
                KeyFrameSet kfs = (*it)->getCurve(i)->getKeyFrames_mt_safe();
                for (KeyFrameSet::iterator it = kfs.begin(); it != kfs.end(); ++it) {
                    times.push_back( it->getTime() );
                }
            }
        }
    }
    timeline->addMultipleKeyframeIndicatorsAdded(times,true);

    _knobs.front()->getHolder()->evaluate_public(NULL, true, Natron::eValueChangedReasonUserEdited);
    first->getHolder()->evaluate_public(NULL, true, Natron::eValueChangedReasonUserEdited);
    if ( first->getHolder()->getApp() ) {
        first->getHolder()->getApp()->redrawAllViewers();
    }

    setText( QObject::tr("Restore default value(s)") );
}

void
RestoreDefaultsCommand::redo()
{
    std::list<SequenceTime> times;
    const boost::shared_ptr<KnobI> & first = _knobs.front();
    boost::shared_ptr<TimeLine> timeline = first->getHolder()->getApp()->getTimeLine();

    for (std::list<boost::shared_ptr<KnobI> >::iterator it = _knobs.begin(); it != _knobs.end(); ++it) {
        if ( (*it)->getHolder()->getApp() ) {
            int dim = (*it)->getDimension();
            for (int i = 0; i < dim; ++i) {
                KeyFrameSet kfs = (*it)->getCurve(i)->getKeyFrames_mt_safe();
                for (KeyFrameSet::iterator it = kfs.begin(); it != kfs.end(); ++it) {
                    times.push_back( it->getTime() );
                }
            }
        }

        (*it)->blockEvaluation();
        for (int d = 0; d < (*it)->getDimension(); ++d) {
            (*it)->resetToDefaultValue(d);
        }
        (*it)->unblockEvaluation();
    }
    timeline->removeMultipleKeyframeIndicator(times,true);

    first->getHolder()->evaluate_public(NULL, true, Natron::eValueChangedReasonUserEdited);
    if ( first->getHolder()->getApp() ) {
        first->getHolder()->getApp()->redrawAllViewers();
    }
    setText( QObject::tr("Restore default value(s)") );
}

