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

#include "DopeSheetHierarchyView.h"

#include <QDebug> //REMOVEME
#include <QHeaderView>
#include <QPainter>
#include <QResizeEvent>
#include <QStyleOption>

#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/Settings.h"

#include "Gui/DockablePanel.h"
#include "Gui/DopeSheet.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiDefines.h"
#include "Gui/KnobGui.h"
#include "Gui/NodeGui.h"
#include "Gui/NodeSettingsPanel.h"

NATRON_NAMESPACE_ENTER;

typedef std::list<boost::shared_ptr<DSKnob> > DSKnobPtrList;


////////////////////////// HierarchyViewSelectionModel //////////////////////////

HierarchyViewSelectionModel::HierarchyViewSelectionModel(DopeSheet* dopesheetModel,
                                                         HierarchyView* view,
                                                         QAbstractItemModel *model,
                                                         QObject *parent)
: QItemSelectionModel(model, parent)
, _dopesheetModel(dopesheetModel)
, _view(view)
{
    connect(model, SIGNAL(destroyed()),
            this, SLOT(deleteLater()));
}

HierarchyViewSelectionModel::~HierarchyViewSelectionModel()
{}

void
HierarchyViewSelectionModel::selectInternal(const QItemSelection &userSelection,
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
            if (!index.isValid()) {
                continue;
            }
            selectChildren(index, &finalSelection);
            
            QItemSelection unitedSelection = selection();
            
            if (command & QItemSelectionModel::Clear) {
                unitedSelection.clear();
            }
            
            unitedSelection.merge(finalSelection, command);

            QTreeWidgetItem* treeItem = _view->getTreeItemForModelIndex(index);
            if (treeItem) {
                boost::shared_ptr<DSNode> isDsNode = _dopesheetModel->mapNameItemToDSNode(treeItem);
            
                if (!isDsNode && index.parent().isValid()) {
                    checkParentsSelectedStates(index, command, unitedSelection, &finalSelection);
                }
            }
            
            
        }
    }
    QItemSelectionModel::select(finalSelection, command);
}

void
HierarchyViewSelectionModel::selectWithRecursion(const QItemSelection &userSelection,
            QItemSelectionModel::SelectionFlags command,
            bool recurse)
{
    selectInternal(userSelection, command, recurse);
}

void HierarchyViewSelectionModel::select(const QItemSelection &userSelection,
                                         QItemSelectionModel::SelectionFlags command)
{
    selectInternal(userSelection, command, true);
}

void HierarchyViewSelectionModel::selectChildren(const QModelIndex &index, QItemSelection *selection) const
{
    int row = 0;
    QModelIndex childIndex = index.child(row, 0);

    while (childIndex.isValid()) {
        if (!selection->contains(childIndex)) {
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

void HierarchyViewSelectionModel::checkParentsSelectedStates(const QModelIndex &index,
                                                             QItemSelectionModel::SelectionFlags flags,
                                                             const QItemSelection &unitedSelection,
                                                             QItemSelection *finalSelection) const
{
    // Fill the list of parents
    std::list<QModelIndex> parentIndexes;
    {
        QModelIndex pIndex = index.parent();
        QTreeWidgetItem* treeItem = _view->getTreeItemForModelIndex(pIndex);
        boost::shared_ptr<DSNode> isParentNode;
        if (treeItem) {
            isParentNode = _dopesheetModel->mapNameItemToDSNode(treeItem);
        }
        int nbLvlOfNodesSelected = 0;
        while (nbLvlOfNodesSelected < 1 && pIndex.isValid()) {
            
            if (isParentNode) {
                ++nbLvlOfNodesSelected;
            }
            parentIndexes.push_back(pIndex);

            pIndex = pIndex.parent();
            if (pIndex.isValid()) {
                treeItem = _view->getTreeItemForModelIndex(pIndex);
                if (treeItem) {
                    isParentNode = _dopesheetModel->mapNameItemToDSNode(treeItem);
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

        while (childIndexIt.isValid()) {
            if (childIndexIt.data(QT_ROLE_CONTEXT_IS_ANIMATED).toBool()) {
                if (!uuSelec.contains(childIndexIt)) {
                    selectParent = false;

                    break;
                }
            }

            ++row;
            childIndexIt = index.child(row, 0);
        }

        if ( (flags & QItemSelectionModel::Select && selectParent))  {
            finalSelection->select(index, index);
            uuSelec.select(index, index);
        }
        else if (flags & QItemSelectionModel::Deselect && !selectParent) {
            finalSelection->select(index, index);
            uuSelec.merge(QItemSelection(index, index),
                          QItemSelectionModel::Deselect);
        }
    }
}


////////////////////////// HierarchyViewDelegate //////////////////////////

/**
 * @brief HierarchyViewItemDelegate::HierarchyViewItemDelegate
 *
 *
 */
HierarchyViewItemDelegate::HierarchyViewItemDelegate(QObject *parent) :
    QStyledItemDelegate(parent)
{}

/**
 * @brief HierarchyViewItemDelegate::sizeHint
 *
 *
 */
QSize HierarchyViewItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option);

    QSize itemSize = QStyledItemDelegate::sizeHint(option, index);

    Natron::DopeSheetItemType nodeType = Natron::DopeSheetItemType(index.data(QT_ROLE_CONTEXT_TYPE).toInt());
    int heightOffset = 0;

    switch (nodeType) {
    case Natron::eDopeSheetItemTypeReader:
    case Natron::eDopeSheetItemTypeRetime:
    case Natron::eDopeSheetItemTypeTimeOffset:
    case Natron::eDopeSheetItemTypeFrameRange:
    case Natron::eDopeSheetItemTypeGroup:
        heightOffset = 10;
        break;
    default:
        break;
    }

    itemSize.rheight() += heightOffset;

    return itemSize;
}

void HierarchyViewItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    painter->save();

    double r, g, b;

    if (option.state & QStyle::State_Selected) {
        r = g = b = 0.941f;
    }
    else {
        r = g = b = 0.11f;
    }

    painter->setPen(QColor::fromRgbF(r, g, b));
    painter->drawText(option.rect, Qt::AlignVCenter, index.data().toString());

    painter->restore();
}


////////////////////////// HierarchyView //////////////////////////

class HierarchyViewPrivate
{
public:
    HierarchyViewPrivate(HierarchyView *qq);
    ~HierarchyViewPrivate();

    /* functions */
    void checkKnobsVisibleState(DSNode *dsNode);
    void checkNodeVisibleState(DSNode *dsNode);
    void checkKnobVisibleState(DSKnob *dsKnob);

    // item related
    QRect getBranchRect(QTreeWidgetItem *item) const;
    QRect getArrowRect(QTreeWidgetItem *item) const;
    QRect getParentArrowRect(QTreeWidgetItem *item, const QRect &branchRect) const;

    boost::shared_ptr<DSNode> itemBelowIsNode(QTreeWidgetItem *item) const;

    // item painting
    void drawPluginIconArea(QPainter *p, boost::shared_ptr<DSNode> dsNode, const QRect &rowRect, bool drawPluginIcon) const;
    void drawColoredIndicators(QPainter *p, QTreeWidgetItem *item, const QRect &itemRect);
    void drawNodeTopSeparation(QPainter *p, QTreeWidgetItem *item, const QRect &rowRect) const;
    void drawNodeBottomSeparation(QPainter *p, boost::shared_ptr<DSNode> dsNode, boost::shared_ptr<DSNode> nodeBelow, const QRect &rowRect) const;

    QColor getDullColor(const QColor &color) const;

    // keyframe selection
    void selectKeyframes(const QList<QTreeWidgetItem *> &items);

    
    /* attributes */
    HierarchyView *q_ptr;
    DopeSheet *dopeSheetModel;
    Gui *gui;
    bool _canResizeOtherWidget;
};

HierarchyViewPrivate::HierarchyViewPrivate(HierarchyView *qq) :
    q_ptr(qq),
    dopeSheetModel(0),
    gui(0),
    _canResizeOtherWidget(true)
{}

HierarchyViewPrivate::~HierarchyViewPrivate()
{}

/**
 * @see HierarchyView::onKeyframeSetOrRemoved()
 */
void HierarchyViewPrivate::checkKnobsVisibleState(DSNode *dsNode)
{
    const DSTreeItemKnobMap& knobRows = dsNode->getItemKnobMap();

    for (DSTreeItemKnobMap::const_iterator it = knobRows.begin(); it != knobRows.end(); ++it) {
        boost::shared_ptr<DSKnob> knobContext = (*it).second;
        QTreeWidgetItem *knobItem = (*it).first;

        // Expand if it's a multidim root item
        if (knobItem->childCount() > 0) {
            knobItem->setExpanded(true);
        }

        if (knobContext->getDimension() <= 0) {
            checkKnobVisibleState(knobContext.get());
        }
    }
}

/**
 * @see HierarchyView::onKeyframeSetOrRemoved()
 */
void HierarchyViewPrivate::checkNodeVisibleState(DSNode *dsNode)
{
    boost::shared_ptr<NodeGui> nodeGui = dsNode->getNodeGui();


    Natron::DopeSheetItemType nodeType = dsNode->getItemType();
    QTreeWidgetItem *nodeItem = dsNode->getTreeItem();

    bool showNode;
    if (nodeType == Natron::eDopeSheetItemTypeCommon) {
        showNode = nodeHasAnimation(nodeGui);
    } else {
        showNode = true;
    }
    
    if (!nodeGui->isSettingsPanelVisible()) {
        showNode = false;
    }

    nodeItem->setData(0, QT_ROLE_CONTEXT_IS_ANIMATED, showNode);

    nodeItem->setHidden(!showNode);
}

/**
 * @see HierarchyView::onKeyframeSetOrRemoved()
 */
void HierarchyViewPrivate::checkKnobVisibleState(DSKnob *dsKnob)
{
    int dim = dsKnob->getDimension();

    KnobGui *knobGui = dsKnob->getKnobGui();

    bool showContext = false;

    if (dsKnob->isMultiDimRoot()) {
        for (int i = 0; i < knobGui->getKnob()->getDimension(); ++i) {
            bool curveIsAnimated = knobGui->getCurve(i)->isAnimated();

            QTreeWidgetItem *dimItem = dsKnob->findDimTreeItem(i);
            dimItem->setHidden(!curveIsAnimated);
            dimItem->setData(0, QT_ROLE_CONTEXT_IS_ANIMATED, curveIsAnimated);

            if (curveIsAnimated) {
                showContext = true;
            }
        }
    }
    else {
        showContext = knobGui->getCurve(dim)->isAnimated();
    }

    QTreeWidgetItem *treeItem = dsKnob->getTreeItem();
    treeItem->setHidden(!showContext);
    treeItem->setData(0, QT_ROLE_CONTEXT_IS_ANIMATED, showContext);
}

/**
 * @brief Returns the branch rect of 'item'.
 */
QRect HierarchyViewPrivate::getBranchRect(QTreeWidgetItem *item) const
{
    QRect itemRect = q_ptr->visualItemRect(item);

    return QRect(q_ptr->rect().left(), itemRect.top(),
                 itemRect.left(), itemRect.height());
}

/**
 * @brief Returns the rect of the expanded indicator of 'item'.
 */
QRect HierarchyViewPrivate::getArrowRect(QTreeWidgetItem *item) const
{
    QRect branchRect = getBranchRect(item);

    if (!item->parent()) {
        return branchRect;
    }

    int parentBranchRectRight = q_ptr->visualItemRect(item->parent()).left();
    int arrowRectWidth  = branchRect.right() - parentBranchRectRight;

    return QRect(parentBranchRectRight, branchRect.top(),
                 arrowRectWidth, branchRect.height());
}

/**
 * @brief Returns the rect of the expanded indicator of the parent of 'item'.
 *
 * You must provide the branch rect of 'item'.
 */
QRect HierarchyViewPrivate::getParentArrowRect(QTreeWidgetItem *item, const QRect &branchRect) const
{
    if (!item->parent()) {
        return branchRect;
    }

    int parentBranchRectRight = q_ptr->visualItemRect(item->parent()).left();
    int arrowRectWidth  = branchRect.right() - parentBranchRectRight;

    return QRect(parentBranchRectRight, branchRect.top(),
                 arrowRectWidth, branchRect.height());
}

/**
 * @brief Returns the DSNode associated with the item below 'item', and a null
 * pointer if it don't exist.
 */
boost::shared_ptr<DSNode> HierarchyViewPrivate::itemBelowIsNode(QTreeWidgetItem *item) const
{
    boost::shared_ptr<DSNode> ret;

    QTreeWidgetItem *itemBelow = q_ptr->itemBelow(item);

    if (itemBelow) {
        ret = dopeSheetModel->mapNameItemToDSNode(itemBelow);
    }

    return ret;
}

void HierarchyViewPrivate::drawPluginIconArea(QPainter *p, boost::shared_ptr<DSNode> dsNode, const QRect &rowRect, bool drawPluginIcon) const
{
    std::string iconFilePath = dsNode->getInternalNode()->getPluginIconFilePath();

    if (!iconFilePath.empty()) {
        QPixmap pix;

        if (pix.load(iconFilePath.c_str())) {
            if (std::max(pix.width(), pix.height()) != NATRON_MEDIUM_BUTTON_ICON_SIZE) {
                pix = pix.scaled(NATRON_MEDIUM_BUTTON_ICON_SIZE, NATRON_MEDIUM_BUTTON_ICON_SIZE,
                                 Qt::KeepAspectRatio, Qt::SmoothTransformation);
            }

            QRect areaRect = rowRect;
            areaRect.setWidth(pix.width() + 4);
            areaRect.moveRight(rowRect.right());

            int r, g ,b;
            appPTR->getCurrentSettings()->getPluginIconFrameColor(&r, &g, &b);
            p->fillRect(areaRect, QColor(r, g, b));

            QRect pluginAreaRect = rowRect;
            pluginAreaRect.setSize(pix.size());
            pluginAreaRect.moveCenter(QPoint(areaRect.center().x(),
                                             rowRect.center().y()));

            if (drawPluginIcon) {
                p->drawPixmap(pluginAreaRect, pix);
            }
        }
    }
}

void HierarchyViewPrivate::drawColoredIndicators(QPainter *p, QTreeWidgetItem *item, const QRect &itemRect)
{
    QTreeWidgetItem *itemIt = item;

    while (itemIt) {
        boost::shared_ptr<DSNode> parentDSNode = dopeSheetModel->findParentDSNode(itemIt);
        QTreeWidgetItem *parentItem = parentDSNode->getTreeItem();
        QColor nodeColor = parentDSNode->getNodeGui()->getCurrentColor();

        QRect target = getArrowRect(parentItem);
        target.setTop(itemRect.top());
        target.setBottom(itemRect.bottom());

        p->fillRect(target, nodeColor);

        itemIt = parentItem->parent();
    }
}

void HierarchyViewPrivate::drawNodeTopSeparation(QPainter *p, QTreeWidgetItem *item, const QRect &rowRect) const
{
    int lineWidth = (appPTR->getCurrentSettings()->getDopeSheetEditorNodeSeparationWith() / 2);
    int lineBegin = q_ptr->rect().left();

    {
        boost::shared_ptr<DSNode> parentNode = dopeSheetModel->mapNameItemToDSNode(item->parent());
        if (parentNode) {
            lineBegin = getBranchRect(parentNode->getTreeItem()).right() + 2;
        }
    }

    QPen pen(Qt::black);
    pen.setWidth(lineWidth);

    p->setPen(pen);
    p->drawLine(lineBegin, rowRect.top() + lineWidth - 1,
                rowRect.right(), rowRect.top() + lineWidth - 1);
}

void HierarchyViewPrivate::drawNodeBottomSeparation(QPainter *p, boost::shared_ptr<DSNode> dsNode,
                                                    boost::shared_ptr<DSNode> nodeBelow, const QRect &rowRect) const
{
    int lineWidth = (appPTR->getCurrentSettings()->getDopeSheetEditorNodeSeparationWith() / 2);
    int lineBegin = q_ptr->rect().left();

    if (dsNode->containsNodeContext()) {
        if (dsNode->getTreeItem()->isExpanded()) {
            lineBegin = getBranchRect(dsNode->getTreeItem()).right() + 2;
        }

    } else {
        boost::shared_ptr<DSNode> parentNode = dopeSheetModel->mapNameItemToDSNode(nodeBelow->getTreeItem()->parent());
        if (parentNode) {
            lineBegin = getBranchRect(parentNode->getTreeItem()).right() + 2;
        }
    }

    QPen pen(Qt::black);
    pen.setWidth(lineWidth);

    p->setPen(pen);

    p->drawLine(lineBegin, rowRect.bottom() - lineWidth + 2,
                rowRect.right(), rowRect.bottom() - lineWidth + 2);
}

/**
 * @brief Returns a desaturated shade of 'color'.
 *
 * This function is used to paint the hierarchy view without too flashy colors.
 */
QColor HierarchyViewPrivate::getDullColor(const QColor &color) const
{
    QColor ret = color;
    ret.setAlpha(87);

    return ret;
}

int HierarchyView::getHeightForItemAndChildren(QTreeWidgetItem *item) const
{
    assert(!item->isHidden());

    // If the node item is collapsed
    if (!item->isExpanded()) {
        return visualItemRect(item).height() + 1;
    }

    // Get the "bottom-most" item
    QTreeWidgetItem *lastChild = lastVisibleChild(item);

    if (lastChild->childCount() > 0 && lastChild->isExpanded()) {
        lastChild = lastVisibleChild(lastChild);
    }

    int top = visualItemRect(item).top();
    int bottom = visualItemRect(lastChild).bottom();

    return (bottom - top) + 1;
}

/**
 * @brief Selects the keyframes associated with each item in 'items'.
 */
void HierarchyViewPrivate::selectKeyframes(const QList<QTreeWidgetItem *> &items)
{
    std::vector<DopeSheetKey> keys;
    std::vector<boost::shared_ptr<DSNode> > nodes;
    Q_FOREACH (QTreeWidgetItem *item, items) {
        boost::shared_ptr<DSKnob> knobContext = dopeSheetModel->mapNameItemToDSKnob(item);
        if (knobContext) {
            dopeSheetModel->getSelectionModel()->makeDopeSheetKeyframesForKnob(knobContext, &keys);
        } else {
            boost::shared_ptr<DSNode> nodeContext = dopeSheetModel->mapNameItemToDSNode(item);
            if (nodeContext) {
                nodes.push_back(nodeContext);
            }
        }
    }

    DopeSheetSelectionModel::SelectionTypeFlags sFlags = DopeSheetSelectionModel::SelectionTypeAdd
    | DopeSheetSelectionModel::SelectionTypeClear | DopeSheetSelectionModel::SelectionTypeRecurse;
    
    dopeSheetModel->getSelectionModel()->makeSelection(keys, nodes, sFlags);
}

HierarchyView::HierarchyView(DopeSheet *dopeSheetModel, Gui *gui, QWidget *parent) :
    QTreeWidget(parent),
    _imp(new HierarchyViewPrivate(this))
{
    _imp->dopeSheetModel = dopeSheetModel;
    _imp->gui = gui;

    header()->close();

    QTreeWidget::setSelectionModel(new HierarchyViewSelectionModel(dopeSheetModel, this, this->model(), this));

    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setColumnCount(1);
    setExpandsOnDoubleClick(false);

    setItemDelegate(new HierarchyViewItemDelegate(this));
    setAttribute(Qt::WA_MacShowFocusRect,0);

    setStyleSheet("HierarchyView { border: 0px; }");

    connect(this, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)),
            this, SLOT(onItemDoubleClicked(QTreeWidgetItem*,int)));

    connect(selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)),
            this, SLOT(onSelectionChanged()));
}

HierarchyView::~HierarchyView()
{}

void
HierarchyView::setCanResizeOtherWidget(bool canResize)
{
    _imp->_canResizeOtherWidget = canResize;
}

void
HierarchyView::resizeEvent(QResizeEvent* e)
{
    QTreeWidget::resizeEvent(e);
    if (_imp->_canResizeOtherWidget && _imp->gui->isTripleSyncEnabled()) {
        _imp->gui->setCurveEditorTreeWidth(e->size().width());
    }
}

boost::shared_ptr<DSKnob> HierarchyView::getDSKnobAt(int y) const
{
    QTreeWidgetItem *itemUnderPoint = itemAt(5, y);

    return _imp->dopeSheetModel->mapNameItemToDSKnob(itemUnderPoint);
}

bool HierarchyView::itemIsVisibleFromOutside(QTreeWidgetItem *item) const
{
    bool ret = true;

    QTreeWidgetItem *it = item->parent();

    while (it) {
        if (!it->isExpanded()) {
            ret = false;

            break;
        }

        it = it->parent();
    }

    return ret;
}

QTreeWidgetItem *HierarchyView::lastVisibleChild(QTreeWidgetItem *item) const
{
    QTreeWidgetItem *ret = 0;

    if (!item->childCount()) {
        ret = item;
    }

    for (int i = item->childCount() - 1; i >= 0; --i) {
        QTreeWidgetItem *child = item->child(i);

        if (!child->isHidden()) {
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
HierarchyView::getTreeItemForModelIndex(const QModelIndex& index) const
{
    return itemFromIndex(index);
}

void HierarchyView::drawRow(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QTreeWidgetItem *item = itemFromIndex(index);

    bool drawPluginIconToo = (item->data(0, QT_ROLE_CONTEXT_TYPE).toInt() < Natron::eDopeSheetItemTypeKnobRoot);
    bool isTreeViewTopItem = !itemAbove(item);
    boost::shared_ptr<DSNode> dsNode = _imp->dopeSheetModel->findParentDSNode(item);

    QRect rowRect = option.rect;
    QRect itemRect = visualItemRect(item);
    QRect branchRect(0, rowRect.y(), itemRect.x(), rowRect.height());

    // Draw row
    {
        painter->save();

        QColor nodeColor = dsNode->getNodeGui()->getCurrentColor();
        QColor nodeColorDull = _imp->getDullColor(nodeColor);

        QColor fillColor = nodeColorDull;

        if (drawPluginIconToo) {
            if (dsNode->containsNodeContext()) {
                fillColor = nodeColor;
            }
        }

        painter->fillRect(itemRect.adjusted(-1, 0, 0, 0), fillColor);

        // Draw the item text
        QStyleOptionViewItemV4 newOpt = viewOptions();
        newOpt.rect = itemRect;

        if (selectionModel()->isSelected(index)) {
            newOpt.state |= QStyle::State_Selected;
        }

        itemDelegate()->paint(painter, newOpt, index);

        _imp->drawColoredIndicators(painter, item, itemRect);

        // Fill the branch rect with color and indicator
        drawBranches(painter, branchRect, index);

        _imp->drawPluginIconArea(painter, dsNode, rowRect, drawPluginIconToo);

        if (drawPluginIconToo && !isTreeViewTopItem) {
            _imp->drawNodeTopSeparation(painter, item, rowRect);
        }

        {
            boost::shared_ptr<DSNode> nodeBelow = _imp->itemBelowIsNode(item);
            if (nodeBelow) {
                _imp->drawNodeBottomSeparation(painter, dsNode, nodeBelow, rowRect);
            }
        }

        painter->restore();
    }
}

void HierarchyView::drawBranches(QPainter *painter, const QRect &rect, const QModelIndex &index) const
{
    QTreeWidgetItem *item = itemFromIndex(index);
    boost::shared_ptr<DSNode> parentDSNode = _imp->dopeSheetModel->findParentDSNode(item);

    {
        QColor nodeColor = parentDSNode->getNodeGui()->getCurrentColor();
        QColor nodeColorDull = _imp->getDullColor(nodeColor);

        // Paint with a dull color to the right edge of the node branch rect
        QRect nodeItemBranchRect = _imp->getBranchRect(parentDSNode->getTreeItem());

        QRect rectForDull(nodeItemBranchRect.right(), rect.top(),
                          rect.right() - nodeItemBranchRect.right(), rect.height());
        painter->fillRect(rectForDull, nodeColorDull);

        // Draw the branch indicator
        QStyleOptionViewItemV4 option = viewOptions();
        option.rect = _imp->getParentArrowRect(item, rect);
        option.displayAlignment = Qt::AlignCenter;

        bool hasChildren = (item->childCount() && !childrenAreHidden(item));
        bool expanded = item->isExpanded();

        if (hasChildren) {
            option.state = QStyle::State_Children;
        }

        if (expanded) {
            option.state |= QStyle::State_Open;
        }

        style()->drawPrimitive(QStyle::PE_IndicatorBranch, &option, painter, this);
    }
}

bool HierarchyView::childrenAreHidden(QTreeWidgetItem *item) const
{
    for (int i = 0; i < item->childCount(); ++i) {
        if (!item->child(i)->isHidden())
            return false;
    }

    return true;
}

QTreeWidgetItem *HierarchyView::getParentItem(QTreeWidgetItem *item) const
{
    QTreeWidgetItem *ret = 0;

    QTreeWidgetItem *parentItem = item->parent();

    if (parentItem) {
        ret = parentItem;
    }
    else {
        QTreeWidget *treeWidget = item->treeWidget();
        assert(treeWidget);

        ret = treeWidget->invisibleRootItem();
    }

    return ret;
}

void HierarchyView::moveItem(QTreeWidgetItem *child, QTreeWidgetItem *newParent) const
{
    assert(newParent);

    QTreeWidgetItem *currentParent = getParentItem(child);

    currentParent->removeChild(child);
    newParent->addChild(child);
}

void HierarchyView::onNodeAdded(DSNode *dsNode)
{
    QTreeWidgetItem *treeItem = dsNode->getTreeItem();

    boost::shared_ptr<DSNode> isInputOfTimeNode = _imp->dopeSheetModel->getNearestTimeNodeFromOutputs(dsNode);
    boost::shared_ptr<DSNode> isFromGroup = _imp->dopeSheetModel->getGroupDSNode(dsNode);

    if (isInputOfTimeNode) {
        isInputOfTimeNode->getTreeItem()->addChild(treeItem);
    }

    if (isFromGroup) {
        isFromGroup->getTreeItem()->addChild(treeItem);
    }

    if (!isInputOfTimeNode && !isFromGroup) {
        addTopLevelItem(treeItem);
    }

    std::vector<boost::shared_ptr<DSNode> > importantNodes = _imp->dopeSheetModel->getImportantNodes(dsNode);
    for (std::vector<boost::shared_ptr<DSNode> >::const_iterator it = importantNodes.begin();
         it != importantNodes.end();
         ++it) {
        boost::shared_ptr<DSNode> n = (*it);

        moveItem(n->getTreeItem(), dsNode->getTreeItem());

        _imp->checkKnobsVisibleState(n.get());
        _imp->checkNodeVisibleState(n.get());

        n->getTreeItem()->setExpanded(true);
    }

    _imp->checkKnobsVisibleState(dsNode);
    _imp->checkNodeVisibleState(dsNode);

    treeItem->setExpanded(true);
}

void HierarchyView::onNodeAboutToBeRemoved(DSNode *dsNode)
{
    QTreeWidgetItem *treeItem = dsNode->getTreeItem();

    // Put the child node items to the upper level
    QList<QTreeWidgetItem *> toMove;

    for (int i = 0; i < treeItem->childCount(); ++i) {
        QTreeWidgetItem *child = treeItem->child(i);

        if (child->data(0, QT_ROLE_CONTEXT_TYPE).toInt() < Natron::eDopeSheetItemTypeKnobRoot) {
            toMove << child;
        }
    }

    QTreeWidgetItem *newParent = getParentItem(treeItem);

    Q_FOREACH (QTreeWidgetItem *nodeItem, toMove) {
        moveItem(nodeItem, newParent);

        boost::shared_ptr<DSNode> dss = _imp->dopeSheetModel->mapNameItemToDSNode(nodeItem);
        _imp->checkKnobsVisibleState(dss.get());
        _imp->checkNodeVisibleState(dss.get());

        nodeItem->setExpanded(true);
    }

    // Remove the item from the tree
    getParentItem(treeItem)->removeChild(treeItem);
}

void HierarchyView::onKeyframeSetOrRemoved(DSKnob *dsKnob)
{
    _imp->checkKnobVisibleState(dsKnob);

    // Check the node item
    boost::shared_ptr<DSNode> parentNode = _imp->dopeSheetModel->findParentDSNode(dsKnob->getTreeItem());
    _imp->checkNodeVisibleState(parentNode.get());
}

void HierarchyView::onKeyframeSelectionChanged(bool recurse)
{
    HierarchyViewSelectionModel *mySelecModel
            = dynamic_cast<HierarchyViewSelectionModel *>(selectionModel());
    assert(mySelecModel);
    
    // Retrieve the knob contexts with selected keyframes
    DSKnobPtrList toCheck;
    DSKeyPtrList selectedKeys;
    std::vector<boost::shared_ptr<DSNode> > selectedNodes;
    
    _imp->dopeSheetModel->getSelectionModel()->getCurrentSelection(&selectedKeys, &selectedNodes);
    
    for (DSKeyPtrList::const_iterator it = selectedKeys.begin();
         it != selectedKeys.end();
         ++it) {
        toCheck.push_back((*it)->getContext());
    }
    
    
    disconnect(selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)),
               this, SLOT(onSelectionChanged()));

    // Compose tree selection from the selected keyframes
    if (toCheck.empty() && selectedNodes.empty()) {
        mySelecModel->selectWithRecursion(QItemSelection(), QItemSelectionModel::Clear, recurse);
    }
    else {
        std::set<QModelIndex> toSelect;

        for (DSKnobPtrList::const_iterator toCheckIt = toCheck.begin();
             toCheckIt != toCheck.end();
             ++toCheckIt) {
            boost::shared_ptr<DSKnob> dsKnob = (*toCheckIt);

            KnobGui *knobGui = dsKnob->getKnobGui();
            int dim = dsKnob->getDimension();

            KeyFrameSet keyframes = knobGui->getCurve(dim)->getKeyFrames_mt_safe();

            bool selectItem = true;

            for (KeyFrameSet::const_iterator kfIt = keyframes.begin();
                 kfIt != keyframes.end();
                 ++kfIt) {
                KeyFrame key = (*kfIt);

                if (!_imp->dopeSheetModel->getSelectionModel()->keyframeIsSelected(dsKnob, key)) {
                    selectItem = false;

                    break;
                }
            }

            QTreeWidgetItem *knobItem = dsKnob->getTreeItem();

            if (selectItem) {
                toSelect.insert(indexFromItem(knobItem));
            }
        }
        
        for (std::vector<boost::shared_ptr<DSNode> >::iterator it = selectedNodes.begin(); it!=selectedNodes.end(); ++it) {
            toSelect.insert(indexFromItem((*it)->getTreeItem()));
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

    connect(selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)),
            this, SLOT(onSelectionChanged()));
}

void HierarchyView::onItemDoubleClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);

    boost::shared_ptr<DSNode> itemDSNode = _imp->dopeSheetModel->findParentDSNode(item);

    boost::shared_ptr<NodeGui> nodeGui = itemDSNode->getNodeGui();

    // Move the nodeGui's settings panel on top
    DockablePanel *panel = 0;

    if (nodeGui) {
        nodeGui->ensurePanelCreated();
    }

    if (nodeGui && nodeGui->getParentMultiInstance()) {
        panel = nodeGui->getParentMultiInstance()->getSettingPanel();
    }
    else {
        panel = nodeGui->getSettingPanel();
    }

    if (nodeGui && panel && nodeGui->isVisible()) {
        if ( !nodeGui->isSettingsPanelVisible() ) {
            nodeGui->setVisibleSettingsPanel(true);
        }

        if ( !nodeGui->wasBeginEditCalled() ) {
            nodeGui->beginEditKnobs();
        }

        _imp->gui->putSettingsPanelFirst(nodeGui->getSettingPanel());
        _imp->gui->getApp()->redrawAllViewers();
    }
}

void HierarchyView::onSelectionChanged()
{
    _imp->selectKeyframes(selectedItems());
}

NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_DopeSheetHierarchyView.cpp"
