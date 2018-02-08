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
#include "TableItemAnim.h"

#include <map>

#include <QTreeWidgetItem>

#include "Engine/KnobItemsTable.h"

#include "Gui/AnimationModule.h"
#include "Gui/KnobItemsTableGui.h"


#include <map>
#include <QTreeWidgetItem>

#include "Engine/AppInstance.h"
#include "Engine/KnobItemsTable.h"
#include "Engine/Project.h"


#include "Gui/AnimationModule.h"
#include "Gui/AnimationModuleSelectionModel.h"
#include "Gui/AnimationModuleView.h"
#include "Gui/AnimationModuleEditor.h"
#include "Gui/AnimationModuleTreeView.h"
#include "Gui/CurveGui.h"
#include "Gui/KnobGui.h"
#include "Gui/KnobAnim.h"
#include "Gui/NodeAnim.h"
#include "Gui/KnobItemsTableGui.h"

NATRON_NAMESPACE_ENTER

struct ItemCurve
{
    QTreeWidgetItem* item;
    //CurveGuiPtr curve;
};
typedef std::map<ViewIdx, ItemCurve> PerViewItemMap;

class TableItemAnimPrivate
{
public:

    TableItemAnimPrivate(const AnimationModuleBasePtr& model)
    : model(model)
    , tableItem()
    , table()
    , parentNode()
    , nameItem(0)
    , knobs()
    , children()
    {

    }

    AnimationModuleBaseWPtr model;
    KnobTableItemWPtr tableItem;
    KnobItemsTableGuiWPtr table;
    NodeAnimWPtr parentNode;
    QTreeWidgetItem* nameItem;
    std::vector<KnobAnimPtr> knobs;
    std::vector<TableItemAnimPtr> children;


};


TableItemAnim::TableItemAnim(const AnimationModuleBasePtr& model,
                             const KnobItemsTableGuiPtr& table,
                             const NodeAnimPtr &parentNode,
                             const KnobTableItemPtr& item)
:  _imp(new TableItemAnimPrivate(model))
{
    _imp->table = table;
    _imp->parentNode = parentNode;
    _imp->tableItem = item;

    connect(item.get(), SIGNAL(labelChanged(QString,TableChangeReasonEnum)), this, SLOT(onInternalItemLabelChanged(QString,TableChangeReasonEnum)));
    connect(item.get(), SIGNAL(availableViewsChanged()), this, SLOT(onInternalItemAvailableViewsChanged()));

    connect(item->getApp()->getProject().get(), SIGNAL(projectViewsChanged()), this, SLOT(onProjectViewsChanged()));
}


// make_shared enabler (because make_shared needs access to the private constructor)
// see https://stackoverflow.com/a/20961251/2607517
struct TableItemAnim::MakeSharedEnabler: public TableItemAnim
{
    MakeSharedEnabler(const AnimationModuleBasePtr& model,
                      const KnobItemsTableGuiPtr& table,
                      const NodeAnimPtr &parentNode,
                      const KnobTableItemPtr& item) : TableItemAnim(model, table, parentNode, item) {
    }
};


TableItemAnimPtr
TableItemAnim::create(const AnimationModuleBasePtr& model,
                        const KnobItemsTableGuiPtr& table,
                        const NodeAnimPtr &parentNode,
                        const KnobTableItemPtr& item,
                        QTreeWidgetItem* parentItem)
{
    TableItemAnimPtr ret = boost::make_shared<TableItemAnim::MakeSharedEnabler>(model, table, parentNode, item);
    ret->initialize(parentItem);
    return ret;
}


TableItemAnim::~TableItemAnim()
{
    destroyItems();
}

void
TableItemAnim::destroyItems()
{

    for (std::size_t i = 0; i < _imp->knobs.size(); ++i) {
        _imp->knobs[i]->destroyItems();
    }
    for (std::size_t i = 0; i < _imp->children.size(); ++i) {
        _imp->children[i]->destroyItems();
    }
    _imp->knobs.clear();
    _imp->children.clear();
    
    delete _imp->nameItem;
    _imp->nameItem = 0;
    

}


void
TableItemAnim::insertChild(int index, const TableItemAnimPtr& child)
{
    if (index < 0 || index >= (int)_imp->children.size()) {
        _imp->children.push_back(child);
    } else {
        std::vector<TableItemAnimPtr>::iterator it = _imp->children.begin();
        std::advance(it, index);
        _imp->children.insert(it, child);
    }

}


QTreeWidgetItem*
TableItemAnim::getRootItem() const
{
    return _imp->nameItem;
}

std::list<ViewIdx>
TableItemAnim::getViewsList() const
{
    KnobTableItemPtr item = getInternalItem();
    if (!item) {
        return std::list<ViewIdx>();
    }
    return item->getViewsList();
}


AnimationModuleBasePtr
TableItemAnim::getModel() const
{
    return _imp->model.lock();
}

KnobTableItemPtr
TableItemAnim::getInternalItem() const
{
    return _imp->tableItem.lock();
}


NodeAnimPtr
TableItemAnim::getNode() const
{
    return _imp->parentNode.lock();
}

const std::vector<TableItemAnimPtr>&
TableItemAnim::getChildren() const
{
    return _imp->children;
}

const std::vector<KnobAnimPtr>&
TableItemAnim::getKnobs() const
{
    return _imp->knobs;
}

void
TableItemAnim::onInternalItemLabelChanged(const QString& label, TableChangeReasonEnum)
{
    if (_imp->nameItem) {
        _imp->nameItem->setText(0, label);
    }
}

bool
TableItemAnim::isRangeDrawingEnabled() const
{
    KnobTableItemPtr item = getInternalItem();
    if (!item) {
        return false;
    }
#pragma message WARN("ToDo")
    return false;
}

RangeD
TableItemAnim::getFrameRange() const
{
#pragma message WARN("ToDo")
    RangeD ret = {0,0};
    return ret;
}

TableItemAnimPtr
TableItemAnim::findTableItem(const KnobTableItemPtr& item) const
{
    for (std::size_t i = 0; i < _imp->children.size(); ++i) {
        if (_imp->children[i]->getInternalItem() == item) {
            return _imp->children[i];
        } else {
            TableItemAnimPtr foundChild = _imp->children[i]->findTableItem(item);
            if (foundChild) {
                return foundChild;
            }
        }
    }
    return TableItemAnimPtr();
}

TableItemAnimPtr
TableItemAnim::removeItem(const KnobTableItemPtr& item)
{
    TableItemAnimPtr ret;
    for (std::vector<TableItemAnimPtr>::iterator it = _imp->children.begin(); it!=_imp->children.end(); ++it) {
        if ((*it)->getInternalItem() == item) {
            ret = *it;
            _imp->children.erase(it);
            break;
        } else {
            ret = (*it)->removeItem(item);
            if (ret) {
                break;
            }
        }
    }
    if (ret) {
        _imp->model.lock()->getSelectionModel()->removeAnyReferenceFromSelection(ret);
    }
    return ret;
}


void
TableItemAnim::initialize(QTreeWidgetItem* parentItem)
{
    KnobTableItemPtr item = getInternalItem();
    QString itemLabel = QString::fromUtf8( item->getLabel().c_str() );

    TableItemAnimPtr thisShared = shared_from_this();
    AnimationModuleBasePtr model = getModel();

    _imp->nameItem = new QTreeWidgetItem;
    _imp->nameItem->setData(0, QT_ROLE_CONTEXT_ITEM_POINTER, qVariantFromValue((void*)thisShared.get()));
    _imp->nameItem->setText(0, itemLabel);
    _imp->nameItem->setData(0, QT_ROLE_CONTEXT_TYPE, eAnimatedItemTypeTableItemRoot);
    _imp->nameItem->setExpanded(true);
    int nCols = getModel()->getTreeColumnsCount();
    if (nCols > ANIMATION_MODULE_TREE_VIEW_COL_VISIBLE) {
        _imp->nameItem->setData(ANIMATION_MODULE_TREE_VIEW_COL_VISIBLE, QT_ROLE_CONTEXT_ITEM_VISIBLE, QVariant(true));
        _imp->nameItem->setIcon(ANIMATION_MODULE_TREE_VIEW_COL_VISIBLE, model->getItemVisibilityIcon(true));
    }
    if (nCols > ANIMATION_MODULE_TREE_VIEW_COL_PLUGIN_ICON) {
        QString iconFilePath = QString::fromUtf8(item->getIconLabelFilePath().c_str());
        AnimationModuleTreeView::setItemIcon(iconFilePath, _imp->nameItem);
    }
    assert(parentItem);
    parentItem->addChild(_imp->nameItem);

    KnobItemsTableGuiPtr table = _imp->table.lock();
    NodeAnimPtr parentNode = _imp->parentNode.lock();
    std::vector<KnobTableItemPtr> children = item->getChildren();
    for (std::size_t i = 0; i < children.size(); ++i) {
        TableItemAnimPtr child(TableItemAnim::create(model, table, parentNode, children[i], _imp->nameItem));
        _imp->children.push_back(child);
    }

    initializeKnobsAnim();
}



void
TableItemAnim::initializeKnobsAnim()
{
    KnobTableItemPtr item = _imp->tableItem.lock();
    const KnobsVec& knobs = item->getKnobs();
    TableItemAnimPtr thisShared = toTableItemAnim(shared_from_this());
    AnimationModuleBasePtr model = getModel();

    for (KnobsVec::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {


        // If the knob is not animted, don't create an item
        if ( !(*it)->canAnimate() || !(*it)->isAnimationEnabled() ) {
            continue;
        }
        KnobAnimPtr knobAnimObject(KnobAnim::create(model, thisShared, _imp->nameItem, *it));
        _imp->knobs.push_back(knobAnimObject);

    } // for all knobs
} // initializeKnobsAnim


void
TableItemAnim::refreshVisibilityConditional(bool refreshHolder)
{
    NodeAnimPtr parentNode = _imp->parentNode.lock();
    if (refreshHolder && parentNode) {
        // Refresh parent which will refresh this item
        parentNode->refreshVisibility();
        return;
    }


    AnimationModulePtr animModule = toAnimationModule(getModel());
    bool onlyShowIfAnimated = false;
    if (animModule) {
        onlyShowIfAnimated = animModule->getEditor()->isOnlyAnimatedItemsVisibleButtonChecked();
    }

    // Refresh knobs
    bool showItem = false;
    const std::vector<KnobAnimPtr>& knobRows = getKnobs();
    for (std::vector<KnobAnimPtr>::const_iterator it = knobRows.begin(); it != knobRows.end(); ++it) {
        (*it)->refreshVisibilityConditional(false /*refreshHolder*/);
        if (!(*it)->getRootItem()->isHidden()) {
            showItem = true;
        }
    }


    // Refresh children
    for (std::vector<TableItemAnimPtr>::const_iterator it = _imp->children.begin(); it!=_imp->children.end(); ++it) {
        (*it)->refreshVisibilityConditional(false);
        if (!(*it)->getRootItem()->isHidden()) {
            showItem = true;
        }
    }

    _imp->nameItem->setData(0, QT_ROLE_CONTEXT_IS_ANIMATED, showItem);
    _imp->nameItem->setHidden(!showItem);
}

void
TableItemAnim::refreshVisibility()
{
    refreshVisibilityConditional(true);
}

void
TableItemAnim::onProjectViewsChanged()
{

}

void
TableItemAnim::onInternalItemAvailableViewsChanged()
{

    refreshVisibility();

    getModel()->refreshSelectionBboxAndUpdateView();
}


NATRON_NAMESPACE_EXIT
NATRON_NAMESPACE_USING
#include "moc_TableItemAnim.cpp"
