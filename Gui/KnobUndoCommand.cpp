/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
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

#include "KnobUndoCommand.h"

#include <stdexcept>
#include <sstream> // stringstream
#include <QDebug>
#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
GCC_DIAG_OFF(unused-parameter)
// /opt/local/include/boost/serialization/smart_cast.hpp:254:25: warning: unused parameter 'u' [-Wunused-parameter]
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/serialization/map.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
GCC_DIAG_ON(unused-parameter)
#endif

#include "Engine/KnobTypes.h"
#include "Engine/KnobFile.h"
#include "Engine/Node.h"
#include "Engine/TimeLine.h"
#include "Engine/AppInstance.h"
#include "Engine/KnobSerialization.h"
#include "Engine/ViewIdx.h"

#include "Gui/GuiApplicationManager.h"


NATRON_NAMESPACE_ENTER
struct PasteUndoCommandPrivate
{
    KnobGuiWPtr knob;
    KnobClipBoardType type;
    int fromDimension;
    int targetDimension;
    boost::shared_ptr<KnobSerialization> originalSerialization;
    KnobPtr fromKnob;

    PasteUndoCommandPrivate()
        : knob()
        , type(eKnobClipBoardTypeCopyLink)
        , fromDimension(-1)
        , targetDimension(-1)
        , originalSerialization(new KnobSerialization)
    {
    }
};

PasteUndoCommand::PasteUndoCommand(const KnobGuiPtr& knob,
                                   KnobClipBoardType type,
                                   int fromDimension,
                                   int targetDimension,
                                   const KnobPtr& fromKnob)
    : QUndoCommand(0)
    , _imp( new PasteUndoCommandPrivate() )
{
    _imp->knob = knob;
    _imp->type = type;
    _imp->fromDimension = fromDimension;
    _imp->targetDimension = targetDimension;
    _imp->fromKnob = fromKnob;

    {
        std::ostringstream ss;
        {
            try {
                boost::archive::xml_oarchive oArchive(ss);
                _imp->originalSerialization->initialize( knob->getKnob() );
                oArchive << boost::serialization::make_nvp("KnobClipboard", *_imp->originalSerialization);
            } catch (...) {
                assert(false);
            }
        }
        _imp->originalSerialization.reset(new KnobSerialization);
        std::string str = ss.str();
        {
            try {
                std::stringstream ss(str);
                boost::archive::xml_iarchive iArchive(ss);
                iArchive >> boost::serialization::make_nvp("KnobClipboard", *_imp->originalSerialization);
            } catch (...) {
                assert(false);
            }
        }
    }
    assert( _imp->originalSerialization->getKnob() );

    assert(knob);
    assert( _imp->targetDimension >= -1 && _imp->targetDimension < _imp->knob.lock()->getKnob()->getDimension() );
    assert( _imp->fromDimension >= -1 && _imp->fromDimension < _imp->fromKnob->getDimension() );
    QString text;
    switch (type) {
    case eKnobClipBoardTypeCopyAnim:
        text = tr("Paste Animation to %1");
        break;
    case eKnobClipBoardTypeCopyValue:
        text = tr("Paste Value to %1");
        break;
    case eKnobClipBoardTypeCopyLink:
        text = tr("Paste Link to %1");
        break;
    }
    setText( text.arg( QString::fromUtf8( knob->getKnob()->getLabel().c_str() ) ) );
}

PasteUndoCommand::~PasteUndoCommand()
{
}

void
PasteUndoCommand::undo()
{
    copyFrom(_imp->originalSerialization->getKnob(), false);
} // undo

void
PasteUndoCommand::redo()
{
    copyFrom(_imp->fromKnob, true);
} // undo

void
PasteUndoCommand::copyFrom(const KnobPtr& serializedKnob,
                           bool isRedo)
{
    KnobPtr internalKnob = _imp->knob.lock()->getKnob();

    switch (_imp->type) {
    case eKnobClipBoardTypeCopyAnim: {
        internalKnob->beginChanges();
        for (int i = 0; i < internalKnob->getDimension(); ++i) {
            if ( ( _imp->targetDimension == -1) || ( i == _imp->targetDimension) ) {
                boost::shared_ptr<Curve> fromCurve;
                if ( ( i == _imp->targetDimension) && ( _imp->fromDimension != -1) ) {
                    fromCurve = serializedKnob->getCurve(ViewIdx(0), _imp->fromDimension);
                } else {
                    fromCurve = serializedKnob->getCurve(ViewIdx(0), i);
                }
                if (!fromCurve) {
                    continue;
                }
                internalKnob->cloneCurve(ViewIdx(0), i, *fromCurve);
            }
        }
        internalKnob->endChanges();
        break;
    }
    case eKnobClipBoardTypeCopyValue: {
        Knob<int>* isInt = dynamic_cast<Knob<int>*>( internalKnob.get() );
        Knob<bool>* isBool = dynamic_cast<Knob<bool>*>( internalKnob.get() );
        Knob<double>* isDouble = dynamic_cast<Knob<double>*>( internalKnob.get() );
        Knob<std::string>* isString = dynamic_cast<Knob<std::string>*>( internalKnob.get() );

        Knob<int>* isFromInt = dynamic_cast<Knob<int>*>( serializedKnob.get() );
        Knob<bool>* isFromBool = dynamic_cast<Knob<bool>*>( serializedKnob.get() );
        Knob<double>* isFromDouble = dynamic_cast<Knob<double>*>( serializedKnob.get() );
        Knob<std::string>* isFromString = dynamic_cast<Knob<std::string>*>( serializedKnob.get() );

        internalKnob->beginChanges();
        for (int i = 0; i < internalKnob->getDimension(); ++i) {
            if ( ( _imp->targetDimension == -1) || ( i == _imp->targetDimension) ) {
                if (isInt && isFromInt) {
                    int f = (i == _imp->targetDimension && _imp->fromDimension != -1) ? isFromInt->getValue(_imp->fromDimension) : isFromInt->getValue(i);
                    isInt->setValue(f, ViewIdx(0), i, eValueChangedReasonNatronInternalEdited, 0);
                } else if (isBool && isFromBool) {
                    bool f = (i == _imp->targetDimension && _imp->fromDimension != -1) ? isFromBool->getValue(_imp->fromDimension) : isFromBool->getValue(i);
                    isBool->setValue(f, ViewIdx(0), i, eValueChangedReasonNatronInternalEdited, 0);
                } else if (isDouble && isFromDouble) {
                    double f = (i == _imp->targetDimension && _imp->fromDimension != -1) ? isFromDouble->getValue(_imp->fromDimension) : isFromDouble->getValue(i);
                    isDouble->setValue(f, ViewIdx(0), i, eValueChangedReasonNatronInternalEdited, 0);
                } else if (isString && isFromString) {
                    std::string f = (i == _imp->targetDimension && _imp->fromDimension != -1) ? isFromString->getValue(_imp->fromDimension) : isFromString->getValue(i);
                    isString->setValue(f, ViewIdx(0), i, eValueChangedReasonNatronInternalEdited, 0);
                }
            }
        }
        internalKnob->endChanges();
        break;
    }
    case eKnobClipBoardTypeCopyLink: {
        //bool useExpression = !KnobI::areTypesCompatibleForSlave(internalKnob.get(), serializedKnob.get());

        internalKnob->beginChanges();
        for (int i = 0; i < internalKnob->getDimension(); ++i) {
            if ( ( _imp->targetDimension == -1) || ( i == _imp->targetDimension) ) {
                if (isRedo) {
                    if (_imp->fromDimension != -1) {
                        internalKnob->slaveTo(i, serializedKnob, _imp->fromDimension);
                    } else {
                        internalKnob->slaveTo(i, serializedKnob, i);
                    }
                } else {
                    internalKnob->unSlave(i, false);
                }
            }
        }
        internalKnob->endChanges();
        break;
    }
    } // switch
} // redo

MultipleKnobEditsUndoCommand::MultipleKnobEditsUndoCommand(const KnobGuiPtr& knob,
                                                           ValueChangedReasonEnum reason,
                                                           bool createNew,
                                                           bool setKeyFrame,
                                                           const Variant & value,
                                                           int dimension,
                                                           double time)
    : QUndoCommand()
    , knobs()
    , createNew(createNew)
    , firstRedoCalled(false)
    , _reason(reason)
{
    assert(knob);
    std::list<ValueToSet>& vlist = knobs[knob];
    ValueToSet v;
    v.newValue = value;
    v.dimension = dimension;
    assert(dimension != -1);
    v.time = time;
    v.setKeyFrame = setKeyFrame;
    v.setValueRetCode = -1;
    vlist.push_back(v);

    KnobHolder* holder = knob->getKnob()->getHolder();
    EffectInstance* effect = dynamic_cast<EffectInstance*>(holder);
    QString holderName;
    if (effect) {
        holderName = QString::fromUtf8( effect->getNode()->getLabel().c_str() );
    }

    setText( tr("Multiple edits for %1").arg(holderName) );
}

MultipleKnobEditsUndoCommand::~MultipleKnobEditsUndoCommand()
{
}

KnobPtr
MultipleKnobEditsUndoCommand::createCopyForKnob(const KnobPtr & originalKnob)
{
    const std::string & typeName = originalKnob->typeName();
    KnobPtr copy;
    int dimension = originalKnob->getDimension();

    if ( typeName == KnobInt::typeNameStatic() ) {
        copy.reset( new KnobInt(NULL, std::string(), dimension, false) );
    } else if ( typeName == KnobBool::typeNameStatic() ) {
        copy.reset( new KnobBool(NULL, std::string(), dimension, false) );
    } else if ( typeName == KnobDouble::typeNameStatic() ) {
        copy.reset( new KnobDouble(NULL, std::string(), dimension, false) );
    } else if ( typeName == KnobChoice::typeNameStatic() ) {
        copy.reset( new KnobChoice(NULL, std::string(), dimension, false) );
    } else if ( typeName == KnobString::typeNameStatic() ) {
        copy.reset( new KnobString(NULL, std::string(), dimension, false) );
    } else if ( typeName == KnobParametric::typeNameStatic() ) {
        copy.reset( new KnobParametric(NULL, std::string(), dimension, false) );
    } else if ( typeName == KnobColor::typeNameStatic() ) {
        copy.reset( new KnobColor(NULL, std::string(), dimension, false) );
    } else if ( typeName == KnobPath::typeNameStatic() ) {
        copy.reset( new KnobPath(NULL, std::string(), dimension, false) );
    } else if ( typeName == KnobFile::typeNameStatic() ) {
        copy.reset( new KnobFile(NULL, std::string(), dimension, false) );
    } else if ( typeName == KnobOutputFile::typeNameStatic() ) {
        copy.reset( new KnobOutputFile(NULL, std::string(), dimension, false) );
    }

    ///If this is another type of knob this is wrong since they do not hold any value
    assert(copy);
    if (!copy) {
        return KnobPtr();
    }
    copy->populate();

    ///make a clone of the original knob at that time and stash it
    copy->clone(originalKnob);

    return copy;
}

void
MultipleKnobEditsUndoCommand::undo()
{
    assert( !knobs.empty() );
    KnobHolder* holder = knobs.begin()->first.lock()->getKnob()->getHolder();
    if (holder) {
        holder->beginChanges();
    }

    for (ParamsMap::iterator it = knobs.begin(); it != knobs.end(); ++it) {
        KnobGuiPtr knobUI = it->first.lock();
        if (!knobUI) {
            continue;
        }
        KnobPtr knob = knobUI->getKnob();
        if (!knob) {
            continue;
        }
        knob->beginChanges();
        Knob<int>* isInt = dynamic_cast<Knob<int>*>( knob.get() );
        Knob<bool>* isBool = dynamic_cast<Knob<bool>*>( knob.get() );
        Knob<double>* isDouble = dynamic_cast<Knob<double>*>( knob.get() );
        Knob<std::string>* isString = dynamic_cast<Knob<std::string>*>( knob.get() );
        KeyFrame k;
        // For setValue calls, only set back the old value once per-dimension
        std::set<int> dimensionsUndone;
        for (std::list<ValueToSet>::iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
            KnobHelper::ValueChangedReturnCodeEnum retCode = (KnobHelper::ValueChangedReturnCodeEnum)it2->setValueRetCode;

            if (retCode == KnobHelper::eValueChangedReturnCodeKeyframeAdded) {
                knobUI->removeKeyFrame( it2->time, it2->dimension, ViewIdx(0) );
            } else {
                if (it2->setKeyFrame) {
                    bool refreshGui =  it2->time == knob->getHolder()->getApp()->getTimeLine()->currentFrame();
                    if (isInt) {
                        knobUI->setValueAtTime<int>(it2->dimension, it2->oldValue.toInt(), it2->time, ViewIdx(0), &k, refreshGui, _reason);
                    } else if (isBool) {
                        knobUI->setValueAtTime<bool>(it2->dimension, it2->oldValue.toBool(), it2->time, ViewIdx(0), &k, refreshGui, _reason);
                    } else if (isDouble) {
                        knobUI->setValueAtTime<double>(it2->dimension, it2->oldValue.toDouble(), it2->time, ViewIdx(0), &k, refreshGui, _reason);
                    } else if (isString) {
                        knobUI->setValueAtTime<std::string>(it2->dimension, it2->oldValue.toString().toStdString(), it2->time, ViewIdx(0), &k, refreshGui, _reason);
                    } else {
                        assert(false);
                    }
                } else {
                    // Only set back the first value that was set in the command
                    if (dimensionsUndone.find(it2->dimension) != dimensionsUndone.end()) {
                        continue;
                    }
                    dimensionsUndone.insert(it2->dimension);
                    if (isInt) {
                        knobUI->setValue<int>(it2->dimension, it2->oldValue.toInt(), &k, true, _reason);
                    } else if (isBool) {
                        knobUI->setValue<bool>(it2->dimension, it2->oldValue.toBool(), &k, true, _reason);
                    } else if (isDouble) {
                        knobUI->setValue<double>(it2->dimension, it2->oldValue.toDouble(), &k, true, _reason);
                    } else if (isString) {
                        knobUI->setValue<std::string>(it2->dimension, it2->oldValue.toString().toStdString(),
                                                      &k, true, _reason);
                    } else {
                        assert(false);
                    }
                }
            }
        }
        knob->endChanges();
    }


    if (holder) {
        holder->endChanges();
    }
} // MultipleKnobEditsUndoCommand::undo

void
MultipleKnobEditsUndoCommand::redo()
{
    assert( !knobs.empty() );
    KnobHolder* holder = knobs.begin()->first.lock()->getKnob()->getHolder();
    if (holder) {
        holder->beginChanges();
    }

    ///this is the first redo command, set values
    for (ParamsMap::iterator it = knobs.begin(); it != knobs.end(); ++it) {
        KnobGuiPtr knobUI = it->first.lock();
        if (!knobUI) {
            continue;
        }
        KnobPtr knob = knobUI->getKnob();
        if (!knob) {
            continue;
        }
        knob->beginChanges();
        Knob<int>* isInt = dynamic_cast<Knob<int>*>( knob.get() );
        Knob<bool>* isBool = dynamic_cast<Knob<bool>*>( knob.get() );
        Knob<double>* isDouble = dynamic_cast<Knob<double>*>( knob.get() );
        Knob<std::string>* isString = dynamic_cast<Knob<std::string>*>( knob.get() );

        for (std::list<ValueToSet>::iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
            KeyFrame k;
            if (!firstRedoCalled) {
                if (isInt) {
                    it2->oldValue.setValue( isInt->getValueAtTime(it2->time, it2->dimension) );
                } else if (isBool) {
                    it2->oldValue.setValue( isBool->getValueAtTime(it2->time, it2->dimension) );
                } else if (isDouble) {
                    it2->oldValue.setValue( isDouble->getValueAtTime(it2->time, it2->dimension) );
                } else if (isString) {
                    it2->oldValue.setValue( isString->getValueAtTime(it2->time, it2->dimension) );
                }
            }

            if (it2->setKeyFrame) {
                bool keyAdded = false;
                bool refreshGui =  it2->time == knob->getHolder()->getApp()->getTimeLine()->currentFrame();
                if (isInt) {
                    keyAdded = knobUI->setValueAtTime<int>(it2->dimension, it2->newValue.toInt(), it2->time, ViewIdx(0), &k, refreshGui, _reason);
                } else if (isBool) {
                    keyAdded = knobUI->setValueAtTime<bool>(it2->dimension, it2->newValue.toBool(), it2->time, ViewIdx(0), &k, refreshGui, _reason);
                } else if (isDouble) {
                    keyAdded = knobUI->setValueAtTime<double>(it2->dimension, it2->newValue.toDouble(), it2->time, ViewIdx(0), &k, refreshGui, _reason);
                } else if (isString) {
                    keyAdded = knobUI->setValueAtTime<std::string>(it2->dimension, it2->newValue.toString().toStdString(), it2->time,
                                                                   ViewIdx(0), &k, refreshGui, _reason);
                } else {
                    assert(false);
                }
                it2->setValueRetCode = keyAdded ? KnobHelper::eValueChangedReturnCodeKeyframeAdded : KnobHelper::eValueChangedReturnCodeKeyframeModified;
            } else {
                if (isInt) {
                    it2->setValueRetCode = knobUI->setValue<int>(it2->dimension, it2->newValue.toInt(), &k, true, _reason);
                } else if (isBool) {
                    it2->setValueRetCode = knobUI->setValue<bool>(it2->dimension, it2->newValue.toBool(), &k, true, _reason);
                } else if (isDouble) {
                    it2->setValueRetCode = knobUI->setValue<double>(it2->dimension, it2->newValue.toDouble(), &k, true, _reason);
                } else if (isString) {
                    it2->setValueRetCode = knobUI->setValue<std::string>(it2->dimension, it2->newValue.toString().toStdString(),
                                                                         &k, true, _reason);
                } else {
                    assert(false);
                }
                if (!firstRedoCalled && !it2->setKeyFrame) {
                    it2->time = knob->getCurrentTime();
                }
            }
        }
        knob->endChanges();
    }

    if (holder) {
        holder->endChanges();
    }

    firstRedoCalled = true;
} // redo

int
MultipleKnobEditsUndoCommand::id() const
{
    return kMultipleKnobsUndoChangeCommandCompressionID;
}

bool
MultipleKnobEditsUndoCommand::mergeWith(const QUndoCommand *command)
{
    /*
     * The command in parameter just had its redo() function call and we attempt to merge it with a previous
     * command that has been redone already
     */
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
        for (; thisIt != knobs.end(); ++thisIt, ++otherIt) {
            if ( thisIt->first.lock() != otherIt->first.lock() ) {
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

    for (ParamsMap::const_iterator otherIt = knobCommand->knobs.begin(); otherIt != knobCommand->knobs.end(); ++otherIt) {
        ParamsMap::iterator foundExistinKnob =  knobs.find(otherIt->first);
        if ( foundExistinKnob == knobs.end() ) {
            knobs.insert(*otherIt);
        } else {
            //copy the other dimension of that knob which changed and set the dimension to -1 so
            //that subsequent calls to undo() and redo() clone all dimensions at once
            //foundExistinKnob->second.copy->clone(otherIt->second.copy,otherIt->second.dimension);
            foundExistinKnob->second.insert( foundExistinKnob->second.end(), otherIt->second.begin(), otherIt->second.end() );
        }
    }

    return true;
}

RestoreDefaultsCommand::RestoreDefaultsCommand(bool isNodeReset,
                                               const std::list<KnobPtr > & knobs,
                                               int targetDim,
                                               QUndoCommand *parent)
    : QUndoCommand(parent)
    , _isNodeReset(isNodeReset)
    , _targetDim(targetDim)
    , _knobs()
{
    for (std::list<KnobPtr >::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        _knobs.push_front(*it);
        _clones.push_back( MultipleKnobEditsUndoCommand::createCopyForKnob(*it) );
    }
}

void
RestoreDefaultsCommand::undo()
{
    assert( _clones.size() == _knobs.size() );

    std::list<SequenceTime> times;
    KnobPtr first = _knobs.front().lock();
    AppInstPtr app = first->getHolder()->getApp();
    assert(app);
    std::list<KnobWPtr >::const_iterator itClone = _clones.begin();
    for (std::list<KnobWPtr >::const_iterator it = _knobs.begin(); it != _knobs.end(); ++it, ++itClone) {
        KnobPtr itKnob = it->lock();
        if (!itKnob) {
            continue;
        }
        KnobPtr itCloneKnob = itClone->lock();
        if (!itCloneKnob) {
            continue;
        }
        itKnob->cloneAndUpdateGui( itCloneKnob.get() );

        if ( itKnob->getHolder()->getApp() ) {
            int dim = itKnob->getDimension();
            for (int i = 0; i < dim; ++i) {
                if ( (i == _targetDim) || (_targetDim == -1) ) {
                    boost::shared_ptr<Curve> c = itKnob->getCurve(ViewIdx(0), i);
                    if (c) {
                        KeyFrameSet kfs = c->getKeyFrames_mt_safe();
                        for (KeyFrameSet::iterator it = kfs.begin(); it != kfs.end(); ++it) {
                            times.push_back( std::floor(it->getTime() + 0.5) );
                        }
                    }
                }
            }
        }
    }
    app->addMultipleKeyframeIndicatorsAdded(times, true);

    first->getHolder()->incrHashAndEvaluate(true, true);
    if ( first->getHolder()->getApp() ) {
        first->getHolder()->getApp()->redrawAllViewers();
    }

    setText( tr("Restore default value(s)") );
}

void
RestoreDefaultsCommand::redo()
{
    std::list<SequenceTime> times;
    KnobPtr first = _knobs.front().lock();
    AppInstPtr app;
    KnobHolder* holder = first->getHolder();
    EffectInstance* isEffect = dynamic_cast<EffectInstance*>(holder);

    if (holder) {
        app = holder->getApp();
        holder->beginChanges();
    }


    /*
       First reset all knobs values, this will not call instanceChanged action
     */
    for (std::list<KnobWPtr >::iterator it = _knobs.begin(); it != _knobs.end(); ++it) {
        KnobPtr itKnob = it->lock();
        if (!itKnob) {
            continue;
        }
        if ( itKnob->getHolder() && itKnob->getHolder()->getApp() ) {
            int dim = itKnob->getDimension();
            for (int i = 0; i < dim; ++i) {
                if ( (i == _targetDim) || (_targetDim == -1) ) {
                    boost::shared_ptr<Curve> c = itKnob->getCurve(ViewIdx(0), i);
                    if (c) {
                        KeyFrameSet kfs = c->getKeyFrames_mt_safe();
                        for (KeyFrameSet::iterator it = kfs.begin(); it != kfs.end(); ++it) {
                            times.push_back( std::floor(it->getTime() + 0.5) );
                        }
                    }
                }
            }
        }

        if ( itKnob->getHolder() ) {
            itKnob->getHolder()->beginChanges();
        }
       // itKnob->blockValueChanges();

        for (int d = 0; d < itKnob->getDimension(); ++d) {
            if ( (d == _targetDim) || (_targetDim == -1) ) {
                itKnob->resetToDefaultValue(d);
            }
        }

        // itKnob->unblockValueChanges();

        if ( itKnob->getHolder() ) {
            itKnob->getHolder()->endChanges(true);
        }
    }

    /*
       Block value changes and call instanceChange on all knobs  afterwards to put back the plug-in
       in a correct state
     */
    double time = 0;
    if (app) {
        time = app->getTimeLine()->currentFrame();
    }
    for (std::list<KnobWPtr >::iterator it = _knobs.begin(); it != _knobs.end(); ++it) {
        KnobPtr itKnob = it->lock();
        if (!itKnob) {
            continue;
        }
        if ( itKnob->getHolder() ) {
            itKnob->getHolder()->onKnobValueChanged_public(itKnob.get(), eValueChangedReasonRestoreDefault, time, ViewIdx(0), true);
        }
    }

    if (app) {
        app->removeMultipleKeyframeIndicator(times, true);
    }


    if ( holder && holder->getApp() ) {
        holder->endChanges();
    }

    if (_isNodeReset && isEffect) {
        isEffect->purgeCaches();
    }


    if ( first->getHolder() ) {
        first->getHolder()->incrHashAndEvaluate(true, true);
        if ( first->getHolder()->getApp() ) {
            first->getHolder()->getApp()->redrawAllViewers();
        }
    }
    setText( tr("Restore default value(s)") );
} // RestoreDefaultsCommand::redo

SetExpressionCommand::SetExpressionCommand(const KnobPtr & knob,
                                           bool hasRetVar,
                                           int dimension,
                                           const std::string& expr,
                                           QUndoCommand *parent)
    : QUndoCommand(parent)
    , _knob(knob)
    , _oldExprs()
    , _hadRetVar()
    , _newExpr(expr)
    , _hasRetVar(hasRetVar)
    , _dimension(dimension)
{
    for (int i = 0; i < knob->getDimension(); ++i) {
        _oldExprs.push_back( knob->getExpression(i) );
        _hadRetVar.push_back( knob->isExpressionUsingRetVariable(i) );
    }
}

void
SetExpressionCommand::undo()
{
    KnobPtr knob = _knob.lock();

    if (!knob) {
        return;
    }
    for (int i = 0; i < knob->getDimension(); ++i) {
        try {
            knob->setExpression(i, _oldExprs[i], _hadRetVar[i], false);
        } catch (...) {
            Dialogs::errorDialog( tr("Expression").toStdString(), tr("The expression is invalid.").toStdString() );
            break;
        }
    }

    knob->evaluateValueChange(_dimension == -1 ? 0 : _dimension, knob->getCurrentTime(), ViewIdx(0), eValueChangedReasonNatronGuiEdited);
    setText( tr("Set expression") );
}

void
SetExpressionCommand::redo()
{
    KnobPtr knob = _knob.lock();

    if (!knob) {
        return;
    }
    if (_dimension == -1) {
        for (int i = 0; i < knob->getDimension(); ++i) {
            try {
                knob->setExpression(i, _newExpr, _hasRetVar, false);
            } catch (...) {
                Dialogs::errorDialog( tr("Expression").toStdString(), tr("The expression is invalid.").toStdString() );
                break;
            }
        }
    } else {
        try {
            knob->setExpression(_dimension, _newExpr, _hasRetVar, false);
        } catch (...) {
            Dialogs::errorDialog( tr("Expression").toStdString(), tr("The expression is invalid.").toStdString() );
        }
    }
    knob->evaluateValueChange(_dimension == -1 ? 0 : _dimension, knob->getCurrentTime(), ViewIdx(0), eValueChangedReasonNatronGuiEdited);
    setText( tr("Set expression") );
}

NATRON_NAMESPACE_EXIT
