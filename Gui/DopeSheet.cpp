/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2021 The Natron developers
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

#include "DopeSheet.h"

#include <algorithm>
#include <stdexcept>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#endif

// Qt includes
#include <QtCore/QDebug>
#include <QtCore/QSize>
#include <QtEvents>
#include <QTreeWidget>
#include <QUndoStack>

// Natron includes
#ifndef NATRON_ENABLE_IO_META_NODES
#include "Engine/AppInstance.h"
#endif
#include "Engine/GroupInput.h"
#include "Engine/GroupOutput.h"
#include "Engine/Knob.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/TimeLine.h"
#include "Engine/ReadNode.h"
#include "Engine/ViewIdx.h"

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

NATRON_NAMESPACE_ENTER

typedef std::map<KnobIWPtr, KnobGui *> KnobsAndGuis;
typedef std::pair<QTreeWidgetItem *, DSNodePtr> TreeItemAndDSNode;
typedef std::pair<QTreeWidgetItem *, DSKnob *> TreeItemAndDSKnob;


////////////////////////// Helpers //////////////////////////

bool
nodeHasAnimation(const NodeGuiPtr &nodeGui)
{
    const KnobsVec &knobs = nodeGui->getNode()->getKnobs();

    for (KnobsVec::const_iterator it = knobs.begin();
         it != knobs.end();
         ++it) {
        KnobIPtr knob = *it;

        if ( knob->hasAnimation() ) {
            return true;
        }
    }

    return false;
}

NATRON_NAMESPACE_ANONYMOUS_ENTER

bool
nodeCanAnimate(const NodePtr &node)
{
    const KnobsVec &knobs = node->getKnobs();

    for (KnobsVec::const_iterator it = knobs.begin();
         it != knobs.end();
         ++it) {
        KnobIPtr knob = *it;

        if ( knob->isAnimationEnabled() ) {
            return true;
        }
    }

    return false;
}

QTreeWidgetItem *
createKnobNameItem(const QString &text,
                   DopeSheetItemType itemType,
                   int dimension,
                   QTreeWidgetItem *parent)
{
    QTreeWidgetItem *ret = new QTreeWidgetItem;

    if (parent) {
        parent->addChild(ret);
    }

    ret->setData(0, QT_ROLE_CONTEXT_TYPE, itemType);

    ret->setData(0, QT_ROLE_CONTEXT_DIM, dimension);


    ret->setText(0, text);

    if ( (itemType == eDopeSheetItemTypeKnobRoot)
         || ( itemType == eDopeSheetItemTypeKnobDim) ) {
        ret->setFlags(ret->flags() & ~Qt::ItemIsDragEnabled & ~Qt::ItemIsDropEnabled);
    }

    return ret;
}

NATRON_NAMESPACE_ANONYMOUS_EXIT


////////////////////////// DopeSheet //////////////////////////

class DopeSheetPrivate
{
public:
    DopeSheetPrivate(DopeSheetEditor* editor,
                     DopeSheet *qq);
    ~DopeSheetPrivate();

    /* functions */
    Node * getNearestTimeFromOutputs_recursive(Node *node, std::list<Node*>& markedNodes) const;
    Node * getNearestReaderFromInputs_recursive(Node *node, std::list<Node*>& markedNodes) const;
    void getInputsConnected_recursive(Node *node, std::vector<DSNodePtr> *result) const;

    void pushUndoCommand(QUndoCommand *cmd);

    /* attributes */
    DopeSheet *q_ptr;
    DSTreeItemNodeMap treeItemNodeMap;
    DopeSheetSelectionModel *selectionModel;
    boost::scoped_ptr<QUndoStack> undoStack;
    std::vector<DopeSheetKey> keyframesClipboard;
    TimeLinePtr timeline;
    DopeSheetEditor* editor;
};

DopeSheetPrivate::DopeSheetPrivate(DopeSheetEditor* editor,
                                   DopeSheet *qq)
    : q_ptr(qq),
    treeItemNodeMap(),
    selectionModel( new DopeSheetSelectionModel(qq) ),
    undoStack( new QUndoStack(qq) ),
    keyframesClipboard(),
    timeline(),
    editor(editor)
{
}

DopeSheetPrivate::~DopeSheetPrivate()
{
    delete selectionModel;
}

void
DopeSheetPrivate::pushUndoCommand(QUndoCommand *cmd)
{
    if (editor) {
        editor->pushUndoCommand(cmd);
    } else {
        delete cmd;
    }
}

Node*
DopeSheetPrivate::getNearestTimeFromOutputs_recursive(Node *node,
                                                      std::list<Node*>& markedNodes) const
{
    const NodesWList &outputs = node->getGuiOutputs();

    if ( std::find(markedNodes.begin(), markedNodes.end(), node) != markedNodes.end() ) {
        return 0;
    }
    markedNodes.push_back(node);
    for (NodesWList::const_iterator it = outputs.begin(); it != outputs.end(); ++it) {
        NodePtr output = it->lock();
        std::string pluginID = output->getPluginID();

        if ( (pluginID == PLUGINID_OFX_RETIME)
             || ( pluginID == PLUGINID_OFX_TIMEOFFSET)
             || ( pluginID == PLUGINID_OFX_FRAMERANGE) ) {
            return output.get();
        } else {
            Node* ret =  getNearestTimeFromOutputs_recursive(output.get(), markedNodes);
            if (ret) {
                return ret;
            }
        }
    }

    return 0;
}

Node *
DopeSheetPrivate::getNearestReaderFromInputs_recursive(Node *node,
                                                       std::list<Node*>& markedNodes) const
{
    const std::vector<NodeWPtr> &inputs = node->getGuiInputs();

    if ( std::find(markedNodes.begin(), markedNodes.end(), node) != markedNodes.end() ) {
        return 0;
    }
    markedNodes.push_back(node);
    for (std::vector<NodeWPtr>::const_iterator it = inputs.begin(); it != inputs.end(); ++it) {
        NodePtr input = it->lock();

        if (!input) {
            continue;
        }

        std::string pluginID = input->getPluginID();
#ifndef NATRON_ENABLE_IO_META_NODES
        if ( ReadNode::isBundledReader( pluginID, input->getApp()->wasProjectCreatedWithLowerCaseIDs() ) ) {
#else
        if (pluginID == PLUGINID_NATRON_READ) {
#endif

            return input.get();
        } else {
            Node* ret = getNearestReaderFromInputs_recursive(input.get(), markedNodes);
            if (ret) {
                return ret;
            }
        }
    }

    return NULL;
}

void
DopeSheetPrivate::getInputsConnected_recursive(Node *node,
                                               std::vector<DSNodePtr> *result) const
{
    const std::vector<NodeWPtr> &inputs = node->getGuiInputs();

    for (std::vector<NodeWPtr>::const_iterator it = inputs.begin(); it != inputs.end(); ++it) {
        NodePtr input = it->lock();

        if (!input) {
            continue;
        }

        DSNodePtr dsNode = q_ptr->findDSNode( input.get() );

        if (dsNode) {
            result->push_back(dsNode);
        }

        getInputsConnected_recursive(input.get(), result);
    }
}

QUndoStack*
DopeSheet::getUndoStack() const
{
    return _imp->undoStack.get();
}

DopeSheet::DopeSheet(Gui *gui,
                     DopeSheetEditor* editor,
                     const TimeLinePtr &timeline)
    : _imp( new DopeSheetPrivate(editor, this) )
{
    _imp->timeline = timeline;

    gui->registerNewUndoStack( _imp->undoStack.get() );
}

DopeSheet::~DopeSheet()
{
    _imp->treeItemNodeMap.clear();
}

DSTreeItemNodeMap
DopeSheet::getItemNodeMap() const
{
    return _imp->treeItemNodeMap;
}

void
DopeSheet::addNode(NodeGuiPtr nodeGui)
{
    // Determinate the node type
    // It will be useful to identify and sort tree items
    DopeSheetItemType nodeType = eDopeSheetItemTypeCommon;
    NodePtr node = nodeGui->getNode();

    //Don't add to the dopesheet nodes that are used by Natron internally (such as rotopaint nodes or file dialog preview nodes)
    if ( !node || !node->getGroup() ) {
        return;
    }

    EffectInstancePtr effectInstance = node->getEffectInstance();
    std::string pluginID = node->getPluginID();

#ifndef NATRON_ENABLE_IO_META_NODES
    if ( ReadNode::isBundledReader( pluginID, node->getApp()->wasProjectCreatedWithLowerCaseIDs() ) ) {
#else
    if (pluginID == PLUGINID_NATRON_READ) {
#endif
        nodeType = eDopeSheetItemTypeReader;
    } else if (effectInstance->getPluginID() == PLUGINID_NATRON_GROUP) {
        nodeType = eDopeSheetItemTypeGroup;
    } else if (pluginID == PLUGINID_OFX_RETIME) {
        nodeType = eDopeSheetItemTypeRetime;
    } else if (pluginID == PLUGINID_OFX_TIMEOFFSET) {
        nodeType = eDopeSheetItemTypeTimeOffset;
    } else if (pluginID == PLUGINID_OFX_FRAMERANGE) {
        nodeType = eDopeSheetItemTypeFrameRange;
    }

    // Discard specific nodes
    if (nodeType == eDopeSheetItemTypeCommon) {
        if ( dynamic_cast<GroupInput *>( effectInstance.get() ) ||
             dynamic_cast<GroupOutput *>( effectInstance.get() ) ) {
            return;
        }

        if ( node->isRotoNode() || node->isRotoPaintingNode() ) {
            return;
        }

        if ( node->getKnobs().empty() ) {
            return;
        }

        if ( !nodeCanAnimate(node) ) {
            return;
        }
    }

    nodeGui->ensurePanelCreated();

    DSNodePtr dsNode = createDSNode(nodeGui, nodeType);

    _imp->treeItemNodeMap.insert( TreeItemAndDSNode(dsNode->getTreeItem(), dsNode) );

    Q_EMIT nodeAdded( dsNode.get() );
} // DopeSheet::addNode

void
DopeSheet::removeNode(NodeGui *node)
{
    DSTreeItemNodeMap::iterator toRemove = _imp->treeItemNodeMap.end();

    for (DSTreeItemNodeMap::iterator it = _imp->treeItemNodeMap.begin();
         it != _imp->treeItemNodeMap.end();
         ++it) {
        if ( (*it).second->getNodeGui().get() == node ) {
            toRemove = it;

            break;
        }
    }

    if ( toRemove == _imp->treeItemNodeMap.end() ) {
        return;
    }

    DSNodePtr dsNode = (*toRemove).second;

    _imp->selectionModel->onNodeAboutToBeRemoved(dsNode);

    Q_EMIT nodeAboutToBeRemoved( dsNode.get() );

    _imp->treeItemNodeMap.erase(toRemove);
}

DSNodePtr DopeSheet::mapNameItemToDSNode(QTreeWidgetItem *nodeTreeItem) const
{
    DSTreeItemNodeMap::const_iterator dsNodeIt = _imp->treeItemNodeMap.find(nodeTreeItem);

    if ( dsNodeIt != _imp->treeItemNodeMap.end() ) {
        return (*dsNodeIt).second;
    }

    return DSNodePtr();
}

DSKnobPtr DopeSheet::mapNameItemToDSKnob(QTreeWidgetItem *knobTreeItem) const
{
    DSKnobPtr ret;
    DSNodePtr dsNode = findParentDSNode(knobTreeItem);

    if (!dsNode) {
        return ret;
    }
    const DSTreeItemKnobMap& knobRows = dsNode->getItemKnobMap();
    DSTreeItemKnobMap::const_iterator clickedDSKnob = knobRows.find(knobTreeItem);

    if ( clickedDSKnob == knobRows.end() ) {
        ret.reset();
    } else {
        ret = clickedDSKnob->second;
    }

    return ret;
}

DSNodePtr DopeSheet::findParentDSNode(QTreeWidgetItem *treeItem) const
{
    QTreeWidgetItem *itemIt = treeItem;
    DSTreeItemNodeMap::const_iterator clickedDSNode = _imp->treeItemNodeMap.find(itemIt);

    while ( clickedDSNode == _imp->treeItemNodeMap.end() ) {
        if (!itemIt) {
            return DSNodePtr();
        }

        if ( itemIt->parent() ) {
            itemIt = itemIt->parent();
            clickedDSNode = _imp->treeItemNodeMap.find(itemIt);
        }
    }

    return (*clickedDSNode).second;
}

DSNodePtr DopeSheet::findDSNode(Node *node) const
{
    for (DSTreeItemNodeMap::const_iterator it = _imp->treeItemNodeMap.begin();
         it != _imp->treeItemNodeMap.end();
         ++it) {
        DSNodePtr dsNode = (*it).second;

        if (dsNode->getInternalNode().get() == node) {
            return dsNode;
        }
    }

    return DSNodePtr();
}

DSNodePtr DopeSheet::findDSNode(const KnobIPtr &knob) const
{
    for (DSTreeItemNodeMap::const_iterator it = _imp->treeItemNodeMap.begin(); it != _imp->treeItemNodeMap.end(); ++it) {
        DSNodePtr dsNode = (*it).second;
        const DSTreeItemKnobMap& knobRows = dsNode->getItemKnobMap();

        for (DSTreeItemKnobMap::const_iterator knobIt = knobRows.begin(); knobIt != knobRows.end(); ++knobIt) {
            DSKnobPtr dsKnob = (*knobIt).second;

            if (dsKnob && dsKnob->getKnobGui() && dsKnob->getKnobGui()->getKnob() == knob) {
                return dsNode;
            }
        }
    }

    return DSNodePtr();
}

DSKnobPtr DopeSheet::findDSKnob(const KnobGui* knobGui) const
{
    for (DSTreeItemNodeMap::const_iterator it = _imp->treeItemNodeMap.begin(); it != _imp->treeItemNodeMap.end(); ++it) {
        DSNodePtr dsNode = (*it).second;
        const DSTreeItemKnobMap& knobRows = dsNode->getItemKnobMap();

        for (DSTreeItemKnobMap::const_iterator knobIt = knobRows.begin(); knobIt != knobRows.end(); ++knobIt) {
            DSKnobPtr dsKnob = (*knobIt).second;

            if (dsKnob->getKnobGui().get() == knobGui) {
                return dsKnob;
            }
        }
    }

    return DSKnobPtr();
}

bool
DopeSheet::isPartOfGroup(DSNode *dsNode) const
{
    NodeGroupPtr parentGroup = boost::dynamic_pointer_cast<NodeGroup>( dsNode->getInternalNode()->getGroup() );

    return bool(parentGroup);
}

DSNodePtr DopeSheet::getGroupDSNode(DSNode *dsNode) const
{
    NodeGroupPtr parentGroup = boost::dynamic_pointer_cast<NodeGroup>( dsNode->getInternalNode()->getGroup() );
    DSNodePtr parentGroupDSNode;

    if (parentGroup) {
        parentGroupDSNode = findDSNode( parentGroup->getNode().get() );
    }

    return parentGroupDSNode;
}

std::vector<DSNodePtr> DopeSheet::getImportantNodes(DSNode *dsNode) const
{
    std::vector<DSNodePtr> ret;
    DopeSheetItemType nodeType = dsNode->getItemType();

    if (nodeType == eDopeSheetItemTypeGroup) {
        NodeGroup *nodeGroup = dsNode->getInternalNode()->isEffectGroup();
        assert(nodeGroup);

        NodesList nodes = nodeGroup->getNodes();
        for (NodesList::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
            NodePtr childNode = (*it);
            DSNodePtr isInDopeSheet = findDSNode( childNode.get() );
            if (isInDopeSheet) {
                ret.push_back(isInDopeSheet);
            }
        }
    } else if ( dsNode->isTimeNode() ) {
        _imp->getInputsConnected_recursive(dsNode->getInternalNode().get(), &ret);
    }


    return ret;
}

/**
 * @brief DopeSheet::getNearestRetimeFromOutputs
 *
 * Returns the first Retime node connected in output with 'dsNode' in the node graph.
 */
DSNodePtr DopeSheet::getNearestTimeNodeFromOutputs(DSNode *dsNode) const
{
    std::list<Node*> markedNodes;
    Node *timeNode = _imp->getNearestTimeFromOutputs_recursive(dsNode->getInternalNode().get(), markedNodes);

    return findDSNode(timeNode);
}

Node *
DopeSheet::getNearestReader(DSNode *timeNode) const
{
    assert( timeNode->isTimeNode() );
    std::list<Node*> markedNodes;
    Node *nearestReader = _imp->getNearestReaderFromInputs_recursive(timeNode->getInternalNode().get(), markedNodes);

    return nearestReader;
}

DopeSheetSelectionModel *
DopeSheet::getSelectionModel() const
{
    return _imp->selectionModel;
}

DopeSheetEditor*
DopeSheet::getEditor() const
{
    return _imp->editor;
}

void
DopeSheet::deleteSelectedKeyframes()
{
    if ( _imp->selectionModel->isEmpty() ) {
        return;
    }

    std::vector<DopeSheetKey> toRemove = _imp->selectionModel->getKeyframesSelectionCopy();

    _imp->selectionModel->clearKeyframeSelection();

    _imp->pushUndoCommand( new DSRemoveKeysCommand(toRemove, _imp->editor) );
}

NATRON_NAMESPACE_ANONYMOUS_ENTER

struct SortIncreasingFunctor
{
    bool operator() (const DopeSheetKeyPtr& lhs,
                     const DopeSheetKeyPtr& rhs)
    {
        DSKnobPtr leftKnobDs = lhs->getContext();
        DSKnobPtr rightKnobDs = rhs->getContext();

        if ( leftKnobDs.get() < rightKnobDs.get() ) {
            return true;
        } else if ( leftKnobDs.get() > rightKnobDs.get() ) {
            return false;
        } else {
            return lhs->key.getTime() < rhs->key.getTime();
        }
    }
};

struct SortDecreasingFunctor
{
    bool operator() (const DopeSheetKeyPtr& lhs,
                     const DopeSheetKeyPtr& rhs)
    {
        DSKnobPtr leftKnobDs = lhs->getContext();
        DSKnobPtr rightKnobDs = rhs->getContext();

        assert(leftKnobDs && rightKnobDs);
        if ( leftKnobDs.get() < rightKnobDs.get() ) {
            return true;
        } else if ( leftKnobDs.get() > rightKnobDs.get() ) {
            return false;
        } else {
            return lhs->key.getTime() > rhs->key.getTime();
        }
    }
};

NATRON_NAMESPACE_ANONYMOUS_EXIT


void
DopeSheet::moveSelectedKeysAndNodes(double dt)
{
    DopeSheetKeyPtrList selectedKeyframes;
    std::vector<DSNodePtr> selectedNodes;

    _imp->selectionModel->getCurrentSelection(&selectedKeyframes, &selectedNodes);

    ///Constraint dt according to keyframe positions
    double maxLeft = INT_MIN;
    double maxRight = INT_MAX;
    std::vector<DopeSheetKeyPtr> vect;
    for (DopeSheetKeyPtrList::iterator it = selectedKeyframes.begin(); it != selectedKeyframes.end(); ++it) {
        DSKnobPtr knobDs = (*it)->getContext();
        if (!knobDs) {
            continue;
        }
        CurvePtr curve = knobDs->getKnobGui()->getCurve( ViewIdx(0), knobDs->getDimension() );
        assert(curve);
        KeyFrame prevKey, nextKey;
        if ( curve->getNextKeyframeTime( (*it)->key.getTime(), &nextKey ) ) {
            if ( !_imp->selectionModel->keyframeIsSelected(knobDs, nextKey) ) {
                double diff = nextKey.getTime() - (*it)->key.getTime() - 1;
                maxRight = std::max( 0., std::min(diff, maxRight) );
            }
        }
        if ( curve->getPreviousKeyframeTime( (*it)->key.getTime(), &prevKey ) ) {
            if ( !_imp->selectionModel->keyframeIsSelected(knobDs, prevKey) ) {
                double diff = prevKey.getTime()  - (*it)->key.getTime() + 1;
                maxLeft = std::min( 0., std::max(diff, maxLeft) );
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
        std::sort( vect.begin(), vect.end(), SortIncreasingFunctor() );
    } else {
        std::sort( vect.begin(), vect.end(), SortDecreasingFunctor() );
    }
    selectedKeyframes.clear();
    for (std::vector<DopeSheetKeyPtr>::iterator it = vect.begin(); it != vect.end(); ++it) {
        selectedKeyframes.push_back(*it);
    }

    _imp->pushUndoCommand( new DSMoveKeysAndNodesCommand(selectedKeyframes, selectedNodes, dt, _imp->editor) );
} // DopeSheet::moveSelectedKeysAndNodes

void
DopeSheet::trimReaderLeft(const DSNodePtr &reader,
                          double newFirstFrame)
{
    NodePtr node = reader->getInternalNode();

    KnobIntBase *firstFrameKnob = dynamic_cast<KnobIntBase *>( node->getKnobByName(kReaderParamNameFirstFrame).get() );
    assert(firstFrameKnob);
    KnobIntBase *lastFrameKnob = dynamic_cast<KnobIntBase *>( node->getKnobByName(kReaderParamNameLastFrame).get() );
    assert(lastFrameKnob);
    KnobIntBase *originalFrameRangeKnob = dynamic_cast<KnobIntBase *>( node->getKnobByName(kReaderParamNameOriginalFrameRange).get() );
    assert(originalFrameRangeKnob);


    int firstFrame = firstFrameKnob->getValue();
    int lastFrame = lastFrameKnob->getValue();
    int originalFirstFrame = originalFrameRangeKnob->getValue();

    newFirstFrame = std::max( (double)newFirstFrame, (double)originalFirstFrame );
    newFirstFrame = std::min( (double)lastFrame, newFirstFrame );
    if (newFirstFrame == firstFrame) {
        return;
    }

    _imp->pushUndoCommand( new DSLeftTrimReaderCommand(reader, firstFrame, newFirstFrame) );
}

void
DopeSheet::trimReaderRight(const DSNodePtr &reader,
                           double newLastFrame)
{
    NodePtr node = reader->getInternalNode();

    KnobIntBase *firstFrameKnob = dynamic_cast<KnobIntBase *>( node->getKnobByName(kReaderParamNameFirstFrame).get() );
    assert(firstFrameKnob);
    KnobIntBase *lastFrameKnob = dynamic_cast<KnobIntBase *>( node->getKnobByName(kReaderParamNameLastFrame).get() );
    assert(lastFrameKnob);
    KnobIntBase *originalFrameRangeKnob = dynamic_cast<KnobIntBase *>( node->getKnobByName(kReaderParamNameOriginalFrameRange).get() );
    assert(originalFrameRangeKnob);

    int firstFrame = firstFrameKnob->getValue();
    int lastFrame = lastFrameKnob->getValue();
    int originalLastFrame = originalFrameRangeKnob->getValue(1);

    newLastFrame = std::min( (double)newLastFrame, (double)originalLastFrame );
    newLastFrame = std::max( (double)firstFrame, newLastFrame );
    if (newLastFrame == lastFrame) {
        return;
    }

    _imp->pushUndoCommand( new DSRightTrimReaderCommand(reader, lastFrame, newLastFrame, _imp->editor) );
}

bool
DopeSheet::canSlipReader(const DSNodePtr &reader) const
{
    NodePtr node = reader->getInternalNode();

    KnobIntBase *firstFrameKnob = dynamic_cast<KnobIntBase *>( node->getKnobByName(kReaderParamNameFirstFrame).get() );
    assert(firstFrameKnob);
    KnobIntBase *lastFrameKnob = dynamic_cast<KnobIntBase *>( node->getKnobByName(kReaderParamNameLastFrame).get() );
    assert(lastFrameKnob);
    KnobIntBase *originalFrameRangeKnob = dynamic_cast<KnobIntBase *>( node->getKnobByName(kReaderParamNameOriginalFrameRange).get() );
    assert(originalFrameRangeKnob);

    ///Slipping means moving the timeOffset parameter by dt and moving firstFrame and lastFrame by -dt
    ///dt is clamped (firstFrame-originalFirstFrame) and (originalLastFrame-lastFrame)

    int currentFirstFrame = firstFrameKnob->getValue();
    int currentLastFrame = lastFrameKnob->getValue();
    int originalFirstFrame = originalFrameRangeKnob->getValue(0);
    int originalLastFrame = originalFrameRangeKnob->getValue(1);

    if ( ( (currentFirstFrame - originalFirstFrame) == 0 ) && ( (currentLastFrame - originalLastFrame) == 0 ) ) {
        return false;
    }

    return true;
}

void
DopeSheet::slipReader(const DSNodePtr &reader,
                      double dt)
{
    NodePtr node = reader->getInternalNode();

    KnobIntBase *firstFrameKnob = dynamic_cast<KnobIntBase *>( node->getKnobByName(kReaderParamNameFirstFrame).get() );
    assert(firstFrameKnob);
    KnobIntBase *lastFrameKnob = dynamic_cast<KnobIntBase *>( node->getKnobByName(kReaderParamNameLastFrame).get() );
    assert(lastFrameKnob);
    KnobIntBase *originalFrameRangeKnob = dynamic_cast<KnobIntBase *>( node->getKnobByName(kReaderParamNameOriginalFrameRange).get() );
    assert(originalFrameRangeKnob);

    ///Slipping means moving the timeOffset parameter by dt and moving firstFrame and lastFrame by -dt
    ///dt is clamped (firstFrame-originalFirstFrame) and (originalLastFrame-lastFrame)

    int currentFirstFrame = firstFrameKnob->getValue();
    int currentLastFrame = lastFrameKnob->getValue();
    int originalFirstFrame = originalFrameRangeKnob->getValue(0);
    int originalLastFrame = originalFrameRangeKnob->getValue(1);

    dt = std::min( dt, (double)(currentFirstFrame - originalFirstFrame) );
    dt = std::max( dt, (double)(currentLastFrame - originalLastFrame) );

    if (dt != 0) {
        _imp->pushUndoCommand( new DSSlipReaderCommand(reader, dt, _imp->editor) );
    }
}

void
DopeSheet::copySelectedKeys()
{
    if ( _imp->selectionModel->isEmpty() ) {
        return;
    }

    _imp->keyframesClipboard.clear();

    DopeSheetKeyPtrList selectedKeyframes;
    std::vector<DSNodePtr> selectedNodes;
    _imp->selectionModel->getCurrentSelection(&selectedKeyframes, &selectedNodes);

    for (DopeSheetKeyPtrList::const_iterator it = selectedKeyframes.begin();
         it != selectedKeyframes.end();
         ++it) {
        DopeSheetKeyPtr selectedKey = (*it);

        _imp->keyframesClipboard.push_back(*selectedKey);
    }
}

void
DopeSheet::renameSelectedNode()
{
    DopeSheetKeyPtrList keys;
    std::vector<DSNodePtr> selectedNodes;

    _imp->selectionModel->getCurrentSelection(&keys, &selectedNodes);
    if ( selectedNodes.empty() || (selectedNodes.size() > 1) ) {
        Dialogs::errorDialog( tr("Rename node").toStdString(), tr("You must select exactly 1 node to rename.").toStdString() );

        return;
    }

    EditNodeNameDialog* dialog = new EditNodeNameDialog(selectedNodes.front()->getNodeGui(), _imp->editor);
    QPoint global = QCursor::pos();
    QSize sizeH = dialog->sizeHint();
    global.rx() -= sizeH.width() / 2;
    global.ry() -= sizeH.height() / 2;
    QPoint realPos = global;
    QObject::connect( dialog, SIGNAL(rejected()), this, SLOT(onNodeNameEditDialogFinished()) );
    QObject::connect( dialog, SIGNAL(accepted()), this, SLOT(onNodeNameEditDialogFinished()) );
    dialog->move( realPos.x(), realPos.y() );
    dialog->raise();
    dialog->show();
}

void
DopeSheet::onNodeNameEditDialogFinished()
{
    EditNodeNameDialog* dialog = qobject_cast<EditNodeNameDialog*>( sender() );

    if (dialog) {
        QDialog::DialogCode code =  (QDialog::DialogCode)dialog->result();
        if (code == QDialog::Accepted) {
            QString newName = dialog->getTypedName();
            if ( NATRON_PYTHON_NAMESPACE::isKeyword(newName.toStdString()) ) {
                Dialogs::errorDialog(dialog->getNode()->getNode()->getLabel(), newName.toStdString() + " is a Python keyword");
            } else {
                QString oldName = QString::fromUtf8( dialog->getNode()->getNode()->getLabel().c_str() );
                _imp->pushUndoCommand( new RenameNodeUndoRedoCommand(dialog->getNode(), oldName, newName) );
            }
        }
        dialog->deleteLater();
    }
}

void
DopeSheet::pasteKeys(bool relative)
{
    std::vector<DopeSheetKey> toPaste;

    for (std::vector<DopeSheetKey>::const_iterator it = _imp->keyframesClipboard.begin(); it != _imp->keyframesClipboard.end(); ++it) {
        DopeSheetKey key = (*it);

        toPaste.push_back(key);
    }

    if ( !toPaste.empty() ) {

        std::list<DSKnobPtr> dstKnobs;
        _imp->editor->getHierarchyView()->getSelectedDSKnobs(&dstKnobs);
        if (dstKnobs.empty()) {
            Dialogs::warningDialog(tr("Paste").toStdString(), tr("You must select at least one parameter dimension on the left to perform this action").toStdString());
            return;
        }
        _imp->pushUndoCommand( new DSPasteKeysCommand(toPaste, dstKnobs, relative, _imp->editor) );
    }
}

void
DopeSheet::pasteKeys(const std::vector<DopeSheetKey>& keys, bool relative)
{
    if ( !keys.empty() ) {
        std::list<DSKnobPtr> dstKnobs;
        _imp->editor->getHierarchyView()->getSelectedDSKnobs(&dstKnobs);
        if (dstKnobs.empty()) {
            Dialogs::warningDialog(tr("Paste").toStdString(), tr("You must select at least one parameter dimension on the left to perform this action").toStdString());
            return;
        }
        _imp->pushUndoCommand( new DSPasteKeysCommand(keys, dstKnobs, relative, _imp->editor) );
    }
}

void
DopeSheet::setSelectedKeysInterpolation(KeyframeTypeEnum keyType)
{
    if ( _imp->selectionModel->isEmpty() ) {
        return;
    }

    DopeSheetKeyPtrList selectedKeyframes;
    std::vector<DSNodePtr> selectedNodes;
    _imp->selectionModel->getCurrentSelection(&selectedKeyframes, &selectedNodes);
    std::list<DSKeyInterpolationChange> changes;

    for (DopeSheetKeyPtrList::iterator it = selectedKeyframes.begin();
         it != selectedKeyframes.end();
         ++it) {
        DopeSheetKeyPtr keyPtr = (*it);
        DSKeyInterpolationChange change(keyPtr->key.getInterpolation(), keyType, keyPtr);

        changes.push_back(change);
    }

    _imp->pushUndoCommand( new DSSetSelectedKeysInterpolationCommand(changes, _imp->editor) );
}

void
DopeSheet::transformSelectedKeys(const Transform::Matrix3x3& transform)
{
    if ( _imp->selectionModel->isEmpty() ) {
        return;
    }
    DopeSheetKeyPtrList selectedKeyframes;
    std::vector<DSNodePtr> selectedNodes;
    _imp->selectionModel->getCurrentSelection(&selectedKeyframes, &selectedNodes);

    _imp->pushUndoCommand( new DSTransformKeysCommand(selectedKeyframes, transform, _imp->editor) );
}

void
DopeSheet::emit_modelChanged()
{
    Q_EMIT modelChanged();
}

SequenceTime
DopeSheet::getCurrentFrame() const
{
    return _imp->timeline->currentFrame();
}

DSNodePtr DopeSheet::createDSNode(const NodeGuiPtr &nodeGui,
                                                  DopeSheetItemType itemType)
{
    // Determinate the node type
    // It will be useful to identify and sort tree items
    NodePtr node = nodeGui->getNode();
    QTreeWidgetItem *nameItem = new QTreeWidgetItem;

    nameItem->setText( 0, QString::fromUtf8( node->getLabel().c_str() ) );
    nameItem->setData(0, QT_ROLE_CONTEXT_TYPE, itemType);
    nameItem->setData(0, QT_ROLE_CONTEXT_IS_ANIMATED, true);

    DSNodePtr dsNode( new DSNode(this, itemType, nodeGui, nameItem) );

    connect( node.get(), SIGNAL(labelChanged(QString)),
             this, SLOT(onNodeNameChanged(QString)) );

    return dsNode;
}

void
DopeSheet::onNodeNameChanged(const QString &name)
{
    Node *node = qobject_cast<Node *>( sender() );
    DSNodePtr dsNode = findDSNode(node);

    if (dsNode) {
        dsNode->getTreeItem()->setText(0, name);
    }
}

void
DopeSheet::onKeyframeSetOrRemoved()
{
    KnobGui* k = qobject_cast<KnobGui *>( sender() );

    if (!k) {
        return;
    }
    DSKnobPtr dsKnob = findDSKnob(k);
    if (dsKnob) {
        Q_EMIT keyframeSetOrRemoved( dsKnob.get() );
    }

    Q_EMIT modelChanged();
}

////////////////////////// DSKnob //////////////////////////

class DSKnobPrivate
{
public:
    DSKnobPrivate();
    ~DSKnobPrivate();

    /* attributes */
    int dimension;
    QTreeWidgetItem *nameItem;
    KnobGuiWPtr knobGui;
    KnobIWPtr knob;
};

DSKnobPrivate::DSKnobPrivate()
    : dimension(-2),
    nameItem(0),
    knobGui(),
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
               const KnobGuiPtr& knobGui)
    : _imp(new DSKnobPrivate)
{
    assert(knobGui);

    _imp->dimension = dimension;
    _imp->nameItem = nameItem;
    _imp->knobGui = knobGui;
    _imp->knob = knobGui->getKnob();
}

DSKnob::~DSKnob()
{}

QTreeWidgetItem *
DSKnob::getTreeItem() const
{
    return _imp->nameItem;
}

QTreeWidgetItem *
DSKnob::findDimTreeItem(int dimension) const
{
    if ( ( isMultiDimRoot() && (dimension == -1) )  || (!isMultiDimRoot() && !dimension) ) {
        return _imp->nameItem;
    }

    QTreeWidgetItem *ret = 0;

    for (int i = 0; i < _imp->nameItem->childCount(); ++i) {
        QTreeWidgetItem *child = _imp->nameItem->child(i);

        if ( dimension == child->data(0, QT_ROLE_CONTEXT_DIM).toInt() ) {
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
KnobGuiPtr
DSKnob::getKnobGui() const
{
    return _imp->knobGui.lock();
}

KnobIPtr
DSKnob::getInternalKnob() const
{
    return _imp->knob.lock();
}

/**
 * @brief DSKnob::isMultiDimRoot
 *
 *
 */
bool
DSKnob::isMultiDimRoot() const
{
    return (_imp->dimension == -1);
}

int
DSKnob::getDimension() const
{
    return _imp->dimension;
}

////////////////////////// DopeSheetSelectionModel //////////////////////////

class DopeSheetSelectionModelPrivate
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
    DopeSheetKeyPtrList selectedKeyframes;
    std::list<DSNodeWPtr> selectedRangeNodes;
};

DopeSheetSelectionModel::DopeSheetSelectionModel(DopeSheet *dopeSheet)
    : _imp(new DopeSheetSelectionModelPrivate)
{
    _imp->dopeSheet = dopeSheet;
}

DopeSheetSelectionModel::~DopeSheetSelectionModel()
{
}

void
DopeSheetSelectionModel::selectAll()
{
    std::vector<DopeSheetKey> result;
    DSTreeItemNodeMap nodeRows = _imp->dopeSheet->getItemNodeMap();
    std::vector<DSNodePtr> selectedNodes;

    for (DSTreeItemNodeMap::const_iterator it = nodeRows.begin(); it != nodeRows.end(); ++it) {
        const DSNodePtr& dsNode = (*it).second;
        const DSTreeItemKnobMap& dsKnobItems = dsNode->getItemKnobMap();

        for (DSTreeItemKnobMap::const_iterator itKnob = dsKnobItems.begin();
             itKnob != dsKnobItems.end();
             ++itKnob) {
            makeDopeSheetKeyframesForKnob( (*itKnob).second, &result );
        }

        if ( dsNode->isRangeDrawingEnabled() ) {
            selectedNodes.push_back(dsNode);
        }
    }

    makeSelection( result, selectedNodes, (DopeSheetSelectionModel::SelectionTypeAdd |
                                           DopeSheetSelectionModel::SelectionTypeClear |
                                           DopeSheetSelectionModel::SelectionTypeRecurse) );
}

void
DopeSheetSelectionModel::makeDopeSheetKeyframesForKnob(const DSKnobPtr &dsKnob,
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
        KeyFrameSet keyframes = dsKnob->getKnobGui()->getCurve(ViewIdx(0), i)->getKeyFrames_mt_safe();
        DSKnobPtr context;
        if (dim == -1) {
            QTreeWidgetItem *childItem = dsKnob->findDimTreeItem(i);
            context = _imp->dopeSheet->mapNameItemToDSKnob(childItem);
        } else {
            context = dsKnob;
        }

        for (KeyFrameSet::const_iterator kIt = keyframes.begin();
             kIt != keyframes.end();
             ++kIt) {
            const KeyFrame& kf = (*kIt);
            result->push_back( DopeSheetKey(context, kf) );
        }
    }
}

void
DopeSheetSelectionModel::clearKeyframeSelection()
{
    if ( _imp->selectedKeyframes.empty() && _imp->selectedRangeNodes.empty() ) {
        return;
    }

    makeSelection(std::vector<DopeSheetKey>(), std::vector<DSNodePtr>(), DopeSheetSelectionModel::SelectionTypeClear);
}

void
DopeSheetSelectionModel::makeSelection(const std::vector<DopeSheetKey> &keys,
                                       const std::vector<DSNodePtr>& rangeNodes,
                                       SelectionTypeFlags selectionFlags)
{
    // Don't allow unsupported combinations
    assert( !(selectionFlags & DopeSheetSelectionModel::SelectionTypeAdd
              &&
              selectionFlags & DopeSheetSelectionModel::SelectionTypeToggle) );

    bool hasChanged = false;
    if (selectionFlags & DopeSheetSelectionModel::SelectionTypeClear) {
        hasChanged = true;
        _imp->selectedKeyframes.clear();
        _imp->selectedRangeNodes.clear();
    }

    for (std::vector<DopeSheetKey>::const_iterator it = keys.begin(); it != keys.end(); ++it) {
        const DopeSheetKey& key = (*it);
        DopeSheetKeyPtrList::iterator isAlreadySelected = keyframeIsSelected(key);

        if ( isAlreadySelected == _imp->selectedKeyframes.end() ) {
            DopeSheetKeyPtr dsKey( new DopeSheetKey(key) );
            hasChanged = true;
            _imp->selectedKeyframes.push_back(dsKey);
        } else if (selectionFlags & DopeSheetSelectionModel::SelectionTypeToggle) {
            _imp->selectedKeyframes.erase(isAlreadySelected);
            hasChanged = true;
        }
    }

    for (std::vector<DSNodePtr>::const_iterator it = rangeNodes.begin(); it != rangeNodes.end(); ++it) {
        std::list<DSNodeWPtr>::iterator found = isRangeNodeSelected(*it);
        if ( found == _imp->selectedRangeNodes.end() ) {
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

bool
DopeSheetSelectionModel::isEmpty() const
{
    return _imp->selectedKeyframes.empty() && _imp->selectedRangeNodes.empty();
}

void
DopeSheetSelectionModel::getCurrentSelection(DopeSheetKeyPtrList* keys,
                                             std::vector<DSNodePtr>* nodes) const
{
    *keys =  _imp->selectedKeyframes;
    for (std::list<DSNodeWPtr>::const_iterator it = _imp->selectedRangeNodes.begin(); it != _imp->selectedRangeNodes.end(); ++it) {
        DSNodePtr n = it->lock();
        if (n) {
            nodes->push_back(n);
        }
    }
}

std::vector<DopeSheetKey> DopeSheetSelectionModel::getKeyframesSelectionCopy() const
{
    std::vector<DopeSheetKey> ret;

    for (DopeSheetKeyPtrList::iterator it = _imp->selectedKeyframes.begin();
         it != _imp->selectedKeyframes.end();
         ++it) {
        ret.push_back( DopeSheetKey(**it) );
    }

    return ret;
}

bool
DopeSheetSelectionModel::hasSingleKeyFrameTimeSelected(double* time) const
{
    bool timeSet = false;
    KnobGuiPtr knob;

    if ( _imp->selectedKeyframes.empty() ) {
        return false;
    }
    for (DopeSheetKeyPtrList::iterator it = _imp->selectedKeyframes.begin(); it != _imp->selectedKeyframes.end(); ++it) {
        DSKnobPtr knobContext = (*it)->context.lock();
        assert(knobContext);
        if (!timeSet) {
            *time = (*it)->key.getTime();
            knob = knobContext->getKnobGui();
            timeSet = true;
        } else {
            if ( ( (*it)->key.getTime() != *time ) || (knobContext->getKnobGui() != knob) ) {
                return false;
            }
        }
    }

    return true;
}

int
DopeSheetSelectionModel::getSelectedKeyframesCount() const
{
    return (int)_imp->selectedKeyframes.size();
}

bool
DopeSheetSelectionModel::keyframeIsSelected(const DSKnobPtr &dsKnob,
                                            const KeyFrame &keyframe) const
{
    for (DopeSheetKeyPtrList::iterator it = _imp->selectedKeyframes.begin(); it != _imp->selectedKeyframes.end(); ++it) {
        DSKnobPtr knobContext = (*it)->context.lock();
        assert(knobContext);
        if ( (knobContext == dsKnob) && ( (*it)->key.getTime() == keyframe.getTime() ) ) {
            return true;
        }
    }

    return false;
}

std::list<DSNodeWPtr>::iterator
DopeSheetSelectionModel::isRangeNodeSelected(const DSNodePtr& node)
{
    for (std::list<DSNodeWPtr>::iterator it = _imp->selectedRangeNodes.begin(); it != _imp->selectedRangeNodes.end(); ++it) {
        DSNodePtr n = it->lock();
        if ( n && (n == node) ) {
            return it;
        }
    }

    return _imp->selectedRangeNodes.end();
}

DopeSheetKeyPtrList::iterator
DopeSheetSelectionModel::keyframeIsSelected(const DopeSheetKey &key) const
{
    for (DopeSheetKeyPtrList::iterator it = _imp->selectedKeyframes.begin(); it != _imp->selectedKeyframes.end(); ++it) {
        if (**it == key) {
            return it;
        }
    }

    return _imp->selectedKeyframes.end();
}

bool
DopeSheetSelectionModel::rangeIsSelected(const DSNodePtr& node) const
{
    for (std::list<DSNodeWPtr>::iterator it = _imp->selectedRangeNodes.begin(); it != _imp->selectedRangeNodes.end(); ++it) {
        DSNodePtr n = it->lock();
        if ( n && (n == node) ) {
            return true;
        }
    }

    return false;
}

void
DopeSheetSelectionModel::emit_keyframeSelectionChanged(bool recurse)
{
    Q_EMIT keyframeSelectionChangedFromModel(recurse);
}

void
DopeSheetSelectionModel::onNodeAboutToBeRemoved(const DSNodePtr &removed)
{
    for (DopeSheetKeyPtrList::iterator it = _imp->selectedKeyframes.begin();
         it != _imp->selectedKeyframes.end(); ) {
        DopeSheetKeyPtr key = (*it);
        DSKnobPtr knobContext = key->context.lock();
        assert(knobContext);

        if (_imp->dopeSheet->findDSNode( knobContext->getInternalKnob() ) == removed) {
            it = _imp->selectedKeyframes.erase(it);
        } else {
            ++it;
        }
    }
}

////////////////////////// DSNode //////////////////////////

class DSNodePrivate
{
public:
    DSNodePrivate();
    ~DSNodePrivate();

    /* functions */
    void initGroupNode();

    /* attributes */
    DopeSheet *dopeSheetModel;
    DopeSheetItemType nodeType;
    NodeGuiWPtr nodeGui;
    QTreeWidgetItem *nameItem;
    DSTreeItemKnobMap itemKnobMap;
    bool isSelected;
};

DSNodePrivate::DSNodePrivate()
    : dopeSheetModel(0),
    nodeType(),
    nodeGui(),
    nameItem(0),
    itemKnobMap(),
    isSelected(false)
{}

DSNodePrivate::~DSNodePrivate()
{}

void
DSNodePrivate::initGroupNode()
{
    /* NodeGuiPtr node = nodeGui.lock();
       if (!node) {
         return;
       }
       NodePtr natronnode = node->getNode();
       assert(natronnode);
       NodeGroup* nodegroup = dynamic_cast<NodeGroup *>(natronnode->getEffectInstance());
       assert(nodegroup);
       if (!nodegroup) {
         return;
       }
       NodesList subNodes = nodegroup->getNodes();

       for (NodesList::const_iterator it = subNodes.begin(); it != subNodes.end(); ++it) {
         NodePtr subNode = (*it);
         NodeGuiPtr subNodeGui = boost::dynamic_pointer_cast<NodeGui>(subNode->getNodeGui());

         if (!subNodeGui || !subNodeGui->getSettingPanel() || !subNodeGui->isSettingsPanelVisible()) {
             continue;
         }
       }*/
}

DSNode::DSNode(DopeSheet *model,
               DopeSheetItemType itemType,
               const NodeGuiPtr &nodeGui,
               QTreeWidgetItem *nameItem)
    : _imp(new DSNodePrivate)
{
    _imp->dopeSheetModel = model;
    _imp->nodeType = itemType;
    _imp->nameItem = nameItem;
    _imp->nodeGui = nodeGui;

    // Create dope sheet knobs
    const std::list<std::pair<KnobIWPtr, KnobGuiPtr> > &knobs = nodeGui->getKnobs();

    for (std::list<std::pair<KnobIWPtr, KnobGuiPtr> >::const_iterator it = knobs.begin();
         it != knobs.end(); ++it) {
        KnobIPtr knob = it->first.lock();
        if (!knob) {
            continue;
        }
        const KnobGuiPtr &knobGui = it->second;

        if ( !knob->canAnimate() || !knob->isAnimationEnabled() ) {
            continue;
        }

        if (knob->getDimension() <= 1) {
            QTreeWidgetItem * nameItem = createKnobNameItem(QString::fromUtf8( knob->getLabel().c_str() ),
                                                            eDopeSheetItemTypeKnobDim,
                                                            0,
                                                            _imp->nameItem);
            DSKnob *dsKnob = new DSKnob(0, nameItem, knobGui);
            _imp->itemKnobMap.insert( TreeItemAndDSKnob(nameItem, dsKnob) );
        } else {
            QTreeWidgetItem *multiDimRootItem = createKnobNameItem(QString::fromUtf8( knob->getLabel().c_str() ),
                                                                   eDopeSheetItemTypeKnobRoot,
                                                                   -1,
                                                                   _imp->nameItem);
            DSKnob *rootDSKnob = new DSKnob(-1, multiDimRootItem, knobGui);
            _imp->itemKnobMap.insert( TreeItemAndDSKnob(multiDimRootItem, rootDSKnob) );

            for (int i = 0; i < knob->getDimension(); ++i) {
                QTreeWidgetItem *dimItem = createKnobNameItem(QString::fromUtf8( knob->getDimensionName(i).c_str() ),
                                                              eDopeSheetItemTypeKnobDim,
                                                              i,
                                                              multiDimRootItem);
                DSKnob *dimDSKnob = new DSKnob(i, dimItem, knobGui);
                _imp->itemKnobMap.insert( TreeItemAndDSKnob(dimItem, dimDSKnob) );
            }
        }

        QObject::connect( knobGui.get(), SIGNAL(keyFrameSet()),
                          _imp->dopeSheetModel, SLOT(onKeyframeSetOrRemoved()) );
        QObject::connect( knobGui.get(), SIGNAL(keyFrameRemoved()),
                          _imp->dopeSheetModel, SLOT(onKeyframeSetOrRemoved()) );
        QObject::connect( knobGui.get(), SIGNAL(refreshDopeSheet()),
                          _imp->dopeSheetModel, SLOT(onKeyframeSetOrRemoved()) );
    }

    // If some subnodes are already in the dope sheet, the connections must be set to update
    // the group's clip rect
    if (_imp->nodeType == eDopeSheetItemTypeGroup) {
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
QTreeWidgetItem *
DSNode::getTreeItem() const
{
    return _imp->nameItem;
}

/**
 * @brief DSNode::getNode
 *
 * Returns the associated node.
 */
NodeGuiPtr
DSNode::getNodeGui() const
{
    return _imp->nodeGui.lock();
}

NodePtr
DSNode::getInternalNode() const
{
    NodeGuiPtr node = getNodeGui();

    return node ? node->getNode() : NodePtr();
}

/**
 * @brief DSNode::getTreeItemsAndDSKnobs
 *
 *
 */
const DSTreeItemKnobMap&
DSNode::getItemKnobMap() const
{
    return _imp->itemKnobMap;
}

DopeSheetItemType
DSNode::getItemType() const
{
    return _imp->nodeType;
}

bool
DSNode::isTimeNode() const
{
    return (_imp->nodeType >= eDopeSheetItemTypeRetime)
           && (_imp->nodeType < eDopeSheetItemTypeGroup);
}

bool
DSNode::isRangeDrawingEnabled() const
{
    return (_imp->nodeType >= eDopeSheetItemTypeReader &&
            _imp->nodeType <= eDopeSheetItemTypeGroup);
}

bool
DSNode::canContainOtherNodeContexts() const
{
    return (_imp->nodeType >= eDopeSheetItemTypeReader + 1
            && _imp->nodeType <= eDopeSheetItemTypeGroup);
}

bool
DSNode::containsNodeContext() const
{
    for (int i = 0; i < _imp->nameItem->childCount(); ++i) {
        int childType = _imp->nameItem->child(i)->data(0, QT_ROLE_CONTEXT_TYPE).toInt();

        if ( (childType != eDopeSheetItemTypeKnobDim)
             && ( childType != eDopeSheetItemTypeKnobRoot) ) {
            return true;
        }
    }

    return false;
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_DopeSheet.cpp"
