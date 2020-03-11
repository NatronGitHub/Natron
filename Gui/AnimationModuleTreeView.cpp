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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "AnimationModuleTreeView.h"

#include <stdexcept>

#include <QHeaderView>
#include <QPainter>
#include <QResizeEvent>
#include <QPixmapCache>
#include <QStyleOption>
#include <QApplication>
#include <QStyle>

#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/Image.h"
#include "Engine/KnobItemsTable.h"
#include "Engine/Settings.h"
#include "Engine/ViewIdx.h"

#include "Gui/DockablePanel.h"
#include "Gui/AnimationModule.h"
#include "Gui/AnimationModuleEditor.h"
#include "Gui/AnimationModuleSelectionModel.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiDefines.h"
#include "Gui/GuiMacros.h"
#include "Gui/KnobAnim.h"
#include "Gui/KnobGui.h"
#include "Gui/NodeGui.h"
#include "Gui/NodeAnim.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/NodeSettingsPanel.h"
#include "Gui/TableItemAnim.h"

NATRON_NAMESPACE_ENTER

////////////////////////// AnimationModuleTreeViewSelectionModel //////////////////////////

AnimationModuleTreeViewSelectionModel::AnimationModuleTreeViewSelectionModel(const AnimationModulePtr& animModule,
                                                                             AnimationModuleTreeView* view,
                                                                             QAbstractItemModel *model,
                                                                             QObject *parent)
: QItemSelectionModel(model, parent)
, _model(animModule)
, _view(view)
{
    connect( model, SIGNAL(destroyed()), this, SLOT(deleteLater()) );
}

AnimationModuleTreeViewSelectionModel::~AnimationModuleTreeViewSelectionModel()
{}


static void
addItemToItemsSet(QTreeWidgetItem* item, bool recurse, std::set<QTreeWidgetItem*>* outSet)
{
    outSet->insert(item);
    if (recurse) {
        int nChildren = item->childCount();
        for (int i = 0; i < nChildren; ++i) {
            QTreeWidgetItem* child = item->child(i);
            if (child) {
                addItemToItemsSet(child, recurse, outSet);
            }
        }
    }
}


static void
itemSelectionToTreeItemsSet(AnimationModuleTreeView* treeView, const QItemSelection &userSelection, std::set<QTreeWidgetItem*>* outList)
{
    QModelIndexList userSelectedIndexes = userSelection.indexes();
    for (QModelIndexList::const_iterator it = userSelectedIndexes.begin(); it != userSelectedIndexes.end(); ++it) {
        if (it->column() > 0) {
            continue;
        }
        QTreeWidgetItem* treeItem = treeView->getTreeItemForModelIndex(*it);
        assert(treeItem);
        if (treeItem) {
            outList->insert(treeItem);
        }
    }
}


static void
addItemToSelection(AnimationModuleTreeView* treeView, QTreeWidgetItem* item, QItemSelection *selection)
{
    if (!item) {
        return;
    }
    int cc = item->columnCount();
    QModelIndex indexCol0 = treeView->indexFromItemPublic(item);
    QModelIndex indexColLast = treeView->model()->index(indexCol0.row(), cc - 1, indexCol0.parent());
    selection->select(indexCol0, indexColLast);
}


static void
treeItemsSetToItemSelection(AnimationModuleTreeView* treeView, const std::set<QTreeWidgetItem*>& itemSet, QItemSelection* selection)
{
    for (std::set<QTreeWidgetItem*>::const_iterator it = itemSet.begin(); it != itemSet.end(); ++it) {
        addItemToSelection(treeView, *it, selection);
    }
}


void
AnimationModuleTreeViewSelectionModel::checkParentsSelectedStates(QTreeWidgetItem* item, QItemSelectionModel::SelectionFlags flags, std::set<QTreeWidgetItem*>* selection) const
{
    assert(item);
    // Recursively fills the list of parents until we hit a parent node
    std::list<QTreeWidgetItem*> parentItems;
    {
        QTreeWidgetItem* parentItem = item->parent();
        while (parentItem) {
            parentItems.push_back(parentItem);
            {
                AnimatedItemTypeEnum type;
                KnobAnimPtr isKnob;
                TableItemAnimPtr isTableItem;
                NodeAnimPtr isNodeItem;
                ViewSetSpec view;
                DimSpec dim;
                bool found = _model.lock()->findItem(parentItem, &type, &isKnob, &isTableItem, &isNodeItem, &view, &dim);
                (void)found;
                if (isNodeItem) {
                    // We recurse until we found the immediate parent node.
                    // We don't recurse on parent group nodes
                    break;
                }
            }
            parentItem = parentItem->parent();

        }
    }

    // For each parent item, If all children are selected, select the item
    for (std::list<QTreeWidgetItem*>::const_iterator it = parentItems.begin(); it != parentItems.end(); ++it) {
        bool selectParent = true;
        {
            int nChildren = (*it)->childCount();
            for (int i = 0; i < nChildren; ++i) {
                QTreeWidgetItem* child = (*it)->child(i);
                if (!child) {
                    continue;
                }

                if (child->isHidden()) {
                    continue;
                }

                // If one of the children is deselected, don't select the parent
                bool isChildSelected = selection->find(child) != selection->end();
                if (!isChildSelected) {
                    selectParent = false;
                    break;
                }
            }
        }
        if ( ((flags & QItemSelectionModel::Select) && selectParent) ) {
            selection->insert(*it);
        } else if ((flags & QItemSelectionModel::Deselect) && !selectParent) {
            std::set<QTreeWidgetItem*>::iterator found = selection->find(*it);
            if (found != selection->end()) {
                selection->erase(found);
            }
        }
    }
} // AnimationModuleTreeViewSelectionModel::checkParentsSelectedStates



void
AnimationModuleTreeViewSelectionModel::selectInternal(const QItemSelection &userSelection, QItemSelectionModel::SelectionFlags command, bool recurse)
{



    // Add currently selected items to selection, unless command has clear flag
    std::set<QTreeWidgetItem*> currentlySelectedItems;
    itemSelectionToTreeItemsSet(_view, selection(), &currentlySelectedItems);
    if (command & QItemSelectionModel::Clear) {
        currentlySelectedItems.clear();
    }

    // Add new items in selection and their children if we need to recurse
    {
        std::set<QTreeWidgetItem*> itemsToAddSet;
        itemSelectionToTreeItemsSet(_view, userSelection, &itemsToAddSet);

        for (std::set<QTreeWidgetItem*>::const_iterator it = itemsToAddSet.begin(); it != itemsToAddSet.end(); ++it) {
            addItemToItemsSet(*it, recurse, &currentlySelectedItems);
        }
    }

    // If we need to recurse, also check if parent items need to be selected

    if (recurse) {
        for (std::set<QTreeWidgetItem*> ::const_iterator it = currentlySelectedItems.begin(); it != currentlySelectedItems.end(); ++it) {

            // If there's a parent for the item and the item itself is not a node,
            // check if we should add the parent to the selection
            AnimatedItemTypeEnum type;
            KnobAnimPtr isKnob;
            TableItemAnimPtr isTableItem;
            NodeAnimPtr isNodeItem;
            ViewSetSpec view;
            DimSpec dim;
            bool found = _model.lock()->findItem(*it, &type, &isKnob, &isTableItem, &isNodeItem, &view, &dim);
            if ( found && !isNodeItem && (*it)->parent() ) {
                checkParentsSelectedStates(*it, command, &currentlySelectedItems);
            }
        }

    }

    QItemSelection finalSelection;
    treeItemsSetToItemSelection(_view, currentlySelectedItems, &finalSelection);

    QItemSelectionModel::select(finalSelection, command);
    _view->update();

} // selectInternal

void
AnimationModuleTreeViewSelectionModel::selectWithRecursion(const QItemSelection &userSelection, QItemSelectionModel::SelectionFlags command, bool recurse)
{
    selectInternal(userSelection, command, recurse);
}

void
AnimationModuleTreeViewSelectionModel::select(const QItemSelection &userSelection, QItemSelectionModel::SelectionFlags command)
{
    selectInternal(userSelection, command, true /*recurse*/);
}


AnimationModuleTreeViewItemDelegate::AnimationModuleTreeViewItemDelegate(AnimationModuleTreeView* treeView)
    : QStyledItemDelegate(treeView)
    ,  _treeView(treeView)
{}


QSize
AnimationModuleTreeViewItemDelegate::sizeHint(const QStyleOptionViewItem &option,
                                    const QModelIndex &index) const
{
    Q_UNUSED(option);

    QSize itemSize = QStyledItemDelegate::sizeHint(option, index);
    AnimatedItemTypeEnum nodeType = AnimatedItemTypeEnum( index.data(QT_ROLE_CONTEXT_TYPE).toInt() );
    int heightOffset = 0;

    switch (nodeType) {
    case eAnimatedItemTypeReader:
    case eAnimatedItemTypeRetime:
    case eAnimatedItemTypeTimeOffset:
    case eAnimatedItemTypeFrameRange:
    case eAnimatedItemTypeGroup:
        heightOffset = TO_DPIY(10);
        break;
    default:
        break;
    }

    itemSize.rheight() += heightOffset;

    return itemSize;
}

void
AnimationModuleTreeViewItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,const QModelIndex &index) const
{


    if (index.column() == ANIMATION_MODULE_TREE_VIEW_COL_LABEL) {
        painter->save();

        double r, g, b;

        // Selection color is hard-coded : Todo make this a setting
        if (option.state & QStyle::State_Selected) {
            appPTR->getCurrentSettings()->getTextColor(&r, &g, &b);
        } else {
            r = g = b = 0.11f;
        }
        QColor c;
        c.setRgbF(Image::clamp(r, 0., 1.), Image::clamp(g, 0., 1.), Image::clamp(b, 0., 1.));
        painter->setPen(c);
        QString str = index.data().toString();
        painter->drawText( option.rect, Qt::AlignVCenter,  str);
        painter->restore();
    } else {
        // Don't draw selected state by default
        QStyleOptionViewItemV4 paintOptions = option;
        paintOptions.state &= ~QStyle::State_Selected;

        if (index.column() == ANIMATION_MODULE_TREE_VIEW_COL_PLUGIN_ICON) {

            paintOptions.decorationAlignment = Qt::AlignLeft | Qt::AlignVCenter;

            // Draw a background color
            double r,g,b;
            appPTR->getCurrentSettings()->getBaseColor(&r, &g, &b);
            QColor c;
            c.setRgbF(Image::clamp(r, 0., 1.), Image::clamp(g, 0., 1.), Image::clamp(b, 0., 1.));

            QStyleOptionViewItemV4 newOpt = paintOptions;

            newOpt.features |= QStyleOptionViewItemV2::HasDecoration;

            newOpt.decorationSize = option.rect.size();
            QRect iconBgRect = _treeView->style()->subElementRect(QStyle::SE_ItemViewItemDecoration, &newOpt);

            painter->fillRect(iconBgRect, QBrush(c));
        } else if (index.column() == ANIMATION_MODULE_TREE_VIEW_COL_VISIBLE) {
            // Draw a small transparant bg color in case the row color does not contrast with the icon
            QColor c(70,70,70);
            painter->save();
            painter->setClipRect(option.rect);
            painter->setOpacity(0.2);

            QStyleOptionViewItemV4 newOpt = option;
            newOpt.features |= QStyleOptionViewItemV2::HasDecoration;
            newOpt.decorationSize = option.rect.size();

            QRect iconBgRect = _treeView->style()->subElementRect(QStyle::SE_ItemViewItemDecoration, &newOpt);
            painter->fillRect(iconBgRect, QBrush(c));
            painter->setOpacity(1.);
            painter->restore();
        }


        QStyledItemDelegate::paint(painter, paintOptions, index);
    }


}

////////////////////////// AnimationModuleTreeView //////////////////////////

class AnimationModuleTreeViewPrivate
{
public:
    AnimationModuleTreeViewPrivate(AnimationModuleTreeView* publicInterface)
    : publicInterface(publicInterface)
    , model()
    , gui(0)
    , selectionRecursionCounter(0)
    {}


    /**
     * @brief Returns the rectangle portion between the tree left edge and the start of the item visual rect after the branch indicator
     **/
    QRect getBranchRect(QTreeWidgetItem *item) const;

    /**
     * @brief Returns the rectangle portion covering the branch indicator (arrow)
     **/
    QRect getArrowRect(QTreeWidgetItem *item, const QRect &branchRect) const;

    NodeAnimPtr itemBelowIsNode(QTreeWidgetItem *item) const;

    void drawParentContainerContour(QPainter *p, const NodeAnimPtr& parentNode, const QRect &itemRect);
    void drawNodeTopSeparation(QPainter *p, const QRect &itemRect) const;

    QColor desaturate(const QColor &color) const;

    void openSettingsPanelForNode(const NodeAnimPtr& node);

    /* attributes */
    AnimationModuleTreeView *publicInterface;
    AnimationModuleWPtr model;
    Gui *gui;
    int selectionRecursionCounter;
};


QRect
AnimationModuleTreeViewPrivate::getBranchRect(QTreeWidgetItem *item) const
{
    QRect itemRect = publicInterface->visualItemRect(item);

    return QRect( publicInterface->rect().left(), itemRect.top(),
                  itemRect.left(), itemRect.height() );
}

QRect
AnimationModuleTreeViewPrivate::getArrowRect(QTreeWidgetItem *item, const QRect &branchRect) const
{

    if ( !item->parent() ) {
        return branchRect;
    }

    int parentBranchRectRight = publicInterface->visualItemRect( item->parent() ).left();
    int arrowRectWidth  = branchRect.right() - parentBranchRectRight;

    return QRect( parentBranchRectRight, branchRect.top(),
                  arrowRectWidth, branchRect.height() );
}


NodeAnimPtr AnimationModuleTreeViewPrivate::itemBelowIsNode(QTreeWidgetItem *item) const
{
    NodeAnimPtr ret;
    QTreeWidgetItem *itemBelow = publicInterface->itemBelow(item);

    if (itemBelow) {
        AnimatedItemTypeEnum type;
        KnobAnimPtr isKnob;
        TableItemAnimPtr isTableItem;
        NodeAnimPtr isNodeItem;
        ViewSetSpec view;
        DimSpec dim;
        bool found = model.lock()->findItem(item, &type, &isKnob, &isTableItem, &isNodeItem, &view, &dim);
        if (found) {
            ret = isNodeItem;
        }
    }

    return ret;
}



void
AnimationModuleTreeViewPrivate::drawParentContainerContour(QPainter *p,
                                                           const NodeAnimPtr& parentNode,
                                                           const QRect &itemRect)
{
    NodeAnimPtr nodeIt = parentNode;
    while (nodeIt) {
        QColor nodeColor = nodeIt->getNodeGui()->getCurrentColor();
        QTreeWidgetItem *nodeItem = nodeIt->getTreeItem();

        QRect branchRect = getBranchRect(nodeItem);
        QRect targetRect = getArrowRect(nodeItem, branchRect);
        targetRect.setTop( itemRect.top() );
        targetRect.setBottom( itemRect.bottom() );

        p->fillRect(targetRect, nodeColor);

        QTreeWidgetItem* parentItem = nodeItem->parent();
        nodeIt.reset();
        if (parentItem) {
            AnimatedItemTypeEnum foundType;
            KnobAnimPtr isKnob;
            TableItemAnimPtr isTableItem;
            NodeAnimPtr isNodeItem;
            ViewSetSpec view;
            DimSpec dim;
            bool found = model.lock()->findItem(parentItem, &foundType, &isKnob, &isTableItem, &isNodeItem, &view, &dim);
            if (found && isNodeItem) {
                nodeIt = isNodeItem;
            }
        }
    }
}

void
AnimationModuleTreeViewPrivate::drawNodeTopSeparation(QPainter *p,  const QRect &itemRect) const
{
    int lineWidth = TO_DPIY(NATRON_ANIMATION_TREE_VIEW_NODE_SEPARATOR_PX);

    QPen pen(Qt::black);
    pen.setWidth(lineWidth);
    p->setPen(pen);
    p->drawLine(itemRect.left(), itemRect.top(), itemRect.right(), itemRect.top());
}



/**
 * @brief Returns a desaturated shade of 'color'.
 *
 * This function is used to paint the hierarchy view without too flashy colors.
 */
QColor
AnimationModuleTreeViewPrivate::desaturate(const QColor &color) const
{
    QColor ret = color;

    ret.setAlpha(87);

    return ret;
}

int
AnimationModuleTreeView::getHeightForItemAndChildren(QTreeWidgetItem *item) const
{
    assert( !item->isHidden() );

    // If the node item is collapsed
    if ( !item->isExpanded() ) {
        return visualItemRect(item).height() + 1;
    }

    // Get the "bottom-most" item
    QTreeWidgetItem *lastChild = lastVisibleChild(item);

    if ( (lastChild->childCount() > 0) && lastChild->isExpanded() ) {
        lastChild = lastVisibleChild(lastChild);
    }

    int top = visualItemRect(item).top();
    int bottom = visualItemRect(lastChild).bottom();

    return (bottom - top) + 1;
}


AnimationModuleTreeView::AnimationModuleTreeView(const AnimationModulePtr& model,
                                                 Gui *gui,
                                                 QWidget *parent)
    : QTreeWidget(parent),
    _imp( new AnimationModuleTreeViewPrivate(this) )
{
    _imp->model = model;
    _imp->gui = gui;

    header()->close();


    AnimationModuleTreeViewSelectionModel* selectionModel = new AnimationModuleTreeViewSelectionModel(model, this, this->model(), this);
    setSelectionModel(selectionModel);

    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setColumnCount(3);

    header()->setStretchLastSection(false);
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    header()->setResizeMode(ANIMATION_MODULE_TREE_VIEW_COL_LABEL, QHeaderView::Stretch);
    header()->setResizeMode(ANIMATION_MODULE_TREE_VIEW_COL_PLUGIN_ICON, QHeaderView::Fixed);
    header()->setResizeMode(ANIMATION_MODULE_TREE_VIEW_COL_VISIBLE, QHeaderView::Fixed);
#else
    header()->setSectionResizeMode(ANIMATION_MODULE_TREE_VIEW_COL_LABEL, QHeaderView::Stretch);
    header()->setSectionResizeMode(ANIMATION_MODULE_TREE_VIEW_COL_PLUGIN_ICON, QHeaderView::Fixed);
    header()->setSectionResizeMode(ANIMATION_MODULE_TREE_VIEW_COL_VISIBLE, QHeaderView::Fixed);
#endif
    header()->resizeSection(ANIMATION_MODULE_TREE_VIEW_COL_PLUGIN_ICON, TO_DPIX(NATRON_MEDIUM_BUTTON_ICON_SIZE + 4));
    header()->resizeSection(ANIMATION_MODULE_TREE_VIEW_COL_VISIBLE, TO_DPIX(NATRON_MEDIUM_BUTTON_ICON_SIZE + 4));

    setExpandsOnDoubleClick(false);

    setItemDelegate( new AnimationModuleTreeViewItemDelegate(this) );

    // Very important otherwise on macOS a bug makes the whole UI refresh
    setAttribute(Qt::WA_MacShowFocusRect, 0);

    setStyleSheet( QString::fromUtf8("AnimationModuleTreeView { border: 0px; }") );

    connect( this, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)),
             this, SLOT(onItemDoubleClicked(QTreeWidgetItem*,int)) );

    connect( selectionModel, SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
             this, SLOT(onTreeSelectionModelSelectionChanged()) );
}

AnimationModuleTreeView::~AnimationModuleTreeView()
{}

AnimationModulePtr
AnimationModuleTreeView::getModel() const
{
    return _imp->model.lock();
}


static bool isItemVisibleRecursiveInternal(QTreeWidgetItem* item, bool checkExpand)
{
    if (!item) {
        return false;
    }
    if (item->isHidden()) {
        return false;
    }
    if (checkExpand && !item->isExpanded()) {
        return false;
    }

    QTreeWidgetItem *parent = item->parent();
    if (parent) {
        return isItemVisibleRecursiveInternal(parent, true);
    }
    return true;
}

bool
AnimationModuleTreeView::isItemVisibleRecursive(QTreeWidgetItem *item) 
{
    return isItemVisibleRecursiveInternal(item, false);
}

QTreeWidgetItem *
AnimationModuleTreeView::lastVisibleChild(QTreeWidgetItem *item) const
{
    QTreeWidgetItem *ret = 0;

    if ( !item->childCount() ) {
        ret = item;
    }

    for (int i = item->childCount() - 1; i >= 0; --i) {
        QTreeWidgetItem *child = item->child(i);

        if ( !child->isHidden() ) {
            ret = child;

            break;
        }
    }

    if (!ret) {
        ret = item;
    }

    return ret;
}

static QTreeWidgetItem* getTreeBottomItemInternal(QTreeWidgetItem* item)
{
    if (item->isHidden()) {
        return 0;
    }

    int nChildren = item->childCount();
    if (nChildren == 0) {
        return item;
    }
    if (!item->isExpanded()) {
        return item;
    }
    QTreeWidgetItem* hasChildRet = 0;
    for (int i = nChildren - 1; i >= 0; --i) {
        hasChildRet = getTreeBottomItemInternal(item->child(i));
        if (hasChildRet) {
            return hasChildRet;
        }
    }

    return item;
}

QTreeWidgetItem*
AnimationModuleTreeView::getTreeBottomItem() const
{
    int nTopLevel = topLevelItemCount();
    if (nTopLevel <= 0) {
        return 0;
    }
    for (int i = nTopLevel - 1; i >= 0; --i) {
        QTreeWidgetItem* subRet = getTreeBottomItemInternal(topLevelItem(i));
        if (subRet) {
            return subRet;
        }
    }
    return 0;
}

int
AnimationModuleTreeView::getTreeBottomYWidgetCoords() const
{
    QTreeWidgetItem* item = getTreeBottomItem();
    if (!item) {
        return 0;
    }
    return visualItemRect(item).bottomLeft().y();
}

QTreeWidgetItem*
AnimationModuleTreeView::getTreeItemForModelIndex(const QModelIndex& index) const
{
    return itemFromIndex(index);
}

void
AnimationModuleTreeView::drawRow(QPainter *painter,
                                 const QStyleOptionViewItem &option,
                                 const QModelIndex &index) const
{
    QTreeWidgetItem *item = itemFromIndex(index);
    
    
    // Check if this is the first row
    bool isTreeViewTopItem = !itemAbove(item);

    // The full row rectangle
    QRect rowRect = option.rect;

    // The rectangle at which the item starts if it is a child of another item
    QRect itemRect = visualItemRect(item);

    QRect branchRect( 0, rowRect.y(), itemRect.x(), rowRect.height() );

    // Find the item in the model corresponding to the tree item
    // and checks that it has the same type for sanity
    AnimatedItemTypeEnum foundType;
    KnobAnimPtr isKnob;
    TableItemAnimPtr isTableItem;
    NodeAnimPtr isNodeItem;
    ViewSetSpec view;
    DimSpec dim;
    bool found = getModel()->findItem(item, &foundType, &isKnob, &isTableItem, &isNodeItem, &view, &dim);
    assert(found);
    (void)found;
    assert(foundType == (AnimatedItemTypeEnum)item->data(0, QT_ROLE_CONTEXT_TYPE).toInt());

    // Check if we want an icon
    bool drawPluginIconToo = false;
    std::string iconFilePath;
    switch (foundType) {
        case eAnimatedItemTypeGroup:
        case eAnimatedItemTypeFrameRange:
        case eAnimatedItemTypeCommon:
        case eAnimatedItemTypeReader:
        case eAnimatedItemTypeRetime:
        case eAnimatedItemTypeTimeOffset:
            drawPluginIconToo = true;
            break;

        case eAnimatedItemTypeKnobDim:
        case eAnimatedItemTypeKnobRoot:
        case eAnimatedItemTypeKnobView:
        case eAnimatedItemTypeTableItemRoot:
            drawPluginIconToo = false;
            break;
    }


    // Draw row
    {
        NodeAnimPtr closestEnclosingNode;
        painter->save();

        QColor nodeColor;
        if (isNodeItem) {
            closestEnclosingNode = isNodeItem;
            if (drawPluginIconToo) {
                NodePtr internalNode = isNodeItem->getNodeGui()->getNode();
                iconFilePath = internalNode->getPluginIconFilePath();
            }
        } else if (isTableItem) {
            closestEnclosingNode = isTableItem->getNode();
            if (drawPluginIconToo) {
                iconFilePath = isTableItem->getInternalItem()->getIconLabelFilePath();
            }
        } else if (isKnob) {
            TableItemAnimPtr isTableItemKnob = toTableItemAnim(isKnob->getHolder());
            NodeAnimPtr isNodeKnob = toNodeAnim(isKnob->getHolder());
            if (isTableItemKnob) {
                closestEnclosingNode = isTableItemKnob->getNode();
            } else if (isNodeKnob) {
                closestEnclosingNode = isNodeKnob;
            }
        }
        if (closestEnclosingNode) {
            nodeColor = closestEnclosingNode->getNodeGui()->getCurrentColor();
        }

        QColor nodeColorDull = _imp->desaturate(nodeColor);
        QColor fillColor = nodeColorDull;

        // For a container, if we draw the icon do not use the desaturated colr
        if (drawPluginIconToo && isNodeItem && isNodeItem->containsNodeContext()) {
            fillColor = nodeColor;
        }

        // Draw the background with the fill color, add 1 pixel on the left
        painter->fillRect(itemRect.adjusted(TO_DPIX(-1), 0, 0, 0), fillColor);

        bool itemSelected = selectionModel()->isSelected(index);

        {
            QAbstractItemDelegate* delegate = itemDelegate();
            QStyleOptionViewItemV4 newOpt = option;

            if (itemSelected) {
                newOpt.state |= QStyle::State_Selected;
            }

            int xOffset = itemRect.x();

            for (int c = 0; c < item->columnCount(); ++c) {
                QModelIndex modelIndex = model()->index(index.row(), c, index.parent());

                int colX = columnViewportPosition(c);
                int colW = header()->sectionSize(c);

                int x;
                if (c == 0) {
                    x = xOffset + colX;
                } else {
                    x = colX;
                }
                newOpt.rect.setRect(x, option.rect.y(), colW, option.rect.height());
                // Call the paint function of our AnimationModuleTreeViewItemDelegate class
                delegate->paint(painter, newOpt, modelIndex);
            }
        }
        
        
        

        // Draw recursively the parent border on the left
        _imp->drawParentContainerContour(painter, closestEnclosingNode, itemRect);

        if (itemSelected) {
            double sR,sG,sB;
            appPTR->getCurrentSettings()->getSelectionColor(&sR, &sG, &sB);
            QColor c;
            c.setRgbF(Image::clamp(sR, 0., 1.), Image::clamp(sG, 0., 1.), Image::clamp(sB, 0., 1.));
            c.setAlphaF(0.4);
            painter->setOpacity(0.4);
            painter->fillRect(rowRect, c);
            painter->setOpacity(1.);
        }
        
        // Fill the branch rect with color and indicator
        drawBranch(painter, branchRect, closestEnclosingNode, nodeColor, item);

        // Separate each node row
        if (isNodeItem && !isTreeViewTopItem) {
            _imp->drawNodeTopSeparation(painter, itemRect);
        }



        painter->restore();
    }
} // AnimationModuleTreeView::drawRow

void
AnimationModuleTreeView::drawBranch(QPainter *painter,
                                    const QRect &rect,
                                    const NodeAnimPtr& closestEnclosingNodeItem,
                                    const QColor& nodeColor,
                                    QTreeWidgetItem* item) const
{

    QColor nodeColorDull = _imp->desaturate(nodeColor);

    // Paint with a dull color to the right edge of the node branch rect
    QRect nodeItemBranchRect = _imp->getBranchRect( closestEnclosingNodeItem->getTreeItem() );
    QRect rectForDull( nodeItemBranchRect.right(), rect.top(), rect.right() - nodeItemBranchRect.right(), rect.height() );

    painter->fillRect(rectForDull, nodeColorDull);

    // Draw the branch indicator
    QStyleOptionViewItemV4 option = viewOptions();
    option.rect = _imp->getArrowRect(item, rect);
    option.displayAlignment = Qt::AlignCenter;

    bool hasChildren = ( item->childCount() && !childrenAreHidden(item) );
    bool expanded = item->isExpanded();

    if (hasChildren) {
        option.state = QStyle::State_Children;
    }

    if (expanded) {
        option.state |= QStyle::State_Open;
    }

    style()->drawPrimitive(QStyle::PE_IndicatorBranch, &option, painter, this);

} // drawBranch

bool
AnimationModuleTreeView::childrenAreHidden(QTreeWidgetItem *item) const
{
    for (int i = 0; i < item->childCount(); ++i) {
        if ( !item->child(i)->isHidden() ) {
            return false;
        }
    }

    return true;
}

QTreeWidgetItem *
AnimationModuleTreeView::getParentItem(QTreeWidgetItem *item) const
{
    QTreeWidgetItem *ret = 0;
    QTreeWidgetItem *parentItem = item->parent();

    if (parentItem) {
        ret = parentItem;
    } else {
        QTreeWidget *treeWidget = item->treeWidget();
        assert(treeWidget);

        ret = treeWidget->invisibleRootItem();
    }

    return ret;
}

void
AnimationModuleTreeView::reparentItem(QTreeWidgetItem *child,
                        QTreeWidgetItem *newParent)
{
    QTreeWidgetItem *currentParent = getParentItem(child);
    if (currentParent) {
        currentParent->removeChild(child);
    }
    if (newParent) {
        newParent->addChild(child);
    } else {
        addTopLevelItem(child);
    }
}

void
AnimationModuleTreeView::onNodeAdded(const NodeAnimPtr& node)
{
    QTreeWidgetItem *treeItem = node->getTreeItem();

    // Is the animation of the given node modified downstream by a time node ?
    AnimationModulePtr model = _imp->model.lock();
    NodeAnimPtr isInputOfTimeNode = model->getNearestTimeNodeFromOutputs(node);

    // Is the node part of a group
    NodeAnimPtr parentGroupAnim = model->getGroupNodeAnim(node);

    // If modified by a time node, add it as a child of the time node
    if (isInputOfTimeNode) {
        isInputOfTimeNode->getTreeItem()->addChild(treeItem);
    } else if (parentGroupAnim) {
        // Otherwise add it to its group parent
        parentGroupAnim->getTreeItem()->addChild(treeItem);
    } else {
        addTopLevelItem(treeItem);
    }

    // Move all nodes that should be parented to this node
    std::vector<NodeAnimPtr> childrenNodes = model->getChildrenNodes(node);
    for (std::vector<NodeAnimPtr>::const_iterator it = childrenNodes.begin();
         it != childrenNodes.end();
         ++it) {

        reparentItem( (*it)->getTreeItem(), node->getTreeItem() );

        (*it)->refreshVisibility();

        (*it)->getTreeItem()->setExpanded(true);
    }

    node->refreshVisibility();

    treeItem->setExpanded(true);
} // onNodeAdded

void
AnimationModuleTreeView::onNodeAboutToBeRemoved(const NodeAnimPtr& node)
{
    if (!node) {
        return;
    }
    QTreeWidgetItem *treeItem = node->getTreeItem();
    if (!treeItem) {
        return;
    }
    // Put the child node items to the upper level
    QList<QTreeWidgetItem *> toMove;

    for (int i = 0; i < treeItem->childCount(); ++i) {
        QTreeWidgetItem *child = treeItem->child(i);

        AnimatedItemTypeEnum type = (AnimatedItemTypeEnum) child->data(0, QT_ROLE_CONTEXT_TYPE).toInt();
        switch (type) {
            case eAnimatedItemTypeGroup:
            case eAnimatedItemTypeFrameRange:
            case eAnimatedItemTypeCommon:
            case eAnimatedItemTypeReader:
            case eAnimatedItemTypeRetime:
            case eAnimatedItemTypeTimeOffset:
                toMove.push_back(child);
                break;
            case eAnimatedItemTypeKnobDim:
            case eAnimatedItemTypeKnobRoot:
            case eAnimatedItemTypeKnobView:
            case eAnimatedItemTypeTableItemRoot:
                break;
        }
    }

    QTreeWidgetItem *newParent = getParentItem(treeItem);

    Q_FOREACH (QTreeWidgetItem * childItem, toMove) {
        reparentItem(childItem, newParent);

        AnimatedItemTypeEnum foundType;
        KnobAnimPtr isKnob;
        TableItemAnimPtr isTableItem;
        NodeAnimPtr isNodeItem;
        ViewSetSpec view;
        DimSpec dim;
        bool found = getModel()->findItem(childItem, &foundType, &isKnob, &isTableItem, &isNodeItem, &view, &dim);
        (void)found;
        if (isNodeItem) {
            isNodeItem->refreshVisibility();
        }
        childItem->setExpanded(true);
    }

    // Remove the item from the tree
    newParent->removeChild(treeItem);
} // onNodeAboutToBeRemoved

void
AnimationModuleTreeViewPrivate::openSettingsPanelForNode(const NodeAnimPtr& node)
{
    NodeGuiPtr nodeGui = node->getNodeGui();

    // Move the nodeGui's settings panel on top
    DockablePanel *panel = 0;

    if (nodeGui) {
        nodeGui->ensurePanelCreated();
    }

    if ( nodeGui && nodeGui->getParentMultiInstance() ) {
        panel = nodeGui->getParentMultiInstance()->getSettingPanel();
    } else {
        panel = nodeGui->getSettingPanel();
    }

    if ( nodeGui && panel && nodeGui->isVisible() ) {
        if ( !nodeGui->isSettingsPanelVisible() ) {
            nodeGui->setVisibleSettingsPanel(true);
        }

        if ( !nodeGui->wasBeginEditCalled() ) {
            nodeGui->beginEditKnobs();
        }

        gui->putSettingsPanelFirst( nodeGui->getSettingPanel() );
        gui->getApp()->redrawAllViewers();
    }

} // openSettingsPanelForNode


void
AnimationModuleTreeView::onSelectionModelKeyframeSelectionChanged(bool recurse)
{
    AnimationModuleTreeViewSelectionModel *mySelecModel = dynamic_cast<AnimationModuleTreeViewSelectionModel *>( selectionModel() );

    assert(mySelecModel);
    if (!mySelecModel) {
        return;
    }

    if (_imp->selectionRecursionCounter) {
        return;
    }

    // Automatically select items in the tree according to the selection made by the user

    // Retrieve the knob anim with selected keyframes

    AnimationModuleSelectionModelPtr selModel = getModel()->getSelectionModel();

    const AnimItemDimViewKeyFramesMap& selectedKeys = selModel->getCurrentKeyFramesSelection();
    const std::list<NodeAnimPtr>& selectedNodes = selModel->getCurrentNodesSelection();
    const std::list<TableItemAnimPtr>& tableItems = selModel->getCurrentTableItemsSelection();


    // For each knob/dim/view, if all keyframes are selected, add the item to the selection
    std::set<QTreeWidgetItem*> toSelect;
    for (AnimItemDimViewKeyFramesMap::const_iterator it = selectedKeys.begin(); it != selectedKeys.end(); ++it) {
        bool selectItem = true;

        KeyFrameSet allKeys;
        it->first.item->getKeyframes(it->first.dim, it->first.view, AnimItemBase::eGetKeyframesTypeMerged, &allKeys);

        if (allKeys.size() != it->second.size()) {
            selectItem = false;
        }

        QTreeWidgetItem *treeItem = it->first.item->getTreeItem(it->first.dim, it->first.view);

        if (selectItem) {
            toSelect.insert(treeItem);
        }
    }

    for (std::list<NodeAnimPtr>::const_iterator it = selectedNodes.begin(); it != selectedNodes.end(); ++it) {
        toSelect.insert((*it)->getTreeItem());
    }

    for (std::list<TableItemAnimPtr>::const_iterator it = tableItems.begin(); it != tableItems.end(); ++it) {
        toSelect.insert((*it)->getRootItem());
    }

    QItemSelection selection;
    treeItemsSetToItemSelection(this, toSelect, &selection);

    ++_imp->selectionRecursionCounter;

    QItemSelectionModel::SelectionFlags flags = QItemSelectionModel::ClearAndSelect;
    mySelecModel->selectWithRecursion(selection, flags, recurse);

    --_imp->selectionRecursionCounter;

} // onSelectionModelKeyframeSelectionChanged

void
AnimationModuleTreeView::onItemDoubleClicked(QTreeWidgetItem *item,
                                   int column)
{
    Q_UNUSED(column);

    AnimatedItemTypeEnum foundType;
    KnobAnimPtr isKnob;
    TableItemAnimPtr isTableItem;
    NodeAnimPtr isNodeItem;
    ViewSetSpec view;
    DimSpec dim;
    bool found = getModel()->findItem(item, &foundType, &isKnob, &isTableItem, &isNodeItem, &view, &dim);
    if (!found) {
        return;
    }
    if (isKnob) {
        isTableItem = toTableItemAnim(isKnob->getHolder());
        if (isTableItem) {
            isNodeItem = isTableItem->getNode();
        } else {
            isNodeItem = toNodeAnim(isKnob->getHolder());
        }
    } else if (isTableItem) {
        isNodeItem = isTableItem->getNode();
    }
    if (isNodeItem) {
        _imp->openSettingsPanelForNode(isNodeItem);
    }

}


void
AnimationModuleTreeView::onTreeSelectionModelSelectionChanged()
{
    getModel()->getSelectionModel()->selectItems(selectedItems());
}

void
AnimationModuleTreeView::keyPressEvent(QKeyEvent* e)
{
    return QTreeWidget::keyPressEvent(e);
}

void
AnimationModuleTreeView::mouseReleaseEvent(QMouseEvent* e)
{
    QModelIndex index = indexAt( e->pos() );
    QTreeWidgetItem* item = itemAt( e->pos() );

    if ( triggerButtonIsRight(e) && index.isValid() ) {
        Q_EMIT itemRightClicked(e->globalPos(), item);
    } else {
        QTreeWidget::mouseReleaseEvent(e);
    }


}

void
AnimationModuleTreeView::setItemIcon(const QString& iconFilePath, QTreeWidgetItem* item)
{
    if (iconFilePath.isEmpty()) {
        return;
    }
    QPixmap pix;
    if (!QPixmapCache::find(iconFilePath, pix) ) {
        if (!pix.load(iconFilePath)) {
            return;
        }
        QPixmapCache::insert(iconFilePath, pix);
    }


    if (std::max( pix.width(), pix.height() ) != NATRON_MEDIUM_BUTTON_ICON_SIZE) {
        pix = pix.scaled(NATRON_MEDIUM_BUTTON_ICON_SIZE, NATRON_MEDIUM_BUTTON_ICON_SIZE,
                         Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    item->setIcon(ANIMATION_MODULE_TREE_VIEW_COL_PLUGIN_ICON, QIcon(pix));
}

std::vector<CurveGuiPtr>
AnimationModuleTreeView::getSelectedCurves() const
{
    QList<QTreeWidgetItem*> items = selectedItems();
    std::vector<CurveGuiPtr> curves;
    AnimationModulePtr model = getModel();
    for (QList<QTreeWidgetItem*>::const_iterator it = items.begin(); it != items.end(); ++it) {
        AnimatedItemTypeEnum foundType;
        KnobAnimPtr isKnob;
        TableItemAnimPtr isTableItem;
        NodeAnimPtr isNodeItem;
        ViewSetSpec view;
        DimSpec dim;
        bool found = model->findItem(*it, &foundType, &isKnob, &isTableItem, &isNodeItem, &view, &dim);
        if (!found) {
            continue;
        }
        AnimItemBasePtr isAnimItem;
        if (isKnob) {
            isAnimItem = isKnob;
        } 
        if (!isAnimItem) {
            continue;
        }
        if (view.isAll()) {
            continue;
        }
        if (dim.isAll()) {
            // If single dimensional or dimensions are folded, make it dimension 0
            if (isAnimItem->getNDimensions() == 1 || !isAnimItem->getAllDimensionsVisible(ViewIdx(view))) {
                dim = DimIdx(0);
            } else {
                continue;
            }
        }
        CurveGuiPtr curve = isAnimItem->getCurveGui(DimIdx(dim), ViewIdx(view));
        if (curve) {
            curves.push_back(curve);
        }
    }
    return curves;
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_AnimationModuleTreeView.cpp"
