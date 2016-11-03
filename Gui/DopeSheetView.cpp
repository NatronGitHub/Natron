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
#include "Gui/AnimationModuleUndoRedo.h"
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

    DopeSheetViewPrivate(Gui* gui, const AnimationModuleBasePtr& model, DopeSheetView *publicInterface);

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

    Qt::CursorShape getCursorDuringHover(const QPoint &widgetCoords) const;
    Qt::CursorShape getCursorForEventState(DopeSheetView::EventStateEnum es) const;

    bool isNearByClipRectLeft(const QPointF& zoomCoordPos, const RectD &clipRect) const;
    bool isNearByClipRectRight(const QPointF& zoomCoordPos, const RectD &clipRect) const;
    bool isNearByClipRectBottom(const QPointF& zoomCoordPos, const RectD &clipRect) const;
    bool isNearByCurrentFrameIndicatorBottom(const QPointF &zoomCoords) const;

    virtual bool isSelectedKeyFramesRectanglePointEnabled(SelectedKeyFramesRectanglePointEnum pt) const
    {
        if (pt == eSelectedKeyFramesRectanglePointMidLeft ||
            pt == eSelectedKeyFramesRectanglePointMidRight ||
            pt == eSelectedKeyFramesRectanglePointCenter) {
            return true;
        }
        return false;
    }

    KeyFrameWithStringSet isNearByKeyframe(const AnimItemBasePtr &item, DimSpec dimension, ViewSetSpec view, const QPointF &widgetCoords) const;

    // Textures


    // Drawing
    void drawScale() const;

    void drawRows() const;

    void drawTreeItemRecursive(QTreeWidgetItem* item, std::list<NodeAnimPtr>* nodesRowsOrdered) const;

    void drawNodeRow(QTreeWidgetItem* treeItem, const NodeAnimPtr& item) const;
    void drawKnobRow(QTreeWidgetItem* treeItem, const KnobAnimPtr& item, DimSpec dimension, ViewSetSpec view) const;
    void drawAnimItemRow(QTreeWidgetItem* treeItem, const AnimItemBasePtr& item, DimSpec dimension, ViewSetSpec view) const;
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


    void updateCurveWidgetFrameRange();

    /* attributes */
    DopeSheetView *_widget;
    AnimationModuleTreeView *treeView;

    std::map<NodeAnimPtr, FrameRange> nodeRanges;
    std::list<NodeAnimPtr> nodeRangesBeingComputed; // to avoid recursion in groups
    int rangeComputationRecursion;

    DopeSheetView::EventStateEnum eventState;

    // for clip (Reader, Time nodes) user interaction
    NodeAnimPtr currentEditedReader;

};

DopeSheetViewPrivate::DopeSheetViewPrivate(Gui* gui, const AnimationModuleBasePtr& model, DopeSheetView *publicInterface)
: AnimationModuleViewPrivateBase(gui, publicInterface, model)
, _widget(0)
, treeView(0)
, nodeRanges()
, nodeRangesBeingComputed()
, rangeComputationRecursion(0)
, eventState(DopeSheetView::eEventStateNoEditingState)
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
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

/**
 * @brief DopeSheetView::~DopeSheetView
 *
 * Destroys the DopeSheetView object.
 */
DopeSheetView::~DopeSheetView()
{
}

QSize
DopeSheetView::sizeHint() const
{
    return QSize(10000, 10000);
}

void
DopeSheetView::initializeImplementation(Gui* gui, const AnimationModuleBasePtr& model, AnimationViewBase* /*publicInterface*/)
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
DopeSheetViewPrivate::getCursorDuringHover(const QPoint &widgetCoords) const
{
    QPointF clickZoomCoords = zoomCtx.toZoomCoordinates( widgetCoords.x(), widgetCoords.y() );

    if ( isNearbySelectedKeyFramesCrossWidget(widgetCoords) ) {
        return getCursorForEventState(DopeSheetView::eEventStateMoveKeyframeSelection);
    } else if ( isNearbyBboxMidRight(widgetCoords) ) {
        return getCursorForEventState(DopeSheetView::eEventStateDraggingMidRightBbox);
    } else if ( isNearbyBboxMidLeft(widgetCoords) ) {
        return getCursorForEventState(DopeSheetView::eEventStateDraggingMidLeftBbox);
    } else if ( isNearbyTimelineBtmPoly(widgetCoords) || isNearbyTimelineTopPoly(widgetCoords)) {
        return getCursorForEventState(DopeSheetView::eEventStateMoveCurrentFrameIndicator);
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
                return getCursorForEventState(DopeSheetView::eEventStateMoveKeyframeSelection);
            }
            if (nodeType == eAnimatedItemTypeReader) {
                if ( isNearByClipRectLeft(clickZoomCoords, nodeClipRect) ) {
                    return getCursorForEventState(DopeSheetView::eEventStateReaderLeftTrim);
                } else if ( isNearByClipRectRight(clickZoomCoords, nodeClipRect) ) {
                    return getCursorForEventState(DopeSheetView::eEventStateReaderRightTrim);
                } else if ( animModel->canSlipReader(*it) && isNearByClipRectBottom(clickZoomCoords, nodeClipRect) ) {
                    return getCursorForEventState(DopeSheetView::eEventStateReaderSlip);
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
                    return getCursorForEventState(DopeSheetView::eEventStatePickKeyframe);
                }
            }
        }
    } // if (treeItem) {

    return getCursorForEventState(DopeSheetView::eEventStateNoEditingState);
} // DopeSheetViewPrivate::getCursorDuringHover

Qt::CursorShape
DopeSheetViewPrivate::getCursorForEventState(DopeSheetView::EventStateEnum es) const
{
    Qt::CursorShape cursorShape;

    switch (es) {
    case DopeSheetView::eEventStatePickKeyframe:
        cursorShape = Qt::CrossCursor;
        break;
    case DopeSheetView::eEventStateMoveKeyframeSelection:
        cursorShape = Qt::OpenHandCursor;
        break;
    case DopeSheetView::eEventStateReaderLeftTrim:
    case DopeSheetView::eEventStateMoveCurrentFrameIndicator:
        cursorShape = Qt::SplitHCursor;
        break;
    case DopeSheetView::eEventStateReaderRightTrim:
        cursorShape = Qt::SplitHCursor;
        break;
    case DopeSheetView::eEventStateReaderSlip:
        cursorShape = Qt::SizeHorCursor;
        break;
    case DopeSheetView::eEventStateDraggingMidLeftBbox:
    case DopeSheetView::eEventStateDraggingMidRightBbox:
        cursorShape = Qt::SplitHCursor;
        break;
    case DopeSheetView::eEventStateNoEditingState:
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
DopeSheetViewPrivate::drawTreeItemRecursive(QTreeWidgetItem* item,  std::list<NodeAnimPtr>* nodesRowsOrdered) const
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

    int nChildren = item->childCount();
    for (int i = 0; i < nChildren; ++i) {
        QTreeWidgetItem* child = item->child(i);
        drawTreeItemRecursive(child, nodesRowsOrdered);
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
    drawAnimItemRow(treeItem, item, dimension, view);
}

void
DopeSheetViewPrivate::drawKnobRow(QTreeWidgetItem* treeItem, const KnobAnimPtr& item, DimSpec dimension, ViewSetSpec view) const
{
    drawAnimItemRow(treeItem, item, dimension, view);
}

void
DopeSheetViewPrivate::drawAnimItemRow(QTreeWidgetItem* treeItem, const AnimItemBasePtr& item, DimSpec dimension, ViewSetSpec view) const
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
    bool drawInDimRow = treeView->isItemVisibleRecursive(treeItem);

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
DopeSheetView::refreshSelectionBoundingBox()
{
    _imp->computeSelectedKeysBRect();
}

void
DopeSheetViewPrivate::computeSelectedKeysBRect()
{
    if (zoomCtx.screenHeight() == 0 || zoomCtx.screenWidth() == 0) {
        return;
    }
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

    if ( selectedKeysBRect.isNull() ) {
        selectedKeysBRect.clear();
        return;
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
    } else {
        selectedKeysBRect.clear();
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
DopeSheetViewPrivate::computeGroupRange(const NodeAnimPtr& groupAnim)
{
    if ( std::find(nodeRangesBeingComputed.begin(), nodeRangesBeingComputed.end(), groupAnim) != nodeRangesBeingComputed.end() ) {
        return;
    }

    NodePtr internalNode = groupAnim->getInternalNode();
    if (!internalNode) {
        return;
    }

    AnimationModulePtr model = toAnimationModule(_model.lock());

    FrameRange range;
    std::set<double> times;
    NodeGroupPtr nodegroup = internalNode->isEffectNodeGroup();
    assert(nodegroup);
    if (!nodegroup) {
        throw std::logic_error("DopeSheetViewPrivate::computeGroupRange: node is not a group");
    }

    nodeRangesBeingComputed.push_back(groupAnim);
    ++rangeComputationRecursion;


    NodesList nodes = nodegroup->getNodes();

    for (NodesList::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        NodeAnimPtr childAnim = model->findNodeAnim(*it);

        if (!childAnim) {
            continue;
        }

        if (!treeView->isItemVisibleRecursive(childAnim->getTreeItem())) {
            continue;
        }

        NodeGuiPtr childGui = childAnim->getNodeGui();


        computeNodeRange(childAnim);

        std::map<NodeAnimPtr, FrameRange >::iterator found = nodeRanges.find(childAnim);
        if ( found != nodeRanges.end() ) {
            times.insert(found->second.first);
            times.insert(found->second.second);
        }


        const KnobsVec &knobs = childGui->getNode()->getKnobs();

        for (KnobsVec::const_iterator it2 = knobs.begin(); it2 != knobs.end(); ++it2) {

            if ( !(*it2)->isAnimationEnabled() || !(*it2)->hasAnimation() ) {
                continue;
            } else {
                int nDims = (*it2)->getNDimensions();
                std::list<ViewIdx> views = (*it2)->getViewsList();
                for (std::list<ViewIdx>::const_iterator it3 = views.begin(); it3 != views.end(); ++it3) {
                    for (int i = 0; i < nDims; ++i) {
                        CurvePtr curve = (*it2)->getCurve(*it3, DimIdx(i));
                        if (!curve) {
                            continue;
                        }
                        int nKeys = curve->getKeyFramesCount();
                        if (nKeys > 0) {
                            KeyFrame k;
                            if (curve->getKeyFrameWithIndex(0, &k)) {
                                times.insert( k.getTime() );
                            }
                            if (curve->getKeyFrameWithIndex(nKeys - 1, &k)) {
                                times.insert( k.getTime() );
                            }
                        }
                    }
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

    nodeRanges[groupAnim] = range;

    --rangeComputationRecursion;
    std::list<NodeAnimPtr>::iterator found = std::find(nodeRangesBeingComputed.begin(), nodeRangesBeingComputed.end(), groupAnim);
    assert( found != nodeRangesBeingComputed.end() );
    nodeRangesBeingComputed.erase(found);
} // DopeSheetViewPrivate::computeGroupRange

void
DopeSheetViewPrivate::onMouseLeftButtonDrag(QMouseEvent *e)
{
    QPointF mouseZoomCoords = zoomCtx.toZoomCoordinates( e->x(), e->y() );
    QPointF lastZoomCoordsOnMousePress = zoomCtx.toZoomCoordinates( _dragStartPoint.x(), _dragStartPoint.y() );
    QPointF lastZoomCoordsOnMouseMove = zoomCtx.toZoomCoordinates( _lastMousePos.x(), _lastMousePos.y() );

    switch (eventState) {
        case DopeSheetView::eEventStateMoveKeyframeSelection: {
            moveSelectedKeyFrames(lastZoomCoordsOnMouseMove, mouseZoomCoords);
            break;
        }
        case DopeSheetView::eEventStateDraggingMidLeftBbox:
        case DopeSheetView::eEventStateDraggingMidRightBbox: {
            AnimationModuleViewPrivateBase::TransformSelectionCenterEnum centerType;
            if (eventState == DopeSheetView::eEventStateDraggingMidLeftBbox) {
                centerType = AnimationModuleViewPrivateBase::eTransformSelectionCenterRight;
            } else {
                centerType = AnimationModuleViewPrivateBase::eTransformSelectionCenterLeft;
            }
            const bool scaleX = true;
            const bool scaleY = false;

            transformSelectedKeyFrames( lastZoomCoordsOnMouseMove, mouseZoomCoords, modCASIsShift(e), centerType, scaleX, scaleY );
            break;
        }
        case DopeSheetView::eEventStateMoveCurrentFrameIndicator: {
            moveCurrentFrameIndicator(mouseZoomCoords.x());
            break;
        }
        case DopeSheetView::eEventStateSelectionByRect: {
            computeSelectionRect(lastZoomCoordsOnMousePress, mouseZoomCoords);
            _widget->redraw();
            break;
        }
        case DopeSheetView::eEventStateReaderLeftTrim: {

            KnobIntPtr timeOffsetKnob = toKnobInt(currentEditedReader->getInternalNode()->getKnobByName(kReaderParamNameTimeOffset));
            assert(timeOffsetKnob);
            double newFirstFrame = std::floor(mouseZoomCoords.x() - timeOffsetKnob->getValue() + 0.5);
            _model.lock()->trimReaderLeft(currentEditedReader, newFirstFrame);


            break;
        }
        case DopeSheetView::eEventStateReaderRightTrim: {
            KnobIntPtr timeOffsetKnob = toKnobInt(currentEditedReader->getInternalNode()->getKnobByName(kReaderParamNameTimeOffset));
            assert(timeOffsetKnob);
            double newLastFrame = std::floor(mouseZoomCoords.x() - timeOffsetKnob->getValue() + 0.5);

            _model.lock()->trimReaderRight(currentEditedReader, newLastFrame);
            break;
        }
        case DopeSheetView::eEventStateReaderSlip: {
            double dt = mouseZoomCoords.x() - lastZoomCoordsOnMouseMove.x();
            _model.lock()->slipReader(currentEditedReader, dt);
            
            break;
        }
        case DopeSheetView::eEventStateNoEditingState:
            eventState = DopeSheetView::eEventStateSelectionByRect;
            break;
        default:
            break;
    } // switch
} // DopeSheetViewPrivate::onMouseLeftButtonDrag

void
DopeSheetViewPrivate::createSelectionFromRect(const RectD &canonicalRect,
                                              AnimItemDimViewKeyFramesMap *keys,
                                              std::vector<NodeAnimPtr >* nodes,
                                              std::vector<TableItemAnimPtr >* /*tableItems*/)
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
    appPTR->getCurrentSettings()->getAnimationModuleEditorBackgroundColor(&r, &g, &b);
}

void
DopeSheetView::centerOnAllItems()
{
    centerOnRangeInternal(getKeyframeRange());
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
DopeSheetView::onNodeAdded(const NodeAnimPtr& node)
{
    AnimationModulePtr animModel = toAnimationModule(_imp->_model.lock());
    AnimatedItemTypeEnum nodeType = node->getItemType();
    NodePtr internalNode = node->getInternalNode();

    if (nodeType == eAnimatedItemTypeCommon) {
        NodeGroupPtr isInGroup = toNodeGroup(internalNode->getGroup());
        if (isInGroup) {
            const KnobsVec& knobs = internalNode->getKnobs();
            for (KnobsVec::const_iterator knobIt = knobs.begin(); knobIt != knobs.end(); ++knobIt) {
                connect((*knobIt)->getSignalSlotHandler().get(), SIGNAL(curveAnimationChanged(std::list<double>,std::list<double>,ViewIdx,DimIdx)),
                         this, SLOT(onKnobAnimationChanged()) );
            }
        }
        // Also connect the lifetime knob
        KnobIntPtr lifeTimeKnob = internalNode->getLifeTimeKnob();
        if (lifeTimeKnob) {
            connect( lifeTimeKnob->getSignalSlotHandler().get(), SIGNAL(mustRefreshKnobGui(ViewSetSpec,DimSpec,ValueChangedReasonEnum)),
                this, SLOT(onRangeNodeKnobChanged()) );
        }

    } else if (nodeType == eAnimatedItemTypeReader) {
        // The dopesheet view must refresh if the user set some values in the settings panel
        // so we connect some signals/slots
        KnobIPtr lastFrameKnob = internalNode->getKnobByName(kReaderParamNameLastFrame);
        if (!lastFrameKnob) {
            return;
        }
        boost::shared_ptr<KnobSignalSlotHandler> lastFrameKnobHandler =  lastFrameKnob->getSignalSlotHandler();
        assert(lastFrameKnob);
        boost::shared_ptr<KnobSignalSlotHandler> startingTimeKnob = internalNode->getKnobByName(kReaderParamNameStartingTime)->getSignalSlotHandler();
        assert(startingTimeKnob);

        connect( lastFrameKnobHandler.get(), SIGNAL(mustRefreshKnobGui(ViewSetSpec,DimSpec,ValueChangedReasonEnum)),
                 this, SLOT(onRangeNodeKnobChanged()) );

        connect( startingTimeKnob.get(), SIGNAL(mustRefreshKnobGui(ViewSetSpec,DimSpec,ValueChangedReasonEnum)),
                 this, SLOT(onRangeNodeKnobChanged()) );

        // We don't make the connection for the first frame knob, because the
        // starting time is updated when it's modified. Thus we avoid two
        // refreshes of the view.
    } else if (nodeType == eAnimatedItemTypeRetime) {
        boost::shared_ptr<KnobSignalSlotHandler> speedKnob =  internalNode->getKnobByName(kRetimeParamNameSpeed)->getSignalSlotHandler();
        assert(speedKnob);

        connect( speedKnob.get(), SIGNAL(mustRefreshKnobGui(ViewSetSpec,DimSpec,ValueChangedReasonEnum)),
                 this, SLOT(onRangeNodeKnobChanged()) );
    } else if (nodeType == eAnimatedItemTypeTimeOffset) {
        boost::shared_ptr<KnobSignalSlotHandler> timeOffsetKnob =  internalNode->getKnobByName(kReaderParamNameTimeOffset)->getSignalSlotHandler();
        assert(timeOffsetKnob);

        connect( timeOffsetKnob.get(), SIGNAL(mustRefreshKnobGui(ViewSetSpec,DimSpec,ValueChangedReasonEnum)),
                 this, SLOT(onRangeNodeKnobChanged()) );
    } else if (nodeType == eAnimatedItemTypeFrameRange) {
        boost::shared_ptr<KnobSignalSlotHandler> frameRangeKnob =  internalNode->getKnobByName(kFrameRangeParamNameFrameRange)->getSignalSlotHandler();
        assert(frameRangeKnob);

        connect( frameRangeKnob.get(), SIGNAL(mustRefreshKnobGui(ViewSetSpec,DimSpec,ValueChangedReasonEnum)),
                 this, SLOT(onRangeNodeKnobChanged()) );
    }

    _imp->computeNodeRange(node);


    NodeAnimPtr parentGroupNodeAnim = animModel->getGroupNodeAnim(node);
    if (parentGroupNodeAnim) {
        _imp->computeGroupRange( parentGroupNodeAnim );
    }

} // DopeSheetView::onNodeAdded

void
DopeSheetView::onNodeAboutToBeRemoved(const NodeAnimPtr& NodeAnim)
{
    // Refresh the group range
    {
        AnimationModulePtr animModel = toAnimationModule(_imp->_model.lock());
        NodeAnimPtr parentGroupNodeAnim = animModel->getGroupNodeAnim(NodeAnim);
        if (parentGroupNodeAnim) {
            _imp->computeGroupRange( parentGroupNodeAnim );
        }
    }

    // remove the node from the frame ranges
    std::map<NodeAnimPtr, FrameRange>::iterator toRemove = _imp->nodeRanges.find(NodeAnim);

    if ( toRemove != _imp->nodeRanges.end() ) {
        _imp->nodeRanges.erase(toRemove);
    }

    // Recompute selection rectangle bounding box
    _imp->computeSelectedKeysBRect();

    // update view
    redraw();
}

void
DopeSheetView::onKnobAnimationChanged()
{
    KnobSignalSlotHandler *knobHandler = qobject_cast<KnobSignalSlotHandler *>(sender());
    if (!knobHandler) {
        return;
    }
    KnobHolderPtr holder = knobHandler->getKnob()->getHolder();
    EffectInstancePtr effectInstance = toEffectInstance(holder);
    if (!effectInstance) {
        return;
    }
    AnimationModulePtr animModel = toAnimationModule(_imp->_model.lock());

    NodeAnimPtr node = animModel->findNodeAnim( effectInstance->getNode() );

    if (!node) {
        return;
    }

    {
        NodeAnimPtr parentGroupNodeAnim = animModel->getGroupNodeAnim( node );
        if (parentGroupNodeAnim) {
            _imp->computeGroupRange( parentGroupNodeAnim );
        }
    }

    // Since this function is called a lot, let a chance to Qt to concatenate events
    // NB: updateGL() does not concatenate
    update();
}

void
DopeSheetView::onRangeNodeKnobChanged()
{
    KnobSignalSlotHandler *knobHandler = qobject_cast<KnobSignalSlotHandler *>(sender());
    if (!knobHandler) {
        return;
    }

    KnobHolderPtr holder = knobHandler->getKnob()->getHolder();
    EffectInstancePtr effectInstance = toEffectInstance(holder);
    if (!effectInstance) {
        return;
    }
    AnimationModulePtr animModel = toAnimationModule(_imp->_model.lock());

    NodeAnimPtr node = animModel->findNodeAnim( effectInstance->getNode() );

    if (!node) {
        return;
    }
    _imp->computeNodeRange(node);

    // Since this function is called a lot, let a chance to Qt to concatenate events
    // NB: updateGL() does not concatenate
    update();
}

void
DopeSheetView::onAnimationTreeViewItemExpandedOrCollapsed(QTreeWidgetItem *item)
{
    // Compute the range rects of affected items
    AnimationModulePtr animModel = toAnimationModule(_imp->_model.lock());
    {
        NodeAnimPtr isNode;
        KnobAnimPtr isKnob;
        TableItemAnimPtr isTableItem;
        AnimatedItemTypeEnum type;
        ViewSetSpec view;
        DimSpec dimension;
        bool found = animModel->findItem(item, &type, &isKnob, &isTableItem, &isNode, &view, &dimension);
        if (!found) {
            return;
        }
        if (isTableItem) {
            isNode = isTableItem->getNode();
        } else if (isKnob) {
            TableItemAnimPtr knobContainer = toTableItemAnim(isKnob->getHolder());
            if (knobContainer) {
                isNode = knobContainer->getNode();
            }
            NodeAnimPtr nodeContainer = toNodeAnim(isKnob->getHolder());
            if (nodeContainer) {
                isNode = nodeContainer;
            }
        }

        if (isNode) {
            _imp->computeRangesBelow(isNode);
        }
    }

    _imp->computeSelectedKeysBRect();

    redraw();
}

void
DopeSheetView::onAnimationTreeViewScrollbarMoved(int /*value*/)
{
    _imp->computeSelectedKeysBRect();

    redraw();
}


/**
 * @brief DopeSheetView::paintGL
 */
void
DopeSheetView::drawView()
{
    _imp->drawScale();
    _imp->drawTimelineMarkers();
    _imp->drawRows();

    if (_imp->eventState == DopeSheetView::eEventStateSelectionByRect) {
        _imp->drawSelectionRect();
    }

    if ( !_imp->selectedKeysBRect.isNull() ) {
        _imp->drawSelectedKeyFramesBbox();
    }


}

void
DopeSheetView::mousePressEvent(QMouseEvent *e)
{
    running_in_main_thread();

    AnimationModulePtr animModule = toAnimationModule(_imp->_model.lock());
    assert(animModule);
    animModule->getEditor()->onInputEventCalled();

    bool didSomething = false;

    _imp->_dragStartPoint = e->pos();
    _imp->_lastMousePos = e->pos();
    _imp->_keyDragLastMovement.ry() = _imp->_keyDragLastMovement.rx() = 0;

    if ( buttonDownIsRight(e) ) {
        _imp->createMenu();
        _imp->_rightClickMenu->exec( mapToGlobal( e->pos() ) );

        e->accept();

        //didSomething = true;
        return;
    }

    if ( buttonDownIsMiddle(e) ) {
        _imp->eventState = DopeSheetView::eEventStateDraggingView;
        didSomething = true;
    } else if ( ( (e->buttons() & Qt::MiddleButton) &&
                  ( ( buttonMetaAlt(e) == Qt::AltModifier) || (e->buttons() & Qt::LeftButton) ) ) ||
                ( (e->buttons() & Qt::LeftButton) &&
                  ( buttonMetaAlt(e) == (Qt::AltModifier | Qt::MetaModifier) ) ) ) {
        // Alt + middle or Left + middle or Crtl + Alt + Left = zoom
        _imp->eventState = eEventStateZoomingView;


        return;
    }

    QPointF clickZoomCoords = _imp->zoomCtx.toZoomCoordinates( e->x(), e->y() );

    if ( buttonDownIsLeft(e) ) {
        if ( !_imp->selectedKeysBRect.isNull() && _imp->isNearbySelectedKeyFramesCrossWidget( e->pos() ) ) {
            _imp->eventState = DopeSheetView::eEventStateMoveKeyframeSelection;
            didSomething = true;
        } else if ( _imp->isNearbyTimelineBtmPoly(e->pos()) || _imp->isNearbyTimelineTopPoly(e->pos()) ) {
            _imp->eventState = DopeSheetView::eEventStateMoveCurrentFrameIndicator;
            didSomething = true;
        } else if ( !_imp->selectedKeysBRect.isNull() && _imp->isNearbyBboxMidLeft( e->pos() ) ) {
            _imp->eventState = DopeSheetView::eEventStateDraggingMidLeftBbox;
            didSomething = true;
        } else if ( !_imp->selectedKeysBRect.isNull() && _imp->isNearbyBboxMidRight( e->pos() ) ) {
            _imp->eventState = DopeSheetView::eEventStateDraggingMidRightBbox;
            didSomething = true;
        }

        AnimationModuleSelectionModel::SelectionTypeFlags sFlags = AnimationModuleSelectionModel::SelectionTypeAdd;
        if ( !modCASIsShift(e) ) {
            sFlags |= AnimationModuleSelectionModel::SelectionTypeClear;
        }

        if (!didSomething) {
            // Look for a range node
            for (std::map<NodeAnimPtr, FrameRange >::const_iterator it = _imp->nodeRanges.begin(); it != _imp->nodeRanges.end(); ++it) {

                QRectF treeItemRect = _imp->treeView->visualItemRect(it->first->getTreeItem());
                const FrameRange& range = it->second;
                RectD nodeClipRect;
                nodeClipRect.x1 = range.first;
                nodeClipRect.x2 = range.second;
                nodeClipRect.y2 = _imp->zoomCtx.toZoomCoordinates( 0, treeItemRect.top() ).y();
                nodeClipRect.y1 = _imp->zoomCtx.toZoomCoordinates( 0, treeItemRect.bottom() ).y();

                AnimatedItemTypeEnum nodeType = it->first->getItemType();

                // If we click inside a range, start dragging
                if ( nodeClipRect.contains( clickZoomCoords.x(), clickZoomCoords.y() ) ) {
                    AnimItemDimViewKeyFramesMap selectedKeys;
                    std::vector<NodeAnimPtr > selectedNodes;
                    selectedNodes.push_back(it->first);
                    std::vector<TableItemAnimPtr > selectedTableItems;
                    animModule->getSelectionModel()->makeSelection(selectedKeys, selectedTableItems, selectedNodes, sFlags);

                    if ( (nodeType == eAnimatedItemTypeGroup) ||
                        ( nodeType == eAnimatedItemTypeReader) ||
                        ( nodeType == eAnimatedItemTypeTimeOffset) ||
                        ( nodeType == eAnimatedItemTypeFrameRange) ||
                        ( nodeType == eAnimatedItemTypeCommon)) {
                        _imp->eventState = DopeSheetView::eEventStateMoveKeyframeSelection;
                        didSomething = true;
                    }
                    break;
                }

                if (nodeType == eAnimatedItemTypeReader) {
                    _imp->currentEditedReader = it->first;
                    if ( _imp->isNearByClipRectLeft(clickZoomCoords, nodeClipRect) ) {
                        AnimItemDimViewKeyFramesMap selectedKeys;
                        std::vector<NodeAnimPtr > selectedNodes;
                        selectedNodes.push_back(it->first);
                        std::vector<TableItemAnimPtr > selectedTableItems;
                        animModule->getSelectionModel()->makeSelection(selectedKeys, selectedTableItems, selectedNodes, sFlags);

                        _imp->eventState = DopeSheetView::eEventStateReaderLeftTrim;
                        didSomething = true;
                        break;
                    } else if ( _imp->isNearByClipRectRight(clickZoomCoords, nodeClipRect) ) {
                        AnimItemDimViewKeyFramesMap selectedKeys;
                        std::vector<NodeAnimPtr > selectedNodes;
                        selectedNodes.push_back(it->first);
                        std::vector<TableItemAnimPtr > selectedTableItems;
                        animModule->getSelectionModel()->makeSelection(selectedKeys, selectedTableItems, selectedNodes, sFlags);
                        _imp->eventState = DopeSheetView::eEventStateReaderRightTrim;
                        didSomething = true;
                        break;
                    } else if ( animModule->canSlipReader(it->first) && _imp->isNearByClipRectBottom(clickZoomCoords, nodeClipRect) ) {
                        AnimItemDimViewKeyFramesMap selectedKeys;
                        std::vector<NodeAnimPtr > selectedNodes;
                        selectedNodes.push_back(it->first);
                        std::vector<TableItemAnimPtr > selectedTableItems;
                        animModule->getSelectionModel()->makeSelection(selectedKeys, selectedTableItems, selectedNodes, sFlags);
                        _imp->eventState = DopeSheetView::eEventStateReaderSlip;
                        didSomething = true;
                        break;
                    }
                }
            } // for all range nodes

        } // if (!didSomething)

        if (!didSomething) {
            QTreeWidgetItem *treeItem = _imp->treeView->itemAt( 0, e->y() );
            // Did not find a range node, look for keyframes
            if (treeItem) {

                AnimatedItemTypeEnum itemType;
                KnobAnimPtr isKnob;
                NodeAnimPtr isNode;
                TableItemAnimPtr isTableItem;
                ViewSetSpec view;
                DimSpec dimension;
                if (animModule->findItem(treeItem, &itemType, &isKnob, &isTableItem, &isNode, &view, &dimension)) {
                    AnimItemBasePtr animItem;
                    if (isTableItem) {
                        animItem = isTableItem;
                    } else if (isKnob) {
                        animItem = isKnob;
                    }
                    if (animItem) {
                        KeyFrameWithStringSet keysUnderMouse = _imp->isNearByKeyframe(animItem, dimension, view, e->pos());

                        if (!keysUnderMouse.empty()) {
                            sFlags |= AnimationModuleSelectionModel::SelectionTypeRecurse;

                            AnimItemDimViewKeyFramesMap selectedKeys;
                            animItem->getKeyframes(dimension, view, &selectedKeys);
                            std::vector<NodeAnimPtr > selectedNodes;
                            std::vector<TableItemAnimPtr > selectedTableItems;
                            animModule->getSelectionModel()->makeSelection(selectedKeys, selectedTableItems, selectedNodes, sFlags);

                            _imp->eventState = DopeSheetView::eEventStateMoveKeyframeSelection;
                            didSomething = true;
                        }
                        
                    }
                }
            } // if (treeItem) {
        } // if (!didSomething)
        Q_UNUSED(didSomething);


        // So the user left clicked on background
        if (_imp->eventState == DopeSheetView::eEventStateNoEditingState) {
            if ( !modCASIsShift(e) ) {
                animModule->getSelectionModel()->clearSelection();
            }

            _imp->selectionRect.x1 = _imp->selectionRect.x2 = clickZoomCoords.x();
            _imp->selectionRect.y1 = _imp->selectionRect.y2 = clickZoomCoords.y();
        }
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
    } else if (_imp->eventState == DopeSheetView::eEventStateZoomingView) {
        _imp->zoomOrPannedSinceLastFit = true;

        int deltaX = 2 * ( e->x() - _imp->_lastMousePos.x() );
        const double par_min = 0.0001;
        const double par_max = 10000.;
        double scaleFactorX = std::pow( NATRON_WHEEL_ZOOM_PER_DELTA, deltaX);
        QPointF zoomCenter = _imp->zoomCtx.toZoomCoordinates( _imp->_dragStartPoint.x(), _imp->_dragStartPoint.y() );

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
        if ( _imp->_gui->isTripleSyncEnabled() ) {
            _imp->updateCurveWidgetFrameRange();
            _imp->_gui->centerOpenedViewersOn( _imp->zoomCtx.left(), _imp->zoomCtx.right() );
        }
    } else if ( buttonDownIsLeft(e) ) {
        _imp->onMouseLeftButtonDrag(e);
    } else if ( buttonDownIsMiddle(e) ) {
        double dx = _imp->zoomCtx.toZoomCoordinates( _imp->_lastMousePos.x(),  _imp->_lastMousePos.y() ).x() - mouseZoomCoords.x();
        QMutexLocker k(&_imp->zoomCtxMutex);
        _imp->zoomCtx.translate(dx, 0);

        redraw();

        // Synchronize the curve editor and opened viewers
        if ( _imp->_gui->isTripleSyncEnabled() ) {
            _imp->updateCurveWidgetFrameRange();
            _imp->_gui->centerOpenedViewersOn( _imp->zoomCtx.left(), _imp->zoomCtx.right() );
        }
    } else {
        caught = false;
    }

    _imp->_lastMousePos = e->pos();
    if (!caught) {
        AnimationModulePtr module = toAnimationModule(_imp->_model.lock());
        if (module) {
            TabWidget* tab = 0;
            tab = module->getEditor()->getParentPane() ;
            if (tab) {
                // If the Viewer is in a tab, send the tab widget the event directly
                qApp->sendEvent(tab, e);
            } else {
                QGLWidget::mouseMoveEvent(e);
            }
        }
    }
} // DopeSheetView::mouseMoveEvent

void
DopeSheetView::mouseReleaseEvent(QMouseEvent *e)
{
    Q_UNUSED(e);

    bool mustRedraw = false;

    if (_imp->eventState == DopeSheetView::eEventStateSelectionByRect) {
        if ( !_imp->selectionRect.isNull() ) {
            AnimItemDimViewKeyFramesMap selectedKeys;
            std::vector<NodeAnimPtr > nodesSelection;
            std::vector<TableItemAnimPtr> tableItemSelection;
            _imp->createSelectionFromRect(_imp->selectionRect, &selectedKeys, &nodesSelection, &tableItemSelection);

            AnimationModuleSelectionModel::SelectionTypeFlags sFlags = ( modCASIsShift(e) )
                                                                 ? AnimationModuleSelectionModel::SelectionTypeToggle
                                                                 : AnimationModuleSelectionModel::SelectionTypeAdd;

            _imp->_model.lock()->getSelectionModel()->makeSelection(selectedKeys, tableItemSelection, nodesSelection, sFlags);

            _imp->computeSelectedKeysBRect();
        }

        _imp->selectionRect.clear();

        mustRedraw = true;
    }

    if ( _imp->_gui->isDraftRenderEnabled() ) {
        _imp->_gui->setDraftRenderEnabled(false);
        bool autoProxyEnabled = appPTR->getCurrentSettings()->isAutoProxyEnabled();
        if (autoProxyEnabled) {
            _imp->_gui->renderAllViewers(true);
        }
    }

    _imp->currentEditedReader.reset();
    _imp->eventState = eEventStateNoEditingState;

    if (mustRedraw) {
        redraw();
    }
} // DopeSheetView::mouseReleaseEvent

void
DopeSheetView::mouseDoubleClickEvent(QMouseEvent *e)
{
    running_in_main_thread();

    if ( modCASIsControl(e) ) {

        AnimationModuleBasePtr model = _imp->_model.lock();

        QTreeWidgetItem *itemUnderPoint = _imp->treeView->itemAt(TO_DPIX(5), e->pos().y());
        AnimatedItemTypeEnum foundType;
        KnobAnimPtr isKnob;
        TableItemAnimPtr isTableItem;
        NodeAnimPtr isNodeItem;
        ViewSetSpec view;
        DimSpec dim;
        bool found = model->findItem(itemUnderPoint, &foundType, &isKnob, &isTableItem, &isNodeItem, &view, &dim);
        (void)found;

        if (isKnob) {

            KnobStringBasePtr isStringKnob = toKnobStringBase(isKnob->getInternalKnob());
            if (isStringKnob) {
                // Cannot add keys on a string knob from the dopesheet
                return;
            }
            KeyFrameWithStringSet underMouse = _imp->isNearByKeyframe(isKnob, dim, view, e->pos());
            if (underMouse.empty()) {

                double keyframeTime = std::floor(_imp->zoomCtx.toZoomCoordinates(e->pos().x(), 0).x() + 0.5);
                _imp->moveCurrentFrameIndicator(keyframeTime);

                AnimItemDimViewKeyFramesMap keysToAdd;
                if (dim.isAll()) {
                    if (view.isAll()) {
                        std::list<ViewIdx> views = isKnob->getViewsList();
                        int nDims = isKnob->getNDimensions();
                        for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
                            for (int i = 0; i < nDims; ++i) {
                                AnimItemDimViewIndexID itemKey(isKnob, *it, DimIdx(i));
                                KeyFrameWithStringSet& keys = keysToAdd[itemKey];
                                KeyFrameWithString k;
                                double yCurve = isKnob->evaluateCurve(false /*useExpr*/, keyframeTime, DimIdx(i), *it);
                                k.key = KeyFrame(keyframeTime, yCurve);
                                keys.insert(k);
                            }
                        }
                    } else {
                        AnimItemDimViewIndexID itemKey(isKnob, ViewIdx(view), DimIdx(dim));
                        KeyFrameWithStringSet& keys = keysToAdd[itemKey];
                        KeyFrameWithString k;
                        double yCurve = isKnob->evaluateCurve(false /*useExpr*/, keyframeTime, DimIdx(dim), ViewIdx(view));
                        k.key = KeyFrame(keyframeTime, yCurve);
                        keys.insert(k);
                    }
                } else {
                    if (view.isAll()) {
                        std::list<ViewIdx> views = isKnob->getViewsList();
                        for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
                            AnimItemDimViewIndexID itemKey(isKnob, *it, DimIdx(dim));
                            KeyFrameWithStringSet& keys = keysToAdd[itemKey];
                            KeyFrameWithString k;
                            double yCurve = isKnob->evaluateCurve(false /*useExpr*/, keyframeTime, DimIdx(dim), *it);
                            k.key = KeyFrame(keyframeTime, yCurve);
                            keys.insert(k);

                        }
                    } else {
                        AnimItemDimViewIndexID itemKey(isKnob, ViewIdx(view), DimIdx(dim));
                        KeyFrameWithStringSet& keys = keysToAdd[itemKey];
                        KeyFrameWithString k;
                        double yCurve = isKnob->evaluateCurve(false /*useExpr*/, keyframeTime, DimIdx(dim), ViewIdx(view));
                        k.key = KeyFrame(keyframeTime, yCurve);
                        keys.insert(k);
                    }

                }

                model->pushUndoCommand( new AddKeysCommand(keysToAdd, model, false /*clearAndAdd*/) );
                
                
            } // !underMouse.empty()
        } // isKnob
    } // isCtrl
} // mouseDoubleClickEvent

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
    if ( _imp->_gui->isTripleSyncEnabled() ) {
        _imp->updateCurveWidgetFrameRange();
        _imp->_gui->centerOpenedViewersOn( _imp->zoomCtx.left(), _imp->zoomCtx.right() );
    }
}


NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_DopeSheetView.cpp"
