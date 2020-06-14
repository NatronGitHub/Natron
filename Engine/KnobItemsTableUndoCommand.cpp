/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2020 The Natron developers
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

#include "KnobItemsTableUndoCommand.h"

#include "Engine/Node.h"
#include "Engine/AppInstance.h"
#include "Engine/KnobItemsTable.h"


NATRON_NAMESPACE_ENTER

AddItemsCommand::AddItemsCommand(const std::list<ItemToAdd>& items)
: UndoCommand()
, _isFirstRedo(true)
, _items(items)
{
    assert(!_items.empty());
    setText( tr("Add Item(s)").toStdString() );
}

AddItemsCommand::AddItemsCommand(const KnobTableItemPtr &item, const KnobTableItemPtr &parentItem, int indexInParent)
    : UndoCommand()
    , _isFirstRedo(true)
{

    ItemToAdd p;
    p.item = item;
    p.parentItem = parentItem;
    p.indexInParent = indexInParent;
    _items.push_back(p);
    setText( tr("Add Item").toStdString() );
}

AddItemsCommand::AddItemsCommand(const KnobTableItemPtr &item)
: UndoCommand()
, _isFirstRedo(true)
{

    ItemToAdd p;
    p.item = item;
    p.parentItem = item->getParent();
    p.indexInParent = item->getIndexInParent();
    _items.push_back(p);
    setText( tr("Add Item").toStdString() );

}

void
AddItemsCommand::undo()
{
    KnobItemsTablePtr model = _items.front().item->getModel();

    model->beginEditSelection();
    for (std::list<ItemToAdd>::const_iterator it = _items.begin(); it != _items.end(); ++it) {
        model->removeItem(it->item, eTableChangeReasonInternal);
    }
    model->endEditSelection(eTableChangeReasonInternal);
    model->getNode()->getApp()->triggerAutoSave();
}

void
AddItemsCommand::redo()
{

    KnobItemsTablePtr model = _items.front().item->getModel();

    model->beginEditSelection();
    model->clearSelection(eTableChangeReasonInternal);


    for (std::list<ItemToAdd>::const_iterator it = _items.begin(); it != _items.end(); ++it) {
        bool skipInsert = false;
        if (_isFirstRedo) {
            // The item may already be at the correct position in the parent, check it
            if (!it->parentItem) {
                if (model->getTopLevelItem(it->indexInParent) == it->item) {
                    skipInsert = true;
                }
            } else {
                if (it->parentItem->getChild(it->indexInParent) == it->item) {
                    skipInsert = true;
                }
            }
        }
        if (!skipInsert) {
            model->insertItem(it->indexInParent, it->item, it->parentItem, eTableChangeReasonInternal);
        }
        model->addToSelection(it->item, eTableChangeReasonInternal);
    }

    model->endEditSelection(eTableChangeReasonInternal);
    model->getNode()->getApp()->triggerAutoSave();
    _isFirstRedo = false;
}

RemoveItemsCommand::RemoveItemsCommand(const std::list<KnobTableItemPtr> &items)
    : UndoCommand()
    , _items()
{
    assert( !items.empty() );
    for (std::list<KnobTableItemPtr>::const_iterator it = items.begin(); it != items.end(); ++it) {
        ItemToRemove t;
        t.item = *it;
        KnobItemsTablePtr model = (*it)->getModel();
        t.prevItem = model->getTopLevelItem((*it)->getIndexInParent() - 1);
        _items.push_back(t);
    }
    setText( tr("Remove Item(s)").toStdString() );
}

void
RemoveItemsCommand::undo()
{
    KnobItemsTablePtr model = _items.begin()->item->getModel();
    model->beginEditSelection();
    model->clearSelection(eTableChangeReasonInternal);
    for (std::list<ItemToRemove>::const_iterator it = _items.begin(); it != _items.end(); ++it) {
        int prevIndex = -1;
        KnobTableItemPtr prevItem = it->prevItem.lock();
        if (prevItem) {
            prevIndex = prevItem->getIndexInParent();
        }
        if (prevIndex != -1) {
            model->insertItem(prevIndex, it->item, it->item->getParent(), eTableChangeReasonInternal);
        } else {
            model->addItem(it->item, it->item->getParent(), eTableChangeReasonInternal);
        }
        model->addToSelection(it->item, eTableChangeReasonInternal);
    }
    model->endEditSelection(eTableChangeReasonInternal);
    model->getNode()->getApp()->triggerAutoSave();
}

void
RemoveItemsCommand::redo()
{
    KnobItemsTablePtr model = _items.begin()->item->getModel();

    KnobTableItemPtr nextItem;
    {
        KnobTableItemPtr item = _items.back().item;
        assert(item);
        int index = item->getIndexInParent();
        std::vector<KnobTableItemPtr> topLevel = model->getTopLevelItems();
        if (!topLevel.empty()) {
            if (index + 1 < (int)topLevel.size()) {
                nextItem = topLevel[index + 1];
            } else {
                if (topLevel[0] != item) {
                    nextItem = topLevel[0];
                }
            }
        }
    }

    model->beginEditSelection();
    model->clearSelection(eTableChangeReasonInternal);
    for (std::list<ItemToRemove>::const_iterator it = _items.begin(); it != _items.end(); ++it) {
        model->removeItem(it->item, eTableChangeReasonInternal);
    }
    if (nextItem) {
        model->addToSelection(nextItem, eTableChangeReasonInternal);
    }
    model->endEditSelection(eTableChangeReasonInternal);
    model->getNode()->getApp()->triggerAutoSave();
}

NATRON_NAMESPACE_EXIT
