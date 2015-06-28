#include "DopeSheetHierarchyView.h"

#include <QDebug>
#include <QHeaderView>
#include <QPainter>
#include <QStyleOption>

#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/Settings.h"

#include "Gui/DockablePanel.h"
#include "Gui/DopeSheet.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/KnobGui.h"
#include "Gui/NodeGui.h"


////////////////////////// Helpers //////////////////////////

namespace {

/**
 * @brief itemHasNoChildVisible
 *
 * Returns true if all childs of 'item' are hidden, otherwise returns
 * false.
 */
bool childrenAreHidden(QTreeWidgetItem *item)
{
    for (int i = 0; i < item->childCount(); ++i) {
        if (!item->child(i)->isHidden())
            return false;
    }

    return true;
}

QTreeWidgetItem *getParentItem(QTreeWidgetItem *item)
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

void moveItem(QTreeWidgetItem *child, QTreeWidgetItem *newParent)
{
    assert(newParent);

    QTreeWidgetItem *currentParent = getParentItem(child);

    currentParent->removeChild(child);
    newParent->addChild(child);
}

} // anon namespace


////////////////////////// HierarchyViewSelectionModel //////////////////////////


HierarchyViewSelectionModel::HierarchyViewSelectionModel(QAbstractItemModel *model,
                                                         QObject *parent) :
    QItemSelectionModel(model, parent)
{
    connect(model, SIGNAL(destroyed()),
            this, SLOT(deleteLater()));
}

HierarchyViewSelectionModel::~HierarchyViewSelectionModel()
{

}

void HierarchyViewSelectionModel::select(const QItemSelection &selection, QItemSelectionModel::SelectionFlags command)
{
    QItemSelection newSelection = selection;

    // Select childrens
    QItemSelection childrenSelection;

    Q_FOREACH (QModelIndex index, selection.indexes()) {
        recursiveSelect(index, childrenSelection);
    }

    newSelection.merge(childrenSelection, command);

    QItemSelectionModel::select(newSelection, command);
}

void HierarchyViewSelectionModel::recursiveSelect(const QModelIndex &index, QItemSelection &selection) const
{
    int row = 0;
    QModelIndex childIndex = index.child(row, 0);

    while (childIndex.isValid()) {
        selection.select(childIndex, childIndex);

        // /!\ recursion
        {
            recursiveSelect(childIndex, selection);
        }

        ++row;
        childIndex = index.child(row, 0);
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

    DopeSheet::ItemType nodeType = DopeSheet::ItemType(index.data(QT_ROLE_CONTEXT_TYPE).toInt());
    int heightOffset = 0;

    switch (nodeType) {
    case DopeSheet::ItemTypeReader:
    case DopeSheet::ItemTypeRetime:
    case DopeSheet::ItemTypeTimeOffset:
    case DopeSheet::ItemTypeFrameRange:
    case DopeSheet::ItemTypeGroup:
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

    boost::shared_ptr<Settings> appSettings = appPTR->getCurrentSettings();
    double r, g, b;

    if (option.state & QStyle::State_Selected) {
        appSettings->getTextColor(&r, &g, &b);
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
};

HierarchyViewPrivate::HierarchyViewPrivate(HierarchyView *qq) :
    q_ptr(qq),
    dopeSheetModel(0),
    gui(0)
{}

HierarchyViewPrivate::~HierarchyViewPrivate()
{}

void HierarchyViewPrivate::checkKnobsVisibleState(DSNode *dsNode)
{
    DSTreeItemKnobMap knobRows = dsNode->getChildData();

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

void HierarchyViewPrivate::checkNodeVisibleState(DSNode *dsNode)
{
    boost::shared_ptr<NodeGui> nodeGui = dsNode->getNodeGui();

    bool showNode = nodeGui->isSettingsPanelVisible();

    DopeSheet::ItemType nodeType = dsNode->getItemType();

    if (nodeType == DopeSheet::ItemTypeCommon) {
        showNode = nodeHasAnimation(nodeGui);
    }
    else if (nodeType == DopeSheet::ItemTypeGroup) {
        NodeGroup *group = dynamic_cast<NodeGroup *>(nodeGui->getNode()->getLiveInstance());

        showNode = showNode && (!dopeSheetModel->groupSubNodesAreHidden(group) || !dsNode->getTreeItem()->childCount());
    }

    dsNode->getTreeItem()->setHidden(!showNode);

    // Hide the parent group item if there's no subnodes displayed
    if (boost::shared_ptr<DSNode> parentGroupDSNode = dopeSheetModel->getGroupDSNode(dsNode)) {
        checkNodeVisibleState(parentGroupDSNode.get());
    }
}

void HierarchyViewPrivate::checkKnobVisibleState(DSKnob *dsKnob)
{
    int dim = dsKnob->getDimension();
    assert(dim <= 0);

    KnobGui *knobGui = dsKnob->getKnobGui();

    bool showContext = false;

    if (dsKnob->isMultiDimRoot()) {
        for (int i = 0; i < knobGui->getKnob()->getDimension(); ++i) {
            bool curveIsAnimated = knobGui->getCurve(i)->isAnimated();

            QTreeWidgetItem *dimItem = dsKnob->findDimTreeItem(i);
            dimItem->setHidden(!curveIsAnimated);
            dimItem->setData(0, QT_ROLE_CONTEXT_IS_ANIMATED,curveIsAnimated);

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

QRect HierarchyViewPrivate::getBranchRect(QTreeWidgetItem *item) const
{
    QRect itemRect = q_ptr->visualItemRect(item);

    return QRect(q_ptr->rect().left(), itemRect.top(),
                 itemRect.left(), itemRect.height());
}

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

boost::shared_ptr<DSNode> HierarchyViewPrivate::itemBelowIsNode(QTreeWidgetItem *item) const
{
    boost::shared_ptr<DSNode> ret;

    QTreeWidgetItem *itemBelow = q_ptr->itemBelow(item);

    if (itemBelow) {
        ret = dopeSheetModel->findDSNode(itemBelow);
    }

    return ret;
}

void HierarchyViewPrivate::drawPluginIconArea(QPainter *p, boost::shared_ptr<DSNode> dsNode, const QRect &rowRect, bool drawPluginIcon) const
{
    std::string iconFilePath = dsNode->getInternalNode()->getPluginIconFilePath();

    if (!iconFilePath.empty()) {
        QPixmap pix;

        if (pix.load(iconFilePath.c_str())) {
            pix = pix.scaled(NATRON_MEDIUM_BUTTON_SIZE - 2, NATRON_MEDIUM_BUTTON_SIZE - 2,
                             Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

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

    if (boost::shared_ptr<DSNode> parentNode = dopeSheetModel->findDSNode(item->parent())) {
        lineBegin = getBranchRect(parentNode->getTreeItem()).right() + 2;
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
    }
    else if (boost::shared_ptr<DSNode> parentNode = dopeSheetModel->findDSNode(nodeBelow->getTreeItem()->parent())) {
        lineBegin = getBranchRect(parentNode->getTreeItem()).right() + 2;
    }

    QPen pen(Qt::black);
    pen.setWidth(lineWidth);

    p->setPen(pen);

    p->drawLine(lineBegin, rowRect.bottom() - lineWidth + 2,
                rowRect.right(), rowRect.bottom() - lineWidth + 2);
}

QColor HierarchyViewPrivate::getDullColor(const QColor &color) const
{
    QColor ret = color;
    ret.setAlpha(87);

    return ret;
}

void HierarchyViewPrivate::selectKeyframes(const QList<QTreeWidgetItem *> &items)
{
    if (items.empty()) {
        dopeSheetModel->getSelectionModel()->clearKeyframeSelection();
    }

    std::vector<DopeSheetKey> keys;

    Q_FOREACH (QTreeWidgetItem *item, items) {
        boost::shared_ptr<DSKnob> knobContext = dopeSheetModel->findDSKnob(item);

        if (knobContext) {
            dopeSheetModel->getSelectionModel()->selectKeyframes(knobContext, &keys);
        }
    }

    dopeSheetModel->getSelectionModel()->makeSelection(keys, DopeSheetSelectionModel::SelectionTypeOneByOne);
}

/**
 * @brief HierarchyView::HierarchyView
 *
 *
 */
HierarchyView::HierarchyView(DopeSheet *dopeSheetModel, Gui *gui, QWidget *parent) :
    QTreeWidget(parent),
    _imp(new HierarchyViewPrivate(this))
{
    _imp->dopeSheetModel = dopeSheetModel;
    _imp->gui = gui;

    connect(dopeSheetModel, SIGNAL(nodeAdded(DSNode *)),
            this, SLOT(onNodeAdded(DSNode *)));

    connect(dopeSheetModel, SIGNAL(nodeAboutToBeRemoved(DSNode *)),
            this, SLOT(onNodeAboutToBeRemoved(DSNode *)));

    connect(dopeSheetModel, SIGNAL(keyframeSetOrRemoved(DSKnob *)),
            this, SLOT(onKeyframeSetOrRemoved(DSKnob *)));

    connect(this, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)),
            this, SLOT(onItemDoubleClicked(QTreeWidgetItem*,int)));

    header()->close();

    QTreeWidget::setSelectionModel(new HierarchyViewSelectionModel(this->model(), this));

    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setColumnCount(1);
    setExpandsOnDoubleClick(false);

    setItemDelegate(new HierarchyViewItemDelegate(this));

    setStyleSheet("HierarchyView { border: 0px; }");
}

HierarchyView::~HierarchyView()
{}

boost::shared_ptr<DSKnob> HierarchyView::getDSKnobAt(int y) const
{
    QTreeWidgetItem *itemUnderPoint = itemAt(5, y);

    return _imp->dopeSheetModel->findDSKnob(itemUnderPoint);
}

void HierarchyView::drawRow(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QTreeWidgetItem *item = itemFromIndex(index);

    bool drawPluginIconToo = false;
    boost::shared_ptr<DSNode> dsNode = _imp->dopeSheetModel->getDSNodeFromItem(item, &drawPluginIconToo);

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

        if (drawPluginIconToo) {
            _imp->drawNodeTopSeparation(painter, item, rowRect);
        }

        if (boost::shared_ptr<DSNode> nodeBelow = _imp->itemBelowIsNode(item)) {
            _imp->drawNodeBottomSeparation(painter, dsNode, nodeBelow, rowRect);
        }

        painter->restore();
    }
}

void HierarchyView::drawBranches(QPainter *painter, const QRect &rect, const QModelIndex &index) const
{
    QTreeWidgetItem *item = itemFromIndex(index);
    boost::shared_ptr<DSNode> parentDSNode = _imp->dopeSheetModel->getDSNodeFromItem(item);

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
    QList<QTreeWidgetItem *> toPut;

    for (int i = 0; i < treeItem->childCount(); ++i) {
        QTreeWidgetItem *child = treeItem->child(i);

        if (child->data(0, QT_ROLE_CONTEXT_TYPE).toInt() < DopeSheet::ItemTypeKnobRoot) {
            toPut << child;
        }
    }

    QTreeWidgetItem *newParent = getParentItem(treeItem);

    Q_FOREACH (QTreeWidgetItem *nodeItem, toPut) {
        moveItem(nodeItem, newParent);

        boost::shared_ptr<DSNode> dss = _imp->dopeSheetModel->findDSNode(nodeItem);
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

/**
 * @brief DopeSheetEditor::onItemDoubleClicked
 *
 * Ensures that the node panel associated with 'item' is the top-most displayed
 * in the Properties panel.
 *
 * This slot is automatically called when an item is double clicked in the
 * hierarchy view.
 */
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
