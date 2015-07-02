#include "DopeSheetEditorUndoRedo.h"

#include <QDebug> //REMOVEME

#include "Engine/Knob.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/NoOp.h"
#include <Engine/ViewerInstance.h>

#include "Global/GlobalDefines.h"

#include "Gui/DockablePanel.h"
#include "Gui/DopeSheet.h"
#include "Gui/KnobGui.h"
#include "Gui/NodeGui.h"


typedef std::map<boost::weak_ptr<KnobI>, KnobGui *> KnobsAndGuis;


////////////////////////// Helpers //////////////////////////

namespace {

static bool dsSelectedKeyLessFunctor(const DSKeyPtr &left,
                                     const DSKeyPtr &right)
{
    return left->key.getTime() < right->key.getTime();
}

void renderOnce(Natron::EffectInstance *effectInstance)
{
    assert(effectInstance);

    std::set<ViewerInstance *> toRender;

    std::list<ViewerInstance *> connectedViewers;
    effectInstance->getNode()->hasViewersConnected(&connectedViewers);

    toRender.insert(connectedViewers.begin(), connectedViewers.end());

    for (std::set<ViewerInstance *>::const_iterator viIt = toRender.begin(); viIt != toRender.end(); ++viIt) {
        ViewerInstance *viewer = (*viIt);

        viewer->renderCurrentFrame(true);
    }
}

void renderOnce(const std::set<KnobHolder *>& holders)
{

    for (std::set<KnobHolder *>::const_iterator khIt = holders.begin(); khIt != holders.end(); ++khIt) {
        Natron::EffectInstance *effectInstance = dynamic_cast<Natron::EffectInstance *>(*khIt);

        effectInstance->endChanges();
    }

}

void moveReader(const NodePtr &reader, double time)
{
    Knob<int> *startingTimeKnob = dynamic_cast<Knob<int> *>(reader->getKnobByName("startingTime").get());
    assert(startingTimeKnob);

    KnobHolder *holder = startingTimeKnob->getHolder();
    Natron::EffectInstance *effectInstance = dynamic_cast<Natron::EffectInstance *>(holder);

    effectInstance->beginChanges();
    KnobHelper::ValueChangedReturnCodeEnum r = startingTimeKnob->setValue(startingTimeKnob->getValue() + time, 0, Natron::eValueChangedReasonNatronGuiEdited, 0);
    effectInstance->endChanges(); // don't discard changes here otherwise the hash of hte node is not updated

    Q_UNUSED(r);

}

} // anon namespace


////////////////////////// DSMoveKeysCommand //////////////////////////

DSMoveKeysCommand::DSMoveKeysCommand(const DSKeyPtrList &keys,
                                     double dt,
                                     DopeSheet *model,
                                     QUndoCommand *parent) :
    QUndoCommand(parent),
    _keys(keys),
    _dt(dt),
    _model(model)
{
    setText(QObject::tr("Move selected keys"));

    _keys.sort(dsSelectedKeyLessFunctor);
}

void DSMoveKeysCommand::undo()
{
    moveSelectedKeyframes(-_dt);
}

void DSMoveKeysCommand::redo()
{
    moveSelectedKeyframes(_dt);
}

void DSMoveKeysCommand::moveSelectedKeyframes(double dt)
{
    std::set<KnobHolder *> knobHolders;

    for (DSKeyPtrList::iterator it = _keys.begin(); it != _keys.end(); ++it) {
        DSKeyPtr selectedKey = (*it);

        boost::shared_ptr<DSKnob> knobContext = selectedKey->context.lock();

        if (!knobContext) {
            continue;
        }

        KnobHolder *holder = knobContext->getKnobGui()->getKnob()->getHolder();

        knobHolders.insert(holder);
    }

    for (std::set<KnobHolder *>::iterator khIt = knobHolders.begin(); khIt != knobHolders.end(); ++khIt) {
        Natron::EffectInstance *effectInstance = dynamic_cast<Natron::EffectInstance *>(*khIt);

        effectInstance->beginChanges();
    }

    for (DSKeyPtrList::iterator it = _keys.begin(); it != _keys.end(); ++it) {
        DSKeyPtr selectedKey = (*it);

        boost::shared_ptr<DSKnob> knobContext = selectedKey->context.lock();
        if (!knobContext) {
            continue;
        }

        boost::shared_ptr<KnobI> knob = knobContext->getKnobGui()->getKnob();

        knob->moveValueAtTime(selectedKey->key.getTime(),
                              knobContext->getDimension(),
                              dt, 0, &selectedKey->key);
    }

    renderOnce(knobHolders);

    _model->getSelectionModel()->emit_keyframeSelectionChanged();
}

int DSMoveKeysCommand::id() const
{
    return kDopeSheetEditorMoveKeysCommandCompressionID;
}

bool DSMoveKeysCommand::mergeWith(const QUndoCommand *other)
{
    const DSMoveKeysCommand *cmd = dynamic_cast<const DSMoveKeysCommand *>(other);

    if (cmd->id() != id()) {
        return false;
    }

    if (cmd->_keys.size() != _keys.size()) {
        return false;
    }

    DSKeyPtrList::const_iterator itOther = cmd->_keys.begin();

    for (DSKeyPtrList::const_iterator it = _keys.begin(); it != _keys.end(); ++it, ++itOther) {
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

    Knob<int> *firstFrameKnob = dynamic_cast<Knob<int> *>(node->getKnobByName("firstFrame").get());
    assert(firstFrameKnob);

    KnobHolder *holder = firstFrameKnob->getHolder();
    Natron::EffectInstance *effectInstance = dynamic_cast<Natron::EffectInstance *>(holder);

    effectInstance->beginChanges();
    KnobHelper::ValueChangedReturnCodeEnum r = firstFrameKnob->setValue(firstFrame, 0, Natron::eValueChangedReasonNatronGuiEdited, 0);
    effectInstance->endChanges(true);

    renderOnce(effectInstance);

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
                                                   DopeSheet *model,
                                                   QUndoCommand *parent) :
    QUndoCommand(parent),
    _readerContext(reader),
    _oldTime(oldTime),
    _newTime(newTime),
    _model(model)
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

    Knob<int> *lastFrameKnob = dynamic_cast<Knob<int> *>(node->getKnobByName("lastFrame").get());
    assert(lastFrameKnob);

    KnobHolder *holder = lastFrameKnob->getHolder();
    Natron::EffectInstance *effectInstance = dynamic_cast<Natron::EffectInstance *>(holder);

    effectInstance->beginChanges();
    KnobHelper::ValueChangedReturnCodeEnum r = lastFrameKnob->setValue(lastFrame, 0, Natron::eValueChangedReasonNatronGuiEdited, 0);
    effectInstance->endChanges(true);

    renderOnce(effectInstance);

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
                                         DopeSheet *model,
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

    Knob<int> *firstFrameKnob = dynamic_cast<Knob<int> *>(node->getKnobByName("firstFrame").get());
    assert(firstFrameKnob);
    Knob<int> *lastFrameKnob = dynamic_cast<Knob<int> *>(node->getKnobByName("lastFrame").get());
    assert(lastFrameKnob);
    Knob<int> *startingTimeKnob = dynamic_cast<Knob<int> *>(node->getKnobByName("timeOffset").get());
    assert(startingTimeKnob);

    KnobHolder *holder = lastFrameKnob->getHolder();
    Natron::EffectInstance *effectInstance = dynamic_cast<Natron::EffectInstance *>(holder);

    effectInstance->beginChanges();
    {
        KnobHelper::ValueChangedReturnCodeEnum r;
        int oldStartingTime = startingTimeKnob->getValue();

        r = firstFrameKnob->setValue(firstFrameKnob->getValue() + dt, 0, Natron::eValueChangedReasonNatronGuiEdited, 0);
        r = lastFrameKnob->setValue(lastFrameKnob->getValue() + dt, 0, Natron::eValueChangedReasonNatronGuiEdited, 0);

        r = startingTimeKnob->setValue(oldStartingTime, 0, Natron::eValueChangedReasonNatronGuiEdited, 0);

        Q_UNUSED(r);

    }
    effectInstance->endChanges(true);

    renderOnce(effectInstance);
}


////////////////////////// DSMoveReaderCommand //////////////////////////

DSMoveReaderCommand::DSMoveReaderCommand(const boost::shared_ptr<DSNode> &reader,
                                         double dt,
                                         DopeSheet *model,
                                         QUndoCommand *parent) :
    QUndoCommand(parent),
    _readerContext(reader),
    _dt(dt),
    _model(model)
{
    setText(QObject::tr("Move reader"));
}

void DSMoveReaderCommand::undo()
{
    boost::shared_ptr<DSNode> node = _readerContext.lock();
    if (!node) {
        return;
    }

    moveReader(node->getInternalNode(), -_dt);
}

void DSMoveReaderCommand::redo()
{
    boost::shared_ptr<DSNode> node = _readerContext.lock();
    if (!node) {
        return;
    }

    moveReader(node->getInternalNode(), _dt);
}

int DSMoveReaderCommand::id() const
{
    return kDopeSheetEditorMoveClipCommandCompressionID;
}

bool DSMoveReaderCommand::mergeWith(const QUndoCommand *other)
{
    const DSMoveReaderCommand *cmd = dynamic_cast<const DSMoveReaderCommand *>(other);

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


////////////////////////// DSRemoveKeysCommand //////////////////////////

DSRemoveKeysCommand::DSRemoveKeysCommand(const std::vector<DopeSheetKey> &keys,
                                         DopeSheet *model,
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

    _model->getSelectionModel()->emit_keyframeSelectionChanged();
}


////////////////////////// DSMoveGroupCommand //////////////////////////

DSMoveGroupCommand::DSMoveGroupCommand(const boost::shared_ptr<DSNode> &group, double dt, DopeSheet *model, QUndoCommand *parent) :
    QUndoCommand(parent),
    _groupContext(group),
    _dt(dt),
    _model(model)
{
    setText(QObject::tr("Move Group Keyframes"));
}

void DSMoveGroupCommand::undo()
{
    moveGroup(-_dt);
}

void DSMoveGroupCommand::redo()
{
    moveGroup(_dt);
}

int DSMoveGroupCommand::id() const
{
    return kDopeSheetEditorMoveGroupCommandCompressionID;
}

bool DSMoveGroupCommand::mergeWith(const QUndoCommand *other)
{
    const DSMoveGroupCommand *cmd = dynamic_cast<const DSMoveGroupCommand *>(other);

    if (cmd->id() != id()) {
        return false;
    }

    boost::shared_ptr<DSNode> node = _groupContext.lock();
    boost::shared_ptr<DSNode> otherNode = cmd->_groupContext.lock();
    if (!node || !otherNode) {
        return false;
    }

    if (node!= otherNode) {
        return false;
    }

    _dt += cmd->_dt;

    return true;
}

void DSMoveGroupCommand::moveGroup(double dt)
{
    boost::shared_ptr<DSNode> nodeContext = _groupContext.lock();
    if (!nodeContext) {
        return;
    }

    NodeGroup *group = dynamic_cast<NodeGroup *>(nodeContext->getInternalNode()->getLiveInstance());
    NodeList nodes = group->getNodes();

    std::set<KnobHolder *> knobHolders;

    for (NodeList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        boost::shared_ptr<NodeGui> nodeGui = boost::dynamic_pointer_cast<NodeGui>((*it)->getNodeGui());

        if (dynamic_cast<GroupInput *>(nodeGui->getNode()->getLiveInstance()) ||
                dynamic_cast<GroupOutput *>(nodeGui->getNode()->getLiveInstance())) {
            continue;
        }

        if (!nodeGui->getSettingPanel() || !nodeGui->isSettingsPanelVisible()) {
            continue;
        }

        NodePtr node = nodeGui->getNode();
        std::string pluginID = node->getPluginID();

        // Move readers
        if (pluginID == PLUGINID_OFX_READOIIO ||
                pluginID == PLUGINID_OFX_READFFMPEG ||
                pluginID == PLUGINID_OFX_READPFM) {
            moveReader(node, dt);
        }

        // Move keyframes
        const KnobsAndGuis &knobs = nodeGui->getKnobs();

        for (KnobsAndGuis::const_iterator knobIt = knobs.begin(); knobIt != knobs.end(); ++knobIt) {
            KnobGui *knobGui = (*knobIt).second;
            boost::shared_ptr<KnobI> knob = knobGui->getKnob();

            if (!knob->hasAnimation()) {
                continue;
            }

            KnobHolder *holder = knob->getHolder();
            knobHolders.insert(holder);

            Natron::EffectInstance *effectInstance = dynamic_cast<Natron::EffectInstance *>(holder);
            effectInstance->beginChanges();

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

    renderOnce(knobHolders);

    _model->getSelectionModel()->clearKeyframeSelection();
    _model->emit_modelChanged();
}


////////////////////////// DSSetSelectedKeysInterpolationCommand //////////////////////////

DSSetSelectedKeysInterpolationCommand::DSSetSelectedKeysInterpolationCommand(const std::list<DSKeyInterpolationChange> &changes,
                                                                             DopeSheet *model,
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

    _model->emit_modelChanged();
}


////////////////////////// DSAddKeysCommand //////////////////////////

DSPasteKeysCommand::DSPasteKeysCommand(const std::vector<DopeSheetKey> &keys,
                                       DopeSheet *model,
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

        SequenceTime currentTime = _model->getCurrentFrame();

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

    _model->emit_modelChanged();
}
