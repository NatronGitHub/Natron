/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2021 The Natron developers
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

#ifndef ROTOPANEL_H
#define ROTOPANEL_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/weak_ptr.hpp>
#endif

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QWidget>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/GlobalDefines.h"

#include "Engine/ViewIdx.h"

#include "Gui/GuiFwd.h"


NATRON_NAMESPACE_ENTER

struct RotoPanelPrivate;

class RotoPanel
    : public QWidget
{
    Q_OBJECT

public:


    RotoPanel(const NodeGuiPtr& n,
              QWidget* parent = 0);

    virtual ~RotoPanel();

    void onTreeOutOfFocusEvent();

    RotoItemPtr getRotoItemForTreeItem(QTreeWidgetItem* treeItem) const;
    QTreeWidgetItem* getTreeItemForRotoItem(const RotoItemPtr & item) const;
    std::string getNodeName() const;
    RotoContextPtr getContext() const;

    void clearSelection();

    void clearAndSelectPreviousItem(const RotoItemPtr & item);

    void pushUndoCommand(QUndoCommand* cmd);

    void showItemMenu(QTreeWidgetItem* item, const QPoint & globalPos);

    void setLastRightClickedItem(QTreeWidgetItem* item);

    void makeCustomWidgetsForItem(const RotoDrawableItemPtr& item,
                                  QTreeWidgetItem* treeItem = NULL);

    NodeGuiPtr getNode() const;

public Q_SLOTS:

    void onGoToPrevKeyframeButtonClicked();

    void onGoToNextKeyframeButtonClicked();

    void onAddKeyframeButtonClicked();

    void onRemoveKeyframeButtonClicked();

    void onRemoveAnimationButtonClicked();

    void onAddLayerButtonClicked();

    void onRemoveItemButtonClicked();

    ///This gets called when the selection changes internally in the RotoContext
    void onSelectionChanged(int reason);

    void onSelectedBezierKeyframeSet(double time);

    void onSelectedBezierKeyframeRemoved(double time);

    void onSelectedBezierAnimationRemoved();

    void onSelectedBezierAboutToClone();

    void onSelectedBezierCloned();

    void onTimeChanged(SequenceTime time, int reason);

    ///A new item has been created internally
    void onItemInserted(int index, int reason);

    ///An item was removed by the user
    void onItemRemoved(const RotoItemPtr& item, int reason);

    ///An item had its inverted state changed
    void onRotoItemInvertedStateChanged();

    ///An item had its shape color changed
    void onRotoItemShapeColorChanged();

    ///An item had its compositing operator changed
    void onRotoItemCompOperatorChanged(ViewSpec /*view*/, int dim, int reason);

    void onCurrentItemCompOperatorChanged(int index);

    ///The user changed the current item in the tree
    void onCurrentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous);

    ///An item content was changed
    void onItemChanged(QTreeWidgetItem* item, int column);

    ///An item was clicked
    void onItemClicked(QTreeWidgetItem* item, int column);

    ///An item was double clicked
    void onItemDoubleClicked(QTreeWidgetItem* item, int column);

    ///Connected to QApplication::focusChanged to deal with an issue of the QTreeWidget
    void onFocusChanged(QWidget* old, QWidget*);

    ///This gets called when the selection in the tree has changed
    void onItemSelectionChanged();

    void onAddLayerActionTriggered();

    void onDeleteItemActionTriggered();

    void onCutItemActionTriggered();

    void onCopyItemActionTriggered();

    void onPasteItemActionTriggered();

    void onDuplicateItemActionTriggered();

    void updateItemGui(QTreeWidgetItem* item);

    void selectAll();

    void onSettingsPanelClosed(bool closed);

    void onItemColorDialogEdited(const QColor & color);

    void onItemGloballyActivatedChanged(const RotoItemPtr& item);
    void onItemLabelChanged(const RotoItemPtr& item);
    void onItemScriptNameChanged(const RotoItemPtr& item);

    void onItemLockChanged(int reason);

    void onOperatorColMinimumSizeChanged(const QSize& size);

private:

    ///Called when the selection changes from another reason than settings panel interaction
    void onSelectionChangedInternal();

    boost::scoped_ptr<RotoPanelPrivate> _imp;
};


class DroppedTreeItem
{
public:
    RotoLayerPtr newParentLayer;
    int insertIndex;
    QTreeWidgetItem* newParentItem;
    QTreeWidgetItem* dropped;
    RotoItemPtr droppedRotoItem;

    DroppedTreeItem()
        : newParentLayer()
        , insertIndex(-1)
        , newParentItem(0)
        , dropped(0)
        , droppedRotoItem()
    {
    }
};

typedef boost::shared_ptr<DroppedTreeItem> DroppedTreeItemPtr;

NATRON_NAMESPACE_EXIT

#endif // ROTOPANEL_H
