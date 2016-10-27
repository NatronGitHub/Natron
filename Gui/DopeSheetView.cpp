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

#include "DopeSheetView.h"

#include <algorithm> // min, max
#include <limits>
#include <stdexcept>

// Qt includes
#include <QApplication>
#include <QMouseEvent>
#include <QScrollBar>
#include <QtCore/QThread>
#include <QToolButton>

// Natron includes
#include "Engine/Curve.h"
#include "Engine/Image.h"
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/Project.h"
#include "Engine/Settings.h"
#include "Engine/StringAnimationManager.h"
#include "Engine/ViewIdx.h"

#include "Global/Enums.h"

#include "Gui/ActionShortcuts.h"
#include "Gui/CurveWidget.h"
#include "Gui/DockablePanel.h"
#include "Gui/AnimationModule.h"
#include "Gui/AnimationModuleEditor.h"
#include "Gui/AnimationModuleTreeView.h"
#include "Gui/AnimationModuleSelectionModel.h"
#include "Gui/AnimationModuleViewPrivateBase.h"
#include "Gui/CurveGui.h"
#include "Gui/Gui.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiDefines.h"
#include "Gui/GuiMacros.h"
#include "Gui/KnobGui.h"
#include "Gui/KnobAnim.h"
#include "Gui/Menu.h"
#include "Gui/NodeGui.h"
#include "Gui/NodeAnim.h"
#include "Gui/TableItemAnim.h"
#include "Gui/TextRenderer.h"
#include "Gui/ticks.h"
#include "Gui/TabWidget.h"

#define NATRON_DOPESHEET_MIN_RANGE_FIT 10

#define CURSOR_WIDTH 15
#define CURSOR_HEIGHT 8

NATRON_NAMESPACE_ENTER;


NATRON_NAMESPACE_ANONYMOUS_ENTER

//Protect declarations in an anonymous namespace

// Typedefs
typedef std::set<double> TimeSet;
typedef std::pair<double, double> FrameRange;
typedef std::map<KnobIWPtr, KnobGui *> KnobsAndGuis;


// Constants
static const int DISTANCE_ACCEPTANCE_FROM_KEYFRAME = 5;
static const int DISTANCE_ACCEPTANCE_FROM_READER_EDGE = 14;
static const int DISTANCE_ACCEPTANCE_FROM_READER_BOTTOM = 8;


////////////////////////// Helpers //////////////////////////


void
running_in_main_thread()
{
    assert( qApp && qApp->thread() == QThread::currentThread() );
}

void
running_in_main_context(const QGLWidget *glWidget)
{
    assert( glWidget->context() == QGLContext::currentContext() );
    Q_UNUSED(glWidget);
}

void
running_in_main_thread_and_context(const QGLWidget *glWidget)
{
    running_in_main_thread();
    running_in_main_context(glWidget);
}

NATRON_NAMESPACE_ANONYMOUS_EXIT


////////////////////////// DopeSheetView //////////////////////////

class DopeSheetViewPrivate : public AnimationModuleViewPrivateBase
{
    Q_DECLARE_TR_FUNCTIONS(DopeSheetView)

public:

    DopeSheetViewPrivate(Gui* gui, AnimationModuleBasePtr& model, DopeSheetView *publicInterface);

    virtual ~DopeSheetViewPrivate()
    {

    }

    /* functions */

    // Helpers
    // This function is just wrong and confusing, it takes keyTime which is in zoom Coordinates  and y which is in widget coordinates
    //QRectF keyframeRect(double keyTime, double y) const;

    // It is now much more explicit in which coord. system each value lies. The returned rectangle is in zoom coordinates.
    RectD getKeyFrameBoundingRectZoomCoords(double keyframeTimeZoomCoords, double keyframeYCenterWidgetCoords) const;

    /*
       QRectF and Qt coordinate system has its y axis top-down, whereas in Natron
       the coordinate system is bottom-up. When using QRectF, top-left is in fact (0,0)
       while in Natron it is (0,h - 1).
       Hence we use different data types to identify the 2 different coordinate systems.
     */
    RectD rectToZoomCoordinates(const QRectF &rect) const;
    QRectF rectToWidgetCoordinates(const RectD &rect) const;
    QRectF nameItemRectToRowRect(const QRectF &rect) const;

    Qt::CursorShape getCursorDuringHover(const QPointF &widgetCoords) const;
    Qt::CursorShape getCursorForEventState(DopeSheetView::EventStateEnum es) const;

    bool isNearByClipRectLeft(const QPointF& zoomCoordPos, const RectD &clipRect) const;
    bool isNearByClipRectRight(const QPointF& zoomCoordPos, const RectD &clipRect) const;
    bool isNearByClipRectBottom(const QPointF& zoomCoordPos, const RectD &clipRect) const;
    bool isNearByCurrentFrameIndicatorBottom(const QPointF &zoomCoords) const;

    bool isNearbySelectedKeysBRectRightEdge(const QPointF& widgetPos) const;
    bool isNearbySelectedKeysBRectLeftEdge(const QPointF& widgetPos) const;

    bool isNearbySelectedKeysBRec(const QPointF& widgetPos) const;

    KeyFrameWithStringSet isNearByKeyframe(const AnimItemBasePtr &item, DimSpec dimension, ViewSetSpec view, const QPointF &widgetCoords) const;

    double clampedMouseOffset(double fromTime, double toTime);

    // Textures


    // Drawing
    void drawScale() const;

    void drawRows() const;

    void drawTreeItemRecursive(QTreeWidgetItem* item, std::list<NodeAnimPtr>* nodesRowsOrdered) const;

    void drawNodeRow(QTreeWidgetItem* treeItem, const NodeAnimPtr& item) const;
    void drawKnobRow(QTreeWidgetItem* treeItem, const KnobAnimPtr& item, DimSpec dimension, ViewSetSpec view) const;
    void drawTableItemRow(QTreeWidgetItem* treeItem, const TableItemAnimPtr& item, DimSpec dimension, ViewSetSpec view) const;

    void drawNodeRowSeparation(const NodeAnimPtr NodeAnim) const;

    void drawRange(const NodeAnimPtr &NodeAnim) const;
    void drawKeyframes(QTreeWidgetItem* treeItem, const AnimItemBasePtr &item, DimSpec dimension, ViewSetSpec view) const;


    void drawGroupOverlay(const NodeAnimPtr &NodeAnim, const NodeAnimPtr &group) const;

    // After or during an user interaction
    void computeSelectionRect(const QPointF &origin, const QPointF &current);

    void computeSelectedKeysBRect();

    void computeRangesBelow(const NodeAnimPtr& NodeAnim);
    void computeNodeRange(const NodeAnimPtr& NodeAnim);
    void computeReaderRange(const NodeAnimPtr& reader);
    void computeRetimeRange(const NodeAnimPtr& retimer);
    void computeTimeOffsetRange(const NodeAnimPtr& timeOffset);
    void computeFRRange(const NodeAnimPtr& frameRange);
    void computeGroupRange(const NodeAnimPtr& group);
    void computeCommonNodeRange(const NodeAnimPtr& node);

    // User interaction
    void onMouseLeftButtonDrag(QMouseEvent *e);

    void createSelectionFromRect(const RectD &rect, AnimItemDimViewKeyFramesMap *result, std::vector<NodeAnimPtr >* selectedNodes, std::vector<TableItemAnimPtr >* selectedItems);

    void moveCurrentFrameIndicator(double dt);

    void updateCurveWidgetFrameRange();

    /* attributes */
    DopeSheetView *_widget;
    AnimationModuleTreeView *treeView;

    std::map<NodeAnimPtr, FrameRange> nodeRanges;
    std::list<NodeAnimPtr> nodeRangesBeingComputed; // to avoid recursion in groups
    int rangeComputationRecursion;


    // for various user interaction
    QPointF lastPosOnMousePress;
    QPointF lastPosOnMouseMove;
    double keyDragLastMovement;
    DopeSheetView::EventStateEnum eventState;

    // for clip (Reader, Time nodes) user interaction
    NodeAnimPtr currentEditedReader;

};

DopeSheetViewPrivate::DopeSheetViewPrivate(Gui* gui, AnimationModuleBasePtr& model, DopeSheetView *publicInterface)
: AnimationModuleViewPrivateBase(gui, publicInterface, model)
, _widget(0)
, treeView(0)
, nodeRanges()
, nodeRangesBeingComputed()
, rangeComputationRecursion(0)
, lastPosOnMousePress()
, lastPosOnMouseMove()
, keyDragLastMovement()
, eventState(DopeSheetView::esNoEditingState)
, currentEditedReader()
{
}

/**
 * @brief DopeSheetView::DopeSheetView
 *
 * Constructs a DopeSheetView object.
 */
DopeSheetView::DopeSheetView(QWidget *parent)
: AnimationViewBase(parent)
, _imp()
{
    setMouseTracking(true);
}

/**
 * @brief DopeSheetView::~DopeSheetView
 *
 * Destroys the DopeSheetView object.
 */
DopeSheetView::~DopeSheetView()
{
}


void
DopeSheetView::initializeImplementation(Gui* gui, AnimationModuleBasePtr& model, AnimationViewBase* /*publicInterface*/)
{
    _imp.reset(new DopeSheetViewPrivate(gui, model, this));
    _imp->_widget = this;
    AnimationModulePtr animModule = toAnimationModule(model);
    assert(animModule);
    _imp->treeView = animModule->getEditor()->getTreeView();
    setImplementation(_imp);
}



RectD
DopeSheetViewPrivate::getKeyFrameBoundingRectZoomCoords(double keyframeTimeZoomCoords,
                                                        double keyframeYCenterWidgetCoords) const
{
    double keyframeTimeWidget = zoomCtx.toWidgetCoordinates(keyframeTimeZoomCoords, 0).x();
    RectD ret;
    QPointF x1y1 = zoomCtx.toZoomCoordinates(keyframeTimeWidget - KF_X_OFFSET, keyframeYCenterWidgetCoords + KF_PIXMAP_SIZE / 2);
    QPointF x2y2 = zoomCtx.toZoomCoordinates(keyframeTimeWidget + KF_X_OFFSET, keyframeYCenterWidgetCoords - KF_PIXMAP_SIZE / 2);

    ret.x1 = x1y1.x();
    ret.y1 = x1y1.y();
    ret.x2 = x2y2.x();
    ret.y2 = x2y2.y();

    return ret;
}

/*
   QRectF and Qt coordinate system has its y axis top-down, whereas in Natron
   the coordinate system is bottom-up. When using QRectF, top-left is in fact (0,0)
   while in Natron it is (0,h - 1).
   Hence we use different data types to identify the 2 different coordinate systems.
 */
RectD
DopeSheetViewPrivate::rectToZoomCoordinates(const QRectF &rect) const
{
    QPointF topLeft = rect.topLeft();

    topLeft = zoomCtx.toZoomCoordinates( topLeft.x(), topLeft.y() );
    RectD ret;
    ret.x1 = topLeft.x();
    ret.y2 = topLeft.y();
    QPointF bottomRight = rect.bottomRight();
    bottomRight = zoomCtx.toZoomCoordinates( bottomRight.x(), bottomRight.y() );
    ret.x2 = bottomRight.x();
    ret.y1 = bottomRight.y();
    assert(ret.y2 >= ret.y1 && ret.x2 >= ret.x1);

    return ret;
}

/*
   QRectF and Qt coordinate system has its y axis top-down, whereas in Natron
   the coordinate system is bottom-up. When using QRectF, top-left is in fact (0,0)
   while in Natron it is (0,h - 1).
   Hence we use different data types to identify the 2 different coordinate systems.
 */
QRectF
DopeSheetViewPrivate::rectToWidgetCoordinates(const RectD &rect) const
{
    QPointF topLeft = zoomCtx.toWidgetCoordinates(rect.x1, rect.y2);
    QPointF bottomRight = zoomCtx.toWidgetCoordinates(rect.x2, rect.y1);

    return QRectF( topLeft.x(), topLeft.y(), bottomRight.x() - topLeft.x(), bottomRight.y() - topLeft.y() );
}

QRectF
DopeSheetViewPrivate::nameItemRectToRowRect(const QRectF &rect) const
{
    QPointF topLeft = rect.topLeft();

    topLeft = zoomCtx.toZoomCoordinates( topLeft.x(), topLeft.y() );
    QPointF bottomRight = rect.bottomRight();
    bottomRight = zoomCtx.toZoomCoordinates( bottomRight.x(), bottomRight.y() );

    return QRectF( QPointF( zoomCtx.left(), topLeft.y() ),
                   QPointF( zoomCtx.right(), bottomRight.y() ) );
}

Qt::CursorShape
DopeSheetViewPrivate::getCursorDuringHover(const QPointF &widgetCoords) const
{
    QPointF clickZoomCoords = zoomCtx.toZoomCoordinates( widgetCoords.x(), widgetCoords.y() );

    if ( isNearbySelectedKeysBRec(widgetCoords) ) {
        return getCursorForEventState(DopeSheetView::esMoveKeyframeSelection);
    } else if ( isNearbySelectedKeysBRectRightEdge(widgetCoords) ) {
        return getCursorForEventState(DopeSheetView::esTransformingKeyframesMiddleRight);
    } else if ( isNearbySelectedKeysBRectLeftEdge(widgetCoords) ) {
        return getCursorForEventState(DopeSheetView::esTransformingKeyframesMiddleLeft);
    } else if ( isNearByCurrentFrameIndicatorBottom(clickZoomCoords) ) {
        return getCursorForEventState(DopeSheetView::esMoveCurrentFrameIndicator);
    }

    // Look for a range node
    AnimationModuleBasePtr animModel = _model.lock();
    const std::list<NodeAnimPtr>& selectedNodes = animModel->getSelectionModel()->getCurrentNodesSelection();
    for (std::list<NodeAnimPtr>::const_iterator it = selectedNodes.begin(); it != selectedNodes.end(); ++it) {
        if ( (*it)->isRangeDrawingEnabled() ) {
            std::map<NodeAnimPtr, FrameRange >::const_iterator foundRange = nodeRanges.find(*it);
            if ( foundRange == nodeRanges.end() ) {
                continue;
            }

            QRectF treeItemRect = treeView->visualItemRect((*it)->getTreeItem());
            const FrameRange& range = foundRange->second;
            RectD nodeClipRect;
            nodeClipRect.x1 = range.first;
            nodeClipRect.x2 = range.second;
            nodeClipRect.y2 = zoomCtx.toZoomCoordinates( 0, treeItemRect.top() ).y();
            nodeClipRect.y1 = zoomCtx.toZoomCoordinates( 0, treeItemRect.bottom() ).y();

            AnimatedItemTypeEnum nodeType = (*it)->getItemType();
            if ( nodeClipRect.contains( clickZoomCoords.x(), clickZoomCoords.y() ) ) {
                return getCursorForEventState(DopeSheetView::esMoveKeyframeSelection);
            }
            if (nodeType == eAnimatedItemTypeReader) {
                if ( isNearByClipRectLeft(clickZoomCoords, nodeClipRect) ) {
                    return getCursorForEventState(DopeSheetView::esReaderLeftTrim);
                } else if ( isNearByClipRectRight(clickZoomCoords, nodeClipRect) ) {
                    return getCursorForEventState(DopeSheetView::esReaderRightTrim);
                } else if ( animModel->canSlipReader(*it) && isNearByClipRectBottom(clickZoomCoords, nodeClipRect) ) {
                    return getCursorForEventState(DopeSheetView::esReaderSlip);
                }
            }
        }
    } // for (AnimTreeItemNodeMap::iterator it = nodes.begin(); it!=nodes.end(); ++it) {

    QTreeWidgetItem *treeItem = treeView->itemAt( 0, widgetCoords.y() );
    //Did not find a range node, look for keyframes
    if (treeItem) {

        AnimatedItemTypeEnum itemType;
        KnobAnimPtr isKnob;
        NodeAnimPtr isNode;
        TableItemAnimPtr isTableItem;
        ViewSetSpec view;
        DimSpec dimension;
        if (animModel->findItem(treeItem, &itemType, &isKnob, &isTableItem, &isNode, &view, &dimension)) {
            AnimItemBasePtr itemBase;
            if (isKnob) {
                itemBase = isKnob;
            } else if (isTableItem) {
                itemBase = isTableItem;
            }
            if (itemBase) {
                KeyFrameWithStringSet keysUnderMouse = isNearByKeyframe(itemBase, dimension, view, widgetCoords);

                if ( !keysUnderMouse.empty() ) {
                    return getCursorForEventState(DopeSheetView::esPickKeyframe);
                }
            }
        }
    } // if (treeItem) {

    return getCursorForEventState(DopeSheetView::esNoEditingState);
} // DopeSheetViewPrivate::getCursorDuringHover

Qt::CursorShape
DopeSheetViewPrivate::getCursorForEventState(DopeSheetView::EventStateEnum es) const
{
    Qt::CursorShape cursorShape;

    switch (es) {
    case DopeSheetView::esPickKeyframe:
        cursorShape = Qt::CrossCursor;
        break;
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
    case DopeSheetView::esTransformingKeyframesMiddleLeft:
    case DopeSheetView::esTransformingKeyframesMiddleRight:
        cursorShape = Qt::SplitHCursor;
        break;
    case DopeSheetView::esNoEditingState:
    default:
        cursorShape = Qt::ArrowCursor;
        break;
    }

    return cursorShape;
}

bool
DopeSheetViewPrivate::isNearByClipRectLeft(const QPointF& zoomCoordPos,
                                           const RectD &clipRect) const
{
    QPointF widgetPos = zoomCtx.toWidgetCoordinates( zoomCoordPos.x(), zoomCoordPos.y() );
    QPointF rectx1y1 = zoomCtx.toWidgetCoordinates( clipRect.left(), clipRect.bottom() );
    QPointF rectx2y2 = zoomCtx.toWidgetCoordinates( clipRect.right(), clipRect.top() );

    return ( (widgetPos.x() >= rectx1y1.x() - TO_DPIX(DISTANCE_ACCEPTANCE_FROM_READER_EDGE)) &&
             ( widgetPos.x() <= rectx1y1.x() ) &&
             (widgetPos.y() <= rectx1y1.y() + TO_DPIX(DISTANCE_ACCEPTANCE_FROM_READER_EDGE)) &&
             (widgetPos.y() >= rectx2y2.y() - TO_DPIX(DISTANCE_ACCEPTANCE_FROM_READER_EDGE)) );
}

bool
DopeSheetViewPrivate::isNearByClipRectRight(const QPointF& zoomCoordPos,
                                            const RectD &clipRect) const
{
    QPointF widgetPos = zoomCtx.toWidgetCoordinates( zoomCoordPos.x(), zoomCoordPos.y() );
    QPointF rectx1y1 = zoomCtx.toWidgetCoordinates( clipRect.left(), clipRect.bottom() );
    QPointF rectx2y2 = zoomCtx.toWidgetCoordinates( clipRect.right(), clipRect.top() );

    return ( ( widgetPos.x() >= rectx2y2.x() ) &&
             (widgetPos.x() <= rectx2y2.x() + TO_DPIX(DISTANCE_ACCEPTANCE_FROM_READER_EDGE)) &&
             (widgetPos.y() <= rectx1y1.y() + TO_DPIX(DISTANCE_ACCEPTANCE_FROM_READER_EDGE)) &&
             (widgetPos.y() >= rectx2y2.y() - TO_DPIX(DISTANCE_ACCEPTANCE_FROM_READER_EDGE)) );
}

bool
DopeSheetViewPrivate::isNearByClipRectBottom(const QPointF& zoomCoordPos,
                                             const RectD &clipRect) const
{
    QPointF widgetPos = zoomCtx.toWidgetCoordinates( zoomCoordPos.x(), zoomCoordPos.y() );
    QPointF rectx1y1 = zoomCtx.toWidgetCoordinates( clipRect.left(), clipRect.bottom() );
    QPointF rectx2y2 = zoomCtx.toWidgetCoordinates( clipRect.right(), clipRect.top() );

    return ( (widgetPos.x() >= rectx1y1.x() - TO_DPIX(DISTANCE_ACCEPTANCE_FROM_READER_BOTTOM)) &&
             (widgetPos.x() <= rectx2y2.x() + TO_DPIX(DISTANCE_ACCEPTANCE_FROM_READER_EDGE)) &&
             (widgetPos.y() <= rectx1y1.y() + TO_DPIX(DISTANCE_ACCEPTANCE_FROM_READER_BOTTOM)) &&
             (widgetPos.y() >= rectx1y1.y() - TO_DPIX(DISTANCE_ACCEPTANCE_FROM_READER_BOTTOM)) );
}

KeyFrameWithStringSet DopeSheetViewPrivate::isNearByKeyframe(const AnimItemBasePtr &item,
                                                                            DimSpec dimension, ViewSetSpec view,
                                                                            const QPointF &widgetCoords) const
{
    KeyFrameWithStringSet ret;
    KeyFrameWithStringSet keys;
    item->getKeyframes(dimension, view, &keys);

    for (KeyFrameWithStringSet::const_iterator it = keys.begin(); it != keys.end(); ++it) {

        double keyframeXWidget = zoomCtx.toWidgetCoordinates(it->key.getTime(), 0).x();

        if (std::abs( widgetCoords.x() - keyframeXWidget ) < TO_DPIX(DISTANCE_ACCEPTANCE_FROM_KEYFRAME)) {
            ret.insert(*it);
        }
    }

    return ret;
}

double
DopeSheetViewPrivate::clampedMouseOffset(double fromTime,
                                         double toTime)
{
    double totalMovement = toTime - fromTime;

    // Clamp the motion to the nearet integer
    totalMovement = std::floor(totalMovement + 0.5);

    double dt = totalMovement - keyDragLastMovement;

    // Update the last drag movement
    keyDragLastMovement = totalMovement;

    return dt;
}



/**
 * @brief DopeSheetViewPrivate::drawScale
 *
 * Draws the dope sheet's grid and time indicators.
 */
void
DopeSheetViewPrivate::drawScale() const
{
    running_in_main_thread_and_context(_widget);

    QPointF bottomLeft = zoomCtx.toZoomCoordinates(0, _widget->height() - 1);
    QPointF topRight = zoomCtx.toZoomCoordinates(_widget->width() - 1, 0);

    // Don't attempt to draw a scale on a widget with an invalid height
    if (_widget->height() <= 1) {
        return;
    }

    QFontMetrics fontM(_widget->font());
    const double smallestTickSizePixel = 5.; // tick size (in pixels) for alpha = 0.
    const double largestTickSizePixel = 1000.; // tick size (in pixels) for alpha = 1.

    // Retrieve the appropriate settings for drawing
    SettingsPtr settings = appPTR->getCurrentSettings();
    double scaleR, scaleG, scaleB;
    settings->getAnimationModuleEditorScaleColor(&scaleR, &scaleG, &scaleB);

    QColor scaleColor;
    scaleColor.setRgbF( Image::clamp(scaleR, 0., 1.),
                        Image::clamp(scaleG, 0., 1.),
                        Image::clamp(scaleB, 0., 1.) );

    // Perform drawing
    {
        GLProtectAttrib<GL_GPU> a(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT);

        GL_GPU::glEnable(GL_BLEND);
        GL_GPU::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        const double rangePixel = _widget->width();
        const double range_min = bottomLeft.x();
        const double range_max = topRight.x();
        const double range = range_max - range_min;
        double smallTickSize;
        bool half_tick;

        ticks_size(range_min, range_max, rangePixel, smallestTickSizePixel, &smallTickSize, &half_tick);
        // nothing happens in the dopesheet at a scale smaller than 1 frame
        if (smallTickSize < 1.) {
            smallTickSize = 1;
            half_tick = false;
        }
        int m1, m2;
        const int ticks_max = 1000;
        double offset;

        ticks_bounds(range_min, range_max, smallTickSize, half_tick, ticks_max, &offset, &m1, &m2);
        std::vector<int> ticks;
        ticks_fill(half_tick, ticks_max, m1, m2, &ticks);

        const double smallestTickSize = range * smallestTickSizePixel / rangePixel;
        const double largestTickSize = range * largestTickSizePixel / rangePixel;
        const double minTickSizeTextPixel = fontM.width( QString::fromUtf8("00") );
        const double minTickSizeText = range * minTickSizeTextPixel / rangePixel;

        glCheckError(GL_GPU);

        for (int i = m1; i <= m2; ++i) {
            double value = i * smallTickSize + offset;
            const double tickSize = ticks[i - m1] * smallTickSize;
            const double alpha = ticks_alpha(smallestTickSize, largestTickSize, tickSize);

            GL_GPU::glColor4f(scaleColor.redF(), scaleColor.greenF(), scaleColor.blueF(), alpha);

            // Draw the vertical lines belonging to the grid
            GL_GPU::glBegin(GL_LINES);
            GL_GPU::glVertex2f( value, bottomLeft.y() );
            GL_GPU::glVertex2f( value, topRight.y() );
            GL_GPU::glEnd();

            glCheckErrorIgnoreOSXBug(GL_GPU);

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

                    renderText(value, bottomLeft.y(), s, c, _widget->font(), Qt::AlignHCenter);

                    // Uncomment the line below to draw the indicator on top too
                    // parent->renderText(value, topRight.y() - 20, s, c, *font);
                }
            }
        }
    }
} // DopeSheetViewPrivate::drawScale


void
DopeSheetViewPrivate::drawTreeItemRecursive(QTreeWidgetItem* item, std::list<NodeAnimPtr>* nodesRowsOrdered) const
{
    assert(item);
    if ( item->isHidden() ) {
        return;
    }

    QTreeWidgetItem* parentItem = item->parent();
    if (parentItem && !parentItem->isExpanded()) {
        return;
    }

    AnimatedItemTypeEnum type = (AnimatedItemTypeEnum)item->data(0, QT_ROLE_CONTEXT_TYPE).toInt();
    void* ptr = item->data(0, QT_ROLE_CONTEXT_ITEM_POINTER).value<void*>();
    assert(ptr);
    if (!ptr) {
        return;
    }
    DimSpec dimension = DimSpec(item->data(0, QT_ROLE_CONTEXT_DIM).toInt());
    ViewSetSpec view = ViewSetSpec(item->data(0, QT_ROLE_CONTEXT_VIEW).toInt());

    NodeAnimPtr isNodeAnim;
    TableItemAnimPtr isTableItemAnim;
    KnobAnimPtr isKnobAnim;
    switch (type) {
        case eAnimatedItemTypeCommon:
        case eAnimatedItemTypeFrameRange:
        case eAnimatedItemTypeGroup:
        case eAnimatedItemTypeReader:
        case eAnimatedItemTypeRetime:
        case eAnimatedItemTypeTimeOffset: {
            isNodeAnim = ((NodeAnim*)ptr)->shared_from_this();
        }   break;
        case eAnimatedItemTypeTableItemAnimation:
        case eAnimatedItemTypeTableItemRoot: {
            isTableItemAnim = toTableItemAnim(((AnimItemBase*)ptr)->shared_from_this());
        }   break;
        case eAnimatedItemTypeKnobDim:
        case eAnimatedItemTypeKnobRoot:
        case eAnimatedItemTypeKnobView: {
            isKnobAnim = toKnobAnim(((AnimItemBase*)ptr)->shared_from_this());
        }   break;
    }

    if (isNodeAnim) {
        drawNodeRow(item, isNodeAnim);
        nodesRowsOrdered->push_back(isNodeAnim);
    } else if (isKnobAnim) {
        drawKnobRow(item, isKnobAnim, dimension, view);
    } else if (isTableItemAnim) {
        drawTableItemRow(item, isTableItemAnim, dimension, view);
    }

} // drawTreeItemRecursive

void
DopeSheetViewPrivate::drawRows() const
{
    running_in_main_thread_and_context(_widget);

    {
        GLProtectAttrib<GL_GPU> a(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT);
        GL_GPU::glEnable(GL_BLEND);
        GL_GPU::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        std::list<NodeAnimPtr> nodesAnimOrdered;
        int nTopLevelTreeItems = treeView->topLevelItemCount();
        for (int i = 0; i < nTopLevelTreeItems; ++i) {
            QTreeWidgetItem* topLevelItem = treeView->topLevelItem(nTopLevelTreeItems);
            if (!topLevelItem) {
                continue;
            }
            drawTreeItemRecursive(topLevelItem, &nodesAnimOrdered);
            
        }

        // Draw node rows separations
        for (std::list<NodeAnimPtr>::const_iterator it = nodesAnimOrdered.begin(); it != nodesAnimOrdered.end(); ++it) {
            bool isTreeViewTopItem = !treeView->itemAbove( (*it)->getTreeItem() );
            if (!isTreeViewTopItem) {
                drawNodeRowSeparation(*it);
            }
        }

    } // GlProtectAttrib
} // DopeSheetViewPrivate::drawRows

void
DopeSheetViewPrivate::drawNodeRow(QTreeWidgetItem* /*treeItem*/, const NodeAnimPtr& item) const
{
    QRectF nameItemRect = treeView->visualItemRect( item->getTreeItem() );
    QRectF rowRect = nameItemRectToRowRect(nameItemRect);
    SettingsPtr settings = appPTR->getCurrentSettings();
    double rootR, rootG, rootB, rootA;

    settings->getAnimationModuleEditorRootRowBackgroundColor(&rootR, &rootG, &rootB, &rootA);

    GL_GPU::glColor4f(rootR, rootG, rootB, rootA);

    GL_GPU::glBegin(GL_POLYGON);
    GL_GPU::glVertex2f( rowRect.left(), rowRect.top() );
    GL_GPU::glVertex2f( rowRect.left(), rowRect.bottom() );
    GL_GPU::glVertex2f( rowRect.right(), rowRect.bottom() );
    GL_GPU::glVertex2f( rowRect.right(), rowRect.top() );
    GL_GPU::glEnd();

    {
        AnimationModulePtr animModel = toAnimationModule(_model.lock());
        if (animModel) {
            NodeAnimPtr group = animModel->getGroupNodeAnim(item);
            if (group) {
                drawGroupOverlay(item, group);
            }
        }
    }

    if ( item->isRangeDrawingEnabled() ) {
        drawRange(item);
    }

}

void
DopeSheetViewPrivate::drawTableItemRow(QTreeWidgetItem* treeItem, const TableItemAnimPtr& item, DimSpec dimension, ViewSetSpec view) const
{

}

void
DopeSheetViewPrivate::drawKnobRow(QTreeWidgetItem* treeItem, const KnobAnimPtr& item, DimSpec dimension, ViewSetSpec view) const
{


    QRectF nameItemRect = treeView->visualItemRect(treeItem);
    QRectF rowRect = nameItemRectToRowRect(nameItemRect);
    SettingsPtr settings = appPTR->getCurrentSettings();
    double bkR, bkG, bkB, bkA;
    if ( dimension.isAll() || view.isAll() ) {
        settings->getAnimationModuleEditorRootRowBackgroundColor(&bkR, &bkG, &bkB, &bkA);
    } else {
        settings->getAnimationModuleEditorKnobRowBackgroundColor(&bkR, &bkG, &bkB, &bkA);
    }

    GL_GPU::glColor4f(bkR, bkG, bkB, bkA);

    GL_GPU::glBegin(GL_POLYGON);
    GL_GPU::glVertex2f( rowRect.left(), rowRect.top() );
    GL_GPU::glVertex2f( rowRect.left(), rowRect.bottom() );
    GL_GPU::glVertex2f( rowRect.right(), rowRect.bottom() );
    GL_GPU::glVertex2f( rowRect.right(), rowRect.top() );
    GL_GPU::glEnd();

    drawKeyframes(treeItem, item, dimension, view);
}

void
DopeSheetViewPrivate::drawNodeRowSeparation(const NodeAnimPtr item) const
{
    GLProtectAttrib<GL_GPU> a(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT | GL_LINE_BIT);
    QRectF nameItemRect = treeView->visualItemRect( item->getTreeItem() );
    QRectF rowRect = nameItemRectToRowRect(nameItemRect);

    GL_GPU::glLineWidth(TO_DPIY(4));
    GL_GPU::glColor4f(0.f, 0.f, 0.f, 1.f);

    GL_GPU::glBegin(GL_LINES);
    GL_GPU::glVertex2f( rowRect.left(), rowRect.top() );
    GL_GPU::glVertex2f( rowRect.right(), rowRect.top() );
    GL_GPU::glEnd();
}

void
DopeSheetViewPrivate::drawRange(const NodeAnimPtr &item) const
{
    // Draw the clip
    {
        std::map<NodeAnimPtr, FrameRange >::const_iterator foundRange = nodeRanges.find( item );
        if ( foundRange == nodeRanges.end() ) {
            return;
        }


        const FrameRange& range = foundRange->second;
        QRectF treeItemRect = treeView->visualItemRect( item->getTreeItem() );
        QPointF treeRectTopLeft = treeItemRect.topLeft();
        treeRectTopLeft = zoomCtx.toZoomCoordinates( treeRectTopLeft.x(), treeRectTopLeft.y() );
        QPointF treeRectBtmRight = treeItemRect.bottomRight();
        treeRectBtmRight = zoomCtx.toZoomCoordinates( treeRectBtmRight.x(), treeRectBtmRight.y() );

        RectD clipRectZoomCoords;
        clipRectZoomCoords.x1 = range.first;
        clipRectZoomCoords.x2 = range.second;
        clipRectZoomCoords.y2 = treeRectTopLeft.y();
        clipRectZoomCoords.y1 = treeRectBtmRight.y();
        GLProtectAttrib<GL_GPU> a(GL_CURRENT_BIT);
        QColor fillColor = item->getNodeGui()->getCurrentColor();
        fillColor = QColor::fromHsl( fillColor.hslHue(), 50, fillColor.lightness() );

        bool isSelected = _model.lock()->getSelectionModel()->isNodeSelected(item);


        // If necessary, draw the original frame range line
        float clipRectCenterY = 0.;
        if ( isSelected && (item->getItemType() == eAnimatedItemTypeReader) ) {
            NodePtr node = item->getInternalNode();

            KnobIntBase *firstFrameKnob = dynamic_cast<KnobIntBase *>( node->getKnobByName(kReaderParamNameFirstFrame).get() );
            assert(firstFrameKnob);

            double speedValue = 1.0f;

            KnobIntBase *originalFrameRangeKnob = dynamic_cast<KnobIntBase *>( node->getKnobByName(kReaderParamNameOriginalFrameRange).get() );
            assert(originalFrameRangeKnob);

            int originalFirstFrame = originalFrameRangeKnob->getValue(DimIdx(0));
            int originalLastFrame = originalFrameRangeKnob->getValue(DimIdx(1));
            int firstFrame = firstFrameKnob->getValue();
            int lineBegin = range.first - (firstFrame - originalFirstFrame);
            int frameCount = originalLastFrame - originalFirstFrame + 1;
            int lineEnd = lineBegin + (frameCount / speedValue);

            clipRectCenterY = (clipRectZoomCoords.y1 + clipRectZoomCoords.y2) / 2.;

            GLProtectAttrib<GL_GPU> aa(GL_CURRENT_BIT | GL_LINE_BIT);
            GL_GPU::glLineWidth(2);

            GL_GPU::glColor4f(fillColor.redF(), fillColor.greenF(), fillColor.blueF(), 1.f);

            GL_GPU::glBegin(GL_LINES);

            //horizontal line
            GL_GPU::glVertex2f(lineBegin, clipRectCenterY);
            GL_GPU::glVertex2f(lineEnd, clipRectCenterY);

            //left end
            GL_GPU::glVertex2d(lineBegin, clipRectZoomCoords.y1);
            GL_GPU::glVertex2d(lineBegin, clipRectZoomCoords.y2);


            //right end
            GL_GPU::glVertex2d(lineEnd, clipRectZoomCoords.y1);
            GL_GPU::glVertex2d(lineEnd, clipRectZoomCoords.y2);

            GL_GPU::glEnd();
        }

        QColor fadedColor;
        fadedColor.setRgbF(fillColor.redF() * 0.5, fillColor.greenF() * 0.5, fillColor.blueF() * 0.5);
        // Fill the range rect


        GL_GPU::glColor4f(fadedColor.redF(), fadedColor.greenF(), fadedColor.blueF(), 1.f);

        GL_GPU::glBegin(GL_POLYGON);
        GL_GPU::glVertex2f( clipRectZoomCoords.left(), clipRectZoomCoords.top() );
        GL_GPU::glVertex2f( clipRectZoomCoords.left(), clipRectZoomCoords.bottom() );
        GL_GPU::glVertex2f( clipRectZoomCoords.right(), clipRectZoomCoords.bottom() );
        GL_GPU::glVertex2f( clipRectZoomCoords.right(), clipRectZoomCoords.top() );
        GL_GPU::glEnd();

        if (isSelected) {
            GL_GPU::glColor4f(fillColor.redF(), fillColor.greenF(), fillColor.blueF(), 1.f);
            GL_GPU::glBegin(GL_LINE_LOOP);
            GL_GPU::glVertex2f( clipRectZoomCoords.left(), clipRectZoomCoords.top() );
            GL_GPU::glVertex2f( clipRectZoomCoords.left(), clipRectZoomCoords.bottom() );
            GL_GPU::glVertex2f( clipRectZoomCoords.right(), clipRectZoomCoords.bottom() );
            GL_GPU::glVertex2f( clipRectZoomCoords.right(), clipRectZoomCoords.top() );
            GL_GPU::glEnd();
        }

        if ( isSelected && (item->getItemType() == eAnimatedItemTypeReader) ) {
            SettingsPtr settings = appPTR->getCurrentSettings();
            double selectionColorRGB[3];
            settings->getSelectionColor(&selectionColorRGB[0], &selectionColorRGB[1], &selectionColorRGB[2]);
            QColor selectionColor;
            selectionColor.setRgbF(selectionColorRGB[0], selectionColorRGB[1], selectionColorRGB[2]);

            QFontMetrics fm(_widget->font());
            int fontHeigt = fm.height();
            QString leftText = QString::number(range.first);
            QString rightText = QString::number(range.second - 1);
            int rightTextW = fm.width(rightText);
            QPointF textLeftPos( zoomCtx.toZoomCoordinates(zoomCtx.toWidgetCoordinates(range.first, 0).x() + 3, 0).x(),
                                 zoomCtx.toZoomCoordinates(0, zoomCtx.toWidgetCoordinates(0, clipRectCenterY).y() + fontHeigt / 2.).y() );

            renderText(textLeftPos.x(), textLeftPos.y(), leftText, selectionColor, _widget->font());

            QPointF textRightPos( zoomCtx.toZoomCoordinates(zoomCtx.toWidgetCoordinates(range.second, 0).x() - rightTextW - 3, 0).x(),
                                  zoomCtx.toZoomCoordinates(0, zoomCtx.toWidgetCoordinates(0, clipRectCenterY).y() + fontHeigt / 2.).y() );

            renderText(textRightPos.x(), textRightPos.y(), rightText, selectionColor, _widget->font());
        }
    }
} // DopeSheetViewPrivate::drawRange


void
DopeSheetViewPrivate::drawKeyframes(QTreeWidgetItem* treeItem, const AnimItemBasePtr &item, DimSpec dimension, ViewSetSpec view) const
{
    running_in_main_thread_and_context(_widget);
    QColor selectionColor;
    {
        double selectionColorRGB[3];
        appPTR->getCurrentSettings()->getSelectionColor(&selectionColorRGB[0], &selectionColorRGB[1], &selectionColorRGB[2]);
        selectionColor.setRgbF(selectionColorRGB[0], selectionColorRGB[1], selectionColorRGB[2]);
    }

    double rowCenterYWidget = treeView->visualItemRect(treeItem).center().y();


    // Draw keyframe in the knob dim row only if it's visible
    bool drawInDimRow = treeView->itemIsVisible(treeItem);

    AnimationModuleBasePtr animModel = _model.lock();
    AnimationModuleSelectionModelPtr selectModel = animModel->getSelectionModel();

    double singleSelectedTime;
    bool hasSingleKfTimeSelected = selectModel->hasSingleKeyFrameTimeSelected(&singleSelectedTime);

    AnimItemDimViewKeyFramesMap dimViewKeys;
    item->getKeyframes(dimension, view, &dimViewKeys);

    for (AnimItemDimViewKeyFramesMap::const_iterator it = dimViewKeys.begin(); it != dimViewKeys.end(); ++it) {

        for (KeyFrameWithStringSet::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {

            const double keyTime = it2->key.getTime();
            RectD zoomKfRect = getKeyFrameBoundingRectZoomCoords(keyTime, rowCenterYWidget);

            bool isKeyFrameSelected = selectModel->isKeyframeSelected(item, it->first.dim, it->first.view, keyTime);

            if (drawInDimRow) {
                DopeSheetViewPrivate::KeyframeTexture texType = kfTextureFromKeyframeType( it2->key.getInterpolation(), isKeyFrameSelected || selectionRect.intersects(zoomKfRect) /*draSelected*/ );

                if (texType != DopeSheetViewPrivate::kfTextureNone) {
                    drawTexturedKeyframe(texType, hasSingleKfTimeSelected && isKeyFrameSelected /*drawKeyTime*/, keyTime, selectionColor, zoomKfRect);
                }
            }
        } // for all keyframes

    } // for all dim/view

} // DopeSheetViewPrivate::drawKeyframes



void
DopeSheetViewPrivate::drawGroupOverlay(const NodeAnimPtr &item,
                                       const NodeAnimPtr &group) const
{
    // Get the overlay color
    double r, g, b;

    item->getNodeGui()->getColor(&r, &g, &b);

    // Compute the area to fill
    int height = treeView->getHeightForItemAndChildren( item->getTreeItem() );
    QRectF nameItemRect = treeView->visualItemRect( item->getTreeItem() );

    assert( nodeRanges.find( group ) != nodeRanges.end() );
    FrameRange groupRange = nodeRanges.find( group )->second; // map::at() is C++11
    RectD overlayRect;
    overlayRect.x1 = groupRange.first;
    overlayRect.x2 = groupRange.second;

    overlayRect.y1 = zoomCtx.toZoomCoordinates(0, nameItemRect.top() + height).y();
    overlayRect.y2 = zoomCtx.toZoomCoordinates( 0, nameItemRect.top() ).y();

    // Perform drawing
    {
        GLProtectAttrib<GL_GPU> a(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT);

        GL_GPU::glColor4f(r, g, b, 0.30f);

        GL_GPU::glBegin(GL_QUADS);
        GL_GPU::glVertex2f( overlayRect.left(), overlayRect.top() );
        GL_GPU::glVertex2f( overlayRect.left(), overlayRect.bottom() );
        GL_GPU::glVertex2f( overlayRect.right(), overlayRect.bottom() );
        GL_GPU::glVertex2f( overlayRect.right(), overlayRect.top() );
        GL_GPU::glEnd();
    }
}

void
DopeSheetViewPrivate::computeSelectionRect(const QPointF &origin,
                                           const QPointF &current)
{
    selectionRect.x1 = std::min( origin.x(), current.x() );
    selectionRect.x2 = std::max( origin.x(), current.x() );
    selectionRect.y1 = std::min( origin.y(), current.y() );
    selectionRect.y2 = std::max( origin.y(), current.y() );
}

void
DopeSheetViewPrivate::computeSelectedKeysBRect()
{
    AnimationModuleBasePtr model = _model.lock();
    const AnimItemDimViewKeyFramesMap& selectedKeyframes = model->getSelectionModel()->getCurrentKeyFramesSelection();
    const std::list<NodeAnimPtr>& selectedNodes = model->getSelectionModel()->getCurrentNodesSelection();
    //const std::list<TableItemAnimPtr>& selectedTableItems = model->getSelectionModel()->getCurrentTableItemsSelection();

    selectedKeysBRect.setupInfinity();

    RectD bbox;
    bool bboxSet = false;
    int nKeysInSelection = 0;
    for (AnimItemDimViewKeyFramesMap::const_iterator it = selectedKeyframes.begin(); it != selectedKeyframes.end(); ++it) {
        for (KeyFrameWithStringSet::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
            ++nKeysInSelection;
            double x = it2->key.getTime();
            double y = it2->key.getValue();
            if (bboxSet) {
                if ( x < bbox.left() ) {
                    bbox.set_left(x);
                }
                if ( x > bbox.right() ) {
                    bbox.set_right(x);
                }
                if ( y > bbox.top() ) {
                    bbox.set_top(y);
                }
                if ( y < bbox.bottom() ) {
                    bbox.set_bottom(y);
                }
            } else {
                bboxSet = true;
                bbox.set_left(x);
                bbox.set_right(x);
                bbox.set_top(y);
                bbox.set_bottom(y);
            }

        }
        
    }

    int nNodesInSelection = 0;
    for (std::list<NodeAnimPtr >::const_iterator it = selectedNodes.begin(); it != selectedNodes.end(); ++it) {
        std::map<NodeAnimPtr, FrameRange >::const_iterator foundRange = nodeRanges.find(*it);
        if ( foundRange == nodeRanges.end() ) {
            continue;
        }

        ++nNodesInSelection;

        const FrameRange& range = foundRange->second;

        //x1,x2 are in zoom coords
        selectedKeysBRect.x1 = std::min(range.first, selectedKeysBRect.x1);
        selectedKeysBRect.x2 = std::max(range.second, selectedKeysBRect.x2);

        QTreeWidgetItem* nodeItem = (*it)->getTreeItem();
        QRect itemRect = treeView->visualItemRect(nodeItem);
        if ( !itemRect.isNull() && !itemRect.isEmpty() ) {
            double y = itemRect.center().y();

            //y1,y2 are in widget coords
            selectedKeysBRect.y1 = std::min(y, selectedKeysBRect.y1);
            selectedKeysBRect.y2 = std::max(y, selectedKeysBRect.y2);
        }
    }

    QTreeWidgetItem *bottomMostItem = treeView->itemAt(0, selectedKeysBRect.y2);
    double bottom = treeView->visualItemRect(bottomMostItem).bottom();
    bottom = zoomCtx.toZoomCoordinates(0, bottom).y();

    QTreeWidgetItem *topMostItem = treeView->itemAt(0, selectedKeysBRect.y1);
    double top = treeView->visualItemRect(topMostItem).top();
    top = zoomCtx.toZoomCoordinates(0, top).y();

    selectedKeysBRect.y1 = bottom;
    selectedKeysBRect.y2 = top;

    if ( !selectedKeysBRect.isNull() ) {
        /// Adjust the bounding rect of the size of a keyframe
        double leftWidget = zoomCtx.toWidgetCoordinates(selectedKeysBRect.x1, 0).x();
        double leftAdjustedZoom = zoomCtx.toZoomCoordinates(leftWidget - KF_X_OFFSET, 0).x();
        double xAdjustOffset = (selectedKeysBRect.x1 - leftAdjustedZoom);

        selectedKeysBRect.x1 -= xAdjustOffset;
        selectedKeysBRect.x2 += xAdjustOffset;
    }
} // DopeSheetViewPrivate::computeSelectedKeysBRect

void
DopeSheetViewPrivate::computeRangesBelow(const NodeAnimPtr& node)
{
    std::vector<NodeAnimPtr> allNodes;
    _model.lock()->getTopLevelNodes(&allNodes);

    double thisNodeY = treeView->visualItemRect( node->getTreeItem() ).y();
    for (std::vector<NodeAnimPtr>::const_iterator it = allNodes.begin(); it != allNodes.end(); ++it) {
        if ( treeView->visualItemRect((*it)->getTreeItem()).y() >=  thisNodeY) {
            computeNodeRange(*it);
        }
    }
}

void
DopeSheetViewPrivate::computeNodeRange(const NodeAnimPtr& node)
{
    AnimatedItemTypeEnum nodeType = node->getItemType();

    switch (nodeType) {
        case eAnimatedItemTypeReader:
            computeReaderRange(node);
            break;
        case eAnimatedItemTypeRetime:
            computeRetimeRange(node);
            break;
        case eAnimatedItemTypeTimeOffset:
            computeTimeOffsetRange(node);
            break;
        case eAnimatedItemTypeFrameRange:
            computeFRRange(node);
            break;
        case eAnimatedItemTypeGroup:
            computeGroupRange(node);
            break;
        case eAnimatedItemTypeCommon:
            computeCommonNodeRange(node);
            break;
        default:
            break;
    }
}

void
DopeSheetViewPrivate::computeReaderRange(const NodeAnimPtr& reader)
{
    if ( std::find(nodeRangesBeingComputed.begin(), nodeRangesBeingComputed.end(), reader) != nodeRangesBeingComputed.end() ) {
        return;
    }

    nodeRangesBeingComputed.push_back(reader);
    ++rangeComputationRecursion;

    NodePtr node = reader->getInternalNode();

    KnobIntBase *startingTimeKnob = dynamic_cast<KnobIntBase *>( node->getKnobByName(kReaderParamNameStartingTime).get() );
    if (!startingTimeKnob) {
        return;
    }
    KnobIntBase *firstFrameKnob = dynamic_cast<KnobIntBase *>( node->getKnobByName(kReaderParamNameFirstFrame).get() );
    if (!firstFrameKnob) {
        return;
    }
    KnobIntBase *lastFrameKnob = dynamic_cast<KnobIntBase *>( node->getKnobByName(kReaderParamNameLastFrame).get() );
    if (!lastFrameKnob) {
        return;
    }

    int startingTimeValue = startingTimeKnob->getValue();
    int firstFrameValue = firstFrameKnob->getValue();
    int lastFrameValue = lastFrameKnob->getValue();
    FrameRange range(startingTimeValue,
                     startingTimeValue + (lastFrameValue - firstFrameValue) + 1);

    nodeRanges[reader] = range;

    AnimationModulePtr model = toAnimationModule(_model.lock());

    {
        NodeAnimPtr isInGroup = model->getGroupNodeAnim(reader);
        if (isInGroup) {
            computeGroupRange( isInGroup );
        }
    }
    {
        NodeAnimPtr isConnectedToTimeNode = model->getNearestTimeNodeFromOutputs(reader);
        if (isConnectedToTimeNode) {
            computeNodeRange( isConnectedToTimeNode );
        }
    }

    --rangeComputationRecursion;
    std::list<NodeAnimPtr>::iterator found = std::find(nodeRangesBeingComputed.begin(), nodeRangesBeingComputed.end(), reader);
    assert( found != nodeRangesBeingComputed.end() );
    nodeRangesBeingComputed.erase(found);
} // DopeSheetViewPrivate::computeReaderRange

void
DopeSheetViewPrivate::computeRetimeRange(const NodeAnimPtr& retimer)
{
    if ( std::find(nodeRangesBeingComputed.begin(), nodeRangesBeingComputed.end(), retimer) != nodeRangesBeingComputed.end() ) {
        return;
    }

    nodeRangesBeingComputed.push_back(retimer);
    ++rangeComputationRecursion;

    NodePtr node = retimer->getInternalNode();
    if (!node) {
        return;
    }
    NodePtr input = node->getInput(0);
    if (input) {
        double inputFirst, inputLast;
        input->getEffectInstance()->getFrameRange_public(0, &inputFirst, &inputLast);

        U64 hash;
        FramesNeededMap framesFirst = node->getEffectInstance()->getFramesNeeded_public(inputFirst, ViewIdx(0), &hash);
        FramesNeededMap framesLast = node->getEffectInstance()->getFramesNeeded_public(inputLast, ViewIdx(0), &hash);
        assert( !framesFirst.empty() && !framesLast.empty() );
        if ( framesFirst.empty() || framesLast.empty() ) {
            return;
        }

        FrameRange range;
        {
            const FrameRangesMap& rangeFirst = framesFirst[0];
            assert( !rangeFirst.empty() );
            if ( rangeFirst.empty() ) {
                return;
            }
            FrameRangesMap::const_iterator it = rangeFirst.find( ViewIdx(0) );
            assert( it != rangeFirst.end() );
            if ( it == rangeFirst.end() ) {
                return;
            }
            const std::vector<OfxRangeD>& frames = it->second;
            assert( !frames.empty() );
            if ( frames.empty() ) {
                return;
            }
            range.first = (frames.front().min);
        }
        {
            FrameRangesMap& rangeLast = framesLast[0];
            assert( !rangeLast.empty() );
            if ( rangeLast.empty() ) {
                return;
            }
            FrameRangesMap::const_iterator it = rangeLast.find( ViewIdx(0) );
            assert( it != rangeLast.end() );
            if ( it == rangeLast.end() ) {
                return;
            }
            const std::vector<OfxRangeD>& frames = it->second;
            assert( !frames.empty() );
            if ( frames.empty() ) {
                return;
            }
            range.second = (frames.front().min);
        }

        nodeRanges[retimer] = range;
    } else {
        nodeRanges[retimer] = FrameRange();
    }

    --rangeComputationRecursion;
    std::list<NodeAnimPtr>::iterator found = std::find(nodeRangesBeingComputed.begin(), nodeRangesBeingComputed.end(), retimer);
    assert( found != nodeRangesBeingComputed.end() );
    if ( found != nodeRangesBeingComputed.end() ) {
        nodeRangesBeingComputed.erase(found);
    }
} // DopeSheetViewPrivate::computeRetimeRange

void
DopeSheetViewPrivate::computeTimeOffsetRange(const NodeAnimPtr& timeOffset)
{
    if ( std::find(nodeRangesBeingComputed.begin(), nodeRangesBeingComputed.end(), timeOffset) != nodeRangesBeingComputed.end() ) {
        return;
    }

    nodeRangesBeingComputed.push_back(timeOffset);
    ++rangeComputationRecursion;

    FrameRange range(0, 0);

    // Retrieve nearest reader useful values
    {
        AnimationModulePtr model = toAnimationModule(_model.lock());

        NodeAnimPtr nearestReader = model->findNodeAnim( model->getNearestReader(timeOffset) );
        if (nearestReader) {
            assert( nodeRanges.find( nearestReader ) != nodeRanges.end() );
            FrameRange nearestReaderRange = nodeRanges.find( nearestReader )->second; // map::at() is C++11

            // Retrieve the time offset values
            KnobIntBasePtr timeOffsetKnob = toKnobIntBase( timeOffset->getInternalNode()->getKnobByName(kReaderParamNameTimeOffset) );
            assert(timeOffsetKnob);

            int timeOffsetValue = timeOffsetKnob->getValue();

            range.first = nearestReaderRange.first + timeOffsetValue;
            range.second = nearestReaderRange.second + timeOffsetValue;
        }
    }

    nodeRanges[timeOffset] = range;

    --rangeComputationRecursion;
    std::list<NodeAnimPtr>::iterator found = std::find(nodeRangesBeingComputed.begin(), nodeRangesBeingComputed.end(), timeOffset);
    assert( found != nodeRangesBeingComputed.end() );
    nodeRangesBeingComputed.erase(found);
}

void
DopeSheetViewPrivate::computeCommonNodeRange(const NodeAnimPtr& node)
{
    if ( std::find(nodeRangesBeingComputed.begin(), nodeRangesBeingComputed.end(), node) != nodeRangesBeingComputed.end() ) {
        return;
    }
    nodeRangesBeingComputed.push_back(node);
    ++rangeComputationRecursion;


    FrameRange range;
    int first,last;
    bool lifetimeEnabled = node->getNodeGui()->getNode()->isLifetimeActivated(&first, &last);

    if (lifetimeEnabled) {
        range.first = first;
        range.second = last;
        nodeRanges[node] = range;
    }

    --rangeComputationRecursion;
    std::list<NodeAnimPtr>::iterator found = std::find(nodeRangesBeingComputed.begin(), nodeRangesBeingComputed.end(), node);
    assert( found != nodeRangesBeingComputed.end() );
    nodeRangesBeingComputed.erase(found);
}

void
DopeSheetViewPrivate::computeFRRange(const NodeAnimPtr& frameRange)
{
    if ( std::find(nodeRangesBeingComputed.begin(), nodeRangesBeingComputed.end(), frameRange) != nodeRangesBeingComputed.end() ) {
        return;
    }

    nodeRangesBeingComputed.push_back(frameRange);
    ++rangeComputationRecursion;

    NodePtr node = frameRange->getInternalNode();

    KnobIntBase *frameRangeKnob = dynamic_cast<KnobIntBase *>( node->getKnobByName("frameRange").get() );
    assert(frameRangeKnob);

    FrameRange range;
    range.first = frameRangeKnob->getValue(DimIdx(0));
    range.second = frameRangeKnob->getValue(DimIdx(1));

    nodeRanges[frameRange] = range;

    --rangeComputationRecursion;
    std::list<NodeAnimPtr>::iterator found = std::find(nodeRangesBeingComputed.begin(), nodeRangesBeingComputed.end(), frameRange);
    assert( found != nodeRangesBeingComputed.end() );
    nodeRangesBeingComputed.erase(found);
}

void
DopeSheetViewPrivate::computeGroupRange(const NodeAnimPtr& group)
{
    if ( std::find(nodeRangesBeingComputed.begin(), nodeRangesBeingComputed.end(), group) != nodeRangesBeingComputed.end() ) {
        return;
    }

    NodePtr node = group->getInternalNode();
    if (!node) {
        return;
    }

    AnimationModulePtr model = toAnimationModule(_model.lock());

    FrameRange range;
    std::set<double> times;
    NodeGroupPtr nodegroup = node->isEffectNodeGroup();
    assert(nodegroup);
    if (!nodegroup) {
        throw std::logic_error("DopeSheetViewPrivate::computeGroupRange: node is not a group");
    }

    nodeRangesBeingComputed.push_back(group);
    ++rangeComputationRecursion;


    NodesList nodes = nodegroup->getNodes();

    for (NodesList::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        NodePtr node = (*it);
        NodeAnimPtr NodeAnim = model->findNodeAnim(node);

        if (!NodeAnim) {
            continue;
        }

        NodeGuiPtr nodeGui = boost::dynamic_pointer_cast<NodeGui>( node->getNodeGui() );

        if ( !nodeGui->getSettingPanel() || !nodeGui->isSettingsPanelVisible() ) {
            continue;
        }

        computeNodeRange(NodeAnim);

        std::map<NodeAnimPtr, FrameRange >::iterator found = nodeRanges.find(NodeAnim);
        if ( found != nodeRanges.end() ) {
            times.insert(found->second.first);
            times.insert(found->second.second);
        }

        const std::list<std::pair<KnobIWPtr, KnobGuiPtr> > &knobs = nodeGui->getKnobs();

        for (std::list<std::pair<KnobIWPtr, KnobGuiPtr> >::const_iterator it = knobs.begin();
             it != knobs.end();
             ++it) {
            const KnobGuiPtr& knobGui = (*it).second;
            KnobIPtr knob = knobGui->getKnob();

            if ( !knob->isAnimationEnabled() || !knob->hasAnimation() ) {
                continue;
            } else {
                for (int i = 0; i < knob->getDimension(); ++i) {
                    KeyFrameSet keyframes = knob->getCurve(ViewIdx(0), i)->getKeyFrames_mt_safe();

                    if ( keyframes.empty() ) {
                        continue;
                    }

                    times.insert( keyframes.begin()->getTime() );
                    times.insert( keyframes.rbegin()->getTime() );
                }
            }
        }
    }

    if (times.size() <= 1) {
        range.first = 0;
        range.second = 0;
    } else {
        range.first = *times.begin();
        range.second = *times.rbegin();
    }

    nodeRanges[group] = range;

    --rangeComputationRecursion;
    std::list<NodeAnimPtr>::iterator found = std::find(nodeRangesBeingComputed.begin(), nodeRangesBeingComputed.end(), group);
    assert( found != nodeRangesBeingComputed.end() );
    nodeRangesBeingComputed.erase(found);
} // DopeSheetViewPrivate::computeGroupRange

void
DopeSheetViewPrivate::onMouseLeftButtonDrag(QMouseEvent *e)
{
    QPointF mouseZoomCoords = zoomCtx.toZoomCoordinates( e->x(), e->y() );
    QPointF lastZoomCoordsOnMousePress = zoomCtx.toZoomCoordinates( lastPosOnMousePress.x(),
                                                                        lastPosOnMousePress.y() );
    QPointF lastZoomCoordsOnMouseMove = zoomCtx.toZoomCoordinates( lastPosOnMouseMove.x(),
                                                                       lastPosOnMouseMove.y() );
    double currentTime = mouseZoomCoords.x();
    double dt = clampedMouseOffset(lastZoomCoordsOnMousePress.x(), currentTime);
    double dv = mouseZoomCoords.y() - lastZoomCoordsOnMouseMove.y();

    switch (eventState) {
    case DopeSheetView::esMoveKeyframeSelection: {
        if ( (dt >= 1.0f) || (dt <= -1.0f) ) {
            if (_gui) {
                _gui->setDraftRenderEnabled(true);
            }
            _model.lock()->moveSelectedKeysAndNodes(dt, 0);
        }

        break;
    }
    case DopeSheetView::esTransformingKeyframesMiddleLeft:
    case DopeSheetView::esTransformingKeyframesMiddleRight: {
        if (_gui) {
            _gui->setDraftRenderEnabled(true);
        }
        bool shiftHeld = modCASIsShift(e);
        QPointF center;
        if (!shiftHeld) {
            if (eventState == DopeSheetView::esTransformingKeyframesMiddleLeft) {
                center.rx() = selectedKeysBRect.x2;
            } else {
                center.rx() = selectedKeysBRect.x1;
            }
            center.ry() = (selectedKeysBRect.y1 + selectedKeysBRect.y2) / 2.;
        } else {
            center = QPointF( (selectedKeysBRect.x1 + selectedKeysBRect.x2) / 2., (selectedKeysBRect.y1 + selectedKeysBRect.y2) / 2. );
        }


        double sx = 1., sy = 1.;
        double tx = 0., ty = 0.;
        double oldX = mouseZoomCoords.x() - dt;
        double oldY = mouseZoomCoords.y() - dv;
        // the scale ratio is the ratio of distances to the center
        double prevDist = ( oldX - center.x() ) * ( oldX - center.x() ) + ( oldY - center.y() ) * ( oldY - center.y() );
        if (prevDist != 0) {
            double dist = ( mouseZoomCoords.x() - center.x() ) * ( mouseZoomCoords.x() - center.x() ) + ( mouseZoomCoords.y() - center.y() ) * ( mouseZoomCoords.y() - center.y() );
            double ratio = std::sqrt(dist / prevDist);
            sx *= ratio;
        }

        Transform::Matrix3x3 transform = Transform::matTransformCanonical( tx, ty, sx, sy, 0, 0, true, 0, center.x(), center.y() );
        _model.lock()->transformSelectedKeys(transform);
        break;
    }
    case DopeSheetView::esMoveCurrentFrameIndicator: {
        if ( (dt >= 1.0f) || (dt <= -1.0f) ) {
            if (_gui) {
                _gui->setDraftRenderEnabled(true);
            }
            moveCurrentFrameIndicator(dt);
        }

        break;
    }
    case DopeSheetView::esSelectionByRect: {
        computeSelectionRect(lastZoomCoordsOnMousePress, mouseZoomCoords);

        _widget->redraw();

        break;
    }
    case DopeSheetView::esReaderLeftTrim: {
        if ( (dt >= 1.0f) || (dt <= -1.0f) ) {
            if (_gui) {
                _gui->setDraftRenderEnabled(true);
            }
            KnobIntBase *timeOffsetKnob = dynamic_cast<KnobIntBase *>( currentEditedReader->getInternalNode()->getKnobByName(kReaderParamNameTimeOffset).get() );
            assert(timeOffsetKnob);

            double newFirstFrame = std::floor(currentTime - timeOffsetKnob->getValue() + 0.5);

            _model.lock()->trimReaderLeft(currentEditedReader, newFirstFrame);
        }

        break;
    }
    case DopeSheetView::esReaderRightTrim: {
        if ( (dt >= 1.0f) || (dt <= -1.0f) ) {
            if (_gui) {
                _gui->setDraftRenderEnabled(true);
            }
            KnobIntBase *timeOffsetKnob = dynamic_cast<KnobIntBase *>( currentEditedReader->getInternalNode()->getKnobByName(kReaderParamNameTimeOffset).get() );
            assert(timeOffsetKnob);

            double newLastFrame = std::floor(currentTime - timeOffsetKnob->getValue() + 0.5);

            _model.lock()->trimReaderRight(currentEditedReader, newLastFrame);
        }

        break;
    }
    case DopeSheetView::esReaderSlip: {
        if ( (dt >= 1.0f) || (dt <= -1.0f) ) {
            if (_gui) {
                _gui->setDraftRenderEnabled(true);
            }
            _model.lock()->slipReader(currentEditedReader, dt);
        }

        break;
    }
    case DopeSheetView::esNoEditingState:
        eventState = DopeSheetView::esSelectionByRect;

        break;
    default:
        break;
    } // switch
} // DopeSheetViewPrivate::onMouseLeftButtonDrag

void
DopeSheetViewPrivate::createSelectionFromRect(const RectD &canonicalRect,
                                              AnimItemDimViewKeyFramesMap *keys,
                                              std::vector<NodeAnimPtr >* nodes,
                                              std::vector<TableItemAnimPtr >* tableItems)
{

    AnimationModuleSelectionModelPtr selectModel = _model.lock()->getSelectionModel();
    const AnimItemDimViewKeyFramesMap& selectedKeys = selectModel->getCurrentKeyFramesSelection();

    for (AnimItemDimViewKeyFramesMap::const_iterator it = selectedKeys.begin(); it != selectedKeys.end(); ++it) {
        CurveGuiPtr guiCurve = it->first.item->getCurveGui(it->first.dim, it->first.view);
        if (!guiCurve) {
            continue;
        }

        KeyFrameSet set = guiCurve->getKeyFrames();
        if ( set.empty() ) {
            continue;
        }

        StringAnimationManagerPtr stringAnim = it->first.item->getInternalAnimItem()->getStringAnimation();


        KeyFrameWithStringSet& outKeys = (*keys)[it->first];

        for ( KeyFrameSet::const_iterator it2 = set.begin(); it2 != set.end(); ++it2) {
            double y = it2->getValue();
            double x = it2->getTime();
            if ( (x <= canonicalRect.x2) && (x >= canonicalRect.x1) && (y <= canonicalRect.y2) && (y >= canonicalRect.y1) ) {
                //KeyPtr newSelectedKey( new SelectedKey(*it, *it2, hasPrev, prevKey, hasNext, nextKey) );
                KeyFrameWithString k;
                k.key = *it2;
                if (stringAnim) {
                    stringAnim->stringFromInterpolatedIndex(it2->getValue(), it->first.view, &k.string);
                }
                outKeys.insert(k);
            }
        }

    }

    const std::list<NodeAnimPtr>& selectedNodes = selectModel->getCurrentNodesSelection();

    for (std::list<NodeAnimPtr>::const_iterator it = selectedNodes.begin(); it != selectedNodes.end(); ++it) {
        std::map<NodeAnimPtr, FrameRange >::const_iterator foundRange = nodeRanges.find(*it);
        if ( foundRange == nodeRanges.end() ) {
            continue;
        }
        QPoint visualRectCenter = treeView->visualItemRect( (*it)->getTreeItem() ).center();
        QPointF center = zoomCtx.toZoomCoordinates( visualRectCenter.x(), visualRectCenter.y() );

        if ( canonicalRect.contains( (foundRange->second.first + foundRange->second.second) / 2., center.y() ) ) {
            nodes->push_back(*it);
        }

    }
}

void
DopeSheetViewPrivate::moveCurrentFrameIndicator(double dt)
{
    AnimationModuleBasePtr animModel = _model.lock();
    if (!animModel) {
        return;
    }
    TimeLinePtr timeline = animModel->getTimeline();
    if (!timeline) {
        return;
    }
    if (!_gui) {
        return;
    }
    _gui->getApp()->setLastViewerUsingTimeline( NodePtr() );

    double toTime = animModel->getTimeline()->currentFrame() + dt;

    _gui->setDraftRenderEnabled(true);
    timeline->seekFrame(SequenceTime(toTime), false, OutputEffectInstancePtr(), eTimelineChangeReasonDopeSheetEditorSeek);
}
void
DopeSheetViewPrivate::updateCurveWidgetFrameRange()
{
    AnimationModulePtr animModel = toAnimationModule(_model.lock());
    if (!animModel) {
        return;
    }
    CurveWidget *curveWidget = animModel->getEditor()->getCurveWidget();
    curveWidget->centerOn( zoomCtx.left(), zoomCtx.right() );
}



std::pair<double, double> DopeSheetView::getKeyframeRange() const
{

    std::pair<double, double> ret = std::make_pair(0, 0);

    AnimItemDimViewKeyFramesMap selectedKeyframes;
    std::vector<NodeAnimPtr > selectedNodes;
    std::vector<TableItemAnimPtr> selectedTableItems;
    _imp->_model.lock()->getSelectionModel()->getAllKeyFrames(&selectedKeyframes, &selectedNodes, &selectedTableItems);

    bool rangeSet = false;
    for (AnimItemDimViewKeyFramesMap::const_iterator it = selectedKeyframes.begin(); it != selectedKeyframes.end(); ++it) {
        if (it->second.empty()) {
            continue;
        }
        if (!rangeSet) {
            ret.first = it->second.begin()->key.getTime();
            ret.second = it->second.rbegin()->key.getTime();
            rangeSet = true;
        } else {
            ret.first = std::min(ret.first,it->second.begin()->key.getTime());
            ret.first = std::max(ret.first,it->second.begin()->key.getTime());
        }
    }
    return ret;
} // DopeSheetView::getKeyframeRange

void
DopeSheetView::getBackgroundColour(double &r,
                                   double &g,
                                   double &b) const
{
    running_in_main_thread();

    // use the same settings as the curve editor
    appPTR->getCurrentSettings()->getCurveEditorBGColor(&r, &g, &b);
}

void
DopeSheetView::centerOnAllItems()
{
    centerOnRangeInternal(getKeyframeRange());
}

void
DopeSheetView::centerOnRangeInternal(const std::pair<double, double>& range)
{

    if (range.first == range.second) {
        return;
    }

    std::pair<double, double> finalRange = range;
    double actualRange = (range.second - range.first);
    if (actualRange < NATRON_DOPESHEET_MIN_RANGE_FIT) {
        double diffRange = NATRON_DOPESHEET_MIN_RANGE_FIT - actualRange;
        diffRange /= 2;
        finalRange.first -= diffRange;
        finalRange.second += diffRange;
    }


    _imp->zoomCtx.fill( finalRange.first, finalRange.second,
                       _imp->zoomCtx.bottom(), _imp->zoomCtx.top() );

    redraw();

}

void
DopeSheetView::centerOnSelection()
{
    running_in_main_thread();

    int selectedKeyframesCount = _imp->_model.lock()->getSelectionModel()->getSelectedKeyframesCount();


    FrameRange range;

    // frame on project bounds
    if (selectedKeyframesCount <= 1) {
        range = getKeyframeRange();
    }  else {
        // or frame on current selection
        range.first = _imp->selectedKeysBRect.left();
        range.second = _imp->selectedKeysBRect.right();
    }
    centerOnRangeInternal(range);
}

void
DopeSheetView::onNodeAdded(const NodeAnimPtr& NodeAnim)
{
    AnimatedItemTypeEnum nodeType = NodeAnim->getItemType();
    NodePtr node = NodeAnim->getInternalNode();
    bool mustComputeNodeRange = true;

    if (nodeType == eAnimatedItemTypeCommon) {
        if ( _imp->model->isPartOfGroup(NodeAnim) ) {
            const std::list<std::pair<KnobIWPtr, KnobGuiPtr> > &knobs = NodeAnim->getNodeGui()->getKnobs();

            for (std::list<std::pair<KnobIWPtr, KnobGuiPtr> >::const_iterator knobIt = knobs.begin(); knobIt != knobs.end(); ++knobIt) {
                KnobIPtr knob = knobIt->first.lock();
                const KnobGuiPtr& knobGui = knobIt->second;
                connect( knob->getSignalSlotHandler().get(), SIGNAL(keyFrameMoved(ViewSpec,int,double,double)),
                         this, SLOT(onKeyframeChanged()) );

                connect( knobGui.get(), SIGNAL(keyFrameSet()),
                         this, SLOT(onKeyframeChanged()) );

                connect( knobGui.get(), SIGNAL(keyFrameRemoved()),
                         this, SLOT(onKeyframeChanged()) );
            }
        }

        mustComputeNodeRange = false;
    } else if (nodeType == eAnimatedItemTypeReader) {
        // The dopesheet view must refresh if the user set some values in the settings panel
        // so we connect some signals/slots
        KnobIPtr lastFrameKnob = node->getKnobByName(kReaderParamNameLastFrame);
        if (!lastFrameKnob) {
            return;
        }
        boost::shared_ptr<KnobSignalSlotHandler> lastFrameKnobHandler =  lastFrameKnob->getSignalSlotHandler();
        assert(lastFrameKnob);
        boost::shared_ptr<KnobSignalSlotHandler> startingTimeKnob = node->getKnobByName(kReaderParamNameStartingTime)->getSignalSlotHandler();
        assert(startingTimeKnob);

        connect( lastFrameKnobHandler.get(), SIGNAL(valueChanged(ViewSpec,int,int)),
                 this, SLOT(onRangeNodeChanged(ViewSpec,int,int)) );

        connect( startingTimeKnob.get(), SIGNAL(valueChanged(ViewSpec,int,int)),
                 this, SLOT(onRangeNodeChanged(ViewSpec,int,int)) );

        // We don't make the connection for the first frame knob, because the
        // starting time is updated when it's modified. Thus we avoid two
        // refreshes of the view.
    } else if (nodeType == eAnimatedItemTypeRetime) {
        boost::shared_ptr<KnobSignalSlotHandler> speedKnob =  node->getKnobByName(kRetimeParamNameSpeed)->getSignalSlotHandler();
        assert(speedKnob);

        connect( speedKnob.get(), SIGNAL(valueChanged(ViewSpec,int,int)),
                 this, SLOT(onRangeNodeChanged(ViewSpec,int,int)) );
    } else if (nodeType == eAnimatedItemTypeTimeOffset) {
        boost::shared_ptr<KnobSignalSlotHandler> timeOffsetKnob =  node->getKnobByName(kReaderParamNameTimeOffset)->getSignalSlotHandler();
        assert(timeOffsetKnob);

        connect( timeOffsetKnob.get(), SIGNAL(valueChanged(ViewSpec,int,int)),
                 this, SLOT(onRangeNodeChanged(ViewSpec,int,int)) );
    } else if (nodeType == eAnimatedItemTypeFrameRange) {
        boost::shared_ptr<KnobSignalSlotHandler> frameRangeKnob =  node->getKnobByName(kFrameRangeParamNameFrameRange)->getSignalSlotHandler();
        assert(frameRangeKnob);

        connect( frameRangeKnob.get(), SIGNAL(valueChanged(ViewSpec,int,int)),
                 this, SLOT(onRangeNodeChanged(ViewSpec,int,int)) );
    }

    if (mustComputeNodeRange) {
        _imp->computeNodeRange(NodeAnim);
    }

    {
        NodeAnimPtr parentGroupNodeAnim = _imp->model->getGroupNodeAnim(NodeAnim);
        if (parentGroupNodeAnim) {
            _imp->computeGroupRange( parentGroupNodeAnim );
        }
    }
} // DopeSheetView::onNodeAdded

void
DopeSheetView::onNodeAboutToBeRemoved(const NodeAnimPtr& NodeAnim)
{
    {
        NodeAnimPtr parentGroupNodeAnim = _imp->model->getGroupNodeAnim(NodeAnim);
        if (parentGroupNodeAnim) {
            _imp->computeGroupRange( parentGroupNodeAnim );
        }
    }

    std::map<NodeAnimPtr, FrameRange>::iterator toRemove = _imp->nodeRanges.find(NodeAnim);

    if ( toRemove != _imp->nodeRanges.end() ) {
        _imp->nodeRanges.erase(toRemove);
    }

    _imp->computeSelectedKeysBRect();

    redraw();
}

void
DopeSheetView::onKeyframeChanged()
{
    QObject *signalSender = sender();
    NodeAnimPtr NodeAnim;

    {
        KnobSignalSlotHandler *knobHandler = qobject_cast<KnobSignalSlotHandler *>(signalSender);
        if (knobHandler) {
            NodeAnim = _imp->model->findNodeAnim( knobHandler->getKnob() );
        } else {
            KnobGui *knobGui = qobject_cast<KnobGui *>(signalSender);
            if (knobGui) {
                NodeAnim = _imp->model->findNodeAnim( knobGui->getKnob() );
            }
        }
    }

    if (!NodeAnim) {
        return;
    }

    {
        NodeAnimPtr parentGroupNodeAnim = _imp->model->getGroupNodeAnim( NodeAnim );
        if (parentGroupNodeAnim) {
            _imp->computeGroupRange( parentGroupNodeAnim );
        }
    }
}

void
DopeSheetView::onRangeNodeChanged(ViewSpec /*view*/,
                                  int /*dimension*/,
                                  int /*reason*/)
{
    QObject *signalSender = sender();
    NodeAnimPtr NodeAnim;

    {
        KnobSignalSlotHandler *knobHandler = qobject_cast<KnobSignalSlotHandler *>(signalSender);
        if (knobHandler) {
            KnobHolderPtr holder = knobHandler->getKnob()->getHolder();
            EffectInstancePtr effectInstance = toEffectInstance(holder);
            assert(effectInstance);
            if (!effectInstance) {
                return;
            }
            NodeAnim = _imp->model->findNodeAnim( effectInstance->getNode() );
        }
    }

    if (!NodeAnim) {
        return;
    }
    _imp->computeNodeRange( NodeAnim );

    //Since this function is called a lot, let a chance to Qt to concatenate events
    //NB: updateGL() does not concatenate
    update();
}

void
DopeSheetView::onHierarchyViewItemExpandedOrCollapsed(QTreeWidgetItem *item)
{
    // Compute the range rects of affected items
    {
        NodeAnimPtr NodeAnim = _imp->model->findParentNodeAnim(item);
        if (NodeAnim) {
            _imp->computeRangesBelow( NodeAnim );
        }
    }

    _imp->computeSelectedKeysBRect();

    redraw();
}

void
DopeSheetView::onHierarchyViewScrollbarMoved(int /*value*/)
{
    _imp->computeSelectedKeysBRect();

    redraw();
}

void
DopeSheetView::refreshSelectionBboxAndRedraw()
{
    _imp->computeSelectedKeysBRect();
    redraw();
}

void
DopeSheetView::onKeyframeSelectionChanged()
{
    refreshSelectionBboxAndRedraw();
}

void
DopeSheetView::initializeGL()
{
    _imp->initializeGLCommon();
}

/**
 * @brief DopeSheetView::resizeGL
 *
 *
 */
void
DopeSheetView::resizeGL(int w,
                        int h)
{
    running_in_main_thread_and_context(this);

    if ( !appPTR->isOpenGLLoaded() ) {
        return;
    }

    if (h == 0) {
        h = 1;
    }

    GL_GPU::glViewport(0, 0, w, h);

    {
        QMutexLocker k(&_imp->zoomCtxMutex);
        _imp->zoomCtx.setScreenSize(w, h);
    }

    // Don't do the following when the height of the widget is irrelevant
    if (h == 1) {
        return;
    }

    // Find out what are the selected keyframes and center on them
    if (!_imp->zoomOrPannedSinceLastFit) {
        centerOnSelection();
    }
}

bool
DopeSheetView::hasDrawnOnce() const
{
    return _imp->drawnOnce;
}

/**
 * @brief DopeSheetView::paintGL
 */
void
DopeSheetView::paintGL()
{
    running_in_main_thread_and_context(this);

    if ( !appPTR->isOpenGLLoaded() ) {
        return;
    }


    glCheckError(GL_GPU);

    if (_imp->zoomCtx.factor() <= 0) {
        return;
    }

    _imp->drawnOnce = true;
    
    double zoomLeft, zoomRight, zoomBottom, zoomTop;
    zoomLeft = _imp->zoomCtx.left();
    zoomRight = _imp->zoomCtx.right();
    zoomBottom = _imp->zoomCtx.bottom();
    zoomTop = _imp->zoomCtx.top();

    // Retrieve the appropriate settings for drawing
    SettingsPtr settings = appPTR->getCurrentSettings();
    double bgR, bgG, bgB;
    settings->getAnimationModuleEditorBackgroundColor(&bgR, &bgG, &bgB);

    if ( (zoomLeft == zoomRight) || (zoomTop == zoomBottom) ) {
        GL_GPU::glClearColor(bgR, bgG, bgB, 1.);
        GL_GPU::glClear(GL_COLOR_BUFFER_BIT);

        return;
    }

    {
        GLProtectAttrib<GL_GPU> a(GL_TRANSFORM_BIT | GL_COLOR_BUFFER_BIT);
        GLProtectMatrix<GL_GPU> p(GL_PROJECTION);

        GL_GPU::glLoadIdentity();
        GL_GPU::glOrtho(zoomLeft, zoomRight, zoomBottom, zoomTop, 1, -1);

        GLProtectMatrix<GL_GPU> m(GL_MODELVIEW);

        GL_GPU::glLoadIdentity();

        glCheckError(GL_GPU);

        GL_GPU::glClearColor(bgR, bgG, bgB, 1.);
        GL_GPU::glClear(GL_COLOR_BUFFER_BIT);

        _imp->drawScale();
        _imp->drawProjectBounds();
        _imp->drawRows();

        if (_imp->eventState == DopeSheetView::esSelectionByRect) {
            _imp->drawSelectionRect();
        }

        if ( !_imp->selectedKeysBRect.isNull() ) {
            _imp->drawSelectedKeysBRect();
        }

        _imp->drawCurrentFrameIndicator();
    }
    glCheckError(GL_GPU);
} // DopeSheetView::paintGL

void
DopeSheetView::mousePressEvent(QMouseEvent *e)
{
    running_in_main_thread();

    _imp->model->getEditor()->onInputEventCalled();

    bool didSomething = false;

    if ( buttonDownIsRight(e) ) {
        _imp->createContextMenu();
        _imp->contextMenu->exec( mapToGlobal( e->pos() ) );

        e->accept();

        //didSomething = true;
        return;
    }

    if ( buttonDownIsMiddle(e) ) {
        _imp->eventState = DopeSheetView::esDraggingView;
        didSomething = true;
    } else if ( ( (e->buttons() & Qt::MiddleButton) &&
                  ( ( buttonMetaAlt(e) == Qt::AltModifier) || (e->buttons() & Qt::LeftButton) ) ) ||
                ( (e->buttons() & Qt::LeftButton) &&
                  ( buttonMetaAlt(e) == (Qt::AltModifier | Qt::MetaModifier) ) ) ) {
        // Alt + middle or Left + middle or Crtl + Alt + Left = zoom
        _imp->eventState = esZoomingView;
        _imp->lastPosOnMousePress = e->pos();
        _imp->lastPosOnMouseMove = e->pos();

        return;
    }

    QPointF clickZoomCoords = _imp->zoomCtx.toZoomCoordinates( e->x(), e->y() );

    if ( buttonDownIsLeft(e) ) {
        if ( !_imp->selectedKeysBRect.isNull() && _imp->isNearbySelectedKeysBRec( e->pos() ) ) {
            _imp->eventState = DopeSheetView::esMoveKeyframeSelection;
            didSomething = true;
        } else if ( _imp->isNearByCurrentFrameIndicatorBottom(clickZoomCoords) ) {
            _imp->eventState = DopeSheetView::esMoveCurrentFrameIndicator;
            didSomething = true;
        } else if ( !_imp->selectedKeysBRect.isNull() && _imp->isNearbySelectedKeysBRectLeftEdge( e->pos() ) ) {
            _imp->eventState = DopeSheetView::esTransformingKeyframesMiddleLeft;
            didSomething = true;
        } else if ( !_imp->selectedKeysBRect.isNull() && _imp->isNearbySelectedKeysBRectRightEdge( e->pos() ) ) {
            _imp->eventState = DopeSheetView::esTransformingKeyframesMiddleRight;
            didSomething = true;
        }

        DopeSheetSelectionModel::SelectionTypeFlags sFlags = DopeSheetSelectionModel::SelectionTypeAdd;
        if ( !modCASIsShift(e) ) {
            sFlags |= DopeSheetSelectionModel::SelectionTypeClear;
        }

        if (!didSomething) {
            ///Look for a range node
            AnimTreeItemNodeMap nodes = _imp->model->getItemNodeMap();
            for (AnimTreeItemNodeMap::iterator it = nodes.begin(); it != nodes.end(); ++it) {
                if ( it->second->isRangeDrawingEnabled() ) {
                    std::map<NodeAnimPtr, FrameRange >::const_iterator foundRange = _imp->nodeRanges.find( it->second );
                    if ( foundRange == _imp->nodeRanges.end() ) {
                        continue;
                    }

                    QRectF treeItemRect = _imp->hierarchyView->visualItemRect(it->first);
                    const FrameRange& range = foundRange->second;
                    RectD nodeClipRect;
                    nodeClipRect.x1 = range.first;
                    nodeClipRect.x2 = range.second;
                    nodeClipRect.y2 = _imp->zoomCtx.toZoomCoordinates( 0, treeItemRect.top() ).y();
                    nodeClipRect.y1 = _imp->zoomCtx.toZoomCoordinates( 0, treeItemRect.bottom() ).y();

                    AnimatedItemType nodeType = it->second->getItemType();

                    if ( nodeClipRect.contains( clickZoomCoords.x(), clickZoomCoords.y() ) ) {
                        std::vector<AnimKeyFrame> keysUnderMouse;
                        std::vector<NodeAnimPtr > selectedNodes;
                        selectedNodes.push_back(it->second);

                        _imp->model->getSelectionModel()->makeSelection(keysUnderMouse, selectedNodes, sFlags);

                        if ( (nodeType == eAnimatedItemTypeGroup) ||
                             ( nodeType == eAnimatedItemTypeReader) ||
                             ( nodeType == eAnimatedItemTypeTimeOffset) ||
                             ( nodeType == eAnimatedItemTypeFrameRange) ) {
                            _imp->eventState = DopeSheetView::esMoveKeyframeSelection;
                            didSomething = true;
                        }
                        break;
                    }

                    if (nodeType == eAnimatedItemTypeReader) {
                        _imp->currentEditedReader = it->second;
                        if ( _imp->isNearByClipRectLeft(clickZoomCoords, nodeClipRect) ) {
                            std::vector<AnimKeyFrame> keysUnderMouse;
                            std::vector<NodeAnimPtr > selectedNodes;
                            selectedNodes.push_back(it->second);

                            _imp->model->getSelectionModel()->makeSelection(keysUnderMouse, selectedNodes, sFlags);
                            _imp->eventState = DopeSheetView::esReaderLeftTrim;
                            didSomething = true;
                            break;
                        } else if ( _imp->isNearByClipRectRight(clickZoomCoords, nodeClipRect) ) {
                            std::vector<AnimKeyFrame> keysUnderMouse;
                            std::vector<NodeAnimPtr > selectedNodes;
                            selectedNodes.push_back(it->second);

                            _imp->model->getSelectionModel()->makeSelection(keysUnderMouse, selectedNodes, sFlags);
                            _imp->eventState = DopeSheetView::esReaderRightTrim;
                            didSomething = true;
                            break;
                        } else if ( _imp->model->canSlipReader(it->second) && _imp->isNearByClipRectBottom(clickZoomCoords, nodeClipRect) ) {
                            std::vector<AnimKeyFrame> keysUnderMouse;
                            std::vector<NodeAnimPtr > selectedNodes;
                            selectedNodes.push_back(it->second);

                            _imp->model->getSelectionModel()->makeSelection(keysUnderMouse, selectedNodes, sFlags);
                            _imp->eventState = DopeSheetView::esReaderSlip;
                            didSomething = true;
                            break;
                        }
                    }
                }
            }
        } // if (!didSomething)

        if (!didSomething) {
            QTreeWidgetItem *treeItem = _imp->hierarchyView->itemAt( 0, e->y() );
            //Did not find a range node, look for keyframes
            if (treeItem) {
                AnimTreeItemNodeMap NodeAnimItems = _imp->model->getItemNodeMap();
                AnimTreeItemNodeMap::const_iterator foundNodeAnim = NodeAnimItems.find(treeItem);
                if ( foundNodeAnim != NodeAnimItems.end() ) {
                    const NodeAnimPtr &NodeAnim = (*foundNodeAnim).second;
                    AnimatedItemType nodeType = NodeAnim->getItemType();
                    if (nodeType == eAnimatedItemTypeCommon) {
                        std::vector<AnimKeyFrame> keysUnderMouse = _imp->isNearByKeyframe( NodeAnim, e->pos() );

                        if ( !keysUnderMouse.empty() ) {
                            std::vector<NodeAnimPtr > selectedNodes;

                            sFlags |= DopeSheetSelectionModel::SelectionTypeRecurse;

                            _imp->model->getSelectionModel()->makeSelection(keysUnderMouse, selectedNodes, sFlags);

                            _imp->eventState = DopeSheetView::esMoveKeyframeSelection;
                            didSomething = true;
                        }
                    }
                } else { // if (foundNodeAnim != NodeAnimItems.end()) {
                    //We may be on a knob row
                    KnobAnimPtr KnobAnim = _imp->hierarchyView->getKnobAnimAt( e->pos().y() );
                    if (KnobAnim) {
                        std::vector<AnimKeyFrame> keysUnderMouse = _imp->isNearByKeyframe( KnobAnim, e->pos() );

                        if ( !keysUnderMouse.empty() ) {
                            sFlags |= DopeSheetSelectionModel::SelectionTypeRecurse;

                            std::vector<NodeAnimPtr > selectedNodes;
                            _imp->model->getSelectionModel()->makeSelection(keysUnderMouse, selectedNodes, sFlags);

                            _imp->eventState = DopeSheetView::esMoveKeyframeSelection;
                            didSomething = true;
                        }
                    }
                }
            } // if (treeItem) {
        } // if (!didSomething)
        Q_UNUSED(didSomething);


        // So the user left clicked on background
        if (_imp->eventState == DopeSheetView::esNoEditingState) {
            if ( !modCASIsShift(e) ) {
                _imp->model->getSelectionModel()->clearKeyframeSelection();

                    /*double keyframeTime = std::floor(_imp->zoomCtx.toZoomCoordinates(e->pos().x(), 0).x() + 0.5);
                    _imp->timeline->seekFrame(SequenceTime(keyframeTime), false, OutputEffectInstancePtr(),
                                              eTimelineChangeReasonAnimationModuleEditorSeek);*/

            }

            _imp->selectionRect.x1 = _imp->selectionRect.x2 = clickZoomCoords.x();
            _imp->selectionRect.y1 = _imp->selectionRect.y2 = clickZoomCoords.y();
        }

        _imp->lastPosOnMousePress = e->pos();
        _imp->keyDragLastMovement = 0.;
    }
} // DopeSheetView::mousePressEvent

void
DopeSheetView::mouseMoveEvent(QMouseEvent *e)
{
    running_in_main_thread();

    QPointF mouseZoomCoords = _imp->zoomCtx.toZoomCoordinates( e->x(), e->y() );

    bool caught = true;
    if (e->buttons() == Qt::NoButton) {
        setCursor( _imp->getCursorDuringHover( e->pos() ) );
        caught = false;
    } else if (_imp->eventState == DopeSheetView::esZoomingView) {
        _imp->zoomOrPannedSinceLastFit = true;

        int deltaX = 2 * ( e->x() - _imp->lastPosOnMouseMove.x() );
        const double par_min = 0.0001;
        const double par_max = 10000.;
        double scaleFactorX = std::pow( NATRON_WHEEL_ZOOM_PER_DELTA, deltaX);
        QPointF zoomCenter = _imp->zoomCtx.toZoomCoordinates( _imp->lastPosOnMousePress.x(),
                                                                  _imp->lastPosOnMousePress.y() );

        // Alt + Wheel: zoom time only, keep point under mouse
        double par = _imp->zoomCtx.aspectRatio() * scaleFactorX;
        if (par <= par_min) {
            par = par_min;
            scaleFactorX = par / _imp->zoomCtx.aspectRatio();
        } else if (par > par_max) {
            par = par_max;
            scaleFactorX = par / _imp->zoomCtx.factor();
        }
        _imp->zoomCtx.zoomx(zoomCenter.x(), zoomCenter.y(), scaleFactorX);

        redraw();

        // Synchronize the dope sheet editor and opened viewers
        if ( _imp->gui->isTripleSyncEnabled() ) {
            _imp->updateCurveWidgetFrameRange();
            _imp->gui->centerOpenedViewersOn( _imp->zoomCtx.left(), _imp->zoomCtx.right() );
        }
    } else if ( buttonDownIsLeft(e) ) {
        _imp->onMouseLeftButtonDrag(e);
    } else if ( buttonDownIsMiddle(e) ) {
        double dx = _imp->zoomCtx.toZoomCoordinates( _imp->lastPosOnMouseMove.x(),
                                                         _imp->lastPosOnMouseMove.y() ).x() - mouseZoomCoords.x();
        QMutexLocker k(&_imp->zoomCtxMutex);
        _imp->zoomCtx.translate(dx, 0);

        redraw();

        // Synchronize the curve editor and opened viewers
        if ( _imp->gui->isTripleSyncEnabled() ) {
            _imp->updateCurveWidgetFrameRange();
            _imp->gui->centerOpenedViewersOn( _imp->zoomCtx.left(), _imp->zoomCtx.right() );
        }
    } else {
        caught = false;
    }

    _imp->lastPosOnMouseMove = e->pos();
    if (!caught) {
        TabWidget* tab = 0;
        tab = _imp->model->getEditor()->getParentPane() ;
        if (tab) {
            // If the Viewer is in a tab, send the tab widget the event directly
            qApp->sendEvent(tab, e);
        } else {
            QGLWidget::mouseMoveEvent(e);
        }
    }
} // DopeSheetView::mouseMoveEvent

void
DopeSheetView::mouseReleaseEvent(QMouseEvent *e)
{
    Q_UNUSED(e);

    bool mustRedraw = false;

    if (_imp->eventState == DopeSheetView::esSelectionByRect) {
        if ( !_imp->selectionRect.isNull() ) {
            std::vector<AnimKeyFrame> tempSelection;
            std::vector<NodeAnimPtr > nodesSelection;
            _imp->createSelectionFromRect(_imp->selectionRect, &tempSelection, &nodesSelection);

            DopeSheetSelectionModel::SelectionTypeFlags sFlags = ( modCASIsShift(e) )
                                                                 ? DopeSheetSelectionModel::SelectionTypeToggle
                                                                 : DopeSheetSelectionModel::SelectionTypeAdd;

            _imp->model->getSelectionModel()->makeSelection(tempSelection, nodesSelection, sFlags);

            _imp->computeSelectedKeysBRect();
        }

        _imp->selectionRect.clear();

        mustRedraw = true;
    } else if (_imp->eventState == DopeSheetView::esMoveCurrentFrameIndicator) {
        if ( _imp->gui->isDraftRenderEnabled() ) {
            _imp->gui->setDraftRenderEnabled(false);
            bool autoProxyEnabled = appPTR->getCurrentSettings()->isAutoProxyEnabled();
            if (autoProxyEnabled) {
                _imp->gui->renderAllViewers(true);
            }
        }
    } else if ( (_imp->eventState == DopeSheetView::esMoveKeyframeSelection) ||
                ( _imp->eventState == DopeSheetView::esReaderLeftTrim) ||
                ( _imp->eventState == DopeSheetView::esReaderRightTrim) ||
                ( _imp->eventState == DopeSheetView::esReaderSlip) ||
                ( _imp->eventState == DopeSheetView::esTransformingKeyframesMiddleLeft) ||
                ( _imp->eventState == DopeSheetView::esTransformingKeyframesMiddleRight) ) {
        if ( _imp->gui->isDraftRenderEnabled() ) {
            _imp->gui->setDraftRenderEnabled(false);
            bool autoProxyEnabled = appPTR->getCurrentSettings()->isAutoProxyEnabled();
            if (autoProxyEnabled) {
                _imp->gui->renderAllViewers(true);
            }
        }
    }

    if (_imp->eventState != esNoEditingState) {
        _imp->eventState = esNoEditingState;

        if (_imp->currentEditedReader) {
            _imp->currentEditedReader.reset();
        }
    }

    if (mustRedraw) {
        redraw();
    }
} // DopeSheetView::mouseReleaseEvent

void
DopeSheetView::mouseDoubleClickEvent(QMouseEvent *e)
{
    running_in_main_thread();

    if ( modCASIsControl(e) ) {
        KnobAnimPtr KnobAnim = _imp->hierarchyView->getKnobAnimAt( e->pos().y() );
        if (KnobAnim) {
            std::vector<AnimKeyFrame> toPaste;
            double keyframeTime = std::floor(_imp->zoomCtx.toZoomCoordinates(e->pos().x(), 0).x() + 0.5);
            int dim = KnobAnim->getDimension();
            KnobIPtr knob = KnobAnim->getInternalKnob();
            bool hasKeyframe = false;
            for (int i = 0; i < knob->getDimension(); ++i) {
                if ( (i == dim) || (dim == -1) ) {
                    CurvePtr curve = knob->getGuiCurve(ViewIdx(0), i);
                    KeyFrame k;
                    if ( curve && curve->getKeyFrameWithTime(keyframeTime, &k) ) {
                        hasKeyframe = true;
                        break;
                    }
                }
            }
            if (!hasKeyframe) {
                _imp->timeline->seekFrame(SequenceTime(keyframeTime), false, OutputEffectInstancePtr(),
                                          eTimelineChangeReasonAnimationModuleEditorSeek);

                KeyFrame k(keyframeTime, 0);
                AnimKeyFrame key(KnobAnim, k);
                toPaste.push_back(key);
                DSPasteKeysCommand::setKeyValueFromKnob(knob, keyframeTime, &k);
                _imp->model->pasteKeys(toPaste, true);
            }
        }
    }
}

void
DopeSheetView::wheelEvent(QWheelEvent *e)
{
    running_in_main_thread();

    // don't handle horizontal wheel (e.g. on trackpad or Might Mouse)
    if (e->orientation() != Qt::Vertical) {
        return;
    }

    static const double par_min = 0.01; // 1 pixel for 100 frames
    static const double par_max = 100.; // 100 pixels per frame is reasonale, see also TimeLineGui::wheelEvent()
    double par;
    double scaleFactor = std::pow( NATRON_WHEEL_ZOOM_PER_DELTA, e->delta() );
    QPointF zoomCenter = _imp->zoomCtx.toZoomCoordinates( e->x(), e->y() );

    _imp->zoomOrPannedSinceLastFit = true;

    par = _imp->zoomCtx.aspectRatio() * scaleFactor;

    if (par <= par_min) {
        par = par_min;
        scaleFactor = par / _imp->zoomCtx.aspectRatio();
    } else if (par > par_max) {
        par = par_max;
        scaleFactor = par / _imp->zoomCtx.aspectRatio();
    }


    {
        QMutexLocker k(&_imp->zoomCtxMutex);
        _imp->zoomCtx.zoomx(zoomCenter.x(), zoomCenter.y(), scaleFactor);
    }
    _imp->computeSelectedKeysBRect();

    redraw();

    // Synchronize the curve editor and opened viewers
    if ( _imp->gui->isTripleSyncEnabled() ) {
        _imp->updateCurveWidgetFrameRange();
        _imp->gui->centerOpenedViewersOn( _imp->zoomCtx.left(), _imp->zoomCtx.right() );
    }
}

/**
 * @brief Get the current orthographic projection
 **/
void
DopeSheetView::getProjection(double *zoomLeft, double *zoomBottom, double *zoomFactor, double *zoomAspectRatio) const
{
    QMutexLocker k(&_imp->zoomCtxMutex);
    *zoomLeft = _imp->zoomCtx.left();
    *zoomBottom = _imp->zoomCtx.bottom();
    *zoomFactor = _imp->zoomCtx.factor();
    *zoomAspectRatio = _imp->zoomCtx.aspectRatio();
}

/**
 * @brief Set the current orthographic projection
 **/
void
DopeSheetView::setProjection(double zoomLeft, double zoomBottom, double zoomFactor, double zoomAspectRatio)
{
    QMutexLocker k(&_imp->zoomCtxMutex);
    _imp->zoomCtx.setZoom(zoomLeft, zoomBottom, zoomFactor, zoomAspectRatio);
}


NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_DopeSheetView.cpp"
