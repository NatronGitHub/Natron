/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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
#include "Engine/KnobUndoCommand.h"
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
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/KnobItemsTableGui.h"
#include "Gui/KnobAnim.h"
#include "Gui/KnobGui.h"
#include "Gui/Menu.h"
#include "Gui/NodeAnim.h"
#include "Gui/NodeGui.h"
#include "Gui/NodeGraph.h"
#include "Gui/NodeGraphUndoRedo.h"
#include "Gui/TableItemAnim.h"

NATRON_NAMESPACE_ENTER

typedef std::map<KnobIWPtr, KnobGui *> KnobsAndGuis;
typedef std::pair<QTreeWidgetItem *, NodeAnimPtr> TreeItemAndNodeAnim;
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

    /* attributes */
    AnimationModule *_publicInterface;
    std::list<NodeAnimPtr> nodes;
    AnimationModuleSelectionModelPtr selectionModel;
    boost::shared_ptr<QUndoStack> undoStack;
    TimeLineWPtr timeline;
    AnimationModuleEditor* editor;
};



AnimationModule::AnimationModule(Gui *gui,
                                 AnimationModuleEditor* editor,
                                 const TimeLinePtr &timeline)
: AnimationModuleBase(gui)
, _imp( new AnimationModulePrivate(editor, this) )
{
    _imp->timeline = timeline;

    gui->registerNewUndoStack(_imp->undoStack);
}


// make_shared enabler (because make_shared needs access to the private constructor)
// see https://stackoverflow.com/a/20961251/2607517
struct AnimationModule::MakeSharedEnabler: public AnimationModule
{
    MakeSharedEnabler(Gui *gui,
                      AnimationModuleEditor* editor,
                      const TimeLinePtr &timeline) : AnimationModule(gui, editor, timeline) {
    }
};


AnimationModulePtr
AnimationModule::create(Gui *gui,
                        AnimationModuleEditor* editor,
                        const TimeLinePtr &timeline)
{
    AnimationModulePtr ret = boost::make_shared<AnimationModule::MakeSharedEnabler>(gui, editor, timeline);
    ret->ensureSelectionModel();
    return ret;
}


AnimationModule::~AnimationModule()
{
    if (!_imp->editor->getGui()->getApp()->isClosing()) {
        _imp->editor->getGui()->removeUndoStack(_imp->undoStack);
    }
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
    if (!node->getEffectInstance()->getAllItemsTables().empty()) {
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
AnimationModule::pushUndoCommand(QUndoCommand *cmd)
{
    if (_imp->editor) {
        _imp->editor->pushUndoCommand(cmd);
    } else {
        // redo() was called, destroy command
        delete cmd;
    }
}


NodePtr
AnimationModulePrivate::getNearestTimeNodeFromOutputs_recursive(const NodePtr& node,
                                                                const NodeCollectionPtr& collection,
                                                                std::list<NodePtr>& markedNodes) const
{
    OutputNodesMap outputs;
    node->getOutputs(outputs);

    if ( std::find(markedNodes.begin(), markedNodes.end(), node) != markedNodes.end() ) {
        return NodePtr();
    }

    // If we go outside of the initial node group scope, return
    if (node->getGroup() != collection) {
        return NodePtr();
    }
    markedNodes.push_back(node);
    for (OutputNodesMap::const_iterator it = outputs.begin(); it != outputs.end(); ++it) {

        std::string pluginID = it->first->getPluginID();
        if ( (pluginID == PLUGINID_OFX_RETIME) || ( pluginID == PLUGINID_OFX_TIMEOFFSET) || ( pluginID == PLUGINID_OFX_FRAMERANGE) ) {
            return it->first;
        } else {
            NodePtr ret =  getNearestTimeNodeFromOutputs_recursive(it->first, collection, markedNodes);
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
    const std::vector<NodeWPtr> &inputs = node->getInputs();

    if ( std::find(markedNodes.begin(), markedNodes.end(), node) != markedNodes.end() ) {
        return NodePtr();
    }
    markedNodes.push_back(node);
    for (std::vector<NodeWPtr>::const_iterator it = inputs.begin(); it != inputs.end(); ++it) {
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
                                            std::vector<NodeAnimPtr> *result) const
{
    if (std::find(markedNodes.begin(), markedNodes.end(), node) != markedNodes.end()) {
        return;
    }

    markedNodes.push_back(node);


    const std::vector<NodeWPtr> &inputs = node->getInputs();
    for (std::vector<NodeWPtr>::const_iterator it = inputs.begin(); it != inputs.end(); ++it) {
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

boost::shared_ptr<QUndoStack>
AnimationModule::getUndoStack() const
{
    return _imp->undoStack;
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
            (*it)->refreshVisibility();
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
    //if ( !getNodeCanAnimate(node) ) {
    //    return;
    //}

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

    NodeAnimPtr anim(NodeAnim::create(toAnimationModule(shared_from_this()), nodeType, nodeGui) );
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
    (*toRemove)->refreshFrameRange();
    
    _imp->selectionModel->removeAnyReferenceFromSelection(*toRemove);
    Q_EMIT nodeAboutToBeRemoved(*toRemove);

    _imp->nodes.erase(toRemove);


    // Recompute selection rectangle bounding box
    refreshSelectionBboxAndUpdateView();

}

bool
AnimationModule::findItem(QTreeWidgetItem* treeItem, AnimatedItemTypeEnum *type, KnobAnimPtr* isKnob, TableItemAnimPtr* isTableItem, NodeAnimPtr* isNodeItem, ViewSetSpec* view, DimSpec* dimension) const
{
    assert(treeItem && type && isKnob && isTableItem && isNodeItem && view && dimension);

    if (!treeItem) {
        return false;
    }

    void* ptr = treeItem->data(0, QT_ROLE_CONTEXT_ITEM_POINTER).value<void*>();
    if (!ptr) {
        return false;
    }

    *type = (AnimatedItemTypeEnum)treeItem->data(0, QT_ROLE_CONTEXT_TYPE).toInt();
    switch (*type) {
        case eAnimatedItemTypeCommon:
        case eAnimatedItemTypeFrameRange:
        case eAnimatedItemTypeGroup:
        case eAnimatedItemTypeReader:
        case eAnimatedItemTypeRetime:
        case eAnimatedItemTypeTimeOffset:
            *isNodeItem = ((NodeAnim*)ptr)->shared_from_this();
            break;
        case eAnimatedItemTypeKnobDim:
        case eAnimatedItemTypeKnobRoot:
        case eAnimatedItemTypeKnobView:
            *isKnob = toKnobAnim(((AnimItemBase*)ptr)->shared_from_this());
            break;
        case eAnimatedItemTypeTableItemRoot:
            *isTableItem = ((TableItemAnim*)ptr)->shared_from_this();
            break;
    }
    *view = ViewSetSpec(treeItem->data(0, QT_ROLE_CONTEXT_VIEW).toInt());
    *dimension = DimSpec(treeItem->data(0, QT_ROLE_CONTEXT_DIM).toInt());
    return true;
} // findItem

void
AnimationModule::getTopLevelNodes(bool onlyVisible, std::vector<NodeAnimPtr>* nodes) const
{
    AnimationModuleTreeView* treeView = _imp->editor->getTreeView();
    for (std::list<NodeAnimPtr>::iterator it = _imp->nodes.begin(); it != _imp->nodes.end(); ++it) {
        if (!onlyVisible || treeView->isItemVisibleRecursive((*it)->getTreeItem())) {
            nodes->push_back(*it);
        }
    }
}

static TableItemAnimPtr findTableItemAnimRecursive(const TableItemAnimPtr& anim, const KnobTableItemPtr& item)
{
    if (anim->getInternalItem() == item) {
        return anim;
    }
    const std::vector<TableItemAnimPtr>& children = anim->getChildren();
    for (std::size_t i = 0; i < children.size(); ++i) {
        TableItemAnimPtr r = findTableItemAnimRecursive(children[i], item);
        if (r) {
            return r;
        }
    }
    return TableItemAnimPtr();
}

TableItemAnimPtr
AnimationModule::findTableItemAnim(const KnobTableItemPtr& item) const
{
    KnobItemsTablePtr model = item->getModel();
    if (!model) {
        return TableItemAnimPtr();
    }
    for (std::list<NodeAnimPtr>::iterator it = _imp->nodes.begin(); it != _imp->nodes.end(); ++it) {
        const std::vector<TableItemAnimPtr>& toplevelItems = (*it)->getTopLevelItems();
        for (std::size_t i = 0; i < toplevelItems.size(); ++i) {
            TableItemAnimPtr r = findTableItemAnimRecursive(toplevelItems[i], item);
            if (r) {
                return r;
            }
        }
    }
    return TableItemAnimPtr();

}

NodeAnimPtr AnimationModule::findNodeAnim(const NodePtr& node) const
{
    if (!node) {
        return NodeAnimPtr();
    }
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
    KnobIPtr internalKnob = knobGui->getKnob();
    for (std::list<NodeAnimPtr>::iterator it = _imp->nodes.begin(); it != _imp->nodes.end(); ++it) {
        const std::vector<KnobAnimPtr>& knobs = (*it)->getKnobs();

        for (std::vector<KnobAnimPtr>::const_iterator it2 = knobs.begin(); it2 != knobs.end(); ++it2) {
            if ((*it2)->getInternalKnob() == internalKnob) {
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

std::vector<NodeAnimPtr> AnimationModule::getChildrenNodes(const NodeAnimPtr& node) const
{
    std::vector<NodeAnimPtr> children;
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

NodeAnimPtr AnimationModule::getNearestTimeNodeFromOutputsInternal(const NodePtr& node) const
{
    std::list<NodePtr> markedNodes;
    NodeCollectionPtr collection = node->getGroup();
    NodePtr timeNode = _imp->getNearestTimeNodeFromOutputs_recursive(node, collection, markedNodes);
    return findNodeAnim(timeNode);
}

NodeAnimPtr AnimationModule::getNearestTimeNodeFromOutputs(const NodeAnimPtr& node) const
{
    NodePtr internalNode = node->getInternalNode();
    return getNearestTimeNodeFromOutputsInternal(internalNode);
}

NodeAnimPtr
AnimationModule::getNearestReaderInternal(const NodePtr& timeNode) const
{
    std::list<NodePtr> markedNodes;
    return findNodeAnim(_imp->getNearestReaderFromInputs_recursive(timeNode, markedNodes));
}

NodeAnimPtr
AnimationModule::getNearestReader(const NodeAnimPtr& timeNode) const
{
    return getNearestReaderInternal(timeNode->getInternalNode());
}

AnimationModuleSelectionModelPtr
AnimationModule::getSelectionModel() const
{
    return _imp->selectionModel;
}

AnimationModuleEditor*
AnimationModule::getEditor() const
{
    return _imp->editor;
}

void
AnimationModule::renameSelectedNode()
{
    const std::list<NodeAnimPtr>& selectedNodes = _imp->selectionModel->getCurrentNodesSelection();
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
            pushUndoCommand( new RenameNodeUndoRedoCommand(dialog->getNode(), oldName, newName) );
        }
        dialog->deleteLater();
    }
}

TimeLinePtr
AnimationModule::getTimeline() const
{
    return _imp->timeline.lock();
}

void
AnimationModule::refreshSelectionBboxAndUpdateView()
{
    _imp->editor->refreshSelectionBboxAndRedrawView();
}

AnimationModuleView*
AnimationModule::getView() const
{
    return _imp->editor->getView();
}

static bool getCurveVisibleRecursive(QTreeWidgetItem* item)
{

    int nc = item->columnCount();
    bool thisItemVisible = true;
    if (nc > ANIMATION_MODULE_TREE_VIEW_COL_VISIBLE) {
        thisItemVisible = item->data(ANIMATION_MODULE_TREE_VIEW_COL_VISIBLE, QT_ROLE_CONTEXT_ITEM_VISIBLE).toBool();
    }
    if (!thisItemVisible) {
        return false;
    }
    QTreeWidgetItem* parentItem = item->parent();
    if (parentItem) {
        return getCurveVisibleRecursive(parentItem);
    }
    return true;
}

bool
AnimationModule::isCurveVisible(const AnimItemBasePtr& item, DimIdx dimension, ViewIdx view) const
{
    QTreeWidgetItem* treeItem = item->getTreeItem(dimension, view);
    if (!treeItem) {
        return false;
    }
    return getCurveVisibleRecursive(treeItem);
}

int
AnimationModule::getTreeColumnsCount() const
{
    return _imp->editor->getTreeView()->columnCount();
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_AnimationModule.cpp"
