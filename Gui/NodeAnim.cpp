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

#include "NodeAnim.h"

#include <QTreeWidgetItem>

#include "Engine/KnobTypes.h"
#include "Engine/KnobItemsTable.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"

#include "Gui/AnimationModule.h"
#include "Gui/AnimationModuleSelectionModel.h"
#include "Gui/KnobAnim.h"
#include "Gui/KnobItemsTableGui.h"
#include "Gui/KnobGui.h"
#include "Gui/NodeGui.h"
#include "Gui/TableItemAnim.h"

NATRON_NAMESPACE_ENTER;

class NodeAnimPrivate
{
public:
    NodeAnimPrivate()
    : model()
    , nodeType()
    , nodeGui()
    , nameItem(0)
    , knobs()
    , topLevelTableItems()
    {

    }


    /* attributes */
    AnimationModuleWPtr model;
    AnimatedItemTypeEnum nodeType;
    NodeGuiWPtr nodeGui;
    boost::scoped_ptr<QTreeWidgetItem> nameItem;
    std::vector<KnobAnimPtr> knobs;
    std::vector<TableItemAnimPtr> topLevelTableItems;
};


NodeAnim::NodeAnim(const AnimationModulePtr &model,
                   const NodeGuiPtr &nodeGui)
: _imp(new NodeAnimPrivate)
{
    _imp->model = model;
    _imp->nodeGui = nodeGui;


}

NodeAnim::~NodeAnim()
{
    // Kill sub items first before killing the top level item
    _imp->knobs.clear();
    _imp->topLevelTableItems.clear();
    _imp->nameItem.reset();
}

void
NodeAnim::initialize(AnimatedItemTypeEnum itemType)
{
    _imp->nodeType = itemType;
    NodePtr internalNode = getNodeGui()->getNode();

    NodeAnimPtr thisShared = shared_from_this();
    _imp->nameItem.reset(new QTreeWidgetItem);
    _imp->nameItem->setData(0, QT_ROLE_CONTEXT_ITEM_POINTER, qVariantFromValue((void*)thisShared.get()));
    _imp->nameItem->setText( 0, QString::fromUtf8( internalNode->getLabel().c_str() ) );
    _imp->nameItem->setData(0, QT_ROLE_CONTEXT_TYPE, itemType);
    _imp->nameItem->setData(0, QT_ROLE_CONTEXT_IS_ANIMATED, true);

    connect( internalNode.get(), SIGNAL(labelChanged(QString)), this, SLOT(onNodeLabelChanged(QString)) );



    initializeKnobsAnim();

    initializeTableItems();
}

static void initializeKnobsAnimInternal(const AnimationModulePtr& model,
                                        const KnobsHolderAnimBasePtr& holder,
                                        QTreeWidgetItem* parent,
                                        const std::vector<KnobGuiPtr> &knobs,
                                        std::vector<KnobAnimPtr>& itemsKnobMap)
{
    for (std::vector<KnobGuiPtr>::const_iterator it = knobs.begin();
         it != knobs.end(); ++it) {

        const KnobGuiPtr &knobGui = *it;
        KnobIPtr knob = knobGui->getKnob();
        if (!knob) {
            continue;
        }

        // If the knob is not animted, don't create an item
        if ( !knob->canAnimate() || !knob->isAnimationEnabled() ) {
            continue;
        }
        assert(knob->getNDimensions() >= 1);

        KnobAnimPtr knobAnimObject(KnobAnim::create(model, holder, parent, knobGui));
        itemsKnobMap.push_back(knobAnimObject);

    } // for all knobs

} // initializeKnobsAnimInternal

void
NodeAnim::initializeKnobsAnim()
{
    // Create dope sheet knobs
    const std::list<std::pair<KnobIWPtr, KnobGuiPtr> > &knobs = _imp->nodeGui.lock()->getKnobs();
    std::vector<KnobGuiPtr> knobsVec;
    for (std::list<std::pair<KnobIWPtr, KnobGuiPtr> >::const_iterator it = knobs.begin(); it!=knobs.end(); ++it) {
        knobsVec.push_back(it->second);
    }

    NodeAnimPtr thisShared = shared_from_this();

    for (std::vector<KnobGuiPtr>::const_iterator it = knobsVec.begin();
         it != knobsVec.end(); ++it) {

        const KnobGuiPtr &knobGui = *it;
        KnobIPtr knob = knobGui->getKnob();
        if (!knob) {
            continue;
        }

        // If the knob is not animted, don't create an item
        if ( !knob->canAnimate() || !knob->isAnimationEnabled() ) {
            continue;
        }
        assert(knob->getNDimensions() >= 1);

        KnobAnimPtr knobAnimObject(KnobAnim::create(getModel(), thisShared, _imp->nameItem.get(), knobGui));
        _imp->knobs.push_back(knobAnimObject);
        
    } // for all knobs

} // initializeKnobsAnim

void
NodeAnim::initializeTableItems()
{
    NodeGuiPtr nodeGui = _imp->nodeGui.lock();
    KnobItemsTableGuiPtr table = nodeGui->getKnobItemsTable();


    KnobItemsTablePtr internalTable = table->getInternalTable();


    connect(internalTable.get(), SIGNAL(itemRemoved(KnobTableItemPtr,TableChangeReasonEnum)), this, SLOT(onTableItemRemoved(KnobTableItemPtr,TableChangeReasonEnum)));
    connect(internalTable.get(), SIGNAL(itemInserted(int,KnobTableItemPtr,TableChangeReasonEnum)), this, SLOT(onTableItemInserted(int,KnobTableItemPtr,TableChangeReasonEnum)));

    NodeAnimPtr thisShared = shared_from_this();

    std::vector<KnobTableItemPtr> allItems = internalTable->getTopLevelItems();
    for (std::size_t i = 0; i < allItems.size(); ++i) {
        TableItemAnimPtr anim(TableItemAnim::create(getModel(), table, thisShared, allItems[i], _imp->nameItem.get()));
        _imp->topLevelTableItems.push_back(anim);
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
NodeAnim::onTableItemRemoved(const KnobTableItemPtr& item, TableChangeReasonEnum)
{

    TableItemAnimPtr found;
    for (std::vector<TableItemAnimPtr>::iterator it = _imp->topLevelTableItems.begin(); it!=_imp->topLevelTableItems.end(); ++it) {
        if ((*it)->getInternalItem() == item) {
            found = *it;
            _imp->topLevelTableItems.erase(it);
            break;
        } else {
            TableItemAnimPtr found = (*it)->removeItem(item);
            if (found) {
                break;
            }
        }
    }
    if (found) {
        getModel()->getSelectionModel()->removeAnyReferenceFromSelection(found);
    }
}

void
NodeAnim::onTableItemInserted(int index, const KnobTableItemPtr& item, TableChangeReasonEnum)
{
    KnobTableItemPtr parentItem = item->getParent();
    TableItemAnimPtr parentAnim;
    if (parentItem) {
        parentAnim = findTableItem(parentItem);
    }
    KnobItemsTableGuiPtr table = getNodeGui()->getKnobItemsTable();
    TableItemAnimPtr anim(TableItemAnim::create(getModel(), table, shared_from_this(), item, _imp->nameItem.get()));
    if (parentItem) {
        parentAnim->insertChild(index, anim);
    } else {
        if (index < 0 || index >= (int)_imp->topLevelTableItems.size()) {
            _imp->topLevelTableItems.push_back(anim);
        } else {
            std::vector<TableItemAnimPtr>::iterator it = _imp->topLevelTableItems.begin();
            std::advance(it, index);
            _imp->topLevelTableItems.insert(it, anim);
        }
    }

}


void
NodeAnim::onNodeLabelChanged(const QString &name)
{
    _imp->nameItem->setText(0, name);
}

AnimationModulePtr
NodeAnim::getModel() const
{
    return _imp->model.lock();
}

QTreeWidgetItem *
NodeAnim::getTreeItem() const
{
    return _imp->nameItem.get();
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
    return node->isLifetimeActivated(&first, &last);
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
    NodeGuiPtr nodeGui = getNodeGui();
    AnimatedItemTypeEnum nodeType = getItemType();
    QTreeWidgetItem *nodeItem = getTreeItem();
    bool showNode = false;

    if (nodeGui->isSettingsPanelVisible() ) {
        switch (nodeType) {
            case eAnimatedItemTypeCommon:
                showNode = AnimationModule::isNodeAnimated(nodeGui);
                break;

                // For range nodes always show them
            case eAnimatedItemTypeFrameRange:
            case eAnimatedItemTypeReader:
            case eAnimatedItemTypeRetime:
            case eAnimatedItemTypeTimeOffset:
#pragma message WARN("For curve editor don't do this")
                showNode = true;
                break;
            case eAnimatedItemTypeGroup:
            {
                // Check if there's at list one animated node in the group
                if (AnimationModule::isNodeAnimated(nodeGui)) {
                    showNode = true;
                }
                if (!showNode) {
                    NodeGroupPtr isGroup = toNodeGroup(nodeGui->getNode()->getEffectInstance());
                    assert(isGroup);
                    NodesList nodes = isGroup->getNodes();
                    for (NodesList::const_iterator it = nodes.begin(); it!=nodes.end(); ++it) {
                        if ((*it)->getEffectInstance()->getHasAnimation()) {
                            showNode = true;
                            break;
                        }
                    }
                }
            }   break;
            default:
                break;
        }
    }

    refreshKnobsVisibility();

    nodeItem->setData(0, QT_ROLE_CONTEXT_IS_ANIMATED, showNode);

    nodeItem->setHidden(!showNode);
} // refreshVisibility


void
NodeAnim::refreshKnobsVisibility()
{
    const std::vector<KnobAnimPtr>& knobRows = getKnobs();

    for (std::vector<KnobAnimPtr>::const_iterator it = knobRows.begin(); it != knobRows.end(); ++it) {
        QTreeWidgetItem *knobItem = (*it)->getRootItem();

        // Expand if it's a multidim root item
        if (knobItem->childCount() > 0) {
            knobItem->setExpanded(true);
        }
        (*it)->refreshVisibilityConditional(false /*refreshHolder*/);
    }
}


NATRON_NAMESPACE_EXIT;
NATRON_NAMESPACE_USING;
#include "moc_NodeAnim.cpp"
