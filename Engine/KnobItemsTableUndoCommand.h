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

#ifndef NATRON_ENGINE_KNOBITEMSTABLEUNDOCOMMAND_H
#define NATRON_ENGINE_KNOBITEMSTABLEUNDOCOMMAND_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <list>
#include <map>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#endif

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QCoreApplication>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/UndoCommand.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

class AddItemsCommand
    : public UndoCommand
{
    Q_DECLARE_TR_FUNCTIONS(AddItemCommand)

public:

    //Hold shared_ptrs otherwise no one is holding a shared_ptr while items are removed from the context
    struct ItemToAdd
    {
        int indexInParent;
        KnobTableItemPtr item;
        KnobTableItemPtr parentItem;
    };

    /**
     * @brief Add multiple items at once
     **/
    AddItemsCommand(const std::list<ItemToAdd>& items);

    /**
     * Add a single item to the given parent at the given index
     **/
    AddItemsCommand(const KnobTableItemPtr &item, const KnobTableItemPtr& parent, int indexInParent);

    /**
     * Add a single item. The item is assumed to be already in its parent 
     **/
    AddItemsCommand(const KnobTableItemPtr &item);

    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;

private:

    bool _isFirstRedo;


    std::list<ItemToAdd> _items;
};


class RemoveItemsCommand
    : public UndoCommand
{
    Q_DECLARE_TR_FUNCTIONS(RemoveTracksCommand)

public:

    //Hold shared_ptrs otherwise no one is holding a shared_ptr while items are removed from the context
    struct ItemToRemove
    {
        KnobTableItemPtr item;
        KnobTableItemWPtr prevItem;
    };

    RemoveItemsCommand(const std::list<KnobTableItemPtr> &items);

    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;

private:



    std::list<ItemToRemove> _items;
};

NATRON_NAMESPACE_EXIT


#endif // NATRON_ENGINE_KNOBITEMSTABLEUNDOCOMMAND_H
