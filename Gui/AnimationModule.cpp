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

#include "AnimationModule.h"

#include <algorithm>
#include <stdexcept>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#endif

// Qt includes
#include <QtCore/QDebug>
#include <QtEvents>
#include <QTreeWidget>
#include <QUndoStack>

// Natron includes
#include "Engine/AnimatingObjectI.h"
#include "Engine/AppInstance.h"
#include "Engine/GroupInput.h"
#include "Engine/GroupOutput.h"
#include "Engine/KnobTypes.h"
#include "Engine/EffectInstance.h"
#include "Engine/KnobItemsTable.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/TimeLine.h"
#include "Engine/Project.h"
#include "Engine/ReadNode.h"
#include "Engine/ViewIdx.h"

#include "Gui/ActionShortcuts.h"
#include "Gui/AnimationModuleSelectionModel.h"
#include "Gui/DockablePanel.h"
#include "Gui/AnimationModuleEditor.h"
#include "Gui/AnimationModuleUndoRedo.h"
#include "Gui/AnimationModuleTreeView.h"
#include "Gui/DopeSheetView.h"
#include "Gui/Gui.h"
#include "Gui/KnobItemsTableGui.h"
#include "Gui/KnobUndoCommand.h"
#include "Gui/KnobAnim.h"
#include "Gui/KnobGui.h"
#include "Gui/Menu.h"
#include "Gui/NodeAnim.h"
#include "Gui/NodeGui.h"
#include "Gui/NodeGraph.h"
#include "Gui/NodeGraphUndoRedo.h"
#include "Gui/TableItemAnim.h"

NATRON_NAMESPACE_ENTER;

typedef std::map<KnobIWPtr, KnobGui *> KnobsAndGuis;
typedef std::pair<QTreeWidgetItem *, NodeAnimPtr > TreeItemAndNodeAnim;
typedef std::pair<QTreeWidgetItem *, KnobAnim *> TreeItemAndKnobAnim;
typedef std::map<DimensionViewPair, QTreeWidgetItem* ,DimensionViewPairCompare> PerDimViewItemMap;
typedef std::map<ViewIdx, QTreeWidgetItem*> PerViewItemMap;

class AnimationModulePrivate
{
public:

    AnimationModulePrivate(AnimationModuleEditor* editor,
                           AnimationModule *publicInterface)
    : _publicInterface(publicInterface)
    , nodes()
    , selectionModel()
    , undoStack( new QUndoStack(publicInterface) )
    , keyframesClipboard()
    , timeline()
    , editor(editor)
    {
    }


    /**
     * @brief Recursively finds downstream of the given node the nearest Time node that may change the time
     **/
    NodePtr getNearestTimeNodeFromOutputs_recursive(const NodePtr& node, const NodeCollectionPtr& collection, std::list<NodePtr>& markedNodes) const;

    /**
     * @brief Recursively finds upstream the nearest reader node
     **/
    NodePtr getNearestReaderFromInputs_recursive(const NodePtr& node, std::list<NodePtr>& markedNodes) const;

    void getInputs_recursive(const NodePtr& node, std::list<NodePtr>& markedNodes, std::vector<NodeAnimPtr> *result) const;

    void pushUndoCommand(QUndoCommand *cmd);

    bool findItemInTableItemRecursive(QTreeWidgetItem* treeItem, const TableItemAnimPtr& tableItem, AnimatedItemTypeEnum *type, KnobAnimPtr* isKnob, TableItemAnimPtr* isTableItem,  ViewSetSpec* view, DimSpec* dimension) const;

    /* attributes */
    AnimationModule *_publicInterface;
    std::list<NodeAnimPtr> nodes;
    boost::scoped_ptr<AnimationModuleSelectionModel> selectionModel;
    boost::scoped_ptr<QUndoStack> undoStack;
    AnimItemDimViewKeyFramesMap keyframesClipboard;
    TimeLineWPtr timeline;
    AnimationModuleEditor* editor;
};



AnimationModule::AnimationModule(Gui *gui,
                                 AnimationModuleEditor* editor,
                                 const TimeLinePtr &timeline)
: _imp( new AnimationModulePrivate(editor, this) )
{
    _imp->timeline = timeline;

    gui->registerNewUndoStack( _imp->undoStack.get() );
}

AnimationModule::~AnimationModule()
{
}

bool
AnimationModule::isNodeAnimated(const NodeGuiPtr &nodeGui)
{
    NodePtr internalNode = nodeGui->getNode();
    if (!internalNode) {
        return false;
    }
    return internalNode->getEffectInstance()->getHasAnimation();
}


bool
AnimationModule::getNodeCanAnimate(const NodePtr &node)
{
    // A node with an items table such as Tracker or RotoPaint always animates
    if (node->getEffectInstance()->getItemsTable()) {
        return true;
    }

    // Check that it has at least one knob that can animate
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





void
AnimationModulePrivate::pushUndoCommand(QUndoCommand *cmd)
{
    if (editor) {
        editor->pushUndoCommand(cmd);
    } else {
        delete cmd;
    }
}

NodePtr
AnimationModulePrivate::getNearestTimeNodeFromOutputs_recursive(const NodePtr& node,
                                                                const NodeCollectionPtr& collection,
                                                                std::list<NodePtr>& markedNodes) const
{
    const NodesWList &outputs = node->getOutputs();

    if ( std::find(markedNodes.begin(), markedNodes.end(), node) != markedNodes.end() ) {
        return NodePtr();
    }

    // If we go outside of the initial node group scope, return
    if (node->getGroup() != collection) {
        return NodePtr();
    }
    markedNodes.push_back(node);
    for (NodesWList::const_iterator it = outputs.begin(); it != outputs.end(); ++it) {
        NodePtr output = it->lock();
        std::string pluginID = output->getPluginID();

        if ( (pluginID == PLUGINID_OFX_RETIME) || ( pluginID == PLUGINID_OFX_TIMEOFFSET) || ( pluginID == PLUGINID_OFX_FRAMERANGE) ) {
            return output;
        } else {
            NodePtr ret =  getNearestTimeNodeFromOutputs_recursive(output, collection, markedNodes);
            if (ret) {
                return ret;
            }
        }
    }

    return NodePtr();
}

NodePtr
AnimationModulePrivate::getNearestReaderFromInputs_recursive(const NodePtr& node,
                                                       std::list<NodePtr>& markedNodes) const
{
    const std::vector<NodeWPtr > &inputs = node->getInputs();

    if ( std::find(markedNodes.begin(), markedNodes.end(), node) != markedNodes.end() ) {
        return NodePtr();
    }
    markedNodes.push_back(node);
    for (std::vector<NodeWPtr >::const_iterator it = inputs.begin(); it != inputs.end(); ++it) {
        NodePtr input = it->lock();

        if (!input) {
            continue;
        }

        if (input->getEffectInstance()->isReader()) {
            return input;
        } else {
            NodePtr ret = getNearestReaderFromInputs_recursive(input, markedNodes);
            if (ret) {
                return ret;
            }
        }
    }

    return NodePtr();
}

void
AnimationModulePrivate::getInputs_recursive(const NodePtr& node,
                                            std::list<NodePtr>& markedNodes,
                                            std::vector<NodeAnimPtr > *result) const
{
    if (std::find(markedNodes.begin(), markedNodes.end(), node) != markedNodes.end()) {
        return;
    }

    markedNodes.push_back(node);


    const std::vector<NodeWPtr > &inputs = node->getInputs();
    for (std::vector<NodeWPtr >::const_iterator it = inputs.begin(); it != inputs.end(); ++it) {
        NodePtr input = it->lock();

        if (!input) {
            continue;
        }

        NodeAnimPtr inputAnim = _publicInterface->findNodeAnim( input );

        if (inputAnim) {
            result->push_back(inputAnim);
        }

        getInputs_recursive(input, markedNodes, result);
    }
}

QUndoStack*
AnimationModule::getUndoStack() const
{
    return _imp->undoStack.get();
}



const std::list<NodeAnimPtr>&
AnimationModule::getNodes() const
{
    return _imp->nodes;
}

void
AnimationModule::ensureSelectionModel()
{
    if (_imp->selectionModel) {
        return;
    }
    _imp->selectionModel.reset(new AnimationModuleSelectionModel(shared_from_this()));
}

void
AnimationModule::addNode(const NodeGuiPtr& nodeGui)
{
    // Check if it already exists
    for (std::list<NodeAnimPtr>::const_iterator it = _imp->nodes.begin(); it != _imp->nodes.end(); ++it) {
        if ((*it)->getNodeGui() == nodeGui) {
            return;
        }
    }
    
    // Determinate the node type
    // It will be useful to identify and sort tree items
    AnimatedItemTypeEnum nodeType = eAnimatedItemTypeCommon;

    NodePtr node = nodeGui->getNode();

    assert(node && node->getGroup());
    if ( !node || !node->getGroup() ) {
        return;
    }

    EffectInstancePtr effectInstance = node->getEffectInstance();

    // Don't add an item for this node if it doesn't have any knob that may animate
    if ( !getNodeCanAnimate(node) ) {
        return;
    }

    std::string pluginID = node->getPluginID();
    NodeGroupPtr isGroup = toNodeGroup(effectInstance);

    if (effectInstance->isReader()) {
        nodeType = eAnimatedItemTypeReader;
    } else if (isGroup) {
        nodeType = eAnimatedItemTypeGroup;
    } else if (pluginID == PLUGINID_OFX_RETIME) {
        nodeType = eAnimatedItemTypeRetime;
    } else if (pluginID == PLUGINID_OFX_TIMEOFFSET) {
        nodeType = eAnimatedItemTypeTimeOffset;
    } else if (pluginID == PLUGINID_OFX_FRAMERANGE) {
        nodeType = eAnimatedItemTypeFrameRange;
    }

    // The NodeAnim should not be created if there's no settings panel.
    assert(nodeGui->getSettingPanel());

    NodeAnimPtr anim(NodeAnim::create(shared_from_this(), nodeType, nodeGui) );
    _imp->nodes.push_back(anim);

    Q_EMIT nodeAdded(anim);
} // AnimationModule::addNode

void
AnimationModule::removeNode(const NodeGuiPtr& node)
{
    std::list<NodeAnimPtr>::iterator toRemove = _imp->nodes.end();

    for (std::list<NodeAnimPtr>::iterator it = _imp->nodes.begin(); it != _imp->nodes.end(); ++it) {
        if ( (*it)->getNodeGui() == node ) {
            toRemove = it;
            break;
        }
    }

    if ( toRemove == _imp->nodes.end() ) {
        return;
    }

    _imp->selectionModel->removeAnyReferenceFromSelection(*toRemove);
    Q_EMIT nodeAboutToBeRemoved(*toRemove);

    _imp->nodes.erase(toRemove);
}

bool
AnimationModulePrivate::findItemInTableItemRecursive(QTreeWidgetItem* treeItem, const TableItemAnimPtr& tableItem, AnimatedItemTypeEnum *type, KnobAnimPtr* isKnob, TableItemAnimPtr* isTableItem,  ViewSetSpec* view, DimSpec* dimension) const
{
    if (tableItem->getTreeItemViewDimension(treeItem, dimension, view, type)) {
        *isTableItem = tableItem;
        return true;
    }

    // If the item is a knob of the table item...
    const std::vector<KnobAnimPtr>& knobs = tableItem->getKnobs();
    for (std::vector<KnobAnimPtr>::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        if ((*it)->getTreeItemViewDimension(treeItem, dimension, view, type)) {
            *isKnob = *it;
            return true;
        }
    }

    // Recurse on children
    const std::vector<TableItemAnimPtr>& children = tableItem->getChildren();
    for (std::size_t i = 0; i < children.size(); ++i) {
        if (findItemInTableItemRecursive(treeItem, children[i], type, isKnob, isTableItem, view, dimension)) {
            return true;
        }
    }
    return false;
}

bool
AnimationModule::findItem(QTreeWidgetItem* treeItem, AnimatedItemTypeEnum *type, KnobAnimPtr* isKnob, TableItemAnimPtr* isTableItem, NodeAnimPtr* isNodeItem, ViewSetSpec* view, DimSpec* dimension) const
{
    assert(type && isKnob && isTableItem && isNodeItem && view && dimension);

    // Only 0 or 1 of the 3 can be valid in output of this function
    isKnob->reset();
    isTableItem->reset();
    isNodeItem->reset();

    NodeAnimPtr parentNode;
    // Find the parent node of the item
    {
        QTreeWidgetItem *itemIt = treeItem;
        std::list<NodeAnimPtr>::iterator found = _imp->nodes.end();
        for (std::list<NodeAnimPtr>::iterator it = _imp->nodes.begin(); it != _imp->nodes.end(); ++it) {
            if ( (*it)->getTreeItem() == itemIt ) {
                found = it;
                break;
            }
        }

        // Recurse until a parent of the item can be mapped to a node name item
        while ( found == _imp->nodes.end() ) {
            if (!itemIt) {
                return NodeAnimPtr();
            }

            if ( itemIt->parent() ) {
                itemIt = itemIt->parent();
                found = _imp->nodes.end();
                for (std::list<NodeAnimPtr>::iterator it = _imp->nodes.begin(); it != _imp->nodes.end(); ++it) {
                    if ( (*it)->getTreeItem() == itemIt ) {
                        found = it;
                        break;
                    }
                }

            }
        }
        if (found == _imp->nodes.end()) {
            return false;
        }
        parentNode = *found;

    }
    if (!parentNode) {
        return false;
    }

    // If the item is a node...
    if (parentNode->getTreeItem() == treeItem) {
        *isNodeItem = parentNode;
        *type = parentNode->getItemType();
        *view = ViewSetSpec::all();
        *dimension = DimSpec::all();
        return true;
    }

    // If the item is a knob of the node...
    const std::vector<KnobAnimPtr>& knobs = parentNode->getKnobs();
    for (std::vector<KnobAnimPtr>::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        if ((*it)->getTreeItemViewDimension(treeItem, dimension, view, type)) {
            *isKnob = *it;
            return true;
        }
    }

    // If this is a table item...
    const std::vector<TableItemAnimPtr>& toplevelItems = parentNode->getTopLevelItems();
    for (std::size_t i = 0; i < toplevelItems.size(); ++i) {
        if (_imp->findItemInTableItemRecursive(treeItem, toplevelItems[i], type, isKnob, isTableItem, view, dimension)) {
            return true;
        }
    }
    return false;
} // findItem

NodeAnimPtr AnimationModule::findNodeAnim(const NodePtr& node) const
{
    for (std::list<NodeAnimPtr>::iterator it = _imp->nodes.begin(); it != _imp->nodes.end(); ++it) {
        if ( (*it)->getInternalNode() == node ) {
            return *it;
        }
    }

    return NodeAnimPtr();
}

NodeAnimPtr AnimationModule::findNodeAnim(const KnobIPtr &knob) const
{
    for (std::list<NodeAnimPtr>::iterator it = _imp->nodes.begin(); it != _imp->nodes.end(); ++it) {
        const std::vector<KnobAnimPtr>& knobs = (*it)->getKnobs();

        for (std::vector<KnobAnimPtr>::const_iterator it2 = knobs.begin(); it2 != knobs.end(); ++it2) {
            if ((*it2)->getInternalKnob() == knob) {
                return *it;
            }
        }
    }

    return NodeAnimPtr();
}

KnobAnimPtr AnimationModule::findKnobAnim(const KnobGuiConstPtr& knobGui) const
{
    for (std::list<NodeAnimPtr>::iterator it = _imp->nodes.begin(); it != _imp->nodes.end(); ++it) {
        const std::vector<KnobAnimPtr>& knobs = (*it)->getKnobs();

        for (std::vector<KnobAnimPtr>::const_iterator it2 = knobs.begin(); it2 != knobs.end(); ++it2) {
            if ((*it2)->getKnobGui() == knobGui) {
                return *it2;
            }
        }
    }
    return KnobAnimPtr();
}


NodeAnimPtr AnimationModule::getGroupNodeAnim(const NodeAnimPtr& node) const
{
    NodePtr internalNode = node->getInternalNode();
    if (!internalNode) {
        return NodeAnimPtr();
    }
    NodeGroupPtr parentGroup = toNodeGroup( internalNode->getGroup() );
    NodeAnimPtr parentGroupNodeAnim;

    if (parentGroup) {
        parentGroupNodeAnim = findNodeAnim( parentGroup->getNode() );
    }

    return parentGroupNodeAnim;
}

std::vector<NodeAnimPtr > AnimationModule::getChildrenNodes(const NodeAnimPtr& node) const
{
    std::vector<NodeAnimPtr > children;
    AnimatedItemTypeEnum nodeType = node->getItemType();
    if (nodeType == eAnimatedItemTypeGroup) {

        // If the node is a group, make all its children to be a child in the tree view
        NodeGroupPtr nodeGroup = node->getInternalNode()->isEffectNodeGroup();
        assert(nodeGroup);

        NodesList nodes = nodeGroup->getNodes();
        for (NodesList::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
            NodePtr childNode = (*it);
            NodeAnimPtr isInDopeSheet = findNodeAnim( childNode );
            if (isInDopeSheet) {
                children.push_back(isInDopeSheet);
            }
        }

    } else if ( node->isTimeNode() ) {
        // If the node is a time node, all input nodes recursively are considered to be a child
        std::list<NodePtr> markedNodes;
        _imp->getInputs_recursive(node->getInternalNode(), markedNodes, &children);
    }

    return children;
}

NodeAnimPtr AnimationModule::getNearestTimeNodeFromOutputs(const NodeAnimPtr& node) const
{
    std::list<NodePtr> markedNodes;
    NodePtr internalNode = node->getInternalNode();
    assert(internalNode);
    NodeCollectionPtr collection = internalNode->getGroup();
    NodePtr timeNode = _imp->getNearestTimeNodeFromOutputs_recursive(node->getInternalNode(), collection, markedNodes);

    return findNodeAnim(timeNode);
}

NodePtr
AnimationModule::getNearestReader(const NodeAnimPtr& timeNode) const
{
    assert( timeNode->isTimeNode() );
    std::list<NodePtr> markedNodes;
    NodePtr nearestReader = _imp->getNearestReaderFromInputs_recursive(timeNode->getInternalNode(), markedNodes);

    return nearestReader;
}

AnimationModuleSelectionModel&
AnimationModule::getSelectionModel() const
{
    return *_imp->selectionModel;
}

AnimationModuleEditor*
AnimationModule::getEditor() const
{
    return _imp->editor;
}

void
AnimationModule::deleteSelectedKeyframes()
{
    if ( _imp->selectionModel->isEmpty() ) {
        return;
    }

    AnimItemDimViewKeyFramesMap selectedKeyframes;
    std::vector<NodeAnimPtr > selectedNodes;
    std::vector<TableItemAnimPtr> tableItems;
    _imp->selectionModel->getCurrentSelection(&selectedKeyframes, &selectedNodes, &tableItems);
    
    _imp->selectionModel->clearSelection();
    


    _imp->pushUndoCommand(new AddOrRemoveKeysCommand(selectedKeyframes, false /*add*/, 0 /*parent*/));
}




void
AnimationModule::moveSelectedKeysAndNodes(double dt, double dv)
{
    AnimItemDimViewKeyFramesMap selectedKeyframes;
    std::vector<NodeAnimPtr > selectedNodes;
    std::vector<TableItemAnimPtr> tableItems;
    _imp->selectionModel->getCurrentSelection(&selectedKeyframes, &selectedNodes, &tableItems);

    // Constraint dt according to keyframe positions
    double maxLeft = INT_MIN;
    double maxRight = INT_MAX;



    for (AnimItemDimViewKeyFramesMap::iterator it = selectedKeyframes.begin(); it != selectedKeyframes.end(); ++it) {
        AnimItemBasePtr item = it->first.item;
        if (!item) {
            continue;
        }
        CurvePtr curve = item->getCurve(it->first.dim, it->first.view);
        if (!curve) {
            continue;
        }

        double epsilon = curve->areKeyFramesTimeClampedToIntegers() ? 1 : 1e-4;


        KeyFrame prevKey, nextKey;
        for (KeyFrameWithStringSet::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {

            // Adjust maxRight so that it can move up to the next keyframe
            if ( curve->getNextKeyframeTime( it2->key.getTime(), &nextKey ) ) {


                // If this keyframe has a next keyframe, check if it is selected
                KeyFrameWithString k;
                k.key = nextKey;
                KeyFrameWithStringSet::const_iterator foundKey = it->second.find(k);
                if (foundKey == it->second.end()) {
                    // The next keyframe is not selected
                    double diff = nextKey.getTime() - it2->key.getTime() - epsilon;
                    maxRight = std::max( 0., std::min(diff, maxRight) );
                }

            }

            // Adjust maxLeft so that it can move up to the previous keyframe
            if ( curve->getPreviousKeyframeTime( it2->key.getTime(), &prevKey ) ) {

                // If this keyframe has a next keyframe, check if it is selected
                KeyFrameWithString k;
                k.key = prevKey;
                KeyFrameWithStringSet::const_iterator foundKey = it->second.find(k);
                if (foundKey == it->second.end()) {
                    // The next keyframe is not selected
                    double diff = prevKey.getTime() - it2->key.getTime() + epsilon;
                    maxLeft = std::min( 0., std::max(diff, maxLeft) );
                }
            }

        } // for all keyframes
    } // for all item/dim/view

    // Do the clamp
    dt = std::min(dt, maxRight);
    dt = std::max(dt, maxLeft);
    if (dt == 0 && dv == 0) {
        return;
    }

    _imp->pushUndoCommand(new WarpKeysCommand(selectedKeyframes, selectedNodes, tableItems, dt, dv));
    
} // AnimationModule::moveSelectedKeysAndNodes

void
AnimationModule::trimReaderLeft(const NodeAnimPtr &reader,
                          double newFirstFrame)
{
    NodePtr node = reader->getInternalNode();

    KnobIntPtr firstFrameKnob = toKnobInt(node->getKnobByName(kReaderParamNameFirstFrame));
    KnobIntPtr lastFrameKnob = toKnobInt(node->getKnobByName(kReaderParamNameLastFrame));
    assert(lastFrameKnob);
    KnobIntPtr originalFrameRangeKnob = toKnobInt( node->getKnobByName(kReaderParamNameOriginalFrameRange));
    assert(originalFrameRangeKnob);


    int firstFrame = firstFrameKnob->getValue();
    int lastFrame = lastFrameKnob->getValue();
    int originalFirstFrame = originalFrameRangeKnob->getValue();

    newFirstFrame = std::max( (double)newFirstFrame, (double)originalFirstFrame );
    newFirstFrame = std::min( (double)lastFrame, newFirstFrame );
    if (newFirstFrame == firstFrame) {
        return;
    }

    _imp->pushUndoCommand( new KnobUndoCommand<int>(firstFrameKnob, firstFrame, (int)newFirstFrame, DimIdx(0), ViewSetSpec(0), eValueChangedReasonUserEdited, tr("Trim Left")));
}

void
AnimationModule::trimReaderRight(const NodeAnimPtr &reader,
                           double newLastFrame)
{
    NodePtr node = reader->getInternalNode();

    KnobIntPtr firstFrameKnob = toKnobInt(node->getKnobByName(kReaderParamNameFirstFrame));
    KnobIntPtr lastFrameKnob = toKnobInt(node->getKnobByName(kReaderParamNameLastFrame));
    assert(lastFrameKnob);
    KnobIntPtr originalFrameRangeKnob = toKnobInt( node->getKnobByName(kReaderParamNameOriginalFrameRange));
    assert(originalFrameRangeKnob);

    int firstFrame = firstFrameKnob->getValue();
    int lastFrame = lastFrameKnob->getValue();
    int originalLastFrame = originalFrameRangeKnob->getValue(DimIdx(1));

    newLastFrame = std::min( (double)newLastFrame, (double)originalLastFrame );
    newLastFrame = std::max( (double)firstFrame, newLastFrame );
    if (newLastFrame == lastFrame) {
        return;
    }

    _imp->pushUndoCommand( new KnobUndoCommand<int>(lastFrameKnob, lastFrame, (int)newLastFrame, DimIdx(0), ViewSetSpec(0), eValueChangedReasonUserEdited, tr("Trim Right")));
}

bool
AnimationModule::canSlipReader(const NodeAnimPtr &reader) const
{
    NodePtr node = reader->getInternalNode();

    KnobIntPtr firstFrameKnob = toKnobInt(node->getKnobByName(kReaderParamNameFirstFrame));
    KnobIntPtr lastFrameKnob = toKnobInt(node->getKnobByName(kReaderParamNameLastFrame));
    assert(lastFrameKnob);
    KnobIntPtr originalFrameRangeKnob = toKnobInt( node->getKnobByName(kReaderParamNameOriginalFrameRange));
    assert(originalFrameRangeKnob);


    ///Slipping means moving the timeOffset parameter by dt and moving firstFrame and lastFrame by -dt
    ///dt is clamped (firstFrame-originalFirstFrame) and (originalLastFrame-lastFrame)

    int currentFirstFrame = firstFrameKnob->getValue();
    int currentLastFrame = lastFrameKnob->getValue();
    int originalFirstFrame = originalFrameRangeKnob->getValue(DimIdx(0));
    int originalLastFrame = originalFrameRangeKnob->getValue(DimIdx(1));

    if ( ( (currentFirstFrame - originalFirstFrame) == 0 ) && ( (currentLastFrame - originalLastFrame) == 0 ) ) {
        return false;
    }

    return true;
}

void
AnimationModule::slipReader(const NodeAnimPtr &reader,
                      double dt)
{
    NodePtr node = reader->getInternalNode();

    KnobIntPtr firstFrameKnob = toKnobInt(node->getKnobByName(kReaderParamNameFirstFrame));
    KnobIntPtr lastFrameKnob = toKnobInt(node->getKnobByName(kReaderParamNameLastFrame));
    assert(lastFrameKnob);
    KnobIntPtr originalFrameRangeKnob = toKnobInt( node->getKnobByName(kReaderParamNameOriginalFrameRange));
    assert(originalFrameRangeKnob);
    KnobIntPtr timeOffsetKnob = toKnobInt( node->getKnobByName(kReaderParamNameTimeOffset));
    assert(timeOffsetKnob);


    ///Slipping means moving the timeOffset parameter by dt and moving firstFrame and lastFrame by -dt
    ///dt is clamped (firstFrame-originalFirstFrame) and (originalLastFrame-lastFrame)

    int currentFirstFrame = firstFrameKnob->getValue();
    int currentLastFrame = lastFrameKnob->getValue();
    int originalFirstFrame = originalFrameRangeKnob->getValue(DimIdx(0));
    int originalLastFrame = originalFrameRangeKnob->getValue(DimIdx(1));

    dt = std::min( dt, (double)(currentFirstFrame - originalFirstFrame) );
    dt = std::max( dt, (double)(currentLastFrame - originalLastFrame) );

    if (dt != 0) {
        EffectInstancePtr effect = node->getEffectInstance();
        effect->beginMultipleEdits(tr("Slip Reader").toStdString());
        firstFrameKnob->setValue(firstFrameKnob->getValue() - dt, ViewSetSpec(0), DimIdx(0), eValueChangedReasonUserEdited);
        lastFrameKnob->setValue(lastFrameKnob->getValue() - dt, ViewSetSpec(0), DimIdx(0), eValueChangedReasonUserEdited);
        timeOffsetKnob->setValue(timeOffsetKnob->getValue() + dt, ViewSetSpec(0), DimIdx(0), eValueChangedReasonUserEdited);
        effect->endMultipleEdits();
    }
}

void
AnimationModule::copySelectedKeys()
{
    if ( _imp->selectionModel->isEmpty() ) {
        return;
    }

    AnimItemDimViewKeyFramesMap selectedKeyframes;
    std::vector<NodeAnimPtr > selectedNodes;
    std::vector<TableItemAnimPtr> tableItems;
    _imp->selectionModel->getCurrentSelection(&selectedKeyframes, &selectedNodes, &tableItems);

    _imp->keyframesClipboard = selectedKeyframes;

}

void
AnimationModule::renameSelectedNode()
{
    AnimItemDimViewKeyFramesMap keys;
    std::vector<NodeAnimPtr > selectedNodes;
    std::vector<TableItemAnimPtr> tableItems;

    _imp->selectionModel->getCurrentSelection(&keys, &selectedNodes, &tableItems);
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
AnimationModule::onNodeNameEditDialogFinished()
{
    EditNodeNameDialog* dialog = qobject_cast<EditNodeNameDialog*>( sender() );

    if (dialog) {
        QDialog::DialogCode code =  (QDialog::DialogCode)dialog->result();
        if (code == QDialog::Accepted) {
            QString newName = dialog->getTypedName();
            QString oldName = QString::fromUtf8( dialog->getNode()->getNode()->getLabel().c_str() );
            _imp->pushUndoCommand( new RenameNodeUndoRedoCommand(dialog->getNode(), oldName, newName) );
        }
        dialog->deleteLater();
    }
}

void
AnimationModule::pasteKeys(bool relative)
{
    pasteKeys(_imp->keyframesClipboard, relative);
  
}

void
AnimationModule::pasteKeys(const AnimItemDimViewKeyFramesMap& keys, bool relative)
{
    if ( !keys.empty() ) {
        std::list<KnobAnimPtr > dstKnobs;
        _imp->editor->getTreeView()->getSelectedKnobAnims(&dstKnobs);
        if (dstKnobs.empty()) {
            Dialogs::warningDialog(tr("Paste").toStdString(), tr("You must select at least one parameter dimension on the left to perform this action").toStdString());
            return;
        }
        _imp->pushUndoCommand( new DSPasteKeysCommand(keys, dstKnobs, relative, _imp->editor) );
    }
}

void
AnimationModule::setSelectedKeysInterpolation(KeyframeTypeEnum keyType)
{
    if ( _imp->selectionModel->isEmpty() ) {
        return;
    }

    AnimItemDimViewKeyFramesMap keys;
    std::vector<NodeAnimPtr > selectedNodes;
    std::vector<TableItemAnimPtr> tableItems;
    
    _imp->selectionModel->getCurrentSelection(&keys, &selectedNodes, &tableItems);

    _imp->pushUndoCommand(new SetKeysInterpolationCommand(keys, keyType, 0));
    
}

void
AnimationModule::transformSelectedKeys(const Transform::Matrix3x3& transform)
{
    if ( _imp->selectionModel->isEmpty() ) {
        return;
    }
    AnimItemDimViewKeyFramesMap keys;
    std::vector<NodeAnimPtr > selectedNodes;
    std::vector<TableItemAnimPtr> tableItems;
    _imp->selectionModel->getCurrentSelection(&keys, &selectedNodes, &tableItems);
    
    boost::scoped_ptr<Curve::AffineKeyFrameWarp> warp(new Curve::AffineKeyFrameWarp(transform));
    
    // Test the warp, if it cannot be applied, do not push an undo/redo command
    if (!WarpKeysCommand::testWarpOnKeys(keys, *warp)) {
        return;
    }
    
    // Try once to warp everything, if it doesn't work fail it now
    _imp->pushUndoCommand(new WarpKeysCommand(keys, transform));
}

SequenceTime
AnimationModule::getCurrentFrame() const
{
    return _imp->timeline.lock()->currentFrame();
}




NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_AnimationModule.cpp"
