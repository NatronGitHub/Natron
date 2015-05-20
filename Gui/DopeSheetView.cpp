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
#include <QUndoStack>

// Natron includes
#include "Engine/Curve.h"
#include "Engine/Image.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/Project.h"
#include "Engine/Settings.h"
#include "Engine/TimeLine.h"

#include "Gui/ActionShortcuts.h"
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
#include "Gui/ZoomContext.h"


// Some macros to ensure the following operations always running in the main thread
#define RUNNING_IN_MAIN_THREAD() \
    assert(qApp && qApp->thread() == QThread::currentThread())

// Note : if you want to call the following functions from the pimpl of a class, ensure it have its
// parent as attribute and pass its name as parameter, or pass 'this' if
// it's called from a pimpled class.
#define RUNNING_IN_MAIN_CONTEXT(glContextOwner) \
    assert(QGLContext::currentContext() == glContextOwner->context())

#define RUNNING_IN_MAIN_THREAD_AND_CONTEXT(glContextOwner) \
    RUNNING_IN_MAIN_THREAD();\
    RUNNING_IN_MAIN_CONTEXT(glContextOwner)


typedef std::set<double> TimeSet;
typedef Knob<int> * KnobIntPtr;
typedef std::vector<DSSelectedKey> DSSelectedKeys;

const int KF_HEIGHT = 10;
const int KF_X_OFFSET = 3;
const int CLICK_DISTANCE_ACCEPTANCE = 5;
const QColor KF_COLOR = QColor::fromRgbF(1.f, 0.f, 0.f);
const QColor SELECTED_KF_COLOR = QColor::fromRgbF(0.961f, 0.961f, 0.047f);


////////////////////////// Helpers //////////////////////////

namespace {

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
        ret.second = QColor::fromRgbF(0.224f, 0.553f, 0.929f);
    }
    else if (nodeType == DSNode::GroupNodeType) {
        ret.first = Qt::black;
        ret.second = QColor::fromRgbF(0.224f, 0.553f, 0.929f);
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

/**
 * @brief recursiveSelect
 *
 * Performs a recursive selection on 'item' 's chilren.
 */
void recursiveSelect(QTreeWidgetItem *item)
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

    if (nodeType == DSNode::ReaderNodeType || nodeType == DSNode::GroupNodeType) {
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

    /* attributes */
    HierarchyView *parent;
    DopeSheet *model;

    Gui *gui;
};

HierarchyViewPrivate::HierarchyViewPrivate(HierarchyView *qq) :
    parent(qq),
    model(0),
    gui(0)
{}

HierarchyViewPrivate::~HierarchyViewPrivate()
{}

/**
 * @brief HierarchyView::HierarchyView
 *
 *
 */
HierarchyView::HierarchyView(DopeSheet *model, Gui *gui, QWidget *parent) :
    QTreeWidget(parent),
    _imp(new HierarchyViewPrivate(this))
{
    connect(model, SIGNAL(dsNodeCreated(DSNode *)),
            this, SLOT(onDSNodeCreated(DSNode *)));

    connect(this, SIGNAL(itemSelectionChanged()),
            this, SLOT(onItemSelectionChanged()));

    connect(this, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)),
            this, SLOT(onItemDoubleClicked(QTreeWidgetItem*,int)));

    connect(this, SIGNAL(itemExpanded(QTreeWidgetItem*)),
            model, SLOT(refreshClipRects()));

    connect(this, SIGNAL(itemCollapsed(QTreeWidgetItem*)),
            model, SLOT(refreshClipRects()));

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
    return visualItemRect(dsKnob->getTreeItem()->child(dim));
}

DSKnob *HierarchyView::getDSKnobAt(const QPoint &point, int *dimension) const
{
    QTreeWidgetItem *itemUnderPoint = itemAt(0, point.y());

    return _imp->model->findDSKnob(itemUnderPoint, dimension);
}

void HierarchyView::onDSNodeCreated(DSNode *dsNode)
{
    // Add item to the tree
    addTopLevelItem(dsNode->getTreeItem());

    dsNode->getTreeItem()->setExpanded(true);

    // Expand all and hide if necessary
    DSRowsKnobData knobRows = dsNode->getRowsKnobData();

    for (int i = 0; i < dsNode->getTreeItem()->childCount(); ++i) {
        QTreeWidgetItem *knobItem = dsNode->getTreeItem()->child(i);

        // Expand if it's a multidim root item
        if (knobItem->childCount() > 0) {
            knobItem->setExpanded(true);
        }

        assert(knobRows.find(knobItem) != knobRows.end());

        DSKnob *dsKnob = knobRows[knobItem];
        dsKnob->checkVisibleState();
    }
}

/**
 * @brief DopeSheetEditor::onItemSelectionChanged
 *
 * Selects recursively the current selected items of the hierarchy view.
 *
 * This slot is automatically called when this current selection has changed.
 */
void HierarchyView::onItemSelectionChanged()
{
    QList<QTreeWidgetItem *> currentItemSelection = selectedItems();

    Q_FOREACH (QTreeWidgetItem *item, currentItemSelection) {
        recursiveSelect(item);
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
            std::string iconFilePath = dsNode->getNodeGui()->getNode()->getPluginIconFilePath();

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
    DopeSheetViewPrivate(DopeSheetView *qq);
    ~DopeSheetViewPrivate();

    /* functions */

    // Helpers
    QRectF rectToZoomCoordinates(const QRectF &rect) const;
    QRectF rectToWidgetCoordinates(const QRectF &rect) const;
    QRectF nameItemRectToRowRect(const QRectF &rect) const;

    DSKeyPtrList::iterator keyframeIsAlreadyInSelected(const DSSelectedKey &key);

    Qt::CursorShape getCursorDuringHover(const QPointF &widgetCoords) const;
    Qt::CursorShape getCursorForEventState(DopeSheetView::EventStateEnum es) const;

    bool isNearByClipRectLeft(double time, const QRectF &clipRect) const;
    bool isNearByClipRectRight(double time, const QRectF &clipRect) const;
    bool isNearByCurrentFrameIndicatorBottom(const QPointF &zoomCoords) const;

    DSSelectedKeys isNearByKeyframe(DSKnob *dsKnob, const QPointF &widgetCoords, int dimension) const;
    DSSelectedKeys isNearByKeyframe(DSNode *dsNode, const QPointF &widgetCoords) const;

    // Drawing
    void drawScale() const;

    void drawRows() const;

    void drawNodeRow(const DSNode *dsNode) const;
    void drawKnobRow(const DSKnob *dsKnob) const;

    void drawClip(DSNode *dsNode) const;
    void drawKeyframes(DSNode *dsNode) const;

    void drawProjectBounds() const;
    void drawCurrentFrameIndicator();

    void drawSelectionRect() const;

    void drawSelectedKeysBRect() const;

    // After or during an user interaction
    void computeTimelinePositions();
    void computeSelectionRect(const QPointF &origin, const QPointF &current);
    void computeReaderRect(DSNode *dsNode);
    void computeGroupRect(DSNode *dsNode);

    // User interaction
    DSSelectedKeys createSelectionFromCursor(DSKnob *dsKnob, const QPointF &widgetCoords, int dimension);
    DSSelectedKeys createSelectionFromCursor(DSNode *dsNode, const QPointF &widgetCoords);
    DSSelectedKeys createSelectionFromRect(const QRectF &rect);

    void makeSelection(const DSSelectedKeys &keys, bool booleanOp);

    void moveCurrentFrameIndicator(double toTime);

    void pushUndoCommand(QUndoCommand *cmd);

    void createContextMenu();

    /* attributes */
    DopeSheetView *parent;

    DopeSheet *model;
    HierarchyView *hierarchyView;

    Gui *gui;

    // necessary to retrieve some useful values for drawing
    boost::shared_ptr<TimeLine> timeline;

    // for rendering
    QFont *font;
    Natron::TextRenderer textRenderer;

    // for navigating
    ZoomContext zoomContext;
    bool zoomOrPannedSinceLastFit;

    // for current time indicator
    QPolygonF currentFrameIndicatorBottomPoly;

    // for keyframe selection
    DSKeyPtrList selectedKeyframes;
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
    boost::scoped_ptr<QUndoStack> undoStack;
    bool hasOpenGLVAOSupport;

    // UI
    Natron::Menu *contextMenu;
};

DopeSheetViewPrivate::DopeSheetViewPrivate(DopeSheetView *qq) :
    parent(qq),
    model(0),
    hierarchyView(0),
    gui(0),
    timeline(),
    font(new QFont(appFont,appFontSize)),
    textRenderer(),
    zoomContext(),
    zoomOrPannedSinceLastFit(false),
    selectedKeyframes(),
    selectionRect(),
    selectedKeysBRect(),
    lastPosOnMousePress(),
    lastPosOnMouseMove(),
    lastTimeOffsetOnMousePress(),
    keyDragLastMovement(),
    eventState(DopeSheetView::esNoEditingState),
    currentEditedReader(0),
    currentEditedGroup(0),
    undoStack(new QUndoStack(parent)),
    hasOpenGLVAOSupport(true),
    contextMenu(new Natron::Menu(parent))
{}

DopeSheetViewPrivate::~DopeSheetViewPrivate()
{
    selectedKeyframes.clear();
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

DSKeyPtrList::iterator DopeSheetViewPrivate::keyframeIsAlreadyInSelected(const DSSelectedKey &key)
{
    for (DSKeyPtrList::iterator it = selectedKeyframes.begin(); it != selectedKeyframes.end(); ++it) {
        boost::shared_ptr<DSSelectedKey> selected = (*it);

        if (*(selected.get()) == key) {
            return it;
        }
    }

    return selectedKeyframes.end();
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
        DSRowsNodeData dsNodeItems = model->getRowsNodeData();
        DSRowsNodeData::const_iterator dsNodeIt = dsNodeItems.find(treeItem);

        if (dsNodeIt != dsNodeItems.end()) {
            DSNode *dsNode = (*dsNodeIt).second;

            std::pair<double, double> range = dsNode->getClipRange();
            QRectF treeItemRect = hierarchyView->getItemRect(dsNode);
            QRectF nodeClipRect = rectToZoomCoordinates(QRectF(QPointF(range.first, treeItemRect.top() + 1),
                                                               QPointF(range.second, treeItemRect.bottom() + 1)));

            DSNode::DSNodeType nodeType = dsNode->getDSNodeType();

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
                    else {
                        ret = getCursorForEventState(DopeSheetView::esClipRepos);
                    }
                }
            }
            else if (nodeType == DSNode::CommonNodeType) {
                DSSelectedKeys keysUnderMouse = isNearByKeyframe(dsNode, widgetCoords);

                if (!keysUnderMouse.empty()) {
                    ret = getCursorForEventState(DopeSheetView::esMoveKeyframeSelection);
                }
            }
        }
        else {
            int knobDim;
            QPointF widgetPos = zoomContext.toWidgetCoordinates(zoomCoords.x(), zoomCoords.y());
            DSKnob *dsKnob =  hierarchyView->getDSKnobAt(QPoint(widgetPos.x(), widgetPos.y()), &knobDim);

            DSSelectedKeys keysUnderMouse = isNearByKeyframe(dsKnob, widgetCoords, knobDim);

            if (!keysUnderMouse.empty()) {
                ret = getCursorForEventState(DopeSheetView::esMoveKeyframeSelection);
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
    case DopeSheetView::esClipRepos:
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
    return ((time >= clipRect.left() - CLICK_DISTANCE_ACCEPTANCE)
            && (time <= clipRect.left() + CLICK_DISTANCE_ACCEPTANCE));
}

bool DopeSheetViewPrivate::isNearByClipRectRight(double time, const QRectF &clipRect) const
{
    return ((time >= clipRect.right() - CLICK_DISTANCE_ACCEPTANCE)
            && (time <= clipRect.right() + CLICK_DISTANCE_ACCEPTANCE));
}

bool DopeSheetViewPrivate::isNearByCurrentFrameIndicatorBottom(const QPointF &zoomCoords) const
{
    return (currentFrameIndicatorBottomPoly.containsPoint(zoomCoords, Qt::OddEvenFill));
}

DSSelectedKeys DopeSheetViewPrivate::isNearByKeyframe(DSKnob *dsKnob, const QPointF &widgetCoords, int dimension) const
{
    DSSelectedKeys ret;

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

            if (std::abs(widgetCoords.x() - keyframeWidgetPos.x()) < CLICK_DISTANCE_ACCEPTANCE) {
                DSSelectedKey key(dsKnob, kf, i);
                ret.push_back(key);
            }
        }
    }

    return ret;
}

DSSelectedKeys DopeSheetViewPrivate::isNearByKeyframe(DSNode *dsNode, const QPointF &widgetCoords) const
{
    DSSelectedKeys ret;

    DSRowsKnobData dsKnobs = dsNode->getRowsKnobData();

    for (DSRowsKnobData::const_iterator it = dsKnobs.begin(); it != dsKnobs.end(); ++it) {
        DSKnob *dsKnob = (*it).second;
        KnobGui *knobGui = dsKnob->getKnobGui();

        for (int i = 0; i < knobGui->getKnob()->getDimension(); ++i) {
            KeyFrameSet keyframes = knobGui->getCurve(i)->getKeyFrames_mt_safe();

            for (KeyFrameSet::const_iterator kIt = keyframes.begin();
                 kIt != keyframes.end();
                 ++kIt) {
                KeyFrame kf = (*kIt);

                QPointF keyframeWidgetPos = zoomContext.toWidgetCoordinates(kf.getTime(), 0);

                if (std::abs(widgetCoords.x() - keyframeWidgetPos.x()) < CLICK_DISTANCE_ACCEPTANCE) {
                    DSSelectedKey key(dsKnob, kf, i);
                    ret.push_back(key);
                }
            }
        }
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
    RUNNING_IN_MAIN_THREAD_AND_CONTEXT(parent);

    QPointF bottomLeft = zoomContext.toZoomCoordinates(0, parent->height() - 1);
    QPointF topRight = zoomContext.toZoomCoordinates(parent->width() - 1, 0);

    // Don't attempt to draw a scale on a widget with an invalid height
    if (parent->height() <= 1) {
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

        const double rangePixel = parent->width();
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

                    parent->renderText(value, bottomLeft.y(), s, c, *font);

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
    RUNNING_IN_MAIN_THREAD_AND_CONTEXT(parent);

    DSRowsNodeData treeItemsAndDSNodes = model->getRowsNodeData();

    // Perform drawing
    {
        GLProtectAttrib a(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT);

        for (DSRowsNodeData::const_iterator it = treeItemsAndDSNodes.begin();
             it != treeItemsAndDSNodes.end();
             ++it) {
            DSNode *dsNode = (*it).second;

            if(dsNode->getTreeItem()->isHidden()) {
                continue;
            }

            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            drawNodeRow(dsNode);

            DSRowsKnobData knobItems = dsNode->getRowsKnobData();
            for (DSRowsKnobData::const_iterator it2 = knobItems.begin();
                 it2 != knobItems.end();
                 ++it2) {

                DSKnob *dsKnob = (*it2).second;

                drawKnobRow(dsKnob);
            }

            DSNode::DSNodeType nodeType = dsNode->getDSNodeType();

            if (nodeType == DSNode::ReaderNodeType || nodeType == DSNode::GroupNodeType) {
                drawClip(dsNode);
            }
            else {
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

    glBegin(GL_QUADS);
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

        glBegin(GL_QUADS);
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
            glBegin(GL_QUADS);
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

        glBegin(GL_QUADS);
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

        std::pair<double, double> range = dsNode->getClipRange();
        QRectF treeItemRect = hierarchyView->getItemRect(dsNode);
        QRectF clipRectZoomCoords = rectToZoomCoordinates(QRectF(QPointF(range.first, treeItemRect.top() + 1),
                                                                 QPointF(range.second, treeItemRect.bottom() + 1)));

        GLProtectAttrib a(GL_CURRENT_BIT);

        // Fill the reader rect
        glColor4f(colors.first.redF(), colors.first.greenF(),
                  colors.first.blueF(), colors.first.alphaF());

        glBegin(GL_QUADS);
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
            KnobIntPtr originalFrameRangeKnob = dynamic_cast<KnobIntPtr>
                    (dsNode->getNodeGui()->getNode()->getKnobByName("originalFrameRange").get());
            KnobIntPtr firstFrameKnob = dynamic_cast<KnobIntPtr>
                    (dsNode->getNodeGui()->getNode()->getKnobByName("firstFrame").get());
            KnobIntPtr lastFrameKnob = dynamic_cast<KnobIntPtr>
                    (dsNode->getNodeGui()->getNode()->getKnobByName("lastFrame").get());

            int framesFromEndToTotal = (originalFrameRangeKnob->getValue(1) - originalFrameRangeKnob->getValue(0))
                    - lastFrameKnob->getValue();

            float clipRectCenterY = clipRectZoomCoords.center().y();

            glLineWidth(1);

            glColor4f(colors.second.redF(), colors.second.greenF(),
                      colors.second.blueF(), colors.second.alphaF());

            glBegin(GL_LINES);
            glVertex2f(clipRectZoomCoords.left() - firstFrameKnob->getValue(), clipRectCenterY);
            glVertex2f(clipRectZoomCoords.left(), clipRectCenterY);

            glVertex2f(clipRectZoomCoords.right(), clipRectCenterY);
            glVertex2f(clipRectZoomCoords.right() + framesFromEndToTotal, clipRectCenterY);
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
    RUNNING_IN_MAIN_THREAD_AND_CONTEXT(parent);

    // Perform drawing
    {
        GLProtectAttrib a(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glEnable(GL_POINT_SMOOTH);

        DSRowsKnobData knobItems = dsNode->getRowsKnobData();

        for (DSRowsKnobData::const_iterator it = knobItems.begin();
             it != knobItems.end();
             ++it) {

            DSKnob *dsKnob = (*it).second;

            // The knob is no longer animated
            if (dsKnob->getTreeItem()->isHidden()) {
                continue;
            }

            KnobGui *knobGui = dsKnob->getKnobGui();

            glColor3f(KF_COLOR.redF(), KF_COLOR.greenF(), KF_COLOR.blackF());

            // Draw keyframes for each dimension of the knob
            for (int dim = 0; dim < knobGui->getKnob()->getDimension(); ++dim) {
                KeyFrameSet keyframes = knobGui->getCurve(dim)->getKeyFrames_mt_safe();

                for (KeyFrameSet::const_iterator kIt = keyframes.begin();
                     kIt != keyframes.end();
                     ++kIt) {
                    KeyFrame kf = (*kIt);

                    double keyTime = kf.getTime();

                    double y = (dsKnob->isMultiDim()) ? hierarchyView->getItemRectForDim(dsKnob, dim).center().y()
                                                      : hierarchyView->getItemRect(dsKnob).center().y();
                    QPointF p = zoomContext.toZoomCoordinates(keyTime, y);

                    QRectF kfRect;
                    kfRect.setHeight(KF_HEIGHT);
                    kfRect.setLeft(zoomContext.toZoomCoordinates(keyTime - KF_X_OFFSET, y).x());
                    kfRect.setRight(zoomContext.toZoomCoordinates(keyTime + KF_X_OFFSET, y).x());
                    kfRect.moveCenter(zoomContext.toWidgetCoordinates(p.x(), p.y()));

                    QRectF zoomKfRect = rectToZoomCoordinates(kfRect);

                    DSKeyPtrList::const_iterator isSelected = selectedKeyframes.end();

                    for (DSKeyPtrList::const_iterator it2 = selectedKeyframes.begin();
                         it2 != selectedKeyframes.end(); it2++) {
                        DSKeyPtr selectedKey = (*it2);

                        if (selectedKey->dimension != dim) {
                            continue;
                        }

                        if (selectedKey->dsKnob == dsKnob && selectedKey->key == kf) {
                            isSelected = it2;
                            glColor3f(SELECTED_KF_COLOR.redF(), SELECTED_KF_COLOR.greenF(), SELECTED_KF_COLOR.blueF());
                            break;
                        }
                    }

                    // Draw keyframe in the knob dim row only if it's visible
                    bool drawInDimRow = dsNode->getTreeItem()->isExpanded() &&
                            ((dsKnob->isMultiDim()) ? dsKnob->getTreeItem()->isExpanded() : true);

                    if (drawInDimRow) {
                        glBegin(GL_QUADS);
                        glVertex2f(zoomKfRect.left(), zoomKfRect.top());
                        glVertex2f(zoomKfRect.left(), zoomKfRect.bottom());
                        glVertex2f(zoomKfRect.right(), zoomKfRect.bottom());
                        glVertex2f(zoomKfRect.right(), zoomKfRect.top());
                        glEnd();
                    }

                    // Draw keyframe in multidim root knob row too
                    bool drawInMultidimRootRow = (dsKnob->isMultiDim() && dsNode->getTreeItem()->isExpanded());
                    if (drawInMultidimRootRow) {
                        p = zoomContext.toZoomCoordinates(keyTime,
                                                          hierarchyView->getItemRect(dsKnob).center().y());

                        kfRect.moveCenter(zoomContext.toWidgetCoordinates(p.x(), p.y()));
                        zoomKfRect = rectToZoomCoordinates(kfRect);

                        glBegin(GL_QUADS);
                        glVertex2f(zoomKfRect.left(), zoomKfRect.top());
                        glVertex2f(zoomKfRect.left(), zoomKfRect.bottom());
                        glVertex2f(zoomKfRect.right(), zoomKfRect.bottom());
                        glVertex2f(zoomKfRect.right(), zoomKfRect.top());
                        glEnd();
                    }

                    // Draw keyframe in node row
                    p = zoomContext.toZoomCoordinates(keyTime,
                                                      hierarchyView->getItemRect(dsNode).center().y());

                    kfRect.moveCenter(zoomContext.toWidgetCoordinates(p.x(), p.y()));
                    zoomKfRect = rectToZoomCoordinates(kfRect);

                    glBegin(GL_QUADS);
                    glVertex2f(zoomKfRect.left(), zoomKfRect.top());
                    glVertex2f(zoomKfRect.left(), zoomKfRect.bottom());
                    glVertex2f(zoomKfRect.right(), zoomKfRect.bottom());
                    glVertex2f(zoomKfRect.right(), zoomKfRect.top());
                    glEnd();

                    if (isSelected != selectedKeyframes.end()) {
                        glColor3f(KF_COLOR.redF(), KF_COLOR.greenF(), KF_COLOR.blackF());
                    }
                }
            }
        }
    }
}

void DopeSheetViewPrivate::drawProjectBounds() const
{
    RUNNING_IN_MAIN_THREAD_AND_CONTEXT(parent);

    double bottom = zoomContext.toZoomCoordinates(0, parent->height() - 1).y();
    double top = zoomContext.toZoomCoordinates(parent->width() - 1, 0).y();

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
    RUNNING_IN_MAIN_THREAD_AND_CONTEXT(parent);

    computeTimelinePositions();

    int top = zoomContext.toZoomCoordinates(0, 0).y();
    int bottom = zoomContext.toZoomCoordinates(parent->width() - 1,
                                               parent->height() - 1).y();

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
    RUNNING_IN_MAIN_THREAD_AND_CONTEXT(parent);

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
        glBegin(GL_QUADS);
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
    RUNNING_IN_MAIN_THREAD_AND_CONTEXT(parent);

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
    RUNNING_IN_MAIN_THREAD();

    double polyHalfWidth = 7.5;
    double polyHeight = 7.5;

    int bottom = zoomContext.toZoomCoordinates(parent->width() - 1,
                                               parent->height() - 1).y();

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

DSSelectedKeys DopeSheetViewPrivate::createSelectionFromCursor(DSKnob *dsKnob, const QPointF &widgetCoords, int dimension)
{
    return isNearByKeyframe(dsKnob, widgetCoords, dimension);
}

DSSelectedKeys DopeSheetViewPrivate::createSelectionFromCursor(DSNode *dsNode, const QPointF &widgetCoords)
{
    return isNearByKeyframe(dsNode, widgetCoords);
}

DSSelectedKeys DopeSheetViewPrivate::createSelectionFromRect(const QRectF& rect)
{
    DSSelectedKeys ret;

    DSRowsNodeData dsNodes = model->getRowsNodeData();

    for (DSRowsNodeData::const_iterator it = dsNodes.begin(); it != dsNodes.end(); ++it) {
        DSNode *dsNode = (*it).second;

        DSRowsKnobData dsKnobs = dsNode->getRowsKnobData();

        for (DSRowsKnobData::const_iterator it2 = dsKnobs.begin(); it2 != dsKnobs.end(); ++it2) {
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
                        ret.push_back(DSSelectedKey(dsKnob, kf, i));
                    }
                }
            }
        }
    }

    return ret;
}

void DopeSheetViewPrivate::makeSelection(const DSSelectedKeys &keys, bool booleanOp)
{
    if (!booleanOp) {
        selectedKeyframes.clear();
    }

    for (DSSelectedKeys::const_iterator it = keys.begin(); it != keys.end(); ++it) {
        DSSelectedKey key = (*it);

        DSKeyPtrList::iterator isAlreadySelected = keyframeIsAlreadyInSelected(key);

        if (isAlreadySelected == selectedKeyframes.end()) {
            DSKeyPtr selected(new DSSelectedKey(key));
            selectedKeyframes.push_back(selected);
        }
        else {
            if (booleanOp) {
                selectedKeyframes.erase(isAlreadySelected);
            }
        }
    }
}

void DopeSheetViewPrivate::moveCurrentFrameIndicator(double toTime)
{
    gui->getApp()->setLastViewerUsingTimeline(boost::shared_ptr<Natron::Node>());

    timeline->seekFrame(SequenceTime(toTime), false, 0, Natron::eTimelineChangeReasonDopeSheetEditorSeek);
}

void DopeSheetViewPrivate::pushUndoCommand(QUndoCommand *cmd)
{
    undoStack->setActive();
    undoStack->push(cmd);
}

void DopeSheetViewPrivate::createContextMenu()
{
    RUNNING_IN_MAIN_THREAD();

    contextMenu->clear();

    // Create menus
    Natron::Menu *editMenu = new Natron::Menu(contextMenu);
    editMenu->setTitle(QObject::tr("Edit"));

    contextMenu->addAction(editMenu->menuAction());

    Natron::Menu *viewMenu = new Natron::Menu(contextMenu);
    viewMenu->setTitle(QObject::tr("Menu"));

    contextMenu->addAction(viewMenu->menuAction());

    // Create actions
    QAction *removeSelectedKeyframesAction = new ActionWithShortcut(kShortcutGroupDopeSheetEditor,
                                                                    kShortcutIDActionDopeSheetEditorDeleteKeys,
                                                                    kShortcutDescActionDopeSheetEditorDeleteKeys,
                                                                    editMenu);
    QObject::connect(removeSelectedKeyframesAction, SIGNAL(triggered()),
                     parent, SLOT(deleteSelectedKeyframes()));
    editMenu->addAction(removeSelectedKeyframesAction);

    QAction *selectAllKeyframesAction = new ActionWithShortcut(kShortcutGroupDopeSheetEditor,
                                                               kShortcutIDActionDopeSheetEditorSelectAllKeyframes,
                                                               kShortcutDescActionDopeSheetEditorSelectAllKeyframes,
                                                               editMenu);
    QObject::connect(selectAllKeyframesAction, SIGNAL(triggered()),
                     parent, SLOT(selectAllKeyframes()));
    editMenu->addAction(selectAllKeyframesAction);

    QAction *frameSelectionAction = new ActionWithShortcut(kShortcutGroupDopeSheetEditor,
                                                           kShortcutIDActionDopeSheetEditorFrameSelection,
                                                           kShortcutDescActionDopeSheetEditorFrameSelection,
                                                           viewMenu);
    QObject::connect(frameSelectionAction, SIGNAL(triggered()),
                     parent, SLOT(frame()));
    viewMenu->addAction(frameSelectionAction);
}

/**
 * @brief DopeSheetView::DopeSheetView
 *
 * Constructs a DopeSheetView object.
 */
DopeSheetView::DopeSheetView(DopeSheet *model, HierarchyView *hierarchyView,
                             Gui *gui,
                             boost::shared_ptr<TimeLine> timeline,
                             QWidget *parent) :
    QGLWidget(parent),
    _imp(new DopeSheetViewPrivate(this))
{
    _imp->model = model;
    _imp->hierarchyView = hierarchyView;

    _imp->gui = gui;
    _imp->gui->registerNewUndoStack(_imp->undoStack.get());

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

    connect(_imp->model, SIGNAL(modelChanged()),
            this, SLOT(update()));

    connect(_imp->hierarchyView, SIGNAL(itemExpanded(QTreeWidgetItem*)),
            this, SLOT(computeSelectedKeysBRect()));

    connect(_imp->hierarchyView, SIGNAL(itemCollapsed(QTreeWidgetItem*)),
            this, SLOT(computeSelectedKeysBRect()));
}

/**
 * @brief DopeSheetView::~DopeSheetView
 *
 * Destroys the DopeSheetView object.
 */
DopeSheetView::~DopeSheetView()
{

}

/**
 * @brief DopeSheetView::swapOpenGLBuffers
 *
 *
 */
void DopeSheetView::swapOpenGLBuffers()
{
    RUNNING_IN_MAIN_THREAD();

    swapBuffers();
}

/**
 * @brief DopeSheetView::redraw
 *
 *
 */
void DopeSheetView::redraw()
{
    RUNNING_IN_MAIN_THREAD();

    update();
}

/**
 * @brief DopeSheetView::getViewportSize
 *
 *
 */
void DopeSheetView::getViewportSize(double &width, double &height) const
{
    RUNNING_IN_MAIN_THREAD();

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
    RUNNING_IN_MAIN_THREAD();

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
    RUNNING_IN_MAIN_THREAD();

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
    RUNNING_IN_MAIN_THREAD();


}

/**
 * @brief DopeSheetView::restoreOpenGLContext
 *
 *
 */
void DopeSheetView::restoreOpenGLContext()
{
    RUNNING_IN_MAIN_THREAD();

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
    if (_imp->selectedKeyframes.size() <= 1) {
        _imp->selectedKeysBRect = QRectF();

        return;
    }

    const int SELECTED_KF_BBOX_BOUNDS_OFFSET = 4;

    QRectF rect;
    QTreeWidgetItem *topMostItem = 0;

    for (DSKeyPtrList::const_iterator it = _imp->selectedKeyframes.begin();
         it != _imp->selectedKeyframes.end();
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

        if (it != _imp->selectedKeyframes.begin()) {
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

void DopeSheetView::clearKeyframeSelection()
{
    _imp->selectedKeyframes.clear();

    computeSelectedKeysBRect();
}

void DopeSheetView::selectAllKeyframes()
{
    DSRowsNodeData dsNodeItems = _imp->model->getRowsNodeData();

    for (DSRowsNodeData::const_iterator it = dsNodeItems.begin(); it != dsNodeItems.end(); ++it) {
        DSNode *dsNode = (*it).second;

        DSRowsKnobData dsKnobItems = dsNode->getRowsKnobData();

        for (DSRowsKnobData::const_iterator itKnob = dsKnobItems.begin(); itKnob != dsKnobItems.end(); ++itKnob) {
            DSKnob *dsKnob = (*itKnob).second;

            for (int i = 0; i < dsKnob->getKnobGui()->getKnob()->getDimension(); ++i) {
                KeyFrameSet keyframes = dsKnob->getKnobGui()->getCurve(i)->getKeyFrames_mt_safe();

                for (KeyFrameSet::const_iterator it = keyframes.begin(); it != keyframes.end(); ++it) {
                    KeyFrame kf = *it;

                    DSSelectedKey key (dsKnob, kf, i);

                    DSKeyPtrList::iterator isAlreadySelected = _imp->keyframeIsAlreadyInSelected(key);

                    if (isAlreadySelected == _imp->selectedKeyframes.end()) {
                        DSKeyPtr selected(new DSSelectedKey(key));

                        _imp->selectedKeyframes.push_back(selected);
                    }
                }
            }
        }
    }

    if (_imp->selectedKeyframes.size() > 1) {
        computeSelectedKeysBRect();
    }

    redraw();
}

void DopeSheetView::deleteSelectedKeyframes()
{
    RUNNING_IN_MAIN_THREAD();

    if (_imp->selectedKeyframes.empty()) {
        return;
    }

    _imp->selectedKeysBRect = QRectF();

    std::vector<DSSelectedKey> toRemove;
    for (DSKeyPtrList::iterator it = _imp->selectedKeyframes.begin(); it != _imp->selectedKeyframes.end(); ++it) {
        toRemove.push_back(DSSelectedKey(**it));
    }

    _imp->pushUndoCommand(new DSRemoveKeysCommand(toRemove, this));

    _imp->selectedKeyframes.clear();

    redraw();
}

void DopeSheetView::frame()
{
    RUNNING_IN_MAIN_THREAD();

    if (_imp->selectedKeyframes.size() == 1) {
        return;
    }

    std::pair<double, double> range;

    // frame on project bounds
    if (_imp->selectedKeyframes.empty()) {
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

    if (_imp->selectedKeyframes.size() > 1) {
        computeSelectedKeysBRect();
    }

    redraw();
}

void DopeSheetView::onTimeLineFrameChanged(SequenceTime sTime, int reason)
{
    Q_UNUSED(sTime);
    Q_UNUSED(reason);

    RUNNING_IN_MAIN_THREAD();

    if (_imp->gui->isGUIFrozen()) {
        return;
    }

    _imp->computeTimelinePositions();

    redraw();
}

void DopeSheetView::onTimeLineBoundariesChanged(int, int)
{
    RUNNING_IN_MAIN_THREAD();

    redraw();
}

/**
 * @brief DopeSheetView::initializeGL
 *
 *
 */
void DopeSheetView::initializeGL()
{
    RUNNING_IN_MAIN_THREAD();

    if ( !glewIsSupported("GL_ARB_vertex_array_object ")) {
        _imp->hasOpenGLVAOSupport = false;
    }
}

/**
 * @brief DopeSheetView::resizeGL
 *
 *
 */
void DopeSheetView::resizeGL(int w, int h)
{
    RUNNING_IN_MAIN_THREAD_AND_CONTEXT(this);

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
    RUNNING_IN_MAIN_THREAD_AND_CONTEXT(this);

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
    RUNNING_IN_MAIN_THREAD();

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
            DSRowsNodeData dsNodeItems = _imp->model->getRowsNodeData();
            DSRowsNodeData::const_iterator dsNodeIt = dsNodeItems.find(treeItem);

            // The user clicked on a reader
            if (dsNodeIt != dsNodeItems.end()) {
                DSNode *dsNode = (*dsNodeIt).second;

                std::pair<double, double> range = dsNode->getClipRange();
                QRectF treeItemRect = _imp->hierarchyView->getItemRect(dsNode);
                QRectF nodeClipRect = _imp->rectToZoomCoordinates(QRectF(QPointF(range.first, treeItemRect.top() + 1),
                                                                         QPointF(range.second, treeItemRect.bottom() + 1)));

                DSNode::DSNodeType nodeType = dsNode->getDSNodeType();

                if (nodeType == DSNode::GroupNodeType) {
                    if (nodeClipRect.contains(clickZoomCoords.x(), clickZoomCoords.y())) {
                        _imp->currentEditedGroup = dsNode;

                        _imp->eventState = DopeSheetView::esGroupRepos;
                    }

                    redraw();
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
                        else {
                            _imp->eventState = DopeSheetView::esClipRepos;
                        }

                        KnobIntPtr timeOffsetKnob = dynamic_cast<KnobIntPtr>
                                (_imp->currentEditedReader->getNodeGui()->getNode()->getKnobByName("timeOffset").get());

                        _imp->lastTimeOffsetOnMousePress = timeOffsetKnob->getValue();
                    }

                    redraw();
                }
                else if (nodeType == DSNode::CommonNodeType) {
                    DSSelectedKeys keysUnderMouse = _imp->createSelectionFromCursor(dsNode, e->pos());

                    if (!keysUnderMouse.empty()) {
                        _imp->makeSelection(keysUnderMouse, modCASIsShift(e));

                        computeSelectedKeysBRect();

                        _imp->eventState = DopeSheetView::esMoveKeyframeSelection;

                        redraw();
                    }
                }
            }
            // Or search for a keyframe
            else {
                int knobDim;
                DSKnob *dsKnob = _imp->hierarchyView->getDSKnobAt(e->pos(), &knobDim);

                if (dsKnob) {
                    DSSelectedKeys keysUnderMouse = _imp->createSelectionFromCursor(dsKnob, e->pos(), knobDim);

                    if (!keysUnderMouse.empty()) {
                        _imp->makeSelection(keysUnderMouse, modCASIsShift(e));

                        computeSelectedKeysBRect();

                        _imp->eventState = DopeSheetView::esMoveKeyframeSelection;

                        redraw();
                    }
                }
            }
        }

        // So the user left clicked on background
        if (_imp->eventState == DopeSheetView::esNoEditingState) {
            if (!modCASIsShift(e)) {
                clearKeyframeSelection();

                redraw();
            }

            _imp->eventState = DopeSheetView::esSelectionByRect;

            _imp->selectionRect.setTopLeft(clickZoomCoords);
            _imp->selectionRect.setBottomRight(clickZoomCoords);
        }

        _imp->lastPosOnMousePress = e->pos();
        _imp->keyDragLastMovement = 0.;
    }
}

void DopeSheetView::mouseMoveEvent(QMouseEvent *e)
{
    RUNNING_IN_MAIN_THREAD();

    QPointF mouseZoomCoords = _imp->zoomContext.toZoomCoordinates(e->x(), e->y());

    if (e->buttons() == Qt::NoButton) {
        setCursor(_imp->getCursorDuringHover(e->pos()));
    }
    else if (buttonDownIsLeft(e)) {
        mouseDragEvent(e);
    }
    else if (buttonDownIsMiddle(e)) {
        double dx = _imp->zoomContext.toZoomCoordinates(_imp->lastPosOnMouseMove.x(),
                                                        _imp->lastPosOnMouseMove.y()).x() - mouseZoomCoords.x();
        _imp->zoomContext.translate(dx, 0);

        redraw();
    }

    _imp->lastPosOnMouseMove = e->pos();
}

void DopeSheetView::mouseReleaseEvent(QMouseEvent *e)
{
    Q_UNUSED(e);

    if (_imp->eventState == DopeSheetView::esSelectionByRect) {
        if (_imp->selectedKeyframes.size() > 1) {
            computeSelectedKeysBRect();
        }

        _imp->selectionRect = QRectF();

        redraw();
    }

    if (_imp->eventState != esNoEditingState) {
        _imp->eventState = esNoEditingState;

        if (_imp->currentEditedReader) {
            _imp->currentEditedReader = 0;
        }

        if (_imp->currentEditedGroup) {
            _imp->currentEditedGroup = 0;
        }

        redraw();
    }
}

void DopeSheetView::mouseDragEvent(QMouseEvent *e)
{
    QPointF mouseZoomCoords = _imp->zoomContext.toZoomCoordinates(e->x(), e->y());
    QPointF lastZoomCoordsOnMousePress = _imp->zoomContext.toZoomCoordinates(_imp->lastPosOnMousePress.x(),
                                                                             _imp->lastPosOnMousePress.y());
    double currentTime = mouseZoomCoords.x();

    switch (_imp->eventState) {
    case DopeSheetView::esMoveKeyframeSelection:
    {
        double totalMovement = currentTime - lastZoomCoordsOnMousePress.x();
        // Clamp the motion to the nearet integer
        totalMovement = std::floor(totalMovement + 0.5);

        double dt = totalMovement - _imp->keyDragLastMovement;

        if (dt >= 1.0f || dt <= -1.0f) {
            _imp->pushUndoCommand(new DSMoveKeysCommand(_imp->selectedKeyframes, dt, this));
        }

        // Update the last drag movement
        _imp->keyDragLastMovement = totalMovement;

        break;
    }
    case DopeSheetView::esMoveCurrentFrameIndicator:
        _imp->moveCurrentFrameIndicator(mouseZoomCoords.x());

        break;
    case DopeSheetView::esSelectionByRect:
    {
        _imp->computeSelectionRect(lastZoomCoordsOnMousePress, mouseZoomCoords);
        DSSelectedKeys tempSelection = _imp->createSelectionFromRect(_imp->rectToZoomCoordinates(_imp->selectionRect));

        _imp->makeSelection(tempSelection, modCASIsShift(e));

        redraw();

        break;
    }
    case DopeSheetView::esReaderLeftTrim:
    {
        KnobIntPtr timeOffsetKnob = dynamic_cast<KnobIntPtr>
                (_imp->currentEditedReader->getNodeGui()->getNode()->getKnobByName("timeOffset").get());
        KnobIntPtr firstFrameKnob = dynamic_cast<KnobIntPtr>
                (_imp->currentEditedReader->getNodeGui()->getNode()->getKnobByName("firstFrame").get());

        double newTime = (currentTime - timeOffsetKnob->getValue());

        _imp->pushUndoCommand(new DSLeftTrimReaderCommand(_imp->currentEditedReader, firstFrameKnob->getValue(), newTime, this));

        break;
    }
    case DopeSheetView::esReaderRightTrim:
    {
        KnobIntPtr timeOffsetKnob = dynamic_cast<KnobIntPtr>
                (_imp->currentEditedReader->getNodeGui()->getNode()->getKnobByName("timeOffset").get());
        KnobIntPtr lastFrameKnob = dynamic_cast<KnobIntPtr>
                (_imp->currentEditedReader->getNodeGui()->getNode()->getKnobByName("lastFrame").get());

        double newTime = (currentTime - timeOffsetKnob->getValue());

        _imp->pushUndoCommand(new DSRightTrimReaderCommand(_imp->currentEditedReader, lastFrameKnob->getValue(), newTime, this));

        break;
    }
    case DopeSheetView::esClipRepos:
    {
        KnobIntPtr timeOffsetKnob = dynamic_cast<KnobIntPtr>
                (_imp->currentEditedReader->getNodeGui()->getNode()->getKnobByName("timeOffset").get());

        int mouseOffset = (lastZoomCoordsOnMousePress.x() - _imp->lastTimeOffsetOnMousePress);
        double newTime = (currentTime - mouseOffset);

        _imp->pushUndoCommand(new DSMoveReaderCommand(_imp->currentEditedReader, timeOffsetKnob->getValue(), newTime, this));

        break;
    }
    case DopeSheetView::esGroupRepos:
    {
        double totalMovement = currentTime - lastZoomCoordsOnMousePress.x();
        // Clamp the motion to the nearet integer
        totalMovement = std::floor(totalMovement + 0.5);

        double dt = totalMovement - _imp->keyDragLastMovement;

        _imp->pushUndoCommand(new DSMoveGroupCommand(_imp->currentEditedGroup, dt, this));

        // Update the last drag movement
        _imp->keyDragLastMovement = totalMovement;

        break;
    }
    default:
        break;
    }
}

void DopeSheetView::wheelEvent(QWheelEvent *e)
{
    RUNNING_IN_MAIN_THREAD();

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
}

void DopeSheetView::enterEvent(QEvent *e)
{
    RUNNING_IN_MAIN_THREAD();

    setFocus();

    QGLWidget::enterEvent(e);
}

void DopeSheetView::focusInEvent(QFocusEvent *e)
{
    QGLWidget::focusInEvent(e);

    _imp->undoStack->setActive();
}

void DopeSheetView::keyPressEvent(QKeyEvent *e)
{
    RUNNING_IN_MAIN_THREAD();

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
    RUNNING_IN_MAIN_THREAD_AND_CONTEXT(this);

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
