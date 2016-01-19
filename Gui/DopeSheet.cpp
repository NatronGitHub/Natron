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

#include "DopeSheet.h"

#include <algorithm>

// Qt includes
#include <QDebug> //REMOVEME
#include <QtEvents>
#include <QTreeWidget>
#include <QUndoStack>

// Natron includes
#include "Engine/GroupInput.h"
#include "Engine/GroupOutput.h"
#include "Engine/Knob.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/TimeLine.h"

#include "Gui/ActionShortcuts.h"
#include "Gui/DockablePanel.h"
#include "Gui/DopeSheetEditor.h"
#include "Gui/DopeSheetEditorUndoRedo.h"
#include "Gui/DopeSheetHierarchyView.h"
#include "Gui/DopeSheetView.h"
#include "Gui/Gui.h"
#include "Gui/KnobGui.h"
#include "Gui/Menu.h"
#include "Gui/NodeGui.h"
#include "Gui/NodeGraph.h"
#include "Gui/NodeGraphUndoRedo.h"

NATRON_NAMESPACE_USING

typedef std::map<boost::weak_ptr<KnobI>, KnobGui *> KnobsAndGuis;
typedef std::pair<QTreeWidgetItem *, boost::shared_ptr<DSNode> > TreeItemAndDSNode;
typedef std::pair<QTreeWidgetItem *, DSKnob *> TreeItemAndDSKnob;


#pragma message WARN("NO_DIM huh?")
const int NO_DIM = -5;


////////////////////////// Helpers //////////////////////////

bool nodeHasAnimation(const boost::shared_ptr<NodeGui> &nodeGui)
{
    const std::vector<boost::shared_ptr<KnobI> > &knobs = nodeGui->getNode()->getKnobs();

    for (std::vector<boost::shared_ptr<KnobI> >::const_iterator it = knobs.begin();
         it != knobs.end();
         ++it) {
        boost::shared_ptr<KnobI> knob = *it;

        if (knob->hasAnimation()) {
            return true;
        }
    }

    return false;
}


namespace {

bool nodeCanAnimate(const NodePtr &node)
{
    const std::vector<boost::shared_ptr<KnobI> > &knobs = node->getKnobs();

    for (std::vector<boost::shared_ptr<KnobI> >::const_iterator it = knobs.begin();
         it != knobs.end();
         ++it) {
        boost::shared_ptr<KnobI> knob = *it;

        if (knob->isAnimationEnabled()) {
            return true;
        }
    }

    return false;
}

QTreeWidgetItem *createKnobNameItem(const QString &text,
                                    Natron::DopeSheetItemType itemType,
                                    int dimension,
                                    QTreeWidgetItem *parent)
{
    QTreeWidgetItem *ret = new QTreeWidgetItem;

    if (parent) {
        parent->addChild(ret);
    }

    ret->setData(0, QT_ROLE_CONTEXT_TYPE, itemType);

    if (dimension != NO_DIM) {
        ret->setData(0, QT_ROLE_CONTEXT_DIM, dimension);
    }

    ret->setText(0, text);

    if (itemType == Natron::eDopeSheetItemTypeKnobRoot
            || itemType == Natron::eDopeSheetItemTypeKnobDim) {
        ret->setFlags(ret->flags() & ~Qt::ItemIsDragEnabled & ~Qt::ItemIsDropEnabled);
    }

    return ret;
}

} // anon namespace


////////////////////////// DopeSheet //////////////////////////

class NATRON_NAMESPACE::DopeSheetPrivate
{
public:
    DopeSheetPrivate(DopeSheetEditor* editor,DopeSheet *qq);
    ~DopeSheetPrivate();

    /* functions */
    Node *getNearestTimeFromOutputs_recursive(Node *node,std::list<Node*>& markedNodes) const;
    Node *getNearestReaderFromInputs_recursive(Node *node,std::list<Node*>& markedNodes) const;
    void getInputsConnected_recursive(Node *node, std::vector<boost::shared_ptr<DSNode> > *result) const;

    void pushUndoCommand(QUndoCommand *cmd);
    
    /* attributes */
    DopeSheet *q_ptr;
    DSTreeItemNodeMap treeItemNodeMap;

    DopeSheetSelectionModel *selectionModel;

    boost::scoped_ptr<QUndoStack> undoStack;

    std::vector<DopeSheetKey> keyframesClipboard;

    boost::shared_ptr<TimeLine> timeline;
    
    DopeSheetEditor* editor;
};

DopeSheetPrivate::DopeSheetPrivate(DopeSheetEditor* editor,DopeSheet *qq) :
    q_ptr(qq),
    treeItemNodeMap(),
    selectionModel(new DopeSheetSelectionModel(qq)),
    undoStack(new QUndoStack(qq)),
    keyframesClipboard(),
    timeline(),
    editor(editor)
{

}

DopeSheetPrivate::~DopeSheetPrivate()
{
    delete selectionModel;
}

Node *DopeSheetPrivate::getNearestTimeFromOutputs_recursive(Node *node,std::list<Node*>& markedNodes) const
{
    const std::list<Node *> &outputs = node->getGuiOutputs();
    if (std::find(markedNodes.begin(), markedNodes.end(), node) != markedNodes.end()) {
        return 0;
    }
    markedNodes.push_back(node);
    for (std::list<Node *>::const_iterator it = outputs.begin(); it != outputs.end(); ++it) {
        Node *output = (*it);

        std::string pluginID = output->getPluginID();

        if (pluginID == PLUGINID_OFX_RETIME
                || pluginID == PLUGINID_OFX_TIMEOFFSET
                || pluginID == PLUGINID_OFX_FRAMERANGE) {
            return output;
        }
        else {
            Node* ret =  getNearestTimeFromOutputs_recursive(output, markedNodes);
            if (ret) {
                return ret;
            }
        }
    }

    return NULL;
}

Node *DopeSheetPrivate::getNearestReaderFromInputs_recursive(Node *node,std::list<Node*>& markedNodes) const
{
    const std::vector<boost::shared_ptr<Node> > &inputs = node->getGuiInputs();
    if (std::find(markedNodes.begin(), markedNodes.end(), node) != markedNodes.end()) {
        return 0;
    }
    markedNodes.push_back(node);
    for (std::vector<boost::shared_ptr<Node> >::const_iterator it = inputs.begin(); it != inputs.end(); ++it) {
        NodePtr input = (*it);

        if (!input) {
            continue;
        }

        std::string pluginID = input->getPluginID();

        if (pluginID == PLUGINID_OFX_READOIIO ||
                pluginID == PLUGINID_OFX_READFFMPEG ||
                pluginID == PLUGINID_OFX_READPFM) {
            return input.get();
        }
        else {
            Node* ret = getNearestReaderFromInputs_recursive(input.get(), markedNodes);
            if (ret) {
                return ret;
            }
        }
    }

    return NULL;
}

void DopeSheetPrivate::getInputsConnected_recursive(Node *node, std::vector<boost::shared_ptr<DSNode> > *result) const
{
    const std::vector<boost::shared_ptr<Node> > &inputs = node->getGuiInputs();

    for (std::vector<boost::shared_ptr<Node> >::const_iterator it = inputs.begin(); it != inputs.end(); ++it) {
        NodePtr input = (*it);

        if (!input) {
            continue;
        }

        boost::shared_ptr<DSNode>dsNode = q_ptr->findDSNode(input.get());

        if (dsNode) {
            result->push_back(dsNode);
        }

        getInputsConnected_recursive(input.get(), result);
    }
}

void DopeSheetPrivate::pushUndoCommand(QUndoCommand *cmd)
{
    undoStack->setActive();
    undoStack->push(cmd);
}

DopeSheet::DopeSheet(Gui *gui, DopeSheetEditor* editor, const boost::shared_ptr<TimeLine> &timeline) :
    _imp(new DopeSheetPrivate(editor, this))
{
    _imp->timeline = timeline;

    gui->registerNewUndoStack(_imp->undoStack.get());
}

DopeSheet::~DopeSheet()
{
    _imp->treeItemNodeMap.clear();
}

DSTreeItemNodeMap DopeSheet::getItemNodeMap() const
{
    return _imp->treeItemNodeMap;
}

void DopeSheet::addNode(boost::shared_ptr<NodeGui> nodeGui)
{
    // Determinate the node type
    // It will be useful to identify and sort tree items
    Natron::DopeSheetItemType nodeType = Natron::eDopeSheetItemTypeCommon;

    NodePtr node = nodeGui->getNode();
    
    //Don't add to the dopesheet nodes that are used by Natron internally (such as rotopaint nodes or file dialog preview nodes)
    if (!node || !node->getGroup()) {
        return;
    }
    
    Natron::EffectInstance *effectInstance = node->getLiveInstance();


    std::string pluginID = node->getPluginID();

    if (pluginID == PLUGINID_OFX_READOIIO
            || pluginID == PLUGINID_OFX_READFFMPEG
            || pluginID == PLUGINID_OFX_READPFM) {
        nodeType = Natron::eDopeSheetItemTypeReader;
    }
    else if (dynamic_cast<NodeGroup *>(effectInstance)) {
        nodeType = Natron::eDopeSheetItemTypeGroup;
    }
    else if (pluginID == PLUGINID_OFX_RETIME) {
        nodeType = Natron::eDopeSheetItemTypeRetime;
    }
    else if (pluginID == PLUGINID_OFX_TIMEOFFSET) {
        nodeType = Natron::eDopeSheetItemTypeTimeOffset;
    }
    else if (pluginID == PLUGINID_OFX_FRAMERANGE) {
        nodeType = Natron::eDopeSheetItemTypeFrameRange;
    }

    // Discard specific nodes
    if (nodeType == Natron::eDopeSheetItemTypeCommon) {
        if (dynamic_cast<GroupInput *>(effectInstance) ||
                dynamic_cast<GroupOutput *>(effectInstance)) {
            return;
        }

        if (node->isRotoNode() || node->isRotoPaintingNode()) {
            return;
        }

        if (node->getKnobs().empty()) {
            return;
        }

        if (!nodeCanAnimate(node)) {
            return;
        }
    }

    nodeGui->ensurePanelCreated();

    boost::shared_ptr<DSNode>dsNode = createDSNode(nodeGui, nodeType);

    _imp->treeItemNodeMap.insert(TreeItemAndDSNode(dsNode->getTreeItem(), dsNode));

    Q_EMIT nodeAdded(dsNode.get());
}

void DopeSheet::removeNode(NodeGui *node)
{
    DSTreeItemNodeMap::iterator toRemove = _imp->treeItemNodeMap.end();

    for (DSTreeItemNodeMap::iterator it = _imp->treeItemNodeMap.begin();
         it != _imp->treeItemNodeMap.end();
         ++it)
    {
        if ((*it).second->getNodeGui().get() == node) {
            toRemove = it;

            break;
        }
    }

    if (toRemove == _imp->treeItemNodeMap.end()) {
        return;
    }

    boost::shared_ptr<DSNode> dsNode = (*toRemove).second;

    _imp->selectionModel->onNodeAboutToBeRemoved(dsNode);

    Q_EMIT nodeAboutToBeRemoved(dsNode.get());

    _imp->treeItemNodeMap.erase(toRemove);
}

boost::shared_ptr<DSNode> DopeSheet::mapNameItemToDSNode(QTreeWidgetItem *nodeTreeItem) const
{
    DSTreeItemNodeMap::const_iterator dsNodeIt = _imp->treeItemNodeMap.find(nodeTreeItem);

    if (dsNodeIt != _imp->treeItemNodeMap.end()) {
        return (*dsNodeIt).second;
    }

    return boost::shared_ptr<DSNode>();
}

boost::shared_ptr<DSKnob> DopeSheet::mapNameItemToDSKnob(QTreeWidgetItem *knobTreeItem) const
{
    boost::shared_ptr<DSKnob> ret;

    boost::shared_ptr<DSNode> dsNode = findParentDSNode(knobTreeItem);
    if (!dsNode) {
        return ret;
    }
    const DSTreeItemKnobMap& knobRows = dsNode->getItemKnobMap();

    DSTreeItemKnobMap::const_iterator clickedDSKnob = knobRows.find(knobTreeItem);

    if (clickedDSKnob == knobRows.end()) {
        ret.reset();
    }
    else {
        ret = clickedDSKnob->second;
    }

    return ret;
}

boost::shared_ptr<DSNode> DopeSheet::findParentDSNode(QTreeWidgetItem *treeItem) const
{
    QTreeWidgetItem *itemIt = treeItem;

    DSTreeItemNodeMap::const_iterator clickedDSNode = _imp->treeItemNodeMap.find(itemIt);
    while (clickedDSNode == _imp->treeItemNodeMap.end()) {
        if (!itemIt) {
            return boost::shared_ptr<DSNode>();
        }

        if (itemIt->parent()) {
            itemIt = itemIt->parent();
            clickedDSNode = _imp->treeItemNodeMap.find(itemIt);
        }
    }

    return (*clickedDSNode).second;
}

boost::shared_ptr<DSNode> DopeSheet::findDSNode(Node *node) const
{
    for (DSTreeItemNodeMap::const_iterator it = _imp->treeItemNodeMap.begin();
         it != _imp->treeItemNodeMap.end();
         ++it) {
        boost::shared_ptr<DSNode> dsNode = (*it).second;

        if (dsNode->getInternalNode().get() == node) {
            return dsNode;
        }
    }

    return boost::shared_ptr<DSNode>();
}

boost::shared_ptr<DSNode> DopeSheet::findDSNode(const boost::shared_ptr<KnobI> &knob) const
{
    for (DSTreeItemNodeMap::const_iterator it = _imp->treeItemNodeMap.begin(); it != _imp->treeItemNodeMap.end(); ++it) {
        boost::shared_ptr<DSNode>dsNode = (*it).second;

        const DSTreeItemKnobMap& knobRows = dsNode->getItemKnobMap();

        for (DSTreeItemKnobMap::const_iterator knobIt = knobRows.begin(); knobIt != knobRows.end(); ++knobIt) {
            boost::shared_ptr<DSKnob> dsKnob = (*knobIt).second;

            if (dsKnob->getKnobGui()->getKnob() == knob) {
                return dsNode;
            }
        }
    }

    return boost::shared_ptr<DSNode>();
}

boost::shared_ptr<DSKnob> DopeSheet::findDSKnob(KnobGui *knobGui) const
{
    for (DSTreeItemNodeMap::const_iterator it = _imp->treeItemNodeMap.begin(); it != _imp->treeItemNodeMap.end(); ++it) {
        boost::shared_ptr<DSNode>dsNode = (*it).second;

        const DSTreeItemKnobMap& knobRows = dsNode->getItemKnobMap();

        for (DSTreeItemKnobMap::const_iterator knobIt = knobRows.begin(); knobIt != knobRows.end(); ++knobIt) {
            boost::shared_ptr<DSKnob> dsKnob = (*knobIt).second;

            if (dsKnob->getKnobGui() == knobGui) {
                return dsKnob;
            }
        }
    }

    return boost::shared_ptr<DSKnob>();
}

bool DopeSheet::isPartOfGroup(DSNode *dsNode) const
{
    boost::shared_ptr<NodeGroup> parentGroup = boost::dynamic_pointer_cast<NodeGroup>(dsNode->getInternalNode()->getGroup());

    return (parentGroup);
}

boost::shared_ptr<DSNode> DopeSheet::getGroupDSNode(DSNode *dsNode) const
{
    boost::shared_ptr<NodeGroup> parentGroup = boost::dynamic_pointer_cast<NodeGroup>(dsNode->getInternalNode()->getGroup());

    boost::shared_ptr<DSNode> parentGroupDSNode;

    if (parentGroup) {
        parentGroupDSNode = findDSNode(parentGroup->getNode().get());
    }

    return parentGroupDSNode;
}

std::vector<boost::shared_ptr<DSNode> > DopeSheet::getImportantNodes(DSNode *dsNode) const
{
    std::vector<boost::shared_ptr<DSNode> > ret;

    Natron::DopeSheetItemType nodeType = dsNode->getItemType();

    if (nodeType == Natron::eDopeSheetItemTypeGroup) {
        NodeGroup *nodeGroup = dynamic_cast<NodeGroup *>(dsNode->getInternalNode()->getLiveInstance());
        assert(nodeGroup);

        NodeList nodes = nodeGroup->getNodes();
        for (NodeList::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
            NodePtr childNode = (*it);

            boost::shared_ptr<DSNode>isInDopeSheet = findDSNode(childNode.get());
            if (isInDopeSheet) {
                ret.push_back(isInDopeSheet);
            }
        }
    }
    else if (dsNode->isTimeNode()) {
        _imp->getInputsConnected_recursive(dsNode->getInternalNode().get(), &ret);
    }


    return ret;
}

/**
 * @brief DopeSheet::getNearestRetimeFromOutputs
 *
 * Returns the first Retime node connected in output with 'dsNode' in the node graph.
 */
boost::shared_ptr<DSNode> DopeSheet::getNearestTimeNodeFromOutputs(DSNode *dsNode) const
{
    std::list<Node*> markedNodes;
    Node *timeNode = _imp->getNearestTimeFromOutputs_recursive(dsNode->getInternalNode().get(),markedNodes);

    return findDSNode(timeNode);
}

Node *DopeSheet::getNearestReader(DSNode *timeNode) const
{
    assert(timeNode->isTimeNode());
    std::list<Node*> markedNodes;
    Node *nearestReader = _imp->getNearestReaderFromInputs_recursive(timeNode->getInternalNode().get(),markedNodes);

    return nearestReader;
}

DopeSheetSelectionModel *DopeSheet::getSelectionModel() const
{
    return _imp->selectionModel;
}

DopeSheetEditor*
DopeSheet::getEditor() const
{
    return _imp->editor;
}

void DopeSheet::deleteSelectedKeyframes()
{
    if (_imp->selectionModel->isEmpty()) {
        return;
    }

    std::vector<DopeSheetKey> toRemove =_imp->selectionModel->getKeyframesSelectionCopy();

    _imp->selectionModel->clearKeyframeSelection();

    _imp->pushUndoCommand(new DSRemoveKeysCommand(toRemove, _imp->editor));
}

namespace {
struct SortIncreasingFunctor {
    
    bool operator() (const DSKeyPtr& lhs,const DSKeyPtr& rhs) {
        boost::shared_ptr<DSKnob> leftKnobDs = lhs->getContext();
        boost::shared_ptr<DSKnob> rightKnobDs = rhs->getContext();
        if (leftKnobDs.get() < rightKnobDs.get()) {
            return true;
        } else if (leftKnobDs.get() > rightKnobDs.get()) {
            return false;
        } else {
            return lhs->key.getTime() < rhs->key.getTime();
        }
    }
};

struct SortDecreasingFunctor {
    
    bool operator() (const DSKeyPtr& lhs,const DSKeyPtr& rhs) {
        boost::shared_ptr<DSKnob> leftKnobDs = lhs->getContext();
        boost::shared_ptr<DSKnob> rightKnobDs = rhs->getContext();
        assert(leftKnobDs && rightKnobDs);
        if (leftKnobDs.get() < rightKnobDs.get()) {
            return true;
        } else if (leftKnobDs.get() > rightKnobDs.get()) {
            return false;
        } else {
            return lhs->key.getTime() > rhs->key.getTime();
        }
    }
};
}

void DopeSheet::moveSelectedKeysAndNodes(double dt)
{
    DSKeyPtrList selectedKeyframes;
    std::vector<boost::shared_ptr<DSNode> > selectedNodes;
    _imp->selectionModel->getCurrentSelection(&selectedKeyframes, &selectedNodes);
    
    ///Constraint dt according to keyframe positions
    double maxLeft = INT_MIN;
    double maxRight = INT_MAX;
    std::vector<DSKeyPtr> vect;
    for (DSKeyPtrList::iterator it = selectedKeyframes.begin(); it!=selectedKeyframes.end(); ++it) {
        boost::shared_ptr<DSKnob> knobDs = (*it)->getContext();
        if (!knobDs) {
            continue;
        }
        boost::shared_ptr<Curve> curve = knobDs->getKnobGui()->getCurve(knobDs->getDimension());
        assert(curve);
        KeyFrame prevKey,nextKey;
        if (curve->getNextKeyframeTime((*it)->key.getTime(), &nextKey)) {
            if (!_imp->selectionModel->keyframeIsSelected(knobDs, nextKey)) {
                double diff = nextKey.getTime() - (*it)->key.getTime() - 1;
                maxRight = std::max(0.,std::min(diff, maxRight));
            }
        }
        if (curve->getPreviousKeyframeTime((*it)->key.getTime(), &prevKey)) {
            if (!_imp->selectionModel->keyframeIsSelected(knobDs, prevKey)) {
                double diff = prevKey.getTime()  - (*it)->key.getTime() + 1;
                maxLeft = std::min(0.,std::max(diff, maxLeft));
            }
        }
        vect.push_back(*it);
    }
    dt = std::min(dt, maxRight);
    dt = std::max(dt, maxLeft);
    if (dt == 0) {
        return;
    }
    
    //Keyframes must be sorted in order according to the user movement otherwise if keyframes are next to each other we might override
    //another keyframe.
    //Can only call sort on random iterators
    if (dt < 0) {
        std::sort(vect.begin(), vect.end(), SortIncreasingFunctor());
    } else {
        std::sort(vect.begin(), vect.end(), SortDecreasingFunctor());
    }
    selectedKeyframes.clear();
    for (std::vector<DSKeyPtr>::iterator it = vect.begin(); it!=vect.end(); ++it) {
        selectedKeyframes.push_back(*it);
    }
    
    _imp->pushUndoCommand(new DSMoveKeysAndNodesCommand(selectedKeyframes, selectedNodes, dt, _imp->editor));
}

void DopeSheet::trimReaderLeft(const boost::shared_ptr<DSNode> &reader, double newFirstFrame)
{
    NodePtr node = reader->getInternalNode();

    Knob<int> *firstFrameKnob = dynamic_cast<Knob<int> *>(node->getKnobByName(kReaderParamNameFirstFrame).get());
    assert(firstFrameKnob);
    Knob<int> *lastFrameKnob = dynamic_cast<Knob<int> *>(node->getKnobByName(kReaderParamNameLastFrame).get());
    assert(lastFrameKnob);
    Knob<int> *originalFrameRangeKnob = dynamic_cast<Knob<int> *>(node->getKnobByName(kReaderParamNameOriginalFrameRange).get());
    assert(originalFrameRangeKnob);

    
    int firstFrame = firstFrameKnob->getValue();
    int lastFrame = lastFrameKnob->getValue();
    int originalFirstFrame = originalFrameRangeKnob->getValue();
    
    newFirstFrame = std::max((double)newFirstFrame, (double)originalFirstFrame);
    newFirstFrame = std::min((double)lastFrame, newFirstFrame);
    if (newFirstFrame == firstFrame) {
        return;
    }

    _imp->pushUndoCommand(new DSLeftTrimReaderCommand(reader, firstFrame, newFirstFrame));

}

void DopeSheet::trimReaderRight(const boost::shared_ptr<DSNode> &reader, double newLastFrame)
{
    NodePtr node = reader->getInternalNode();

    Knob<int> *firstFrameKnob = dynamic_cast<Knob<int> *>(node->getKnobByName(kReaderParamNameFirstFrame).get());
    assert(firstFrameKnob);
    Knob<int> *lastFrameKnob = dynamic_cast<Knob<int> *>(node->getKnobByName(kReaderParamNameLastFrame).get());
    assert(lastFrameKnob);
    Knob<int> *originalFrameRangeKnob = dynamic_cast<Knob<int> *>(node->getKnobByName(kReaderParamNameOriginalFrameRange).get());
    assert(originalFrameRangeKnob);

    int firstFrame = firstFrameKnob->getValue();
    int lastFrame = lastFrameKnob->getValue();
    int originalLastFrame = originalFrameRangeKnob->getValue(1);
    
    newLastFrame = std::min((double)newLastFrame, (double)originalLastFrame);
    newLastFrame = std::max((double)firstFrame, newLastFrame);
    if (newLastFrame == lastFrame) {
        return;
    }

    _imp->pushUndoCommand(new DSRightTrimReaderCommand(reader, lastFrame, newLastFrame, _imp->editor));

}

bool
DopeSheet::canSlipReader(const boost::shared_ptr<DSNode> &reader) const
{
    NodePtr node = reader->getInternalNode();
    
    Knob<int> *firstFrameKnob = dynamic_cast<Knob<int> *>(node->getKnobByName(kReaderParamNameFirstFrame).get());
    assert(firstFrameKnob);
    Knob<int> *lastFrameKnob = dynamic_cast<Knob<int> *>(node->getKnobByName(kReaderParamNameLastFrame).get());
    assert(lastFrameKnob);
    Knob<int> *originalFrameRangeKnob = dynamic_cast<Knob<int> *>(node->getKnobByName(kReaderParamNameOriginalFrameRange).get());
    assert(originalFrameRangeKnob);
    
    ///Slipping means moving the timeOffset parameter by dt and moving firstFrame and lastFrame by -dt
    ///dt is clamped (firstFrame-originalFirstFrame) and (originalLastFrame-lastFrame)
    
    int currentFirstFrame = firstFrameKnob->getValue();
    int currentLastFrame = lastFrameKnob->getValue();
    int originalFirstFrame = originalFrameRangeKnob->getValue(0);
    int originalLastFrame = originalFrameRangeKnob->getValue(1);
    
    if ((currentFirstFrame - originalFirstFrame) == 0 && (currentLastFrame - originalLastFrame) == 0) {
        return false;
    }
    return true;
}

void DopeSheet::slipReader(const boost::shared_ptr<DSNode> &reader, double dt)
{
    NodePtr node = reader->getInternalNode();

    Knob<int> *firstFrameKnob = dynamic_cast<Knob<int> *>(node->getKnobByName(kReaderParamNameFirstFrame).get());
    assert(firstFrameKnob);
    Knob<int> *lastFrameKnob = dynamic_cast<Knob<int> *>(node->getKnobByName(kReaderParamNameLastFrame).get());
    assert(lastFrameKnob);
    Knob<int> *originalFrameRangeKnob = dynamic_cast<Knob<int> *>(node->getKnobByName(kReaderParamNameOriginalFrameRange).get());
    assert(originalFrameRangeKnob);
    
    ///Slipping means moving the timeOffset parameter by dt and moving firstFrame and lastFrame by -dt
    ///dt is clamped (firstFrame-originalFirstFrame) and (originalLastFrame-lastFrame)

    int currentFirstFrame = firstFrameKnob->getValue();
    int currentLastFrame = lastFrameKnob->getValue();
    int originalFirstFrame = originalFrameRangeKnob->getValue(0);
    int originalLastFrame = originalFrameRangeKnob->getValue(1);
    
    dt = std::min(dt, (double)(currentFirstFrame - originalFirstFrame));
    dt = std::max(dt, (double)(currentLastFrame - originalLastFrame));
    
    if (dt != 0) {
        _imp->pushUndoCommand(new DSSlipReaderCommand(reader, dt, _imp->editor));
    }
}

void DopeSheet::copySelectedKeys()
{
    if (_imp->selectionModel->isEmpty()) {
        return;
    }

    _imp->keyframesClipboard.clear();

    DSKeyPtrList selectedKeyframes;
    std::vector<boost::shared_ptr<DSNode> > selectedNodes;
    _imp->selectionModel->getCurrentSelection(&selectedKeyframes, &selectedNodes);

    for (DSKeyPtrList::const_iterator it = selectedKeyframes.begin();
         it != selectedKeyframes.end();
         ++it) {
        DSKeyPtr selectedKey = (*it);

        _imp->keyframesClipboard.push_back(*selectedKey);
    }
}

void
DopeSheet::renameSelectedNode()
{
    DSKeyPtrList keys;
    std::vector<boost::shared_ptr<DSNode> > selectedNodes;
    _imp->selectionModel->getCurrentSelection(&keys, &selectedNodes);
    if (selectedNodes.empty() || selectedNodes.size() > 1) {
        natronErrorDialog(tr("Rename node").toStdString(), tr("You must select exactly 1 node to rename.").toStdString());
        return;
    }
    
    ;
    

    EditNodeNameDialog* dialog = new EditNodeNameDialog(selectedNodes.front()->getNodeGui(),_imp->editor);
    
    QPoint global = QCursor::pos();
    QSize sizeH = dialog->sizeHint();
    global.rx() -= sizeH.width() / 2;
    global.ry() -= sizeH.height() / 2;
    QPoint realPos = global;
    
    
    
    QObject::connect(dialog ,SIGNAL(rejected()), this, SLOT(onNodeNameEditDialogFinished()));
    QObject::connect(dialog ,SIGNAL(accepted()), this, SLOT(onNodeNameEditDialogFinished()));
    dialog->move( realPos.x(), realPos.y() );
    dialog->raise();
    dialog->show();

}

void
DopeSheet::onNodeNameEditDialogFinished()
{
    EditNodeNameDialog* dialog = qobject_cast<EditNodeNameDialog*>(sender());
    if (dialog) {
        QDialog::DialogCode code =  (QDialog::DialogCode)dialog->result();
        if (code == QDialog::Accepted) {
            QString newName = dialog->getTypedName();
            QString oldName = QString(dialog->getNode()->getNode()->getLabel().c_str());
            _imp->pushUndoCommand(new RenameNodeUndoRedoCommand(dialog->getNode(),oldName,newName));
            
        }
        dialog->deleteLater();
    }

}

void DopeSheet::pasteKeys()
{
    std::vector<DopeSheetKey> toPaste;

    for (std::vector<DopeSheetKey>::const_iterator it = _imp->keyframesClipboard.begin(); it != _imp->keyframesClipboard.end(); ++it) {
        DopeSheetKey key = (*it);

        toPaste.push_back(key);
    }
    if (!toPaste.empty()) {
        _imp->pushUndoCommand(new DSPasteKeysCommand(toPaste, _imp->editor));
    }
}

void
DopeSheet::pasteKeys(const std::vector<DopeSheetKey>& keys)
{
    if (!keys.empty()) {
        _imp->pushUndoCommand(new DSPasteKeysCommand(keys, _imp->editor));
    }
    
}

void DopeSheet::setSelectedKeysInterpolation(KeyframeTypeEnum keyType)
{

    if (_imp->selectionModel->isEmpty()) {
        return;
    }

    DSKeyPtrList selectedKeyframes;
    std::vector<boost::shared_ptr<DSNode> > selectedNodes;
    _imp->selectionModel->getCurrentSelection(&selectedKeyframes, &selectedNodes);
    std::list<DSKeyInterpolationChange> changes;

    for (DSKeyPtrList::iterator it = selectedKeyframes.begin();
         it != selectedKeyframes.end();
         ++it) {
        DSKeyPtr keyPtr = (*it);
        DSKeyInterpolationChange change(keyPtr->key.getInterpolation(), keyType, keyPtr);

        changes.push_back(change);
    }

    _imp->pushUndoCommand(new DSSetSelectedKeysInterpolationCommand(changes, _imp->editor));
}

void
DopeSheet::transformSelectedKeys(const Transform::Matrix3x3& transform)
{
    if (_imp->selectionModel->isEmpty()) {
        return;
    }
    DSKeyPtrList selectedKeyframes;
    std::vector<boost::shared_ptr<DSNode> > selectedNodes;
    _imp->selectionModel->getCurrentSelection(&selectedKeyframes, &selectedNodes);
    
    _imp->pushUndoCommand(new DSTransformKeysCommand(selectedKeyframes, transform, _imp->editor));
}

void DopeSheet::setUndoStackActive()
{
    _imp->undoStack->setActive();
}

void DopeSheet::emit_modelChanged()
{
    Q_EMIT modelChanged();
}

SequenceTime DopeSheet::getCurrentFrame() const
{
    return _imp->timeline->currentFrame();
}

boost::shared_ptr<DSNode> DopeSheet::createDSNode(const boost::shared_ptr<NodeGui> &nodeGui, Natron::DopeSheetItemType itemType)
{
    // Determinate the node type
    // It will be useful to identify and sort tree items
    NodePtr node = nodeGui->getNode();

    QTreeWidgetItem *nameItem = new QTreeWidgetItem;
    nameItem->setText(0, node->getLabel().c_str());
    nameItem->setData(0, QT_ROLE_CONTEXT_TYPE, itemType);
    nameItem->setData(0, QT_ROLE_CONTEXT_IS_ANIMATED, true);

    boost::shared_ptr<DSNode> dsNode(new DSNode(this, itemType, nodeGui, nameItem));

    connect(node.get(), SIGNAL(labelChanged(QString)),
            this, SLOT(onNodeNameChanged(QString)));

    return dsNode;
}
void DopeSheet::onNodeNameChanged(const QString &name)
{
    Node *node = qobject_cast<Node *>(sender());
    boost::shared_ptr<DSNode>dsNode = findDSNode(node);
    if (dsNode) {
        dsNode->getTreeItem()->setText(0, name);
    }
}

void DopeSheet::onKeyframeSetOrRemoved()
{
    boost::shared_ptr<DSKnob> dsKnob = findDSKnob(qobject_cast<KnobGui *>(sender()));
    if (dsKnob) {
        Q_EMIT keyframeSetOrRemoved(dsKnob.get());
    }

    Q_EMIT modelChanged();
}


////////////////////////// DSKnob //////////////////////////

class NATRON_NAMESPACE::DSKnobPrivate
{
public:
    DSKnobPrivate();
    ~DSKnobPrivate();

    /* attributes */
    int dimension;
    QTreeWidgetItem *nameItem;
    KnobGui *knobGui;
    boost::shared_ptr<KnobI> knob;
};

DSKnobPrivate::DSKnobPrivate() :
    dimension(-2),
    nameItem(0),
    knobGui(0),
    knob()
{}

DSKnobPrivate::~DSKnobPrivate()
{}


/**
 * @class DSKnob
 *
 * The DSKnob class describes a knob' set of keyframes in the
 * DopeSheet.
 */

/**
 * @brief DSKnob::DSKnob
 *
 * Constructs a DSKnob.
 * Adds an item in the hierarchy view with 'parentItem' as parent item.
 *
 * 'knob', 'dimension' and 'isMultiDim' areused to name this item.
 *
 *' knobGui' is used to ensure that the DopeSheet graphical elements will
 * properly react to any keyframe modification.
 *
 * /!\ We should improve the classes design.
 */
DSKnob::DSKnob(int dimension,
               QTreeWidgetItem *nameItem,
               KnobGui *knobGui) :
    _imp(new DSKnobPrivate)
{
    assert(knobGui);

    _imp->dimension = dimension;
    _imp->nameItem = nameItem;
    _imp->knobGui = knobGui;
    _imp->knob = knobGui->getKnob();
}

DSKnob::~DSKnob()
{}

QTreeWidgetItem *DSKnob::getTreeItem() const
{
    return _imp->nameItem;
}

QTreeWidgetItem *DSKnob::findDimTreeItem(int dimension) const
{
    if ( (isMultiDimRoot() && (dimension == -1) )  || (!isMultiDimRoot() && !dimension) ) {
        return _imp->nameItem;
    }

    QTreeWidgetItem *ret = 0;

    for (int i = 0; i < _imp->nameItem->childCount(); ++i) {
        QTreeWidgetItem *child = _imp->nameItem->child(i);

        if (dimension == child->data(0, QT_ROLE_CONTEXT_DIM).toInt()) {
            ret = child;

            break;
        }
    }

    return ret;
}

/**
 * @brief DSKnob::getKnobGui
 *
 *
 */
KnobGui *DSKnob::getKnobGui() const
{
    return _imp->knobGui;
}

boost::shared_ptr<KnobI> DSKnob::getInternalKnob() const
{
    return _imp->knob;
}

/**
 * @brief DSKnob::isMultiDimRoot
 *
 *
 */
bool DSKnob::isMultiDimRoot() const
{
    return (_imp->dimension == -1);
}

int DSKnob::getDimension() const
{
    return _imp->dimension;
}


////////////////////////// DopeSheetSelectionModel //////////////////////////

class NATRON_NAMESPACE::DopeSheetSelectionModelPrivate
{
public:
    DopeSheetSelectionModelPrivate()
    : dopeSheet(0)
    , selectedKeyframes()
    , selectedRangeNodes()
    {

    }

    /* attributes */
    DopeSheet *dopeSheet;
    DSKeyPtrList selectedKeyframes;
    std::list<boost::weak_ptr<DSNode> > selectedRangeNodes;
};

DopeSheetSelectionModel::DopeSheetSelectionModel(DopeSheet *dopeSheet) :
    _imp(new DopeSheetSelectionModelPrivate)
{
    _imp->dopeSheet = dopeSheet;
}

DopeSheetSelectionModel::~DopeSheetSelectionModel()
{

}

void DopeSheetSelectionModel::selectAll()
{
    std::vector<DopeSheetKey> result;

    DSTreeItemNodeMap nodeRows = _imp->dopeSheet->getItemNodeMap();
    std::vector<boost::shared_ptr<DSNode> > selectedNodes;
    for (DSTreeItemNodeMap::const_iterator it = nodeRows.begin(); it != nodeRows.end(); ++it) {
        const boost::shared_ptr<DSNode>& dsNode = (*it).second;

        const DSTreeItemKnobMap& dsKnobItems = dsNode->getItemKnobMap();
        
        for (DSTreeItemKnobMap::const_iterator itKnob = dsKnobItems.begin();
             itKnob != dsKnobItems.end();
             ++itKnob) {
            makeDopeSheetKeyframesForKnob((*itKnob).second, &result);
        }

        if (dsNode->isRangeDrawingEnabled()) {
            selectedNodes.push_back(dsNode);
        }
    }

    makeSelection(result, selectedNodes, (DopeSheetSelectionModel::SelectionTypeAdd |
                                          DopeSheetSelectionModel::SelectionTypeClear |
                                          DopeSheetSelectionModel::SelectionTypeRecurse));
}

void DopeSheetSelectionModel::makeDopeSheetKeyframesForKnob(const boost::shared_ptr<DSKnob> &dsKnob,
                                                            std::vector<DopeSheetKey> *result)
{
    assert(dsKnob);

    int dim = dsKnob->getDimension();

    int startDim = 0;
    int endDim = dsKnob->getInternalKnob()->getDimension();

    if (dim > -1) {
        startDim = dim;
        endDim = dim + 1;
    }

    for (int i = startDim; i < endDim; ++i) {
        KeyFrameSet keyframes = dsKnob->getKnobGui()->getCurve(i)->getKeyFrames_mt_safe();
        boost::shared_ptr<DSKnob> context;
        if (dim == -1) {
            QTreeWidgetItem *childItem = dsKnob->findDimTreeItem(i);
            context = _imp->dopeSheet->mapNameItemToDSKnob(childItem);
        }
        else {
            context = dsKnob;
        }

        for (KeyFrameSet::const_iterator kIt = keyframes.begin();
             kIt != keyframes.end();
             ++kIt) {
            const KeyFrame& kf = (*kIt);
            result->push_back(DopeSheetKey(context, kf));
        }
    }
}

void DopeSheetSelectionModel::clearKeyframeSelection()
{
    if (_imp->selectedKeyframes.empty() && _imp->selectedRangeNodes.empty()) {
        return;
    }

    makeSelection(std::vector<DopeSheetKey>(), std::vector<boost::shared_ptr<DSNode> >(), DopeSheetSelectionModel::SelectionTypeClear);
}

void DopeSheetSelectionModel::makeSelection(const std::vector<DopeSheetKey> &keys,
                                            const std::vector<boost::shared_ptr<DSNode> >& rangeNodes,
                                            SelectionTypeFlags selectionFlags)
{
    // Don't allow unsupported combinations
    assert(! (selectionFlags & DopeSheetSelectionModel::SelectionTypeAdd
              &&
              selectionFlags & DopeSheetSelectionModel::SelectionTypeToggle));

    bool hasChanged = false;
    if (selectionFlags & DopeSheetSelectionModel::SelectionTypeClear) {
        hasChanged = true;
        _imp->selectedKeyframes.clear();
        _imp->selectedRangeNodes.clear();
    }

    for (std::vector<DopeSheetKey>::const_iterator it = keys.begin(); it != keys.end(); ++it) {
        const DopeSheetKey& key = (*it);

        DSKeyPtrList::iterator isAlreadySelected = keyframeIsSelected(key);

        if (isAlreadySelected == _imp->selectedKeyframes.end()) {
            DSKeyPtr dsKey(new DopeSheetKey(key));
            hasChanged = true;
            _imp->selectedKeyframes.push_back(dsKey);
        }
        else if (selectionFlags & DopeSheetSelectionModel::SelectionTypeToggle) {
            _imp->selectedKeyframes.erase(isAlreadySelected);
            hasChanged = true;
        }
    }
    
    for (std::vector<boost::shared_ptr<DSNode> >::const_iterator it = rangeNodes.begin(); it!=rangeNodes.end(); ++it) {
        
        std::list<boost::weak_ptr<DSNode> >::iterator found = isRangeNodeSelected(*it);
        if (found == _imp->selectedRangeNodes.end()) {
            _imp->selectedRangeNodes.push_back(*it);
            hasChanged = true;
        } else if (selectionFlags & DopeSheetSelectionModel::SelectionTypeToggle) {
            _imp->selectedRangeNodes.erase(found);
            hasChanged = true;
        }

    }

    if (hasChanged) {
        emit_keyframeSelectionChanged(selectionFlags & DopeSheetSelectionModel::SelectionTypeRecurse);
    }
}

bool DopeSheetSelectionModel::isEmpty() const
{
    return _imp->selectedKeyframes.empty() && _imp->selectedRangeNodes.empty();
}

void DopeSheetSelectionModel::getCurrentSelection(DSKeyPtrList* keys, std::vector<boost::shared_ptr<DSNode> >* nodes) const
{
    *keys =  _imp->selectedKeyframes;
    for (std::list<boost::weak_ptr<DSNode> >::const_iterator it = _imp->selectedRangeNodes.begin(); it!=_imp->selectedRangeNodes.end(); ++it) {
        boost::shared_ptr<DSNode> n = it->lock();
        if (n) {
            nodes->push_back(n);
        }
    }
}

std::vector<DopeSheetKey> DopeSheetSelectionModel::getKeyframesSelectionCopy() const
{
    std::vector<DopeSheetKey> ret;

    for (DSKeyPtrList::iterator it = _imp->selectedKeyframes.begin();
         it != _imp->selectedKeyframes.end();
         ++it) {
        ret.push_back(DopeSheetKey(**it));
    }

    return ret;
}

bool
DopeSheetSelectionModel::hasSingleKeyFrameTimeSelected(double* time) const
{
    bool timeSet = false;
    KnobGui * knob = 0;
    if (_imp->selectedKeyframes.empty()) {
        return false;
    }
    for (DSKeyPtrList::iterator it = _imp->selectedKeyframes.begin(); it != _imp->selectedKeyframes.end(); ++it) {
        boost::shared_ptr<DSKnob> knobContext = (*it)->context.lock();
        assert(knobContext);
        if (!timeSet) {
            *time = (*it)->key.getTime();
            knob = knobContext->getKnobGui();
            timeSet = true;
        } else {
            if ((*it)->key.getTime() != *time || knobContext->getKnobGui() != knob) {
                return false;
            }
        }
    }
    return true;
}

int DopeSheetSelectionModel::getSelectedKeyframesCount() const
{
    return (int)_imp->selectedKeyframes.size();
}

bool DopeSheetSelectionModel::keyframeIsSelected(const boost::shared_ptr<DSKnob> &dsKnob, const KeyFrame &keyframe) const
{

    for (DSKeyPtrList::iterator it = _imp->selectedKeyframes.begin(); it != _imp->selectedKeyframes.end(); ++it) {
        boost::shared_ptr<DSKnob> knobContext = (*it)->context.lock();
        assert(knobContext);
        if (knobContext == dsKnob && (*it)->key.getTime() == keyframe.getTime()) {
            return true;
        }
    }

    return false;
}

std::list<boost::weak_ptr<DSNode> >::iterator
DopeSheetSelectionModel::isRangeNodeSelected(const boost::shared_ptr<DSNode>& node)
{
    for (std::list<boost::weak_ptr<DSNode> >::iterator it = _imp->selectedRangeNodes.begin(); it!=_imp->selectedRangeNodes.end(); ++it) {
        boost::shared_ptr<DSNode> n = it->lock();
        if (n && n == node) {
            return it;
        }
    }
    return _imp->selectedRangeNodes.end();
}

DSKeyPtrList::iterator DopeSheetSelectionModel::keyframeIsSelected(const DopeSheetKey &key) const
{
    for (DSKeyPtrList::iterator it = _imp->selectedKeyframes.begin(); it != _imp->selectedKeyframes.end(); ++it) {
        if (**it == key) {
            return it;
        }
    }

    return _imp->selectedKeyframes.end();
}

bool
DopeSheetSelectionModel::rangeIsSelected(const boost::shared_ptr<DSNode>& node) const
{
    for (std::list<boost::weak_ptr<DSNode> >::iterator it = _imp->selectedRangeNodes.begin(); it!=_imp->selectedRangeNodes.end(); ++it) {
        boost::shared_ptr<DSNode> n = it->lock();
        if (n && n == node) {
            return true;
        }
    }
    return false;

}

void DopeSheetSelectionModel::emit_keyframeSelectionChanged(bool recurse)
{
    Q_EMIT keyframeSelectionChangedFromModel(recurse);
}

void DopeSheetSelectionModel::onNodeAboutToBeRemoved(const boost::shared_ptr<DSNode> &removed)
{
    for (DSKeyPtrList::iterator it = _imp->selectedKeyframes.begin();
         it != _imp->selectedKeyframes.end();) {
        DSKeyPtr key = (*it);

        boost::shared_ptr<DSKnob> knobContext = key->context.lock();
        assert(knobContext);

        if (_imp->dopeSheet->findDSNode(knobContext->getInternalKnob()) == removed) {
            it = _imp->selectedKeyframes.erase(it);
        } else {
            ++it;
        }
    }
}


////////////////////////// DSNode //////////////////////////

class NATRON_NAMESPACE::DSNodePrivate
{
public:
    DSNodePrivate();
    ~DSNodePrivate();

    /* functions */
    void initGroupNode();

    /* attributes */
    DopeSheet *dopeSheetModel;

    Natron::DopeSheetItemType nodeType;

    boost::weak_ptr<NodeGui> nodeGui;

    QTreeWidgetItem *nameItem;

    DSTreeItemKnobMap itemKnobMap;

    bool isSelected;
};

DSNodePrivate::DSNodePrivate() :
    dopeSheetModel(0),
    nodeType(),
    nodeGui(),
    nameItem(0),
    itemKnobMap(),
    isSelected(false)
{}

DSNodePrivate::~DSNodePrivate()
{}

void DSNodePrivate::initGroupNode()
{
   /* boost::shared_ptr<NodeGui> node = nodeGui.lock();
    if (!node) {
        return;
    }
    boost::shared_ptr<Node> natronnode = node->getNode();
    assert(natronnode);
    NodeGroup* nodegroup = dynamic_cast<NodeGroup *>(natronnode->getLiveInstance());
    assert(nodegroup);
    if (!nodegroup) {
        return;
    }
    NodeList subNodes = nodegroup->getNodes();

    for (NodeList::const_iterator it = subNodes.begin(); it != subNodes.end(); ++it) {
        NodePtr subNode = (*it);
        boost::shared_ptr<NodeGui> subNodeGui = boost::dynamic_pointer_cast<NodeGui>(subNode->getNodeGui());

        if (!subNodeGui || !subNodeGui->getSettingPanel() || !subNodeGui->isSettingsPanelVisible()) {
            continue;
        }
    }*/
}

DSNode::DSNode(DopeSheet *model,
               Natron::DopeSheetItemType itemType,
               const boost::shared_ptr<NodeGui> &nodeGui,
               QTreeWidgetItem *nameItem) :
    _imp(new DSNodePrivate)
{
    _imp->dopeSheetModel = model;
    _imp->nodeType = itemType;
    _imp->nameItem = nameItem;
    _imp->nodeGui = nodeGui;

    // Create dope sheet knobs
    const KnobsAndGuis &knobs = nodeGui->getKnobs();

    for (KnobsAndGuis::const_iterator it = knobs.begin();
         it != knobs.end(); ++it) {
        boost::shared_ptr<KnobI> knob = it->first.lock();
        if (!knob) {
            continue;
        }
        KnobGui *knobGui = it->second;

        if (!knob->canAnimate() || !knob->isAnimationEnabled()) {
            continue;
        }

        if (knob->getDimension() <= 1) {
            QTreeWidgetItem * nameItem = createKnobNameItem(knob->getLabel().c_str(),
                                                            Natron::eDopeSheetItemTypeKnobDim,
                                                            0,
                                                            _imp->nameItem);

            DSKnob *dsKnob = new DSKnob(0, nameItem, knobGui);
            _imp->itemKnobMap.insert(TreeItemAndDSKnob(nameItem, dsKnob));
        }
        else {
            QTreeWidgetItem *multiDimRootItem = createKnobNameItem(knob->getLabel().c_str(),
                                                                   Natron::eDopeSheetItemTypeKnobRoot,
                                                                   -1,
                                                                   _imp->nameItem);

            DSKnob *rootDSKnob = new DSKnob(-1, multiDimRootItem, knobGui);
            _imp->itemKnobMap.insert(TreeItemAndDSKnob(multiDimRootItem, rootDSKnob));

            for (int i = 0; i < knob->getDimension(); ++i) {
                QTreeWidgetItem *dimItem = createKnobNameItem(knob->getDimensionName(i).c_str(),
                                                              Natron::eDopeSheetItemTypeKnobDim,
                                                              i,
                                                              multiDimRootItem);

                DSKnob *dimDSKnob = new DSKnob(i, dimItem, knobGui);
                _imp->itemKnobMap.insert(TreeItemAndDSKnob(dimItem, dimDSKnob));
            }
        }

        QObject::connect(knobGui, SIGNAL(keyFrameSet()),
                         _imp->dopeSheetModel, SLOT(onKeyframeSetOrRemoved()));

        QObject::connect(knobGui, SIGNAL(keyFrameRemoved()),
                         _imp->dopeSheetModel, SLOT(onKeyframeSetOrRemoved()));
        
        QObject::connect(knobGui, SIGNAL(refreshDopeSheet()),
                         _imp->dopeSheetModel, SLOT(onKeyframeSetOrRemoved()));
    }

    // If some subnodes are already in the dope sheet, the connections must be set to update
    // the group's clip rect
    if (_imp->nodeType == Natron::eDopeSheetItemTypeGroup) {
        _imp->initGroupNode();
    }
}

/**
 * @brief DSNode::~DSNode
 *
 * Deletes all this node's params.
 */
DSNode::~DSNode()
{
    delete _imp->nameItem;

    _imp->itemKnobMap.clear();
}

/**
 * @brief DSNode::getNameItem
 *
 * Returns the hierarchy view item associated with this node.
 */
QTreeWidgetItem *DSNode::getTreeItem() const
{
    return _imp->nameItem;
}

/**
 * @brief DSNode::getNode
 *
 * Returns the associated node.
 */
boost::shared_ptr<NodeGui> DSNode::getNodeGui() const
{
    return _imp->nodeGui.lock();
}

boost::shared_ptr<Node> DSNode::getInternalNode() const
{
    boost::shared_ptr<NodeGui> node = getNodeGui();
    return node ? node->getNode() : NodePtr();
}

/**
 * @brief DSNode::getTreeItemsAndDSKnobs
 *
 *
 */
const DSTreeItemKnobMap& DSNode::getItemKnobMap() const
{
    return _imp->itemKnobMap;
}

Natron::DopeSheetItemType DSNode::getItemType() const
{
    return _imp->nodeType;
}

bool DSNode::isTimeNode() const
{
    return (_imp->nodeType >= Natron::eDopeSheetItemTypeRetime)
            && (_imp->nodeType < Natron::eDopeSheetItemTypeGroup);
}

bool DSNode::isRangeDrawingEnabled() const
{
    return (_imp->nodeType >= Natron::eDopeSheetItemTypeReader &&
            _imp->nodeType <= Natron::eDopeSheetItemTypeGroup);
}

bool DSNode::canContainOtherNodeContexts() const
{
    return (_imp->nodeType >= Natron::eDopeSheetItemTypeReader + 1
            && _imp->nodeType <= Natron::eDopeSheetItemTypeGroup);
}

bool DSNode::containsNodeContext() const
{
    for (int i = 0; i < _imp->nameItem->childCount(); ++i) {
        int childType = _imp->nameItem->child(i)->data(0, QT_ROLE_CONTEXT_TYPE).toInt();

        if (childType != Natron::eDopeSheetItemTypeKnobDim
                && childType != Natron::eDopeSheetItemTypeKnobRoot) {
            return true;
        }

    }

    return false;
}

#include "moc_DopeSheet.cpp"
