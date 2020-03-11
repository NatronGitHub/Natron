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


#ifndef NATRON_GUI_KNOBITEMSTABLEGUI_H
#define NATRON_GUI_KNOBITEMSTABLEGUI_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****


#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#endif

#include <QObject>
#include <QSize>
#include <QGridLayout>

#include "Gui/GuiFwd.h"
#include "Global/GlobalDefines.h"

#include "Gui/KnobGuiContainerI.h"

NATRON_NAMESPACE_ENTER


struct KnobItemsTableGuiPrivate;
class KnobItemsTableGui : public QObject, public KnobGuiContainerI
{

    GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
    GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    KnobItemsTableGui(const KnobItemsTablePtr& table,
                      DockablePanel* panel,
                      QWidget* parent);

    virtual ~KnobItemsTableGui();

    ////// Overriden from KnobGuiContainerI
    virtual Gui* getGui() const OVERRIDE FINAL;
    virtual const QUndoCommand* getLastUndoCommand() const OVERRIDE FINAL;
    virtual void pushUndoCommand(QUndoCommand* cmd) OVERRIDE FINAL;
    virtual KnobGuiPtr getKnobGui(const KnobIPtr& knob) const OVERRIDE FINAL;
    virtual int getItemsSpacingOnSameLine() const OVERRIDE FINAL;
    virtual NodeGuiPtr getNodeGui() const OVERRIDE FINAL;
    //////

    std::vector<KnobGuiPtr> getKnobsForItem(const KnobTableItemPtr& item) const;

    void addWidgetsToLayout(QLayout* layout);

    QLayout* getContainerLayout() const;

    KnobItemsTablePtr getInternalTable() const;

    virtual QWidget* createKnobHorizontalFieldContainer(QWidget* parent) const OVERRIDE FINAL;

public Q_SLOTS:


    void onDeleteItemsActionTriggered();
    void onCopyItemsActionTriggered();
    void onPasteItemsActionTriggered();
    void onCutItemsActionTriggered();
    void onDuplicateItemsActionTriggered();

    void onSelectAllItemsActionTriggered();

    void onTableItemDataChanged(const TableItemPtr& item, int col, int role);
    void onModelSelectionChanged(const QItemSelection& selected,const QItemSelection& deselected);
    
    void onModelSelectionChanged(const std::list<KnobTableItemPtr>& addedToSelection, const std::list<KnobTableItemPtr>& removedFromSelection, TableChangeReasonEnum reason);
    void onModelItemRemoved(const KnobTableItemPtr& item, TableChangeReasonEnum reason);
    void onModelItemInserted(int index, const KnobTableItemPtr& item, TableChangeReasonEnum reason);

    void onItemLabelChanged(const QString& label, TableChangeReasonEnum reason);
    void onItemIconChanged(TableChangeReasonEnum reason);

    void onViewItemRightClicked(const QPoint& globalPos, const TableItemPtr& item);

    void onRightClickActionTriggered();

    void onItemAnimationCurveChanged(std::list<double> added, std::list<double> removed, ViewIdx view);

private:

    friend class KnobItemsTableView;

    boost::scoped_ptr<KnobItemsTableGuiPrivate> _imp;
};

NATRON_NAMESPACE_EXIT

#endif // NATRON_GUI_KNOBITEMSTABLEGUI_H
