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

#include "KnobUndoCommand.h"

#include <stdexcept>

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
#include "Engine/ViewIdx.h"

#include "Gui/GuiApplicationManager.h"

#include "Serialization/CurveSerialization.h"
#include "Serialization/KnobSerialization.h"

NATRON_NAMESPACE_ENTER;
struct PasteUndoCommandPrivate
{
    KnobGuiWPtr knob;
    KnobClipBoardType type;
    int fromDimension;
    int targetDimension;
    SERIALIZATION_NAMESPACE::KnobSerializationPtr toKnobSerialization, fromKnobSerialization;
    KnobIWPtr fromKnob;

    PasteUndoCommandPrivate()
        : knob()
        , type(eKnobClipBoardTypeCopyLink)
        , fromDimension(-1)
        , targetDimension(-1)
        , toKnobSerialization()
        , fromKnobSerialization()
        , fromKnob()
    {
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
    _imp->fromKnob = fromKnob;
    _imp->knob = knob;
    _imp->type = type;
    _imp->fromDimension = fromDimension;
    _imp->targetDimension = targetDimension;

    _imp->toKnobSerialization.reset(new SERIALIZATION_NAMESPACE::KnobSerialization);
    knob->getKnob()->toSerialization(_imp->toKnobSerialization.get());
    _imp->fromKnobSerialization.reset(new SERIALIZATION_NAMESPACE::KnobSerialization);
    fromKnob->toSerialization(_imp->fromKnobSerialization.get());


    assert(knob);
    assert( _imp->targetDimension >= -1 && _imp->targetDimension < _imp->knob.lock()->getKnob()->getDimension() );
    assert( _imp->fromDimension >= -1 && _imp->fromDimension < _imp->fromKnobSerialization->_dimension );
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
    copyFrom(_imp->toKnobSerialization, _imp->knob.lock()->getKnob(), false);
} // undo

void
PasteUndoCommand::redo()
{
    copyFrom(_imp->fromKnobSerialization, _imp->fromKnob.lock(), true);
} // undo

void
PasteUndoCommand::copyFrom(const SERIALIZATION_NAMESPACE::KnobSerializationPtr& serializedKnob,
                           const KnobIPtr& fromKnob,
                           bool isRedo)
{
    KnobIPtr internalKnob = _imp->knob.lock()->getKnob();

    switch (_imp->type) {
    case eKnobClipBoardTypeCopyAnim: {
        internalKnob->beginChanges();
        for (int i = 0; i < internalKnob->getDimension(); ++i) {
            if ( ( _imp->targetDimension == -1) || ( i == _imp->targetDimension) ) {
                CurvePtr fromCurve(new Curve());
                if ( ( i == _imp->targetDimension) && ( _imp->fromDimension != -1) ) {
                    if (serializedKnob->_values[_imp->fromDimension]._animationCurve.keys.empty()) {
                        fromCurve->fromSerialization(serializedKnob->_values[_imp->fromDimension]._animationCurve);
                    }
                } else {
                    if (!serializedKnob->_values[i]._animationCurve.keys.empty()) {
                        fromCurve->fromSerialization(serializedKnob->_values[i]._animationCurve);
                    }
                }

                internalKnob->cloneCurve(ViewIdx(0), i, *fromCurve);
            }
        }
        internalKnob->endChanges();
        break;
    }
    case eKnobClipBoardTypeCopyValue: {
        KnobIntBasePtr isInt = toKnobIntBase( internalKnob );
        KnobBoolBasePtr isBool = toKnobBoolBase( internalKnob );
        KnobDoubleBasePtr isDouble = toKnobDoubleBase( internalKnob );
        KnobStringBasePtr isString = toKnobStringBase( internalKnob );

        internalKnob->beginChanges();
        for (int i = 0; i < internalKnob->getDimension(); ++i) {
            int fromDim =  (i == _imp->targetDimension && _imp->fromDimension != -1) ? _imp->fromDimension : i;
            if ( ( _imp->targetDimension == -1) || ( i == _imp->targetDimension) ) {
                internalKnob->restoreValueFromSerialization(serializedKnob->_values[fromDim], i, false);
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
                        internalKnob->slaveTo(i, fromKnob, _imp->fromDimension);
                    } else {
                        internalKnob->slaveTo(i, fromKnob, i);
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

    KnobHolderPtr holder = knob->getKnob()->getHolder();
    EffectInstancePtr effect = toEffectInstance(holder);
    QString holderName;
    if (effect) {
        holderName = QString::fromUtf8( effect->getNode()->getLabel().c_str() );
    }

    setText( tr("Multiple edits for %1").arg(holderName) );
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
        copy = KnobInt::create(KnobHolderPtr(), std::string(), dimension, false);
    } else if ( typeName == KnobBool::typeNameStatic() ) {
        copy = KnobBool::create(KnobHolderPtr(), std::string(), dimension, false);
    } else if ( typeName == KnobDouble::typeNameStatic() ) {
        copy = KnobDouble::create(KnobHolderPtr(), std::string(), dimension, false);
    } else if ( typeName == KnobChoice::typeNameStatic() ) {
        copy = KnobChoice::create(KnobHolderPtr(), std::string(), dimension, false);
    } else if ( typeName == KnobString::typeNameStatic() ) {
        copy = KnobString::create(KnobHolderPtr(), std::string(), dimension, false);
    } else if ( typeName == KnobParametric::typeNameStatic() ) {
        copy = KnobParametric::create(KnobHolderPtr(), std::string(), dimension, false);
    } else if ( typeName == KnobColor::typeNameStatic() ) {
        copy = KnobColor::create(KnobHolderPtr(), std::string(), dimension, false);
    } else if ( typeName == KnobPath::typeNameStatic() ) {
        copy = KnobPath::create(KnobHolderPtr(), std::string(), dimension, false);
    } else if ( typeName == KnobFile::typeNameStatic() ) {
        copy = KnobFile::create(KnobHolderPtr(), std::string(), dimension, false);
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
    KnobHolderPtr holder = knobs.begin()->first.lock()->getKnob()->getHolder();
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
        knob->beginChanges();
        KnobIntBasePtr isInt = toKnobIntBase(knob);
        KnobBoolBasePtr isBool = toKnobBoolBase(knob);
        KnobDoubleBasePtr isDouble = toKnobDoubleBase(knob);
        KnobStringBasePtr isString = toKnobStringBase(knob);
        KeyFrame k;
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
    KnobHolderPtr holder = knobs.begin()->first.lock()->getKnob()->getHolder();
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
        knob->beginChanges();
        KnobIntBasePtr isInt = toKnobIntBase(knob);
        KnobBoolBasePtr isBool = toKnobBoolBase(knob);
        KnobDoubleBasePtr isDouble = toKnobDoubleBase(knob);
        KnobStringBasePtr isString = toKnobStringBase(knob);

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
                                               const std::list<KnobIPtr > & knobs,
                                               int targetDim,
                                               QUndoCommand *parent)
    : QUndoCommand(parent)
    , _isNodeReset(isNodeReset)
    , _targetDim(targetDim)
    , _knobs()
{
    setText( tr("Restore default value(s)") );

    
    for (std::list<KnobIPtr >::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        KnobButtonPtr isButton = toKnobButton(*it);
        KnobPagePtr isPage = toKnobPage(*it);
        KnobSeparatorPtr isSep = toKnobSeparator(*it);
        if (isSep || isPage || (isButton && !isButton->getIsCheckable())) {
            continue;
        }
        _knobs.push_front(*it);
        SERIALIZATION_NAMESPACE::KnobSerializationPtr s(new SERIALIZATION_NAMESPACE::KnobSerialization());
        (*it)->toSerialization(s.get());
        _serializations.push_back(s);
    }
}

void
RestoreDefaultsCommand::undo()
{
    assert( _serializations.size() == _knobs.size() );

    std::list<SequenceTime> times;
    KnobIPtr first = _knobs.front().lock();
    AppInstancePtr app = first->getHolder()->getApp();
    assert(app);
    SERIALIZATION_NAMESPACE::KnobSerializationList::const_iterator itClone = _serializations.begin();
    for (std::list<KnobIWPtr >::const_iterator it = _knobs.begin(); it != _knobs.end(); ++it, ++itClone) {
        KnobIPtr itKnob = it->lock();
        if (!itKnob) {
            continue;
        }
       
        itKnob->fromSerialization(**itClone);

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

    first->getHolder()->invalidateCacheHashAndEvaluate(true, true);
    if ( first->getHolder()->getApp() ) {
        first->getHolder()->getApp()->redrawAllViewers();
    }

}

void
RestoreDefaultsCommand::redo()
{
    std::list<SequenceTime> times;
    KnobIPtr first = _knobs.front().lock();
    AppInstancePtr app;
    KnobHolderPtr holder = first->getHolder();
    EffectInstancePtr isEffect = toEffectInstance(holder);

    if (holder) {
        app = holder->getApp();
        holder->beginChanges();
    }


    /*
       First reset all knobs values, this will not call instanceChanged action
     */
    for (std::list<KnobIWPtr >::iterator it = _knobs.begin(); it != _knobs.end(); ++it) {
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
    for (std::list<KnobIWPtr >::iterator it = _knobs.begin(); it != _knobs.end(); ++it) {
        KnobIPtr itKnob = it->lock();
        if (!itKnob) {
            continue;
        }
        if ( itKnob->getHolder() ) {
            itKnob->getHolder()->onKnobValueChanged_public(itKnob, eValueChangedReasonRestoreDefault, time, ViewIdx(0), true);
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
        first->getHolder()->invalidateCacheHashAndEvaluate(true, true);
        if ( first->getHolder()->getApp() ) {
            first->getHolder()->getApp()->redrawAllViewers();
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
    setText( tr("Set expression") );
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
    setText( tr("Set expression") );
}

NATRON_NAMESPACE_EXIT;
