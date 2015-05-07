#include "DopeSheetEditorUndoRedo.h"

#include <QDebug>

#include "Engine/Knob.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/NoOp.h"

#include "Global/GlobalDefines.h"

#include "Gui/DockablePanel.h"
#include "Gui/DopeSheet.h"
#include "Gui/DopeSheetView.h"
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

} // anon namespace


////////////////////////// DSMoveKeysCommand //////////////////////////

DSMoveKeysCommand::DSMoveKeysCommand(const DSKeyPtrList &keys,
                                     double dt,
                                     DopeSheetView *view,
                                     QUndoCommand *parent) :
    QUndoCommand(parent),
    _keys(keys),
    _dt(dt),
    _view(view)
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
    for (DSKeyPtrList::iterator it = _keys.begin(); it != _keys.end(); ++it) {
        moveKey((*it), dt);
    }

    _view->computeSelectedKeysBRect();
    _view->update();
}

void DSMoveKeysCommand::moveKey(DSKeyPtr selectedKey, double dt)
{
    boost::shared_ptr<KnobI> knob = selectedKey->dsKnob->getKnobGui()->getKnob();

    knob->beginChanges();
    knob->moveValueAtTime(selectedKey->key.getTime(),
                          selectedKey->dimension,
                          dt, 0, &selectedKey->key);
    knob->endChanges();

    _view->update();
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

DSLeftTrimReaderCommand::DSLeftTrimReaderCommand(DSNode *dsNodeReader,
                                                 double oldTime,
                                                 double newTime,
                                                 DopeSheetView *view,
                                                 QUndoCommand *parent) :
    QUndoCommand(parent),
    _dsNodeReader(dsNodeReader),
    _oldTime(oldTime),
    _newTime(newTime),
    _view(view)
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

void DSLeftTrimReaderCommand::trimLeft(double time)
{
    Knob<int> *firstFrameKnob = dynamic_cast<Knob<int> *>
            (_dsNodeReader->getNodeGui()->getNode()->getKnobByName("firstFrame").get());

    firstFrameKnob->beginChanges();
    firstFrameKnob->setValue(time, 0, Natron::eValueChangedReasonNatronGuiEdited, 0);
    firstFrameKnob->endChanges();

    _view->update();
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

    if (cmd->_dsNodeReader != _dsNodeReader) {
        return false;
    }

    _newTime = cmd->_oldTime;

    return true;
}


////////////////////////// DSRightTrimReaderCommand //////////////////////////

DSRightTrimReaderCommand::DSRightTrimReaderCommand(DSNode *dsNodeReader,
                                                   double oldTime, double newTime,
                                                   DopeSheetView *view,
                                                   QUndoCommand *parent) :
    QUndoCommand(parent),
    _dsNodeReader(dsNodeReader),
    _oldTime(oldTime),
    _newTime(newTime),
    _view(view)
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

void DSRightTrimReaderCommand::trimRight(double time)
{
    Knob<int> *lastFrameKnob = dynamic_cast<Knob<int> *>
            (_dsNodeReader->getNodeGui()->getNode()->getKnobByName("lastFrame").get());

    lastFrameKnob->beginChanges();
    lastFrameKnob->setValue(time, 0, Natron::eValueChangedReasonNatronGuiEdited, 0);
    lastFrameKnob->endChanges();

    _view->update();
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

    if (cmd->_dsNodeReader != _dsNodeReader) {
        return false;
    }

    _newTime = cmd->_oldTime;

    return true;
}


////////////////////////// DSMoveReaderCommand //////////////////////////

DSMoveReaderCommand::DSMoveReaderCommand(DSNode *dsNodeReader,
                                         double oldTime,
                                         double newTime,
                                         DopeSheetView *view,
                                         QUndoCommand *parent) :
    QUndoCommand(parent),
    _dsNodeReader(dsNodeReader),
    _oldTime(oldTime),
    _newTime(newTime),
    _view(view)
{
    setText(QObject::tr("Move reader"));
}

void DSMoveReaderCommand::undo()
{
    moveClip(_oldTime);
}

void DSMoveReaderCommand::redo()
{
    moveClip(_newTime);
}

void DSMoveReaderCommand::moveClip(double time)
{
    Knob<int> *timeOffsetKnob = dynamic_cast<Knob<int> *>
            (_dsNodeReader->getNodeGui()->getNode()->getKnobByName("timeOffset").get());

    timeOffsetKnob->beginChanges();
    timeOffsetKnob->setValue(time, 0, Natron::eValueChangedReasonNatronGuiEdited, 0);
    timeOffsetKnob->endChanges();

    _view->update();
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

    if (cmd->_dsNodeReader != _dsNodeReader) {
        return false;
    }

    _newTime = cmd->_oldTime;

    return true;
}


////////////////////////// DSRemoveKeysCommand //////////////////////////

DSRemoveKeysCommand::DSRemoveKeysCommand(const std::vector<DSSelectedKey> &keys,
                                         DopeSheetView *view,
                                         QUndoCommand *parent) :
    QUndoCommand(parent),
    _keys(keys),
    _view(view)
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
    for (std::vector<DSSelectedKey>::iterator it = _keys.begin(); it != _keys.end(); ++it) {
        DSSelectedKey selected = (*it);
        KnobGui *knobGui = selected.dsKnob->getKnobGui();

        if (add) {
            knobGui->setKeyframe(selected.key.getTime(), selected.key, selected.dimension);
        }
        else {
            knobGui->removeKeyFrame(selected.key.getTime(), selected.dimension);
        }
    }

    _view->update();
}


////////////////////////// DSMoveGroupCommand //////////////////////////

DSMoveGroupCommand::DSMoveGroupCommand(DSNode *dsNodeGroup, double dt, DopeSheetView *view, QUndoCommand *parent) :
    QUndoCommand(parent),
    _dsNodeGroup(dsNodeGroup),
    _dt(dt),
    _view(view)
{
    setText(QObject::tr("Move Group Keyframes"));
}

void DSMoveGroupCommand::undo()
{
    moveGroupKeyframes(-_dt);
}

void DSMoveGroupCommand::redo()
{
    moveGroupKeyframes(_dt);
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

    if (cmd->_dsNodeGroup != _dsNodeGroup) {
        return false;
    }

    _dt += cmd->_dt;

    return true;
}

void DSMoveGroupCommand::moveGroupKeyframes(double dt)
{
    NodeGroup *group = dynamic_cast<NodeGroup *>(_dsNodeGroup->getNodeGui()->getNode()->getLiveInstance());
    NodeList nodes = group->getNodes();

    for (NodeList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        boost::shared_ptr<NodeGui> nodeGui = boost::dynamic_pointer_cast<NodeGui>((*it)->getNodeGui());

        if (dynamic_cast<GroupInput *>(nodeGui->getNode()->getLiveInstance()) ||
                dynamic_cast<GroupOutput *>(nodeGui->getNode()->getLiveInstance())) {
            continue;
        }

        if (!nodeGui->getSettingPanel() || !nodeGui->getSettingPanel()->isVisible()) {
            continue;
        }

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

                    knob->beginChanges();
                    knob->moveValueAtTime(kf.getTime(), dim, dt, 0, &fake);
                    knob->endChanges();
                }
            }
        }
    }

    _dsNodeGroup->computeClipRect();

    _view->clearKeyframeSelection();

    _view->update();
}
