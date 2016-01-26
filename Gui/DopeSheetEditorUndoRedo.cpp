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

#include "DopeSheetEditorUndoRedo.h"

#include <QDebug> //REMOVEME

#include "Engine/Knob.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include <Engine/ViewerInstance.h>

#include "Global/GlobalDefines.h"

#include "Gui/DockablePanel.h"
#include "Gui/KnobGui.h"
#include "Gui/NodeGui.h"
#include "Gui/DopeSheetEditor.h"
#include "Gui/DopeSheet.h"
#include "Gui/DopeSheetView.h"

NATRON_NAMESPACE_ENTER;

typedef std::map<boost::weak_ptr<KnobI>, KnobGui *> KnobsAndGuis;


////////////////////////// Helpers //////////////////////////

namespace {



/**
 * @brief Move the node 'reader' on project timeline, by offsetting its
 * starting time by 'dt'.
 */
void moveReader(const NodePtr &reader, double dt)
{
    Knob<int> *startingTimeKnob = dynamic_cast<Knob<int> *>(reader->getKnobByName(kReaderParamNameStartingTime).get());
    assert(startingTimeKnob);
    KnobHelper::ValueChangedReturnCodeEnum s = startingTimeKnob->setValue(startingTimeKnob->getValue() + dt, 0, eValueChangedReasonNatronGuiEdited, 0);
    Q_UNUSED(s);
}
    
void moveTimeOffset(const NodePtr& node, double dt)
{
    Knob<int>* timeOffsetKnob = dynamic_cast<Knob<int>*>(node->getKnobByName(kTimeOffsetParamNameTimeOffset).get());
    assert(timeOffsetKnob);
    KnobHelper::ValueChangedReturnCodeEnum s = timeOffsetKnob->setValue(timeOffsetKnob->getValue() + dt, 0, eValueChangedReasonNatronGuiEdited, 0);
    Q_UNUSED(s);
}

void moveFrameRange(const NodePtr& node, double dt)
{
    Knob<int>* frameRangeKnob = dynamic_cast<Knob<int>*>(node->getKnobByName(kFrameRangeParamNameFrameRange).get());
    assert(frameRangeKnob);
    frameRangeKnob->setValues(frameRangeKnob->getValue(0) + dt, frameRangeKnob->getValue(1)  + dt, eValueChangedReasonNatronGuiEdited);
}
    
void moveGroupNode(DopeSheetEditor* model, const NodePtr& node, double dt)
{
    NodeGroup *group = node->isEffectGroup();
    assert(group);
    NodesList nodes;
    group->getNodes_recursive(nodes,true);
    
    for (NodesList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        NodeGuiPtr nodeGui = boost::dynamic_pointer_cast<NodeGui>((*it)->getNodeGui());
        assert(nodeGui);
        std::string pluginID = (*it)->getPluginID();
        
        NodeGroup* isChildGroup = (*it)->isEffectGroup();
        
        // Move readers
        if (pluginID == PLUGINID_OFX_READOIIO ||
            pluginID == PLUGINID_OFX_READFFMPEG ||
            pluginID == PLUGINID_OFX_READPFM) {
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
            const KnobPtr& knob = *knobIt;
            if (!knob->hasAnimation()) {
                continue;
            }
        
            for (int dim = 0; dim < knob->getDimension(); ++dim) {
                if (!knob->isAnimated(dim)) {
                    continue;
                }
                KeyFrameSet keyframes = knob->getCurve(dim)->getKeyFrames_mt_safe();
                
                for (KeyFrameSet::iterator kfIt = keyframes.begin(); kfIt != keyframes.end(); ++kfIt) {
                    KeyFrame kf = (*kfIt);
                    
                    KeyFrame fake;
                    
                    knob->moveValueAtTime(eCurveChangeReasonDopeSheet,kf.getTime(), dim, dt, 0, &fake);
                }
            }
        }
    }
    
}
    
} // anon namespace


////////////////////////// DSMoveKeysCommand //////////////////////////

DSMoveKeysAndNodesCommand::DSMoveKeysAndNodesCommand(const DSKeyPtrList &keys,
                                                         const std::vector<boost::shared_ptr<DSNode> >& nodes,
                                                         double dt,
                                                         DopeSheetEditor *model,
                                                         QUndoCommand *parent) :
QUndoCommand(parent),
_keys(keys),
_nodes(),
_dt(dt),
_model(model)
{
    setText(QObject::tr("Move selected keys"));
    std::set<NodePtr > nodesSet;
    for (std::vector<boost::shared_ptr<DSNode> >::const_iterator it = nodes.begin(); it!=nodes.end(); ++it) {
        DopeSheetItemType type = (*it)->getItemType();
        if (type != eDopeSheetItemTypeReader &&
            type != eDopeSheetItemTypeGroup &&
            type != eDopeSheetItemTypeTimeOffset &&
            type != eDopeSheetItemTypeFrameRange) {
            //Note that Retime nodes cannot be moved
            continue;
        }
        _nodes.push_back(*it);
        nodesSet.insert((*it)->getInternalNode());
        NodeGroup* isGroup = (*it)->getInternalNode()->isEffectGroup();
        if (isGroup) {
            NodesList recurseNodes;
            isGroup->getNodes_recursive(recurseNodes, true);
            for (NodesList::iterator it = recurseNodes.begin(); it!=recurseNodes.end(); ++it) {
                nodesSet.insert(*it);
            }
        }
    }
    
    for (DSKeyPtrList::iterator it = _keys.begin(); it != _keys.end(); ++it) {
        KnobHolder* holder = (*it)->getContext()->getInternalKnob()->getHolder();
        assert(holder);
        EffectInstance* isEffect = dynamic_cast<EffectInstance*>(holder);
        if (isEffect) {
            nodesSet.insert(isEffect->getNode());
        }
    }
    
    for (std::set<NodePtr >::iterator it = nodesSet.begin(); it!=nodesSet.end(); ++it) {
        _allDifferentNodes.push_back(*it);
    }
}

void DSMoveKeysAndNodesCommand::undo()
{
    moveSelection(-_dt);
}

void DSMoveKeysAndNodesCommand::redo()
{
    moveSelection(_dt);
}

void DSMoveKeysAndNodesCommand::moveSelection(double dt)
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
    for (DSKeyPtrList::iterator it = _keys.begin(); it != _keys.end(); ++it) {
        const DSKeyPtr& selectedKey = (*it);

        boost::shared_ptr<DSKnob> knobContext = selectedKey->context.lock();
        if (!knobContext) {
            continue;
        }

        KnobPtr knob = knobContext->getKnobGui()->getKnob();

        knob->moveValueAtTime(eCurveChangeReasonDopeSheet,selectedKey->key.getTime(),
                              knobContext->getDimension(),
                              dt, 0, &selectedKey->key);
    }
    ////////////Handle selected nodes
    for (std::vector<boost::shared_ptr<DSNode> >::iterator it = _nodes.begin(); it != _nodes.end(); ++it) {
        DopeSheetItemType type = (*it)->getItemType();
        if (type == eDopeSheetItemTypeReader) {
            moveReader((*it)->getInternalNode(), dt);
        } else if (type == eDopeSheetItemTypeFrameRange)  {
            moveFrameRange((*it)->getInternalNode(), dt);
        } else if (type == eDopeSheetItemTypeTimeOffset) {
            moveTimeOffset((*it)->getInternalNode(), dt);
        } else if (type == eDopeSheetItemTypeGroup) {
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
}

int DSMoveKeysAndNodesCommand::id() const
{
    return kDopeSheetEditorMoveKeysCommandCompressionID;
}

bool DSMoveKeysAndNodesCommand::mergeWith(const QUndoCommand *other)
{
    const DSMoveKeysAndNodesCommand *cmd = dynamic_cast<const DSMoveKeysAndNodesCommand *>(other);
    assert(cmd);
    if (!cmd || cmd->id() != id()) {
        return false;
    }

    if (cmd->_keys.size() != _keys.size()) {
        return false;
    }
    
    if (cmd->_nodes.size() != _nodes.size()) {
        return false;
    }
    
    {
        DSKeyPtrList::const_iterator itOther = cmd->_keys.begin();
        
        for (DSKeyPtrList::const_iterator it = _keys.begin(); it != _keys.end(); ++it, ++itOther) {
            if (*itOther != *it) {
                return false;
            }
        }
    }
    
    std::vector<boost::shared_ptr<DSNode> >::const_iterator itOther = cmd->_nodes.begin();
    for (std::vector<boost::shared_ptr<DSNode> >::const_iterator it = _nodes.begin(); it!=_nodes.end(); ++it,++itOther) {
        if (*itOther != *it) {
            return false;
        }
    }

    _dt += cmd->_dt;

    return true;
}

//////////////////////////DSTransformKeysCommand //////////////////////////

DSTransformKeysCommand::DSTransformKeysCommand(const DSKeyPtrList &keys,
                       const Transform::Matrix3x3& transform,
                       DopeSheetEditor *model,
                       QUndoCommand *parent)
: QUndoCommand(parent)
, _firstRedoCalled(false)
, _transform(transform)
, _model(model)
{
    for (DSKeyPtrList::const_iterator it = keys.begin(); it != keys.end(); ++it) {
        boost::shared_ptr<DSKnob> knobContext = (*it)->context.lock();
        if (!knobContext) {
            continue;
        }
        
        TransformKeyData& data = _keys[knobContext];
        data.keys.push_back(*it);
    }
    
    setText(QObject::tr("Scale keyframes"));
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
    
    std::list<KnobHolder*> differentKnobs;
    for (TransformKeys::iterator it = _keys.begin(); it != _keys.end(); ++it) {
        
        KnobHolder* holder = it->first->getInternalKnob()->getHolder();
        if (holder) {
            if ( std::find(differentKnobs.begin(), differentKnobs.end(), holder) == differentKnobs.end() ) {
                differentKnobs.push_back(holder);
                holder->beginChanges();
            }
        }
    }
    
    for (TransformKeys::iterator it = _keys.begin(); it != _keys.end(); ++it) {
        it->first->getInternalKnob()->cloneCurve(it->first->getDimension(),*it->second.oldCurve);
    }
    for (std::list<KnobHolder*>::iterator it= differentKnobs.begin(); it!=differentKnobs.end(); ++it) {
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
    
    std::list<KnobHolder*> differentKnobs;
    for (TransformKeys::iterator it = _keys.begin(); it != _keys.end(); ++it) {
        
        KnobHolder* holder = it->first->getInternalKnob()->getHolder();
        if (holder) {
            if ( std::find(differentKnobs.begin(), differentKnobs.end(), holder) == differentKnobs.end() ) {
                differentKnobs.push_back(holder);
                holder->beginChanges();
            }
        }
    }
    
    if (!_firstRedoCalled) {
        for (TransformKeys::iterator it = _keys.begin(); it != _keys.end(); ++it) {
            it->second.oldCurve.reset(new Curve(*it->first->getInternalKnob()->getCurve(it->first->getDimension())));
            
        }
        for (TransformKeys::iterator it = _keys.begin(); it != _keys.end(); ++it) {
            for (DSKeyPtrList::iterator it2 = it->second.keys.begin(); it2 != it->second.keys.end(); ++it2) {
                transformKey(*it2);
            }
        }

        for (TransformKeys::iterator it = _keys.begin(); it != _keys.end(); ++it) {
            it->second.newCurve.reset(new Curve(*it->first->getInternalKnob()->getCurve(it->first->getDimension())));
        }
        _firstRedoCalled = true;
    } else {
        for (TransformKeys::iterator it = _keys.begin(); it != _keys.end(); ++it) {
            it->first->getInternalKnob()->cloneCurve(it->first->getDimension(),*it->second.newCurve);
        }

    }
    
    for (std::list<KnobHolder*>::iterator it= differentKnobs.begin(); it!=differentKnobs.end(); ++it) {
        (*it)->endChanges();
    }
    
    view->setUpdatesEnabled(true);
    
    
    _model->refreshSelectionBboxAndRedrawView();

}

void
DSTransformKeysCommand::transformKey(const DSKeyPtr& key)
{
    boost::shared_ptr<DSKnob> knobContext = key->context.lock();
    if (!knobContext) {
        return;
    }
    
    KnobPtr knob = knobContext->getKnobGui()->getKnob();
    knob->transformValueAtTime(eCurveChangeReasonDopeSheet,key->key.getTime(), knobContext->getDimension(), _transform, &key->key);
}

int
DSTransformKeysCommand::id() const
{
    return kDopeSheetEditorTransformKeysCommandCompressionID;
}

bool
DSTransformKeysCommand::mergeWith(const QUndoCommand *other)
{
    const DSTransformKeysCommand *cmd = dynamic_cast<const DSTransformKeysCommand *>(other);
    assert(cmd);
    if (!cmd || cmd->id() != id()) {
        return false;
    }
    
    if (cmd->_keys.size() != _keys.size()) {
        return false;
    }
    {
        TransformKeys::const_iterator itOther = cmd->_keys.begin();
        
        for (TransformKeys::iterator it = _keys.begin(); it != _keys.end(); ++it) {
            if (itOther->second.keys.size() != it->second.keys.size()) {
                return false;
            }
            
            DSKeyPtrList::const_iterator kItOther = itOther->second.keys.begin();
            for (DSKeyPtrList::const_iterator it2 = it->second.keys.begin(); it2!=it->second.keys.end(); ++it2, ++kItOther) {
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

DSLeftTrimReaderCommand::DSLeftTrimReaderCommand(const boost::shared_ptr<DSNode> &reader,
                                                 double oldTime,
                                                 double newTime,
                                                 QUndoCommand *parent) :
    QUndoCommand(parent),
    _readerContext(reader),
    _oldTime(oldTime),
    _newTime(newTime)
{
    setText(QObject::tr("Trim left"));
}

void DSLeftTrimReaderCommand::undo()
{
    trimLeft(_oldTime);
}

void DSLeftTrimReaderCommand::redo()
{
    trimLeft(_newTime);
}

void DSLeftTrimReaderCommand::trimLeft(double firstFrame)
{
    boost::shared_ptr<DSNode> nodeContext = _readerContext.lock();
    if (!nodeContext) {
        return;
    }

    NodePtr node = nodeContext->getInternalNode();

    Knob<int> *firstFrameKnob = dynamic_cast<Knob<int> *>(node->getKnobByName(kReaderParamNameFirstFrame).get());
    assert(firstFrameKnob);
    if (!firstFrameKnob) {
        return;
    }
    KnobHolder *holder = firstFrameKnob->getHolder();
    EffectInstance *effectInstance = dynamic_cast<EffectInstance *>(holder);
    assert(effectInstance);
    if (!effectInstance) {
        return;
    }
    effectInstance->beginChanges();
    KnobHelper::ValueChangedReturnCodeEnum r = firstFrameKnob->setValue(firstFrame, 0, eValueChangedReasonNatronGuiEdited, 0);
    effectInstance->endChanges();

    Q_UNUSED(r);
}

int DSLeftTrimReaderCommand::id() const
{
    return kDopeSheetEditorLeftTrimCommandCompressionID;
}

bool DSLeftTrimReaderCommand::mergeWith(const QUndoCommand *other)
{
    const DSLeftTrimReaderCommand *cmd = dynamic_cast<const DSLeftTrimReaderCommand *>(other);
    assert(cmd);
    if (!cmd || cmd->id() != id()) {
        return false;
    }

    boost::shared_ptr<DSNode> node = _readerContext.lock();
    boost::shared_ptr<DSNode> otherNode = cmd->_readerContext.lock();
    if (!node || !otherNode) {
        return false;
    }

    if (node!= otherNode) {
        return false;
    }

    _newTime = cmd->_oldTime;

    return true;
}


////////////////////////// DSRightTrimReaderCommand //////////////////////////

DSRightTrimReaderCommand::DSRightTrimReaderCommand(const boost::shared_ptr<DSNode> &reader,
                                                   double oldTime, double newTime,
                                                   DopeSheetEditor * /*model*/,
                                                   QUndoCommand *parent) :
    QUndoCommand(parent),
    _readerContext(reader),
    _oldTime(oldTime),
    _newTime(newTime)
{
    setText(QObject::tr("Trim right"));
}

void DSRightTrimReaderCommand::undo()
{
    trimRight(_oldTime);
}

void DSRightTrimReaderCommand::redo()
{
    trimRight(_newTime);
}

void DSRightTrimReaderCommand::trimRight(double lastFrame)
{
    boost::shared_ptr<DSNode> nodeContext = _readerContext.lock();
    if (!nodeContext) {
        return;
    }

    NodePtr node = nodeContext->getInternalNode();

    Knob<int> *lastFrameKnob = dynamic_cast<Knob<int> *>(node->getKnobByName(kReaderParamNameLastFrame).get());
    assert(lastFrameKnob);
    if (!lastFrameKnob) {
        return;
    }
    KnobHolder *holder = lastFrameKnob->getHolder();
    EffectInstance *effectInstance = dynamic_cast<EffectInstance *>(holder);
    assert(effectInstance);
    if (!effectInstance) {
        return;
    }
    effectInstance->beginChanges();
    KnobHelper::ValueChangedReturnCodeEnum r = lastFrameKnob->setValue(lastFrame, 0, eValueChangedReasonNatronGuiEdited, 0);
    effectInstance->endChanges();

    Q_UNUSED(r);
}

int DSRightTrimReaderCommand::id() const
{
    return kDopeSheetEditorRightTrimCommandCompressionID;
}

bool DSRightTrimReaderCommand::mergeWith(const QUndoCommand *other)
{
    const DSRightTrimReaderCommand *cmd = dynamic_cast<const DSRightTrimReaderCommand *>(other);
    assert(cmd);
    if (!cmd || cmd->id() != id()) {
        return false;
    }

    boost::shared_ptr<DSNode> node = _readerContext.lock();
    boost::shared_ptr<DSNode> otherNode = cmd->_readerContext.lock();
    if (!node || !otherNode) {
        return false;
    }

    if (node!= otherNode) {
        return false;
    }

    _newTime = cmd->_oldTime;

    return true;
}


////////////////////////// DSSlipReaderCommand //////////////////////////

DSSlipReaderCommand::DSSlipReaderCommand(const boost::shared_ptr<DSNode> &dsNodeReader,
                                         double dt,
                                         DopeSheetEditor *model,
                                         QUndoCommand *parent) :
    QUndoCommand(parent),
    _readerContext(dsNodeReader),
    _dt(dt),
    _model(model)
{
    setText(QObject::tr("Slip reader"));
}

void DSSlipReaderCommand::undo()
{
    slipReader(-_dt);
}

void DSSlipReaderCommand::redo()
{
    slipReader(_dt);
}

int DSSlipReaderCommand::id() const
{
    return kDopeSheetEditorSlipReaderCommandCompressionID;
}

bool DSSlipReaderCommand::mergeWith(const QUndoCommand *other)
{
    const DSSlipReaderCommand *cmd = dynamic_cast<const DSSlipReaderCommand *>(other);
    assert(cmd);
    if (!cmd || cmd->id() != id()) {
        return false;
    }

    boost::shared_ptr<DSNode> node = _readerContext.lock();
    boost::shared_ptr<DSNode> otherNode = cmd->_readerContext.lock();
    if (!node || !otherNode) {
        return false;
    }

    if (node!= otherNode) {
        return false;
    }

    _dt += cmd->_dt;

    return true;
}

void DSSlipReaderCommand::slipReader(double dt)
{
    boost::shared_ptr<DSNode> nodeContext = _readerContext.lock();
    if (!nodeContext) {
        return;
    }

    NodePtr node = nodeContext->getInternalNode();

    Knob<int> *firstFrameKnob = dynamic_cast<Knob<int> *>(node->getKnobByName(kReaderParamNameFirstFrame).get());
    assert(firstFrameKnob);
    Knob<int> *lastFrameKnob = dynamic_cast<Knob<int> *>(node->getKnobByName(kReaderParamNameLastFrame).get());
    assert(lastFrameKnob);
    Knob<int> *timeOffsetKnob = dynamic_cast<Knob<int> *>(node->getKnobByName(kReaderParamNameTimeOffset).get());
    assert(timeOffsetKnob);
    Knob<int> *startingTimeKnob = dynamic_cast<Knob<int> *>(node->getKnobByName(kReaderParamNameStartingTime).get());
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
    QObject::disconnect(lastFrameKnob->getSignalSlotHandler().get(), SIGNAL(valueChanged(int, int)),
            view, SLOT(onRangeNodeChanged(int, int)));
    
    QObject::disconnect(startingTimeKnob->getSignalSlotHandler().get(), SIGNAL(valueChanged(int, int)),
            view, SLOT(onRangeNodeChanged(int, int)));

    
    KnobHolder *holder = lastFrameKnob->getHolder();
    EffectInstance *effectInstance = dynamic_cast<EffectInstance *>(holder);
    assert(effectInstance);
    if (!effectInstance) {
        return;
    }
    effectInstance->beginChanges();
    {
        KnobHelper::ValueChangedReturnCodeEnum r;

        r = firstFrameKnob->setValue(firstFrameKnob->getValue() - dt, 0, eValueChangedReasonNatronGuiEdited, 0);
        Q_UNUSED(r);
        r = lastFrameKnob->setValue(lastFrameKnob->getValue() - dt, 0, eValueChangedReasonNatronGuiEdited, 0);
        Q_UNUSED(r);
        r = timeOffsetKnob->setValue(timeOffsetKnob->getValue() + dt, 0, eValueChangedReasonNatronGuiEdited, 0);
        Q_UNUSED(r);
    }
    effectInstance->endChanges();
    
    
    QObject::connect(lastFrameKnob->getSignalSlotHandler().get(), SIGNAL(valueChanged(int, int)),
                        view, SLOT(onRangeNodeChanged(int, int)));
    
    QObject::connect(startingTimeKnob->getSignalSlotHandler().get(), SIGNAL(valueChanged(int, int)),
                        view, SLOT(onRangeNodeChanged(int, int)));

    view->update();
}



////////////////////////// DSRemoveKeysCommand //////////////////////////

DSRemoveKeysCommand::DSRemoveKeysCommand(const std::vector<DopeSheetKey> &keys,
                                         DopeSheetEditor *model,
                                         QUndoCommand *parent) :
    QUndoCommand(parent),
    _keys(keys),
    _model(model)
{
    setText(QObject::tr("Delete selected keyframes"));
}

void DSRemoveKeysCommand::undo()
{
    addOrRemoveKeyframe(true);
}

void DSRemoveKeysCommand::redo()
{
    addOrRemoveKeyframe(false);
}

void DSRemoveKeysCommand::addOrRemoveKeyframe(bool add)
{
    for (std::vector<DopeSheetKey>::iterator it = _keys.begin(); it != _keys.end(); ++it) {
        DopeSheetKey selected = (*it);

        boost::shared_ptr<DSKnob> knobContext = selected.context.lock();
        if (!knobContext) {
            continue;
        }

        KnobGui *knobGui = knobContext->getKnobGui();

        if (add) {
            knobGui->setKeyframe(selected.key.getTime(), selected.key, knobContext->getDimension());
        }
        else {
            knobGui->removeKeyFrame(selected.key.getTime(), knobContext->getDimension());
            knobContext->getTreeItem()->setSelected(false);
        }
    }

    _model->refreshSelectionBboxAndRedrawView();
}




////////////////////////// DSSetSelectedKeysInterpolationCommand //////////////////////////

DSSetSelectedKeysInterpolationCommand::DSSetSelectedKeysInterpolationCommand(const std::list<DSKeyInterpolationChange> &changes,
                                                                             DopeSheetEditor *model,
                                                                             QUndoCommand *parent) :
    QUndoCommand(parent),
    _changes(changes),
    _model(model)
{
    setText(QObject::tr("Set selected keys interpolation"));
}

void DSSetSelectedKeysInterpolationCommand::undo()
{
    setInterpolation(true);
}

void DSSetSelectedKeysInterpolationCommand::redo()
{
    setInterpolation(false);
}

void DSSetSelectedKeysInterpolationCommand::setInterpolation(bool undo)
{
    for (std::list<DSKeyInterpolationChange>::iterator it = _changes.begin(); it != _changes.end(); ++it) {
        KeyframeTypeEnum interp = undo ? it->_oldInterpType : it->_newInterpType;

        boost::shared_ptr<DSKnob> knobContext = it->_key->context.lock();
        if (!knobContext) {
            continue;
        }

        knobContext->getKnobGui()->getKnob()->setInterpolationAtTime(eCurveChangeReasonDopeSheet,knobContext->getDimension(),
                                                                     it->_key->key.getTime(),
                                                                     interp,
                                                                     &it->_key->key);
    }

    _model->refreshSelectionBboxAndRedrawView();
}


////////////////////////// DSAddKeysCommand //////////////////////////

DSPasteKeysCommand::DSPasteKeysCommand(const std::vector<DopeSheetKey> &keys,
                                       DopeSheetEditor *model,
                                       QUndoCommand *parent) :
    QUndoCommand(parent),
    _refTime(0),
    _refKeyindex(-1),
    _keys(),
    _model(model)
{
    _refTime = _model->getTimelineCurrentTime();
    setText(QObject::tr("Paste keyframes"));
    for (std::size_t i = 0; i < keys.size(); ++i) {
        _keys.push_back(keys[i]);
        if (_refKeyindex == -1) {
            _refKeyindex = i;
        } else {
            if (keys[i].key.getTime() < _keys[_refKeyindex].key.getTime()) {
                _refKeyindex = i;
            }
        }
    }
}

void DSPasteKeysCommand::undo()
{
    addOrRemoveKeyframe(false);
}

void DSPasteKeysCommand::redo()
{
    addOrRemoveKeyframe(true);
}

void DSPasteKeysCommand::addOrRemoveKeyframe(bool add)
{

    for (std::size_t i = 0; i < _keys.size(); ++i) {

        boost::shared_ptr<DSKnob> knobContext = _keys[i].context.lock();
        if (!knobContext) {
            continue;
        }
        int dim = knobContext->getDimension();
        
        KnobPtr knob = knobContext->getInternalKnob();
        knob->beginChanges();

        double keyTime = _keys[i].key.getTime();

        double setTime = keyTime - _keys[_refKeyindex].key.getTime() + _refTime;

        if (add) {
            Knob<double>* isDouble = dynamic_cast<Knob<double>*>(knob.get());
            Knob<bool>* isBool = dynamic_cast<Knob<bool>*>(knob.get());
            Knob<int>* isInt = dynamic_cast<Knob<int>*>(knob.get());
            Knob<std::string>* isString = dynamic_cast<Knob<std::string>*>(knob.get());
            
            for (int j = 0; j < knob->getDimension(); ++j) {
                if (dim == -1 || j == dim) {
                    KeyFrame k = _keys[i].key;
                    k.setTime(setTime);
                    
                    if (isDouble) {
                        k.setValue(isDouble->getValueAtTime(keyTime));
                    } else if (isBool) {
                        k.setValue(isBool->getValueAtTime(keyTime));
                    } else if (isInt) {
                        k.setValue(isInt->getValueAtTime(keyTime));
                    } else if (isString) {
                        std::string v = isString->getValueAtTime(keyTime);
                        double keyFrameValue = 0.;
                        AnimatingKnobStringHelper* isStringAnimatedKnob = dynamic_cast<AnimatingKnobStringHelper*>(this);
                        assert(isStringAnimatedKnob);
                        if (isStringAnimatedKnob) {
                            isStringAnimatedKnob->stringToKeyFrameValue(keyTime,v,&keyFrameValue);
                        }
                        k.setValue(keyFrameValue);
                    }
                    knob->setKeyFrame(k, j, eValueChangedReasonNatronGuiEdited);
                }
            }
        }
        else {
            for (int j = 0; j < knob->getDimension(); ++j) {
                if (dim == -1 || j == dim) {
                    knob->deleteValueAtTime(eCurveChangeReasonDopeSheet,setTime, j);
                }
            }
        }

        knob->endChanges();
    }

    _model->refreshSelectionBboxAndRedrawView();
}

NATRON_NAMESPACE_EXIT;
