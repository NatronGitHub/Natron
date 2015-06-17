#include "DopeSheetView.h"

#include <algorithm>

// Qt includes
#include <QApplication>
#include <QHeaderView>
#include <QMouseEvent>
#include <QScrollBar>
#include <QStyledItemDelegate>
#include <QStyleOption>
#include <QThread>
#include <QToolButton>

// Natron includes
#include "Engine/Curve.h"
#include "Engine/Image.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/Project.h"
#include "Engine/Settings.h"

#include "Global/Enums.h"

#include "Gui/ActionShortcuts.h"
#include "Gui/CurveEditor.h"
#include "Gui/CurveWidget.h"
#include "Gui/DockablePanel.h"
#include "Gui/DopeSheet.h"
#include "Gui/DopeSheetEditorUndoRedo.h"
#include "Gui/Gui.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiMacros.h"
#include "Gui/Histogram.h"
#include "Gui/KnobGui.h"
#include "Gui/Menu.h"
#include "Gui/NodeGraph.h"
#include "Gui/NodeGui.h"
#include "Gui/TextRenderer.h"
#include "Gui/ticks.h"
#include "Gui/ViewerGL.h"
#include "Gui/ViewerTab.h"
#include "Gui/ZoomContext.h"


// Typedefs
typedef std::set<double> TimeSet;
typedef std::pair<double, double> FrameRange;
typedef std::map<boost::weak_ptr<KnobI>, KnobGui *> KnobsAndGuis;


// Constants
const int KF_TEXTURES_COUNT = 18;
const int KF_PIXMAP_SIZE = 14;
const int KF_X_OFFSET = KF_PIXMAP_SIZE / 2;
const int DISTANCE_ACCEPTANCE_FROM_KEYFRAME = 5;
const int DISTANCE_ACCEPTANCE_FROM_READER_EDGE = 14;
const int DISTANCE_ACCEPTANCE_FROM_READER_BOTTOM = 8;

const int NODE_SEPARATION_WIDTH = 4;


////////////////////////// Helpers //////////////////////////

namespace {

void running_in_main_thread() {
    assert(qApp && qApp->thread() == QThread::currentThread());
}

void running_in_main_context(const QGLWidget *glWidget) {
    assert(glWidget->context() == QGLContext::currentContext());
    Q_UNUSED(glWidget);
}

void running_in_main_thread_and_context(const QGLWidget *glWidget) {
    running_in_main_thread();
    running_in_main_context(glWidget);
}

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

bool itemIsVisibleFromOutside(QTreeWidgetItem *item)
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

int firstVisibleParentCenterY(QTreeWidgetItem * item)
{
    int ret = 0;

    QTreeWidgetItem *it = item->parent();

    while (it) {
        assert(it->treeWidget());

        QRect itemRect = it->treeWidget()->visualItemRect(it);

        if (itemRect.isNull()) {
            it = it->parent();
        }
        else {
            ret = itemRect.center().y();

            break;
        }
    }

    return ret;
}

QTreeWidgetItem *lastVisibleChild(QTreeWidgetItem *item)
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

} // anon namespace


////////////////////////// HierarchyViewDelegate //////////////////////////

/**
 * @brief The HierarchyViewItemDelegate class
 *
 *
 */

class HierarchyViewItemDelegate : public QStyledItemDelegate
{
public:
    explicit HierarchyViewItemDelegate(HierarchyView *hierarchyView);

    virtual QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const OVERRIDE FINAL;
    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const OVERRIDE FINAL;

private:
    HierarchyView *m_hierarchyView;
};


/**
 * @brief HierarchyViewItemDelegate::HierarchyViewItemDelegate
 *
 *
 */
HierarchyViewItemDelegate::HierarchyViewItemDelegate(HierarchyView *hierarchyView) :
    QStyledItemDelegate(hierarchyView),
    m_hierarchyView(hierarchyView)
{}

/**
 * @brief HierarchyViewItemDelegate::sizeHint
 *
 *
 */
QSize HierarchyViewItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option);

    QTreeWidgetItem *item = m_hierarchyView->itemFromIndex(index);

    QSize itemSize = QStyledItemDelegate::sizeHint(option, index);

    DopeSheet::ItemType nodeType = DopeSheet::ItemType(item->type());
    int newItemHeight = 0;

    switch (nodeType) {
    case DopeSheet::ItemTypeReader:
    case DopeSheet::ItemTypeRetime:
    case DopeSheet::ItemTypeTimeOffset:
    case DopeSheet::ItemTypeFrameRange:
    case DopeSheet::ItemTypeGroup:
        newItemHeight = 10;
        break;
    default:
        break;
    }

    itemSize.rheight() += newItemHeight;

    return itemSize;
}

void HierarchyViewItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QTreeWidgetItem *item = m_hierarchyView->itemFromIndex(index);

    painter->save();

    boost::shared_ptr<Settings> appSettings = appPTR->getCurrentSettings();
    double r, g, b;

    if (item->isSelected()) {
        appSettings->getTextColor(&r, &g, &b);
    }
    else {
        r = g = b = 0.11f;
    }

    painter->setPen(QColor::fromRgbF(r, g, b));
    painter->drawText(option.rect, Qt::AlignVCenter, item->text(0));

    painter->restore();
}


////////////////////////// HierarchyView //////////////////////////

class HierarchyViewPrivate
{
public:
    HierarchyViewPrivate(HierarchyView *qq);
    ~HierarchyViewPrivate();

    /* functions */
    void insertNodeItem(DSNode *dsNode);
    void checkKnobsVisibleState(DSNode *dsNode);
    void processChildNodes(DSNode *dsNode);

    DSNode *getDSNodeFromItem(QTreeWidgetItem *item, bool *itemIsNode = 0) const;
    QTreeWidgetItem *getParentItem(QTreeWidgetItem *item) const;
    int indexInParent(QTreeWidgetItem *item) const;
    void moveChildTo(DSNode *child, DSNode *newParent);

    void checkNodeVisibleState(DSNode *dsNode);
    void checkKnobVisibleState(DSKnob *dsKnob);

    void recursiveSelect(QTreeWidgetItem *item);

    DSNode *itemBelowIsNode(QTreeWidgetItem *item) const;

    QRect getBranchRect(QTreeWidgetItem *item) const;
    QRect getArrowRect(QTreeWidgetItem *item) const;
    QRect getParentArrowRect(QTreeWidgetItem *item, const QRect &branchRect) const;

    QColor getDullColor(const QColor &color) const;

    // item painting
    void drawPluginIconArea(QPainter *p, DSNode *dsNode, const QRect &rowRect, bool drawPluginIcon) const;
    void drawColoredIndicators(QPainter *p, QTreeWidgetItem *item, const QRect &itemRect);
    void drawNodeTopSeparation(QPainter *p, QTreeWidgetItem *item, const QRect &rowRect) const;
    void drawNodeBottomSeparation(QPainter *p, DSNode *dsNode, DSNode *nodeBelow, const QRect &rowRect) const;

    // keyframe selection
    void selectKeyframes(const QList<QTreeWidgetItem *> &items);

    void blockSelectionSignals(bool block);

    /* attributes */
    HierarchyView *q_ptr;
    DopeSheet *model;

    Gui *gui;
};

HierarchyViewPrivate::HierarchyViewPrivate(HierarchyView *qq) :
    q_ptr(qq),
    model(0),
    gui(0)
{}

HierarchyViewPrivate::~HierarchyViewPrivate()
{}

void HierarchyViewPrivate::insertNodeItem(DSNode *dsNode)
{
    QTreeWidgetItem *treeItem = dsNode->getTreeItem();
    DopeSheet::ItemType nodeType = dsNode->getItemType();

    DSNode *isAffectedByTimeNode = model->getNearestTimeNodeFromOutputs(dsNode);
    DSNode *isFromGroup = model->getGroupDSNode(dsNode);

    if (isAffectedByTimeNode) {
        isAffectedByTimeNode->getTreeItem()->addChild(treeItem);
    }

    if (isFromGroup) {
        isFromGroup->getTreeItem()->addChild(treeItem);
    }

    if (!isAffectedByTimeNode && !isFromGroup) {
        q_ptr->addTopLevelItem(treeItem);
    }

    if (dsNode->isTimeNode()) {
        std::vector<DSNode *> affectedNodes = model->getInputsConnected(dsNode);
        for (std::vector<DSNode *>::const_iterator it = affectedNodes.begin();
             it != affectedNodes.end();
             ++it) {
            DSNode *input = (*it);

            moveChildTo(input, dsNode);
            checkKnobsVisibleState(input);
            input->getTreeItem()->setExpanded(true);
        }
    }
    else if (nodeType == DopeSheet::ItemTypeGroup) {
        std::vector<DSNode *> childNodes = model->getNodesFromGroup(dsNode);

        for (std::vector<DSNode *>::const_iterator it = childNodes.begin(); it != childNodes.end(); ++it) {
            DSNode *child = (*it);

            moveChildTo(child, dsNode);
            checkKnobsVisibleState(child);
            child->getTreeItem()->setExpanded(true);
        }
    }
}

void HierarchyViewPrivate::checkKnobsVisibleState(DSNode *dsNode)
{
    DSKnobRows knobRows = dsNode->getChildData();

    for (DSKnobRows::const_iterator it = knobRows.begin(); it != knobRows.end(); ++it) {
        DSKnob *dsKnob = (*it).second;
        QTreeWidgetItem *knobItem = (*it).first;

        // Expand if it's a multidim root item
        if (knobItem->childCount() > 0) {
            knobItem->setExpanded(true);
        }

        if (dsKnob->getDimension() <= 0) {
            checkKnobVisibleState(dsKnob);
        }
    }
}

void HierarchyViewPrivate::processChildNodes(DSNode *dsNode)
{
    QTreeWidgetItem *treeItem = dsNode->getTreeItem();

    // Put its child node items at the up level
    for (int i = treeItem->childCount() - 1; i >= 0; --i) {
        QTreeWidgetItem *child = treeItem->child(i);

        if (DSNode *nodeToMove = model->findDSNode(child)) {
            QTreeWidgetItem *newParent = getParentItem(treeItem);

            treeItem->takeChild(treeItem->indexOfChild(child));
            newParent->addChild(child);

            child->setExpanded(true);
            checkKnobsVisibleState(nodeToMove);
        }
    }
}

DSNode *HierarchyViewPrivate::getDSNodeFromItem(QTreeWidgetItem *item, bool *itemIsNode) const
{
    DSNode *dsNode = model->findDSNode(item);

    if (!dsNode) {
        dsNode = model->findParentDSNode(item);
    }
    else if (itemIsNode) {
        *itemIsNode = true;
    }

    return dsNode;
}

QTreeWidgetItem *HierarchyViewPrivate::getParentItem(QTreeWidgetItem *item) const
{
    return (item->parent()) ? item->parent() : q_ptr->invisibleRootItem();
}

int HierarchyViewPrivate::indexInParent(QTreeWidgetItem *item) const
{
    QTreeWidgetItem *parentItem = getParentItem(item);

    return parentItem->indexOfChild(item);
}

void HierarchyViewPrivate::moveChildTo(DSNode *child, DSNode *newParent)
{
    QTreeWidgetItem *newParentItem = newParent->getTreeItem();
    assert(newParent && newParentItem);

    QTreeWidgetItem *childItem = child->getTreeItem();
    QTreeWidgetItem *currentParent = getParentItem(childItem);

    childItem = currentParent->takeChild(indexInParent(childItem));

    newParentItem->addChild(childItem);
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

        showNode = showNode && (!model->groupSubNodesAreHidden(group) || !dsNode->getTreeItem()->childCount());
    }

    dsNode->getTreeItem()->setHidden(!showNode);

    // Hide the parent group item if there's no subnodes displayed
    if (DSNode *parentGroupDSNode = model->getGroupDSNode(dsNode)) {
        checkNodeVisibleState(parentGroupDSNode);
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

            dsKnob->findDimTreeItem(i)->setHidden(!curveIsAnimated);

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

    // Check the node item
    DSNode *parentNode = model->findParentDSNode(treeItem);
    checkNodeVisibleState(parentNode);
}

/**
 * @brief recursiveSelect
 *
 * Performs a recursive selection on 'item' 's chilren.
 */
void HierarchyViewPrivate::recursiveSelect(QTreeWidgetItem *item)
{
    if (item->childCount() > 0 && !childrenAreHidden(item)) {
        for (int i = 0; i < item->childCount(); ++i) {
            QTreeWidgetItem *childItem = item->child(i);
            childItem->setSelected(true);

            // /!\ recursion
            recursiveSelect(childItem);
        }
    }
}

DSNode *HierarchyViewPrivate::itemBelowIsNode(QTreeWidgetItem *item) const
{
    DSNode *ret = 0;

    QTreeWidgetItem *itemBelow = q_ptr->itemBelow(item);

    if (itemBelow) {
        ret = model->findDSNode(itemBelow);
    }

    return ret;
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

QColor HierarchyViewPrivate::getDullColor(const QColor &color) const
{
    QColor ret = color;
    ret.setAlpha(87);

    return ret;
}

void HierarchyViewPrivate::drawPluginIconArea(QPainter *p, DSNode *dsNode, const QRect &rowRect, bool drawPluginIcon) const
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
        DSNode *parentDSNode = model->findParentDSNode(itemIt);
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
    int lineWidth = (NODE_SEPARATION_WIDTH / 2);
    int lineBegin = q_ptr->rect().left();

    if (DSNode *parentNode = model->findDSNode(item->parent())) {
        lineBegin = getBranchRect(parentNode->getTreeItem()).right() + 2;
    }

    QPen pen(Qt::black);
    pen.setWidth(lineWidth);

    p->setPen(pen);
    p->drawLine(lineBegin, rowRect.top() + lineWidth - 1,
                rowRect.right(), rowRect.top() + lineWidth - 1);
}

void HierarchyViewPrivate::drawNodeBottomSeparation(QPainter *p, DSNode *dsNode,
                                                    DSNode *nodeBelow, const QRect &rowRect) const
{
    int lineWidth = (NODE_SEPARATION_WIDTH / 2);
    int lineBegin = q_ptr->rect().left();

    if (dsNode->containsNodeContext()) {
        if (dsNode->getTreeItem()->isExpanded()) {
            lineBegin = getBranchRect(dsNode->getTreeItem()).right() + 2;
        }
    }
    else if (DSNode *parentNode = model->findDSNode(nodeBelow->getTreeItem()->parent())) {
        lineBegin = getBranchRect(parentNode->getTreeItem()).right() + 2;
    }

    QPen pen(Qt::black);
    pen.setWidth(lineWidth);

    p->setPen(pen);

    p->drawLine(lineBegin, rowRect.bottom() - lineWidth + 2,
                rowRect.right(), rowRect.bottom() - lineWidth + 2);
}

void HierarchyViewPrivate::selectKeyframes(const QList<QTreeWidgetItem *> &items)
{
    std::vector<DSSelectedKey> keys;

    Q_FOREACH (QTreeWidgetItem *item, items) {
        DSKnob *dsKnob = model->findDSKnob(item);

        if (dsKnob) {
            model->getSelectionModel()->selectKeyframes(dsKnob, &keys);
        }
    }

    model->getSelectionModel()->makeSelection(keys);
}

void HierarchyViewPrivate::blockSelectionSignals(bool block)
{
    if (block) {
        QObject::disconnect(q_ptr, SIGNAL(itemSelectionChanged()),
                            q_ptr, SLOT(onItemSelectionChanged()));
    }
    else {
        QObject::connect(q_ptr, SIGNAL(itemSelectionChanged()),
                         q_ptr, SLOT(onItemSelectionChanged()));
    }
}

/**
 * @brief HierarchyView::HierarchyView
 *
 *
 */
HierarchyView::HierarchyView(DopeSheet *model, Gui *gui, QWidget *parent) :
    QTreeWidget(parent),
    _imp(new HierarchyViewPrivate(this))
{
    connect(model, SIGNAL(nodeAdded(DSNode *)),
            this, SLOT(onNodeAdded(DSNode *)));

    connect(model, SIGNAL(nodeAboutToBeRemoved(DSNode*)),
            this, SLOT(onNodeAboutToBeRemoved(DSNode*)));

    connect(model, SIGNAL(keyframeSetOrRemoved(DSKnob*)),
            this, SLOT(onKeyframeSetOrRemoved(DSKnob *)));

    connect(this, SIGNAL(itemSelectionChanged()),
            this, SLOT(onItemSelectionChanged()));

    connect(this, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)),
            this, SLOT(onItemDoubleClicked(QTreeWidgetItem*,int)));

    _imp->model = model;
    _imp->gui = gui;

    header()->close();

    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setColumnCount(1);
    setExpandsOnDoubleClick(false);

    setItemDelegate(new HierarchyViewItemDelegate(this));

    setStyleSheet("HierarchyView { border: 0px; }");
}

HierarchyView::~HierarchyView()
{}

DSKnob *HierarchyView::getDSKnobAt(int y) const
{
    QTreeWidgetItem *itemUnderPoint = itemAt(5, y);

    return _imp->model->findDSKnob(itemUnderPoint);
}

void HierarchyView::onNodeAdded(DSNode *dsNode)
{
    _imp->insertNodeItem(dsNode);
    _imp->checkKnobsVisibleState(dsNode);

    dsNode->getTreeItem()->setExpanded(true);

    _imp->checkNodeVisibleState(dsNode);
}

void HierarchyView::onNodeAboutToBeRemoved(DSNode *dsNode)
{
    QTreeWidgetItem *treeItem = dsNode->getTreeItem();

    _imp->processChildNodes(dsNode);

    QTreeWidgetItem *treeItemParent = _imp->getParentItem(treeItem);
    treeItemParent->removeChild(treeItem);

    _imp->checkNodeVisibleState(dsNode);
}

void HierarchyView::onKeyframeSetOrRemoved(DSKnob *dsKnob)
{
    _imp->checkKnobVisibleState(dsKnob);
}

void HierarchyView::onItemSelectionChanged()
{
    _imp->blockSelectionSignals(true);

    if (selectedItems().empty()) {
        _imp->model->getSelectionModel()->clearKeyframeSelection();
    }
    else {
        Q_FOREACH (QTreeWidgetItem *item, selectedItems()) {
            _imp->recursiveSelect(item);
        }

        _imp->selectKeyframes(selectedItems());
    }

    _imp->blockSelectionSignals(false);
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

    DSNode *itemDSNode = _imp->model->findParentDSNode(item);

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

void HierarchyView::drawRow(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QTreeWidgetItem *item = itemFromIndex(index);

    bool drawPluginIconToo = false;
    DSNode *dsNode = _imp->getDSNodeFromItem(item, &drawPluginIconToo);

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

        itemDelegate()->paint(painter, newOpt, index);

        _imp->drawColoredIndicators(painter, item, itemRect);

        // Fill the branch rect with color and indicator
        drawBranches(painter, branchRect, index);

        _imp->drawPluginIconArea(painter, dsNode, rowRect, drawPluginIconToo);

        if (drawPluginIconToo) {
            _imp->drawNodeTopSeparation(painter, item, rowRect);
        }

        if (DSNode *nodeBelow = _imp->itemBelowIsNode(item)) {
            _imp->drawNodeBottomSeparation(painter, dsNode, nodeBelow, rowRect);
        }

        painter->restore();
    }
}

void HierarchyView::drawBranches(QPainter *painter, const QRect &rect, const QModelIndex &index) const
{
    QTreeWidgetItem *item = itemFromIndex(index);
    DSNode *parentDSNode = _imp->getDSNodeFromItem(item);

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


////////////////////////// DopeSheetView //////////////////////////

class DopeSheetViewPrivate
{
public:
    enum KeyframeTexture {
        kfTextureNone = -2,
        kfTextureInterpConstant = 0,
        kfTextureInterpConstantSelected,

        kfTextureInterpLinear,
        kfTextureInterpLinearSelected,

        kfTextureInterpCurve,
        kfTextureInterpCurveSelected,

        kfTextureInterpBreak,
        kfTextureInterpBreakSelected,

        kfTextureInterpCurveC,
        kfTextureInterpCurveCSelected,

        kfTextureInterpCurveH,
        kfTextureInterpCurveHSelected,

        kfTextureInterpCurveR,
        kfTextureInterpCurveRSelected,

        kfTextureInterpCurveZ,
        kfTextureInterpCurveZSelected,

        kfTextureRoot,
        kfTextureRootSelected,
    };

    DopeSheetViewPrivate(DopeSheetView *qq);
    ~DopeSheetViewPrivate();

    /* functions */

    // Helpers
    QRectF keyframeRect(double keyTime, double y) const;

    QRectF rectToZoomCoordinates(const QRectF &rect) const;
    QRectF rectToWidgetCoordinates(const QRectF &rect) const;
    QRectF nameItemRectToRowRect(const QRectF &rect) const;

    Qt::CursorShape getCursorDuringHover(const QPointF &widgetCoords) const;
    Qt::CursorShape getCursorForEventState(DopeSheetView::EventStateEnum es) const;

    bool isNearByClipRectLeft(double time, const QRectF &clipRect) const;
    bool isNearByClipRectRight(double time, const QRectF &clipRect) const;
    bool isNearByClipRectBottom(double y, const QRectF &clipRect) const;
    bool isNearByCurrentFrameIndicatorBottom(const QPointF &zoomCoords) const;

    std::vector<DSSelectedKey> isNearByKeyframe(DSKnob *dsKnob, const QPointF &widgetCoords) const;
    std::vector<DSSelectedKey> isNearByKeyframe(DSNode *dsNode, const QPointF &widgetCoords) const;

    double clampedMouseOffset(double fromTime, double toTime);

    int getHeightForItemAndChildren(QTreeWidgetItem *item) const;

    // Textures
    void generateKeyframeTextures();
    DopeSheetViewPrivate::KeyframeTexture kfTextureFromKeyframeType(Natron::KeyframeTypeEnum kfType, bool selected) const;

    // Drawing
    void drawScale() const;

    void drawRows() const;

    void drawNodeRow(const DSNode *dsNode) const;
    void drawKnobRow(const DSKnob *dsKnob) const;

    void drawNodeRowSeparation(const DSNode *dsNode) const;

    void drawRange(DSNode *dsNode) const;
    void drawKeyframes(DSNode *dsNode) const;

    void drawTexturedKeyframe(DopeSheetViewPrivate::KeyframeTexture textureType, const QRectF &rect) const;

    void drawGroupOverlay(DSNode *dsNode, DSNode *group) const;

    void drawProjectBounds() const;
    void drawCurrentFrameIndicator();

    void drawSelectionRect() const;

    void drawSelectedKeysBRect() const;

    void renderText(double x, double y,
                    const QString &text,
                    const QColor &color,
                    const QFont &font) const;

    // After or during an user interaction
    void computeTimelinePositions();
    void computeSelectionRect(const QPointF &origin, const QPointF &current);

    void computeSelectedKeysBRect();

    void computeRangesBelow(DSNode *dsNode);
    void computeNodeRange(DSNode *dsNode);
    void computeReaderRange(DSNode *reader);
    void computeRetimeRange(DSNode *retimer);
    void computeTimeOffsetRange(DSNode *timeOffset);
    void computeFRRange(DSNode *frameRange);
    void computeGroupRange(DSNode *group);

    // User interaction
    void onMouseDrag(QMouseEvent *e);

    void createSelectionFromRect(const QRectF &rect, std::vector<DSSelectedKey> *result);

    void moveCurrentFrameIndicator(double toTime);

    void createContextMenu();

    void updateCurveWidgetFrameRange();

    /* attributes */
    DopeSheetView *q_ptr;

    DopeSheet *model;
    HierarchyView *hierarchyView;

    Gui *gui;

    // necessary to retrieve some useful values for drawing
    boost::shared_ptr<TimeLine> timeline;

    //
    std::map<DSNode *, FrameRange > nodeRanges;

    // for rendering
    QFont *font;
    Natron::TextRenderer textRenderer;

    // for textures
    GLuint *kfTexturesIDs;
    QImage *kfTexturesImages;

    // for navigating
    ZoomContext zoomContext;
    bool zoomOrPannedSinceLastFit;

    // for current time indicator
    QPolygonF currentFrameIndicatorBottomPoly;

    // for keyframe selection
    QRectF selectionRect;

    // keyframe selection rect
    QRectF selectedKeysBRect;

    // for various user interaction
    QPointF lastPosOnMousePress;
    QPointF lastPosOnMouseMove;
    double lastTimeOffsetOnMousePress;
    double keyDragLastMovement;

    DopeSheetView::EventStateEnum eventState;

    // for clip (Reader, Time nodes) user interaction
    DSNode *currentEditedReader;
    DSNode *currentEditedGroup;

    // others
    bool hasOpenGLVAOSupport;

    // UI
    Natron::Menu *contextMenu;
};

DopeSheetViewPrivate::DopeSheetViewPrivate(DopeSheetView *qq) :
    q_ptr(qq),
    model(0),
    hierarchyView(0),
    gui(0),
    timeline(),
    nodeRanges(),
    font(new QFont(appFont,appFontSize)),
    textRenderer(),
    kfTexturesIDs(new GLuint[KF_TEXTURES_COUNT]),
    kfTexturesImages(new QImage[KF_TEXTURES_COUNT]),
    zoomContext(),
    zoomOrPannedSinceLastFit(false),
    selectionRect(),
    selectedKeysBRect(),
    lastPosOnMousePress(),
    lastPosOnMouseMove(),
    lastTimeOffsetOnMousePress(),
    keyDragLastMovement(),
    eventState(DopeSheetView::esNoEditingState),
    currentEditedReader(0),
    currentEditedGroup(0),
    hasOpenGLVAOSupport(true),
    contextMenu(new Natron::Menu(q_ptr))
{}

DopeSheetViewPrivate::~DopeSheetViewPrivate()
{
    glDeleteTextures(KF_TEXTURES_COUNT, kfTexturesIDs);

    delete []kfTexturesImages;
    delete []kfTexturesIDs;
}

QRectF DopeSheetViewPrivate::keyframeRect(double keyTime, double y) const
{
    QPointF p = zoomContext.toZoomCoordinates(keyTime, y);

    QRectF ret;
    ret.setHeight(KF_PIXMAP_SIZE);
    ret.setLeft(zoomContext.toZoomCoordinates(keyTime - KF_X_OFFSET, y).x());
    ret.setRight(zoomContext.toZoomCoordinates(keyTime + KF_X_OFFSET, y).x());
    ret.moveCenter(zoomContext.toWidgetCoordinates(p.x(), p.y()));

    return ret;
}

/**
 * @brief DopeSheetViewPrivate::rectToZoomCoordinates
 *
 *
 */
QRectF DopeSheetViewPrivate::rectToZoomCoordinates(const QRectF &rect) const
{
    QPointF topLeft(rect.left(),
                    zoomContext.toZoomCoordinates(rect.left(),
                                                  rect.top()).y());
    QPointF bottomRight(rect.right(),
                        zoomContext.toZoomCoordinates(rect.right(),
                                                      rect.bottom()).y());

    return QRectF(topLeft, bottomRight);
}

QRectF DopeSheetViewPrivate::rectToWidgetCoordinates(const QRectF &rect) const
{
    QPointF topLeft(rect.left(),
                    zoomContext.toWidgetCoordinates(rect.left(),
                                                    rect.top()).y());
    QPointF bottomRight(rect.right(),
                        zoomContext.toWidgetCoordinates(rect.right(),
                                                        rect.bottom()).y());

    return QRectF(topLeft, bottomRight);
}

QRectF DopeSheetViewPrivate::nameItemRectToRowRect(const QRectF &rect) const
{
    QRectF r = rectToZoomCoordinates(rect);

    double rowTop = r.top();
    double rowBottom = r.bottom() - 1;

    return QRectF(QPointF(zoomContext.left(), rowTop),
                  QPointF(zoomContext.right(), rowBottom));
}

Qt::CursorShape DopeSheetViewPrivate::getCursorDuringHover(const QPointF &widgetCoords) const
{
    Qt::CursorShape ret = Qt::ArrowCursor;

    QPointF zoomCoords = zoomContext.toZoomCoordinates(widgetCoords.x(), widgetCoords.y());

    // Does the user hovering the keyframe selection bounding rect ?
    QRectF selectedKeysBRectZoomCoords = rectToZoomCoordinates(selectedKeysBRect);

    if (selectedKeysBRectZoomCoords.isValid() && selectedKeysBRectZoomCoords.contains(zoomCoords)) {
        ret = getCursorForEventState(DopeSheetView::esMoveKeyframeSelection);
    }
    // Or does he hovering the current frame indicator ?
    else if (isNearByCurrentFrameIndicatorBottom(zoomCoords)) {
        ret = getCursorForEventState(DopeSheetView::esMoveCurrentFrameIndicator);
    }
    // Or does he hovering on a row's element ?
    else if (QTreeWidgetItem *treeItem = hierarchyView->itemAt(5, widgetCoords.y())) {
        DSNodeRows dsNodeItems = model->getNodeRows();
        DSNodeRows::const_iterator dsNodeIt = dsNodeItems.find(treeItem);

        if (dsNodeIt != dsNodeItems.end()) {
            DSNode *dsNode = (*dsNodeIt).second;
            DopeSheet::ItemType nodeType = dsNode->getItemType();

            QRectF treeItemRect = hierarchyView->getItemRect(dsNode);

            if (dsNode->isRangeDrawingEnabled()) {
                FrameRange range = nodeRanges.at(dsNode);

                QRectF nodeClipRect = rectToZoomCoordinates(QRectF(QPointF(range.first, treeItemRect.top() + 1),
                                                                   QPointF(range.second, treeItemRect.bottom() + 1)));

                if (nodeType == DopeSheet::ItemTypeGroup) {
                    if (nodeClipRect.contains(zoomCoords.x(), zoomCoords.y())) {
                        ret = getCursorForEventState(DopeSheetView::esGroupRepos);
                    }
                }
                else if (nodeType == DopeSheet::ItemTypeReader) {
                    if (nodeClipRect.contains(zoomCoords.x(), zoomCoords.y())) {
                        if (isNearByClipRectLeft(zoomCoords.x(), nodeClipRect)) {
                            ret = getCursorForEventState(DopeSheetView::esReaderLeftTrim);
                        }
                        else if (isNearByClipRectRight(zoomCoords.x(), nodeClipRect)) {
                            ret = getCursorForEventState(DopeSheetView::esReaderRightTrim);
                        }
                        else if (isNearByClipRectBottom(zoomCoords.y(), nodeClipRect)) {
                            ret = getCursorForEventState(DopeSheetView::esReaderSlip);
                        }
                        else {
                            ret = getCursorForEventState(DopeSheetView::esReaderRepos);
                        }
                    }
                }
            }
            else if (nodeType == DopeSheet::ItemTypeCommon) {
                std::vector<DSSelectedKey> keysUnderMouse = isNearByKeyframe(dsNode, widgetCoords);

                if (!keysUnderMouse.empty()) {
                    ret = getCursorForEventState(DopeSheetView::esPickKeyframe);
                }
            }
        }
        else {
            DSKnob *dsKnob =  hierarchyView->getDSKnobAt(widgetCoords.y());

            std::vector<DSSelectedKey> keysUnderMouse = isNearByKeyframe(dsKnob, widgetCoords);

            if (!keysUnderMouse.empty()) {
                ret = getCursorForEventState(DopeSheetView::esPickKeyframe);
            }
        }
    }
    else {
        ret = getCursorForEventState(DopeSheetView::esNoEditingState);
    }

    return ret;
}

Qt::CursorShape DopeSheetViewPrivate::getCursorForEventState(DopeSheetView::EventStateEnum es) const
{
    Qt::CursorShape cursorShape;

    switch (es) {
    case DopeSheetView::esPickKeyframe:
        cursorShape = Qt::CrossCursor;
        break;
    case DopeSheetView::esReaderRepos:
    case DopeSheetView::esGroupRepos:
    case DopeSheetView::esMoveKeyframeSelection:
        cursorShape = Qt::OpenHandCursor;
        break;
    case DopeSheetView::esReaderLeftTrim:
    case DopeSheetView::esMoveCurrentFrameIndicator:
        cursorShape = Qt::SplitHCursor;
        break;
    case DopeSheetView::esReaderRightTrim:
        cursorShape = Qt::SplitHCursor;
        break;
    case DopeSheetView::esReaderSlip:
        cursorShape = Qt::SizeHorCursor;
        break;
    case DopeSheetView::esNoEditingState:
    default:
        cursorShape = Qt::ArrowCursor;
        break;
    }

    return cursorShape;
}

bool DopeSheetViewPrivate::isNearByClipRectLeft(double time, const QRectF &clipRect) const
{
    double timeWidgetCoords = zoomContext.toWidgetCoordinates(time, 0).x();
    double rectLeftWidgetCoords = zoomContext.toWidgetCoordinates(clipRect.left(), 0).x();

    return ( (timeWidgetCoords >= rectLeftWidgetCoords - DISTANCE_ACCEPTANCE_FROM_READER_EDGE)
             && (timeWidgetCoords <= rectLeftWidgetCoords + DISTANCE_ACCEPTANCE_FROM_READER_EDGE));
}

bool DopeSheetViewPrivate::isNearByClipRectRight(double time, const QRectF &clipRect) const
{
    double timeWidgetCoords = zoomContext.toWidgetCoordinates(time, 0).x();
    double rectRightWidgetCoords = zoomContext.toWidgetCoordinates(clipRect.right(), 0).x();

    return ( (timeWidgetCoords >= rectRightWidgetCoords - DISTANCE_ACCEPTANCE_FROM_READER_EDGE)
             && (timeWidgetCoords <= rectRightWidgetCoords + DISTANCE_ACCEPTANCE_FROM_READER_EDGE));
}

bool DopeSheetViewPrivate::isNearByClipRectBottom(double y, const QRectF &clipRect) const
{
    return ( (y >= clipRect.bottom())
             && (y <= clipRect.bottom() + DISTANCE_ACCEPTANCE_FROM_READER_BOTTOM));
}

bool DopeSheetViewPrivate::isNearByCurrentFrameIndicatorBottom(const QPointF &zoomCoords) const
{
    return (currentFrameIndicatorBottomPoly.containsPoint(zoomCoords, Qt::OddEvenFill));
}

std::vector<DSSelectedKey> DopeSheetViewPrivate::isNearByKeyframe(DSKnob *dsKnob, const QPointF &widgetCoords) const
{
    assert(dsKnob);

    std::vector<DSSelectedKey> ret;

    boost::shared_ptr<KnobI> knob = dsKnob->getKnobGui()->getKnob();

    int dim = dsKnob->getDimension();

    int startDim = 0;
    int endDim = knob->getDimension();

    if (dim > -1) {
        startDim = dim;
        endDim = dim + 1;
    }

    for (int i = startDim; i < endDim; ++i) {
        KeyFrameSet keyframes = knob->getCurve(i)->getKeyFrames_mt_safe();

        for (KeyFrameSet::const_iterator kIt = keyframes.begin();
             kIt != keyframes.end();
             ++kIt) {
            KeyFrame kf = (*kIt);

            QPointF keyframeWidgetPos = zoomContext.toWidgetCoordinates(kf.getTime(), 0);

            if (std::abs(widgetCoords.x() - keyframeWidgetPos.x()) < DISTANCE_ACCEPTANCE_FROM_KEYFRAME) {
                DSKnob *context = 0;
                if (dim == -1) {
                    QTreeWidgetItem *childItem = dsKnob->findDimTreeItem(i);
                    context = model->findDSKnob(childItem);
                }
                else {
                    context = dsKnob;
                }

                ret.push_back(DSSelectedKey(context, kf));
            }
        }
    }

    return ret;
}

std::vector<DSSelectedKey> DopeSheetViewPrivate::isNearByKeyframe(DSNode *dsNode, const QPointF &widgetCoords) const
{
    std::vector<DSSelectedKey> ret;

    DSKnobRows dsKnobs = dsNode->getChildData();

    for (DSKnobRows::const_iterator it = dsKnobs.begin(); it != dsKnobs.end(); ++it) {
        DSKnob *dsKnob = (*it).second;
        KnobGui *knobGui = dsKnob->getKnobGui();

        int dim = dsKnob->getDimension();

        if (dim == -1) {
            continue;
        }

        KeyFrameSet keyframes = knobGui->getCurve(dim)->getKeyFrames_mt_safe();

        for (KeyFrameSet::const_iterator kIt = keyframes.begin();
             kIt != keyframes.end();
             ++kIt) {
            KeyFrame kf = (*kIt);

            QPointF keyframeWidgetPos = zoomContext.toWidgetCoordinates(kf.getTime(), 0);

            if (std::abs(widgetCoords.x() - keyframeWidgetPos.x()) < DISTANCE_ACCEPTANCE_FROM_KEYFRAME) {
                ret.push_back(DSSelectedKey(dsKnob, kf));
            }
        }
    }

    return ret;
}

double DopeSheetViewPrivate::clampedMouseOffset(double fromTime, double toTime)
{
    double totalMovement = toTime - fromTime;
    // Clamp the motion to the nearet integer
    totalMovement = std::floor(totalMovement + 0.5);

    double dt = totalMovement - keyDragLastMovement;

    // Update the last drag movement
    keyDragLastMovement = totalMovement;

    return dt;
}

int DopeSheetViewPrivate::getHeightForItemAndChildren(QTreeWidgetItem *item) const
{
    assert(!item->isHidden());

    // If the node item is collapsed
    if (!item->isExpanded()) {
        return hierarchyView->visualItemRect(item).height() + 1;
    }

    // Get the "bottom-most" item
    QTreeWidgetItem *lastChild = lastVisibleChild(item);

    if (lastChild->childCount() > 0 && lastChild->isExpanded()) {
        lastChild = lastVisibleChild(lastChild);
    }

    int top = hierarchyView->visualItemRect(item).top();
    int bottom = hierarchyView->visualItemRect(lastChild).bottom();

    return (bottom - top) + 1;
}

void DopeSheetViewPrivate::generateKeyframeTextures()
{
    kfTexturesImages[0].load(NATRON_IMAGES_PATH "interp_constant.png");
    kfTexturesImages[1].load(NATRON_IMAGES_PATH "interp_constant_selected.png");
    kfTexturesImages[2].load(NATRON_IMAGES_PATH "interp_linear.png");
    kfTexturesImages[3].load(NATRON_IMAGES_PATH "interp_linear_selected.png");
    kfTexturesImages[4].load(NATRON_IMAGES_PATH "interp_curve.png");
    kfTexturesImages[5].load(NATRON_IMAGES_PATH "interp_curve_selected.png");
    kfTexturesImages[6].load(NATRON_IMAGES_PATH "interp_break.png");
    kfTexturesImages[7].load(NATRON_IMAGES_PATH "interp_break_selected.png");
    kfTexturesImages[8].load(NATRON_IMAGES_PATH "interp_curve_c.png");
    kfTexturesImages[9].load(NATRON_IMAGES_PATH "interp_curve_c_selected.png");
    kfTexturesImages[10].load(NATRON_IMAGES_PATH "interp_curve_h.png");
    kfTexturesImages[11].load(NATRON_IMAGES_PATH "interp_curve_h_selected.png");
    kfTexturesImages[12].load(NATRON_IMAGES_PATH "interp_curve_r.png");
    kfTexturesImages[13].load(NATRON_IMAGES_PATH "interp_curve_r_selected.png");
    kfTexturesImages[14].load(NATRON_IMAGES_PATH "interp_curve_z.png");
    kfTexturesImages[15].load(NATRON_IMAGES_PATH "interp_curve_z_selected.png");
    kfTexturesImages[16].load(NATRON_IMAGES_PATH "keyframe_node_root.png");
    kfTexturesImages[17].load(NATRON_IMAGES_PATH "keyframe_node_root_selected.png");

    for (int i = 0; i < KF_TEXTURES_COUNT; ++i) {
        kfTexturesImages[i] = kfTexturesImages[i].scaled(KF_PIXMAP_SIZE, KF_PIXMAP_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        kfTexturesImages[i] = QGLWidget::convertToGLFormat(kfTexturesImages[i]);
    }

    glGenTextures(KF_TEXTURES_COUNT, kfTexturesIDs);
}

DopeSheetViewPrivate::KeyframeTexture DopeSheetViewPrivate::kfTextureFromKeyframeType(Natron::KeyframeTypeEnum kfType, bool selected) const
{
    DopeSheetViewPrivate::KeyframeTexture ret = DopeSheetViewPrivate::kfTextureNone;

    switch (kfType) {
    case Natron::eKeyframeTypeConstant:
        ret = (selected) ? DopeSheetViewPrivate::kfTextureInterpConstantSelected : DopeSheetViewPrivate::kfTextureInterpConstant;
        break;
    case Natron::eKeyframeTypeLinear:
        ret = (selected) ? DopeSheetViewPrivate::kfTextureInterpLinearSelected : DopeSheetViewPrivate::kfTextureInterpLinear;
        break;
    case Natron::eKeyframeTypeBroken:
        ret = (selected) ? DopeSheetViewPrivate::kfTextureInterpBreakSelected : DopeSheetViewPrivate::kfTextureInterpBreak;
        break;
    case Natron::eKeyframeTypeFree:
        ret = (selected) ? DopeSheetViewPrivate::kfTextureInterpLinearSelected : DopeSheetViewPrivate::kfTextureInterpLinear;
        break;
    case Natron::eKeyframeTypeSmooth:
        ret = (selected) ? DopeSheetViewPrivate::kfTextureInterpCurveZSelected : DopeSheetViewPrivate::kfTextureInterpCurveZ;
        break;
    case Natron::eKeyframeTypeCatmullRom:
        ret = (selected) ? DopeSheetViewPrivate::kfTextureInterpCurveRSelected : DopeSheetViewPrivate::kfTextureInterpCurveR;
        break;
    case Natron::eKeyframeTypeCubic:
        ret = (selected) ? DopeSheetViewPrivate::kfTextureInterpCurveCSelected : DopeSheetViewPrivate::kfTextureInterpCurveC;
        break;
    case Natron::eKeyframeTypeHorizontal:
        ret = (selected) ? DopeSheetViewPrivate::kfTextureInterpCurveHSelected : DopeSheetViewPrivate::kfTextureInterpCurveH;
        break;
    default:
        ret = DopeSheetViewPrivate::kfTextureNone;
        break;
    }

    return ret;
}

/**
 * @brief DopeSheetViewPrivate::drawScale
 *
 * Draws the dope sheet's grid and time indicators.
 */
void DopeSheetViewPrivate::drawScale() const
{
    running_in_main_thread_and_context(q_ptr);

    QPointF bottomLeft = zoomContext.toZoomCoordinates(0, q_ptr->height() - 1);
    QPointF topRight = zoomContext.toZoomCoordinates(q_ptr->width() - 1, 0);

    // Don't attempt to draw a scale on a widget with an invalid height
    if (q_ptr->height() <= 1) {
        return;
    }

    QFontMetrics fontM(*font);
    const double smallestTickSizePixel = 5.; // tick size (in pixels) for alpha = 0.
    const double largestTickSizePixel = 1000.; // tick size (in pixels) for alpha = 1.
    std::vector<double> acceptedDistances;
    acceptedDistances.push_back(1.);
    acceptedDistances.push_back(5.);
    acceptedDistances.push_back(10.);
    acceptedDistances.push_back(50.);

    // Retrieve the appropriate settings for drawing
    boost::shared_ptr<Settings> settings = appPTR->getCurrentSettings();
    double scaleR, scaleG, scaleB;
    settings->getDopeSheetEditorScaleColor(&scaleR, &scaleG, &scaleB);

    QColor scaleColor;
    scaleColor.setRgbF(Natron::clamp(scaleR),
                       Natron::clamp(scaleG),
                       Natron::clamp(scaleB));

    // Perform drawing
    {
        GLProtectAttrib a(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        const double rangePixel = q_ptr->width();
        const double range_min = bottomLeft.x();
        const double range_max = topRight.x();
        const double range = range_max - range_min;

        double smallTickSize;
        bool half_tick;

        ticks_size(range_min, range_max, rangePixel, smallestTickSizePixel, &smallTickSize, &half_tick);

        int m1, m2;
        const int ticks_max = 1000;
        double offset;

        ticks_bounds(range_min, range_max, smallTickSize, half_tick, ticks_max, &offset, &m1, &m2);
        std::vector<int> ticks;
        ticks_fill(half_tick, ticks_max, m1, m2, &ticks);

        const double smallestTickSize = range * smallestTickSizePixel / rangePixel;
        const double largestTickSize = range * largestTickSizePixel / rangePixel;
        const double minTickSizeTextPixel = fontM.width( QString("00") );
        const double minTickSizeText = range * minTickSizeTextPixel / rangePixel;

        for (int i = m1; i <= m2; ++i) {

            double value = i * smallTickSize + offset;
            const double tickSize = ticks[i - m1] * smallTickSize;
            const double alpha = ticks_alpha(smallestTickSize, largestTickSize, tickSize);

            glColor4f(scaleColor.redF(), scaleColor.greenF(), scaleColor.blueF(), alpha);

            // Draw the vertical lines belonging to the grid
            glBegin(GL_LINES);
            glVertex2f(value, bottomLeft.y());
            glVertex2f(value, topRight.y());
            glEnd();

            glCheckError();

            // Draw the time indicators
            if (tickSize > minTickSizeText) {
                const int tickSizePixel = rangePixel * tickSize / range;
                const QString s = QString::number(value);
                const int sSizePixel = fontM.width(s);

                if (tickSizePixel > sSizePixel) {
                    const int sSizeFullPixel = sSizePixel + minTickSizeTextPixel;
                    double alphaText = 1.0; //alpha;

                    if (tickSizePixel < sSizeFullPixel) {
                        // when the text size is between sSizePixel and sSizeFullPixel,
                        // draw it with a lower alpha
                        alphaText *= (tickSizePixel - sSizePixel) / (double)minTickSizeTextPixel;
                    }

                    QColor c = scaleColor;
                    c.setAlpha(255 * alphaText);

                    renderText(value, bottomLeft.y(), s, c, *font);

                    // Uncomment the line below to draw the indicator on top too
                    // parent->renderText(value, topRight.y() - 20, s, c, *font);
                }
            }
        }
    }
}

/**
 * @brief DopeSheetViewPrivate::drawRows
 *
 *
 *
 * These rows have the same height as an item from the hierarchy view.
 */
void DopeSheetViewPrivate::drawRows() const
{
    running_in_main_thread_and_context(q_ptr);

    DSNodeRows treeItemsAndDSNodes = model->getNodeRows();

    // Perform drawing
    {
        GLProtectAttrib a(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT);

        for (DSNodeRows::const_iterator it = treeItemsAndDSNodes.begin();
             it != treeItemsAndDSNodes.end();
             ++it) {
            QTreeWidgetItem *treeItem = (*it).first;

            if(treeItem->isHidden()) {
                continue;
            }

            if (QTreeWidgetItem *parentItem = treeItem->parent()) {
                if (!parentItem->isExpanded()) {
                    continue;
                }
            }

            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            DSNode *dsNode = (*it).second;

            drawNodeRow(dsNode);

            DSKnobRows knobItems = dsNode->getChildData();
            for (DSKnobRows::const_iterator it2 = knobItems.begin();
                 it2 != knobItems.end();
                 ++it2) {

                DSKnob *dsKnob = (*it2).second;

                drawKnobRow(dsKnob);
            }

            if (DSNode *group = model->getGroupDSNode(dsNode)) {
                drawGroupOverlay(dsNode, group);
            }

            DopeSheet::ItemType nodeType = dsNode->getItemType();

            if (dsNode->isRangeDrawingEnabled()) {
                drawRange(dsNode);
            }

            if (nodeType != DopeSheet::ItemTypeGroup) {
                drawKeyframes(dsNode);
            }
        }

        // Draw node rows separations
        for (DSNodeRows::const_iterator it = treeItemsAndDSNodes.begin();
             it != treeItemsAndDSNodes.end();
             ++it) {
            DSNode *dsNode = (*it).second;
            drawNodeRowSeparation(dsNode);
        }
    }
}

/**
 * @brief DopeSheetViewPrivate::drawNodeRow
 *
 *
 */
void DopeSheetViewPrivate::drawNodeRow(const DSNode *dsNode) const
{
    GLProtectAttrib a(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT);

    QRectF nameItemRect = hierarchyView->getItemRect(dsNode);
    QRectF rowRect = nameItemRectToRowRect(nameItemRect);

    boost::shared_ptr<Settings> settings = appPTR->getCurrentSettings();
    double rootR, rootG, rootB, rootA;
    settings->getDopeSheetEditorRootRowBackgroundColor(&rootR, &rootG, &rootB, &rootA);

    glColor4f(rootR, rootG, rootB, rootA);

    glBegin(GL_POLYGON);
    glVertex2f(rowRect.left(), rowRect.top());
    glVertex2f(rowRect.left(), rowRect.bottom());
    glVertex2f(rowRect.right(), rowRect.bottom());
    glVertex2f(rowRect.right(), rowRect.top());
    glEnd();
}

/**
 * @brief DopeSheetViewPrivate::drawKnobRow
 *
 *
 */
void DopeSheetViewPrivate::drawKnobRow(const DSKnob *dsKnob) const
{
    if (dsKnob->getTreeItem()->isHidden()) {
        return;
    }

    GLProtectAttrib a(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT);

    QRectF nameItemRect = hierarchyView->getItemRect(dsKnob);
    QRectF rowRect = nameItemRectToRowRect(nameItemRect);

    boost::shared_ptr<Settings> settings = appPTR->getCurrentSettings();

    double bkR, bkG, bkB, bkA;
    if (dsKnob->isMultiDimRoot()) {
        settings->getDopeSheetEditorRootRowBackgroundColor(&bkR, &bkG, &bkB, &bkA);
    }
    else {
        settings->getDopeSheetEditorKnobRowBackgroundColor(&bkR, &bkG, &bkB, &bkA);
    }

    glColor4f(bkR, bkG, bkB, bkA);

    glBegin(GL_POLYGON);
    glVertex2f(rowRect.left(), rowRect.top());
    glVertex2f(rowRect.left(), rowRect.bottom());
    glVertex2f(rowRect.right(), rowRect.bottom());
    glVertex2f(rowRect.right(), rowRect.top());
    glEnd();
}

void DopeSheetViewPrivate::drawNodeRowSeparation(const DSNode *dsNode) const
{
    GLProtectAttrib a(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT | GL_LINE_BIT);

    QRectF nameItemRect = hierarchyView->getItemRect(dsNode);
    QRectF rowRect = nameItemRectToRowRect(nameItemRect);

    glLineWidth(NODE_SEPARATION_WIDTH);
    glColor4f(0.f, 0.f, 0.f, 1.f);

    glBegin(GL_LINES);
    glVertex2f(rowRect.left(), rowRect.top());
    glVertex2f(rowRect.right(), rowRect.top());
    glEnd();
}

void DopeSheetViewPrivate::drawRange(DSNode *dsNode) const
{
    // Draw the clip
    {
        FrameRange range = nodeRanges.at(dsNode);

        QRectF treeItemRect = hierarchyView->getItemRect(dsNode);

        QRectF clipRectZoomCoords = rectToZoomCoordinates(QRectF(QPointF(range.first, treeItemRect.top() + 1),
                                                                 QPointF(range.second, treeItemRect.bottom() + 1)));

        GLProtectAttrib a(GL_CURRENT_BIT);

        QColor fillColor = dsNode->getNodeGui()->getCurrentColor();
        fillColor = QColor::fromHsl(fillColor.hslHue(), 50, fillColor.lightness());

        // If necessary, draw the original frame range line
        if (dsNode->getItemType() == DopeSheet::ItemTypeReader) {
            NodePtr node = dsNode->getInternalNode();

            Knob<int> *firstFrameKnob = dynamic_cast<Knob<int> *>(node->getKnobByName("firstFrame").get());
            assert(firstFrameKnob);

            double speedValue = 1.0f;

            if (DSNode *nearestRetimer = model->getNearestTimeNodeFromOutputs(dsNode)) {
                if (nearestRetimer->getItemType() == DopeSheet::ItemTypeRetime) {
                    Knob<double> *speedKnob =  dynamic_cast<Knob<double> *>(nearestRetimer->getInternalNode()->getKnobByName("speed").get());
                    assert(speedKnob);

                    speedValue = speedKnob->getValue();
                }
            }

            Knob<int> *originalFrameRangeKnob = dynamic_cast<Knob<int> *>(node->getKnobByName("originalFrameRange").get());
            assert(originalFrameRangeKnob);

            int lineBegin = clipRectZoomCoords.left() - firstFrameKnob->getValue() + 1;

            int frameCount = originalFrameRangeKnob->getValue(1) - originalFrameRangeKnob->getValue(0) + 1;
            int lineEnd = lineBegin + (frameCount / speedValue);

            float clipRectCenterY = clipRectZoomCoords.center().y();

            GLProtectAttrib aa(GL_CURRENT_BIT | GL_LINE_BIT);
            glLineWidth(1);

            glColor4f(fillColor.redF(), fillColor.greenF(), fillColor.blueF(), 1.f);

            glBegin(GL_LINES);
            glVertex2f(lineBegin, clipRectCenterY);
            glVertex2f(lineEnd, clipRectCenterY);
            glEnd();
        }

        // Fill the range rect
        glColor4f(fillColor.redF(), fillColor.greenF(), fillColor.blueF(), 1.f);

        glBegin(GL_POLYGON);
        glVertex2f(clipRectZoomCoords.left(), clipRectZoomCoords.top());
        glVertex2f(clipRectZoomCoords.left(), clipRectZoomCoords.bottom() + 1);
        glVertex2f(clipRectZoomCoords.right(), clipRectZoomCoords.bottom() + 1);
        glVertex2f(clipRectZoomCoords.right(), clipRectZoomCoords.top());
        glEnd();
    }

    // Draw the outline
    //        glLineWidth(2);

    ////        glColor4f(bkColorR, bkColorG, bkColorB, 1.f);

    //        glBegin(GL_LINE_LOOP);
    //        glVertex2f(clipRectZoomCoords.left(), clipRectZoomCoords.top());
    //        glVertex2f(clipRectZoomCoords.left(), clipRectZoomCoords.bottom() + 2);
    //        glVertex2f(clipRectZoomCoords.right(), clipRectZoomCoords.bottom() + 2);
    //        glVertex2f(clipRectZoomCoords.right(), clipRectZoomCoords.top());
    //        glEnd();
    //    }

    // Draw the preview
    //    {
    //        if ( node->isRenderingPreview() ) {
    //            return;
    //        }

    //        int w = readerRect.width();
    //        int h = readerRect.height();

    //        size_t dataSize = 4 * w * h;
    //        {
    //#ifndef __NATRON_WIN32__
    //            unsigned int* buf = (unsigned int*)calloc(dataSize, 1);
    //#else
    //            unsigned int* buf = (unsigned int*)malloc(dataSize);
    //            for (int i = 0; i < w * h; ++i) {
    //                buf[i] = qRgba(0,0,0,255);
    //            }
    //#endif
    //            bool success = node->makePreviewImage((startingTime - lastFrame) / 2, &w, &h, buf);

    //            if (success) {
    //                QImage img(reinterpret_cast<const uchar *>(buf), w, h, QImage::Format_ARGB32);
    //                GLuint textureId = parent->bindTexture(img);

    //                parent->drawTexture(rectToZoomCoordinates(QRectF(readerRect.left(),
    //                                                                 readerRect.top(),
    //                                                                 w, h)),
    //                                    textureId);
    //            }

    //            free(buf);
    //        }
    //    }
}


/**
 * @brief DopeSheetViewPrivate::drawKeyframes
 *
 *
 */
void DopeSheetViewPrivate::drawKeyframes(DSNode *dsNode) const
{
    running_in_main_thread_and_context(q_ptr);

    // Perform drawing
    {
        GLProtectAttrib a(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        DSKnobRows knobItems = dsNode->getChildData();
        for (DSKnobRows::const_iterator it = knobItems.begin();
             it != knobItems.end();
             ++it) {

            DSKnob *dsKnob = (*it).second;
            QTreeWidgetItem *knobTreeItem = dsKnob->getTreeItem();

            // The knob is no longer animated
            if (knobTreeItem->isHidden()) {
                continue;
            }

            int dim = dsKnob->getDimension();

            if (dim == -1) {
                continue;
            }

            KeyFrameSet keyframes = dsKnob->getKnobGui()->getCurve(dim)->getKeyFrames_mt_safe();

            for (KeyFrameSet::const_iterator kIt = keyframes.begin();
                 kIt != keyframes.end();
                 ++kIt) {
                KeyFrame kf = (*kIt);

                double keyTime = kf.getTime();
                double rowCenterY = hierarchyView->getItemRect(dsKnob).center().y();

                QRectF zoomKfRect = rectToZoomCoordinates(keyframeRect(keyTime, rowCenterY));

                bool kfIsSelectedOrHighlighted = model->getSelectionModel()->keyframeIsSelected(dsKnob, kf)
                        || ( selectionRect.intersects(zoomKfRect)
                             || rectToZoomCoordinates(selectedKeysBRect).intersects(zoomKfRect) );

                // Draw keyframe in the knob dim row only if it's visible
                bool drawInDimRow = itemIsVisibleFromOutside(knobTreeItem);

                if (drawInDimRow) {
                    DopeSheetViewPrivate::KeyframeTexture texType = kfTextureFromKeyframeType(kf.getInterpolation(),
                                                                                              kfIsSelectedOrHighlighted);

                    if (texType != DopeSheetViewPrivate::kfTextureNone) {
                        drawTexturedKeyframe(texType, zoomKfRect);
                    }
                }

                // Draw keyframe in the root sections too
                DopeSheetViewPrivate::KeyframeTexture rootKfTexType = (kfIsSelectedOrHighlighted)
                        ? DopeSheetViewPrivate::kfTextureRootSelected
                        : DopeSheetViewPrivate::kfTextureRoot;

                QTreeWidgetItem *knobParentItem = knobTreeItem->parent();
                DSNode *nodeContext = model->findDSNode(knobParentItem);

                bool drawInMultidimRootRow = itemIsVisibleFromOutside(knobParentItem);

                if (drawInMultidimRootRow) {
                    if (!nodeContext) {
                        double newCenterY = hierarchyView->visualItemRect(knobParentItem).center().y();
                        zoomKfRect = rectToZoomCoordinates(keyframeRect(keyTime, newCenterY));

                        drawTexturedKeyframe(rootKfTexType, zoomKfRect);
                    }
                }

                nodeContext = model->findParentDSNode(knobParentItem);

                bool drawInNodeRoot = itemIsVisibleFromOutside(nodeContext->getTreeItem());

                if (drawInNodeRoot) {
                    double newCenterY = hierarchyView->getItemRect(nodeContext).center().y();
                    zoomKfRect = rectToZoomCoordinates(keyframeRect(keyTime, newCenterY));

                    drawTexturedKeyframe(rootKfTexType, zoomKfRect);
                }
            }
        }
    }
}

void DopeSheetViewPrivate::drawTexturedKeyframe(DopeSheetViewPrivate::KeyframeTexture textureType, const QRectF &rect) const
{
    GLProtectAttrib a(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT | GL_TRANSFORM_BIT);
    GLProtectMatrix pr(GL_MODELVIEW);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, kfTexturesIDs[textureType]);

    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, KF_PIXMAP_SIZE, KF_PIXMAP_SIZE, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, kfTexturesImages[textureType].bits());

    glScaled(1.0d / zoomContext.factor(),
             1.0d / zoomContext.factor(),
             1.0d);

    glBegin(GL_POLYGON);
    glTexCoord2f(0.0f, 1.0f);
    glVertex2f(rect.left(), rect.top());
    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(rect.left(), rect.bottom());
    glTexCoord2f(1.0f, 0.0f);
    glVertex2f(rect.right(), rect.bottom());
    glTexCoord2f(1.0f, 1.0f);
    glVertex2f(rect.right(), rect.top());
    glEnd();

    glColor4f(1, 1, 1, 1);
    glBindTexture(GL_TEXTURE_2D, 0);

    glDisable(GL_TEXTURE_2D);
}

void DopeSheetViewPrivate::drawGroupOverlay(DSNode *dsNode, DSNode *group) const
{
    // Get the overlay color
    double r, g, b;
    dsNode->getNodeGui()->getColor(&r, &g, &b);

    // Compute the area to fill
    int height = getHeightForItemAndChildren(dsNode->getTreeItem()) ;
    QRectF nameItemRect = hierarchyView->visualItemRect(dsNode->getTreeItem());
    int top = nameItemRect.top();

    FrameRange groupRange = nodeRanges.at(group);
    int left = groupRange.first;

    QRectF overlayRect = rectToZoomCoordinates(QRectF(left, top,
                                                      (groupRange.second - groupRange.first), height));

    // Perform drawing
    {
        GLProtectAttrib a(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT);

        glColor4f(r, g, b, 0.30f);

        glBegin(GL_QUADS);
        glVertex2f(overlayRect.left(), overlayRect.top());
        glVertex2f(overlayRect.left(), overlayRect.bottom());
        glVertex2f(overlayRect.right(), overlayRect.bottom());
        glVertex2f(overlayRect.right(), overlayRect.top());
        glEnd();
    }
}

void DopeSheetViewPrivate::drawProjectBounds() const
{
    running_in_main_thread_and_context(q_ptr);

    double bottom = zoomContext.toZoomCoordinates(0, q_ptr->height() - 1).y();
    double top = zoomContext.toZoomCoordinates(q_ptr->width() - 1, 0).y();

    int projectStart, projectEnd;
    gui->getApp()->getFrameRange(&projectStart, &projectEnd);

    boost::shared_ptr<Settings> settings = appPTR->getCurrentSettings();
    double colorR, colorG, colorB;
    settings->getTimelineBoundsColor(&colorR, &colorG, &colorB);

    // Perform drawing
    {
        GLProtectAttrib a(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT);

        glColor4f(colorR, colorG, colorB, 1.f);

        // Draw start bound
        glBegin(GL_LINES);
        glVertex2f(projectStart, top);
        glVertex2f(projectStart, bottom);
        glEnd();

        // Draw end bound
        glBegin(GL_LINES);
        glVertex2f(projectEnd, top);
        glVertex2f(projectEnd, bottom);
        glEnd();
    }
}

/**
 * @brief DopeSheetViewPrivate::drawIndicator
 *
 *
 */
void DopeSheetViewPrivate::drawCurrentFrameIndicator()
{
    running_in_main_thread_and_context(q_ptr);

    computeTimelinePositions();

    int top = zoomContext.toZoomCoordinates(0, 0).y();
    int bottom = zoomContext.toZoomCoordinates(q_ptr->width() - 1,
                                               q_ptr->height() - 1).y();

    int currentFrame = timeline->currentFrame();

    // Retrieve settings for drawing
    boost::shared_ptr<Settings> settings = appPTR->getCurrentSettings();
    double colorR, colorG, colorB;
    settings->getTimelinePlayheadColor(&colorR, &colorG, &colorB);

    // Perform drawing
    {
        GLProtectAttrib a(GL_CURRENT_BIT | GL_HINT_BIT | GL_ENABLE_BIT |
                          GL_LINE_BIT | GL_POLYGON_BIT | GL_COLOR_BUFFER_BIT);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glEnable(GL_LINE_SMOOTH);
        glHint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);

        glColor4f(colorR, colorG, colorB, 1.f);

        glBegin(GL_LINES);
        glVertex2f(currentFrame, top);
        glVertex2f(currentFrame, bottom);
        glEnd();

        glEnable(GL_POLYGON_SMOOTH);
        glHint(GL_POLYGON_SMOOTH_HINT, GL_DONT_CARE);

        // Draw top polygon
        //        glBegin(GL_POLYGON);
        //        glVertex2f(currentTime - polyHalfWidth, top);
        //        glVertex2f(currentTime + polyHalfWidth, top);
        //        glVertex2f(currentTime, top - polyHeight);
        //        glEnd();

        // Draw bottom polygon
        glBegin(GL_POLYGON);
        glVertex2f(currentFrameIndicatorBottomPoly.at(0).x(), currentFrameIndicatorBottomPoly.at(0).y());
        glVertex2f(currentFrameIndicatorBottomPoly.at(1).x(), currentFrameIndicatorBottomPoly.at(1).y());
        glVertex2f(currentFrameIndicatorBottomPoly.at(2).x(), currentFrameIndicatorBottomPoly.at(2).y());
        glEnd();
    }
}

/**
 * @brief DopeSheetViewPrivate::drawSelectionRect
 *
 *
 */
void DopeSheetViewPrivate::drawSelectionRect() const
{
    running_in_main_thread_and_context(q_ptr);

    QPointF topLeft = selectionRect.topLeft();
    QPointF bottomRight = selectionRect.bottomRight();

    // Perform drawing
    {
        GLProtectAttrib a(GL_HINT_BIT | GL_ENABLE_BIT | GL_LINE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT | GL_LINE_BIT);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_LINE_SMOOTH);
        glHint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);

        glColor4f(0.3, 0.3, 0.3, 0.2);

        // Draw rect
        glBegin(GL_POLYGON);
        glVertex2f(topLeft.x(), bottomRight.y());
        glVertex2f(topLeft.x(), topLeft.y());
        glVertex2f(bottomRight.x(), topLeft.y());
        glVertex2f(bottomRight.x(), bottomRight.y());
        glEnd();

        glLineWidth(1.5);

        // Draw outline
        glColor4f(0.5,0.5,0.5,1.);
        glBegin(GL_LINE_LOOP);
        glVertex2f(topLeft.x(), bottomRight.y());
        glVertex2f(topLeft.x(), topLeft.y());
        glVertex2f(bottomRight.x(), topLeft.y());
        glVertex2f(bottomRight.x(), bottomRight.y());
        glEnd();

        glCheckError();
    }
}

/**
 * @brief DopeSheetViewPrivate::drawSelectedKeysBRect
 *
 *
 */
void DopeSheetViewPrivate::drawSelectedKeysBRect() const
{
    running_in_main_thread_and_context(q_ptr);

    QRectF bRect = rectToZoomCoordinates(selectedKeysBRect);

    // Perform drawing
    {
        GLProtectAttrib a(GL_HINT_BIT | GL_ENABLE_BIT | GL_LINE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT | GL_LINE_BIT);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_LINE_SMOOTH);
        glHint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);

        glLineWidth(1.5);

        glColor4f(0.5, 0.5, 0.5, 1.);

        // Draw outline
        glBegin(GL_LINE_LOOP);
        glVertex2f(selectedKeysBRect.left(), bRect.bottom());
        glVertex2f(selectedKeysBRect.left(), bRect.top());
        glVertex2f(selectedKeysBRect.right(), bRect.top());
        glVertex2f(selectedKeysBRect.right(), bRect.bottom());
        glEnd();

        // Draw center cross lines
        const int CROSS_LINE_OFFSET = 10;

        QPointF bRectCenter = bRect.center();
        QPointF bRectCenterWidgetCoords = zoomContext.toWidgetCoordinates(bRectCenter.x(), bRectCenter.y());

        QLineF horizontalLine(zoomContext.toZoomCoordinates(bRectCenterWidgetCoords.x() - CROSS_LINE_OFFSET, bRectCenterWidgetCoords.y()),
                              zoomContext.toZoomCoordinates(bRectCenterWidgetCoords.x() + CROSS_LINE_OFFSET, bRectCenterWidgetCoords.y()));

        QLineF verticalLine(zoomContext.toZoomCoordinates(bRectCenterWidgetCoords.x(), bRectCenterWidgetCoords.y() - CROSS_LINE_OFFSET),
                            zoomContext.toZoomCoordinates(bRectCenterWidgetCoords.x(), bRectCenterWidgetCoords.y() + CROSS_LINE_OFFSET));

        glBegin(GL_LINES);
        glVertex2f(horizontalLine.p1().x(), horizontalLine.p1().y());
        glVertex2f(horizontalLine.p2().x(), horizontalLine.p2().y());

        glVertex2f(verticalLine.p1().x(), verticalLine.p1().y());
        glVertex2f(verticalLine.p2().x(), verticalLine.p2().y());
        glEnd();

        glCheckError();
    }
}

void DopeSheetViewPrivate::renderText(double x, double y, const QString &text, const QColor &color, const QFont &font) const
{
    running_in_main_thread_and_context(q_ptr);

    if ( text.isEmpty() ) {
        return;
    }

    double w = double(q_ptr->width());
    double h = double(q_ptr->height());

    double bottom = zoomContext.bottom();
    double left = zoomContext.left();
    double top =  zoomContext.top();
    double right = zoomContext.right();

    if (w <= 0 || h <= 0 || right <= left || top <= bottom) {
        return;
    }

    double scalex = (right-left) / w;
    double scaley = (top-bottom) / h;

    textRenderer.renderText(x, y, scalex, scaley, text, color, font);

    glCheckError();

}

void DopeSheetViewPrivate::computeTimelinePositions()
{
    running_in_main_thread();

    double polyHalfWidth = 7.5;
    double polyHeight = 7.5;

    int bottom = zoomContext.toZoomCoordinates(q_ptr->width() - 1,
                                               q_ptr->height() - 1).y();

    int currentFrame = timeline->currentFrame();

    QPointF bottomCursorBottom(currentFrame, bottom);
    QPointF bottomCursorBottomWidgetCoords = zoomContext.toWidgetCoordinates(bottomCursorBottom.x(), bottomCursorBottom.y());

    QPointF leftPoint = zoomContext.toZoomCoordinates(bottomCursorBottomWidgetCoords.x() - polyHalfWidth, bottomCursorBottomWidgetCoords.y());
    QPointF rightPoint = zoomContext.toZoomCoordinates(bottomCursorBottomWidgetCoords.x() + polyHalfWidth, bottomCursorBottomWidgetCoords.y());
    QPointF topPoint = zoomContext.toZoomCoordinates(bottomCursorBottomWidgetCoords.x(), bottomCursorBottomWidgetCoords.y() - polyHeight);

    currentFrameIndicatorBottomPoly.clear();

    currentFrameIndicatorBottomPoly << leftPoint
                                    << rightPoint
                                    << topPoint;
}

void DopeSheetViewPrivate::computeSelectionRect(const QPointF &origin, const QPointF &current)
{
    double xmin = std::min(origin.x(), current.x());
    double xmax = std::max(origin.x(), current.x());
    double ymin = std::min(origin.y(), current.y());
    double ymax = std::max(origin.y(), current.y());

    selectionRect.setTopLeft(QPointF(xmin, ymin));
    selectionRect.setBottomRight(QPointF(xmax, ymax));
}

void DopeSheetViewPrivate::computeSelectedKeysBRect()
{
    std::vector<DSSelectedKey> selectedKeyframes = model->getSelectionModel()->getSelectionCopy();

    if (selectedKeyframes.size() <= 1) {
        selectedKeysBRect = QRectF();

        return;
    }

    std::set<double> ys;
    std::set<double> keyTimes;

    for (std::vector<DSSelectedKey>::const_iterator it = selectedKeyframes.begin();
         it != selectedKeyframes.end();
         ++it) {
        QTreeWidgetItem *keyItem = (*it).context->getTreeItem();

        double x = (*it).key.getTime();
        keyTimes.insert(x);

        QRect keyItemRect = hierarchyView->visualItemRect(keyItem);

        double y;

        if (!keyItemRect.isNull() && !keyItemRect.isEmpty()) {
            y = keyItemRect.center().y();
        }
        else {
            y = firstVisibleParentCenterY(keyItem);
        }

        ys.insert(y);
    }

    // Adjust the top to the associated item bottom
    QTreeWidgetItem *bottomMostItem = hierarchyView->itemAt(0, *(ys.rbegin()));
    double top = hierarchyView->visualItemRect(bottomMostItem).bottom();

    // Adjust the bottom to the node item top
    QTreeWidgetItem *topMostItem = hierarchyView->itemAt(0, *(ys.begin()));
    DSNode *parentDSNode = model->findParentDSNode(topMostItem);

    if (!parentDSNode) {
        return;
    }

    QTreeWidgetItem *topMostItemNodeItem = parentDSNode->getTreeItem();

    double bottom = hierarchyView->visualItemRect(topMostItemNodeItem).top();

    double left = *(keyTimes.begin());
    double right = *(keyTimes.rbegin());

    QPointF topLeft(left, top);
    QPointF bottomRight(right, bottom);

    selectedKeysBRect.setTopLeft(topLeft);
    selectedKeysBRect.setBottomRight(bottomRight);

    if (!selectedKeysBRect.isNull()) {
        double xAdjustOffset = (zoomContext.toZoomCoordinates(left, 0).x() -
                                zoomContext.toZoomCoordinates(left - KF_X_OFFSET, 0).x());

        selectedKeysBRect.adjust(-xAdjustOffset, 0, xAdjustOffset, 0);
    }
}

void DopeSheetViewPrivate::computeRangesBelow(DSNode *dsNode)
{
    DSNodeRows nodeRows = model->getNodeRows();

    for (DSNodeRows::const_iterator it = nodeRows.begin(); it != nodeRows.end(); ++it) {
        QTreeWidgetItem *item = (*it).first;
        DSNode *toCompute = (*it).second;

        if (hierarchyView->visualItemRect(item).y() >= hierarchyView->visualItemRect(dsNode->getTreeItem()).y()) {
            computeNodeRange(toCompute);
        }
    }
}

void DopeSheetViewPrivate::computeNodeRange(DSNode *dsNode)
{
    DopeSheet::ItemType nodeType = dsNode->getItemType();

    switch (nodeType) {
    case DopeSheet::ItemTypeReader:
        computeReaderRange(dsNode);
        break;
    case DopeSheet::ItemTypeRetime:
        computeRetimeRange(dsNode);
        break;
    case DopeSheet::ItemTypeTimeOffset:
        computeTimeOffsetRange(dsNode);
        break;
    case DopeSheet::ItemTypeFrameRange:
        computeFRRange(dsNode);
        break;
    case DopeSheet::ItemTypeGroup:
        computeGroupRange(dsNode);
        break;
    default:
        break;
    }
}

void DopeSheetViewPrivate::computeReaderRange(DSNode *reader)
{
    NodePtr node = reader->getInternalNode();

    Knob<int> *startingTimeKnob = dynamic_cast<Knob<int> *>(node->getKnobByName("startingTime").get());
    assert(startingTimeKnob);
    Knob<int> *firstFrameKnob = dynamic_cast<Knob<int> *>(node->getKnobByName("firstFrame").get());
    assert(firstFrameKnob);
    Knob<int> *lastFrameKnob = dynamic_cast<Knob<int> *>(node->getKnobByName("lastFrame").get());
    assert(lastFrameKnob);

    int startingTimeValue = startingTimeKnob->getValue();
    int firstFrameValue = firstFrameKnob->getValue();
    int lastFrameValue = lastFrameKnob->getValue();

    FrameRange range(startingTimeValue,
                     startingTimeValue + (lastFrameValue - firstFrameValue) + 1);

    nodeRanges[reader] = range;

    if (DSNode *isInGroup = model->getGroupDSNode(reader)) {
        computeGroupRange(isInGroup);
    }
}

void DopeSheetViewPrivate::computeRetimeRange(DSNode *retimer)
{
    NodePtr node = retimer->getInternalNode();

    if (Natron::Node *nearestReader = model->getNearestReader(retimer)) {
        Knob<int> *startingTimeKnob = dynamic_cast<Knob<int> *>(nearestReader->getKnobByName("startingTime").get());
        assert(startingTimeKnob);
        Knob<int> *firstFrameKnob = dynamic_cast<Knob<int> *>(nearestReader->getKnobByName("firstFrame").get());
        assert(firstFrameKnob);
        Knob<int> *lastFrameKnob = dynamic_cast<Knob<int> *>(nearestReader->getKnobByName("lastFrame").get());
        assert(lastFrameKnob);

        int startingTimeValue = startingTimeKnob->getValue();
        int firstFrameValue = firstFrameKnob->getValue();
        int lastFrameValue = lastFrameKnob->getValue();

        Knob<double> *speedKnob =  dynamic_cast<Knob<double> *>(node->getKnobByName("speed").get());
        assert(speedKnob);

        double speedValue = speedKnob->getValue();

        int frameCount = lastFrameValue - firstFrameValue + 1;
        int rangeEnd = startingTimeValue + (frameCount / speedValue);

        FrameRange range;
        range.first = startingTimeValue;
        range.second = rangeEnd;

        nodeRanges[retimer] = range;
    }
    else {
        nodeRanges[retimer] = FrameRange();
    }
}

void DopeSheetViewPrivate::computeTimeOffsetRange(DSNode *timeOffset)
{
    FrameRange range(0, 0);

    // Retrieve nearest reader useful values
    if (DSNode *nearestReader = model->findDSNode(model->getNearestReader(timeOffset))) {
        FrameRange nearestReaderRange = nodeRanges.at(nearestReader);

        // Retrieve the time offset values
        Knob<int> *timeOffsetKnob = dynamic_cast<Knob<int> *>(timeOffset->getInternalNode()->getKnobByName("timeOffset").get());
        assert(timeOffsetKnob);

        int timeOffsetValue = timeOffsetKnob->getValue();

        range.first = nearestReaderRange.first + timeOffsetValue;
        range.second = nearestReaderRange.second + timeOffsetValue;
    }

    nodeRanges[timeOffset] = range;
}

void DopeSheetViewPrivate::computeFRRange(DSNode *frameRange)
{
    NodePtr node = frameRange->getInternalNode();

    Knob<int> *frameRangeKnob = dynamic_cast<Knob<int> *>(node->getKnobByName("frameRange").get());
    assert(frameRangeKnob);

    FrameRange range;
    range.first = frameRangeKnob->getValue(0);
    range.second = frameRangeKnob->getValue(1);

    nodeRanges[frameRange] = range;
}

void DopeSheetViewPrivate::computeGroupRange(DSNode *group)
{
    NodePtr node = group->getInternalNode();

    FrameRange range;
    std::set<double> times;

    NodeList nodes = dynamic_cast<NodeGroup *>(node->getLiveInstance())->getNodes();

    for (NodeList::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        NodePtr node = (*it);

        if (!model->findDSNode(node.get())) {
            continue;
        }

        boost::shared_ptr<NodeGui> nodeGui = boost::dynamic_pointer_cast<NodeGui>(node->getNodeGui());

        if (!nodeGui->getSettingPanel() || !nodeGui->isSettingsPanelVisible()) {
            continue;
        }

        std::string pluginID = node->getPluginID();

        if (pluginID == PLUGINID_OFX_READOIIO ||
                pluginID == PLUGINID_OFX_READFFMPEG ||
                pluginID == PLUGINID_OFX_READPFM) {
            Knob<int> *startingTimeKnob = dynamic_cast<Knob<int> *>(node->getKnobByName("startingTime").get());
            assert(startingTimeKnob);
            Knob<int> *firstFrameKnob = dynamic_cast<Knob<int> *>(node->getKnobByName("firstFrame").get());
            assert(firstFrameKnob);
            Knob<int> *lastFrameKnob = dynamic_cast<Knob<int> *>(node->getKnobByName("lastFrame").get());
            assert(lastFrameKnob);

            int startingTimeValue = startingTimeKnob->getValue();
            int firstFrameValue = firstFrameKnob->getValue();
            int lastFrameValue = lastFrameKnob->getValue();

            times.insert(startingTimeValue);
            times.insert(startingTimeValue + (lastFrameValue - firstFrameValue) + 1);
        }

        const KnobsAndGuis &knobs = nodeGui->getKnobs();

        for (KnobsAndGuis::const_iterator it = knobs.begin();
             it != knobs.end();
             ++it) {
            KnobGui *knobGui = (*it).second;
            boost::shared_ptr<KnobI> knob = knobGui->getKnob();

            if (!knob->isAnimationEnabled() || !knob->hasAnimation()) {
                continue;
            }
            else {
                for (int i = 0; i < knob->getDimension(); ++i) {
                    KeyFrameSet keyframes = knob->getCurve(i)->getKeyFrames_mt_safe();

                    if (keyframes.empty()) {
                        continue;
                    }

                    times.insert(keyframes.begin()->getTime());
                    times.insert(keyframes.rbegin()->getTime());
                }
            }
        }
    }

    if (times.size() <= 1) {
        range.first = 0;
        range.second = 0;
    }
    else {
        range.first = *times.begin();
        range.second = *times.rbegin();
    }

    nodeRanges[group] = range;
}

void DopeSheetViewPrivate::onMouseDrag(QMouseEvent *e)
{
    QPointF mouseZoomCoords = zoomContext.toZoomCoordinates(e->x(), e->y());
    QPointF lastZoomCoordsOnMousePress = zoomContext.toZoomCoordinates(lastPosOnMousePress.x(),
                                                                       lastPosOnMousePress.y());
    double currentTime = mouseZoomCoords.x();

    double dt = clampedMouseOffset(lastZoomCoordsOnMousePress.x(), currentTime);

    switch (eventState) {
    case DopeSheetView::esMoveKeyframeSelection:
    {
        if (dt >= 1.0f || dt <= -1.0f) {
            model->moveSelectedKeys(dt);
        }

        break;
    }
    case DopeSheetView::esMoveCurrentFrameIndicator:
        moveCurrentFrameIndicator(mouseZoomCoords.x());

        break;
    case DopeSheetView::esSelectionByRect:
    {
        computeSelectionRect(lastZoomCoordsOnMousePress, mouseZoomCoords);

        q_ptr->redraw();

        break;
    }
    case DopeSheetView::esReaderLeftTrim:
    {
        if (dt >= 1.0f || dt <= -1.0f) {
            Knob<int> *timeOffsetKnob = dynamic_cast<Knob<int> *>(currentEditedReader->getInternalNode()->getKnobByName("timeOffset").get());
            assert(timeOffsetKnob);

            double newFirstFrame = std::floor(currentTime - timeOffsetKnob->getValue() + 0.5);

            model->trimReaderLeft(currentEditedReader, newFirstFrame);
        }

        break;
    }
    case DopeSheetView::esReaderRightTrim:
    {
        if (dt >= 1.0f || dt <= -1.0f) {
            Knob<int> *timeOffsetKnob = dynamic_cast<Knob<int> *>(currentEditedReader->getInternalNode()->getKnobByName("timeOffset").get());
            assert(timeOffsetKnob);

            double newLastFrame = std::floor(currentTime - timeOffsetKnob->getValue() + 0.5);

            model->trimReaderRight(currentEditedReader, newLastFrame);
        }

        break;
    }
    case DopeSheetView::esReaderSlip:
    {
        if (dt >= 1.0f || dt <= -1.0f) {
            model->slipReader(currentEditedReader, dt);
        }

        break;
    }
    case DopeSheetView::esReaderRepos:
    {
        if (dt >= 1.0f || dt <= -1.0f) {
            model->moveReader(currentEditedReader, dt);
        }

        break;
    }
    case DopeSheetView::esGroupRepos:
    {
        if (dt >= 1.0f || dt <= -1.0f) {
            model->moveGroup(currentEditedGroup, dt);
        }

        break;
    }
    case DopeSheetView::esNoEditingState:
        eventState = DopeSheetView::esSelectionByRect;

        break;
    default:
        break;
    }
}

void DopeSheetViewPrivate::createSelectionFromRect(const QRectF &rect, std::vector<DSSelectedKey> *result)
{
    QRectF zoomCoordsRect = rectToZoomCoordinates(rect);

    DSNodeRows dsNodes = model->getNodeRows();

    for (DSNodeRows::const_iterator it = dsNodes.begin(); it != dsNodes.end(); ++it) {
        DSNode *dsNode = (*it).second;

        DSKnobRows dsKnobs = dsNode->getChildData();

        for (DSKnobRows::const_iterator it2 = dsKnobs.begin(); it2 != dsKnobs.end(); ++it2) {
            DSKnob *dsKnob = (*it2).second;
            int dim = dsKnob->getDimension();

            if (dim == -1) {
                continue;
            }

            KeyFrameSet keyframes = dsKnob->getKnobGui()->getCurve(dim)->getKeyFrames_mt_safe();

            for (KeyFrameSet::const_iterator kIt = keyframes.begin();
                 kIt != keyframes.end();
                 ++kIt) {
                KeyFrame kf = (*kIt);

                double x = kf.getTime();
                double y = hierarchyView->getItemRect(dsKnob).center().y();

                QRectF zoomKfRect = rectToZoomCoordinates(keyframeRect(x, y));

                if (zoomCoordsRect.intersects(zoomKfRect)) {
                    result->push_back(DSSelectedKey(dsKnob, kf));
                }
            }
        }
    }
}

void DopeSheetViewPrivate::moveCurrentFrameIndicator(double toTime)
{
    gui->getApp()->setLastViewerUsingTimeline(boost::shared_ptr<Natron::Node>());

    timeline->seekFrame(SequenceTime(toTime), false, 0, Natron::eTimelineChangeReasonDopeSheetEditorSeek);
}

void DopeSheetViewPrivate::createContextMenu()
{
    running_in_main_thread();

    contextMenu->clear();

    // Create menus

    // Edit menu
    Natron::Menu *editMenu = new Natron::Menu(contextMenu);
    editMenu->setTitle(QObject::tr("Edit"));

    contextMenu->addAction(editMenu->menuAction());

    // Interpolation menu
    Natron::Menu *interpMenu = new Natron::Menu(contextMenu);
    interpMenu->setTitle(QObject::tr("Interpolation"));

    contextMenu->addAction(interpMenu->menuAction());

    // View menu
    Natron::Menu *viewMenu = new Natron::Menu(contextMenu);
    viewMenu->setTitle(QObject::tr("View"));

    contextMenu->addAction(viewMenu->menuAction());

    // Create actions

    // Edit actions
    QAction *removeSelectedKeyframesAction = new ActionWithShortcut(kShortcutGroupDopeSheetEditor,
                                                                    kShortcutIDActionDopeSheetEditorDeleteKeys,
                                                                    kShortcutDescActionDopeSheetEditorDeleteKeys,
                                                                    editMenu);
    QObject::connect(removeSelectedKeyframesAction, SIGNAL(triggered()),
                     q_ptr, SLOT(deleteSelectedKeyframes()));
    editMenu->addAction(removeSelectedKeyframesAction);

    QAction *copySelectedKeyframesAction = new ActionWithShortcut(kShortcutGroupDopeSheetEditor,
                                                                  kShortcutIDActionDopeSheetEditorCopySelectedKeyframes,
                                                                  kShortcutDescActionDopeSheetEditorCopySelectedKeyframes,
                                                                  editMenu);
    QObject::connect(copySelectedKeyframesAction, SIGNAL(triggered()),
                     q_ptr, SLOT(copySelectedKeyframes()));
    editMenu->addAction(copySelectedKeyframesAction);

    QAction *pasteKeyframesAction = new ActionWithShortcut(kShortcutGroupDopeSheetEditor,
                                                           kShortcutIDActionDopeSheetEditorPasteKeyframes,
                                                           kShortcutDescActionDopeSheetEditorPasteKeyframes,
                                                           editMenu);
    QObject::connect(pasteKeyframesAction, SIGNAL(triggered()),
                     q_ptr, SLOT(pasteKeyframes()));
    editMenu->addAction(pasteKeyframesAction);

    QAction *selectAllKeyframesAction = new ActionWithShortcut(kShortcutGroupDopeSheetEditor,
                                                               kShortcutIDActionDopeSheetEditorSelectAllKeyframes,
                                                               kShortcutDescActionDopeSheetEditorSelectAllKeyframes,
                                                               editMenu);
    QObject::connect(selectAllKeyframesAction, SIGNAL(triggered()),
                     q_ptr, SLOT(selectAllKeyframes()));
    editMenu->addAction(selectAllKeyframesAction);


    // Interpolation actions
    QAction *constantInterpAction = new ActionWithShortcut(kShortcutGroupDopeSheetEditor,
                                                           kShortcutIDActionCurveEditorConstant,
                                                           kShortcutDescActionCurveEditorConstant,
                                                           interpMenu);
    QPixmap pix;
    appPTR->getIcon(Natron::NATRON_PIXMAP_INTERP_CONSTANT, &pix);
    constantInterpAction->setIcon(QIcon(pix));
    constantInterpAction->setIconVisibleInMenu(true);

    QObject::connect(constantInterpAction, SIGNAL(triggered()),
                     q_ptr, SLOT(constantInterpSelectedKeyframes()));

    interpMenu->addAction(constantInterpAction);

    QAction *linearInterpAction = new ActionWithShortcut(kShortcutGroupDopeSheetEditor,
                                                         kShortcutIDActionCurveEditorLinear,
                                                         kShortcutDescActionCurveEditorLinear,
                                                         interpMenu);

    appPTR->getIcon(Natron::NATRON_PIXMAP_INTERP_LINEAR, &pix);
    linearInterpAction->setIcon(QIcon(pix));
    linearInterpAction->setIconVisibleInMenu(true);

    QObject::connect(linearInterpAction, SIGNAL(triggered()),
                     q_ptr, SLOT(linearInterpSelectedKeyframes()));

    interpMenu->addAction(linearInterpAction);

    QAction *smoothInterpAction = new ActionWithShortcut(kShortcutGroupDopeSheetEditor,
                                                         kShortcutIDActionCurveEditorSmooth,
                                                         kShortcutDescActionCurveEditorSmooth,
                                                         interpMenu);

    appPTR->getIcon(Natron::NATRON_PIXMAP_INTERP_CURVE_Z, &pix);
    smoothInterpAction->setIcon(QIcon(pix));
    smoothInterpAction->setIconVisibleInMenu(true);

    QObject::connect(smoothInterpAction, SIGNAL(triggered()),
                     q_ptr, SLOT(smoothInterpSelectedKeyframes()));

    interpMenu->addAction(smoothInterpAction);

    QAction *catmullRomInterpAction = new ActionWithShortcut(kShortcutGroupDopeSheetEditor,
                                                             kShortcutIDActionCurveEditorCatmullrom,
                                                             kShortcutDescActionCurveEditorCatmullrom,
                                                             interpMenu);
    appPTR->getIcon(Natron::NATRON_PIXMAP_INTERP_CURVE_R, &pix);
    catmullRomInterpAction->setIcon(QIcon(pix));
    catmullRomInterpAction->setIconVisibleInMenu(true);

    QObject::connect(catmullRomInterpAction, SIGNAL(triggered()),
                     q_ptr, SLOT(catmullRomInterpSelectedKeyframes()));

    interpMenu->addAction(catmullRomInterpAction);

    QAction *cubicInterpAction = new ActionWithShortcut(kShortcutGroupDopeSheetEditor,
                                                        kShortcutIDActionCurveEditorCubic,
                                                        kShortcutDescActionCurveEditorCubic,
                                                        interpMenu);
    appPTR->getIcon(Natron::NATRON_PIXMAP_INTERP_CURVE_C, &pix);
    cubicInterpAction->setIcon(QIcon(pix));
    cubicInterpAction->setIconVisibleInMenu(true);

    QObject::connect(cubicInterpAction, SIGNAL(triggered()),
                     q_ptr, SLOT(cubicInterpSelectedKeyframes()));

    interpMenu->addAction(cubicInterpAction);

    QAction *horizontalInterpAction = new ActionWithShortcut(kShortcutGroupDopeSheetEditor,
                                                             kShortcutIDActionCurveEditorHorizontal,
                                                             kShortcutDescActionCurveEditorHorizontal,
                                                             interpMenu);
    appPTR->getIcon(Natron::NATRON_PIXMAP_INTERP_CURVE_H, &pix);
    horizontalInterpAction->setIcon(QIcon(pix));
    horizontalInterpAction->setIconVisibleInMenu(true);

    QObject::connect(horizontalInterpAction, SIGNAL(triggered()),
                     q_ptr, SLOT(horizontalInterpSelectedKeyframes()));

    interpMenu->addAction(horizontalInterpAction);

    QAction *breakInterpAction = new ActionWithShortcut(kShortcutGroupDopeSheetEditor,
                                                        kShortcutIDActionCurveEditorBreak,
                                                        kShortcutDescActionCurveEditorBreak,
                                                        interpMenu);
    appPTR->getIcon(Natron::NATRON_PIXMAP_INTERP_BREAK, &pix);
    breakInterpAction->setIcon(QIcon(pix));
    breakInterpAction->setIconVisibleInMenu(true);

    QObject::connect(breakInterpAction, SIGNAL(triggered()),
                     q_ptr, SLOT(breakInterpSelectedKeyframes()));

    interpMenu->addAction(breakInterpAction);

    // View actions
    QAction *frameSelectionAction = new ActionWithShortcut(kShortcutGroupDopeSheetEditor,
                                                           kShortcutIDActionDopeSheetEditorFrameSelection,
                                                           kShortcutDescActionDopeSheetEditorFrameSelection,
                                                           viewMenu);
    QObject::connect(frameSelectionAction, SIGNAL(triggered()),
                     q_ptr, SLOT(frame()));
    viewMenu->addAction(frameSelectionAction);
}

void DopeSheetViewPrivate::updateCurveWidgetFrameRange()
{
    CurveWidget *curveWidget = gui->getCurveEditor()->getCurveWidget();

    curveWidget->centerOn(zoomContext.left(), zoomContext.right());
}


/**
 * @brief DopeSheetView::DopeSheetView
 *
 * Constructs a DopeSheetView object.
 */
DopeSheetView::DopeSheetView(DopeSheet *model, HierarchyView *hierarchyView,
                             Gui *gui,
                             const boost::shared_ptr<TimeLine> &timeline,
                             QWidget *parent) :
    QGLWidget(parent),
    _imp(new DopeSheetViewPrivate(this))
{
    _imp->model = model;
    _imp->hierarchyView = hierarchyView;
    _imp->gui = gui;
    _imp->timeline = timeline;

    setMouseTracking(true);

    if (timeline) {
        boost::shared_ptr<Natron::Project> project = gui->getApp()->getProject();
        assert(project);

        connect(timeline.get(), SIGNAL(frameChanged(SequenceTime,int)), this, SLOT(onTimeLineFrameChanged(SequenceTime,int)));
        connect(project.get(), SIGNAL(frameRangeChanged(int,int)), this, SLOT(onTimeLineBoundariesChanged(int,int)));

        onTimeLineFrameChanged(timeline->currentFrame(), Natron::eValueChangedReasonNatronGuiEdited);

        int left,right;
        project->getFrameRange(&left, &right);
        onTimeLineBoundariesChanged(left, right);
    }

    connect(_imp->model, SIGNAL(nodeAdded(DSNode*)),
            this, SLOT(onNodeAdded(DSNode *)));

    connect(_imp->model, SIGNAL(nodeAboutToBeRemoved(DSNode*)),
            this, SLOT(onNodeAboutToBeRemoved(DSNode *)));

    connect(_imp->model, SIGNAL(modelChanged()),
            this, SLOT(redraw()));

    connect(_imp->model->getSelectionModel(), SIGNAL(keyframeSelectionChanged()),
            this, SLOT(onKeyframeSelectionChanged()));

    connect(_imp->hierarchyView->verticalScrollBar(), SIGNAL(valueChanged(int)),
            this, SLOT(onHierarchyViewScrollbarMoved(int)));

    connect(_imp->hierarchyView, SIGNAL(itemExpanded(QTreeWidgetItem*)),
            this, SLOT(onHierarchyViewItemExpandedOrCollapsed(QTreeWidgetItem*)));

    connect(_imp->hierarchyView, SIGNAL(itemCollapsed(QTreeWidgetItem*)),
            this, SLOT(onHierarchyViewItemExpandedOrCollapsed(QTreeWidgetItem*)));
}

/**
 * @brief DopeSheetView::~DopeSheetView
 *
 * Destroys the DopeSheetView object.
 */
DopeSheetView::~DopeSheetView()
{

}

void DopeSheetView::centerOn(double xMin, double xMax)
{
    _imp->zoomContext.fill(xMin, xMax, _imp->zoomContext.bottom(), _imp->zoomContext.top());

    redraw();
}

/**
 * @brief DopeSheetView::swapOpenGLBuffers
 *
 *
 */
void DopeSheetView::swapOpenGLBuffers()
{
    running_in_main_thread();

    swapBuffers();
}

/**
 * @brief DopeSheetView::redraw
 *
 *
 */
void DopeSheetView::redraw()
{
    running_in_main_thread();

    update();
}

/**
 * @brief DopeSheetView::getViewportSize
 *
 *
 */
void DopeSheetView::getViewportSize(double &width, double &height) const
{
    running_in_main_thread();

    width = this->width();
    height = this->height();
}

/**
 * @brief DopeSheetView::getPixelScale
 *
 *
 */
void DopeSheetView::getPixelScale(double &xScale, double &yScale) const
{
    running_in_main_thread();

    xScale = _imp->zoomContext.screenPixelWidth();
    yScale = _imp->zoomContext.screenPixelHeight();
}

/**
 * @brief DopeSheetView::getBackgroundColour
 *
 *
 */
void DopeSheetView::getBackgroundColour(double &r, double &g, double &b) const
{
    running_in_main_thread();

    // use the same settings as the curve editor
    appPTR->getCurrentSettings()->getCurveEditorBGColor(&r, &g, &b);
}

/**
 * @brief DopeSheetView::saveOpenGLContext
 *
 *
 */
void DopeSheetView::saveOpenGLContext()
{
    running_in_main_thread();


}

/**
 * @brief DopeSheetView::restoreOpenGLContext
 *
 *
 */
void DopeSheetView::restoreOpenGLContext()
{
    running_in_main_thread();

}

/**
 * @brief DopeSheetView::getCurrentRenderScale
 *
 *
 */
unsigned int DopeSheetView::getCurrentRenderScale() const
{
    return 0;
}

void DopeSheetView::selectAllKeyframes()
{
    _imp->model->getSelectionModel()->selectAllKeyframes();

    if (_imp->model->getSelectionModel()->getSelectedKeyframesCount() > 1) {
        _imp->computeSelectedKeysBRect();
    }

    redraw();
}

void DopeSheetView::deleteSelectedKeyframes()
{
    running_in_main_thread();

    _imp->model->deleteSelectedKeyframes();

    redraw();
}

void DopeSheetView::centerOn()
{
    running_in_main_thread();

    int selectedKeyframesCount = _imp->model->getSelectionModel()->getSelectedKeyframesCount();

    if (selectedKeyframesCount == 1) {
        return;
    }

    FrameRange range;

    // frame on project bounds
    if (!selectedKeyframesCount) {
        range = _imp->model->getKeyframeRange();
    }
    // or frame on current selection
    else {
        range.first = _imp->selectedKeysBRect.left();
        range.second = _imp->selectedKeysBRect.right();
    }

    if (range.first == 0 && range.second == 0) {
        return;
    }

    _imp->zoomContext.fill(range.first, range.second,
                           _imp->zoomContext.bottom(), _imp->zoomContext.top());

    _imp->computeTimelinePositions();

    if (selectedKeyframesCount > 1) {
        _imp->computeSelectedKeysBRect();
    }

    redraw();
}

void DopeSheetView::constantInterpSelectedKeyframes()
{
    running_in_main_thread();

    _imp->model->setSelectedKeysInterpolation(Natron::eKeyframeTypeConstant);
}

void DopeSheetView::linearInterpSelectedKeyframes()
{
    running_in_main_thread();

    _imp->model->setSelectedKeysInterpolation(Natron::eKeyframeTypeLinear);
}

void DopeSheetView::smoothInterpSelectedKeyframes()
{
    running_in_main_thread();

    _imp->model->setSelectedKeysInterpolation(Natron::eKeyframeTypeSmooth);
}

void DopeSheetView::catmullRomInterpSelectedKeyframes()
{
    running_in_main_thread();

    _imp->model->setSelectedKeysInterpolation(Natron::eKeyframeTypeCatmullRom);
}

void DopeSheetView::cubicInterpSelectedKeyframes()
{
    running_in_main_thread();

    _imp->model->setSelectedKeysInterpolation(Natron::eKeyframeTypeCubic);
}

void DopeSheetView::horizontalInterpSelectedKeyframes()
{
    running_in_main_thread();

    _imp->model->setSelectedKeysInterpolation(Natron::eKeyframeTypeHorizontal);
}

void DopeSheetView::breakInterpSelectedKeyframes()
{
    running_in_main_thread();

    _imp->model->setSelectedKeysInterpolation(Natron::eKeyframeTypeBroken);
}

void DopeSheetView::copySelectedKeyframes()
{
    running_in_main_thread();

    _imp->model->copySelectedKeys();
}

void DopeSheetView::pasteKeyframes()
{
    running_in_main_thread();

    _imp->model->pasteKeys();
}

void DopeSheetView::onTimeLineFrameChanged(SequenceTime sTime, int reason)
{
    Q_UNUSED(sTime);
    Q_UNUSED(reason);

    running_in_main_thread();

    if (_imp->gui->isGUIFrozen()) {
        return;
    }

    _imp->computeTimelinePositions();

    redraw();
}

void DopeSheetView::onTimeLineBoundariesChanged(int, int)
{
    running_in_main_thread();

    redraw();
}

void DopeSheetView::onNodeAdded(DSNode *dsNode)
{
    DopeSheet::ItemType nodeType = dsNode->getItemType();
    NodePtr node = dsNode->getInternalNode();

    bool mustComputeNodeRange = true;

    if (nodeType == DopeSheet::ItemTypeCommon) {
        if (_imp->model->isPartOfGroup(dsNode)) {
            const KnobsAndGuis &knobs = dsNode->getNodeGui()->getKnobs();

            for (KnobsAndGuis::const_iterator knobIt = knobs.begin(); knobIt != knobs.end(); ++knobIt) {
                boost::shared_ptr<KnobI> knob = knobIt->first.lock();
                KnobGui *knobGui = knobIt->second;
                connect(knob->getSignalSlotHandler().get(), SIGNAL(keyFrameMoved(int,int,int)),
                        this, SLOT(onKeyframeChanged()));

                connect(knobGui, SIGNAL(keyFrameSet()),
                        this, SLOT(onKeyframeChanged()));

                connect(knobGui, SIGNAL(keyFrameRemoved()),
                        this, SLOT(onKeyframeChanged()));
            }
        }

        mustComputeNodeRange = false;
    }
    else if (nodeType == DopeSheet::ItemTypeReader) {
        // The dopesheet view must refresh if the user set some values in the settings panel
        // so we connect some signals/slots
        boost::shared_ptr<KnobSignalSlotHandler> lastFrameKnob =  node->getKnobByName("lastFrame")->getSignalSlotHandler();
        assert(lastFrameKnob);
        boost::shared_ptr<KnobSignalSlotHandler> startingTimeKnob = node->getKnobByName("startingTime")->getSignalSlotHandler();
        assert(startingTimeKnob);

        connect(lastFrameKnob.get(), SIGNAL(valueChanged(int, int)),
                this, SLOT(onRangeNodeChanged(int, int)));

        connect(startingTimeKnob.get(), SIGNAL(valueChanged(int, int)),
                this, SLOT(onRangeNodeChanged(int, int)));

        // We don't make the connection for the first frame knob, because the
        // starting time is updated when it's modified. Thus we avoid two
        // refreshes of the view.
    }
    else if (nodeType == DopeSheet::ItemTypeRetime) {
        boost::shared_ptr<KnobSignalSlotHandler> speedKnob =  node->getKnobByName("speed")->getSignalSlotHandler();
        assert(speedKnob);

        connect(speedKnob.get(), SIGNAL(valueChanged(int, int)),
                this, SLOT(onRangeNodeChanged(int, int)));
    }
    else if (nodeType == DopeSheet::ItemTypeTimeOffset) {
        boost::shared_ptr<KnobSignalSlotHandler> timeOffsetKnob =  node->getKnobByName("timeOffset")->getSignalSlotHandler();
        assert(timeOffsetKnob);

        connect(timeOffsetKnob.get(), SIGNAL(valueChanged(int, int)),
                this, SLOT(onRangeNodeChanged(int, int)));
    }
    else if (nodeType == DopeSheet::ItemTypeFrameRange) {
        boost::shared_ptr<KnobSignalSlotHandler> frameRangeKnob =  node->getKnobByName("frameRange")->getSignalSlotHandler();
        assert(frameRangeKnob);

        connect(frameRangeKnob.get(), SIGNAL(valueChanged(int, int)),
                this, SLOT(onRangeNodeChanged(int, int)));
    }

    if (mustComputeNodeRange) {
        _imp->computeNodeRange(dsNode);
    }

    if (DSNode *parentGroupDSNode = _imp->model->getGroupDSNode(dsNode)) {
        _imp->computeGroupRange(parentGroupDSNode);
    }
}

void DopeSheetView::onNodeAboutToBeRemoved(DSNode *dsNode)
{
    if (DSNode *parentGroupDSNode = _imp->model->getGroupDSNode(dsNode)) {
        _imp->computeGroupRange(parentGroupDSNode);
    }

    std::map<DSNode *, FrameRange>::iterator toRemove = _imp->nodeRanges.find(dsNode);

    if (toRemove != _imp->nodeRanges.end()) {
        _imp->nodeRanges.erase(toRemove);
    }

    _imp->computeSelectedKeysBRect();

    redraw();
}

void DopeSheetView::onKeyframeChanged()
{
    QObject *signalSender = sender();

    DSNode *dsNode = 0;

    if (KnobSignalSlotHandler *knobHandler = qobject_cast<KnobSignalSlotHandler *>(signalSender)) {
        dsNode = _imp->model->findDSNode(knobHandler->getKnob());
    }
    else if (KnobGui *knobGui = qobject_cast<KnobGui *>(signalSender)) {
        dsNode = _imp->model->findDSNode(knobGui->getKnob());
    }

    if (!dsNode) {
        return;
    }

    if (DSNode *parentGroupDSNode = _imp->model->getGroupDSNode(dsNode)) {
        _imp->computeGroupRange(parentGroupDSNode);
    }
}

void DopeSheetView::onRangeNodeChanged(int /*dimension*/, int /*reason*/)
{
    QObject *signalSender = sender();

    DSNode *dsNode = 0;

    if (KnobSignalSlotHandler *knobHandler = qobject_cast<KnobSignalSlotHandler *>(signalSender)) {
        KnobHolder *holder = knobHandler->getKnob()->getHolder();
        Natron::EffectInstance *effectInstance = dynamic_cast<Natron::EffectInstance *>(holder);

        dsNode = _imp->model->findDSNode(effectInstance->getNode().get());
    }

    assert(dsNode);

    _imp->computeNodeRange(dsNode);

    redraw();
}

void DopeSheetView::onHierarchyViewItemExpandedOrCollapsed(QTreeWidgetItem *item)
{
    // Compute the range rects of affected items
    if (DSNode *dsNode = _imp->model->findParentDSNode(item)) {
        _imp->computeRangesBelow(dsNode);
    }

    _imp->computeSelectedKeysBRect();

    redraw();
}

void DopeSheetView::onHierarchyViewScrollbarMoved(int /*value*/)
{
    redraw();
}

void DopeSheetView::onKeyframeSelectionChanged()
{
    _imp->computeSelectedKeysBRect();

    redraw();
}

/**
 * @brief DopeSheetView::initializeGL
 *
 *
 */
void DopeSheetView::initializeGL()
{
    running_in_main_thread();

    if ( !glewIsSupported("GL_ARB_vertex_array_object ")) {
        _imp->hasOpenGLVAOSupport = false;
    }

    _imp->generateKeyframeTextures();
}

/**
 * @brief DopeSheetView::resizeGL
 *
 *
 */
void DopeSheetView::resizeGL(int w, int h)
{
    running_in_main_thread_and_context(this);

    if (h == 0) {

    }

    glViewport(0, 0, w, h);

    _imp->zoomContext.setScreenSize(w, h);

    // Don't do the following when the height of the widget is irrelevant
    if (h == 1) {
        return;
    }

    // Find out what are the selected keyframes and center on them
    if (!_imp->zoomOrPannedSinceLastFit) {
        //TODO see CurveWidget::resizeGL
    }
}

/**
 * @brief DopeSheetView::paintGL
 *
 *
 */
void DopeSheetView::paintGL()
{
    running_in_main_thread_and_context(this);

    glCheckError();

    if (_imp->zoomContext.factor() <= 0) {
        return;
    }

    double zoomLeft, zoomRight, zoomBottom, zoomTop;
    zoomLeft = _imp->zoomContext.left();
    zoomRight = _imp->zoomContext.right();
    zoomBottom = _imp->zoomContext.bottom();
    zoomTop = _imp->zoomContext.top();

    // Retrieve the appropriate settings for drawing
    boost::shared_ptr<Settings> settings = appPTR->getCurrentSettings();
    double bgR, bgG, bgB;
    settings->getDopeSheetEditorBackgroundColor(&bgR, &bgG, &bgB);

    if ((zoomLeft == zoomRight) || (zoomTop == zoomBottom)) {
        glClearColor(bgR, bgG, bgB, 1.);
        glClear(GL_COLOR_BUFFER_BIT);

        return;
    }

    {
        GLProtectAttrib a(GL_TRANSFORM_BIT | GL_COLOR_BUFFER_BIT);
        GLProtectMatrix p(GL_PROJECTION);

        glLoadIdentity();
        glOrtho(zoomLeft, zoomRight, zoomBottom, zoomTop, 1, -1);

        GLProtectMatrix m(GL_MODELVIEW);

        glLoadIdentity();

        glCheckError();

        glClearColor(bgR, bgG, bgB, 1.);
        glClear(GL_COLOR_BUFFER_BIT);

        _imp->drawScale();
        _imp->drawProjectBounds();
        _imp->drawRows();

        if (_imp->eventState == DopeSheetView::esSelectionByRect) {
            _imp->drawSelectionRect();
        }

        if (_imp->rectToZoomCoordinates(_imp->selectedKeysBRect).isValid()) {
            _imp->drawSelectedKeysBRect();
        }

        _imp->drawCurrentFrameIndicator();
    }
}

void DopeSheetView::mousePressEvent(QMouseEvent *e)
{
    running_in_main_thread();

    if ( buttonDownIsRight(e) ) {
        _imp->createContextMenu();
        _imp->contextMenu->exec(mapToGlobal(e->pos()));

        e->accept();

        return;
    }

    if (buttonDownIsMiddle(e)) {
        _imp->eventState = DopeSheetView::esDraggingView;
    }

    QPointF clickZoomCoords = _imp->zoomContext.toZoomCoordinates(e->x(), e->y());

    if (buttonDownIsLeft(e)) {
        if (_imp->isNearByCurrentFrameIndicatorBottom(clickZoomCoords)) {
            _imp->eventState = DopeSheetView::esMoveCurrentFrameIndicator;
        }
        if (_imp->rectToZoomCoordinates(_imp->selectedKeysBRect).contains(clickZoomCoords)) {
            _imp->eventState = DopeSheetView::esMoveKeyframeSelection;
        }
        else if (QTreeWidgetItem *treeItem = _imp->hierarchyView->itemAt(0, e->y())) {
            DSNodeRows dsNodeItems = _imp->model->getNodeRows();
            DSNodeRows::const_iterator dsNodeIt = dsNodeItems.find(treeItem);

            // The user clicked on a reader
            if (dsNodeIt != dsNodeItems.end()) {
                DSNode *dsNode = (*dsNodeIt).second;
                DopeSheet::ItemType nodeType = dsNode->getItemType();

                QRectF treeItemRect = _imp->hierarchyView->getItemRect(dsNode);

                if (dsNode->isRangeDrawingEnabled()) {
                    FrameRange range = _imp->nodeRanges[dsNode];
                    QRectF nodeClipRect = _imp->rectToZoomCoordinates(QRectF(QPointF(range.first, treeItemRect.top() + 1),
                                                                             QPointF(range.second, treeItemRect.bottom() + 1)));

                    if (nodeType == DopeSheet::ItemTypeGroup) {
                        if (nodeClipRect.contains(clickZoomCoords.x(), clickZoomCoords.y())) {
                            _imp->currentEditedGroup = dsNode;

                            _imp->eventState = DopeSheetView::esGroupRepos;
                        }
                    }
                    else if (nodeType == DopeSheet::ItemTypeReader) {
                        if (nodeClipRect.contains(clickZoomCoords.x(), clickZoomCoords.y())) {
                            _imp->currentEditedReader = dsNode;

                            if (_imp->isNearByClipRectLeft(clickZoomCoords.x(), nodeClipRect)) {
                                _imp->eventState = DopeSheetView::esReaderLeftTrim;
                            }
                            else if (_imp->isNearByClipRectRight(clickZoomCoords.x(), nodeClipRect)) {
                                _imp->eventState = DopeSheetView::esReaderRightTrim;
                            }
                            else if (_imp->isNearByClipRectBottom(clickZoomCoords.y(), nodeClipRect)) {
                                _imp->eventState = DopeSheetView::esReaderSlip;
                            }
                            else {
                                _imp->eventState = DopeSheetView::esReaderRepos;
                            }

                            Knob<int> *timeOffsetKnob = dynamic_cast<Knob<int> *>(_imp->currentEditedReader->getInternalNode()->getKnobByName("timeOffset").get());
                            assert(timeOffsetKnob);

                            _imp->lastTimeOffsetOnMousePress = timeOffsetKnob->getValue();
                        }
                    }
                }
                else if (nodeType == DopeSheet::ItemTypeCommon) {
                    std::vector<DSSelectedKey> keysUnderMouse = _imp->isNearByKeyframe(dsNode, e->pos());

                    if (!keysUnderMouse.empty()) {
                        _imp->model->getSelectionModel()->makeSelection(keysUnderMouse);

                        _imp->eventState = DopeSheetView::esMoveKeyframeSelection;
                    }
                }
            }
            // Or search for a keyframe
            else {
                DSKnob *dsKnob = _imp->hierarchyView->getDSKnobAt(e->pos().y());

                if (dsKnob) {
                    std::vector<DSSelectedKey> keysUnderMouse = _imp->isNearByKeyframe(dsKnob, e->pos());

                    if (!keysUnderMouse.empty()) {
                        _imp->model->getSelectionModel()->makeSelection(keysUnderMouse);

                        _imp->eventState = DopeSheetView::esMoveKeyframeSelection;
                    }
                }
            }
        }

        // So the user left clicked on background
        if (_imp->eventState == DopeSheetView::esNoEditingState) {
            if (!modCASIsShift(e)) {
                _imp->model->getSelectionModel()->clearKeyframeSelection();
            }

            _imp->selectionRect.setTopLeft(clickZoomCoords);
            _imp->selectionRect.setBottomRight(clickZoomCoords);
        }

        _imp->lastPosOnMousePress = e->pos();
        _imp->keyDragLastMovement = 0.;
    }
}

void DopeSheetView::mouseMoveEvent(QMouseEvent *e)
{
    running_in_main_thread();

    QPointF mouseZoomCoords = _imp->zoomContext.toZoomCoordinates(e->x(), e->y());

    if (e->buttons() == Qt::NoButton) {
        setCursor(_imp->getCursorDuringHover(e->pos()));
    }
    else if (buttonDownIsLeft(e)) {
        _imp->onMouseDrag(e);
    }
    else if (buttonDownIsMiddle(e)) {
        double dx = _imp->zoomContext.toZoomCoordinates(_imp->lastPosOnMouseMove.x(),
                                                        _imp->lastPosOnMouseMove.y()).x() - mouseZoomCoords.x();
        _imp->zoomContext.translate(dx, 0);

        redraw();

        // Synchronize the curve editor and opened viewers
        if (_imp->gui->isTripleSyncEnabled()) {
            _imp->updateCurveWidgetFrameRange();
            _imp->gui->centerOpenedViewersOn(_imp->zoomContext.left(), _imp->zoomContext.right());
        }
    }

    _imp->lastPosOnMouseMove = e->pos();
}

void DopeSheetView::mouseReleaseEvent(QMouseEvent *e)
{
    Q_UNUSED(e);

    bool mustRedraw = false;

    if (_imp->eventState == DopeSheetView::esSelectionByRect) {
        if (_imp->selectionRect.isValid()) {

            std::vector<DSSelectedKey> tempSelection;
            _imp->createSelectionFromRect(_imp->rectToZoomCoordinates(_imp->selectionRect), &tempSelection);

            _imp->model->getSelectionModel()->makeSelection(tempSelection);

            _imp->computeSelectedKeysBRect();
        }

        _imp->selectionRect = QRectF();

        mustRedraw = true;
    }

    if (_imp->eventState != esNoEditingState) {
        _imp->eventState = esNoEditingState;

        if (_imp->currentEditedReader) {
            _imp->currentEditedReader = 0;
        }

        if (_imp->currentEditedGroup) {
            _imp->currentEditedGroup = 0;
        }
    }

    if (mustRedraw) {
        redraw();
    }
}

void DopeSheetView::wheelEvent(QWheelEvent *e)
{
    running_in_main_thread();

    // don't handle horizontal wheel (e.g. on trackpad or Might Mouse)
    if (e->orientation() != Qt::Vertical) {
        return;
    }

    const double par_min = 0.0001;
    const double par_max = 10000.;

    double par;
    double scaleFactor = std::pow(NATRON_WHEEL_ZOOM_PER_DELTA, e->delta());
    QPointF zoomCenter = _imp->zoomContext.toZoomCoordinates(e->x(), e->y());

    _imp->zoomOrPannedSinceLastFit = true;

    par = _imp->zoomContext.aspectRatio() * scaleFactor;

    if (par <= par_min) {
        par = par_min;
        scaleFactor = par / _imp->zoomContext.aspectRatio();
    }
    else if (par > par_max) {
        par = par_max;
        scaleFactor = par / _imp->zoomContext.factor();
    }

    if (scaleFactor >= par_max || scaleFactor <= par_min) {
        return;
    }

    _imp->zoomContext.zoomx(zoomCenter.x(), zoomCenter.y(), scaleFactor);

    _imp->computeSelectedKeysBRect();

    redraw();

    // Synchronize the curve editor and opened viewers
    if (_imp->gui->isTripleSyncEnabled()) {
        _imp->updateCurveWidgetFrameRange();
        _imp->gui->centerOpenedViewersOn(_imp->zoomContext.left(), _imp->zoomContext.right());
    }
}

void DopeSheetView::enterEvent(QEvent *e)
{
    running_in_main_thread();

    setFocus();

    QGLWidget::enterEvent(e);
}

void DopeSheetView::focusInEvent(QFocusEvent *e)
{
    QGLWidget::focusInEvent(e);

    _imp->model->setUndoStackActive();
}

void DopeSheetView::keyPressEvent(QKeyEvent *e)
{
    running_in_main_thread();

    Qt::KeyboardModifiers modifiers = e->modifiers();
    Qt::Key key = Qt::Key(e->key());

    if (isKeybind(kShortcutGroupDopeSheetEditor, kShortcutIDActionDopeSheetEditorDeleteKeys, modifiers, key)) {
        deleteSelectedKeyframes();
    }
    else if (isKeybind(kShortcutGroupDopeSheetEditor, kShortcutIDActionDopeSheetEditorFrameSelection, modifiers, key)) {
        centerOn();
    }
    else if (isKeybind(kShortcutGroupDopeSheetEditor, kShortcutIDActionDopeSheetEditorSelectAllKeyframes, modifiers, key)) {
        selectAllKeyframes();
    }
    else if (isKeybind(kShortcutGroupDopeSheetEditor, kShortcutIDActionCurveEditorConstant, modifiers, key)) {
        constantInterpSelectedKeyframes();
    }
    else if (isKeybind(kShortcutGroupDopeSheetEditor, kShortcutIDActionCurveEditorLinear, modifiers, key)) {
        linearInterpSelectedKeyframes();
    }
    else if (isKeybind(kShortcutGroupDopeSheetEditor, kShortcutIDActionCurveEditorSmooth, modifiers, key)) {
        smoothInterpSelectedKeyframes();
    }
    else if (isKeybind(kShortcutGroupDopeSheetEditor, kShortcutIDActionCurveEditorCatmullrom, modifiers, key)) {
        catmullRomInterpSelectedKeyframes();
    }
    else if (isKeybind(kShortcutGroupDopeSheetEditor, kShortcutIDActionCurveEditorCubic, modifiers, key)) {
        cubicInterpSelectedKeyframes();
    }
    else if (isKeybind(kShortcutGroupDopeSheetEditor, kShortcutIDActionCurveEditorHorizontal, modifiers, key)) {
        horizontalInterpSelectedKeyframes();
    }
    else if (isKeybind(kShortcutGroupDopeSheetEditor, kShortcutIDActionCurveEditorBreak, modifiers, key)) {
        breakInterpSelectedKeyframes();
    }
    else if (isKeybind(kShortcutGroupDopeSheetEditor, kShortcutIDActionDopeSheetEditorCopySelectedKeyframes, modifiers, key)) {
        copySelectedKeyframes();
    }
    else if (isKeybind(kShortcutGroupDopeSheetEditor, kShortcutIDActionDopeSheetEditorPasteKeyframes, modifiers, key)) {
        pasteKeyframes();
    }
}

