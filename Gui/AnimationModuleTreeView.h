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

#ifndef Natron_Gui_AnimationModuleTreeView_H
#define Natron_Gui_AnimationModuleTreeView_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <set>
#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#endif

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)

#include <QtCore/QtGlobal>
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QStyledItemDelegate>
#else
#include <QtGui/QTreeWidget>
#include <QtGui/QStyledItemDelegate>
#endif

CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Gui/GuiFwd.h"


NATRON_NAMESPACE_ENTER


#define NATRON_ANIMATION_TREE_VIEW_NODE_SEPARATOR_PX 4

#define ANIMATION_MODULE_TREE_VIEW_COL_LABEL 0
#define ANIMATION_MODULE_TREE_VIEW_COL_VISIBLE 1
#define ANIMATION_MODULE_TREE_VIEW_COL_PLUGIN_ICON 2

class AnimationModuleTreeViewPrivate;

/**
 * @brief This class is a part of the tree view.
 *
 * The tree view provides a custom selection of its items.
 * For example, if the user clicks on an tree node (and not a leaf), all its
 * children must be selected too. If he clicks on a leaf, its parents must be
 * selected only if all their children are selected too.
 *
 * Deal with QTreeWidgetItem::setSelected() is just painful so an instance of
 * this class is set as the selection model of the AnimationModuleTreeView class.
 *
 */
class AnimationModuleTreeViewSelectionModel
    : public QItemSelectionModel
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:


    explicit AnimationModuleTreeViewSelectionModel(const AnimationModulePtr& animModule,
                                                   AnimationModuleTreeView* view,
                                                   QAbstractItemModel *model,
                                                   QObject *parent = 0);
    virtual ~AnimationModuleTreeViewSelectionModel();


    /**
     * @brief Select the given selection, this is essentially the same as the base class if
     * recurse is false. On the other hand if recurse is true we also select children
     * and add parents to the selection if needed.
     **/
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
     * @brief Selects parents of 'index' and put them in 'selection'.
     */
    void checkParentsSelectedStates(QTreeWidgetItem* item, QItemSelectionModel::SelectionFlags flags, std::set<QTreeWidgetItem*>* selection) const;

private:

    AnimationModuleWPtr _model;
    AnimationModuleTreeView* _view;
};


/**
 * @brief The AnimationModuleTreeView class represents the tree along side the Curve Editor and
 * the DopeSheet.
 */
class AnimationModuleTreeView
    : public QTreeWidget
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:
    explicit AnimationModuleTreeView(const AnimationModulePtr& model,
                                     Gui *gui,
                                     QWidget *parent = 0);

    virtual ~AnimationModuleTreeView();

    AnimationModulePtr getModel() const;


    /**
     * @brief Returns true if 'item' is fully visible.
     *
     * If one of its parents is collapsed, returns false.
     */
    static bool isItemVisibleRecursive(QTreeWidgetItem *item);

    /**
     * @brief Returns the height occupied in the view by 'item' and its
     * children.
     */
    int getHeightForItemAndChildren(QTreeWidgetItem *item) const;


    /**
     * @brief Returns the last visible (not hidden and not collapsed
     * in its parent) child of 'item".
     */
    QTreeWidgetItem * lastVisibleChild(QTreeWidgetItem *item) const;
    QTreeWidgetItem* getTreeItemForModelIndex(const QModelIndex& index) const;

    QModelIndex indexFromItemPublic(QTreeWidgetItem* item) 
    {
        return indexFromItem(item);
    }

    QTreeWidgetItem* getTreeBottomItem() const;

    int getTreeBottomYWidgetCoords() const;

    static void setItemIcon(const QString& iconFilePath, QTreeWidgetItem* item);

    std::vector<CurveGuiPtr> getSelectedCurves() const;

protected:
    virtual void drawRow(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const OVERRIDE FINAL;
    void drawBranch(QPainter *painter, const QRect &rect, const NodeAnimPtr& closestEnclosingNodeItem, const QColor& nodeColor, QTreeWidgetItem* item) const;

    /**
     * @brief Returns true if all childrens of 'item' are hidden,
     * otherwise returns false.
     */
    bool childrenAreHidden(QTreeWidgetItem *item) const;

    /**
     * @brief A convenience function that allow the caller to get the parent
     * item of 'item', no matter if 'item' is a top level or a child item.
     */
    QTreeWidgetItem * getParentItem(QTreeWidgetItem *item) const;

    /**
     * @brief Removes 'child' from its parent and append it to the list of
     * children of 'newParent'.
     */
    void reparentItem(QTreeWidgetItem *child, QTreeWidgetItem *newParent);

Q_SIGNALS:

    void itemRightClicked(QPoint pos, QTreeWidgetItem* item);

private Q_SLOTS:

    void onNodeAdded(const NodeAnimPtr& NodeAnim);
    
    void onNodeAboutToBeRemoved(const NodeAnimPtr& NodeAnim);

    void onItemDoubleClicked(QTreeWidgetItem *item, int column);

    void onTreeSelectionModelSelectionChanged();

    void onSelectionModelKeyframeSelectionChanged(bool recurse);

private:

    virtual void keyPressEvent(QKeyEvent* e) OVERRIDE FINAL;
    virtual void mouseReleaseEvent(QMouseEvent* e) OVERRIDE FINAL;

    boost::scoped_ptr<AnimationModuleTreeViewPrivate> _imp;
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
class AnimationModuleTreeViewItemDelegate
    : public QStyledItemDelegate
{
    AnimationModuleTreeView* _treeView;
public:
    explicit AnimationModuleTreeViewItemDelegate(AnimationModuleTreeView *treeView);

    virtual QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const OVERRIDE FINAL;
    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const OVERRIDE FINAL;
};

NATRON_NAMESPACE_EXIT

#endif // Natron_Gui_AnimationModuleTreeView_H
