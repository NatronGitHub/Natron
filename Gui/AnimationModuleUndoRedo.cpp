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

#include "AnimationModuleUndoRedo.h"

#include <cmath>
#include <stdexcept>
#include <list>
#include <QtCore/QDebug>

#include "Global/GlobalDefines.h"

#include "Gui/AnimationModuleBase.h"
#include "Gui/AnimationModuleSelectionModel.h"
#include "Gui/AnimationModuleView.h"
#include "Gui/CurveGui.h"
#include "Gui/KnobGui.h"
#include "Gui/NodeAnim.h"
#include "Gui/NodeGui.h"
#include "Gui/TableItemAnim.h"

#include "Engine/Bezier.h"
#include "Engine/Knob.h"
#include "Engine/Curve.h"
#include "Engine/EffectInstance.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/KnobTypes.h"
#include "Engine/StringAnimationManager.h"
#include "Engine/Transform.h"
#include "Engine/ViewIdx.h"

NATRON_NAMESPACE_ENTER;


template <typename T>
static void convertVariantTimeValuePairToTypedList(const std::list<VariantTimeValuePair>& inList,
                                                   std::list<TimeValuePair<T> >* outList)
{
    for (std::list<VariantTimeValuePair>::const_iterator it = inList.begin(); it!=inList.end(); ++it) {
        TimeValuePair<T> p(it->time, variantToType<T>(it->value));
        outList->push_back(p);
    }
}

template <typename T>
static void convertVariantKeyStringSetToTypedList(const KeyFrameWithStringSet& inList,
                                                  double offset,
                                                  std::list<TimeValuePair<T> >* outList)
{
    for (KeyFrameWithStringSet::const_iterator it = inList.begin(); it!=inList.end(); ++it) {
        TimeValuePair<T> p(it->key.getTime() + offset, it->key.getValue());
        outList->push_back(p);
    }
}

template <>
void convertVariantKeyStringSetToTypedList(const KeyFrameWithStringSet& inList,
                                           double offset,
                                           std::list<TimeValuePair<std::string> >* outList)
{
    for (KeyFrameWithStringSet::const_iterator it = inList.begin(); it!=inList.end(); ++it) {
        TimeValuePair<std::string> p(it->key.getTime() + offset, it->string);
        outList->push_back(p);
    }
}




static void
removeKeyFrames(const AnimItemDimViewKeyFramesMap& keys, const AnimItemBasePtr dstItem)
{

    for (AnimItemDimViewKeyFramesMap::const_iterator it = keys.begin(); it != keys.end(); ++it) {

        AnimatingObjectIPtr obj;
        if (!dstItem) {
            obj = it->first.item->getInternalAnimItem();
        } else {
            obj = dstItem->getInternalAnimItem();
        }
        if (!obj) {
            continue;
        }

        const KeyFrameWithStringSet& keyStringSet = it->second;

        std::list<double> keyTimes;
        for (KeyFrameWithStringSet ::const_iterator it2 = keyStringSet.begin(); it2 != keyStringSet.end(); ++it2) {
            keyTimes.push_back(it2->key.getTime());
        }
        obj->deleteValuesAtTime(keyTimes, it->first.view, it->first.dim);

    }
}

static void
addKeyFrames(const AnimItemDimViewKeyFramesMap& keys,
             bool clearExisting,
             double offset,
             const AnimItemBasePtr targetItem,
             const DimSpec& targetItemDimension,
             const ViewSetSpec& targetItemView)
{
    for (AnimItemDimViewKeyFramesMap::const_iterator it = keys.begin(); it != keys.end(); ++it) {


        const KeyFrameWithStringSet& keyStringSet = it->second;

        DimSpec dim;
        ViewSetSpec view;
        AnimatingObjectIPtr obj;
        if (!targetItem) {
            dim = it->first.dim;
            view = it->first.view;
            obj = it->first.item->getInternalAnimItem();
        } else {
            dim = targetItemDimension;
            view = targetItemView;
            obj = targetItem->getInternalAnimItem();
        }
        if (!obj) {
            continue;
        }

        if (clearExisting) {
            // Remove all existing animation
            obj->removeAnimation(view, dim);
        }

        AnimatingObjectI::KeyframeDataTypeEnum dataType = obj->getKeyFrameDataType();
        switch (dataType) {
            case AnimatingObjectI::eKeyframeDataTypeNone:
            case AnimatingObjectI::eKeyframeDataTypeDouble:
            {
                std::list<DoubleTimeValuePair> keysList;
                convertVariantKeyStringSetToTypedList<double>(keyStringSet, offset, &keysList);
                obj->setMultipleDoubleValueAtTime(keysList, view, dim);
            }   break;
            case AnimatingObjectI::eKeyframeDataTypeBool:
            {
                std::list<BoolTimeValuePair> keysList;
                convertVariantKeyStringSetToTypedList<bool>(keyStringSet, offset, &keysList);
                obj->setMultipleBoolValueAtTime(keysList, view, dim);

            }   break;
            case AnimatingObjectI::eKeyframeDataTypeString:
            {
                std::list<StringTimeValuePair> keysList;
                convertVariantKeyStringSetToTypedList<std::string>(keyStringSet, offset, &keysList);
                obj->setMultipleStringValueAtTime(keysList, view, dim);

            }   break;
            case AnimatingObjectI::eKeyframeDataTypeInt:
            {
                std::list<IntTimeValuePair> keysList;
                convertVariantKeyStringSetToTypedList<int>(keyStringSet, offset, &keysList);
                obj->setMultipleIntValueAtTime(keysList, view, dim);


            }   break;
        } // end switch


    }
} // addKeyFrames



static void animItemDimViewCreateOldCurve(const AnimItemBasePtr& item, const DimIdx& dim, const ViewIdx& view, ItemDimViewCurveSet* oldCurves) {

    CurvePtr curve = item->getCurve(dim, view);
    if (!curve) {
        return;
    }
    AnimItemDimViewIndexIDWithCurve key;
    key.key.item = item;
    key.key.view = view;
    key.key.dim = dim;
    key.oldCurveState.reset(new Curve);
    key.oldCurveState->clone(*curve);
    oldCurves->insert(key);

}

static void animItemDimViewSpecCreateOldCurve(const AnimItemBasePtr& item, const DimSpec& dim, const ViewSetSpec& view, ItemDimViewCurveSet* oldCurves) {

    if (dim.isAll()) {
        int nDims = item->getNDimensions();
        if (view.isAll()) {
            std::list<ViewIdx> viewsList = item->getViewsList();
            for (std::list<ViewIdx>::const_iterator it = viewsList.begin(); it != viewsList.end(); ++it) {
                for (int i = 0; i < nDims; ++i) {
                    animItemDimViewCreateOldCurve(item, DimIdx(i), *it, oldCurves);
                }
            }
        } else {
            for (int i = 0; i < nDims; ++i) {
                animItemDimViewCreateOldCurve(item, DimIdx(i), ViewIdx(view), oldCurves);
            }
        }

    } else {
        if (view.isAll()) {
            std::list<ViewIdx> viewsList = item->getViewsList();
            for (std::list<ViewIdx>::const_iterator it = viewsList.begin(); it != viewsList.end(); ++it) {
                animItemDimViewCreateOldCurve(item, DimIdx(dim), *it, oldCurves);
            }
        } else {
            animItemDimViewCreateOldCurve(item, DimIdx(dim), ViewIdx(view), oldCurves);
        }
    }
}

static void animItemDimViewCreateOldCurveSet(const AnimItemDimViewKeyFramesMap& keys, ItemDimViewCurveSet* oldCurves)
{
    for (AnimItemDimViewKeyFramesMap::const_iterator it = keys.begin(); it != keys.end(); ++it) {
        animItemDimViewCreateOldCurve(it->first.item, it->first.dim, it->first.view, oldCurves);
    }
}

static void keysWithOldCurveSetClone(const ItemDimViewCurveSet& oldCurves)
{
    for (ItemDimViewCurveSet::const_iterator it = oldCurves.begin(); it != oldCurves.end(); ++it) {
        AnimatingObjectIPtr obj = it->key.item->getInternalAnimItem();
        if (!obj) {
            continue;
        }

        CurvePtr curve = obj->getAnimationCurve(it->key.view, it->key.dim);
        if (!curve) {
            continue;
        }

        // Clone the old curve state
        obj->cloneCurve(it->key.view, it->key.dim, *it->oldCurveState, 0 /*offset*/, 0 /*range*/, 0 /*stringAnimation*/);
    }
}


AddKeysCommand::AddKeysCommand(const AnimItemDimViewKeyFramesMap & keys,
                               const AnimationModuleBasePtr& model,
                               bool replaceExistingAnimation,
                               QUndoCommand *parent)
: QUndoCommand(parent)
, _model(model)
, _replaceExistingAnimation(replaceExistingAnimation)
, _keys(keys)
, _isFirstRedo(true)
{
    animItemDimViewCreateOldCurveSet(_keys, &_oldCurves);
    setText( tr("Add KeyFrame(s)") );
}

void
AddKeysCommand::undo()
{
    keysWithOldCurveSetClone(_oldCurves);
} // undo

void
AddKeysCommand::redo()
{
    addKeyFrames(_keys, _replaceExistingAnimation, 0 /*offset*/, AnimItemBasePtr(), DimSpec(0) /*irrelevant*/, ViewSetSpec(0) /*irrelevant*/);
    if (!_isFirstRedo) {
        AnimationModuleBasePtr model = _model.lock();
        if (model) {
            model->setCurrentSelection(_keys, std::vector<TableItemAnimPtr>(), std::vector<NodeAnimPtr>());
        }
    }
    _isFirstRedo = false;
} // redo

RemoveKeysCommand::RemoveKeysCommand(const AnimItemDimViewKeyFramesMap & keys,
                                     const AnimationModuleBasePtr& model,
                                     QUndoCommand *parent)
: QUndoCommand(parent)
, _model(model)
, _keys(keys)
{
    animItemDimViewCreateOldCurveSet(_keys, &_oldCurves);
    setText( tr("Remove KeyFrame(s)") );
}

void
RemoveKeysCommand::undo()
{
    keysWithOldCurveSetClone(_oldCurves);
    AnimationModuleBasePtr model = _model.lock();
    if (model) {
        model->setCurrentSelection(_keys, std::vector<TableItemAnimPtr>(), std::vector<NodeAnimPtr>());
    }
} // undo

void
RemoveKeysCommand::redo()
{
    removeKeyFrames(_keys, AnimItemBasePtr());
    AnimationModuleBasePtr model = _model.lock();
    if (model) {
        model->getSelectionModel()->clearSelection();
    }
} // redo


PasteKeysCommand::PasteKeysCommand(const AnimItemDimViewKeyFramesMap & keys,
                                   const AnimationModuleBasePtr& model,
                                   const AnimItemBasePtr& target,
                                   DimSpec targetDim,
                                   ViewSetSpec targetView,
                                   bool pasteRelativeToCurrentTime,
                                   double currentTime,
                                   QUndoCommand *parent)
: QUndoCommand(parent)
, _model(model)
, _offset(0)
, _target(target)
, _targetDim(targetView)
, _targetView(targetView)
, _keys()
{

    animItemDimViewSpecCreateOldCurve(target, targetDim, targetView, &_oldCurves);
    double minSelectedKeyTime(std::numeric_limits<double>::infinity());
    if (pasteRelativeToCurrentTime) {

        for (AnimItemDimViewKeyFramesMap::const_iterator it = keys.begin(); it != keys.end(); ++it) {
            if (it->second.empty()) {
                continue;
            }
            double minTimeForCurve = it->second.begin()->key.getTime();
            minSelectedKeyTime = std::min(minSelectedKeyTime, minTimeForCurve);
        }
        if (minSelectedKeyTime != std::numeric_limits<double>::infinity()) {
            _offset = currentTime - minSelectedKeyTime;
        }

    }

    setText( tr("Paste KeyFrame(s)") );
    
}

void
PasteKeysCommand::undo()
{
    keysWithOldCurveSetClone(_oldCurves);
    AnimationModuleBasePtr model = _model.lock();
    if (model) {
        model->setCurrentSelection(_keys, std::vector<TableItemAnimPtr>(), std::vector<NodeAnimPtr>());
    }
} // undo


void
PasteKeysCommand::redo()
{
    addKeyFrames(_keys, false /*replaceExistingKeys*/, _offset, _target, _targetDim, _targetView);
    AnimationModuleBasePtr model = _model.lock();
    if (model) {
        AnimItemDimViewKeyFramesMap newSelection;
        AnimationModuleSelectionModel::addAnimatedItemKeyframes(_target, _targetDim, _targetView, &newSelection);
        model->setCurrentSelection(newSelection, std::vector<TableItemAnimPtr>(), std::vector<NodeAnimPtr>());
    }
} // redo



static void
moveReader(const NodePtr &reader,
           double dt)
{
    KnobIntBasePtr startingTimeKnob = toKnobIntBase( reader->getKnobByName(kReaderParamNameStartingTime) );
    assert(startingTimeKnob);
    ValueChangedReturnCodeEnum s = startingTimeKnob->setValue(startingTimeKnob->getValue() + dt, ViewSetSpec::all(), DimIdx(0), eValueChangedReasonNatronGuiEdited, 0);
    Q_UNUSED(s);
}

static void
moveTimeOffset(const NodePtr& node,
               double dt)
{
    KnobIntBasePtr timeOffsetKnob = toKnobIntBase( node->getKnobByName(kTimeOffsetParamNameTimeOffset) );
    assert(timeOffsetKnob);
    ValueChangedReturnCodeEnum s = timeOffsetKnob->setValue(timeOffsetKnob->getValue() + dt, ViewSetSpec::all(), DimIdx(0), eValueChangedReasonNatronGuiEdited, 0);
    Q_UNUSED(s);
}

static void
moveFrameRange(const NodePtr& node,
               double dt)
{
    KnobIntBasePtr frameRangeKnob = toKnobIntBase( node->getKnobByName(kFrameRangeParamNameFrameRange) );
    assert(frameRangeKnob);
    std::vector<int> values(2);
    values[0] = frameRangeKnob->getValue(DimIdx(0)) + dt;
    values[1] = frameRangeKnob->getValue(DimIdx(1)) + dt;
    frameRangeKnob->setValueAcrossDimensions(values, DimIdx(0), ViewSetSpec::all(), eValueChangedReasonNatronGuiEdited);
}

static void
moveNodeIfLifetimeActivated(const NodePtr& node,
                            double dt)
{
    KnobBoolPtr lifeTimeEnabledKnob = node->getLifeTimeEnabledKnob();
    if (!lifeTimeEnabledKnob || !lifeTimeEnabledKnob->getValue()) {
        return;
    }
    KnobIntPtr lifeTimeKnob = node->getLifeTimeKnob();
    if (!lifeTimeKnob) {
        return;
    }
    std::vector<int> values(2);
    values[0] = lifeTimeKnob->getValue(DimIdx(0)) + dt;
    values[1] = lifeTimeKnob->getValue(DimIdx(1)) + dt;
    lifeTimeKnob->setValueAcrossDimensions(values, DimIdx(0), ViewSetSpec::all(), eValueChangedReasonNatronGuiEdited);
}

static void
moveGroupNode(const NodePtr& node,
              double dt)
{
    NodeGroupPtr group = node->isEffectNodeGroup();

    assert(group);
    NodesList nodes;
    group->getNodes_recursive(nodes, true);

    for (NodesList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        NodeGuiPtr nodeGui = boost::dynamic_pointer_cast<NodeGui>( (*it)->getNodeGui() );
        assert(nodeGui);
        std::string pluginID = (*it)->getPluginID();
        NodeGroupPtr isChildGroup = (*it)->isEffectNodeGroup();

        // Move readers
        if (pluginID == PLUGINID_NATRON_READ) {
            moveReader(*it, dt);
        } else if (pluginID == PLUGINID_OFX_TIMEOFFSET) {
            moveTimeOffset(*it, dt);
        } else if (pluginID == PLUGINID_OFX_FRAMERANGE) {
            moveFrameRange(*it, dt);
        } else if (isChildGroup) {
            moveGroupNode(*it, dt);
        }

        // Move keyframes
        const KnobsVec &knobs = (*it)->getKnobs();

        for (KnobsVec::const_iterator knobIt = knobs.begin(); knobIt != knobs.end(); ++knobIt) {
            const KnobIPtr& knob = *knobIt;
            if ( !knob->hasAnimation() ) {
                continue;
            }

            std::list<ViewIdx> views = knob->getViewsList();
            for (int dim = 0; dim < knob->getNDimensions(); ++dim) {
                for (std::list<ViewIdx>::const_iterator it2 = views.begin(); it2 != views.end(); ++it2) {
                    CurvePtr curve = knob->getCurve(*it2, DimIdx(dim));
                    if (!curve) {
                        continue;
                    }
                    KeyFrameSet keyframes = curve->getKeyFrames_mt_safe();
                    if (keyframes.empty()) {
                        continue;
                    }
                    std::list<double> keysToMove;
                    for (KeyFrameSet::const_iterator it3 = keyframes.begin(); it3 != keyframes.end(); ++it3) {
                        keysToMove.push_back(it3->getTime());
                    }
                    for (std::list<double> ::iterator kfIt = keysToMove.begin(); kfIt != keysToMove.end(); ++kfIt) {
                        knob->moveValueAtTime(*kfIt, *it2, DimIdx(dim), dt, 0 /*dv*/, 0);
                    }
                }

            }
        }
    }
} // moveGroupNode


WarpKeysCommand::WarpKeysCommand(const AnimItemDimViewKeyFramesMap &keys,
                                 const AnimationModuleBasePtr& model,
                                 const std::vector<NodeAnimPtr >& nodes,
                                 const std::vector<TableItemAnimPtr>& tableItems,
                                 double dt,
                                 double dv,
                                 QUndoCommand *parent )
: QUndoCommand(parent)
, _model(model)
, _keys(keys)
, _nodes(nodes)
, _tableItems(tableItems)
{
    _warp.reset(new Curve::TranslationKeyFrameWarp(dt, dv));
    setText( tr("Move KeyFrame(s)") );
}

WarpKeysCommand::WarpKeysCommand(const AnimItemDimViewKeyFramesMap& keys,
                                 const AnimationModuleBasePtr& model,
                                 const Transform::Matrix3x3& matrix,
                                 QUndoCommand *parent)
: QUndoCommand(parent)
, _model(model)
, _keys(keys)
{
    _warp.reset(new Curve::AffineKeyFrameWarp(matrix));
    setText( tr("Transform KeyFrame(s)") );
}

bool
WarpKeysCommand::testWarpOnKeys(const AnimItemDimViewKeyFramesMap& inKeys, const Curve::KeyFrameWarp& warp)
{
    for (AnimItemDimViewKeyFramesMap::const_iterator it = inKeys.begin(); it!=inKeys.end();++it) {
        AnimatingObjectIPtr obj = it->first.item->getInternalAnimItem();
        if (!obj) {
            continue;
        }
        
        CurvePtr originalCurve = obj->getAnimationCurve(it->first.view, it->first.dim);
        if (!originalCurve) {
            continue;
        }
        
        // Work on a local copy
        Curve tmpCurve;
        tmpCurve.clone(*originalCurve);
        
        
        const KeyFrameWithStringSet& keyStringSet = it->second;
        
        // Make-up keyframe times to warp for this item/view/dim
        std::list<double> keyTimes;
        for (KeyFrameWithStringSet ::const_iterator it2 = keyStringSet.begin(); it2 != keyStringSet.end(); ++it2) {
            keyTimes.push_back(it2->key.getTime());
        }

        
        if (!tmpCurve.transformKeyframesValueAndTime(keyTimes, warp)) {
            return false;
        }
        
    }
    return true;
    
} // testWarpOnKeys

void
WarpKeysCommand::warpKeys()
{

    Curve::TranslationKeyFrameWarp* isTranslation = dynamic_cast<Curve::TranslationKeyFrameWarp*>(_warp.get());
    if (isTranslation) {
        double dt = isTranslation->getDT();
        for (std::vector<NodeAnimPtr >::iterator it = _nodes.begin(); it != _nodes.end(); ++it) {
            AnimatedItemTypeEnum type = (*it)->getItemType();
            if (type == eAnimatedItemTypeReader) {
                moveReader( (*it)->getInternalNode(), dt);
            } else if (type == eAnimatedItemTypeFrameRange) {
                moveFrameRange( (*it)->getInternalNode(), dt );
            } else if (type == eAnimatedItemTypeTimeOffset) {
                moveTimeOffset( (*it)->getInternalNode(), dt );
            } else if (type == eAnimatedItemTypeGroup) {
                moveGroupNode((*it)->getInternalNode(), dt);
            } else if (type == eAnimatedItemTypeCommon) {
                moveNodeIfLifetimeActivated((*it)->getInternalNode(), dt);
            }
        }

        for (std::vector<TableItemAnimPtr>::iterator it = _tableItems.begin(); it != _tableItems.end(); ++it) {
#pragma message WARN("TODO: move lifetime table item")
        }
    }
    
    
    for (AnimItemDimViewKeyFramesMap::iterator it = _keys.begin(); it!=_keys.end();++it) {
        AnimatingObjectIPtr obj = it->first.item->getInternalAnimItem();
        if (!obj) {
            continue;
        }

        const KeyFrameWithStringSet& keyStringSet = it->second;

        // Make-up keyframe times to warp for this item/view/dim
        std::list<double> keyTimes;
        for (KeyFrameWithStringSet ::const_iterator it2 = keyStringSet.begin(); it2 != keyStringSet.end(); ++it2) {
            keyTimes.push_back(it2->key.getTime());
        }

        // Warp keys...
        std::vector<KeyFrame> newKeyframe;
        if (obj->warpValuesAtTime(keyTimes, it->first.view, it->first.dim, *_warp, &newKeyframe)) {

            // Modify original keys by warped keys
            StringAnimationManagerPtr stringAnim = obj->getStringAnimation();
            KeyFrameWithStringSet newKeyStringSet;
            for (std::size_t i = 0; i < newKeyframe.size(); ++i) {
                KeyFrameWithString k;
                k.key = newKeyframe[i];
                if (stringAnim) {
                    stringAnim->stringFromInterpolatedIndex(k.key.getValue(), it->first.view, &k.string);
                }
                newKeyStringSet.insert(k);
            }

            it->second = newKeyStringSet;
        }
    } // for all objects

    AnimationModuleBasePtr model = _model.lock();
    if (model) {
        model->setCurrentSelection(_keys, std::vector<TableItemAnimPtr>(), std::vector<NodeAnimPtr>());
    }

} // warpKeys

void
WarpKeysCommand::undo()
{
    _warp->setWarpInverted(true);
    warpKeys();
}

void
WarpKeysCommand::redo()
{
    _warp->setWarpInverted(false);
    warpKeys();
}

bool
WarpKeysCommand::mergeWith(const QUndoCommand * command)
{
    const WarpKeysCommand* cmd = dynamic_cast<const WarpKeysCommand*>(command);
    if (!cmd || cmd->id() != id()) {
        return false;
    }
    if ( cmd->_keys.size() != _keys.size() ) {
        return false;
    }

    AnimItemDimViewKeyFramesMap::const_iterator itother = cmd->_keys.begin();
    for (AnimItemDimViewKeyFramesMap::const_iterator it = _keys.begin(); it != _keys.end(); ++it, ++itother) {
        if (itother->first.item != it->first.item || itother->first.view != it->first.view || itother->first.dim != it->first.dim) {
            return false;
        }

        if ( itother->second.size() != it->second.size() ) {
            return false;
        }

        KeyFrameWithStringSet::const_iterator itOtherKey = itother->second.begin();
        for (KeyFrameWithStringSet::const_iterator itKey = it->second.begin(); itKey != it->second.end(); ++itKey, ++itOtherKey) {
            if (itKey->key.getTime() != itOtherKey->key.getTime()) {
                return false;
            }
        }
    }
    
    if ( cmd->_nodes.size() != _nodes.size() ) {
        return false;
    }
    
    {
        std::vector<NodeAnimPtr >::const_iterator itOther = cmd->_nodes.begin();
        for (std::vector<NodeAnimPtr >::const_iterator it = _nodes.begin(); it != _nodes.end(); ++it, ++itOther) {
            if (*itOther != *it) {
                return false;
            }
        }
    }
    
    if ( cmd->_tableItems.size() != _tableItems.size() ) {
        return false;
    }
    
    {
        std::vector<TableItemAnimPtr >::const_iterator itOther = cmd->_tableItems.begin();
        for (std::vector<TableItemAnimPtr >::const_iterator it = _tableItems.begin(); it != _tableItems.end(); ++it, ++itOther) {
            if (*itOther != *it) {
                return false;
            }
        }
    }


    return _warp->mergeWith(*cmd->_warp);

} // WarpKeysCommand::mergeWith

int
WarpKeysCommand::id() const
{
    return kCurveEditorMoveMultipleKeysCommandCompressionID;
}

SetKeysInterpolationCommand::SetKeysInterpolationCommand(const AnimItemDimViewKeyFramesMap & keys,
                                                         const AnimationModuleBasePtr& model,
                                                         KeyframeTypeEnum newInterpolation,
                                                         QUndoCommand *parent)
: QUndoCommand(parent)
, _model(model)
, _keys(keys)
, _newInterpolation(newInterpolation)
, _isFirstRedo(true)
{
    animItemDimViewCreateOldCurveSet(_keys, &_oldCurves);
    setText( tr("Set KeyFrame(s) Interpolation") );
    
}


void
SetKeysInterpolationCommand::undo()
{
    keysWithOldCurveSetClone(_oldCurves);
    AnimationModuleBasePtr model = _model.lock();
    if (model) {
        model->setCurrentSelection(_keys, std::vector<TableItemAnimPtr>(), std::vector<NodeAnimPtr>());
    }
}

void
SetKeysInterpolationCommand::redo()
{
    for (AnimItemDimViewKeyFramesMap::iterator it = _keys.begin(); it != _keys.end(); ++it) {
        AnimatingObjectIPtr obj = it->first.item->getInternalAnimItem();
        if (!obj) {
            continue;
        }

        std::list<double> keyTimes;
        for (KeyFrameWithStringSet ::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
            keyTimes.push_back(it2->key.getTime());
        }

        obj->setInterpolationAtTimes(it->first.view, it->first.dim, keyTimes, _newInterpolation);
    }
    if (!_isFirstRedo) {
        AnimationModuleBasePtr model = _model.lock();
        if (model) {
            model->setCurrentSelection(_keys, std::vector<TableItemAnimPtr>(), std::vector<NodeAnimPtr>());
        }
    }
    _isFirstRedo = true;
}


MoveTangentCommand::MoveTangentCommand(const AnimationModuleBasePtr& model,
                                       SelectedTangentEnum deriv,
                                       const AnimItemBasePtr& object,
                                       DimIdx dimension,
                                       ViewIdx view,
                                       const KeyFrame& k,
                                       double dx,
                                       double dy,
                                       QUndoCommand *parent)
: QUndoCommand(parent)
, _model(model)
, _object(object)
, _dimension(dimension)
, _view(view)
, _keyTime(k.getTime())
, _deriv(deriv)
, _oldInterp( k.getInterpolation() )
, _oldLeft( k.getLeftDerivative() )
, _oldRight( k.getRightDerivative() )
, _setBoth(false)
, _isFirstRedo(true)
{
    CurvePtr curve = object->getCurve(dimension,view);
    assert(curve);

    KeyFrameSet keys = curve->getKeyFrames_mt_safe();
    KeyFrameSet::const_iterator cur = keys.find(k);

    assert( cur != keys.end() );

    //find next and previous keyframes
    KeyFrameSet::const_iterator prev = cur;
    if ( prev != keys.begin() ) {
        --prev;
    } else {
        prev = keys.end();
    }
    KeyFrameSet::const_iterator next = cur;
    if ( next != keys.end() ) {
        ++next;
    }

    // handle first and last keyframe correctly:
    // - if their interpolation was eKeyframeTypeCatmullRom or eKeyframeTypeCubic, then it becomes eKeyframeTypeFree
    // - in all other cases it becomes eKeyframeTypeBroken
    KeyframeTypeEnum interp = k.getInterpolation();
    bool keyframeIsFirstOrLast = ( prev == keys.end() || next == keys.end() );
    bool interpIsNotBroken = (interp != eKeyframeTypeBroken);
    bool interpIsCatmullRomOrCubicOrFree = (interp == eKeyframeTypeCatmullRom ||
                                            interp == eKeyframeTypeCubic ||
                                            interp == eKeyframeTypeFree);
    _setBoth = keyframeIsFirstOrLast ? interpIsCatmullRomOrCubicOrFree  || curve->isCurvePeriodic() : interpIsNotBroken;

    bool isLeft;
    if (deriv == eSelectedTangentLeft) {
        //if dx is not of the good sign it would make the curve uncontrollable
        if (dx <= 0) {
            dx = 0.0001;
        }
        isLeft = true;
    } else {
        //if dx is not of the good sign it would make the curve uncontrollable
        if (dx >= 0) {
            dx = -0.0001;
        }
        isLeft = false;
    }
    double derivative = dy / dx;

    if (_setBoth) {
        _newInterp = eKeyframeTypeFree;
        _newLeft = derivative;
        _newRight = derivative;
    } else {
        if (isLeft) {
            _newLeft = derivative;
            _newRight = _oldRight;
        } else {
            _newLeft = _oldLeft;
            _newRight = derivative;
        }
        _newInterp = eKeyframeTypeBroken;
    }

    setText( tr("Move KeyFrame Slope") );

}

MoveTangentCommand::MoveTangentCommand(const AnimationModuleBasePtr& model,
                                       SelectedTangentEnum deriv,
                                       const AnimItemBasePtr& object,
                                       DimIdx dimension,
                                       ViewIdx view,
                                       const KeyFrame& k,
                                       double derivative,
                                       QUndoCommand *parent)
: QUndoCommand(parent)
, _model(model)
, _object(object)
, _dimension(dimension)
, _view(view)
, _keyTime(k.getTime())
, _deriv(deriv)
, _oldInterp( k.getInterpolation() )
, _oldLeft( k.getLeftDerivative() )
, _oldRight( k.getRightDerivative() )
, _setBoth(true)
, _isFirstRedo(true)
{
    _newInterp = _oldInterp == eKeyframeTypeBroken ? eKeyframeTypeBroken : eKeyframeTypeFree;
    _setBoth = _newInterp == eKeyframeTypeFree;

    switch (deriv) {
        case eSelectedTangentLeft:
        _newLeft = derivative;
        if (_newInterp == eKeyframeTypeBroken) {
            _newRight = _oldRight;
        } else {
            _newRight = derivative;
        }
        break;
    case eSelectedTangentRight:
        _newRight = derivative;
        if (_newInterp == eKeyframeTypeBroken) {
            _newLeft = _oldLeft;
        } else {
            _newLeft = derivative;
        }
    default:
        break;
    }
    setText( tr("Move KeyFrame Slope") );

}

void
MoveTangentCommand::setNewDerivatives(bool undo)
{
    AnimItemBasePtr item = _object.lock();
    if (!item) {
        return;
    }
    AnimatingObjectIPtr obj = item->getInternalAnimItem();
    if (!obj) {
        return;
    }

    double left = undo ? _oldLeft : _newLeft;
    double right = undo ? _oldRight : _newRight;
    KeyframeTypeEnum interp = undo ? _oldInterp : _newInterp;
    if (_setBoth) {
        obj->setLeftAndRightDerivativesAtTime(_view, _dimension, _keyTime, left, right);
    } else {
        bool isLeft = _deriv == eSelectedTangentLeft;
        obj->setDerivativeAtTime(_view, _dimension, _keyTime, isLeft ? left : right, isLeft);
    }
    obj->setInterpolationAtTime(_view, _dimension, _keyTime, interp);

}

void
MoveTangentCommand::undo()
{
    setNewDerivatives(true);
    AnimationModuleBasePtr model = _model.lock();
    if (model) {
        AnimItemDimViewKeyFramesMap newSelection;
        AnimationModuleSelectionModel::addAnimatedItemKeyframes(_object.lock(), _dimension, _view, &newSelection);
        model->setCurrentSelection(newSelection, std::vector<TableItemAnimPtr>(), std::vector<NodeAnimPtr>());
    }
}

void
MoveTangentCommand::redo()
{
    setNewDerivatives(false);
    if (!_isFirstRedo) {
        AnimationModuleBasePtr model = _model.lock();
        if (model) {
            AnimItemDimViewKeyFramesMap newSelection;
            AnimationModuleSelectionModel::addAnimatedItemKeyframes(_object.lock(), _dimension, _view, &newSelection);
            model->setCurrentSelection(newSelection, std::vector<TableItemAnimPtr>(), std::vector<NodeAnimPtr>());
        }
    }
    _isFirstRedo = true;
}

int
MoveTangentCommand::id() const
{
    return kCurveEditorMoveTangentsCommandCompressionID;
}

bool
MoveTangentCommand::mergeWith(const QUndoCommand * command)
{
    const MoveTangentCommand* cmd = dynamic_cast<const MoveTangentCommand*>(command);
    if (!cmd || cmd->id() == id()) {
        return false;
    }

    if (cmd->_object.lock() != _object.lock() || cmd->_dimension != _dimension || cmd->_keyTime != _keyTime) {
        return false;
    }


    _newInterp = cmd->_newInterp;
    _newLeft = cmd->_newLeft;
    _newRight = cmd->_newRight;

    return true;

}


NATRON_NAMESPACE_EXIT;
