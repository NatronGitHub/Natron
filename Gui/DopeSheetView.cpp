#include "DopeSheetView.h"

#include <algorithm>

// Qt includes
#include <QApplication>
#include <QHeaderView>
#include <QMouseEvent>
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

const QColor CLIP_OUTLINE_COLOR = QColor::fromRgbF(0.224f, 0.553f, 0.929f);
const QColor TIME_NODE_OUTLINE_COLOR = QColor::fromRgb(229, 205, 52);
const QColor GROUP_OUTLINE_COLOR = QColor::fromRgb(229, 61, 52);


////////////////////////// Helpers //////////////////////////

namespace {

void running_in_main_thread() {
    assert(qApp && qApp->thread() == QThread::currentThread());
}

void running_in_main_context(const QGLWidget *glWidget) {
    assert(glWidget->context() == QGLContext::currentContext());
}

void running_in_main_thread_and_context(const QGLWidget *glWidget) {
    running_in_main_thread();
    running_in_main_context(glWidget);
}


/**
 * @brief ClipColors
 *
 * A convenience typedef for storing useful colors for drawing:
 * - the first element defines the fill color of the clip ;
 * - the second element defines the outline color.
 */
typedef std::pair<QColor, QColor> ClipColors;

ClipColors getClipColors(DSNode::DSNodeType nodeType)
{
    ClipColors ret;

    if (nodeType == DSNode::ReaderNodeType) {
        ret.first = Qt::black;
        ret.second = CLIP_OUTLINE_COLOR;
    }
    else if (nodeType == DSNode::GroupNodeType) {
        ret.first = Qt::black;
        ret.second = GROUP_OUTLINE_COLOR;
    }
    else if (nodeType == DSNode::FrameRangeNodeType) {
        ret.first = Qt::black;
        ret.second = TIME_NODE_OUTLINE_COLOR;
    }

    return ret;
}

/**
 * @brief itemHasNoChildVisible
 *
 * Returns true if all childs of 'item' are hidden, otherwise returns
 * false.
 */
bool itemHasNoChildVisible(QTreeWidgetItem *item)
{
    for (int i = 0; i < item->childCount(); ++i) {
        if (!item->child(i)->isHidden())
            return false;
    }

    return true;
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

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;

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

    DSNode::DSNodeType nodeType = DSNode::DSNodeType(item->type());

    if (nodeType == DSNode::ReaderNodeType
            || nodeType == DSNode::GroupNodeType
            || nodeType == DSNode::RetimeNodeType
            || nodeType == DSNode::TimeOffsetNodeType
            || nodeType == DSNode::FrameRangeNodeType) {
        itemSize.rheight() += 10;
    }

    return itemSize;
}


////////////////////////// HierarchyView //////////////////////////

class HierarchyViewPrivate
{
public:
    HierarchyViewPrivate(HierarchyView *qq);
    ~HierarchyViewPrivate();

    /* functions */
    void insertNodeItem(DSNode *dsNode);
    void expandAndCheckKnobItems(DSNode *dsNode);
    void putChildrenNodesAtTopLevel(DSNode *dsNode);

    QTreeWidgetItem *getParentItem(QTreeWidgetItem *item) const;
    int indexInParent(QTreeWidgetItem *item) const;
    void moveChildTo(DSNode *child, DSNode *newParent);

    void checkNodeVisibleState(DSNode *dsNode);
    void checkKnobVisibleState(DSKnob *dsKnob);

    void recursiveSelect(QTreeWidgetItem *item);

    void selectSelectedKeyframesItems(bool selected);

    void selectKeyframes(const QList<QTreeWidgetItem *> &items);

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

    if (DSNode *nearestTimeNode = model->getNearestTimeNodeFromOutputs(dsNode)) {
        nearestTimeNode->getTreeItem()->insertChild(0, treeItem);
    }
    else if (dsNode->getDSNodeType() == DSNode::RetimeNodeType
             || dsNode->getDSNodeType() == DSNode::TimeOffsetNodeType
             || dsNode->getDSNodeType() == DSNode::FrameRangeNodeType) {
        std::vector<DSNode *> inputs = model->getInputsConnected(dsNode);

        bool hasNoInputs = true;

        for (std::vector<DSNode *>::const_iterator it = inputs.begin(); it != inputs.end(); ++it) {
            DSNode *input = (*it);

            DSNode *nearestTimeNode = 0;

            if ( (nearestTimeNode = model->getNearestTimeNodeFromOutputs(input)) ) {
                QTreeWidgetItem *inputTreeItem = input->getTreeItem();
                QTreeWidgetItem *inputParentItem = getParentItem(inputTreeItem);

                // Put the input in the time node's children
                inputTreeItem = inputParentItem->takeChild(indexInParent(inputTreeItem));

                nearestTimeNode->getTreeItem()->insertChild(0, inputTreeItem);

                hasNoInputs = false;

                // Add the time node as top level item
                q_ptr->addTopLevelItem(nearestTimeNode->getTreeItem());

                input->getTreeItem()->setExpanded(true);
                expandAndCheckKnobItems(input);
            }
        }

        if (hasNoInputs) {
            q_ptr->addTopLevelItem(dsNode->getTreeItem());

            dsNode->getTreeItem()->setExpanded(true);
            expandAndCheckKnobItems(dsNode);
        }
    }
    else {
        q_ptr->addTopLevelItem(treeItem);

        treeItem->setExpanded(true);
        expandAndCheckKnobItems(dsNode);
    }
}

void HierarchyViewPrivate::expandAndCheckKnobItems(DSNode *dsNode)
{
    DSKnobRow knobRows = dsNode->getChildData();

    for (DSKnobRow::const_iterator it = knobRows.begin(); it != knobRows.end(); ++it) {
        DSKnob *dsKnob = (*it).second;
        QTreeWidgetItem *knobItem = (*it).first;

        // Expand if it's a multidim root item
        if (knobItem->childCount() > 0) {
            knobItem->setExpanded(true);
        }

        checkKnobVisibleState(dsKnob);
    }
}

void HierarchyViewPrivate::putChildrenNodesAtTopLevel(DSNode *dsNode)
{
    QTreeWidgetItem *treeItem = dsNode->getTreeItem();

    for (int i = 0; i < treeItem->childCount(); ++i) {
        if (DSNode *nodeToMove = model->findDSNode(treeItem->child(0))) {
            QTreeWidgetItem *itemToMove = nodeToMove->getTreeItem();

            treeItem->takeChild(0);

            q_ptr->addTopLevelItem(itemToMove);

            itemToMove->setExpanded(true);
            expandAndCheckKnobItems(nodeToMove);
        }
        else {
            break;
        }
    }
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
    QTreeWidgetItem *currentParent = getParentItem(child->getTreeItem());
    currentParent->takeChild(indexInParent(child->getTreeItem()));

    if (newParent) {
        newParent->getTreeItem()->addChild(child->getTreeItem());
    }
    else {
        q_ptr->addTopLevelItem(child->getTreeItem());
    }
}

void HierarchyViewPrivate::checkNodeVisibleState(DSNode *dsNode)
{
    boost::shared_ptr<NodeGui> nodeGui = dsNode->getNodeGui();
    nodeGui->setVisibleSettingsPanel(true);

    bool showItem = nodeGui->isSettingsPanelVisible();

    DSNode::DSNodeType nodeType = dsNode->getDSNodeType();

    if (nodeType == DSNode::CommonNodeType) {
        showItem = model->nodeHasAnimation(nodeGui);
    }
    else if (nodeType == DSNode::GroupNodeType) {
        NodeGroup *group = dynamic_cast<NodeGroup *>(nodeGui->getNode()->getLiveInstance());

        showItem = showItem && !model->groupSubNodesAreHidden(group);
    }

    dsNode->getTreeItem()->setHidden(!showItem);

    // Hide the parent group item if there's no subnodes displayed
    if (DSNode *parentGroupDSNode = model->getGroupDSNode(dsNode)) {
        checkNodeVisibleState(parentGroupDSNode);
    }
}

void HierarchyViewPrivate::checkKnobVisibleState(DSKnob *dsKnob)
{
    QTreeWidgetItem *treeItem = dsKnob->getTreeItem();
    QTreeWidgetItem *nodeTreeItem = dsKnob->getTreeItem()->parent();

    KnobGui *knobGui = dsKnob->getKnobGui();

    if (dsKnob->isMultiDim()) {
        for (int i = 0; i < knobGui->getKnob()->getDimension(); ++i) {
            if (knobGui->getCurve(i)->isAnimated()) {
                if(treeItem->child(i)->isHidden()) {
                    treeItem->child(i)->setHidden(false);
                }
            }
            else {
                if (!treeItem->child(i)->isHidden()) {
                    treeItem->child(i)->setHidden(true);
                }
            }
        }

        if (itemHasNoChildVisible(treeItem)) {
            treeItem->setHidden(true);
        }
        else {
            treeItem->setHidden(false);
        }
    }
    else {
        if (knobGui->getCurve(0)->isAnimated()) {
            treeItem->setHidden(false);
        }
        else {
            treeItem->setHidden(true);
        }
    }

    if (itemHasNoChildVisible(nodeTreeItem)) {
        nodeTreeItem->setHidden(true);
    }
    else if (nodeTreeItem->isHidden()) {
        nodeTreeItem->setHidden(false);
    }

    checkNodeVisibleState(model->findDSNode(nodeTreeItem));
}

/**
 * @brief recursiveSelect
 *
 * Performs a recursive selection on 'item' 's chilren.
 */
void HierarchyViewPrivate::recursiveSelect(QTreeWidgetItem *item)
{
    if (item->childCount() > 0 && !itemHasNoChildVisible(item)) {
        for (int i = 0; i < item->childCount(); ++i) {
            QTreeWidgetItem *childItem = item->child(i);
            childItem->setSelected(true);

            // /!\ recursion
            recursiveSelect(childItem);
        }
    }
}

void HierarchyViewPrivate::selectSelectedKeyframesItems(bool aselect)
{
    QObject::disconnect(q_ptr, SIGNAL(itemSelectionChanged()),
                        q_ptr, SLOT(onItemSelectionChanged()));

    DSKeyPtrList selectedKeyframes = model->getSelectedKeyframes();

    for (DSKeyPtrList::const_iterator it = selectedKeyframes.begin(); it != selectedKeyframes.end(); ++it) {
        DSKeyPtr selectedKey = (*it);

        if (selectedKey->dimTreeItem->isSelected() != aselect) {
            selectedKey->dimTreeItem->setSelected(aselect);
        }

        if (selectedKey->dsKnob->getTreeItem()->isSelected() != aselect) {
            selectedKey->dsKnob->getTreeItem()->setSelected(aselect);
        }

        if (selectedKey->dsKnob->getTreeItem()->parent()->isSelected() != aselect) {
            selectedKey->dsKnob->getTreeItem()->parent()->setSelected(aselect);
        }
    }

    QObject::connect(q_ptr, SIGNAL(itemSelectionChanged()),
                     q_ptr, SLOT(onItemSelectionChanged()));
}

void HierarchyViewPrivate::selectKeyframes(const QList<QTreeWidgetItem *> &items)
{
    std::vector<DSSelectedKey> keys;

    Q_FOREACH (QTreeWidgetItem *item, items) {
        model->selectKeyframes(item, &keys);
    }

    model->makeSelection(keys);
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

    connect(model, SIGNAL(nodeSettingsPanelOpened(DSNode*)),
            this, SLOT(onNodeSettingsPanelOpened(DSNode*)));

    connect(model, SIGNAL(groupNodeSettingsPanelCloseChanged(DSNode*)),
            this, SLOT(onGroupNodeSettingsPanelCloseChanged(DSNode*)));

    connect(model, SIGNAL(keyframeSelectionAboutToBeCleared()),
            this, SLOT(onKeyframeSelectionAboutToBeCleared()));

    connect(model, SIGNAL(keyframeSelectionChanged()),
            this, SLOT(onKeyframeSelectionChanged()));

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
}

HierarchyView::~HierarchyView()
{}

QRectF HierarchyView::getItemRect(const DSNode *dsNode) const
{
    return visualItemRect(dsNode->getTreeItem());
}

QRectF HierarchyView::getItemRect(const DSKnob *dsKnob) const
{
    return visualItemRect(dsKnob->getTreeItem());
}

QRectF HierarchyView::getItemRectForDim(const DSKnob *dsKnob, int dim) const
{
    return visualItemRect(_imp->model->findTreeItemForDim(dsKnob, dim));
}

DSKnob *HierarchyView::getDSKnobAt(const QPoint &point, int *dimension) const
{
    QTreeWidgetItem *itemUnderPoint = itemAt(0, point.y());

    return _imp->model->findDSKnob(itemUnderPoint, dimension);
}

void HierarchyView::onNodeAdded(DSNode *dsNode)
{
    _imp->insertNodeItem(dsNode);

    dsNode->getTreeItem()->setExpanded(true);

    if (!dsNode->getTreeItem()->isHidden()) {
        _imp->expandAndCheckKnobItems(dsNode);
    }
}

void HierarchyView::onNodeAboutToBeRemoved(DSNode *dsNode)
{
    QTreeWidgetItem *treeItem = dsNode->getTreeItem();
    bool isTopLevelItem = !treeItem->parent();

    if (isTopLevelItem) {
        _imp->putChildrenNodesAtTopLevel(dsNode);
    }
}

void HierarchyView::onKeyframeSetOrRemoved(DSKnob *dsKnob)
{
    _imp->checkKnobVisibleState(dsKnob);
}

void HierarchyView::onNodeSettingsPanelOpened(DSNode *dsNode)
{
    _imp->expandAndCheckKnobItems(dsNode);
}

void HierarchyView::onGroupNodeSettingsPanelCloseChanged(DSNode *dsNode)
{
    assert(dsNode->getDSNodeType() == DSNode::GroupNodeType);

    _imp->checkNodeVisibleState(dsNode);
}

void HierarchyView::onKeyframeSelectionAboutToBeCleared()
{
    qDebug() << "HierarchyView::onKeyframeSelectionAboutToBeCleared";
    _imp->selectSelectedKeyframesItems(false);
}

void HierarchyView::onKeyframeSelectionChanged()
{
    qDebug() << "HierarchyView::onKeyframeSelectionChanged";
    _imp->selectSelectedKeyframesItems(true);
}

void HierarchyView::onItemSelectionChanged()
{
    qDebug() << "HierarchyView::onItemSelectionChanged";
    if (selectedItems().empty()) {
        _imp->model->clearKeyframeSelection();
    }
    else {
        Q_FOREACH (QTreeWidgetItem *item, selectedItems()) {
            _imp->recursiveSelect(item);
        }

        _imp->selectKeyframes(selectedItems());
    }
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
    QTreeWidget::drawRow(painter, option, index);

    QTreeWidgetItem *item = itemFromIndex(index);

    QRect rowRect = option.rect;

    // Draw the plugin icon
    {
        DSNode *dsNode = _imp->model->findDSNode(item);

        if (dsNode) {
            std::string iconFilePath = dsNode->getNode()->getPluginIconFilePath();

            if (!iconFilePath.empty()) {
                QPixmap pix;

                if (pix.load(iconFilePath.c_str())) {
                    pix = pix.scaled(NATRON_MEDIUM_BUTTON_SIZE - 2, NATRON_MEDIUM_BUTTON_SIZE - 2,
                                     Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

                    QRect pluginIconRect = rowRect;
                    pluginIconRect.setSize(pix.size());
                    pluginIconRect.moveRight(rowRect.right() - 2);
                    pluginIconRect.moveCenter(QPoint(pluginIconRect.center().x(),
                                                     rowRect.center().y()));

                    painter->drawPixmap(pluginIconRect, pix);
                }
            }
        }
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

    std::vector<DSSelectedKey> isNearByKeyframe(DSKnob *dsKnob, const QPointF &widgetCoords, int dimension) const;
    std::vector<DSSelectedKey> isNearByKeyframe(DSNode *dsNode, const QPointF &widgetCoords) const;

    double clampedMouseOffset(double fromTime, double toTime);

    // Textures
    void generateKeyframeTextures();
    DopeSheetViewPrivate::KeyframeTexture kfTextureFromKeyframeType(Natron::KeyframeTypeEnum kfType, bool selected) const;

    // Drawing
    void drawScale() const;

    void drawRows() const;

    void drawNodeRow(const DSNode *dsNode) const;
    void drawKnobRow(const DSKnob *dsKnob) const;

    void drawClip(DSNode *dsNode) const;
    void drawKeyframes(DSNode *dsNode) const;

    void drawTexturedKeyframe(DopeSheetViewPrivate::KeyframeTexture textureType, const QRectF &rect) const;

    void drawProjectBounds() const;
    void drawCurrentFrameIndicator();

    void drawSelectionRect() const;

    void drawSelectedKeysBRect() const;

    // After or during an user interaction
    void computeTimelinePositions();
    void computeSelectionRect(const QPointF &origin, const QPointF &current);
    void computeRangesBelow(DSNode *dsNode);
    void computeNodeRange(DSNode *dsNode);
    void computeReaderRange(DSNode *reader);
    void computeGroupRange(DSNode *group);
    void computeFRRange(DSNode *frameRange);

    // User interaction
    void onMouseDrag(QMouseEvent *e);

    std::vector<DSSelectedKey> createSelectionFromRect(const QRectF &rect);

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

    double rowTop = r.topLeft().y();
    double rowBottom = r.bottomRight().y() - 1;

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
    else if (QTreeWidgetItem *treeItem = hierarchyView->itemAt(0, widgetCoords.y())) {
        DSNodeRows dsNodeItems = model->getNodeRows();
        DSNodeRows::const_iterator dsNodeIt = dsNodeItems.find(treeItem);

        if (dsNodeIt != dsNodeItems.end()) {
            DSNode *dsNode = (*dsNodeIt).second;
            DSNode::DSNodeType nodeType = dsNode->getDSNodeType();

            QRectF treeItemRect = hierarchyView->getItemRect(dsNode);

            if (dsNode->hasRange()) {
                FrameRange range = nodeRanges.at(dsNode);

                QRectF nodeClipRect = rectToZoomCoordinates(QRectF(QPointF(range.first, treeItemRect.top() + 1),
                                                                   QPointF(range.second, treeItemRect.bottom() + 1)));

                if (nodeType == DSNode::GroupNodeType) {
                    if (nodeClipRect.contains(zoomCoords.x(), zoomCoords.y())) {
                        ret = getCursorForEventState(DopeSheetView::esGroupRepos);
                    }
                }
                else if (nodeType == DSNode::ReaderNodeType) {
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
            else if (nodeType == DSNode::CommonNodeType) {
                std::vector<DSSelectedKey> keysUnderMouse = isNearByKeyframe(dsNode, widgetCoords);

                if (!keysUnderMouse.empty()) {
                    ret = getCursorForEventState(DopeSheetView::esPickKeyframe);
                }
            }
        }
        else {
            int knobDim;
            QPointF widgetPos = zoomContext.toWidgetCoordinates(zoomCoords.x(), zoomCoords.y());
            DSKnob *dsKnob =  hierarchyView->getDSKnobAt(QPoint(widgetPos.x(), widgetPos.y()), &knobDim);

            std::vector<DSSelectedKey> keysUnderMouse = isNearByKeyframe(dsKnob, widgetCoords, knobDim);

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

std::vector<DSSelectedKey> DopeSheetViewPrivate::isNearByKeyframe(DSKnob *dsKnob, const QPointF &widgetCoords, int dimension) const
{
    std::vector<DSSelectedKey> ret;

    boost::shared_ptr<KnobI> knob = dsKnob->getKnobGui()->getKnob();

    int startDim = 0;
    int endDim = knob->getDimension();

    if (dimension > -1) {
        startDim = dimension;
        endDim = dimension + 1;
    }

    for (int i = startDim; i < endDim; ++i) {
        KeyFrameSet keyframes = knob->getCurve(i)->getKeyFrames_mt_safe();

        for (KeyFrameSet::const_iterator kIt = keyframes.begin();
             kIt != keyframes.end();
             ++kIt) {
            KeyFrame kf = (*kIt);

            QPointF keyframeWidgetPos = zoomContext.toWidgetCoordinates(kf.getTime(), 0);

            if (std::abs(widgetCoords.x() - keyframeWidgetPos.x()) < DISTANCE_ACCEPTANCE_FROM_KEYFRAME) {
                DSSelectedKey key(dsKnob, kf, model->findTreeItemForDim(dsKnob, i), i);
                ret.push_back(key);
            }
        }
    }

    return ret;
}

std::vector<DSSelectedKey> DopeSheetViewPrivate::isNearByKeyframe(DSNode *dsNode, const QPointF &widgetCoords) const
{
    std::vector<DSSelectedKey> ret;

    DSKnobRow dsKnobs = dsNode->getChildData();

    for (DSKnobRow::const_iterator it = dsKnobs.begin(); it != dsKnobs.end(); ++it) {
        DSKnob *dsKnob = (*it).second;
        KnobGui *knobGui = dsKnob->getKnobGui();

        for (int i = 0; i < knobGui->getKnob()->getDimension(); ++i) {
            KeyFrameSet keyframes = knobGui->getCurve(i)->getKeyFrames_mt_safe();

            for (KeyFrameSet::const_iterator kIt = keyframes.begin();
                 kIt != keyframes.end();
                 ++kIt) {
                KeyFrame kf = (*kIt);

                QPointF keyframeWidgetPos = zoomContext.toWidgetCoordinates(kf.getTime(), 0);

                if (std::abs(widgetCoords.x() - keyframeWidgetPos.x()) < DISTANCE_ACCEPTANCE_FROM_KEYFRAME) {
                    DSSelectedKey key(dsKnob, kf, model->findTreeItemForDim(dsKnob, i), i);
                    ret.push_back(key);
                }
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

                    q_ptr->renderText(value, bottomLeft.y(), s, c, *font);

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
            DSNode *dsNode = (*it).second;

            if(dsNode->getTreeItem()->isHidden()) {
                continue;
            }

            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            drawNodeRow(dsNode);

            DSKnobRow knobItems = dsNode->getChildData();
            for (DSKnobRow::const_iterator it2 = knobItems.begin();
                 it2 != knobItems.end();
                 ++it2) {

                DSKnob *dsKnob = (*it2).second;

                drawKnobRow(dsKnob);
            }

            DSNode::DSNodeType nodeType = dsNode->getDSNodeType();

            if (dsNode->hasRange()) {
                drawClip(dsNode);
            }

            if (nodeType != DSNode::GroupNodeType) {
                drawKeyframes(dsNode);
            }
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
    glVertex2f(rowRect.topLeft().x(), rowRect.topLeft().y());
    glVertex2f(rowRect.bottomLeft().x(), rowRect.bottomLeft().y());
    glVertex2f(rowRect.bottomRight().x(), rowRect.bottomRight().y());
    glVertex2f(rowRect.topRight().x(), rowRect.topRight().y());
    glEnd();
}

/**
 * @brief DopeSheetViewPrivate::drawKnobRow
 *
 *
 */
void DopeSheetViewPrivate::drawKnobRow(const DSKnob *dsKnob) const
{
    GLProtectAttrib a(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT);

    boost::shared_ptr<Settings> settings = appPTR->getCurrentSettings();

    if (dsKnob->isMultiDim()) {
        // Draw root row
        QRectF nameItemRect = hierarchyView->getItemRect(dsKnob);
        QRectF rowRect = nameItemRectToRowRect(nameItemRect);

        double rootR, rootG, rootB, rootA;
        settings->getDopeSheetEditorRootRowBackgroundColor(&rootR, &rootG, &rootB, &rootA);

        glColor4f(rootR, rootG, rootB, rootA);

        glBegin(GL_POLYGON);
        glVertex2f(rowRect.topLeft().x(), rowRect.topLeft().y());
        glVertex2f(rowRect.bottomLeft().x(), rowRect.bottomLeft().y());
        glVertex2f(rowRect.bottomRight().x(), rowRect.bottomRight().y());
        glVertex2f(rowRect.topRight().x(), rowRect.topRight().y());
        glEnd();

        // Draw child rows
        double knobR, knobG, knobB, knobA;
        settings->getDopeSheetEditorKnobRowBackgroundColor(&knobR, &knobG, &knobB, &knobA);

        glColor4f(knobR, knobG, knobB, knobA);

        for (int i = 0; i < dsKnob->getKnobGui()->getKnob()->getDimension(); ++i) {
            QRectF nameChildItemRect = hierarchyView->getItemRectForDim(dsKnob, i);
            QRectF childrowRect = nameItemRectToRowRect(nameChildItemRect);

            // Draw child row
            glBegin(GL_POLYGON);
            glVertex2f(childrowRect.topLeft().x(), childrowRect.topLeft().y());
            glVertex2f(childrowRect.bottomLeft().x(), childrowRect.bottomLeft().y());
            glVertex2f(childrowRect.bottomRight().x(), childrowRect.bottomRight().y());
            glVertex2f(childrowRect.topRight().x(), childrowRect.topRight().y());
            glEnd();
        }
    }
    else {
        QRectF nameItemRect = hierarchyView->getItemRect(dsKnob);
        QRectF rowRect = nameItemRectToRowRect(nameItemRect);

        double knobR, knobG, knobB, knobA;
        settings->getDopeSheetEditorKnobRowBackgroundColor(&knobR, &knobG, &knobB, &knobA);

        glColor4f(knobR, knobG, knobB, knobA);

        glBegin(GL_POLYGON);
        glVertex2f(rowRect.topLeft().x(), rowRect.topLeft().y());
        glVertex2f(rowRect.bottomLeft().x(), rowRect.bottomLeft().y());
        glVertex2f(rowRect.bottomRight().x(), rowRect.bottomRight().y());
        glVertex2f(rowRect.topRight().x(), rowRect.topRight().y());
        glEnd();

    }
}

void DopeSheetViewPrivate::drawClip(DSNode *dsNode) const
{
    // Draw the clip
    {
        ClipColors colors = getClipColors(dsNode->getDSNodeType());

        FrameRange range = nodeRanges.at(dsNode);

        QRectF treeItemRect = hierarchyView->getItemRect(dsNode);

        QRectF clipRectZoomCoords = rectToZoomCoordinates(QRectF(QPointF(range.first, treeItemRect.top() + 1),
                                                                 QPointF(range.second, treeItemRect.bottom() + 1)));

        GLProtectAttrib a(GL_CURRENT_BIT);

        // Fill the reader rect
        glColor4f(colors.first.redF(), colors.first.greenF(),
                  colors.first.blueF(), colors.first.alphaF());

        glBegin(GL_POLYGON);
        glVertex2f(clipRectZoomCoords.topLeft().x(), clipRectZoomCoords.topLeft().y());
        glVertex2f(clipRectZoomCoords.bottomLeft().x(), clipRectZoomCoords.bottomLeft().y() + 2);
        glVertex2f(clipRectZoomCoords.bottomRight().x(), clipRectZoomCoords.bottomRight().y() + 2);
        glVertex2f(clipRectZoomCoords.topRight().x(), clipRectZoomCoords.topRight().y());
        glEnd();

        glLineWidth(2);

        // Draw the outline
        glColor4f(colors.second.redF(), colors.second.greenF(),
                  colors.second.blueF(), colors.second.alphaF());

        glBegin(GL_LINE_LOOP);
        glVertex2f(clipRectZoomCoords.topLeft().x(), clipRectZoomCoords.topLeft().y());
        glVertex2f(clipRectZoomCoords.bottomLeft().x(), clipRectZoomCoords.bottomLeft().y() + 2);
        glVertex2f(clipRectZoomCoords.bottomRight().x(), clipRectZoomCoords.bottomRight().y() + 2);
        glVertex2f(clipRectZoomCoords.topRight().x(), clipRectZoomCoords.topRight().y());
        glEnd();

        // If necessary, draw the original frame range line
        if (dsNode->getDSNodeType() == DSNode::ReaderNodeType) {
            NodePtr node = dsNode->getNode();

            Knob<int> *firstFrameKnob = dynamic_cast<Knob<int> *>(node->getKnobByName("firstFrame").get());
            assert(firstFrameKnob);
            Knob<int> *lastFrameKnob = dynamic_cast<Knob<int> *>(node->getKnobByName("lastFrame").get());
            assert(lastFrameKnob);
            Knob<int> *originalFrameRangeKnob = dynamic_cast<Knob<int> *>(node->getKnobByName("originalFrameRange").get());
            assert(originalFrameRangeKnob);

            int framesFromEndToTotal = (originalFrameRangeKnob->getValue(1) - originalFrameRangeKnob->getValue(0))
                    - lastFrameKnob->getValue();

            float clipRectCenterY = clipRectZoomCoords.center().y();

            glLineWidth(1);

            glColor4f(colors.second.redF(), colors.second.greenF(),
                      colors.second.blueF(), colors.second.alphaF());

            glBegin(GL_LINES);
            glVertex2f(clipRectZoomCoords.left() - firstFrameKnob->getValue() + 1, clipRectCenterY);
            glVertex2f(clipRectZoomCoords.left(), clipRectCenterY);

            glVertex2f(clipRectZoomCoords.right(), clipRectCenterY);
            glVertex2f(clipRectZoomCoords.right() + framesFromEndToTotal + 1, clipRectCenterY);
            glEnd();
        }
    }

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

        DSKnobRow knobItems = dsNode->getChildData();

        std::map<double, bool> nodeTimes;

        for (DSKnobRow::const_iterator it = knobItems.begin();
             it != knobItems.end();
             ++it) {

            DSKnob *dsKnob = (*it).second;

            // The knob is no longer animated
            if (dsKnob->getTreeItem()->isHidden()) {
                continue;
            }

            std::map<double, bool> knobTimes;

            KnobGui *knobGui = dsKnob->getKnobGui();

            // Draw keyframes for each dimension of the knob
            for (int dim = 0; dim < knobGui->getKnob()->getDimension(); ++dim) {
                KeyFrameSet keyframes = knobGui->getCurve(dim)->getKeyFrames_mt_safe();

                for (KeyFrameSet::const_iterator kIt = keyframes.begin();
                     kIt != keyframes.end();
                     ++kIt) {
                    KeyFrame kf = (*kIt);

                    double keyTime = kf.getTime();

                    bool knobTimeNotExists = (knobTimes.find(keyTime) == knobTimes.end());
                    bool nodeTimeNotExists = (nodeTimes.find(keyTime) == nodeTimes.end());

                    if (knobTimeNotExists) {
                        knobTimes[keyTime] = false;
                    }

                    if (nodeTimeNotExists) {
                        nodeTimes[keyTime] = false;
                    }

                    double y = (dsKnob->isMultiDim()) ? hierarchyView->getItemRectForDim(dsKnob, dim).center().y()
                                                      : hierarchyView->getItemRect(dsKnob).center().y();

                    QRectF zoomKfRect = rectToZoomCoordinates(keyframeRect(keyTime, y));

                    bool keyframeIsSelected = model->keyframeIsSelected(dim, dsKnob, kf);

                    if (keyframeIsSelected) {
                        if (dsKnob->isMultiDim()) {
                            knobTimes[keyTime] = true;
                        }

                        nodeTimes[keyTime] = true;
                    }

                    // Draw keyframe in the knob dim row only if it's visible
                    bool drawInDimRow = dsNode->getTreeItem()->isExpanded() &&
                            ((dsKnob->isMultiDim()) ? dsKnob->getTreeItem()->isExpanded() : true);

                    if (drawInDimRow) {
                        DopeSheetViewPrivate::KeyframeTexture texType = kfTextureFromKeyframeType(kf.getInterpolation(),
                                                                                                  keyframeIsSelected);

                        if (texType != DopeSheetViewPrivate::kfTextureNone) {
                            drawTexturedKeyframe(texType, zoomKfRect);
                        }
                    }
                }
            }

            // Draw keyframes in multidim knob root rows if necessary
            for (std::map<double, bool>::const_iterator it = knobTimes.begin(); it != knobTimes.end(); ++it) {
                bool drawInMultidimRootRow = (dsKnob->isMultiDim() && dsNode->getTreeItem()->isExpanded());

                if (!drawInMultidimRootRow) {
                    continue;
                }

                double keyTime = (*it).first;
                double y = hierarchyView->getItemRect(dsKnob).center().y();

                QRectF zoomRootKfRect = rectToZoomCoordinates(keyframeRect(keyTime, y));

                bool drawSelected = (*it).second;

                if (drawSelected) {
                    drawTexturedKeyframe(DopeSheetViewPrivate::kfTextureRootSelected, zoomRootKfRect);
                }
                else {
                    drawTexturedKeyframe(DopeSheetViewPrivate::kfTextureRoot, zoomRootKfRect);
                }

            }
        }

        // Draw keyframes in node rows if necessary
        QTreeWidgetItem *nodeTreeItem = dsNode->getTreeItem();
        for (std::map<double, bool>::const_iterator it = nodeTimes.begin(); it != nodeTimes.end(); ++it) {
            bool drawInNodeRow = true || (nodeTreeItem->parent() && nodeTreeItem->parent()->isExpanded());

            if (!drawInNodeRow) {
                continue;
            }

            // Draw keyframe in node row
            if (dsNode->getDSNodeType() == DSNode::CommonNodeType) {
                double keyTime = (*it).first;
                double y = hierarchyView->getItemRect(dsNode).center().y();

                QRectF zoomNodeKfRect = rectToZoomCoordinates(keyframeRect(keyTime, y));

                bool drawSelected = (*it).second;

                if (drawSelected) {
                    drawTexturedKeyframe(DopeSheetViewPrivate::kfTextureRootSelected, zoomNodeKfRect);
                }
                else {
                    drawTexturedKeyframe(DopeSheetViewPrivate::kfTextureRoot, zoomNodeKfRect);
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

void DopeSheetViewPrivate::drawProjectBounds() const
{
    running_in_main_thread_and_context(q_ptr);

    double bottom = zoomContext.toZoomCoordinates(0, q_ptr->height() - 1).y();
    double top = zoomContext.toZoomCoordinates(q_ptr->width() - 1, 0).y();

    int projectStart, projectEnd;
    gui->getApp()->getFrameRange(&projectStart, &projectEnd);

    // Perform drawing
    {
        GLProtectAttrib a(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT);

        glColor4f(1.f, 1.f, 1.f, 1.f);

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
    double gridR, gridG, gridB;
    settings->getDopeSheetEditorGridColor(&gridR, &gridG, &gridB);

    // Perform drawing
    {
        GLProtectAttrib a(GL_CURRENT_BIT | GL_HINT_BIT | GL_ENABLE_BIT |
                          GL_LINE_BIT | GL_POLYGON_BIT | GL_COLOR_BUFFER_BIT);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glEnable(GL_LINE_SMOOTH);
        glHint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);

        glColor3f(gridR, gridG, gridB);

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
        GLProtectAttrib a(GL_HINT_BIT | GL_ENABLE_BIT | GL_LINE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT);

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
        GLProtectAttrib a(GL_HINT_BIT | GL_ENABLE_BIT | GL_LINE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT);

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
    DSNode::DSNodeType nodeType = dsNode->getDSNodeType();

    switch (nodeType) {
    case DSNode::ReaderNodeType:
        computeReaderRange(dsNode);
        break;
    case DSNode::GroupNodeType:
        computeGroupRange(dsNode);
        break;
    case DSNode::FrameRangeNodeType:
        computeFRRange(dsNode);
        break;
    default:
        break;
    }
}

void DopeSheetViewPrivate::computeReaderRange(DSNode *reader)
{
    NodePtr node = reader->getNode();

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
                     startingTimeValue + (lastFrameValue - firstFrameValue));

    nodeRanges[reader] = range;
}

void DopeSheetViewPrivate::computeGroupRange(DSNode *group)
{
    NodePtr node = group->getNode();

    FrameRange range;

    std::vector<double> dimFirstKeys;
    std::vector<double> dimLastKeys;

    NodeList nodes = dynamic_cast<NodeGroup *>(node->getLiveInstance())->getNodes();

    for (NodeList::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        NodePtr node = (*it);

        boost::shared_ptr<NodeGui> nodeGui = boost::dynamic_pointer_cast<NodeGui>(node->getNodeGui());


        if (!nodeGui->getSettingPanel() || !nodeGui->getSettingPanel()->isVisible()) {
            continue;
        }

        const std::vector<boost::shared_ptr<KnobI> > &knobs = node->getKnobs();

        for (std::vector<boost::shared_ptr<KnobI> >::const_iterator it = knobs.begin();
             it != knobs.end();
             ++it) {
            boost::shared_ptr<KnobI> knob = (*it);

            if (!knob->canAnimate() || !knob->hasAnimation()) {
                continue;
            }
            else {
                for (int i = 0; i < knob->getDimension(); ++i) {
                    KeyFrameSet keyframes = knob->getCurve(i)->getKeyFrames_mt_safe();

                    if (keyframes.empty()) {
                        continue;
                    }

                    dimFirstKeys.push_back(keyframes.begin()->getTime());
                    dimLastKeys.push_back(keyframes.rbegin()->getTime());
                }
            }
        }
    }

    if (dimFirstKeys.empty() || dimLastKeys.empty()) {
        range.first = 0;
        range.second = 0;
    }
    else {
        range.first = *std::min_element(dimFirstKeys.begin(), dimFirstKeys.end());
        range.second = *std::max_element(dimLastKeys.begin(), dimLastKeys.end());
    }

    nodeRanges[group] = range;
}

void DopeSheetViewPrivate::computeFRRange(DSNode *frameRange)
{
    NodePtr node = frameRange->getNode();

    Knob<int> *frameRangeKnob = dynamic_cast<Knob<int> *>(node->getKnobByName("frameRange").get());
    assert(frameRangeKnob);

    FrameRange range;
    range.first = frameRangeKnob->getValue(0);
    range.second = frameRangeKnob->getValue(1);

    nodeRanges[frameRange] = range;
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

        std::vector<DSSelectedKey> tempSelection = createSelectionFromRect(rectToZoomCoordinates(selectionRect));

        if (modCASIsShift(e)) {
            model->makeBooleanSelection(tempSelection);
        }
        else {
            model->makeSelection(tempSelection);
        }

        q_ptr->redraw();

        break;
    }
    case DopeSheetView::esReaderLeftTrim:
    {
        if (dt >= 1.0f || dt <= -1.0f) {
            Knob<int> *timeOffsetKnob = dynamic_cast<Knob<int> *>(currentEditedReader->getNode()->getKnobByName("timeOffset").get());
            assert(timeOffsetKnob);

            double newFirstFrame = std::floor(currentTime - timeOffsetKnob->getValue() + 0.5);

            model->trimReaderLeft(currentEditedReader, newFirstFrame);
        }

        break;
    }
    case DopeSheetView::esReaderRightTrim:
    {
        if (dt >= 1.0f || dt <= -1.0f) {
            Knob<int> *timeOffsetKnob = dynamic_cast<Knob<int> *>(currentEditedReader->getNode()->getKnobByName("timeOffset").get());
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
            int mouseOffset = (lastZoomCoordsOnMousePress.x() - lastTimeOffsetOnMousePress);
            double newTime = (currentTime - mouseOffset);

            model->moveReader(currentEditedReader, newTime);
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

std::vector<DSSelectedKey> DopeSheetViewPrivate::createSelectionFromRect(const QRectF &rect)
{
    std::vector<DSSelectedKey> ret;

    DSNodeRows dsNodes = model->getNodeRows();

    for (DSNodeRows::const_iterator it = dsNodes.begin(); it != dsNodes.end(); ++it) {
        DSNode *dsNode = (*it).second;

        DSKnobRow dsKnobs = dsNode->getChildData();

        for (DSKnobRow::const_iterator it2 = dsKnobs.begin(); it2 != dsKnobs.end(); ++it2) {
            DSKnob *dsKnob = (*it2).second;
            KnobGui *knobGui = dsKnob->getKnobGui();

            for (int i = 0; i < knobGui->getKnob()->getDimension(); ++i) {
                KeyFrameSet keyframes = knobGui->getCurve(i)->getKeyFrames_mt_safe();

                for (KeyFrameSet::const_iterator kIt = keyframes.begin();
                     kIt != keyframes.end();
                     ++kIt) {
                    KeyFrame kf = (*kIt);

                    double rowCenterY = (dsKnob->isMultiDim()) ? hierarchyView->getItemRectForDim(dsKnob, i).center().y()
                                                               : hierarchyView->getItemRect(dsKnob).center().y();

                    double x = kf.getTime();

                    if ((rect.left() <= x) && (rect.right() >= x)
                            && (rect.top() >= rowCenterY) && (rect.bottom() <= rowCenterY)) {
                        ret.push_back(DSSelectedKey(dsKnob, kf, model->findTreeItemForDim(dsKnob, i), i));
                    }
                }
            }
        }
    }

    return ret;
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

    connect(model, SIGNAL(groupNodeSettingsPanelCloseChanged(DSNode*)),
            this, SLOT(onGroupNodeSettingsPanelCloseChanged(DSNode*)));

    connect(_imp->model, SIGNAL(modelChanged()),
            this, SLOT(redraw()));

    connect(_imp->model, SIGNAL(keyframeSelectionChanged()),
            this, SLOT(onKeyframeSelectionChanged()));

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

void DopeSheetView::frame(double xMin, double xMax)
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

    qDebug() << "redraw";

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

void DopeSheetView::computeSelectedKeysBRect()
{
    DSKeyPtrList selectedKeyframes = _imp->model->getSelectedKeyframes();

    if (selectedKeyframes.size() <= 1) {
        _imp->selectedKeysBRect = QRectF();

        return;
    }

    const int SELECTED_KF_BBOX_BOUNDS_OFFSET = 4;

    QRectF rect;
    QTreeWidgetItem *topMostItem = 0;

    for (DSKeyPtrList::const_iterator it = selectedKeyframes.begin();
         it != selectedKeyframes.end();
         ++it) {
        DSKeyPtr selected = (*it);

        double x = selected->key.getTime();
        double y = 0;

        QTreeWidgetItem *knobTreeItem = selected->dsKnob->getTreeItem();
        QTreeWidgetItem *selectedNodeTreeItem = knobTreeItem->parent();

        if (!selectedNodeTreeItem->isExpanded()) {
            y = _imp->hierarchyView->visualItemRect(selectedNodeTreeItem).center().y();
        }
        else {
            if (selected->dsKnob->isMultiDim()) {
                if (knobTreeItem->isExpanded()) {
                    for (int i = knobTreeItem->childCount() - 1; i >= 0  ; --i) {
                        if (!knobTreeItem->child(i)->isHidden()) {
                            y = _imp->hierarchyView->visualItemRect(knobTreeItem->child(i)).center().y();

                            break;
                        }
                    }
                }
                else {
                    y = _imp->hierarchyView->visualItemRect(knobTreeItem).center().y();
                }
            }
            else {
                y = _imp->hierarchyView->visualItemRect(knobTreeItem).center().y();
            }
        }

        if (it != selectedKeyframes.begin()) {
            if (x < rect.left()) {
                rect.setLeft(x);
            }

            if (x > rect.right()) {
                rect.setRight(x);
            }

            if (y > rect.top()) {
                rect.setTop(y);
            }

            if (_imp->hierarchyView->visualItemRect(selectedNodeTreeItem).center().y()
                    < _imp->hierarchyView->visualItemRect(topMostItem).center().y()) {
                topMostItem = selectedNodeTreeItem;
            }
        }
        else {
            rect.setLeft(x);
            rect.setRight(x);
            rect.setTop(y);
            rect.setBottom(y);

            topMostItem = selectedNodeTreeItem;
        }
    }

    QPointF topLeft(rect.left(), rect.top());
    QPointF bottomRight(rect.right(), rect.bottom());

    _imp->selectedKeysBRect.setTopLeft(topLeft);
    _imp->selectedKeysBRect.setBottomRight(bottomRight);

    if (!_imp->selectedKeysBRect.isNull()) {
        double bottom = _imp->hierarchyView->visualItemRect(topMostItem).center().y();

        _imp->selectedKeysBRect.setBottom(bottom);

        double xAdjustOffset = (_imp->zoomContext.toZoomCoordinates(rect.left(), 0).x() -
                                _imp->zoomContext.toZoomCoordinates(rect.left() - KF_X_OFFSET, 0).x());

        _imp->selectedKeysBRect.adjust(-xAdjustOffset, SELECTED_KF_BBOX_BOUNDS_OFFSET,
                                       xAdjustOffset, -SELECTED_KF_BBOX_BOUNDS_OFFSET);
    }
}

void DopeSheetView::selectAllKeyframes()
{
    _imp->model->selectAllKeyframes();

    if (_imp->model->getSelectedKeyframesCount() > 1) {
        computeSelectedKeysBRect();
    }

    redraw();
}

void DopeSheetView::deleteSelectedKeyframes()
{
    running_in_main_thread();

    _imp->model->deleteSelectedKeyframes();

    redraw();
}

void DopeSheetView::frame()
{
    running_in_main_thread();

    int selectedKeyframesCount = _imp->model->getSelectedKeyframesCount();

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
        computeSelectedKeysBRect();
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

    qDebug() << "DopeSheetView::onTimeLineFrameChanged";

    running_in_main_thread();

    if (_imp->gui->isGUIFrozen()) {
        return;
    }

    _imp->computeTimelinePositions();

    redraw();
}

void DopeSheetView::onTimeLineBoundariesChanged(int, int)
{
    qDebug() << "DopeSheetView::onTimeLineBoundariesChanged";
    running_in_main_thread();

    redraw();
}

void DopeSheetView::onNodeAdded(DSNode *dsNode)
{
    DSNode::DSNodeType nodeType = dsNode->getDSNodeType();
    NodePtr node = dsNode->getNode();

    if (nodeType == DSNode::CommonNodeType) {
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
    }
    else if (nodeType == DSNode::ReaderNodeType) {
        // The dopesheet view must refresh if the user set some values in the settings panel
        // so we connect some signals/slots
        boost::shared_ptr<KnobSignalSlotHandler> lastFrameKnob =  node->getKnobByName("lastFrame")->getSignalSlotHandler();
        assert(lastFrameKnob);
        boost::shared_ptr<KnobSignalSlotHandler> startingTimeKnob = node->getKnobByName("startingTime")->getSignalSlotHandler();
        assert(startingTimeKnob);

        connect(lastFrameKnob.get(), SIGNAL(valueChanged(int, int)),
                this, SLOT(onReaderChanged(int, int)));

        connect(startingTimeKnob.get(), SIGNAL(valueChanged(int, int)),
                this, SLOT(onReaderChanged(int, int)));

        // We don't make the connection for the first frame knob, because the
        // starting time is updated when it's modified. Thus we avoid two
        // refreshes of the view.

        _imp->computeReaderRange(dsNode);
    }
    else if (nodeType == DSNode::FrameRangeNodeType) {
        boost::shared_ptr<KnobSignalSlotHandler> frameRangeKnob =  node->getKnobByName("frameRange")->getSignalSlotHandler();
        assert(frameRangeKnob);

        connect(frameRangeKnob.get(), SIGNAL(valueChanged(int, int)),
                this, SLOT(onFrameRangeNodeChanged(int, int)));

        _imp->computeFRRange(dsNode);
    }
    else if (nodeType == DSNode::GroupNodeType) {
        _imp->computeGroupRange(dsNode);
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

    redraw();
}

void DopeSheetView::onKeyframeChanged()
{
    qDebug() << "DopeSheetView::onKeyframeChanged";
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

void DopeSheetView::onReaderChanged(int /*dimension*/, int /*reason*/)
{
    QObject *signalSender = sender();

    DSNode *dsNode = 0;

    if (KnobSignalSlotHandler *knobHandler = qobject_cast<KnobSignalSlotHandler *>(signalSender)) {
        dsNode = _imp->model->findDSNode(knobHandler->getKnob());
    }

    if (dsNode) {
        assert(dsNode->getDSNodeType() == DSNode::ReaderNodeType);

        _imp->computeReaderRange(dsNode);

        redraw();
    }
}

void DopeSheetView::onFrameRangeNodeChanged(int /*dimension*/, int /*reason*/)
{
    QObject *signalSender = sender();

    DSNode *dsNode = 0;

    if (KnobSignalSlotHandler *knobHandler = qobject_cast<KnobSignalSlotHandler *>(signalSender)) {
        KnobHolder *holder = knobHandler->getKnob()->getHolder();
        Natron::EffectInstance *effectInstance = dynamic_cast<Natron::EffectInstance *>(holder);

        dsNode = _imp->model->findDSNode(effectInstance->getNode().get());
    }

    if (dsNode) {
        assert(dsNode->getDSNodeType() == DSNode::FrameRangeNodeType);

        _imp->computeFRRange(dsNode);

        redraw();
    }
}

void DopeSheetView::onHierarchyViewItemExpandedOrCollapsed(QTreeWidgetItem *item)
{
    Q_UNUSED(item);

    qDebug() << "DopeSheetView::onHierarchyViewItemExpandedOrCollapsed";

    // Compute the range rects of affected items
    if (DSNode *dsNode = _imp->model->findParentDSNode(item)) {
        _imp->computeRangesBelow(dsNode);
    }

    computeSelectedKeysBRect();

    redraw();
}

void DopeSheetView::onGroupNodeSettingsPanelCloseChanged(DSNode *dsNode)
{
    if (DSNode *parentGroupDSNode = _imp->model->getGroupDSNode(dsNode)) {
        _imp->computeGroupRange(parentGroupDSNode);
    }
}

void DopeSheetView::onKeyframeSelectionChanged()
{
    qDebug() << "DopeSheetView::onKeyframeSelectionChanged";
    computeSelectedKeysBRect();

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
        _imp->drawRows();

        if (_imp->eventState == DopeSheetView::esSelectionByRect) {
            _imp->drawSelectionRect();
        }

        if (_imp->rectToZoomCoordinates(_imp->selectedKeysBRect).isValid()) {
            _imp->drawSelectedKeysBRect();
        }

        _imp->drawProjectBounds();
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
                DSNode::DSNodeType nodeType = dsNode->getDSNodeType();

                QRectF treeItemRect = _imp->hierarchyView->getItemRect(dsNode);

                if (dsNode->hasRange()) {
                    FrameRange range = _imp->nodeRanges[dsNode];
                    QRectF nodeClipRect = _imp->rectToZoomCoordinates(QRectF(QPointF(range.first, treeItemRect.top() + 1),
                                                                             QPointF(range.second, treeItemRect.bottom() + 1)));

                    if (nodeType == DSNode::GroupNodeType) {
                        if (nodeClipRect.contains(clickZoomCoords.x(), clickZoomCoords.y())) {
                            _imp->currentEditedGroup = dsNode;

                            _imp->eventState = DopeSheetView::esGroupRepos;
                        }
                    }
                    else if (nodeType == DSNode::ReaderNodeType) {
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

                            Knob<int> *timeOffsetKnob = dynamic_cast<Knob<int> *>(_imp->currentEditedReader->getNode()->getKnobByName("timeOffset").get());
                            assert(timeOffsetKnob);

                            _imp->lastTimeOffsetOnMousePress = timeOffsetKnob->getValue();
                        }
                    }
                }
                else if (nodeType == DSNode::CommonNodeType) {
                    std::vector<DSSelectedKey> keysUnderMouse = _imp->isNearByKeyframe(dsNode, e->pos());

                    if (!keysUnderMouse.empty()) {
                        if (modCASIsShift(e)) {
                            _imp->model->makeBooleanSelection(keysUnderMouse);
                        }
                        else {
                            _imp->model->makeSelection(keysUnderMouse);
                        }

                        _imp->eventState = DopeSheetView::esMoveKeyframeSelection;
                    }
                }
            }
            // Or search for a keyframe
            else {
                int knobDim;
                DSKnob *dsKnob = _imp->hierarchyView->getDSKnobAt(e->pos(), &knobDim);

                if (dsKnob) {
                    std::vector<DSSelectedKey> keysUnderMouse = _imp->isNearByKeyframe(dsKnob, e->pos(), knobDim);

                    if (!keysUnderMouse.empty()) {
                        if (modCASIsShift(e)) {
                            _imp->model->makeBooleanSelection(keysUnderMouse);
                        }
                        else {
                            _imp->model->makeSelection(keysUnderMouse);
                        }

                        _imp->eventState = DopeSheetView::esMoveKeyframeSelection;
                    }
                }
            }
        }

        // So the user left clicked on background
        if (_imp->eventState == DopeSheetView::esNoEditingState) {
            if (!modCASIsShift(e)) {
                _imp->model->clearKeyframeSelection();
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
        if (_imp->model->getSelectedKeyframesCount() > 1) {
            computeSelectedKeysBRect();
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

    computeSelectedKeysBRect();

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
        frame();
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

/**
* @brief DopeSheetView::renderText
*
*
*/
void DopeSheetView::renderText(double x, double y,
                               const QString &text,
                               const QColor &color,
                               const QFont &font) const
{
    running_in_main_thread_and_context(this);

    if ( text.isEmpty() ) {
        return;
    }

    double w = double(width());
    double h = double(height());

    double bottom = _imp->zoomContext.bottom();
    double left = _imp->zoomContext.left();
    double top =  _imp->zoomContext.top();
    double right = _imp->zoomContext.right();

    if (w <= 0 || h <= 0 || right <= left || top <= bottom) {
        return;
    }

    double scalex = (right-left) / w;
    double scaley = (top-bottom) / h;

    _imp->textRenderer.renderText(x, y, scalex, scaley, text, color, font);

    glCheckError();
}
