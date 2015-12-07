/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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

#ifndef DOPESHEETHIERARCHYVIEW_H
#define DOPESHEETHIERARCHYVIEW_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#endif

#include "Global/GLIncludes.h" //!<must be included before QGlWidget because of gl.h and glew.h

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtGui/QTreeWidget>
#include <QtGui/QStyledItemDelegate>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Gui/GuiFwd.h"


class HierarchyViewPrivate;

/**
 * @brief This class is a part of the hierarchy view.
 *
 * The hierarchy view provides a custom selection of its items.
 * For example, if the user clicks on an tree node (and not a leaf), all its
 * children must be selected too. If he clicks on a leaf, its parents must be
 * selected only if all their children are selected too.
 *
 * Deal with QTreeWidgetItem::setSelected() is just painful so an instance of
 * this class is set as the selection model of the HierarchyView class.
 *
 */
class HierarchyViewSelectionModel : public QItemSelectionModel
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:
    
    
    explicit HierarchyViewSelectionModel(DopeSheet* dopesheetModel,
                                         HierarchyView* view,
                                         QAbstractItemModel *model,
                                         QObject *parent = 0);
    ~HierarchyViewSelectionModel();

    void selectWithRecursion(const QItemSelection &userSelection,
                QItemSelectionModel::SelectionFlags command,
                bool recurse);
    
public Q_SLOTS:
    
    virtual void select(const QItemSelection &userSelection,
                        QItemSelectionModel::SelectionFlags command) OVERRIDE FINAL;

private: /* functions */
    
    void selectInternal(const QItemSelection &userSelection,
                QItemSelectionModel::SelectionFlags command,
                bool recurse);

    
    /**
     * @brief Selects recursively all children of 'index' and put them in
     * 'selection'.
     */
    void selectChildren(const QModelIndex &index, QItemSelection *selection) const;

    /**
     * @brief Selects parents of 'index' and put them in 'selection'.
     */
    void checkParentsSelectedStates(const QModelIndex &index, QItemSelectionModel::SelectionFlags flags,
                                   const QItemSelection &unitedSelection, QItemSelection *finalSelection) const;
    
private:
    
    DopeSheet* _dopesheetModel;
    HierarchyView* _view;
};


/**
 * @brief The HierarchyView class describes the hierarchy view of the dope
 * sheet editor.
 *
 * The hierarchy view displays the name of each node/knob referenced by the
 * dope sheet editor and handles their organization.
 *
 * /!\ We should use a custom model and not the built-in model of QTreeWidget,
 * for more control.
 *
 * /!\ Use a QSortFilterProxyModel should be considered to hide the items
 * instead of deal with QTreeWidgetItem::setHidden().
 *
 * The other features of the hierarchy view are :
 * - select the associated keyframes by select an item.
 * - put a settings panel on top by double clicking on an item.
 */
class HierarchyView : public QTreeWidget
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:
    explicit HierarchyView(DopeSheet *dopeSheetModel, Gui *gui, QWidget *parent = 0);
    ~HierarchyView();

    /**
     * @brief Returns a pointer to the DSKnob associated with the item at
     * the coordinates (0, y) in the tree widget's viewport.
     */
    boost::shared_ptr<DSKnob> getDSKnobAt(int y) const;

    /**
     * @brief Returns true if 'item' is fully visible.
     *
     * If one of its parents is collapsed, returns false.
     */
    bool itemIsVisibleFromOutside(QTreeWidgetItem *item) const;

    /**
     * @brief Returns the height occuped in the view by 'item' and its
     * children.
     */
    int getHeightForItemAndChildren(QTreeWidgetItem *item) const;

    /**
     * @brief Returns the last visible (not hidden and not collapsed
     * in its parent) child of 'item".
     */
    QTreeWidgetItem *lastVisibleChild(QTreeWidgetItem *item) const;
    
    QTreeWidgetItem* getTreeItemForModelIndex(const QModelIndex& index) const;

    void setCanResizeOtherWidget(bool canResize);
    
protected:
    virtual void drawRow(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const OVERRIDE FINAL;
    virtual void drawBranches(QPainter *painter, const QRect &rect, const QModelIndex &index) const OVERRIDE FINAL;

    /**
     * @brief Returns true if all childrens of 'item' are hidden,
     * otherwise returns false.
     */
    bool childrenAreHidden(QTreeWidgetItem *item) const;

    /**
     * @brief A conveniance function that allow the caller to get the parent
     * item of 'item', no matter if 'item' is a top level or a child item.
     */
    QTreeWidgetItem *getParentItem(QTreeWidgetItem *item) const;

    /**
     * @brief Removes 'child' from its parent and append it to the list of
     * children of 'newParent'.
     */
    void moveItem(QTreeWidgetItem *child, QTreeWidgetItem *newParent) const;

private Q_SLOTS:
    /**
     * @brief Inserts the item associated with 'dsNode' in the hierarchy view.
     *
     * This slot is automatically called after the dope sheet model
     * created 'dsNode'.
     */
    void onNodeAdded(DSNode *dsNode);

    /**
     * @brief Removes the item associated with 'dsNode' from the hierarchy
     * view.
     *
     * This slot is automatically called just before the dope sheet model
     * remove 'dsNode'.
     */
    void onNodeAboutToBeRemoved(DSNode *dsNode);

    /**
     * @brief Checks if the item associated with 'dsKnob' must be shown
     * or hidden. Checks also the visible state of the item associated
     * with its node.
     *
     * If the knob has no keyframes then the item is hidden.
     *
     * This slot is automatically called when a keyframe associated with
     * 'dsKnob' is set or removed.
     */
    void onKeyframeSetOrRemoved(DSKnob *dsKnob);

    /**
     * @brief Check the selected state of the knob context items which have
     * selected keyframes. If all keyframes of the dimension are selected,
     * then the item is selected too.
     *
     * This slot is automatically called when a keyframe selection is changed
     * (keyframe added/removed, selection moved) in the dope sheet model.
     */
    void onKeyframeSelectionChanged(bool recurse);

    /**
     * @brief Puts the settings panel associated with 'item' on top of the
     * others.
     *
     * This slot is automatically called when an item is double cliccked.
     */
    void onItemDoubleClicked(QTreeWidgetItem *item, int column);

    /**
     * @brief Selects all keyframes associated with the current selected
     * items.
     *
     * This slot is automatically called when an item selection is performed
     * by the user.
     */
    void onSelectionChanged();

private:
    
    virtual void resizeEvent(QResizeEvent* e) OVERRIDE FINAL;
    
    boost::scoped_ptr<HierarchyViewPrivate> _imp;
};


/**
 * @brief This class is a part of the hierarchy view.
 *
 * The hierarchy view content is drawn in a very custom way.
 *
 * This delegate just draw the text of an item with a white color if it's
 * selected, otherwise a dark color is used.
 *
 * It also sets the size of each item : the height of an item associated with a
 * knob or a node is unchanged, instead of the height of a range-based node.
*/
class HierarchyViewItemDelegate : public QStyledItemDelegate
{
public:
    explicit HierarchyViewItemDelegate(QObject *parent = 0);

    virtual QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const OVERRIDE FINAL;
    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const OVERRIDE FINAL;
};

#endif // DOPESHEETHIERARCHYVIEW_H
