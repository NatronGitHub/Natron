/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2020 The Natron developers
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

#include "Engine/KnobTypes.h"
#include "Engine/KnobFile.h"
#include "Engine/Project.h"
#include "Engine/Node.h"
#include "Engine/TimeLine.h"
#include "Engine/AppInstance.h"
#include "Engine/ViewIdx.h"


#include "Serialization/CurveSerialization.h"
#include "Serialization/KnobSerialization.h"


NATRON_NAMESPACE_ENTER

SetMultiChoiceKnobValueUndoCommand::SetMultiChoiceKnobValueUndoCommand(const KnobChoicePtr& knob,
                                                                       const std::vector<ChoiceOption> &oldSelection,
                                                                       const std::vector<ChoiceOption> &newSelection,
                                                                       ViewSetSpec view,
                                                                       ValueChangedReasonEnum reason)
: _knob(knob)
, _view(view)
, _reason(reason)
, _oldIndices(oldSelection)
, _newIndices(newSelection)
{
    setText(tr("Set Value of %1").arg( QString::fromUtf8( knob->getLabel().c_str() ) ).toStdString());
}

SetMultiChoiceKnobValueUndoCommand::~SetMultiChoiceKnobValueUndoCommand()
{

}

void
SetMultiChoiceKnobValueUndoCommand::undo()
{
    KnobChoicePtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setActiveEntries_multi(_oldIndices, _view, _reason);
}

void
SetMultiChoiceKnobValueUndoCommand::redo()
{
    KnobChoicePtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setActiveEntries_multi(_newIndices, _view, _reason);
}

bool
SetMultiChoiceKnobValueUndoCommand::mergeWith(const UndoCommandPtr& command)
{
    const SetMultiChoiceKnobValueUndoCommand *knobCommand = dynamic_cast<const SetMultiChoiceKnobValueUndoCommand *>(command.get());

    if (!knobCommand) {
        return false;
    }

    KnobChoicePtr knob = knobCommand->_knob.lock();
    if ((_knob.lock() != knob) || (_view != knobCommand->_view)) {
        return false;
    }


    _newIndices = knobCommand->_newIndices;

    return true;
}


struct PasteKnobClipBoardUndoCommandPrivate
{
    // The knob on which we paste
    KnobIWPtr knob;

    // The clipboard type
    KnobClipBoardType type;


    DimSpec fromDimension;
    DimSpec targetDimension;
    ViewSetSpec fromView;
    ViewSetSpec targetView;

    // Original serialization of both knobs
    SERIALIZATION_NAMESPACE::KnobSerializationPtr toKnobSerialization, fromKnobSerialization;

    // We keep a pointer to the knob from which we paste in case we need to setup a link
    KnobIWPtr fromKnob;

    PasteKnobClipBoardUndoCommandPrivate()
    : knob()
    , type(eKnobClipBoardTypeCopyLink)
    , fromDimension(DimSpec::all())
    , targetDimension(DimSpec::all())
    , fromView(0)
    , targetView(0)
    , toKnobSerialization()
    , fromKnobSerialization()
    , fromKnob()
    {
    }
};

PasteKnobClipBoardUndoCommand::PasteKnobClipBoardUndoCommand(const KnobIPtr& knob,
                                                             KnobClipBoardType type,
                                                             DimSpec fromDimension,
                                                             DimSpec targetDimensionIn,
                                                             ViewSetSpec fromView,
                                                             ViewSetSpec targetViewIn,
                                                             const KnobIPtr& fromKnob)
: UndoCommand()
, _imp( new PasteKnobClipBoardUndoCommandPrivate() )
{
    assert(knob && fromKnob);

    // If target view is all but target is not multi-view, convert back to main view
    // Also, if all dimensions are folded, convert to all dimensions
    knob->convertDimViewArgAccordingToKnobState(targetDimensionIn, targetViewIn, &_imp->targetDimension, &_imp->targetView);


    _imp->fromKnob = fromKnob;
    _imp->knob = knob;
    _imp->type = type;
    _imp->fromDimension = fromDimension;
    _imp->fromView = fromView;

    _imp->toKnobSerialization.reset(new SERIALIZATION_NAMESPACE::KnobSerialization);
    knob->toSerialization(_imp->toKnobSerialization.get());
    _imp->fromKnobSerialization.reset(new SERIALIZATION_NAMESPACE::KnobSerialization);
    fromKnob->toSerialization(_imp->fromKnobSerialization.get());


    QString text;
    switch (type) {
    case eKnobClipBoardTypeCopyAnim:
        text = tr("Paste Animation on %1").arg( QString::fromUtf8( knob->getLabel().c_str() ) );
        break;
    case eKnobClipBoardTypeCopyValue:
        text = tr("Paste Value on %1").arg( QString::fromUtf8( knob->getLabel().c_str() ) );
        break;
    case eKnobClipBoardTypeCopyLink:
        text = tr("Link %1 to %2").arg( QString::fromUtf8( fromKnob->getLabel().c_str() ) ).arg( QString::fromUtf8( knob->getLabel().c_str() ) );
        break;
    case eKnobClipBoardTypeCopyExpressionLink:
        text = tr("Link with Expression %1 to %2").arg( QString::fromUtf8( fromKnob->getLabel().c_str() ) ).arg( QString::fromUtf8( knob->getLabel().c_str() ) );
        break;
    case eKnobClipBoardTypeCopyExpressionMultCurveLink:
        text = tr("Set curve(frame)*%1 on %2").arg( QString::fromUtf8( fromKnob->getLabel().c_str() ) ).arg( QString::fromUtf8( knob->getLabel().c_str() ) );
        break;
    }
    setText(text.toStdString());
}

PasteKnobClipBoardUndoCommand::~PasteKnobClipBoardUndoCommand()
{
}

void
PasteKnobClipBoardUndoCommand::undo()
{
    copyFrom(_imp->toKnobSerialization, _imp->knob.lock(), false);
} // undo

void
PasteKnobClipBoardUndoCommand::redo()
{
    copyFrom(_imp->fromKnobSerialization, _imp->fromKnob.lock(), true);
} // undo

void
PasteKnobClipBoardUndoCommand::copyFrom(const SERIALIZATION_NAMESPACE::KnobSerializationPtr& serializedKnob,
                           const KnobIPtr& fromKnob,
                           bool isRedo)
{
    KnobIPtr internalKnob = _imp->knob.lock();
    if (!internalKnob) {
        return;
    }

    // Get the target type
    KnobIntBasePtr isInt = toKnobIntBase( internalKnob );
    KnobBoolBasePtr isBool = toKnobBoolBase( internalKnob );
    KnobDoubleBasePtr isDouble = toKnobDoubleBase( internalKnob );
    KnobStringBasePtr isString = toKnobStringBase( internalKnob );


    // Get view names
    std::vector<std::string> projectViewNames;
    if (internalKnob->getHolder() && internalKnob->getHolder()->getApp()) {
        projectViewNames = internalKnob->getHolder()->getApp()->getProject()->getProjectViewNames();
    }

    // group changes under the same evaluation
    ScopedChanges_RAII changes(internalKnob.get());


    std::list<ViewIdx> targetKnobViews = internalKnob->getViewsList();


    for (std::list<ViewIdx>::const_iterator it = targetKnobViews.begin(); it != targetKnobViews.end(); ++it) {
        if ( ( !_imp->targetView.isAll()) && ( *it != _imp->targetView) ) {
            continue;
        }

        
        for (int i = 0; i < internalKnob->getNDimensions(); ++i) {
            if ( ( !_imp->targetDimension.isAll()) && ( i != _imp->targetDimension) ) {
                continue;
            }

            ViewIdx fromView;
            DimIdx fromDim;

            if ( !_imp->targetDimension.all() && !_imp->fromDimension.isAll() ) {
                fromDim = DimIdx(_imp->fromDimension);
            } else {
                // If the source knob dimension is all or target dimension is all copy dimension to dimension respectively
                fromDim = DimIdx(i);
            }

            if ( !_imp->targetView.isAll() && !_imp->fromView.isAll() ) {
                fromView = ViewIdx(_imp->fromView);
            } else {
                // If the source knob view is all or target view is all copy view to view respectively
                fromView = *it;
            }


            switch (_imp->type) {
                case eKnobClipBoardTypeCopyAnim: {
                    std::string fromViewName;
                    if (fromView >= 0 && fromView < (int)projectViewNames.size()) {
                        fromViewName = projectViewNames[_imp->fromView];
                    } else {
                        fromViewName = "Main";
                    }
                    SERIALIZATION_NAMESPACE::KnobSerialization::PerViewValueSerializationMap::const_iterator foundFromView = serializedKnob->_values.find(fromViewName);
                    if (foundFromView == serializedKnob->_values.end()) {
                        continue;
                    }
                    if (fromDim < 0 || fromDim >= (int) foundFromView->second.size()) {
                        continue;
                    }


                    // Read the curve from the clipboard
                    CurvePtr fromCurve(new Curve());
                    

                    if (!foundFromView->second[_imp->fromDimension]._animationCurve.keys.empty()) {
                        fromCurve->fromSerialization(foundFromView->second[_imp->fromDimension]._animationCurve);
                    }


                    internalKnob->cloneCurve(*it, DimIdx(i), *fromCurve, 0 /*offset*/, 0 /*range*/);

                    break;
                }
                case eKnobClipBoardTypeCopyValue: {
                    std::string fromViewName;
                    if (fromView >= 0 && fromView < (int)projectViewNames.size()) {
                        fromViewName = projectViewNames[_imp->fromView];
                    } else {
                        fromViewName = "Main";
                    }
                    SERIALIZATION_NAMESPACE::KnobSerialization::PerViewValueSerializationMap::const_iterator foundFromView = serializedKnob->_values.find(fromViewName);
                    if (foundFromView == serializedKnob->_values.end()) {
                        continue;
                    }
                    if (fromDim < 0 || fromDim >= (int) foundFromView->second.size()) {
                        continue;
                    }

                    internalKnob->restoreValueFromSerialization(foundFromView->second[fromDim], DimIdx(i), *it);

                    break;
                }
                case eKnobClipBoardTypeCopyLink: {
                    if (isRedo) {
                        internalKnob->linkTo(fromKnob, DimIdx(i), fromDim, *it, fromView);
                    } else {
                        internalKnob->unlink(DimIdx(i), *it, false /*copyState*/);
                    }
                    break;
                }
                case eKnobClipBoardTypeCopyExpressionLink:
                case eKnobClipBoardTypeCopyExpressionMultCurveLink:
                {
                    if (isRedo) {
                        std::string expression = makeLinkExpression(projectViewNames, internalKnob, _imp->type == eKnobClipBoardTypeCopyExpressionMultCurveLink, eExpressionLanguageExprTk, fromKnob, _imp->fromDimension, _imp->fromView, _imp->targetDimension, _imp->targetView);
                        const bool hasRetVar = false;
                        try {
                            // Don't fail if exception is invalid, it should have been tested prior to creating an undo/redo command, otherwise user is going
                            // to hit CTRL-Z and nothing will happen
                            internalKnob->setExpression(DimIdx(i), *it, expression, eExpressionLanguagePython, hasRetVar, /*failIfInvalid*/ false);
                        } catch (...) {
                        }
                    } else { // !isRedo
                        internalKnob->clearExpression(DimIdx(i), *it);
                    } // isRedo
                    break;
                }
            }; // switch
            
        } // for all dimensions
    } // for all views

} // redo

std::string
PasteKnobClipBoardUndoCommand::makeLinkExpression(const std::vector<std::string>& projectViewNames,
                                                  const KnobIPtr& targetKnob,
                                                  bool multCurve,
                                                  ExpressionLanguageEnum language,
                                                  const KnobIPtr& fromKnob,
                                                  DimSpec fromDimension,
                                                  ViewSetSpec fromView,
                                                  DimSpec targetDimension,
                                                  ViewSetSpec targetView)
{
    EffectInstancePtr fromEffect = toEffectInstance( fromKnob->getHolder() );
    EffectInstancePtr toEffect = toEffectInstance( targetKnob->getHolder() );
    assert(fromEffect && toEffect);
    if (!fromEffect || !toEffect) {
        return std::string();
    }

    std::stringstream ss;
    if (fromEffect == toEffect) {
        // Same node, use thisNode
        ss << "thisNode.";
    } else {
        // If the container of the effect is a group, prepend thisGroup, otherwise use
        // the canonical app prefix
        NodeGroupPtr isEffectContainerGroup;
        {
            NodeCollectionPtr effectContainer = fromEffect->getNode()->getGroup();
            isEffectContainerGroup = toNodeGroup(effectContainer);
        }
        if (isEffectContainerGroup) {
            ss << "thisGroup.";
        } else {
            if (language == eExpressionLanguagePython) {
                ss << fromEffect->getApp()->getAppIDString() << ".";
            } else {
                ss << "app.";
            }
        }
        ss << fromEffect->getNode()->getScriptName_mt_safe() << ".";
    }

    // Call getValue on the fromKnob
    ss << fromKnob->getName();
    switch (language) {
        case eExpressionLanguagePython: {
            ss << ".getValue(";
            if (fromKnob->getNDimensions() > 1) {
                if (fromDimension.isAll()) {
                    ss << "dimension";
                } else {
                    ss << fromDimension;
                }
            }
            std::list<ViewIdx> sourceViews = fromKnob->getViewsList();
            if (sourceViews.size() > 1) {
                ss << ", ";
                if (fromView.isAll()) {
                    ss << "view";
                } else {
                    if (fromView >= 0 && fromView < (int)projectViewNames.size()) {
                        ss << projectViewNames[fromView];
                    } else {
                        ss << "Main";
                    }
                }
            }
            ss << ")";

            // Also check if we need to multiply by the target knob's curve
            if (multCurve) {
                ss << " * curve(frame, ";
                if (targetDimension.isAll()) {
                    ss << "dimension";
                } else {
                    ss << targetDimension;
                }

                std::list<ViewIdx> targetKnobViews = targetKnob->getViewsList();
                if (targetKnobViews.size() > 1) {
                    ss << ", ";
                    if (targetView.isAll()) {
                        ss << "view";
                    } else {
                        
                        if (targetView >= 0 && targetView < (int)projectViewNames.size()) {
                            ss << projectViewNames[targetView];
                        } else {
                            ss << "Main";
                        }
                    }
                }
                
                ss << ")";
            }

        }   break;
        case eExpressionLanguageExprTk: {
            /*std::list<ViewIdx> sourceViews = fromKnob->getViewsList();
            if (sourceViews.size() > 1) {
                if (fromView.isAll()) {
                    ss << "view";
                } else {
                    if (fromView >= 0 && fromView < (int)projectViewNames.size()) {
                        ss << projectViewNames[fromView];
                    } else {
                        ss << "Main";
                    }
                }
            }*/
            if (fromKnob->getNDimensions() > 1) {
                ss << ".";
                if (fromDimension.isAll()) {
                    ss << "dimension";
                } else {
                    ss << fromDimension;
                }
            }

            // Also check if we need to multiply by the target knob's curve
            if (multCurve) {
                ss << " * curve(frame, ";
                if (targetDimension.isAll()) {
                    ss << "dimension";
                } else {
                    ss << targetDimension;
                }

                std::list<ViewIdx> targetKnobViews = targetKnob->getViewsList();
                if (targetKnobViews.size() > 1) {
                    ss << ", ";
                    if (targetView.isAll()) {
                        ss << "view";
                    } else {

                        if (targetView >= 0 && targetView < (int)projectViewNames.size()) {
                            ss << projectViewNames[targetView];
                        } else {
                            ss << "Main";
                        }
                    }
                }

                ss << ")";
            }
        }   break;
    } // switch(language)
        return ss.str();
} // makeLinkExpression



MultipleKnobEditsUndoCommand::MultipleKnobEditsUndoCommand(const KnobIPtr& knob,
                                                           const QString& commandName,
                                                           ValueChangedReasonEnum reason,
                                                           ValueChangedReturnCodeEnum setValueRetCode,
                                                           bool createNew,
                                                           bool setKeyFrame,
                                                           const PerDimViewKeyFramesMap& oldValue,
                                                           const KeyFrame & newValue,
                                                           DimSpec dimension,
                                                           ViewSetSpec view)
    : UndoCommand()
    , knobs()
    , createNew(createNew)
    , firstRedoCalled(false)
{
    assert(knob);
    std::list<ValueToSet>& vlist = knobs[knob];
    vlist.push_back(ValueToSet());

    // Add the new value to set to the list (which may be not empty)
    ValueToSet &v = vlist.back();
    v.newValue = newValue;
    v.dimension = dimension;
    assert(dimension != -1);

    v.setKeyFrame = setKeyFrame;
    v.view = view;
    v.setValueRetCode = setValueRetCode;
    v.reason = reason;
    v.oldValues = oldValue;

    KnobHolderPtr holder = knob->getHolder();
    EffectInstancePtr effect = toEffectInstance(holder);
    QString holderName;
    if (effect) {
        holderName = QString::fromUtf8( effect->getNode()->getLabel().c_str() );
    }

    // Set the command name in the Edit menu
    if (!commandName.isEmpty()) {
        setText(QString(QString::fromUtf8("%1: ").arg(holderName) + commandName).toStdString());
    } else {
        // If no command name passed, make up a generic command name
        setText( tr("%1: Multiple Parameters Edits").arg(holderName).toStdString() );
    }
}

MultipleKnobEditsUndoCommand::~MultipleKnobEditsUndoCommand()
{
}

static ValueChangedReturnCodeEnum setOldValueForDimView(const KnobIPtr& knob,
                                                        ValueChangedReasonEnum reason,
                                                        bool setKeyFrame,
                                                        bool hasChanged,
                                                        ValueChangedReturnCodeEnum retCode,
                                                        DimIdx dim,
                                                        ViewIdx view_i,
                                                        const PerDimViewKeyFramesMap& oldValues)
{
    ValueChangedReturnCodeEnum ret = eValueChangedReturnCodeNothingChanged;
    DimensionViewPair key = {dim, view_i};
    PerDimViewKeyFramesMap::const_iterator foundDimView = oldValues.find(key);
    if (foundDimView == oldValues.end()) {
        return ret;
    }


    const std::list<KeyFrame>& keys = foundDimView->second;

    AnimatingObjectI::SetKeyFrameArgs args;
    args.dimension = dim;
    args.view = view_i;
    args.flags = eSetKeyFrameFlagSetValue | eSetKeyFrameFlagReplaceProperties;
    args.callKnobChangedHandlerEvenIfNothingChanged = hasChanged;
    args.reason = reason;
    assert(keys.size() == 1);
    if (setKeyFrame &&  retCode != eValueChangedReturnCodeKeyframeAdded) {
        ret = knob->setKeyFrame(args, keys.front());
    } else {
        ret = knob->setValueFromKeyFrame(args, keys.front());
    }
    return ret;
}

void
MultipleKnobEditsUndoCommand::undo()
{
    assert( !knobs.empty() );
    KnobHolderPtr holder = knobs.begin()->first.lock()->getHolder();
    ScopedChanges_RAII changes(holder.get());
    for (ParamsMap::iterator it = knobs.begin(); it != knobs.end(); ++it) {

        KnobIPtr knob = it->first.lock();
        if (!knob) {
            continue;
        }

        if (it->second.empty()) {
            continue;
        }

        // All knobs must belong to the same node!
        assert(knob->getHolder() == holder);


        bool hasChanged = false;

        std::list<ValueToSet>::iterator next = it->second.begin();
        ++next;

        if (it->second.size() > 1) {
            // block knobChanged handler for this knob until the last change so we don't clutter the main-thread with useless action calls
            knob->blockValueChanges();
        }

        std::set<int> dimensionsUndone;
        for (std::list<ValueToSet>::iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {

            if (next == it->second.end() && it->second.size() > 1) {
                // Re-enable knobChanged for the last change on this knob
                knob->unblockValueChanges();
            }

            // If we added a keyframe (due to auto-keying or not) remove it
            if (it2->setValueRetCode == eValueChangedReturnCodeKeyframeAdded) {
                knob->deleteValueAtTime(it2->newValue.getTime(), it2->view, it2->dimension, eValueChangedReasonUserEdited);
            }
            // Only set back the first value that was set in the command
            if (dimensionsUndone.find(it2->dimension) != dimensionsUndone.end()) {
                continue;
            }
            dimensionsUndone.insert(it2->dimension);
            int nDims = knob->getNDimensions();
            std::list<ViewIdx> allViews = knob->getViewsList();
            if (it2->dimension.isAll()) {
                for (int i = 0; i < nDims; ++i) {
                    if (it2->view.isAll()) {
                        for (std::list<ViewIdx>::const_iterator it3 = allViews.begin(); it3!=allViews.end(); ++it3) {
                            hasChanged |= setOldValueForDimView(knob, it2->reason, it2->setKeyFrame, hasChanged,  it2->setValueRetCode, DimIdx(i), *it3, it2->oldValues) != eValueChangedReturnCodeNothingChanged;
                        }
                    } else {
                        ViewIdx view_i = knob->checkIfViewExistsOrFallbackMainView(ViewIdx(it2->view));
                        hasChanged |= setOldValueForDimView(knob, it2->reason, it2->setKeyFrame, hasChanged,  it2->setValueRetCode, DimIdx(i), view_i, it2->oldValues) != eValueChangedReturnCodeNothingChanged;
                    }
                }

            } else {
                if (it2->view.isAll()) {
                    for (std::list<ViewIdx>::const_iterator it3 = allViews.begin(); it3!=allViews.end(); ++it3) {
                        hasChanged |= setOldValueForDimView(knob, it2->reason, it2->setKeyFrame, hasChanged,  it2->setValueRetCode, DimIdx(it2->dimension), *it3, it2->oldValues) != eValueChangedReturnCodeNothingChanged;
                    }
                } else {
                    ViewIdx view_i = knob->checkIfViewExistsOrFallbackMainView(ViewIdx(it2->view));
                    hasChanged |= setOldValueForDimView(knob, it2->reason, it2->setKeyFrame,  hasChanged,  it2->setValueRetCode, DimIdx(it2->dimension), view_i, it2->oldValues) != eValueChangedReturnCodeNothingChanged;
                }
            }

            if (next != it->second.end()) {
                ++next;
            }

        }
    } // for all knobs

} // MultipleKnobEditsUndoCommand::undo

void
MultipleKnobEditsUndoCommand::redo()
{
    // The first time, everything is handled within setValue/setValueAtTime already
    if (!firstRedoCalled) {
        firstRedoCalled = true;
        return;
    }

    assert( !knobs.empty() );
    KnobHolderPtr holder = knobs.begin()->first.lock()->getHolder();

    // Make sure we get a single evaluation
    ScopedChanges_RAII changes(holder.get());

    for (ParamsMap::iterator it = knobs.begin(); it != knobs.end(); ++it) {

        KnobIPtr knob = it->first.lock();
        if (!knob) {
            continue;
        }

        if (it->second.empty()) {
            continue;
        }

        // All knobs must belong to the same node!
        assert(knob->getHolder() == holder);
        bool hasChanged = false;

        std::list<ValueToSet>::iterator next = it->second.begin();
        ++next;

        if (it->second.size() > 1) {
            // block knobChanged handler for this knob until the last change so we don't clutter the main-thread with useless action calls
            knob->blockValueChanges();
        }
        for (std::list<ValueToSet>::iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
            if (next == it->second.end() && it->second.size() > 1) {
                // Re-enable knobChanged for the last change on this knob
                knob->unblockValueChanges();
            }
            AnimatingObjectI::SetKeyFrameArgs args;
            args.dimension = it2->dimension;
            args.view = it2->view;
            args.reason = it2->reason;
            args.callKnobChangedHandlerEvenIfNothingChanged = hasChanged;
            if (it2->setKeyFrame) {
                it2->setValueRetCode = knob->setKeyFrame(args, it2->newValue);
            } else {
                it2->setValueRetCode = knob->setValueFromKeyFrame(args, it2->newValue);
            }
            hasChanged |= it2->setValueRetCode != eValueChangedReturnCodeNothingChanged;

            if (next != it->second.end()) {
                ++next;
            }

        }
    }


} // redo

bool
MultipleKnobEditsUndoCommand::mergeWith(const UndoCommandPtr &command)
{
    /*
     * The command in parameter just had its redo() function call and we attempt to merge it with a previous
     * command that has been redone already
     */

    const MultipleKnobEditsUndoCommand *knobCommand = dynamic_cast<const MultipleKnobEditsUndoCommand *>(command.get());

    if (!knobCommand) {
        return false;
    }

    assert(knobs.size() >= 1 && knobCommand->knobs.size() == 1);


    // Check that the holder is the same, otherwise don't merge
    {
        KnobIPtr knob = knobs.begin()->first.lock();
        if (!knob) {
            return false;
        }
        KnobHolderPtr holder = knob->getHolder();
        KnobHolderPtr otherHolder = knobCommand->knobs.begin()->first.lock()->getHolder();
        if (holder != otherHolder) {
            return false;
        }

    }

    // If all knobs are the same between the old and new command, ignore the createNew flag and merge them anyway
    bool ignoreCreateNew = false;
    if ( knobs.size() == knobCommand->knobs.size() ) {

        // Only 1 iteration will be made because the knobCommand in parameter only has 1 knob anyway
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

    // Ok merge it, add parameters to the map
    for (ParamsMap::const_iterator otherIt = knobCommand->knobs.begin(); otherIt != knobCommand->knobs.end(); ++otherIt) {
        ParamsMap::iterator foundExistinKnob =  knobs.find(otherIt->first);
        if ( foundExistinKnob == knobs.end() ) {
            knobs.insert(*otherIt);
        } else {
            // copy the other dimension of that knob which changed and set the dimension to -1 so
            // that subsequent calls to undo() and redo() clone all dimensions at once
            foundExistinKnob->second.insert( foundExistinKnob->second.end(), otherIt->second.begin(), otherIt->second.end() );
        }
    }

    return true;
}

RestoreDefaultsCommand::RestoreDefaultsCommand(const std::list<KnobIPtr> & knobs,
                                               DimSpec targetDim,
                                               ViewSetSpec targetView)
: UndoCommand()
, _targetDim(targetDim)
, _targetView(targetView)
, _knobs()
{
    setText( tr("Restore Default Value(s)").toStdString() );


    for (std::list<KnobIPtr>::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        KnobButtonPtr isButton = toKnobButton(*it);
        KnobPagePtr isPage = toKnobPage(*it);
        KnobSeparatorPtr isSep = toKnobSeparator(*it);
        if (isSep || isPage || (isButton && !isButton->getIsCheckable())) {
            continue;
        }

        SERIALIZATION_NAMESPACE::KnobSerializationPtr s(new SERIALIZATION_NAMESPACE::KnobSerialization());
        (*it)->toSerialization(s.get());
        _serializations.push_back(s);
        _knobs.push_back(*it);

    }
}

void
RestoreDefaultsCommand::undo()
{
    assert( _serializations.size() == _knobs.size() );

    KnobIPtr first = _knobs.front().lock();
    AppInstancePtr app = first->getHolder()->getApp();
    assert(app);
    SERIALIZATION_NAMESPACE::KnobSerializationList::const_iterator itClone = _serializations.begin();
    for (std::list<KnobIWPtr>::const_iterator it = _knobs.begin(); it != _knobs.end(); ++it, ++itClone) {
        KnobIPtr itKnob = it->lock();
        if (!itKnob) {
            continue;
        }
       
        itKnob->fromSerialization(**itClone);
    }

    if ( first->getHolder()->getApp() ) {
        first->getHolder()->getApp()->redrawAllViewers();
    }

}

void
RestoreDefaultsCommand::redo()
{
    KnobIPtr first = _knobs.front().lock();
    AppInstancePtr app;
    KnobHolderPtr holder = first->getHolder();
    EffectInstancePtr isEffect = toEffectInstance(holder);

    ScopedChanges_RAII changes(holder.get());
    if (holder) {
        app = holder->getApp();
    }

     //  First reset all knobs values, this will not call instanceChanged action
    for (std::list<KnobIWPtr>::iterator it = _knobs.begin(); it != _knobs.end(); ++it) {
        KnobIPtr itKnob = it->lock();
        if (!itKnob) {
            continue;
        }
        if ( itKnob->getHolder() ) {
            itKnob->getHolder()->beginChanges();
        }
        itKnob->resetToDefaultValue(_targetDim, _targetView);
        if ( itKnob->getHolder() ) {
            itKnob->getHolder()->endChanges(true);
        }

    }

    //   Call instanceChange on all knobs afterwards to put back the plug-in
    //   in a correct state
    TimeValue time(0);
    if (app) {
        time = TimeValue(app->getTimeLine()->currentFrame());
    }
    for (std::list<KnobIWPtr>::iterator it = _knobs.begin(); it != _knobs.end(); ++it) {
        KnobIPtr itKnob = it->lock();
        if (!itKnob) {
            continue;
        }
        if ( itKnob->getHolder() ) {
            itKnob->getHolder()->onKnobValueChanged_public(itKnob, eValueChangedReasonRestoreDefault, time, ViewIdx(0));
        }
    }

    if ( first->getHolder() ) {
        if ( first->getHolder()->getApp() ) {
            first->getHolder()->getApp()->redrawAllViewers();
        }
    }
} // RestoreDefaultsCommand::redo

static void getOldExprForDimView(const KnobIPtr& knob, DimIdx dim, ViewIdx view, SetExpressionCommand::PerDimViewExprMap* ret)
{
    DimensionViewPair key = {dim, view};
    SetExpressionCommand::Expr& e = (*ret)[key];
    e.expression = knob->getExpression(dim, view);
    e.hasRetVar = knob->isExpressionUsingRetVariable(view, dim);
}

SetExpressionCommand::SetExpressionCommand(const KnobIPtr & knob,
                                           ExpressionLanguageEnum newLang,
                                           bool hasRetVar,
                                           DimSpec dimension,
                                           ViewSetSpec view,
                                           const std::string& expr)
: UndoCommand()
, _knob(knob)
, _oldExprs()
, _newExpr(expr)
, _newLang(newLang)
, _hasRetVar(hasRetVar)
, _dimension(dimension)
, _view(view)
{
    int nDims = knob->getNDimensions();
    std::list<ViewIdx> allViews = knob->getViewsList();
    if (dimension.isAll()) {
        for (int i = 0; i < nDims; ++i) {
            if (view.isAll()) {
                for (std::list<ViewIdx>::const_iterator it = allViews.begin(); it!=allViews.end(); ++it) {
                    getOldExprForDimView(knob, DimIdx(i), *it, &_oldExprs);
                }
            } else {
                getOldExprForDimView(knob, DimIdx(i), ViewIdx(view), &_oldExprs);
            }
        }

    } else {
        if (view.isAll()) {
            for (std::list<ViewIdx>::const_iterator it = allViews.begin(); it!=allViews.end(); ++it) {
                getOldExprForDimView(knob, DimIdx(dimension), *it, &_oldExprs);
            }
        } else {
            getOldExprForDimView(knob, DimIdx(dimension), ViewIdx(view), &_oldExprs);
        }
    }

    setText( tr("Set Expression").toStdString() );


}

static void setOldExprForDimView(const KnobIPtr& knob, DimIdx dim, ViewIdx view, const SetExpressionCommand::PerDimViewExprMap& ret)
{
    DimensionViewPair key = {dim, view};
    SetExpressionCommand::PerDimViewExprMap::const_iterator foundDimView = ret.find(key);
    if (foundDimView == ret.end()) {
        return;
    }
    knob->setExpression(dim, view, foundDimView->second.expression, foundDimView->second.language, foundDimView->second.hasRetVar, false /*failIfInvalid*/);

}

void
SetExpressionCommand::undo()
{
    KnobIPtr knob = _knob.lock();
    if (!knob) {
        return;
    }

    ScopedChanges_RAII changes(knob.get());
    try {
        int nDims = knob->getNDimensions();
        std::list<ViewIdx> allViews = knob->getViewsList();
        if (_dimension.isAll()) {
            for (int i = 0; i < nDims; ++i) {
                if (_view.isAll()) {
                    for (std::list<ViewIdx>::const_iterator it = allViews.begin(); it!=allViews.end(); ++it) {
                        setOldExprForDimView(knob, DimIdx(i), *it, _oldExprs);
                    }
                } else {
                    setOldExprForDimView(knob, DimIdx(i), ViewIdx(_view), _oldExprs);
                }
            }

        } else {
            if (_view.isAll()) {
                for (std::list<ViewIdx>::const_iterator it = allViews.begin(); it!=allViews.end(); ++it) {
                    setOldExprForDimView(knob, DimIdx(_dimension), *it, _oldExprs);
                }
            } else {
                setOldExprForDimView(knob, DimIdx(_dimension), ViewIdx(_view), _oldExprs);
            }
        }
    } catch (...) {
        Dialogs::errorDialog( tr("Expression").toStdString(), tr("The expression is invalid.").toStdString() );
    }
}

void
SetExpressionCommand::redo()
{
    KnobIPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    try {
        // Don't fail if exception is invalid, it should have been tested prior to creating an undo/redo command, otherwise user is going
        // to hit CTRL-Z and nothing will happen
        knob->setExpression(_dimension, _view, _newExpr, _newLang, _hasRetVar, /*failIfInvalid*/ false);
    } catch (...) {
        Dialogs::errorDialog( tr("Expression").toStdString(), tr("The expression is invalid.").toStdString() );
    }

}

NATRON_NAMESPACE_EXIT
