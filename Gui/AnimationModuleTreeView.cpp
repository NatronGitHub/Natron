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

#include "AnimationModuleTreeView.h"

#include <stdexcept>

#include <QtCore/QDebug> //REMOVEME
#include <QHeaderView>
#include <QPainter>
#include <QResizeEvent>
#include <QPixmapCache>
#include <QStyleOption>

#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/KnobItemsTable.h"
#include "Engine/Settings.h"
#include "Engine/ViewIdx.h"

#include "Gui/DockablePanel.h"
#include "Gui/AnimationModule.h"
#include "Gui/AnimationModuleSelectionModel.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiDefines.h"
#include "Gui/KnobAnim.h"
#include "Gui/KnobGui.h"
#include "Gui/NodeGui.h"
#include "Gui/NodeAnim.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/NodeSettingsPanel.h"
#include "Gui/TableItemAnim.h"

NATRON_NAMESPACE_ENTER;

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

void
AnimationModuleTreeViewSelectionModel::selectInternal(const QItemSelection &userSelection,
                                            QItemSelectionModel::SelectionFlags command,
                                            bool recurse)
{
    QItemSelection finalSelection = userSelection;

    if (recurse) {
        QModelIndexList userSelectedIndexes = userSelection.indexes();

        for (QModelIndexList::const_iterator it = userSelectedIndexes.begin();
             it != userSelectedIndexes.end();
             ++it) {
            const QModelIndex& index = (*it);
            if ( !index.isValid() ) {
                continue;
            }

            // Add children of the index to the selection
            selectChildren(index, &finalSelection);

            QItemSelection unitedSelection = selection();

            if (command & QItemSelectionModel::Clear) {
                unitedSelection.clear();
            }

            unitedSelection.merge(finalSelection, command);

            QTreeWidgetItem* treeItem = _view->getTreeItemForModelIndex(index);
            assert(treeItem);
            if (treeItem) {
                // If there's a parent for the item and the item itself is not a node,
                // check if we should add the parent to the selection
                AnimatedItemTypeEnum type;
                KnobAnimPtr isKnob;
                TableItemAnimPtr isTableItem;
                NodeAnimPtr isNodeItem;
                ViewSetSpec view;
                DimSpec dim;
                bool found = _model.lock()->findItem(treeItem, &type, &isKnob, &isTableItem, &isNodeItem, &view, &dim);
                if ( found && !isNodeItem && index.parent().isValid() ) {
                    checkParentsSelectedStates(index, command, unitedSelection, &finalSelection);
                }
            }
        }
    }
    QItemSelectionModel::select(finalSelection, command);
} // selectInternal

void
AnimationModuleTreeViewSelectionModel::selectWithRecursion(const QItemSelection &userSelection,
                                                 QItemSelectionModel::SelectionFlags command,
                                                 bool recurse)
{
    selectInternal(userSelection, command, recurse);
}

void
AnimationModuleTreeViewSelectionModel::select(const QItemSelection &userSelection,
                                    QItemSelectionModel::SelectionFlags command)
{
    selectInternal(userSelection, command, true);
}

void
AnimationModuleTreeViewSelectionModel::selectChildren(const QModelIndex &index,
                                            QItemSelection *selection) const
{
    int row = 0;
    QModelIndex childIndex = index.child(row, 0);

    while ( childIndex.isValid() ) {
        if ( !selection->contains(childIndex) ) {
            selection->select(childIndex, childIndex);
        }

        // /!\ recursion
        {
            selectChildren(childIndex, selection);
        }

        ++row;
        childIndex = index.child(row, 0);
    }
}

void
AnimationModuleTreeViewSelectionModel::checkParentsSelectedStates(const QModelIndex &index,
                                                        QItemSelectionModel::SelectionFlags flags,
                                                        const QItemSelection &unitedSelection,
                                                        QItemSelection *finalSelection) const
{
    // Recursively fills the list of parents until we hit a parent node
    std::list<QModelIndex> parentIndexes;
    {
        QModelIndex pIndex = index.parent();
        QTreeWidgetItem* parentItem = _view->getTreeItemForModelIndex(pIndex);
        NodeAnimPtr isParentNode;
        if (parentItem) {
            AnimatedItemTypeEnum type;
            KnobAnimPtr isKnob;
            TableItemAnimPtr isTableItem;
            NodeAnimPtr isNodeItem;
            ViewSetSpec view;
            DimSpec dim;
            bool found = _model.lock()->findItem(parentItem, &type, &isKnob, &isTableItem, &isNodeItem, &view, &dim);
            if (found) {
                isParentNode = isNodeItem;
            }
        }
        int nbLvlOfNodesSelected = 0;
        while ( nbLvlOfNodesSelected < 1 && pIndex.isValid() ) {
            if (isParentNode) {
                ++nbLvlOfNodesSelected;
            }
            parentIndexes.push_back(pIndex);

            pIndex = pIndex.parent();
            if ( pIndex.isValid() ) {
                parentItem = _view->getTreeItemForModelIndex(pIndex);
                if (parentItem) {
                    AnimatedItemTypeEnum type;
                    KnobAnimPtr isKnob;
                    TableItemAnimPtr isTableItem;
                    NodeAnimPtr isNodeItem;
                    ViewSetSpec view;
                    DimSpec dim;
                    bool found = _model.lock()->findItem(parentItem, &type, &isKnob, &isTableItem, &isNodeItem, &view, &dim);
                    if (found) {
                        isParentNode = isNodeItem;
                    }
                }
            }
        }
    }
    QItemSelection uuSelec = unitedSelection;

    // If all children are selected, select the parent
    for (std::list<QModelIndex>::const_iterator it = parentIndexes.begin();
         it != parentIndexes.end();
         ++it) {
        QModelIndex index = (*it);
        bool selectParent = true;
        int row = 0;
        QModelIndex childIndexIt = index.child(row, 0);

        while ( childIndexIt.isValid() ) {
            if ( childIndexIt.data(QT_ROLE_CONTEXT_IS_ANIMATED).toBool() ) {
                if ( !uuSelec.contains(childIndexIt) ) {
                    selectParent = false;

                    break;
                }
            }

            ++row;
            childIndexIt = index.child(row, 0);
        }

        if ( (flags & QItemSelectionModel::Select && selectParent) ) {
            finalSelection->select(index, index);
            uuSelec.select(index, index);
        } else if (flags & QItemSelectionModel::Deselect && !selectParent) {
            finalSelection->select(index, index);
            uuSelec.merge(QItemSelection(index, index),
                          QItemSelectionModel::Deselect);
        }
    }
} // AnimationModuleTreeViewSelectionModel::checkParentsSelectedStates



AnimationModuleTreeViewItemDelegate::AnimationModuleTreeViewItemDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
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
AnimationModuleTreeViewItemDelegate::paint(QPainter *painter,
                                 const QStyleOptionViewItem &option,
                                 const QModelIndex &index) const
{
    painter->save();

    double r, g, b;

    // Selection color is hard-coded : Todo make this a setting
    if (option.state & QStyle::State_Selected) {
        r = g = b = 0.941f;
    } else {
        r = g = b = 0.11f;
    }

    painter->setPen( QColor::fromRgbF(r, g, b) );
    painter->drawText( option.rect, Qt::AlignVCenter, index.data().toString() );

    painter->restore();
}

////////////////////////// AnimationModuleTreeView //////////////////////////

class AnimationModuleTreeViewPrivate
{
public:
    AnimationModuleTreeViewPrivate(AnimationModuleTreeView* publicInterface)
    : publicInterface(publicInterface)
    , model()
    , gui(0)
    , _canResizeOtherWidget(true)
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

    // item painting
    void drawPluginIconArea(QPainter *p, const std::string& iconFilePath, const QRect &rowRect, bool drawPluginIcon) const;
    void drawParentContainerContour(QPainter *p, const NodeAnimPtr& parentNode, const QRect &itemRect);
    void drawNodeTopSeparation(QPainter *p, const NodeAnimPtr& node, const QRect &rowRect) const;
    //void drawNodeBottomSeparation(QPainter *p, const NodeAnimPtr& node, const NodeAnimPtr& nodeBelow, const QRect &rowRect) const;

    QColor desaturate(const QColor &color) const;

    // keyframe selection
    void selectKeyframes(const QList<QTreeWidgetItem *> &items);


    /* attributes */
    AnimationModuleTreeView *publicInterface;
    AnimationModuleWPtr model;
    Gui *gui;
    bool _canResizeOtherWidget;
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
AnimationModuleTreeViewPrivate::drawPluginIconArea(QPainter *p,
                                                   const std::string& iconFilePath,
                                                   const QRect &rowRect,
                                                   bool drawPluginIcon) const
{

    if ( iconFilePath.empty() ) {
        return;
    }

    QString fileName = QString::fromUtf8( iconFilePath.c_str() );

    QPixmap pix;

    if (!QPixmapCache::find(fileName, pix) ) {
        if (!pix.load(fileName)) {
            return;
        }
        QPixmapCache::insert(fileName, pix);

    }


    if (std::max( pix.width(), pix.height() ) != NATRON_MEDIUM_BUTTON_ICON_SIZE) {
        pix = pix.scaled(NATRON_MEDIUM_BUTTON_ICON_SIZE, NATRON_MEDIUM_BUTTON_ICON_SIZE,
                         Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    QRect areaRect = rowRect;
    areaRect.setWidth(pix.width() + 4);
    areaRect.moveRight( rowRect.right() );

    int r, g, b;
    appPTR->getCurrentSettings()->getPluginIconFrameColor(&r, &g, &b);
    p->fillRect( areaRect, QColor(r, g, b) );

    QRect pluginAreaRect = rowRect;
    pluginAreaRect.setSize( pix.size() );
    pluginAreaRect.moveCenter( QPoint( areaRect.center().x(),
                                      rowRect.center().y() ) );

    if (drawPluginIcon) {
        p->drawPixmap(pluginAreaRect, pix);
    }

} // drawPluginIconArea

void
AnimationModuleTreeViewPrivate::drawParentContainerContour(QPainter *p,
                                                           const NodeAnimPtr& parentNode,
                                                           const QRect &itemRect)
{
    NodeAnimPtr nodeIt = parentNode;
    while (nodeIt) {
        QColor nodeColor = parentNode->getNodeGui()->getCurrentColor();
        QTreeWidgetItem *nodeItem = parentNode->getTreeItem();

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
AnimationModuleTreeViewPrivate::drawNodeTopSeparation(QPainter *p,
                                            const NodeAnimPtr& node,
                                            const QRect &rowRect) const
{
    int lineWidth = TO_DPIX(4);
    int lineBegin = getBranchRect( node->getTreeItem() ).right() + TO_DPIX(2);


    QPen pen(Qt::black);
    pen.setWidth(lineWidth);
    p->setPen(pen);
    p->drawLine(lineBegin, rowRect.top() + lineWidth - 1, rowRect.right(), rowRect.top() + lineWidth - 1);
}

#if 0
void
AnimationModuleTreeViewPrivate::drawNodeBottomSeparation(QPainter *p,
                                               const NodeAnimPtr& node,
                                               const NodeAnimPtr& nodeBelow,
                                               const QRect &rowRect) const
{
    int lineWidth = (TO_DPIX(4) / 2);
    int lineBegin = publicInterface->rect().left();

    if ( node->containsNodeContext() ) {
        if ( node->getTreeItem()->isExpanded() ) {
            lineBegin = getBranchRect( node->getTreeItem() ).right() + TO_DPIX(2);
        }
    } else {
        NodeAnimPtr parentNode = model->mapNameItemToNodeAnim( nodeBelow->getTreeItem()->parent() );
        if (parentNode) {
            lineBegin = getBranchRect( parentNode->getTreeItem() ).right() + TO_DPIX(2);
        }
    }

    QPen pen(Qt::black);
    pen.setWidth(lineWidth);

    p->setPen(pen);

    p->drawLine(lineBegin, rowRect.bottom() - lineWidth + 2,
                rowRect.right(), rowRect.bottom() - lineWidth + 2);
}
#endif

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

void
AnimationModuleTreeViewPrivate::selectKeyframes(const QList<QTreeWidgetItem *> &items)
{
    AnimItemDimViewKeyFramesMap keys;
    std::vector<NodeAnimPtr > nodes;
    std::vector<TableItemAnimPtr> tableItems;

    AnimationModulePtr animModel = model.lock();
    Q_FOREACH (QTreeWidgetItem * item, items) {
        AnimatedItemTypeEnum foundType;
        KnobAnimPtr isKnob;
        TableItemAnimPtr isTableItem;
        NodeAnimPtr isNodeItem;
        ViewSetSpec view;
        DimSpec dim;
        bool found = animModel->findItem(item, &foundType, &isKnob, &isTableItem, &isNodeItem, &view, &dim);
        if (!found) {
            continue;
        }

        if (isKnob) {
            isKnob->getKeyframes(dim, view, &keys);
        } else if (isNodeItem) {
            nodes.push_back(isNodeItem);
        } else if (isTableItem) {
            tableItems.push_back(isTableItem);
        }
    }

    AnimationModuleSelectionModel::SelectionTypeFlags sFlags = AnimationModuleSelectionModel::SelectionTypeAdd
                                                         | AnimationModuleSelectionModel::SelectionTypeClear | AnimationModuleSelectionModel::SelectionTypeRecurse;

    animModel->getSelectionModel().makeSelection(keys, tableItems, nodes, sFlags);
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
    setColumnCount(1);
    setExpandsOnDoubleClick(false);

    setItemDelegate( new AnimationModuleTreeViewItemDelegate(this) );

    // Very import otherwise on macOS a bug makes the whole UI refresh
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

void
AnimationModuleTreeView::setCanResizeOtherWidget(bool canResize)
{
    _imp->_canResizeOtherWidget = canResize;
}

void
AnimationModuleTreeView::getSelectedKnobAnims(std::list<KnobAnimPtr >* knobs) const
{
    QList<QTreeWidgetItem*> items = selectedItems();
    for (QList<QTreeWidgetItem*>::iterator it = items.begin(); it != items.end(); ++it) {
        KnobAnimPtr k = _imp->model->mapNameItemToKnobAnim(*it);
        if (k) {
            knobs->push_back(k);
        }
    }
}

void
AnimationModuleTreeView::resizeEvent(QResizeEvent* e)
{
    QTreeWidget::resizeEvent(e);

    if ( _imp->_canResizeOtherWidget && _imp->gui->isTripleSyncEnabled() ) {
        _imp->gui->setCurveEditorTreeWidth( e->size().width() );
    }
}

KnobAnimPtr AnimationModuleTreeView::getKnobAnimAt(int y) const
{
    QTreeWidgetItem *itemUnderPoint = itemAt(TO_DPIX(5), y);
    AnimatedItemTypeEnum foundType;
    KnobAnimPtr isKnob;
    TableItemAnimPtr isTableItem;
    NodeAnimPtr isNodeItem;
    ViewSetSpec view;
    DimSpec dim;
    bool found = getModel()->findItem(itemUnderPoint, &foundType, &isKnob, &isTableItem, &isNodeItem, &view, &dim);
    (void)found;
    if (isKnob) {
        return isKnob;
    }
    return KnobAnimPtr();
}

bool
AnimationModuleTreeView::itemIsVisibleFromOutside(QTreeWidgetItem *item) const
{
    bool ret = true;
    QTreeWidgetItem *it = item->parent();

    while (it) {
        if ( !it->isExpanded() ) {
            ret = false;

            break;
        }

        it = it->parent();
    }

    return ret;
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
    bool drawPluginIconToo;
    std::string iconFilePath;
    switch (foundType) {
        case eAnimatedItemTypeGroup:
        case eAnimatedItemTypeFrameRange:
        case eAnimatedItemTypeCommon:
        case eAnimatedItemTypeReader:
        case eAnimatedItemTypeRetime:
        case eAnimatedItemTypeTimeOffset:
        case eAnimatedItemTypeTableItemAnimation:
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
            isNodeItem = closestEnclosingNode;
            if (drawPluginIconToo) {
                iconFilePath = isNodeItem->getNodeGui()->getNode()->getPluginIconFilePath();
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
                closestEnclosingNode = isNodeKnob->getNodeGui();
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

        // Draw the item text
        QStyleOptionViewItemV4 newOpt = viewOptions();
        newOpt.rect = itemRect;

        if ( selectionModel()->isSelected(index) ) {
            newOpt.state |= QStyle::State_Selected;
        }
        // Call the paint function of our AnimationModuleTreeViewItemDelegate class
        itemDelegate()->paint(painter, newOpt, index);

        // Draw recursively the parent border on the left
        _imp->drawParentContainerContour(painter, closestEnclosingNode, itemRect);

        // Fill the branch rect with color and indicator
        drawBranch(painter, branchRect, closestEnclosingNode, nodeColor, item);

        // Draw the plug-in or item icon
        _imp->drawPluginIconArea(painter, iconFilePath, rowRect, drawPluginIconToo);

        // Separate each node row
        if (isNodeItem && !isTreeViewTopItem) {
            _imp->drawNodeTopSeparation(painter, isNodeItem, rowRect);
        }

        // If the next item is a node as well, draw a separation
        /*{
            NodeAnimPtr nodeBelow = _imp->itemBelowIsNode(item);
            if (nodeBelow) {
                _imp->drawNodeBottomSeparation(painter, closestEnclosingNode, nodeBelow, rowRect);
            }
        }*/

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
    std::vector<NodeAnimPtr > childrenNodes = model->getChildrenNodes(node);
    for (std::vector<NodeAnimPtr >::const_iterator it = childrenNodes.begin();
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
            case eAnimatedItemTypeTableItemAnimation:
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



// For each unique item/dim/view key, the number of selected keyframes
typedef std::map<AnimItemDimViewID, int, AnimItemDimViewID_CompareLess> AnimItemDimViewMap;

void
AnimationModuleTreeView::onSelectionModelKeyframeSelectionChanged(bool recurse)
{
    AnimationModuleTreeViewSelectionModel *mySelecModel = dynamic_cast<AnimationModuleTreeViewSelectionModel *>( selectionModel() );

    assert(mySelecModel);
    if (!mySelecModel) {
        throw std::logic_error("AnimationModuleTreeView::onKeyframeSelectionChanged");
    }

    // Automatically select items in the tree according to the selection made by the user

    // Retrieve the knob anim with selected keyframes
    AnimItemDimViewMap uniqueRows;

    AnimKeyFramePtrList selectedKeys;
    std::vector<NodeAnimPtr > selectedNodes;
    std::vector<TableItemAnimPtr> tableItems;

    getModel()->getSelectionModel().getCurrentSelection(&selectedKeys, &selectedNodes, &tableItems);

    for (AnimKeyFramePtrList::const_iterator it = selectedKeys.begin(); it != selectedKeys.end(); ++it) {
        AnimItemDimViewID k;
        k.item = (*it)->getContext();
        k.dim = (*it)->getDimension();
        k.view = (*it)->getView();

        AnimItemDimViewMap::iterator found = uniqueRows.find(k);
        if (found != uniqueRows.end()) {
            ++found->second;
        } else {
            found->second = 1;
        }
    }


    // Prevent recursion
    disconnect( selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
                this, SLOT(onTreeSelectionModelSelectionChanged()) );

    // Compose tree selection from the selected keyframes
    if ( uniqueRows.empty() && selectedNodes.empty() ) {
        // Clear selection if empty
        mySelecModel->selectWithRecursion(QItemSelection(), QItemSelectionModel::Clear, recurse);
    } else {

        // For each knob/dim/view, if all keyframes are selected, add the item to the selection
        std::set<QModelIndex> toSelect;
        for (AnimItemDimViewMap::const_iterator it = uniqueRows.begin(); it != uniqueRows.end(); ++it) {
            bool selectItem = true;

            std::vector<AnimKeyFrame> keys;
            it->first.item->getKeyframes(it->first.dim, it->first.view, &keys);

            if ((int)keys.size() != it->second) {
                selectItem = false;
            }

            QTreeWidgetItem *knobItem = it->first.item->getRootItem();

            if (selectItem) {
                toSelect.insert( indexFromItem(knobItem) );
            }
        }

        for (std::vector<NodeAnimPtr >::iterator it = selectedNodes.begin(); it != selectedNodes.end(); ++it) {
            toSelect.insert( indexFromItem( (*it)->getTreeItem() ) );
        }

        for (std::vector<TableItemAnimPtr>::iterator it = tableItems.begin(); it != tableItems.end(); ++it) {
            toSelect.insert( indexFromItem( (*it)->getRootItem() ) );
        }

        QItemSelection selection;

        for (std::set<QModelIndex>::const_iterator indexIt = toSelect.begin();
             indexIt != toSelect.end();
             ++indexIt) {
            QModelIndex index = (*indexIt);

            selection.select(index, index);
        }

        QItemSelectionModel::SelectionFlags flags = QItemSelectionModel::ClearAndSelect;
        mySelecModel->selectWithRecursion(selection, flags, recurse);
    }

    connect( selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
             this, SLOT(onTreeSelectionModelSelectionChanged()) );
} // AnimationModuleTreeView::onKeyframeSelectionChanged

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
    if (!isNodeItem) {
        return;
    }
    NodeGuiPtr nodeGui = isNodeItem->getNodeGui();

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

        _imp->gui->putSettingsPanelFirst( nodeGui->getSettingPanel() );
        _imp->gui->getApp()->redrawAllViewers();
    }
}

void
AnimationModuleTreeView::onTreeSelectionModelSelectionChanged()
{
    _imp->selectKeyframes( selectedItems() );
}

NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_AnimationModuleTreeView.cpp"
