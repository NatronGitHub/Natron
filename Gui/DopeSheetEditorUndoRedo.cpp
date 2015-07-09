#include "DopeSheetEditorUndoRedo.h"

#include <QDebug> //REMOVEME

#include "Engine/Knob.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/NoOp.h"
#include <Engine/ViewerInstance.h>

#include "Global/GlobalDefines.h"

#include "Gui/DockablePanel.h"
#include "Gui/KnobGui.h"
#include "Gui/NodeGui.h"
#include "Gui/DopeSheetEditor.h"
#include "Gui/DopeSheet.h"


typedef std::map<boost::weak_ptr<KnobI>, KnobGui *> KnobsAndGuis;


////////////////////////// Helpers //////////////////////////

namespace {

/**
 * @brief A little helper to sort a DSKeyPtrList.
 *
 * This is used in the DSMoveKeysCommand.
 */
static bool dsSelectedKeyLessFunctor(const DSKeyPtr &left,
                                     const DSKeyPtr &right)
{
    return left->key.getTime() < right->key.getTime();
}

/**
 * @brief Move the node 'reader' on project timeline, by offsetting its
 * starting time by 'dt'.
 */
void moveReader(const NodePtr &reader, double dt)
{
    Knob<int> *startingTimeKnob = dynamic_cast<Knob<int> *>(reader->getKnobByName(kReaderParamNameStartingTime).get());
    assert(startingTimeKnob);
    (void)startingTimeKnob->setValue(startingTimeKnob->getValue() + dt, 0, Natron::eValueChangedReasonNatronGuiEdited, 0);
}
    
void moveTimeOffset(const NodePtr& node, double dt)
{
    Knob<int>* timeOffsetKnob = dynamic_cast<Knob<int>*>(node->getKnobByName(kTimeOffsetParamNameTimeOffset).get());
    assert(timeOffsetKnob);
    (void)timeOffsetKnob->setValue(timeOffsetKnob->getValue() + dt, 0, Natron::eValueChangedReasonNatronGuiEdited, 0);
}

void moveFrameRange(const NodePtr& node, double dt)
{
    Knob<int>* frameRangeKnob = dynamic_cast<Knob<int>*>(node->getKnobByName(kFrameRangeParamNameFrameRange).get());
    assert(frameRangeKnob);
    (void)frameRangeKnob->setValues(frameRangeKnob->getValue(0) + dt, frameRangeKnob->getValue(1)  + dt, Natron::eValueChangedReasonNatronGuiEdited);
}
    
void moveGroupNode(DopeSheetEditor* model, const NodePtr& node, double dt)
{
    NodeGroup *group = dynamic_cast<NodeGroup *>(node->getLiveInstance());
    assert(group);
    NodeList nodes;
    group->getNodes_recursive(nodes);
    
    for (NodeList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        boost::shared_ptr<NodeGui> nodeGui = boost::dynamic_pointer_cast<NodeGui>((*it)->getNodeGui());
        assert(nodeGui);
        std::string pluginID = (*it)->getPluginID();
        
        NodeGroup* isChildGroup = dynamic_cast<NodeGroup*>((*it)->getLiveInstance());
        
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
        const KnobsAndGuis &knobs = nodeGui->getKnobs();
        
        for (KnobsAndGuis::const_iterator knobIt = knobs.begin(); knobIt != knobs.end(); ++knobIt) {
            KnobGui *knobGui = (*knobIt).second;
            boost::shared_ptr<KnobI> knob = knobGui->getKnob();
            if (!knob->hasAnimation()) {
                continue;
            }
        
            for (int dim = 0; dim < knobGui->getKnob()->getDimension(); ++dim) {
                if (!knob->isAnimated(dim)) {
                    continue;
                }
                KeyFrameSet keyframes = knobGui->getCurve(dim)->getKeyFrames_mt_safe();
                
                for (KeyFrameSet::iterator kfIt = keyframes.begin(); kfIt != keyframes.end(); ++kfIt) {
                    KeyFrame kf = (*kfIt);
                    
                    KeyFrame fake;
                    
                    knob->moveValueAtTime(kf.getTime(), dim, dt, 0, &fake);
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
    std::set<boost::shared_ptr<Natron::Node> > nodesSet;
    for (std::vector<boost::shared_ptr<DSNode> >::const_iterator it = nodes.begin(); it!=nodes.end(); ++it) {
        DopeSheet::ItemType type = (*it)->getItemType();
        if (type != DopeSheet::ItemTypeReader &&
            type != DopeSheet::ItemTypeGroup &&
            type != DopeSheet::ItemTypeTimeOffset &&
            type != DopeSheet::ItemTypeFrameRange) {
            //Note that Retime nodes cannot be moved
            continue;
        }
        _nodes.push_back(*it);
        nodesSet.insert((*it)->getInternalNode());
        NodeGroup* isGroup = dynamic_cast<NodeGroup*>((*it)->getInternalNode()->getLiveInstance());
        if (isGroup) {
            NodeList recurseNodes;
            isGroup->getNodes_recursive(recurseNodes);
            for (NodeList::iterator it = recurseNodes.begin(); it!=recurseNodes.end(); ++it) {
                nodesSet.insert(*it);
            }
        }
    }
    
    for (DSKeyPtrList::iterator it = _keys.begin(); it != _keys.end(); ++it) {
        KnobHolder* holder = (*it)->getContext()->getInternalKnob()->getHolder();
        assert(holder);
        Natron::EffectInstance* isEffect = dynamic_cast<Natron::EffectInstance*>(holder);
        if (isEffect) {
            nodesSet.insert(isEffect->getNode());
        }
    }
    
    for (std::set<boost::shared_ptr<Natron::Node> >::iterator it = nodesSet.begin(); it!=nodesSet.end(); ++it) {
        _allDifferentNodes.push_back(*it);
    }
    _keys.sort(dsSelectedKeyLessFunctor);
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

    for (std::list<boost::weak_ptr<Natron::Node> >::iterator khIt = _allDifferentNodes.begin(); khIt != _allDifferentNodes.end(); ++khIt) {
        NodePtr node = khIt->lock();
        if (!node) {
            continue;
        }
        node->getLiveInstance()->beginChanges();
    }

    
    //////////Handle selected keyframes
    for (DSKeyPtrList::iterator it = _keys.begin(); it != _keys.end(); ++it) {
        const DSKeyPtr& selectedKey = (*it);

        boost::shared_ptr<DSKnob> knobContext = selectedKey->context.lock();
        if (!knobContext) {
            continue;
        }

        boost::shared_ptr<KnobI> knob = knobContext->getKnobGui()->getKnob();

        knob->moveValueAtTime(selectedKey->key.getTime(),
                              knobContext->getDimension(),
                              dt, 0, &selectedKey->key);
    }
    ////////////Handle selected nodes
    for (std::vector<boost::shared_ptr<DSNode> >::iterator it = _nodes.begin(); it != _nodes.end(); ++it) {
        DopeSheet::ItemType type = (*it)->getItemType();
        if (type == DopeSheet::ItemTypeReader) {
            moveReader((*it)->getInternalNode(), dt);
        } else if (type == DopeSheet::ItemTypeFrameRange)  {
            moveFrameRange((*it)->getInternalNode(), dt);
        } else if (type == DopeSheet::ItemTypeTimeOffset) {
            moveTimeOffset((*it)->getInternalNode(), dt);
        } else if (type == DopeSheet::ItemTypeGroup) {
            moveGroupNode(_model, (*it)->getInternalNode(), dt);
        }
    }
    

    for (std::list<boost::weak_ptr<Natron::Node> >::const_iterator khIt = _allDifferentNodes.begin(); khIt != _allDifferentNodes.end(); ++khIt) {
        NodePtr node = khIt->lock();
        if (!node) {
            continue;
        }
        node->getLiveInstance()->endChanges();
    }

    _model->refreshSelectionBboxAndRedrawView();
}

int DSMoveKeysAndNodesCommand::id() const
{
    return kDopeSheetEditorMoveKeysCommandCompressionID;
}

bool DSMoveKeysAndNodesCommand::mergeWith(const QUndoCommand *other)
{
    const DSMoveKeysAndNodesCommand *cmd = dynamic_cast<const DSMoveKeysAndNodesCommand *>(other);

    if (cmd->id() != id()) {
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

    KnobHolder *holder = firstFrameKnob->getHolder();
    Natron::EffectInstance *effectInstance = dynamic_cast<Natron::EffectInstance *>(holder);

    effectInstance->beginChanges();
    KnobHelper::ValueChangedReturnCodeEnum r = firstFrameKnob->setValue(firstFrame, 0, Natron::eValueChangedReasonNatronGuiEdited, 0);
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

    if (cmd->id() != id()) {
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

    KnobHolder *holder = lastFrameKnob->getHolder();
    Natron::EffectInstance *effectInstance = dynamic_cast<Natron::EffectInstance *>(holder);

    effectInstance->beginChanges();
    KnobHelper::ValueChangedReturnCodeEnum r = lastFrameKnob->setValue(lastFrame, 0, Natron::eValueChangedReasonNatronGuiEdited, 0);
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

    if (cmd->id() != id()) {
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
                                         DopeSheetEditor * /*model*/,
                                         QUndoCommand *parent) :
    QUndoCommand(parent),
    _readerContext(dsNodeReader),
    _dt(dt)
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

    if (cmd->id() != id()) {
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

    KnobHolder *holder = lastFrameKnob->getHolder();
    Natron::EffectInstance *effectInstance = dynamic_cast<Natron::EffectInstance *>(holder);

    effectInstance->beginChanges();
    {
        KnobHelper::ValueChangedReturnCodeEnum r;

        r = firstFrameKnob->setValue(firstFrameKnob->getValue() - dt, 0, Natron::eValueChangedReasonNatronGuiEdited, 0);
        r = lastFrameKnob->setValue(lastFrameKnob->getValue() - dt, 0, Natron::eValueChangedReasonNatronGuiEdited, 0);
        r = timeOffsetKnob->setValue(timeOffsetKnob->getValue() + dt, 0, Natron::eValueChangedReasonNatronGuiEdited, 0);

        Q_UNUSED(r);

    }
    effectInstance->endChanges();

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
        Natron::KeyframeTypeEnum interp = undo ? it->_oldInterpType : it->_newInterpType;

        boost::shared_ptr<DSKnob> knobContext = it->_key->context.lock();
        if (!knobContext) {
            continue;
        }

        knobContext->getKnobGui()->getKnob()->setInterpolationAtTime(knobContext->getDimension(),
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
    _keys(keys),
    _model(model)
{
    setText(QObject::tr("Paste keyframes"));
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
    for (std::vector<DopeSheetKey>::const_iterator it = _keys.begin(); it != _keys.end(); ++it) {
        DopeSheetKey key = (*it);

        boost::shared_ptr<DSKnob> knobContext = key.context.lock();
        if (!knobContext) {
            continue;
        }

        boost::shared_ptr<KnobI> knob = knobContext->getInternalKnob();
        knob->beginChanges();

        SequenceTime currentTime = _model->getTimelineCurrentTime();

        double keyTime = key.key.getTime();

        if (add) {
            Knob<double>* isDouble = dynamic_cast<Knob<double>*>(knob.get());
            Knob<bool>* isBool = dynamic_cast<Knob<bool>*>(knob.get());
            Knob<int>* isInt = dynamic_cast<Knob<int>*>(knob.get());
            Knob<std::string>* isString = dynamic_cast<Knob<std::string>*>(knob.get());

            if (isDouble) {
                isDouble->setValueAtTime(currentTime, isDouble->getValueAtTime(keyTime), knobContext->getDimension());
            } else if (isBool) {
                isBool->setValueAtTime(currentTime, isBool->getValueAtTime(keyTime), knobContext->getDimension());
            } else if (isInt) {
                isInt->setValueAtTime(currentTime, isInt->getValueAtTime(keyTime), knobContext->getDimension());
            } else if (isString) {
                isString->setValueAtTime(currentTime, isString->getValueAtTime(keyTime), knobContext->getDimension());
            }
        }
        else {
            knob->deleteValueAtTime(currentTime, knobContext->getDimension());
        }

        knob->endChanges();
    }

    _model->refreshSelectionBboxAndRedrawView();
}
