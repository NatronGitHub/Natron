/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2022 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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
    KnobSerializationPtr originalSerialization;
    KnobIPtr fromKnob;

    PasteUndoCommandPrivate()
        : knob()
        , type(eKnobClipBoardTypeCopyLink)
        , fromDimension(-1)
        , targetDimension(-1)
        , originalSerialization()
    {
        originalSerialization = std::make_shared<KnobSerialization>();
    }
};

PasteUndoCommand::PasteUndoCommand(const KnobGuiPtr& knob,
                                   KnobClipBoardType type,
                                   int fromDimension,
                                   int targetDimension,
                                   const KnobIPtr& fromKnob)
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
        _imp->originalSerialization = std::make_shared<KnobSerialization>();
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
PasteUndoCommand::copyFrom(const KnobIPtr& serializedKnob,
                           bool isRedo)
{
    KnobIPtr internalKnob = _imp->knob.lock()->getKnob();

    switch (_imp->type) {
    case eKnobClipBoardTypeCopyAnim: {
        internalKnob->beginChanges();
        for (int i = 0; i < internalKnob->getDimension(); ++i) {
            if ( ( _imp->targetDimension == -1) || ( i == _imp->targetDimension) ) {
                CurvePtr fromCurve;
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
        KnobIntBase* isInt = dynamic_cast<KnobIntBase*>( internalKnob.get() );
        KnobBoolBase* isBool = dynamic_cast<KnobBoolBase*>( internalKnob.get() );
        KnobDoubleBase* isDouble = dynamic_cast<KnobDoubleBase*>( internalKnob.get() );
        KnobStringBase* isString = dynamic_cast<KnobStringBase*>( internalKnob.get() );

        KnobIntBase* isFromInt = dynamic_cast<KnobIntBase*>( serializedKnob.get() );
        KnobBoolBase* isFromBool = dynamic_cast<KnobBoolBase*>( serializedKnob.get() );
        KnobDoubleBase* isFromDouble = dynamic_cast<KnobDoubleBase*>( serializedKnob.get() );
        KnobStringBase* isFromString = dynamic_cast<KnobStringBase*>( serializedKnob.get() );

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
    if (createNew) {
        //qDebug() << "new MultipleKnobEditsUndoCommand";
    }
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

    setText( tr("Multiple edits of %1").arg(holderName) );
}

MultipleKnobEditsUndoCommand::~MultipleKnobEditsUndoCommand()
{
}

KnobIPtr
MultipleKnobEditsUndoCommand::createCopyForKnob(const KnobIPtr & originalKnob)
{
    const std::string & typeName = originalKnob->typeName();
    KnobIPtr copy;
    int dimension = originalKnob->getDimension();

    if ( typeName == KnobInt::typeNameStatic() ) {
        copy = std::make_shared<KnobInt>((KnobHolder*)NULL, std::string(), dimension, false);
    } else if ( typeName == KnobBool::typeNameStatic() ) {
        copy = std::make_shared<KnobBool>((KnobHolder*)NULL, std::string(), dimension, false);
    } else if ( typeName == KnobDouble::typeNameStatic() ) {
        copy = std::make_shared<KnobDouble>((KnobHolder*)NULL, std::string(), dimension, false);
    } else if ( typeName == KnobChoice::typeNameStatic() ) {
        copy = std::make_shared<KnobChoice>((KnobHolder*)NULL, std::string(), dimension, false);
    } else if ( typeName == KnobString::typeNameStatic() ) {
        copy = std::make_shared<KnobString>((KnobHolder*)NULL, std::string(), dimension, false);
    } else if ( typeName == KnobParametric::typeNameStatic() ) {
        copy = std::make_shared<KnobParametric>((KnobHolder*)NULL, std::string(), dimension, false);
    } else if ( typeName == KnobColor::typeNameStatic() ) {
        copy = std::make_shared<KnobColor>((KnobHolder*)NULL, std::string(), dimension, false);
    } else if ( typeName == KnobPath::typeNameStatic() ) {
        copy = std::make_shared<KnobPath>((KnobHolder*)NULL, std::string(), dimension, false);
    } else if ( typeName == KnobFile::typeNameStatic() ) {
        copy = std::make_shared<KnobFile>((KnobHolder*)NULL, std::string(), dimension, false);
    } else if ( typeName == KnobOutputFile::typeNameStatic() ) {
        copy = std::make_shared<KnobOutputFile>((KnobHolder*)NULL, std::string(), dimension, false);
    }

    ///If this is another type of knob this is wrong since they do not hold any value
    assert(copy);
    if (!copy) {
        return KnobIPtr();
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
        KnobIPtr knob = knobUI->getKnob();
        if (!knob) {
            continue;
        }
        //qDebug() << "Undo" << knob->getName().c_str();
        knob->beginChanges();
        KnobIntBase* isInt = dynamic_cast<KnobIntBase*>( knob.get() );
        KnobBoolBase* isBool = dynamic_cast<KnobBoolBase*>( knob.get() );
        KnobDoubleBase* isDouble = dynamic_cast<KnobDoubleBase*>( knob.get() );
        KnobStringBase* isString = dynamic_cast<KnobStringBase*>( knob.get() );
        KeyFrame k;
        // For setValue calls, only set back the old value once per-dimension
        std::set<int> dimensionsUndone;
        for (std::list<ValueToSet>::iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
            KnobHelper::ValueChangedReturnCodeEnum retCode = (KnobHelper::ValueChangedReturnCodeEnum)it2->setValueRetCode;

            if (retCode == KnobHelper::eValueChangedReturnCodeKeyframeAdded) {
                //qDebug() << "Undo " << knob->getName().c_str() << "[" << it2->dimension << "] remove keyFrame";
                knobUI->removeKeyFrame( it2->time, it2->dimension, ViewIdx(0) );
            } else {
                if (it2->setKeyFrame) {
                    bool refreshGui =  it2->time == knob->getHolder()->getApp()->getTimeLine()->currentFrame();
                    if (isInt) {
                        //qDebug() << "Undo " << knob->getName().c_str() << "[" << it2->dimension << "] to " << it2->oldValue.toInt() << "at time" << it2->time;
                        knobUI->setValueAtTime<int>(it2->dimension, it2->oldValue.toInt(), it2->time, ViewIdx(0), &k, refreshGui, _reason);
                    } else if (isBool) {
                        //qDebug() << "Undo " << knob->getName().c_str() << "[" << it2->dimension << "] to " << it2->oldValue.toBool() << "at time" << it2->time;
                        knobUI->setValueAtTime<bool>(it2->dimension, it2->oldValue.toBool(), it2->time, ViewIdx(0), &k, refreshGui, _reason);
                    } else if (isDouble) {
                        //qDebug() << "Undo " << knob->getName().c_str() << "[" << it2->dimension << "] to " << it2->oldValue.toDouble() << "at time" << it2->time;
                        knobUI->setValueAtTime<double>(it2->dimension, it2->oldValue.toDouble(), it2->time, ViewIdx(0), &k, refreshGui, _reason);
                    } else if (isString) {
                        //qDebug() << "Undo " << knob->getName().c_str() << "[" << it2->dimension << "] to " << it2->oldValue.toString() << "at time" << it2->time;
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
                        int oldValue = it2->oldValue.toInt();
                        if (isInt->getValueAtTime(it2->time, it2->dimension) != oldValue) {
                            //qDebug() << "Undo" << knob->getName().c_str() << "[" << it2->dimension << "] to " << it2->oldValue.toInt();
                            knobUI->setValue<int>(it2->dimension, oldValue, &k, true, _reason);
                        }
                    } else if (isBool) {
                        bool oldValue = it2->oldValue.toBool();
                        if (isBool->getValueAtTime(it2->time, it2->dimension) != oldValue) {
                            //qDebug() << "Undo" << knob->getName().c_str() << "[" << it2->dimension << "] to " << it2->oldValue.toBool();
                            knobUI->setValue<bool>(it2->dimension, oldValue, &k, true, _reason);
                        }
                    } else if (isDouble) {
                        double oldValue = it2->oldValue.toDouble();
                        if (isDouble->getValueAtTime(it2->time, it2->dimension) != oldValue) {
                            //qDebug() << "Undo" << knob->getName().c_str() << "[" << it2->dimension << "] to " << it2->oldValue.toDouble();
                            knobUI->setValue<double>(it2->dimension, oldValue, &k, true, _reason);
                        }
                    } else if (isString) {
                        std::string oldValue = it2->oldValue.toString().toStdString();
                        if (isString->getValueAtTime(it2->time, it2->dimension) != oldValue) {
                            //qDebug() << "Undo" << knob->getName().c_str() << "[" << it2->dimension << "] to " << it2->oldValue.toString();
                            knobUI->setValue<std::string>(it2->dimension, oldValue,
                                                          &k, true, _reason);
                        }
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
        KnobIPtr knob = knobUI->getKnob();
        if (!knob) {
            continue;
        }
        //qDebug() << "redo: knob" << knob->getName().c_str();
        knob->beginChanges();
        KnobIntBase* isInt = dynamic_cast<KnobIntBase*>( knob.get() );
        KnobBoolBase* isBool = dynamic_cast<KnobBoolBase*>( knob.get() );
        KnobDoubleBase* isDouble = dynamic_cast<KnobDoubleBase*>( knob.get() );
        KnobStringBase* isString = dynamic_cast<KnobStringBase*>( knob.get() );

        for (std::list<ValueToSet>::iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
            //qDebug() << "redo: value change" << it2->dimension << it2->time << it2->setKeyFrame << "new:" << it2->newValue << "old:" << it2->oldValue;
            KeyFrame k;
            if (!firstRedoCalled) {
                //qDebug() << "redo: !firstRedoCalled (not the first redo), setting old value";
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
                    int newValue = it2->newValue.toInt();
                    if (isInt->getValueAtTime(it2->time, it2->dimension) != newValue) {
                        it2->setValueRetCode = knobUI->setValue<int>(it2->dimension, newValue, &k, true, _reason);
                    }
                } else if (isBool) {
                    bool newValue = it2->newValue.toBool();
                    if (isBool->getValueAtTime(it2->time, it2->dimension) != newValue) {
                        it2->setValueRetCode = knobUI->setValue<bool>(it2->dimension, newValue, &k, true, _reason);
                    }
                } else if (isDouble) {
                    double newValue = it2->newValue.toDouble();
                    if (isDouble->getValueAtTime(it2->time, it2->dimension) != newValue) {
                        it2->setValueRetCode = knobUI->setValue<double>(it2->dimension, newValue, &k, true, _reason);
                    }
                } else if (isString) {
                    std::string newValue = it2->newValue.toString().toStdString();
                    if (isString->getValueAtTime(it2->time, it2->dimension) != newValue) {
                        it2->setValueRetCode = knobUI->setValue<std::string>(it2->dimension, newValue, &k, true, _reason);
                    }
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
    //qDebug() << "redo: firstRedoCalled=true";
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
        //qDebug() << "mergeWith failed: knobCommand=" << bool(knobCommand) << "id=" << command->id() << "!=" << id();
        return false;
    }

    // if all knobs from the new command are included in the old command, ignore the createNew flag and merge them anyway
    if (knobCommand->createNew) {
        bool ignoreCreateNew = false;
        //qDebug() << "mergeWith" << knobs.size() << "with" << knobCommand->knobs.size();
        if ( knobs.size() >= knobCommand->knobs.size() ) {
            ParamsMap::const_iterator thisIt = knobs.begin();
            ParamsMap::const_iterator otherIt = knobCommand->knobs.begin();
            bool oneDifferent = false;
            for (; thisIt != knobs.end() && otherIt != knobCommand->knobs.end(); ++thisIt) {
                KnobGuiPtr thisItKey = thisIt->first.lock();
                KnobGuiPtr otherItKey = otherIt->first.lock();

                if (otherIt->first.lock() < thisIt->first.lock()) {
                    //qDebug() << thisItKey->getKnob()->getName().c_str() << "!=" << otherItKey->getKnob()->getName().c_str();
                    
                    oneDifferent = true;
                    break;
                } else if (!(thisIt->first.lock() < otherIt->first.lock())) {
                    //qDebug() << thisItKey->getKnob()->getName().c_str() << "==" << otherItKey->getKnob()->getName().c_str();
                    ++otherIt;
                }
            }
            if (!oneDifferent) {
                ignoreCreateNew = true;
            }
        }
        
        if (!ignoreCreateNew) {
            //qDebug() << "mergeWith failed";
            return false;
        }
    }
    //qDebug() << "mergeWith: list of existing knob/values:";
    //for (ParamsMap::const_iterator thisIt = knobs.begin(); thisIt != knobs.end(); ++thisIt) {
    //    qDebug() << "mergeWith existing knob" << thisIt->first.lock()->getKnob()->getName().c_str();
    //    for (std::list<ValueToSet>::const_iterator vit = thisIt->second.begin();
    //         vit != thisIt->second.end(); ++vit) {
    //        qDebug() << "mergeWith: existing value change" << vit->dimension << vit->time << vit->setKeyFrame << "new:" << vit->newValue << "old:" << vit->oldValue;
    //    }
    //
    //}
    
    for (ParamsMap::const_iterator otherIt = knobCommand->knobs.begin(); otherIt != knobCommand->knobs.end(); ++otherIt) {
        ParamsMap::iterator foundExistinKnob =  knobs.find(otherIt->first);
        if ( foundExistinKnob == knobs.end() ) {
            //qDebug() << "mergeWith: adding knob" << otherIt->first.lock()->getKnob()->getName().c_str();

            knobs.insert(*otherIt);
        } else {
            //qDebug() << "mergeWith: adding values to knob" << otherIt->first.lock()->getKnob()->getName().c_str();
            // Merge the value changes with the existing ones:
            // for each ValueToSet v in otherIt->second,
            // if there is a ValueToSet w in foundExistinKnob->second such that
            // - v.dimension == w.dimension
            // - v.time == w.time
            // - v.setKeyFrame == w.setKeyFrame
            // - v.oldValue == w.newValue (optional)
            // then set w.newValue = v.newValue
            // else add new value change: foundExistinKnob->second.push_back(v)
            for (std::list<ValueToSet>::const_iterator vit = otherIt->second.begin();
                 vit != otherIt->second.end(); ++vit) {
                //qDebug() << "mergeWith: adding value change" << vit->dimension << vit->time << vit->setKeyFrame << "new:" << vit->newValue << "old:" << vit->oldValue;
                bool found = false;
                for (std::list<ValueToSet>::iterator wit = foundExistinKnob->second.begin();
                     !found && wit != foundExistinKnob->second.end(); ++wit) {
                    //qDebug() << "mergeWith: testing compat with value change" << wit->dimension << wit->time << wit->setKeyFrame << "new:" << wit->newValue << "old:" << wit->oldValue;
                    if (vit->dimension == wit->dimension &&
                        vit->time == wit->time &&
                        vit->setKeyFrame == wit->setKeyFrame) {
                        //qDebug() << "mergeWith: update value for dim" << vit->dimension << "of knob" << foundExistinKnob->first.lock()->getKnob()->getName().c_str();
                        if (vit->oldValue != wit->newValue) {
                            qDebug() << "mergeWith: warning: old=" << vit->oldValue << "!= new=" << wit->newValue;
                        }
                        if (wit->newValue != vit->newValue) {
                            //qDebug() << "mergeWith: update newValue" << wit->newValue << "->" << vit->newValue << "oldValue remains" << wit->oldValue;
                            wit->newValue = vit->newValue;
                        }
                        found = true;
                    } else {
                        //qDebug() << "mergeWith incompatible value change";
                    }
                }
                if (!found) {
                    //qDebug() << "mergeWith did not find dim" << vit->dimension << "of knob" << foundExistinKnob->first.lock()->getKnob()->getName().c_str() << "-> adding it";
                    foundExistinKnob->second.push_back(*vit);
                }
            }
        }
    }

    //qDebug() << "mergeWith succeeded";
    return true;
}

RestoreDefaultsCommand::RestoreDefaultsCommand(bool isNodeReset,
                                               const std::list<KnobIPtr> & knobs,
                                               int targetDim,
                                               QUndoCommand *parent)
    : QUndoCommand(parent)
    , _isNodeReset(isNodeReset)
    , _targetDim(targetDim)
    , _knobs()
{
    for (std::list<KnobIPtr>::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        _knobs.push_back(*it);
        _clones.push_back( MultipleKnobEditsUndoCommand::createCopyForKnob(*it) );
    }

    KnobIPtr first = _knobs.front().lock();
    KnobHolder* holder = first ? first->getHolder() : nullptr;
    EffectInstance* effect = dynamic_cast<EffectInstance*>(holder);
    QString holderName;
    if (effect) {
        holderName = QString::fromUtf8( effect->getNode()->getLabel().c_str() );
    }

    if (_knobs.size() == 1) {
        setText( tr("Reset %1.%2 to default").arg(holderName).arg( QString::fromUtf8( first->getLabel().c_str() ) ) );
    } else {
        setText( tr("Reset %1 to default").arg(holderName) );
    }
}

void
RestoreDefaultsCommand::undo()
{
    assert( _clones.size() == _knobs.size() );

    std::list<SequenceTime> times;
    KnobIPtr first = _knobs.front().lock();
    KnobHolder* holder = first ? first->getHolder() : nullptr;
    AppInstancePtr app;
    if (holder) {
        app = holder->getApp();
    }
    assert(app);
    std::list<KnobIPtr>::iterator itClone = _clones.begin();
    for (std::list<KnobIWPtr>::const_iterator it = _knobs.begin(); it != _knobs.end(); ++it, ++itClone) {
        KnobIPtr itKnob = it->lock();
        if (!itKnob) {
            // The Knob probably doesn't exist anymore.
            // We won't need the clone, so we may as well release it (we own it).
            itClone->reset();

            continue;
        }
        if (!*itClone) {
            assert(false); // We own the clone, so it should always be valid

            continue;
        }
        itKnob->cloneAndUpdateGui( itClone->get() );

        if ( itKnob->getHolder()->getApp() ) {
            int dim = itKnob->getDimension();
            for (int i = 0; i < dim; ++i) {
                if ( (i == _targetDim) || (_targetDim == -1) ) {
                    CurvePtr c = itKnob->getCurve(ViewIdx(0), i);
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

    holder->incrHashAndEvaluate(true, true);
    if ( app ) {
        app->redrawAllViewers();
    }
}

void
RestoreDefaultsCommand::redo()
{
    std::list<SequenceTime> times;
    KnobIPtr first = _knobs.front().lock();
    AppInstancePtr app;
    KnobHolder* holder = first ? first->getHolder() : nullptr;
    EffectInstance* isEffect = dynamic_cast<EffectInstance*>(holder);

    if (holder) {
        app = holder->getApp();
        holder->beginChanges();
    }


    /*
       First reset all knobs values, this will not call instanceChanged action
     */
    for (std::list<KnobIWPtr>::iterator it = _knobs.begin(); it != _knobs.end(); ++it) {
        KnobIPtr itKnob = it->lock();
        if (!itKnob) {
            continue;
        }
        if ( itKnob->getHolder() && itKnob->getHolder()->getApp() ) {
            int dim = itKnob->getDimension();
            for (int i = 0; i < dim; ++i) {
                if ( (i == _targetDim) || (_targetDim == -1) ) {
                    CurvePtr c = itKnob->getCurve(ViewIdx(0), i);
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
    for (std::list<KnobIWPtr>::iterator it = _knobs.begin(); it != _knobs.end(); ++it) {
        KnobIPtr itKnob = it->lock();
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

    if ( holder ) {
        holder->incrHashAndEvaluate(true, true);
        if ( app ) {
            app->redrawAllViewers();
        }
    }
} // RestoreDefaultsCommand::redo

SetExpressionCommand::SetExpressionCommand(const KnobIPtr & knob,
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
    setText( tr("Set expression") );
}

void
SetExpressionCommand::undo()
{
    KnobIPtr knob = _knob.lock();

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
}

void
SetExpressionCommand::redo()
{
    KnobIPtr knob = _knob.lock();

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
}

NATRON_NAMESPACE_EXIT
