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

#include "Gui/AnimationModule.h"
#include "Gui/CurveEditor.h"
#include "Gui/CurveGui.h"
#include "Gui/KnobGui.h"
#include "Gui/NodeAnim.h"
#include "Gui/TableItemAnim.h"
#include "Gui/CurveWidget.h"

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
removeKeyFrames(const AnimItemDimViewKeyFramesMap& keys)
{
    
    for (AnimItemDimViewKeyFramesMap::const_iterator it = keys.begin(); it != keys.end(); ++it) {
        
        AnimatingObjectIPtr obj = it->first.item->getInternalAnimItem();
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
                                     const std::list<AnimItemBasePtr>& dstItems)
{
    for (AnimItemDimViewKeyFramesMap::const_iterator it = keys.begin(); it != keys.end(); ++it) {
        
       
        const KeyFrameWithStringSet& keyStringSet = it->second;
      
        AnimatingObjectIPtr obj = it->first.item->getInternalAnimItem();
        if (!obj) {
            continue;
        }
        
        
        std::list<AnimatingObjectIPtr> targets;
        for (std::list<AnimItemBasePtr>::const_iterator it2 = dstItems.begin(); it2!= dstItems.end(); ++it2) {
            targets.push_back((*it2)->getInternalAnimItem());
        }
        if (targets.empty()) {
            targets.push_back(obj);
        }
       
        if (clearExisting) {
            // Remove all existing animation
            obj->removeAnimation(it->first.view, it->first.dim);
        }

        for (std::list<AnimatingObjectIPtr>::const_iterator it2 = targets.begin(); it2 != targets.end(); ++it2) {
            AnimatingObjectI::KeyframeDataTypeEnum dataType = (*it2)->getKeyFrameDataType();
            switch (dataType) {
                case AnimatingObjectI::eKeyframeDataTypeNone:
                case AnimatingObjectI::eKeyframeDataTypeDouble:
                {
                    std::list<DoubleTimeValuePair> keysList;
                    convertVariantKeyStringSetToTypedList<double>(keyStringSet, offset, &keysList);
                    (*it2)->setMultipleDoubleValueAtTime(keysList, it->first.view, it->first.dim);
                }   break;
                case AnimatingObjectI::eKeyframeDataTypeBool:
                {
                    std::list<BoolTimeValuePair> keysList;
                    convertVariantKeyStringSetToTypedList<bool>(keyStringSet, offset, &keysList);
                    (*it2)->setMultipleBoolValueAtTime(keysList, it->first.view, it->first.dim);
                    
                }   break;
                case AnimatingObjectI::eKeyframeDataTypeString:
                {
                    std::list<StringTimeValuePair> keysList;
                    convertVariantKeyStringSetToTypedList<std::string>(keyStringSet, offset, &keysList);
                    (*it2)->setMultipleStringValueAtTime(keysList, it->first.view, it->first.dim);
                    
                }   break;
                case AnimatingObjectI::eKeyframeDataTypeInt:
                {
                    std::list<IntTimeValuePair> keysList;
                    convertVariantKeyStringSetToTypedList<int>(keyStringSet, offset, &keysList);
                    (*it2)->setMultipleIntValueAtTime(keysList, it->first.view, it->first.dim);
                    
                    
                }   break;
            } // end switch
        }
        
    }
} // addKeyFrames




AddKeysCommand::AddKeysCommand(const AnimItemDimViewKeyFramesMap & keys,
                               QUndoCommand *parent)
: QUndoCommand(parent)
, _keys(keys)
{
    setText( tr("Add KeyFrame(s)") );
}

void
AddKeysCommand::undo()
{
    removeKeyFrames(_keys);
} // undo

void
AddKeysCommand::redo()
{
    addKeyFrames(_keys, false, 0, std::list<AnimItemBasePtr>());
} // redo

RemoveKeysCommand::RemoveKeysCommand(const AnimItemDimViewKeyFramesMap & keys,
                                    QUndoCommand *parent)
: QUndoCommand(parent)
,  _keys(keys)
{
    setText( tr("Remove KeyFrame(s)") );
}

void
RemoveKeysCommand::undo()
{
    addKeyFrames(_keys, false, 0, std::list<AnimItemBasePtr>());
} // undo

void
RemoveKeysCommand::redo()
{
    removeKeyFrames(_keys);
} // redo


PasteKeysCommand::PasteKeysCommand(const AnimItemDimViewKeyFramesMap & keys,
                                   const std::list<AnimItemBasePtr>& dstAnim,
                                   bool pasteRelativeToCurrentTime,
                                   double currentTime,
                                   QUndoCommand *parent)
: QUndoCommand(parent)
, _offset(0)
, _dstAnims(dstAnim)
, _keys(keys)
{
    
    double minSelectedKeyTime(std::numeric_limits<double>::infinity())
    if (pasteRelativeToCurrentTime) {
        _flags |= eAddFlagsRelative;
        
        
        for (AnimItemDimViewKeyFramesMap::const_iterator it = keys.begin(); it != keys.end(); ++it) {
            if (it->second.empty()) {
                continue;
            }
            double minTimeForCurve = it->second.begin()->key.getTime();
            minSelectedKeyTime = std::min(minSelectedKeyTime, minTimeForCurve);
        }
    }
    _offset = !pasteRelativeToCurrentTime ? 0 : currentTime - minSelectedKeyTime;
    
    setText( tr("Paste KeyFrame(s)") );
    
}

void
PasteKeysCommand::undo()
{
    
} // undo

void
PasteKeysCommand::redo()
{
    
} // redo

static void animItemDimViewKeysMapConvert(const AnimItemDimViewKeyFramesMap& keys, KeysWithOldCurveMap* newKeys)
{
    for (AnimItemDimViewKeyFramesMap::const_iterator it = keys.begin(); it != keys.end(); ++it) {
        AnimItemDimViewIndexIDWithCurve key;
        key.key = it->first;
        CurvePtr curve = it->first.item->getCurve(it->first.dim, it->first.view);
        if (!curve) {
            continue;
        }
        key.oldCurveState.reset(new Curve);
        key.oldCurveState->clone(*curve);
        (*newKeys)[key] = it->second;
    }
}

static void keysWithOldCurveMapClone(const KeysWithOldCurveMap& keys)
{
    for (KeysWithOldCurveMap::const_iterator it = keys.begin(); it != keys.end(); ++it) {
        AnimatingObjectIPtr obj = it->first.key.item->getInternalAnimItem();
        if (!obj) {
            continue;
        }

        CurvePtr curve = obj->getAnimationCurve(it->first.key.view, it->first.key.dim);
        if (!curve) {
            continue;
        }

        // Clone the old curve state
        obj->cloneCurve(it->first.key.view, it->first.key.dim, *it->first.oldCurveState, 0 /*offset*/, 0 /*range*/, 0 /*stringAnimation*/);
        
    }
}

SetKeysCommand::SetKeysCommand(const AnimItemDimViewKeyFramesMap & keys, QUndoCommand *parent)
: QUndoCommand(parent)
, _keys()
{
    animItemDimViewKeysMapConvert(keys, &_keys);
    setText( tr("Set keyframe(s)") );
}

void
SetKeysCommand::undo()
{
    keysWithOldCurveMapClone(_keys);
}

void
SetKeysCommand::redo()
{
    for (KeysWithOldCurveMap::iterator it = _keys.begin(); it != _keys.end(); ++it) {
        AnimatingObjectIPtr obj = it->first.key.item->getInternalAnimItem();
        if (!obj) {
            continue;
        }

        // Remove all existing animation
        obj->removeAnimation(it->first.key.view, it->first.key.dim);

        const KeyFrameWithStringSet& keyStringSet = it->second;

        // Add new keyframes
        AnimatingObjectI::KeyframeDataTypeEnum dataType = obj->getKeyFrameDataType();
        switch (dataType) {
            case AnimatingObjectI::eKeyframeDataTypeNone:
            case AnimatingObjectI::eKeyframeDataTypeDouble:
            {
                std::list<DoubleTimeValuePair> keysList;
                convertVariantKeyStringSetToTypedList<double>(keyStringSet, &keysList);
                obj->setMultipleDoubleValueAtTime(keysList, it->first.key.view, it->first.key.dim);
            }   break;
            case AnimatingObjectI::eKeyframeDataTypeBool:
            {
                std::list<BoolTimeValuePair> keysList;
                convertVariantKeyStringSetToTypedList<bool>(keyStringSet, &keysList);
                obj->setMultipleBoolValueAtTime(keysList, it->first.key.view, it->first.key.dim);

            }   break;
            case AnimatingObjectI::eKeyframeDataTypeString:
            {
                std::list<StringTimeValuePair> keysList;
                convertVariantKeyStringSetToTypedList<std::string>(keyStringSet, &keysList);
                obj->setMultipleStringValueAtTime(keysList, it->first.key.view, it->first.key.dim);

            }   break;
            case AnimatingObjectI::eKeyframeDataTypeInt:
            {
                std::list<IntTimeValuePair> keysList;
                convertVariantKeyStringSetToTypedList<int>(keyStringSet, &keysList);
                obj->setMultipleIntValueAtTime(keysList, it->first.key.view, it->first.key.dim);


            }   break;
        } // end switch

    } // for all objects
} // redo



/**
 * @brief Move the node 'reader' on project timeline, by offsetting its
 * starting time by 'dt'.
 */
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
                                 const std::vector<NodeAnimPtr >& nodes,
                                 const std::vector<TableItemAnimPtr>& tableItems,
                                 double dt,
                                 double dv,
                                 QUndoCommand *parent )
: QUndoCommand(parent)
, _keys(keys)
, _nodes(nodes)
, _tableItems(tableItems)
{
    _warp.reset(new Curve::TranslationKeyFrameWarp(dt, dv));
    setText( tr("Move KeyFrame(s)") );

    /*std::set<NodeAnimPtr> uniqueNodeAnims;
    for (std::vector<NodeAnimPtr >::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        AnimatedItemTypeEnum type = (*it)->getItemType();
        if ((*it)->isRangeDrawingEnabled()) {
            uniqueNodeAnims.insert(*it);
        }
        NodeGroupPtr isGroup = (*it)->getInternalNode()->isEffectNodeGroup();
        if (isGroup) {
            NodesList recurseNodes;
            isGroup->getNodes_recursive(recurseNodes, true);
            for (NodesList::iterator it = recurseNodes.begin(); it != recurseNodes.end(); ++it) {
                uniqueNodeAnims.insert(*it);
            }
        }
    }*/

}

WarpKeysCommand::WarpKeysCommand(const AnimItemDimViewKeyFramesMap& keys,
                                 const Transform::Matrix3x3& matrix,
                                 QUndoCommand *parent)
: QUndoCommand(parent)
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
}

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
                            KeyframeTypeEnum newInterpolation,
                            QUndoCommand *parent)
: QUndoCommand(parent)
, _keys()
, _newInterpolation(newInterpolation)
{
    animItemDimViewKeysMapConvert(keys, &_keys);
    setText( tr("Set KeyFrame(s) Interpolation") );

}


void
SetKeysInterpolationCommand::undo()
{
    keysWithOldCurveMapClone(_keys);
}

void
SetKeysInterpolationCommand::redo()
{
    for (KeysWithOldCurveMap::iterator it = _keys.begin(); it != _keys.end(); ++it) {
        AnimatingObjectIPtr obj = it->first.key.item->getInternalAnimItem();
        if (!obj) {
            continue;
        }


        std::list<double> keyTimes;
        for (KeyFrameWithStringSet ::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
            keyTimes.push_back(it2->key.getTime());
        }

        obj->setInterpolationAtTimes(it->first.key.view, it->first.key.dim, keyTimes, _newInterpolation);
    }
}


MoveTangentCommand::MoveTangentCommand(SelectedTangentEnum deriv,
                                       const AnimatingObjectIPtr& object,
                                       DimIdx dimension,
                                       ViewIdx view,
                                       const KeyFrame& k,
                                       double dx,
                                       double dy,
                                       QUndoCommand *parent)
: QUndoCommand(parent)
, _object(object)
, _dimension(dimension)
, _view(view)
, _keyTime(k.getTime())
, _deriv(deriv)
, _oldInterp( k.getInterpolation() )
, _oldLeft( k.getLeftDerivative() )
, _oldRight( k.getRightDerivative() )
, _setBoth(false)
{
    CurvePtr curve = object->getAnimationCurve(view, dimension);
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

MoveTangentCommand::MoveTangentCommand(SelectedTangentEnum deriv,
                                       const AnimatingObjectIPtr& object,
                                       DimIdx dimension,
                                       ViewIdx view,
                                       const KeyFrame& k,
                                       double derivative,
                                       QUndoCommand *parent)
    : QUndoCommand(parent)
    , _object(object)
    , _dimension(dimension)
    , _view(view)
    , _keyTime(k.getTime())
    , _deriv(deriv)
    , _oldInterp( k.getInterpolation() )
    , _oldLeft( k.getLeftDerivative() )
    , _oldRight( k.getRightDerivative() )
    , _setBoth(true)
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
    AnimatingObjectIPtr obj = _object.lock();
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
}

void
MoveTangentCommand::redo()
{
    setNewDerivatives(false);
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



PasteKeysCommand::PasteKeysCommand(const AnimItemDimViewKeyFramesMap &keys,
                                   const std::list<AnimItemBasePtr>& dstAnim,
                                   bool pasteRelativeToRefTime,
                                   double currentTime,
                                   QUndoCommand *parent)
: QUndoCommand(parent)
, _refTime(currentTime)
, _pasteRelativeToRefTime(pasteRelativeToRefTime)
, _keys(keys)
, _dstAnim(dstAnim)
{

    for (AnimItemDimViewKeyFramesMap::const_iterator it = keys.begin(); it != keys.end(); ++it) {
        if (it->second.empty()) {
            continue;
        }
        double minTimeForCurve = it->second.begin()->key.getTime();
        _minSelectedKeyTime = std::min(_minSelectedKeyTime, minTimeForCurve);
    }
    
    setText( tr("Paste KeyFrame(s)") );
}

void
PasteKeysCommand::undo()
{
    addOrRemoveKeyframe(false);
}

void
PasteKeysCommand::redo()
{
    addOrRemoveKeyframe(true);
}

void
PasteKeysCommand::setKeyValueFromKnob(const KnobIPtr& knob, double keyTime, KeyFrame* key)
{
    KnobDoubleBasePtr isDouble = toKnobDoubleBase(knob);
    KnobBoolBasePtr isBool = toKnobBoolBase(knob);
    KnobIntBasePtr isInt = toKnobIntBase(knob);
    KnobStringBasePtr isString = toKnobStringBase(knob);
    
    
    if (isDouble) {
        key->setValue( isDouble->getValueAtTime(keyTime) );
    } else if (isBool) {
        key->setValue( isBool->getValueAtTime(keyTime) );
    } else if (isInt) {
        key->setValue( isInt->getValueAtTime(keyTime) );
    } else if (isString) {
        std::string v = isString->getValueAtTime(keyTime);
        double keyFrameValue = 0.;
        AnimatingKnobStringHelperPtr isStringAnimatedKnob = boost::dynamic_pointer_cast<AnimatingKnobStringHelper>(isString);
        assert(isStringAnimatedKnob);
        if (isStringAnimatedKnob) {
            isStringAnimatedKnob->stringToKeyFrameValue(keyTime, ViewIdx(0), v, &keyFrameValue);
        }
        key->setValue(keyFrameValue);
    }
}

void
PasteKeysCommand::addOrRemoveKeyframe(bool add)
{
    for (std::list<AnimItemBasePtr>::const_iterator it = _dstAnim.begin(); it != _dstAnim.end(); ++it) {
        
        AnimatingObjectIPtr animItem = (*it)->getInternalAnimItem();
        if (!animItem) {
            continue;
        }
        for (AnimItemDimViewKeyFramesMap::const_iterator it2 = _keys.begin(); it2 != _keys.end(); ++it2) {
            
            for (KeyFrameWithStringSet::const_iterator it3 = it2->second.begin(); it3 != it2->second.end(); ++it3) {
                double keyTime = it3->key.getTime();
                double setTime = _pasteRelativeToRefTime ? keyTime - _minSelectedKeyTime + _refTime : keyTime;
                if (!add) {
                    animItem->deleteValueAtTime(time, <#Natron::ViewSetSpec view#>, <#Natron::DimSpec dimension#>)
                }
            }
        }
        for (std::size_t i = 0; i < _keys.size(); ++i) {
            
            int dim = knobContext->getDimension();
            KnobIPtr knob = knobContext->getInternalKnob();
            knob->beginChanges();
            
            double keyTime = _keys[i].key.getTime();
            double setTime = _pasteRelativeToRefTime ? keyTime - _keys[_refKeyindex].key.getTime() + _refTime : keyTime;
            
            if (add) {
                
                for (int j = 0; j < knob->getDimension(); ++j) {
                    if ( (dim == -1) || (j == dim) ) {
                        KeyFrame k = _keys[i].key;
                        k.setTime(setTime);
                        
                        knob->setKeyFrame(k, ViewSpec::all(), j, eValueChangedReasonNatronGuiEdited);
                    }
                }
            } else {
                for (int j = 0; j < knob->getDimension(); ++j) {
                    if ( (dim == -1) || (j == dim) ) {
                        knob->deleteValueAtTime(eCurveChangeReasonDopeSheet, setTime, ViewSpec::all(), j, i == 0);
                    }
                }
            }
            
            knob->endChanges();
        }
    }
    
    
    _model->refreshSelectionBboxAndRedrawView();
} // PasteKeysCommand::addOrRemoveKeyframe

NATRON_NAMESPACE_EXIT;
