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
#include "Engine/ViewIdx.h"

#include "Global/Enums.h"

#include "Gui/ActionShortcuts.h"
#include "Gui/CurveEditor.h"
#include "Gui/CurveWidget.h"
#include "Gui/DockablePanel.h"
#include "Gui/AnimationModule.h"
#include "Gui/AnimationModuleEditor.h"
#include "Gui/AnimationModuleTreeView.h"
#include "Gui/DopeSheetHierarchyView.h"
#include "Gui/Gui.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiDefines.h"
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
#include "Gui/TabWidget.h"

#define NATRON_DOPESHEET_MIN_RANGE_FIT 10

NATRON_NAMESPACE_ENTER;


NATRON_NAMESPACE_ANONYMOUS_ENTER

//Protect declarations in an anonymous namespace

// Typedefs
typedef std::set<double> TimeSet;
typedef std::pair<double, double> FrameRange;
typedef std::map<KnobIWPtr, KnobGui *> KnobsAndGuis;


// Constants
static const int KF_TEXTURES_COUNT = 18;
static const int KF_PIXMAP_SIZE = 14;
static const int KF_X_OFFSET = KF_PIXMAP_SIZE / 2;
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

class DopeSheetViewPrivate
{
    Q_DECLARE_TR_FUNCTIONS(DopeSheetView)

public:
    enum KeyframeTexture
    {
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

        kfTextureMaster,
        kfTextureMasterSelected,
    };

    DopeSheetViewPrivate(DopeSheetView *publicInterface);
    ~DopeSheetViewPrivate();

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

    std::vector<AnimItemDimViewAndTime> isNearByKeyframe(const KnobAnimPtr &KnobAnim, const QPointF &widgetCoords) const;
    std::vector<AnimItemDimViewAndTime> isNearByKeyframe(NodeAnimPtr NodeAnim, const QPointF &widgetCoords) const;

    double clampedMouseOffset(double fromTime, double toTime);

    // Textures
    void generateKeyframeTextures();
    DopeSheetViewPrivate::KeyframeTexture kfTextureFromKeyframeType(KeyframeTypeEnum kfType, bool selected) const;

    // Drawing
    void drawScale() const;

    void drawRows() const;

    void drawNodeRow(const NodeAnimPtr NodeAnim) const;
    void drawKnobRow(const KnobAnimPtr KnobAnim) const;

    void drawNodeRowSeparation(const NodeAnimPtr NodeAnim) const;

    void drawRange(const NodeAnimPtr &NodeAnim) const;
    void drawKeyframes(const NodeAnimPtr &NodeAnim) const;

    void drawTexturedKeyframe(DopeSheetViewPrivate::KeyframeTexture textureType,
                              bool drawTime,
                              double time,
                              const QColor& textColor,
                              const RectD &rect) const;

    void drawGroupOverlay(const NodeAnimPtr &NodeAnim, const NodeAnimPtr &group) const;

    void drawProjectBounds() const;
    void drawCurrentFrameIndicator();

    void drawSelectionRect() const;

    void drawSelectedKeysBRect() const;

    void renderText(double x, double y,
                    const QString &text,
                    const QColor &color,
                    const QFont &font,
                    int flags = 0) const; //!< see http://doc.qt.io/qt-4.8/qpainter.html#drawText-10

    // After or during an user interaction
    void computeTimelinePositions();
    void computeSelectionRect(const QPointF &origin, const QPointF &current);

    void computeSelectedKeysBRect();

    void computeRangesBelow(const NodeAnimPtr& NodeAnim);
    void computeNodeRange(const NodeAnimPtr& NodeAnim);
    void computeReaderRange(const NodeAnimPtr& reader);
    void computeRetimeRange(const NodeAnimPtr& retimer);
    void computeTimeOffsetRange(const NodeAnimPtr& timeOffset);
    void computeFRRange(const NodeAnimPtr& frameRange);
    void computeGroupRange(const NodeAnimPtr& group);

    // User interaction
    void onMouseLeftButtonDrag(QMouseEvent *e);

    void createSelectionFromRect(const RectD &rect, std::vector<AnimItemDimViewAndTime> *result, std::vector<NodeAnimPtr >* selectedNodes);

    void moveCurrentFrameIndicator(double dt);

    void createContextMenu();

    void updateCurveWidgetFrameRange();

    /* attributes */
    DopeSheetView *publicInterface;
    AnimationModuleBaseWPtr model;
    AnimationModuleTreeView *treeView;
    Gui *gui;

    // necessary to retrieve some useful values for drawing
    TimeLinePtr timeline;

    //
    std::map<NodeAnimPtr, FrameRange> nodeRanges;
    std::list<NodeAnimPtr> nodeRangesBeingComputed; // to avoid recursion in groups
    int rangeComputationRecursion;

    // for rendering
    QFont *font;
    TextRenderer textRenderer;

    // for textures
    GLuint kfTexturesIDs[KF_TEXTURES_COUNT];

    // for navigating
    mutable QMutex zoomContextMutex;
    ZoomContext zoomContext;
    bool zoomOrPannedSinceLastFit;

    // for current time indicator
    QPolygonF currentFrameIndicatorBottomPoly;

    // for keyframe selection
    // This is in zoom coordinates...
    RectD selectionRect;

    // keyframe selection rect
    // This is in zoom coordinates
    RectD selectedKeysBRect;

    // for various user interaction
    QPointF lastPosOnMousePress;
    QPointF lastPosOnMouseMove;
    double keyDragLastMovement;
    DopeSheetView::EventStateEnum eventState;

    // for clip (Reader, Time nodes) user interaction
    NodeAnimPtr currentEditedReader;


    // UI
    Menu *contextMenu;

    // True if paintGL was run at least once
    bool drawnOnce;
};

DopeSheetViewPrivate::DopeSheetViewPrivate(DopeSheetView *publicInterface)
    : publicInterface(publicInterface)
    , model()
    , treeView(0)
    , gui(0)
    , timeline()
    , nodeRanges()
    , nodeRangesBeingComputed()
    , rangeComputationRecursion(0)
    , font( new QFont(appFont, appFontSize) )
    , textRenderer()
    , kfTexturesIDs()
    , zoomContext()
    , zoomOrPannedSinceLastFit(false)
    , selectionRect()
    , selectedKeysBRect()
    , lastPosOnMousePress()
    , lastPosOnMouseMove()
    , keyDragLastMovement()
    , eventState(DopeSheetView::esNoEditingState)
    , currentEditedReader()
    , contextMenu( new Menu(publicInterface) )
    , drawnOnce(false)
{
}

DopeSheetViewPrivate::~DopeSheetViewPrivate()
{
    GL_GPU::glDeleteTextures(KF_TEXTURES_COUNT, kfTexturesIDs);
}

/*
   This function is just wrong and confusing, it takes keyTime which is in zoom Coordinates  and y which is in widget coordinates
   Do not uncomment
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
 */

RectD
DopeSheetViewPrivate::getKeyFrameBoundingRectZoomCoords(double keyframeTimeZoomCoords,
                                                        double keyframeYCenterWidgetCoords) const
{
    double keyframeTimeWidget = zoomContext.toWidgetCoordinates(keyframeTimeZoomCoords, 0).x();
    RectD ret;
    QPointF x1y1 = zoomContext.toZoomCoordinates(keyframeTimeWidget - KF_X_OFFSET, keyframeYCenterWidgetCoords + KF_PIXMAP_SIZE / 2);
    QPointF x2y2 = zoomContext.toZoomCoordinates(keyframeTimeWidget + KF_X_OFFSET, keyframeYCenterWidgetCoords - KF_PIXMAP_SIZE / 2);

    ret.x1 = x1y1.x();
    ret.y1 = x1y1.y();
    ret.x2 = x2y2.x();
    ret.y2 = x2y2.y();

    return ret;
}

/**
 * @brief Converts the given (x,y) coordinates which are in OpenGL canonical coordinates to widget coordinates.
 **/
void
DopeSheetView::toWidgetCoordinates(double *x,
                                   double *y) const
{
    QPointF p = _imp->zoomContext.toWidgetCoordinates(*x, *y);

    *x = p.x();
    *y = p.y();
}

/**
 * @brief Converts the given (x,y) coordinates which are in widget coordinates to OpenGL canonical coordinates
 **/
void
DopeSheetView::toCanonicalCoordinates(double *x,
                                      double *y) const
{
    QPointF p = _imp->zoomContext.toZoomCoordinates(*x, *y);

    *x = p.x();
    *y = p.y();
}

/**
 * @brief Returns the font height, i.e: the height of the highest letter for this font
 **/
int
DopeSheetView::getWidgetFontHeight() const
{
    return fontMetrics().height();
}

/**
 * @brief Returns for a string the estimated pixel size it would take on the widget
 **/
int
DopeSheetView::getStringWidthForCurrentFont(const std::string& string) const
{
    return fontMetrics().width( QString::fromUtf8( string.c_str() ) );
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

    topLeft = zoomContext.toZoomCoordinates( topLeft.x(), topLeft.y() );
    RectD ret;
    ret.x1 = topLeft.x();
    ret.y2 = topLeft.y();
    QPointF bottomRight = rect.bottomRight();
    bottomRight = zoomContext.toZoomCoordinates( bottomRight.x(), bottomRight.y() );
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
    QPointF topLeft = zoomContext.toWidgetCoordinates(rect.x1, rect.y2);
    QPointF bottomRight = zoomContext.toWidgetCoordinates(rect.x2, rect.y1);

    return QRectF( topLeft.x(), topLeft.y(), bottomRight.x() - topLeft.x(), bottomRight.y() - topLeft.y() );
}

QRectF
DopeSheetViewPrivate::nameItemRectToRowRect(const QRectF &rect) const
{
    QPointF topLeft = rect.topLeft();

    topLeft = zoomContext.toZoomCoordinates( topLeft.x(), topLeft.y() );
    QPointF bottomRight = rect.bottomRight();
    bottomRight = zoomContext.toZoomCoordinates( bottomRight.x(), bottomRight.y() );

    return QRectF( QPointF( zoomContext.left(), topLeft.y() ),
                   QPointF( zoomContext.right(), bottomRight.y() ) );
}

Qt::CursorShape
DopeSheetViewPrivate::getCursorDuringHover(const QPointF &widgetCoords) const
{
    QPointF clickZoomCoords = zoomContext.toZoomCoordinates( widgetCoords.x(), widgetCoords.y() );

    if ( isNearbySelectedKeysBRec(widgetCoords) ) {
        return getCursorForEventState(DopeSheetView::esMoveKeyframeSelection);
    } else if ( isNearbySelectedKeysBRectRightEdge(widgetCoords) ) {
        return getCursorForEventState(DopeSheetView::esTransformingKeyframesMiddleRight);
    } else if ( isNearbySelectedKeysBRectLeftEdge(widgetCoords) ) {
        return getCursorForEventState(DopeSheetView::esTransformingKeyframesMiddleLeft);
    } else if ( isNearByCurrentFrameIndicatorBottom(clickZoomCoords) ) {
        return getCursorForEventState(DopeSheetView::esMoveCurrentFrameIndicator);
    }

    ///Look for a range node
    AnimTreeItemNodeMap nodes = model->getItemNodeMap();
    for (AnimTreeItemNodeMap::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        if ( it->second->isRangeDrawingEnabled() ) {
            std::map<NodeAnimPtr, FrameRange >::const_iterator foundRange = nodeRanges.find( it->second );
            if ( foundRange == nodeRanges.end() ) {
                continue;
            }

            QRectF treeItemRect = hierarchyView->visualItemRect(it->first);
            const FrameRange& range = foundRange->second;
            RectD nodeClipRect;
            nodeClipRect.x1 = range.first;
            nodeClipRect.x2 = range.second;
            nodeClipRect.y2 = zoomContext.toZoomCoordinates( 0, treeItemRect.top() ).y();
            nodeClipRect.y1 = zoomContext.toZoomCoordinates( 0, treeItemRect.bottom() ).y();

            AnimatedItemType nodeType = it->second->getItemType();
            if ( nodeClipRect.contains( clickZoomCoords.x(), clickZoomCoords.y() ) ) {
                if ( (nodeType == eAnimatedItemTypeGroup) ||
                     ( nodeType == eAnimatedItemTypeReader) ||
                     ( nodeType == eAnimatedItemTypeTimeOffset) ||
                     ( nodeType == eAnimatedItemTypeFrameRange) ) {
                    return getCursorForEventState(DopeSheetView::esMoveKeyframeSelection);
                }
            }
            if (nodeType == eAnimatedItemTypeReader) {
                if ( isNearByClipRectLeft(clickZoomCoords, nodeClipRect) ) {
                    return getCursorForEventState(DopeSheetView::esReaderLeftTrim);
                } else if ( isNearByClipRectRight(clickZoomCoords, nodeClipRect) ) {
                    return getCursorForEventState(DopeSheetView::esReaderRightTrim);
                } else if ( model->canSlipReader(it->second) && isNearByClipRectBottom(clickZoomCoords, nodeClipRect) ) {
                    return getCursorForEventState(DopeSheetView::esReaderSlip);
                }
            }
        }
    } // for (AnimTreeItemNodeMap::iterator it = nodes.begin(); it!=nodes.end(); ++it) {

    QTreeWidgetItem *treeItem = hierarchyView->itemAt( 0, widgetCoords.y() );
    //Did not find a range node, look for keyframes
    if (treeItem) {
        AnimTreeItemNodeMap NodeAnimItems = model->getItemNodeMap();
        AnimTreeItemNodeMap::const_iterator foundNodeAnim = NodeAnimItems.find(treeItem);
        if ( foundNodeAnim != NodeAnimItems.end() ) {
            const NodeAnimPtr &NodeAnim = (*foundNodeAnim).second;
            AnimatedItemType nodeType = NodeAnim->getItemType();
            if (nodeType == eAnimatedItemTypeCommon) {
                std::vector<AnimKeyFrame> keysUnderMouse = isNearByKeyframe(NodeAnim, widgetCoords);

                if ( !keysUnderMouse.empty() ) {
                    return getCursorForEventState(DopeSheetView::esPickKeyframe);
                }
            }
        } else { // if (foundNodeAnim != NodeAnimItems.end()) {
            //We may be on a knob row
            KnobAnimPtr KnobAnim = hierarchyView->getKnobAnimAt( widgetCoords.y() );
            if (KnobAnim) {
                std::vector<AnimKeyFrame> keysUnderMouse = isNearByKeyframe(KnobAnim, widgetCoords);

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
    QPointF widgetPos = zoomContext.toWidgetCoordinates( zoomCoordPos.x(), zoomCoordPos.y() );
    QPointF rectx1y1 = zoomContext.toWidgetCoordinates( clipRect.left(), clipRect.bottom() );
    QPointF rectx2y2 = zoomContext.toWidgetCoordinates( clipRect.right(), clipRect.top() );

    return ( (widgetPos.x() >= rectx1y1.x() - DISTANCE_ACCEPTANCE_FROM_READER_EDGE) &&
             ( widgetPos.x() <= rectx1y1.x() ) &&
             (widgetPos.y() <= rectx1y1.y() + DISTANCE_ACCEPTANCE_FROM_READER_EDGE) &&
             (widgetPos.y() >= rectx2y2.y() - DISTANCE_ACCEPTANCE_FROM_READER_EDGE) );
}

bool
DopeSheetViewPrivate::isNearByClipRectRight(const QPointF& zoomCoordPos,
                                            const RectD &clipRect) const
{
    QPointF widgetPos = zoomContext.toWidgetCoordinates( zoomCoordPos.x(), zoomCoordPos.y() );
    QPointF rectx1y1 = zoomContext.toWidgetCoordinates( clipRect.left(), clipRect.bottom() );
    QPointF rectx2y2 = zoomContext.toWidgetCoordinates( clipRect.right(), clipRect.top() );

    return ( ( widgetPos.x() >= rectx2y2.x() ) &&
             (widgetPos.x() <= rectx2y2.x() + DISTANCE_ACCEPTANCE_FROM_READER_EDGE) &&
             (widgetPos.y() <= rectx1y1.y() + DISTANCE_ACCEPTANCE_FROM_READER_EDGE) &&
             (widgetPos.y() >= rectx2y2.y() - DISTANCE_ACCEPTANCE_FROM_READER_EDGE) );
}

bool
DopeSheetViewPrivate::isNearByClipRectBottom(const QPointF& zoomCoordPos,
                                             const RectD &clipRect) const
{
    QPointF widgetPos = zoomContext.toWidgetCoordinates( zoomCoordPos.x(), zoomCoordPos.y() );
    QPointF rectx1y1 = zoomContext.toWidgetCoordinates( clipRect.left(), clipRect.bottom() );
    QPointF rectx2y2 = zoomContext.toWidgetCoordinates( clipRect.right(), clipRect.top() );

    return ( (widgetPos.x() >= rectx1y1.x() - DISTANCE_ACCEPTANCE_FROM_READER_BOTTOM) &&
             (widgetPos.x() <= rectx2y2.x() + DISTANCE_ACCEPTANCE_FROM_READER_EDGE) &&
             (widgetPos.y() <= rectx1y1.y() + DISTANCE_ACCEPTANCE_FROM_READER_BOTTOM) &&
             (widgetPos.y() >= rectx1y1.y() - DISTANCE_ACCEPTANCE_FROM_READER_BOTTOM) );
}

bool
DopeSheetViewPrivate::isNearByCurrentFrameIndicatorBottom(const QPointF &zoomCoords) const
{
    return ( currentFrameIndicatorBottomPoly.containsPoint(zoomCoords, Qt::OddEvenFill) );
}

bool
DopeSheetViewPrivate::isNearbySelectedKeysBRectRightEdge(const QPointF& widgetPos) const
{
    QPointF topLeft = zoomContext.toWidgetCoordinates(selectedKeysBRect.x1, selectedKeysBRect.y2);
    QPointF bottomRight = zoomContext.toWidgetCoordinates(selectedKeysBRect.x2, selectedKeysBRect.y1);

    return ( widgetPos.x() >= ( bottomRight.x() ) &&
             widgetPos.x() <= (bottomRight.x() + DISTANCE_ACCEPTANCE_FROM_READER_EDGE) &&
             widgetPos.y() <= (bottomRight.y() + DISTANCE_ACCEPTANCE_FROM_READER_EDGE) &&
             widgetPos.y() >= (topLeft.y() - DISTANCE_ACCEPTANCE_FROM_READER_EDGE) );
}

bool
DopeSheetViewPrivate::isNearbySelectedKeysBRectLeftEdge(const QPointF& widgetPos) const
{
    QPointF topLeft = zoomContext.toWidgetCoordinates(selectedKeysBRect.x1, selectedKeysBRect.y2);
    QPointF bottomRight = zoomContext.toWidgetCoordinates(selectedKeysBRect.x2, selectedKeysBRect.y1);

    return ( widgetPos.x() >= (topLeft.x() - DISTANCE_ACCEPTANCE_FROM_READER_EDGE) &&
             widgetPos.x() <= ( topLeft.x() ) &&
             widgetPos.y() <= (bottomRight.y() + DISTANCE_ACCEPTANCE_FROM_READER_EDGE) &&
             widgetPos.y() >= (topLeft.y() - DISTANCE_ACCEPTANCE_FROM_READER_EDGE) );
}

bool
DopeSheetViewPrivate::isNearbySelectedKeysBRec(const QPointF& widgetPos) const
{
    QPointF topLeft = zoomContext.toWidgetCoordinates(selectedKeysBRect.x1, selectedKeysBRect.y2);
    QPointF bottomRight = zoomContext.toWidgetCoordinates(selectedKeysBRect.x2, selectedKeysBRect.y1);

    return ( widgetPos.x() >= ( topLeft.x() ) &&
             widgetPos.x() <= ( bottomRight.x() ) &&
             widgetPos.y() <= (bottomRight.y() + DISTANCE_ACCEPTANCE_FROM_READER_EDGE) &&
             widgetPos.y() >= (topLeft.y() - DISTANCE_ACCEPTANCE_FROM_READER_EDGE) );
}

std::vector<AnimKeyFrame> DopeSheetViewPrivate::isNearByKeyframe(const KnobAnimPtr &KnobAnim,
                                                                 const QPointF &widgetCoords) const
{
    assert(KnobAnim);

    std::vector<AnimKeyFrame> ret;
    KnobIPtr knob = KnobAnim->getKnobGui()->getKnob();
    int dim = KnobAnim->getDimension();
    int startDim = 0;
    int endDim = knob->getDimension();

    if (dim > -1) {
        startDim = dim;
        endDim = dim + 1;
    }

    for (int i = startDim; i < endDim; ++i) {
        KeyFrameSet keyframes = knob->getCurve(ViewIdx(0), i)->getKeyFrames_mt_safe();

        for (KeyFrameSet::const_iterator kIt = keyframes.begin();
             kIt != keyframes.end();
             ++kIt) {
            KeyFrame kf = (*kIt);
            QPointF keyframeWidgetPos = zoomContext.toWidgetCoordinates(kf.getTime(), 0);

            if (std::abs( widgetCoords.x() - keyframeWidgetPos.x() ) < DISTANCE_ACCEPTANCE_FROM_KEYFRAME) {
                KnobAnimPtr context;
                if (dim == -1) {
                    QTreeWidgetItem *childItem = KnobAnim->findDimTreeItem(i);
                    context = model->mapNameItemToKnobAnim(childItem);
                } else {
                    context = KnobAnim;
                }

                ret.push_back( AnimKeyFrame(context, kf) );
            }
        }
    }

    return ret;
}

std::vector<AnimKeyFrame> DopeSheetViewPrivate::isNearByKeyframe(NodeAnimPtr NodeAnim,
                                                                 const QPointF &widgetCoords) const
{
    std::vector<AnimKeyFrame> ret;
    const AnimTreeItemKnobMap& KnobAnims = NodeAnim->getItemKnobMap();

    for (AnimTreeItemKnobMap::const_iterator it = KnobAnims.begin(); it != KnobAnims.end(); ++it) {
        KnobAnimPtr KnobAnim = (*it).second;
        KnobGuiPtr knobGui = KnobAnim->getKnobGui();
        assert(knobGui);
        int dim = KnobAnim->getDimension();

        if (dim == -1) {
            continue;
        }

        KeyFrameSet keyframes = knobGui->getCurve(ViewIdx(0), dim)->getKeyFrames_mt_safe();

        for (KeyFrameSet::const_iterator kIt = keyframes.begin();
             kIt != keyframes.end();
             ++kIt) {
            KeyFrame kf = (*kIt);
            QPointF keyframeWidgetPos = zoomContext.toWidgetCoordinates(kf.getTime(), 0);

            if (std::abs( widgetCoords.x() - keyframeWidgetPos.x() ) < DISTANCE_ACCEPTANCE_FROM_KEYFRAME) {
                ret.push_back( AnimKeyFrame(KnobAnim, kf) );
            }
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

void
DopeSheetViewPrivate::generateKeyframeTextures()
{
    QImage kfTexturesImages[KF_TEXTURES_COUNT];

    kfTexturesImages[0].load( QString::fromUtf8(NATRON_IMAGES_PATH "interp_constant.png") );
    kfTexturesImages[1].load( QString::fromUtf8(NATRON_IMAGES_PATH "interp_constant_selected.png") );
    kfTexturesImages[2].load( QString::fromUtf8(NATRON_IMAGES_PATH "interp_linear.png") );
    kfTexturesImages[3].load( QString::fromUtf8(NATRON_IMAGES_PATH "interp_linear_selected.png") );
    kfTexturesImages[4].load( QString::fromUtf8(NATRON_IMAGES_PATH "interp_curve.png") );
    kfTexturesImages[5].load( QString::fromUtf8(NATRON_IMAGES_PATH "interp_curve_selected.png") );
    kfTexturesImages[6].load( QString::fromUtf8(NATRON_IMAGES_PATH "interp_break.png") );
    kfTexturesImages[7].load( QString::fromUtf8(NATRON_IMAGES_PATH "interp_break_selected.png") );
    kfTexturesImages[8].load( QString::fromUtf8(NATRON_IMAGES_PATH "interp_curve_c.png") );
    kfTexturesImages[9].load( QString::fromUtf8(NATRON_IMAGES_PATH "interp_curve_c_selected.png") );
    kfTexturesImages[10].load( QString::fromUtf8(NATRON_IMAGES_PATH "interp_curve_h.png") );
    kfTexturesImages[11].load( QString::fromUtf8(NATRON_IMAGES_PATH "interp_curve_h_selected.png") );
    kfTexturesImages[12].load( QString::fromUtf8(NATRON_IMAGES_PATH "interp_curve_r.png") );
    kfTexturesImages[13].load( QString::fromUtf8(NATRON_IMAGES_PATH "interp_curve_r_selected.png") );
    kfTexturesImages[14].load( QString::fromUtf8(NATRON_IMAGES_PATH "interp_curve_z.png") );
    kfTexturesImages[15].load( QString::fromUtf8(NATRON_IMAGES_PATH "interp_curve_z_selected.png") );
    kfTexturesImages[16].load( QString::fromUtf8(NATRON_IMAGES_PATH "keyframe_node_root.png") );
    kfTexturesImages[17].load( QString::fromUtf8(NATRON_IMAGES_PATH "keyframe_node_root_selected.png") );

    GL_GPU::glGenTextures(KF_TEXTURES_COUNT, kfTexturesIDs);

    GL_GPU::glEnable(GL_TEXTURE_2D);

    for (int i = 0; i < KF_TEXTURES_COUNT; ++i) {
        if (std::max( kfTexturesImages[i].width(), kfTexturesImages[i].height() ) != KF_PIXMAP_SIZE) {
            kfTexturesImages[i] = kfTexturesImages[i].scaled(KF_PIXMAP_SIZE, KF_PIXMAP_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
        kfTexturesImages[i] = QGLWidget::convertToGLFormat(kfTexturesImages[i]);
        GL_GPU::glBindTexture(GL_TEXTURE_2D, kfTexturesIDs[i]);

        GL_GPU::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        GL_GPU::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        GL_GPU::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        GL_GPU::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        GL_GPU::glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, KF_PIXMAP_SIZE, KF_PIXMAP_SIZE, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, kfTexturesImages[i].bits() );
    }

    GL_GPU::glBindTexture(GL_TEXTURE_2D, 0);
    GL_GPU::glDisable(GL_TEXTURE_2D);
}

DopeSheetViewPrivate::KeyframeTexture
DopeSheetViewPrivate::kfTextureFromKeyframeType(KeyframeTypeEnum kfType,
                                                bool selected) const
{
    DopeSheetViewPrivate::KeyframeTexture ret = DopeSheetViewPrivate::kfTextureNone;

    switch (kfType) {
    case eKeyframeTypeConstant:
        ret = (selected) ? DopeSheetViewPrivate::kfTextureInterpConstantSelected : DopeSheetViewPrivate::kfTextureInterpConstant;
        break;
    case eKeyframeTypeLinear:
        ret = (selected) ? DopeSheetViewPrivate::kfTextureInterpLinearSelected : DopeSheetViewPrivate::kfTextureInterpLinear;
        break;
    case eKeyframeTypeBroken:
        ret = (selected) ? DopeSheetViewPrivate::kfTextureInterpBreakSelected : DopeSheetViewPrivate::kfTextureInterpBreak;
        break;
    case eKeyframeTypeFree:
        ret = (selected) ? DopeSheetViewPrivate::kfTextureInterpCurveSelected : DopeSheetViewPrivate::kfTextureInterpCurve;
        break;
    case eKeyframeTypeSmooth:
        ret = (selected) ? DopeSheetViewPrivate::kfTextureInterpCurveZSelected : DopeSheetViewPrivate::kfTextureInterpCurveZ;
        break;
    case eKeyframeTypeCatmullRom:
        ret = (selected) ? DopeSheetViewPrivate::kfTextureInterpCurveRSelected : DopeSheetViewPrivate::kfTextureInterpCurveR;
        break;
    case eKeyframeTypeCubic:
        ret = (selected) ? DopeSheetViewPrivate::kfTextureInterpCurveCSelected : DopeSheetViewPrivate::kfTextureInterpCurveC;
        break;
    case eKeyframeTypeHorizontal:
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
void
DopeSheetViewPrivate::drawScale() const
{
    running_in_main_thread_and_context(publicInterface);

    QPointF bottomLeft = zoomContext.toZoomCoordinates(0, publicInterface->height() - 1);
    QPointF topRight = zoomContext.toZoomCoordinates(publicInterface->width() - 1, 0);

    // Don't attempt to draw a scale on a widget with an invalid height
    if (publicInterface->height() <= 1) {
        return;
    }

    QFontMetrics fontM(*font);
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

        const double rangePixel = publicInterface->width();
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

                    renderText(value, bottomLeft.y(), s, c, *font, Qt::AlignHCenter);

                    // Uncomment the line below to draw the indicator on top too
                    // parent->renderText(value, topRight.y() - 20, s, c, *font);
                }
            }
        }
    }
} // DopeSheetViewPrivate::drawScale

/**
 * @brief DopeSheetViewPrivate::drawRows
 *
 *
 *
 * These rows have the same height as an item from the hierarchy view.
 */
void
DopeSheetViewPrivate::drawRows() const
{
    running_in_main_thread_and_context(publicInterface);

    AnimTreeItemNodeMap treeItemsAndNodeAnims = model->getItemNodeMap();

    // Perform drawing
    {
        GLProtectAttrib<GL_GPU> a(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT);

        for (AnimTreeItemNodeMap::const_iterator it = treeItemsAndNodeAnims.begin();
             it != treeItemsAndNodeAnims.end();
             ++it) {
            QTreeWidgetItem *treeItem = (*it).first;

            if ( treeItem->isHidden() ) {
                continue;
            }

            if ( QTreeWidgetItem * parentItem = treeItem->parent() ) {
                if ( !parentItem->isExpanded() ) {
                    continue;
                }
            }

            GL_GPU::glEnable(GL_BLEND);
            GL_GPU::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            NodeAnimPtr NodeAnim = (*it).second;

            drawNodeRow(NodeAnim);

            const AnimTreeItemKnobMap& knobItems = NodeAnim->getItemKnobMap();
            for (AnimTreeItemKnobMap::const_iterator it2 = knobItems.begin();
                 it2 != knobItems.end();
                 ++it2) {
                KnobAnimPtr KnobAnim = (*it2).second;

                drawKnobRow(KnobAnim);
            }

            {
                NodeAnimPtr group = model->getGroupNodeAnim( NodeAnim );
                if (group) {
                    drawGroupOverlay(NodeAnim, group);
                }
            }

            AnimatedItemType nodeType = NodeAnim->getItemType();

            if ( NodeAnim->isRangeDrawingEnabled() ) {
                drawRange(NodeAnim);
            }

            if (nodeType != eAnimatedItemTypeGroup) {
                drawKeyframes(NodeAnim);
            }
        }

        // Draw node rows separations
        for (AnimTreeItemNodeMap::const_iterator it = treeItemsAndNodeAnims.begin();
             it != treeItemsAndNodeAnims.end();
             ++it) {
            NodeAnimPtr NodeAnim = (*it).second;
            bool isTreeViewTopItem = !hierarchyView->itemAbove( NodeAnim->getTreeItem() );
            if (!isTreeViewTopItem) {
                drawNodeRowSeparation(NodeAnim);
            }
        }
    }
} // DopeSheetViewPrivate::drawRows

/**
 * @brief DopeSheetViewPrivate::drawNodeRow
 *
 *
 */
void
DopeSheetViewPrivate::drawNodeRow(const NodeAnimPtr NodeAnim) const
{
    GLProtectAttrib<GL_GPU> a(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT);
    QRectF nameItemRect = hierarchyView->visualItemRect( NodeAnim->getTreeItem() );
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
}

/**
 * @brief DopeSheetViewPrivate::drawKnobRow
 *
 *
 */
void
DopeSheetViewPrivate::drawKnobRow(const KnobAnimPtr KnobAnim) const
{
    if ( KnobAnim->getTreeItem()->isHidden() ) {
        return;
    }

    GLProtectAttrib<GL_GPU> a(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT);
    QRectF nameItemRect = hierarchyView->visualItemRect( KnobAnim->getTreeItem() );
    QRectF rowRect = nameItemRectToRowRect(nameItemRect);
    SettingsPtr settings = appPTR->getCurrentSettings();
    double bkR, bkG, bkB, bkA;
    if ( KnobAnim->isMultiDimRoot() ) {
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
}

void
DopeSheetViewPrivate::drawNodeRowSeparation(const NodeAnimPtr NodeAnim) const
{
    GLProtectAttrib<GL_GPU> a(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT | GL_LINE_BIT);
    QRectF nameItemRect = hierarchyView->visualItemRect( NodeAnim->getTreeItem() );
    QRectF rowRect = nameItemRectToRowRect(nameItemRect);

    GL_GPU::glLineWidth(TO_DPIY(4));
    GL_GPU::glColor4f(0.f, 0.f, 0.f, 1.f);

    GL_GPU::glBegin(GL_LINES);
    GL_GPU::glVertex2f( rowRect.left(), rowRect.top() );
    GL_GPU::glVertex2f( rowRect.right(), rowRect.top() );
    GL_GPU::glEnd();
}

void
DopeSheetViewPrivate::drawRange(const NodeAnimPtr &NodeAnim) const
{
    // Draw the clip
    {
        std::map<NodeAnimPtr, FrameRange >::const_iterator foundRange = nodeRanges.find( NodeAnim );
        if ( foundRange == nodeRanges.end() ) {
            return;
        }


        const FrameRange& range = foundRange->second;
        QRectF treeItemRect = hierarchyView->visualItemRect( NodeAnim->getTreeItem() );
        QPointF treeRectTopLeft = treeItemRect.topLeft();
        treeRectTopLeft = zoomContext.toZoomCoordinates( treeRectTopLeft.x(), treeRectTopLeft.y() );
        QPointF treeRectBtmRight = treeItemRect.bottomRight();
        treeRectBtmRight = zoomContext.toZoomCoordinates( treeRectBtmRight.x(), treeRectBtmRight.y() );

        RectD clipRectZoomCoords;
        clipRectZoomCoords.x1 = range.first;
        clipRectZoomCoords.x2 = range.second;
        clipRectZoomCoords.y2 = treeRectTopLeft.y();
        clipRectZoomCoords.y1 = treeRectBtmRight.y();
        GLProtectAttrib<GL_GPU> a(GL_CURRENT_BIT);
        QColor fillColor = NodeAnim->getNodeGui()->getCurrentColor();
        fillColor = QColor::fromHsl( fillColor.hslHue(), 50, fillColor.lightness() );

        bool isSelected = model->getSelectionModel()->isRangeNodeSelected(NodeAnim);


        // If necessary, draw the original frame range line
        float clipRectCenterY = 0.;
        if ( isSelected && (NodeAnim->getItemType() == eAnimatedItemTypeReader) ) {
            NodePtr node = NodeAnim->getInternalNode();

            KnobIntBase *firstFrameKnob = dynamic_cast<KnobIntBase *>( node->getKnobByName(kReaderParamNameFirstFrame).get() );
            assert(firstFrameKnob);

            double speedValue = 1.0f;

            KnobIntBase *originalFrameRangeKnob = dynamic_cast<KnobIntBase *>( node->getKnobByName(kReaderParamNameOriginalFrameRange).get() );
            assert(originalFrameRangeKnob);

            int originalFirstFrame = originalFrameRangeKnob->getValue(0);
            int originalLastFrame = originalFrameRangeKnob->getValue(1);
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

        if ( isSelected && (NodeAnim->getItemType() == eAnimatedItemTypeReader) ) {
            SettingsPtr settings = appPTR->getCurrentSettings();
            double selectionColorRGB[3];
            settings->getSelectionColor(&selectionColorRGB[0], &selectionColorRGB[1], &selectionColorRGB[2]);
            QColor selectionColor;
            selectionColor.setRgbF(selectionColorRGB[0], selectionColorRGB[1], selectionColorRGB[2]);

            QFontMetrics fm(*font);
            int fontHeigt = fm.height();
            QString leftText = QString::number(range.first);
            QString rightText = QString::number(range.second - 1);
            int rightTextW = fm.width(rightText);
            QPointF textLeftPos( zoomContext.toZoomCoordinates(zoomContext.toWidgetCoordinates(range.first, 0).x() + 3, 0).x(),
                                 zoomContext.toZoomCoordinates(0, zoomContext.toWidgetCoordinates(0, clipRectCenterY).y() + fontHeigt / 2.).y() );

            renderText(textLeftPos.x(), textLeftPos.y(), leftText, selectionColor, *font);

            QPointF textRightPos( zoomContext.toZoomCoordinates(zoomContext.toWidgetCoordinates(range.second, 0).x() - rightTextW - 3, 0).x(),
                                  zoomContext.toZoomCoordinates(0, zoomContext.toWidgetCoordinates(0, clipRectCenterY).y() + fontHeigt / 2.).y() );

            renderText(textRightPos.x(), textRightPos.y(), rightText, selectionColor, *font);
        }
    }
} // DopeSheetViewPrivate::drawRange

/**
 * @brief DopeSheetViewPrivate::drawKeyframes
 *
 *
 */
void
DopeSheetViewPrivate::drawKeyframes(const NodeAnimPtr &NodeAnim) const
{
    running_in_main_thread_and_context(publicInterface);

    SettingsPtr settings = appPTR->getCurrentSettings();
    double selectionColorRGB[3];
    settings->getSelectionColor(&selectionColorRGB[0], &selectionColorRGB[1], &selectionColorRGB[2]);
    QColor selectionColor;
    selectionColor.setRgbF(selectionColorRGB[0], selectionColorRGB[1], selectionColorRGB[2]);

    // Perform drawing
    {
        GLProtectAttrib<GL_GPU> a(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT);

        GL_GPU::glEnable(GL_BLEND);
        GL_GPU::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        const AnimTreeItemKnobMap& knobItems = NodeAnim->getItemKnobMap();
        double kfTimeSelected;
        int hasSingleKfTimeSelected = model->getSelectionModel()->hasSingleKeyFrameTimeSelected(&kfTimeSelected);
        std::map<double, bool> nodeKeytimes;
        std::map<KnobAnim *, std::map<double, bool> > knobsKeytimes;

        for (AnimTreeItemKnobMap::const_iterator it = knobItems.begin();
             it != knobItems.end();
             ++it) {
            KnobAnimPtr KnobAnim = (*it).second;
            QTreeWidgetItem *knobTreeItem = KnobAnim->getTreeItem();

            // The knob is no longer animated
            if ( knobTreeItem->isHidden() ) {
                continue;
            }

            int dim = KnobAnim->getDimension();

            if (dim == -1) {
                continue;
            }

            KeyFrameSet keyframes = KnobAnim->getKnobGui()->getCurve(ViewIdx(0), dim)->getKeyFrames_mt_safe();

            for (KeyFrameSet::const_iterator kIt = keyframes.begin();
                 kIt != keyframes.end();
                 ++kIt) {
                KeyFrame kf = (*kIt);
                double keyTime = kf.getTime();

                // Clip keyframes horizontally //TODO Clip vertically too
                if ( ( keyTime < zoomContext.left() ) || ( keyTime > zoomContext.right() ) ) {
                    continue;
                }

                double rowCenterYWidget = hierarchyView->visualItemRect( KnobAnim->getTreeItem() ).center().y();
                RectD zoomKfRect = getKeyFrameBoundingRectZoomCoords(keyTime, rowCenterYWidget);
                bool kfSelected = model->getSelectionModel()->keyframeIsSelected(KnobAnim, kf);

                // Draw keyframe in the knob dim row only if it's visible
                bool drawInDimRow = hierarchyView->itemIsVisibleFromOutside(knobTreeItem);

                if (drawInDimRow) {
                    DopeSheetViewPrivate::KeyframeTexture texType = kfTextureFromKeyframeType( kf.getInterpolation(),
                                                                                               kfSelected || selectionRect.intersects(zoomKfRect) );

                    if (texType != DopeSheetViewPrivate::kfTextureNone) {
                        drawTexturedKeyframe(texType, hasSingleKfTimeSelected && kfSelected,
                                             kfTimeSelected, selectionColor, zoomKfRect);
                    }
                }

                // Fill the knob times map
                {
                    KnobAnimPtr rootKnobAnim = model->mapNameItemToKnobAnim( knobTreeItem->parent() );
                    if (rootKnobAnim) {
                        assert(rootKnobAnim);
                        const std::map<double, bool>& map = knobsKeytimes[rootKnobAnim.get()];
                        bool knobTimeExists = ( map.find(keyTime) != map.end() );

                        if (!knobTimeExists) {
                            knobsKeytimes[rootKnobAnim.get()][keyTime] = kfSelected;
                        } else {
                            bool knobTimeIsSelected = knobsKeytimes[rootKnobAnim.get()][keyTime];

                            if (!knobTimeIsSelected && kfSelected) {
                                knobsKeytimes[rootKnobAnim.get()][keyTime] = true;
                            }
                        }
                    }
                }

                // Fill the node times map
                bool nodeTimeExists = ( nodeKeytimes.find(keyTime) != nodeKeytimes.end() );

                if (!nodeTimeExists) {
                    nodeKeytimes[keyTime] = kfSelected;
                } else {
                    bool nodeTimeIsSelected = nodeKeytimes[keyTime];

                    if (!nodeTimeIsSelected && kfSelected) {
                        nodeKeytimes[keyTime] = true;
                    }
                }
            }
        }

        // Draw master keys in knob root section
        for (std::map<KnobAnim *, std::map<double, bool> >::const_iterator it = knobsKeytimes.begin();
             it != knobsKeytimes.end();
             ++it) {
            QTreeWidgetItem *knobRootItem = (*it).first->getTreeItem();
            std::map<double, bool> knobTimes = (*it).second;

            for (std::map<double, bool>::const_iterator mIt = knobTimes.begin();
                 mIt != knobTimes.end();
                 ++mIt) {
                double time = (*mIt).first;
                bool drawSelected = (*mIt).second;
                bool drawInKnobRootRow = hierarchyView->itemIsVisibleFromOutside(knobRootItem);

                if (drawInKnobRootRow) {
                    double newCenterY = hierarchyView->visualItemRect(knobRootItem).center().y();
                    RectD zoomKfRect = getKeyFrameBoundingRectZoomCoords(time, newCenterY);
                    DopeSheetViewPrivate::KeyframeTexture textureType = (drawSelected)
                                                                        ? DopeSheetViewPrivate::kfTextureMasterSelected
                                                                        : DopeSheetViewPrivate::kfTextureMaster;

                    drawTexturedKeyframe(textureType, hasSingleKfTimeSelected && drawSelected,
                                         kfTimeSelected, selectionColor, zoomKfRect);
                }
            }
        }

        // Draw master keys in node section
        for (std::map<double, bool>::const_iterator it = nodeKeytimes.begin();
             it != nodeKeytimes.end();
             ++it) {
            double time = (*it).first;
            bool drawSelected = (*it).second;
            QTreeWidgetItem *nodeItem = NodeAnim->getTreeItem();
            bool drawInNodeRow = hierarchyView->itemIsVisibleFromOutside(nodeItem);

            if (drawInNodeRow) {
                double newCenterY = hierarchyView->visualItemRect(nodeItem).center().y();
                RectD zoomKfRect = getKeyFrameBoundingRectZoomCoords(time, newCenterY);
                DopeSheetViewPrivate::KeyframeTexture textureType = (drawSelected)
                                                                    ? DopeSheetViewPrivate::kfTextureMasterSelected
                                                                    : DopeSheetViewPrivate::kfTextureMaster;

                drawTexturedKeyframe(textureType, hasSingleKfTimeSelected && drawSelected,
                                     kfTimeSelected, selectionColor, zoomKfRect);
            }
        }
    }
} // DopeSheetViewPrivate::drawKeyframes

void
DopeSheetViewPrivate::drawTexturedKeyframe(DopeSheetViewPrivate::KeyframeTexture textureType,
                                           bool drawTime,
                                           double time,
                                           const QColor& textColor,
                                           const RectD &rect) const
{
    GLProtectAttrib<GL_GPU> a(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT | GL_TRANSFORM_BIT);
    GLProtectMatrix<GL_GPU> pr(GL_MODELVIEW);

    GL_GPU::glEnable(GL_TEXTURE_2D);
    GL_GPU::glBindTexture(GL_TEXTURE_2D, kfTexturesIDs[textureType]);

    GL_GPU::glBegin(GL_POLYGON);
    GL_GPU::glTexCoord2f(0.0f, 1.0f);
    GL_GPU::glVertex2f( rect.left(), rect.top() );
    GL_GPU::glTexCoord2f(0.0f, 0.0f);
    GL_GPU::glVertex2f( rect.left(), rect.bottom() );
    GL_GPU::glTexCoord2f(1.0f, 0.0f);
    GL_GPU::glVertex2f( rect.right(), rect.bottom() );
    GL_GPU::glTexCoord2f(1.0f, 1.0f);
    GL_GPU::glVertex2f( rect.right(), rect.top() );
    GL_GPU::glEnd();

    GL_GPU::glColor4f(1, 1, 1, 1);
    GL_GPU::glBindTexture(GL_TEXTURE_2D, 0);

    GL_GPU::glDisable(GL_TEXTURE_2D);

    if (drawTime) {
        QString text = QString::number(time);
        QPointF p = zoomContext.toWidgetCoordinates( rect.right(), rect.bottom() );
        p.rx() += 3;
        p = zoomContext.toZoomCoordinates( p.x(), p.y() );
        renderText(p.x(), p.y(), text, textColor, *font);
    }
}

void
DopeSheetViewPrivate::drawGroupOverlay(const NodeAnimPtr &NodeAnim,
                                       const NodeAnimPtr &group) const
{
    // Get the overlay color
    double r, g, b;

    NodeAnim->getNodeGui()->getColor(&r, &g, &b);

    // Compute the area to fill
    int height = hierarchyView->getHeightForItemAndChildren( NodeAnim->getTreeItem() );
    QRectF nameItemRect = hierarchyView->visualItemRect( NodeAnim->getTreeItem() );

    assert( nodeRanges.find( group ) != nodeRanges.end() );
    FrameRange groupRange = nodeRanges.find( group )->second; // map::at() is C++11
    RectD overlayRect;
    overlayRect.x1 = groupRange.first;
    overlayRect.x2 = groupRange.second;

    overlayRect.y1 = zoomContext.toZoomCoordinates(0, nameItemRect.top() + height).y();
    overlayRect.y2 = zoomContext.toZoomCoordinates( 0, nameItemRect.top() ).y();

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
DopeSheetViewPrivate::drawProjectBounds() const
{
    running_in_main_thread_and_context(publicInterface);

    double bottom = zoomContext.toZoomCoordinates(0, publicInterface->height() - 1).y();
    double top = zoomContext.toZoomCoordinates(publicInterface->width() - 1, 0).y();
    double projectStart, projectEnd;
    if (!gui) {
        return;
    }
    GuiAppInstancePtr app = gui->getApp();
    if (!app) {
        return;
    }
    app->getFrameRange(&projectStart, &projectEnd);

    SettingsPtr settings = appPTR->getCurrentSettings();
    double colorR, colorG, colorB;
    settings->getTimelineBoundsColor(&colorR, &colorG, &colorB);

    // Perform drawing
    {
        GLProtectAttrib<GL_GPU> a(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT);

        GL_GPU::glColor4f(colorR, colorG, colorB, 1.f);

        // Draw start bound
        GL_GPU::glBegin(GL_LINES);
        GL_GPU::glVertex2f(projectStart, top);
        GL_GPU::glVertex2f(projectStart, bottom);
        GL_GPU::glEnd();

        // Draw end bound
        GL_GPU::glBegin(GL_LINES);
        GL_GPU::glVertex2f(projectEnd, top);
        GL_GPU::glVertex2f(projectEnd, bottom);
        GL_GPU::glEnd();
    }
}

/**
 * @brief DopeSheetViewPrivate::drawIndicator
 *
 *
 */
void
DopeSheetViewPrivate::drawCurrentFrameIndicator()
{
    running_in_main_thread_and_context(publicInterface);

    computeTimelinePositions();

    int top = zoomContext.toZoomCoordinates(0, 0).y();
    int bottom = zoomContext.toZoomCoordinates(publicInterface->width() - 1,
                                               publicInterface->height() - 1).y();
    int currentFrame = timeline->currentFrame();

    // Retrieve settings for drawing
    SettingsPtr settings = appPTR->getCurrentSettings();
    double colorR, colorG, colorB;
    settings->getTimelinePlayheadColor(&colorR, &colorG, &colorB);

    // Perform drawing
    {
        GLProtectAttrib<GL_GPU> a(GL_CURRENT_BIT | GL_HINT_BIT | GL_ENABLE_BIT |
                          GL_LINE_BIT | GL_POLYGON_BIT | GL_COLOR_BUFFER_BIT);

        GL_GPU::glEnable(GL_BLEND);
        GL_GPU::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        GL_GPU::glEnable(GL_LINE_SMOOTH);
        GL_GPU::glHint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);

        GL_GPU::glColor4f(colorR, colorG, colorB, 1.f);

        GL_GPU::glBegin(GL_LINES);
        GL_GPU::glVertex2f(currentFrame, top);
        GL_GPU::glVertex2f(currentFrame, bottom);
        GL_GPU::glEnd();

        GL_GPU::glEnable(GL_POLYGON_SMOOTH);
        GL_GPU::glHint(GL_POLYGON_SMOOTH_HINT, GL_DONT_CARE);

        // Draw bottom polygon
        GL_GPU::glBegin(GL_POLYGON);
        GL_GPU::glVertex2f( currentFrameIndicatorBottomPoly.at(0).x(), currentFrameIndicatorBottomPoly.at(0).y() );
        GL_GPU::glVertex2f( currentFrameIndicatorBottomPoly.at(1).x(), currentFrameIndicatorBottomPoly.at(1).y() );
        GL_GPU::glVertex2f( currentFrameIndicatorBottomPoly.at(2).x(), currentFrameIndicatorBottomPoly.at(2).y() );
        GL_GPU::glEnd();
    }
} // DopeSheetViewPrivate::drawCurrentFrameIndicator

/**
 * @brief DopeSheetViewPrivate::drawSelectionRect
 *
 *
 */
void
DopeSheetViewPrivate::drawSelectionRect() const
{
    running_in_main_thread_and_context(publicInterface);

    // Perform drawing
    {
        GLProtectAttrib<GL_GPU> a(GL_HINT_BIT | GL_ENABLE_BIT | GL_LINE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT | GL_LINE_BIT);

        GL_GPU::glEnable(GL_BLEND);
        GL_GPU::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        GL_GPU::glEnable(GL_LINE_SMOOTH);
        GL_GPU::glHint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);

        GL_GPU::glColor4f(0.3, 0.3, 0.3, 0.2);

        // Draw rect
        GL_GPU::glBegin(GL_POLYGON);
        GL_GPU::glVertex2f(selectionRect.x1, selectionRect.y1);
        GL_GPU::glVertex2f(selectionRect.x1, selectionRect.y2);
        GL_GPU::glVertex2f(selectionRect.x2, selectionRect.y2);
        GL_GPU::glVertex2f(selectionRect.x2, selectionRect.y1);
        GL_GPU::glEnd();

        GL_GPU::glLineWidth(1.5);

        // Draw outline
        GL_GPU::glColor4f(0.5, 0.5, 0.5, 1.);
        GL_GPU::glBegin(GL_LINE_LOOP);
        GL_GPU::glVertex2f(selectionRect.x1, selectionRect.y1);
        GL_GPU::glVertex2f(selectionRect.x1, selectionRect.y2);
        GL_GPU::glVertex2f(selectionRect.x2, selectionRect.y2);
        GL_GPU::glVertex2f(selectionRect.x2, selectionRect.y1);
        GL_GPU::glEnd();

        glCheckError(GL_GPU);
    }
}

/**
 * @brief DopeSheetViewPrivate::drawSelectedKeysBRect
 *
 *
 */
void
DopeSheetViewPrivate::drawSelectedKeysBRect() const
{
    running_in_main_thread_and_context(publicInterface);

    // Perform drawing
    {
        GLProtectAttrib<GL_GPU> a(GL_HINT_BIT | GL_ENABLE_BIT | GL_LINE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT | GL_LINE_BIT);

        GL_GPU::glEnable(GL_BLEND);
        GL_GPU::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        GL_GPU::glEnable(GL_LINE_SMOOTH);
        GL_GPU::glHint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);

        GL_GPU::glLineWidth(1.5);

        GL_GPU::glColor4f(0.5, 0.5, 0.5, 1.);

        // Draw outline
        GL_GPU::glBegin(GL_LINE_LOOP);
        GL_GPU::glVertex2f( selectedKeysBRect.left(), selectedKeysBRect.bottom() );
        GL_GPU::glVertex2f( selectedKeysBRect.left(), selectedKeysBRect.top() );
        GL_GPU::glVertex2f( selectedKeysBRect.right(), selectedKeysBRect.top() );
        GL_GPU::glVertex2f( selectedKeysBRect.right(), selectedKeysBRect.bottom() );
        GL_GPU::glEnd();

        // Draw center cross lines
        const int CROSS_LINE_OFFSET = 10;
        QPointF bRectCenter( (selectedKeysBRect.x1 + selectedKeysBRect.x2) / 2., (selectedKeysBRect.y1 + selectedKeysBRect.y2) / 2. );
        QPointF bRectCenterWidgetCoords = zoomContext.toWidgetCoordinates( bRectCenter.x(), bRectCenter.y() );
        QLineF horizontalLine( zoomContext.toZoomCoordinates( bRectCenterWidgetCoords.x() - CROSS_LINE_OFFSET, bRectCenterWidgetCoords.y() ),
                               zoomContext.toZoomCoordinates( bRectCenterWidgetCoords.x() + CROSS_LINE_OFFSET, bRectCenterWidgetCoords.y() ) );
        QLineF verticalLine( zoomContext.toZoomCoordinates(bRectCenterWidgetCoords.x(), bRectCenterWidgetCoords.y() - CROSS_LINE_OFFSET),
                             zoomContext.toZoomCoordinates(bRectCenterWidgetCoords.x(), bRectCenterWidgetCoords.y() + CROSS_LINE_OFFSET) );

        GL_GPU::glBegin(GL_LINES);
        GL_GPU::glVertex2f( horizontalLine.p1().x(), horizontalLine.p1().y() );
        GL_GPU::glVertex2f( horizontalLine.p2().x(), horizontalLine.p2().y() );

        GL_GPU::glVertex2f( verticalLine.p1().x(), verticalLine.p1().y() );
        GL_GPU::glVertex2f( verticalLine.p2().x(), verticalLine.p2().y() );
        GL_GPU::glEnd();

        glCheckError(GL_GPU);
    }
}

void
DopeSheetViewPrivate::renderText(double x,
                                 double y,
                                 const QString &text,
                                 const QColor &color,
                                 const QFont &font,
                                 int flags) const
{
    running_in_main_thread_and_context(publicInterface);

    if ( text.isEmpty() ) {
        return;
    }

    double w = double( publicInterface->width() );
    double h = double( publicInterface->height() );
    double bottom = zoomContext.bottom();
    double left = zoomContext.left();
    double top =  zoomContext.top();
    double right = zoomContext.right();

    if ( (w <= 0) || (h <= 0) || (right <= left) || (top <= bottom) ) {
        return;
    }

    double scalex = (right - left) / w;
    double scaley = (top - bottom) / h;

    textRenderer.renderText(x, y, scalex, scaley, text, color, font, flags);

    glCheckError(GL_GPU);
}

void
DopeSheetViewPrivate::computeTimelinePositions()
{
    running_in_main_thread();

    if ( (zoomContext.screenWidth() <= 0) || (zoomContext.screenHeight() <= 0) ) {
        return;
    }
    double polyHalfWidth = 7.5;
    double polyHeight = 7.5;
    int bottom = zoomContext.toZoomCoordinates(publicInterface->width() - 1,
                                               publicInterface->height() - 1).y();
    int currentFrame = timeline->currentFrame();
    QPointF bottomCursorBottom(currentFrame, bottom);
    QPointF bottomCursorBottomWidgetCoords = zoomContext.toWidgetCoordinates( bottomCursorBottom.x(), bottomCursorBottom.y() );
    QPointF leftPoint = zoomContext.toZoomCoordinates( bottomCursorBottomWidgetCoords.x() - polyHalfWidth, bottomCursorBottomWidgetCoords.y() );
    QPointF rightPoint = zoomContext.toZoomCoordinates( bottomCursorBottomWidgetCoords.x() + polyHalfWidth, bottomCursorBottomWidgetCoords.y() );
    QPointF topPoint = zoomContext.toZoomCoordinates(bottomCursorBottomWidgetCoords.x(), bottomCursorBottomWidgetCoords.y() - polyHeight);

    currentFrameIndicatorBottomPoly.clear();

    currentFrameIndicatorBottomPoly << leftPoint
                                    << rightPoint
                                    << topPoint;
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
    AnimKeyFramePtrList selectedKeyframes;
    std::vector<NodeAnimPtr > selectedNodes;

    model->getSelectionModel()->getCurrentSelection(&selectedKeyframes, &selectedNodes);

    if ( ( (selectedKeyframes.size() <= 1) && selectedNodes.empty() ) ||
         ( selectedKeyframes.empty() && ( selectedNodes.size() <= 1) ) ) {
        selectedKeysBRect.clear();

        return;
    }

    selectedKeysBRect.setupInfinity();

    for (AnimKeyFramePtrList::const_iterator it = selectedKeyframes.begin();
         it != selectedKeyframes.end();
         ++it) {
        KnobAnimPtr knobContext = (*it)->context.lock();
        assert(knobContext);

        QTreeWidgetItem *keyItem = knobContext->getTreeItem();
        double time = (*it)->key.getTime();

        //x1,x2 are in zoom coords
        selectedKeysBRect.x1 = std::min(time, selectedKeysBRect.x1);
        selectedKeysBRect.x2 = std::max(time, selectedKeysBRect.x2);


        QRect keyItemRect = hierarchyView->visualItemRect(keyItem);
        if ( !keyItemRect.isNull() && !keyItemRect.isEmpty() ) {
            double y = keyItemRect.center().y();

            //y1,y2 are in widget coords
            selectedKeysBRect.y1 = std::min(y, selectedKeysBRect.y1);
            selectedKeysBRect.y2 = std::max(y, selectedKeysBRect.y2);
        }
    }

    for (std::vector<NodeAnimPtr >::iterator it = selectedNodes.begin(); it != selectedNodes.end(); ++it) {
        std::map<NodeAnimPtr, FrameRange >::const_iterator foundRange = nodeRanges.find(*it);
        if ( foundRange == nodeRanges.end() ) {
            continue;
        }
        const FrameRange& range = foundRange->second;

        //x1,x2 are in zoom coords
        selectedKeysBRect.x1 = std::min(range.first, selectedKeysBRect.x1);
        selectedKeysBRect.x2 = std::max(range.second, selectedKeysBRect.x2);

        QTreeWidgetItem* nodeItem = (*it)->getTreeItem();
        QRect itemRect = hierarchyView->visualItemRect(nodeItem);
        if ( !itemRect.isNull() && !itemRect.isEmpty() ) {
            double y = itemRect.center().y();

            //y1,y2 are in widget coords
            selectedKeysBRect.y1 = std::min(y, selectedKeysBRect.y1);
            selectedKeysBRect.y2 = std::max(y, selectedKeysBRect.y2);
        }
    }

    QTreeWidgetItem *bottomMostItem = hierarchyView->itemAt(0, selectedKeysBRect.y2);
    double bottom = hierarchyView->visualItemRect(bottomMostItem).bottom();
    bottom = zoomContext.toZoomCoordinates(0, bottom).y();

    QTreeWidgetItem *topMostItem = hierarchyView->itemAt(0, selectedKeysBRect.y1);
    double top = hierarchyView->visualItemRect(topMostItem).top();
    top = zoomContext.toZoomCoordinates(0, top).y();

    selectedKeysBRect.y1 = bottom;
    selectedKeysBRect.y2 = top;

    if ( !selectedKeysBRect.isNull() ) {
        /// Adjust the bounding rect of the size of a keyframe
        double leftWidget = zoomContext.toWidgetCoordinates(selectedKeysBRect.x1, 0).x();
        double leftAdjustedZoom = zoomContext.toZoomCoordinates(leftWidget - KF_X_OFFSET, 0).x();
        double xAdjustOffset = (selectedKeysBRect.x1 - leftAdjustedZoom);

        selectedKeysBRect.x1 -= xAdjustOffset;
        selectedKeysBRect.x2 += xAdjustOffset;
    }
} // DopeSheetViewPrivate::computeSelectedKeysBRect

void
DopeSheetViewPrivate::computeRangesBelow(const NodeAnimPtr& NodeAnim)
{
    AnimTreeItemNodeMap nodeRows = model->getItemNodeMap();

    for (AnimTreeItemNodeMap::const_iterator it = nodeRows.begin(); it != nodeRows.end(); ++it) {
        QTreeWidgetItem *item = (*it).first;
        NodeAnimPtr toCompute = (*it).second;

        if ( hierarchyView->visualItemRect(item).y() >= hierarchyView->visualItemRect( NodeAnim->getTreeItem() ).y() ) {
            computeNodeRange( toCompute );
        }
    }
}

void
DopeSheetViewPrivate::computeNodeRange(const NodeAnimPtr& NodeAnim)
{
    AnimatedItemType nodeType = NodeAnim->getItemType();

    switch (nodeType) {
    case eAnimatedItemTypeReader:
        computeReaderRange(NodeAnim);
        break;
    case eAnimatedItemTypeRetime:
        computeRetimeRange(NodeAnim);
        break;
    case eAnimatedItemTypeTimeOffset:
        computeTimeOffsetRange(NodeAnim);
        break;
    case eAnimatedItemTypeFrameRange:
        computeFRRange(NodeAnim);
        break;
    case eAnimatedItemTypeGroup:
        computeGroupRange(NodeAnim);
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
#ifdef DEBUG
#pragma message WARN("only considering first view")
#endif
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
    range.first = frameRangeKnob->getValue(0);
    range.second = frameRangeKnob->getValue(1);

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
    QPointF mouseZoomCoords = zoomContext.toZoomCoordinates( e->x(), e->y() );
    QPointF lastZoomCoordsOnMousePress = zoomContext.toZoomCoordinates( lastPosOnMousePress.x(),
                                                                        lastPosOnMousePress.y() );
    QPointF lastZoomCoordsOnMouseMove = zoomContext.toZoomCoordinates( lastPosOnMouseMove.x(),
                                                                       lastPosOnMouseMove.y() );
    double currentTime = mouseZoomCoords.x();
    double dt = clampedMouseOffset(lastZoomCoordsOnMousePress.x(), currentTime);
    double dv = mouseZoomCoords.y() - lastZoomCoordsOnMouseMove.y();

    switch (eventState) {
    case DopeSheetView::esMoveKeyframeSelection: {
        if ( (dt >= 1.0f) || (dt <= -1.0f) ) {
            if (gui) {
                gui->setDraftRenderEnabled(true);
            }
            model->moveSelectedKeysAndNodes(dt);
        }

        break;
    }
    case DopeSheetView::esTransformingKeyframesMiddleLeft:
    case DopeSheetView::esTransformingKeyframesMiddleRight: {
        if (gui) {
            gui->setDraftRenderEnabled(true);
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
        model->transformSelectedKeys(transform);
        break;
    }
    case DopeSheetView::esMoveCurrentFrameIndicator: {
        if ( (dt >= 1.0f) || (dt <= -1.0f) ) {
            if (gui) {
                gui->setDraftRenderEnabled(true);
            }
            moveCurrentFrameIndicator(dt);
        }

        break;
    }
    case DopeSheetView::esSelectionByRect: {
        computeSelectionRect(lastZoomCoordsOnMousePress, mouseZoomCoords);

        publicInterface->redraw();

        break;
    }
    case DopeSheetView::esReaderLeftTrim: {
        if ( (dt >= 1.0f) || (dt <= -1.0f) ) {
            if (gui) {
                gui->setDraftRenderEnabled(true);
            }
            KnobIntBase *timeOffsetKnob = dynamic_cast<KnobIntBase *>( currentEditedReader->getInternalNode()->getKnobByName(kReaderParamNameTimeOffset).get() );
            assert(timeOffsetKnob);

            double newFirstFrame = std::floor(currentTime - timeOffsetKnob->getValue() + 0.5);

            model->trimReaderLeft(currentEditedReader, newFirstFrame);
        }

        break;
    }
    case DopeSheetView::esReaderRightTrim: {
        if ( (dt >= 1.0f) || (dt <= -1.0f) ) {
            if (gui) {
                gui->setDraftRenderEnabled(true);
            }
            KnobIntBase *timeOffsetKnob = dynamic_cast<KnobIntBase *>( currentEditedReader->getInternalNode()->getKnobByName(kReaderParamNameTimeOffset).get() );
            assert(timeOffsetKnob);

            double newLastFrame = std::floor(currentTime - timeOffsetKnob->getValue() + 0.5);

            model->trimReaderRight(currentEditedReader, newLastFrame);
        }

        break;
    }
    case DopeSheetView::esReaderSlip: {
        if ( (dt >= 1.0f) || (dt <= -1.0f) ) {
            if (gui) {
                gui->setDraftRenderEnabled(true);
            }
            model->slipReader(currentEditedReader, dt);
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
DopeSheetViewPrivate::createSelectionFromRect(const RectD &zoomCoordsRect,
                                              std::vector<AnimKeyFrame> *result,
                                              std::vector<NodeAnimPtr >* selectedNodes)
{
    AnimTreeItemNodeMap NodeAnims = model->getItemNodeMap();

    for (AnimTreeItemNodeMap::const_iterator it = NodeAnims.begin(); it != NodeAnims.end(); ++it) {
        const NodeAnimPtr& NodeAnim = (*it).second;
        const AnimTreeItemKnobMap& KnobAnims = NodeAnim->getItemKnobMap();

        for (AnimTreeItemKnobMap::const_iterator it2 = KnobAnims.begin(); it2 != KnobAnims.end(); ++it2) {
            KnobAnimPtr KnobAnim = (*it2).second;
            int dim = KnobAnim->getDimension();

            if (dim == -1) {
                continue;
            }

            KeyFrameSet keyframes = KnobAnim->getKnobGui()->getCurve(ViewIdx(0), dim)->getKeyFrames_mt_safe();

            for (KeyFrameSet::const_iterator kIt = keyframes.begin();
                 kIt != keyframes.end();
                 ++kIt) {
                KeyFrame kf = (*kIt);
                double y = hierarchyView->visualItemRect( KnobAnim->getTreeItem() ).center().y();
                RectD zoomKfRect = getKeyFrameBoundingRectZoomCoords(kf.getTime(), y);

                if ( zoomCoordsRect.intersects(zoomKfRect) ) {
                    result->push_back( AnimKeyFrame(KnobAnim, kf) );
                }
            }
        }

        std::map<NodeAnimPtr, FrameRange >::const_iterator foundRange = nodeRanges.find( NodeAnim );
        if ( foundRange != nodeRanges.end() ) {
            QPoint visualRectCenter = hierarchyView->visualItemRect( NodeAnim->getTreeItem() ).center();
            QPointF center = zoomContext.toZoomCoordinates( visualRectCenter.x(), visualRectCenter.y() );
            if ( zoomCoordsRect.contains( (foundRange->second.first + foundRange->second.second) / 2., center.y() ) ) {
                selectedNodes->push_back(NodeAnim);
            }
        }
    }
}

void
DopeSheetViewPrivate::moveCurrentFrameIndicator(double dt)
{
    if (!gui) {
        return;
    }
    gui->getApp()->setLastViewerUsingTimeline( NodePtr() );

    double toTime = timeline->currentFrame() + dt;

    gui->setDraftRenderEnabled(true);
    timeline->seekFrame(SequenceTime(toTime), false, OutputEffectInstancePtr(),
                        eTimelineChangeReasonAnimationModuleEditorSeek);
}

void
DopeSheetViewPrivate::createContextMenu()
{
    running_in_main_thread();

    contextMenu->clear();

    // Create menus

    // Edit menu
    Menu *editMenu = new Menu(contextMenu);
    editMenu->setTitle( tr("Edit") );

    contextMenu->addAction( editMenu->menuAction() );

    // Interpolation menu
    Menu *interpMenu = new Menu(contextMenu);
    interpMenu->setTitle( tr("Interpolation") );

    contextMenu->addAction( interpMenu->menuAction() );

    // View menu
    Menu *viewMenu = new Menu(contextMenu);
    viewMenu->setTitle( tr("View") );

    contextMenu->addAction( viewMenu->menuAction() );

    // Create actions

    // Edit actions
    QAction *removeSelectedKeyframesAction = new ActionWithShortcut(kShortcutGroupAnimationModuleEditor,
                                                                    kShortcutIDActionAnimationModuleEditorDeleteKeys,
                                                                    kShortcutDescActionAnimationModuleEditorDeleteKeys,
                                                                    editMenu);
    QObject::connect( removeSelectedKeyframesAction, SIGNAL(triggered()),
                      publicInterface, SLOT(deleteSelectedKeyframes()) );
    editMenu->addAction(removeSelectedKeyframesAction);

    QAction *copySelectedKeyframesAction = new ActionWithShortcut(kShortcutGroupAnimationModuleEditor,
                                                                  kShortcutIDActionAnimationModuleEditorCopySelectedKeyframes,
                                                                  kShortcutDescActionAnimationModuleEditorCopySelectedKeyframes,
                                                                  editMenu);
    QObject::connect( copySelectedKeyframesAction, SIGNAL(triggered()),
                      publicInterface, SLOT(copySelectedKeyframes()) );
    editMenu->addAction(copySelectedKeyframesAction);

    QAction *pasteKeyframesAction = new ActionWithShortcut(kShortcutGroupAnimationModuleEditor,
                                                           kShortcutIDActionAnimationModuleEditorPasteKeyframes,
                                                           kShortcutDescActionAnimationModuleEditorPasteKeyframes,
                                                           editMenu);
    QObject::connect( pasteKeyframesAction, SIGNAL(triggered()),
                      publicInterface, SLOT(pasteKeyframesRelative()) );
    editMenu->addAction(pasteKeyframesAction);

    QAction *pasteKeyframesAbsAction = new ActionWithShortcut(kShortcutGroupAnimationModuleEditor,
                                                           kShortcutIDActionAnimationModuleEditorPasteKeyframesAbsolute,
                                                           kShortcutDescActionAnimationModuleEditorPasteKeyframesAbsolute,
                                                           editMenu);
    QObject::connect( pasteKeyframesAbsAction, SIGNAL(triggered()),
                     publicInterface, SLOT(pasteKeyframesAbsolute()) );
    editMenu->addAction(pasteKeyframesAbsAction);

    QAction *selectAllKeyframesAction = new ActionWithShortcut(kShortcutGroupAnimationModuleEditor,
                                                               kShortcutIDActionAnimationModuleEditorSelectAllKeyframes,
                                                               kShortcutDescActionAnimationModuleEditorSelectAllKeyframes,
                                                               editMenu);
    QObject::connect( selectAllKeyframesAction, SIGNAL(triggered()),
                      publicInterface, SLOT(onSelectedAllTriggered()) );
    editMenu->addAction(selectAllKeyframesAction);


    // Interpolation actions
    QAction *constantInterpAction = new ActionWithShortcut(kShortcutGroupAnimationModuleEditor,
                                                           kShortcutIDActionCurveEditorConstant,
                                                           kShortcutDescActionCurveEditorConstant,
                                                           interpMenu);
    QPixmap pix;
    appPTR->getIcon(NATRON_PIXMAP_INTERP_CONSTANT, NATRON_MEDIUM_BUTTON_ICON_SIZE, &pix);
    constantInterpAction->setIcon( QIcon(pix) );
    constantInterpAction->setIconVisibleInMenu(true);

    QObject::connect( constantInterpAction, SIGNAL(triggered()),
                      publicInterface, SLOT(constantInterpSelectedKeyframes()) );

    interpMenu->addAction(constantInterpAction);

    QAction *linearInterpAction = new ActionWithShortcut(kShortcutGroupAnimationModuleEditor,
                                                         kShortcutIDActionCurveEditorLinear,
                                                         kShortcutDescActionCurveEditorLinear,
                                                         interpMenu);

    appPTR->getIcon(NATRON_PIXMAP_INTERP_LINEAR, NATRON_MEDIUM_BUTTON_ICON_SIZE, &pix);
    linearInterpAction->setIcon( QIcon(pix) );
    linearInterpAction->setIconVisibleInMenu(true);

    QObject::connect( linearInterpAction, SIGNAL(triggered()),
                      publicInterface, SLOT(linearInterpSelectedKeyframes()) );

    interpMenu->addAction(linearInterpAction);

    QAction *smoothInterpAction = new ActionWithShortcut(kShortcutGroupAnimationModuleEditor,
                                                         kShortcutIDActionCurveEditorSmooth,
                                                         kShortcutDescActionCurveEditorSmooth,
                                                         interpMenu);

    appPTR->getIcon(NATRON_PIXMAP_INTERP_CURVE_Z, NATRON_MEDIUM_BUTTON_ICON_SIZE, &pix);
    smoothInterpAction->setIcon( QIcon(pix) );
    smoothInterpAction->setIconVisibleInMenu(true);

    QObject::connect( smoothInterpAction, SIGNAL(triggered()),
                      publicInterface, SLOT(smoothInterpSelectedKeyframes()) );

    interpMenu->addAction(smoothInterpAction);

    QAction *catmullRomInterpAction = new ActionWithShortcut(kShortcutGroupAnimationModuleEditor,
                                                             kShortcutIDActionCurveEditorCatmullrom,
                                                             kShortcutDescActionCurveEditorCatmullrom,
                                                             interpMenu);
    appPTR->getIcon(NATRON_PIXMAP_INTERP_CURVE_R, NATRON_MEDIUM_BUTTON_ICON_SIZE, &pix);
    catmullRomInterpAction->setIcon( QIcon(pix) );
    catmullRomInterpAction->setIconVisibleInMenu(true);

    QObject::connect( catmullRomInterpAction, SIGNAL(triggered()),
                      publicInterface, SLOT(catmullRomInterpSelectedKeyframes()) );

    interpMenu->addAction(catmullRomInterpAction);

    QAction *cubicInterpAction = new ActionWithShortcut(kShortcutGroupAnimationModuleEditor,
                                                        kShortcutIDActionCurveEditorCubic,
                                                        kShortcutDescActionCurveEditorCubic,
                                                        interpMenu);
    appPTR->getIcon(NATRON_PIXMAP_INTERP_CURVE_C, NATRON_MEDIUM_BUTTON_ICON_SIZE, &pix);
    cubicInterpAction->setIcon( QIcon(pix) );
    cubicInterpAction->setIconVisibleInMenu(true);

    QObject::connect( cubicInterpAction, SIGNAL(triggered()),
                      publicInterface, SLOT(cubicInterpSelectedKeyframes()) );

    interpMenu->addAction(cubicInterpAction);

    QAction *horizontalInterpAction = new ActionWithShortcut(kShortcutGroupAnimationModuleEditor,
                                                             kShortcutIDActionCurveEditorHorizontal,
                                                             kShortcutDescActionCurveEditorHorizontal,
                                                             interpMenu);
    appPTR->getIcon(NATRON_PIXMAP_INTERP_CURVE_H, NATRON_MEDIUM_BUTTON_ICON_SIZE, &pix);
    horizontalInterpAction->setIcon( QIcon(pix) );
    horizontalInterpAction->setIconVisibleInMenu(true);

    QObject::connect( horizontalInterpAction, SIGNAL(triggered()),
                      publicInterface, SLOT(horizontalInterpSelectedKeyframes()) );

    interpMenu->addAction(horizontalInterpAction);

    QAction *breakInterpAction = new ActionWithShortcut(kShortcutGroupAnimationModuleEditor,
                                                        kShortcutIDActionCurveEditorBreak,
                                                        kShortcutDescActionCurveEditorBreak,
                                                        interpMenu);
    appPTR->getIcon(NATRON_PIXMAP_INTERP_BREAK, NATRON_MEDIUM_BUTTON_ICON_SIZE, &pix);
    breakInterpAction->setIcon( QIcon(pix) );
    breakInterpAction->setIconVisibleInMenu(true);

    QObject::connect( breakInterpAction, SIGNAL(triggered()),
                      publicInterface, SLOT(breakInterpSelectedKeyframes()) );

    interpMenu->addAction(breakInterpAction);

    // View actions
    QAction *frameSelectionAction = new ActionWithShortcut(kShortcutGroupAnimationModuleEditor,
                                                           kShortcutIDActionAnimationModuleEditorFrameSelection,
                                                           kShortcutDescActionAnimationModuleEditorFrameSelection,
                                                           viewMenu);
    QObject::connect( frameSelectionAction, SIGNAL(triggered()),
                      publicInterface, SLOT(centerOnSelection()) );
    viewMenu->addAction(frameSelectionAction);
} // DopeSheetViewPrivate::createContextMenu

void
DopeSheetViewPrivate::updateCurveWidgetFrameRange()
{
    CurveWidget *curveWidget = gui->getCurveEditor()->getCurveWidget();

    curveWidget->centerOn( zoomContext.left(), zoomContext.right() );
}

/**
 * @brief DopeSheetView::DopeSheetView
 *
 * Constructs a DopeSheetView object.
 */
DopeSheetView::DopeSheetView(const AnimationModuleBasePtr& model,
                             AnimationModuleTreeView *treeView,
                             Gui *gui,
                             const TimeLinePtr &timeline,
                             QWidget *parent)
    : QGLWidget(parent)
    , _imp( new DopeSheetViewPrivate(this) )
{
    _imp->model = model;
    _imp->hierarchyView = hierarchyView;
    _imp->gui = gui;
    _imp->timeline = timeline;

    setMouseTracking(true);

    if (timeline) {
        ProjectPtr project = gui->getApp()->getProject();
        assert(project);

        connect( timeline.get(), SIGNAL(frameChanged(SequenceTime,int)), this, SLOT(onTimeLineFrameChanged(SequenceTime,int)) );
        connect( project.get(), SIGNAL(frameRangeChanged(int,int)), this, SLOT(onTimeLineBoundariesChanged(int,int)) );

        onTimeLineFrameChanged(timeline->currentFrame(), eValueChangedReasonNatronGuiEdited);

        double left, right;
        project->getFrameRange(&left, &right);
        onTimeLineBoundariesChanged(left, right);
    }
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
DopeSheetView::centerOn(double xMin,
                        double xMax)
{
    double ymin = _imp->zoomContext.bottom();
    double ymax = _imp->zoomContext.top();

    if (ymin >= ymax) {
        return;
    }
    _imp->zoomContext.fill( xMin, xMax, ymin, ymax );

    redraw();
}

std::pair<double, double> DopeSheetView::getKeyframeRange() const
{
    std::pair<double, double> ret;
    std::vector<double> dimFirstKeys;
    std::vector<double> dimLastKeys;
    AnimTreeItemNodeMap NodeAnimItems = _imp->model->getItemNodeMap();

    for (AnimTreeItemNodeMap::const_iterator it = NodeAnimItems.begin(); it != NodeAnimItems.end(); ++it) {
        if ( (*it).first->isHidden() ) {
            continue;
        }

        const NodeAnimPtr& NodeAnim = (*it).second;
        const AnimTreeItemKnobMap& KnobAnimItems = NodeAnim->getItemKnobMap();

        for (AnimTreeItemKnobMap::const_iterator itKnob = KnobAnimItems.begin(); itKnob != KnobAnimItems.end(); ++itKnob) {
            if ( (*itKnob).first->isHidden() ) {
                continue;
            }

            const KnobAnimPtr& KnobAnim = (*itKnob).second;

            for (int i = 0; i < KnobAnim->getKnobGui()->getKnob()->getDimension(); ++i) {
                KeyFrameSet keyframes = KnobAnim->getKnobGui()->getCurve(ViewIdx(0), i)->getKeyFrames_mt_safe();

                if ( keyframes.empty() ) {
                    continue;
                }

                dimFirstKeys.push_back( keyframes.begin()->getTime() );
                dimLastKeys.push_back( keyframes.rbegin()->getTime() );
            }
        }

        // Also append the range of the clip if this is a Reader/Group/Time node
        std::map<NodeAnimPtr, FrameRange >::const_iterator foundRange = _imp->nodeRanges.find( NodeAnim );
        if ( foundRange != _imp->nodeRanges.end() ) {
            const FrameRange& range = foundRange->second;
            if ( !dimFirstKeys.empty() ) {
                dimFirstKeys[0] = range.first;
            } else {
                dimFirstKeys.push_back(range.first);
            }
            if ( !dimLastKeys.empty() ) {
                dimLastKeys[0] = range.second;
            } else {
                dimLastKeys.push_back(range.second);
            }
        }
    }

    if ( dimFirstKeys.empty() || dimLastKeys.empty() ) {
        ret.first = 0;
        ret.second = 0;
    } else {
        ret.first = *std::min_element( dimFirstKeys.begin(), dimFirstKeys.end() );
        ret.second = *std::max_element( dimLastKeys.begin(), dimLastKeys.end() );
    }

    return ret;
} // DopeSheetView::getKeyframeRange

/**
 * @brief DopeSheetView::swapOpenGLBuffers
 *
 *
 */
void
DopeSheetView::swapOpenGLBuffers()
{
    running_in_main_thread();

    swapBuffers();
}

/**
 * @brief DopeSheetView::redraw
 *
 *
 */
void
DopeSheetView::redraw()
{
    running_in_main_thread();

    update();
}


void
DopeSheetView::getOpenGLContextFormat(int* depthPerComponents, bool* hasAlpha) const
{
    QGLFormat f = format();
    *hasAlpha = f.alpha();
    int r = f.redBufferSize();
    if (r == -1) {
        r = 8;// taken from qgl.h
    }
    int g = f.greenBufferSize();
    if (g == -1) {
        g = 8;// taken from qgl.h
    }
    int b = f.blueBufferSize();
    if (b == -1) {
        b = 8;// taken from qgl.h
    }
    int size = r;
    size = std::min(size, g);
    size = std::min(size, b);
    *depthPerComponents = size;
}

/**
 * @brief DopeSheetView::getViewportSize
 *
 *
 */
void
DopeSheetView::getViewportSize(double &width,
                               double &height) const
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
void
DopeSheetView::getPixelScale(double &xScale,
                             double &yScale) const
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
void
DopeSheetView::getBackgroundColour(double &r,
                                   double &g,
                                   double &b) const
{
    running_in_main_thread();

    // use the same settings as the curve editor
    appPTR->getCurrentSettings()->getCurveEditorBGColor(&r, &g, &b);
}

RectD
DopeSheetView::getViewportRect() const
{
    RectD bbox;
    {
        bbox.x1 = _imp->zoomContext.left();
        bbox.y1 = _imp->zoomContext.bottom();
        bbox.x2 = _imp->zoomContext.right();
        bbox.y2 = _imp->zoomContext.top();
    }

    return bbox;
}

void
DopeSheetView::getCursorPosition(double& x,
                                 double& y) const
{
    QPoint p = QCursor::pos();

    p = mapFromGlobal(p);
    QPointF mappedPos = _imp->zoomContext.toZoomCoordinates( p.x(), p.y() );
    x = mappedPos.x();
    y = mappedPos.y();
}

/**
 * @brief DopeSheetView::saveOpenGLContext
 *
 *
 */
void
DopeSheetView::saveOpenGLContext()
{
    running_in_main_thread();
}

/**
 * @brief DopeSheetView::restoreOpenGLContext
 *
 *
 */
void
DopeSheetView::restoreOpenGLContext()
{
    running_in_main_thread();
}

/**
 * @brief DopeSheetView::getCurrentRenderScale
 *
 *
 */
unsigned int
DopeSheetView::getCurrentRenderScale() const
{
    return 0;
}

void
DopeSheetView::onSelectedAllTriggered()
{
    _imp->model->getSelectionModel()->selectAll();

    if (_imp->model->getSelectionModel()->getSelectedKeyframesCount() > 1) {
        _imp->computeSelectedKeysBRect();
    }

    redraw();
}

void
DopeSheetView::deleteSelectedKeyframes()
{
    running_in_main_thread();

    _imp->model->deleteSelectedKeyframes();

    redraw();
}

void
DopeSheetView::centerOnSelection()
{
    running_in_main_thread();

    int selectedKeyframesCount = _imp->model->getSelectionModel()->getSelectedKeyframesCount();

    if (selectedKeyframesCount == 1) {
        return;
    }

    FrameRange range;

    // frame on project bounds
    if (!selectedKeyframesCount) {
        range = getKeyframeRange();
    }
    // or frame on current selection
    else {
        range.first = _imp->selectedKeysBRect.left();
        range.second = _imp->selectedKeysBRect.right();
    }

    if (range.first == range.second) {
        return;
    }

    double actualRange = (range.second - range.first);
    if (actualRange < NATRON_DOPESHEET_MIN_RANGE_FIT) {
        double diffRange = NATRON_DOPESHEET_MIN_RANGE_FIT - actualRange;
        diffRange /= 2;
        range.first -= diffRange;
        range.second += diffRange;
    }


    _imp->zoomContext.fill( range.first, range.second,
                            _imp->zoomContext.bottom(), _imp->zoomContext.top() );

    _imp->computeTimelinePositions();

    if (selectedKeyframesCount > 1) {
        _imp->computeSelectedKeysBRect();
    }

    redraw();
}

void
DopeSheetView::constantInterpSelectedKeyframes()
{
    running_in_main_thread();

    _imp->model->setSelectedKeysInterpolation(eKeyframeTypeConstant);
}

void
DopeSheetView::linearInterpSelectedKeyframes()
{
    running_in_main_thread();

    _imp->model->setSelectedKeysInterpolation(eKeyframeTypeLinear);
}

void
DopeSheetView::smoothInterpSelectedKeyframes()
{
    running_in_main_thread();

    _imp->model->setSelectedKeysInterpolation(eKeyframeTypeSmooth);
}

void
DopeSheetView::catmullRomInterpSelectedKeyframes()
{
    running_in_main_thread();

    _imp->model->setSelectedKeysInterpolation(eKeyframeTypeCatmullRom);
}

void
DopeSheetView::cubicInterpSelectedKeyframes()
{
    running_in_main_thread();

    _imp->model->setSelectedKeysInterpolation(eKeyframeTypeCubic);
}

void
DopeSheetView::horizontalInterpSelectedKeyframes()
{
    running_in_main_thread();

    _imp->model->setSelectedKeysInterpolation(eKeyframeTypeHorizontal);
}

void
DopeSheetView::breakInterpSelectedKeyframes()
{
    running_in_main_thread();

    _imp->model->setSelectedKeysInterpolation(eKeyframeTypeBroken);
}

void
DopeSheetView::copySelectedKeyframes()
{
    running_in_main_thread();

    _imp->model->copySelectedKeys();
}

void
DopeSheetView::pasteKeyframes(bool relative)
{
    running_in_main_thread();

    _imp->model->pasteKeys(relative);
}

void
DopeSheetView::onTimeLineFrameChanged(SequenceTime sTime,
                                      int reason)
{
    Q_UNUSED(sTime);
    Q_UNUSED(reason);

    running_in_main_thread();

    if ( _imp->gui->isGUIFrozen() ) {
        return;
    }

    _imp->computeTimelinePositions();

    if ( isVisible() ) {
        redraw();
    }
}

void
DopeSheetView::onTimeLineBoundariesChanged(int,
                                           int)
{
    running_in_main_thread();
    if ( isVisible() ) {
        redraw();
    }
}

void
DopeSheetView::onNodeAdded(const NodeAnimPtr& NodeAnim)
{
    AnimatedItemType nodeType = NodeAnim->getItemType();
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

/**
 * @brief DopeSheetView::initializeGL
 *
 *
 */
void
DopeSheetView::initializeGL()
{
    running_in_main_thread();
    appPTR->initializeOpenGLFunctionsOnce();

    if ( !appPTR->isOpenGLLoaded() ) {
        return;
    }

    _imp->generateKeyframeTextures();
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
        QMutexLocker k(&_imp->zoomContextMutex);
        _imp->zoomContext.setScreenSize(w, h);
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

    if (_imp->zoomContext.factor() <= 0) {
        return;
    }

    _imp->drawnOnce = true;
    
    double zoomLeft, zoomRight, zoomBottom, zoomTop;
    zoomLeft = _imp->zoomContext.left();
    zoomRight = _imp->zoomContext.right();
    zoomBottom = _imp->zoomContext.bottom();
    zoomTop = _imp->zoomContext.top();

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

    QPointF clickZoomCoords = _imp->zoomContext.toZoomCoordinates( e->x(), e->y() );

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
                    nodeClipRect.y2 = _imp->zoomContext.toZoomCoordinates( 0, treeItemRect.top() ).y();
                    nodeClipRect.y1 = _imp->zoomContext.toZoomCoordinates( 0, treeItemRect.bottom() ).y();

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

                    /*double keyframeTime = std::floor(_imp->zoomContext.toZoomCoordinates(e->pos().x(), 0).x() + 0.5);
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

    QPointF mouseZoomCoords = _imp->zoomContext.toZoomCoordinates( e->x(), e->y() );

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
        QPointF zoomCenter = _imp->zoomContext.toZoomCoordinates( _imp->lastPosOnMousePress.x(),
                                                                  _imp->lastPosOnMousePress.y() );

        // Alt + Wheel: zoom time only, keep point under mouse
        double par = _imp->zoomContext.aspectRatio() * scaleFactorX;
        if (par <= par_min) {
            par = par_min;
            scaleFactorX = par / _imp->zoomContext.aspectRatio();
        } else if (par > par_max) {
            par = par_max;
            scaleFactorX = par / _imp->zoomContext.factor();
        }
        _imp->zoomContext.zoomx(zoomCenter.x(), zoomCenter.y(), scaleFactorX);

        redraw();

        // Synchronize the dope sheet editor and opened viewers
        if ( _imp->gui->isTripleSyncEnabled() ) {
            _imp->updateCurveWidgetFrameRange();
            _imp->gui->centerOpenedViewersOn( _imp->zoomContext.left(), _imp->zoomContext.right() );
        }
    } else if ( buttonDownIsLeft(e) ) {
        _imp->onMouseLeftButtonDrag(e);
    } else if ( buttonDownIsMiddle(e) ) {
        double dx = _imp->zoomContext.toZoomCoordinates( _imp->lastPosOnMouseMove.x(),
                                                         _imp->lastPosOnMouseMove.y() ).x() - mouseZoomCoords.x();
        QMutexLocker k(&_imp->zoomContextMutex);
        _imp->zoomContext.translate(dx, 0);

        redraw();

        // Synchronize the curve editor and opened viewers
        if ( _imp->gui->isTripleSyncEnabled() ) {
            _imp->updateCurveWidgetFrameRange();
            _imp->gui->centerOpenedViewersOn( _imp->zoomContext.left(), _imp->zoomContext.right() );
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
            double keyframeTime = std::floor(_imp->zoomContext.toZoomCoordinates(e->pos().x(), 0).x() + 0.5);
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
    QPointF zoomCenter = _imp->zoomContext.toZoomCoordinates( e->x(), e->y() );

    _imp->zoomOrPannedSinceLastFit = true;

    par = _imp->zoomContext.aspectRatio() * scaleFactor;

    if (par <= par_min) {
        par = par_min;
        scaleFactor = par / _imp->zoomContext.aspectRatio();
    } else if (par > par_max) {
        par = par_max;
        scaleFactor = par / _imp->zoomContext.aspectRatio();
    }


    {
        QMutexLocker k(&_imp->zoomContextMutex);
        _imp->zoomContext.zoomx(zoomCenter.x(), zoomCenter.y(), scaleFactor);
    }
    _imp->computeSelectedKeysBRect();

    redraw();

    // Synchronize the curve editor and opened viewers
    if ( _imp->gui->isTripleSyncEnabled() ) {
        _imp->updateCurveWidgetFrameRange();
        _imp->gui->centerOpenedViewersOn( _imp->zoomContext.left(), _imp->zoomContext.right() );
    }
}

void
DopeSheetView::focusInEvent(QFocusEvent *e)
{
    QGLWidget::focusInEvent(e);
}

/**
 * @brief Get the current orthographic projection
 **/
void
DopeSheetView::getProjection(double *zoomLeft, double *zoomBottom, double *zoomFactor, double *zoomAspectRatio) const
{
    QMutexLocker k(&_imp->zoomContextMutex);
    *zoomLeft = _imp->zoomContext.left();
    *zoomBottom = _imp->zoomContext.bottom();
    *zoomFactor = _imp->zoomContext.factor();
    *zoomAspectRatio = _imp->zoomContext.aspectRatio();
}

/**
 * @brief Set the current orthographic projection
 **/
void
DopeSheetView::setProjection(double zoomLeft, double zoomBottom, double zoomFactor, double zoomAspectRatio)
{
    QMutexLocker k(&_imp->zoomContextMutex);
    _imp->zoomContext.setZoom(zoomLeft, zoomBottom, zoomFactor, zoomAspectRatio);
}


NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_DopeSheetView.cpp"
