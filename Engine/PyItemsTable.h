/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
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

#ifndef ENGINE_PYITEMSTABLE_H
#define ENGINE_PYITEMSTABLE_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <list>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#endif

#include "Engine/PyParameter.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER;
NATRON_PYTHON_NAMESPACE_ENTER;

class ItemBase
{
    Q_DECLARE_TR_FUNCTIONS(ItemsTable)

    KnobTableItemWPtr _item;

public:

    ItemBase(const KnobTableItemPtr& item);

    virtual ~ItemBase();

    void setLabel(const QString & name);
    QString getLabel() const;

    void setIconFilePath(const QString& icon);
    QString getIconFilePath() const;

    QString getScriptName() const;

    ItemBase* getParent() const;

    int getIndexInParent() const;

    std::list<ItemBase*> getChildren() const;

    std::list<Param*> getParams() const;

    Param* getParam(const QString& name) const;


    KnobTableItemPtr getInternalItem() const
    {
        return _item.lock();
    }

protected:

    bool getViewSetSpecFromViewName(const QString& viewName, ViewSetSpec* view) const;
    bool getViewIdxFromViewName(const QString& viewName, ViewIdx* view) const;

};

class ItemsTable
{
    Q_DECLARE_TR_FUNCTIONS(ItemsTable)

    friend class ItemBase;

    KnobItemsTableWPtr _table;

public:
    
    ItemsTable(const KnobItemsTablePtr& table);

    virtual ~ItemsTable();

    KnobItemsTablePtr getInternalModel() const
    {
        return _table.lock();
    }

    ItemBase* getItemByFullyQualifiedScriptName(const QString& name) const;

    std::list<ItemBase*> getTopLevelItems() const;

    std::list<ItemBase*> getSelectedItems() const;

    void insertItem(int index, const ItemBase* item, const ItemBase* parent);

    void removeItem(const ItemBase* item);

    QString getAttributeName() const;

    QString getTableName() const;

    bool isModelParentingEnabled() const;

    static ItemBase* createPyItemWrapper(const KnobTableItemPtr& item);

};

NATRON_PYTHON_NAMESPACE_EXIT;
NATRON_NAMESPACE_EXIT;

#endif // ENGINE_PYITEMSTABLE_H
