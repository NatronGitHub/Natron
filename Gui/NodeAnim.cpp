/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 * Copyright (C) 2018-2020 The Natron developers
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

#include "NodeAnim.h"

#include <QTreeWidgetItem>
#include <QPixmapCache>

#include "Engine/KnobTypes.h"
#include "Engine/KnobItemsTable.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"

#include "Gui/AnimationModule.h"
#include "Gui/AnimationModuleSelectionModel.h"
#include "Gui/AnimationModuleTreeView.h"
#include "Gui/AnimationModuleEditor.h"
#include "Gui/KnobAnim.h"
#include "Gui/KnobItemsTableGui.h"
#include "Gui/KnobGui.h"
#include "Gui/NodeGui.h"
#include "Gui/GuiDefines.h"
#include "Gui/TableItemAnim.h"

NATRON_NAMESPACE_ENTER

class NodeAnimPrivate
{
public:
    NodeAnimPrivate(NodeAnim* publicInterface)
    : _publicInterface(publicInterface)
    , model()
    , nodeType()
    , nodeGui()
    , nameItem(0)
    , knobs()
    , topLevelTableItems()
    , frameRange()
    , rangeComputationRecursion(0)
    {
        frameRange.min = frameRange.max = 0.;
    }


    /* attributes */
    NodeAnim* _publicInterface;
    AnimationModuleWPtr model;
    AnimatedItemTypeEnum nodeType;
    NodeGuiWPtr nodeGui;
    QTreeWidgetItem* nameItem;
    std::vector<KnobAnimPtr> knobs;
    std::vector<TableItemAnimPtr> topLevelTableItems;


    // Frame range
    RangeD frameRange;
    // to avoid recursion in groups
    int rangeComputationRecursion;

public:

    void computeNodeRange();
    void computeReaderRange();
    void computeRetimeRange();
    void computeTimeOffsetRange();
    void computeFRRange();
    void computeGroupRange();
    void computeCommonNodeRange();

    void refreshParentContainerRange();

    void removeItem(const KnobTableItemPtr& item, TableChangeReasonEnum reason);
    void insertItem(int index, const KnobTableItemPtr& item, TableChangeReasonEnum reason);

};

// Small RAII style class to prevent recursion in frame range computation
class ComputingFrameRangeRecursion_RAII
{
    NodeAnimPrivate* _imp;
public:

    ComputingFrameRangeRecursion_RAII(NodeAnimPrivate* imp)
    : _imp(imp)
    {
        ++_imp->rangeComputationRecursion;
    }

    ~ComputingFrameRangeRecursion_RAII()
    {
        --_imp->rangeComputationRecursion;
    }
};


NodeAnim::NodeAnim(const AnimationModulePtr &model,
                   const NodeGuiPtr &nodeGui)
: _imp(new NodeAnimPrivate(this))
{
    _imp->model = model;
    _imp->nodeGui = nodeGui;


}


// make_shared enabler (because make_shared needs access to the private constructor)
// see https://stackoverflow.com/a/20961251/2607517
struct NodeAnim::MakeSharedEnabler: public NodeAnim
{
    MakeSharedEnabler(const AnimationModulePtr &model,
                      const NodeGuiPtr &nodeGui) : NodeAnim(model, nodeGui) {
    }
};


NodeAnimPtr
NodeAnim::create(const AnimationModulePtr& model,
                   AnimatedItemTypeEnum itemType,
                   const NodeGuiPtr &nodeGui)
{
    NodeAnimPtr ret = boost::make_shared<NodeAnim::MakeSharedEnabler>(model, nodeGui);
    ret->initialize(itemType);
    return ret;
}


NodeAnim::~NodeAnim()
{
    // Kill sub items first before killing the top level item
    for (std::size_t i = 0; i < _imp->knobs.size(); ++i) {
        _imp->knobs[i]->destroyItems();
    }
    for (std::size_t i = 0; i < _imp->topLevelTableItems.size(); ++i) {
        _imp->topLevelTableItems[i]->destroyItems();
    }
    _imp->knobs.clear();
    _imp->topLevelTableItems.clear();

    AnimationModuleBasePtr model = getModel();
    bool isTearingDown;
    if (model) {
        isTearingDown = model->isAboutToBeDestroyed();
    } else {
        isTearingDown = true;
    }

    if (!isTearingDown) {
        delete _imp->nameItem;
    }
    _imp->nameItem = 0;
}

void
NodeAnim::initialize(AnimatedItemTypeEnum nodeType)
{
    _imp->nodeType = nodeType;
    NodePtr internalNode = getNodeGui()->getNode();

    AnimationModuleBasePtr model = getModel();
    NodeAnimPtr thisShared = shared_from_this();
    _imp->nameItem = new QTreeWidgetItem;
    _imp->nameItem->setData(0, QT_ROLE_CONTEXT_ITEM_POINTER, qVariantFromValue((void*)thisShared.get()));
    _imp->nameItem->setText( 0, QString::fromUtf8( internalNode->getLabel().c_str() ) );
    _imp->nameItem->setData(0, QT_ROLE_CONTEXT_TYPE, nodeType);
    _imp->nameItem->setData(0, QT_ROLE_CONTEXT_IS_ANIMATED, true);
    _imp->nameItem->setExpanded(true);
    int nCols = getModel()->getTreeColumnsCount();
    if (nCols > ANIMATION_MODULE_TREE_VIEW_COL_VISIBLE) {
        _imp->nameItem->setData(ANIMATION_MODULE_TREE_VIEW_COL_VISIBLE, QT_ROLE_CONTEXT_ITEM_VISIBLE, QVariant(true));
        _imp->nameItem->setIcon(ANIMATION_MODULE_TREE_VIEW_COL_VISIBLE, model->getItemVisibilityIcon(true));
    }
    if (nCols > ANIMATION_MODULE_TREE_VIEW_COL_PLUGIN_ICON) {
        QString iconFilePath = QString::fromUtf8(internalNode->getPluginIconFilePath().c_str());
        AnimationModuleTreeView::setItemIcon(iconFilePath, _imp->nameItem);
    }

    connect( internalNode.get(), SIGNAL(labelChanged(QString, QString)), this, SLOT(onNodeLabelChanged(QString, QString)) );

    initializeKnobsAnim();

    initializeTableItems();

    // Connect signals/slots to knobs to compute the frame range
    AnimationModulePtr animModel = toAnimationModule(model);
    assert(animModel);

    if (nodeType == eAnimatedItemTypeCommon) {
        // Also connect the lifetime knob
        KnobIntPtr lifeTimeKnob = internalNode->getEffectInstance()->getLifeTimeKnob();
        if (lifeTimeKnob) {
            connect( lifeTimeKnob->getSignalSlotHandler().get(), SIGNAL(mustRefreshKnobGui(ViewSetSpec,DimSpec,ValueChangedReasonEnum)), this, SLOT(onFrameRangeKnobChanged()) );
        }

    } else if (nodeType == eAnimatedItemTypeReader) {
        // The dopesheet view must refresh if the user set some values in the settings panel
        // so we connect some signals/slots
        KnobIPtr lastFrameKnob = internalNode->getKnobByName(kReaderParamNameLastFrame);
        if (!lastFrameKnob) {
            return;
        }
        boost::shared_ptr<KnobSignalSlotHandler> lastFrameKnobHandler =  lastFrameKnob->getSignalSlotHandler();
        assert(lastFrameKnob);
        boost::shared_ptr<KnobSignalSlotHandler> startingTimeKnob = internalNode->getKnobByName(kReaderParamNameStartingTime)->getSignalSlotHandler();
        assert(startingTimeKnob);

        connect( lastFrameKnobHandler.get(), SIGNAL(mustRefreshKnobGui(ViewSetSpec,DimSpec,ValueChangedReasonEnum)), this, SLOT(onFrameRangeKnobChanged()) );

        connect( startingTimeKnob.get(), SIGNAL(mustRefreshKnobGui(ViewSetSpec,DimSpec,ValueChangedReasonEnum)), this, SLOT(onFrameRangeKnobChanged()) );

        // We don't make the connection for the first frame knob, because the
        // starting time is updated when it's modified. Thus we avoid two
        // refreshes of the view.
    } else if (nodeType == eAnimatedItemTypeRetime) {
        boost::shared_ptr<KnobSignalSlotHandler> speedKnob =  internalNode->getKnobByName(kRetimeParamNameSpeed)->getSignalSlotHandler();
        assert(speedKnob);

        connect( speedKnob.get(), SIGNAL(mustRefreshKnobGui(ViewSetSpec,DimSpec,ValueChangedReasonEnum)), this, SLOT(onFrameRangeKnobChanged()) );
    } else if (nodeType == eAnimatedItemTypeTimeOffset) {
        boost::shared_ptr<KnobSignalSlotHandler> timeOffsetKnob =  internalNode->getKnobByName(kReaderParamNameTimeOffset)->getSignalSlotHandler();
        assert(timeOffsetKnob);

        connect( timeOffsetKnob.get(), SIGNAL(mustRefreshKnobGui(ViewSetSpec,DimSpec,ValueChangedReasonEnum)), this, SLOT(onFrameRangeKnobChanged()) );
    } else if (nodeType == eAnimatedItemTypeFrameRange) {
        boost::shared_ptr<KnobSignalSlotHandler> frameRangeKnob =  internalNode->getKnobByName(kFrameRangeParamNameFrameRange)->getSignalSlotHandler();
        assert(frameRangeKnob);

        connect( frameRangeKnob.get(), SIGNAL(mustRefreshKnobGui(ViewSetSpec,DimSpec,ValueChangedReasonEnum)),  this, SLOT(onFrameRangeKnobChanged()) );
    }
    
    refreshFrameRange();

} // initialize

void
NodeAnim::onFrameRangeKnobChanged()
{
    refreshVisibility();
}

void
NodeAnim::initializeKnobsAnim()
{
    // Create dope sheet knobs
    NodeGuiPtr nodeUI = _imp->nodeGui.lock();
    assert(nodeUI);
    const KnobsVec& knobs = nodeUI->getNode()->getKnobs();

    NodeAnimPtr thisShared = shared_from_this();


    for (KnobsVec::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {

        // If the knob is not animted, don't create an item
        if ( !(*it)->canAnimate() || !(*it)->isAnimationEnabled() ) {
            continue;
        }



        KnobAnimPtr knobAnimObject(KnobAnim::create(getModel(), thisShared, _imp->nameItem, *it));
        _imp->knobs.push_back(knobAnimObject);
        
    } // for all knobs

} // initializeKnobsAnim

void
NodeAnim::initializeTableItems()
{
    NodeGuiPtr nodeGui = _imp->nodeGui.lock();
    std::list<KnobItemsTableGuiPtr> tables = nodeGui->getAllKnobItemsTables();
    for (std::list<KnobItemsTableGuiPtr>::const_iterator it = tables.begin(); it != tables.end(); ++it) {
        KnobItemsTablePtr internalTable = (*it)->getInternalTable();
        if (!internalTable) {
            return;
        }

        connect(internalTable.get(), SIGNAL(itemRemoved(KnobTableItemPtr,TableChangeReasonEnum)), this, SLOT(onTableItemRemoved(KnobTableItemPtr,TableChangeReasonEnum)));
        connect(internalTable.get(), SIGNAL(itemInserted(int,KnobTableItemPtr,TableChangeReasonEnum)), this, SLOT(onTableItemInserted(int,KnobTableItemPtr,TableChangeReasonEnum)));

        NodeAnimPtr thisShared = shared_from_this();

        std::vector<KnobTableItemPtr> allItems = internalTable->getTopLevelItems();
        for (std::size_t i = 0; i < allItems.size(); ++i) {
            TableItemAnimPtr anim(TableItemAnim::create(getModel(), *it, thisShared, allItems[i], _imp->nameItem));
            _imp->topLevelTableItems.push_back(anim);
        }

    }


} // initializeTableItems

TableItemAnimPtr
NodeAnim::findTableItem(const KnobTableItemPtr& item) const
{
    for (std::size_t i = 0; i < _imp->topLevelTableItems.size(); ++i) {
        if (_imp->topLevelTableItems[i]->getInternalItem() == item) {
            return _imp->topLevelTableItems[i];
        } else {
            TableItemAnimPtr foundChild = _imp->topLevelTableItems[i]->findTableItem(item);
            if (foundChild) {
                return foundChild;
            }
        }
    }
    return TableItemAnimPtr();
}

void
NodeAnim::onTableItemRemoved(const KnobTableItemPtr& item, TableChangeReasonEnum reason)
{
    _imp->removeItem(item, reason);
}

void
NodeAnimPrivate::removeItem(const KnobTableItemPtr& item, TableChangeReasonEnum /*reason*/)
{
    TableItemAnimPtr found;
    for (std::vector<TableItemAnimPtr>::iterator it = topLevelTableItems.begin(); it!=topLevelTableItems.end(); ++it) {
        if ((*it)->getInternalItem() == item) {
            found = *it;
            topLevelTableItems.erase(it);
            model.lock()->getSelectionModel()->removeAnyReferenceFromSelection(found);
            break;
        } else {
            TableItemAnimPtr found = (*it)->removeItem(item);
            if (found) {
                break;
            }
        }
    }
}

void
NodeAnimPrivate::insertItem(int index, const KnobTableItemPtr& item, TableChangeReasonEnum reason)
{

    // The item already exists
    if (_publicInterface->findTableItem(item)) {
        return;
    }
    KnobTableItemPtr parentItem = item->getParent();
    TableItemAnimPtr parentAnim;
    if (parentItem) {

        // If the parent item is not yet in the model, do not create item, the parent will create its children recursively
        if (parentItem->getIndexInParent() == -1) {
            return;
        }
        parentAnim = _publicInterface->findTableItem(parentItem);
    }
    KnobItemsTableGuiPtr table = _publicInterface->getNodeGui()->getKnobItemsTable(item->getModel()->getTableIdentifier());
    assert(table);
    if (parentItem) {
        TableItemAnimPtr anim(TableItemAnim::create(_publicInterface->getModel(), table, _publicInterface->shared_from_this(), item, parentAnim->getRootItem()));
        parentAnim->insertChild(index, anim);
    } else {
        TableItemAnimPtr anim(TableItemAnim::create(_publicInterface->getModel(), table, _publicInterface->shared_from_this(), item, nameItem));
        if (index < 0 || index >= (int)topLevelTableItems.size()) {
            topLevelTableItems.push_back(anim);
        } else {
            std::vector<TableItemAnimPtr>::iterator it = topLevelTableItems.begin();
            std::advance(it, index);
            topLevelTableItems.insert(it, anim);
        }
    }

    // Create children recursively
    std::vector<KnobTableItemPtr> children = item->getChildren();
    for (std::size_t i = 0; i < children.size(); ++i) {
        insertItem(i, children[i], reason);
    }

}

void
NodeAnim::onTableItemInserted(int index, const KnobTableItemPtr& item, TableChangeReasonEnum reason)
{
    _imp->insertItem(index, item, reason);
}


void
NodeAnim::onNodeLabelChanged(const QString &/*oldName*/, const QString& newName)
{
    _imp->nameItem->setText(0, newName);
}

AnimationModulePtr
NodeAnim::getModel() const
{
    return _imp->model.lock();
}

QTreeWidgetItem *
NodeAnim::getTreeItem() const
{
    return _imp->nameItem;
}

NodeGuiPtr
NodeAnim::getNodeGui() const
{
    return _imp->nodeGui.lock();
}

NodePtr
NodeAnim::getInternalNode() const
{
    NodeGuiPtr node = getNodeGui();

    return node ? node->getNode() : NodePtr();
}

/**
 * @brief NodeAnim::getTreeItemsAndKnobAnims
 *
 *
 */
const std::vector<KnobAnimPtr>&
NodeAnim::getKnobs() const
{
    return _imp->knobs;
}

const std::vector<TableItemAnimPtr>&
NodeAnim::getTopLevelItems() const
{
    return _imp->topLevelTableItems;
}

AnimatedItemTypeEnum
NodeAnim::getItemType() const
{
    return _imp->nodeType;
}

bool
NodeAnim::isTimeNode() const
{
    return _imp->nodeType == eAnimatedItemTypeRetime ||
    _imp->nodeType == eAnimatedItemTypeFrameRange ||
    _imp->nodeType == eAnimatedItemTypeTimeOffset;
}

bool
NodeAnim::canContainOtherNodeContexts() const
{
    return isTimeNode() || _imp->nodeType == eAnimatedItemTypeGroup;
}

bool
NodeAnim::isRangeDrawingEnabled() const
{
    if (canContainOtherNodeContexts() || _imp->nodeType == eAnimatedItemTypeReader) {
        return true;
    }
    // Check if the lifetime nodes are set
    NodePtr node = getInternalNode();
    if (!node) {
        return false;
    }
    int first,last;
    return node->getEffectInstance()->isLifetimeActivated(&first, &last);
}

bool
NodeAnim::containsNodeContext() const
{
    
    for (int i = 0; i < _imp->nameItem->childCount(); ++i) {
        int childType = _imp->nameItem->child(i)->data(0, QT_ROLE_CONTEXT_TYPE).toInt();

        if (childType == eAnimatedItemTypeCommon ||
            canContainOtherNodeContexts()) {
            return true;
        }
    }

    return false;
}

void
NodeAnim::refreshVisibility()
{

    AnimationModulePtr animModule = getModel();
    QTreeWidgetItem *nodeItem = getTreeItem();
    bool showNode = false;
    int nChildren = nodeItem->childCount();

    // Refresh children, which will recursively refresh their children
    for (int i = 0; i < nChildren; ++i) {
        QTreeWidgetItem* child = nodeItem->child(i);

        AnimatedItemTypeEnum type;
        KnobAnimPtr isKnob;
        TableItemAnimPtr isTableItem;
        NodeAnimPtr isNodeItem;
        ViewSetSpec view;
        DimSpec dim;
        bool found = animModule->findItem(child, &type, &isKnob, &isTableItem, &isNodeItem, &view, &dim);
        if (!found) {
            continue;
        }
        if (isTableItem) {
            isTableItem->refreshVisibilityConditional(false /*refreshParent*/);
        } else if (isNodeItem) {
            isNodeItem->refreshVisibility();
        } else if (isKnob) {
            isKnob->refreshVisibilityConditional(false /*refreshHolder*/);
        }
        if (!child->isHidden()) {
            showNode = true;
        }
    }

    if (!showNode) {
        // If so far none of the children should be displayed, still check if the node has a range
        if (isRangeDrawingEnabled()) {
            showNode = true;
        }
        
    }

    // If settings panel is not opened and the "Keep in Animation Module" knob is not checked, hide the node.
    NodeGuiPtr nodeGui = getNodeGui();
    bool keepInAnimationModule = nodeGui->getNode()->getEffectInstance()->isKeepInAnimationModuleButtonDown();
    if (!keepInAnimationModule && !nodeGui->isSettingsPanelVisible()) {
        showNode = false;
    }

    refreshFrameRange();

    nodeItem->setData(0, QT_ROLE_CONTEXT_IS_ANIMATED, showNode);

    nodeItem->setHidden(!showNode);


} // refreshVisibility



RangeD
NodeAnim::getFrameRange() const
{
    return _imp->frameRange;
}

void
NodeAnim::refreshFrameRange()
{
    _imp->computeNodeRange();
}

void
NodeAnimPrivate::computeNodeRange()
{
    if (rangeComputationRecursion > 0) {
        return;
    }
    {
        ComputingFrameRangeRecursion_RAII computingFrameRangeRecursionCounter(this);

        frameRange.min = frameRange.max = 0;

        switch (nodeType) {
            case eAnimatedItemTypeReader:
                computeReaderRange();
                break;
            case eAnimatedItemTypeRetime:
                computeRetimeRange();
                break;
            case eAnimatedItemTypeTimeOffset:
                computeTimeOffsetRange();
                break;
            case eAnimatedItemTypeFrameRange:
                computeFRRange();
                break;
            case eAnimatedItemTypeGroup:
                computeGroupRange();
                break;
            case eAnimatedItemTypeCommon:
                computeCommonNodeRange();
                break;
            default:
                break;
        }
        
        refreshParentContainerRange();
    } // ComputingFrameRangeRecursion_RAII
} // computeNodeRange

void
NodeAnimPrivate::refreshParentContainerRange()
{
    NodeGuiPtr nodeUI = nodeGui.lock();
    NodePtr node = nodeUI->getNode();
    if (!node) {
        return;
    }
    AnimationModulePtr isAnimModule = toAnimationModule(model.lock());
    assert(isAnimModule);

    // If inside a group, refresh the group
    {
        NodeGroupPtr parentGroup = toNodeGroup( node->getGroup() );
        NodeAnimPtr parentGroupNodeAnim;

        if (parentGroup) {
            parentGroupNodeAnim = isAnimModule->findNodeAnim( parentGroup->getNode() );
        }
        if (parentGroupNodeAnim) {
            parentGroupNodeAnim->refreshFrameRange();
        }
    }
    // if modified by a time node, refresh its frame range as well
    {
        NodeAnimPtr isConnectedToTimeNode = isAnimModule->getNearestTimeNodeFromOutputsInternal(node);
        if (isConnectedToTimeNode) {
            isConnectedToTimeNode->refreshFrameRange();
        }
    }
} // refreshParentContainerRange

void
NodeAnimPrivate::computeReaderRange()
{

    NodeGuiPtr nodeUI = nodeGui.lock();
    NodePtr node = nodeUI->getNode();
    if (!node) {
        return;
    }
    KnobIntBase *startingTimeKnob = dynamic_cast<KnobIntBase *>( node->getKnobByName(kReaderParamNameStartingTime).get() );
    if (!startingTimeKnob) {
        return;
    }
    KnobIntBase *firstFrameKnob = dynamic_cast<KnobIntBase *>( node->getKnobByName(kReaderParamNameFirstFrame).get() );
    if (!firstFrameKnob) {
        return;
    }
    KnobIntBase *lastFrameKnob = dynamic_cast<KnobIntBase *>( node->getKnobByName(kReaderParamNameLastFrame).get() );
    if (!lastFrameKnob) {
        return;
    }

    int startingTimeValue = startingTimeKnob->getValue();
    int firstFrameValue = firstFrameKnob->getValue();
    int lastFrameValue = lastFrameKnob->getValue();
    frameRange.min = startingTimeValue;
    frameRange.max = startingTimeValue + (lastFrameValue - firstFrameValue) + 1;

} // computeReaderRange

void
NodeAnimPrivate::computeRetimeRange()
{
    NodeGuiPtr nodeUI = nodeGui.lock();
    NodePtr node = nodeUI->getNode();
    if (!node) {
        return;
    }
    NodePtr input = node->getInput(0);
    if (!input) {
        return;
    }
    if (input) {
        RangeD inputRange = {0, 0};
        {
            GetFrameRangeResultsPtr results;
            ActionRetCodeEnum stat = input->getEffectInstance()->getFrameRange_public(&results);
            if (!isFailureRetCode(stat)) {
                results->getFrameRangeResults(&inputRange);
            }
        }

        FramesNeededMap framesFirst, framesLast;
        {
            GetFramesNeededResultsPtr results;
            ActionRetCodeEnum stat = node->getEffectInstance()->getFramesNeeded_public(TimeValue(inputRange.min), ViewIdx(0), &results);
            if (!isFailureRetCode(stat)) {
                results->getFramesNeeded(&framesFirst);
            }

            stat = node->getEffectInstance()->getFramesNeeded_public(TimeValue(inputRange.max), ViewIdx(0), &results);
            if (!isFailureRetCode(stat)) {
                results->getFramesNeeded(&framesLast);
            }
        }
        assert( !framesFirst.empty() && !framesLast.empty() );
        if ( framesFirst.empty() || framesLast.empty() ) {
            return;
        }

        {
            const FrameRangesMap& rangeFirst = framesFirst[0];
            assert( !rangeFirst.empty() );
            if ( rangeFirst.empty() ) {
                return;
            }
            FrameRangesMap::const_iterator it = rangeFirst.find( ViewIdx(0) );
            assert( it != rangeFirst.end() );
            if ( it == rangeFirst.end() ) {
                return;
            }
            const std::vector<OfxRangeD>& frames = it->second;
            assert( !frames.empty() );
            if ( frames.empty() ) {
                return;
            }
            frameRange.min = (frames.front().min);
        }
        {
            FrameRangesMap& rangeLast = framesLast[0];
            assert( !rangeLast.empty() );
            if ( rangeLast.empty() ) {
                return;
            }
            FrameRangesMap::const_iterator it = rangeLast.find( ViewIdx(0) );
            assert( it != rangeLast.end() );
            if ( it == rangeLast.end() ) {
                return;
            }
            const std::vector<OfxRangeD>& frames = it->second;
            assert( !frames.empty() );
            if ( frames.empty() ) {
                return;
            }
            frameRange.max = (frames.front().min);
        }
    }

} // computeRetimeRange

void
NodeAnimPrivate::computeTimeOffsetRange()
{
    NodeGuiPtr nodeUI = nodeGui.lock();
    NodePtr node = nodeUI->getNode();
    if (!node) {
        return;
    }

    // Retrieve nearest reader useful values
    {
        AnimationModulePtr isAnimModel = toAnimationModule(model.lock());

        NodeAnimPtr nearestReader = isAnimModel->getNearestReaderInternal(node);
        if (nearestReader) {

            // Retrieve the time offset values
            KnobIntBasePtr timeOffsetKnob = toKnobIntBase(node->getKnobByName(kReaderParamNameTimeOffset));
            assert(timeOffsetKnob);
            int timeOffsetValue = timeOffsetKnob->getValue();

            frameRange = nearestReader->getFrameRange();
            frameRange.min += timeOffsetValue;
            frameRange.max += timeOffsetValue;
        }
    }

} // computeTimeOffsetRange

void
NodeAnimPrivate::computeCommonNodeRange()
{

    NodeGuiPtr nodeUI = nodeGui.lock();
    NodePtr node = nodeUI->getNode();
    if (!node) {
        return;
    }
    int first,last;
    bool lifetimeEnabled = node->getEffectInstance()->isLifetimeActivated(&first, &last);

    if (lifetimeEnabled) {
        frameRange.min = first;
        frameRange.max = last;
    }

}

void
NodeAnimPrivate::computeFRRange()
{

    NodeGuiPtr nodeUI = nodeGui.lock();
    NodePtr node = nodeUI->getNode();
    if (!node) {
        return;
    }

    KnobIntBasePtr frameRangeKnob = toKnobIntBase(node->getKnobByName(kFrameRangeParamNameFrameRange));
    assert(frameRangeKnob);

    frameRange.min = frameRangeKnob->getValue(DimIdx(0));
    frameRange.max = frameRangeKnob->getValue(DimIdx(1));
}

void
NodeAnimPrivate::computeGroupRange()
{

    NodeGuiPtr nodeUI = nodeGui.lock();
    NodePtr node = nodeUI->getNode();
    if (!node) {
        return;
    }

    AnimationModulePtr isAnimModel = toAnimationModule(model.lock());
    if (!isAnimModel) {
        return;
    }
    NodeGroupPtr nodegroup = node->isEffectNodeGroup();
    assert(nodegroup);
    if (!nodegroup) {
        return;
    }


    AnimationModuleTreeView* treeView = isAnimModel->getEditor()->getTreeView();

    NodesList nodes = nodegroup->getNodes();

    std::set<double> times;

    for (NodesList::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        NodeAnimPtr childAnim = isAnimModel->findNodeAnim(*it);

        if (!childAnim) {
            continue;
        }

        if (!treeView->isItemVisibleRecursive(childAnim->getTreeItem())) {
            continue;
        }

        childAnim->refreshFrameRange();
        RangeD childRange = childAnim->getFrameRange();
        times.insert(childRange.min);
        times.insert(childRange.max);

        // Also check the child knobs keyframes
        NodeGuiPtr childGui = childAnim->getNodeGui();
        const KnobsVec &knobs = childGui->getNode()->getKnobs();

        for (KnobsVec::const_iterator it2 = knobs.begin(); it2 != knobs.end(); ++it2) {

            if ( !(*it2)->isAnimationEnabled() || !(*it2)->hasAnimation() ) {
                continue;
            } else {
                // For each dimension and for each split view get the first/last keyframe (if any)
                int nDims = (*it2)->getNDimensions();
                std::list<ViewIdx> views = (*it2)->getViewsList();
                for (std::list<ViewIdx>::const_iterator it3 = views.begin(); it3 != views.end(); ++it3) {
                    for (int i = 0; i < nDims; ++i) {
                        CurvePtr curve = (*it2)->getAnimationCurve(*it3, DimIdx(i));
                        if (!curve) {
                            continue;
                        }
                        int nKeys = curve->getKeyFramesCount();
                        if (nKeys > 0) {
                            KeyFrame k;
                            if (curve->getKeyFrameWithIndex(0, &k)) {
                                times.insert( k.getTime() );
                            }
                            if (curve->getKeyFrameWithIndex(nKeys - 1, &k)) {
                                times.insert( k.getTime() );
                            }
                        }
                    }
                }
            }
        } // for all knobs
    } // for all children nodes

    if (times.size() <= 1) {
        frameRange.min = 0;
        frameRange.max = 0;
    } else {
        frameRange.min = *times.begin();
        frameRange.max = *times.rbegin();
    }

} // computeGroupRange


NATRON_NAMESPACE_EXIT
NATRON_NAMESPACE_USING
#include "moc_NodeAnim.cpp"
