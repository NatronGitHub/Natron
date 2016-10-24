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
#include "TableItemAnim.h"

#include <map>

#include <QTreeWidgetItem>

#include "Engine/KnobItemsTable.h"

#include "Gui/AnimationModule.h"
#include "Gui/KnobItemsTableGui.h"

NATRON_NAMESPACE_ENTER;

#include <map>
#include <QTreeWidgetItem>

#include "Engine/AppInstance.h"
#include "Engine/KnobItemsTable.h"
#include "Engine/Project.h"

#include "Gui/AnimationModule.h"
#include "Gui/KnobItemsTableGui.h"

typedef std::map<ViewIdx, QTreeWidgetItem*> PerViewItemMap;

class TableItemAnimPrivate
{
public:

    TableItemAnimPrivate()
    : tableItem()
    , table()
    , parentNode()
    , nameItem(0)
    , animationItems()
    , knobs()
    , children()
    {

    }

    KnobTableItemWPtr tableItem;
    KnobItemsTableGuiWPtr table;
    NodeAnimWPtr parentNode;
    QTreeWidgetItem* nameItem;
    PerViewItemMap animationItems;
    std::vector<KnobAnimPtr> knobs;
    std::vector<TableItemAnimPtr> children;
};


TableItemAnim::TableItemAnim(const AnimationModulePtr& model,
                             const KnobItemsTableGuiPtr& table,
                             const NodeAnimPtr &parentNode,
                             const KnobTableItemPtr& item,
                             QTreeWidgetItem* parentItem)
: AnimItemBase(model)
, _imp(new TableItemAnimPrivate())
{
    _imp->table = table;
    _imp->parentNode = parentNode;
    _imp->tableItem = item;

    QString itemLabel = QString::fromUtf8( item->getLabel().c_str() );

    _imp->nameItem = new QTreeWidgetItem;
    _imp->nameItem->setText(0, itemLabel);
    _imp->nameItem->setData(0, QT_ROLE_CONTEXT_TYPE, eAnimatedItemTypeTableItemRoot);
    assert(parentItem);
    parentItem->addChild(_imp->nameItem);

    if (item->getCanAnimateUserKeyframes()) {
        std::list<ViewIdx> views = item->getViewsList();
        const std::vector<std::string>& projectViews = item->getApp()->getProject()->getProjectViewNames();
        for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
            QString viewName;
            if (*it >= 0 && *it < (int)projectViews.size()) {
                viewName = QString::fromUtf8(projectViews[*it].c_str());
            }
            QTreeWidgetItem* animationItem = new QTreeWidgetItem;
            QString itemLabel;
            if (views.size() > 1) {
                itemLabel += viewName + QString::fromUtf8(" ");
                itemLabel += tr("Animation");
            }
            animationItem->setText(0, itemLabel);
            animationItem->setData(0, QT_ROLE_CONTEXT_TYPE, eAnimatedItemTypeTableItemAnimation);
            _imp->animationItems[*it] = animationItem;
            _imp->nameItem->addChild(animationItem);
        }

    }

    std::vector<KnobTableItemPtr> children = item->getChildren();
    for (std::size_t i = 0; i < children.size(); ++i) {
        TableItemAnimPtr child(TableItemAnim::create(model, table, parentNode, children[i], _imp->nameItem));
        _imp->children.push_back(child);
    }
}

TableItemAnim::~TableItemAnim()
{
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

QTreeWidgetItem *
TableItemAnim::getTreeItem(DimIdx /*dimension*/, ViewIdx view) const
{
    PerViewItemMap::const_iterator foundView = _imp->animationItems.find(view);
    if (foundView == _imp->animationItems.end()) {
        return 0;
    }
    return foundView->second;
}

CurvePtr
TableItemAnim::getCurve(DimIdx dimension, ViewIdx view) const
{
    KnobTableItemPtr item = getInternalItem();
    if (!item) {
        return CurvePtr();
    }
    return item->getAnimationCurve(view, dimension);
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

int
TableItemAnim::getNDimensions() const
{
    return 1;
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

bool
TableItemAnim::isRangeDrawingEnabled() const
{
    KnobTableItemPtr item = getInternalItem();
    if (!item) {
        return false;
    }
#pragma message WARN("ToDo")
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
    for (std::vector<TableItemAnimPtr>::iterator it = _imp->children.begin(); it!=_imp->children.end(); ++it) {
        if ((*it)->getInternalItem() == item) {
            TableItemAnimPtr ret = *it;
            _imp->children.erase(it);
            return ret;
        } else {
            TableItemAnimPtr foundChild = (*it)->removeItem(item);
            if (foundChild) {
                return foundChild;
            }
        }
    }
    return TableItemAnimPtr();
}

bool
TableItemAnim::getTreeItemViewDimension(QTreeWidgetItem* item, DimSpec* dimension, ViewSetSpec* view, AnimatedItemTypeEnum* type) const
{
    if (item == _imp->nameItem) {
        *dimension = DimSpec::all();
        *view = ViewSetSpec::all();
        *type = eAnimatedItemTypeTableItemRoot;
        return true;
    }

    for (PerViewItemMap::const_iterator it = _imp->animationItems.begin(); it != _imp->animationItems.end(); ++it) {
        if (it->second == item) {
            *view = ViewSetSpec(it->first);
            *dimension = DimSpec(0);
            *type = eAnimatedItemTypeTableItemAnimation;
            return true;
        }
    }
    return false;
}

void
TableItemAnim::initialize()
{
    initializeKnobsAnim();
}

void
TableItemAnim::initializeKnobsAnim()
{
    KnobTableItemPtr item = _imp->tableItem.lock();
    std::vector<KnobGuiPtr> knobs = _imp->table.lock()->getKnobsForItem(item);
    TableItemAnimPtr thisShared = toTableItemAnim(shared_from_this());


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

        KnobAnimPtr knobAnimObject(KnobAnim::create(getModel(), thisShared, _imp->nameItem.get(), knobGui));
        _imp->knobs.push_back(knobAnimObject);

    } // for all knobs
} // initializeKnobsAnim

void
TableItemAnim::refreshKnobsVisibility()
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

void
TableItemAnim::refreshVisibility()
{

}

NATRON_NAMESPACE_EXIT;
NATRON_NAMESPACE_USING;
#include "moc_TableItemAnim.cpp"