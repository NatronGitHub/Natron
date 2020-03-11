/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 * Copyright (C) 2018-2020 The Natron developers
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
#include "Engine/Transform.h"
#include "Engine/ViewIdx.h"

NATRON_NAMESPACE_ENTER


template <typename T>
static void convertVariantTimeValuePairToTypedList(const std::list<VariantTimeValuePair>& inList,
                                                   std::list<TimeValuePair<T> >* outList)
{
    for (std::list<VariantTimeValuePair>::const_iterator it = inList.begin(); it!=inList.end(); ++it) {
        TimeValuePair<T> p(it->time, variantToType<T>(it->value));
        outList->push_back(p);
    }
}

static void convertKeySetToList(const KeyFrameSet& inList,
                                                  double offset,
                                                  std::list<KeyFrame>* outList)
{
    for (KeyFrameSet::const_iterator it = inList.begin(); it!=inList.end(); ++it) {
        KeyFrame k = *it;
        k.setTime(TimeValue(it->getTime() + offset));
        outList->push_back(k);
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

        const KeyFrameSet& keyStringSet = it->second;

        std::list<double> keyTimes;
        for (KeyFrameSet ::const_iterator it2 = keyStringSet.begin(); it2 != keyStringSet.end(); ++it2) {
            keyTimes.push_back(it2->getTime());
        }
        obj->deleteValuesAtTime(keyTimes, it->first.view, it->first.dim, eValueChangedReasonUserEdited);

    }
}

static void
addKeyFrames(const AnimItemDimViewKeyFramesMap& keys,
             bool clearExisting,
             double offset,
             const AnimItemBasePtr& targetItem,
             const DimSpec& targetItemDimension,
             const ViewSetSpec& targetItemView)
{
    for (AnimItemDimViewKeyFramesMap::const_iterator it = keys.begin(); it != keys.end(); ++it) {


        const KeyFrameSet& keyStringSet = it->second;

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
            obj->removeAnimation(view, dim, eValueChangedReasonUserEdited);
        }

        AnimatingObjectI::SetKeyFrameArgs args;
        args.view = view;
        args.dimension = dim;

        std::list<KeyFrame> keysList;
        convertKeySetToList(keyStringSet, offset, &keysList);
        obj->setMultipleKeyFrames(args, keysList);
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

    std::list<ViewIdx> viewsList = item->getViewsList();
    int nDims = item->getNDimensions();
    for (std::list<ViewIdx>::const_iterator it = viewsList.begin(); it != viewsList.end(); ++it) {
        if (!view.isAll() && view != *it) {
            continue;
        }

        DimSpec thisDimension = dim;
        // If the item has its dimensions folded and we modify dimension 0, also modify other dimensions
        if (thisDimension == 0 && !item->getAllDimensionsVisible(*it)) {
            thisDimension = DimSpec::all();
        }

        for (int i = 0; i < nDims; ++i) {
            if (!thisDimension.isAll() && dim != i) {
                continue;
            }

            animItemDimViewCreateOldCurve(item, DimIdx(i), *it, oldCurves);

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
        obj->cloneCurve(it->key.view, it->key.dim, *it->oldCurveState, 0 /*offset*/, 0 /*range*/);
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
    AnimationModuleBasePtr model = _model.lock();
    if (model) {
        model->setCurrentSelection(AnimItemDimViewKeyFramesMap(), std::vector<TableItemAnimPtr>(), std::vector<NodeAnimPtr>());
    }
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
            double minTimeForCurve = it->second.begin()->getTime();
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
    ValueChangedReturnCodeEnum s = startingTimeKnob->setValue(startingTimeKnob->getValue() + dt, ViewSetSpec::all(), DimIdx(0), eValueChangedReasonUserEdited, 0);
    Q_UNUSED(s);
}

static void
moveTimeOffset(const NodePtr& node,
               double dt)
{
    KnobIntBasePtr timeOffsetKnob = toKnobIntBase( node->getKnobByName(kTimeOffsetParamNameTimeOffset) );
    assert(timeOffsetKnob);
    ValueChangedReturnCodeEnum s = timeOffsetKnob->setValue(timeOffsetKnob->getValue() + dt, ViewSetSpec::all(), DimIdx(0), eValueChangedReasonUserEdited, 0);
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
    frameRangeKnob->setValueAcrossDimensions(values, DimIdx(0), ViewSetSpec::all(), eValueChangedReasonUserEdited);
}

static void
moveNodeIfLifetimeActivated(const NodePtr& node,
                            double dt)
{
    KnobBoolPtr lifeTimeEnabledKnob = node->getEffectInstance()->getLifeTimeEnabledKnob();
    if (!lifeTimeEnabledKnob || !lifeTimeEnabledKnob->getValue()) {
        return;
    }
    KnobIntPtr lifeTimeKnob = node->getEffectInstance()->getLifeTimeKnob();
    if (!lifeTimeKnob) {
        return;
    }
    std::vector<int> values(2);
    values[0] = lifeTimeKnob->getValue(DimIdx(0)) + dt;
    values[1] = lifeTimeKnob->getValue(DimIdx(1)) + dt;
    lifeTimeKnob->setValueAcrossDimensions(values, DimIdx(0), ViewSetSpec::all(), eValueChangedReasonUserEdited);
}

static void
moveGroupNode(const NodePtr& node,
              double dt)
{
    NodeGroupPtr group = node->isEffectNodeGroup();

    assert(group);
    NodesList nodes;
    group->getNodes_recursive(nodes);

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
                    CurvePtr curve = knob->getAnimationCurve(*it2, DimIdx(dim));
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
                        knob->moveValueAtTime(TimeValue(*kfIt), *it2, DimIdx(dim), dt, 0 /*dv*/, 0);
                    }
                }

            }
        }
    }
} // moveGroupNode


void
WarpKeysCommand::animMapToInternalMap(const AnimItemDimViewKeyFramesMap& keys, KeyFramesWithStringIndicesMap* internalMap)
{
    for (AnimItemDimViewKeyFramesMap::const_iterator it = keys.begin(); it!=keys.end(); ++it) {
        CurvePtr curve = it->first.item->getCurve(it->first.dim, it->first.view);
        assert(curve);
        KeyFrameWithStringIndexSet& newSet = (*internalMap)[it->first];
        for (KeyFrameSet::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
            KeyFrameWithStringIndex k;
            k.k = *it2;
            k.index = curve->keyFrameIndex(it2->getTime());
            assert(k.index != -1);
            newSet.insert(k);
        }
    }
}

void
WarpKeysCommand::internalMapToKeysMap(const KeyFramesWithStringIndicesMap& internalMap, AnimItemDimViewKeyFramesMap* keys)
{
    for (KeyFramesWithStringIndicesMap::const_iterator it = internalMap.begin(); it!=internalMap.end(); ++it) {
        KeyFrameSet& newSet = (*keys)[it->first];
        for (KeyFrameWithStringIndexSet::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
            newSet.insert(it2->k);
        }
    }
}

WarpKeysCommand::WarpKeysCommand(const AnimItemDimViewKeyFramesMap &keys,
                                 const AnimationModuleBasePtr& model,
                                 const std::vector<NodeAnimPtr>& nodes,
                                 const std::vector<TableItemAnimPtr>& tableItems,
                                 double dt,
                                 double dv,
                                 QUndoCommand *parent )
: QUndoCommand(parent)
, _model(model)
, _keys()
, _nodes(nodes)
, _tableItems(tableItems)
{
    _warp.reset(new Curve::TranslationKeyFrameWarp(dt, dv));
    animMapToInternalMap(keys, &_keys);
    setText( tr("Move KeyFrame(s)") );
}


WarpKeysCommand::WarpKeysCommand(const AnimItemDimViewKeyFramesMap& keys,
                                 const AnimationModuleBasePtr& model,
                                 const Transform::Matrix3x3& matrix,
                                 QUndoCommand *parent)
: QUndoCommand(parent)
, _model(model)
, _keys()
{
    _warp.reset(new Curve::AffineKeyFrameWarp(matrix));
    animMapToInternalMap(keys, &_keys);
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
        
        
        const KeyFrameSet& keyStringSet = it->second;
        
        // Make-up keyframe times to warp for this item/view/dim
        std::list<double> keyTimes;
        for (KeyFrameSet ::const_iterator it2 = keyStringSet.begin(); it2 != keyStringSet.end(); ++it2) {
            keyTimes.push_back(it2->getTime());
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
        for (std::vector<NodeAnimPtr>::iterator it = _nodes.begin(); it != _nodes.end(); ++it) {
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

        //for (std::vector<TableItemAnimPtr>::iterator it = _tableItems.begin(); it != _tableItems.end(); ++it) {
#pragma message WARN("TODO: move lifetime table item")
        //}
    }
    
    
    for (KeyFramesWithStringIndicesMap::iterator it = _keys.begin(); it!=_keys.end();++it) {
        AnimatingObjectIPtr obj = it->first.item->getInternalAnimItem();
        if (!obj) {
            continue;
        }

        const KeyFrameWithStringIndexSet& keyStringSet = it->second;

        // Make-up keyframe times to warp for this item/view/dim
        std::list<double> keyTimes;
        for (KeyFrameWithStringIndexSet ::const_iterator it2 = keyStringSet.begin(); it2 != keyStringSet.end(); ++it2) {
            keyTimes.push_back(it2->k.getTime());
        }

        // Warp keys...
        std::vector<KeyFrame> newKeyframe;
        if (obj->warpValuesAtTime(keyTimes, it->first.view, it->first.dim, *_warp, &newKeyframe)) {

            assert(newKeyframe.size() == keyStringSet.size());

            // Modify original keys by warped keys
            KeyFrameWithStringIndexSet newKeyStringSet;
            KeyFrameWithStringIndexSet::const_iterator keysIt = keyStringSet.begin();
            for (std::size_t i = 0; i < newKeyframe.size(); ++i, ++keysIt) {

                // Copy the new key, its time and Y value may have changed
                KeyFrameWithStringIndex k;
                k.k = newKeyframe[i];

                // Copy index and string - they did not change
                k.index = keysIt->index;

                newKeyStringSet.insert(k);
            }

            it->second = newKeyStringSet;
        }
    } // for all objects

    AnimationModuleBasePtr model = _model.lock();
    if (model) {
        AnimItemDimViewKeyFramesMap keys;
        internalMapToKeysMap(_keys, &keys);
        model->setCurrentSelection(keys, _tableItems, _nodes);
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
    if (!cmd) {
        return false;
    }

    // Not the same number of curves, bail
    if ( cmd->_keys.size() != _keys.size() ) {
        return false;
    }

    // Check if all curves are the same, and for each of them check that keyframes indices are the same
    {
        KeyFramesWithStringIndicesMap::const_iterator itother = cmd->_keys.begin();
        for (KeyFramesWithStringIndicesMap::const_iterator it = _keys.begin(); it != _keys.end(); ++it, ++itother) {
            if (itother->first.item != it->first.item || itother->first.view != it->first.view || itother->first.dim != it->first.dim) {
                return false;
            }

            if ( itother->second.size() != it->second.size() ) {
                return false;
            }

            CurvePtr thisCurve = it->first.item->getCurve(it->first.dim, it->first.view);
            assert(thisCurve && thisCurve == itother->first.item->getCurve(itother->first.dim, itother->first.view));


            KeyFrameWithStringIndexSet::const_iterator itOtherKey = itother->second.begin();
            for (KeyFrameWithStringIndexSet::const_iterator itKey = it->second.begin(); itKey != it->second.end(); ++itKey, ++itOtherKey) {
                if (itKey->index != itOtherKey->index) {
                    return false;
                }
            }
        }
    }

    // Check that nodes are the same
    if ( cmd->_nodes.size() != _nodes.size() ) {
        return false;
    }
    
    {
        std::vector<NodeAnimPtr>::const_iterator itOther = cmd->_nodes.begin();
        for (std::vector<NodeAnimPtr>::const_iterator it = _nodes.begin(); it != _nodes.end(); ++it, ++itOther) {
            if (*itOther != *it) {
                return false;
            }
        }
    }

    // Check that table items are the same
    if ( cmd->_tableItems.size() != _tableItems.size() ) {
        return false;
    }
    
    {
        std::vector<TableItemAnimPtr>::const_iterator itOther = cmd->_tableItems.begin();
        for (std::vector<TableItemAnimPtr>::const_iterator it = _tableItems.begin(); it != _tableItems.end(); ++it, ++itOther) {
            if (*itOther != *it) {
                return false;
            }
        }
    }


    // Check that the warp was merged OK
    bool warpMerged = _warp->mergeWith(*cmd->_warp);
    if (!warpMerged) {
        return false;
    }


    // Merge keyframes
    KeyFramesWithStringIndicesMap::const_iterator itother = cmd->_keys.begin();
    for (KeyFramesWithStringIndicesMap::iterator it = _keys.begin(); it != _keys.end(); ++it, ++itother) {
        it->second = itother->second;
    }
    return warpMerged;

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
        for (KeyFrameSet::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
            keyTimes.push_back(it2->getTime());
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
                                       const AnimItemDimViewKeyFrame& keyframe,
                                       double dx,
                                       double dy,
                                       QUndoCommand *parent)
: QUndoCommand(parent)
, _model(model)
, _oldKey(keyframe)
, _newKey(keyframe)
, _deriv(deriv)
, _setBoth(false)
, _isFirstRedo(true)
{

    // Compute derivative
    CurvePtr curve = keyframe.id.item->getCurve(keyframe.id.dim,keyframe.id.view);
    assert(curve);

    KeyFrameSet keys = curve->getKeyFrames_mt_safe();
    KeyFrameSet::const_iterator cur = keys.find(keyframe.key);

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
    KeyframeTypeEnum interp = keyframe.key.getInterpolation();
    bool keyframeIsFirstOrLast = ( prev == keys.end() || next == keys.end() );
    bool interpIsNotBroken = (interp != eKeyframeTypeBroken);
    bool interpIsCatmullRomOrCubicOrFree = (interp == eKeyframeTypeCatmullRom ||
                                            interp == eKeyframeTypeCubic ||
                                            interp == eKeyframeTypeFree);
    _setBoth = keyframeIsFirstOrLast ? interpIsCatmullRomOrCubicOrFree  || curve->isCurvePeriodic() : interpIsNotBroken;

    if (deriv == eSelectedTangentLeft) {
        //if dx is not of the good sign it would make the curve uncontrollable
        if (dx <= 0) {
            dx = 0.0001;
        }
    } else {
        //if dx is not of the good sign it would make the curve uncontrollable
        if (dx >= 0) {
            dx = -0.0001;
        }
    }
    double derivative = dy / dx;

    if (_setBoth) {
        _newKey.key.setInterpolation(eKeyframeTypeFree);
        _newKey.key.setLeftDerivative(derivative);
        _newKey.key.setRightDerivative(derivative);
    } else {
        if (deriv == eSelectedTangentLeft) {
            _newKey.key.setLeftDerivative(derivative);
        } else {
            _newKey.key.setRightDerivative(derivative);
        }
        _newKey.key.setInterpolation(eKeyframeTypeBroken);
    }

    setText( tr("Move KeyFrame Slope") );

}

MoveTangentCommand::MoveTangentCommand(const AnimationModuleBasePtr& model,
                                       SelectedTangentEnum deriv,
                                       const AnimItemDimViewKeyFrame& keyframe,
                                       double derivative,
                                       QUndoCommand *parent)
: QUndoCommand(parent)
, _model(model)
, _oldKey(keyframe)
, _newKey(keyframe)
, _deriv(deriv)
, _setBoth(true)
, _isFirstRedo(true)
{
    KeyframeTypeEnum newInterp = _newKey.key.getInterpolation() == eKeyframeTypeBroken ? eKeyframeTypeBroken : eKeyframeTypeFree;
    _newKey.key.setInterpolation(newInterp);
    _oldKey.key.setInterpolation(newInterp);
    _setBoth = newInterp == eKeyframeTypeFree;

    switch (deriv) {
        case eSelectedTangentLeft:
            _newKey.key.setLeftDerivative(derivative);
            if (newInterp != eKeyframeTypeBroken) {
                _newKey.key.setRightDerivative(derivative);
            }
            break;
        case eSelectedTangentRight:
            _newKey.key.setRightDerivative(derivative);
            if (newInterp != eKeyframeTypeBroken) {
               _newKey.key.setLeftDerivative(derivative);
            }
        default:
            break;
    }
    setText( tr("Move KeyFrame Slope") );
    
}

void
MoveTangentCommand::setNewDerivatives(bool undo)
{
    AnimatingObjectIPtr obj = _oldKey.id.item->getInternalAnimItem();
    if (!obj) {
        return;
    }

    double left = undo ? _oldKey.key.getLeftDerivative() : _newKey.key.getLeftDerivative();
    double right = undo ? _oldKey.key.getRightDerivative() : _newKey.key.getRightDerivative();
    KeyframeTypeEnum interp = undo ? _oldKey.key.getInterpolation() : _newKey.key.getInterpolation();
    if (_setBoth) {
        obj->setLeftAndRightDerivativesAtTime(_oldKey.id.view, _oldKey.id.dim, _oldKey.key.getTime(), left, right);
    } else {
        bool isLeft = _deriv == eSelectedTangentLeft;
        obj->setDerivativeAtTime(_oldKey.id.view, _oldKey.id.dim, _oldKey.key.getTime(),  isLeft ? left : right, isLeft);
    }
    obj->setInterpolationAtTime(_oldKey.id.view, _oldKey.id.dim, _oldKey.key.getTime(),  interp);

}

void
MoveTangentCommand::undo()
{
    setNewDerivatives(true);
    AnimationModuleBasePtr model = _model.lock();
    if (model) {
        AnimItemDimViewKeyFramesMap newSelection;
        newSelection[_oldKey.id].insert(_oldKey.key);
        model->setCurrentSelection(newSelection, std::vector<TableItemAnimPtr>(), std::vector<NodeAnimPtr>());
  
    }
}

void
MoveTangentCommand::redo()
{
    setNewDerivatives(false);
    AnimationModuleBasePtr model = _model.lock();
    if (model) {
        AnimItemDimViewKeyFramesMap newSelection;
        newSelection[_newKey.id].insert(_newKey.key);
        model->setCurrentSelection(newSelection, std::vector<TableItemAnimPtr>(), std::vector<NodeAnimPtr>());
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

    if (cmd->_newKey.id.item != _newKey.id.item || cmd->_newKey.id.dim != _newKey.id.dim || cmd->_newKey.id.view != _newKey.id.view || cmd->_newKey.key.getTime() != _newKey.key.getTime()) {
        return false;
    }

    _newKey.key.setInterpolation(cmd->_newKey.key.getInterpolation());
    _newKey.key.setLeftDerivative(cmd->_newKey.key.getLeftDerivative());
    _newKey.key.setRightDerivative(cmd->_newKey.key.getRightDerivative());

    return true;

}


NATRON_NAMESPACE_EXIT
