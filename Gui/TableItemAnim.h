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

#ifndef NATRON_GUI_TABLEITEMANIM_H
#define NATRON_GUI_TABLEITEMANIM_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Gui/GuiFwd.h"

CLANG_DIAG_OFF(deprecated)
#include <QtCore/QCoreApplication>
CLANG_DIAG_ON(deprecated)

#include "Gui/AnimItemBase.h"

NATRON_NAMESPACE_ENTER

class TableItemAnimPrivate;
class TableItemAnim : public QObject, public KnobsHolderAnimBase, public boost::enable_shared_from_this<TableItemAnim>
{

    GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
    GCC_DIAG_SUGGEST_OVERRIDE_ON

    struct MakeSharedEnabler;

    TableItemAnim(const AnimationModuleBasePtr& model,
                  const KnobItemsTableGuiPtr& table,
                  const NodeAnimPtr &parentNode,
                  const KnobTableItemPtr& item);

public:
    static TableItemAnimPtr create(const AnimationModuleBasePtr& model,
                                   const KnobItemsTableGuiPtr& table,
                                   const NodeAnimPtr &parentNode,
                                   const KnobTableItemPtr& item,
                                   QTreeWidgetItem* parentItem);

    virtual ~TableItemAnim();

    AnimationModuleBasePtr getModel() const;

    KnobTableItemPtr getInternalItem() const;

    NodeAnimPtr getNode() const;

    const std::vector<TableItemAnimPtr>& getChildren() const;

    virtual const std::vector<KnobAnimPtr>& getKnobs() const OVERRIDE FINAL WARN_UNUSED_RETURN;

    bool isRangeDrawingEnabled() const;

    //// Overriden from AnimItemBase
    QTreeWidgetItem * getRootItem() const WARN_UNUSED_RETURN;
    std::list<ViewIdx> getViewsList() const WARN_UNUSED_RETURN;
    ////

    void refreshVisibilityConditional(bool refreshHolder);

    virtual void refreshVisibility() OVERRIDE FINAL;

    RangeD getFrameRange() const;

    void destroyItems();

public Q_SLOTS:

    void onInternalItemLabelChanged(const QString& label, TableChangeReasonEnum);

    void onInternalItemAvailableViewsChanged();

    void onProjectViewsChanged();

private:

    void insertChild(int index, const TableItemAnimPtr& child);

    void initialize(QTreeWidgetItem* parentItem);

    TableItemAnimPtr findTableItem(const KnobTableItemPtr& item) const;

    TableItemAnimPtr removeItem(const KnobTableItemPtr& item);

    friend class NodeAnim;
    friend class NodeAnimPrivate;

    void initializeKnobsAnim();

    boost::scoped_ptr<TableItemAnimPrivate> _imp;
};

inline TableItemAnimPtr toTableItemAnim(const KnobsHolderAnimBasePtr& p)
{
    return boost::dynamic_pointer_cast<TableItemAnim>(p);
}

inline TableItemAnimPtr toTableItemAnim(const AnimItemBasePtr& p)
{
    return boost::dynamic_pointer_cast<TableItemAnim>(p);
}

NATRON_NAMESPACE_EXIT

#endif // NATRON_GUI_TABLEITEMANIM_H
