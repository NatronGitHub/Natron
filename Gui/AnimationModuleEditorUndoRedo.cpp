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

#include "AnimationModuleEditorUndoRedo.h"

#include <stdexcept>

#include <QtCore/QDebug>

#include "Engine/Knob.h"
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/ReadNode.h"
#include "Engine/ViewIdx.h"
#include "Engine/ViewerInstance.h"

#include "Global/GlobalDefines.h"

#include "Gui/DockablePanel.h"
#include "Gui/KnobGui.h"
#include "Gui/NodeGui.h"
#include "Gui/AnimationModuleEditor.h"
#include "Gui/AnimationModule.h"
#include "Gui/DopeSheetView.h"

NATRON_NAMESPACE_ENTER;

typedef std::map<KnobIWPtr, KnobGui *> KnobsAndGuis;


////////////////////////// Helpers //////////////////////////


NATRON_NAMESPACE_ANONYMOUS_ENTER

/**
 * @brief Move the node 'reader' on project timeline, by offsetting its
 * starting time by 'dt'.
 */
void
moveReader(const NodePtr &reader,
           double dt)
{
    KnobIntBasePtr startingTimeKnob = toKnobIntBase( reader->getKnobByName(kReaderParamNameStartingTime) );
    assert(startingTimeKnob);
    ValueChangedReturnCodeEnum s = startingTimeKnob->setValue(startingTimeKnob->getValue() + dt, ViewSetSpec::all(), DimIdx(0), eValueChangedReasonNatronGuiEdited, 0);
    Q_UNUSED(s);
}

void
moveTimeOffset(const NodePtr& node,
               double dt)
{
    KnobIntBasePtr timeOffsetKnob = toKnobIntBase( node->getKnobByName(kTimeOffsetParamNameTimeOffset) );
    assert(timeOffsetKnob);
    ValueChangedReturnCodeEnum s = timeOffsetKnob->setValue(timeOffsetKnob->getValue() + dt, ViewSetSpec::all(), DimIdx(0), eValueChangedReasonNatronGuiEdited, 0);
    Q_UNUSED(s);
}

void
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

void
moveGroupNode(AnimationModuleEditor* model,
              const NodePtr& node,
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
            moveGroupNode(model, *it, dt);
        }

        // Move keyframes
        const KnobsVec &knobs = (*it)->getKnobs();

        for (KnobsVec::const_iterator knobIt = knobs.begin(); knobIt != knobs.end(); ++knobIt) {
            const KnobIPtr& knob = *knobIt;
            if ( !knob->hasAnimation() ) {
                continue;
            }

            for (int dim = 0; dim < knob->getNDimensions(); ++dim) {
                if ( !knob->isAnimated( DimIdx(dim), ViewIdx(0) ) ) {
                    continue;
                }
                KeyFrameSet keyframes = knob->getCurve(ViewIdx(0), DimIdx(dim))->getKeyFrames_mt_safe();

                for (KeyFrameSet::iterator kfIt = keyframes.begin(); kfIt != keyframes.end(); ++kfIt) {
                    KeyFrame kf = (*kfIt);
                    KeyFrame fake;

                    knob->moveValueAtTime(eCurveChangeReasonDopeSheet, kf.getTime(), ViewSpec::all(), dim, dt, 0, &fake);
                }
            }
        }
    }
} // moveGroupNode

NATRON_NAMESPACE_ANONYMOUS_EXIT


////////////////////////// DSMoveKeysCommand //////////////////////////

DSMoveKeysAndNodesCommand::DSMoveKeysAndNodesCommand(const AnimKeyFramePtrList &keys,
                                                     const std::vector<NodeAnimPtr >& nodes,
                                                     double dt,
                                                     AnimationModuleEditor *model,
                                                     QUndoCommand *parent)
    : QUndoCommand(parent),
    _keys(keys),
    _nodes(),
    _dt(dt),
    _model(model)
{
    setText( tr("Move selected keys") );
    std::set<NodePtr > nodesSet;
    for (std::vector<NodeAnimPtr >::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        AnimatedItemType type = (*it)->getItemType();
        if ( (type != eAnimatedItemTypeReader) &&
             ( type != eAnimatedItemTypeGroup) &&
             ( type != eAnimatedItemTypeTimeOffset) &&
             ( type != eAnimatedItemTypeFrameRange) ) {
            //Note that Retime nodes cannot be moved
            continue;
        }
        _nodes.push_back(*it);
        nodesSet.insert( (*it)->getInternalNode() );
        NodeGroupPtr isGroup = (*it)->getInternalNode()->isEffectNodeGroup();
        if (isGroup) {
            NodesList recurseNodes;
            isGroup->getNodes_recursive(recurseNodes, true);
            for (NodesList::iterator it = recurseNodes.begin(); it != recurseNodes.end(); ++it) {
                nodesSet.insert(*it);
            }
        }
    }

    for (AnimKeyFramePtrList::iterator it = _keys.begin(); it != _keys.end(); ++it) {
        KnobHolderPtr holder = (*it)->getContext()->getInternalKnob()->getHolder();
        assert(holder);
        EffectInstancePtr isEffect = toEffectInstance(holder);
        if (isEffect) {
            nodesSet.insert( isEffect->getNode() );
        }
    }

    for (std::set<NodePtr >::iterator it = nodesSet.begin(); it != nodesSet.end(); ++it) {
        _allDifferentNodes.push_back(*it);
    }
}

void
DSMoveKeysAndNodesCommand::undo()
{
    moveSelection(-_dt);
}

void
DSMoveKeysAndNodesCommand::redo()
{
    moveSelection(_dt);
}

void
DSMoveKeysAndNodesCommand::moveSelection(double dt)
{
    /*
       The view is going to get MANY update() calls since every knob change will trigger a computeNodeRange() in DopeSheetView
       We explicitly disable update on the dopesheet view and re-enable it afterwards.
     */
    DopeSheetView* view = _model->getDopesheetView();

    view->setUpdatesEnabled(false);

    for (NodesWList::iterator khIt = _allDifferentNodes.begin(); khIt != _allDifferentNodes.end(); ++khIt) {
        NodePtr node = khIt->lock();
        if (!node) {
            continue;
        }
        node->getEffectInstance()->beginChanges();
    }


    //////////Handle selected keyframes
    for (AnimKeyFramePtrList::iterator it = _keys.begin(); it != _keys.end(); ++it) {
        const AnimKeyFramePtr& selectedKey = (*it);
        KnobAnimPtr knobContext = selectedKey->context.lock();
        if (!knobContext) {
            continue;
        }

        KnobIPtr knob = knobContext->getKnobGui()->getKnob();

        knob->moveValueAtTime(eCurveChangeReasonDopeSheet, selectedKey->key.getTime(), ViewIdx(0),
                              knobContext->getDimension(),
                              dt, 0, &selectedKey->key);
    }
    ////////////Handle selected nodes
    for (std::vector<NodeAnimPtr >::iterator it = _nodes.begin(); it != _nodes.end(); ++it) {
        AnimatedItemType type = (*it)->getItemType();
        if (type == eAnimatedItemTypeReader) {
            moveReader( (*it)->getInternalNode(), dt );
        } else if (type == eAnimatedItemTypeFrameRange) {
            moveFrameRange( (*it)->getInternalNode(), dt );
        } else if (type == eAnimatedItemTypeTimeOffset) {
            moveTimeOffset( (*it)->getInternalNode(), dt );
        } else if (type == eAnimatedItemTypeGroup) {
            moveGroupNode(_model, (*it)->getInternalNode(), dt);
        }
    }


    for (NodesWList::const_iterator khIt = _allDifferentNodes.begin(); khIt != _allDifferentNodes.end(); ++khIt) {
        NodePtr node = khIt->lock();
        if (!node) {
            continue;
        }
        node->getEffectInstance()->endChanges();
    }

    view->setUpdatesEnabled(true);


    _model->refreshSelectionBboxAndRedrawView();
} // DSMoveKeysAndNodesCommand::moveSelection

int
DSMoveKeysAndNodesCommand::id() const
{
    return kAnimationModuleEditorMoveKeysCommandCompressionID;
}

bool
DSMoveKeysAndNodesCommand::mergeWith(const QUndoCommand *other)
{
    const DSMoveKeysAndNodesCommand *cmd = dynamic_cast<const DSMoveKeysAndNodesCommand *>(other);

    assert(cmd);
    if ( !cmd || ( cmd->id() != id() ) ) {
        return false;
    }

    if ( cmd->_keys.size() != _keys.size() ) {
        return false;
    }

    if ( cmd->_nodes.size() != _nodes.size() ) {
        return false;
    }

    {
        AnimKeyFramePtrList::const_iterator itOther = cmd->_keys.begin();

        for (AnimKeyFramePtrList::const_iterator it = _keys.begin(); it != _keys.end(); ++it, ++itOther) {
            if (*itOther != *it) {
                return false;
            }
        }
    }

    std::vector<NodeAnimPtr >::const_iterator itOther = cmd->_nodes.begin();
    for (std::vector<NodeAnimPtr >::const_iterator it = _nodes.begin(); it != _nodes.end(); ++it, ++itOther) {
        if (*itOther != *it) {
            return false;
        }
    }

    _dt += cmd->_dt;

    return true;
}

//////////////////////////DSTransformKeysCommand //////////////////////////

DSTransformKeysCommand::DSTransformKeysCommand(const AnimKeyFramePtrList &keys,
                                               const Transform::Matrix3x3& transform,
                                               AnimationModuleEditor *model,
                                               QUndoCommand *parent)
    : QUndoCommand(parent)
    , _firstRedoCalled(false)
    , _transform(transform)
    , _model(model)
{
    for (AnimKeyFramePtrList::const_iterator it = keys.begin(); it != keys.end(); ++it) {
        KnobAnimPtr knobContext = (*it)->context.lock();
        if (!knobContext) {
            continue;
        }

        TransformKeyData& data = _keys[knobContext];
        data.keys.push_back(*it);
    }

    setText( tr("Scale keyframes") );
}

void
DSTransformKeysCommand::undo()
{
    /*
       The view is going to get MANY update() calls since every knob change will trigger a computeNodeRange() in DopeSheetView
       We explicitly disable update on the dopesheet view and re-enable it afterwards.
     */
    DopeSheetView* view = _model->getDopesheetView();

    view->setUpdatesEnabled(false);

    std::list<KnobHolderPtr> differentKnobs;
    for (TransformKeys::iterator it = _keys.begin(); it != _keys.end(); ++it) {
        KnobHolderPtr holder = it->first->getInternalKnob()->getHolder();
        if (holder) {
            if ( std::find(differentKnobs.begin(), differentKnobs.end(), holder) == differentKnobs.end() ) {
                differentKnobs.push_back(holder);
                holder->beginChanges();
            }
        }
    }

    for (TransformKeys::iterator it = _keys.begin(); it != _keys.end(); ++it) {
        it->first->getInternalKnob()->cloneCurve(ViewSpec::all(), it->first->getDimension(), *it->second.oldCurve, 0 /*offset*/, 0 /*range*/, 0 /*stringAnim*/, eCurveChangeReasonDopeSheet);
    }
    for (std::list<KnobHolderPtr>::iterator it = differentKnobs.begin(); it != differentKnobs.end(); ++it) {
        (*it)->endChanges();
    }

    view->setUpdatesEnabled(true);


    _model->refreshSelectionBboxAndRedrawView();
}

void
DSTransformKeysCommand::redo()
{
    /*
       The view is going to get MANY update() calls since every knob change will trigger a computeNodeRange() in DopeSheetView
       We explicitly disable update on the dopesheet view and re-enable it afterwards.
     */
    DopeSheetView* view = _model->getDopesheetView();

    view->setUpdatesEnabled(false);

    std::list<KnobHolderPtr> differentKnobs;
    for (TransformKeys::iterator it = _keys.begin(); it != _keys.end(); ++it) {
        KnobHolderPtr holder = it->first->getInternalKnob()->getHolder();
        if (holder) {
            if ( std::find(differentKnobs.begin(), differentKnobs.end(), holder) == differentKnobs.end() ) {
                differentKnobs.push_back(holder);
                holder->beginChanges();
            }
        }
    }

    if (!_firstRedoCalled) {
        for (TransformKeys::iterator it = _keys.begin(); it != _keys.end(); ++it) {
            it->second.oldCurve.reset( new Curve( *it->first->getInternalKnob()->getCurve( ViewIdx(0), it->first->getDimension() ) ) );
        }
        for (TransformKeys::iterator it = _keys.begin(); it != _keys.end(); ++it) {
            for (AnimKeyFramePtrList::iterator it2 = it->second.keys.begin(); it2 != it->second.keys.end(); ++it2) {
                transformKey(*it2);
            }
        }

        for (TransformKeys::iterator it = _keys.begin(); it != _keys.end(); ++it) {
            it->second.newCurve.reset( new Curve( *it->first->getInternalKnob()->getCurve( ViewIdx(0), it->first->getDimension() ) ) );
        }
        _firstRedoCalled = true;
    } else {
        for (TransformKeys::iterator it = _keys.begin(); it != _keys.end(); ++it) {
            it->first->getInternalKnob()->cloneCurve(ViewIdx(0), it->first->getDimension(), *it->second.newCurve, 0 /*offset*/, 0 /*range*/, 0 /*stringAnim*/, eCurveChangeReasonDopeSheet);
        }
    }

    for (std::list<KnobHolderPtr>::iterator it = differentKnobs.begin(); it != differentKnobs.end(); ++it) {
        (*it)->endChanges();
    }

    view->setUpdatesEnabled(true);


    _model->refreshSelectionBboxAndRedrawView();
}

void
DSTransformKeysCommand::transformKey(const AnimKeyFramePtr& key)
{
    KnobAnimPtr knobContext = key->context.lock();

    if (!knobContext) {
        return;
    }

    KnobIPtr knob = knobContext->getKnobGui()->getKnob();
    knob->transformValueAtTime(eCurveChangeReasonDopeSheet, key->key.getTime(), ViewSpec::all(), knobContext->getDimension(), _transform, &key->key);
}

int
DSTransformKeysCommand::id() const
{
    return kAnimationModuleEditorTransformKeysCommandCompressionID;
}

bool
DSTransformKeysCommand::mergeWith(const QUndoCommand *other)
{
    const DSTransformKeysCommand *cmd = dynamic_cast<const DSTransformKeysCommand *>(other);

    assert(cmd);
    if ( !cmd || ( cmd->id() != id() ) ) {
        return false;
    }

    if ( cmd->_keys.size() != _keys.size() ) {
        return false;
    }
    {
        TransformKeys::const_iterator itOther = cmd->_keys.begin();

        for (TransformKeys::iterator it = _keys.begin(); it != _keys.end(); ++it) {
            if ( itOther->second.keys.size() != it->second.keys.size() ) {
                return false;
            }

            AnimKeyFramePtrList::const_iterator kItOther = itOther->second.keys.begin();
            for (AnimKeyFramePtrList::const_iterator it2 = it->second.keys.begin(); it2 != it->second.keys.end(); ++it2, ++kItOther) {
                if (*it2 != *kItOther) {
                    return false;
                }
            }
        }
    }

    _transform = Transform::matMul(_transform, cmd->_transform);

    return true;
}

////////////////////////// DSLeftTrimReaderCommand //////////////////////////

DSLeftTrimReaderCommand::DSLeftTrimReaderCommand(const NodeAnimPtr &reader,
                                                 double oldTime,
                                                 double newTime,
                                                 QUndoCommand *parent)
    : QUndoCommand(parent),
    _readerContext(reader),
    _oldTime(oldTime),
    _newTime(newTime)
{
    setText( tr("Trim left") );
}

void
DSLeftTrimReaderCommand::undo()
{
    trimLeft(_oldTime);
}

void
DSLeftTrimReaderCommand::redo()
{
    trimLeft(_newTime);
}

void
DSLeftTrimReaderCommand::trimLeft(double firstFrame)
{
    NodeAnimPtr nodeContext = _readerContext.lock();

    if (!nodeContext) {
        return;
    }

    NodePtr node = nodeContext->getInternalNode();

    KnobIntBase *firstFrameKnob = dynamic_cast<KnobIntBase *>( node->getKnobByName(kReaderParamNameFirstFrame).get() );
    assert(firstFrameKnob);
    if (!firstFrameKnob) {
        return;
    }
    KnobHolderPtr holder = firstFrameKnob->getHolder();
    EffectInstancePtr effectInstance = toEffectInstance(holder);
    assert(effectInstance);
    if (!effectInstance) {
        return;
    }
    effectInstance->beginChanges();
    ValueChangedReturnCodeEnum r = firstFrameKnob->setValue(firstFrame, ViewSpec::all(), 0, eValueChangedReasonNatronGuiEdited, 0);
    effectInstance->endChanges();

    Q_UNUSED(r);
}

int
DSLeftTrimReaderCommand::id() const
{
    return kAnimationModuleEditorLeftTrimCommandCompressionID;
}

bool
DSLeftTrimReaderCommand::mergeWith(const QUndoCommand *other)
{
    const DSLeftTrimReaderCommand *cmd = dynamic_cast<const DSLeftTrimReaderCommand *>(other);

    assert(cmd);
    if ( !cmd || ( cmd->id() != id() ) ) {
        return false;
    }

    NodeAnimPtr node = _readerContext.lock();
    NodeAnimPtr otherNode = cmd->_readerContext.lock();
    if (!node || !otherNode) {
        return false;
    }

    if (node != otherNode) {
        return false;
    }

    _newTime = cmd->_oldTime;

    return true;
}

////////////////////////// DSRightTrimReaderCommand //////////////////////////

DSRightTrimReaderCommand::DSRightTrimReaderCommand(const NodeAnimPtr &reader,
                                                   double oldTime,
                                                   double newTime,
                                                   AnimationModuleEditor * /*model*/,
                                                   QUndoCommand *parent)
    : QUndoCommand(parent),
    _readerContext(reader),
    _oldTime(oldTime),
    _newTime(newTime)
{
    setText( tr("Trim right") );
}

void
DSRightTrimReaderCommand::undo()
{
    trimRight(_oldTime);
}

void
DSRightTrimReaderCommand::redo()
{
    trimRight(_newTime);
}

void
DSRightTrimReaderCommand::trimRight(double lastFrame)
{
    NodeAnimPtr nodeContext = _readerContext.lock();

    if (!nodeContext) {
        return;
    }

    NodePtr node = nodeContext->getInternalNode();

    KnobIntBase *lastFrameKnob = dynamic_cast<KnobIntBase *>( node->getKnobByName(kReaderParamNameLastFrame).get() );
    assert(lastFrameKnob);
    if (!lastFrameKnob) {
        return;
    }
    KnobHolderPtr holder = lastFrameKnob->getHolder();
    EffectInstancePtr effectInstance = toEffectInstance(holder);
    assert(effectInstance);
    if (!effectInstance) {
        return;
    }
    effectInstance->beginChanges();
    ValueChangedReturnCodeEnum r = lastFrameKnob->setValue(lastFrame, ViewSpec::all(), 0, eValueChangedReasonNatronGuiEdited, 0);
    effectInstance->endChanges();

    Q_UNUSED(r);
}

int
DSRightTrimReaderCommand::id() const
{
    return kAnimationModuleEditorRightTrimCommandCompressionID;
}

bool
DSRightTrimReaderCommand::mergeWith(const QUndoCommand *other)
{
    const DSRightTrimReaderCommand *cmd = dynamic_cast<const DSRightTrimReaderCommand *>(other);

    assert(cmd);
    if ( !cmd || ( cmd->id() != id() ) ) {
        return false;
    }

    NodeAnimPtr node = _readerContext.lock();
    NodeAnimPtr otherNode = cmd->_readerContext.lock();
    if (!node || !otherNode) {
        return false;
    }

    if (node != otherNode) {
        return false;
    }

    _newTime = cmd->_oldTime;

    return true;
}

////////////////////////// DSSlipReaderCommand //////////////////////////

DSSlipReaderCommand::DSSlipReaderCommand(const NodeAnimPtr &NodeAnimReader,
                                         double dt,
                                         AnimationModuleEditor *model,
                                         QUndoCommand *parent)
    : QUndoCommand(parent),
    _readerContext(NodeAnimReader),
    _dt(dt),
    _model(model)
{
    setText( tr("Slip reader") );
}

void
DSSlipReaderCommand::undo()
{
    slipReader(-_dt);
}

void
DSSlipReaderCommand::redo()
{
    slipReader(_dt);
}

int
DSSlipReaderCommand::id() const
{
    return kAnimationModuleEditorSlipReaderCommandCompressionID;
}

bool
DSSlipReaderCommand::mergeWith(const QUndoCommand *other)
{
    const DSSlipReaderCommand *cmd = dynamic_cast<const DSSlipReaderCommand *>(other);

    assert(cmd);
    if ( !cmd || ( cmd->id() != id() ) ) {
        return false;
    }

    NodeAnimPtr node = _readerContext.lock();
    NodeAnimPtr otherNode = cmd->_readerContext.lock();
    if (!node || !otherNode) {
        return false;
    }

    if (node != otherNode) {
        return false;
    }

    _dt += cmd->_dt;

    return true;
}

void
DSSlipReaderCommand::slipReader(double dt)
{
    NodeAnimPtr nodeContext = _readerContext.lock();

    if (!nodeContext) {
        return;
    }

    NodePtr node = nodeContext->getInternalNode();

    KnobIntBase *firstFrameKnob = dynamic_cast<KnobIntBase *>( node->getKnobByName(kReaderParamNameFirstFrame).get() );
    assert(firstFrameKnob);
    KnobIntBase *lastFrameKnob = dynamic_cast<KnobIntBase *>( node->getKnobByName(kReaderParamNameLastFrame).get() );
    assert(lastFrameKnob);
    KnobIntBase *timeOffsetKnob = dynamic_cast<KnobIntBase *>( node->getKnobByName(kReaderParamNameTimeOffset).get() );
    assert(timeOffsetKnob);
    KnobIntBase *startingTimeKnob = dynamic_cast<KnobIntBase *>( node->getKnobByName(kReaderParamNameStartingTime).get() );
    assert(startingTimeKnob);
    if (!firstFrameKnob || !lastFrameKnob || !timeOffsetKnob || !startingTimeKnob) {
        return;
    }


    /*
       Since the lastFrameKnob and startingTimeKnob have their signal connected to the computeNodeRange function in DopeSheetview
       We disconnect them beforehand and reconnect them afterwards, otherwise the dopesheet view is going to get many redraw() calls
       for nothing.
     */
    DopeSheetView* view = _model->getDopesheetView();
    QObject::disconnect( lastFrameKnob->getSignalSlotHandler().get(), SIGNAL(valueChanged(ViewSpec,int,int)),
                         view, SLOT(onRangeNodeChanged(ViewSpec,int,int)) );
    QObject::disconnect( startingTimeKnob->getSignalSlotHandler().get(), SIGNAL(valueChanged(ViewSpec,int,int)),
                         view, SLOT(onRangeNodeChanged(ViewSpec,int,int)) );
    KnobHolderPtr holder = lastFrameKnob->getHolder();
    EffectInstancePtr effectInstance = toEffectInstance(holder);
    assert(effectInstance);
    if (!effectInstance) {
        return;
    }
    effectInstance->beginChanges();
    {
        ValueChangedReturnCodeEnum r;

        r = firstFrameKnob->setValue(firstFrameKnob->getValue() - dt, ViewSpec::all(), 0, eValueChangedReasonNatronGuiEdited, 0);
        Q_UNUSED(r);
        r = lastFrameKnob->setValue(lastFrameKnob->getValue() - dt, ViewSpec::all(),  0, eValueChangedReasonNatronGuiEdited, 0);
        Q_UNUSED(r);
        r = timeOffsetKnob->setValue(timeOffsetKnob->getValue() + dt, ViewSpec::all(), 0, eValueChangedReasonNatronGuiEdited, 0);
        Q_UNUSED(r);
    }
    effectInstance->endChanges();


    QObject::connect( lastFrameKnob->getSignalSlotHandler().get(), SIGNAL(valueChanged(ViewSpec,int,int)),
                      view, SLOT(onRangeNodeChanged(ViewSpec,int,int)) );
    QObject::connect( startingTimeKnob->getSignalSlotHandler().get(), SIGNAL(valueChanged(ViewSpec,int,int)),
                      view, SLOT(onRangeNodeChanged(ViewSpec,int,int)) );

    view->update();
} // DSSlipReaderCommand::slipReader

////////////////////////// DSRemoveKeysCommand //////////////////////////

DSRemoveKeysCommand::DSRemoveKeysCommand(const std::vector<AnimKeyFrame> &keys,
                                         AnimationModuleEditor *model,
                                         QUndoCommand *parent)
    : QUndoCommand(parent),
    _keys(keys),
    _model(model)
{
    setText( tr("Delete selected keyframes") );
}

void
DSRemoveKeysCommand::undo()
{
    addOrRemoveKeyframe(true);
}

void
DSRemoveKeysCommand::redo()
{
    addOrRemoveKeyframe(false);
}

void
DSRemoveKeysCommand::addOrRemoveKeyframe(bool add)
{
    std::set<KnobGuiPtr> knobsSet;
    for (std::vector<AnimKeyFrame>::iterator it = _keys.begin(); it != _keys.end(); ++it) {
        AnimKeyFrame selected = (*it);
        KnobAnimPtr knobContext = selected.context.lock();
        if (!knobContext) {
            continue;
        }

        KnobGuiPtr knobGui = knobContext->getKnobGui();
        assert(knobGui);

        KnobIPtr knob = knobGui->getKnob();
        assert(knob);
        std::pair<std::set<KnobGuiPtr>::iterator,bool> ok = knobsSet.insert(knobGui);
        if (ok.second) {
            knob->beginChanges();
        }
        if (add) {
            knob->setKeyframe( selected.key.getTime(), selected.key, knobContext->getDimension(), ViewIdx(0) );
        } else {
            knobGui->removeKeyFrame( selected.key.getTime(), knobContext->getDimension(), ViewIdx(0) );
            knobContext->getTreeItem()->setSelected(false);
        }
    }

    for (std::set<KnobGuiPtr>::iterator it = knobsSet.begin(); it != knobsSet.end(); ++it) {
        (*it)->getKnob()->endChanges();
    }

    _model->refreshSelectionBboxAndRedrawView();
}

////////////////////////// DSSetSelectedKeysInterpolationCommand //////////////////////////

DSSetSelectedKeysInterpolationCommand::DSSetSelectedKeysInterpolationCommand(const std::list<DSKeyInterpolationChange> &changes,
                                                                             AnimationModuleEditor *model,
                                                                             QUndoCommand *parent)
    : QUndoCommand(parent),
    _changes(changes),
    _model(model)
{
    setText( tr("Set selected keys interpolation") );
}

void
DSSetSelectedKeysInterpolationCommand::undo()
{
    setInterpolation(true);
}

void
DSSetSelectedKeysInterpolationCommand::redo()
{
    setInterpolation(false);
}

void
DSSetSelectedKeysInterpolationCommand::setInterpolation(bool undo)
{
    for (std::list<DSKeyInterpolationChange>::iterator it = _changes.begin(); it != _changes.end(); ++it) {
        KeyframeTypeEnum interp = undo ? it->_oldInterpType : it->_newInterpType;
        KnobAnimPtr knobContext = it->_key->context.lock();
        if (!knobContext) {
            continue;
        }

        knobContext->getKnobGui()->getKnob()->setInterpolationAtTime(eCurveChangeReasonDopeSheet,
                                                                     ViewSpec::all(),
                                                                     knobContext->getDimension(),
                                                                     it->_key->key.getTime(),
                                                                     interp,
                                                                     &it->_key->key);
    }

    _model->refreshSelectionBboxAndRedrawView();
}

////////////////////////// DSAddKeysCommand //////////////////////////

DSPasteKeysCommand::DSPasteKeysCommand(const std::vector<AnimKeyFrame> &keys,
                                       const std::list<KnobAnimPtr >& dstKnobs,
                                       bool pasteRelativeToRefTime,
                                       AnimationModuleEditor *model,
                                       QUndoCommand *parent)
    : QUndoCommand(parent),
    _refTime(0),
    _refKeyindex(-1),
    _pasteRelativeToRefTime(pasteRelativeToRefTime),
    _keys(),
    _model(model)
{

    for (std::list<KnobAnimPtr >::const_iterator it = dstKnobs.begin(); it != dstKnobs.end(); ++it) {
        _dstKnobs.push_back(*it);
    }

    _refTime = _model->getTimelineCurrentTime();
    setText( tr("Paste keyframes") );
    for (std::size_t i = 0; i < keys.size(); ++i) {
        _keys.push_back(keys[i]);
        if (_refKeyindex == -1) {
            _refKeyindex = i;
        } else {
            if ( keys[i].key.getTime() < _keys[_refKeyindex].key.getTime() ) {
                _refKeyindex = i;
            }
        }
    }
}

void
DSPasteKeysCommand::undo()
{
    addOrRemoveKeyframe(false);
}

void
DSPasteKeysCommand::redo()
{
    addOrRemoveKeyframe(true);
}

void
DSPasteKeysCommand::setKeyValueFromKnob(const KnobIPtr& knob, double keyTime, KeyFrame* key)
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
DSPasteKeysCommand::addOrRemoveKeyframe(bool add)
{
    for (std::list<boost::weak_ptr<KnobAnim> >::const_iterator it = _dstKnobs.begin(); it != _dstKnobs.end(); ++it) {
        KnobAnimPtr knobContext = it->lock();
        if (!knobContext) {
            continue;
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
} // DSPasteKeysCommand::addOrRemoveKeyframe

NATRON_NAMESPACE_EXIT;
